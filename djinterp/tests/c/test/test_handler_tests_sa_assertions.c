#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_record_assertion
  Tests d_test_handler_record_assertion behavior.
  Tests the following:
  - recording on NULL handler does not crash
  - recording a NULL assertion does not crash
  - passing assertion increments correct counters
  - failing assertion increments correct counters
  - mixed accumulation tracks pass/fail totals correctly
*/
bool
d_tests_sa_handler_record_assertion
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_record_assertion ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL handler does not crash
    {
        struct d_assert* a;

        a = d_assert(true, "T", "F");
        d_test_handler_record_assertion(NULL, a);
        d_assert_free(a);

        if (!d_assert_standalone(true,
                                 "record on NULL handler",
                                 "Recording on NULL handler does not crash",
                                 _test_info))
        {
            all_ok = false;
        }
    }

    // NULL assertion does not crash
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            d_test_handler_record_assertion(h, NULL);

            if (!d_assert_standalone(true,
                                     "record NULL assertion",
                                     "Recording NULL assertion does not crash",
                                     _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // passing assertion updates counters
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            struct d_assert*    a;
            struct d_test_result r;

            a = d_assert(true, "P", "F");
            d_test_handler_record_assertion(h, a);
            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.assertions_run == 1)    &&
                      (r.assertions_passed == 1) &&
                      (r.assertions_failed == 0) ),
                    "passing assertion stats",
                    "assertions: run=1,passed=1,failed=0",
                    _test_info))
            {
                all_ok = false;
            }

            d_assert_free(a);
            d_test_handler_free(h);
        }
    }

    // failing assertion updates counters
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            struct d_assert*    a;
            struct d_test_result r;

            a = d_assert(false, "P", "F");
            d_test_handler_record_assertion(h, a);
            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.assertions_run == 1)    &&
                      (r.assertions_passed == 0) &&
                      (r.assertions_failed == 1) ),
                    "failing assertion stats",
                    "assertions: run=1,passed=0,failed=1",
                    _test_info))
            {
                all_ok = false;
            }

            d_assert_free(a);
            d_test_handler_free(h);
        }
    }

    // mixed accumulation: 3 pass + 2 fail = 5 run
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            struct d_test_result r;
            int                  i;

            for (i = 0; i < 3; i++)
            {
                struct d_assert* a;

                a = d_assert(true, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            for (i = 0; i < 2; i++)
            {
                struct d_assert* a;

                a = d_assert(false, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.assertions_run == 5)    &&
                      (r.assertions_passed == 3) &&
                      (r.assertions_failed == 2) ),
                    "mixed accumulation",
                    "5 total: 3 passed, 2 failed",
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
        printf("%s[PASS] record_assertion\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] record_assertion\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_assertion_statistics
  Tests assertion statistics tracking over larger sets.
  Tests the following:
  - large set of 100 assertions with correct pass/fail split
  - assertions persist alongside test execution counts
*/
bool
d_tests_sa_handler_assertion_statistics
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing assertion statistics ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // large set: 100 assertions (every 3rd fails)
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            struct d_test_result r;
            int                  i;

            for (i = 0; i < 100; i++)
            {
                struct d_assert* a;
                bool             pass;

                pass = (i % 3 != 0);
                a    = d_assert(pass, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(r.assertions_run == 100,
                                     "100 assertions tracked",
                                     "All 100 assertions counted",
                                     _test_info))
            {
                all_ok = false;
            }

            // 34 fail (0,3,6,...,99), 66 pass
            if (!d_assert_standalone(
                    ( (r.assertions_passed == 66) &&
                      (r.assertions_failed == 34) ),
                    "correct pass/fail split",
                    "66 passed, 34 failed",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // assertions persist alongside test execution
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            struct d_assert* a;

            a = d_assert(true, "P", "F");
            d_test_handler_record_assertion(h, a);
            d_assert_free(a);

            {
                struct d_test* t;

                t = d_test_new(handler_test_passing, NULL);
                if (t)
                {
                    d_test_handler_run_test(h, t, NULL);
                    d_test_free(t);
                }
            }

            {
                struct d_test_result r;

                r = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (r.assertions_run == 1) &&
                          (r.tests_run == 1) ),
                        "assertions alongside tests",
                        "Both assertions and tests tracked",
                        _test_info))
                {
                    all_ok = false;
                }
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] assertion_statistics\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] assertion_statistics\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
