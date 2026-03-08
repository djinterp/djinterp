/******************************************************************************
* djinterp [test]                            test_handler_tests_sa_integration.c
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_complex_execution
  Tests large-scale execution with 10 blocks x 10 tests.
  Tests the following:
  - 100 tests all execute
  - 10 blocks all counted
  - every 5th test fails (20 failures)
  - remaining 80 tests pass
*/
bool
d_tests_sa_handler_complex_execution
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   ok;
    struct d_test_handler* h;
    struct d_test_block*   blk;
    struct d_test_result   r;
    int                    b;
    int                    t2;

    printf("  --- Testing complex execution ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // 10 blocks x 10 tests (every 5th fails)
    h = d_test_handler_new(NULL);

    if (h)
    {
        for (b = 0; b < 10; b++)
        {
            blk = d_test_block_new(NULL, 0);

            if (blk)
            {
                for (t2 = 0; t2 < 10; t2++)
                {
                    if ((b * 10 + t2) % 5 != 0)
                    {
                        helper_add_passing_test_to_block(blk);
                    }
                    else
                    {
                        helper_add_failing_test_to_block(blk);
                    }
                }

                d_test_handler_run_block(h, blk, NULL);
                d_test_block_free(blk);
            }
        }

        r = d_test_handler_get_results(h);

        if (!d_assert_standalone(
                r.tests_run == 100,
                "100 tests run",
                "All 100 executed",
                _test_info))
        {
            ok = false;
        }

        if (!d_assert_standalone(
                r.blocks_run == 10,
                "10 blocks run",
                "All 10 counted",
                _test_info))
        {
            ok = false;
        }

        if (!d_assert_standalone(
                r.tests_failed == 20,
                "20 failures",
                "Every 5th failed",
                _test_info))
        {
            ok = false;
        }

        if (!d_assert_standalone(
                r.tests_passed == 80,
                "80 passes",
                "80 passed",
                _test_info))
        {
            ok = false;
        }

        d_test_handler_free(h);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] complex_execution\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] complex_execution\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}


/*
d_tests_sa_handler_event_integration
  Tests event integration with mixed pass/fail and reset.
  Tests the following:
  - SUCCESS events match pass count
  - FAILURE events match fail count
  - events continue firing after result reset
*/
bool
d_tests_sa_handler_event_integration
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   ok;
    struct d_test_handler* h;
    struct d_test_block*   b;
    struct d_test*         t1;
    struct d_test*         t2;
    int                    before;
    int                    i;

    printf("  --- Testing event integration ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // events with mixed pass/fail
    reset_event_counters();
    h = d_test_handler_new_with_events(NULL, 20);

    if (h)
    {
        d_test_handler_register_listener(h,
                                         D_TEST_EVENT_SUCCESS,
                                         callback_success,
                                         true);
        d_test_handler_register_listener(h,
                                         D_TEST_EVENT_FAILURE,
                                         callback_failure,
                                         true);

        b = d_test_block_new(NULL, 0);

        if (b)
        {
            for (i = 0; i < 3; i++)
            {
                helper_add_passing_test_to_block(b);
            }

            for (i = 0; i < 2; i++)
            {
                helper_add_failing_test_to_block(b);
            }

            d_test_handler_run_block(h, b, NULL);

            if (!d_assert_standalone(
                    g_event_success_count == 3,
                    "3 SUCCESS events",
                    "Matches passes",
                    _test_info))
            {
                ok = false;
            }

            if (!d_assert_standalone(
                    g_event_failure_count == 2,
                    "2 FAILURE events",
                    "Matches fails",
                    _test_info))
            {
                ok = false;
            }

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    // events continue after reset
    reset_event_counters();
    h = d_test_handler_new_with_events(NULL, 20);

    if (h)
    {
        d_test_handler_register_listener(h,
                                         D_TEST_EVENT_START,
                                         callback_start,
                                         true);

        // run first test
        t1 = helper_make_passing_test();

        if (t1)
        {
            d_test_handler_run_test(h, t1, NULL);
            d_test_free(t1);
        }

        // reset results, then run another test
        before = g_event_start_count;
        d_test_handler_reset_results(h);

        t2 = helper_make_passing_test();

        if (t2)
        {
            d_test_handler_run_test(h, t2, NULL);
            d_test_free(t2);
        }

        if (!d_assert_standalone(
                g_event_start_count > before,
                "events after reset",
                "Continue firing",
                _test_info))
        {
            ok = false;
        }

        d_test_handler_free(h);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] event_integration\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] event_integration\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}


/*
d_tests_sa_handler_statistics_accuracy
  Tests comprehensive statistics accuracy.
  Tests the following:
  - test counts: 10 run, 7 passed, 3 failed
  - assertion counts: 20 run, 15 passed, 5 failed
  - pass rate is 70%
  - assertion rate is 75%
  - stats match between handlers with and without events
*/
bool
d_tests_sa_handler_statistics_accuracy
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   ok;
    struct d_test_handler* h;
    struct d_test_handler* h1;
    struct d_test_handler* h2;
    struct d_test*         t;
    struct d_assert*       a;
    struct d_test_result   r;
    struct d_test_result   r1;
    struct d_test_result   r2;
    double                 pr;
    double                 ar;
    int                    i;

    printf("  --- Testing statistics accuracy ---\n");
    ip = _test_info->tests_passed;
    ok = true;

    // comprehensive: tests + assertions + rates
    h = d_test_handler_new(NULL);

    if (h)
    {
        // 7 passing tests
        for (i = 0; i < 7; i++)
        {
            t = helper_make_passing_test();

            if (t)
            {
                d_test_handler_run_test(h, t, NULL);
                d_test_free(t);
            }
        }

        // 3 failing tests
        for (i = 0; i < 3; i++)
        {
            t = helper_make_failing_test();

            if (t)
            {
                d_test_handler_run_test(h, t, NULL);
                d_test_free(t);
            }
        }

        // 15 passing assertions
        for (i = 0; i < 15; i++)
        {
            a = d_assert_new(true, "P", "F");
            d_test_handler_record_assertion(h, a);
            d_assert_free(a);
        }

        // 5 failing assertions
        for (i = 0; i < 5; i++)
        {
            a = d_assert_new(false, "P", "F");
            d_test_handler_record_assertion(h, a);
            d_assert_free(a);
        }

        r = d_test_handler_get_results(h);

        // verify test counts
        if (!d_assert_standalone(
                ( (r.tests_run == 10)    &&
                  (r.tests_passed == 7)  &&
                  (r.tests_failed == 3) ),
                "test counts",
                "10 run, 7 passed, 3 failed",
                _test_info))
        {
            ok = false;
        }

        // verify assertion counts
        if (!d_assert_standalone(
                ( (r.assertions_run == 20)    &&
                  (r.assertions_passed == 15) &&
                  (r.assertions_failed == 5) ),
                "assertion counts",
                "20 run, 15 passed, 5 failed",
                _test_info))
        {
            ok = false;
        }

        // verify pass rate
        pr = d_test_handler_get_pass_rate(h);

        if (!d_assert_standalone(
                ( (pr >= 69.9) && (pr <= 70.1) ),
                "pass rate 70%",
                "70% pass rate",
                _test_info))
        {
            ok = false;
        }

        // verify assertion rate
        ar = d_test_handler_get_assertion_rate(h);

        if (!d_assert_standalone(
                ( (ar >= 74.9) && (ar <= 75.1) ),
                "assert rate 75%",
                "75% assertion rate",
                _test_info))
        {
            ok = false;
        }

        d_test_handler_free(h);
    }

    // stats match with/without events
    h1 = d_test_handler_new(NULL);
    h2 = d_test_handler_new_with_events(NULL, 20);

    if (h1 && h2)
    {
        for (i = 0; i < 5; i++)
        {
            t = helper_make_passing_test();

            if (t)
            {
                d_test_handler_run_test(h1, t, NULL);
                d_test_free(t);
            }

            t = helper_make_passing_test();

            if (t)
            {
                d_test_handler_run_test(h2, t, NULL);
                d_test_free(t);
            }
        }

        r1 = d_test_handler_get_results(h1);
        r2 = d_test_handler_get_results(h2);

        if (!d_assert_standalone(
                ( (r1.tests_run == r2.tests_run)       &&
                  (r1.tests_passed == r2.tests_passed) ),
                "stats match",
                "Same with/without events",
                _test_info))
        {
            ok = false;
        }

        d_test_handler_free(h1);
        d_test_handler_free(h2);
    }

    if (ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] statistics_accuracy\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] statistics_accuracy\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}
