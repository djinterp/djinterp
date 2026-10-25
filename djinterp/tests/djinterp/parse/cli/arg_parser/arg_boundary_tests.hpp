/*******************************************************************************
* djinterp [parse]                                        arg_boundary_tests.hpp
*
* The argument parser's C++ boundary suite: the functions of the caller's
* that the C library calls through the face.
*   Every kind of function a caller gives the face -- an option's callbacks,
* each(), validators and transformations, a command's callbacks, a type's
* conversion and writer, the functions that write a type name, a default, a
* failure, a usage line, a footer or a version, filters, help's steps, a
* configuration format's operations, and sinks -- runs inside C frames,
* under the face's guard (ISSUES 46). This suite runs each: returning each
* kind of result it may -- nothing, a cli::status, and, where CLI11's
* returns one, a bool -- and, in a build with exceptions, throwing a
* std::runtime_error, std::bad_alloc and an exception of no standard type,
* which the guard catches with `...`, row by row of ISSUES 47's table, with
* what the parse, help or the writer comes to. Every section is the
* module's own: CLI11 has no status for a function to return, and lets
* what one throws leave parse().
*   The suite is a table of sections, defined in arg_boundary_tests.cpp
* beside the sections themselves; without exceptions -- -fno-exceptions --
* only the results' section is, and the filters' and the configuration
* formats' sections are only where their knobs build them, so the table's
* length is the number of sections this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_boundary_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_BOUNDARY_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_BOUNDARY_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../c/tests_section.h"  // d_tests_section


// 1.  RESULTS
bool d_tests_arg_cpp_boundary_results();

#if (D_INTERNAL_ARG_PARSER_CPP_CATCH == 1)
// 2.  EXCEPTIONS
bool d_tests_arg_cpp_boundary_thrown_options();
bool d_tests_arg_cpp_boundary_thrown_commands();
bool d_tests_arg_cpp_boundary_thrown_text();
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
bool d_tests_arg_cpp_boundary_thrown_filters();
#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
bool d_tests_arg_cpp_boundary_thrown_config();
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES
#endif  // D_INTERNAL_ARG_PARSER_CPP_CATCH

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_boundary[];
extern const std::size_t     d_tests_arg_cpp_boundary_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_BOUNDARY_TESTS_HPP
