/*******************************************************************************
* djinterp [parse]                                            cli_plugin_tests.h
*
* The suite of the command-line kit's plugins (cli_plugin.c): the program's
* own file; finding a command's module along PROGRAM_PLUGIN_PATH, beside the
* program and in ../lib/PROGRAM, by the tag the program's file carries
* first; loading one and refusing one that is another program's, another
* version's or another interface's, or no module of the interface at all,
* and saying why; a program running its commands' modules, the host's
* record and the arguments handed over, and a module that is a program of
* its own through the kit; the module's side, made from the host; and
* every call given nothing.
*   The modules are the probes cli_tests_probe.c builds, `prog-KIND`, in the
* directory the runner is given, or its own.
*
*
* path:      /tests/djinterp/parse/cli/c/cli_plugin_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_PARSE_CLI_C_CLI_PLUGIN_TESTS_H
#define DJINTERP_PARSE_CLI_C_CLI_PLUGIN_TESTS_H 1

// std
#include <stdbool.h>                   // bool
#include <stddef.h>                    // size_t
// djinterp
#include "../../c/tests_section.h"     // d_tests_section


// d_tests_cli_plugin_modules
//   setting: the directory the probe modules were built in; NULL for the
// runner's own. The runner says it before the first section.
void d_tests_cli_plugin_modules(const char* _directory);

// 1.  THE PROGRAM'S FILE
bool d_tests_cli_plugin_executable(void);

// 2.  FINDING
bool d_tests_cli_plugin_finding(void);

// 3.  LOADING
bool d_tests_cli_plugin_loading(void);

// 4.  RUNNING
bool d_tests_cli_plugin_running(void);

// 5.  A PROGRAM OF ITS OWN
bool d_tests_cli_plugin_kit(void);

// 6.  THE MODULE'S SIDE
bool d_tests_cli_plugin_module_side(void);

// 7.  NULLS
bool d_tests_cli_plugin_nulls(void);

// the suite's sections, in the order they run
extern const struct d_tests_section d_tests_cli_plugin[];
extern const size_t                 d_tests_cli_plugin_count;


#endif  // DJINTERP_PARSE_CLI_C_CLI_PLUGIN_TESTS_H
