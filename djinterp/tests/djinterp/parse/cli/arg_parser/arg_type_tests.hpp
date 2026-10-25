/*******************************************************************************
* djinterp [parse]                                            arg_type_tests.hpp
*
* The argument parser's C++ type suite: C++ types as option values, through
* the C++ face's type_traits.
*   The C type suite holds the C library's descriptors and conversions to
* CLI11 value by value; this one holds the face's choice of them -- every
* kind of type CLI11 binds, each converted from what a parse gives and
* written as help and defaults show it: the built-in numbers, truth values
* and characters, text, the standard containers and maps, pairs, tuples,
* arrays and complex numbers, enumerations, types made from text or
* numbers, streamed types, optional values, string views, paths and wide
* text -- their sections CLI11's own transcripts of the same definitions
* (the suite's CASES section, compiled against CLI11 by
* tools/cppref/cppref.py). Then the face's own: its descriptors, convert and
* to_text called directly, traits of the caller's own, and where the face
* writes or reads a value otherwise than CLI11, in sections of their own.
*   The suite is a table of sections, defined in arg_type_tests.cpp beside
* the sections themselves; a section exists from the language level, and
* under the configuration, that its subject exists at, so the table's length
* is the number of sections this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_type_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TYPE_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TYPE_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../c/tests_section.h"  // d_tests_section


// 1.  BUILT-IN TYPES
bool d_tests_arg_cpp_type_numbers();
#if D_ENV_HAS_LONG_LONG
bool d_tests_arg_cpp_type_numbers_long_long();
#endif
bool d_tests_arg_cpp_type_texts();

// 2.  COMPOSED TYPES
bool d_tests_arg_cpp_type_containers();
bool d_tests_arg_cpp_type_compound();
bool d_tests_arg_cpp_type_defaults();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_type_kinds_cpp11();
#endif
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
bool d_tests_arg_cpp_type_kinds_cpp17();
#endif
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
bool d_tests_arg_cpp_type_wide_text();
#endif

// 3.  THE TRAITS THEMSELVES
bool d_tests_arg_cpp_type_names();
bool d_tests_arg_cpp_type_convert();
bool d_tests_arg_cpp_type_caller_traits();

// 4.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_type_departures();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_type_departures_cpp11();
#endif
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
bool d_tests_arg_cpp_type_departures_cpp17();
#endif
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
bool d_tests_arg_cpp_type_departures_wide();
#endif

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_type[];
extern const std::size_t     d_tests_arg_cpp_type_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_TYPE_TESTS_HPP
