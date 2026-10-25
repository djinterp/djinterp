/*******************************************************************************
* djinterp [parse]                                       arg_validator_tests.hpp
*
* The argument parser's C++ validator suite: CLI11's validators, through
* the C++ face's cli::validator.
*   The C validator suites hold the C library's validators to CLI11 value
* by value; this one holds the face's way to them -- each of CLI11's
* validators made by the face's function of its name, over the C++ types it
* takes, as a value: described, named, set and combined with `&`, `|` and
* `!`, run over values directly and given to options -- their sections
* CLI11's own transcripts of the same definitions (the suite's CASES
* section, compiled against CLI11 by tools/cppref/cppref.py). Then the
* face's own: a validator as a value -- copied, moved, swapped and handed
* over -- the handle on one an option holds, filters as values, and where
* the face departs from CLI11, in sections of their own.
*   The suite is a table of sections, defined in arg_validator_tests.cpp
* beside the sections themselves; a section exists from the language level,
* and under the configuration, that its subject exists at, so the table's
* length is the number of sections this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_validator_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.21
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_VALIDATOR_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_VALIDATOR_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../c/tests_section.h"  // d_tests_section


// 1.  DEFAULT VALIDATORS
bool d_tests_arg_cpp_validator_defaults();
bool d_tests_arg_cpp_validator_settings();
bool d_tests_arg_cpp_validator_composed();
bool d_tests_arg_cpp_validator_in_a_parse();

// 2.  EXTRA VALIDATORS
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_validator_extra();
bool d_tests_arg_cpp_validator_member();
bool d_tests_arg_cpp_validator_transform();
bool d_tests_arg_cpp_validator_units();
#endif

// 3.  PATH VALIDATORS
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
bool d_tests_arg_cpp_validator_paths();
bool d_tests_arg_cpp_validator_path_sizes();
#endif

// 4.  WHAT C++11 BRINGS
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_validator_cpp11();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_validator_cpp11_extra();
#endif
#endif

// 5.  THE FACE'S OWN
bool d_tests_arg_cpp_validator_value();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_validator_value_extra();
#endif
bool d_tests_arg_cpp_validator_handles();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_validator_moved();
#endif
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_validator_filters();
#endif

// 6.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_validator_departures();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_validator_departures_extra();
#endif
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_validator_departures_cpp11();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_validator_departures_cpp11_extra();
#endif
#endif

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_validator[];
extern const std::size_t     d_tests_arg_cpp_validator_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_VALIDATOR_TESTS_HPP
