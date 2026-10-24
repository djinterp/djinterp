/*******************************************************************************
* djinterp [parse]                                            arg_parser_tests.h
*
* The suite of the argument parser itself (arg_parser.c): ending a parse,
* as CLI11's exit() ends one, from the root and from any command, into
* sinks given and into the parser's own; which failure message an error is
* written with; D_ARG_PARSE, CLI11_PARSE; parsing a vector of arguments and
* a command line held in one string; the root's name taken from the
* program's; what one parse leaves for the next; the endings given a
* status rather than an error; the endings and the parses given nothing to
* end or to parse; and `--name=` read as getopt_long reads it, where the
* parser is told to.
*   Every expected transcript is CLI11's, from the parser suite's reference
* program, but DEPARTED_FAILURE's, the module's own where it departs from
* CLI11; the statuses' and the nulls' sections', which no CLI11 call can be
* given; and the empty values', the module's own where its switch is on.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_parser_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.22
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSER_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSER_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  EXITS
bool d_tests_arg_parser_exits(void);

// 2.  COMMAND EXITS
bool d_tests_arg_parser_command_exits(void);

// 3.  OWN SINKS
bool d_tests_arg_parser_own_sinks(void);

// 4.  FAILURE LOOKUP
bool d_tests_arg_parser_failure_lookup(void);

// 5.  ENDINGS
bool d_tests_arg_parser_endings(void);

// 6.  MACRO
bool d_tests_arg_parser_macro(void);

// 7.  ARGUMENTS
bool d_tests_arg_parser_arguments(void);

// 8.  LINES
bool d_tests_arg_parser_lines(void);

// 9.  NAMES
bool d_tests_arg_parser_names(void);

// 10. STATE
bool d_tests_arg_parser_state(void);

// 11. STATUSES
bool d_tests_arg_parser_statuses(void);

// 12. NULLS
bool d_tests_arg_parser_nulls(void);

// 13. EMPTY VALUES
bool d_tests_arg_parser_empty_values(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_parser[];
extern const size_t                 d_tests_arg_parser_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSER_TESTS_H
