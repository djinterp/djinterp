/*******************************************************************************
* djinterp [net]                                            http_tests_sa_main.c
*
* The runner for net/http/http.h's standalone tests.
*   Runs every test in order, reports each, and prints tests before
* assertions, as the net and tcp suites do.
*
*
* path:      /tests/djinterp/net/http/http_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
*                                                            revised: 2026.09.28
*******************************************************************************/
#include "./http_tests_sa.h"  // the suite
// std
#include <stdio.h>  // printf


/*
d_tests_sa_http_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_http_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_http_case CASES[] =
    {
        { "methods",        d_tests_sa_http_methods },
        { "versions",       d_tests_sa_http_versions },
        { "status codes",   d_tests_sa_http_status },
        { "reason phrases", d_tests_sa_http_reasons },
        { "field syntax",   d_tests_sa_http_syntax },
        { "common names",   d_tests_sa_http_names },
        { "error names",    d_tests_sa_http_errors },
        { "http/1.1 heads", d_tests_sa_http1_heads },
        { "head parts",     d_tests_sa_http1_parts },
        { "byte at a time", d_tests_sa_http1_incremental },
        { "head limits",    d_tests_sa_http1_limits },
        { "field lookup",   d_tests_sa_http1_lookup }
    };
    bool all = true;

    // each case in order, counted and reported
    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        const bool passed = CASES[i].run(_counter);

        _counter->tests_total  += 1u;
        _counter->tests_passed += (passed) ? 1u : 0u;
        printf("  %s  %s\n",
               (passed) ? "pass" : "FAIL",
               CASES[i].name);
        all = (passed) && all;
    }

    return all;
}

/*
main
  Counts from zero, runs every test, and prints tests before assertions.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_test_counter_reset(&counter);
    printf("djinterp [net] http standalone tests\n");

    const bool passed = d_tests_sa_http_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
