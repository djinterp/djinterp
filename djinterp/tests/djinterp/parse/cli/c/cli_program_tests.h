/*******************************************************************************
* djinterp [parse]                                           cli_program_tests.h
*
* The suite of the command-line kit's program (cli_program.c): what making a
* program gives its parser; the endings a parse calls for, and the statuses
* they exit with; the commands a parse reaches, and their work; the `help`
* and `completion` commands; the standard options -- `-q`, `-v`, `--color`
* -- and the rules the grammar reads arguments by (R3, R7, R8); the parser's
* failures in the GNU shape; the reports a command's work makes; a program
* of one command; `/?`; which arguments are taken again from the wide
* command line on Windows; and every call given nothing.
*   Each case table runs a program the section's fixture makes, a fresh one
* for each case, and compares the status it exits with and all it writes:
* the words of every message are the kit's (GRAMMAR.md, section 8), and a
* change to one is a change to the grammar.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_program_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_C_CLI_PROGRAM_TESTS_H
#define DJINTERP_PARSE_CLI_C_CLI_PROGRAM_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../c/tests_section.h"     // d_tests_section


// 1.  MAKING
bool d_tests_cli_program_making(void);

// 2.  ENDINGS
bool d_tests_cli_program_endings(void);

// 3.  COMMANDS
bool d_tests_cli_program_commands(void);

// 4.  STANDARD COMMANDS
bool d_tests_cli_program_standard(void);

// 5.  STANDARD OPTIONS
bool d_tests_cli_program_options(void);

// 6.  FAILURES
bool d_tests_cli_program_failures(void);

// 7.  REPORTS
bool d_tests_cli_program_reports(void);

// 8.  ONE COMMAND
bool d_tests_cli_program_single(void);

// 9.  SLASH
bool d_tests_cli_program_slash(void);

// 10. ARGUMENTS
bool d_tests_cli_program_arguments(void);

// 11. NULLS
bool d_tests_cli_program_nulls(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_cli_program[];
extern const size_t                 d_tests_cli_program_count;


#endif  // DJINTERP_PARSE_CLI_C_CLI_PROGRAM_TESTS_H
