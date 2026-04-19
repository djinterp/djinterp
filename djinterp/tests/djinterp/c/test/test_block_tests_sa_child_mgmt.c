#include "../../../tests/c/test/test_block_tests_sa.h"


/******************************************************************************
 * CHILD MANAGEMENT TESTS
 *****************************************************************************/

/*
d_tests_sa_test_block_add_child_valid
  Tests d_test_block_add_child with valid block and child.
  Tests the following:
  - returns true on success
  - child count increments by 1
  - added child is retrievable
*/
bool
d_tests_sa_test_block_add_child_valid
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_assert*     assertion;
    struct d_test_type*  child_type;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    assertion = d_assert_true(true, "pass", "fail");

    if (!assertion)
    {
        d_test_block_free(block);

        return false;
    }

    child_type = d_test_type_new(D_TEST_TYPE_ASSERT, assertion);

    if (!child_type)
    {
        d_assert_free(assertion);
        d_test_block_free(block);

        return false;
    }

    result = d_test_block_add_child(block, child_type);

    all_passed &= d_assert_standalone(
        result == true,
        "add_child_valid: return value",
        "add_child should return true on success",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 1,
        "add_child_valid: child count",
        "child count should be 1 after adding one child",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_get_child_at(block, 0) == child_type,
        "add_child_valid: retrieval",
        "retrieved child should match the one added",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_add_child_null_block
  Tests d_test_block_add_child with NULL block.
  Tests the following:
  - returns false when block is NULL
*/
bool
d_tests_sa_test_block_add_child_null_block
(
    struct d_test_counter* _test_info
)
{
    struct d_assert*    assertion;
    struct d_test_type* child_type;
    bool                result;
    bool                all_passed;

    all_passed = true;

    assertion = d_assert_true(true, "pass", "fail");

    if (!assertion)
    {
        return false;
    }

    child_type = d_test_type_new(D_TEST_TYPE_ASSERT, assertion);

    if (!child_type)
    {
        d_assert_free(assertion);

        return false;
    }

    result = d_test_block_add_child(NULL, child_type);

    all_passed &= d_assert_standalone(
        result == false,
        "add_child_null_block: return value",
        "add_child with NULL block should return false",
        _test_info);

    // cleanup: child was not added, we must free it
    d_test_type_free(child_type);

    return all_passed;
}

/*
d_tests_sa_test_block_add_child_null_child
  Tests d_test_block_add_child with NULL child.
  Tests the following:
  - returns false when child is NULL
  - block state is unchanged
*/
bool
d_tests_sa_test_block_add_child_null_child
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    result = d_test_block_add_child(block, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "add_child_null_child: return value",
        "add_child with NULL child should return false",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 0,
        "add_child_null_child: count unchanged",
        "child count should remain 0",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_add_test_valid
  Tests d_test_block_add_test with valid block and test.
  Tests the following:
  - returns true on success
  - child count increments
  - child type is D_TEST_TYPE_TEST
*/
bool
d_tests_sa_test_block_add_test_valid
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test*       test;
    struct d_test_type*  retrieved;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    test = d_test_new(NULL, 0);

    if (!test)
    {
        d_test_block_free(block);

        return false;
    }

    result = d_test_block_add_test(block, test);

    all_passed &= d_assert_standalone(
        result == true,
        "add_test_valid: return value",
        "add_test should return true on success",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 1,
        "add_test_valid: child count",
        "child count should be 1",
        _test_info);

    // verify the wrapped child has the correct type tag
    retrieved = d_test_block_get_child_at(block, 0);

    all_passed &= d_assert_standalone(
        retrieved != NULL,
        "add_test_valid: retrieval non-NULL",
        "retrieved child should be non-NULL",
        _test_info);

    all_passed &= d_assert_standalone(
        retrieved != NULL && retrieved->type == D_TEST_TYPE_TEST,
        "add_test_valid: type tag",
        "wrapped child type should be D_TEST_TYPE_TEST",
        _test_info);

    // block owns the test now via the wrapper
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_add_test_null_block
  Tests d_test_block_add_test with NULL block.
  Tests the following:
  - returns false when block is NULL
*/
bool
d_tests_sa_test_block_add_test_null_block
(
    struct d_test_counter* _test_info
)
{
    struct d_test* test;
    bool           result;
    bool           all_passed;

    all_passed = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return false;
    }

    result = d_test_block_add_test(NULL, test);

    all_passed &= d_assert_standalone(
        result == false,
        "add_test_null_block: return value",
        "add_test with NULL block should return false",
        _test_info);

    // test was not consumed; free it ourselves
    d_test_free(test);

    return all_passed;
}

/*
d_tests_sa_test_block_add_test_null_test
  Tests d_test_block_add_test with NULL test.
  Tests the following:
  - returns false when test is NULL
*/
bool
d_tests_sa_test_block_add_test_null_test
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    result = d_test_block_add_test(block, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "add_test_null_test: return value",
        "add_test with NULL test should return false",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_add_block_valid
  Tests d_test_block_add_block with valid parent and child blocks.
  Tests the following:
  - returns true on success
  - parent child count increments
  - child type tag is D_TEST_TYPE_TEST_BLOCK
*/
bool
d_tests_sa_test_block_add_block_valid
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* parent;
    struct d_test_block* child;
    struct d_test_type*  retrieved;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    parent = d_test_block_new(NULL, 0);

    if (!parent)
    {
        return false;
    }

    child = d_test_block_new(NULL, 0);

    if (!child)
    {
        d_test_block_free(parent);

        return false;
    }

    result = d_test_block_add_block(parent, child);

    all_passed &= d_assert_standalone(
        result == true,
        "add_block_valid: return value",
        "add_block should return true on success",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(parent) == 1,
        "add_block_valid: child count",
        "parent child count should be 1",
        _test_info);

    // verify wrapped child type
    retrieved = d_test_block_get_child_at(parent, 0);

    all_passed &= d_assert_standalone(
        retrieved != NULL && retrieved->type == D_TEST_TYPE_TEST_BLOCK,
        "add_block_valid: type tag",
        "wrapped child type should be D_TEST_TYPE_TEST_BLOCK",
        _test_info);

    // parent owns the child block via wrapper
    d_test_block_free(parent);

    return all_passed;
}

/*
d_tests_sa_test_block_add_block_null_parent
  Tests d_test_block_add_block with NULL parent.
  Tests the following:
  - returns false when parent is NULL
*/
bool
d_tests_sa_test_block_add_block_null_parent
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* child;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    child = d_test_block_new(NULL, 0);

    if (!child)
    {
        return false;
    }

    result = d_test_block_add_block(NULL, child);

    all_passed &= d_assert_standalone(
        result == false,
        "add_block_null_parent: return value",
        "add_block with NULL parent should return false",
        _test_info);

    // child was not consumed
    d_test_block_free(child);

    return all_passed;
}

/*
d_tests_sa_test_block_add_block_null_child
  Tests d_test_block_add_block with NULL child.
  Tests the following:
  - returns false when child block is NULL
*/
bool
d_tests_sa_test_block_add_block_null_child
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* parent;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    parent = d_test_block_new(NULL, 0);

    if (!parent)
    {
        return false;
    }

    result = d_test_block_add_block(parent, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "add_block_null_child: return value",
        "add_block with NULL child should return false",
        _test_info);

    d_test_block_free(parent);

    return all_passed;
}

/*
d_tests_sa_test_block_child_count_valid
  Tests d_test_block_child_count with multiple children added.
  Tests the following:
  - count reflects the actual number of children
  - count updates correctly after each addition
*/
bool
d_tests_sa_test_block_child_count_valid
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test*       test1;
    struct d_test*       test2;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 0,
        "child_count_valid: initial",
        "initial child count should be 0",
        _test_info);

    test1 = d_test_new(NULL, 0);
    test2 = d_test_new(NULL, 0);

    if (test1)
    {
        d_test_block_add_test(block, test1);
    }

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 1,
        "child_count_valid: after first add",
        "child count should be 1 after first add",
        _test_info);

    if (test2)
    {
        d_test_block_add_test(block, test2);
    }

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 2,
        "child_count_valid: after second add",
        "child count should be 2 after second add",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_child_count_null
  Tests d_test_block_child_count with NULL block.
  Tests the following:
  - returns 0 for NULL block
*/
bool
d_tests_sa_test_block_child_count_null
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_assert_standalone(
        d_test_block_child_count(NULL) == 0,
        "child_count_null: result",
        "child count of NULL block should be 0",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_child_count_empty
  Tests d_test_block_child_count on a freshly created empty block.
  Tests the following:
  - empty block has child count of 0
*/
bool
d_tests_sa_test_block_child_count_empty
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

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 0,
        "child_count_empty: result",
        "freshly created block should have 0 children",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_get_child_at_valid
  Tests d_test_block_get_child_at with valid index.
  Tests the following:
  - returns correct child at each index
  - multiple children are stored in order
*/
bool
d_tests_sa_test_block_get_child_at_valid
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_assert*     a1;
    struct d_assert*     a2;
    struct d_test_type*  t1;
    struct d_test_type*  t2;
    struct d_test_type*  children[2];
    bool                 all_passed;

    all_passed = true;

    // create two distinct assertions
    a1 = d_assert_true(true, "first_pass", "first_fail");
    a2 = d_assert_true(false, "second_pass", "second_fail");

    if ( (!a1) || (!a2) )
    {
        d_assert_free(a1);
        d_assert_free(a2);

        return false;
    }

    t1 = d_test_type_new(D_TEST_TYPE_ASSERT, a1);
    t2 = d_test_type_new(D_TEST_TYPE_ASSERT, a2);

    if ( (!t1) || (!t2) )
    {
        if (t1) { d_test_type_free(t1); } else { d_assert_free(a1); }
        if (t2) { d_test_type_free(t2); } else { d_assert_free(a2); }

        return false;
    }

    children[0] = t1;
    children[1] = t2;

    block = d_test_block_new(children, 2);

    if (!block)
    {
        d_test_type_free(t1);
        d_test_type_free(t2);

        return false;
    }

    // verify order is preserved
    all_passed &= d_assert_standalone(
        d_test_block_get_child_at(block, 0) == t1,
        "get_child_at_valid: index 0",
        "child at index 0 should match first child added",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_get_child_at(block, 1) == t2,
        "get_child_at_valid: index 1",
        "child at index 1 should match second child added",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_get_child_at_null_block
  Tests d_test_block_get_child_at with NULL block.
  Tests the following:
  - returns NULL for NULL block
*/
bool
d_tests_sa_test_block_get_child_at_null_block
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_assert_standalone(
        d_test_block_get_child_at(NULL, 0) == NULL,
        "get_child_at_null: result",
        "get_child_at with NULL block should return NULL",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_get_child_at_out_of_bounds
  Tests d_test_block_get_child_at with out-of-bounds index.
  Tests the following:
  - returns NULL for index >= child count
*/
bool
d_tests_sa_test_block_get_child_at_out_of_bounds
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test_type*  result;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return false;
    }

    // empty block, any index is out of bounds
    result = d_test_block_get_child_at(block, 0);

    all_passed &= d_assert_standalone(
        result == NULL,
        "get_child_at_oob: index 0 on empty",
        "out-of-bounds index should return NULL",
        _test_info);

    result = d_test_block_get_child_at(block, 999);

    all_passed &= d_assert_standalone(
        result == NULL,
        "get_child_at_oob: index 999",
        "large out-of-bounds index should return NULL",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_child_mgmt_all
  Aggregation function for all child management tests.
  Tests the following:
  - d_test_block_add_child (valid, NULL block, NULL child)
  - d_test_block_add_test (valid, NULL block, NULL test)
  - d_test_block_add_block (valid, NULL parent, NULL child)
  - d_test_block_child_count (valid, NULL, empty)
  - d_test_block_get_child_at (valid, NULL, out-of-bounds)
*/
bool
d_tests_sa_test_block_child_mgmt_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_test_block_add_child_valid(_test_info);
    all_passed &= d_tests_sa_test_block_add_child_null_block(_test_info);
    all_passed &= d_tests_sa_test_block_add_child_null_child(_test_info);
    all_passed &= d_tests_sa_test_block_add_test_valid(_test_info);
    all_passed &= d_tests_sa_test_block_add_test_null_block(_test_info);
    all_passed &= d_tests_sa_test_block_add_test_null_test(_test_info);
    all_passed &= d_tests_sa_test_block_add_block_valid(_test_info);
    all_passed &= d_tests_sa_test_block_add_block_null_parent(_test_info);
    all_passed &= d_tests_sa_test_block_add_block_null_child(_test_info);
    all_passed &= d_tests_sa_test_block_child_count_valid(_test_info);
    all_passed &= d_tests_sa_test_block_child_count_null(_test_info);
    all_passed &= d_tests_sa_test_block_child_count_empty(_test_info);
    all_passed &= d_tests_sa_test_block_get_child_at_valid(_test_info);
    all_passed &= d_tests_sa_test_block_get_child_at_null_block(_test_info);
    all_passed &= d_tests_sa_test_block_get_child_at_out_of_bounds(_test_info);

    return all_passed;
}
