#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_rebuild_lookup
  Tests the d_registry_rebuild_lookup function.
  Tests the following:
  - NULL registry does not crash
  - rebuilds lookup from row keys only (aliases are dropped)
  - lookup_count equals row count after rebuild
  - all canonical keys are findable after rebuild
  - previous aliases are removed by rebuild
  - empty registry rebuild sets lookup_count to 0
*/
bool
d_tests_sa_registry_rebuild_lookup
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL does not crash
    d_registry_rebuild_lookup(NULL);
    result = d_assert_standalone(
        true,
        "rebuild_null",
        "NULL registry rebuild should not crash",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // add rows and an alias
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);

        d_registry_add_alias(reg, "alpha", "a");

        // verify alias exists before rebuild
        result = d_assert_standalone(
            d_registry_get(reg, "a") != NULL,
            "rebuild_alias_before",
            "Alias 'a' should exist before rebuild",
            _counter) && result;

        // test 2: rebuild
        d_registry_rebuild_lookup(reg);

        // test 3: lookup_count == count
        result = d_assert_standalone(
            reg->lookup_count == d_registry_count(reg),
            "rebuild_lookup_count",
            "lookup_count should equal row count after rebuild",
            _counter) && result;

        // test 4: canonical keys findable
        result = d_assert_standalone(
            d_registry_get(reg, "alpha") != NULL
            && d_registry_get(reg, "beta") != NULL,
            "rebuild_canon_ok",
            "Canonical keys should be findable after rebuild",
            _counter) && result;

        // test 5: aliases dropped
        result = d_assert_standalone(
            d_registry_get(reg, "a") == NULL,
            "rebuild_alias_dropped",
            "Alias 'a' should be dropped after rebuild",
            _counter) && result;

        // test 6: empty registry
        d_registry_clear(reg);
        d_registry_rebuild_lookup(reg);

        result = d_assert_standalone(
            reg->lookup_count == 0,
            "rebuild_empty",
            "Empty registry rebuild should set lookup_count to 0",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_sort_lookup
  Tests the d_registry_sort_lookup function.
  Tests the following:
  - NULL registry does not crash
  - single-entry or empty lookup does not crash
  - after sort, entries are in ascending key order
  - case-insensitive sort uses nocase comparison
*/
bool
d_tests_sa_registry_sort_lookup
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    size_t             i;
    bool               sorted;

    result = true;

    // test 1: NULL does not crash
    d_registry_sort_lookup(NULL);
    result = d_assert_standalone(
        true,
        "sort_null",
        "NULL registry sort should not crash",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty sort
        d_registry_sort_lookup(reg);
        result = d_assert_standalone(
            true,
            "sort_empty",
            "Empty registry sort should not crash",
            _counter) && result;

        // add in reverse order
        row.key = "zebra";   row.value = 1;
        d_registry_add(reg, &row);
        row.key = "mango";   row.value = 2;
        d_registry_add(reg, &row);
        row.key = "apple";   row.value = 3;
        d_registry_add(reg, &row);

        // d_registry_add already sorts; force a manual verify
        d_registry_sort_lookup(reg);

        // test 3: verify ascending order
        sorted = true;

        for (i = 1; i < reg->lookup_count; ++i)
        {
            if (d_strcmp_n(reg->lookup[i - 1].key,
                           strlen(reg->lookup[i - 1].key),
                           reg->lookup[i].key,
                           strlen(reg->lookup[i].key)) > 0)
            {
                sorted = false;
                break;
            }
        }

        result = d_assert_standalone(
            sorted == true,
            "sort_ascending",
            "Lookup entries should be in ascending key order",
            _counter) && result;

        // test 4: case-insensitive sort
        reg->flags |= (uint8_t)D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        // just verify it completes without crash and keys findable
        result = d_assert_standalone(
            d_registry_get(reg, "APPLE") != NULL,
            "sort_nocase",
            "Case-insensitive sort should enable nocase lookup",
            _counter) && result;

        reg->flags &= (uint8_t)~D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_lookup_maint_all
  Aggregation function that runs all lookup maintenance tests.
*/
bool
d_tests_sa_registry_lookup_maint_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Lookup Maintenance Functions\n");
    printf("  ----------------------------------------\n");

    result = d_tests_sa_registry_rebuild_lookup(_counter) && result;
    result = d_tests_sa_registry_sort_lookup(_counter)    && result;

    return result;
}
