/*******************************************************************************
* djinterp [parse]                                             arg_parse_tests.h
*
* The suite of the argument parser's parse (arg_parse.c, arg_parse_option.c,
* arg_parse_positional.c): how arguments are told apart and cut; what a
* command does with arguments it does not expect; Windows-style options and
* names of several characters after one dash; subcommands entered, left and
* found; dot notation; option groups; prefix commands; how many values an
* option takes; which positional takes an argument; the definitions checked
* as a parse begins; and the callbacks the parse itself runs.
*   Every expected transcript is CLI11's, from the parse suite's reference
* program.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_parse_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSE_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSE_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  TOKENS
bool d_tests_arg_parsing_tokens(void);

// 2.  EXTRAS
bool d_tests_arg_parsing_extras(void);

// 3.  STYLES
bool d_tests_arg_parsing_styles(void);

// 4.  SUBCOMMANDS
bool d_tests_arg_parsing_subcommands(void);

// 5.  DOTS
bool d_tests_arg_parsing_dots(void);

// 6.  GROUPS
bool d_tests_arg_parsing_groups(void);

// 7.  PREFIX COMMANDS
bool d_tests_arg_parsing_prefix_commands(void);

// 8.  VALUES
bool d_tests_arg_parsing_values(void);

// 9.  POSITIONALS
bool d_tests_arg_parsing_positionals(void);

// 10. CHECKS
bool d_tests_arg_parsing_checks(void);

// 11. CALLBACKS
bool d_tests_arg_parsing_callbacks(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_parsing[];
extern const size_t                 d_tests_arg_parsing_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PARSE_TESTS_H
