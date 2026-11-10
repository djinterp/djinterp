/*******************************************************************************
* djinterp [container]                                   registry_tests_lookup.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_lookup.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_get
  Tests the d_registry_get function.
  Tests the following:
  - NULL registry returns NULL
  - NULL key returns NULL
  - empty registry returns NULL
  - finds row by canonical key
  - not-found key returns NULL
  - finds row by alias after alias is added
  - case-insensitive flag affects lookup
  - case-sensitive mode does not match wrong case
  - returns pointer into the rows array (mutable access)
*/
bool d_tests_registry_get(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_get");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "get_null_reg",
                              d_registry_get(NULL, "key") == NULL,
                              "NULL registry should return NULL") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_null_key",
                                  d_registry_get(reg, NULL) == NULL,
                                  "NULL key should return NULL") && result;

        // test 3: empty registry
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_empty",
                                  d_registry_get(reg, "missing") == NULL,
                                  "Empty registry lookup should return NULL") && result;

        // add rows
        row.key   = "alpha";
        row.value = 100;
        d_registry_add(reg, &row);

        row.key   = "beta";
        row.value = 200;
        d_registry_add(reg, &row);

        row.key   = "gamma";
        row.value = 300;
        d_registry_add(reg, &row);

        // test 4: find by canonical key
        found = (struct test_row*)d_registry_get(reg, "alpha");
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_canonical",
                                  found != NULL && found->value == 100,
                                  "'alpha' should be found with value 100") && result;

        // test 5: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_not_found",
                                  d_registry_get(reg, "omega") == NULL,
                                  "'omega' should not be found") && result;

        // test 6: find by alias
        d_registry_add_alias(reg, "beta", "b");
        found = (struct test_row*)d_registry_get(reg, "b");
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_alias",
                                  found != NULL && found->value == 200,
                                  "Alias 'b' should resolve to beta (value 200)") && result;

        // test 7: case-insensitive mode
        reg->flags |= (uint8_t)D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        found = (struct test_row*)d_registry_get(reg, "GAMMA");
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_nocase",
                                  found != NULL && found->value == 300,
                                  "Case-insensitive 'GAMMA' should find 'gamma'") && result;

        // test 8: case-sensitive rejects wrong case
        reg->flags &= (uint8_t)~D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        result = D_TEST_CHECK_MSG(_suite,
                                  "get_case_sensitive",
                                  d_registry_get(reg, "GAMMA") == NULL,
                                  "Case-sensitive 'GAMMA' should not find 'gamma'") && result;

        // test 9: returned pointer is mutable
        found = (struct test_row*)d_registry_get(reg, "alpha");

        if (found)
        {
            found->value = 999;
            found = (struct test_row*)d_registry_get(reg, "alpha");

            result = D_TEST_CHECK_MSG(_suite,
                                      "get_mutable",
                                      found != NULL && found->value == 999,
                                      "Returned pointer should allow mutation") && result;
        }

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_lookup_all
  Aggregation function that runs all primary lookup tests.
*/
bool d_tests_registry_lookup_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_lookup_all");

    result = true;

    result = d_tests_registry_get(_suite) && result;

    return result;
}
