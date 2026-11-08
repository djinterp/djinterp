/*******************************************************************************
* djinterp [test]                                       dstring_tests_capacity.c
*
*   Unit tests for dj_string capacity management functions:
*     - d_string_reserve
*     - d_string_shrink_to_fit
*     - d_string_capacity
*     - d_string_resize
*
*
* path:      /tests/djinterp/c/dstring_tests_capacity.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "dstring_tests.h"


/******************************************************************************
* d_tests_dstring_reserve
******************************************************************************/

/*
d_tests_dstring_reserve
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
bool d_tests_dstring_reserve(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                original_capacity;
    size_t                original_size;
    bool                  result;

    d_test_suite_test(_suite, "d_string_reserve");

    // test 1: NULL string returns false
    result = d_string_reserve(NULL, 100);
    D_TEST_CHECK_MSG(_suite,
                     "null_string_returns_false",
                     !(result),
                     "d_string_reserve(NULL, ...) should return false");

    // test 2: reserve 0 on empty string
    str = d_string_new();

    if (str)
    {
        result = d_string_reserve(str, 0);
        D_TEST_CHECK_MSG(_suite,
                         "reserve_0_succeeds",
                         result,
                         "reserve 0 should succeed (no-op)");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "reserve_0_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 3: reserve less than current capacity
    str = d_string_new_with_capacity(100);

    if (str)
    {
        original_capacity = str->capacity;
        result = d_string_reserve(str, 50);

        D_TEST_CHECK_MSG(_suite,
                         "reserve_less_succeeds",
                         result,
                         "reserve less than capacity should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "reserve_less_capacity_unchanged",
                         (str->capacity) == (original_capacity),
                         "capacity should remain unchanged when reserving less");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "reserve_less_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "reserve_less_cap_skipped", false, "skipped");
    }

    // test 4: reserve equal to current capacity
    str = d_string_new_with_capacity(64);

    if (str)
    {
        original_capacity = str->capacity;
        result = d_string_reserve(str, original_capacity);

        D_TEST_CHECK_MSG(_suite,
                         "reserve_equal_succeeds",
                         result,
                         "reserve equal to capacity should succeed");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "reserve_equal_skipped", false, "skipped");
    }

    // test 5: reserve more than current capacity
    str = d_string_new_with_capacity(16);

    if (str)
    {
        original_capacity = str->capacity;
        result = d_string_reserve(str, 256);

        D_TEST_CHECK_MSG(_suite,
                         "reserve_more_succeeds",
                         result,
                         "reserve more than capacity should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "reserve_more_capacity_increased",
                         str->capacity >= 256,
                         "capacity should be at least the requested amount");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "reserve_more_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "reserve_more_cap_skipped", false, "skipped");
    }

    // test 6: reserve on string with content (content preserved)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        original_size = str->size;
        result = d_string_reserve(str, 1000);

        D_TEST_CHECK_MSG(_suite,
                         "reserve_with_content_succeeds",
                         result,
                         "reserve on string with content should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "reserve_content_preserved",
                         d_test_suite_str_equal((str->text), ("Hello World")),
                         "content should be preserved after reserve");

        D_TEST_CHECK_MSG(_suite,
                         "reserve_size_preserved",
                         (str->size) == (original_size),
                         "size should be unchanged after reserve");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "reserve_content_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "reserve_preserved_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "reserve_size_skipped", false, "skipped");
    }

    // test 7: reserve very large capacity
    str = d_string_new();

    if (str)
    {
        result = d_string_reserve(str, 100000);

        D_TEST_CHECK_MSG(_suite,
                         "reserve_large_succeeds",
                         result,
                         "reserve very large capacity should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "reserve_large_capacity_sufficient",
                         str->capacity >= 100000,
                         "capacity should be at least 100000");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "reserve_large_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "reserve_large_cap_skipped", false, "skipped");
    }

    // test 8: multiple successive reserves
    str = d_string_new();

    if (str)
    {
        result = d_string_reserve(str, 32);
        result = result && d_string_reserve(str, 64);
        result = result && d_string_reserve(str, 128);
        result = result && d_string_reserve(str, 256);

        D_TEST_CHECK_MSG(_suite,
                         "multiple_reserves_succeed",
                         result && (str->capacity >= 256),
                         "multiple successive reserves should all succeed");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "multiple_reserves_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_shrink_to_fit
******************************************************************************/

/*
d_tests_dstring_shrink_to_fit
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
bool d_tests_dstring_shrink_to_fit(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                original_size;
    bool                  result;

    d_test_suite_test(_suite, "d_string_shrink_to_fit");

    // test 1: NULL string returns false
    result = d_string_shrink_to_fit(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_string_returns_false",
                     !(result),
                     "d_string_shrink_to_fit(NULL) should return false");

    // test 2: shrink empty string
    str = d_string_new_with_capacity(1000);

    if (str)
    {
        result = d_string_shrink_to_fit(str);

        D_TEST_CHECK_MSG(_suite,
                         "shrink_empty_succeeds",
                         result,
                         "shrink empty string should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "shrink_empty_capacity_reduced",
                         str->capacity < 1000,
                         "capacity should be reduced for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "shrink_empty_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "shrink_empty_cap_skipped", false, "skipped");
    }

    // test 3: shrink string with excess capacity
    str = d_string_new_with_capacity(1000);

    if (str)
    {
        // add some content
        d_string_assign_cstr(str, "Short");
        original_size = str->size;

        result = d_string_shrink_to_fit(str);

        D_TEST_CHECK_MSG(_suite,
                         "shrink_excess_succeeds",
                         result,
                         "shrink string with excess capacity should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "shrink_excess_capacity_reduced",
                         str->capacity < 1000,
                         "capacity should be significantly reduced");

        // test 4: content preserved
        D_TEST_CHECK_MSG(_suite,
                         "shrink_content_preserved",
                         d_test_suite_str_equal((str->text), ("Short")),
                         "content should be preserved after shrink");

        // test 5: size unchanged
        D_TEST_CHECK_MSG(_suite,
                         "shrink_size_unchanged",
                         (str->size) == (original_size),
                         "size should be unchanged after shrink");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "shrink_excess_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "shrink_excess_cap_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "shrink_content_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "shrink_size_skipped", false, "skipped");
    }

    // test 6: capacity should be at least size + 1
    str = d_string_new_with_capacity(500);

    if (str)
    {
        d_string_assign_cstr(str, "Test string here");
        d_string_shrink_to_fit(str);

        D_TEST_CHECK_MSG(_suite,
                         "shrink_capacity_sufficient",
                         str->capacity >= str->size + 1,
                         "capacity should be at least size + 1 for null terminator");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "shrink_capacity_skipped", false, "skipped");
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

        D_TEST_CHECK_MSG(_suite,
                         "shrink_already_optimal_succeeds",
                         result,
                         "shrink when already optimal should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "shrink_already_optimal_unchanged",
                         (str->capacity) == (cap_after_first),
                         "capacity should remain unchanged when already optimal");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "shrink_optimal_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite,
                         "shrink_optimal_cap_skipped",
                         false,
                         "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_capacity
******************************************************************************/

/*
d_tests_dstring_capacity
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
bool d_tests_dstring_capacity(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                cap;

    d_test_suite_test(_suite, "d_string_capacity");

    // test 1: NULL string returns 0
    cap = d_string_capacity(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_string_returns_0",
                     (cap) == (0),
                     "d_string_capacity(NULL) should return 0");

    // test 2: new string has default capacity (at least 16)
    str = d_string_new();

    if (str)
    {
        cap = d_string_capacity(str);

        D_TEST_CHECK_MSG(_suite,
                         "new_string_default_capacity",
                         cap >= 16,
                         "new string should have at least default capacity (16)");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "new_string_cap_skipped", false, "skipped");
    }

    // test 3: string with specified capacity
    str = d_string_new_with_capacity(256);

    if (str)
    {
        cap = d_string_capacity(str);

        D_TEST_CHECK_MSG(_suite,
                         "specified_capacity",
                         cap >= 256,
                         "capacity should be at least the specified amount");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "specified_cap_skipped", false, "skipped");
    }

    // test 4: capacity after reserve
    str = d_string_new();

    if (str)
    {
        d_string_reserve(str, 500);
        cap = d_string_capacity(str);

        D_TEST_CHECK_MSG(_suite,
                         "capacity_after_reserve",
                         cap >= 500,
                         "capacity should reflect reserve");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "reserve_cap_skipped", false, "skipped");
    }

    // test 5: capacity after shrink_to_fit
    str = d_string_new_with_capacity(1000);

    if (str)
    {
        d_string_assign_cstr(str, "Hi");
        d_string_shrink_to_fit(str);
        cap = d_string_capacity(str);

        D_TEST_CHECK_MSG(_suite,
                         "capacity_after_shrink",
                         cap < 1000,
                         "capacity should be reduced after shrink_to_fit");

        D_TEST_CHECK_MSG(_suite,
                         "capacity_at_least_size_plus_1",
                         cap >= str->size + 1,
                         "capacity should be at least size + 1");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "shrink_cap_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "shrink_min_skipped", false, "skipped");
    }

    // test 6: capacity matches struct field
    str = d_string_new_with_capacity(128);

    if (str)
    {
        cap = d_string_capacity(str);

        D_TEST_CHECK_MSG(_suite,
                         "capacity_matches_field",
                         (cap) == (str->capacity),
                         "d_string_capacity() should return struct field value");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "capacity_field_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_resize
******************************************************************************/

/*
d_tests_dstring_resize
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
bool d_tests_dstring_resize(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_resize");

    // test 1: NULL string returns false
    result = d_string_resize(NULL, 10);
    D_TEST_CHECK_MSG(_suite,
                     "null_string_returns_false",
                     !(result),
                     "d_string_resize(NULL, ...) should return false");

    // test 2: resize to 0 (clear string)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_resize(str, 0);

        D_TEST_CHECK_MSG(_suite,
                         "resize_to_0_succeeds",
                         result,
                         "resize to 0 should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "resize_to_0_size",
                         (str->size) == (0),
                         "size should be 0 after resize to 0");

        D_TEST_CHECK_MSG(_suite,
                         "resize_to_0_null_terminated",
                         (str->text != NULL) && (str->text[0] == '\0'),
                         "string should be null-terminated after resize to 0");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "resize_0_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "resize_0_size_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "resize_0_null_skipped", false, "skipped");
    }

    // test 3: resize smaller (truncate)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_resize(str, 5);

        D_TEST_CHECK_MSG(_suite,
                         "resize_smaller_succeeds",
                         result,
                         "resize smaller should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "resize_smaller_size",
                         (str->size) == (5),
                         "size should be 5 after truncation");

        D_TEST_CHECK_MSG(_suite,
                         "resize_smaller_content",
                         d_test_suite_str_equal((str->text), ("Hello")),
                         "content should be truncated to 'Hello'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "resize_smaller_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite,
                         "resize_smaller_size_skipped",
                         false,
                         "skipped");
        D_TEST_CHECK_MSG(_suite,
                         "resize_smaller_content_skipped",
                         false,
                         "skipped");
    }

    // test 4: resize to same size (no-op)
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        result = d_string_resize(str, 4);

        D_TEST_CHECK_MSG(_suite,
                         "resize_same_succeeds",
                         result,
                         "resize to same size should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "resize_same_content_unchanged",
                         d_test_suite_str_equal((str->text), ("Test")),
                         "content should be unchanged");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "resize_same_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite,
                         "resize_same_content_skipped",
                         false,
                         "skipped");
    }

    // test 5: resize larger (pad with nulls)
    str = d_string_new_from_cstr("Hi");

    if (str)
    {
        result = d_string_resize(str, 10);

        D_TEST_CHECK_MSG(_suite,
                         "resize_larger_succeeds",
                         result,
                         "resize larger should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "resize_larger_size",
                         (str->size) == (10),
                         "size should be 10 after resize");

        // verify original content preserved
        D_TEST_CHECK_MSG(_suite,
                         "resize_larger_content_preserved",
                         (str->text[0] == 'H') && (str->text[1] == 'i'),
                         "original content should be preserved");

        // verify null padding
        D_TEST_CHECK_MSG(_suite,
                         "resize_larger_null_padded",
                         (str->text[2] == '\0') && (str->text[9] == '\0'),
                         "new space should be null-padded");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "resize_larger_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite,
                         "resize_larger_size_skipped",
                         false,
                         "skipped");
        D_TEST_CHECK_MSG(_suite,
                         "resize_larger_content_skipped",
                         false,
                         "skipped");
        D_TEST_CHECK_MSG(_suite,
                         "resize_larger_null_skipped",
                         false,
                         "skipped");
    }

    // test 6: resize empty string to non-empty
    str = d_string_new();

    if (str)
    {
        result = d_string_resize(str, 20);

        D_TEST_CHECK_MSG(_suite,
                         "resize_empty_to_nonempty_succeeds",
                         result,
                         "resize empty string to non-empty should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "resize_empty_to_nonempty_size",
                         (str->size) == (20),
                         "size should be 20");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "resize_empty_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "resize_empty_size_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_capacity_all
******************************************************************************/

/*
d_tests_dstring_capacity_all
  Runs all capacity management tests and returns an aggregate test object
  containing all results.

Parameter(s):
  (none)
Return:
  Test object containing all capacity management test results.
*/
bool d_tests_dstring_capacity_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "dj_string Capacity Management");

    // run all capacity management tests
    d_tests_dstring_reserve(_suite);
    d_tests_dstring_shrink_to_fit(_suite);
    d_tests_dstring_capacity(_suite);
    d_tests_dstring_resize(_suite);

    return true;
}
