#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_depth_tracking
  Tests depth tracking during block execution.
  Tests the following:
  - fresh handler has current_depth=0
  - current_depth resets to 0 after block execution
  - max_depth=1 for single block
  - sequential blocks maintain correct depth
*/
bool
d_tests_sa_handler_depth_tracking
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing depth tracking ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // fresh handler and single block depth
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            if (!d_assert_standalone(h->current_depth == 0,
                                     "fresh depth=0",
                                     "Fresh handler depth=0",
                                     _test_info))
            {
                all_ok = false;
            }

            {
                struct d_test_block* b;

                b = d_test_block_new(5, NULL);

                if (b)
                {
                    struct d_test*       t;
                    struct d_test_type*  ty;
                    struct d_test_result r;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[0] = *ty;
                    b->count    = 1;
                    free(ty);

                    d_test_handler_run_block(h, b, NULL);

                    if (!d_assert_standalone(
                            h->current_depth == 0,
                            "depth resets after block",
                            "current_depth=0 after execution",
                            _test_info))
                    {
                        all_ok = false;
                    }

                    r = d_test_handler_get_results(h);

                    if (!d_assert_standalone(r.max_depth == 1,
                                             "max_depth=1 single block",
                                             "Single block max_depth=1",
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

    // sequential blocks maintain correct depth
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            int  i;
            bool seq_ok;

            seq_ok = true;

            for (i = 0; i < 3; i++)
            {
                struct d_test_block* b;

                b = d_test_block_new(5, NULL);

                if (b)
                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    b->tests[0] = *ty;
                    b->count    = 1;
                    free(ty);

                    d_test_handler_run_block(h, b, NULL);

                    if (h->current_depth != 0)
                    {
                        seq_ok = false;
                    }

                    d_test_block_free(b);
                }
            }

            if (!d_assert_standalone(seq_ok,
                                     "sequential depth tracking",
                                     "Depth correct across sequential blocks",
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
        printf("%s[PASS] depth_tracking\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] depth_tracking\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_max_depth
  Tests max_depth tracking across shallow and deep execution.
  Tests the following:
  - fresh handler has max_depth=0
  - max_depth tracks the highest depth reached
  - reset clears max_depth to 0
*/
bool
d_tests_sa_handler_max_depth
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing max_depth ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_result r;

            r = d_test_handler_get_results(h);

            // fresh max_depth=0
            if (!d_assert_standalone(r.max_depth == 0,
                                     "fresh max_depth=0",
                                     "Fresh=0",
                                     _test_info))
            {
                all_ok = false;
            }

            // run shallow block (depth=1)
            {
                struct d_test_block* sh;

                sh = d_test_block_new(5, NULL);

                if (sh)
                {
                    struct d_test*      t;
                    struct d_test_type* ty;

                    t  = d_test_new(handler_test_passing, NULL);
                    ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    sh->tests[0] = *ty;
                    sh->count    = 1;
                    free(ty);

                    d_test_handler_run_block(h, sh, NULL);
                    d_test_block_free(sh);
                }
            }

            // run deep block (depth=2, outer -> inner -> test)
            {
                struct d_test_block* outer;
                struct d_test_block* inner;

                outer = d_test_block_new(5, NULL);
                inner = d_test_block_new(5, NULL);

                if ( (outer) && (inner) )
                {
                    struct d_test*      t2;
                    struct d_test_type* ty2;
                    struct d_test_type* ity;

                    t2  = d_test_new(handler_test_passing, NULL);
                    ty2 = d_test_type_new(D_TEST_TYPE_TEST, t2);
                    inner->tests[0] = *ty2;
                    inner->count    = 1;
                    free(ty2);

                    ity = d_test_type_new(D_TEST_TYPE_TEST_BLOCK,
                                          inner);
                    outer->tests[0] = *ity;
                    outer->count    = 1;
                    free(ity);

                    d_test_handler_run_block(h, outer, NULL);
                    d_test_block_free(outer);
                }
            }

            r = d_test_handler_get_results(h);

            // max_depth should track the deepest (2)
            if (!d_assert_standalone(r.max_depth == 2,
                                     "max_depth tracks highest",
                                     "max_depth=2 after shallow+deep",
                                     _test_info))
            {
                all_ok = false;
            }

            // reset clears max_depth
            d_test_handler_reset_results(h);
            r = d_test_handler_get_results(h);

            if (!d_assert_standalone(r.max_depth == 0,
                                     "reset clears max_depth",
                                     "max_depth=0 after reset",
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
        printf("%s[PASS] max_depth\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] max_depth\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_block_counting
  Tests block counting across various scenarios.
  Tests the following:
  - fresh handler has blocks_run=0
  - empty block is counted
  - 5 sequential blocks counted correctly
  - nested blocks count each level (outer + inner = 2)
*/
bool
d_tests_sa_handler_block_counting
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing block counting ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_result r;

            r = d_test_handler_get_results(h);

            // fresh blocks=0
            if (!d_assert_standalone(r.blocks_run == 0,
                                     "fresh blocks=0",
                                     "Fresh handler blocks=0",
                                     _test_info))
            {
                all_ok = false;
            }

            // empty block counts
            {
                struct d_test_block* e;

                e = d_test_block_new(5, NULL);

                if (e)
                {
                    d_test_handler_run_block(h, e, NULL);
                    r = d_test_handler_get_results(h);

                    if (!d_assert_standalone(r.blocks_run == 1,
                                             "empty block counted",
                                             "Empty block increments",
                                             _test_info))
                    {
                        all_ok = false;
                    }

                    d_test_block_free(e);
                }
            }

            // 5 sequential blocks
            {
                int i;

                d_test_handler_reset_results(h);

                for (i = 0; i < 5; i++)
                {
                    struct d_test_block* b;

                    b = d_test_block_new(5, NULL);

                    if (b)
                    {
                        struct d_test*      t;
                        struct d_test_type* ty;

                        t  = d_test_new(handler_test_passing, NULL);
                        ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                        b->tests[0] = *ty;
                        b->count    = 1;
                        free(ty);

                        d_test_handler_run_block(h, b, NULL);
                        d_test_block_free(b);
                    }
                }

                r = d_test_handler_get_results(h);

                if (!d_assert_standalone(r.blocks_run == 5,
                                         "5 blocks counted",
                                         "5 sequential blocks counted",
                                         _test_info))
                {
                    all_ok = false;
                }
            }

            // nested blocks count each level
            {
                struct d_test_block* outer;
                struct d_test_block* inner;

                d_test_handler_reset_results(h);

                outer = d_test_block_new(5, NULL);
                inner = d_test_block_new(3, NULL);

                if ( (outer) && (inner) )
                {
                    struct d_test*      t;
                    struct d_test_type* tty;
                    struct d_test_type* ity;

                    t   = d_test_new(handler_test_passing, NULL);
                    tty = d_test_type_new(D_TEST_TYPE_TEST, t);
                    inner->tests[0] = *tty;
                    inner->count    = 1;
                    free(tty);

                    ity = d_test_type_new(D_TEST_TYPE_TEST_BLOCK,
                                          inner);
                    outer->tests[0] = *ity;
                    outer->count    = 1;
                    free(ity);

                    d_test_handler_run_block(h, outer, NULL);
                    r = d_test_handler_get_results(h);

                    if (!d_assert_standalone(r.blocks_run == 2,
                                             "nested blocks=2",
                                             "Nested outer+inner=2",
                                             _test_info))
                    {
                        all_ok = false;
                    }

                    d_test_block_free(outer);
                }
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] block_counting\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] block_counting\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
