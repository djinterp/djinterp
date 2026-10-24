/*******************************************************************************
* bitwisedom [tools]                                          bitwisedom_gen.cpp
*
* bitwisedom-gen: a sheet to a header, as build scripts have always called
* it.
*   `bitwisedom-gen ARGUMENTS` is `bitwisedom gen ARGUMENTS` for the arguments
* bitwisedom-gen took, with the same exit status: 0 when it did what it was
* asked; 1 when the sheet has errors (the header is still written, without
* the symbols in error); 2 for a usage, file or format problem. Its other
* two jobs keep their spellings: `--list-examples` is `bitwisedom examples`,
* and `--save-example NAME FILE` is `bitwisedom convert example:NAME -o
* FILE`.
*
*     bitwisedom-gen [OPTION]... [SHEET]
*       SHEET                 a sheet file, a bitfield or an export; - for
*                             standard input; or --example NAME
*       -o, --out=FILE        the header (default: standard output)
*       --self-test=FILE      the header's compile-time self-test
*       --bitfield=FILE       the sheet as a bitfield (JSON)
*       --export=FILE         everything about it (JSON)
*       --lang=c|cpp          --form=macros|constants
*       --level=c99|...|cpp23 --style=djinterp|plain
*       --word=8|16|32|64     a wide sheet's array words, in bits
*       --date=YYYY.MM.DD     the banner's date (default: today's)
*       --check               report the sheet's issues; write nothing
*       --list-examples       --save-example NAME FILE
*
*   Its work is gen's (cli.hpp), so the two cannot drift apart; the
* self-test it writes is the model's (selftest.hpp), which CTest compiles at
* every language level, and runs for a wide sheet: how the generator is
* proved, not just run.
*
* path:      /src/djinterp/tools/bitwisedom/tools/bitwisedom_gen.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
*                                                            revised: 2026.10.22
*******************************************************************************/
// bitwisedom
#include <djinterp/tools/bitwisedom/cli.hpp>  // run_gen_front


int
main(
    int    _argc,
    char** _argv
)
{
    return bitwisedom::run_gen_front(_argc,
                                     _argv);
}
