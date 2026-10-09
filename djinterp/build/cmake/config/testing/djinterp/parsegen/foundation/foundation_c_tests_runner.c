/*******************************************************************************
* djinterp [parsegen]                                foundation_c_tests_runner.c
*
* Entry point for the parse/parsegen foundation's C suites.
*   Built as C and linked against the same library the C++ suites use; the two
* runners together cover both faces of every module. A suite is a table of
* sections its own source defines, and this walks each table in turn: which
* sections a configuration has is decided where the sections are, so the
* count printed here differs by configuration, by design.
*
*
* path:      /build/cmake/config/testing/djinterp/parsegen/foundation/foundation_c_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // printf
// djinterp
#include "../../../../../../../tests/djinterp/parse/c/parse_c_tests.h"        // the substrate, from C
#include "../../../../../../../tests/djinterp/parsegen/c/parsegen_c_tests.h"  // parsegen, from C


// d_tests_c_suite
//   struct: one suite -- its table, and where its length is kept. The length
// is another translation unit's constant, so it is reached by address.
struct d_tests_c_suite
{
    const struct d_tests_section* sections;
    const size_t*                 count;
};

// SUITES
//   constant: every C suite, the substrate before the generator built on it.
static const struct d_tests_c_suite SUITES[] =
{
    { d_tests_parse_c,    &d_tests_parse_c_count    },
    { d_tests_parsegen_c, &d_tests_parsegen_c_count }
};

/*
main
  Runs every section of every C suite, reporting each as it finishes so that a
crash still shows which one it was in.
*/
int
main(void)
{
    int passed = 0;
    int failed = 0;

    printf("djinterp parse/parsegen foundation: C tests\n");

    for (size_t s = 0u; s < (sizeof(SUITES) / sizeof(SUITES[0])); s++)
    {
        for (size_t i = 0u; i < *SUITES[s].count; i++)
        {
            const struct d_tests_section* const section =
                &SUITES[s].sections[i];
            const bool                          ok      = section->run();

            printf("  [%s] %s\n", ok ? "PASS" : "FAIL", section->name);

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

    printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}
