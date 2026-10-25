/*******************************************************************************
* djinterp [parse]                                            arg_argv_tests.hpp
*
* The argument parser's C++ argv suite: arguments from text, through the C++
* face's functions over the standard library's strings.
*   The C argv suite holds the C library's splitting, quoting, response
* files and conversions to their references rule by rule; this one holds
* the face's way to them: lines split by CLI11's rules, a program's name
* found at the start of a line, and wide text made UTF-8 and UTF-8 made
* wide -- their sections CLI11's own transcripts of the same calls (the
* suite's CASES section, compiled against CLI11 by tools/cppref/cppref.py).
* Then the face's own: the other rules and the switches, words quoted so
* that they split back whole, response files, the status form of widen(),
* and where the face departs from CLI11.
*   The suite is a table of sections, defined in arg_argv_tests.cpp beside
* the sections themselves; a section exists in the builds its subject
* exists in, so the table's length is the number of sections this build
* has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_argv_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_ARGV_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_ARGV_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../c/tests_section.h"  // d_tests_section


// 1.  CLI11'S SPLITTING AND ENCODING
bool d_tests_arg_cpp_argv_split();
bool d_tests_arg_cpp_argv_program();
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
bool d_tests_arg_cpp_argv_encoding();
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// 2.  THE FACE'S OWN
bool d_tests_arg_cpp_argv_rules();
bool d_tests_arg_cpp_argv_quoted();
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
bool d_tests_arg_cpp_argv_response();
#endif  // D_INTERNAL_ARG_PARSER_RESPONSE_FILES
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
bool d_tests_arg_cpp_argv_wide();

// 3.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_argv_departures();
#endif  // D_INTERNAL_ARG_PARSER_WIDE_ARGV

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_argv[];
extern const std::size_t     d_tests_arg_cpp_argv_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_ARGV_TESTS_HPP
