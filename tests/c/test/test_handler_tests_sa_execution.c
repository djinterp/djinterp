/******************************************************************************
* djinterp [test]                              test_handler_tests_sa_execution.c
*
*   Unit tests for test_handler execution functions.
*
* author(s): Samuel 'teer' Neal-Blim                           date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_run_test
  Tests d_test_handler_run_test behavior.
  Tests the following:
  - NULL handler and test returns false
  - NULL test with valid handler returns false
  - passing test updates stats correctly
  - failing test updates stats correctly
  - accumulation across multiple tests (3 pass + 2 fail)
*/
bool
d_tests_sa_handler_run_test
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test*         t;
    struct d_test_result   res;
    bool                   r;
    int                    i;

    printf("  --- Testing d_test_handler_run_test ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: NULL handler and test returns false
    result = d_assert_standalone(
        d_test_handler_run_test(NULL, NULL, NULL) == false,
        "run_test_null_params",
        "NULL handler+test should return false",
        _test_info) && result;

    // test 2: NULL test with valid handler returns false
    h = d_test_handler_new(NULL);

    if (h)
    {
        result = d_assert_standalone(
            d_test_handler_run_test(h, NULL, NULL) == false,
            "run_test_null_test",
            "NULL test with valid handler should return false",
            _test_info) && result;

        d_test_handler_free(h);
    }

    // test 3: passing test updates stats correctly
    h = d_test_handler_new(NULL);

    if (h)
    {
        t = helper_make_passing_test();

        if (t)
        {
            r   = d_test_handler_run_test(h, t, NULL);
            res = d_test_handler_get_results(h);

            result = d_assert_standalone(
                ( (r == true)             &&
                  (res.tests_run == 1)    &&
                  (res.tests_passed == 1) &&
                  (res.tests_failed == 0) ),
                "passing_test_stats",
                "passing test should give run=1, passed=1, failed=0",
                _test_info) && result;

            d_test_free(t);
        }

        d_test_handler_free(h);
    }

    // test 4: failing test updates stats correctly
    h = d_test_handler_new(NULL);

    if (h)
    {
        t = helper_make_failing_test();

        if (t)
        {
            r   = d_test_handler_run_test(h, t, NULL);
            res = d_test_handler_get_results(h);

            result = d_assert_standalone(
                ( (r == false)            &&
                  (res.tests_run == 1)    &&
                  (res.tests_passed == 0) &&
                  (res.tests_failed == 1) ),
                "failing_test_stats",
                "failing test should give run=1, passed=0, failed=1",
                _test_info) && result;

            d_test_free(t);
        }

        d_test_handler_free(h);
    }

    // test 5: accumulation across 5 tests (3 pass + 2 fail)
    h = d_test_handler_new(NULL);

    if (h)
    {
        for (i = 0; i < 3; i++)
        {
            t = helper_make_passing_test();

            if (t)
            {
                d_test_handler_run_test(h, t, NULL);
                d_test_free(t);
            }
        }

        for (i = 0; i < 2; i++)
        {
            t = helper_make_failing_test();

            if (t)
            {
                d_test_handler_run_test(h, t, NULL);
                d_test_free(t);
            }
        }

        res = d_test_handler_get_results(h);

        result = d_assert_standalone(
            ( (res.tests_run == 5)    &&
              (res.tests_passed == 3) &&
              (res.tests_failed == 2) ),
            "accumulation_three_plus_two",
            "5 tests should give 3 passed and 2 failed",
            _test_info) && result;

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] run_test\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_test\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}

/*
d_tests_sa_handler_run_block
  Tests d_test_handler_run_block behavior.
  Tests the following:
  - NULL parameters return false
  - empty block succeeds and increments blocks_run
  - block with 3 passing tests reports correct stats
  - block with mixed results reports overall failure
  - depth tracking updates max_depth and resets current_depth
*/
bool
d_tests_sa_handler_run_block
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test_block*   b;
    struct d_test_result   res;
    bool                   r;
    int                    i;

    printf("  --- Testing d_test_handler_run_block ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: NULL parameters return false
    result = d_assert_standalone(
        d_test_handler_run_block(NULL, NULL, NULL) == false,
        "run_block_null_params",
        "NULL parameters should return false",
        _test_info) && result;

    // test 2: empty block succeeds and increments blocks_run
    h = d_test_handler_new(NULL);
    b = d_test_block_new(NULL, 0);

    if ( (h) &&
         (b) )
    {
        r   = d_test_handler_run_block(h, b, NULL);
        res = d_test_handler_get_results(h);

        result = d_assert_standalone(
            ( (r == true)           &&
              (res.blocks_run == 1) &&
              (res.tests_run == 0) ),
            "empty_block_succeeds",
            "empty block should succeed with blocks_run=1, tests_run=0",
            _test_info) && result;

        d_test_block_free(b);
    }

    if (h)
    {
        d_test_handler_free(h);
    }

    // test 3: block with 3 passing tests
    h = d_test_handler_new(NULL);
    b = d_test_block_new(NULL, 0);

    if ( (h) &&
         (b) )
    {
        for (i = 0; i < 3; i++)
        {
            helper_add_passing_test_to_block(b);
        }

        r   = d_test_handler_run_block(h, b, NULL);
        res = d_test_handler_get_results(h);

        result = d_assert_standalone(
            ( (r == true)             &&
              (res.tests_run == 3)    &&
              (res.tests_passed == 3) &&
              (res.blocks_run == 1) ),
            "block_three_passing",
            "block with 3 passing tests should all pass",
            _test_info) && result;

        d_test_block_free(b);
    }

    if (h)
    {
        d_test_handler_free(h);
    }

    // test 4: block with mixed results
    h = d_test_handler_new(NULL);
    b = d_test_block_new(NULL, 0);

    if ( (h) &&
         (b) )
    {
        helper_add_passing_test_to_block(b);
        helper_add_failing_test_to_block(b);

        r   = d_test_handler_run_block(h, b, NULL);
        res = d_test_handler_get_results(h);

        result = d_assert_standalone(
            ( (r == false)            &&
              (res.tests_run == 2)    &&
              (res.tests_passed == 1) &&
              (res.tests_failed == 1) ),
            "mixed_block_overall_false",
            "block with mixed results should return false",
            _test_info) && result;

        d_test_block_free(b);
    }

    if (h)
    {
        d_test_handler_free(h);
    }

    // test 5: depth tracking
    h = d_test_handler_new(NULL);
    b = d_test_block_new(NULL, 0);

    if ( (h) &&
         (b) )
    {
        helper_add_passing_test_to_block(b);
        d_test_handler_run_block(h, b, NULL);
        res = d_test_handler_get_results(h);

        result = d_assert_standalone(
            ( (res.max_depth >= 1)    &&
              (h->current_depth == 0) ),
            "block_depth_tracking",
            "max_depth should be >=1 and current_depth should reset to 0",
            _test_info) && result;

        d_test_block_free(b);
    }

    if (h)
    {
        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] run_block\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_block\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
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
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test*         t;
    struct d_test_type*    ty;
    struct d_test_type     dummy;
    bool                   r;

    printf("  --- Testing d_test_handler_run_test_type ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: NULL parameters return false
    result = d_assert_standalone(
        d_test_handler_run_test_type(NULL, NULL, NULL) == false,
        "run_test_type_null",
        "NULL parameters should return false",
        _test_info) && result;

    // test 2: test type wrapping a passing test
    h = d_test_handler_new(NULL);

    if (h)
    {
        t  = helper_make_passing_test();
        ty = d_test_type_new(D_TEST_TYPE_TEST, t);

        if (ty)
        {
            r = d_test_handler_run_test_type(h, ty, NULL);

            result = d_assert_standalone(
                r == true,
                "test_type_dispatches_test",
                "run_test_type should dispatch to run_test correctly",
                _test_info) && result;

            d_test_type_free(ty);
        }

        d_test_handler_free(h);
    }

    // test 3: unknown type returns false
    h = d_test_handler_new(NULL);

    if (h)
    {
        d_memset(&dummy, 0, sizeof(dummy));
        dummy.type = D_TEST_TYPE_UNKNOWN;

        result = d_assert_standalone(
            d_test_handler_run_test_type(h, &dummy, NULL) == false,
            "unknown_type_returns_false",
            "D_TEST_TYPE_UNKNOWN should return false",
            _test_info) && result;

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] run_test_type\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_test_type\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
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
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test_block*   outer;
    struct d_test_block*   inner;
    struct d_test_block*   l1;
    struct d_test_block*   l2;
    struct d_test_block*   l3;
    struct d_test_result   res;

    printf("  --- Testing nested execution ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: two-level nesting (outer -> inner -> test)
    h = d_test_handler_new(NULL);

    if (h)
    {
        outer = d_test_block_new(NULL, 0);
        inner = d_test_block_new(NULL, 0);

        if ( (outer) &&
             (inner) )
        {
            helper_add_passing_test_to_block(inner);
            helper_add_block_child_to_block(outer, inner);
            d_test_handler_run_block(h, outer, NULL);
            res = d_test_handler_get_results(h);

            result = d_assert_standalone(
                ( (res.max_depth == 2)    &&
                  (res.blocks_run == 2)   &&
                  (res.tests_run == 1)    &&
                  (h->current_depth == 0) ),
                "two_level_nesting",
                "should give depth=2, blocks=2, tests=1, "
                "current_depth=0",
                _test_info) && result;

            d_test_block_free(outer);
        }

        d_test_handler_free(h);
    }

    // test 2: three-level nesting (l1 -> l2 -> l3 -> test)
    h = d_test_handler_new(NULL);

    if (h)
    {
        l1 = d_test_block_new(NULL, 0);
        l2 = d_test_block_new(NULL, 0);
        l3 = d_test_block_new(NULL, 0);

        if ( (l1) &&
             (l2) &&
             (l3) )
        {
            helper_add_passing_test_to_block(l3);
            helper_add_block_child_to_block(l2, l3);
            helper_add_block_child_to_block(l1, l2);
            d_test_handler_run_block(h, l1, NULL);
            res = d_test_handler_get_results(h);

            result = d_assert_standalone(
                ( (res.max_depth == 3)  &&
                  (res.blocks_run == 3) ),
                "three_level_nesting",
                "should give depth=3, blocks=3",
                _test_info) && result;

            d_test_block_free(l1);
        }

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] nested_execution\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] nested_execution\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
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
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test*         t;

    printf("  --- Testing config override ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: NULL runtime config uses handler default
    h = d_test_handler_new(NULL);

    if (h)
    {
        t = helper_make_passing_test();

        if (t)
        {
            result = d_assert_standalone(
                d_test_handler_run_test(h, t, NULL) == true,
                "null_config_uses_default",
                "NULL config should fall back to handler default",
                _test_info) && result;

            d_test_free(t);
        }

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] config_override\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] config_override\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
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
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test_module*  m;
    struct d_test_block*   b;
    struct d_test_result   res;

    printf("  --- Testing d_test_handler_run_module ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: NULL handler and module returns false
    result = d_assert_standalone(
        d_test_handler_run_module(NULL, NULL, NULL) == false,
        "run_module_null_params",
        "NULL handler and module should return false",
        _test_info) && result;

    // test 2: NULL module with valid handler returns false
    h = d_test_handler_new(NULL);

    if (h)
    {
        result = d_assert_standalone(
            d_test_handler_run_module(h, NULL, NULL) == false,
            "run_module_null_module",
            "NULL module with valid handler should return false",
            _test_info) && result;

        d_test_handler_free(h);
    }

    // test 3: valid module with block increments modules_run
    h = d_test_handler_new(NULL);
    m = d_test_module_new("test_mod", NULL);

    if ( (h) &&
         (m) )
    {
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            d_test_module_add_child(m, b);
            d_test_handler_run_module(h, m, NULL);
            res = d_test_handler_get_results(h);

            result = d_assert_standalone(
                res.modules_run >= 1,
                "module_execution_counted",
                "modules_run should be incremented",
                _test_info) && result;
        }

        d_test_module_free(m);
        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] run_module\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_module\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
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
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;

    printf("  --- Testing run_tree and run_session ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // test 1: NULL parameters for run_tree
    result = d_assert_standalone(
        d_test_handler_run_tree(NULL, NULL, NULL) == false,
        "run_tree_null_params",
        "NULL parameters for run_tree should return false",
        _test_info) && result;

    // test 2: NULL parameters for run_session
    result = d_assert_standalone(
        d_test_handler_run_session(NULL, NULL) == false,
        "run_session_null_params",
        "NULL parameters for run_session should return false",
        _test_info) && result;

    // test 3: valid handler with NULL tree
    h = d_test_handler_new(NULL);

    if (h)
    {
        result = d_assert_standalone(
            d_test_handler_run_tree(h, NULL, NULL) == false,
            "run_tree_null_tree",
            "valid handler with NULL tree should return false",
            _test_info) && result;

        d_test_handler_free(h);
    }

    // test 4: valid handler with NULL session
    h = d_test_handler_new(NULL);

    if (h)
    {
        result = d_assert_standalone(
            d_test_handler_run_session(h, NULL) == false,
            "run_session_null_session",
            "valid handler with NULL session should return false",
            _test_info) && result;

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] run_tree_and_session\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] run_tree_and_session\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}