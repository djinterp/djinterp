/*******************************************************************************
* djinterp [parse]                                       arg_formatter_tests.hpp
*
* The argument parser's C++ formatter suite: help's formatter, through the
* C++ face's cli::formatter.
*   The C formatter suite holds the C library's help to CLI11's line by
* line; this one holds the face's way to it: CLI11's Formatter as a value
* whose settings and labels are read back and given to a command, help
* written under each, CLI11's FormatterLambda, steps replaced by classes
* deriving from the formatter, as CLI11's subclasses replace its make_*
* functions, and copies -- their sections CLI11's own transcripts of the
* same definitions (the suite's CASES section, compiled against CLI11 by
* tools/cppref/cppref.py). Then the face's own: a step that fails, help's
* longest line, and where the face departs from CLI11.
*   The suite is a table of sections, defined in arg_formatter_tests.cpp
* beside the sections themselves, so its length is the number of sections
* this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_formatter_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_FORMATTER_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_FORMATTER_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../c/tests_section.h"  // d_tests_section


// 1.  CLI11'S FORMATTER
bool d_tests_arg_cpp_formatter_settings();
bool d_tests_arg_cpp_formatter_help();
bool d_tests_arg_cpp_formatter_lambda();
bool d_tests_arg_cpp_formatter_steps();
bool d_tests_arg_cpp_formatter_copies();

// 2.  THE FACE'S OWN
bool d_tests_arg_cpp_formatter_failed();
bool d_tests_arg_cpp_formatter_lines();

// 3.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_formatter_departures();

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_formatter[];
extern const std::size_t     d_tests_arg_cpp_formatter_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_FORMATTER_TESTS_HPP
