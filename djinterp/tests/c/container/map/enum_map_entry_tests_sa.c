/******************************************************************************
* djinterp [test]                                    enum_map_entry_tests_sa.c
*
*   Tests for enum_map_entry module - entry structure and macros only.
*   This tests the macros defined in enum_map_entry.h, not the map operations.
*
*
* path:      \test\container\map\enum_map_entry_tests_sa.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.24
******************************************************************************/

#include ".\enum_map_entry_tests_sa.h"


/******************************************************************************
 * BASIC ENTRY CREATION MACRO TESTS
 *****************************************************************************/

/*
d_tests_enum_map_entry_basic_macros
  Tests D_ENUM_ENTRY and D_ENUM_KEY_ENTRY (legacy alias).
  Tests the following:
  - D_ENUM_ENTRY creates entry with correct key
  - D_ENUM_ENTRY creates entry with correct value
  - D_ENUM_ENTRY handles NULL value
  - D_ENUM_ENTRY handles various pointer types
  - D_ENUM_KEY_ENTRY (legacy) works identically
  - entries are properly initialized
*/
struct d_test_object*
d_tests_enum_map_entry_basic_macros
(
    void
)
{
    struct d_test_object* group;
    int                   test_int = 42;
    char*                 test_str = "hello";
    bool                  test_correct_key;
    bool                  test_correct_value;
    bool                  test_null_value;
    bool                  test_pointer_types;
    bool                  test_legacy_alias;
    bool                  test_initialized;
    size_t                idx;

    // test 1: correct key
    struct d_enum_map_entry e1 = D_ENUM_ENTRY(TEST_COLOR_RED, &test_int);
    test_correct_key = (e1.key == (int)TEST_COLOR_RED);

    // test 2: correct value
    test_correct_value = (e1.value == (void*)&test_int);

    // test 3: NULL value
    struct d_enum_map_entry e2 = D_ENUM_ENTRY(TEST_COLOR_GREEN, NULL);
    test_null_value = (e2.key == (int)TEST_COLOR_GREEN) && (e2.value == NULL);

    // test 4: various pointer types
    struct d_enum_map_entry e3 = D_ENUM_ENTRY(TEST_COLOR_BLUE, test_str);
    test_pointer_types = (e3.key == (int)TEST_COLOR_BLUE) && 
                         (e3.value == (void*)test_str) &&
                         (strcmp((char*)e3.value, "hello") == 0);

    // test 5: legacy alias D_ENUM_KEY_ENTRY
    struct d_enum_map_entry e4 = D_ENUM_KEY_ENTRY(TEST_COLOR_YELLOW, &test_int);
    test_legacy_alias = (e4.key == (int)TEST_COLOR_YELLOW) && 
                        (e4.value == (void*)&test_int);

    // test 6: properly initialized (no garbage)
    struct d_enum_map_entry e5 = D_ENUM_ENTRY(0, NULL);
    test_initialized = (e5.key == 0) && (e5.value == NULL);

    // build result tree
    group = d_test_object_new_interior("D_ENUM_ENTRY & D_ENUM_KEY_ENTRY", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("correct_key",
                                           test_correct_key,
                                           "creates entry with correct key");
    group->elements[idx++] = D_ASSERT_TRUE("correct_value",
                                           test_correct_value,
                                           "creates entry with correct value");
    group->elements[idx++] = D_ASSERT_TRUE("null_value",
                                           test_null_value,
                                           "handles NULL value");
    group->elements[idx++] = D_ASSERT_TRUE("pointer_types",
                                           test_pointer_types,
                                           "handles various pointer types");
    group->elements[idx++] = D_ASSERT_TRUE("legacy_alias",
                                           test_legacy_alias,
                                           "D_ENUM_KEY_ENTRY works identically");
    group->elements[idx++] = D_ASSERT_TRUE("initialized",
                                           test_initialized,
                                           "entries properly initialized");

    return group;
}


/******************************************************************************
 * TYPED ENTRY CREATION MACRO TESTS
 *****************************************************************************/

/*
d_tests_enum_map_entry_typed_macros
  Tests specialized typed entry creation macros.
  Tests the following:
  - D_ENUM_ENTRY_STR creates string entry correctly
  - D_ENUM_ENTRY_INT creates integer entry correctly
  - D_ENUM_ENTRY_NULL creates NULL entry correctly
  - D_ENUM_ENTRY_SELF creates identity entry correctly
  - integer values preserved across cast
  - negative integers handled correctly
  - string literals accessible
*/
struct d_test_object*
d_tests_enum_map_entry_typed_macros
(
    void
)
{
    struct d_test_object* group;
    bool                  test_str_entry;
    bool                  test_int_entry;
    bool                  test_null_entry;
    bool                  test_self_entry;
    bool                  test_int_preservation;
    bool                  test_negative_int;
    bool                  test_str_accessible;
    size_t                idx;

    // test 1: D_ENUM_ENTRY_STR
    struct d_enum_map_entry e1 = D_ENUM_ENTRY_STR(TEST_COLOR_RED, "red");
    test_str_entry = (e1.key == (int)TEST_COLOR_RED) && 
                     (e1.value != NULL) &&
                     (strcmp((char*)e1.value, "red") == 0);

    // test 2: D_ENUM_ENTRY_INT
    struct d_enum_map_entry e2 = D_ENUM_ENTRY_INT(TEST_COLOR_GREEN, 42);
    test_int_entry = (e2.key == (int)TEST_COLOR_GREEN) && 
                     ((intptr_t)e2.value == 42);

    // test 3: D_ENUM_ENTRY_NULL
    struct d_enum_map_entry e3 = D_ENUM_ENTRY_NULL(TEST_COLOR_BLUE);
    test_null_entry = (e3.key == (int)TEST_COLOR_BLUE) && (e3.value == NULL);

    // test 4: D_ENUM_ENTRY_SELF
    struct d_enum_map_entry e4 = D_ENUM_ENTRY_SELF(TEST_COLOR_YELLOW);
    test_self_entry = (e4.key == (int)TEST_COLOR_YELLOW) && 
                      ((intptr_t)e4.value == (int)TEST_COLOR_YELLOW) &&
                      (e4.key == (intptr_t)e4.value);

    // test 5: integer value preservation
    struct d_enum_map_entry e5 = D_ENUM_ENTRY_INT(100, 999);
    test_int_preservation = (e5.key == 100) && ((intptr_t)e5.value == 999);

    // test 6: negative integers
    struct d_enum_map_entry e6 = D_ENUM_ENTRY_INT(-5, -100);
    test_negative_int = (e6.key == -5) && ((intptr_t)e6.value == -100);

    // test 7: string literals accessible
    struct d_enum_map_entry e7 = D_ENUM_ENTRY_STR(200, "Hello, World!");
    test_str_accessible = (e7.key == 200) && 
                          (strcmp((char*)e7.value, "Hello, World!") == 0) &&
                          (strlen((char*)e7.value) == 13);

    // build result tree
    group = d_test_object_new_interior("Typed Entry Macros", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("str_entry",
                                           test_str_entry,
                                           "D_ENUM_ENTRY_STR creates string entry");
    group->elements[idx++] = D_ASSERT_TRUE("int_entry",
                                           test_int_entry,
                                           "D_ENUM_ENTRY_INT creates integer entry");
    group->elements[idx++] = D_ASSERT_TRUE("null_entry",
                                           test_null_entry,
                                           "D_ENUM_ENTRY_NULL creates NULL entry");
    group->elements[idx++] = D_ASSERT_TRUE("self_entry",
                                           test_self_entry,
                                           "D_ENUM_ENTRY_SELF creates identity entry");
    group->elements[idx++] = D_ASSERT_TRUE("int_preservation",
                                           test_int_preservation,
                                           "integer values preserved across cast");
    group->elements[idx++] = D_ASSERT_TRUE("negative_int",
                                           test_negative_int,
                                           "negative integers handled correctly");
    group->elements[idx++] = D_ASSERT_TRUE("str_accessible",
                                           test_str_accessible,
                                           "string literals accessible");

    return group;
}


/******************************************************************************
 * ENTRY CREATION AGGREGATOR
 *****************************************************************************/

/*
d_tests_enum_map_entry_creation_all
  Runs all entry creation macro tests.
  Tests the following:
  - Basic entry macros
  - Typed entry macros
*/
struct d_test_object*
d_tests_enum_map_entry_creation_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Entry Creation Macros", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_enum_map_entry_basic_macros();
    group->elements[idx++] = d_tests_enum_map_entry_typed_macros();

    return group;
}


/******************************************************************************
 * ENTRY COMPARISON MACRO TESTS
 *****************************************************************************/

/*
d_tests_enum_map_entry_comparison_macros
  Tests entry comparison macros.
  Tests the following:
  - D_ENUM_ENTRY_KEY_EQ compares keys correctly
  - D_ENUM_ENTRY_KEY_EQ returns false for different keys
  - D_ENUM_ENTRY_VAL_EQ compares values correctly
  - D_ENUM_ENTRY_VAL_EQ returns false for different values
  - D_ENUM_ENTRY_EQ compares both key and value
  - D_ENUM_ENTRY_EQ requires both to match
  - handles NULL values in comparisons
*/
struct d_test_object*
d_tests_enum_map_entry_comparison_macros
(
    void
)
{
    struct d_test_object* group;
    int                   val1 = 42;
    int                   val2 = 99;
    bool                  test_key_eq_true;
    bool                  test_key_eq_false;
    bool                  test_val_eq_true;
    bool                  test_val_eq_false;
    bool                  test_entry_eq_true;
    bool                  test_entry_eq_requires_both;
    bool                  test_null_values;
    size_t                idx;

    // test 1: D_ENUM_ENTRY_KEY_EQ returns true for same key
    struct d_enum_map_entry e1 = D_ENUM_ENTRY(TEST_COLOR_RED, &val1);
    struct d_enum_map_entry e2 = D_ENUM_ENTRY(TEST_COLOR_RED, &val2);
    test_key_eq_true = D_ENUM_ENTRY_KEY_EQ(e1, e2);

    // test 2: D_ENUM_ENTRY_KEY_EQ returns false for different keys
    struct d_enum_map_entry e3 = D_ENUM_ENTRY(TEST_COLOR_GREEN, &val1);
    test_key_eq_false = !D_ENUM_ENTRY_KEY_EQ(e1, e3);

    // test 3: D_ENUM_ENTRY_VAL_EQ returns true for same value
    struct d_enum_map_entry e4 = D_ENUM_ENTRY(TEST_COLOR_BLUE, &val1);
    struct d_enum_map_entry e5 = D_ENUM_ENTRY(TEST_COLOR_YELLOW, &val1);
    test_val_eq_true = D_ENUM_ENTRY_VAL_EQ(e4, e5);

    // test 4: D_ENUM_ENTRY_VAL_EQ returns false for different values
    struct d_enum_map_entry e6 = D_ENUM_ENTRY(TEST_COLOR_PURPLE, &val2);
    test_val_eq_false = !D_ENUM_ENTRY_VAL_EQ(e4, e6);

    // test 5: D_ENUM_ENTRY_EQ returns true when both match
    struct d_enum_map_entry e7 = D_ENUM_ENTRY(TEST_COLOR_RED, &val1);
    struct d_enum_map_entry e8 = D_ENUM_ENTRY(TEST_COLOR_RED, &val1);
    test_entry_eq_true = D_ENUM_ENTRY_EQ(e7, e8);

    // test 6: D_ENUM_ENTRY_EQ requires both to match
    struct d_enum_map_entry e9 = D_ENUM_ENTRY(TEST_COLOR_RED, &val2);   // same key, diff value
    struct d_enum_map_entry e10 = D_ENUM_ENTRY(TEST_COLOR_GREEN, &val1); // diff key, same value
    test_entry_eq_requires_both = !D_ENUM_ENTRY_EQ(e7, e9) && !D_ENUM_ENTRY_EQ(e7, e10);

    // test 7: handles NULL values
    struct d_enum_map_entry e11 = D_ENUM_ENTRY_NULL(TEST_COLOR_BLUE);
    struct d_enum_map_entry e12 = D_ENUM_ENTRY_NULL(TEST_COLOR_BLUE);
    test_null_values = D_ENUM_ENTRY_EQ(e11, e12);

    // build result tree
    group = d_test_object_new_interior("Entry Comparison Macros", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("key_eq_true",
                                           test_key_eq_true,
                                           "D_ENUM_ENTRY_KEY_EQ returns true for same key");
    group->elements[idx++] = D_ASSERT_TRUE("key_eq_false",
                                           test_key_eq_false,
                                           "D_ENUM_ENTRY_KEY_EQ returns false for different keys");
    group->elements[idx++] = D_ASSERT_TRUE("val_eq_true",
                                           test_val_eq_true,
                                           "D_ENUM_ENTRY_VAL_EQ returns true for same value");
    group->elements[idx++] = D_ASSERT_TRUE("val_eq_false",
                                           test_val_eq_false,
                                           "D_ENUM_ENTRY_VAL_EQ returns false for different values");
    group->elements[idx++] = D_ASSERT_TRUE("entry_eq_true",
                                           test_entry_eq_true,
                                           "D_ENUM_ENTRY_EQ returns true when both match");
    group->elements[idx++] = D_ASSERT_TRUE("entry_eq_requires_both",
                                           test_entry_eq_requires_both,
                                           "D_ENUM_ENTRY_EQ requires both to match");
    group->elements[idx++] = D_ASSERT_TRUE("null_values",
                                           test_null_values,
                                           "handles NULL values in comparisons");

    return group;
}


/******************************************************************************
 * SENTINEL MARKER TESTS
 *****************************************************************************/

/*
d_tests_enum_map_entry_sentinel_macros
  Tests sentinel marker macros.
  Tests the following:
  - D_ENUM_ENTRY_SENTINEL creates sentinel
  - D_ENUM_ENTRY_IS_SENTINEL detects sentinel
  - sentinel has INT_MIN key
  - sentinel has NULL value
  - D_ENUM_ENTRY_IS_SENTINEL returns false for normal entries
  - multiple sentinels are equal
*/
struct d_test_object*
d_tests_enum_map_entry_sentinel_macros
(
    void
)
{
    struct d_test_object* group;
    int                   dummy = 42;
    bool                  test_creates_sentinel;
    bool                  test_detects_sentinel;
    bool                  test_sentinel_key;
    bool                  test_sentinel_value;
    bool                  test_false_for_normal;
    bool                  test_multiple_sentinels;
    size_t                idx;

    // test 1: D_ENUM_ENTRY_SENTINEL creates sentinel
    struct d_enum_map_entry sent = D_ENUM_ENTRY_SENTINEL;
    test_creates_sentinel = (sent.key == D_ENUM_MAP_SENTINEL_KEY) && 
                           (sent.value == NULL);

    // test 2: D_ENUM_ENTRY_IS_SENTINEL detects sentinel
    test_detects_sentinel = D_ENUM_ENTRY_IS_SENTINEL(sent);

    // test 3: sentinel has INT_MIN key (reserved value)
    test_sentinel_key = (sent.key == INT_MIN);

    // test 4: sentinel has NULL value
    test_sentinel_value = (sent.value == NULL);

    // test 5: D_ENUM_ENTRY_IS_SENTINEL returns false for normal entries
    struct d_enum_map_entry normal1 = D_ENUM_ENTRY(TEST_COLOR_RED, &dummy);
    struct d_enum_map_entry normal2 = D_ENUM_ENTRY_NULL(0);
    struct d_enum_map_entry normal3 = D_ENUM_ENTRY_INT(INT_MAX, 42);
    test_false_for_normal = !D_ENUM_ENTRY_IS_SENTINEL(normal1) && 
                           !D_ENUM_ENTRY_IS_SENTINEL(normal2) &&
                           !D_ENUM_ENTRY_IS_SENTINEL(normal3);

    // test 6: multiple sentinels are equal
    struct d_enum_map_entry sent2 = D_ENUM_ENTRY_SENTINEL;
    test_multiple_sentinels = D_ENUM_ENTRY_EQ(sent, sent2);

    // build result tree
    group = d_test_object_new_interior("Sentinel Marker Macros", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("creates_sentinel",
                                           test_creates_sentinel,
                                           "D_ENUM_ENTRY_SENTINEL creates sentinel");
    group->elements[idx++] = D_ASSERT_TRUE("detects_sentinel",
                                           test_detects_sentinel,
                                           "D_ENUM_ENTRY_IS_SENTINEL detects sentinel");
    group->elements[idx++] = D_ASSERT_TRUE("sentinel_key",
                                           test_sentinel_key,
                                           "sentinel has INT_MIN key");
    group->elements[idx++] = D_ASSERT_TRUE("sentinel_value",
                                           test_sentinel_value,
                                           "sentinel has NULL value");
    group->elements[idx++] = D_ASSERT_TRUE("false_for_normal",
                                           test_false_for_normal,
                                           "returns false for normal entries");
    group->elements[idx++] = D_ASSERT_TRUE("multiple_sentinels",
                                           test_multiple_sentinels,
                                           "multiple sentinels are equal");

    return group;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_enum_map_entry_run_all
  Master test runner for all enum_map_entry tests.
  Tests the following:
  - Entry creation macros
  - Entry comparison macros
  - Sentinel marker macros
  
  Note: This only tests the macros and entry structure.
  Map operations (min_enum_map) are tested separately.
*/
struct d_test_object*
d_tests_enum_map_entry_run_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    // create master test group
    group = d_test_object_new_interior("enum_map_entry Module Tests", 3);

    if (!group)
    {
        return NULL;
    }

    // add all test categories
    idx = 0;
    group->elements[idx++] = d_tests_enum_map_entry_creation_all();
    group->elements[idx++] = d_tests_enum_map_entry_comparison_macros();
    group->elements[idx++] = d_tests_enum_map_entry_sentinel_macros();

    return group;
}
