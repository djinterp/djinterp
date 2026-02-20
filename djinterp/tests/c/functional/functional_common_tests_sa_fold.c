#include ".\functional_common_tests_sa.h"


// --- local helper: accumulator that sums ints ---
static bool
test_helper_sum_int
(
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
static bool
test_helper_product_int
(
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
static bool
test_helper_digit_concat
(
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
static bool
test_helper_failing_accumulator
(
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
d_tests_sa_functional_fold_left
  Tests d_functional_fold_left for correctness.
  Tests the following:
  - summing {1,2,3,4,5} starting from 0 produces 15
  - product of {1,2,3,4} starting from 1 produces 24
  - single element fold works correctly
  - left fold order: digit_concat on {1,2,3} from 0 => 123
  - NULL input returns false
  - NULL accumulator returns false
  - NULL combine returns false
  - zero count returns false
  - zero element_size returns false
  - failing accumulator causes false return
*/
bool
d_tests_sa_functional_fold_left
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  input[5];
    int  accumulator;
    bool result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

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

    all_passed &= d_assert_standalone(
        result == true,
        "fold_left: sum returns true",
        "expected true for valid fold operation",
        _counter);

    all_passed &= d_assert_standalone(
        accumulator == 15,
        "fold_left: sum of {1,2,3,4,5} is 15",
        "expected 15 as sum",
        _counter);

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

    all_passed &= d_assert_standalone(
        (result == true) && (accumulator == 24),
        "fold_left: product of {1,2,3,4} is 24",
        "expected 24 as product",
        _counter);

    // --- test: single element ---
    input[0]    = 42;
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         1,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= d_assert_standalone(
        (result == true) && (accumulator == 42),
        "fold_left: single element fold",
        "expected 42 for single element sum from 0",
        _counter);

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

    all_passed &= d_assert_standalone(
        (result == true) && (accumulator == 123),
        "fold_left: digit concat {1,2,3} => 123",
        "expected 123 for left-to-right digit concatenation",
        _counter);

    // --- test: NULL input returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(NULL,
                                         5,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_left: NULL input returns false",
        "expected false when input is NULL",
        _counter);

    // --- test: NULL accumulator returns false ---
    result = d_functional_fold_left(input,
                                    5,
                                    sizeof(int),
                                    NULL,
                                    test_helper_sum_int,
                                    NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_left: NULL accumulator returns false",
        "expected false when accumulator is NULL",
        _counter);

    // --- test: NULL combine returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         5,
                                         sizeof(int),
                                         &accumulator,
                                         NULL,
                                         NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_left: NULL combine returns false",
        "expected false when combine function is NULL",
        _counter);

    // --- test: zero count returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         0,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_left: zero count returns false",
        "expected false when count is 0",
        _counter);

    // --- test: zero element_size returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         5,
                                         0,
                                         &accumulator,
                                         test_helper_sum_int,
                                         NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_left: zero element_size returns false",
        "expected false when element_size is 0",
        _counter);

    // --- test: failing accumulator returns false ---
    accumulator = 0;
    result      = d_functional_fold_left(input,
                                         3,
                                         sizeof(int),
                                         &accumulator,
                                         test_helper_failing_accumulator,
                                         NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_left: failing accumulator returns false",
        "expected false when accumulator function fails",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_fold_right
  Tests d_functional_fold_right for correctness.
  Tests the following:
  - summing {1,2,3,4,5} starting from 0 produces 15 (same as left)
  - right fold order: digit_concat on {1,2,3} from 0 => 321
  - single element fold works correctly
  - NULL input returns false
  - NULL accumulator returns false
  - NULL combine returns false
  - zero count returns false
  - zero element_size returns false
*/
bool
d_tests_sa_functional_fold_right
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  input[5];
    int  accumulator;
    bool result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

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

    all_passed &= d_assert_standalone(
        (result == true) && (accumulator == 15),
        "fold_right: sum of {1,2,3,4,5} is 15",
        "expected 15 (commutative operation same as fold_left)",
        _counter);

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

    all_passed &= d_assert_standalone(
        (result == true) && (accumulator == 321),
        "fold_right: digit concat {1,2,3} => 321",
        "expected 321 for right-to-left digit concatenation",
        _counter);

    // --- test: single element ---
    input[0]    = 42;
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          1,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= d_assert_standalone(
        (result == true) && (accumulator == 42),
        "fold_right: single element fold",
        "expected 42 for single element sum from 0",
        _counter);

    // --- test: NULL input returns false ---
    accumulator = 0;
    result      = d_functional_fold_right(NULL,
                                          5,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_right: NULL input returns false",
        "expected false when input is NULL",
        _counter);

    // --- test: NULL accumulator returns false ---
    result = d_functional_fold_right(input,
                                     5,
                                     sizeof(int),
                                     NULL,
                                     test_helper_sum_int,
                                     NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_right: NULL accumulator returns false",
        "expected false when accumulator is NULL",
        _counter);

    // --- test: NULL combine returns false ---
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          5,
                                          sizeof(int),
                                          &accumulator,
                                          NULL,
                                          NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_right: NULL combine returns false",
        "expected false when combine function is NULL",
        _counter);

    // --- test: zero count returns false ---
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          0,
                                          sizeof(int),
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_right: zero count returns false",
        "expected false when count is 0",
        _counter);

    // --- test: zero element_size returns false ---
    accumulator = 0;
    result      = d_functional_fold_right(input,
                                          5,
                                          0,
                                          &accumulator,
                                          test_helper_sum_int,
                                          NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "fold_right: zero element_size returns false",
        "expected false when element_size is 0",
        _counter);

    return all_passed;
}
