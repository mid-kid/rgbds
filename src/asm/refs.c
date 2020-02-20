#include <stdio.h>
#include <string.h>

#include "helpers.h"
#include "asm/warning.h"
#include "asm/util.h"
#include "asm/refs.h"
#include "asm/main.h"
#include "asm/asm.h"

struct file {
	struct file *next;
	int symbols_len;
	int symbols_arrlen;
	struct sSymbol **symbols;
	unsigned used:1;
	unsigned in_section:1;
	unsigned has_charmap:1;
	char name[];
};
static struct file *files[HASHSIZE];

static void **ptrarr_init(void ***arr, int *arrlen, int *len)
{
	*len = 0;
	*arrlen = 0x100;
	*arr = malloc(*arrlen * sizeof(void *));
	if (!*arr) fatalerror("No memory for array");
	return *arr;
}

static void **ptrarr_append(void ***arr, int *arrlen, int *len, void *item)
{
	if (++(*len) >= *arrlen) {
		*arrlen *= 2;
		*arr = realloc(*arr, *arrlen * sizeof(void *));
		if (!*arr) fatalerror("No memory for array");
	}
	(*arr)[*len - 1] = item;
	return *arr;
}

static void **ptrarr_find(void **arr, int len, void *needle)
{
	for (void **item = arr; item < arr + len; item++) {
		if (*item == needle) return item;
	}
	return NULL;
}

static void **ptrarr_add(void ***arr, int *arrlen, int *len, void *item)
{
	if (ptrarr_find(*arr, *len, item)) return *arr;
	return ptrarr_append(arr, arrlen, len, item);
}

static struct file **file_find(char *filename)
{
	struct file **file = &(files[calchash(filename) % HASHSIZE]);
	while (*file != NULL) {
		if (strcmp((*file)->name, filename) == 0) break;
		file = &((*file)->next);
	}
	return file;
}

static struct file *file_add(char *filename)
{
	struct file **file = file_find(filename);
	if (*file != NULL) return *file;

	(*file) = malloc(sizeof(struct file) + strlen(filename) + 1);
	if (!*file) fatalerror("No memory for filename");
	(*file)->next = NULL;
	strcpy((*file)->name, filename);
	ptrarr_init((void ***)&(*file)->symbols, &(*file)->symbols_arrlen, &(*file)->symbols_len);
	(*file)->used = 0;
	(*file)->in_section = 0;
	(*file)->has_charmap = 0;
	return *file;
}

static struct file *file_addref(char *filename, struct sSymbol *symbol)
{
	struct file *file = file_add(filename);
	ptrarr_add((void ***)&file->symbols, &file->symbols_arrlen, &file->symbols_len, symbol);
	return file;
}

static void file_usefile(char *filename)
{
	struct file *file = file_add(filename);
	if (file->used) return;
	file->used = 1;

	for (struct sSymbol **symbol = file->symbols;
			symbol < file->symbols + file->symbols_len;
			symbol++) {
		file_usefile((*symbol)->tzFileName);
	}

	puts(file->name);
}

static void putsym(struct sSymbol *symbol)
{
	fprintf(stderr, "%s (%s):", symbol->tzName, symbol->tzFileName);
	if (symbol->type & SYM_LABEL) fprintf(stderr, " LABEL");
	if (symbol->type & SYM_EQU) fprintf(stderr, " EQU");
	if (symbol->type & SYM_SET) fprintf(stderr, " SET");
	if (symbol->type & SYM_MACRO) fprintf(stderr, " MACRO");
	if (symbol->type & SYM_EQUS) fprintf(stderr, " EQUS");
	if (symbol->type & SYM_REF) fprintf(stderr, " REF");
	putc('\n', stderr);
}

struct sSymbol *refs_usesymbol(struct sSymbol *symbol)
{
	if (CurrentOptions.mode != RUN_MODE_USEDINC) return symbol;

	// Reason for not treating SET symbols as used:
	// They can be created by any macro anywhere, and as such aren't bound
	//   to always be "first defined" by a certain file.
	// Because some people use macros to define structures and whatnot,
	//   we shouldn't just ignore all symbols created with macros either.
	// Please use EQU to have a file "claim" ownership of a certain symbol.

	// It's safe to say that a symbol that doesn't have SYM_SET
	//   at the time of use will never get it since that's a hard error.
	if (symbol && symbol != pPCSymbol
			&& !(symbol->type == SYM_SET)) {
		symbol->isUsed = 1;
		struct file *file = file_addref(tzCurrentFileName, symbol);
		if (pCurrentSection) file->in_section = 1;
	}
	return symbol;
}

// TODO: Actually check which characters from which file are used.
void refs_addcharmap(void) {
	struct file *file = file_add(tzCurrentFileName);
	file->has_charmap = 1;
}
void refs_usecharmap(void) {
	for (struct file **hashfile = files;
			hashfile < files + HASHSIZE; hashfile++) {
		struct file *file = *hashfile;
		while (file) {
			// HACK: Includes every file that has a charmap
			if (file->has_charmap)
				file->in_section = 1;
			file = file->next;
		}
	}
}

void refs_writeusedfiles(char *tzMainfile)
{
	// Report on used symbols
	if (CurrentOptions.verbose) {
		for (struct sSymbol **hashsym = tHashedSymbols;
				hashsym < tHashedSymbols + HASHSIZE; hashsym++) {
			struct sSymbol *symbol = *hashsym;
			while (symbol) {
				if (!(symbol->isUsed)) {
					symbol = symbol->pNext;
					continue;
				}

				putsym(symbol);
				symbol = symbol->pNext;
			}
		}
	}

	// Report on all file references
	if (CurrentOptions.verbose) {
		for (struct file **hashfile = files;
				hashfile < files + HASHSIZE; hashfile++) {
			struct file *file = *hashfile;
			while (file) {
				for (struct sSymbol **symbol = file->symbols;
						symbol < file->symbols + file->symbols_len;
						symbol++) {
					fprintf(stderr, "%s -> %s (%s)\n", file->name, (*symbol)->tzFileName, (*symbol)->tzName);
				}
				file = file->next;
			}
		}
	}

	// Figure out which files are used from files that have sections
	for (struct file **hashfile = files;
			hashfile < files + HASHSIZE; hashfile++) {
		struct file *file = *hashfile;
		while (file) {
			if (file->in_section) file_usefile(file->name);
			file = file->next;
		}
	}

	// Just... to be sure
	file_usefile(tzMainfile);
}
