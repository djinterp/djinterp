/*******************************************************************************
* djinterp [parse]                                         arg_command_tests.hpp
*
* The argument parser's C++ command suite: CLI11's App interface through the
* C++ face -- the parser, which is CLI11's App at the root, and the handles
* on subcommands and option groups, cli::command and cli::option_group.
*   The C command suite holds the C library's commands to CLI11 case by case;
* this one holds the face to it member by member, through each of the three:
* options and flags in every form CLI11 adds them, the special options,
* subcommands and option groups found, moved and removed, names and text,
* every setting and the option defaults, callbacks, needs and excludes, what
* a parse leaves to ask about, help, triggers, and options deprecated and
* retired -- their sections CLI11's own transcripts of the same definitions
* (the suite's CASES section, compiled against CLI11 by
* tools/cppref/cppref.py); and what the face does that CLI11 does not -- a
* handle on no command, callbacks returning a status, pre_callback as a
* setting, a failure message over the face's types, formatters borrowed --
* in sections of the module's own.
*   The suite is a table of sections, defined in arg_command_tests.cpp beside
* the sections themselves; a section exists from the language level, and
* under the configuration, that its subject exists at, so the table's length
* is the number of sections this build has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/arg_command_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMAND_TESTS_HPP
#define DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMAND_TESTS_HPP 1

// std
#include <cstddef>                  // std::size_t
// djinterp
#include "../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../../../inc/djinterp/env/env.h"  // language levels
#include "../../c/tests_section.h"  // d_tests_section


// 1.  THE HANDLES
bool d_tests_arg_cpp_command_handles();

// 2.  OPTIONS
bool d_tests_arg_cpp_command_add();
bool d_tests_arg_cpp_command_flags();
bool d_tests_arg_cpp_command_special();
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
bool d_tests_arg_cpp_command_special_config();
#endif

// 3.  SUBCOMMANDS AND GROUPS
bool d_tests_arg_cpp_command_subcommands();
bool d_tests_arg_cpp_command_groups();
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
bool d_tests_arg_cpp_command_groups_several();
#endif
bool d_tests_arg_cpp_command_removal();

// 4.  SETTINGS
bool d_tests_arg_cpp_command_names();
bool d_tests_arg_cpp_command_settings();
bool d_tests_arg_cpp_command_option_defaults();
bool d_tests_arg_cpp_command_callbacks();
bool d_tests_arg_cpp_command_relations();

// 5.  QUERIES
bool d_tests_arg_cpp_command_listed();
bool d_tests_arg_cpp_command_text();
#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)
bool d_tests_arg_cpp_command_text_config();
#endif

// 6.  UTILITIES
bool d_tests_arg_cpp_command_triggers();
bool d_tests_arg_cpp_command_lifecycle();

// 7.  DEPARTURES FROM CLI11
bool d_tests_arg_cpp_command_none_found();
bool d_tests_arg_cpp_command_statuses();
bool d_tests_arg_cpp_command_pre_callback();
bool d_tests_arg_cpp_command_failure_function();
bool d_tests_arg_cpp_command_borrowed();

// the suite's sections, in the order they run
extern const d_tests_section d_tests_arg_cpp_command[];
extern const std::size_t     d_tests_arg_cpp_command_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_ARG_COMMAND_TESTS_HPP
