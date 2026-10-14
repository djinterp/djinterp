/*******************************************************************************
* djinterp [parse]                                              arg_type_tests.h
*
* The suite of the argument parser's value types (arg_type.h).
*   The built-in descriptors and what they describe; the readers of integers
* under both rules, of floating point in any locale, of truth values and of
* flag values; every built-in type's conversion at its limits, from the empty
* text, and when a value does not convert; the text each type writes; a
* descriptor of the caller's own; and the sums the sum policy makes.
*   The suite is a table of sections, defined in arg_type_tests.c beside the
* sections themselves, so its length is the number of sections this build
* has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_type_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TYPE_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TYPE_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  TYPE DESCRIPTORS
bool d_tests_arg_type_builtin_descriptors(void);

// 2.1  Integers
bool d_tests_arg_type_long_c_rule(void);
bool d_tests_arg_type_long_decimal_rule(void);
bool d_tests_arg_type_ulong(void);
bool d_tests_arg_type_intmax(void);
bool d_tests_arg_type_integer_widths(void);
bool d_tests_arg_type_characters(void);

// 2.2  Floating point
bool d_tests_arg_type_floating(void);
bool d_tests_arg_type_floating_special(void);
bool d_tests_arg_type_floating_locale(void);

// 2.3  Truth values
bool d_tests_arg_type_bool(void);
bool d_tests_arg_type_flag_value(void);

// 2.4  Through a descriptor
bool d_tests_arg_type_empty_is_zero(void);
bool d_tests_arg_type_failure_keeps_target(void);
bool d_tests_arg_type_text_types(void);
bool d_tests_arg_type_format(void);
bool d_tests_arg_type_arguments(void);
bool d_tests_arg_type_descriptor_arguments(void);
bool d_tests_arg_type_custom_descriptor(void);

// 2.5  Sums
bool d_tests_arg_type_sums(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_type[];
extern const size_t                 d_tests_arg_type_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_TYPE_TESTS_H
