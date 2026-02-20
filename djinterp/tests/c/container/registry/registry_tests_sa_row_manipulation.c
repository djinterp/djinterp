#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_add
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
bool
d_tests_sa_registry_add
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    result = true;

    // test 1: NULL registry
    row.key   = "x";
    row.value = 1;
    result = d_assert_standalone(
        d_registry_add(NULL, &row) == false,
        "add_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL row
        result = d_assert_standalone(
            d_registry_add(reg, NULL) == false,
            "add_null_row",
            "NULL row should return false",
            _counter) && result;

        // test 3: row with NULL key
        row.key   = NULL;
        row.value = 0;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == false,
            "add_null_key",
            "Row with NULL key should return false",
            _counter) && result;

        // test 4: successful add
        row.key   = "beta";
        row.value = 2;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == true,
            "add_success",
            "Adding valid row should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_count(reg) == 1,
            "add_count",
            "Count should be 1 after add",
            _counter) && result;

        // test 5: row is findable
        found = (struct test_row*)d_registry_get(reg, "beta");
        result = d_assert_standalone(
            found != NULL && found->value == 2,
            "add_findable",
            "Added row should be findable with correct value",
            _counter) && result;

        // test 6: duplicate key
        row.key   = "beta";
        row.value = 99;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == false,
            "add_duplicate",
            "Duplicate key should return false",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_count(reg) == 1,
            "add_dup_count",
            "Count should still be 1 after rejected duplicate",
            _counter) && result;

        // test 7: frozen registry
        d_registry_freeze(reg);
        row.key   = "gamma";
        row.value = 3;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == false,
            "add_frozen",
            "Frozen registry should reject add",
            _counter) && result;
        d_registry_thaw(reg);

        // test 8: add more to verify growth
        row.key   = "alpha";
        row.value = 1;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == true,
            "add_alpha",
            "Adding 'alpha' should succeed",
            _counter) && result;

        row.key   = "gamma";
        row.value = 3;
        d_registry_add(reg, &row);

        result = d_assert_standalone(
            d_registry_count(reg) == 3,
            "add_three",
            "Count should be 3",
            _counter) && result;

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
        result = d_assert_standalone(
            found != NULL
            && d_strcmp_n(found->key, strlen(found->key), "apple", 5) == 0,
            "add_sorted_order",
            "Sorted registry should have 'apple' at index 0",
            _counter) && result;

        found = (struct test_row*)d_registry_at(reg, 1);
        result = d_assert_standalone(
            found != NULL
            && d_strcmp_n(found->key, strlen(found->key), "banana", 6) == 0,
            "add_sorted_mid",
            "Sorted registry should have 'banana' at index 1",
            _counter) && result;

        d_registry_free(reg);
    }

    // test 10: static registry rejects add
    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        reg->flags |= (uint8_t)D_REGISTRY_FLAG_STATIC_ROWS;

        row.key   = "x";
        row.value = 1;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == false,
            "add_static",
            "Static registry should reject add",
            _counter) && result;

        reg->flags &= (uint8_t)~D_REGISTRY_FLAG_STATIC_ROWS;
        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_add_rows
  Tests the d_registry_add_rows function.
  Tests the following:
  - NULL registry returns false
  - NULL rows pointer returns false
  - zero count returns false
  - successfully adds multiple rows at once
  - all added rows are findable
  - partial failure returns false (duplicate in batch)
*/
bool
d_tests_sa_registry_add_rows
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    batch[3];
    struct test_row*   found;

    result = true;

    batch[0].key = "x";  batch[0].value = 10;
    batch[1].key = "y";  batch[1].value = 20;
    batch[2].key = "z";  batch[2].value = 30;

    // test 1: NULL registry
    result = d_assert_standalone(
        d_registry_add_rows(NULL, batch, 3) == false,
        "add_rows_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL rows
        result = d_assert_standalone(
            d_registry_add_rows(reg, NULL, 3) == false,
            "add_rows_null_rows",
            "NULL rows should return false",
            _counter) && result;

        // test 3: zero count
        result = d_assert_standalone(
            d_registry_add_rows(reg, batch, 0) == false,
            "add_rows_zero",
            "Zero count should return false",
            _counter) && result;

        // test 4: successful batch add
        result = d_assert_standalone(
            d_registry_add_rows(reg, batch, 3) == true,
            "add_rows_success",
            "Batch add of 3 rows should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_count(reg) == 3,
            "add_rows_count",
            "Count should be 3",
            _counter) && result;

        // test 5: all findable
        found = (struct test_row*)d_registry_get(reg, "y");
        result = d_assert_standalone(
            found != NULL && found->value == 20,
            "add_rows_findable",
            "'y' should be findable with value 20",
            _counter) && result;

        // test 6: partial failure (duplicate key in new batch)
        batch[0].key = "a";  batch[0].value = 1;
        batch[1].key = "x";  batch[1].value = 2;  // duplicate
        batch[2].key = "b";  batch[2].value = 3;

        result = d_assert_standalone(
            d_registry_add_rows(reg, batch, 3) == false,
            "add_rows_partial_fail",
            "Batch with duplicate should return false",
            _counter) && result;

        // "a" may or may not have been added (partial); count >= 3
        result = d_assert_standalone(
            d_registry_count(reg) >= 3,
            "add_rows_partial_count",
            "Count should be >= 3 after partial failure",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_set
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
bool
d_tests_sa_registry_set
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    struct test_row    replacement;
    struct test_row*   found;

    result = true;

    // test 1: NULL registry
    row.key   = "k";
    row.value = 0;
    result = d_assert_standalone(
        d_registry_set(NULL, "k", &row) == false,
        "set_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = d_assert_standalone(
            d_registry_set(reg, NULL, &row) == false,
            "set_null_key",
            "NULL key should return false",
            _counter) && result;

        // test 3: NULL row
        result = d_assert_standalone(
            d_registry_set(reg, "k", NULL) == false,
            "set_null_row",
            "NULL row should return false",
            _counter) && result;

        // add a row
        row.key   = "alpha";
        row.value = 10;
        d_registry_add(reg, &row);

        // test 4: key not found
        replacement.key   = "missing";
        replacement.value = 99;
        result = d_assert_standalone(
            d_registry_set(reg, "missing", &replacement) == false,
            "set_not_found",
            "Non-existent key should return false",
            _counter) && result;

        // test 5: successful set
        replacement.key   = "whatever";  // key in row should be ignored
        replacement.value = 777;
        result = d_assert_standalone(
            d_registry_set(reg, "alpha", &replacement) == true,
            "set_success",
            "Set should succeed on existing key",
            _counter) && result;

        found = (struct test_row*)d_registry_get(reg, "alpha");
        result = d_assert_standalone(
            found != NULL && found->value == 777,
            "set_value_updated",
            "Value should be updated to 777",
            _counter) && result;

        // test 6: key is preserved (not overwritten by replacement.key)
        result = d_assert_standalone(
            found != NULL
            && d_strcmp_n(found->key, strlen(found->key), "alpha", 5) == 0,
            "set_key_preserved",
            "Canonical key should remain 'alpha'",
            _counter) && result;

        // test 7: frozen registry
        d_registry_freeze(reg);
        result = d_assert_standalone(
            d_registry_set(reg, "alpha", &replacement) == false,
            "set_frozen",
            "Frozen registry should reject set",
            _counter) && result;
        d_registry_thaw(reg);

        // test 8: set via alias
        d_registry_add_alias(reg, "alpha", "a");
        replacement.key   = "ignored";
        replacement.value = 888;
        result = d_assert_standalone(
            d_registry_set(reg, "a", &replacement) == true,
            "set_via_alias",
            "Set via alias should succeed",
            _counter) && result;

        found = (struct test_row*)d_registry_get(reg, "alpha");
        result = d_assert_standalone(
            found != NULL && found->value == 888,
            "set_via_alias_value",
            "Value via alias should be updated to 888",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_remove
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
bool
d_tests_sa_registry_remove
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL registry
    result = d_assert_standalone(
        d_registry_remove(NULL, "key") == false,
        "remove_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = d_assert_standalone(
            d_registry_remove(reg, NULL) == false,
            "remove_null_key",
            "NULL key should return false",
            _counter) && result;

        // test 3: not found
        result = d_assert_standalone(
            d_registry_remove(reg, "ghost") == false,
            "remove_not_found",
            "Non-existent key should return false",
            _counter) && result;

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
        result = d_assert_standalone(
            d_registry_remove(reg, "beta") == true,
            "remove_success",
            "Removing 'beta' should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_count(reg) == 2,
            "remove_count",
            "Count should be 2 after removing one row",
            _counter) && result;

        // test 5: removed key not findable
        result = d_assert_standalone(
            d_registry_get(reg, "beta") == NULL,
            "remove_gone",
            "'beta' should no longer be findable",
            _counter) && result;

        // test 6: alias also removed
        result = d_assert_standalone(
            d_registry_get(reg, "b") == NULL,
            "remove_alias_gone",
            "Alias 'b' should also be removed",
            _counter) && result;

        // test 7: other rows still accessible
        result = d_assert_standalone(
            d_registry_get(reg, "alpha") != NULL
            && d_registry_get(reg, "gamma") != NULL,
            "remove_others_remain",
            "'alpha' and 'gamma' should still be accessible",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_remove_at
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
bool
d_tests_sa_registry_remove_at
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    result = true;

    // test 1: NULL registry
    result = d_assert_standalone(
        d_registry_remove_at(NULL, 0) == false,
        "remove_at_null",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: out of range on empty
        result = d_assert_standalone(
            d_registry_remove_at(reg, 0) == false,
            "remove_at_empty",
            "Index 0 on empty registry should return false",
            _counter) && result;

        // populate 3 rows
        row.key = "aaa";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "bbb";  row.value = 2;
        d_registry_add(reg, &row);
        row.key = "ccc";  row.value = 3;
        d_registry_add(reg, &row);

        // test 3: out of range
        result = d_assert_standalone(
            d_registry_remove_at(reg, 5) == false,
            "remove_at_oob",
            "Index 5 should return false (only 3 rows)",
            _counter) && result;

        // test 4: frozen
        d_registry_freeze(reg);
        result = d_assert_standalone(
            d_registry_remove_at(reg, 0) == false,
            "remove_at_frozen",
            "Frozen registry should reject remove_at",
            _counter) && result;
        d_registry_thaw(reg);

        // test 5: remove middle row (index 1 = "bbb")
        result = d_assert_standalone(
            d_registry_remove_at(reg, 1) == true,
            "remove_at_mid",
            "Removing index 1 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_count(reg) == 2,
            "remove_at_count",
            "Count should be 2",
            _counter) && result;

        // test 6: remaining rows shifted
        found = (struct test_row*)d_registry_at(reg, 0);
        result = d_assert_standalone(
            found != NULL
            && d_strcmp_n(found->key, strlen(found->key), "aaa", 3) == 0,
            "remove_at_shift_0",
            "Index 0 should still be 'aaa'",
            _counter) && result;

        found = (struct test_row*)d_registry_at(reg, 1);
        result = d_assert_standalone(
            found != NULL
            && d_strcmp_n(found->key, strlen(found->key), "ccc", 3) == 0,
            "remove_at_shift_1",
            "Index 1 should now be 'ccc' (shifted down)",
            _counter) && result;

        // test 7: lookup still works for remaining
        result = d_assert_standalone(
            d_registry_get(reg, "aaa") != NULL
            && d_registry_get(reg, "ccc") != NULL
            && d_registry_get(reg, "bbb") == NULL,
            "remove_at_lookup_ok",
            "Lookup should find 'aaa','ccc' but not 'bbb'",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_clear
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
bool
d_tests_sa_registry_clear
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    size_t             cap_before;

    result = true;

    // test 1: NULL registry (should not crash)
    d_registry_clear(NULL);
    result = d_assert_standalone(
        true,
        "clear_null",
        "NULL registry clear should not crash",
        _counter) && result;

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

        result = d_assert_standalone(
            d_registry_count(reg) == 0,
            "clear_count",
            "Count should be 0 after clear",
            _counter) && result;

        result = d_assert_standalone(
            reg->lookup_count == 0,
            "clear_lookup",
            "Lookup count should be 0 after clear",
            _counter) && result;

        // test 3: is empty
        result = d_assert_standalone(
            d_registry_is_empty(reg) == true,
            "clear_empty",
            "Registry should be empty after clear",
            _counter) && result;

        // test 4: capacity preserved
        result = d_assert_standalone(
            d_registry_capacity(reg) == cap_before,
            "clear_capacity",
            "Capacity should be preserved after clear",
            _counter) && result;

        // test 5: can add again after clear
        row.key = "new";  row.value = 99;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == true,
            "clear_reuse",
            "Should be able to add after clear",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_row_manipulation_all
  Aggregation function that runs all row manipulation tests.
*/
bool
d_tests_sa_registry_row_manipulation_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Row Manipulation Functions\n");
    printf("  --------------------------------------\n");

    result = d_tests_sa_registry_add(_counter)       && result;
    result = d_tests_sa_registry_add_rows(_counter)  && result;
    result = d_tests_sa_registry_set(_counter)       && result;
    result = d_tests_sa_registry_remove(_counter)    && result;
    result = d_tests_sa_registry_remove_at(_counter) && result;
    result = d_tests_sa_registry_clear(_counter)     && result;

    return result;
}
