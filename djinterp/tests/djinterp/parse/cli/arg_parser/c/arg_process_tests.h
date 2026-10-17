/*******************************************************************************
* djinterp [parse]                                           arg_process_tests.h
*
* The suite of what ends the argument parser's parse (arg_process.c,
* arg_process_config.c): what is required, of options, subcommands and
* option groups; needs and excludes; how many subcommands and options a
* command, and an option group, requires; help and version requests; the
* order of callbacks, those that end a parse, and options' priorities;
* arguments left over; and, under their knobs, values from the environment
* and configuration files that cannot be read.
*   Every expected transcript is CLI11's, from the processing suite's
* reference program.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_process_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.14
*                                                            revised: 2026.10.14
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PROCESS_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PROCESS_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../../../../inc/djinterp/config/parse/cli/arg_parser/cfg_arg_parser.h"  // the knobs
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  REQUIRED
bool d_tests_arg_process_required(void);

// 2.  RELATIONS
bool d_tests_arg_process_relations(void);

// 3.  SUBCOMMAND COUNTS
bool d_tests_arg_process_subcommand_counts(void);

// 4.  OPTION COUNTS
bool d_tests_arg_process_option_counts(void);

// 5.  GROUP COUNTS
bool d_tests_arg_process_group_counts(void);

// 6.  HELP
bool d_tests_arg_process_help(void);

// 7.  CALLBACKS
bool d_tests_arg_process_callbacks(void);

// 8.  FAILURES
bool d_tests_arg_process_failures(void);

// 9.  PRIORITIES
bool d_tests_arg_process_priorities(void);

// 10. EXTRAS
bool d_tests_arg_process_extras(void);

#if (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1)

// 11. ENVIRONMENT
bool d_tests_arg_process_environment(void);

#endif  // D_INTERNAL_ARG_PARSER_ENVIRONMENT

#if (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1)

// 12. CONFIGURATION
bool d_tests_arg_process_configuration(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES

#if ( (D_INTERNAL_ARG_PARSER_CONFIG_FILES == 1) &&                             \
      (D_INTERNAL_ARG_PARSER_ENVIRONMENT == 1) )

// 13. CONFIGURATION FROM THE ENVIRONMENT
bool d_tests_arg_process_configuration_environment(void);

#endif  // D_INTERNAL_ARG_PARSER_CONFIG_FILES, D_INTERNAL_ARG_PARSER_ENVIRONMENT

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_process[];
extern const size_t                 d_tests_arg_process_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_PROCESS_TESTS_H
