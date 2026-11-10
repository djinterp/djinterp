/*******************************************************************************
* djinterp [functional]                           functional_common_tests_fold.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_fold.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// --- local helper: accumulator that sums ints ---
static
bool test_helper_sum_int(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if ( (!_accumulated) ||
         (!_element) )
    {
        return false;
    }

    *(int*)_accumulated += *(const int*)_element;

    return true;
}


// --- local helper: accumulator that multiplies ints ---
static
bool test_helper_product_int(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if ( (!_accumulated) ||
         (!_element) )
    {
        return false;
    }

    *(int*)_accumulated *= *(const int*)_element;

    return true;
}


// --- local helper: accumulator that appends int to running total ---
// builds a "digit string" by: acc = acc * 10 + element
static
bool test_helper_digit_concat(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if ( (!_accumulated) ||
         (!_element) )
    {
        return false;
    }

    *(int*)_accumulated = (*(int*)_accumulated * 10)
                          + *(const int*)_element;

    return true;
}


// --- local helper: accumulator that always fails ---
static
bool test_helper_failing_accumulator(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_accumulated;
    (void)_element;
    (void)_context;

    return false;
}


/*
d_tests_functional_fold_left
  Tests d_functional_fold_left for correctness.
  Tests the following:
  - summing {1,2,3,4,5} starting from 0 produces 15
  - product of {1,2,3,4} starting from 1 produces 24
  - single element fold works correctly
  - left fold order: digit_concat on {1,2,3} from 0 => 123
  - NULL input returns false
  - NULL accumulator returns false
  - NULL combine returns false
  - zero count succeeds, the accumulator unchanged
  - zero element_size returns false
  - failing accumulator causes false return
*/
bool d_tests_functional_fold_left(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  input[5];
    int  accumulator;
    bool result;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_fold_left");

    all_passed = true;

    // --- test: sum {1,2,3,4,5} from 0 ---
    input[0]    = 1;
    input[1]    = 2;
    input[2]    = 3;
    input[3]    = 4;
    input[4]    = 5;
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         5,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: sum returns true",
                                   result == true,
                                   "expected true for valid fold operation");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: sum of {1,2,3,4,5} is 15",
                                   accumulator == 15,
                                   "expected 15 as sum");

    // --- test: product {1,2,3,4} from 1 ---
    input[0]    = 1;
    input[1]    = 2;
    input[2]    = 3;
    input[3]    = 4;
    accumulator = 1;
    result      = d_functional_fold_left(input,
                                         4,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_product_int,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: product of {1,2,3,4} is 24",
                                   (result == true) && (accumulator == 24),
                                   "expected 24 as product");

    // --- test: single element ---
    input[0]    = 42;
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         1,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: single element fold",
                                   (result == true) && (accumulator == 42),
                                   "expected 42 for single element sum from 0");

    // --- test: left fold order via digit concatenation ---
    // fold_left on {1,2,3} with digit_concat from 0:
    //   step 1: 0*10 + 1 = 1
    //   step 2: 1*10 + 2 = 12
    //   step 3: 12*10 + 3 = 123
    input[0]    = 1;
    input[1]    = 2;
    input[2]    = 3;
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         3,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_digit_concat,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: digit concat {1,2,3} => 123",
                                   (result == true) && (accumulator == 123),
                                   "expected 123 for left-to-right digit concatenation");

    // --- test: NULL input returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(NULL,
                                         5,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: NULL input returns false",
                                   result == false,
                                   "expected false when input is NULL");

    // --- test: NULL accumulator returns false ---
    result = d_functional_fold_left(input,
                                    5,
                                    sizeof(int),
                                    NULL,
                                    test_helper_sum_int,
                                    NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: NULL accumulator returns false",
                                   result == false,
                                   "expected false when accumulator is NULL");

    // --- test: NULL combine returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         5,
                                         sizeof(int),
                                         &accumulator,
                                         NULL,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: NULL combine returns false",
                                   result == false,
                                   "expected false when combine function is NULL");

    // --- test: zero count leaves the accumulator ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         0,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: zero count succeeds",
                                   (result == true) && (accumulator == 0),
                                   "expected true, the accumulator unchanged");

    // --- test: zero element_size returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         5,
                                         0,
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: zero element_size returns false",
                                   result == false,
                                   "expected false when element_size is 0");

    // --- test: failing accumulator returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         3,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_failing_accumulator,
                                         NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_left: failing accumulator returns false",
                                   result == false,
                                   "expected false when accumulator function fails");

    return all_passed;
}


/*
d_tests_functional_fold_right
  Tests d_functional_fold_right for correctness.
  Tests the following:
  - summing {1,2,3,4,5} starting from 0 produces 15 (same as left)
  - right fold order: digit_concat on {1,2,3} from 0 => 321
  - single element fold works correctly
  - NULL input returns false
  - NULL accumulator returns false
  - NULL combine returns false
  - zero count succeeds, the accumulator unchanged
  - zero element_size returns false
*/
bool d_tests_functional_fold_right(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  input[5];
    int  accumulator;
    bool result;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_fold_right");

    all_passed = true;

    // --- test: sum {1,2,3,4,5} from 0 (commutative: same as left) ---
    input[0]    = 1;
    input[1]    = 2;
    input[2]    = 3;
    input[3]    = 4;
    input[4]    = 5;
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          5,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: sum of {1,2,3,4,5} is 15",
                                   (result == true) && (accumulator == 15),
                                   "expected 15 (commutative operation same as fold_left)");

    // --- test: right fold order via digit concatenation ---
    // fold_right on {1,2,3} with digit_concat from 0:
    //   step 1 (element 3): 0*10 + 3 = 3
    //   step 2 (element 2): 3*10 + 2 = 32
    //   step 3 (element 1): 32*10 + 1 = 321
    input[0]    = 1;
    input[1]    = 2;
    input[2]    = 3;
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          3,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_digit_concat,
                                          NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: digit concat {1,2,3} => 321",
                                   (result == true) && (accumulator == 321),
                                   "expected 321 for right-to-left digit concatenation");

    // --- test: single element ---
    input[0]    = 42;
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          1,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: single element fold",
                                   (result == true) && (accumulator == 42),
                                   "expected 42 for single element sum from 0");

    // --- test: NULL input returns false ---
    accumulator = 0;
    result      = d_functional_fold_right(NULL,
                                          5,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: NULL input returns false",
                                   result == false,
                                   "expected false when input is NULL");

    // --- test: NULL accumulator returns false ---
    result = d_functional_fold_right(input,
                                     5,
                                     sizeof(int),
                                     NULL,
                                     test_helper_sum_int,
                                     NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: NULL accumulator returns false",
                                   result == false,
                                   "expected false when accumulator is NULL");

    // --- test: NULL combine returns false ---
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          5,
                                          sizeof(int),
                                          &accumulator,
                                          NULL,
                                          NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: NULL combine returns false",
                                   result == false,
                                   "expected false when combine function is NULL");

    // --- test: zero count leaves the accumulator ---
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          0,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: zero count succeeds",
                                   (result == true) && (accumulator == 0),
                                   "expected true, the accumulator unchanged");

    // --- test: zero element_size returns false ---
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          5,
                                          0,
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold_right: zero element_size returns false",
                                   result == false,
                                   "expected false when element_size is 0");

    return all_passed;
}
