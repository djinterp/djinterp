/*******************************************************************************
* djinterp [parse]                                   arg_validator_extra_tests.h
*
* The suite of CLI11's extra checks (arg_validator_extra.h, section 1): the
* type validators and Number, over the built-in types and one of the
* caller's; the bounds; ValidIPV4; composites of them; the values the module
* writes otherwise than CLI11; the checks that cannot be made; what each
* completes as; and the checks run inside a parse. The sets and mappings are
* in arg_validator_member_tests.h, and the numbers with units in
* arg_validator_unit_tests.h.
*   Everything here is compiled only with D_INTERNAL_ARG_PARSER_EXTRA_
* VALIDATORS; without it there is no suite, and the runner runs none.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_extra_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_EXTRA_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_EXTRA_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 1.  TYPES
bool d_tests_arg_extra_type_integers(void);
bool d_tests_arg_extra_type_others(void);
bool d_tests_arg_extra_type_custom(void);

// 2.  BOUNDS
bool d_tests_arg_extra_bound_integers(void);
bool d_tests_arg_extra_bound_others(void);

// 3.  ADDRESSES
bool d_tests_arg_extra_ipv4(void);

// 4.  COMPOSITES
bool d_tests_arg_extra_composites(void);

// 5.  DEPARTURES FROM CLI11
bool d_tests_arg_extra_departures(void);

// 6.  CONSTRUCTION AND COMPLETION
bool d_tests_arg_extra_construction(void);
bool d_tests_arg_extra_value_kinds(void);

// 7.  IN A PARSE
bool d_tests_arg_extra_in_a_parse(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_extra[];
extern const size_t                 d_tests_arg_extra_count;

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_EXTRA_TESTS_H
