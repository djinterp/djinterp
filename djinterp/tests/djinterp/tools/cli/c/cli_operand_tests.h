/*******************************************************************************
* djinterp [c][tools]                                        cli_operand_tests.h
*
* The suite of the command-line kit's operands that name something other
* than a file (cli_operand.c): which words are named operands of a prefix
* and which are files' names; the warning where a file has a named
* operand's name, its absence under `-q`, for a directory and for a link
* that leads nowhere; and every call given nothing, or a prefix R13 does not
* allow.
*   As the program suite's, each case table runs a program the section's
* fixture makes, a fresh one for each case, and compares the status it
* exits with and all it writes.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_operand_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.25
*                                                            revised: 2026.10.25
*******************************************************************************/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_OPERAND_TESTS_H
#define DJINTERP_TOOLS_CLI_C_CLI_OPERAND_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // specs


// 1.  NAMED OPERANDS
bool d_tests_cli_operand_named(void);

// 2.  NULLS
bool d_tests_cli_operand_nulls(void);

// the suite's sections, in the order they run, and the block they make
extern const struct dj_test_case_spec  d_tests_cli_operand[];
extern const struct dj_test_block_spec d_tests_cli_operand_block;


#endif  // DJINTERP_TOOLS_CLI_C_CLI_OPERAND_TESTS_H
