/*******************************************************************************
* djinterp [tools]                                               djinterp_main.c
*
* The djinterp program's main: its parser, its program
* (djinterp_commands.h), and the kit's run of it.
*
*
* path:      /src/djinterp/tools/djinterp/djinterp_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
// std
#include <stdio.h>   // fputs, stderr
#include <string.h>  // memset
// djinterp
#include "../../../../inc/djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // the parser
#include "../../../../inc/djinterp/tools/cli/c/cli_program.h"  // the program
#include "./djinterp_commands.h"  // d_djinterp_program_make


/*
main
  The program made over its parser and run; a parser that could not be made
said, as the kit says the want of memory.
*/
int main(
    int    _argc,
    char** _argv
)
{
    struct dj_arg_parser  parser;
    struct dj_cli_program program;
    int                   status;

    memset(&program, 0, sizeof(program));

    if (d_arg_parser_init(&parser,
                          "the djinterp language's command line: its djins "
                          "and their settings",
                          "djinterp") != D_ARG_OK)
    {
        (void)fputs("djinterp: error: out of memory\n", stderr);

        return D_CLI_EXIT_TROUBLE;
    }

    // a program whose making failed says why when it runs
    (void)d_djinterp_program_make(&program, &parser);
    status = d_cli_program_run(&program, _argc,
                               (const char* const*)_argv);

    d_cli_program_release(&program);
    d_arg_parser_release(&parser);

    return status;
}
