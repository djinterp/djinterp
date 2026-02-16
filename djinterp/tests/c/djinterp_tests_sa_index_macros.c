#include ".\djinterp_tests_sa.h"


/******************************************************************************
 * D_CLAMP_INDEX TESTS
 *****************************************************************************/

bool
d_tests_sa_macro_clamp_index
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing D_CLAMP_INDEX macro ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    ssize_t result;

    // Test 1: Negative indices should clamp to 0
    result = D_CLAMP_INDEX(-1, 5);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(-1, 5) clamps to 0",
                            "Negative index should clamp to 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(-10, 5);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(-10, 5) clamps to 0",
                            "Large negative index should clamp to 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Valid indices should pass through unchanged
    result = D_CLAMP_INDEX(0, 5);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(0, 5) returns 0",
                            "Index 0 should remain 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(2, 5);
    if (!d_assert_standalone(result == 2,
                            "D_CLAMP_INDEX(2, 5) returns 2",
                            "Valid middle index should remain unchanged", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(4, 5);
    if (!d_assert_standalone(result == 4,
                            "D_CLAMP_INDEX(4, 5) returns 4",
                            "Last valid index should remain unchanged", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 3: Too-large indices should clamp to SIZE-1
    result = D_CLAMP_INDEX(5, 5);
    if (!d_assert_standalone(result == 4,
                            "D_CLAMP_INDEX(5, 5) clamps to 4",
                            "Index at SIZE should clamp to SIZE-1", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(10, 5);
    if (!d_assert_standalone(result == 4,
                            "D_CLAMP_INDEX(10, 5) clamps to 4",
                            "Large index should clamp to SIZE-1", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 4: Empty array (SIZE == 0) edge case
    result = D_CLAMP_INDEX(0, 0);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(0, 0) returns 0",
                            "Empty array should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(-1, 0);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(-1, 0) returns 0",
                            "Negative index on empty array should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(5, 0);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(5, 0) returns 0",
                            "Positive index on empty array should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 5: Single element array
    result = D_CLAMP_INDEX(-1, 1);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(-1, 1) returns 0",
                            "Negative index on size-1 array clamps to 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(0, 1);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(0, 1) returns 0",
                            "Index 0 on size-1 array returns 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_CLAMP_INDEX(1, 1);
    if (!d_assert_standalone(result == 0,
                            "D_CLAMP_INDEX(1, 1) returns 0",
                            "Index 1 on size-1 array clamps to 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] D_CLAMP_INDEX macro test passed\n");
    }
    else
    {
        printf("  [FAIL] D_CLAMP_INDEX macro test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_SAFE_ARR_IDX TESTS - CRITICAL SAFETY FEATURE
 *****************************************************************************/

bool
d_tests_sa_macro_safe_arr_idx
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing D_SAFE_ARR_IDX macro ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;

    // Setup test arrays
    int int_array[5] = {10, 20, 30, 40, 50};
    char char_array[3] = {'A', 'B', 'C'};
    double double_array[4] = {1.1, 2.2, 3.3, 4.4};

    // Test 1: Valid positive indices
    int result_int = D_SAFE_ARR_IDX(int_array, 0);
    if (!d_assert_standalone(result_int == 10,
                            "D_SAFE_ARR_IDX(int_array, 0) returns first element",
                            "Should return 10", _test_info))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, 2);
    if (!d_assert_standalone(result_int == 30,
                            "D_SAFE_ARR_IDX(int_array, 2) returns third element",
                            "Should return 30", _test_info))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, 4);
    if (!d_assert_standalone(result_int == 50,
                            "D_SAFE_ARR_IDX(int_array, 4) returns last element",
                            "Should return 50", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Valid negative indices (Python-style)
    result_int = D_SAFE_ARR_IDX(int_array, -1);
    if (!d_assert_standalone(result_int == 50,
                            "D_SAFE_ARR_IDX(int_array, -1) returns last element",
                            "Should return 50", _test_info))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, -2);
    if (!d_assert_standalone(result_int == 40,
                            "D_SAFE_ARR_IDX(int_array, -2) returns second-to-last",
                            "Should return 40", _test_info))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, -5);
    if (!d_assert_standalone(result_int == 10,
                            "D_SAFE_ARR_IDX(int_array, -5) returns first element",
                            "Should return 10", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 3: Out-of-bounds positive indices (should return first element)
    result_int = D_SAFE_ARR_IDX(int_array, 5);
    if (!d_assert_standalone(result_int == 10,
                            "D_SAFE_ARR_IDX(int_array, 5) returns first element (OOB)",
                            "Should return 10 for out-of-bounds", _test_info))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, 100);
    if (!d_assert_standalone(result_int == 10,
                            "D_SAFE_ARR_IDX(int_array, 100) returns first element (OOB)",
                            "Should return 10 for large out-of-bounds", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 4: Out-of-bounds negative indices (should return first element)
    result_int = D_SAFE_ARR_IDX(int_array, -6);
    if (!d_assert_standalone(result_int == 10,
                            "D_SAFE_ARR_IDX(int_array, -6) returns first element (OOB)",
                            "Should return 10 for negative out-of-bounds", _test_info))
    {
        all_assertions_passed = false;
    }

    result_int = D_SAFE_ARR_IDX(int_array, -100);
    if (!d_assert_standalone(result_int == 10,
                            "D_SAFE_ARR_IDX(int_array, -100) returns first element (OOB)",
                            "Should return 10 for large negative out-of-bounds", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 5: Different data types (char array)
    char result_char = D_SAFE_ARR_IDX(char_array, 0);
    if (!d_assert_standalone(result_char == 'A',
                            "D_SAFE_ARR_IDX works with char arrays",
                            "Should return 'A'", _test_info))
    {
        all_assertions_passed = false;
    }

    result_char = D_SAFE_ARR_IDX(char_array, -1);
    if (!d_assert_standalone(result_char == 'C',
                            "D_SAFE_ARR_IDX(char_array, -1) returns last char",
                            "Should return 'C'", _test_info))
    {
        all_assertions_passed = false;
    }

    result_char = D_SAFE_ARR_IDX(char_array, 10);
    if (!d_assert_standalone(result_char == 'A',
                            "D_SAFE_ARR_IDX(char_array, 10) returns first char (OOB)",
                            "Should return 'A' for out-of-bounds", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 6: Different data types (double array)
    double result_double = D_SAFE_ARR_IDX(double_array, 1);
    if (!d_assert_standalone(result_double == 2.2,
                            "D_SAFE_ARR_IDX works with double arrays",
                            "Should return 2.2", _test_info))
    {
        all_assertions_passed = false;
    }

    result_double = D_SAFE_ARR_IDX(double_array, -1);
    if (!d_assert_standalone(result_double == 4.4,
                            "D_SAFE_ARR_IDX(double_array, -1) returns last double",
                            "Should return 4.4", _test_info))
    {
        all_assertions_passed = false;
    }

    result_double = D_SAFE_ARR_IDX(double_array, 100);
    if (!d_assert_standalone(result_double == 1.1,
                            "D_SAFE_ARR_IDX(double_array, 100) returns first (OOB)",
                            "Should return 1.1 for out-of-bounds", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] D_SAFE_ARR_IDX macro test passed\n");
    }
    else
    {
        printf("  [FAIL] D_SAFE_ARR_IDX macro test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_IS_VALID_INDEX TESTS
 *****************************************************************************/

bool
d_tests_sa_macro_is_valid_index
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing D_IS_VALID_INDEX macro ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    bool   result;

    // Test 1: Valid positive indices
    result = D_IS_VALID_INDEX(0, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX(0, 5) is valid",
                            "Index 0 should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX(4, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX(4, 5) is valid",
                            "Last index should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 2: Invalid positive indices
    result = D_IS_VALID_INDEX(5, 5);
    if (!d_assert_standalone(result == false,
                            "D_IS_VALID_INDEX(5, 5) is invalid",
                            "Index at SIZE should be invalid", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 3: Valid negative indices
    result = D_IS_VALID_INDEX(-1, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX(-1, 5) is valid",
                            "Index -1 should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX(-5, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX(-5, 5) is valid",
                            "Index -SIZE should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 4: Invalid negative indices
    result = D_IS_VALID_INDEX(-6, 5);
    if (!d_assert_standalone(result == false,
                            "D_IS_VALID_INDEX(-6, 5) is invalid",
                            "Index beyond -SIZE should be invalid", _test_info))
    {
        all_assertions_passed = false;
    }

    // Test 5: Empty array
    result = D_IS_VALID_INDEX(0, 0);
    if (!d_assert_standalone(result == false,
                            "D_IS_VALID_INDEX(0, 0) is invalid",
                            "Any index on empty array should be invalid", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] D_IS_VALID_INDEX macro test passed\n");
    }
    else
    {
        printf("  [FAIL] D_IS_VALID_INDEX macro test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_INDEX_IN_BOUNDS TESTS
 *****************************************************************************/

bool
d_tests_sa_macro_index_in_bounds
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing D_INDEX_IN_BOUNDS macro (alias consistency) ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;

    // Test alias consistency with D_IS_VALID_INDEX_N
    bool result1 = D_INDEX_IN_BOUNDS(0, 5);
    bool result2 = D_IS_VALID_INDEX_N(0, 5);
    if (!d_assert_standalone(result1 == result2,
                            "D_INDEX_IN_BOUNDS(0,5) == D_IS_VALID_INDEX_N(0,5)",
                            "Aliases should agree", _test_info))
    {
        all_assertions_passed = false;
    }

    result1 = D_INDEX_IN_BOUNDS(-1, 5);
    result2 = D_IS_VALID_INDEX_N(-1, 5);
    if (!d_assert_standalone(result1 == result2,
                            "D_INDEX_IN_BOUNDS(-1,5) == D_IS_VALID_INDEX_N(-1,5)",
                            "Aliases should agree", _test_info))
    {
        all_assertions_passed = false;
    }

    result1 = D_INDEX_IN_BOUNDS(5, 5);
    result2 = D_IS_VALID_INDEX_N(5, 5);
    if (!d_assert_standalone(result1 == result2,
                            "D_INDEX_IN_BOUNDS(5,5) == D_IS_VALID_INDEX_N(5,5)",
                            "Aliases should agree on invalid index", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] D_INDEX_IN_BOUNDS alias consistency test passed\n");
    }
    else
    {
        printf("  [FAIL] D_INDEX_IN_BOUNDS alias consistency test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_IS_VALID_INDEX_N TESTS
 *****************************************************************************/

bool
d_tests_sa_macro_is_valid_index_n
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing D_IS_VALID_INDEX_N macro ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    bool   result;

    // Valid range: [-SIZE, SIZE)
    result = D_IS_VALID_INDEX_N(0, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX_N(0, 5) is valid",
                            "Index 0 should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(4, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX_N(4, 5) is valid",
                            "Index 4 should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-1, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX_N(-1, 5) is valid",
                            "Index -1 should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-5, 5);
    if (!d_assert_standalone(result == true,
                            "D_IS_VALID_INDEX_N(-5, 5) is valid",
                            "Index -5 should be valid", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(5, 5);
    if (!d_assert_standalone(result == false,
                            "D_IS_VALID_INDEX_N(5, 5) is invalid",
                            "Index 5 should be invalid", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-6, 5);
    if (!d_assert_standalone(result == false,
                            "D_IS_VALID_INDEX_N(-6, 5) is invalid",
                            "Index -6 should be invalid", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] D_IS_VALID_INDEX_N macro test passed\n");
    }
    else
    {
        printf("  [FAIL] D_IS_VALID_INDEX_N macro test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_NEG_IDX TESTS
 *****************************************************************************/

bool
d_tests_sa_macro_neg_idx
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing D_NEG_IDX macro ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    ssize_t result;

    // Positive indices pass through
    result = D_NEG_IDX(0, 5);
    if (!d_assert_standalone(result == 0,
                            "D_NEG_IDX(0, 5) returns 0",
                            "Positive index should pass through", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(3, 5);
    if (!d_assert_standalone(result == 3,
                            "D_NEG_IDX(3, 5) returns 3",
                            "Positive index should pass through", _test_info))
    {
        all_assertions_passed = false;
    }

    // Negative indices convert
    result = D_NEG_IDX(-1, 5);
    if (!d_assert_standalone(result == 4,
                            "D_NEG_IDX(-1, 5) returns 4",
                            "Index -1 should convert to 4", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-5, 5);
    if (!d_assert_standalone(result == 0,
                            "D_NEG_IDX(-5, 5) returns 0",
                            "Index -5 should convert to 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] D_NEG_IDX macro test passed\n");
    }
    else
    {
        printf("  [FAIL] D_NEG_IDX macro test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * D_ARR_IDX TESTS
 *****************************************************************************/

bool
d_tests_sa_macro_arr_idx
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing D_ARR_IDX macro ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;

    int test_array[5] = {10, 20, 30, 40, 50};

    // Positive indices
    int result = D_ARR_IDX(test_array, 0);
    if (!d_assert_standalone(result == 10,
                            "D_ARR_IDX(array, 0) returns first element",
                            "Should return 10", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_ARR_IDX(test_array, 2);
    if (!d_assert_standalone(result == 30,
                            "D_ARR_IDX(array, 2) returns third element",
                            "Should return 30", _test_info))
    {
        all_assertions_passed = false;
    }

    // Negative indices
    result = D_ARR_IDX(test_array, -1);
    if (!d_assert_standalone(result == 50,
                            "D_ARR_IDX(array, -1) returns last element",
                            "Should return 50", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_ARR_IDX(test_array, -5);
    if (!d_assert_standalone(result == 10,
                            "D_ARR_IDX(array, -5) returns first element",
                            "Should return 10", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] D_ARR_IDX macro test passed\n");
    }
    else
    {
        printf("  [FAIL] D_ARR_IDX macro test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * CONSISTENCY TESTS
 *****************************************************************************/

bool
d_tests_sa_macro_function_consistency
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing macro-function consistency ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;

    // D_NEG_IDX should match d_index_convert_fast for negative indices
    ssize_t macro_result = D_NEG_IDX(-1, 5);
    size_t func_result = d_index_convert_fast(-1, 5);
    if (!d_assert_standalone(macro_result == (ssize_t)func_result,
                            "D_NEG_IDX(-1, 5) matches d_index_convert_fast(-1, 5)",
                            "Macro and function should agree", _test_info))
    {
        all_assertions_passed = false;
    }

    // D_IS_VALID_INDEX_N should match d_index_is_valid behavior
    bool macro_valid = D_IS_VALID_INDEX_N(2, 5);
    bool func_valid = d_index_is_valid(2, 5);
    if (!d_assert_standalone(macro_valid == func_valid,
                            "D_IS_VALID_INDEX_N(2, 5) matches d_index_is_valid(2, 5)",
                            "Macro and function should agree", _test_info))
    {
        all_assertions_passed = false;
    }

    // Update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] Macro-function consistency test passed\n");
    }
    else
    {
        printf("  [FAIL] Macro-function consistency test failed\n");
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * COMPREHENSIVE INDEX MACRO TESTS
 *****************************************************************************/

bool
d_tests_sa_index_macros_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n========================================\n");
    printf("  INDEX MANIPULATION MACRO TESTS\n");
    printf("========================================\n");

    struct d_test_counter module_counter = {0, 0, 0, 0};

    // Run all index macro tests
    d_tests_sa_macro_clamp_index(&module_counter);
    d_tests_sa_macro_index_in_bounds(&module_counter);
    d_tests_sa_macro_safe_arr_idx(&module_counter);
    d_tests_sa_macro_is_valid_index(&module_counter);
    d_tests_sa_macro_is_valid_index_n(&module_counter);
    d_tests_sa_macro_neg_idx(&module_counter);
    d_tests_sa_macro_arr_idx(&module_counter);
    d_tests_sa_macro_function_consistency(&module_counter);

    // Update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    bool all_passed = (module_counter.tests_passed == module_counter.tests_total);

    printf("\n--- Index Manipulation Macro Tests Summary ---\n");
    printf("  Assertions: %zu/%zu passed (%.1f%%)\n",
           module_counter.assertions_passed, module_counter.assertions_total,
           (module_counter.assertions_total > 0) ?
           (100.0 * module_counter.assertions_passed / module_counter.assertions_total) : 0.0);
    printf("  Unit Tests: %zu/%zu passed (%.1f%%)\n",
           module_counter.tests_passed, module_counter.tests_total,
           (module_counter.tests_total > 0) ?
           (100.0 * module_counter.tests_passed / module_counter.tests_total) : 0.0);
    printf("  Status: %s\n", all_passed ? "[PASS]" : "[FAIL]");
    printf("========================================\n\n");

    return all_passed;
}
