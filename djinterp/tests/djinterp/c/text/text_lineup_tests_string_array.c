/*******************************************************************************
* djinterp [text]                               text_lineup_tests_string_array.c
*
*
* path:      /tests/djinterp/c/text/text_lineup_tests_string_array.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_lineup_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's
                                                                // C face


/******************************************************************************
 * SECTION II: dj_string_array OPERATIONS
 *****************************************************************************/

/*
d_tests_text_lineup_string_array_new
  Tests d_string_array_new for correct array allocation.
  Tests the following:
  - returns non-NULL for a valid capacity
  - count field is initialised to zero
  - elements pointer is non-NULL for capacity > 0
  - capacity field reflects the requested capacity
  - returns non-NULL for capacity of 0
  - elements pointer may be NULL when capacity is 0
  - handles large capacity without returning NULL
*/
bool d_tests_text_lineup_string_array_new(struct dj_test_suite* _suite)
{
    struct dj_string_array* arr_normal;
    struct dj_string_array* arr_zero;
    struct dj_string_array* arr_large;
    bool                   test_not_null;
    bool                   test_count_zero;
    bool                   test_elements_not_null;
    bool                   test_capacity;
    bool                   test_zero_cap_not_null;
    bool                   test_zero_cap_elements;
    bool                   test_large;

    // test 1: non-NULL for standard capacity
    arr_normal           = d_string_array_new(8);
    test_not_null        = (arr_normal != NULL);

    // test 2: count is 0
    test_count_zero = test_not_null && (arr_normal->count == 0);

    // test 3: elements is non-NULL (calloc'd)
    test_elements_not_null = test_not_null && (arr_normal->elements != NULL);

    // test 4: capacity matches request
    test_capacity = test_not_null && (arr_normal->capacity == 8);

    // test 5 & 6: capacity == 0 is allowed; elements may be NULL
    arr_zero               = d_string_array_new(0);
    test_zero_cap_not_null = (arr_zero != NULL);
    test_zero_cap_elements = test_zero_cap_not_null &&
                             (arr_zero->count == 0);

    // test 7: large capacity
    arr_large    = d_string_array_new(1024);
    test_large   = (arr_large != NULL) && (arr_large->capacity >= 1024);

    // cleanup
    d_string_array_free(arr_normal);
    d_string_array_free(arr_zero);
    d_string_array_free(arr_large);

    // build result tree
    d_test_suite_test(_suite, "d_string_array_new");

    D_TEST_CHECK_MSG(_suite,
                     "not_null",
                     test_not_null,
                     "returns non-NULL for valid capacity");
    D_TEST_CHECK_MSG(_suite,
                     "count_zero",
                     test_count_zero,
                     "count is 0 on fresh allocation");
    D_TEST_CHECK_MSG(_suite,
                     "elements_not_null",
                     test_elements_not_null,
                     "elements pointer is non-NULL");
    D_TEST_CHECK_MSG(_suite,
                     "capacity",
                     test_capacity,
                     "capacity matches requested value");
    D_TEST_CHECK_MSG(_suite,
                     "zero_cap_not_null",
                     test_zero_cap_not_null,
                     "capacity 0 still returns non-NULL");
    D_TEST_CHECK_MSG(_suite,
                     "zero_cap_elements",
                     test_zero_cap_elements,
                     "count is 0 for zero-capacity array");
    D_TEST_CHECK_MSG(_suite, "large", test_large, "handles large capacity");

    return true;
}


/*
d_tests_text_lineup_string_array_reserve
  Tests d_string_array_reserve for capacity growth.
  Tests the following:
  - returns true when requested capacity already met
  - capacity is unchanged when request <= current
  - returns false for NULL array
  - capacity grows when requested size exceeds current
  - existing elements are preserved after growth
  - count is unchanged after growth
  - after growth, capacity >= min_capacity
*/
bool d_tests_text_lineup_string_array_reserve(struct dj_test_suite* _suite)
{
    struct dj_string_array* arr;
    bool                   result;
    size_t                 old_cap;
    bool                   test_sufficient_true;
    bool                   test_sufficient_unchanged;
    bool                   test_null;
    bool                   test_grows;
    bool                   test_elements_preserved;
    bool                   test_count_unchanged;
    bool                   test_meets_min;

    // test 1 & 2: already sufficient — returns true, capacity unchanged
    arr = d_string_array_new(16);
    test_sufficient_true      = false;
    test_sufficient_unchanged = false;

    if (arr)
    {
        result = d_string_array_reserve(arr, 8);
        test_sufficient_true      = (result == true);
        test_sufficient_unchanged = (arr->capacity >= 16);
        d_string_array_free(arr);
    }

    // test 3: NULL array → false
    test_null = (d_string_array_reserve(NULL, 32) == false);

    // test 4-7: growth tests
    arr = d_string_array_new(4);
    test_grows             = false;
    test_elements_preserved = false;
    test_count_unchanged    = false;
    test_meets_min          = false;

    if (arr)
    {
        d_string_array_append(arr, "first");
        d_string_array_append(arr, "second");
        old_cap = arr->capacity;

        result = d_string_array_reserve(arr, 64);

        // test 4: capacity grew
        test_grows = (result == true) && (arr->capacity > old_cap);

        // test 5: elements[0] is still "first"
        test_elements_preserved =
            (arr->elements != NULL) &&
            (arr->elements[0] != NULL) &&
            d_strequals(arr->elements[0],
                        d_strnlen(arr->elements[0],
                                  D_TEST_LINEUP_STRNLEN_MAX),
                        "first",
                        5);

        // test 6: count is still 2
        test_count_unchanged = (arr->count == 2);

        // test 7: capacity >= 64
        test_meets_min = (arr->capacity >= 64);

        d_string_array_free(arr);
    }

    // build result tree
    d_test_suite_test(_suite, "d_string_array_reserve");

    D_TEST_CHECK_MSG(_suite,
                     "sufficient_true",
                     test_sufficient_true,
                     "returns true when capacity sufficient");
    D_TEST_CHECK_MSG(_suite,
                     "sufficient_unchanged",
                     test_sufficient_unchanged,
                     "capacity unchanged when sufficient");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "returns false for NULL array");
    D_TEST_CHECK_MSG(_suite, "grows", test_grows, "capacity grows when needed");
    D_TEST_CHECK_MSG(_suite,
                     "elements_preserved",
                     test_elements_preserved,
                     "existing elements preserved after growth");
    D_TEST_CHECK_MSG(_suite,
                     "count_unchanged",
                     test_count_unchanged,
                     "count unchanged after growth");
    D_TEST_CHECK_MSG(_suite,
                     "meets_min",
                     test_meets_min,
                     "capacity >= requested minimum");

    return true;
}


/*
d_tests_text_lineup_string_array_append
  Tests d_string_array_append for correct string insertion.
  Tests the following:
  - appended string is accessible at the correct index
  - appended string is a copy (own allocation)
  - count increments after each append
  - returns false for NULL array
  - returns false for NULL string
  - auto-grows when count reaches capacity
  - successive appends produce the correct element order
*/
bool d_tests_text_lineup_string_array_append(struct dj_test_suite* _suite)
{
    struct dj_string_array* arr;
    bool                   result;
    bool                   test_accessible;
    bool                   test_is_copy;
    bool                   test_count;
    bool                   test_null_arr;
    bool                   test_null_str;
    bool                   test_auto_grow;
    bool                   test_order;

    // test 1: appended string is accessible
    arr            = d_string_array_new(4);
    test_accessible = false;

    if (arr)
    {
        d_string_array_append(arr, "hello");
        test_accessible = (arr->elements != NULL) &&
                          (arr->elements[0] != NULL) &&
                          d_strequals(arr->elements[0],
                                      d_strnlen(arr->elements[0],
                                                D_TEST_LINEUP_STRNLEN_MAX),
                                      "hello",
                                      5);
        d_string_array_free(arr);
    }

    // test 2: appended string is an independent copy
    arr         = d_string_array_new(4);
    test_is_copy = false;

    if (arr)
    {
        char literal[] = "owned";

        d_string_array_append(arr, literal);

        // pointer must differ from the literal's address
        test_is_copy = (arr->elements[0] != literal);
        d_string_array_free(arr);
    }

    // test 3: count increments
    arr        = d_string_array_new(4);
    test_count  = false;

    if (arr)
    {
        d_string_array_append(arr, "a");
        d_string_array_append(arr, "b");
        d_string_array_append(arr, "c");
        test_count = (arr->count == 3);
        d_string_array_free(arr);
    }

    // test 4: NULL array → false
    test_null_arr = (d_string_array_append(NULL, "test") == false);

    // test 5: NULL string → false
    arr           = d_string_array_new(4);
    test_null_str  = false;

    if (arr)
    {
        test_null_str = (d_string_array_append(arr, NULL) == false);
        d_string_array_free(arr);
    }

    // test 6: auto-grows past initial capacity
    arr            = d_string_array_new(2);
    test_auto_grow  = false;

    if (arr)
    {
        result = true;
        result = result && d_string_array_append(arr, "one");
        result = result && d_string_array_append(arr, "two");

        // this must trigger a realloc
        result = result && d_string_array_append(arr, "three");
        test_auto_grow = result && (arr->count == 3);
        d_string_array_free(arr);
    }

    // test 7: order of elements is preserved
    arr        = d_string_array_new(4);
    test_order  = false;

    if (arr)
    {
        d_string_array_append(arr, "first");
        d_string_array_append(arr, "second");
        d_string_array_append(arr, "third");

        test_order =
            d_strequals(arr->elements[0],
                        d_strnlen(arr->elements[0],
                                  D_TEST_LINEUP_STRNLEN_MAX),
                        "first", 5) &&
            d_strequals(arr->elements[1],
                        d_strnlen(arr->elements[1],
                                  D_TEST_LINEUP_STRNLEN_MAX),
                        "second", 6) &&
            d_strequals(arr->elements[2],
                        d_strnlen(arr->elements[2],
                                  D_TEST_LINEUP_STRNLEN_MAX),
                        "third", 5);
        d_string_array_free(arr);
    }

    // build result tree
    d_test_suite_test(_suite, "d_string_array_append");

    D_TEST_CHECK_MSG(_suite,
                     "accessible",
                     test_accessible,
                     "appended string accessible at index");
    D_TEST_CHECK_MSG(_suite,
                     "is_copy",
                     test_is_copy,
                     "appended string is an independent copy");
    D_TEST_CHECK_MSG(_suite,
                     "count",
                     test_count,
                     "count increments on each append");
    D_TEST_CHECK_MSG(_suite,
                     "null_arr",
                     test_null_arr,
                     "returns false for NULL array");
    D_TEST_CHECK_MSG(_suite,
                     "null_str",
                     test_null_str,
                     "returns false for NULL string");
    D_TEST_CHECK_MSG(_suite,
                     "auto_grow",
                     test_auto_grow,
                     "auto-grows past initial capacity");
    D_TEST_CHECK_MSG(_suite, "order", test_order, "element order is preserved");

    return true;
}


/*
d_tests_text_lineup_string_array_generate_series
  Tests d_string_array_generate_series for correct token generation.
  Tests the following:
  - ascending series produces the correct element count
  - first element matches expected value for ascending range
  - last element matches expected value for ascending range
  - descending series (_step < 0) is supported
  - non-unit step produces correct count
  - zero_padding pads numbers to the specified width
  - prefix is prepended to each element
  - suffix is appended to each element
  - step == 0 returns NULL
  - inverted range (start > end with positive step) returns NULL
*/
bool d_tests_text_lineup_string_array_generate_series(
    struct dj_test_suite* _suite)
{
    struct dj_string_array* arr;
    bool                   test_count;
    bool                   test_first;
    bool                   test_last;
    bool                   test_descending;
    bool                   test_step;
    bool                   test_padding;
    bool                   test_prefix;
    bool                   test_suffix;
    bool                   test_zero_step;
    bool                   test_inverted;

    // test 1: ascending series 1..5 → 5 elements
    arr        = d_string_array_generate_series(1, 5, 1, NULL, NULL, 0);
    test_count  = (arr != NULL) && (arr->count == 5);
    d_string_array_free(arr);

    // test 2: first element is "1"
    arr        = d_string_array_generate_series(1, 5, 1, NULL, NULL, 0);
    test_first  = false;

    if (arr && arr->count >= 1)
    {
        test_first = d_strequals(arr->elements[0],
                                 d_strnlen(arr->elements[0],
                                           D_TEST_LINEUP_STRNLEN_MAX),
                                 "1",
                                 1);
    }

    d_string_array_free(arr);

    // test 3: last element is "5"
    arr       = d_string_array_generate_series(1, 5, 1, NULL, NULL, 0);
    test_last  = false;

    if (arr && arr->count >= 5)
    {
        test_last = d_strequals(arr->elements[4],
                                d_strnlen(arr->elements[4],
                                          D_TEST_LINEUP_STRNLEN_MAX),
                                "5",
                                1);
    }

    d_string_array_free(arr);

    // test 4: descending series 5..1 step -1 → 5 elements
    arr             = d_string_array_generate_series(5, 1, -1, NULL, NULL, 0);
    test_descending  = false;

    if (arr)
    {
        test_descending = (arr->count == 5) &&
                           d_strequals(arr->elements[0],
                                       d_strnlen(arr->elements[0],
                                                 D_TEST_LINEUP_STRNLEN_MAX),
                                       "5",
                                       1);
    }

    d_string_array_free(arr);

    // test 5: step of 2 → 0,2,4,6,8 = 5 elements
    arr       = d_string_array_generate_series(0, 8, 2, NULL, NULL, 0);
    test_step  = (arr != NULL) && (arr->count == 5);
    d_string_array_free(arr);

    // test 6: zero_padding of 3 → "001", "002", ..., "010"
    arr          = d_string_array_generate_series(1, 3, 1, NULL, NULL, 3);
    test_padding  = false;

    if (arr && arr->count >= 1)
    {
        test_padding = d_strequals(arr->elements[0],
                                   d_strnlen(arr->elements[0],
                                             D_TEST_LINEUP_STRNLEN_MAX),
                                   "001",
                                   3);
    }

    d_string_array_free(arr);

    // test 7: prefix "x" → "x1", "x2", "x3"
    arr        = d_string_array_generate_series(1, 3, 1, "x", NULL, 0);
    test_prefix = false;

    if (arr && arr->count >= 1)
    {
        test_prefix = d_strequals(arr->elements[0],
                                  d_strnlen(arr->elements[0],
                                            D_TEST_LINEUP_STRNLEN_MAX),
                                  "x1",
                                  2);
    }

    d_string_array_free(arr);

    // test 8: suffix "px" → "1px", "2px"
    arr        = d_string_array_generate_series(1, 2, 1, NULL, "px", 0);
    test_suffix = false;

    if (arr && arr->count >= 1)
    {
        test_suffix = d_strequals(arr->elements[0],
                                  d_strnlen(arr->elements[0],
                                            D_TEST_LINEUP_STRNLEN_MAX),
                                  "1px",
                                  3);
    }

    d_string_array_free(arr);

    // test 9: step == 0 → NULL
    arr            = d_string_array_generate_series(1, 5, 0, NULL, NULL, 0);
    test_zero_step  = (arr == NULL);
    d_string_array_free(arr);

    // test 10: positive step with start > end → NULL
    arr           = d_string_array_generate_series(5, 1, 1, NULL, NULL, 0);
    test_inverted  = (arr == NULL);
    d_string_array_free(arr);

    // build result tree
    d_test_suite_test(_suite, "d_string_array_generate_series");

    D_TEST_CHECK_MSG(_suite,
                     "count",
                     test_count,
                     "ascending series: correct element count");
    D_TEST_CHECK_MSG(_suite,
                     "first",
                     test_first,
                     "first element matches _start");
    D_TEST_CHECK_MSG(_suite, "last", test_last, "last element matches _end");
    D_TEST_CHECK_MSG(_suite,
                     "descending",
                     test_descending,
                     "descending series is supported");
    D_TEST_CHECK_MSG(_suite,
                     "step",
                     test_step,
                     "non-unit step produces correct count");
    D_TEST_CHECK_MSG(_suite,
                     "padding",
                     test_padding,
                     "zero_padding pads to minimum width");
    D_TEST_CHECK_MSG(_suite,
                     "prefix",
                     test_prefix,
                     "prefix is prepended to each element");
    D_TEST_CHECK_MSG(_suite,
                     "suffix",
                     test_suffix,
                     "suffix is appended to each element");
    D_TEST_CHECK_MSG(_suite,
                     "zero_step",
                     test_zero_step,
                     "step == 0 returns NULL");
    D_TEST_CHECK_MSG(_suite,
                     "inverted",
                     test_inverted,
                     "inverted range with positive step → NULL");

    return true;
}


/*
d_tests_text_lineup_string_array_generate_series_capacity
  Tests d_string_array_generate_series_capacity for extra-capacity allocation.
  Tests the following:
  - returns the same elements as generate_series for equal parameters
  - array capacity equals token_count + _additional_capacity
  - count equals the expected number of tokens
  - elements beyond count remain NULL (calloc'd slots)
  - additional_capacity of 0 behaves identically to generate_series
  - step == 0 still returns NULL
*/
bool d_tests_text_lineup_string_array_generate_series_capacity(
    struct dj_test_suite* _suite)
{
    struct dj_string_array* arr;
    struct dj_string_array* arr_ref;
    bool                   test_same_elements;
    bool                   test_extra_capacity;
    bool                   test_count;
    bool                   test_null_slots;
    bool                   test_zero_extra;
    bool                   test_invalid;

    // test 1: elements match generate_series for same params
    arr     = d_string_array_generate_series_capacity(1, 3, 1,
                                                      NULL, NULL, 0,
                                                      5);
    arr_ref = d_string_array_generate_series(1, 3, 1, NULL, NULL, 0);

    test_same_elements = false;

    if (arr && arr_ref && (arr->count == arr_ref->count))
    {
        size_t i;

        test_same_elements = true;

        for (i = 0; i < arr->count; i++)
        {
            if ( !d_strequals(arr->elements[i],
                              d_strnlen(arr->elements[i],
                                        D_TEST_LINEUP_STRNLEN_MAX),
                              arr_ref->elements[i],
                              d_strnlen(arr_ref->elements[i],
                                        D_TEST_LINEUP_STRNLEN_MAX)) )
            {
                test_same_elements = false;
                break;
            }
        }
    }

    d_string_array_free(arr);
    d_string_array_free(arr_ref);

    // test 2: capacity == token_count + additional
    // series 1..4 → 4 tokens; additional = 6 → capacity = 10
    arr                 = d_string_array_generate_series_capacity(1, 4, 1,
                                                                  NULL, NULL,
                                                                  0, 6);
    test_extra_capacity  = (arr != NULL) && (arr->capacity >= 4 + 6);
    d_string_array_free(arr);

    // test 3: count == expected token count
    arr        = d_string_array_generate_series_capacity(0, 9, 1,
                                                         NULL, NULL, 0, 10);
    test_count  = (arr != NULL) && (arr->count == 10);
    d_string_array_free(arr);

    // test 4: slots beyond count are NULL
    // 3 tokens + 3 extra = capacity 6; elements[3..5] should be NULL
    arr            = d_string_array_generate_series_capacity(1, 3, 1,
                                                             NULL, NULL, 0,
                                                             3);
    test_null_slots = false;

    if (arr && (arr->capacity >= 6) && (arr->count == 3))
    {
        test_null_slots = (arr->elements[3] == NULL) &&
                          (arr->elements[4] == NULL) &&
                          (arr->elements[5] == NULL);
    }

    d_string_array_free(arr);

    // test 5: additional_capacity == 0 → behaves like generate_series
    arr            = d_string_array_generate_series_capacity(1, 5, 1,
                                                             NULL, NULL, 0,
                                                             0);
    test_zero_extra = (arr != NULL) && (arr->count == 5);
    d_string_array_free(arr);

    // test 6: invalid params (step == 0) → NULL
    arr           = d_string_array_generate_series_capacity(1, 5, 0,
                                                            NULL, NULL, 0,
                                                            4);
    test_invalid   = (arr == NULL);
    d_string_array_free(arr);

    // build result tree
    d_test_suite_test(_suite, "d_string_array_generate_series_capacity");

    D_TEST_CHECK_MSG(_suite,
                     "same_elements",
                     test_same_elements,
                     "elements match generate_series");
    D_TEST_CHECK_MSG(_suite,
                     "extra_capacity",
                     test_extra_capacity,
                     "capacity == token_count + additional");
    D_TEST_CHECK_MSG(_suite,
                     "count",
                     test_count,
                     "count equals expected token count");
    D_TEST_CHECK_MSG(_suite,
                     "null_slots",
                     test_null_slots,
                     "extra slots are initialised to NULL");
    D_TEST_CHECK_MSG(_suite,
                     "zero_extra",
                     test_zero_extra,
                     "zero additional behaves like base fn");
    D_TEST_CHECK_MSG(_suite,
                     "invalid",
                     test_invalid,
                     "invalid params return NULL");

    return true;
}


/*
d_tests_text_lineup_string_array_all
  Runs all dj_string_array tests (Section II).
  Tests the following:
  - d_string_array_new
  - d_string_array_reserve
  - d_string_array_append
  - d_string_array_generate_series
  - d_string_array_generate_series_capacity
*/
bool d_tests_text_lineup_string_array_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "dj_string_array Operations");

    d_tests_text_lineup_string_array_new(_suite);
    d_tests_text_lineup_string_array_reserve(_suite);
    d_tests_text_lineup_string_array_append(_suite);
    d_tests_text_lineup_string_array_generate_series(_suite);
    d_tests_text_lineup_string_array_generate_series_capacity(_suite);

    return true;
}
