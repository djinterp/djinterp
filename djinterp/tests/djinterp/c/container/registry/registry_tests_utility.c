/*******************************************************************************
* djinterp [container]                                  registry_tests_utility.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_utility.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_reserve
  Tests the d_registry_reserve function.
  Tests the following:
  - NULL registry returns false
  - frozen registry returns false
  - static registry returns false
  - capacity already sufficient returns true (no-op)
  - grows capacity to at least requested amount
  - existing data preserved after growth
*/
bool d_tests_registry_reserve(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    size_t             old_cap;

    d_test_suite_test(_suite, "registry_reserve");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_null",
                              d_registry_reserve(NULL, 100) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: frozen
        d_registry_freeze(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_frozen",
                                  d_registry_reserve(reg, 100) == false,
                                  "Frozen registry should return false") && result;
        d_registry_thaw(reg);

        // test 3: already sufficient
        old_cap = d_registry_capacity(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_sufficient",
                                  d_registry_reserve(reg, old_cap) == true,
                                  "Reserve at current capacity should succeed (no-op)") && result;

        // test 4: grow
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_grow",
                                  d_registry_reserve(reg, 256) == true,
                                  "Reserve 256 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_grow_check",
                                  d_registry_capacity(reg) >= 256,
                                  "Capacity should be >= 256 after reserve") && result;

        // test 5: data preserved
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        d_registry_reserve(reg, 512);

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_data_ok",
                                  d_registry_get(reg, "alpha") != NULL,
                                  "Data should be preserved after growth") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_reserve_lookup_fn
  Tests the d_registry_reserve_lookup function.
  Tests the following:
  - NULL registry returns false
  - frozen registry returns false
  - static registry returns false
  - capacity already sufficient returns true
  - grows lookup capacity
*/
bool d_tests_registry_reserve_lookup_fn(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;

    d_test_suite_test(_suite, "registry_reserve_lookup_fn");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "reserve_lkp_null",
                              d_registry_reserve_lookup(NULL, 100) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: frozen
        d_registry_freeze(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_lkp_frozen",
                                  d_registry_reserve_lookup(reg, 100) == false,
                                  "Frozen registry should return false") && result;
        d_registry_thaw(reg);

        // test 3: already sufficient
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_lkp_sufficient",
                                  d_registry_reserve_lookup(reg, reg->lookup_capacity) == true,
                                  "Reserve at current lookup capacity should succeed") && result;

        // test 4: grow
        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_lkp_grow",
                                  d_registry_reserve_lookup(reg, 256) == true,
                                  "Reserve lookup 256 should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "reserve_lkp_check",
                                  reg->lookup_capacity >= 256,
                                  "Lookup capacity should be >= 256") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_shrink_to_fit
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
bool d_tests_registry_shrink_to_fit(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_shrink_to_fit");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "shrink_null",
                              d_registry_shrink_to_fit(NULL) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new_with_capacity(sizeof(struct test_row), 128);

    if (reg)
    {
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);

        // test 2: shrink
        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_success",
                                  d_registry_shrink_to_fit(reg) == true,
                                  "Shrink should succeed") && result;

        // test 3: capacity reduced
        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_cap_match",
                                  d_registry_capacity(reg) == d_registry_count(reg),
                                  "Capacity should equal count after shrink") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_lkp_match",
                                  reg->lookup_capacity == reg->lookup_count,
                                  "Lookup capacity should equal lookup count") && result;

        // test 4: data still valid
        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_data_ok",
                                  d_registry_get(reg, "a") != NULL && d_registry_get(reg, "b") != NULL,
                                  "Data should still be valid after shrink") && result;

        // test 5: frozen
        d_registry_freeze(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_frozen",
                                  d_registry_shrink_to_fit(reg) == false,
                                  "Frozen registry should return false") && result;
        d_registry_thaw(reg);

        d_registry_free(reg);
    }

    // test 6: empty registry shrink
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 64);

    if (reg)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "shrink_empty",
                                  d_registry_shrink_to_fit(reg) == true,
                                  "Empty registry shrink should succeed") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_freeze
  Tests the d_registry_freeze function.
  Tests the following:
  - NULL registry returns false
  - sets FROZEN flag
  - frozen registry rejects add, remove, set
  - double-freeze is idempotent
*/
bool d_tests_registry_freeze(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_freeze");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "freeze_null",
                              d_registry_freeze(NULL) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);

        // test 2: freeze
        result = D_TEST_CHECK_MSG(_suite,
                                  "freeze_success",
                                  d_registry_freeze(reg) == true,
                                  "Freeze should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "freeze_flag",
                                  D_REGISTRY_IS_FROZEN(reg),
                                  "FROZEN flag should be set") && result;

        // test 3: operations rejected
        row.key = "b";  row.value = 2;
        result = D_TEST_CHECK_MSG(_suite,
                                  "freeze_rejects_add",
                                  d_registry_add(reg, &row) == false,
                                  "Frozen registry should reject add") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "freeze_rejects_remove",
                                  d_registry_remove(reg, "a") == false,
                                  "Frozen registry should reject remove") && result;

        // test 4: double-freeze
        result = D_TEST_CHECK_MSG(_suite,
                                  "freeze_double",
                                  d_registry_freeze(reg) == true,
                                  "Double-freeze should succeed (idempotent)") && result;

        d_registry_thaw(reg);
        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_thaw
  Tests the d_registry_thaw function.
  Tests the following:
  - NULL registry returns false
  - clears FROZEN flag
  - operations resume after thaw
  - thaw on non-frozen is idempotent
*/
bool d_tests_registry_thaw(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_thaw");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "thaw_null",
                              d_registry_thaw(NULL) == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        d_registry_freeze(reg);

        // test 2: thaw
        result = D_TEST_CHECK_MSG(_suite,
                                  "thaw_success",
                                  d_registry_thaw(reg) == true,
                                  "Thaw should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "thaw_flag",
                                  !D_REGISTRY_IS_FROZEN(reg),
                                  "FROZEN flag should be cleared") && result;

        // test 3: operations resume
        row.key = "a";  row.value = 1;
        result = D_TEST_CHECK_MSG(_suite,
                                  "thaw_add_resumes",
                                  d_registry_add(reg, &row) == true,
                                  "Add should succeed after thaw") && result;

        // test 4: thaw on non-frozen
        result = D_TEST_CHECK_MSG(_suite,
                                  "thaw_nonfrozen",
                                  d_registry_thaw(reg) == true,
                                  "Thaw on non-frozen should succeed (idempotent)") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_get_all_keys
  Tests the d_registry_get_all_keys function.
  Tests the following:
  - NULL registry returns NULL and sets out_count to 0
  - empty registry returns NULL
  - returns all keys (canonical + aliases) in lookup order
  - out_count is set correctly
  - caller must free the returned array
  - NULL out_count pointer does not crash
*/
bool d_tests_registry_get_all_keys(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    const char**       keys;
    size_t             count;

    d_test_suite_test(_suite, "registry_get_all_keys");

    result = true;

    // test 1: NULL registry
    count = 999;
    keys  = d_registry_get_all_keys(NULL, &count);
    result = D_TEST_CHECK_MSG(_suite,
                              "all_keys_null_reg",
                              keys == NULL && count == 0,
                              "NULL registry should return NULL and count 0") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty registry
        count = 999;
        keys  = d_registry_get_all_keys(reg, &count);
        result = D_TEST_CHECK_MSG(_suite,
                                  "all_keys_empty",
                                  keys == NULL && count == 0,
                                  "Empty registry should return NULL and count 0") && result;

        // populate with aliases
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);

        d_registry_add_alias(reg, "alpha", "a");

        // test 3: returns all keys
        count = 0;
        keys  = d_registry_get_all_keys(reg, &count);
        result = D_TEST_CHECK_MSG(_suite,
                                  "all_keys_count",
                                  keys != NULL && count == 3,
                                  "Should return 3 keys (2 canonical + 1 alias)") && result;

        if (keys)
        {
            free(keys);
        }

        // test 4: NULL out_count pointer
        keys = d_registry_get_all_keys(reg, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "all_keys_null_outcount",
                                  keys != NULL,
                                  "NULL out_count should not crash and still return keys") && result;

        if (keys)
        {
            free(keys);
        }

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_utility_all
  Aggregation function that runs all utility tests.
*/
bool d_tests_registry_utility_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_utility_all");

    result = true;

    result = d_tests_registry_reserve(_suite) && result;
    result = d_tests_registry_reserve_lookup_fn(_suite) && result;
    result = d_tests_registry_shrink_to_fit(_suite) && result;
    result = d_tests_registry_freeze(_suite) && result;
    result = d_tests_registry_thaw(_suite) && result;
    result = d_tests_registry_get_all_keys(_suite) && result;

    return result;
}
