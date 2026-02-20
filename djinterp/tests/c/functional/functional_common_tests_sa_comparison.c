#include ".\functional_common_tests_sa.h"


/*
d_tests_sa_functional_compare_int
  Tests d_functional_compare_int for correctness.
  Tests the following:
  - equal values return 0
  - first less than second returns negative
  - first greater than second returns positive
  - zero compared with zero returns 0
  - negative values compare correctly
  - large values (INT_MAX, INT_MIN) compare correctly
  - NULL first pointer returns 0
  - NULL second pointer returns 0
  - both NULL returns 0
  - context parameter is ignored
*/
bool
d_tests_sa_functional_compare_int
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  a;
    int  b;
    int  result;
    int  dummy_context;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: equal values return 0 ---
    a      = 42;
    b      = 42;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_int: equal values return 0",
        "expected 0 when both values are equal",
        _counter);

    // --- test: first less than second returns negative ---
    a      = 10;
    b      = 20;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_int: a < b returns negative",
        "expected negative when first is less than second",
        _counter);

    // --- test: first greater than second returns positive ---
    a      = 20;
    b      = 10;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result > 0,
        "compare_int: a > b returns positive",
        "expected positive when first is greater than second",
        _counter);

    // --- test: zero vs zero returns 0 ---
    a      = 0;
    b      = 0;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_int: zero vs zero returns 0",
        "expected 0 for two zeros",
        _counter);

    // --- test: negative values compare correctly ---
    a      = -10;
    b      = -5;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_int: -10 < -5 returns negative",
        "expected negative for -10 vs -5",
        _counter);

    // --- test: negative vs positive ---
    a      = -1;
    b      = 1;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_int: negative vs positive returns negative",
        "expected negative for -1 vs 1",
        _counter);

    // --- test: large positive values ---
    a      = 2147483647;  /* INT_MAX */
    b      = -2147483647; /* near INT_MIN */
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result > 0,
        "compare_int: INT_MAX > near-INT_MIN returns positive",
        "expected positive for extreme range comparison",
        _counter);

    // --- test: NULL first pointer returns 0 ---
    b      = 42;
    result = d_functional_compare_int(NULL, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_int: NULL first returns 0",
        "expected 0 when first pointer is NULL",
        _counter);

    // --- test: NULL second pointer returns 0 ---
    a      = 42;
    result = d_functional_compare_int(&a, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_int: NULL second returns 0",
        "expected 0 when second pointer is NULL",
        _counter);

    // --- test: both NULL returns 0 ---
    result = d_functional_compare_int(NULL, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_int: both NULL returns 0",
        "expected 0 when both pointers are NULL",
        _counter);

    // --- test: context is ignored ---
    a             = 10;
    b             = 20;
    dummy_context = 999;
    result        = d_functional_compare_int(&a, &b, &dummy_context);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_int: context is ignored",
        "expected negative; context should not affect result",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_compare_size_t
  Tests d_functional_compare_size_t for correctness.
  Tests the following:
  - equal values return 0
  - first less than second returns negative
  - first greater than second returns positive
  - zero vs zero returns 0
  - large values compare correctly
  - NULL first pointer returns 0
  - NULL second pointer returns 0
  - both NULL returns 0
*/
bool
d_tests_sa_functional_compare_size_t
(
    struct d_test_counter* _counter
)
{
    bool   all_passed;
    size_t a;
    size_t b;
    int    result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: equal values return 0 ---
    a      = 100;
    b      = 100;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_size_t: equal values return 0",
        "expected 0 when both values are equal",
        _counter);

    // --- test: first less than second returns negative ---
    a      = 10;
    b      = 20;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_size_t: a < b returns negative",
        "expected negative when first is less",
        _counter);

    // --- test: first greater than second returns positive ---
    a      = 20;
    b      = 10;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result > 0,
        "compare_size_t: a > b returns positive",
        "expected positive when first is greater",
        _counter);

    // --- test: zero vs zero ---
    a      = 0;
    b      = 0;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_size_t: zero vs zero returns 0",
        "expected 0 for two zeros",
        _counter);

    // --- test: zero vs nonzero ---
    a      = 0;
    b      = 1;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_size_t: 0 < 1 returns negative",
        "expected negative for 0 vs 1",
        _counter);

    // --- test: large value ---
    a      = (size_t)-1;  /* SIZE_MAX */
    b      = 0;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result > 0,
        "compare_size_t: SIZE_MAX > 0 returns positive",
        "expected positive for SIZE_MAX vs 0",
        _counter);

    // --- test: NULL first pointer returns 0 ---
    b      = 42;
    result = d_functional_compare_size_t(NULL, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_size_t: NULL first returns 0",
        "expected 0 when first pointer is NULL",
        _counter);

    // --- test: NULL second pointer returns 0 ---
    a      = 42;
    result = d_functional_compare_size_t(&a, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_size_t: NULL second returns 0",
        "expected 0 when second pointer is NULL",
        _counter);

    // --- test: both NULL returns 0 ---
    result = d_functional_compare_size_t(NULL, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_size_t: both NULL returns 0",
        "expected 0 when both pointers are NULL",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_compare_double
  Tests d_functional_compare_double for correctness.
  Tests the following:
  - equal values return 0
  - first less than second returns negative
  - first greater than second returns positive
  - zero vs zero returns 0
  - negative values compare correctly
  - fractional values compare correctly
  - very small difference detected
  - NULL first pointer returns 0
  - NULL second pointer returns 0
  - both NULL returns 0
*/
bool
d_tests_sa_functional_compare_double
(
    struct d_test_counter* _counter
)
{
    bool   all_passed;
    double a;
    double b;
    int    result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: equal values return 0 ---
    a      = 3.14;
    b      = 3.14;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_double: equal values return 0",
        "expected 0 when both values are equal",
        _counter);

    // --- test: first less than second returns negative ---
    a      = 1.5;
    b      = 2.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_double: a < b returns negative",
        "expected negative when first is less",
        _counter);

    // --- test: first greater than second returns positive ---
    a      = 2.5;
    b      = 1.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result > 0,
        "compare_double: a > b returns positive",
        "expected positive when first is greater",
        _counter);

    // --- test: zero vs zero ---
    a      = 0.0;
    b      = 0.0;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_double: zero vs zero returns 0",
        "expected 0 for two zeros",
        _counter);

    // --- test: negative values compare correctly ---
    a      = -10.5;
    b      = -5.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_double: -10.5 < -5.5 returns negative",
        "expected negative for -10.5 vs -5.5",
        _counter);

    // --- test: fractional difference ---
    a      = 1.001;
    b      = 1.002;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_double: 1.001 < 1.002 returns negative",
        "expected negative for small fractional difference",
        _counter);

    // --- test: negative vs positive ---
    a      = -0.5;
    b      = 0.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result < 0,
        "compare_double: negative vs positive returns negative",
        "expected negative for -0.5 vs 0.5",
        _counter);

    // --- test: NULL first pointer returns 0 ---
    b      = 42.0;
    result = d_functional_compare_double(NULL, &b, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_double: NULL first returns 0",
        "expected 0 when first pointer is NULL",
        _counter);

    // --- test: NULL second pointer returns 0 ---
    a      = 42.0;
    result = d_functional_compare_double(&a, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_double: NULL second returns 0",
        "expected 0 when second pointer is NULL",
        _counter);

    // --- test: both NULL returns 0 ---
    result = d_functional_compare_double(NULL, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == 0,
        "compare_double: both NULL returns 0",
        "expected 0 when both pointers are NULL",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_equal_int
  Tests d_functional_equal_int for correctness.
  Tests the following:
  - equal values return true
  - unequal values return false
  - zero equals zero
  - negative values equal
  - NULL first pointer returns false
  - NULL second pointer returns false
  - both NULL returns false
  - context is ignored
*/
bool
d_tests_sa_functional_equal_int
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  a;
    int  b;
    bool result;
    int  dummy_context;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: equal values return true ---
    a      = 42;
    b      = 42;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "equal_int: equal values return true",
        "expected true when both values are 42",
        _counter);

    // --- test: unequal values return false ---
    a      = 10;
    b      = 20;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_int: unequal values return false",
        "expected false for 10 vs 20",
        _counter);

    // --- test: zero equals zero ---
    a      = 0;
    b      = 0;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "equal_int: zero equals zero",
        "expected true for 0 vs 0",
        _counter);

    // --- test: negative values equal ---
    a      = -7;
    b      = -7;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "equal_int: negative values equal",
        "expected true for -7 vs -7",
        _counter);

    // --- test: negative vs positive ---
    a      = -1;
    b      = 1;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_int: -1 != 1",
        "expected false for -1 vs 1",
        _counter);

    // --- test: NULL first pointer returns false ---
    b      = 42;
    result = d_functional_equal_int(NULL, &b, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_int: NULL first returns false",
        "expected false when first pointer is NULL",
        _counter);

    // --- test: NULL second pointer returns false ---
    a      = 42;
    result = d_functional_equal_int(&a, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_int: NULL second returns false",
        "expected false when second pointer is NULL",
        _counter);

    // --- test: both NULL returns false ---
    result = d_functional_equal_int(NULL, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_int: both NULL returns false",
        "expected false when both pointers are NULL",
        _counter);

    // --- test: context is ignored ---
    a             = 42;
    b             = 42;
    dummy_context = 999;
    result        = d_functional_equal_int(&a, &b, &dummy_context);

    all_passed &= d_assert_standalone(
        result == true,
        "equal_int: context is ignored",
        "expected true; context should not affect result",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_equal_size_t
  Tests d_functional_equal_size_t for correctness.
  Tests the following:
  - equal values return true
  - unequal values return false
  - zero equals zero
  - large values compared correctly
  - NULL first pointer returns false
  - NULL second pointer returns false
  - both NULL returns false
*/
bool
d_tests_sa_functional_equal_size_t
(
    struct d_test_counter* _counter
)
{
    bool   all_passed;
    size_t a;
    size_t b;
    bool   result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: equal values return true ---
    a      = 100;
    b      = 100;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "equal_size_t: equal values return true",
        "expected true when both values are 100",
        _counter);

    // --- test: unequal values return false ---
    a      = 10;
    b      = 20;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_size_t: unequal values return false",
        "expected false for 10 vs 20",
        _counter);

    // --- test: zero equals zero ---
    a      = 0;
    b      = 0;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "equal_size_t: zero equals zero",
        "expected true for 0 vs 0",
        _counter);

    // --- test: large values ---
    a      = (size_t)-1;  /* SIZE_MAX */
    b      = (size_t)-1;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "equal_size_t: SIZE_MAX equals SIZE_MAX",
        "expected true for SIZE_MAX vs SIZE_MAX",
        _counter);

    // --- test: NULL first pointer returns false ---
    b      = 42;
    result = d_functional_equal_size_t(NULL, &b, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_size_t: NULL first returns false",
        "expected false when first pointer is NULL",
        _counter);

    // --- test: NULL second pointer returns false ---
    a      = 42;
    result = d_functional_equal_size_t(&a, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_size_t: NULL second returns false",
        "expected false when second pointer is NULL",
        _counter);

    // --- test: both NULL returns false ---
    result = d_functional_equal_size_t(NULL, NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "equal_size_t: both NULL returns false",
        "expected false when both pointers are NULL",
        _counter);

    return all_passed;
}
