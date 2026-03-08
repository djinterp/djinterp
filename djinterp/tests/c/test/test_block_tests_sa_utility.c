#include "./test_block_tests_sa.h"


/******************************************************************************
 * UTILITY TESTS
 *****************************************************************************/

/*
d_tests_sa_test_block_print_valid
  Tests d_test_block_print with a valid block and prefix.
  Tests the following:
  - no crash when printing a valid block
  - function completes normally with a prefix string
*/
bool
d_tests_sa_test_block_print_valid
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // should print without crashing
    d_test_block_print(block, "  ", 2);

    all_passed &= d_assert_standalone(
        true,
        "print_valid: no crash",
        "printing a valid block with prefix should not crash",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_print_null
  Tests d_test_block_print with NULL block.
  Tests the following:
  - no crash when printing NULL block
  - function handles NULL gracefully (prints "NULL" message)
*/
bool
d_tests_sa_test_block_print_null
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    // should print a NULL indicator without crashing
    d_test_block_print(NULL, "  ", 2);

    all_passed &= d_assert_standalone(
        true,
        "print_null: no crash",
        "printing a NULL block should not crash",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_print_null_prefix
  Tests d_test_block_print with NULL prefix.
  Tests the following:
  - no crash when prefix is NULL
  - function defaults to empty prefix string
*/
bool
d_tests_sa_test_block_print_null_prefix
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // should default to "" prefix
    d_test_block_print(block, NULL, 0);

    all_passed &= d_assert_standalone(
        true,
        "print_null_prefix: no crash",
        "printing with NULL prefix should not crash",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_count_tests_empty
  Tests d_test_block_count_tests on an empty block.
  Tests the following:
  - returns 0 for a block with no children
*/
bool
d_tests_sa_test_block_count_tests_empty
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    size_t               count;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    count = d_test_block_count_tests(block);

    all_passed &= d_assert_standalone(
        count == 0,
        "count_tests_empty: result",
        "empty block should have 0 tests",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_count_tests_mixed
  Tests d_test_block_count_tests with mixed child types.
  Tests the following:
  - only children with D_TEST_TYPE_TEST are counted
  - assertions, test_fns, and blocks are not counted
*/
bool
d_tests_sa_test_block_count_tests_mixed
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test_block* child_block;
    struct d_test*       test1;
    struct d_test*       test2;
    struct d_assert*     assertion;
    struct d_test_type*  assert_type;
    size_t               count;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // add two tests
    test1 = d_test_new(NULL, 0);

    if (test1)
    {
        d_test_block_add_test(block, test1);
    }

    test2 = d_test_new(NULL, 0);

    if (test2)
    {
        d_test_block_add_test(block, test2);
    }

    // add an assertion (should NOT be counted as a test)
    assertion = d_assert_true(true, "pass", "fail");

    if (assertion)
    {
        assert_type = d_test_type_new(D_TEST_TYPE_ASSERT, assertion);

        if (assert_type)
        {
            d_test_block_add_child(block, assert_type);
        }
        else
        {
            d_assert_free(assertion);
        }
    }

    // add a nested block (should NOT be counted as a test)
    child_block = d_test_block_new(NULL, 0);

    if (child_block)
    {
        d_test_block_add_block(block, child_block);
    }

    count = d_test_block_count_tests(block);

    all_passed &= d_assert_standalone(
        count == 2,
        "count_tests_mixed: result",
        "should count only D_TEST_TYPE_TEST children (2)",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_count_tests_null
  Tests d_test_block_count_tests with NULL block.
  Tests the following:
  - returns 0 for NULL block
*/
bool
d_tests_sa_test_block_count_tests_null
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_assert_standalone(
        d_test_block_count_tests(NULL) == 0,
        "count_tests_null: result",
        "count_tests on NULL block should return 0",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_count_blocks_empty
  Tests d_test_block_count_blocks on an empty block.
  Tests the following:
  - returns 0 for a block with no children
*/
bool
d_tests_sa_test_block_count_blocks_empty
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    size_t               count;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    count = d_test_block_count_blocks(block);

    all_passed &= d_assert_standalone(
        count == 0,
        "count_blocks_empty: result",
        "empty block should have 0 child blocks",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_count_blocks_mixed
  Tests d_test_block_count_blocks with mixed child types.
  Tests the following:
  - only children with D_TEST_TYPE_TEST_BLOCK are counted
  - tests and assertions are not counted
*/
bool
d_tests_sa_test_block_count_blocks_mixed
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test_block* child1;
    struct d_test_block* child2;
    struct d_test_block* child3;
    struct d_test*       test;
    size_t               count;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // add three child blocks
    child1 = d_test_block_new(NULL, 0);
    child2 = d_test_block_new(NULL, 0);
    child3 = d_test_block_new(NULL, 0);

    if (child1)
    {
        d_test_block_add_block(block, child1);
    }

    if (child2)
    {
        d_test_block_add_block(block, child2);
    }

    if (child3)
    {
        d_test_block_add_block(block, child3);
    }

    // add a test (should NOT be counted as a block)
    test = d_test_new(NULL, 0);

    if (test)
    {
        d_test_block_add_test(block, test);
    }

    count = d_test_block_count_blocks(block);

    all_passed &= d_assert_standalone(
        count == 3,
        "count_blocks_mixed: result",
        "should count only D_TEST_TYPE_TEST_BLOCK children (3)",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_count_blocks_null
  Tests d_test_block_count_blocks with NULL block.
  Tests the following:
  - returns 0 for NULL block
*/
bool
d_tests_sa_test_block_count_blocks_null
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_assert_standalone(
        d_test_block_count_blocks(NULL) == 0,
        "count_blocks_null: result",
        "count_blocks on NULL block should return 0",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_utility_all
  Aggregation function for all utility tests.
  Tests the following:
  - d_test_block_print (valid, NULL block, NULL prefix)
  - d_test_block_count_tests (empty, mixed, NULL)
  - d_test_block_count_blocks (empty, mixed, NULL)
*/
bool
d_tests_sa_test_block_utility_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_test_block_print_valid(_test_info);
    all_passed &= d_tests_sa_test_block_print_null(_test_info);
    all_passed &= d_tests_sa_test_block_print_null_prefix(_test_info);
    all_passed &= d_tests_sa_test_block_count_tests_empty(_test_info);
    all_passed &= d_tests_sa_test_block_count_tests_mixed(_test_info);
    all_passed &= d_tests_sa_test_block_count_tests_null(_test_info);
    all_passed &= d_tests_sa_test_block_count_blocks_empty(_test_info);
    all_passed &= d_tests_sa_test_block_count_blocks_mixed(_test_info);
    all_passed &= d_tests_sa_test_block_count_blocks_null(_test_info);

    return all_passed;
}
