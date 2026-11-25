/*******************************************************************************
* djinterp [c][net]                                                  ssh_tests.c
*
* The section of net's SSH suite, in DTest's C face: every test, in the
* order run, each a test of its own in module ssh.
*
*
* path:      /tests/djinterp/c/modules/net/ssh/ssh_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.11.20
*******************************************************************************/
#include "./ssh_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_ssh
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_ssh(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_ssh_case CASES[] =
    {
        { "read promise",       d_tests_ssh_read_promise },
        { "failure kept",       d_tests_ssh_failure_kept },
        { "mild failures",      d_tests_ssh_failure_mild },
        { "read arguments",     d_tests_ssh_read_arguments },
        { "view transfers",     d_tests_ssh_view_transfers },
        { "view ends",          d_tests_ssh_view_ends },
        { "view closing",       d_tests_ssh_view_closing },
        { "view failures",      d_tests_ssh_view_failures },
        { "failures elsewhere", d_tests_ssh_view_elsewhere },
        { "unopened views",     d_tests_ssh_view_unopened }
    };
    bool all = true;

    (void)d_test_suite_module(_suite, "ssh");
    (void)d_test_suite_describe(_suite,
                                "SSH's sessions and channels, over the toy "
                                "server's scripted peer");

    // each case in order, in a test of its own
    for (size_t i = 0u; i < (sizeof(CASES) / sizeof(CASES[0])); ++i)
    {
        (void)d_test_suite_test(_suite,
                                CASES[i].name);

        const bool passed = CASES[i].run(_suite);
        all = (passed) && all;
    }

    return all;
}


