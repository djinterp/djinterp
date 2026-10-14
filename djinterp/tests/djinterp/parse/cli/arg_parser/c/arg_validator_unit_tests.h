/*******************************************************************************
* djinterp [parse]                                    arg_validator_unit_tests.h
*
* The suite of CLI11's numbers with units (arg_validator_extra.h, section 3):
* AsNumberWithUnit over integer and floating-point types, its options, and
* the units it is given -- none, one twice, in either case -- AsSizeValue in
* both of its bases, the two in composites, the one number written otherwise
* than CLI11, the units that cannot be made, what the two complete as, and
* the size check inside a parse.
*   Everything here is compiled only with the extra validators
* (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS); without them there is no suite,
* and the runner runs none.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_unit_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_UNIT_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_UNIT_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 1.  NUMBERS WITH UNITS
bool d_tests_arg_unit_integers(void);
bool d_tests_arg_unit_others(void);
bool d_tests_arg_unit_case(void);
bool d_tests_arg_unit_maps(void);

// 2.  SIZES
bool d_tests_arg_unit_sizes(void);

// 3.  COMPOSITES
bool d_tests_arg_unit_composites(void);

// 4.  DEPARTURES FROM CLI11
bool d_tests_arg_unit_departures(void);

// 5.  CONSTRUCTION AND COMPLETION
bool d_tests_arg_unit_construction(void);
bool d_tests_arg_unit_value_kinds(void);

// 6.  IN A PARSE
bool d_tests_arg_unit_in_a_parse(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_unit[];
extern const size_t                 d_tests_arg_unit_count;

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_UNIT_TESTS_H
