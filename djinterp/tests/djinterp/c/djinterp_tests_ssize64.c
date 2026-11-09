/*******************************************************************************
* djinterp [test]                                       djinterp_tests_ssize64.c
*
* This is a test file for `djinterp.h` unit tests.
* For the file itself, go to `\inc\djinterp.h`.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/djinterp_tests_ssize64.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.11.11
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTING_CORE_
#define DJINTERP_TESTING_CORE_ 1

// std
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"
#include "../../../inc/djinterp/c/memory/dmemory.h"
#include "../../../inc/djinterp/c/dmacro.h"  // D_ARRAY_COUNT
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// `dj_index` function tests (in header order)
bool d_tests_djinterp_index_convert_fast(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_index_convert_safe(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_index_is_valid(
    struct dj_test_suite* _suite
);

// `ssize64` creation and access tests
bool d_tests_djinterp_ssize64_creation(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_ssize64_access(
    struct dj_test_suite* _suite
);

// `ssize64` arithmetic operation tests
bool d_tests_djinterp_ssize64_arithmetic_functions(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_ssize64_arithmetic_macros(
    struct dj_test_suite* _suite
);

// `ssize64` comparison tests
bool d_tests_djinterp_ssize64_comparison_ssize64(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_ssize64_comparison_other_types(
    struct dj_test_suite* _suite
);

// `ssize64` conversion and validation tests
bool d_tests_djinterp_ssize64_conversion_functions(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_ssize64_validation_functions(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_ssize64_edge_cases(
    struct dj_test_suite* _suite
);

// macro utility tests (in logical order)
bool d_tests_djinterp_macro_array_utilities(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_macro_index_validation(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_macro_negative_indexing(
    struct dj_test_suite* _suite
);

// integration and consistency tests
bool d_tests_djinterp_function_macro_consistency(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_extreme_edge_cases(
    struct dj_test_suite* _suite
);

// comprehensive test suite runners
bool d_tests_djinterp_index_core_all(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_macro_all(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_integration_all(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_core_all(
    struct dj_test_suite* _suite
);
bool d_tests_djinterp_core_all_extended(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTING_CORE_


///////////////////////////////////////////////////////////////////////////////
///                    I. dj_index CORE FUNCTION TESTS                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tests_djinterp_index_convert_fast
  comprehensive testing of d_index_convert_fast function
  Tests the following:
  - positive index conversion (identity function)
  - negative index conversion to positive equivalents
  - boundary conditions with various array sizes
  - edge cases including zero-sized arrays
  - large array sizes and extreme indices
*/
bool d_tests_djinterp_index_convert_fast(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "index_convert_fast");

    bool   all_assertions_passed = true;
    size_t result;

    // test positive indices (should return unchanged)
    result = d_index_convert_fast(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Positive index 0 returns 0",
                          result == 0,
                          "d_index_convert_fast(0, 5) should return 0"))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(3, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Positive index 3 returns 3",
                          result == 3,
                          "d_index_convert_fast(3, 5) should return 3"))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(4, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Last valid positive index returns correctly",
                          result == 4,
                          "d_index_convert_fast(4, 5) should return 4"))
    {
        all_assertions_passed = false;
    }

    // test negative indices (conversion to positive)
    result = d_index_convert_fast(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative index -1 converts to last element",
                          result == 4,
                          "d_index_convert_fast(-1, 5) should return 4"))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-3, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative index -3 converts correctly",
                          result == 2,
                          "d_index_convert_fast(-3, 5) should return 2"))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative index -5 converts to first element",
                          result == 0,
                          "d_index_convert_fast(-5, 5) should return 0"))
    {
        all_assertions_passed = false;
    }

    // test edge cases with size 1 array
    result = d_index_convert_fast(0, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index 0 valid for size 1 array",
                          result == 0,
                          "d_index_convert_fast(0, 1) should return 0"))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-1, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -1 converts correctly for size 1 array",
                          result == 0,
                          "d_index_convert_fast(-1, 1) should return 0"))
    {
        all_assertions_passed = false;
    }

    // test larger array sizes
    size_t large_size = 1000;
    result = d_index_convert_fast(-1, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative index works with large arrays",
                          result == large_size - 1,
                          "d_index_convert_fast(-1, 1000) should return 999"))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-large_size, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative index equal to size converts to 0",
                          result == 0,
                          "d_index_convert_fast(-1000, 1000) should return 0"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_index_convert_safe
  comprehensive testing of d_index_convert_safe function
  Tests the following:
  - valid index conversions with proper output storage
  - invalid index rejection with output preservation
  - null pointer handling for destination parameter
  - boundary condition validation
  - error condition handling
*/
bool d_tests_djinterp_index_convert_safe(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "index_convert_safe");

    bool   all_assertions_passed = true;
    size_t output_index;
    bool   result;

    // test valid positive indices
    output_index = 999; // sentinel value
    result = d_index_convert_safe(0, 5, &output_index);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid positive index 0 returns success",
                          result == true,
                          "d_index_convert_safe(0, 5, &out) should return true"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid index 0 outputs correct value",
                          output_index == 0,
                          "Output should be 0 for valid index"))
    {
        all_assertions_passed = false;
    }

    output_index = 999;
    result = d_index_convert_safe(4, 5, &output_index);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid positive index 4 returns success",
                          result == true,
                          "d_index_convert_safe(4, 5, &out) should return true"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid index 4 outputs correct value",
                          output_index == 4,
                          "Output should be 4 for valid index"))
    {
        all_assertions_passed = false;
    }

    // test valid negative indices
    output_index = 999;
    result = d_index_convert_safe(-1, 5, &output_index);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid negative index -1 returns success",
                          result == true,
                          "d_index_convert_safe(-1, 5, &out) should return true"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid negative index -1 outputs correct value",
                          output_index == 4,
                          "Output should be 4 for index -1"))
    {
        all_assertions_passed = false;
    }

    output_index = 999;
    result = d_index_convert_safe(-5, 5, &output_index);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid negative index -5 returns success",
                          result == true,
                          "d_index_convert_safe(-5, 5, &out) should return true"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Valid negative index -5 outputs correct value",
                          output_index == 0,
                          "Output should be 0 for index -5"))
    {
        all_assertions_passed = false;
    }

    // test invalid indices (out of bounds)
    output_index = 999;
    result = d_index_convert_safe(5, 5, &output_index);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Invalid positive index returns failure",
                          result == false,
                          "d_index_convert_safe(5, 5, &out) should return false"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Invalid index preserves output value",
                          output_index == 999,
                          "Output should remain 999 for invalid index"))
    {
        all_assertions_passed = false;
    }

    output_index = 999;
    result = d_index_convert_safe(-6, 5, &output_index);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Invalid negative index returns failure",
                          result == false,
                          "d_index_convert_safe(-6, 5, &out) should return false"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Invalid negative index preserves output value",
                          output_index == 999,
                          "Output should remain 999 for invalid index"))
    {
        all_assertions_passed = false;
    }

    // test null pointer handling
    result = d_index_convert_safe(2, 5, NULL);
    if (!D_TEST_CHECK_MSG(_suite,
                          "NULL destination pointer returns failure",
                          result == false,
                          "d_index_convert_safe with NULL destination should fail"))
    {
        all_assertions_passed = false;
    }

    // test empty array
    output_index = 999;
    result = d_index_convert_safe(0, 0, &output_index);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Any index on empty array returns failure",
                          result == false,
                          "d_index_convert_safe(0, 0, &out) should return false"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Empty array preserves output value",
                          output_index == 999,
                          "Output should remain 999 for empty array"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_index_is_valid
  comprehensive testing of d_index_is_valid function
  Tests the following:
  - valid positive and negative index recognition
  - invalid index rejection for out-of-bounds cases
  - empty array handling
  - boundary conditions and edge cases
  - consistency with conversion function behavior
*/
bool d_tests_djinterp_index_is_valid(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "index_is_valid");

    bool   all_assertions_passed = true;
    bool   result;

    // test valid positive indices
    result = d_index_is_valid(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index 0 is valid for size 5",
                          result == true,
                          "d_index_is_valid(0, 5) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(4, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index 4 is valid for size 5",
                          result == true,
                          "d_index_is_valid(4, 5) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(2, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index 2 is valid for size 5",
                          result == true,
                          "d_index_is_valid(2, 5) should return true"))
    {
        all_assertions_passed = false;
    }

    // test invalid positive indices
    result = d_index_is_valid(5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index 5 is invalid for size 5",
                          result == false,
                          "d_index_is_valid(5, 5) should return false"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(100, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Large positive index is invalid",
                          result == false,
                          "d_index_is_valid(100, 5) should return false"))
    {
        all_assertions_passed = false;
    }

    // test valid negative indices
    result = d_index_is_valid(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -1 is valid for size 5",
                          result == true,
                          "d_index_is_valid(-1, 5) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -5 is valid for size 5",
                          result == true,
                          "d_index_is_valid(-5, 5) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-3, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -3 is valid for size 5",
                          result == true,
                          "d_index_is_valid(-3, 5) should return true"))
    {
        all_assertions_passed = false;
    }

    // test invalid negative indices
    result = d_index_is_valid(-6, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -6 is invalid for size 5",
                          result == false,
                          "d_index_is_valid(-6, 5) should return false"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-100, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Large negative index is invalid",
                          result == false,
                          "d_index_is_valid(-100, 5) should return false"))
    {
        all_assertions_passed = false;
    }

    // test edge cases with size 1
    result = d_index_is_valid(0, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index 0 is valid for size 1",
                          result == true,
                          "d_index_is_valid(0, 1) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-1, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -1 is valid for size 1",
                          result == true,
                          "d_index_is_valid(-1, 1) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(1, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index 1 is invalid for size 1",
                          result == false,
                          "d_index_is_valid(1, 1) should return false"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-2, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -2 is invalid for size 1",
                          result == false,
                          "d_index_is_valid(-2, 1) should return false"))
    {
        all_assertions_passed = false;
    }

    // test empty array (size 0)
    result = d_index_is_valid(0, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Any index is invalid for empty array",
                          result == false,
                          "d_index_is_valid(0, 0) should return false"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-1, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative index invalid for empty array",
                          result == false,
                          "d_index_is_valid(-1, 0) should return false"))
    {
        all_assertions_passed = false;
    }

    // test large array sizes
    size_t large_size = 1000000;
    result = d_index_is_valid(-1, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -1 valid for large arrays",
                          result == true,
                          "d_index_is_valid(-1, 1000000) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-large_size, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative size equal to array size is valid",
                          result == true,
                          "d_index_is_valid(-1000000, 1000000) should return true"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-(ssize_t)large_size - 1, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative size greater than array size is invalid",
                          result == false,
                          "d_index_is_valid(-1000001, 1000000) should return false"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_index_core_all
  runs all core dj_index function tests
  Tests the following:
  - all dj_index function family tests
  - reports aggregate results
*/
bool d_tests_djinterp_index_core_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "index_core_all");

    d_tests_djinterp_index_convert_fast(_suite);
    d_tests_djinterp_index_convert_safe(_suite);
    d_tests_djinterp_index_is_valid(_suite);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
///                        II. MACRO VALIDATION TESTS                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tests_djinterp_macro_array_utilities
  comprehensive testing of array utility macros
  Tests the following:
  - D_ARRAY_TOTAL_SIZE calculation accuracy
  - D_ARRAY_COUNT compile-time array sizing
  - various data types and array sizes
  - edge cases with zero-sized elements
*/
bool d_tests_djinterp_macro_array_utilities(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_array_utilities");

    bool   all_assertions_passed = true;

    // test D_ARRAY_TOTAL_SIZE macro
    size_t total_size = D_ARRAY_TOTAL_SIZE(sizeof(int), 10);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_TOTAL_SIZE calculates correctly for int array",
                          total_size == sizeof(int) * 10,
                          "Macro should calculate total array size correctly"))
    {
        all_assertions_passed = false;
    }

    total_size = D_ARRAY_TOTAL_SIZE(sizeof(char), 100);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_TOTAL_SIZE works with char arrays",
                          total_size == 100,
                          "Char array total size should be element count"))
    {
        all_assertions_passed = false;
    }

    total_size = D_ARRAY_TOTAL_SIZE(sizeof(double), 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_TOTAL_SIZE works with double arrays",
                          total_size == sizeof(double) * 5,
                          "Double array total size should be correct"))
    {
        all_assertions_passed = false;
    }

    // test D_ARRAY_COUNT macro with actual arrays
    int int_array[10];
    size_t count = D_ARRAY_COUNT(int_array);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT correctly counts int array elements",
                          count == 10,
                          "Should return 10 for 10-element int array"))
    {
        all_assertions_passed = false;
    }

    char char_array[25];
    count = D_ARRAY_COUNT(char_array);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT correctly counts char array elements",
                          count == 25,
                          "Should return 25 for 25-element char array"))
    {
        all_assertions_passed = false;
    }

    double double_array[3];
    count = D_ARRAY_COUNT(double_array);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT correctly counts double array elements",
                          count == 3,
                          "Should return 3 for 3-element double array"))
    {
        all_assertions_passed = false;
    }

    // test edge case with single element array
    int single_element[1];
    count = D_ARRAY_COUNT(single_element);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARRAY_COUNT works with single element arrays",
                          count == 1,
                          "Should return 1 for single-element array"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_macro_index_validation
  comprehensive testing of index validation macros
  Tests the following:
  - D_IS_VALID_INDEX_N macro validation logic
  - D_INDEX_IN_BOUNDS alias functionality
  - positive and negative index validation
  - edge cases with various array sizes
*/
bool d_tests_djinterp_macro_index_validation(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_index_validation");

    bool   all_assertions_passed = true;
    bool   result;

    // test D_IS_VALID_INDEX_N with valid positive indices
    result = D_IS_VALID_INDEX_N(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts valid positive index 0",
                          result == true,
                          "Index 0 should be valid for size 5"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(4, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts valid positive index 4",
                          result == true,
                          "Index 4 should be valid for size 5"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(2, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts valid positive index 2",
                          result == true,
                          "Index 2 should be valid for size 5"))
    {
        all_assertions_passed = false;
    }

    // test D_IS_VALID_INDEX_N with invalid positive indices
    result = D_IS_VALID_INDEX_N(5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects invalid positive index 5",
                          result == false,
                          "Index 5 should be invalid for size 5"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(10, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects out-of-bounds positive index",
                          result == false,
                          "Index 10 should be invalid for size 5"))
    {
        all_assertions_passed = false;
    }

    // test D_IS_VALID_INDEX_N with valid negative indices
    result = D_IS_VALID_INDEX_N(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts valid negative index -1",
                          result == true,
                          "Index -1 should be valid for size 5"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts valid negative index -5",
                          result == true,
                          "Index -5 should be valid for size 5"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-3, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts valid negative index -3",
                          result == true,
                          "Index -3 should be valid for size 5"))
    {
        all_assertions_passed = false;
    }

    // test D_IS_VALID_INDEX_N with invalid negative indices
    result = D_IS_VALID_INDEX_N(-6, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects invalid negative index -6",
                          result == false,
                          "Index -6 should be invalid for size 5"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-10, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects out-of-bounds negative index",
                          result == false,
                          "Index -10 should be invalid for size 5"))
    {
        all_assertions_passed = false;
    }

    // test D_INDEX_IN_BOUNDS alias (should behave identically)
    result = D_INDEX_IN_BOUNDS(2, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INDEX_IN_BOUNDS alias works for valid index",
                          result == true,
                          "Alias should work identically to original macro"))
    {
        all_assertions_passed = false;
    }

    result = D_INDEX_IN_BOUNDS(-2, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INDEX_IN_BOUNDS alias works for valid negative index",
                          result == true,
                          "Alias should work for negative indices"))
    {
        all_assertions_passed = false;
    }

    result = D_INDEX_IN_BOUNDS(10, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INDEX_IN_BOUNDS alias rejects invalid index",
                          result == false,
                          "Alias should reject invalid indices"))
    {
        all_assertions_passed = false;
    }

    // test edge cases with empty array
    result = D_IS_VALID_INDEX_N(0, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects any index for empty array",
                          result == false,
                          "No index should be valid for size 0"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-1, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects negative index for empty array",
                          result == false,
                          "Negative index should be invalid for size 0"))
    {
        all_assertions_passed = false;
    }

    // test edge cases with size 1 array
    result = D_IS_VALID_INDEX_N(0, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts index 0 for size 1",
                          result == true,
                          "Index 0 should be valid for size 1"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-1, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N accepts index -1 for size 1",
                          result == true,
                          "Index -1 should be valid for size 1"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(1, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects index 1 for size 1",
                          result == false,
                          "Index 1 should be invalid for size 1"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-2, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N rejects index -2 for size 1",
                          result == false,
                          "Index -2 should be invalid for size 1"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_macro_negative_indexing
  comprehensive testing of negative indexing utility macros
  Tests the following:
  - D_NEG_IDX conversion logic for various indices
  - D_ARR_IDX array access with negative indices
  - D_SAFE_ARR_IDX bounds-checked array access
  - edge cases and boundary conditions
*/
bool d_tests_djinterp_macro_negative_indexing(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_negative_indexing");

    bool   all_assertions_passed = true;
    size_t result;

    // test D_NEG_IDX conversion macro
    result = D_NEG_IDX(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX preserves positive index 0",
                          result == 0,
                          "Positive index 0 should remain 0"))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(3, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX preserves positive index 3",
                          result == 3,
                          "Positive index 3 should remain 3"))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX converts negative index -1 to 4",
                          result == 4,
                          "Index -1 should convert to last element (4)"))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-3, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX converts negative index -3 to 2",
                          result == 2,
                          "Index -3 should convert to element 2"))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX converts negative index -5 to 0",
                          result == 0,
                          "Index -5 should convert to first element (0)"))
    {
        all_assertions_passed = false;
    }

    // test D_ARR_IDX array access macro
    int test_array[5] = {10, 20, 30, 40, 50};

    int value = D_ARR_IDX(test_array, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX accesses first element with index 0",
                          value == 10,
                          "Should access first element (10)"))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, 2);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX accesses middle element with index 2",
                          value == 30,
                          "Should access third element (30)"))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, 4);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX accesses last element with index 4",
                          value == 50,
                          "Should access last element (50)"))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, -1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX accesses last element with index -1",
                          value == 50,
                          "Should access last element (50) via negative index"))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, -3);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX accesses element with index -3",
                          value == 30,
                          "Should access third element (30) via negative index"))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, -5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX accesses first element with index -5",
                          value == 10,
                          "Should access first element (10) via negative index"))
    {
        all_assertions_passed = false;
    }

    // test D_SAFE_ARR_IDX: the element for a valid index, and the
    // first element for one out of range
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX, valid positive index",
                          D_SAFE_ARR_IDX(test_array, 2) == 30,
                          "should give the element"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX, valid negative index",
                          D_SAFE_ARR_IDX(test_array, -2) == 40,
                          "should give the element"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX, out-of-range positive index",
                          D_SAFE_ARR_IDX(test_array, 10) == test_array[0],
                          "should give the first element"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX, out-of-range negative index",
                          D_SAFE_ARR_IDX(test_array, -10) == test_array[0],
                          "should give the first element"))
    {
        all_assertions_passed = false;
    }

    // test with single element array
    int single_array[1] = {99};

    value = D_ARR_IDX(single_array, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX works with single element array index 0",
                          value == 99,
                          "Should access single element (99)"))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(single_array, -1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX works with single element array index -1",
                          value == 99,
                          "Should access single element (99) via negative index"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX works with single element array",
                          D_SAFE_ARR_IDX(single_array, 0) == 99,
                          "should give the single element"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_macro_all
  runs all macro validation tests
  Tests the following:
  - array utility macros
  - index validation macros
  - negative indexing macros
  - reports aggregate results
*/
bool d_tests_djinterp_macro_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "macro_all");

    d_tests_djinterp_macro_array_utilities(_suite);
    d_tests_djinterp_macro_index_validation(_suite);
    d_tests_djinterp_macro_negative_indexing(_suite);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
///                      III. INTEGRATION TESTS                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tests_djinterp_function_macro_consistency
  tests consistency between function and macro implementations
  Tests the following:
  - function vs macro validation consistency
  - index conversion agreement between approaches
  - edge case handling consistency
  - performance vs safety trade-offs
*/
bool d_tests_djinterp_function_macro_consistency(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "function_macro_consistency");

    bool   all_assertions_passed = true;

    // test consistency between d_index_is_valid and D_IS_VALID_INDEX_N
    for (dj_index idx = -10; idx <= 10; idx++)
    {
        for (size_t size = 0; size <= 10; size++)
        {
            bool func_result = d_index_is_valid(idx, size);
            bool macro_result = D_IS_VALID_INDEX_N(idx, size);

            if (func_result != macro_result)
            {
                if (!D_TEST_CHECK_MSG(_suite,
                                      "Function and macro validation should agree",
                                      false,
                                      "d_index_is_valid and D_IS_VALID_INDEX_N disagree"))
                {
                    all_assertions_passed = false;
                    (void)d_test_suite_note(_suite, "idx=%d size=%zu: "
                                            "function %d, macro %d", (int)idx,
                                            size, func_result, macro_result);
                    break; // exit inner loop on first failure
                }
            }
        }
        if (!all_assertions_passed) break; // exit outer loop on failure
    }

    // test consistency between d_index_convert_fast and D_NEG_IDX
    size_t test_sizes[] = {1, 5, 10, 100};
    dj_index test_indices[] = {-5, -1, 0, 1, 4};

    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++)
    {
        size_t size = test_sizes[i];
        for (size_t j = 0; j < sizeof(test_indices) / sizeof(test_indices[0]); j++)
        {
            dj_index idx = test_indices[j];

            // only test valid indices to ensure meaningful comparison
            if (d_index_is_valid(idx, size))
            {
                size_t func_result = d_index_convert_fast(idx, size);
                size_t macro_result = (size_t)D_NEG_IDX(idx, size);

                if (!D_TEST_CHECK_MSG(_suite,
                                      "Function and macro conversion should agree",
                                      func_result == macro_result,
                                      "d_index_convert_fast and D_NEG_IDX should agree on valid indices"))
                {
                    all_assertions_passed = false;
                    (void)d_test_suite_note(_suite, "idx=%d size=%zu: "
                                            "function %zu, macro %zu", (int)idx,
                                            size, func_result, macro_result);
                }
            }
        }
    }

    // test array access consistency
    int test_array[5] = {100, 200, 300, 400, 500};

    for (dj_index idx = -5; idx < 5; idx++)
    {
        if (d_index_is_valid(idx, 5))
        {
            size_t converted_idx = d_index_convert_fast(idx, 5);
            int func_value = test_array[converted_idx];
            int macro_value = D_ARR_IDX(test_array, idx);

            if (!D_TEST_CHECK_MSG(_suite,
                                  "Function-based and macro-based array access should agree",
                                  func_value == macro_value,
                                  "Array access methods should yield same results"))
            {
                all_assertions_passed = false;
                (void)d_test_suite_note(_suite, "idx=%d: function %d, macro %d",
                                        (int)idx, func_value, macro_value);
            }
        }
    }

    // test that safe functions reject what unsafe versions might handle incorrectly
    size_t safe_output;
    bool safe_result = d_index_convert_safe(10, 5, &safe_output); // out of bounds
    if (!D_TEST_CHECK_MSG(_suite,
                          "Safe function should reject out-of-bounds index",
                          safe_result == false,
                          "d_index_convert_safe should reject invalid indices"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_extreme_edge_cases
  tests extreme boundary conditions and edge cases
  Tests the following:
  - SIZE_MAX boundary behavior
  - signed integer overflow/underflow handling
  - zero-sized array behavior
  - maximum negative index handling
  - type conversion edge cases
*/
bool d_tests_djinterp_extreme_edge_cases(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "extreme_edge_cases");

    bool   all_assertions_passed = true;

    // test behavior with very large array sizes
    size_t large_size = 1000000;
    bool result = d_index_is_valid(-1, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Index -1 should be valid for large arrays",
                          result == true,
                          "Large arrays should support negative indexing"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-(ssize_t)large_size, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative size equal to array size should be valid",
                          result == true,
                          "Most negative valid index should be accepted"))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-(ssize_t)large_size - 1, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Negative size greater than array should be invalid",
                          result == false,
                          "Out-of-bounds negative index should be rejected"))
    {
        all_assertions_passed = false;
    }

    // test macro behavior with large indices
    bool macro_result = D_IS_VALID_INDEX_N(-1, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Macro should handle large arrays correctly",
                          macro_result == true,
                          "D_IS_VALID_INDEX_N should work with large arrays"))
    {
        all_assertions_passed = false;
    }

    // test conversion with large arrays
    size_t converted = d_index_convert_fast(-1, large_size);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Conversion should work correctly with large arrays",
                          converted == large_size - 1,
                          "Should convert to last element of large array"))
    {
        all_assertions_passed = false;
    }

    // test zero array size edge cases
    result = d_index_is_valid(0, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "No index should be valid for zero-sized array",
                          result == false,
                          "Zero-sized arrays should reject all indices"))
    {
        all_assertions_passed = false;
    }

    macro_result = D_IS_VALID_INDEX_N(-1, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Macro should reject indices for zero-sized array",
                          macro_result == false,
                          "Zero-sized arrays should reject negative indices"))
    {
        all_assertions_passed = false;
    }

    // test safe function with extreme values
    size_t safe_output = 999;
    bool safe_result = d_index_convert_safe(-1000000, 5, &safe_output);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Safe function should reject extremely negative indices",
                          safe_result == false,
                          "Very negative indices should be rejected"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "Safe function should preserve output on failure",
                          safe_output == 999,
                          "Output should be unchanged after failed conversion"))
    {
        all_assertions_passed = false;
    }

    // test potential overflow conditions
    // Note: This tests behavior near the limits of ssize_t
    ssize_t very_negative = -1000000000;
    result = d_index_is_valid(very_negative, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "Very large negative indices should be invalid",
                          result == false,
                          "Extremely negative indices should be rejected"))
    {
        all_assertions_passed = false;
    }

    // test that macros handle edge cases properly
    size_t clamp_result = D_CLAMP_INDEX(1000, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX should clamp oversized indices",
                          clamp_result == 4,
                          "Should clamp to maximum valid index"))
    {
        all_assertions_passed = false;
    }

    clamp_result = D_CLAMP_INDEX(-1000, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX should clamp undersized indices",
                          clamp_result == 0,
                          "Should clamp to minimum valid index"))
    {
        all_assertions_passed = false;
    }

    // test empty array clamping
    clamp_result = D_CLAMP_INDEX(5, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX should handle empty arrays",
                          clamp_result == 0,
                          "Should return 0 for empty arrays"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}

/*
d_tests_djinterp_integration_all
  runs all integration tests
  Tests the following:
  - function-macro consistency tests
  - extreme edge case tests
  - reports aggregate results
*/
bool d_tests_djinterp_integration_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "integration_all");

    d_tests_djinterp_function_macro_consistency(_suite);
    d_tests_djinterp_extreme_edge_cases(_suite);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
///                       IV. COMPREHENSIVE TEST SUITE                      ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tests_djinterp_core_all
  comprehensive test suite runner for all djinterp core functionality
  Tests the following:
  - all dj_index core function tests
  - all macro validation tests
  - all integration tests
  - provides comprehensive test coverage summary
*/
bool d_tests_djinterp_core_all(
    struct dj_test_suite* _suite
)
{
    d_tests_djinterp_index_core_all(_suite);
    d_tests_djinterp_macro_all(_suite);
    d_tests_djinterp_integration_all(_suite);

    return true;
}

/*
d_tests_djinterp_core_all_extended
  extended test suite runner including ssize64 functionality
  Tests the following:
  - all core djinterp functionality
  - complete ssize64 extended signed integer system
  - comprehensive coverage of all components
*/
bool d_tests_djinterp_core_all_extended(
    struct dj_test_suite* _suite
)
{
    d_tests_djinterp_core_all(_suite);

    return true;
}
