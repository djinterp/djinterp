/******************************************************************************
* djinterp [test]                                test_handler_tests_sa_stats.c
* 
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.05
******************************************************************************/
#include "./test_handler_tests_sa.h"


/*
d_tests_sa_handler_depth_tracking
  Tests depth tracking across block execution.
  Tests the following:
  - fresh handler has current_depth=0
  - current_depth resets to 0 after block run
  - max_depth=1 for single block
  - sequential blocks maintain correct depth
*/
bool
d_tests_sa_handler_depth_tracking
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
    bool                   seq_ok;

    printf("  --- Testing depth tracking ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    // single block depth tracking
    h = d_test_handler_new(NULL);

    if (h)
    {
        // test 1: fresh handler starts at depth 0
        result = d_assert_standalone(
            h->current_depth == 0,
            "fresh_depth_zero",
            "fresh handler should have current_depth=0",
            _test_info) && result;

        b = d_test_block_new(NULL, 0);

        if (b)
        {
            helper_add_passing_test_to_block(b);
            d_test_handler_run_block(h, b, NULL);

            // test 2: depth resets after execution
            result = d_assert_standalone(
                h->current_depth == 0,
                "depth_resets_after_run",
                "current_depth should reset to 0 after block run",
                _test_info) && result;

            r = d_test_handler_get_results(h);

            // test 3: max depth is 1 for single block
            result = d_assert_standalone(
                r.max_depth == 1,
                "max_depth_single_block",
                "single block should set max_depth=1",
                _test_info) && result;

            d_test_block_free(b);
        }

        d_test_handler_free(h);
    }

    // sequential blocks maintain correct depth
    h = d_test_handler_new(NULL);

    if (h)
    {
        seq_ok = true;

        for (i = 0; i < 3; i++)
        {
            b = d_test_block_new(NULL, 0);

            if (b)
            {
                helper_add_passing_test_to_block(b);
                d_test_handler_run_block(h, b, NULL);

                if (h->current_depth != 0)
                {
                    seq_ok = false;
                }

                d_test_block_free(b);
            }
        }

        // test 4: depth correct across sequential blocks
        result = d_assert_standalone(
            seq_ok,
            "sequential_depth_reset",
            "depth should be 0 after each sequential block",
            _test_info) && result;

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] depth_tracking\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] depth_tracking\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}

/*
d_tests_sa_handler_max_depth
  Tests max_depth tracking across varying nesting levels.
  Tests the following:
  - fresh handler has max_depth=0
  - shallow block sets max_depth=1
  - nested block increases max_depth to 2
  - max_depth tracks the highest value seen
  - reset clears max_depth to 0
*/
bool
d_tests_sa_handler_max_depth
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test_block*   sh;
    struct d_test_block*   outer;
    struct d_test_block*   inner;
    struct d_test_result   r;

    printf("  --- Testing max_depth ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    h = d_test_handler_new(NULL);

    if (h)
    {
        // test 1: fresh max_depth is 0
        r = d_test_handler_get_results(h);

        result = d_assert_standalone(
            r.max_depth == 0,
            "fresh_max_depth_zero",
            "fresh handler should have max_depth=0",
            _test_info) && result;

        // shallow block (depth 1)
        sh = d_test_block_new(NULL, 0);

        if (sh)
        {
            helper_add_passing_test_to_block(sh);
            d_test_handler_run_block(h, sh, NULL);
            d_test_block_free(sh);
        }

        // nested block (depth 2)
        outer = d_test_block_new(NULL, 0);
        inner = d_test_block_new(NULL, 0);

        if ( (outer) &&
             (inner) )
        {
            helper_add_passing_test_to_block(inner);
            helper_add_block_child_to_block(outer, inner);
            d_test_handler_run_block(h, outer, NULL);
            d_test_block_free(outer);
        }

        // test 2: max tracks highest depth seen
        r = d_test_handler_get_results(h);

        result = d_assert_standalone(
            r.max_depth == 2,
            "max_depth_tracks_highest",
            "max_depth should be 2 after shallow+nested",
            _test_info) && result;

        // test 3: reset clears max_depth
        d_test_handler_reset_results(h);
        r = d_test_handler_get_results(h);

        result = d_assert_standalone(
            r.max_depth == 0,
            "reset_clears_max_depth",
            "max_depth should be 0 after reset",
            _test_info) && result;

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] max_depth\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] max_depth\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}

/*
d_tests_sa_handler_block_counting
  Tests blocks_run counter accuracy.
  Tests the following:
  - fresh handler has blocks_run=0
  - empty block still increments blocks_run
  - 5 sequential blocks counted correctly
  - nested blocks count each level (outer+inner=2)
*/
bool
d_tests_sa_handler_block_counting
(
    struct d_test_counter* _test_info
)
{
    size_t                 ip;
    bool                   result;
    struct d_test_handler* h;
    struct d_test_block*   b;
    struct d_test_block*   outer;
    struct d_test_block*   inner;
    struct d_test_result   r;
    int                    i;

    printf("  --- Testing block counting ---\n");
    ip     = _test_info->tests_passed;
    result = true;

    h = d_test_handler_new(NULL);

    if (h)
    {
        // test 1: fresh handler has blocks_run=0
        r = d_test_handler_get_results(h);

        result = d_assert_standalone(
            r.blocks_run == 0,
            "fresh_blocks_zero",
            "fresh handler should have blocks_run=0",
            _test_info) && result;

        // test 2: empty block increments blocks_run
        b = d_test_block_new(NULL, 0);

        if (b)
        {
            d_test_handler_run_block(h, b, NULL);
            r = d_test_handler_get_results(h);

            result = d_assert_standalone(
                r.blocks_run == 1,
                "empty_block_counted",
                "empty block should increment blocks_run to 1",
                _test_info) && result;

            d_test_block_free(b);
        }

        // test 3: 5 sequential blocks counted correctly
        d_test_handler_reset_results(h);

        for (i = 0; i < 5; i++)
        {
            b = d_test_block_new(NULL, 0);

            if (b)
            {
                helper_add_passing_test_to_block(b);
                d_test_handler_run_block(h, b, NULL);
                d_test_block_free(b);
            }
        }

        r = d_test_handler_get_results(h);

        result = d_assert_standalone(
            r.blocks_run == 5,
            "five_sequential_blocks",
            "5 sequential blocks should give blocks_run=5",
            _test_info) && result;

        // test 4: nested blocks count each level
        d_test_handler_reset_results(h);
        outer = d_test_block_new(NULL, 0);
        inner = d_test_block_new(NULL, 0);

        if ( (outer) &&
             (inner) )
        {
            helper_add_passing_test_to_block(inner);
            helper_add_block_child_to_block(outer, inner);
            d_test_handler_run_block(h, outer, NULL);
            r = d_test_handler_get_results(h);

            result = d_assert_standalone(
                r.blocks_run == 2,
                "nested_blocks_both_counted",
                "outer+inner should give blocks_run=2",
                _test_info) && result;

            d_test_block_free(outer);
        }

        d_test_handler_free(h);
    }

    if (result)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] block_counting\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] block_counting\n", D_INDENT);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > ip);
}