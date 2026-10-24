/*******************************************************************************
* bitwisedom [tools]                                         bitwisedom_main.cpp
*
* bitwisedom: the command line's main.
*   Everything is run_cli's (cli.hpp): the commands that read a sheet, those
* that make or change one, and `gui`, which loads the window's module.
*
* path:      /src/djinterp/tools/bitwisedom/tools/bitwisedom_main.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.22
*                                                            revised: 2026.10.22
*******************************************************************************/
// bitwisedom
#include <djinterp/tools/bitwisedom/cli.hpp>  // run_cli


int
main(
    int    _argc,
    char** _argv
)
{
    return bitwisedom::run_cli(_argc,
                               _argv);
}
