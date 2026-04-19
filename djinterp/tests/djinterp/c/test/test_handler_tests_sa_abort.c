/******************************************************************************
* djinterp [test]                                 test_handler_tests_sa_abort.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_abort_on_failure
  Tests abort_on_failure behavior in the test handler.
  Tests the following:
  - fresh handler starts with abort_on_failure disabled
  - D_TEST_HANDLER_FLAG_ABORT_ON_FAIL flag enables it
  - disabled: all 5 tests run despite mid-block failure
  - enabled: execution stops after first failure (3 of 5)
  - enabled with no failures: all 5 tests still run
*/
bool
d_tests_sa_handler_abort_on_failure
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test_block*   b;
    struct d_test_result   r;
    int                    i;

    printf("  --- Testing abort_on_failure ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: fresh handler starts with abort_on_failure=false
    h = d_test_handler_new(NULL);

    if (h)
    {
        result = d_assert_standalone(
            h->abort_on_failure == false,
            "fresh_abort_false",
            "fresh handler should have abort_on_failure=false",
            _test_info) && result;

        d_test_handler_free(h);
    }

    // test 2: flag enables abort_on_failure at construction
    h = d_test_handler_new_full(NULL,
                                0,
                                0,
                                D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);

    if (h)
    {
        result = d_assert_standalone(
            h->abort_on_failure == true,
            "flag_sets_abort",
            "ABORT_ON_FAIL flag should set abort_on_failure=true",
            _test_info) && result;

        d_test_handler_free(h);
    }

    // test 3: disabled — all 5 tests run despite failure in middle
    h = d_test_handler_new(NULL);

    if (h)
    {
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            // 2 passing, 1 failing, 2 passing
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

            result = d_assert_standalone(
                ( (r.tests_run == 5)    &&
                  (r.tests_passed == 4) &&
                  (r.tests_failed == 1) ),
                "disabled_all_five_run",
                "all 5 tests should run when abort is disabled",
                _test_info) && result;

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    // test 4: enabled — stops after first failure (3 of 5 run)
    h = d_test_handler_new(NULL);

    if (h)
    {
        h->abort_on_failure = true;
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            // 2 passing, 1 failing, 2 passing
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

            result = d_assert_standalone(
                ( (r.tests_run == 3)    &&
                  (r.tests_passed == 2) &&
                  (r.tests_failed == 1) ),
                "enabled_stops_at_failure",
                "abort should stop after first failure (3 of 5 run)",
                _test_info) && result;

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    // test 5: enabled with no failures — all 5 still run
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

            result = d_assert_standalone(
                r.tests_run == 5,
                "no_failures_all_run",
                "all 5 should run when abort enabled but no failures",
                _test_info) && result;

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    if (result)
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