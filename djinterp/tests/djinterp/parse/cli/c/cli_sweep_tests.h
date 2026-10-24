/*******************************************************************************
* djinterp [parse]                                             cli_sweep_tests.h
*
* The command-line kit's allocation sweep: every allocation refused in turn.
*   A case is one thing a program does -- making itself, a command line run
* to its end, a file written or read, a module found or loaded -- run once as
* it is, counting its allocations, then once for each of them with that one
* refused. Every run must come to an end, with no block of memory left live,
* and must either do all that its clean run did or end in trouble: the
* status the clean run gave with the same output, its diagnostics those of
* the clean run or fewer, as a report that cannot be composed is dropped; or
* D_CLI_EXIT_TROUBLE with no output the clean run would not have begun.
* Nothing is ever said to have worked that did not: a file written is the
* new file or the old one, whole, with nothing left beside it, and a
* module's run is given what it was given in the clean run.
*   The suite wraps malloc, calloc, realloc and free at link time, as GNU
* ld's --wrap, lld's and MinGW's do, so it is built only where the linker
* has the switch: by its own runner, which the leaf links with it. Its
* plugin cases load the probe modules from the directory the runner is
* given, as the plugin suite does.
*   The suite is a table of sections, defined in cli_sweep_tests.c beside
* the sections themselves; every build has all five, each sweeping what its
* knobs build.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_sweep_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_C_CLI_SWEEP_TESTS_H
#define DJINTERP_PARSE_CLI_C_CLI_SWEEP_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../c/tests_section.h"     // d_tests_section


// 0.  THE MODULES
//   setting: the directory the probe modules are in, as the runner was
// given it, made absolute before any section moves into a scratch directory;
// NULL for the runner's own.
void d_tests_cli_sweep_modules(const char* _directory);

// 1.  MAKING
bool d_tests_cli_sweep_making(void);

// 2.  ENDINGS
bool d_tests_cli_sweep_endings(void);

// 3.  WORK
bool d_tests_cli_sweep_work(void);

// 4.  FILES
bool d_tests_cli_sweep_files(void);

// 5.  PLUGINS
bool d_tests_cli_sweep_plugins(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_cli_sweep[];
extern const size_t                 d_tests_cli_sweep_count;


#endif  // DJINTERP_PARSE_CLI_C_CLI_SWEEP_TESTS_H
