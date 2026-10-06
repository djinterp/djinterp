/*******************************************************************************
* djinterp [net]                                             net_tests_sa_main.c
*
* Entry point for the net foundation standalone tests.
*   Runs the suite and reports its counts. Exits 0 only if every test passed.
*
*
* path:      /tests/djinterp/net/net_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.27
*                                                            revised: 2026.09.27
*******************************************************************************/
#include "./net_tests_sa.h"  // the suite
// std
#include <stdio.h>  // printf


/*
main
  Counts from zero, runs every test, and prints tests before assertions, as
the SMTP suite does.
*/
int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_test_counter_reset(&counter);
    printf("djinterp [net] foundation standalone tests\n");

    const bool passed = d_tests_sa_net_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
