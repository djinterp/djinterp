/*******************************************************************************
* djinterp [parse]                                         arg_validator_tests.h
*
* The suite of the argument parser's validators (arg_validator.h).
*   A value under validation; the validator record, its copies and its
* settings; running one, describing one and listing what it accepts; the
* composites CLI11 writes with `&`, `|` and `!`; validators of the caller's
* own; and CLI11's default validators -- ranges, the number signs, escaped
* strings -- run over the values CLI11's were run over, with CLI11's
* verdicts, messages and descriptions.
*   The suite is a table of sections, defined in arg_validator_tests.c beside
* the sections themselves, so its length is the number of sections this
* build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  VALIDATION
bool d_tests_arg_validator_validation(void);

// 2.  VALIDATORS
bool d_tests_arg_validator_defaults(void);
bool d_tests_arg_validator_copy(void);
bool d_tests_arg_validator_settings(void);
bool d_tests_arg_validator_choices(void);
bool d_tests_arg_validator_value_kinds(void);

// 3.  COMPOSITION
bool d_tests_arg_validator_and(void);
bool d_tests_arg_validator_or(void);
bool d_tests_arg_validator_not(void);
bool d_tests_arg_validator_not_or(void);
bool d_tests_arg_validator_escaped_and(void);
bool d_tests_arg_validator_inactive(void);
bool d_tests_arg_validator_half_active(void);
bool d_tests_arg_validator_error_stops(void);
bool d_tests_arg_validator_failed(void);

// 4.  CUSTOM VALIDATORS
bool d_tests_arg_validator_custom(void);
bool d_tests_arg_validator_owned_context(void);
bool d_tests_arg_validator_described(void);

// 5.  DEFAULT VALIDATORS
bool d_tests_arg_validator_range_int(void);
bool d_tests_arg_validator_range_uint(void);
bool d_tests_arg_validator_range_double(void);
bool d_tests_arg_validator_range_char(void);
bool d_tests_arg_validator_positive(void);
bool d_tests_arg_validator_non_negative(void);
bool d_tests_arg_validator_escaped(void);
bool d_tests_arg_validator_non_modifying(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_validator[];
extern const size_t                 d_tests_arg_validator_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_TESTS_H
