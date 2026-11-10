/*******************************************************************************
* djinterp [container]                             registry_tests_lookup_maint.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_lookup_maint.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_rebuild_lookup
  Tests the d_registry_rebuild_lookup function.
  Tests the following:
  - NULL registry does not crash
  - rebuilds lookup from row keys only (aliases are dropped)
  - lookup_count equals row count after rebuild
  - all canonical keys are findable after rebuild
  - previous aliases are removed by rebuild
  - empty registry rebuild sets lookup_count to 0
*/
bool d_tests_registry_rebuild_lookup(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_rebuild_lookup");

    result = true;

    // test 1: NULL does not crash
    d_registry_rebuild_lookup(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "rebuild_null",
                              true,
                              "NULL registry rebuild should not crash") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "rebuild_alias_before",
                                  d_registry_get(reg, "a") != NULL,
                                  "Alias 'a' should exist before rebuild") && result;

        // test 2: rebuild
        d_registry_rebuild_lookup(reg);

        // test 3: lookup_count == count
        result = D_TEST_CHECK_MSG(_suite,
                                  "rebuild_lookup_count",
                                  reg->lookup_count == d_registry_count(reg),
                                  "lookup_count should equal row count after rebuild") && result;

        // test 4: canonical keys findable
        result = D_TEST_CHECK_MSG(_suite,
                                  "rebuild_canon_ok",
                                  d_registry_get(reg, "alpha") != NULL && d_registry_get(reg, "beta") != NULL,
                                  "Canonical keys should be findable after rebuild") && result;

        // test 5: aliases dropped
        result = D_TEST_CHECK_MSG(_suite,
                                  "rebuild_alias_dropped",
                                  d_registry_get(reg, "a") == NULL,
                                  "Alias 'a' should be dropped after rebuild") && result;

        // test 6: empty registry
        d_registry_clear(reg);
        d_registry_rebuild_lookup(reg);

        result = D_TEST_CHECK_MSG(_suite,
                                  "rebuild_empty",
                                  reg->lookup_count == 0,
                                  "Empty registry rebuild should set lookup_count to 0") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_sort_lookup
  Tests the d_registry_sort_lookup function.
  Tests the following:
  - NULL registry does not crash
  - single-entry or empty lookup does not crash
  - after sort, entries are in ascending key order
  - case-insensitive sort uses nocase comparison
*/
bool d_tests_registry_sort_lookup(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    size_t             i;
    bool               sorted;

    d_test_suite_test(_suite, "registry_sort_lookup");

    result = true;

    // test 1: NULL does not crash
    d_registry_sort_lookup(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "sort_null",
                              true,
                              "NULL registry sort should not crash") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty sort
        d_registry_sort_lookup(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_empty",
                                  true,
                                  "Empty registry sort should not crash") && result;

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

        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_ascending",
                                  sorted == true,
                                  "Lookup entries should be in ascending key order") && result;

        // test 4: case-insensitive sort
        reg->flags |= (uint8_t)D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        // just verify it completes without crash and keys findable
        result = D_TEST_CHECK_MSG(_suite,
                                  "sort_nocase",
                                  d_registry_get(reg, "APPLE") != NULL,
                                  "Case-insensitive sort should enable nocase lookup") && result;

        reg->flags &= (uint8_t)~D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_lookup_maint_all
  Aggregation function that runs all lookup maintenance tests.
*/
bool d_tests_registry_lookup_maint_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_lookup_maint_all");

    result = true;

    result = d_tests_registry_rebuild_lookup(_suite) && result;
    result = d_tests_registry_sort_lookup(_suite)    && result;

    return result;
}
