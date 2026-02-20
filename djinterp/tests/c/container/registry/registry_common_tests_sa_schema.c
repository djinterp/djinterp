#include ".\registry_common_tests_sa.h"


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
        (d_type_info64)0,                  \
        NULL,                              \
        NULL                               \
    }


/*
d_tests_sa_registry_schema_max_enum_key_single
  Tests d_registry_schema_max_enum_key with a single row schema.
  Tests the following:
  - single row returns its enum_key as max
  - works with enum_key = 0
  - works with enum_key = 42
*/
bool
d_tests_sa_registry_schema_max_enum_key_single
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema_zero[] =
    {
        D_TEST_SCHEMA_ROW("key_zero", 0)
    };

    struct d_registry_schema_row schema_nonzero[] =
    {
        D_TEST_SCHEMA_ROW("key_fortytwo", 42)
    };

    result = true;

    // test 1: single row with enum_key = 0
    max_key = d_registry_schema_max_enum_key(schema_zero, 1);
    result  = d_assert_standalone(
        max_key == 0,
        "max_enum_key_single_zero",
        "Single row with enum_key=0 should return 0",
        _counter) && result;

    // test 2: single row with enum_key = 42
    max_key = d_registry_schema_max_enum_key(schema_nonzero, 1);
    result  = d_assert_standalone(
        max_key == 42,
        "max_enum_key_single_nonzero",
        "Single row with enum_key=42 should return 42",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_multiple
  Tests d_registry_schema_max_enum_key with multiple rows.
  Tests the following:
  - correctly finds maximum among multiple different values
  - handles non-contiguous enum_key values
*/
bool
d_tests_sa_registry_schema_max_enum_key_multiple
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 5),
        D_TEST_SCHEMA_ROW("key_b", 10),
        D_TEST_SCHEMA_ROW("key_c", 3),
        D_TEST_SCHEMA_ROW("key_d", 7),
        D_TEST_SCHEMA_ROW("key_e", 15)
    };

    result = true;

    // test: multiple rows, max is 15
    max_key = d_registry_schema_max_enum_key(schema, 5);
    result  = d_assert_standalone(
        max_key == 15,
        "max_enum_key_multiple",
        "Multiple rows should return max enum_key (15)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_first_is_max
  Tests d_registry_schema_max_enum_key when first element has max value.
  Tests the following:
  - correctly identifies max when it appears first
*/
bool
d_tests_sa_registry_schema_max_enum_key_first_is_max
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_first", 100),
        D_TEST_SCHEMA_ROW("key_second", 50),
        D_TEST_SCHEMA_ROW("key_third", 25),
        D_TEST_SCHEMA_ROW("key_fourth", 10)
    };

    result = true;

    // test: max is first element
    max_key = d_registry_schema_max_enum_key(schema, 4);
    result  = d_assert_standalone(
        max_key == 100,
        "max_enum_key_first_is_max",
        "Should find max (100) at first position",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_last_is_max
  Tests d_registry_schema_max_enum_key when last element has max value.
  Tests the following:
  - correctly identifies max when it appears last
*/
bool
d_tests_sa_registry_schema_max_enum_key_last_is_max
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_first", 10),
        D_TEST_SCHEMA_ROW("key_second", 25),
        D_TEST_SCHEMA_ROW("key_third", 50),
        D_TEST_SCHEMA_ROW("key_fourth", 100)
    };

    result = true;

    // test: max is last element
    max_key = d_registry_schema_max_enum_key(schema, 4);
    result  = d_assert_standalone(
        max_key == 100,
        "max_enum_key_last_is_max",
        "Should find max (100) at last position",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_middle_is_max
  Tests d_registry_schema_max_enum_key when middle element has max value.
  Tests the following:
  - correctly identifies max when it appears in the middle
*/
bool
d_tests_sa_registry_schema_max_enum_key_middle_is_max
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_first", 10),
        D_TEST_SCHEMA_ROW("key_second", 100),
        D_TEST_SCHEMA_ROW("key_third", 50)
    };

    result = true;

    // test: max is middle element
    max_key = d_registry_schema_max_enum_key(schema, 3);
    result  = d_assert_standalone(
        max_key == 100,
        "max_enum_key_middle_is_max",
        "Should find max (100) at middle position",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_null_schema
  Tests d_registry_schema_max_enum_key with NULL schema.
  Tests the following:
  - NULL schema returns 0
*/
bool
d_tests_sa_registry_schema_max_enum_key_null_schema
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    result = true;

    // test: NULL schema
    max_key = d_registry_schema_max_enum_key(NULL, 5);
    result  = d_assert_standalone(
        max_key == 0,
        "max_enum_key_null_schema",
        "NULL schema should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_zero_count
  Tests d_registry_schema_max_enum_key with zero count.
  Tests the following:
  - count=0 returns 0 even with valid schema pointer
*/
bool
d_tests_sa_registry_schema_max_enum_key_zero_count
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 100),
        D_TEST_SCHEMA_ROW("key_b", 200)
    };

    result = true;

    // test: zero count with valid schema pointer
    max_key = d_registry_schema_max_enum_key(schema, 0);
    result  = d_assert_standalone(
        max_key == 0,
        "max_enum_key_zero_count",
        "Zero count should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_all_same
  Tests d_registry_schema_max_enum_key when all enum_keys are the same.
  Tests the following:
  - correctly returns the common value when all keys are identical
*/
bool
d_tests_sa_registry_schema_max_enum_key_all_same
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 42),
        D_TEST_SCHEMA_ROW("key_b", 42),
        D_TEST_SCHEMA_ROW("key_c", 42),
        D_TEST_SCHEMA_ROW("key_d", 42)
    };

    result = true;

    // test: all same enum_key
    max_key = d_registry_schema_max_enum_key(schema, 4);
    result  = d_assert_standalone(
        max_key == 42,
        "max_enum_key_all_same",
        "All same enum_key (42) should return 42",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_max_uint16
  Tests d_registry_schema_max_enum_key with maximum uint16_t value.
  Tests the following:
  - correctly handles UINT16_MAX (65535)
*/
bool
d_tests_sa_registry_schema_max_enum_key_max_uint16
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_small", 100),
        D_TEST_SCHEMA_ROW("key_max", UINT16_MAX),
        D_TEST_SCHEMA_ROW("key_medium", 1000)
    };

    result = true;

    // test: UINT16_MAX (65535)
    max_key = d_registry_schema_max_enum_key(schema, 3);
    result  = d_assert_standalone(
        max_key == UINT16_MAX,
        "max_enum_key_uint16_max",
        "Should correctly handle UINT16_MAX (65535)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_zero_keys
  Tests d_registry_schema_max_enum_key when all enum_keys are zero.
  Tests the following:
  - correctly returns 0 when all keys are 0
*/
bool
d_tests_sa_registry_schema_max_enum_key_zero_keys
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    struct d_registry_schema_row schema[] =
    {
        D_TEST_SCHEMA_ROW("key_a", 0),
        D_TEST_SCHEMA_ROW("key_b", 0),
        D_TEST_SCHEMA_ROW("key_c", 0)
    };

    result = true;

    // test: all zero enum_keys
    max_key = d_registry_schema_max_enum_key(schema, 3);
    result  = d_assert_standalone(
        max_key == 0,
        "max_enum_key_all_zeros",
        "All zero enum_keys should return 0",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_sparse
  Tests d_registry_schema_max_enum_key with sparse enum_key values.
  Tests the following:
  - handles large gaps between enum_key values
  - typical cvar/registry usage pattern with aliases sharing enum_keys
*/
bool
d_tests_sa_registry_schema_max_enum_key_sparse
(
    struct d_test_counter* _counter
)
{
    bool     result;
    uint16_t max_key;

    // simulate registry schema with aliases (same enum_key, different keys)
    struct d_registry_schema_row schema[] =
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

    result = true;

    // test: sparse enum_keys with aliases
    max_key = d_registry_schema_max_enum_key(schema, 10);
    result  = d_assert_standalone(
        max_key == 100,
        "max_enum_key_sparse",
        "Sparse enum_keys should return max (100)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_registry_schema_max_enum_key_all
  Aggregation function that runs all d_registry_schema_max_enum_key tests.
*/
bool
d_tests_sa_registry_schema_max_enum_key_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Schema Max Enum Key Functions (d_registry_schema_max_enum_key)\n");
    printf("  --------------------------------------------------------------------------\n");

    result = d_tests_sa_registry_schema_max_enum_key_single(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_multiple(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_first_is_max(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_last_is_max(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_middle_is_max(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_null_schema(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_zero_count(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_all_same(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_max_uint16(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_zero_keys(_counter) && result;
    result = d_tests_sa_registry_schema_max_enum_key_sparse(_counter) && result;

    return result;
}
