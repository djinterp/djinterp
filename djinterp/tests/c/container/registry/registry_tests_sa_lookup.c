#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_get
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
bool
d_tests_sa_registry_get
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
        d_registry_get(NULL, "key") == NULL,
        "get_null_reg",
        "NULL registry should return NULL",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = d_assert_standalone(
            d_registry_get(reg, NULL) == NULL,
            "get_null_key",
            "NULL key should return NULL",
            _counter) && result;

        // test 3: empty registry
        result = d_assert_standalone(
            d_registry_get(reg, "missing") == NULL,
            "get_empty",
            "Empty registry lookup should return NULL",
            _counter) && result;

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
        result = d_assert_standalone(
            found != NULL && found->value == 100,
            "get_canonical",
            "'alpha' should be found with value 100",
            _counter) && result;

        // test 5: not found
        result = d_assert_standalone(
            d_registry_get(reg, "omega") == NULL,
            "get_not_found",
            "'omega' should not be found",
            _counter) && result;

        // test 6: find by alias
        d_registry_add_alias(reg, "beta", "b");
        found = (struct test_row*)d_registry_get(reg, "b");
        result = d_assert_standalone(
            found != NULL && found->value == 200,
            "get_alias",
            "Alias 'b' should resolve to beta (value 200)",
            _counter) && result;

        // test 7: case-insensitive mode
        reg->flags |= (uint8_t)D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        found = (struct test_row*)d_registry_get(reg, "GAMMA");
        result = d_assert_standalone(
            found != NULL && found->value == 300,
            "get_nocase",
            "Case-insensitive 'GAMMA' should find 'gamma'",
            _counter) && result;

        // test 8: case-sensitive rejects wrong case
        reg->flags &= (uint8_t)~D_REGISTRY_FLAG_CASE_INSENSITIVE;
        d_registry_sort_lookup(reg);

        result = d_assert_standalone(
            d_registry_get(reg, "GAMMA") == NULL,
            "get_case_sensitive",
            "Case-sensitive 'GAMMA' should not find 'gamma'",
            _counter) && result;

        // test 9: returned pointer is mutable
        found = (struct test_row*)d_registry_get(reg, "alpha");

        if (found)
        {
            found->value = 999;
            found = (struct test_row*)d_registry_get(reg, "alpha");

            result = d_assert_standalone(
                found != NULL && found->value == 999,
                "get_mutable",
                "Returned pointer should allow mutation",
                _counter) && result;
        }

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_lookup_all
  Aggregation function that runs all primary lookup tests.
*/
bool
d_tests_sa_registry_lookup_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Primary Lookup Function\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_registry_get(_counter) && result;

    return result;
}
