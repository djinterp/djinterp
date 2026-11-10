/*******************************************************************************
* djinterp [container]                               registry_tests_comparison.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_comparison.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_lookup_compare
  Tests the d_registry_lookup_compare function.
  Tests the following:
  - equal keys return 0
  - "apple" < "banana" returns negative
  - "banana" > "apple" returns positive
  - NULL key handling (NULL < non-NULL)
  - both NULL returns 0
  - case-sensitive: "Apple" < "apple" (uppercase < lowercase in ASCII)
*/
bool d_tests_registry_lookup_compare(
    struct dj_test_suite* _suite
)
{
    bool                              result;
    struct dj_registry_lookup_entry    a;
    struct dj_registry_lookup_entry    b;
    int                               cmp;

    d_test_suite_test(_suite, "registry_lookup_compare");

    result = true;

    // test 1: equal
    a.key = "hello";  a.row_index = 0;
    b.key = "hello";  b.row_index = 1;
    cmp = d_registry_lookup_compare(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_cmp_equal",
                              cmp == 0,
                              "Equal keys should return 0") && result;

    // test 2: less than
    a.key = "apple";
    b.key = "banana";
    cmp = d_registry_lookup_compare(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_cmp_less",
                              cmp < 0,
                              "'apple' < 'banana' should return negative") && result;

    // test 3: greater than
    a.key = "banana";
    b.key = "apple";
    cmp = d_registry_lookup_compare(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_cmp_greater",
                              cmp > 0,
                              "'banana' > 'apple' should return positive") && result;

    // test 4: NULL < non-NULL
    a.key = NULL;
    b.key = "hello";
    cmp = d_registry_lookup_compare(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_cmp_null_lt",
                              cmp < 0,
                              "NULL < non-NULL should return negative") && result;

    // test 5: non-NULL > NULL
    a.key = "hello";
    b.key = NULL;
    cmp = d_registry_lookup_compare(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_cmp_null_gt",
                              cmp > 0,
                              "non-NULL > NULL should return positive") && result;

    // test 6: both NULL
    a.key = NULL;
    b.key = NULL;
    cmp = d_registry_lookup_compare(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_cmp_both_null",
                              cmp == 0,
                              "Both NULL should return 0") && result;

    // test 7: case-sensitive ordering
    a.key = "Apple";
    b.key = "apple";
    cmp = d_registry_lookup_compare(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_cmp_case",
                              cmp < 0,
                              "Case-sensitive: 'Apple' < 'apple'") && result;

    return result;
}

/*
d_tests_registry_lookup_compare_nocase
  Tests the d_registry_lookup_compare_nocase function.
  Tests the following:
  - equal keys (same case) return 0
  - equal keys (different case) return 0
  - "alpha" < "beta" case-insensitive
  - NULL handling same as case-sensitive
  - mixed-case ordering is correct
*/
bool d_tests_registry_lookup_compare_nocase(
    struct dj_test_suite* _suite
)
{
    bool                              result;
    struct dj_registry_lookup_entry    a;
    struct dj_registry_lookup_entry    b;
    int                               cmp;

    d_test_suite_test(_suite, "registry_lookup_compare_nocase");

    result = true;

    // test 1: equal same case
    a.key = "hello";  a.row_index = 0;
    b.key = "hello";  b.row_index = 1;
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_nocase_equal",
                              cmp == 0,
                              "Same case equal keys should return 0") && result;

    // test 2: equal different case
    a.key = "Hello";
    b.key = "hELLO";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_nocase_diff_case",
                              cmp == 0,
                              "Different case equal keys should return 0") && result;

    // test 3: less than
    a.key = "ALPHA";
    b.key = "beta";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_nocase_less",
                              cmp < 0,
                              "'ALPHA' < 'beta' case-insensitive") && result;

    // test 4: greater than
    a.key = "Zebra";
    b.key = "apple";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_nocase_greater",
                              cmp > 0,
                              "'Zebra' > 'apple' case-insensitive") && result;

    // test 5: NULL handling
    a.key = NULL;
    b.key = "x";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_nocase_null",
                              cmp < 0,
                              "NULL < non-NULL in nocase compare") && result;

    // test 6: both NULL
    a.key = NULL;
    b.key = NULL;
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = D_TEST_CHECK_MSG(_suite,
                              "lkp_nocase_both_null",
                              cmp == 0,
                              "Both NULL should return 0 in nocase compare") && result;

    return result;
}

/*
d_tests_registry_comparison_all
  Aggregation function that runs all internal comparison tests.
*/
bool d_tests_registry_comparison_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_comparison_all");

    result = true;

    result = d_tests_registry_lookup_compare(_suite) && result;
    result = d_tests_registry_lookup_compare_nocase(_suite) && result;

    return result;
}
