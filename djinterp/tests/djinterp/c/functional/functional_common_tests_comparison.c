/*******************************************************************************
* djinterp [functional]                     functional_common_tests_comparison.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_comparison.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/**
 * @brief Part 1 of d_tests_functional_compare_int.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: equal values return 0
 *   - test: first less than second returns negative
 *   - test: first greater than second returns positive
 *   - test: zero vs zero returns 0
 *   - test: negative values compare correctly
 *   - test: negative vs positive
 *   - test: large positive values
 *   - test: a NULL first pointer sorts first
 */
static bool
d_internal_tests_functional_compare_int_part1(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  a;
    int  b;
    int  result;

    all_passed = true;

    // --- test: equal values return 0 ---
    a      = 42;
    b      = 42;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: equal values return 0",
                                   result == 0,
                                   "expected 0 when both values are equal");

    // --- test: first less than second returns negative ---
    a      = 10;
    b      = 20;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: a < b returns negative",
                                   result < 0,
                                   "expected negative when first is less than second");

    // --- test: first greater than second returns positive ---
    a      = 20;
    b      = 10;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: a > b returns positive",
                                   result > 0,
                                   "expected positive when first is greater than second");

    // --- test: zero vs zero returns 0 ---
    a      = 0;
    b      = 0;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: zero vs zero returns 0",
                                   result == 0,
                                   "expected 0 for two zeros");

    // --- test: negative values compare correctly ---
    a      = -10;
    b      = -5;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: -10 < -5 returns negative",
                                   result < 0,
                                   "expected negative for -10 vs -5");

    // --- test: negative vs positive ---
    a      = -1;
    b      = 1;
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: negative vs positive returns negative",
                                   result < 0,
                                   "expected negative for -1 vs 1");

    // --- test: large positive values ---
    a      = 2147483647;  /* INT_MAX */
    b      = -2147483647; /* near INT_MIN */
    result = d_functional_compare_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: INT_MAX > near-INT_MIN returns positive",
                                   result > 0,
                                   "expected positive for extreme range comparison");

    // --- test: a NULL first pointer sorts first ---
    b      = 42;
    result = d_functional_compare_int(NULL, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: NULL first sorts first",
                                   result < 0,
                                   "expected negative: a missing operand sorts first");

    return all_passed;
}


/**
 * @brief Part 2 of d_tests_functional_compare_int.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: a NULL second pointer sorts last
 *   - test: both NULL returns 0
 *   - test: context is ignored
 */
static bool
d_internal_tests_functional_compare_int_part2(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  a;
    int  b;
    int  result;
    int  dummy_context;

    all_passed = true;

    // --- test: a NULL second pointer sorts last ---
    a      = 42;
    result = d_functional_compare_int(&a, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: NULL second sorts last",
                                   result > 0,
                                   "expected positive: a missing operand sorts first");

    // --- test: both NULL returns 0 ---
    result = d_functional_compare_int(NULL, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: both NULL returns 0",
                                   result == 0,
                                   "expected 0 when both pointers are NULL");

    // --- test: context is ignored ---
    a             = 10;
    b             = 20;
    dummy_context = 999;
    result        = d_functional_compare_int(&a, &b, &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_int: context is ignored",
                                   result < 0,
                                   "expected negative; context should not affect result");

    return all_passed;
}


/*
d_tests_functional_compare_int
  Tests d_functional_compare_int for correctness.
  Tests the following:
  - equal values return 0
  - first less than second returns negative
  - first greater than second returns positive
  - zero compared with zero returns 0
  - negative values compare correctly
  - large values (INT_MAX, INT_MIN) compare correctly
  - a NULL first pointer sorts first (negative)
  - a NULL second pointer sorts last (positive)
  - both NULL returns 0
  - context parameter is ignored
*/
bool d_tests_functional_compare_int(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_compare_int");

    all_passed = true;
    all_passed &= d_internal_tests_functional_compare_int_part1(_suite);
    all_passed &= d_internal_tests_functional_compare_int_part2(_suite);

    return all_passed;
}


/**
 * @brief Part 1 of d_tests_functional_compare_size_t.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: equal values return 0
 *   - test: first less than second returns negative
 *   - test: first greater than second returns positive
 *   - test: zero vs zero
 *   - test: zero vs nonzero
 *   - test: large value
 *   - test: a NULL first pointer sorts first
 *   - test: a NULL second pointer sorts last
 */
static bool
d_internal_tests_functional_compare_size_t_part1(
    struct dj_test_suite* _suite
)
{
    bool   all_passed;
    size_t a;
    size_t b;
    int    result;

    all_passed = true;

    // --- test: equal values return 0 ---
    a      = 100;
    b      = 100;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: equal values return 0",
                                   result == 0,
                                   "expected 0 when both values are equal");

    // --- test: first less than second returns negative ---
    a      = 10;
    b      = 20;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: a < b returns negative",
                                   result < 0,
                                   "expected negative when first is less");

    // --- test: first greater than second returns positive ---
    a      = 20;
    b      = 10;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: a > b returns positive",
                                   result > 0,
                                   "expected positive when first is greater");

    // --- test: zero vs zero ---
    a      = 0;
    b      = 0;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: zero vs zero returns 0",
                                   result == 0,
                                   "expected 0 for two zeros");

    // --- test: zero vs nonzero ---
    a      = 0;
    b      = 1;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: 0 < 1 returns negative",
                                   result < 0,
                                   "expected negative for 0 vs 1");

    // --- test: large value ---
    a      = (size_t)-1;  /* SIZE_MAX */
    b      = 0;
    result = d_functional_compare_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: SIZE_MAX > 0 returns positive",
                                   result > 0,
                                   "expected positive for SIZE_MAX vs 0");

    // --- test: a NULL first pointer sorts first ---
    b      = 42;
    result = d_functional_compare_size_t(NULL, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: NULL first sorts first",
                                   result < 0,
                                   "expected negative: a missing operand sorts first");

    // --- test: a NULL second pointer sorts last ---
    a      = 42;
    result = d_functional_compare_size_t(&a, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: NULL second sorts last",
                                   result > 0,
                                   "expected positive: a missing operand sorts first");

    return all_passed;
}


/**
 * @brief Part 2 of d_tests_functional_compare_size_t.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: both NULL returns 0
 */
static bool
d_internal_tests_functional_compare_size_t_part2(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  result;

    all_passed = true;

    // --- test: both NULL returns 0 ---
    result = d_functional_compare_size_t(NULL, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_size_t: both NULL returns 0",
                                   result == 0,
                                   "expected 0 when both pointers are NULL");

    return all_passed;
}


/*
d_tests_functional_compare_size_t
  Tests d_functional_compare_size_t for correctness.
  Tests the following:
  - equal values return 0
  - first less than second returns negative
  - first greater than second returns positive
  - zero vs zero returns 0
  - large values compare correctly
  - a NULL first pointer sorts first (negative)
  - a NULL second pointer sorts last (positive)
  - both NULL returns 0
*/
bool d_tests_functional_compare_size_t(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_compare_size_t");

    all_passed = true;
    all_passed &= d_internal_tests_functional_compare_size_t_part1(_suite);
    all_passed &= d_internal_tests_functional_compare_size_t_part2(_suite);

    return all_passed;
}


/**
 * @brief Part 1 of d_tests_functional_compare_double.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: equal values return 0
 *   - test: first less than second returns negative
 *   - test: first greater than second returns positive
 *   - test: zero vs zero
 *   - test: negative values compare correctly
 *   - test: fractional difference
 *   - test: negative vs positive
 *   - test: a NULL first pointer sorts first
 */
static bool
d_internal_tests_functional_compare_double_part1(
    struct dj_test_suite* _suite
)
{
    bool   all_passed;
    double a;
    double b;
    int    result;

    all_passed = true;

    // --- test: equal values return 0 ---
    a      = 3.14;
    b      = 3.14;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: equal values return 0",
                                   result == 0,
                                   "expected 0 when both values are equal");

    // --- test: first less than second returns negative ---
    a      = 1.5;
    b      = 2.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: a < b returns negative",
                                   result < 0,
                                   "expected negative when first is less");

    // --- test: first greater than second returns positive ---
    a      = 2.5;
    b      = 1.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: a > b returns positive",
                                   result > 0,
                                   "expected positive when first is greater");

    // --- test: zero vs zero ---
    a      = 0.0;
    b      = 0.0;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: zero vs zero returns 0",
                                   result == 0,
                                   "expected 0 for two zeros");

    // --- test: negative values compare correctly ---
    a      = -10.5;
    b      = -5.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: -10.5 < -5.5 returns negative",
                                   result < 0,
                                   "expected negative for -10.5 vs -5.5");

    // --- test: fractional difference ---
    a      = 1.001;
    b      = 1.002;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: 1.001 < 1.002 returns negative",
                                   result < 0,
                                   "expected negative for small fractional difference");

    // --- test: negative vs positive ---
    a      = -0.5;
    b      = 0.5;
    result = d_functional_compare_double(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: negative vs positive returns negative",
                                   result < 0,
                                   "expected negative for -0.5 vs 0.5");

    // --- test: a NULL first pointer sorts first ---
    b      = 42.0;
    result = d_functional_compare_double(NULL, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: NULL first sorts first",
                                   result < 0,
                                   "expected negative: a missing operand sorts first");

    return all_passed;
}


/**
 * @brief Part 2 of d_tests_functional_compare_double.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: a NULL second pointer sorts last
 *   - test: both NULL returns 0
 */
static bool
d_internal_tests_functional_compare_double_part2(
    struct dj_test_suite* _suite
)
{
    bool   all_passed;
    double a;
    int    result;

    all_passed = true;

    // --- test: a NULL second pointer sorts last ---
    a      = 42.0;
    result = d_functional_compare_double(&a, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: NULL second sorts last",
                                   result > 0,
                                   "expected positive: a missing operand sorts first");

    // --- test: both NULL returns 0 ---
    result = d_functional_compare_double(NULL, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "compare_double: both NULL returns 0",
                                   result == 0,
                                   "expected 0 when both pointers are NULL");

    return all_passed;
}


/*
d_tests_functional_compare_double
  Tests d_functional_compare_double for correctness.
  Tests the following:
  - equal values return 0
  - first less than second returns negative
  - first greater than second returns positive
  - zero vs zero returns 0
  - negative values compare correctly
  - fractional values compare correctly
  - very small difference detected
  - a NULL first pointer sorts first (negative)
  - a NULL second pointer sorts last (positive)
  - both NULL returns 0
*/
bool d_tests_functional_compare_double(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_compare_double");

    all_passed = true;
    all_passed &= d_internal_tests_functional_compare_double_part1(_suite);
    all_passed &= d_internal_tests_functional_compare_double_part2(_suite);

    return all_passed;
}


/**
 * @brief Part 1 of d_tests_functional_equal_int.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: equal values return true
 *   - test: unequal values return false
 *   - test: zero equals zero
 *   - test: negative values equal
 *   - test: negative vs positive
 *   - test: NULL first pointer returns false
 *   - test: NULL second pointer returns false
 *   - test: two NULLs are equal
 */
static bool
d_internal_tests_functional_equal_int_part1(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  a;
    int  b;
    bool result;

    all_passed = true;

    // --- test: equal values return true ---
    a      = 42;
    b      = 42;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: equal values return true",
                                   result == true,
                                   "expected true when both values are 42");

    // --- test: unequal values return false ---
    a      = 10;
    b      = 20;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: unequal values return false",
                                   result == false,
                                   "expected false for 10 vs 20");

    // --- test: zero equals zero ---
    a      = 0;
    b      = 0;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: zero equals zero",
                                   result == true,
                                   "expected true for 0 vs 0");

    // --- test: negative values equal ---
    a      = -7;
    b      = -7;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: negative values equal",
                                   result == true,
                                   "expected true for -7 vs -7");

    // --- test: negative vs positive ---
    a      = -1;
    b      = 1;
    result = d_functional_equal_int(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: -1 != 1",
                                   result == false,
                                   "expected false for -1 vs 1");

    // --- test: NULL first pointer returns false ---
    b      = 42;
    result = d_functional_equal_int(NULL, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: NULL first returns false",
                                   result == false,
                                   "expected false when first pointer is NULL");

    // --- test: NULL second pointer returns false ---
    a      = 42;
    result = d_functional_equal_int(&a, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: NULL second returns false",
                                   result == false,
                                   "expected false when second pointer is NULL");

    // --- test: two NULLs are equal ---
    result = d_functional_equal_int(NULL, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: both NULL are equal",
                                   result == true,
                                   "expected true: two missing operands are equal");

    return all_passed;
}


/**
 * @brief Part 2 of d_tests_functional_equal_int.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test: context is ignored
 */
static bool
d_internal_tests_functional_equal_int_part2(
    struct dj_test_suite* _suite
)
{
    bool all_passed;
    int  a;
    int  b;
    bool result;
    int  dummy_context;

    all_passed = true;

    // --- test: context is ignored ---
    a             = 42;
    b             = 42;
    dummy_context = 999;
    result        = d_functional_equal_int(&a, &b, &dummy_context);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_int: context is ignored",
                                   result == true,
                                   "expected true; context should not affect result");

    return all_passed;
}


/*
d_tests_functional_equal_int
  Tests d_functional_equal_int for correctness.
  Tests the following:
  - equal values return true
  - unequal values return false
  - zero equals zero
  - negative values equal
  - NULL first pointer returns false
  - NULL second pointer returns false
  - two NULLs are equal
  - context is ignored
*/
bool d_tests_functional_equal_int(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_equal_int");

    all_passed = true;
    all_passed &= d_internal_tests_functional_equal_int_part1(_suite);
    all_passed &= d_internal_tests_functional_equal_int_part2(_suite);

    return all_passed;
}


/*
d_tests_functional_equal_size_t
  Tests d_functional_equal_size_t for correctness.
  Tests the following:
  - equal values return true
  - unequal values return false
  - zero equals zero
  - large values compared correctly
  - NULL first pointer returns false
  - NULL second pointer returns false
  - two NULLs are equal
*/
bool d_tests_functional_equal_size_t(
    struct dj_test_suite* _suite
)
{
    bool   all_passed;
    size_t a;
    size_t b;
    bool   result;

    // validate parameter
    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "functional_equal_size_t");

    all_passed = true;

    // --- test: equal values return true ---
    a      = 100;
    b      = 100;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_size_t: equal values return true",
                                   result == true,
                                   "expected true when both values are 100");

    // --- test: unequal values return false ---
    a      = 10;
    b      = 20;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_size_t: unequal values return false",
                                   result == false,
                                   "expected false for 10 vs 20");

    // --- test: zero equals zero ---
    a      = 0;
    b      = 0;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_size_t: zero equals zero",
                                   result == true,
                                   "expected true for 0 vs 0");

    // --- test: large values ---
    a      = (size_t)-1;  /* SIZE_MAX */
    b      = (size_t)-1;
    result = d_functional_equal_size_t(&a, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_size_t: SIZE_MAX equals SIZE_MAX",
                                   result == true,
                                   "expected true for SIZE_MAX vs SIZE_MAX");

    // --- test: NULL first pointer returns false ---
    b      = 42;
    result = d_functional_equal_size_t(NULL, &b, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_size_t: NULL first returns false",
                                   result == false,
                                   "expected false when first pointer is NULL");

    // --- test: NULL second pointer returns false ---
    a      = 42;
    result = d_functional_equal_size_t(&a, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_size_t: NULL second returns false",
                                   result == false,
                                   "expected false when second pointer is NULL");

    // --- test: two NULLs are equal ---
    result = d_functional_equal_size_t(NULL, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "equal_size_t: both NULL are equal",
                                   result == true,
                                   "expected true: two missing operands are equal");

    return all_passed;
}
