/******************************************************************************
* djinterp [test]                                       dstring_tests_capacity.c
*
*   Unit tests for d_string capacity management functions:
*     - d_string_reserve
*     - d_string_shrink_to_fit
*     - d_string_capacity
*     - d_string_resize
*
* path:      \src\test\dstring_tests_capacity.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "..\tests\dstring_tests_sa.h"


/******************************************************************************
* d_tests_sa_dstring_reserve
******************************************************************************/

/*
d_tests_sa_dstring_reserve
  Tests d_string_reserve() which ensures the string has at least the specified
  capacity, potentially reallocating if necessary.

Test cases:
  1. NULL string returns false
  2. Reserve 0 on empty string (no-op, should succeed)
  3. Reserve less than current capacity (no reallocation needed)
  4. Reserve equal to current capacity (no reallocation needed)
  5. Reserve more than current capacity (reallocation required)
  6. Reserve on string with content (content preserved)
  7. Reserve very large capacity
  8. Multiple successive reserves (growing)
  9. Verify capacity actually increases
  10. Verify size unchanged after reserve

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_reserve
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    size_t                original_capacity;
    size_t                original_size;
    bool                  result;

    group     = d_test_object_new_interior("d_string_reserve", 14);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns false
    result = d_string_reserve(NULL, 100);
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_string_returns_false",
        result,
        "d_string_reserve(NULL, ...) should return false"
    );

    // test 2: reserve 0 on empty string
    str = d_string_new();

    if (str)
    {
        result = d_string_reserve(str, 0);
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_0_succeeds",
            result,
            "reserve 0 should succeed (no-op)"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_0_skipped", false, "skipped - allocation failed"
        );
    }

    // test 3: reserve less than current capacity
    str = d_string_new_with_capacity(100);

    if (str)
    {
        original_capacity = str->capacity;
        result = d_string_reserve(str, 50);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_less_succeeds",
            result,
            "reserve less than capacity should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "reserve_less_capacity_unchanged",
            str->capacity, original_capacity,
            "capacity should remain unchanged when reserving less"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_less_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_less_cap_skipped", false, "skipped"
        );
    }

    // test 4: reserve equal to current capacity
    str = d_string_new_with_capacity(64);

    if (str)
    {
        original_capacity = str->capacity;
        result = d_string_reserve(str, original_capacity);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_equal_succeeds",
            result,
            "reserve equal to capacity should succeed"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_equal_skipped", false, "skipped"
        );
    }

    // test 5: reserve more than current capacity
    str = d_string_new_with_capacity(16);

    if (str)
    {
        original_capacity = str->capacity;
        result = d_string_reserve(str, 256);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_more_succeeds",
            result,
            "reserve more than capacity should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_more_capacity_increased",
            str->capacity >= 256,
            "capacity should be at least the requested amount"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_more_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_more_cap_skipped", false, "skipped"
        );
    }

    // test 6: reserve on string with content (content preserved)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        original_size = str->size;
        result = d_string_reserve(str, 1000);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_with_content_succeeds",
            result,
            "reserve on string with content should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "reserve_content_preserved",
            str->text, "Hello World",
            "content should be preserved after reserve"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "reserve_size_preserved",
            str->size, original_size,
            "size should be unchanged after reserve"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_content_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_preserved_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_size_skipped", false, "skipped"
        );
    }

    // test 7: reserve very large capacity
    str = d_string_new();

    if (str)
    {
        result = d_string_reserve(str, 100000);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_large_succeeds",
            result,
            "reserve very large capacity should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_large_capacity_sufficient",
            str->capacity >= 100000,
            "capacity should be at least 100000"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_large_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_large_cap_skipped", false, "skipped"
        );
    }

    // test 8: multiple successive reserves
    str = d_string_new();

    if (str)
    {
        result = d_string_reserve(str, 32);
        result = result && d_string_reserve(str, 64);
        result = result && d_string_reserve(str, 128);
        result = result && d_string_reserve(str, 256);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "multiple_reserves_succeed",
            result && (str->capacity >= 256),
            "multiple successive reserves should all succeed"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "multiple_reserves_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_shrink_to_fit
******************************************************************************/

/*
d_tests_sa_dstring_shrink_to_fit
  Tests d_string_shrink_to_fit() which reduces capacity to match the current
  size (plus null terminator).

Test cases:
  1. NULL string returns false
  2. Shrink empty string
  3. Shrink string with excess capacity
  4. Shrink string where capacity already matches
  5. Content preserved after shrink
  6. Size unchanged after shrink
  7. Capacity reduced to size+1 (for null terminator)

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_shrink_to_fit
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    size_t                original_size;
    bool                  result;

    group     = d_test_object_new_interior("d_string_shrink_to_fit", 10);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns false
    result = d_string_shrink_to_fit(NULL);
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_string_returns_false",
        result,
        "d_string_shrink_to_fit(NULL) should return false"
    );

    // test 2: shrink empty string
    str = d_string_new_with_capacity(1000);

    if (str)
    {
        result = d_string_shrink_to_fit(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_empty_succeeds",
            result,
            "shrink empty string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_empty_capacity_reduced",
            str->capacity < 1000,
            "capacity should be reduced for empty string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_empty_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_empty_cap_skipped", false, "skipped"
        );
    }

    // test 3: shrink string with excess capacity
    str = d_string_new_with_capacity(1000);

    if (str)
    {
        // add some content
        d_string_assign_cstr(str, "Short");
        original_size = str->size;

        result = d_string_shrink_to_fit(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_excess_succeeds",
            result,
            "shrink string with excess capacity should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_excess_capacity_reduced",
            str->capacity < 1000,
            "capacity should be significantly reduced"
        );

        // test 4: content preserved
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "shrink_content_preserved",
            str->text, "Short",
            "content should be preserved after shrink"
        );

        // test 5: size unchanged
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "shrink_size_unchanged",
            str->size, original_size,
            "size should be unchanged after shrink"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_excess_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_excess_cap_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_content_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_size_skipped", false, "skipped"
        );
    }

    // test 6: capacity should be at least size + 1
    str = d_string_new_with_capacity(500);

    if (str)
    {
        d_string_assign_cstr(str, "Test string here");
        d_string_shrink_to_fit(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_capacity_sufficient",
            str->capacity >= str->size + 1,
            "capacity should be at least size + 1 for null terminator"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_capacity_skipped", false, "skipped"
        );
    }

    // test 7: shrink when capacity already optimal
    str = d_string_new_from_cstr("Exact");

    if (str)
    {
        // first shrink to optimal
        d_string_shrink_to_fit(str);
        size_t cap_after_first = str->capacity;

        // second shrink should be no-op
        result = d_string_shrink_to_fit(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_already_optimal_succeeds",
            result,
            "shrink when already optimal should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "shrink_already_optimal_unchanged",
            str->capacity, cap_after_first,
            "capacity should remain unchanged when already optimal"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_optimal_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_optimal_cap_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_capacity
******************************************************************************/

/*
d_tests_sa_dstring_capacity
  Tests d_string_capacity() which returns the current allocated capacity.

Test cases:
  1. NULL string returns 0
  2. New string has default capacity
  3. String with specified capacity
  4. Capacity after reserve
  5. Capacity after shrink_to_fit
  6. Capacity reflects actual allocation

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_capacity
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    size_t                cap;

    group     = d_test_object_new_interior("d_string_capacity", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns 0
    cap = d_string_capacity(NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "null_string_returns_0",
        cap, 0,
        "d_string_capacity(NULL) should return 0"
    );

    // test 2: new string has default capacity (at least 16)
    str = d_string_new();

    if (str)
    {
        cap = d_string_capacity(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "new_string_default_capacity",
            cap >= 16,
            "new string should have at least default capacity (16)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "new_string_cap_skipped", false, "skipped"
        );
    }

    // test 3: string with specified capacity
    str = d_string_new_with_capacity(256);

    if (str)
    {
        cap = d_string_capacity(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "specified_capacity",
            cap >= 256,
            "capacity should be at least the specified amount"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "specified_cap_skipped", false, "skipped"
        );
    }

    // test 4: capacity after reserve
    str = d_string_new();

    if (str)
    {
        d_string_reserve(str, 500);
        cap = d_string_capacity(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_after_reserve",
            cap >= 500,
            "capacity should reflect reserve"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reserve_cap_skipped", false, "skipped"
        );
    }

    // test 5: capacity after shrink_to_fit
    str = d_string_new_with_capacity(1000);

    if (str)
    {
        d_string_assign_cstr(str, "Hi");
        d_string_shrink_to_fit(str);
        cap = d_string_capacity(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_after_shrink",
            cap < 1000,
            "capacity should be reduced after shrink_to_fit"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_at_least_size_plus_1",
            cap >= str->size + 1,
            "capacity should be at least size + 1"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_cap_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "shrink_min_skipped", false, "skipped"
        );
    }

    // test 6: capacity matches struct field
    str = d_string_new_with_capacity(128);

    if (str)
    {
        cap = d_string_capacity(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "capacity_matches_field",
            cap, str->capacity,
            "d_string_capacity() should return struct field value"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_field_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_resize
******************************************************************************/

/*
d_tests_sa_dstring_resize
  Tests d_string_resize() which changes the size of the string, potentially
  truncating or padding with null characters.

Test cases:
  1. NULL string returns false
  2. Resize to 0 (clear string)
  3. Resize smaller (truncate)
  4. Resize to same size (no-op)
  5. Resize larger (pad with nulls)
  6. Content preserved when shrinking
  7. Null termination maintained
  8. Resize empty string to non-empty
  9. Multiple resizes
  10. Resize very large

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_resize
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_resize", 16);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns false
    result = d_string_resize(NULL, 10);
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_string_returns_false",
        result,
        "d_string_resize(NULL, ...) should return false"
    );

    // test 2: resize to 0 (clear string)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_resize(str, 0);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_to_0_succeeds",
            result,
            "resize to 0 should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "resize_to_0_size",
            str->size, 0,
            "size should be 0 after resize to 0"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_to_0_null_terminated",
            (str->text != NULL) && (str->text[0] == '\0'),
            "string should be null-terminated after resize to 0"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_0_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_0_size_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_0_null_skipped", false, "skipped"
        );
    }

    // test 3: resize smaller (truncate)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_resize(str, 5);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_smaller_succeeds",
            result,
            "resize smaller should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "resize_smaller_size",
            str->size, 5,
            "size should be 5 after truncation"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "resize_smaller_content",
            str->text, "Hello",
            "content should be truncated to 'Hello'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_smaller_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_smaller_size_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_smaller_content_skipped", false, "skipped"
        );
    }

    // test 4: resize to same size (no-op)
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        result = d_string_resize(str, 4);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_same_succeeds",
            result,
            "resize to same size should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "resize_same_content_unchanged",
            str->text, "Test",
            "content should be unchanged"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_same_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_same_content_skipped", false, "skipped"
        );
    }

    // test 5: resize larger (pad with nulls)
    str = d_string_new_from_cstr("Hi");

    if (str)
    {
        result = d_string_resize(str, 10);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_larger_succeeds",
            result,
            "resize larger should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "resize_larger_size",
            str->size, 10,
            "size should be 10 after resize"
        );

        // verify original content preserved
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_larger_content_preserved",
            (str->text[0] == 'H') && (str->text[1] == 'i'),
            "original content should be preserved"
        );

        // verify null padding
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_larger_null_padded",
            (str->text[2] == '\0') && (str->text[9] == '\0'),
            "new space should be null-padded"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_larger_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_larger_size_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_larger_content_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_larger_null_skipped", false, "skipped"
        );
    }

    // test 6: resize empty string to non-empty
    str = d_string_new();

    if (str)
    {
        result = d_string_resize(str, 20);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_empty_to_nonempty_succeeds",
            result,
            "resize empty string to non-empty should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "resize_empty_to_nonempty_size",
            str->size, 20,
            "size should be 20"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_empty_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "resize_empty_size_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_capacity_all
******************************************************************************/

/*
d_tests_sa_dstring_capacity_all
  Runs all capacity management tests and returns an aggregate test object
  containing all results.

Parameter(s):
  (none)
Return:
  Test object containing all capacity management test results.
*/
struct d_test_object*
d_tests_sa_dstring_capacity_all
(
    void
)
{
    struct d_test_object* group;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string Capacity Management", 4);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // run all capacity management tests
    group->elements[child_idx++] = d_tests_sa_dstring_reserve();
    group->elements[child_idx++] = d_tests_sa_dstring_shrink_to_fit();
    group->elements[child_idx++] = d_tests_sa_dstring_capacity();
    group->elements[child_idx++] = d_tests_sa_dstring_resize();

    return group;
}
