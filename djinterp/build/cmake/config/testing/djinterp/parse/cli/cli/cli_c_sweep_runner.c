/*******************************************************************************
* djinterp [parse]                                          cli_c_sweep_runner.c
*
* Entry point for the command-line kit's allocation sweep.
*   Built as C and linked against the kit and the argument parser with
* malloc, calloc, realloc and free wrapped (-Wl,--wrap=malloc and kin), so
* that the sweep's wrappers stand in front of every allocation either makes;
* a linker without the switch cannot build it, and the leaf leaves it out
* there. It walks the sweep's table as the C runner walks the suites', and
* prints the same summary line.
*   NO_COLOR is set before the first section, as the C runner sets it. The
* one argument, when given, is the directory the probe modules were built
* in, which the plugin cases load them from.
*
*
* path:      /build/cmake/config/testing/djinterp/parse/cli/cli/cli_c_sweep_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
*                                                            revised: 2026.10.23
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // printf, fflush
// djinterp
#include "../../../../../../../../tests/djinterp/parse/cli/c/cli_sweep_tests.h"  // the sweep
#include "../../../../../../../../tests/djinterp/parse/cli/c/cli_tests_support.h"  // d_tests_cli_setenv


/*
main
  Runs every section of the sweep, reporting each as it finishes.
*/
int
main(
    int    _argc,
    char** _argv
)
{
    int passed = 0;
    int failed = 0;

    printf("djinterp command-line kit: allocation sweep\n");

    // nothing coloured unless a case asks; the probe modules' directory
    (void)d_tests_cli_setenv("NO_COLOR",
                             "1");
    d_tests_cli_sweep_modules((_argc > 1) ? _argv[1] : NULL);

    // each section
    for (size_t i = 0u; i < d_tests_cli_sweep_count; i++)
    {
        const struct d_tests_section* const section = &d_tests_cli_sweep[i];
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

    printf("passed: %d   failed: %d\n",
           passed,
           failed);

    return (failed == 0) ? 0 : 1;
}
