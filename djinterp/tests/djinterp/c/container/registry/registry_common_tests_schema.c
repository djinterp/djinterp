/*******************************************************************************
* djinterp [container]                            registry_common_tests_schema.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_common_tests_schema.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER DEFINITIONS FOR TEST SCHEMA ROWS
 *****************************************************************************/

// D_TEST_SCHEMA_ROW
//   macro: convenience macro for creating test schema rows.
// Simplifies test setup by providing default values for optional fields.
#define D_TEST_SCHEMA_ROW(_key, _enum_key) \
    {                                      \
        (_key),                            \
        NULL,                              \
        (uint16_t)(_enum_key),             \
        (dj_type_info64)0,                  \
        NULL,                              \
        NULL                               \
    }


/*
d_tests_registry_schema_max_enum_key_single
  Tests d_registry_schema_max_enum_key with a single row schema.
  Tests the following:
  - single row returns its enum_key as max
  - works with enum_key = 0
  - works with enum_key = 42
*/
bool d_tests_registry_schema_max_enum_key_single(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema_zero[] =
    {
        D_TEST_SCHEMA_ROW("key_zero", 0)
    };

    struct dj_registry_schema_row schema_nonzero[] =
    {
        D_TEST_SCHEMA_ROW("key_fortytwo", 42)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_single");

    result = true;

    // test 1: single row with enum_key = 0
    max_key = d_registry_schema_max_enum_key(schema_zero, 1);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_single_zero",
                               max_key == 0,
                               "Single row with enum_key=0 should return 0") && result;

    // test 2: single row with enum_key = 42
    max_key = d_registry_schema_max_enum_key(schema_nonzero, 1);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_single_nonzero",
                               max_key == 42,
                               "Single row with enum_key=42 should return 42") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_multiple
  Tests d_registry_schema_max_enum_key with multiple rows.
  Tests the following:
  - correctly finds maximum among multiple different values
  - handles non-contiguous enum_key values
*/
bool d_tests_registry_schema_max_enum_key_multiple(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 5),
        D_TEST_SCHEMA_ROW("key_b", 10),
        D_TEST_SCHEMA_ROW("key_c", 3),
        D_TEST_SCHEMA_ROW("key_d", 7),
        D_TEST_SCHEMA_ROW("key_e", 15)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_multiple");

    result = true;

    // test: multiple rows, max is 15
    max_key = d_registry_schema_max_enum_key(schema, 5);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_multiple",
                               max_key == 15,
                               "Multiple rows should return max enum_key (15)") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_first_is_max
  Tests d_registry_schema_max_enum_key when first element has max value.
  Tests the following:
  - correctly identifies max when it appears first
*/
bool d_tests_registry_schema_max_enum_key_first_is_max(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_first", 100),
        D_TEST_SCHEMA_ROW("key_second", 50),
        D_TEST_SCHEMA_ROW("key_third", 25),
        D_TEST_SCHEMA_ROW("key_fourth", 10)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_first_is_max");

    result = true;

    // test: max is first element
    max_key = d_registry_schema_max_enum_key(schema, 4);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_first_is_max",
                               max_key == 100,
                               "Should find max (100) at first position") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_last_is_max
  Tests d_registry_schema_max_enum_key when last element has max value.
  Tests the following:
  - correctly identifies max when it appears last
*/
bool d_tests_registry_schema_max_enum_key_last_is_max(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_first", 10),
        D_TEST_SCHEMA_ROW("key_second", 25),
        D_TEST_SCHEMA_ROW("key_third", 50),
        D_TEST_SCHEMA_ROW("key_fourth", 100)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_last_is_max");

    result = true;

    // test: max is last element
    max_key = d_registry_schema_max_enum_key(schema, 4);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_last_is_max",
                               max_key == 100,
                               "Should find max (100) at last position") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_middle_is_max
  Tests d_registry_schema_max_enum_key when middle element has max value.
  Tests the following:
  - correctly identifies max when it appears in the middle
*/
bool d_tests_registry_schema_max_enum_key_middle_is_max(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_first", 10),
        D_TEST_SCHEMA_ROW("key_second", 100),
        D_TEST_SCHEMA_ROW("key_third", 50)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_middle_is_max");

    result = true;

    // test: max is middle element
    max_key = d_registry_schema_max_enum_key(schema, 3);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_middle_is_max",
                               max_key == 100,
                               "Should find max (100) at middle position") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_null_schema
  Tests d_registry_schema_max_enum_key with NULL schema.
  Tests the following:
  - NULL schema returns 0
*/
bool d_tests_registry_schema_max_enum_key_null_schema(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    d_test_suite_test(_suite, "registry_schema_max_enum_key_null_schema");

    result = true;

    // test: NULL schema
    max_key = d_registry_schema_max_enum_key(NULL, 5);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_null_schema",
                               max_key == 0,
                               "NULL schema should return 0") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_zero_count
  Tests d_registry_schema_max_enum_key with zero count.
  Tests the following:
  - count=0 returns 0 even with valid schema pointer
*/
bool d_tests_registry_schema_max_enum_key_zero_count(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 100),
        D_TEST_SCHEMA_ROW("key_b", 200)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_zero_count");

    result = true;

    // test: zero count with valid schema pointer
    max_key = d_registry_schema_max_enum_key(schema, 0);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_zero_count",
                               max_key == 0,
                               "Zero count should return 0") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_all_same
  Tests d_registry_schema_max_enum_key when all enum_keys are the same.
  Tests the following:
  - correctly returns the common value when all keys are identical
*/
bool d_tests_registry_schema_max_enum_key_all_same(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 42),
        D_TEST_SCHEMA_ROW("key_b", 42),
        D_TEST_SCHEMA_ROW("key_c", 42),
        D_TEST_SCHEMA_ROW("key_d", 42)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_all_same");

    result = true;

    // test: all same enum_key
    max_key = d_registry_schema_max_enum_key(schema, 4);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_all_same",
                               max_key == 42,
                               "All same enum_key (42) should return 42") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_max_uint16
  Tests d_registry_schema_max_enum_key with maximum uint16_t value.
  Tests the following:
  - correctly handles UINT16_MAX (65535)
*/
bool d_tests_registry_schema_max_enum_key_max_uint16(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_small", 100),
        D_TEST_SCHEMA_ROW("key_max", UINT16_MAX),
        D_TEST_SCHEMA_ROW("key_medium", 1000)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_max_uint16");

    result = true;

    // test: UINT16_MAX (65535)
    max_key = d_registry_schema_max_enum_key(schema, 3);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_uint16_max",
                               max_key == UINT16_MAX,
                               "Should correctly handle UINT16_MAX (65535)") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_zero_keys
  Tests d_registry_schema_max_enum_key when all enum_keys are zero.
  Tests the following:
  - correctly returns 0 when all keys are 0
*/
bool d_tests_registry_schema_max_enum_key_zero_keys(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 0),
        D_TEST_SCHEMA_ROW("key_b", 0),
        D_TEST_SCHEMA_ROW("key_c", 0)
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_zero_keys");

    result = true;

    // test: all zero enum_keys
    max_key = d_registry_schema_max_enum_key(schema, 3);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_all_zeros",
                               max_key == 0,
                               "All zero enum_keys should return 0") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_sparse
  Tests d_registry_schema_max_enum_key with sparse enum_key values.
  Tests the following:
  - handles large gaps between enum_key values
  - typical cvar/registry usage pattern with aliases sharing enum_keys
*/
bool d_tests_registry_schema_max_enum_key_sparse(
    struct dj_test_suite* _suite
)
{
    bool     result;
    uint16_t max_key;

    // simulate registry schema with aliases (same enum_key, different keys)
    struct dj_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("sv_maxplayers",     0),
        D_TEST_SCHEMA_ROW("maxplayers",        0),    // alias for sv_maxplayers
        D_TEST_SCHEMA_ROW("sv_hostname",       1),
        D_TEST_SCHEMA_ROW("hostname",          1),    // alias
        D_TEST_SCHEMA_ROW("sv_password",       5),    // gap in enum_keys
        D_TEST_SCHEMA_ROW("password",          5),    // alias
        D_TEST_SCHEMA_ROW("sv_cheats",         100),  // large gap
        D_TEST_SCHEMA_ROW("cheats",            100),  // alias
        D_TEST_SCHEMA_ROW("sv_gravity",        50),
        D_TEST_SCHEMA_ROW("gravity",           50)    // alias
    };

    d_test_suite_test(_suite, "registry_schema_max_enum_key_sparse");

    result = true;

    // test: sparse enum_keys with aliases
    max_key = d_registry_schema_max_enum_key(schema, 10);
    result  = D_TEST_CHECK_MSG(_suite,
                               "max_enum_key_sparse",
                               max_key == 100,
                               "Sparse enum_keys should return max (100)") && result;

    return result;
}


/*
d_tests_registry_schema_max_enum_key_all
  Aggregation function that runs all d_registry_schema_max_enum_key tests.
*/
bool d_tests_registry_schema_max_enum_key_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_schema_max_enum_key_all");

    result = true;

    result = d_tests_registry_schema_max_enum_key_single(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_multiple(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_first_is_max(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_last_is_max(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_middle_is_max(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_null_schema(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_zero_count(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_all_same(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_max_uint16(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_zero_keys(_suite) && result;
    result = d_tests_registry_schema_max_enum_key_sparse(_suite) && result;

    return result;
}
