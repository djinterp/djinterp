/*******************************************************************************
* djinterp [parse]                                          cli_c_tests_runner.c
*
* Entry point for the command-line kit's C suites.
*   Built as C and linked against the kit and the argument parser, built as
* C at the C floor. A suite is a table of sections its own source defines,
* and this walks each table in turn: which sections a configuration has is
* decided where the sections are, so the count printed here differs by
* configuration, by design. Each result is flushed as it is printed, so
* that a section that crashes is the one after the last result shown.
*   NO_COLOR is set before the first section, so that a program's `auto`
* colours nothing even when the runner writes to a terminal. The one
* argument, when given, is the directory the probe modules were built in,
* which the plugin suite loads them from.
*
*
* path:      /build/cmake/config/testing/djinterp/parse/cli/cli/cli_c_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // printf, fflush
// djinterp
#include "../../../../../../../../tests/djinterp/parse/cli/c/cli_format_tests.h"  // help
#include "../../../../../../../../tests/djinterp/parse/cli/c/cli_io_tests.h"  // streams and files
#include "../../../../../../../../tests/djinterp/parse/cli/c/cli_plugin_tests.h"  // modules
#include "../../../../../../../../tests/djinterp/parse/cli/c/cli_program_tests.h"  // the program
#include "../../../../../../../../tests/djinterp/parse/cli/c/cli_tests_support.h"  // d_tests_cli_setenv


// d_tests_cli_c_suite
//   struct: one suite -- its table, and where its length is kept. The length
// is another translation unit's constant, so it is reached by address.
struct d_tests_cli_c_suite
{
    const struct d_tests_section* sections;
    const size_t*                 count;
};

// SUITES
//   constant: every C suite this build has, in the order they run: the
// streams and files the others write through, the help the program lays
// out, the program, then the modules it loads.
static const struct d_tests_cli_c_suite SUITES[] =
{
    { d_tests_cli_io,      &d_tests_cli_io_count      },
    { d_tests_cli_format,  &d_tests_cli_format_count  },
    { d_tests_cli_program, &d_tests_cli_program_count },
    { d_tests_cli_plugin,  &d_tests_cli_plugin_count  }
};

/*
main
  Runs every section of every C suite, reporting each as it finishes.
*/
int
main(
    int    _argc,
    char** _argv
)
{
    int passed = 0;
    int failed = 0;

    printf("djinterp command-line kit: C tests\n");

    // nothing coloured unless a case asks; the probe modules' directory
    (void)d_tests_cli_setenv("NO_COLOR",
                             "1");
    d_tests_cli_plugin_modules((_argc > 1) ? _argv[1] : NULL);

    // each suite
    for (size_t s = 0u; s < (sizeof(SUITES) / sizeof(SUITES[0])); s++)
    {
        // each section of the suite
        for (size_t i = 0u; i < *SUITES[s].count; i++)
        {
            const struct d_tests_section* const section =
                &SUITES[s].sections[i];
            const bool                          ok      = section->run();

            printf("  [%s] %s\n",
                   ok ? "PASS" : "FAIL",
                   section->name);
            (void)fflush(stdout);

            // tally for the summary line
            if (ok)
            {
                passed++;
            }
            else
            {
                failed++;
            }
        }
    }

    printf("passed: %d   failed: %d\n",
           passed,
           failed);

    return (failed == 0) ? 0 : 1;
}
