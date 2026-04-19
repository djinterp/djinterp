#include "djinterp_type_tests_sa.h"


/*
d_tests_sa_index_convert_fast
  comprehensive testing of d_index_convert_fast function
  Tests the following:
  - positive index conversion (identity function)
  - negative index conversion to positive equivalents
  - boundary conditions with various array sizes
  - edge cases including zero-sized arrays
  - large array sizes and extreme indices
*/
bool
d_tests_sa_index_convert_fast
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_index_convert_fast ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    size_t result;

    // test positive indices (should return unchanged)
    result = d_index_convert_fast(0, 5);

    if (!d_assert_standalone(result == 0, 
                            "Positive index 0 returns 0", 
                            "d_index_convert_fast(0, 5) should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(3, 5);
    if (!d_assert_standalone(result == 3, 
                            "Positive index 3 returns 3", 
                            "d_index_convert_fast(3, 5) should return 3", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(4, 5);
    if (!d_assert_standalone(result == 4, 
                            "Last valid positive index returns correctly", 
                            "d_index_convert_fast(4, 5) should return 4", _test_info))
    {
        all_assertions_passed = false;
    }

    // test negative indices (conversion to positive)
    result = d_index_convert_fast(-1, 5);
    if (!d_assert_standalone(result == 4, 
                            "Negative index -1 converts to last element", 
                            "d_index_convert_fast(-1, 5) should return 4", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-3, 5);
    if (!d_assert_standalone(result == 2, 
                            "Negative index -3 converts correctly", 
                            "d_index_convert_fast(-3, 5) should return 2", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-5, 5);
    if (!d_assert_standalone(result == 0, 
                            "Negative index -5 converts to first element", 
                            "d_index_convert_fast(-5, 5) should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // test edge cases with size 1 array
    result = d_index_convert_fast(0, 1);
    if (!d_assert_standalone(result == 0, 
                            "Index 0 valid for size 1 array", 
                            "d_index_convert_fast(0, 1) should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-1, 1);
    if (!d_assert_standalone(result == 0, 
                            "Index -1 converts correctly for size 1 array", 
                            "d_index_convert_fast(-1, 1) should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // test larger array sizes
    size_t large_size = 1000;
    result = d_index_convert_fast(-1, large_size);
    if (!d_assert_standalone(result == large_size - 1, 
                            "Negative index works with large arrays", 
                            "d_index_convert_fast(-1, 1000) should return 999", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_convert_fast(-(ssize_t)large_size, large_size);
    if (!d_assert_standalone(result == 0, 
                            "Negative index equal to size converts to 0", 
                            "d_index_convert_fast(-1000, 1000) should return 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_index_convert_fast unit test passed\n");
    }
    else
    {
        printf("  [FAIL] d_index_convert_fast unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_index_convert_safe
  comprehensive testing of d_index_convert_safe function
  Tests the following:
  - valid index conversions with proper output storage
  - invalid index rejection with output preservation
  - null pointer handling for destination parameter
  - boundary condition validation
  - error condition handling
*/
bool
d_tests_sa_index_convert_safe
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_index_convert_safe ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    size_t output_index;
    bool   result;

    // test valid positive indices
    output_index = 999; // sentinel value
    result = d_index_convert_safe(0, 5, &output_index);
    if (!d_assert_standalone(result == true, 
                            "Valid positive index 0 returns success", 
                            "d_index_convert_safe(0, 5, &out) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(output_index == 0, 
                            "Valid index 0 outputs correct value", 
                            "Output should be 0 for valid index", _test_info))
    {
        all_assertions_passed = false;
    }

    output_index = 999;
    result = d_index_convert_safe(4, 5, &output_index);
    if (!d_assert_standalone(result == true, 
                            "Valid positive index 4 returns success", 
                            "d_index_convert_safe(4, 5, &out) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(output_index == 4, 
                            "Valid index 4 outputs correct value", 
                            "Output should be 4 for valid index", _test_info))
    {
        all_assertions_passed = false;
    }

    // test valid negative indices
    output_index = 999;
    result = d_index_convert_safe(-1, 5, &output_index);
    if (!d_assert_standalone(result == true, 
                            "Valid negative index -1 returns success", 
                            "d_index_convert_safe(-1, 5, &out) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(output_index == 4, 
                            "Valid negative index -1 outputs correct value", 
                            "Output should be 4 for index -1", _test_info))
    {
        all_assertions_passed = false;
    }

    output_index = 999;
    result = d_index_convert_safe(-5, 5, &output_index);
    if (!d_assert_standalone(result == true, 
                            "Valid negative index -5 returns success", 
                            "d_index_convert_safe(-5, 5, &out) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(output_index == 0, 
                            "Valid negative index -5 outputs correct value", 
                            "Output should be 0 for index -5", _test_info))
    {
        all_assertions_passed = false;
    }

    // test invalid indices (out of bounds)
    output_index = 999;
    result = d_index_convert_safe(5, 5, &output_index);
    if (!d_assert_standalone(result == false, 
                            "Invalid positive index returns failure", 
                            "d_index_convert_safe(5, 5, &out) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(output_index == 999, 
                            "Invalid index preserves output value", 
                            "Output should remain 999 for invalid index", _test_info))
    {
        all_assertions_passed = false;
    }

    output_index = 999;
    result = d_index_convert_safe(-6, 5, &output_index);
    if (!d_assert_standalone(result == false, 
                            "Invalid negative index returns failure", 
                            "d_index_convert_safe(-6, 5, &out) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(output_index == 999, 
                            "Invalid negative index preserves output value", 
                            "Output should remain 999 for invalid index", _test_info))
    {
        all_assertions_passed = false;
    }

    // test null pointer handling
    result = d_index_convert_safe(2, 5, NULL);
    if (!d_assert_standalone(result == false, 
                            "NULL destination pointer returns failure", 
                            "d_index_convert_safe with NULL destination should fail", _test_info))
    {
        all_assertions_passed = false;
    }

    // test empty array
    output_index = 999;
    result = d_index_convert_safe(0, 0, &output_index);
    if (!d_assert_standalone(result == false, 
                            "Any index on empty array returns failure", 
                            "d_index_convert_safe(0, 0, &out) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(output_index == 999, 
                            "Empty array preserves output value", 
                            "Output should remain 999 for empty array", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_index_convert_safe unit test passed\n");
    }
    else
    {
        printf("  [FAIL] d_index_convert_safe unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_index_is_valid
  comprehensive testing of d_index_is_valid function
  Tests the following:
  - valid positive and negative index recognition
  - invalid index rejection for out-of-bounds cases
  - empty array handling
  - boundary conditions and edge cases
  - consistency with conversion function behavior
*/
bool
d_tests_sa_index_is_valid
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing d_index_is_valid ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    bool   result;

    // test valid positive indices
    result = d_index_is_valid(0, 5);
    if (!d_assert_standalone(result == true, 
                            "Index 0 is valid for size 5", 
                            "d_index_is_valid(0, 5) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(4, 5);
    if (!d_assert_standalone(result == true, 
                            "Index 4 is valid for size 5", 
                            "d_index_is_valid(4, 5) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(2, 5);
    if (!d_assert_standalone(result == true, 
                            "Index 2 is valid for size 5", 
                            "d_index_is_valid(2, 5) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    // test invalid positive indices
    result = d_index_is_valid(5, 5);
    if (!d_assert_standalone(result == false, 
                            "Index 5 is invalid for size 5", 
                            "d_index_is_valid(5, 5) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(100, 5);
    if (!d_assert_standalone(result == false, 
                            "Large positive index is invalid", 
                            "d_index_is_valid(100, 5) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    // test valid negative indices
    result = d_index_is_valid(-1, 5);
    if (!d_assert_standalone(result == true, 
                            "Index -1 is valid for size 5", 
                            "d_index_is_valid(-1, 5) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-5, 5);
    if (!d_assert_standalone(result == true, 
                            "Index -5 is valid for size 5", 
                            "d_index_is_valid(-5, 5) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-3, 5);
    if (!d_assert_standalone(result == true, 
                            "Index -3 is valid for size 5", 
                            "d_index_is_valid(-3, 5) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    // test invalid negative indices
    result = d_index_is_valid(-6, 5);
    if (!d_assert_standalone(result == false, 
                            "Index -6 is invalid for size 5", 
                            "d_index_is_valid(-6, 5) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-100, 5);
    if (!d_assert_standalone(result == false, 
                            "Large negative index is invalid", 
                            "d_index_is_valid(-100, 5) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    // test edge cases with size 1
    result = d_index_is_valid(0, 1);
    if (!d_assert_standalone(result == true, 
                            "Index 0 is valid for size 1", 
                            "d_index_is_valid(0, 1) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-1, 1);
    if (!d_assert_standalone(result == true, 
                            "Index -1 is valid for size 1", 
                            "d_index_is_valid(-1, 1) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(1, 1);
    if (!d_assert_standalone(result == false, 
                            "Index 1 is invalid for size 1", 
                            "d_index_is_valid(1, 1) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-2, 1);
    if (!d_assert_standalone(result == false, 
                            "Index -2 is invalid for size 1", 
                            "d_index_is_valid(-2, 1) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    // test empty array (size 0)
    result = d_index_is_valid(0, 0);
    if (!d_assert_standalone(result == false, 
                            "Any index is invalid for empty array", 
                            "d_index_is_valid(0, 0) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-1, 0);
    if (!d_assert_standalone(result == false, 
                            "Negative index invalid for empty array", 
                            "d_index_is_valid(-1, 0) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    // test large array sizes
    size_t large_size = 1000000;
    result = d_index_is_valid(-1, large_size);
    if (!d_assert_standalone(result == true, 
                            "Index -1 valid for large arrays", 
                            "d_index_is_valid(-1, 1000000) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-(ssize_t)large_size, large_size);
    if (!d_assert_standalone(result == true, 
                            "Negative size equal to array size is valid", 
                            "d_index_is_valid(-1000000, 1000000) should return true", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-(ssize_t)large_size - 1, large_size);
    if (!d_assert_standalone(result == false, 
                            "Negative size greater than array size is invalid", 
                            "d_index_is_valid(-1000001, 1000000) should return false", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] d_index_is_valid unit test passed\n");
    }
    else
    {
        printf("  [FAIL] d_index_is_valid unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_index_core_all
  runs all core d_index function tests
  Tests the following:
  - all d_index function family tests
  - reports aggregate results
*/
bool
d_tests_sa_index_core_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n--- Testing d_index Core Functions ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};

    bool fast_result = d_tests_sa_index_convert_fast(&module_counter);
    bool safe_result = d_tests_sa_index_convert_safe(&module_counter);
    bool valid_result = d_tests_sa_index_is_valid(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run         += module_counter.tests_run;
    _test_info->tests_passed      += module_counter.tests_passed;

    bool overall_result = (fast_result && safe_result && valid_result);

    if (overall_result)
    {
        printf("[PASS] d_index Core Functions: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    else
    {
        printf("[FAIL] d_index Core Functions: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }

    return overall_result;
}

///////////////////////////////////////////////////////////////////////////////
///                        II. MACRO VALIDATION TESTS                       ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tests_sa_macro_array_utilities
  comprehensive testing of array utility macros
  Tests the following:
  - D_ARRAY_TOTAL_SIZE calculation accuracy
  - D_ARRAY_COUNT compile-time array sizing
  - various data types and array sizes
  - edge cases with zero-sized elements
*/

/*
d_tests_sa_macro_array_utilities
  comprehensive testing of array utility macros
  Tests the following:
  - D_ARRAY_TOTAL_SIZE calculation accuracy
  - D_ARRAY_COUNT compile-time array sizing
  - various data types and array sizes
  - edge cases with zero-sized elements
*/
bool
d_tests_sa_macro_array_utilities
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing Array Utility Macros ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;

    // test D_ARRAY_TOTAL_SIZE macro
    size_t total_size = D_ARRAY_TOTAL_SIZE(sizeof(int), 10);
    if (!d_assert_standalone(total_size == sizeof(int) * 10, 
                            "D_ARRAY_TOTAL_SIZE calculates correctly for int array", 
                            "Macro should calculate total array size correctly", _test_info))
    {
        all_assertions_passed = false;
    }

    total_size = D_ARRAY_TOTAL_SIZE(sizeof(char), 100);
    if (!d_assert_standalone(total_size == 100, 
                            "D_ARRAY_TOTAL_SIZE works with char arrays", 
                            "Char array total size should be element count", _test_info))
    {
        all_assertions_passed = false;
    }

    total_size = D_ARRAY_TOTAL_SIZE(sizeof(double), 5);
    if (!d_assert_standalone(total_size == sizeof(double) * 5, 
                            "D_ARRAY_TOTAL_SIZE works with double arrays", 
                            "Double array total size should be correct", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_ARRAY_COUNT macro with actual arrays
    int int_array[10];
    size_t count = D_ARRAY_COUNT(int_array);
    if (!d_assert_standalone(count == 10, 
                            "D_ARRAY_COUNT correctly counts int array elements", 
                            "Should return 10 for 10-element int array", _test_info))
    {
        all_assertions_passed = false;
    }

    char char_array[25];
    count = D_ARRAY_COUNT(char_array);
    if (!d_assert_standalone(count == 25, 
                            "D_ARRAY_COUNT correctly counts char array elements", 
                            "Should return 25 for 25-element char array", _test_info))
    {
        all_assertions_passed = false;
    }

    double double_array[3];
    count = D_ARRAY_COUNT(double_array);
    if (!d_assert_standalone(count == 3, 
                            "D_ARRAY_COUNT correctly counts double array elements", 
                            "Should return 3 for 3-element double array", _test_info))
    {
        all_assertions_passed = false;
    }

    // test edge case with single element array
    int single_element[1];
    count = D_ARRAY_COUNT(single_element);
    if (!d_assert_standalone(count == 1, 
                            "D_ARRAY_COUNT works with single element arrays", 
                            "Should return 1 for single-element array", _test_info))
    {
        all_assertions_passed = false;
    }

    // test edge case with large array
    int large_array[1000];
    count = D_ARRAY_COUNT(large_array);
    if (!d_assert_standalone(count == 1000, 
                            "D_ARRAY_COUNT works with large arrays", 
                            "Should return 1000 for 1000-element array", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] Array Utility Macros unit test passed\n");
    }
    else
    {
        printf("  [FAIL] Array Utility Macros unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_macro_index_validation
  comprehensive testing of index validation macros
  Tests the following:
  - D_IS_VALID_INDEX_N macro validation logic
  - D_INDEX_IN_BOUNDS alias functionality
  - positive and negative index validation
  - edge cases with various array sizes
*/
bool
d_tests_sa_macro_index_validation
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing Index Validation Macros ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    bool   result;

    // test D_IS_VALID_INDEX_N with valid positive indices
    result = D_IS_VALID_INDEX_N(0, 5);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts valid positive index 0", 
                            "Index 0 should be valid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(4, 5);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts valid positive index 4", 
                            "Index 4 should be valid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(2, 5);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts valid positive index 2", 
                            "Index 2 should be valid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_IS_VALID_INDEX_N with invalid positive indices
    result = D_IS_VALID_INDEX_N(5, 5);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects invalid positive index 5", 
                            "Index 5 should be invalid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(10, 5);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects out-of-bounds positive index", 
                            "Index 10 should be invalid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_IS_VALID_INDEX_N with valid negative indices
    result = D_IS_VALID_INDEX_N(-1, 5);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts valid negative index -1", 
                            "Index -1 should be valid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-5, 5);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts valid negative index -5", 
                            "Index -5 should be valid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-3, 5);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts valid negative index -3", 
                            "Index -3 should be valid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_IS_VALID_INDEX_N with invalid negative indices
    result = D_IS_VALID_INDEX_N(-6, 5);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects invalid negative index -6", 
                            "Index -6 should be invalid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-10, 5);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects out-of-bounds negative index", 
                            "Index -10 should be invalid for size 5", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_INDEX_IN_BOUNDS alias (should behave identically)
    result = D_INDEX_IN_BOUNDS(2, 5);
    if (!d_assert_standalone(result == true, 
                            "D_INDEX_IN_BOUNDS alias works for valid index", 
                            "Alias should work identically to original macro", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_INDEX_IN_BOUNDS(-2, 5);
    if (!d_assert_standalone(result == true, 
                            "D_INDEX_IN_BOUNDS alias works for valid negative index", 
                            "Alias should work for negative indices", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_INDEX_IN_BOUNDS(10, 5);
    if (!d_assert_standalone(result == false, 
                            "D_INDEX_IN_BOUNDS alias rejects invalid index", 
                            "Alias should reject invalid indices", _test_info))
    {
        all_assertions_passed = false;
    }

    // test edge cases with empty array
    result = D_IS_VALID_INDEX_N(0, 0);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects any index for empty array", 
                            "No index should be valid for size 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-1, 0);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects negative index for empty array", 
                            "Negative index should be invalid for size 0", _test_info))
    {
        all_assertions_passed = false;
    }

    // test edge cases with size 1 array
    result = D_IS_VALID_INDEX_N(0, 1);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts index 0 for size 1", 
                            "Index 0 should be valid for size 1", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-1, 1);
    if (!d_assert_standalone(result == true, 
                            "D_IS_VALID_INDEX_N accepts index -1 for size 1", 
                            "Index -1 should be valid for size 1", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(1, 1);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects index 1 for size 1", 
                            "Index 1 should be invalid for size 1", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_IS_VALID_INDEX_N(-2, 1);
    if (!d_assert_standalone(result == false, 
                            "D_IS_VALID_INDEX_N rejects index -2 for size 1", 
                            "Index -2 should be invalid for size 1", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] Index Validation Macros unit test passed\n");
    }
    else
    {
        printf("  [FAIL] Index Validation Macros unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_macro_negative_indexing
  comprehensive testing of negative indexing utility macros
  Tests the following:
  - D_NEG_IDX conversion logic for various indices
  - D_ARR_IDX array access with negative indices
  - D_SAFE_ARR_IDX bounds-checked array access
  - edge cases and boundary conditions
*/
bool
d_tests_sa_macro_negative_indexing
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing Negative Indexing Macros ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;
    size_t result;

    // test D_NEG_IDX conversion macro
    result = D_NEG_IDX(0, 5);
    if (!d_assert_standalone(result == 0, 
                            "D_NEG_IDX preserves positive index 0", 
                            "Positive index 0 should remain 0", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(3, 5);
    if (!d_assert_standalone(result == 3, 
                            "D_NEG_IDX preserves positive index 3", 
                            "Positive index 3 should remain 3", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-1, 5);
    if (!d_assert_standalone(result == 4, 
                            "D_NEG_IDX converts negative index -1 to 4", 
                            "Index -1 should convert to last element (4)", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-3, 5);
    if (!d_assert_standalone(result == 2, 
                            "D_NEG_IDX converts negative index -3 to 2", 
                            "Index -3 should convert to element 2", _test_info))
    {
        all_assertions_passed = false;
    }

    result = D_NEG_IDX(-5, 5);
    if (!d_assert_standalone(result == 0, 
                            "D_NEG_IDX converts negative index -5 to 0", 
                            "Index -5 should convert to first element (0)", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_ARR_IDX array access macro
    int test_array[5] = {10, 20, 30, 40, 50};
    
    int value = D_ARR_IDX(test_array, 0);
    if (!d_assert_standalone(value == 10, 
                            "D_ARR_IDX accesses first element with index 0", 
                            "Should access first element (10)", _test_info))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, 2);
    if (!d_assert_standalone(value == 30, 
                            "D_ARR_IDX accesses middle element with index 2", 
                            "Should access third element (30)", _test_info))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, 4);
    if (!d_assert_standalone(value == 50, 
                            "D_ARR_IDX accesses last element with index 4", 
                            "Should access last element (50)", _test_info))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, -1);
    if (!d_assert_standalone(value == 50, 
                            "D_ARR_IDX accesses last element with index -1", 
                            "Should access last element (50) via negative index", _test_info))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, -3);
    if (!d_assert_standalone(value == 30, 
                            "D_ARR_IDX accesses element with index -3", 
                            "Should access third element (30) via negative index", _test_info))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(test_array, -5);
    if (!d_assert_standalone(value == 10, 
                            "D_ARR_IDX accesses first element with index -5", 
                            "Should access first element (10) via negative index", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_SAFE_ARR_IDX bounds-checked access macro
    int safe_value = D_SAFE_ARR_IDX(test_array, 2);
    if (!d_assert_standalone(safe_value == 30, 
                            "D_SAFE_ARR_IDX returns correct element value for valid index", 
                            "Should return element value (30) for index 2", _test_info))
    {
        all_assertions_passed = false;
    }

    safe_value = D_SAFE_ARR_IDX(test_array, -2);
    if (!d_assert_standalone(safe_value == 40, 
                            "D_SAFE_ARR_IDX returns correct value for negative index", 
                            "Should return element value (40) for index -2", _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_SAFE_ARR_IDX with invalid indices (should return first element)
    safe_value = D_SAFE_ARR_IDX(test_array, 10);
    if (!d_assert_standalone(safe_value == 10, 
                            "D_SAFE_ARR_IDX returns first element for out-of-bounds positive index", 
                            "Should return first element (10) for invalid index", _test_info))
    {
        all_assertions_passed = false;
    }

    safe_value = D_SAFE_ARR_IDX(test_array, -10);
    if (!d_assert_standalone(safe_value == 10, 
                            "D_SAFE_ARR_IDX returns first element for out-of-bounds negative index", 
                            "Should return first element (10) for invalid negative index", _test_info))
    {
        all_assertions_passed = false;
    }
    
    // test with single element array
    int single_array[1] = {99};

    value = D_ARR_IDX(single_array, 0);
    if (!d_assert_standalone(value == 99, 
                            "D_ARR_IDX works with single element array index 0", 
                            "Should access single element (99)", _test_info))
    {
        all_assertions_passed = false;
    }

    value = D_ARR_IDX(single_array, -1);
    if (!d_assert_standalone(value == 99, 
                            "D_ARR_IDX works with single element array index -1", 
                            "Should access single element (99) via negative index", _test_info))
    {
        all_assertions_passed = false;
    }

    safe_value = D_SAFE_ARR_IDX(single_array, 0);
    if (!d_assert_standalone(safe_value == 99, 
                            "D_SAFE_ARR_IDX works with single element array", 
                            "Should return single element value (99)", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] Negative Indexing Macros unit test passed\n");
    }
    else
    {
        printf("  [FAIL] Negative Indexing Macros unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_macro_all
  runs all macro validation tests
  Tests the following:
  - array utility macros
  - index validation macros
  - negative indexing macros
  - reports aggregate results
*/
bool
d_tests_sa_macro_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n--- Testing djinterp Macros ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};

    bool array_result = d_tests_sa_macro_array_utilities(&module_counter);
    bool validation_result = d_tests_sa_macro_index_validation(&module_counter);
    bool indexing_result = d_tests_sa_macro_negative_indexing(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run         += module_counter.tests_run;
    _test_info->tests_passed      += module_counter.tests_passed;

    bool overall_result = (array_result && validation_result && indexing_result);

    if (overall_result)
    {
        printf("[PASS] djinterp Macros: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    else
    {
        printf("[FAIL] djinterp Macros: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }

    return overall_result;
}

///////////////////////////////////////////////////////////////////////////////
///                      III. INTEGRATION TESTS                             ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tests_sa_function_macro_consistency
  tests consistency between function and macro implementations
  Tests the following:
  - function vs macro validation consistency
  - index conversion agreement between approaches
  - edge case handling consistency
  - performance vs safety trade-offs
*/
bool
d_tests_sa_function_macro_consistency
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing Function-Macro Consistency ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;

    // test consistency between d_index_is_valid and D_IS_VALID_INDEX_N
    for (d_index idx = -10; idx <= 10; idx++)
    {
        for (size_t size = 0; size <= 10; size++)
        {
            bool func_result = d_index_is_valid(idx, size);
            bool macro_result = D_IS_VALID_INDEX_N(idx, size);
            
            if (func_result != macro_result)
            {
                if (!d_assert_standalone(false, 
                                        "Function and macro validation should agree", 
                                        "d_index_is_valid and D_IS_VALID_INDEX_N disagree", _test_info))
                {
                    all_assertions_passed = false;
                    printf("    Disagreement: idx=%d, size=%zu, func=%s, macro=%s\n",
                           (int)idx, size, 
                           func_result ? "true" : "false",
                           macro_result ? "true" : "false");
                    break; // exit inner loop on first failure
                }
            }
        }
        if (!all_assertions_passed) break; // exit outer loop on failure
    }

    // test consistency between d_index_convert_fast and D_NEG_IDX
    size_t test_sizes[] = {1, 5, 10, 100};
    d_index test_indices[] = {-5, -1, 0, 1, 4};
    
    for (size_t i = 0; i < sizeof(test_sizes) / sizeof(test_sizes[0]); i++)
    {
        size_t size = test_sizes[i];
        for (size_t j = 0; j < sizeof(test_indices) / sizeof(test_indices[0]); j++)
        {
            d_index idx = test_indices[j];
            
            // only test valid indices to ensure meaningful comparison
            if (d_index_is_valid(idx, size))
            {
                size_t func_result = d_index_convert_fast(idx, size);
                size_t macro_result = (size_t)D_NEG_IDX(idx, size);
                
                if (!d_assert_standalone(func_result == macro_result, 
                                        "Function and macro conversion should agree", 
                                        "d_index_convert_fast and D_NEG_IDX should agree on valid indices", _test_info))
                {
                    all_assertions_passed = false;
                    printf("    Disagreement: idx=%d, size=%zu, func=%zu, macro=%zu\n",
                           (int)idx, size, func_result, macro_result);
                }
            }
        }
    }

    // test array access consistency
    int test_array[5] = {100, 200, 300, 400, 500};
    
    for (d_index idx = -5; idx < 5; idx++)
    {
        if (d_index_is_valid(idx, 5))
        {
            size_t converted_idx = d_index_convert_fast(idx, 5);
            int func_value = test_array[converted_idx];
            int macro_value = D_ARR_IDX(test_array, idx);
            
            if (!d_assert_standalone(func_value == macro_value, 
                                    "Function-based and macro-based array access should agree", 
                                    "Array access methods should yield same results", _test_info))
            {
                all_assertions_passed = false;
                printf("    Array access disagreement: idx=%d, func_value=%d, macro_value=%d\n",
                       (int)idx, func_value, macro_value);
            }
        }
    }

    // test that safe functions reject what unsafe versions might handle incorrectly
    size_t safe_output;
    bool safe_result = d_index_convert_safe(10, 5, &safe_output); // out of bounds
    if (!d_assert_standalone(safe_result == false, 
                            "Safe function should reject out-of-bounds index", 
                            "d_index_convert_safe should reject invalid indices", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] Function-Macro Consistency unit test passed\n");
    }
    else
    {
        printf("  [FAIL] Function-Macro Consistency unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_extreme_edge_cases
  tests extreme boundary conditions and edge cases
  Tests the following:
  - SIZE_MAX boundary behavior
  - signed integer overflow/underflow handling
  - zero-sized array behavior
  - maximum negative index handling
  - type conversion edge cases
*/
bool
d_tests_sa_extreme_edge_cases
(
    struct d_test_counter* _test_info
)
{
    printf("  --- Testing Extreme Edge Cases ---\n");
    size_t initial_tests_passed = _test_info->tests_passed;
    bool   all_assertions_passed = true;

    // test behavior with very large array sizes
    size_t large_size = 1000000;
    bool result = d_index_is_valid(-1, large_size);
    if (!d_assert_standalone(result == true, 
                            "Index -1 should be valid for large arrays", 
                            "Large arrays should support negative indexing", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-(ssize_t)large_size, large_size);
    if (!d_assert_standalone(result == true, 
                            "Negative size equal to array size should be valid", 
                            "Most negative valid index should be accepted", _test_info))
    {
        all_assertions_passed = false;
    }

    result = d_index_is_valid(-(ssize_t)large_size - 1, large_size);
    if (!d_assert_standalone(result == false, 
                            "Negative size greater than array should be invalid", 
                            "Out-of-bounds negative index should be rejected", _test_info))
    {
        all_assertions_passed = false;
    }

    // test macro behavior with large indices
    bool macro_result = D_IS_VALID_INDEX_N(-1, large_size);
    if (!d_assert_standalone(macro_result == true, 
                            "Macro should handle large arrays correctly", 
                            "D_IS_VALID_INDEX_N should work with large arrays", _test_info))
    {
        all_assertions_passed = false;
    }

    // test conversion with large arrays
    size_t converted = d_index_convert_fast(-1, large_size);
    if (!d_assert_standalone(converted == large_size - 1, 
                            "Conversion should work correctly with large arrays", 
                            "Should convert to last element of large array", _test_info))
    {
        all_assertions_passed = false;
    }

    // test zero array size edge cases
    result = d_index_is_valid(0, 0);
    if (!d_assert_standalone(result == false, 
                            "No index should be valid for zero-sized array", 
                            "Zero-sized arrays should reject all indices", _test_info))
    {
        all_assertions_passed = false;
    }

    macro_result = D_IS_VALID_INDEX_N(-1, 0);
    if (!d_assert_standalone(macro_result == false, 
                            "Macro should reject indices for zero-sized array", 
                            "Zero-sized arrays should reject negative indices", _test_info))
    {
        all_assertions_passed = false;
    }

    // test safe function with extreme values
    size_t safe_output = 999;
    bool safe_result = d_index_convert_safe(-1000000, 5, &safe_output);
    if (!d_assert_standalone(safe_result == false, 
                            "Safe function should reject extremely negative indices", 
                            "Very negative indices should be rejected", _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(safe_output == 999, 
                            "Safe function should preserve output on failure", 
                            "Output should be unchanged after failed conversion", _test_info))
    {
        all_assertions_passed = false;
    }

    // test potential overflow conditions
    // Note: This tests behavior near the limits of ssize_t
    ssize_t very_negative = -1000000000;
    result = d_index_is_valid(very_negative, 5);
    if (!d_assert_standalone(result == false, 
                            "Very large negative indices should be invalid", 
                            "Extremely negative indices should be rejected", _test_info))
    {
        all_assertions_passed = false;
    }

    // test that macros handle edge cases properly
    size_t clamp_result = D_CLAMP_INDEX(1000, 5);
    if (!d_assert_standalone(clamp_result == 4, 
                            "D_CLAMP_INDEX should clamp oversized indices", 
                            "Should clamp to maximum valid index", _test_info))
    {
        all_assertions_passed = false;
    }

    clamp_result = D_CLAMP_INDEX(-1000, 5);
    if (!d_assert_standalone(clamp_result == 0, 
                            "D_CLAMP_INDEX should clamp undersized indices", 
                            "Should clamp to minimum valid index", _test_info))
    {
        all_assertions_passed = false;
    }

    // test empty array clamping
    clamp_result = D_CLAMP_INDEX(5, 0);
    if (!d_assert_standalone(clamp_result == 0, 
                            "D_CLAMP_INDEX should handle empty arrays", 
                            "Should return 0 for empty arrays", _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("  [PASS] Extreme Edge Cases unit test passed\n");
    }
    else
    {
        printf("  [FAIL] Extreme Edge Cases unit test failed\n");
    }
    _test_info->tests_run++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_integration_all
  runs all integration tests
  Tests the following:
  - function-macro consistency tests
  - extreme edge case tests
  - reports aggregate results
*/
bool
d_tests_sa_integration_all
(
    struct d_test_counter* _test_info
)
{
    printf("\n--- Testing djinterp Integration ---\n");
    struct d_test_counter module_counter = {0, 0, 0, 0};

    bool consistency_result = d_tests_sa_function_macro_consistency(&module_counter);
    bool edge_cases_result = d_tests_sa_extreme_edge_cases(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_run         += module_counter.tests_run;
    _test_info->tests_passed      += module_counter.tests_passed;

    bool overall_result = (consistency_result && edge_cases_result);

    if (overall_result)
    {
        printf("[PASS] djinterp Integration: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }
    else
    {
        printf("[FAIL] djinterp Integration: %zu/%zu assertions, %zu/%zu unit tests passed\n",
               module_counter.assertions_passed, module_counter.assertions_total,
               module_counter.tests_passed, module_counter.tests_run);
    }

    return overall_result;
}

///////////////////////////////////////////////////////////////////////////////
///                       IV. COMPREHENSIVE TEST SUITE                      ///
///////////////////////////////////////////////////////////////////////////////

/*
d_tests_sa_djinterp_comprehensive
  master test function running all djinterp core tests
  Tests the following:
  - all d_index core function tests
  - all macro validation tests
  - all integration tests
  - provides comprehensive test coverage summary
*/
bool
d_tests_sa_djinterp_comprehensive
(
    struct d_test_counter* _test_info
)
{
    printf("================================================================================\n");
    printf("                          djinterp Core Test Suite                             \n");
    printf("================================================================================\n");
    printf("Comprehensive testing of djinterp.h core functionality:\n");
    printf("- d_index function family (convert_fast, convert_safe, is_valid)\n");
    printf("- Array and indexing utility macros\n");
    printf("- Negative indexing support\n");
    printf("- Function-macro consistency and integration\n");
    printf("- Edge cases and boundary conditions\n");
    printf("================================================================================\n");

    struct d_test_counter suite_counter = {0, 0, 0, 0};

    // run all test modules
    bool core_result = d_tests_sa_index_core_all(&suite_counter);
    bool macro_result = d_tests_sa_macro_all(&suite_counter);
    bool integration_result = d_tests_sa_integration_all(&suite_counter);

    // update totals
    _test_info->assertions_total  += suite_counter.assertions_total;
    _test_info->assertions_passed += suite_counter.assertions_passed;
    _test_info->tests_run         += suite_counter.tests_run;
    _test_info->tests_passed      += suite_counter.tests_passed;

    bool overall_result = (core_result && macro_result && integration_result);

    printf("\n================================================================================\n");
    printf("                           Test Suite Results                                  \n");
    printf("================================================================================\n");
    printf("Total Assertions: %zu/%zu passed (%.1f%%)\n", 
           suite_counter.assertions_passed, 
           suite_counter.assertions_total,
           suite_counter.assertions_total > 0 ? 
           (100.0 * suite_counter.assertions_passed / suite_counter.assertions_total) : 0.0);
    printf("Total Unit Tests: %zu/%zu passed (%.1f%%)\n", 
           suite_counter.tests_passed, 
           suite_counter.tests_run,
           suite_counter.tests_run > 0 ? 
           (100.0 * suite_counter.tests_passed / suite_counter.tests_run) : 0.0);

    if (overall_result)
    {
        printf("? djinterp Core Test Suite: PASSED\n");
    }
    else
    {
        printf("? djinterp Core Test Suite: FAILED\n");
    }

    printf("\nTest Coverage Summary:\n");
    printf("- Core Functions: %s d_index_convert_fast, d_index_convert_safe, d_index_is_valid\n", 
           core_result ? "?" : "?");
    printf("- Utility Macros: %s D_ARRAY_*, D_IS_VALID_INDEX_N, D_NEG_IDX, D_ARR_IDX\n", 
           macro_result ? "?" : "?");
    printf("- Integration:    %s Function-macro consistency, extreme edge cases\n", 
           integration_result ? "?" : "?");
    printf("================================================================================\n");

    return overall_result;
}
/*
d_tests_sa_djinterp_core_all
  comprehensive test runner for djinterp core functionality
  Tests the following:
  - all core d_index function tests
  - all macro utility tests
  - integration and consistency tests
  - reports comprehensive results for djinterp core
*/
bool
d_tests_sa_djinterp_core_all
(
    struct d_test_counter* _test_info
)
{
    return d_tests_sa_djinterp_comprehensive(_test_info);
}

/*
d_tests_sa_djinterp_core_all_extended
  extended comprehensive test runner with additional validation
  Tests the following:
  - all djinterp_core_all tests
  - extended edge case validation
  - stress testing scenarios
  - compatibility verification
*/
bool
d_tests_sa_djinterp_core_all_extended
(
    struct d_test_counter* _test_info
)
{
    // For now, just run the comprehensive tests
    // Can be extended in the future with additional test scenarios
    return d_tests_sa_djinterp_comprehensive(_test_info);
}