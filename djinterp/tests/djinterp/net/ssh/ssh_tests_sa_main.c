/*******************************************************************************
* djinterp [net]                                             ssh_tests_sa_main.c
*
* The SSH standalone suite: its runner and entry point.
*   Runs every test over the toy engine and exits 0 only if each passed. It
* needs neither an SSH library at run time nor a server.
*
*
* path:      /tests/djinterp/net/ssh/ssh_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.14
*******************************************************************************/
#include "./ssh_tests_sa.h"  // the suite
// std
#include <stdio.h>  // printf


/*
d_tests_sa_ssh_run_all
  Runs every test, reporting each, and counts tests into `_counter`.
*/
bool
d_tests_sa_ssh_run_all(
    struct d_test_counter* _counter
)
{
    static const struct d_tests_ssh_case CASES[] =
    {
        { "read promise",       d_tests_sa_ssh_read_promise },
        { "failure kept",       d_tests_sa_ssh_failure_kept },
        { "mild failures",      d_tests_sa_ssh_failure_mild },
        { "read arguments",     d_tests_sa_ssh_read_arguments },
        { "view transfers",     d_tests_sa_ssh_view_transfers },
        { "view ends",          d_tests_sa_ssh_view_ends },
        { "view closing",       d_tests_sa_ssh_view_closing },
        { "view failures",      d_tests_sa_ssh_view_failures },
        { "failures elsewhere", d_tests_sa_ssh_view_elsewhere },
        { "unopened views",     d_tests_sa_ssh_view_unopened }
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
    printf("djinterp [net] ssh standalone tests\n");

    const bool passed = d_tests_sa_ssh_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
