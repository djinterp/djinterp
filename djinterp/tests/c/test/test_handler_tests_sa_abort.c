/******************************************************************************
* djinterp [test]                                 test_handler_tests_sa_abort.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_abort_on_failure
  Tests the abort_on_failure behavior of the test handler.
  Tests the following:
  - fresh handler has abort_on_failure=false
  - D_TEST_HANDLER_FLAG_ABORT_ON_FAIL flag sets abort_on_failure
  - disabled: all 5 tests run despite mid-block failure
  - enabled: execution stops after first failure (3 of 5)
  - no failures: all tests run even with abort enabled
*/
bool
d_tests_sa_handler_abort_on_failure
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   ok;
    struct d_test_handler* h;
    struct d_test_block*   b;
    struct d_test_result   r;
    int                    i;

    printf("  --- Testing abort_on_failure ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // fresh handler: abort false
    h = d_test_handler_new(NULL);

    if (h)
    {
        if (!d_assert_standalone(
                h->abort_on_failure == false,
                "fresh abort=false",
                "Starts false",
                _test_info))
        {
            ok = false;
        }

        d_test_handler_free(h);
    }

    // set via flag
    h = d_test_handler_new_full(NULL,
                                0,
                                0,
                                D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);

    if (h)
    {
        if (!d_assert_standalone(
                h->abort_on_failure == true,
                "flag sets abort",
                "Flag sets abort_on_failure",
                _test_info))
        {
            ok = false;
        }

        d_test_handler_free(h);
    }

    // disabled: all 5 run despite failure in middle
    h = d_test_handler_new(NULL);

    if (h)
    {
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            for (i = 0; i < 2; i++)
            {
                helper_add_passing_test_to_block(b);
            }

            helper_add_failing_test_to_block(b);

            for (i = 0; i < 2; i++)
            {
                helper_add_passing_test_to_block(b);
            }

            d_test_handler_run_block(h, b, NULL);
            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.tests_run == 5)    &&
                      (r.tests_passed == 4) &&
                      (r.tests_failed == 1) ),
                    "disabled: all 5 run",
                    "All 5 run when abort disabled",
                    _test_info))
            {
                ok = false;
            }

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    // enabled: stops after first failure (3 of 5)
    h = d_test_handler_new(NULL);

    if (h)
    {
        h->abort_on_failure = true;
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            for (i = 0; i < 2; i++)
            {
                helper_add_passing_test_to_block(b);
            }

            helper_add_failing_test_to_block(b);

            for (i = 0; i < 2; i++)
            {
                helper_add_passing_test_to_block(b);
            }

            d_test_handler_run_block(h, b, NULL);
            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.tests_run == 3)    &&
                      (r.tests_passed == 2) &&
                      (r.tests_failed == 1) ),
                    "enabled: 3 of 5",
                    "Abort stops at first failure",
                    _test_info))
            {
                ok = false;
            }

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    // no failures: all run even with abort enabled
    h = d_test_handler_new(NULL);

    if (h)
    {
        h->abort_on_failure = true;
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            for (i = 0; i < 5; i++)
            {
                helper_add_passing_test_to_block(b);
            }

            d_test_handler_run_block(h, b, NULL);
            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    r.tests_run == 5,
                    "no failures: all run",
                    "All 5 run when no failures",
                    _test_info))
            {
                ok = false;
            }

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] abort_on_failure\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] abort_on_failure\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}
