/*******************************************************************************
* djinterp [c]                                              smtp_tests_sa_main.c
*
* djinterp SMTP kernel tests -- runner.
*   Runs the standalone SMTP suite and exits non-zero if any test failed.
* Build with the four sources under src/djinterp/c/net/smtp, the standalone
* test runtime, -lssl -lcrypto where D_ENV_SMTP_LINKS_OPENSSL, and -pthread.
*
*
* path:      /tests/djinterp/c/net/smtp/smtp_tests_sa_main.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.26
*                                                            revised: 2026.09.26
*******************************************************************************/
#include "./smtp_tests_sa.h"
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // printf


int
main(void)
{
    struct d_test_counter counter = { .assertions_passed = 0u };

    d_test_counter_reset(&counter);
    printf("djinterp [c/net/smtp] standalone tests\n");

    const bool passed = d_tests_sa_smtp_run_all(&counter);

    printf("%zu/%zu tests, %zu/%zu assertions\n",
           counter.tests_passed,
           counter.tests_total,
           counter.assertions_passed,
           counter.assertions_total);

    return (passed) ? 0 : 1;
}
