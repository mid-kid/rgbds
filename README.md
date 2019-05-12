RGBDS-analysis
==============

This is a fork of [RGBDS](https://github.com/rednex/rgbds) that implements a suite of static analysis tools for RGBDS-based codebases.  
Since it's a fork, it's fully RGBDS-compatible, and it should behave exactly like the original. This fork will be kept up-to-date and benefit from any future features and improvements introduced in RGBDS.  
While this fork can still be used to assemble a correct gameboy ROM, it isn't in any way meant to be a replacement for `rgbds`. Please use the upstream codebase to assemble your code.

It should be noted that not all tools work completely correctly at the time of writing, though they work well enough for the purposes for which they've been used thus far. Take the output generated with a grain of salt and be ready to do some troubleshooting.


How to use
----------

The different tools in RGBDS-analysis are implemented through the `-m` command-line switch to the `rgbasm` program. Passing a valid "mode" to this switch will have it perform a certain operation on your source file and output on the standard output.


### `unusedinc`

"include-what-you-use"

TODO


### `usedinc`

Lists all (recursively) `INCLUDE`d files from a certain source file that are actually used in the process of creating the resulting binary, excluding files that aren't.

Used with the `-v` (verbose) option, this tool will output additional information about used symbols and what files are referenced through which used symbols to standard error.

Caveats:
- Uses of symbols defined through `SET` or `=` won't cause a file to become "used", since they don't necessarily belong to a single file.
- If any charmap character is used, all files that use the `charmap` directive will be considered as used, even if the characters it defines are unused.


### `uuset`

"useless use of SET"

TODO


Architectural improvements
--------------------------

The codebase, as it is right now, is nothing more than a hack on top of rgbds that hooks into a few of its core functions, to get some things done I wanted it for, and is as such rather messy. It has a lot of room for improvement.

For example, an output file is still generated when running any of the utilities, even if never actually written to a file, and you have to modify some of the behavior to accomplish some things. There is also not really a good way to have a dedicated CLI for each tool right now, or somehow else separate each tool.

It'd be nice to make `rgbasm` into a library, on top of which every tool can be implemented. Maybe generate bindings for some languages (e.g. python), to make development of analysis tools easier. However, doing this would require us to free memory properly (such that a ton of files can be parsed in a single run without exhausting memory), and somehow make a stable API for the parser.

This isn't something I'm going to do anytime soon, however, due to how much of a refactor that'd require, but if anyone's willing to help improve the state of this codebase (and upstream RGBDS), this'd be a welcome improvement.
