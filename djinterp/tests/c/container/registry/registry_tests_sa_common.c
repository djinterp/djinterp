/******************************************************************************
 * SECTION XI: REGISTRY_COMMON FUNCTION TESTS
 *****************************************************************************/

#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_strcmp
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
bool
d_tests_sa_registry_strcmp
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  cmp;

    result = true;

    // test 1: both NULL
    cmp = d_registry_strcmp(NULL, NULL, true);
    result = d_assert_standalone(
        cmp == 0,
        "strcmp_both_null",
        "Both NULL should return 0",
        _counter) && result;

    // test 2: NULL < non-NULL
    cmp = d_registry_strcmp(NULL, "hello", true);
    result = d_assert_standalone(
        cmp < 0,
        "strcmp_null_lt",
        "NULL < non-NULL should return negative",
        _counter) && result;

    // test 3: non-NULL > NULL
    cmp = d_registry_strcmp("hello", NULL, true);
    result = d_assert_standalone(
        cmp > 0,
        "strcmp_null_gt",
        "non-NULL > NULL should return positive",
        _counter) && result;

    // test 4: equal strings case-sensitive
    cmp = d_registry_strcmp("hello", "hello", true);
    result = d_assert_standalone(
        cmp == 0,
        "strcmp_equal_cs",
        "Equal strings case-sensitive should return 0",
        _counter) && result;

    // test 5: less than case-sensitive
    cmp = d_registry_strcmp("apple", "banana", true);
    result = d_assert_standalone(
        cmp < 0,
        "strcmp_less_cs",
        "'apple' < 'banana' case-sensitive",
        _counter) && result;

    // test 6: greater than case-sensitive
    cmp = d_registry_strcmp("banana", "apple", true);
    result = d_assert_standalone(
        cmp > 0,
        "strcmp_greater_cs",
        "'banana' > 'apple' case-sensitive",
        _counter) && result;

    // test 7: case-insensitive equal
    cmp = d_registry_strcmp("Hello", "hELLO", false);
    result = d_assert_standalone(
        cmp == 0,
        "strcmp_equal_ci",
        "'Hello' == 'hELLO' case-insensitive",
        _counter) && result;

    // test 8: case-insensitive less
    cmp = d_registry_strcmp("ALPHA", "beta", false);
    result = d_assert_standalone(
        cmp < 0,
        "strcmp_less_ci",
        "'ALPHA' < 'beta' case-insensitive",
        _counter) && result;

    // test 9: case-insensitive greater
    cmp = d_registry_strcmp("Zebra", "apple", false);
    result = d_assert_standalone(
        cmp > 0,
        "strcmp_greater_ci",
        "'Zebra' > 'apple' case-insensitive",
        _counter) && result;

    // test 10: same pointer
    {
        const char* s = "test";
        cmp = d_registry_strcmp(s, s, true);
        result = d_assert_standalone(
            cmp == 0,
            "strcmp_same_ptr",
            "Same pointer should return 0",
            _counter) && result;
    }

    // test 11: prefix comparison
    cmp = d_registry_strcmp("abc", "abcd", true);
    result = d_assert_standalone(
        cmp < 0,
        "strcmp_prefix",
        "'abc' < 'abcd' (prefix is shorter)",
        _counter) && result;

    cmp = d_registry_strcmp("abcd", "abc", false);
    result = d_assert_standalone(
        cmp > 0,
        "strcmp_prefix_ci",
        "'abcd' > 'abc' case-insensitive",
        _counter) && result;

    // test 12: empty strings
    cmp = d_registry_strcmp("", "", true);
    result = d_assert_standalone(
        cmp == 0,
        "strcmp_empty",
        "Empty strings should return 0",
        _counter) && result;

    // test 13: empty vs non-empty
    cmp = d_registry_strcmp("", "a", true);
    result = d_assert_standalone(
        cmp < 0,
        "strcmp_empty_vs_nonempty",
        "'' < 'a'",
        _counter) && result;

    // test 14: case-sensitive distinguishes case
    cmp = d_registry_strcmp("Hello", "hELLO", true);
    result = d_assert_standalone(
        cmp != 0,
        "strcmp_case_matters",
        "'Hello' != 'hELLO' when case-sensitive",
        _counter) && result;

    return result;
}

/*
d_tests_sa_registry_schema_max_enum_key
  Tests the d_registry_schema_max_enum_key function.
  Tests the following:
  - NULL schema returns 0
  - zero count returns 0
  - single-entry returns that entry's enum_key
  - multiple entries returns the maximum
  - maximum is not necessarily the last element
*/
bool
d_tests_sa_registry_schema_max_enum_key
(
    struct d_test_counter* _counter
)
{
    bool                            result;
    uint16_t                        max_key;
    struct d_registry_schema_row    schema[4];

    result = true;

    // test 1: NULL schema
    max_key = d_registry_schema_max_enum_key(NULL, 5);
    result = d_assert_standalone(
        max_key == 0,
        "schema_max_null",
        "NULL schema should return 0",
        _counter) && result;

    // test 2: zero count
    schema[0].key       = "a";
    schema[0].enum_key  = 10;
    max_key = d_registry_schema_max_enum_key(schema, 0);
    result = d_assert_standalone(
        max_key == 0,
        "schema_max_zero",
        "Zero count should return 0",
        _counter) && result;

    // test 3: single entry
    schema[0].key           = "first";
    schema[0].abbreviation  = NULL;
    schema[0].enum_key      = 7;
    schema[0].type          = 0;
    schema[0].default_value = NULL;
    schema[0].description   = NULL;

    max_key = d_registry_schema_max_enum_key(schema, 1);
    result = d_assert_standalone(
        max_key == 7,
        "schema_max_single",
        "Single entry should return its enum_key (7)",
        _counter) && result;

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
    result = d_assert_standalone(
        max_key == 99,
        "schema_max_multi",
        "Maximum enum_key should be 99",
        _counter) && result;

    // test 5: all same
    schema[0].enum_key = 42;
    schema[1].enum_key = 42;
    schema[2].enum_key = 42;
    schema[3].enum_key = 42;

    max_key = d_registry_schema_max_enum_key(schema, 4);
    result = d_assert_standalone(
        max_key == 42,
        "schema_max_all_same",
        "All same enum_key should return 42",
        _counter) && result;

    // test 6: max at first element
    schema[0].enum_key = 200;
    schema[1].enum_key = 1;
    schema[2].enum_key = 2;
    schema[3].enum_key = 3;

    max_key = d_registry_schema_max_enum_key(schema, 4);
    result = d_assert_standalone(
        max_key == 200,
        "schema_max_first",
        "Max at first element should return 200",
        _counter) && result;

    // test 7: max at last element
    schema[0].enum_key = 1;
    schema[1].enum_key = 2;
    schema[2].enum_key = 3;
    schema[3].enum_key = 500;

    max_key = d_registry_schema_max_enum_key(schema, 4);
    result = d_assert_standalone(
        max_key == 500,
        "schema_max_last",
        "Max at last element should return 500",
        _counter) && result;

    return result;
}

/*
d_tests_sa_registry_common_all
  Aggregation function that runs all registry_common tests.
*/
bool
d_tests_sa_registry_common_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Registry Common Functions\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_registry_strcmp(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key(_counter) && result;

    return result;
}
