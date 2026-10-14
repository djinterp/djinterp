/*******************************************************************************
* djinterp [parse]                                             arg_smoke_tests.h
*
* The argument parser's smoke suite: the C API end to end, from C.
*   Each section defines a command line the way a program would, parses it,
* and checks what came of it against CLI11's result for the same definitions
* and arguments: options and flags in their spellings, positionals, `--`
* and `++`, subcommands, bound variables, the errors with their messages and
* exit codes, help and versions as d_arg_parser_exit writes them, main's
* `argv` and D_ARG_PARSE, a command line in one string, definition errors,
* a parser parsed twice, and callbacks in the order they run. The suites of
* the separate modules go deeper; this one shows that the parts work
* together.
*   The suite is a table of sections, defined in arg_smoke_tests.c beside the
* sections themselves, so its length is the number of sections this build
* has.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_smoke_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SMOKE_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SMOKE_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// the command line: options, flags, positionals and subcommands
bool d_tests_arg_smoke_mixed(void);
bool d_tests_arg_smoke_short_clusters(void);
bool d_tests_arg_smoke_flag_values(void);
bool d_tests_arg_smoke_subcommands(void);
bool d_tests_arg_smoke_bound(void);
bool d_tests_arg_smoke_callbacks(void);

// failures: what each error says, and the code it exits with
bool d_tests_arg_smoke_required(void);
bool d_tests_arg_smoke_needs(void);
bool d_tests_arg_smoke_excludes(void);
bool d_tests_arg_smoke_missing_value(void);
bool d_tests_arg_smoke_conversion(void);
bool d_tests_arg_smoke_definition_error(void);

// ending: help, versions, failure messages
bool d_tests_arg_smoke_help(void);
bool d_tests_arg_smoke_version(void);
bool d_tests_arg_smoke_failure_message(void);

// entry points: main's arguments, D_ARG_PARSE, a line, a second parse
bool d_tests_arg_smoke_main(void);
bool d_tests_arg_smoke_parse_macro(void);
bool d_tests_arg_smoke_line(void);
bool d_tests_arg_smoke_reparse(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_smoke[];
extern const size_t                 d_tests_arg_smoke_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SMOKE_TESTS_H
