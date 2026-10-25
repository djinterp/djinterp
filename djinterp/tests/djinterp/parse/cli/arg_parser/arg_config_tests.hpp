/*******************************************************************************
* djinterp [parse]                                          arg_config_tests.hpp
*
* The argument parser's C++ configuration suite: configuration formats,
* through the C++ face's cli::config_format.
*   The C configuration suite holds the C library's reading and writing to
* CLI11's text by text; this one holds the face's way to them: CLI11's
* ConfigTOML and ConfigINI as values whose settings shape what is read and
* written, text and files read into items, a command's values written, a
* format given to a command for its parse, operations replaced by classes
* deriving from the format, as CLI11's subclasses replace its virtual
* functions, and formats copied -- their sections CLI11's own transcripts
* of the same definitions (the suite's CASES section, compiled against
* CLI11 by tools/cppref/cppref.py). Then the face's own: the settings read
* back, items built, operations that fail, and where the face departs from
* CLI11; and, without configuration files built in, what a format and a
* configuration option come to.
*   The suite is a table of sections, defined in arg_config_tests.cpp
* beside the sections themselves; a section exists in the builds its
* subject exists in, so the table's length is the number of sections this
* build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_config_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_CONFIG_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_CONFIG_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // knobs
#include "../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
// 1.  CLI11'S CONFIGURATION
bool d_tests_arg_cpp_config_read();
bool d_tests_arg_cpp_config_written();
bool d_tests_arg_cpp_config_parsed();
bool d_tests_arg_cpp_config_replaced();
bool d_tests_arg_cpp_config_copies();
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// 2.  THE FACE'S OWN
bool d_tests_arg_cpp_config_settings();
bool d_tests_arg_cpp_config_items();
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
bool d_tests_arg_cpp_config_failed();

// 3.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_config_departures();
#else
// 3.  WITHOUT CONFIGURATION FILES
bool d_tests_arg_cpp_config_unbuilt();
#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_config[];
extern const std::size_t     d_tests_arg_cpp_config_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_CONFIG_TESTS_HPP
