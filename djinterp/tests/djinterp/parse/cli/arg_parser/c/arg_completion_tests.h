/*******************************************************************************
* djinterp [parse]                                        arg_completion_tests.h
*
* The suite of shell completion (arg_completion.h): the names the shells are
* looked up by; what the scripts are made of -- the identifier a program's
* functions are named by, the lines their marks are replaced in, and the
* first line of a description; what d_arg_completion_script refuses; the
* scripts for programs of every kind of name and for command trees of every
* kind of option, value, command and setting, in bash, zsh, fish and
* PowerShell; and the option d_arg_parser_set_completion_flag makes, in a
* parse.
*   CLI11 has no shell completion, so every transcript is the module's own,
* worked out by tools/gen/completion_ref.py from the rules the header
* states. What the scripts do in their shells stays with the completion
* probes, which run them there.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_completion_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.17
*                                                            revised: 2026.10.17
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMPLETION_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMPLETION_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../c/tests_section.h"  // d_tests_section


#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)

// 1.  SHELL NAMES
bool d_tests_arg_completion_shell_from_name(void);

// 2.  SHELL NUMBERS
bool d_tests_arg_completion_shell_name(void);

// 3.  IDENTIFIERS
bool d_tests_arg_completion_identifiers(void);

// 4.  LINES
bool d_tests_arg_completion_script_lines(void);

// 5.  DESCRIPTIONS
bool d_tests_arg_completion_line_text(void);

// 6.  REFUSALS
bool d_tests_arg_completion_refusals(void);

// 7.  PROGRAM NAMES
bool d_tests_arg_completion_program_names(void);

// 8.  THE ROOT ALONE
bool d_tests_arg_completion_tree_root(void);

// 9.  OPTIONS
bool d_tests_arg_completion_tree_options(void);

// 10. VALUES
bool d_tests_arg_completion_tree_values(void);

// 11. COMMANDS
bool d_tests_arg_completion_tree_commands(void);

// 12. SWITCHES
bool d_tests_arg_completion_tree_switches(void);

// 13. POSITIONALS
bool d_tests_arg_completion_tree_positionals(void);

// 14. QUOTING
bool d_tests_arg_completion_tree_quoting(void);

#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)

// 15. SETS AND MAPPINGS
bool d_tests_arg_completion_tree_member(void);

#endif  // D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS

#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)

// 16. PATHS
bool d_tests_arg_completion_tree_path(void);

#endif  // D_INTERNAL_ARG_PARSER_PATH_VALIDATORS

// 17. THE COMPLETION FLAG
bool d_tests_arg_completion_flag(void);

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 18. THE FLAG AND CONFIGURATION FILES
bool d_tests_arg_completion_flag_config(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

// 19. THE FLAG AND THE ENVIRONMENT
bool d_tests_arg_completion_flag_environment(void);

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_completion[];
extern const size_t                 d_tests_arg_completion_count;

#endif  // D_INTERNAL_ARG_PARSER_COMPLETION


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_COMPLETION_TESTS_H
