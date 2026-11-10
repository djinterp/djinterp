/*******************************************************************************
* djinterp [container]                                   registry_tests_common.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
/******************************************************************************
 * SECTION XI: REGISTRY_COMMON FUNCTION TESTS
 *****************************************************************************/

// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/**
 * @brief Part 1 of d_tests_registry_strcmp.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 1: both NULL
 *   - test 2: NULL < non-NULL
 *   - test 3: non-NULL > NULL
 *   - test 4: equal strings case-sensitive
 *   - test 5: less than case-sensitive
 *   - test 6: greater than case-sensitive
 *   - test 7: case-insensitive equal
 *   - test 8: case-insensitive less
 *   - test 9: case-insensitive greater
 *   - test 10: same pointer
 */
static bool
d_internal_tests_registry_strcmp_part1(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp;

    result = true;

    // test 1: both NULL
    cmp = d_registry_strcmp(NULL, NULL, true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_both_null",
                              cmp == 0,
                              "Both NULL should return 0") && result;

    // test 2: NULL < non-NULL
    cmp = d_registry_strcmp(NULL, "hello", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_null_lt",
                              cmp < 0,
                              "NULL < non-NULL should return negative") && result;

    // test 3: non-NULL > NULL
    cmp = d_registry_strcmp("hello", NULL, true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_null_gt",
                              cmp > 0,
                              "non-NULL > NULL should return positive") && result;

    // test 4: equal strings case-sensitive
    cmp = d_registry_strcmp("hello", "hello", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_equal_cs",
                              cmp == 0,
                              "Equal strings case-sensitive should return 0") && result;

    // test 5: less than case-sensitive
    cmp = d_registry_strcmp("apple", "banana", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_less_cs",
                              cmp < 0,
                              "'apple' < 'banana' case-sensitive") && result;

    // test 6: greater than case-sensitive
    cmp = d_registry_strcmp("banana", "apple", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_greater_cs",
                              cmp > 0,
                              "'banana' > 'apple' case-sensitive") && result;

    // test 7: case-insensitive equal
    cmp = d_registry_strcmp("Hello", "hELLO", false);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_equal_ci",
                              cmp == 0,
                              "'Hello' == 'hELLO' case-insensitive") && result;

    // test 8: case-insensitive less
    cmp = d_registry_strcmp("ALPHA", "beta", false);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_less_ci",
                              cmp < 0,
                              "'ALPHA' < 'beta' case-insensitive") && result;

    // test 9: case-insensitive greater
    cmp = d_registry_strcmp("Zebra", "apple", false);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_greater_ci",
                              cmp > 0,
                              "'Zebra' > 'apple' case-insensitive") && result;

    // test 10: same pointer
    {
        const char* s = "test";
        cmp = d_registry_strcmp(s, s, true);
        result = D_TEST_CHECK_MSG(_suite,
                                  "strcmp_same_ptr",
                                  cmp == 0,
                                  "Same pointer should return 0") && result;
    }

    return result;
}


/**
 * @brief Part 2 of d_tests_registry_strcmp.
 *
 * @param[in,out] _suite  the suite the checks are recorded in.
 * @return true if every check of the part passed.
 *
 * The test runs its parts in order, so that no function body is over
 * 100 lines. A part declares its own locals: no value crosses from one
 * part to the next.
 *
 * Sections:
 *   - test 11: prefix comparison
 *   - test 12: empty strings
 *   - test 13: empty vs non-empty
 *   - test 14: case-sensitive distinguishes case
 */
static bool
d_internal_tests_registry_strcmp_part2(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  cmp;

    result = true;

    // test 11: prefix comparison
    cmp = d_registry_strcmp("abc", "abcd", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_prefix",
                              cmp < 0,
                              "'abc' < 'abcd' (prefix is shorter)") && result;

    cmp = d_registry_strcmp("abcd", "abc", false);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_prefix_ci",
                              cmp > 0,
                              "'abcd' > 'abc' case-insensitive") && result;

    // test 12: empty strings
    cmp = d_registry_strcmp("", "", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_empty",
                              cmp == 0,
                              "Empty strings should return 0") && result;

    // test 13: empty vs non-empty
    cmp = d_registry_strcmp("", "a", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_empty_vs_nonempty",
                              cmp < 0,
                              "'' < 'a'") && result;

    // test 14: case-sensitive distinguishes case
    cmp = d_registry_strcmp("Hello", "hELLO", true);
    result = D_TEST_CHECK_MSG(_suite,
                              "strcmp_case_matters",
                              cmp != 0,
                              "'Hello' != 'hELLO' when case-sensitive") && result;

    return result;
}


/*
d_tests_registry_strcmp
  Tests the d_registry_strcmp function.
  Tests the following:
  - both NULL returns 0
  - NULL < non-NULL (returns negative)
  - non-NULL > NULL (returns positive)
  - equal strings (case-sensitive) return 0
  - "apple" < "banana" case-sensitive returns negative
  - "banana" > "apple" case-sensitive returns positive
  - case-insensitive: "Hello" == "hELLO" returns 0
  - case-insensitive: "ALPHA" < "beta" returns negative
  - same pointer returns 0
  - prefix comparison: "abc" < "abcd"
  - empty strings: "" == ""
*/
bool d_tests_registry_strcmp(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_test(_suite, "registry_strcmp");

    result = true;
    result &= d_internal_tests_registry_strcmp_part1(_suite);
    result &= d_internal_tests_registry_strcmp_part2(_suite);

    return result;
}

/*
d_tests_registry_schema_max_enum_key
  Tests the d_registry_schema_max_enum_key function.
  Tests the following:
  - NULL schema returns 0
  - zero count returns 0
  - single-entry returns that entry's enum_key
  - multiple entries returns the maximum
  - maximum is not necessarily the last element
*/
bool d_tests_registry_schema_max_enum_key(
    struct dj_test_suite* _suite
)
{
    bool                            result;
    uint16_t                        max_key;
    struct dj_registry_schema_row    schema[4];

    d_test_suite_test(_suite, "registry_schema_max_enum_key");

    result = true;

    // test 1: NULL schema
    max_key = d_registry_schema_max_enum_key(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "schema_max_null",
                              max_key == 0,
                              "NULL schema should return 0") && result;

    // test 2: zero count
    schema[0].key       = "a";
    schema[0].enum_key  = 10;
    max_key = d_registry_schema_max_enum_key(schema, 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "schema_max_zero",
                              max_key == 0,
                              "Zero count should return 0") && result;

    // test 3: single entry
    schema[0].key           = "first";
    schema[0].abbreviation  = NULL;
    schema[0].enum_key      = 7;
    schema[0].type          = 0;
    schema[0].default_value = NULL;
    schema[0].description   = NULL;

    max_key = d_registry_schema_max_enum_key(schema, 1);
    result = D_TEST_CHECK_MSG(_suite,
                              "schema_max_single",
                              max_key == 7,
                              "Single entry should return its enum_key (7)") && result;

    // test 4: multiple entries, max in the middle
    schema[0].enum_key = 3;
    schema[1].enum_key = 99;
    schema[2].enum_key = 50;
    schema[3].enum_key = 10;

    // fill required fields
    schema[1].key = "b";  schema[1].abbreviation = NULL;
    schema[1].type = 0;   schema[1].default_value = NULL;
    schema[1].description = NULL;
    schema[2].key = "c";  schema[2].abbreviation = NULL;
    schema[2].type = 0;   schema[2].default_value = NULL;
    schema[2].description = NULL;
    schema[3].key = "d";  schema[3].abbreviation = NULL;
    schema[3].type = 0;   schema[3].default_value = NULL;
    schema[3].description = NULL;

    max_key = d_registry_schema_max_enum_key(schema, 4);
    result = D_TEST_CHECK_MSG(_suite,
                              "schema_max_multi",
                              max_key == 99,
                              "Maximum enum_key should be 99") && result;

    // test 5: all same
    schema[0].enum_key = 42;
    schema[1].enum_key = 42;
    schema[2].enum_key = 42;
    schema[3].enum_key = 42;

    max_key = d_registry_schema_max_enum_key(schema, 4);
    result = D_TEST_CHECK_MSG(_suite,
                              "schema_max_all_same",
                              max_key == 42,
                              "All same enum_key should return 42") && result;

    // test 6: max at first element
    schema[0].enum_key = 200;
    schema[1].enum_key = 1;
    schema[2].enum_key = 2;
    schema[3].enum_key = 3;

    max_key = d_registry_schema_max_enum_key(schema, 4);
    result = D_TEST_CHECK_MSG(_suite,
                              "schema_max_first",
                              max_key == 200,
                              "Max at first element should return 200") && result;

    // test 7: max at last element
    schema[0].enum_key = 1;
    schema[1].enum_key = 2;
    schema[2].enum_key = 3;
    schema[3].enum_key = 500;

    max_key = d_registry_schema_max_enum_key(schema, 4);
    result = D_TEST_CHECK_MSG(_suite,
                              "schema_max_last",
                              max_key == 500,
                              "Max at last element should return 500") && result;

    return result;
}

/*
d_tests_registry_common_all
  Aggregation function that runs all registry_common tests.
*/
bool d_tests_registry_common_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_common_all");

    result = true;

    result = d_tests_registry_strcmp(_suite) && result;
    result = d_tests_registry_schema_max_enum_key(_suite) && result;

    return result;
}
