#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_complex_execution
  Tests complex execution with many blocks and tests.
  Tests the following:
  - 10 blocks x 10 tests with every 5th failing
  - correct total counts: 100 run, 80 passed, 20 failed
  - all 10 blocks counted
*/
bool
d_tests_sa_handler_complex_execution
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing complex execution ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // 10 blocks x 10 tests (every 5th fails)
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_result r;
            int                  b;

            for (b = 0; b < 10; b++)
            {
                struct d_test_block* blk;

                blk = d_test_block_new(15, NULL);

                if (blk)
                {
                    int t2;

                    for (t2 = 0; t2 < 10; t2++)
                    {
                        bool                pass;
                        struct d_test*      t;
                        struct d_test_type* ty;

                        pass = ((b * 10 + t2) % 5 != 0);
                        t    = d_test_new(pass
                                   ? handler_test_passing
                                   : handler_test_failing,
                                   NULL);
                        ty   = d_test_type_new(D_TEST_TYPE_TEST, t);
                        blk->tests[t2] = *ty;
                        free(ty);
                    }

                    blk->count = 10;
                    d_test_handler_run_block(h, blk, NULL);
                    d_test_block_free(blk);
                }
            }

            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(r.tests_run == 100,
                                     "100 tests run",
                                     "All 100 tests executed",
                                     _test_info))
            {
                all_ok = false;
            }

            if (!d_assert_standalone(r.blocks_run == 10,
                                     "10 blocks run",
                                     "All 10 blocks counted",
                                     _test_info))
            {
                all_ok = false;
            }

            if (!d_assert_standalone(r.tests_failed == 20,
                                     "20 failures",
                                     "20 tests failed (every 5th)",
                                     _test_info))
            {
                all_ok = false;
            }

            if (!d_assert_standalone(r.tests_passed == 80,
                                     "80 passes",
                                     "80 tests passed",
                                     _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] complex_execution\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] complex_execution\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_event_integration
  Tests event firing with mixed pass/fail execution.
  Tests the following:
  - SUCCESS and FAILURE events match pass/fail counts in a block
  - events continue to fire after handler reset
*/
bool
d_tests_sa_handler_event_integration
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing event integration ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // events fire correctly with mixed pass/fail in block
    {
        struct d_test_handler* h;

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

            {
                struct d_test_block* b;

                b = d_test_block_new(10, NULL);

                if (b)
                {
                    int i;

                    // 3 passing tests
                    for (i = 0; i < 3; i++)
                    {
                        struct d_test*      t;
                        struct d_test_type* ty;

                        t  = d_test_new(handler_test_passing, NULL);
                        ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                        b->tests[i] = *ty;
                        free(ty);
                    }

                    // 2 failing tests
                    for (i = 3; i < 5; i++)
                    {
                        struct d_test*      t;
                        struct d_test_type* ty;

                        t  = d_test_new(handler_test_failing, NULL);
                        ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                        b->tests[i] = *ty;
                        free(ty);
                    }

                    b->count = 5;
                    d_test_handler_run_block(h, b, NULL);

                    if (!d_assert_standalone(
                            g_event_success_count == 3,
                            "3 SUCCESS events",
                            "SUCCESS matches passes",
                            _test_info))
                    {
                        all_ok = false;
                    }

                    if (!d_assert_standalone(
                            g_event_failure_count == 2,
                            "2 FAILURE events",
                            "FAILURE matches fails",
                            _test_info))
                    {
                        all_ok = false;
                    }

                    d_test_block_free(b);
                }
            }

            d_test_handler_free(h);
        }
    }

    // events continue after reset
    {
        struct d_test_handler* h;

        reset_event_counters();
        h = d_test_handler_new_with_events(NULL, 20);

        if (h)
        {
            struct d_test* t1;
            int            before;

            d_test_handler_register_listener(h,
                                             D_TEST_EVENT_START,
                                             callback_start,
                                             true);

            t1 = d_test_new(handler_test_passing, NULL);

            if (t1)
            {
                d_test_handler_run_test(h, t1, NULL);
                d_test_free(t1);
            }

            before = g_event_start_count;
            d_test_handler_reset_results(h);

            {
                struct d_test* t2;

                t2 = d_test_new(handler_test_passing, NULL);

                if (t2)
                {
                    d_test_handler_run_test(h, t2, NULL);
                    d_test_free(t2);
                }
            }

            if (!d_assert_standalone(g_event_start_count > before,
                                     "events after reset",
                                     "Events fire after reset",
                                     _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] event_integration\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] event_integration\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_statistics_accuracy
  Tests comprehensive statistics accuracy.
  Tests the following:
  - correct test counts after 7 pass + 3 fail
  - correct assertion counts after 15 pass + 5 fail
  - 70% pass rate computed correctly
  - 75% assertion rate computed correctly
  - stats consistent between handler with and without events
*/
bool
d_tests_sa_handler_statistics_accuracy
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing statistics accuracy ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // comprehensive: tests + assertions + blocks + rates
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_result r;
            double               pr;
            double               ar;
            int                  i;

            // 7 passing tests
            for (i = 0; i < 7; i++)
            {
                struct d_test* t;

                t = d_test_new(handler_test_passing, NULL);

                if (t)
                {
                    d_test_handler_run_test(h, t, NULL);
                    d_test_free(t);
                }
            }

            // 3 failing tests
            for (i = 0; i < 3; i++)
            {
                struct d_test* t;

                t = d_test_new(handler_test_failing, NULL);

                if (t)
                {
                    d_test_handler_run_test(h, t, NULL);
                    d_test_free(t);
                }
            }

            // 15 passing assertions
            for (i = 0; i < 15; i++)
            {
                struct d_assert* a;

                a = d_assert(true, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            // 5 failing assertions
            for (i = 0; i < 5; i++)
            {
                struct d_assert* a;

                a = d_assert(false, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.tests_run == 10)    &&
                      (r.tests_passed == 7)  &&
                      (r.tests_failed == 3) ),
                    "test counts",
                    "10 run, 7 passed, 3 failed",
                    _test_info))
            {
                all_ok = false;
            }

            if (!d_assert_standalone(
                    ( (r.assertions_run == 20)    &&
                      (r.assertions_passed == 15) &&
                      (r.assertions_failed == 5) ),
                    "assertion counts",
                    "20 run, 15 passed, 5 failed",
                    _test_info))
            {
                all_ok = false;
            }

            pr = d_test_handler_get_pass_rate(h);

            if (!d_assert_standalone(
                    ( (pr >= 69.9) && (pr <= 70.1) ),
                    "pass rate 70%",
                    "70% pass rate",
                    _test_info))
            {
                all_ok = false;
            }

            ar = d_test_handler_get_assertion_rate(h);

            if (!d_assert_standalone(
                    ( (ar >= 74.9) && (ar <= 75.1) ),
                    "assert rate 75%",
                    "75% assertion rate",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // stats consistent with and without events
    {
        struct d_test_handler* h1;
        struct d_test_handler* h2;

        h1 = d_test_handler_new(NULL);
        h2 = d_test_handler_new_with_events(NULL, 20);

        if ( (h1) && (h2) )
        {
            struct d_test_result r1;
            struct d_test_result r2;
            int                  i;

            for (i = 0; i < 5; i++)
            {
                struct d_test* t1;
                struct d_test* t2;

                t1 = d_test_new(handler_test_passing, NULL);
                t2 = d_test_new(handler_test_passing, NULL);

                if (t1)
                {
                    d_test_handler_run_test(h1, t1, NULL);
                    d_test_free(t1);
                }

                if (t2)
                {
                    d_test_handler_run_test(h2, t2, NULL);
                    d_test_free(t2);
                }
            }

            r1 = d_test_handler_get_results(h1);
            r2 = d_test_handler_get_results(h2);

            if (!d_assert_standalone(
                    ( (r1.tests_run == r2.tests_run) &&
                      (r1.tests_passed == r2.tests_passed) ),
                    "stats match with/without events",
                    "Same stats regardless of events",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h1);
            d_test_handler_free(h2);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] statistics_accuracy\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] statistics_accuracy\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
