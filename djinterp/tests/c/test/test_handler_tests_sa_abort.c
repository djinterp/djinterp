#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_abort_on_failure
  Tests abort_on_failure behavior in the test handler.
  Tests the following:
  - fresh handler starts with abort_on_failure disabled
  - D_TEST_HANDLER_FLAG_ABORT_ON_FAIL flag enables it
  - disabled: all tests run despite mid-block failure
  - enabled: execution stops after first failure
  - enabled with no failures: all tests still run
*/
bool
d_tests_sa_handler_abort_on_failure
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing abort_on_failure ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // fresh handler: abort_on_failure defaults to false
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            if (!d_assert_standalone(h->abort_on_failure == false,
                                     "fresh abort=false",
                                     "Starts false",
                                     _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // set via flag at construction
    {
        struct d_test_handler* h;

        h = d_test_handler_new_full(NULL,
                                    0,
                                    0,
                                    D_TEST_HANDLER_FLAG_ABORT_ON_FAIL);
        if (h)
        {
            if (!d_assert_standalone(h->abort_on_failure == true,
                                     "flag sets abort",
                                     "Flag sets abort_on_failure",
                                     _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // disabled: all tests run despite failure (5 tests, fail in middle)
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_block* b;

            h->abort_on_failure = false;
            b = d_test_block_new(10, NULL);

            if (b)
            {
                struct d_test_result r;
                int                  i;

                for (i = 0; i < 2; i++)
                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    
                    d_test_block_add_child()

                    free(ty);
                }

                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_failing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[2] = *ty;

                    free(ty);
                }

                for (i = 3; i < 5; i++)
                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[i] = *ty;
                    free(ty);
                }

                b->count = 5;
                d_test_handler_run_block(h, b, NULL);
                r = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (r.tests_run == 5)    &&
                          (r.tests_passed == 4) &&
                          (r.tests_failed == 1) ),
                        "disabled: all 5 run",
                        "All 5 tests run when abort disabled",
                        _test_info))
                {
                    all_ok = false;
                }

                d_test_block_free(b);
            }

            d_test_handler_free(h);
        }
    }

    // enabled: stops after first failure (only 3 of 5 run)
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            struct d_test_block* b;

            h->abort_on_failure = true;
            b = d_test_block_new(10, NULL);

            if (b)
            {
                struct d_test_result r;
                int                  i;

                for (i = 0; i < 2; i++)
                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[i] = *ty;
                    free(ty);
                }

                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_failing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[2] = *ty;
                    free(ty);
                }

                for (i = 3; i < 5; i++)
                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[i] = *ty;
                    free(ty);
                }

                b->count = 5;
                d_test_handler_run_block(h, b, NULL);
                r = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (r.tests_run == 3)    &&
                          (r.tests_passed == 2) &&
                          (r.tests_failed == 1) ),
                        "enabled: 3 of 5 run",
                        "Abort stops at first failure",
                        _test_info))
                {
                    all_ok = false;
                }

                d_test_block_free(b);
            }

            d_test_handler_free(h);
        }
    }

    // no failures: all run even with abort enabled
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);
        if (h)
        {
            struct d_test_block* b;

            h->abort_on_failure = true;
            b = d_test_block_new(5, NULL);

            if (b)
            {
                struct d_test_result r;
                int                  i;

                for (i = 0; i < 5; i++)
                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[i] = *ty;
                    free(ty);
                }

                b->count = 5;
                d_test_handler_run_block(h, b, NULL);
                r = d_test_handler_get_results(h);

                if (!d_assert_standalone(r.tests_run == 5,
                                         "no failures: all run",
                                         "All 5 run when no failures",
                                         _test_info))
                {
                    all_ok = false;
                }

                d_test_block_free(b);
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] abort_on_failure\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] abort_on_failure\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
