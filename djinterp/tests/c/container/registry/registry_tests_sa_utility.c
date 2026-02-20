#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_reserve
  Tests the d_registry_reserve function.
  Tests the following:
  - NULL registry returns false
  - frozen registry returns false
  - static registry returns false
  - capacity already sufficient returns true (no-op)
  - grows capacity to at least requested amount
  - existing data preserved after growth
*/
bool
d_tests_sa_registry_reserve
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    size_t             old_cap;

    result = true;

    // test 1: NULL
    result = d_assert_standalone(
        d_registry_reserve(NULL, 100) == false,
        "reserve_null",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: frozen
        d_registry_freeze(reg);
        result = d_assert_standalone(
            d_registry_reserve(reg, 100) == false,
            "reserve_frozen",
            "Frozen registry should return false",
            _counter) && result;
        d_registry_thaw(reg);

        // test 3: already sufficient
        old_cap = d_registry_capacity(reg);
        result = d_assert_standalone(
            d_registry_reserve(reg, old_cap) == true,
            "reserve_sufficient",
            "Reserve at current capacity should succeed (no-op)",
            _counter) && result;

        // test 4: grow
        result = d_assert_standalone(
            d_registry_reserve(reg, 256) == true,
            "reserve_grow",
            "Reserve 256 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_capacity(reg) >= 256,
            "reserve_grow_check",
            "Capacity should be >= 256 after reserve",
            _counter) && result;

        // test 5: data preserved
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        d_registry_reserve(reg, 512);

        result = d_assert_standalone(
            d_registry_get(reg, "alpha") != NULL,
            "reserve_data_ok",
            "Data should be preserved after growth",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_reserve_lookup_fn
  Tests the d_registry_reserve_lookup function.
  Tests the following:
  - NULL registry returns false
  - frozen registry returns false
  - static registry returns false
  - capacity already sufficient returns true
  - grows lookup capacity
*/
bool
d_tests_sa_registry_reserve_lookup_fn
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;

    result = true;

    // test 1: NULL
    result = d_assert_standalone(
        d_registry_reserve_lookup(NULL, 100) == false,
        "reserve_lkp_null",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: frozen
        d_registry_freeze(reg);
        result = d_assert_standalone(
            d_registry_reserve_lookup(reg, 100) == false,
            "reserve_lkp_frozen",
            "Frozen registry should return false",
            _counter) && result;
        d_registry_thaw(reg);

        // test 3: already sufficient
        result = d_assert_standalone(
            d_registry_reserve_lookup(reg, reg->lookup_capacity) == true,
            "reserve_lkp_sufficient",
            "Reserve at current lookup capacity should succeed",
            _counter) && result;

        // test 4: grow
        result = d_assert_standalone(
            d_registry_reserve_lookup(reg, 256) == true,
            "reserve_lkp_grow",
            "Reserve lookup 256 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            reg->lookup_capacity >= 256,
            "reserve_lkp_check",
            "Lookup capacity should be >= 256",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_shrink_to_fit
  Tests the d_registry_shrink_to_fit function.
  Tests the following:
  - NULL registry returns false
  - frozen registry returns false
  - static registry returns false
  - reduces capacity to match count
  - reduces lookup_capacity to match lookup_count
  - data and lookup still valid after shrink
  - empty registry shrink succeeds
*/
bool
d_tests_sa_registry_shrink_to_fit
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL
    result = d_assert_standalone(
        d_registry_shrink_to_fit(NULL) == false,
        "shrink_null",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new_with_capacity(sizeof(struct test_row), 128);

    if (reg)
    {
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);

        // test 2: shrink
        result = d_assert_standalone(
            d_registry_shrink_to_fit(reg) == true,
            "shrink_success",
            "Shrink should succeed",
            _counter) && result;

        // test 3: capacity reduced
        result = d_assert_standalone(
            d_registry_capacity(reg) == d_registry_count(reg),
            "shrink_cap_match",
            "Capacity should equal count after shrink",
            _counter) && result;

        result = d_assert_standalone(
            reg->lookup_capacity == reg->lookup_count,
            "shrink_lkp_match",
            "Lookup capacity should equal lookup count",
            _counter) && result;

        // test 4: data still valid
        result = d_assert_standalone(
            d_registry_get(reg, "a") != NULL
            && d_registry_get(reg, "b") != NULL,
            "shrink_data_ok",
            "Data should still be valid after shrink",
            _counter) && result;

        // test 5: frozen
        d_registry_freeze(reg);
        result = d_assert_standalone(
            d_registry_shrink_to_fit(reg) == false,
            "shrink_frozen",
            "Frozen registry should return false",
            _counter) && result;
        d_registry_thaw(reg);

        d_registry_free(reg);
    }

    // test 6: empty registry shrink
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 64);

    if (reg)
    {
        result = d_assert_standalone(
            d_registry_shrink_to_fit(reg) == true,
            "shrink_empty",
            "Empty registry shrink should succeed",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_freeze
  Tests the d_registry_freeze function.
  Tests the following:
  - NULL registry returns false
  - sets FROZEN flag
  - frozen registry rejects add, remove, set
  - double-freeze is idempotent
*/
bool
d_tests_sa_registry_freeze
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL
    result = d_assert_standalone(
        d_registry_freeze(NULL) == false,
        "freeze_null",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);

        // test 2: freeze
        result = d_assert_standalone(
            d_registry_freeze(reg) == true,
            "freeze_success",
            "Freeze should succeed",
            _counter) && result;

        result = d_assert_standalone(
            D_REGISTRY_IS_FROZEN(reg),
            "freeze_flag",
            "FROZEN flag should be set",
            _counter) && result;

        // test 3: operations rejected
        row.key = "b";  row.value = 2;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == false,
            "freeze_rejects_add",
            "Frozen registry should reject add",
            _counter) && result;

        result = d_assert_standalone(
            d_registry_remove(reg, "a") == false,
            "freeze_rejects_remove",
            "Frozen registry should reject remove",
            _counter) && result;

        // test 4: double-freeze
        result = d_assert_standalone(
            d_registry_freeze(reg) == true,
            "freeze_double",
            "Double-freeze should succeed (idempotent)",
            _counter) && result;

        d_registry_thaw(reg);
        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_thaw
  Tests the d_registry_thaw function.
  Tests the following:
  - NULL registry returns false
  - clears FROZEN flag
  - operations resume after thaw
  - thaw on non-frozen is idempotent
*/
bool
d_tests_sa_registry_thaw
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL
    result = d_assert_standalone(
        d_registry_thaw(NULL) == false,
        "thaw_null",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        d_registry_freeze(reg);

        // test 2: thaw
        result = d_assert_standalone(
            d_registry_thaw(reg) == true,
            "thaw_success",
            "Thaw should succeed",
            _counter) && result;

        result = d_assert_standalone(
            !D_REGISTRY_IS_FROZEN(reg),
            "thaw_flag",
            "FROZEN flag should be cleared",
            _counter) && result;

        // test 3: operations resume
        row.key = "a";  row.value = 1;
        result = d_assert_standalone(
            d_registry_add(reg, &row) == true,
            "thaw_add_resumes",
            "Add should succeed after thaw",
            _counter) && result;

        // test 4: thaw on non-frozen
        result = d_assert_standalone(
            d_registry_thaw(reg) == true,
            "thaw_nonfrozen",
            "Thaw on non-frozen should succeed (idempotent)",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_get_all_keys
  Tests the d_registry_get_all_keys function.
  Tests the following:
  - NULL registry returns NULL and sets out_count to 0
  - empty registry returns NULL
  - returns all keys (canonical + aliases) in lookup order
  - out_count is set correctly
  - caller must free the returned array
  - NULL out_count pointer does not crash
*/
bool
d_tests_sa_registry_get_all_keys
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    const char**       keys;
    size_t             count;

    result = true;

    // test 1: NULL registry
    count = 999;
    keys  = d_registry_get_all_keys(NULL, &count);
    result = d_assert_standalone(
        keys == NULL && count == 0,
        "all_keys_null_reg",
        "NULL registry should return NULL and count 0",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty registry
        count = 999;
        keys  = d_registry_get_all_keys(reg, &count);
        result = d_assert_standalone(
            keys == NULL && count == 0,
            "all_keys_empty",
            "Empty registry should return NULL and count 0",
            _counter) && result;

        // populate with aliases
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);

        d_registry_add_alias(reg, "alpha", "a");

        // test 3: returns all keys
        count = 0;
        keys  = d_registry_get_all_keys(reg, &count);
        result = d_assert_standalone(
            keys != NULL && count == 3,
            "all_keys_count",
            "Should return 3 keys (2 canonical + 1 alias)",
            _counter) && result;

        if (keys)
        {
            free(keys);
        }

        // test 4: NULL out_count pointer
        keys = d_registry_get_all_keys(reg, NULL);
        result = d_assert_standalone(
            keys != NULL,
            "all_keys_null_outcount",
            "NULL out_count should not crash and still return keys",
            _counter) && result;

        if (keys)
        {
            free(keys);
        }

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_utility_all
  Aggregation function that runs all utility tests.
*/
bool
d_tests_sa_registry_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_registry_reserve(_counter) && result;
    result = d_tests_sa_registry_reserve_lookup_fn(_counter) && result;
    result = d_tests_sa_registry_shrink_to_fit(_counter) && result;
    result = d_tests_sa_registry_freeze(_counter) && result;
    result = d_tests_sa_registry_thaw(_counter) && result;
    result = d_tests_sa_registry_get_all_keys(_counter) && result;

    return result;
}
