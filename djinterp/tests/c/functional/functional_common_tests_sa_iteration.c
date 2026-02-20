#include ".\functional_common_tests_sa.h"


// --- local helper: consumer that increments each int element by 1 ---
static void
test_helper_increment_int
(
    void* _element,
    void* _context
)
{
    (void)_context;

    if (_element)
    {
        (*(int*)_element)++;
    }

    return;
}


// --- local helper: const consumer that sums into context ---
static void
test_helper_sum_to_context
(
    const void* _element,
    void*       _context
)
{
    if ( (_element) &&
         (_context) )
    {
        *(int*)_context += *(const int*)_element;
    }

    return;
}


// --- local helper: predicate that tests if an int is positive ---
static bool
test_helper_is_positive
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if (!_element)
    {
        return false;
    }

    return (*(const int*)_element > 0);
}


// --- local helper: predicate that tests if an int is even ---
static bool
test_helper_is_even
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if (!_element)
    {
        return false;
    }

    return ((*(const int*)_element % 2) == 0);
}


// --- local helper: predicate that tests if int > 100 ---
static bool
test_helper_gt_100
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if (!_element)
    {
        return false;
    }

    return (*(const int*)_element > 100);
}


/*
d_tests_sa_functional_for_each
  Tests d_functional_for_each for correctness.
  Tests the following:
  - applies consumer to each element (increment all ints)
  - single element array
  - NULL input silently returns (no crash)
  - NULL apply silently returns
  - zero count silently returns
  - zero element_size silently returns
*/
bool
d_tests_sa_functional_for_each
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  data[4];

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: increment all elements by 1 ---
    data[0] = 10;
    data[1] = 20;
    data[2] = 30;
    data[3] = 40;
    d_functional_for_each(data,
                          4,
                          sizeof(int),
                          test_helper_increment_int,
                          NULL);

    all_passed &= d_assert_standalone(
        (data[0] == 11) &&
        (data[1] == 21) &&
        (data[2] == 31) &&
        (data[3] == 41),
        "for_each: increment all elements",
        "expected {11, 21, 31, 41} after incrementing",
        _counter);

    // --- test: single element ---
    data[0] = 99;
    d_functional_for_each(data,
                          1,
                          sizeof(int),
                          test_helper_increment_int,
                          NULL);

    all_passed &= d_assert_standalone(
        data[0] == 100,
        "for_each: single element increment",
        "expected 100 after incrementing 99",
        _counter);

    // --- test: NULL input does not crash ---
    d_functional_for_each(NULL,
                          4,
                          sizeof(int),
                          test_helper_increment_int,
                          NULL);

    all_passed &= d_assert_standalone(
        true,
        "for_each: NULL input does not crash",
        "expected no crash with NULL input",
        _counter);

    // --- test: NULL apply does not crash ---
    d_functional_for_each(data,
                          4,
                          sizeof(int),
                          NULL,
                          NULL);

    all_passed &= d_assert_standalone(
        true,
        "for_each: NULL apply does not crash",
        "expected no crash with NULL apply function",
        _counter);

    // --- test: zero count does not crash ---
    data[0] = 50;
    d_functional_for_each(data,
                          0,
                          sizeof(int),
                          test_helper_increment_int,
                          NULL);

    all_passed &= d_assert_standalone(
        data[0] == 50,
        "for_each: zero count leaves data unchanged",
        "expected data unchanged when count is 0",
        _counter);

    // --- test: zero element_size does not crash ---
    data[0] = 50;
    d_functional_for_each(data,
                          4,
                          0,
                          test_helper_increment_int,
                          NULL);

    all_passed &= d_assert_standalone(
        data[0] == 50,
        "for_each: zero element_size leaves data unchanged",
        "expected data unchanged when element_size is 0",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_for_each_const
  Tests d_functional_for_each_const for correctness.
  Tests the following:
  - applies const consumer to sum elements via context
  - single element array
  - NULL input silently returns
  - NULL apply silently returns
  - zero count silently returns
  - zero element_size silently returns
*/
bool
d_tests_sa_functional_for_each_const
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  data[4];
    int  sum;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: sum elements via const consumer ---
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    sum     = 0;
    d_functional_for_each_const(data,
                                4,
                                sizeof(int),
                                test_helper_sum_to_context,
                                &sum);

    all_passed &= d_assert_standalone(
        sum == 10,
        "for_each_const: sum via context is 10",
        "expected 10 as sum of {1,2,3,4}",
        _counter);

    // --- test: single element ---
    data[0] = 42;
    sum     = 0;
    d_functional_for_each_const(data,
                                1,
                                sizeof(int),
                                test_helper_sum_to_context,
                                &sum);

    all_passed &= d_assert_standalone(
        sum == 42,
        "for_each_const: single element sum",
        "expected 42 for single element",
        _counter);

    // --- test: NULL input does not crash ---
    sum = 0;
    d_functional_for_each_const(NULL,
                                4,
                                sizeof(int),
                                test_helper_sum_to_context,
                                &sum);

    all_passed &= d_assert_standalone(
        sum == 0,
        "for_each_const: NULL input leaves sum unchanged",
        "expected 0 when input is NULL",
        _counter);

    // --- test: NULL apply does not crash ---
    d_functional_for_each_const(data,
                                4,
                                sizeof(int),
                                NULL,
                                &sum);

    all_passed &= d_assert_standalone(
        true,
        "for_each_const: NULL apply does not crash",
        "expected no crash with NULL apply function",
        _counter);

    // --- test: zero count does not modify context ---
    sum = 0;
    d_functional_for_each_const(data,
                                0,
                                sizeof(int),
                                test_helper_sum_to_context,
                                &sum);

    all_passed &= d_assert_standalone(
        sum == 0,
        "for_each_const: zero count leaves context unchanged",
        "expected 0 when count is 0",
        _counter);

    // --- test: zero element_size does not modify context ---
    sum = 0;
    d_functional_for_each_const(data,
                                4,
                                0,
                                test_helper_sum_to_context,
                                &sum);

    all_passed &= d_assert_standalone(
        sum == 0,
        "for_each_const: zero element_size leaves context unchanged",
        "expected 0 when element_size is 0",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_any
  Tests d_functional_any for correctness.
  Tests the following:
  - returns true when at least one element matches
  - returns false when no elements match
  - returns true for single matching element
  - returns false for single non-matching element
  - returns true when all elements match
  - NULL input returns false
  - NULL predicate returns false
  - zero count returns false
  - zero element_size returns false
*/
bool
d_tests_sa_functional_any
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  data[5];
    bool result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: some elements match ---
    data[0] = -3;
    data[1] = -1;
    data[2] = 0;
    data[3] = 5;
    data[4] = -2;
    result   = d_functional_any(data,
                                5,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "any: some positive in mixed array returns true",
        "expected true; element 5 is positive",
        _counter);

    // --- test: no elements match ---
    data[0] = -3;
    data[1] = -1;
    data[2] = 0;
    data[3] = -5;
    data[4] = -2;
    result   = d_functional_any(data,
                                5,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "any: no positive in array returns false",
        "expected false; no positive elements",
        _counter);

    // --- test: all elements match ---
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    result   = d_functional_any(data,
                                3,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "any: all positive returns true",
        "expected true when all elements are positive",
        _counter);

    // --- test: single matching element ---
    data[0] = 1;
    result   = d_functional_any(data,
                                1,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "any: single positive element returns true",
        "expected true for single positive element",
        _counter);

    // --- test: single non-matching element ---
    data[0] = -1;
    result   = d_functional_any(data,
                                1,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "any: single negative element returns false",
        "expected false for single negative element",
        _counter);

    // --- test: NULL input returns false ---
    result = d_functional_any(NULL,
                              5,
                              sizeof(int),
                              test_helper_is_positive,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "any: NULL input returns false",
        "expected false when input is NULL",
        _counter);

    // --- test: NULL predicate returns false ---
    result = d_functional_any(data,
                              5,
                              sizeof(int),
                              NULL,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "any: NULL predicate returns false",
        "expected false when predicate is NULL",
        _counter);

    // --- test: zero count returns false ---
    result = d_functional_any(data,
                              0,
                              sizeof(int),
                              test_helper_is_positive,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "any: zero count returns false",
        "expected false when count is 0",
        _counter);

    // --- test: zero element_size returns false ---
    result = d_functional_any(data,
                              5,
                              0,
                              test_helper_is_positive,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "any: zero element_size returns false",
        "expected false when element_size is 0",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_all
  Tests d_functional_all for correctness.
  Tests the following:
  - returns true when all elements match
  - returns false when some elements don't match
  - returns false when no elements match
  - single matching element
  - single non-matching element
  - NULL input returns false
  - NULL predicate returns false
  - zero count returns false
  - zero element_size returns false
*/
bool
d_tests_sa_functional_all
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  data[5];
    bool result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: all elements match ---
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    result   = d_functional_all(data,
                                5,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "all: all positive returns true",
        "expected true when all elements are positive",
        _counter);

    // --- test: one element doesn't match ---
    data[0] = 1;
    data[1] = 2;
    data[2] = -3;
    data[3] = 4;
    data[4] = 5;
    result   = d_functional_all(data,
                                5,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "all: one non-positive returns false",
        "expected false; element -3 is not positive",
        _counter);

    // --- test: no elements match ---
    data[0] = -1;
    data[1] = -2;
    data[2] = -3;
    result   = d_functional_all(data,
                                3,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "all: no positive returns false",
        "expected false when no elements are positive",
        _counter);

    // --- test: single matching element ---
    data[0] = 1;
    result   = d_functional_all(data,
                                1,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "all: single positive returns true",
        "expected true for single positive element",
        _counter);

    // --- test: single non-matching element ---
    data[0] = -1;
    result   = d_functional_all(data,
                                1,
                                sizeof(int),
                                test_helper_is_positive,
                                NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "all: single negative returns false",
        "expected false for single negative element",
        _counter);

    // --- test: NULL input returns false ---
    result = d_functional_all(NULL,
                              5,
                              sizeof(int),
                              test_helper_is_positive,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "all: NULL input returns false",
        "expected false when input is NULL",
        _counter);

    // --- test: NULL predicate returns false ---
    result = d_functional_all(data,
                              5,
                              sizeof(int),
                              NULL,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "all: NULL predicate returns false",
        "expected false when predicate is NULL",
        _counter);

    // --- test: zero count returns false ---
    result = d_functional_all(data,
                              0,
                              sizeof(int),
                              test_helper_is_positive,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "all: zero count returns false",
        "expected false when count is 0",
        _counter);

    // --- test: zero element_size returns false ---
    result = d_functional_all(data,
                              5,
                              0,
                              test_helper_is_positive,
                              NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "all: zero element_size returns false",
        "expected false when element_size is 0",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_none
  Tests d_functional_none for correctness.
  Tests the following:
  - returns true when no elements match
  - returns false when some elements match
  - returns false when all elements match
  - single matching element returns false
  - single non-matching element returns true
  - NULL input returns false
  - NULL predicate returns false
  - zero count returns false
  - zero element_size returns false
*/
bool
d_tests_sa_functional_none
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  data[5];
    bool result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: no elements match ---
    data[0] = -1;
    data[1] = -2;
    data[2] = -3;
    data[3] = 0;
    data[4] = -5;
    result   = d_functional_none(data,
                                 5,
                                 sizeof(int),
                                 test_helper_is_positive,
                                 NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "none: no positive returns true",
        "expected true when no elements are positive",
        _counter);

    // --- test: some elements match ---
    data[0] = -1;
    data[1] = 2;
    data[2] = -3;
    result   = d_functional_none(data,
                                 3,
                                 sizeof(int),
                                 test_helper_is_positive,
                                 NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "none: some positive returns false",
        "expected false; element 2 is positive",
        _counter);

    // --- test: all elements match ---
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    result   = d_functional_none(data,
                                 3,
                                 sizeof(int),
                                 test_helper_is_positive,
                                 NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "none: all positive returns false",
        "expected false when all elements match",
        _counter);

    // --- test: single matching element ---
    data[0] = 1;
    result   = d_functional_none(data,
                                 1,
                                 sizeof(int),
                                 test_helper_is_positive,
                                 NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "none: single positive returns false",
        "expected false for single positive element",
        _counter);

    // --- test: single non-matching element ---
    data[0] = -1;
    result   = d_functional_none(data,
                                 1,
                                 sizeof(int),
                                 test_helper_is_positive,
                                 NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "none: single negative returns true",
        "expected true for single negative element",
        _counter);

    // --- test: NULL input returns false ---
    result = d_functional_none(NULL,
                               5,
                               sizeof(int),
                               test_helper_is_positive,
                               NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "none: NULL input returns false",
        "expected false when input is NULL",
        _counter);

    // --- test: NULL predicate returns false ---
    result = d_functional_none(data,
                               5,
                               sizeof(int),
                               NULL,
                               NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "none: NULL predicate returns false",
        "expected false when predicate is NULL",
        _counter);

    // --- test: zero count returns false ---
    result = d_functional_none(data,
                               0,
                               sizeof(int),
                               test_helper_is_positive,
                               NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "none: zero count returns false",
        "expected false when count is 0",
        _counter);

    // --- test: zero element_size returns false ---
    result = d_functional_none(data,
                               5,
                               0,
                               test_helper_is_positive,
                               NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "none: zero element_size returns false",
        "expected false when element_size is 0",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_count_if
  Tests d_functional_count_if for correctness.
  Tests the following:
  - counts correct number of matching elements
  - returns full count when all match
  - returns 0 when none match
  - single element matches
  - single element doesn't match
  - NULL input returns 0
  - NULL predicate returns 0
  - zero count returns 0
  - zero element_size returns 0
*/
bool
d_tests_sa_functional_count_if
(
    struct d_test_counter* _counter
)
{
    bool   all_passed;
    int    data[6];
    size_t result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: count even elements in mixed array ---
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    data[5] = 6;
    result   = d_functional_count_if(data,
                                     6,
                                     sizeof(int),
                                     test_helper_is_even,
                                     NULL);

    all_passed &= d_assert_standalone(
        result == 3,
        "count_if: 3 even in {1,2,3,4,5,6}",
        "expected 3 even elements",
        _counter);

    // --- test: all elements match ---
    data[0] = 2;
    data[1] = 4;
    data[2] = 6;
    result   = d_functional_count_if(data,
                                     3,
                                     sizeof(int),
                                     test_helper_is_even,
                                     NULL);

    all_passed &= d_assert_standalone(
        result == 3,
        "count_if: all even returns full count",
        "expected 3 when all elements are even",
        _counter);

    // --- test: no elements match ---
    data[0] = 1;
    data[1] = 3;
    data[2] = 5;
    result   = d_functional_count_if(data,
                                     3,
                                     sizeof(int),
                                     test_helper_is_even,
                                     NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "count_if: no even returns 0",
        "expected 0 when no elements are even",
        _counter);

    // --- test: count elements > 100 (none match) ---
    data[0] = 10;
    data[1] = 20;
    data[2] = 30;
    result   = d_functional_count_if(data,
                                     3,
                                     sizeof(int),
                                     test_helper_gt_100,
                                     NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "count_if: none > 100 returns 0",
        "expected 0 when no elements exceed 100",
        _counter);

    // --- test: NULL input returns 0 ---
    result = d_functional_count_if(NULL,
                                   6,
                                   sizeof(int),
                                   test_helper_is_even,
                                   NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "count_if: NULL input returns 0",
        "expected 0 when input is NULL",
        _counter);

    // --- test: NULL predicate returns 0 ---
    result = d_functional_count_if(data,
                                   6,
                                   sizeof(int),
                                   NULL,
                                   NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "count_if: NULL predicate returns 0",
        "expected 0 when predicate is NULL",
        _counter);

    // --- test: zero count returns 0 ---
    result = d_functional_count_if(data,
                                   0,
                                   sizeof(int),
                                   test_helper_is_even,
                                   NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "count_if: zero count returns 0",
        "expected 0 when count is 0",
        _counter);

    // --- test: zero element_size returns 0 ---
    result = d_functional_count_if(data,
                                   6,
                                   0,
                                   test_helper_is_even,
                                   NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "count_if: zero element_size returns 0",
        "expected 0 when element_size is 0",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_find_if
  Tests d_functional_find_if for correctness.
  Tests the following:
  - finds first matching element in mixed array
  - returns pointer into original array
  - returns NULL when no elements match
  - finds first element when it matches
  - finds last element when only it matches
  - single matching element
  - single non-matching element
  - NULL input returns NULL
  - NULL predicate returns NULL
  - zero count returns NULL
  - zero element_size returns NULL
*/
bool
d_tests_sa_functional_find_if
(
    struct d_test_counter* _counter
)
{
    bool  all_passed;
    int   data[5];
    void* result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: find first even in {1,2,3,4,5} ---
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    result   = d_functional_find_if(data,
                                    5,
                                    sizeof(int),
                                    test_helper_is_even,
                                    NULL);

    all_passed &= d_assert_standalone(
        result != NULL,
        "find_if: finds first even element",
        "expected non-NULL result for first even",
        _counter);

    // guard value and pointer checks behind non-NULL result
    if (result != NULL)
    {
        all_passed &= d_assert_standalone(
            *(int*)result == 2,
            "find_if: first even element is 2",
            "expected value 2 at found position",
            _counter);

        // --- test: returned pointer is into original array ---
        all_passed &= d_assert_standalone(
            result == (void*)&data[1],
            "find_if: pointer into original array",
            "expected pointer to data[1]",
            _counter);
    }

    // --- test: no match returns NULL ---
    data[0] = 1;
    data[1] = 3;
    data[2] = 5;
    result   = d_functional_find_if(data,
                                    3,
                                    sizeof(int),
                                    test_helper_is_even,
                                    NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "find_if: no even returns NULL",
        "expected NULL when no elements match",
        _counter);

    // --- test: first element matches ---
    data[0] = 2;
    data[1] = 3;
    data[2] = 5;
    result   = d_functional_find_if(data,
                                    3,
                                    sizeof(int),
                                    test_helper_is_even,
                                    NULL);

    all_passed &= d_assert_standalone(
        (result != NULL) && (result == (void*)&data[0]),
        "find_if: first element matches returns &data[0]",
        "expected pointer to first element",
        _counter);

    // --- test: only last element matches ---
    data[0] = 1;
    data[1] = 3;
    data[2] = 5;
    data[3] = 7;
    data[4] = 8;
    result   = d_functional_find_if(data,
                                    5,
                                    sizeof(int),
                                    test_helper_is_even,
                                    NULL);

    all_passed &= d_assert_standalone(
        result != NULL,
        "find_if: only last element matches (non-NULL)",
        "expected non-NULL when last element is even",
        _counter);

    if (result != NULL)
    {
        all_passed &= d_assert_standalone(
            *(int*)result == 8,
            "find_if: only last element matches value",
            "expected value 8 at last matching position",
            _counter);
    }

    // --- test: NULL input returns NULL ---
    result = d_functional_find_if(NULL,
                                  5,
                                  sizeof(int),
                                  test_helper_is_even,
                                  NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "find_if: NULL input returns NULL",
        "expected NULL when input is NULL",
        _counter);

    // --- test: NULL predicate returns NULL ---
    result = d_functional_find_if(data,
                                  5,
                                  sizeof(int),
                                  NULL,
                                  NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "find_if: NULL predicate returns NULL",
        "expected NULL when predicate is NULL",
        _counter);

    // --- test: zero count returns NULL ---
    result = d_functional_find_if(data,
                                  0,
                                  sizeof(int),
                                  test_helper_is_even,
                                  NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "find_if: zero count returns NULL",
        "expected NULL when count is 0",
        _counter);

    // --- test: zero element_size returns NULL ---
    result = d_functional_find_if(data,
                                  5,
                                  0,
                                  test_helper_is_even,
                                  NULL);

    all_passed &= d_assert_standalone(
        result == NULL,
        "find_if: zero element_size returns NULL",
        "expected NULL when element_size is 0",
        _counter);

    return all_passed;
}