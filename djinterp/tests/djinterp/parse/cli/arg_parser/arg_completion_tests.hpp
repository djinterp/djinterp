/*******************************************************************************
* djinterp [parse]                                      arg_completion_tests.hpp
*
* The argument parser's C++ completion suite: shell completion scripts,
* through the C++ face's functions.
*   The C completion suite holds the scripts themselves -- each shell's
* text for each kind of tree, and the scripts run by the shells; this one
* holds the face's way to them: the shells by name, and a command's script
* for each shell, made a std::string or written to a sink, which is the C
* library's to the byte, for the name the command has or one given, from
* the root or a subcommand; and the failures. Every section is the
* module's own, as CLI11 writes no completion script; the option that asks
* for one is the parser suite's.
*   The suite is a table of sections, defined in arg_completion_tests.cpp
* beside the sections themselves. Without completion built in, there is no
* suite, as there is no C one, and the runner leaves it out.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_completion_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMPLETION_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMPLETION_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

// 1.  SHELLS AND SCRIPTS
bool d_tests_arg_cpp_completion_shells();
bool d_tests_arg_cpp_completion_scripts();
bool d_tests_arg_cpp_completion_failed();

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_completion[];
extern const std::size_t     d_tests_arg_cpp_completion_count;

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMPLETION_TESTS_HPP
