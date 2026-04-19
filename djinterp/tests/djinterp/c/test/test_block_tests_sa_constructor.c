#include "./test_block_tests_sa.h"


/******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR TESTS
 *****************************************************************************/

/*
d_tests_sa_test_block_new_no_children
  Tests d_test_block_new with no children (NULL array, 0 count).
  Tests the following:
  - block is created successfully (non-NULL)
  - child count is zero
  - children vector is initialized
*/
bool
d_tests_sa_test_block_new_no_children
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 all_passed;

    all_passed = true;

    // create block with no children
    block = d_test_block_new(NULL, 0);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_no_children: allocation",
        "block should be non-NULL after creation with no children",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 0,
        "new_no_children: child count",
        "child count should be 0 for block with no children",
        _test_info);

    // cleanup
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_new_with_children
  Tests d_test_block_new with a valid child array.
  Tests the following:
  - block is created with correct child count
  - child can be retrieved at index 0
  - child type tag is preserved
*/
bool
d_tests_sa_test_block_new_with_children
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_assert*     assertion;
    struct d_test_type*  child_type;
    struct d_test_type*  children[1];
    bool                 all_passed;

    all_passed = true;

    // create an assertion to wrap as a child
    assertion = d_assert_true(true, "pass", "fail");

    all_passed &= d_assert_standalone(
        assertion != NULL,
        "new_with_children: assertion alloc",
        "helper assertion should be non-NULL",
        _test_info);

    if (!assertion)
    {
        return false;
    }

    // wrap assertion in a test type
    child_type = d_test_type_new(D_TEST_TYPE_ASSERT, assertion);

    all_passed &= d_assert_standalone(
        child_type != NULL,
        "new_with_children: test_type alloc",
        "test type wrapper should be non-NULL",
        _test_info);

    if (!child_type)
    {
        d_assert_free(assertion);

        return false;
    }

    children[0] = child_type;

    // create block with one child
    block = d_test_block_new(children, 1);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_with_children: block alloc",
        "block should be non-NULL",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 1,
        "new_with_children: child count",
        "block should have exactly 1 child",
        _test_info);

    // verify the child is retrievable and has correct type
    all_passed &= d_assert_standalone(
        d_test_block_get_child_at(block, 0) != NULL,
        "new_with_children: child retrieval",
        "child at index 0 should be non-NULL",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_get_child_at(block, 0)->type == D_TEST_TYPE_ASSERT,
        "new_with_children: child type",
        "child type should be D_TEST_TYPE_ASSERT",
        _test_info);

    // cleanup (block owns children)
    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_new_null_children_zero_count
  Tests d_test_block_new with NULL children array and 0 count.
  Tests the following:
  - block is created successfully (valid edge case)
  - child count is zero
*/
bool
d_tests_sa_test_block_new_null_children_zero_count
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new(NULL, 0);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_null_zero: allocation",
        "NULL children with 0 count should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 0,
        "new_null_zero: child count",
        "child count should be 0",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_new_null_children_nonzero_count
  Tests d_test_block_new with NULL children array and nonzero count.
  Tests the following:
  - block creation fails gracefully (internal add_children rejects)
  - returns NULL because add_children fails with NULL + nonzero
*/
bool
d_tests_sa_test_block_new_null_children_nonzero_count
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 all_passed;

    all_passed = true;

    // NULL children with nonzero count triggers the internal guard
    block = d_test_block_new(NULL, 5);

    all_passed &= d_assert_standalone(
        block == NULL,
        "new_null_nonzero: rejection",
        "NULL children with nonzero count should return NULL",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_new_with_null_child_entry
  Tests d_test_block_new where the children array contains a NULL entry.
  Tests the following:
  - NULL entries in the children array are silently skipped
  - block is still created successfully
  - child count reflects only non-NULL entries added
*/
bool
d_tests_sa_test_block_new_with_null_child_entry
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_assert*     assertion;
    struct d_test_type*  child_type;
    struct d_test_type*  children[3];
    bool                 all_passed;

    all_passed = true;

    // create one valid child, rest are NULL
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

    children[0] = NULL;
    children[1] = child_type;
    children[2] = NULL;

    block = d_test_block_new(children, 3);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_null_entry: allocation",
        "block with NULL entries should still be created",
        _test_info);

    // only the non-NULL child should have been added
    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 1,
        "new_null_entry: child count",
        "only 1 non-NULL child should have been added",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_new_args_basic
  Tests d_test_block_new_args with valid arguments and no children.
  Tests the following:
  - block is created successfully
  - block is non-NULL
  - child count is zero
  - args are not stored on block (handled externally)
*/
bool
d_tests_sa_test_block_new_args_basic
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test_arg    args[1];
    bool                 all_passed;

    all_passed = true;

    // use a known config key
    args[0].key   = "name";
    args[0].value = (const void*)"test_block_args";

    block = d_test_block_new_args(args, 1, NULL, 0);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_args_basic: allocation",
        "block created with args should be non-NULL",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 0,
        "new_args_basic: child count",
        "block with no children should have 0 count",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_new_args_no_args
  Tests d_test_block_new_args with NULL args and zero count.
  Tests the following:
  - block is created successfully even with no args
  - no crash on NULL args
*/
bool
d_tests_sa_test_block_new_args_no_args
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 all_passed;

    all_passed = true;

    block = d_test_block_new_args(NULL, 0, NULL, 0);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_args_no_args: allocation",
        "block with NULL args and 0 count should succeed",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_new_args_null_args_nonzero
  Tests d_test_block_new_args with NULL args but nonzero count.
  Tests the following:
  - the args are ignored (block delegates to d_test_block_new)
  - block is still created
*/
bool
d_tests_sa_test_block_new_args_null_args_nonzero
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    bool                 all_passed;

    all_passed = true;

    // NULL args with nonzero count - args are ignored by new_args
    block = d_test_block_new_args(NULL, 3, NULL, 0);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_args_null_nonzero: allocation",
        "NULL args with nonzero count should still create block",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_new_args_with_children
  Tests d_test_block_new_args with both args and children.
  Tests the following:
  - block is created with children
  - child count matches
*/
bool
d_tests_sa_test_block_new_args_with_children
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_test_arg    args[1];
    struct d_assert*     assertion;
    struct d_test_type*  child_type;
    struct d_test_type*  children[1];
    bool                 all_passed;

    all_passed = true;

    args[0].key   = "name";
    args[0].value = (const void*)"args_with_children_block";

    // create a child assertion
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

    children[0] = child_type;

    block = d_test_block_new_args(args, 1, children, 1);

    all_passed &= d_assert_standalone(
        block != NULL,
        "new_args_children: allocation",
        "block with args and children should be non-NULL",
        _test_info);

    all_passed &= d_assert_standalone(
        d_test_block_child_count(block) == 1,
        "new_args_children: child count",
        "block should have 1 child",
        _test_info);

    d_test_block_free(block);

    return all_passed;
}

/*
d_tests_sa_test_block_validate_args_null
  Tests d_test_block_validate_args with NULL args.
  Tests the following:
  - returns NULL for NULL args
*/
bool
d_tests_sa_test_block_validate_args_null
(
    struct d_test_counter* _test_info
)
{
    struct d_test_options* options;
    bool                   all_passed;

    all_passed = true;

    options = d_test_block_validate_args(NULL, 5);

    all_passed &= d_assert_standalone(
        options == NULL,
        "validate_args_null: result",
        "NULL args should return NULL options",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_validate_args_zero_count
  Tests d_test_block_validate_args with zero arg count.
  Tests the following:
  - returns NULL for zero count
*/
bool
d_tests_sa_test_block_validate_args_zero_count
(
    struct d_test_counter* _test_info
)
{
    struct d_test_options* options;
    struct d_test_arg      args[1];
    bool                   all_passed;

    all_passed = true;

    args[0].key   = "name";
    args[0].value = (const void*)"test";

    options = d_test_block_validate_args(args, 0);

    all_passed &= d_assert_standalone(
        options == NULL,
        "validate_args_zero: result",
        "zero arg count should return NULL options",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_validate_args_null_key_entry
  Tests d_test_block_validate_args with an arg that has a NULL key.
  Tests the following:
  - NULL key entries are skipped without crash
  - options is still returned (valid but with no applied entries)
*/
bool
d_tests_sa_test_block_validate_args_null_key_entry
(
    struct d_test_counter* _test_info
)
{
    struct d_test_options* options;
    struct d_test_arg      args[1];
    bool                   all_passed;

    all_passed = true;

    args[0].key   = NULL;
    args[0].value = (const void*)"ignored_value";

    options = d_test_block_validate_args(args, 1);

    // should return a valid options even if all keys were NULL
    all_passed &= d_assert_standalone(
        options != NULL,
        "validate_args_null_key: result",
        "options should be created even with NULL key entries",
        _test_info);

    if (options)
    {
        d_test_options_free(options);
    }

    return all_passed;
}

/*
d_tests_sa_test_block_free_null
  Tests d_test_block_free with NULL input.
  Tests the following:
  - no crash when freeing NULL
*/
bool
d_tests_sa_test_block_free_null
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    // should not crash
    d_test_block_free(NULL);

    all_passed &= d_assert_standalone(
        true,
        "free_null: no crash",
        "freeing NULL should not crash",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_free_valid
  Tests d_test_block_free with a fully populated block.
  Tests the following:
  - block with children is freed without leaks
  - no crash during recursive child cleanup
*/
bool
d_tests_sa_test_block_free_valid
(
    struct d_test_counter* _test_info
)
{
    struct d_test_block* block;
    struct d_assert*     assertion;
    struct d_test_type*  child_type;
    struct d_test_type*  children[1];
    bool                 all_passed;

    all_passed = true;

    // create a block with a child
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

    children[0] = child_type;

    block = d_test_block_new(children, 1);

    if (!block)
    {
        d_test_type_free(child_type);

        return false;
    }

    // free the populated block
    d_test_block_free(block);

    all_passed &= d_assert_standalone(
        true,
        "free_valid: no crash",
        "freeing a populated block should not crash",
        _test_info);

    return all_passed;
}

/*
d_tests_sa_test_block_constructor_all
  Aggregation function for all constructor/destructor tests.
  Tests the following:
  - d_test_block_new (multiple variants)
  - d_test_block_new_args (multiple variants)
  - d_test_block_validate_args (edge cases)
  - d_test_block_free (NULL and valid)
*/
bool
d_tests_sa_test_block_constructor_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_test_block_new_no_children(_test_info);
    all_passed &= d_tests_sa_test_block_new_with_children(_test_info);
    all_passed &= d_tests_sa_test_block_new_null_children_zero_count(_test_info);
    all_passed &= d_tests_sa_test_block_new_null_children_nonzero_count(_test_info);
    all_passed &= d_tests_sa_test_block_new_with_null_child_entry(_test_info);
    all_passed &= d_tests_sa_test_block_new_args_basic(_test_info);
    all_passed &= d_tests_sa_test_block_new_args_no_args(_test_info);
    all_passed &= d_tests_sa_test_block_new_args_null_args_nonzero(_test_info);
    all_passed &= d_tests_sa_test_block_new_args_with_children(_test_info);
    all_passed &= d_tests_sa_test_block_validate_args_null(_test_info);
    all_passed &= d_tests_sa_test_block_validate_args_zero_count(_test_info);
    all_passed &= d_tests_sa_test_block_validate_args_null_key_entry(_test_info);
    all_passed &= d_tests_sa_test_block_free_null(_test_info);
    all_passed &= d_tests_sa_test_block_free_valid(_test_info);

    return all_passed;
}
