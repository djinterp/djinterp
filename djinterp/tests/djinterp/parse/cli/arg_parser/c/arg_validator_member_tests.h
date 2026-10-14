/*******************************************************************************
* djinterp [parse]                                  arg_validator_member_tests.h
*
* The suite of CLI11's sets and mappings (arg_validator_extra.h, section 2):
* IsMember, Transformer and CheckedTransformer over text and over types, with
* CLI11's built-in filters and with filters of the caller's, their contexts
* borrowed or owned; a type of the caller's compared by the text it writes;
* the choices each lists for shell completion; the sets that cannot be made;
* and the three inside a parse.
*   Everything here is compiled only with the extra validators
* (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS); without them there is no suite,
* and the runner runs none.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_validator_member_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_MEMBER_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_MEMBER_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knob
#include "../../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 1.  SETS
bool d_tests_arg_member_text(void);
bool d_tests_arg_member_filters(void);
bool d_tests_arg_member_typed(void);
bool d_tests_arg_member_duplicates(void);

// 2.  MAPPINGS
bool d_tests_arg_member_transformer(void);
bool d_tests_arg_member_checked(void);

// 3.  COMPOSITES
bool d_tests_arg_member_composites(void);

// 4.  FILTERS OF THE CALLER'S
bool d_tests_arg_member_filter_function(void);
bool d_tests_arg_member_filter_context(void);

// 5.  TYPES OF THE CALLER'S
bool d_tests_arg_member_caller_type(void);

// 6.  CHOICES AND CONSTRUCTION
bool d_tests_arg_member_choices(void);
bool d_tests_arg_member_construction(void);

// 7.  IN A PARSE
bool d_tests_arg_member_in_a_parse(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_member[];
extern const size_t                 d_tests_arg_member_count;

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_VALIDATOR_MEMBER_TESTS_H
