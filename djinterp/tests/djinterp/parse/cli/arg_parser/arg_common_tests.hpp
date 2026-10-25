/*******************************************************************************
* djinterp [parse]                                          arg_common_tests.hpp
*
* The argument parser's C++ common suite: the vocabulary the C++ face's
* headers share.
*   CLI11's exit codes under the face's names, held to CLI11's own (the
* suite's CASES section, compiled against CLI11 by tools/cppref/cppref.py);
* then what the face has that CLI11 has not, in sections of the module's
* own: the status a call returns where CLI11 throws, text viewed rather
* than copied, the string parameter every call takes, the view of an error
* report, the views of the C library's lists, the buffer help is built in,
* and the sinks over a std::string and a std::ostream. The enumerations'
* values are the C library's, which the build itself asserts.
*   The suite is a table of sections, defined in arg_common_tests.cpp
* beside the sections themselves; a section exists from the language level
* its subject exists at, so the table's length is the number of sections
* this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_common_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMON_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMON_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../c/tests_section.h"  // d_tests_section


// 1.  CLI11'S VOCABULARY
bool d_tests_arg_cpp_common_exit_codes();
bool d_tests_arg_cpp_common_vocabulary();

// 2.  STATUSES
bool d_tests_arg_cpp_common_statuses();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_common_statuses_cpp11();
#endif

// 3.  TEXT AND PARAMETERS
bool d_tests_arg_cpp_common_text();
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
bool d_tests_arg_cpp_common_text_cpp17();
#endif
bool d_tests_arg_cpp_common_c_string();

// 4.  REPORTS, LISTS AND BUFFERS
bool d_tests_arg_cpp_common_errors();
bool d_tests_arg_cpp_common_lists();
bool d_tests_arg_cpp_common_buffers();
bool d_tests_arg_cpp_common_sinks();

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_common[];
extern const std::size_t     d_tests_arg_cpp_common_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMON_TESTS_HPP
