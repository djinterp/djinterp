/*******************************************************************************
* djinterp [parse]                                          arg_parser_tests.hpp
*
* The argument parser's C++ parser suite: the parser itself -- CLI11's App at
* the root, through the C++ face's cli::arg_parser -- and what it costs.
*   It holds the face to CLI11 for what a program does with a parser once its
* definitions are made: the parse in each of its forms, the ending exit()
* writes for each kind of result, D_ARG_PARSE as CLI11_PARSE, the name a
* parse gives a root without one, and what a parse forgets -- its sections
* CLI11's own transcripts of the same definitions (the suite's CASES
* section, compiled against CLI11 by tools/cppref/cppref.py). Then what the
* face has that CLI11 has not, in sections of the module's own: the errors
* as views, the parser's output, statuses ended that are not the last
* parse's, response files, suggestions and the completion flag, the
* departures of its parse forms -- and the zero-overhead checks: the parser
* adds no member to its C struct, every handle is one pointer, every value
* type is its C struct, from C++11 each trivially copyable.
*   The suite is a table of sections, defined in arg_parser_tests.cpp beside
* the sections themselves; a section exists from the language level, and
* under the configuration, that its subject exists at, so the table's length
* is the number of sections this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_parser_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_PARSER_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_PARSER_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../c/tests_section.h"  // d_tests_section


// 1.  ENDINGS
bool d_tests_arg_cpp_parser_exits();
bool d_tests_arg_cpp_parser_macro();

// 2.  PARSES
bool d_tests_arg_cpp_parser_forms();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_parser_forms_moved();
#endif
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
bool d_tests_arg_cpp_parser_forms_stream();
#endif
#if (D_INTERNAL_ARG_PARSER_WIDE_ARGV == 1)
bool d_tests_arg_cpp_parser_forms_wide();
#endif
bool d_tests_arg_cpp_parser_names();
bool d_tests_arg_cpp_parser_state();

// 3.  THE PARSER'S OWN
bool d_tests_arg_cpp_parser_errors();
bool d_tests_arg_cpp_parser_output();
bool d_tests_arg_cpp_parser_statuses();
#if (D_INTERNAL_ARG_PARSER_RESPONSE_FILES == 1)
bool d_tests_arg_cpp_parser_response_files();
#endif
#if (D_INTERNAL_ARG_PARSER_SUGGESTIONS == 1)
bool d_tests_arg_cpp_parser_suggestions();
#endif
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
bool d_tests_arg_cpp_parser_completion_flag();
#endif

// 4.  ZERO OVERHEAD
bool d_tests_arg_cpp_parser_layout();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_parser_traits();
#endif

// 5.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_parser_departures();

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_parser[];
extern const std::size_t     d_tests_arg_cpp_parser_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_PARSER_TESTS_HPP
