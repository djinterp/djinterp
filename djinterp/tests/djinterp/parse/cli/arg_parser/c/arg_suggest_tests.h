/*******************************************************************************
* djinterp [parse]                                           arg_suggest_tests.h
*
* The suite of the parser's suggestions (arg_suggest.c), the line "Did you
* mean '<name>'?" the message for arguments a parse did not expect gains
* for each one: an option's name mistyped in each form, and compared as the
* option compares names; a subcommand's and an alias; how near a name must
* be; and which command reports, and the names it gathers. And the edit
* distance those are measured by, held to the whole table.
*   CLI11 has no suggestions (ISSUES 21). Each section of cases is run
* twice: with them, against the reference's own working out of the rules
* over the App CLI11 parsed, and without them, against CLI11's message.
* Where suggestions are not built, both passes write CLI11's, and the
* distance is not there to measure.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_suggest_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SUGGEST_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SUGGEST_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  OPTIONS
bool d_tests_arg_suggest_options(void);

// 2.  SUBCOMMANDS
bool d_tests_arg_suggest_commands(void);

// 3.  DISTANCES
bool d_tests_arg_suggest_distances(void);

// 4.  WHERE A NAME IS LOOKED FOR
bool d_tests_arg_suggest_reach(void);

#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)

// 5.  THE EDIT DISTANCE
bool d_tests_arg_suggest_edit_distance(void);

#endif  // D_INTERNAL_ARG_PARSER_SUGGESTIONS

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_suggest[];
extern const size_t                 d_tests_arg_suggest_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SUGGEST_TESTS_H
