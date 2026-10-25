/*******************************************************************************
* djinterp [parse]                                            arg_option_tests.h
*
* The suite of the argument parser's options (arg_option.h): what a
* definition string makes of names, and the names a command refuses; how a
* name is matched, with case and underscores ignored or not, and the names
* two options share; an option's text and type name, its counts, defaults
* -- given as text or as typed values -- relations and validators; how a
* value is split, how a multi-option policy reduces what was given, and how
* the values read as one text or as a list; the value a flag name stands
* for; the callback and a bound variable; and what an option says about its
* values.
*   Every expected transcript is CLI11's, from the option suite's reference
* program, but where a constant says it departs.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_option_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.17
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_OPTION_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_OPTION_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  NAMES
bool d_tests_arg_option_names(void);
bool d_tests_arg_option_flag_names(void);
bool d_tests_arg_option_taken(void);

// 2.  MATCHING
bool d_tests_arg_option_matching(void);
bool d_tests_arg_option_conflicts(void);

// 3.  DEFINITION
bool d_tests_arg_option_text(void);
bool d_tests_arg_option_counts(void);
bool d_tests_arg_option_defaults(void);
bool d_tests_arg_option_default_typed(void);
bool d_tests_arg_option_relations(void);
bool d_tests_arg_option_validators(void);

// 4.  VALUES
bool d_tests_arg_option_configurations(void);
bool d_tests_arg_option_as(void);
bool d_tests_arg_option_flag_values(void);

// 5.  CALLBACKS AND BINDING
bool d_tests_arg_option_callbacks(void);
bool d_tests_arg_option_binding(void);

// 6.  RESULTS
bool d_tests_arg_option_results(void);

// 7.  DEPARTURES FROM CLI11
bool d_tests_arg_option_departures(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_option[];
extern const size_t                 d_tests_arg_option_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_OPTION_TESTS_H
