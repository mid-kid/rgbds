#ifndef RGBDS_REFS_H
#define RGBDS_REFS_H

#include "asm/symbol.h"

struct sSymbol *refs_usesymbol(struct sSymbol *symbol);
void refs_addcharmap(void);
void refs_usecharmap(void);
void refs_writeusedfiles(char *tzMainfile);

#endif
