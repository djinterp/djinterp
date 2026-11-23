/*******************************************************************************
* djinterp [c][tools]                                         cli_format_tests.h
*
* The suite of the command-line kit's help (cli_format.c): the page laid out
* as GNU programs lay it out -- the usage line, the description as a
* sentence, then the positionals, the commands by group, the options by
* group, the option groups and the options a subcommand takes from above,
* and the footer; each entry's left column, its names, its value and
* whether the value may be left out; the right column chosen, and a name
* past it; text wrapped to the page, in code points; what help hides; the
* usage line a command writes for itself; a page at other widths; the
* formatter without a program; the modes the kit leaves to the parser; the
* width help takes on a terminal and off one; and the description as JSON,
* `help --format=json` and the writer it is written with.
*
* HOW TO USE
* ----------
*   The kit's C suite runs each section through d_tests_cli_format_block;
* each section can be run alone, as a dj_test_case_spec's function.
*
* OPTIONS
* -------
*   None: a section takes no arguments.
*
* CONFIGURATION
* -------------
*   D_CFG_CLI_FORMATTER 0 leaves help the parser's, and the sections on the
* kit's layout out; the width help takes and the description run in every
* build.
*
*
* path:      /tests/djinterp/tools/cli/c/cli_format_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.11.22
*******************************************************************************/

#ifndef DJINTERP_TOOLS_CLI_C_CLI_FORMAT_TESTS_H
#define DJINTERP_TOOLS_CLI_C_CLI_FORMAT_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // specs


// 1.  PAGES
bool d_tests_cli_format_pages(void);

// 2.  WIDTHS
bool d_tests_cli_format_widths(void);

// 3.  WITHOUT A PROGRAM
bool d_tests_cli_format_bare(void);

// 4.  MODES
bool d_tests_cli_format_modes(void);

// 5.  THE WIDTH HELP TAKES
bool d_tests_cli_format_help_width(void);

// 6.  AS JSON
bool d_tests_cli_format_json(void);

// the suite's sections, in the order they run, and the block they make
extern const struct dj_test_case_spec  d_tests_cli_format[];
extern const struct dj_test_block_spec d_tests_cli_format_block;


#endif  // DJINTERP_TOOLS_CLI_C_CLI_FORMAT_TESTS_H
