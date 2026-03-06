#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_get_results
  Tests d_test_handler_get_results behavior.
  Tests the following:
  - NULL handler returns zeroed struct
  - fresh handler returns all zeros
  - after execution returns correct values
*/
bool
d_tests_sa_handler_get_results
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_get_results ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL handler returns zeroed struct
    {
        struct d_test_result r;

        r = d_test_handler_get_results(NULL);

        if (!d_assert_standalone(
                ( (r.tests_run == 0)      &&
                  (r.assertions_run == 0) &&
                  (r.blocks_run == 0)     &&
                  (r.max_depth == 0) ),
                "get_results NULL",
                "NULL handler returns zeroed results",
                _test_info))
        {
            all_ok = false;
        }
    }

    // fresh handler returns zeros
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_result r;

            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.tests_run == 0)    &&
                      (r.tests_passed == 0) &&
                      (r.tests_failed == 0) ),
                    "fresh handler zeros",
                    "Fresh handler has zero results",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // after execution returns correct values
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_result r;
            int                  i;

            for (i = 0; i < 3; i++)
            {
                struct d_test* t;

                t = d_test_new(handler_test_passing, NULL);

                if (t)
                {
                    d_test_handler_run_test(h, t, NULL);
                    d_test_free(t);
                }
            }

            for (i = 0; i < 2; i++)
            {
                struct d_test* t;

                t = d_test_new(handler_test_failing, NULL);

                if (t)
                {
                    d_test_handler_run_test(h, t, NULL);
                    d_test_free(t);
                }
            }

            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (r.tests_run == 5)    &&
                      (r.tests_passed == 3) &&
                      (r.tests_failed == 2) ),
                    "results after execution",
                    "Correct results after 5 tests",
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
        printf("%s[PASS] get_results\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] get_results\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_reset_results
  Tests d_test_handler_reset_results behavior.
  Tests the following:
  - NULL handler does not crash
  - reset clears all fields and current_depth
  - handler is usable after reset
*/
bool
d_tests_sa_handler_reset_results
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_reset_results ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL handler does not crash
    d_test_handler_reset_results(NULL);

    if (!d_assert_standalone(true,
                             "reset NULL",
                             "Reset NULL does not crash",
                             _test_info))
    {
        all_ok = false;
    }

    // reset clears all fields
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test* t;

            t = d_test_new(handler_test_passing, NULL);

            if (t)
            {
                d_test_handler_run_test(h, t, NULL);
                d_test_free(t);
            }

            d_test_handler_reset_results(h);

            {
                struct d_test_result r;

                r = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (r.tests_run == 0)      &&
                          (r.tests_passed == 0)   &&
                          (r.assertions_run == 0) &&
                          (r.blocks_run == 0)     &&
                          (r.max_depth == 0)      &&
                          (h->current_depth == 0) ),
                        "reset clears all",
                        "All fields zero after reset",
                        _test_info))
                {
                    all_ok = false;
                }
            }

            d_test_handler_free(h);
        }
    }

    // usable after reset
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test* t1;

            t1 = d_test_new(handler_test_passing, NULL);

            if (t1)
            {
                d_test_handler_run_test(h, t1, NULL);
                d_test_free(t1);
            }

            d_test_handler_reset_results(h);

            {
                struct d_test* t2;

                t2 = d_test_new(handler_test_passing, NULL);

                if (t2)
                {
                    bool                result;
                    struct d_test_result res;

                    result = d_test_handler_run_test(h, t2, NULL);
                    res    = d_test_handler_get_results(h);

                    if (!d_assert_standalone(
                            ( (result == true) &&
                              (res.tests_run == 1) ),
                            "usable after reset",
                            "Handler works after reset",
                            _test_info))
                    {
                        all_ok = false;
                    }

                    d_test_free(t2);
                }
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] reset_results\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] reset_results\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_print_results
  Tests d_test_handler_print_results safety.
  Tests the following:
  - print with NULL handler is safe
  - print with NULL label is safe
  - print with valid handler and label succeeds
*/
bool
d_tests_sa_handler_print_results
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_print_results ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // print with NULL handler
    d_test_handler_print_results(NULL, "NULL Handler");

    if (!d_assert_standalone(true,
                             "print NULL",
                             "Print NULL handler safe",
                             _test_info))
    {
        all_ok = false;
    }

    // print with NULL label and valid handler
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            d_test_handler_print_results(h, NULL);

            if (!d_assert_standalone(true,
                                     "print NULL label",
                                     "Print NULL label safe",
                                     _test_info))
            {
                all_ok = false;
            }

            // print with valid label
            d_test_handler_print_results(h, "Test");

            if (!d_assert_standalone(true,
                                     "print valid",
                                     "Print valid succeeds",
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
        printf("%s[PASS] print_results\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] print_results\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_get_pass_rate
  Tests d_test_handler_get_pass_rate computation.
  Tests the following:
  - NULL handler returns 0.0
  - no tests run returns 0.0
  - all passing returns 100.0
  - 50% pass rate computed correctly
*/
bool
d_tests_sa_handler_get_pass_rate
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_get_pass_rate ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL handler returns 0.0
    if (!d_assert_standalone(
            d_test_handler_get_pass_rate(NULL) == 0.0,
            "pass_rate NULL",
            "NULL=0.0",
            _test_info))
    {
        all_ok = false;
    }

    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            int i;

            // no tests run returns 0.0
            if (!d_assert_standalone(
                    d_test_handler_get_pass_rate(h) == 0.0,
                    "pass_rate no tests",
                    "No tests=0.0",
                    _test_info))
            {
                all_ok = false;
            }

            // all passing returns 100.0
            for (i = 0; i < 5; i++)
            {
                struct d_test* t;

                t = d_test_new(handler_test_passing, NULL);

                if (t)
                {
                    d_test_handler_run_test(h, t, NULL);
                    d_test_free(t);
                }
            }

            if (!d_assert_standalone(
                    d_test_handler_get_pass_rate(h) == 100.0,
                    "pass_rate 100%",
                    "All pass=100.0",
                    _test_info))
            {
                all_ok = false;
            }

            // 50% pass rate
            d_test_handler_reset_results(h);

            for (i = 0; i < 3; i++)
            {
                struct d_test* t;

                t = d_test_new(handler_test_passing, NULL);

                if (t)
                {
                    d_test_handler_run_test(h, t, NULL);
                    d_test_free(t);
                }
            }

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

            if (!d_assert_standalone(
                    d_test_handler_get_pass_rate(h) == 50.0,
                    "pass_rate 50%",
                    "50% pass",
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
        printf("%s[PASS] get_pass_rate\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] get_pass_rate\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_get_assertion_rate
  Tests d_test_handler_get_assertion_rate computation.
  Tests the following:
  - NULL handler returns 0.0
  - all passing assertions returns 100.0
  - 60% assertion pass rate computed correctly
*/
bool
d_tests_sa_handler_get_assertion_rate
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_get_assertion_rate ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL handler returns 0.0
    if (!d_assert_standalone(
            d_test_handler_get_assertion_rate(NULL) == 0.0,
            "assert_rate NULL",
            "NULL=0.0",
            _test_info))
    {
        all_ok = false;
    }

    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            int i;

            // all passing assertions returns 100.0
            for (i = 0; i < 10; i++)
            {
                struct d_assert* a;

                a = d_assert(true, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            if (!d_assert_standalone(
                    d_test_handler_get_assertion_rate(h) == 100.0,
                    "assert_rate 100%",
                    "All pass=100.0",
                    _test_info))
            {
                all_ok = false;
            }

            // 60% assertion rate (6 pass, 4 fail)
            d_test_handler_reset_results(h);

            for (i = 0; i < 6; i++)
            {
                struct d_assert* a;

                a = d_assert(true, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            for (i = 0; i < 4; i++)
            {
                struct d_assert* a;

                a = d_assert(false, "P", "F");
                d_test_handler_record_assertion(h, a);
                d_assert_free(a);
            }

            {
                double rate;

                rate = d_test_handler_get_assertion_rate(h);

                if (!d_assert_standalone(
                        ( (rate >= 59.9) && (rate <= 60.1) ),
                        "assert_rate 60%",
                        "60% assertion rate",
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
        printf("%s[PASS] get_assertion_rate\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] get_assertion_rate\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
