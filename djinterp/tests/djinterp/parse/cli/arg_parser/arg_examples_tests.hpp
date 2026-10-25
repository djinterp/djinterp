/*******************************************************************************
* djinterp [parse]                                        arg_examples_tests.hpp
*
* The argument parser's C++ examples suite: CLI11's own examples, ported to
* the C++ face.
*   Each of CLI11 2.7.2's examples that ports is a section: its program
* written through the face, run with the arguments CLI11's own tests give
* it, and others where they show more, as its main runs them --
* CLI11_PARSE's ending, then what the example prints -- the transcripts
* CLI11's own, of the same text compiled against it (the suite's CASES
* section, through tools/cppref/cppref.py). The examples are C++11, so the
* suite is, and an example built on a part a knob leaves out is not in a
* build without it. What does not port, and what was changed to port, is
* said in arg_examples_tests.cpp.
*   The suite is a table of sections, defined in arg_examples_tests.cpp
* beside the sections themselves; below C++11 there is no suite, and the
* runner leaves it out.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_examples_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_EXAMPLES_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_EXAMPLES_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../c/tests_section.h"  // d_tests_section


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// 1.  CLI11'S EXAMPLES
bool d_tests_arg_cpp_examples_simple();
bool d_tests_arg_cpp_examples_subcommands();
bool d_tests_arg_cpp_examples_subcom_partitioned();
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
bool d_tests_arg_cpp_examples_config_app();
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
bool d_tests_arg_cpp_examples_option_groups();
bool d_tests_arg_cpp_examples_positional_arity();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_examples_positional_validation();
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
bool d_tests_arg_cpp_examples_date_validator();
bool d_tests_arg_cpp_examples_custom_validator();
bool d_tests_arg_cpp_examples_shapes();
bool d_tests_arg_cpp_examples_ranges();
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
bool d_tests_arg_cpp_examples_validators();
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
bool d_tests_arg_cpp_examples_groups();
bool d_tests_arg_cpp_examples_inter_argument_order();
bool d_tests_arg_cpp_examples_prefix_command();
bool d_tests_arg_cpp_examples_minimal();
bool d_tests_arg_cpp_examples_arg_capture();
bool d_tests_arg_cpp_examples_callback_passthrough();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_examples_enum();
bool d_tests_arg_cpp_examples_enum_ostream();
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
bool d_tests_arg_cpp_examples_digit_args();
bool d_tests_arg_cpp_examples_modhelp();
bool d_tests_arg_cpp_examples_array_option();
bool d_tests_arg_cpp_examples_subcom_in_files();
bool d_tests_arg_cpp_examples_formatter();
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
bool d_tests_arg_cpp_examples_nested();
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
bool d_tests_arg_cpp_examples_subcom_help();
bool d_tests_arg_cpp_examples_retired();
bool d_tests_arg_cpp_examples_close_match();
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
bool d_tests_arg_cpp_examples_help_usage();
#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS
bool d_tests_arg_cpp_examples_test_exe();

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_examples[];
extern const std::size_t     d_tests_arg_cpp_examples_count;

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_EXAMPLES_TESTS_HPP
