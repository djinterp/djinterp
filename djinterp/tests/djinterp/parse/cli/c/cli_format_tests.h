/*******************************************************************************
* djinterp [parse]                                            cli_format_tests.h
*
* The suite of the command-line kit's help (cli_format.c): the page laid out
* as GNU programs lay it out -- the usage line, the description as a
* sentence, then the positionals, the commands by group, the options by
* group, the option groups and the options a subcommand takes from above,
* and the footer; each entry's left column, its names, its value and
* whether the value may be left out; the right column chosen, and a name
* past it; text wrapped to the page, in code points; what help hides; the
* usage line a command writes for itself; a page at other widths; the
* formatter without a program; the modes the kit leaves to the parser; and
* the width help takes on a terminal and off one.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_format_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_C_CLI_FORMAT_TESTS_H
#define DJINTERP_PARSE_CLI_C_CLI_FORMAT_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../c/tests_section.h"     // d_tests_section


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

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_cli_format[];
extern const size_t                 d_tests_cli_format_count;


#endif  // DJINTERP_PARSE_CLI_C_CLI_FORMAT_TESTS_H
