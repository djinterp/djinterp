#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_lookup_compare
  Tests the d_registry_lookup_compare function.
  Tests the following:
  - equal keys return 0
  - "apple" < "banana" returns negative
  - "banana" > "apple" returns positive
  - NULL key handling (NULL < non-NULL)
  - both NULL returns 0
  - case-sensitive: "Apple" < "apple" (uppercase < lowercase in ASCII)
*/
bool
d_tests_sa_registry_lookup_compare
(
    struct d_test_counter* _counter
)
{
    bool                              result;
    struct d_registry_lookup_entry    a;
    struct d_registry_lookup_entry    b;
    int                               cmp;

    result = true;

    // test 1: equal
    a.key = "hello";  a.row_index = 0;
    b.key = "hello";  b.row_index = 1;
    cmp = d_registry_lookup_compare(&a, &b);
    result = d_assert_standalone(
        cmp == 0,
        "lkp_cmp_equal",
        "Equal keys should return 0",
        _counter) && result;

    // test 2: less than
    a.key = "apple";
    b.key = "banana";
    cmp = d_registry_lookup_compare(&a, &b);
    result = d_assert_standalone(
        cmp < 0,
        "lkp_cmp_less",
        "'apple' < 'banana' should return negative",
        _counter) && result;

    // test 3: greater than
    a.key = "banana";
    b.key = "apple";
    cmp = d_registry_lookup_compare(&a, &b);
    result = d_assert_standalone(
        cmp > 0,
        "lkp_cmp_greater",
        "'banana' > 'apple' should return positive",
        _counter) && result;

    // test 4: NULL < non-NULL
    a.key = NULL;
    b.key = "hello";
    cmp = d_registry_lookup_compare(&a, &b);
    result = d_assert_standalone(
        cmp < 0,
        "lkp_cmp_null_lt",
        "NULL < non-NULL should return negative",
        _counter) && result;

    // test 5: non-NULL > NULL
    a.key = "hello";
    b.key = NULL;
    cmp = d_registry_lookup_compare(&a, &b);
    result = d_assert_standalone(
        cmp > 0,
        "lkp_cmp_null_gt",
        "non-NULL > NULL should return positive",
        _counter) && result;

    // test 6: both NULL
    a.key = NULL;
    b.key = NULL;
    cmp = d_registry_lookup_compare(&a, &b);
    result = d_assert_standalone(
        cmp == 0,
        "lkp_cmp_both_null",
        "Both NULL should return 0",
        _counter) && result;

    // test 7: case-sensitive ordering
    a.key = "Apple";
    b.key = "apple";
    cmp = d_registry_lookup_compare(&a, &b);
    result = d_assert_standalone(
        cmp < 0,
        "lkp_cmp_case",
        "Case-sensitive: 'Apple' < 'apple'",
        _counter) && result;

    return result;
}

/*
d_tests_sa_registry_lookup_compare_nocase
  Tests the d_registry_lookup_compare_nocase function.
  Tests the following:
  - equal keys (same case) return 0
  - equal keys (different case) return 0
  - "alpha" < "beta" case-insensitive
  - NULL handling same as case-sensitive
  - mixed-case ordering is correct
*/
bool
d_tests_sa_registry_lookup_compare_nocase
(
    struct d_test_counter* _counter
)
{
    bool                              result;
    struct d_registry_lookup_entry    a;
    struct d_registry_lookup_entry    b;
    int                               cmp;

    result = true;

    // test 1: equal same case
    a.key = "hello";  a.row_index = 0;
    b.key = "hello";  b.row_index = 1;
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = d_assert_standalone(
        cmp == 0,
        "lkp_nocase_equal",
        "Same case equal keys should return 0",
        _counter) && result;

    // test 2: equal different case
    a.key = "Hello";
    b.key = "hELLO";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = d_assert_standalone(
        cmp == 0,
        "lkp_nocase_diff_case",
        "Different case equal keys should return 0",
        _counter) && result;

    // test 3: less than
    a.key = "ALPHA";
    b.key = "beta";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = d_assert_standalone(
        cmp < 0,
        "lkp_nocase_less",
        "'ALPHA' < 'beta' case-insensitive",
        _counter) && result;

    // test 4: greater than
    a.key = "Zebra";
    b.key = "apple";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = d_assert_standalone(
        cmp > 0,
        "lkp_nocase_greater",
        "'Zebra' > 'apple' case-insensitive",
        _counter) && result;

    // test 5: NULL handling
    a.key = NULL;
    b.key = "x";
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = d_assert_standalone(
        cmp < 0,
        "lkp_nocase_null",
        "NULL < non-NULL in nocase compare",
        _counter) && result;

    // test 6: both NULL
    a.key = NULL;
    b.key = NULL;
    cmp = d_registry_lookup_compare_nocase(&a, &b);
    result = d_assert_standalone(
        cmp == 0,
        "lkp_nocase_both_null",
        "Both NULL should return 0 in nocase compare",
        _counter) && result;

    return result;
}

/*
d_tests_sa_registry_comparison_all
  Aggregation function that runs all internal comparison tests.
*/
bool
d_tests_sa_registry_comparison_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Internal Comparison Functions\n");
    printf("  -----------------------------------------\n");

    result = d_tests_sa_registry_lookup_compare(_counter) && result;
    result = d_tests_sa_registry_lookup_compare_nocase(_counter) && result;

    return result;
}
