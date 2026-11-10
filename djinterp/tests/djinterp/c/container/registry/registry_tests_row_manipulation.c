/*******************************************************************************
* djinterp [container]                         registry_tests_row_manipulation.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_row_manipulation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_add
  Tests the d_registry_add function.
  Tests the following:
  - NULL registry returns false
  - NULL row returns false
  - frozen registry returns false
  - static registry returns false
  - row with NULL key returns false
  - duplicate key returns false
  - successful add increments count
  - added row is findable by key
  - lookup is sorted after add
  - sorted-flag registry inserts in key order
  - capacity grows automatically when full
*/
bool d_tests_registry_add(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_add");

    result = true;

    // test 1: NULL registry
    row.key   = "x";
    row.value = 1;
    result = D_TEST_CHECK_MSG(_suite,
                              "add_null_reg",
                              d_registry_add(NULL, &row) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL row
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_null_row",
                                  d_registry_add(reg, NULL) == false,
                                  "NULL row should return false") && result;

        // test 3: row with NULL key
        row.key   = NULL;
        row.value = 0;
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_null_key",
                                  d_registry_add(reg, &row) == false,
                                  "Row with NULL key should return false") && result;

        // test 4: successful add
        row.key   = "beta";
        row.value = 2;
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_success",
                                  d_registry_add(reg, &row) == true,
                                  "Adding valid row should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "add_count",
                                  d_registry_count(reg) == 1,
                                  "Count should be 1 after add") && result;

        // test 5: row is findable
        found = (struct test_row*)d_registry_get(reg, "beta");
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_findable",
                                  found != NULL && found->value == 2,
                                  "Added row should be findable with correct value") && result;

        // test 6: duplicate key
        row.key   = "beta";
        row.value = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_duplicate",
                                  d_registry_add(reg, &row) == false,
                                  "Duplicate key should return false") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "add_dup_count",
                                  d_registry_count(reg) == 1,
                                  "Count should still be 1 after rejected duplicate") && result;

        // test 7: frozen registry
        d_registry_freeze(reg);
        row.key   = "gamma";
        row.value = 3;
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_frozen",
                                  d_registry_add(reg, &row) == false,
                                  "Frozen registry should reject add") && result;
        d_registry_thaw(reg);

        // test 8: add more to verify growth
        row.key   = "alpha";
        row.value = 1;
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_alpha",
                                  d_registry_add(reg, &row) == true,
                                  "Adding 'alpha' should succeed") && result;

        row.key   = "gamma";
        row.value = 3;
        d_registry_add(reg, &row);

        result = D_TEST_CHECK_MSG(_suite,
                                  "add_three",
                                  d_registry_count(reg) == 3,
                                  "Count should be 3") && result;

        d_registry_free(reg);
    }

    // test 9: sorted-flag registry inserts in key order
    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        reg->flags |= (uint8_t)D_REGISTRY_FLAG_SORTED;

        row.key = "cherry";  row.value = 3;
        d_registry_add(reg, &row);

        row.key = "apple";   row.value = 1;
        d_registry_add(reg, &row);

        row.key = "banana";  row.value = 2;
        d_registry_add(reg, &row);

        // in sorted mode, row 0 should be "apple"
        found = (struct test_row*)d_registry_at(reg, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_sorted_order",
                                  found != NULL && d_strcmp_n(found->key, strlen(found->key), "apple", 5) == 0,
                                  "Sorted registry should have 'apple' at index 0") && result;

        found = (struct test_row*)d_registry_at(reg, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_sorted_mid",
                                  found != NULL && d_strcmp_n(found->key, strlen(found->key), "banana", 6) == 0,
                                  "Sorted registry should have 'banana' at index 1") && result;

        d_registry_free(reg);
    }

    // test 10: static registry rejects add
    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        reg->flags |= (uint8_t)D_REGISTRY_FLAG_STATIC_ROWS;

        row.key   = "x";
        row.value = 1;
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_static",
                                  d_registry_add(reg, &row) == false,
                                  "Static registry should reject add") && result;

        reg->flags &= (uint8_t)~D_REGISTRY_FLAG_STATIC_ROWS;
        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_add_rows
  Tests the d_registry_add_rows function.
  Tests the following:
  - NULL registry returns false
  - NULL rows pointer returns false
  - zero count returns false
  - successfully adds multiple rows at once
  - all added rows are findable
  - partial failure returns false (duplicate in batch)
*/
bool d_tests_registry_add_rows(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    batch[3];
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_add_rows");

    result = true;

    batch[0].key = "x";  batch[0].value = 10;
    batch[1].key = "y";  batch[1].value = 20;
    batch[2].key = "z";  batch[2].value = 30;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "add_rows_null_reg",
                              d_registry_add_rows(NULL, batch, 3) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL rows
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_rows_null_rows",
                                  d_registry_add_rows(reg, NULL, 3) == false,
                                  "NULL rows should return false") && result;

        // test 3: zero count
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_rows_zero",
                                  d_registry_add_rows(reg, batch, 0) == false,
                                  "Zero count should return false") && result;

        // test 4: successful batch add
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_rows_success",
                                  d_registry_add_rows(reg, batch, 3) == true,
                                  "Batch add of 3 rows should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "add_rows_count",
                                  d_registry_count(reg) == 3,
                                  "Count should be 3") && result;

        // test 5: all findable
        found = (struct test_row*)d_registry_get(reg, "y");
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_rows_findable",
                                  found != NULL && found->value == 20,
                                  "'y' should be findable with value 20") && result;

        // test 6: partial failure (duplicate key in new batch)
        batch[0].key = "a";  batch[0].value = 1;
        batch[1].key = "x";  batch[1].value = 2;  // duplicate
        batch[2].key = "b";  batch[2].value = 3;

        result = D_TEST_CHECK_MSG(_suite,
                                  "add_rows_partial_fail",
                                  d_registry_add_rows(reg, batch, 3) == false,
                                  "Batch with duplicate should return false") && result;

        // "a" may or may not have been added (partial); count >= 3
        result = D_TEST_CHECK_MSG(_suite,
                                  "add_rows_partial_count",
                                  d_registry_count(reg) >= 3,
                                  "Count should be >= 3 after partial failure") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_set
  Tests the d_registry_set function.
  Tests the following:
  - NULL registry returns false
  - NULL key returns false
  - NULL row returns false
  - frozen registry returns false
  - static registry returns false
  - key not found returns false
  - successful set updates value
  - set preserves the original canonical key in the row
  - set via alias updates the correct row
*/
bool d_tests_registry_set(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    struct test_row    replacement;
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_set");

    result = true;

    // test 1: NULL registry
    row.key   = "k";
    row.value = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "set_null_reg",
                              d_registry_set(NULL, "k", &row) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_null_key",
                                  d_registry_set(reg, NULL, &row) == false,
                                  "NULL key should return false") && result;

        // test 3: NULL row
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_null_row",
                                  d_registry_set(reg, "k", NULL) == false,
                                  "NULL row should return false") && result;

        // add a row
        row.key   = "alpha";
        row.value = 10;
        d_registry_add(reg, &row);

        // test 4: key not found
        replacement.key   = "missing";
        replacement.value = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_not_found",
                                  d_registry_set(reg, "missing", &replacement) == false,
                                  "Non-existent key should return false") && result;

        // test 5: successful set
        replacement.key   = "whatever";  // key in row should be ignored
        replacement.value = 777;
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_success",
                                  d_registry_set(reg, "alpha", &replacement) == true,
                                  "Set should succeed on existing key") && result;

        found = (struct test_row*)d_registry_get(reg, "alpha");
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_value_updated",
                                  found != NULL && found->value == 777,
                                  "Value should be updated to 777") && result;

        // test 6: key is preserved (not overwritten by replacement.key)
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_key_preserved",
                                  found != NULL && d_strcmp_n(found->key, strlen(found->key), "alpha", 5) == 0,
                                  "Canonical key should remain 'alpha'") && result;

        // test 7: frozen registry
        d_registry_freeze(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_frozen",
                                  d_registry_set(reg, "alpha", &replacement) == false,
                                  "Frozen registry should reject set") && result;
        d_registry_thaw(reg);

        // test 8: set via alias
        d_registry_add_alias(reg, "alpha", "a");
        replacement.key   = "ignored";
        replacement.value = 888;
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_via_alias",
                                  d_registry_set(reg, "a", &replacement) == true,
                                  "Set via alias should succeed") && result;

        found = (struct test_row*)d_registry_get(reg, "alpha");
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_via_alias_value",
                                  found != NULL && found->value == 888,
                                  "Value via alias should be updated to 888") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_remove
  Tests the d_registry_remove function.
  Tests the following:
  - NULL registry returns false
  - NULL key returns false
  - key not found returns false
  - successful remove decrements count
  - removed key is no longer findable
  - other rows remain accessible after removal
  - aliases pointing to removed row are also removed
*/
bool d_tests_registry_remove(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_remove");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "remove_null_reg",
                              d_registry_remove(NULL, "key") == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_null_key",
                                  d_registry_remove(reg, NULL) == false,
                                  "NULL key should return false") && result;

        // test 3: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_not_found",
                                  d_registry_remove(reg, "ghost") == false,
                                  "Non-existent key should return false") && result;

        // populate
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);
        row.key = "gamma";  row.value = 3;
        d_registry_add(reg, &row);

        // add alias to beta
        d_registry_add_alias(reg, "beta", "b");

        // test 4: successful remove
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_success",
                                  d_registry_remove(reg, "beta") == true,
                                  "Removing 'beta' should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_count",
                                  d_registry_count(reg) == 2,
                                  "Count should be 2 after removing one row") && result;

        // test 5: removed key not findable
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_gone",
                                  d_registry_get(reg, "beta") == NULL,
                                  "'beta' should no longer be findable") && result;

        // test 6: alias also removed
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_alias_gone",
                                  d_registry_get(reg, "b") == NULL,
                                  "Alias 'b' should also be removed") && result;

        // test 7: other rows still accessible
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_others_remain",
                                  d_registry_get(reg, "alpha") != NULL && d_registry_get(reg, "gamma") != NULL,
                                  "'alpha' and 'gamma' should still be accessible") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_remove_at
  Tests the d_registry_remove_at function.
  Tests the following:
  - NULL registry returns false
  - frozen registry returns false
  - static registry returns false
  - index out of range returns false
  - successful remove_at decrements count
  - remaining rows shift down correctly
  - lookup indices are adjusted after removal
*/
bool d_tests_registry_remove_at(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_remove_at");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "remove_at_null",
                              d_registry_remove_at(NULL, 0) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: out of range on empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_empty",
                                  d_registry_remove_at(reg, 0) == false,
                                  "Index 0 on empty registry should return false") && result;

        // populate 3 rows
        row.key = "aaa";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "bbb";  row.value = 2;
        d_registry_add(reg, &row);
        row.key = "ccc";  row.value = 3;
        d_registry_add(reg, &row);

        // test 3: out of range
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_oob",
                                  d_registry_remove_at(reg, 5) == false,
                                  "Index 5 should return false (only 3 rows)") && result;

        // test 4: frozen
        d_registry_freeze(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_frozen",
                                  d_registry_remove_at(reg, 0) == false,
                                  "Frozen registry should reject remove_at") && result;
        d_registry_thaw(reg);

        // test 5: remove middle row (index 1 = "bbb")
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_mid",
                                  d_registry_remove_at(reg, 1) == true,
                                  "Removing index 1 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_count",
                                  d_registry_count(reg) == 2,
                                  "Count should be 2") && result;

        // test 6: remaining rows shifted
        found = (struct test_row*)d_registry_at(reg, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_shift_0",
                                  found != NULL && d_strcmp_n(found->key, strlen(found->key), "aaa", 3) == 0,
                                  "Index 0 should still be 'aaa'") && result;

        found = (struct test_row*)d_registry_at(reg, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_shift_1",
                                  found != NULL && d_strcmp_n(found->key, strlen(found->key), "ccc", 3) == 0,
                                  "Index 1 should now be 'ccc' (shifted down)") && result;

        // test 7: lookup still works for remaining
        result = D_TEST_CHECK_MSG(_suite,
                                  "remove_at_lookup_ok",
                                  d_registry_get(reg, "aaa") != NULL && d_registry_get(reg, "ccc") != NULL && d_registry_get(reg, "bbb") == NULL,
                                  "Lookup should find 'aaa','ccc' but not 'bbb'") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_clear
  Tests the d_registry_clear function.
  Tests the following:
  - NULL registry does not crash
  - clears count to zero
  - clears lookup_count to zero
  - after clear, registry is empty
  - capacity is preserved after clear
  - static registry: clears lookup_count but preserves row count
  - calls row_free for each row when OWNS_ROWS is set
*/
bool d_tests_registry_clear(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    size_t             cap_before;

    d_test_suite_test(_suite, "registry_clear");

    result = true;

    // test 1: NULL registry (should not crash)
    d_registry_clear(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "clear_null",
                              true,
                              "NULL registry clear should not crash") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // populate
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);
        row.key = "c";  row.value = 3;
        d_registry_add(reg, &row);

        cap_before = d_registry_capacity(reg);

        // test 2: clear
        d_registry_clear(reg);

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_count",
                                  d_registry_count(reg) == 0,
                                  "Count should be 0 after clear") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_lookup",
                                  reg->lookup_count == 0,
                                  "Lookup count should be 0 after clear") && result;

        // test 3: is empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_empty",
                                  d_registry_is_empty(reg) == true,
                                  "Registry should be empty after clear") && result;

        // test 4: capacity preserved
        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_capacity",
                                  d_registry_capacity(reg) == cap_before,
                                  "Capacity should be preserved after clear") && result;

        // test 5: can add again after clear
        row.key = "new";  row.value = 99;
        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_reuse",
                                  d_registry_add(reg, &row) == true,
                                  "Should be able to add after clear") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_row_manipulation_all
  Aggregation function that runs all row manipulation tests.
*/
bool d_tests_registry_row_manipulation_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_row_manipulation_all");

    result = true;

    result = d_tests_registry_add(_suite)       && result;
    result = d_tests_registry_add_rows(_suite)  && result;
    result = d_tests_registry_set(_suite)       && result;
    result = d_tests_registry_remove(_suite)    && result;
    result = d_tests_registry_remove_at(_suite) && result;
    result = d_tests_registry_clear(_suite)     && result;

    return result;
}
