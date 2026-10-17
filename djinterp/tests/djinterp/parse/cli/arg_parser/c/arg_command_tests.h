/*******************************************************************************
* djinterp [parse]                                           arg_command_tests.h
*
* The suite of the argument parser's commands (arg_command.h): the options a
* command makes and the names it refuses them; the options it lists; flags,
* plain, bound to variables, and calling functions, with the counts and sums
* those are given; the options that ask for help, the version and
* configuration files; subcommands and option groups, the names they are
* refused, what they start with and how they are found; options and
* subcommands moved into groups, and removing either; names and aliases, the
* settings and what subcommands take of them, usage and footer text,
* callbacks, needs and excludes; the names a command answers to, how it is
* named, and what a parse leaves to ask about; triggers, and options
* deprecated and retired.
*   Every expected transcript is CLI11's, from the command suite's reference
* program, but where a section says it departs.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_command_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMAND_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMAND_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  OPTIONS
bool d_tests_arg_command_add(void);
bool d_tests_arg_command_listed(void);
bool d_tests_arg_command_flags(void);
bool d_tests_arg_command_flag_bound(void);
bool d_tests_arg_command_flag_sums(void);
bool d_tests_arg_command_special(void);

// 2.  SUBCOMMANDS AND GROUPS
bool d_tests_arg_command_subcommands(void);
bool d_tests_arg_command_moves(void);
bool d_tests_arg_command_removal(void);

// 3.  SETTINGS
bool d_tests_arg_command_names(void);
bool d_tests_arg_command_settings(void);
bool d_tests_arg_command_usage_text(void);
bool d_tests_arg_command_callbacks(void);
bool d_tests_arg_command_relations(void);
bool d_tests_arg_command_relation_parses(void);

// 4.  QUERIES
bool d_tests_arg_command_checks(void);
bool d_tests_arg_command_display(void);
bool d_tests_arg_command_parsed(void);

// 5.  UTILITIES
bool d_tests_arg_command_triggers(void);
bool d_tests_arg_command_lifecycle(void);

// 6.  DEPARTURES FROM CLI11
bool d_tests_arg_command_unread_sums(void);
bool d_tests_arg_command_decimal_sums(void);
bool d_tests_arg_command_swept(void);
bool d_tests_arg_command_kept_in_place(void);
bool d_tests_arg_command_kept_apart(void);
bool d_tests_arg_command_moved_links(void);
bool d_tests_arg_command_trigger_swept(void);
bool d_tests_arg_command_contexts(void);
bool d_tests_arg_command_out_of_range(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_command[];
extern const size_t                 d_tests_arg_command_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMMAND_TESTS_H
