#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_run_test
  Tests d_test_handler_run_test behavior.
  Tests the following:
  - NULL handler and test returns false
  - NULL test with valid handler returns false
  - passing test updates stats correctly
  - failing test updates stats correctly
  - accumulation across multiple tests
*/
bool
d_tests_sa_handler_run_test
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_run_test ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL handler and test
    if (!d_assert_standalone(
            d_test_handler_run_test(NULL, NULL, NULL) == false,
            "run_test NULL params",
            "NULL handler+test returns false",
            _test_info))
    {
        all_ok = false;
    }

    // NULL test with valid handler
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            if (!d_assert_standalone(
                    d_test_handler_run_test(h, NULL, NULL) == false,
                    "run_test NULL test",
                    "NULL test returns false",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // passing test updates stats
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test* t;

            t = d_test_new(handler_test_passing, NULL);

            if (t)
            {
                bool                result;
                struct d_test_result res;

                result = d_test_handler_run_test(h, t, NULL);
                res    = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (result == true)        &&
                          (res.tests_run == 1)    &&
                          (res.tests_passed == 1) &&
                          (res.tests_failed == 0) ),
                        "passing test stats",
                        "Passing test: run=1,passed=1,failed=0",
                        _test_info))
                {
                    all_ok = false;
                }

                d_test_free(t);
            }

            d_test_handler_free(h);
        }
    }

    // failing test updates stats
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test* t;

            t = d_test_new(handler_test_failing, NULL);

            if (t)
            {
                bool                result;
                struct d_test_result res;

                result = d_test_handler_run_test(h, t, NULL);
                res    = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (result == false)       &&
                          (res.tests_run == 1)    &&
                          (res.tests_passed == 0) &&
                          (res.tests_failed == 1) ),
                        "failing test stats",
                        "Failing test: run=1,passed=0,failed=1",
                        _test_info))
                {
                    all_ok = false;
                }

                d_test_free(t);
            }

            d_test_handler_free(h);
        }
    }

    // accumulation across 5 tests (3 pass, 2 fail)
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_result res;
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

            res = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (res.tests_run == 5)    &&
                      (res.tests_passed == 3) &&
                      (res.tests_failed == 2) ),
                    "accumulation 3+2",
                    "5 tests: 3 passed, 2 failed",
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
        printf("%s[PASS] run_test\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_test\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_run_block
  Tests d_test_handler_run_block behavior.
  Tests the following:
  - NULL parameters return false
  - empty block succeeds and increments blocks_run
  - block with passing tests reports correct stats
  - block with mixed results reports overall failure
  - depth tracking updates max_depth and resets current_depth
*/
bool
d_tests_sa_handler_run_block
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_run_block ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL parameters
    if (!d_assert_standalone(
            d_test_handler_run_block(NULL, NULL, NULL) == false,
            "run_block NULL params",
            "NULL returns false",
            _test_info))
    {
        all_ok = false;
    }

    // empty block succeeds and increments blocks_run
    {
        struct d_test_handler* h;
        struct d_test_block*   b;

        h = d_test_handler_new(NULL);
        b = d_test_block_new(10, NULL);

        if ( (h) && (b) )
        {
            bool                result;
            struct d_test_result res;

            result = d_test_handler_run_block(h, b, NULL);
            res    = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (result == true)       &&
                      (res.blocks_run == 1)  &&
                      (res.tests_run == 0) ),
                    "empty block",
                    "Empty block: succeeds, blocks_run=1, tests_run=0",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_block_free(b);
        }

        if (h)
        {
            d_test_handler_free(h);
        }
    }

    // block with passing tests
    {
        struct d_test_handler* h;
        struct d_test_block*   b;

        h = d_test_handler_new(NULL);
        b = d_test_block_new(10, NULL);

        if ( (h) && (b) )
        {
            bool                result;
            struct d_test_result res;
            int                  i;

            for (i = 0; i < 3; i++)
            {
                struct d_test*      t;
                struct d_test_type* ty;

                t  = d_test_new(handler_test_passing, NULL);
                ty = d_test_type_new(D_TEST_TYPE_TEST, t);
                b->tests[i] = *ty;
                free(ty);
            }

            b->count = 3;
            result   = d_test_handler_run_block(h, b, NULL);
            res      = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (result == true)          &&
                      (res.tests_run == 3)     &&
                      (res.tests_passed == 3)  &&
                      (res.blocks_run == 1) ),
                    "block with passing tests",
                    "Block: 3 tests all pass",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_block_free(b);
        }

        if (h)
        {
            d_test_handler_free(h);
        }
    }

    // block with mixed results
    {
        struct d_test_handler* h;
        struct d_test_block*   b;

        h = d_test_handler_new(NULL);
        b = d_test_block_new(10, NULL);

        if ( (h) && (b) )
        {
            struct d_test*      t1;
            struct d_test*      t2;
            struct d_test_type* ty1;
            struct d_test_type* ty2;
            bool                result;
            struct d_test_result res;

            t1  = d_test_new(handler_test_passing, NULL);
            t2  = d_test_new(handler_test_failing, NULL);
            ty1 = d_test_type_new(D_TEST_TYPE_TEST, t1);
            ty2 = d_test_type_new(D_TEST_TYPE_TEST, t2);

            b->tests[0] = *ty1;
            free(ty1);
            b->tests[1] = *ty2;
            free(ty2);
            b->count = 2;

            result = d_test_handler_run_block(h, b, NULL);
            res    = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (result == false)        &&
                      (res.tests_run == 2)    &&
                      (res.tests_passed == 1) &&
                      (res.tests_failed == 1) ),
                    "mixed block",
                    "Block with mixed results: overall=false",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_block_free(b);
        }

        if (h)
        {
            d_test_handler_free(h);
        }
    }

    // depth tracking
    {
        struct d_test_handler* h;
        struct d_test_block*   b;

        h = d_test_handler_new(NULL);
        b = d_test_block_new(5, NULL);

        if ( (h) && (b) )
        {
            struct d_test*       t;
            struct d_test_type*  ty;
            struct d_test_result res;

            t  = d_test_new(handler_test_passing, NULL);
            ty = d_test_type_new(D_TEST_TYPE_TEST, t);
            b->tests[0] = *ty;
            free(ty);
            b->count = 1;

            d_test_handler_run_block(h, b, NULL);
            res = d_test_handler_get_results(h);

            if (!d_assert_standalone(
                    ( (res.max_depth >= 1)      &&
                      (h->current_depth == 0) ),
                    "block depth tracking",
                    "max_depth>=1, current_depth resets to 0",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_block_free(b);
        }

        if (h)
        {
            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] run_block\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_block\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_run_test_type
  Tests d_test_handler_run_test_type dispatch behavior.
  Tests the following:
  - NULL parameters return false
  - test type wrapping a passing test dispatches correctly
  - unknown type returns false
*/
bool
d_tests_sa_handler_run_test_type
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_run_test_type ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL parameters
    if (!d_assert_standalone(
            d_test_handler_run_test_type(NULL, NULL, NULL) == false,
            "run_test_type NULL",
            "NULL returns false",
            _test_info))
    {
        all_ok = false;
    }

    // test type wrapping a passing test
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test*      t;
            struct d_test_type* ty;

            t  = d_test_new(handler_test_passing, NULL);
            ty = d_test_type_new(D_TEST_TYPE_TEST, t);

            if (ty)
            {
                bool result;

                result = d_test_handler_run_test_type(h, ty, NULL);

                if (!d_assert_standalone(result == true,
                                         "test_type wrapping test",
                                         "run_test_type dispatches to run_test correctly",
                                         _test_info))
                {
                    all_ok = false;
                }

                d_test_type_free(ty);
            }

            d_test_handler_free(h);
        }
    }

    // unknown type returns false
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_type dummy;

            d_memset(&dummy, 0, sizeof(dummy));
            dummy.type = D_TEST_TYPE_UNKNOWN;

            if (!d_assert_standalone(
                    d_test_handler_run_test_type(h, &dummy, NULL) == false,
                    "unknown type returns false",
                    "D_TEST_TYPE_UNKNOWN returns false",
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
        printf("%s[PASS] run_test_type\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_test_type\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_nested_execution
  Tests nested block execution and depth tracking.
  Tests the following:
  - two-level nesting: depth=2, blocks=2, tests=1, current_depth=0
  - three-level nesting: depth=3, blocks=3
*/
bool
d_tests_sa_handler_nested_execution
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing nested execution ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // two-level nesting: outer -> inner -> test
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_block* outer;
            struct d_test_block* inner;

            outer = d_test_block_new(10, NULL);
            inner = d_test_block_new(5, NULL);

            if ( (outer) && (inner) )
            {
                struct d_test*       t;
                struct d_test_type*  tty;
                struct d_test_type*  ity;
                struct d_test_result res;

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
                res = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (res.max_depth == 2)      &&
                          (res.blocks_run == 2)     &&
                          (res.tests_run == 1)      &&
                          (h->current_depth == 0) ),
                        "two-level nesting",
                        "depth=2, blocks=2, tests=1, current_depth=0",
                        _test_info))
                {
                    all_ok = false;
                }

                d_test_block_free(outer);
            }

            d_test_handler_free(h);
        }
    }

    // three-level nesting
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test_block* l1;
            struct d_test_block* l2;
            struct d_test_block* l3;

            l1 = d_test_block_new(5, NULL);
            l2 = d_test_block_new(5, NULL);
            l3 = d_test_block_new(5, NULL);

            if ( (l1) && (l2) && (l3) )
            {
                struct d_test*       t;
                struct d_test_type*  tty;
                struct d_test_type*  l3ty;
                struct d_test_type*  l2ty;
                struct d_test_result res;

                t    = d_test_new(handler_test_passing, NULL);
                tty  = d_test_type_new(D_TEST_TYPE_TEST, t);
                l3->tests[0] = *tty;
                l3->count    = 1;
                free(tty);

                l3ty = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, l3);
                l2->tests[0] = *l3ty;
                l2->count    = 1;
                free(l3ty);

                l2ty = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, l2);
                l1->tests[0] = *l2ty;
                l1->count    = 1;
                free(l2ty);

                d_test_handler_run_block(h, l1, NULL);
                res = d_test_handler_get_results(h);

                if (!d_assert_standalone(
                        ( (res.max_depth == 3) &&
                          (res.blocks_run == 3) ),
                        "three-level nesting",
                        "depth=3, blocks=3",
                        _test_info))
                {
                    all_ok = false;
                }

                d_test_block_free(l1);
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] nested_execution\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] nested_execution\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_config_override
  Tests runtime config override behavior.
  Tests the following:
  - NULL runtime config falls back to handler default
*/
bool
d_tests_sa_handler_config_override
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing config override ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL runtime config uses handler default
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            struct d_test* t;

            t = d_test_new(handler_test_passing, NULL);

            if (t)
            {
                if (!d_assert_standalone(
                        d_test_handler_run_test(h, t, NULL) == true,
                        "NULL runtime config",
                        "NULL config uses default",
                        _test_info))
                {
                    all_ok = false;
                }

                d_test_free(t);
            }

            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] config_override\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] config_override\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_run_module
  Tests d_test_handler_run_module behavior.
  Tests the following:
  - NULL handler and module returns false
  - NULL module with valid handler returns false
  - valid module with block increments modules_run
*/
bool
d_tests_sa_handler_run_module
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing d_test_handler_run_module ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL handler and module
    if (!d_assert_standalone(
            d_test_handler_run_module(NULL, NULL, NULL) == false,
            "run_module NULL params",
            "NULL returns false",
            _test_info))
    {
        all_ok = false;
    }

    // NULL module with valid handler
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            if (!d_assert_standalone(
                    d_test_handler_run_module(h, NULL, NULL) == false,
                    "run_module NULL module",
                    "NULL module returns false",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // valid module with block
    {
        struct d_test_handler* h;
        struct d_test_module*  m;

        h = d_test_handler_new(NULL);
        m = d_test_module_new("test_mod", NULL);

        if ( (h) && (m) )
        {
            struct d_test_block* b;

            b = d_test_block_new("blk", NULL);

            if (b)
            {
                struct d_test_result res;

                d_test_module_add_block(m, b);
                d_test_handler_run_module(h, m, NULL);
                res = d_test_handler_get_results(h);

                if (!d_assert_standalone(res.modules_run >= 1,
                                         "module execution counted",
                                         "modules_run incremented",
                                         _test_info))
                {
                    all_ok = false;
                }
            }

            d_test_module_free(m);
            d_test_handler_free(h);
        }
    }

    if (all_ok)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] run_module\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_module\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_handler_run_tree_and_session
  Tests d_test_handler_run_tree and d_test_handler_run_session with NULL
parameters.
  Tests the following:
  - NULL parameters for run_tree return false
  - NULL parameters for run_session return false
  - valid handler with NULL tree returns false
  - valid handler with NULL session returns false
*/
bool
d_tests_sa_handler_run_tree_and_session
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_ok;

    printf("  --- Testing run_tree and run_session ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_ok               = true;

    // NULL parameters for run_tree
    if (!d_assert_standalone(
            d_test_handler_run_tree(NULL, NULL, NULL) == false,
            "run_tree NULL",
            "NULL returns false",
            _test_info))
    {
        all_ok = false;
    }

    // NULL parameters for run_session
    if (!d_assert_standalone(
            d_test_handler_run_session(NULL, NULL) == false,
            "run_session NULL",
            "NULL returns false",
            _test_info))
    {
        all_ok = false;
    }

    // valid handler with NULL tree
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            if (!d_assert_standalone(
                    d_test_handler_run_tree(h, NULL, NULL) == false,
                    "run_tree NULL tree",
                    "NULL tree returns false",
                    _test_info))
            {
                all_ok = false;
            }

            d_test_handler_free(h);
        }
    }

    // valid handler with NULL session
    {
        struct d_test_handler* h;

        h = d_test_handler_new(NULL);

        if (h)
        {
            if (!d_assert_standalone(
                    d_test_handler_run_session(h, NULL) == false,
                    "run_session NULL session",
                    "NULL session returns false",
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
        printf("%s[PASS] run_tree_and_session\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_tree_and_session\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
