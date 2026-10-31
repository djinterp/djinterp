/*******************************************************************************
* djinterp [tools]                                              cli_edit_tests.h
*
* The suite of the command-line kit's commands that change a file
* (cli_edit.c): the operand and the options each rule gives a command, and
* their help; where each command line sends the result, under A, the
* default, and B, D_CLI_PROGRAM_EDITS_TO_OUTPUT; a file written back, one
* written elsewhere, standard output, a dry run, standard input, and writes
* that fail, the file left as it was; and every call given nothing.
*   As the program suite's, each case table runs a program the section's
* fixture makes, a fresh one for each case, and compares the status it
* exits with and all it writes; a write's case also compares the files it
* leaves.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_edit_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_EDIT_TESTS_H
#define DJINTERP_TOOLS_CLI_C_CLI_EDIT_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../../c/modules/parse/tests_section.h"  // dj_tests_section


// 1.  HELP
bool d_tests_cli_edit_help(void);

// 2.  WHERE IT GOES
bool d_tests_cli_edit_where(void);

// 3.  WRITING
bool d_tests_cli_edit_writing(void);

// 4.  NULLS
bool d_tests_cli_edit_nulls(void);

// the suite's sections, in the order they run
extern const struct dj_tests_section d_tests_cli_edit[];
extern const size_t                 d_tests_cli_edit_count;


#endif  // DJINTERP_TOOLS_CLI_C_CLI_EDIT_TESTS_H
