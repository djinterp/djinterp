/*******************************************************************************
* djinterp [parse]                                   arg_parser_c_sweep_runner.c
*
* Entry point for the argument parser's allocation sweep.
*   Built as C and linked against the parser's library with malloc, calloc,
* realloc and free wrapped (-Wl,--wrap=malloc and kin), so that the sweep's
* wrappers stand in front of every allocation the library makes; a linker
* without the switch cannot build it, and the leaf leaves it out there. It
* walks the sweep's table as the C runner walks the suites', and prints the
* same summary line.
*
*
* path:      /build/cmake/config/testing/djinterp/parse/cli/arg_parser/arg_parser_c_sweep_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // printf, fflush
// djinterp
#include "../../../../../../../../tests/djinterp/parse/cli/arg_parser/c/arg_sweep_tests.h"  // the sweep


/*
main
  Runs every section of the sweep, reporting each as it finishes.
*/
int
main(void)
{
    int passed = 0;
    int failed = 0;

    printf("djinterp argument parser: allocation sweep\n");

    // each section
    for (size_t i = 0u; i < d_tests_arg_sweep_count; i++)
    {
        const struct d_tests_section* const section = &d_tests_arg_sweep[i];
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
