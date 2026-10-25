/*******************************************************************************
* djinterp [parse]                                          arg_option_tests.hpp
*
* The argument parser's C++ option suite: CLI11's Option interface through
* the C++ face's handle, cli::option.
*   The C option suite holds the C library's options to CLI11 case by case;
* this one holds the face to it member by member: every setting through the
* handle, read back through every reader; names, defaults, relations,
* validators and each() as CLI11's Option has them, their sections CLI11's
* own transcripts of the same definitions (the suite's CASES section,
* compiled against CLI11 by tools/cppref/cppref.py); and what the face does
* that CLI11 does not -- a handle on no option, callbacks returning a status
* or a truth value, binding a variable after the fact, failures as values --
* in sections of the module's own.
*   The suite is a table of sections, defined in arg_option_tests.cpp beside
* the sections themselves; a section exists from the language level, and
* under the configuration, that its subject exists at, so the table's length
* is the number of sections this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_option_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_OPTION_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_OPTION_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../c/tests_section.h"  // d_tests_section


// 1.  THE HANDLE
bool d_tests_arg_cpp_option_handle();

// 2.  SETTINGS
bool d_tests_arg_cpp_option_settings();
bool d_tests_arg_cpp_option_names();

// 3.  DEFINITION
bool d_tests_arg_cpp_option_defaults();
bool d_tests_arg_cpp_option_relations();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_option_relations_several();
#endif
bool d_tests_arg_cpp_option_validators();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_option_validators_mapped();
#endif
bool d_tests_arg_cpp_option_each();

// 4.  VALUES
bool d_tests_arg_cpp_option_results();
bool d_tests_arg_cpp_option_callbacks();
bool d_tests_arg_cpp_option_binding();

// 5.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_option_departures();

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_option[];
extern const std::size_t     d_tests_arg_cpp_option_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_OPTION_TESTS_HPP
