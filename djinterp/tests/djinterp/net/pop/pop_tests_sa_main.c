/*******************************************************************************
* djinterp [net]                                             pop_tests_sa_main.c
*
* The POP3 kernel's standalone suite: its runner and entry point.
*   Runs every test and exits 0 only if each passed.
*
*
* path:      /tests/djinterp/net/pop/pop_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./pop_tests_sa.h"  // the suite
// std
#include <stdio.h>  // printf


/*
d_tests_sa_pop_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_pop_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_pop_case CASES[] =
    {
        { "errors from net",        d_tests_sa_pop_from_net },
        { "sending",                d_tests_sa_pop_send },
        { "send failures",          d_tests_sa_pop_send_failures },
        { "lines in pieces",        d_tests_sa_pop_read_pieces },
        { "bodies and long lines",  d_tests_sa_pop_read_body },
        { "strictness",             d_tests_sa_pop_read_strict },
        { "stalls",                 d_tests_sa_pop_read_stalls },
        { "read failures",          d_tests_sa_pop_read_failures },
        { "read refusals",          d_tests_sa_pop_read_refusals },
        { "a session over a pipe",  d_tests_sa_pop_session }
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
  Runs every test and prints tests before assertions, as the other suites
do.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_tests_net_counter_reset(&counter);
    printf("djinterp [net] pop standalone tests\n");

    const bool passed = d_tests_sa_pop_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
