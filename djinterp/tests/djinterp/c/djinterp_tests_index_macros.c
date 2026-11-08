/*******************************************************************************
* djinterp [test]                                  djinterp_tests_index_macros.c
*
*
* path:      /tests/djinterp/c/djinterp_tests_index_macros.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./djinterp_tests.h"


/******************************************************************************
 * D_CLAMP_INDEX TESTS
 *****************************************************************************/

bool d_tests_djinterp_macro_clamp_index(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_clamp_index");

    bool   all_assertions_passed = true;
    ssize_t result;

    // Test 1: Negative indices should clamp to 0
    result = D_CLAMP_INDEX(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(-1, 5) clamps to 0",
                          result == 0,
                          "Negative index should clamp to 0"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(-10, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(-10, 5) clamps to 0",
                          result == 0,
                          "Large negative index should clamp to 0"))
    {
        all_assertions_passed = false;
    }

    // Test 2: Valid indices should pass through unchanged
    result = D_CLAMP_INDEX(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(0, 5) returns 0",
                          result == 0,
                          "Index 0 should remain 0"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(2, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(2, 5) returns 2",
                          result == 2,
                          "Valid middle index should remain unchanged"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(4, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(4, 5) returns 4",
                          result == 4,
                          "Last valid index should remain unchanged"))
    {
        all_assertions_passed = false;
    }

    // Test 3: Too-large indices should clamp to SIZE-1
    result = D_CLAMP_INDEX(5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(5, 5) clamps to 4",
                          result == 4,
                          "Index at SIZE should clamp to SIZE-1"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(10, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(10, 5) clamps to 4",
                          result == 4,
                          "Large index should clamp to SIZE-1"))
    {
        all_assertions_passed = false;
    }

    // Test 4: Empty array (SIZE == 0) edge case
    result = D_CLAMP_INDEX(0, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(0, 0) returns 0",
                          result == 0,
                          "Empty array should return 0"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(-1, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(-1, 0) returns 0",
                          result == 0,
                          "Negative index on empty array should return 0"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(5, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(5, 0) returns 0",
                          result == 0,
                          "Positive index on empty array should return 0"))
    {
        all_assertions_passed = false;
    }

    // Test 5: Single element array
    result = D_CLAMP_INDEX(-1, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(-1, 1) returns 0",
                          result == 0,
                          "Negative index on size-1 array clamps to 0"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(0, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(0, 1) returns 0",
                          result == 0,
                          "Index 0 on size-1 array returns 0"))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(1, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CLAMP_INDEX(1, 1) returns 0",
                          result == 0,
                          "Index 1 on size-1 array clamps to 0"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_SAFE_ARR_IDX TESTS - CRITICAL SAFETY FEATURE
 *****************************************************************************/

bool d_tests_djinterp_macro_safe_arr_idx(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_safe_arr_idx");

    bool   all_assertions_passed = true;

    // Setup test arrays
    int int_array[5] = {10, 20, 30, 40, 50};
    char char_array[3] = {'A', 'B', 'C'};
    double double_array[4] = {1.1, 2.2, 3.3, 4.4};

    // Test 1: Valid positive indices
    int result_int = D_SAFE_ARR_IDX(int_array, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, 0) returns first element",
                          result_int == 10,
                          "Should return 10"))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, 2);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, 2) returns third element",
                          result_int == 30,
                          "Should return 30"))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, 4);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, 4) returns last element",
                          result_int == 50,
                          "Should return 50"))
    {
        all_assertions_passed = false;
    }

    // Test 2: Valid negative indices (Python-style)
    result_int = D_SAFE_ARR_IDX(int_array, -1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, -1) returns last element",
                          result_int == 50,
                          "Should return 50"))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, -2);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, -2) returns second-to-last",
                          result_int == 40,
                          "Should return 40"))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, -5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, -5) returns first element",
                          result_int == 10,
                          "Should return 10"))
    {
        all_assertions_passed = false;
    }

    // Test 3: Out-of-bounds positive indices (should return first element)
    result_int = D_SAFE_ARR_IDX(int_array, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, 5) returns first element (OOB)",
                          result_int == 10,
                          "Should return 10 for out-of-bounds"))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, 100);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, 100) returns first element (OOB)",
                          result_int == 10,
                          "Should return 10 for large out-of-bounds"))
    {
        all_assertions_passed = false;
    }

    // Test 4: Out-of-bounds negative indices (should return first element)
    result_int = D_SAFE_ARR_IDX(int_array, -6);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, -6) returns first element (OOB)",
                          result_int == 10,
                          "Should return 10 for negative out-of-bounds"))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, -100);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(int_array, -100) returns first element (OOB)",
                          result_int == 10,
                          "Should return 10 for large negative out-of-bounds"))
    {
        all_assertions_passed = false;
    }

    // Test 5: Different data types (char array)
    char result_char = D_SAFE_ARR_IDX(char_array, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX works with char arrays",
                          result_char == 'A',
                          "Should return 'A'"))
    {
        all_assertions_passed = false;
    }

    result_char = D_SAFE_ARR_IDX(char_array, -1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(char_array, -1) returns last char",
                          result_char == 'C',
                          "Should return 'C'"))
    {
        all_assertions_passed = false;
    }

    result_char = D_SAFE_ARR_IDX(char_array, 10);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(char_array, 10) returns first char (OOB)",
                          result_char == 'A',
                          "Should return 'A' for out-of-bounds"))
    {
        all_assertions_passed = false;
    }

    // Test 6: Different data types (double array)
    double result_double = D_SAFE_ARR_IDX(double_array, 1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX works with double arrays",
                          result_double == 2.2,
                          "Should return 2.2"))
    {
        all_assertions_passed = false;
    }

    result_double = D_SAFE_ARR_IDX(double_array, -1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(double_array, -1) returns last double",
                          result_double == 4.4,
                          "Should return 4.4"))
    {
        all_assertions_passed = false;
    }

    result_double = D_SAFE_ARR_IDX(double_array, 100);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_SAFE_ARR_IDX(double_array, 100) returns first (OOB)",
                          result_double == 1.1,
                          "Should return 1.1 for out-of-bounds"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_IS_VALID_INDEX TESTS
 *****************************************************************************/

bool d_tests_djinterp_macro_is_valid_index(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_is_valid_index");

    bool   all_assertions_passed = true;
    bool   result;

    // Test 1: Valid positive indices
    result = D_IS_VALID_INDEX(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX(0, 5) is valid",
                          result == true,
                          "Index 0 should be valid"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX(4, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX(4, 5) is valid",
                          result == true,
                          "Last index should be valid"))
    {
        all_assertions_passed = false;
    }

    // Test 2: Invalid positive indices
    result = D_IS_VALID_INDEX(5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX(5, 5) is invalid",
                          result == false,
                          "Index at SIZE should be invalid"))
    {
        all_assertions_passed = false;
    }

    // Test 3: Valid negative indices
    result = D_IS_VALID_INDEX(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX(-1, 5) is valid",
                          result == true,
                          "Index -1 should be valid"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX(-5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX(-5, 5) is valid",
                          result == true,
                          "Index -SIZE should be valid"))
    {
        all_assertions_passed = false;
    }

    // Test 4: Invalid negative indices
    result = D_IS_VALID_INDEX(-6, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX(-6, 5) is invalid",
                          result == false,
                          "Index beyond -SIZE should be invalid"))
    {
        all_assertions_passed = false;
    }

    // Test 5: Empty array
    result = D_IS_VALID_INDEX(0, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX(0, 0) is invalid",
                          result == false,
                          "Any index on empty array should be invalid"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_INDEX_IN_BOUNDS TESTS
 *****************************************************************************/

bool d_tests_djinterp_macro_index_in_bounds(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_index_in_bounds");

    bool   all_assertions_passed = true;

    // Test alias consistency with D_IS_VALID_INDEX_N
    bool result1 = D_INDEX_IN_BOUNDS(0, 5);
    bool result2 = D_IS_VALID_INDEX_N(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INDEX_IN_BOUNDS(0,5) == D_IS_VALID_INDEX_N(0,5)",
                          result1 == result2,
                          "Aliases should agree"))
    {
        all_assertions_passed = false;
    }

    result1 = D_INDEX_IN_BOUNDS(-1, 5);
    result2 = D_IS_VALID_INDEX_N(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INDEX_IN_BOUNDS(-1,5) == D_IS_VALID_INDEX_N(-1,5)",
                          result1 == result2,
                          "Aliases should agree"))
    {
        all_assertions_passed = false;
    }

    result1 = D_INDEX_IN_BOUNDS(5, 5);
    result2 = D_IS_VALID_INDEX_N(5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_INDEX_IN_BOUNDS(5,5) == D_IS_VALID_INDEX_N(5,5)",
                          result1 == result2,
                          "Aliases should agree on invalid index"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_IS_VALID_INDEX_N TESTS
 *****************************************************************************/

bool d_tests_djinterp_macro_is_valid_index_n(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_is_valid_index_n");

    bool   all_assertions_passed = true;
    bool   result;

    // Valid range: [-SIZE, SIZE)
    result = D_IS_VALID_INDEX_N(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N(0, 5) is valid",
                          result == true,
                          "Index 0 should be valid"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(4, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N(4, 5) is valid",
                          result == true,
                          "Index 4 should be valid"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N(-1, 5) is valid",
                          result == true,
                          "Index -1 should be valid"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N(-5, 5) is valid",
                          result == true,
                          "Index -5 should be valid"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N(5, 5) is invalid",
                          result == false,
                          "Index 5 should be invalid"))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-6, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N(-6, 5) is invalid",
                          result == false,
                          "Index -6 should be invalid"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_NEG_IDX TESTS
 *****************************************************************************/

bool d_tests_djinterp_macro_neg_idx(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_neg_idx");

    bool   all_assertions_passed = true;
    ssize_t result;

    // Positive indices pass through
    result = D_NEG_IDX(0, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX(0, 5) returns 0",
                          result == 0,
                          "Positive index should pass through"))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(3, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX(3, 5) returns 3",
                          result == 3,
                          "Positive index should pass through"))
    {
        all_assertions_passed = false;
    }

    // Negative indices convert
    result = D_NEG_IDX(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX(-1, 5) returns 4",
                          result == 4,
                          "Index -1 should convert to 4"))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-5, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX(-5, 5) returns 0",
                          result == 0,
                          "Index -5 should convert to 0"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * D_ARR_IDX TESTS
 *****************************************************************************/

bool d_tests_djinterp_macro_arr_idx(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_arr_idx");

    bool   all_assertions_passed = true;

    int test_array[5] = {10, 20, 30, 40, 50};

    // Positive indices
    int result = D_ARR_IDX(test_array, 0);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX(array, 0) returns first element",
                          result == 10,
                          "Should return 10"))
    {
        all_assertions_passed = false;
    }

    result = D_ARR_IDX(test_array, 2);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX(array, 2) returns third element",
                          result == 30,
                          "Should return 30"))
    {
        all_assertions_passed = false;
    }

    // Negative indices
    result = D_ARR_IDX(test_array, -1);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX(array, -1) returns last element",
                          result == 50,
                          "Should return 50"))
    {
        all_assertions_passed = false;
    }

    result = D_ARR_IDX(test_array, -5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ARR_IDX(array, -5) returns first element",
                          result == 10,
                          "Should return 10"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * CONSISTENCY TESTS
 *****************************************************************************/

bool d_tests_djinterp_macro_function_consistency(
    struct dj_test_suite* _suite
)
{
    d_test_suite_test(_suite, "macro_function_consistency");

    bool   all_assertions_passed = true;

    // D_NEG_IDX should match d_index_convert_fast for negative indices
    ssize_t macro_result = D_NEG_IDX(-1, 5);
    size_t func_result = d_index_convert_fast(-1, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_NEG_IDX(-1, 5) matches d_index_convert_fast(-1, 5)",
                          macro_result == (ssize_t)func_result,
                          "Macro and function should agree"))
    {
        all_assertions_passed = false;
    }

    // D_IS_VALID_INDEX_N should match d_index_is_valid behavior
    bool macro_valid = D_IS_VALID_INDEX_N(2, 5);
    bool func_valid = d_index_is_valid(2, 5);
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_IS_VALID_INDEX_N(2, 5) matches d_index_is_valid(2, 5)",
                          macro_valid == func_valid,
                          "Macro and function should agree"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * COMPREHENSIVE INDEX MACRO TESTS
 *****************************************************************************/

bool d_tests_djinterp_index_macros_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "index_macros_all");

    d_tests_djinterp_macro_clamp_index(_suite);
    d_tests_djinterp_macro_index_in_bounds(_suite);
    d_tests_djinterp_macro_safe_arr_idx(_suite);
    d_tests_djinterp_macro_is_valid_index(_suite);
    d_tests_djinterp_macro_is_valid_index_n(_suite);
    d_tests_djinterp_macro_neg_idx(_suite);
    d_tests_djinterp_macro_arr_idx(_suite);
    d_tests_djinterp_macro_function_consistency(_suite);

    return true;
}
