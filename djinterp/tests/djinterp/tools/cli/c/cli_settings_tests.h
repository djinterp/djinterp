/*******************************************************************************
* djinterp [tools]                                          cli_settings_tests.h
*
* The suite of the command-line kit's settings (cli_settings.c): the
* settings a command line gives taken off it in each of their forms, and
* nothing else -- never past `--`, never an option's value; those it names
* badly, each a usage error; the user's settings file, its failing
* statements said by their lines and the rest read on; the environment under
* both prefixes, the program's winning; and what the kit reads, each source
* standing where the pattern puts it.
*   Each case table runs a program the section's fixture makes, a fresh one
* for each case, the run's settings let go of between them, and compares the
* status it exits with and all it writes. The cases run with the user's
* settings in a scratch directory of their own, so that a real one is never
* read.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_settings_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TAKEN
2.  REFUSED
3.  THE USER'S FILE
4.  THE ENVIRONMENT
5.  WHAT THE KIT READS
*/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_SETTINGS_TESTS_H
#define DJINTERP_TOOLS_CLI_C_CLI_SETTINGS_TESTS_H 1

// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
// djinterp
#include "../../../c/modules/parse/tests_section.h"  // dj_tests_section


// 1.  TAKEN
bool d_tests_cli_settings_taken(void);
// 2.  REFUSED
bool d_tests_cli_settings_refused(void);
// 3.  THE USER'S FILE
bool d_tests_cli_settings_file(void);
// 4.  THE ENVIRONMENT
bool d_tests_cli_settings_environment(void);
// 5.  WHAT THE KIT READS
bool d_tests_cli_settings_reads(void);

// the suite's sections, in the order they run; none where the option core
// is not built, or outside testing (D_TESTING)
extern const struct dj_tests_section d_tests_cli_settings[];
extern const size_t                 d_tests_cli_settings_count;


#endif  // DJINTERP_TOOLS_CLI_C_CLI_SETTINGS_TESTS_H
