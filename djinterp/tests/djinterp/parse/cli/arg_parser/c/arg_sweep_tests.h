/*******************************************************************************
* djinterp [parse]                                             arg_sweep_tests.h
*
* The argument parser's allocation sweep: every allocation refused in turn.
*   Each section runs one scenario -- a program written with the C API, from
* its definitions through a parse to the text it writes, then released --
* once as it is, counting its allocations, then once for each of them with
* that one refused. Each run must come to an end, with no memory left live
* once the parser is released, and with nothing a refused definition made
* left behind in the command it was refused from: an option, a flag or a
* subcommand that came back NULL is not among its command's.
*   The suite wraps malloc, calloc, realloc and free at link time, as GNU
* ld's --wrap and lld's do, so it is built only where the linker has the
* switch: by its own runner, which the leaf links with it.
*   The suite is a table of sections, defined in arg_sweep_tests.c beside
* the sections themselves; every build has all five, each sweeping what its
* knobs build.
*
*
* path:      /tests/djinterp/parse/cli/arg_parser/c/arg_sweep_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SWEEP_TESTS_H
#define DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SWEEP_TESTS_H 1

// std
#include <stdbool.h>                  // bool
#include <stddef.h>                   // size_t
// djinterp
#include "../../../c/tests_section.h"  // d_tests_section


// 1.  THE SCENARIOS
bool d_tests_arg_sweep_options(void);
bool d_tests_arg_sweep_flags(void);
bool d_tests_arg_sweep_commands(void);
bool d_tests_arg_sweep_validators(void);
bool d_tests_arg_sweep_text(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_arg_sweep[];
extern const size_t                 d_tests_arg_sweep_count;


#endif  // DJINTERP_PARSE_CLI_ARG_PARSER_C_ARG_SWEEP_TESTS_H
