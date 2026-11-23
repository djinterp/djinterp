/*******************************************************************************
* djinterp [c][net]                                                  pop_tests.c
*
* The section of net's POP3 suite, in DTest's C face: every test, in the
* order run, each a test of its own in module pop.
*
*
* path:      /tests/djinterp/c/modules/net/pop/pop_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./pop_tests.h"  // the suite
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest


/*
d_tests_pop
  The suite's section: opens its module, then a test per case, which the
case's checks land in. Returns whether every case held.
*/
bool d_tests_pop(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_pop_case CASES[] =
    {
        { "errors from net",        d_tests_pop_from_net },
        { "sending",                d_tests_pop_send },
        { "send failures",          d_tests_pop_send_failures },
        { "lines in pieces",        d_tests_pop_read_pieces },
        { "bodies and long lines",  d_tests_pop_read_body },
        { "strictness",             d_tests_pop_read_strict },
        { "stalls",                 d_tests_pop_read_stalls },
        { "read failures",          d_tests_pop_read_failures },
        { "read refusals",          d_tests_pop_read_refusals },
        { "a session over a pipe",  d_tests_pop_session }
    };
    bool all = true;

    (void)d_test_suite_module(_suite, "pop");
    (void)d_test_suite_describe(_suite,
                                "POP3's client: reading, sending and sessions");

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
