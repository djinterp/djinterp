#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_contains
  Tests the d_registry_contains function.
  Tests the following:
  - NULL registry returns false
  - NULL key returns false
  - empty registry returns false
  - finds existing key
  - does not find non-existent key
  - finds via alias
*/
bool
d_tests_sa_registry_contains
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
        d_registry_contains(NULL, "key") == false,
        "contains_null_reg",
        "NULL registry should return false",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = d_assert_standalone(
            d_registry_contains(reg, NULL) == false,
            "contains_null_key",
            "NULL key should return false",
            _counter) && result;

        // test 3: empty
        result = d_assert_standalone(
            d_registry_contains(reg, "alpha") == false,
            "contains_empty",
            "Empty registry should return false",
            _counter) && result;

        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);

        // test 4: found
        result = d_assert_standalone(
            d_registry_contains(reg, "alpha") == true,
            "contains_found",
            "'alpha' should be found",
            _counter) && result;

        // test 5: not found
        result = d_assert_standalone(
            d_registry_contains(reg, "beta") == false,
            "contains_not_found",
            "'beta' should not be found",
            _counter) && result;

        // test 6: via alias
        d_registry_add_alias(reg, "alpha", "a");
        result = d_assert_standalone(
            d_registry_contains(reg, "a") == true,
            "contains_alias",
            "Alias 'a' should be found",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_index_of
  Tests the d_registry_index_of function.
  Tests the following:
  - NULL registry returns -1
  - NULL key returns -1
  - key not found returns -1
  - returns correct row index for existing key
  - returns correct index via alias (same row)
*/
bool
d_tests_sa_registry_index_of
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;
    ssize_t            idx;

    result = true;

    // test 1: NULL registry
    result = d_assert_standalone(
        d_registry_index_of(NULL, "key") == (ssize_t)-1,
        "indexof_null_reg",
        "NULL registry should return -1",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = d_assert_standalone(
            d_registry_index_of(reg, NULL) == (ssize_t)-1,
            "indexof_null_key",
            "NULL key should return -1",
            _counter) && result;

        // test 3: not found
        result = d_assert_standalone(
            d_registry_index_of(reg, "x") == (ssize_t)-1,
            "indexof_not_found",
            "Non-existent key should return -1",
            _counter) && result;

        // add rows
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);
        row.key = "gamma";  row.value = 3;
        d_registry_add(reg, &row);

        // test 4: correct index
        idx = d_registry_index_of(reg, "alpha");
        result = d_assert_standalone(
            idx == 0,
            "indexof_alpha",
            "'alpha' should be at index 0",
            _counter) && result;

        idx = d_registry_index_of(reg, "gamma");
        result = d_assert_standalone(
            idx == 2,
            "indexof_gamma",
            "'gamma' should be at index 2",
            _counter) && result;

        // test 5: alias returns same row index
        d_registry_add_alias(reg, "beta", "b");
        idx = d_registry_index_of(reg, "b");
        result = d_assert_standalone(
            idx == d_registry_index_of(reg, "beta"),
            "indexof_alias",
            "Alias 'b' should return same index as 'beta'",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_at
  Tests the d_registry_at function.
  Tests the following:
  - NULL registry returns NULL
  - index out of range returns NULL
  - valid index returns correct row pointer
  - returned pointer allows reading row data
*/
bool
d_tests_sa_registry_at
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
        d_registry_at(NULL, 0) == NULL,
        "at_null_reg",
        "NULL registry should return NULL",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: out of range on empty
        result = d_assert_standalone(
            d_registry_at(reg, 0) == NULL,
            "at_empty",
            "Index 0 on empty registry should return NULL",
            _counter) && result;

        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 20;
        d_registry_add(reg, &row);

        // test 3: out of range
        result = d_assert_standalone(
            d_registry_at(reg, 5) == NULL,
            "at_oob",
            "Index 5 should return NULL (only 2 rows)",
            _counter) && result;

        // test 4: valid index
        found = (struct test_row*)d_registry_at(reg, 0);
        result = d_assert_standalone(
            found != NULL
            && d_strcmp_n(found->key, strlen(found->key), "alpha", 5) == 0
            && found->value == 10,
            "at_valid",
            "Index 0 should be 'alpha' with value 10",
            _counter) && result;

        found = (struct test_row*)d_registry_at(reg, 1);
        result = d_assert_standalone(
            found != NULL
            && d_strcmp_n(found->key, strlen(found->key), "beta", 4) == 0
            && found->value == 20,
            "at_valid_1",
            "Index 1 should be 'beta' with value 20",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_count
  Tests the d_registry_count function.
  Tests the following:
  - NULL registry returns 0
  - empty registry returns 0
  - count reflects number of rows added
  - count decrements after remove
*/
bool
d_tests_sa_registry_count
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
        d_registry_count(NULL) == 0,
        "count_null",
        "NULL registry should return 0",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty
        result = d_assert_standalone(
            d_registry_count(reg) == 0,
            "count_empty",
            "Empty registry should return 0",
            _counter) && result;

        // test 3: after adds
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);

        result = d_assert_standalone(
            d_registry_count(reg) == 2,
            "count_two",
            "Count should be 2 after adding 2 rows",
            _counter) && result;

        // test 4: after remove
        d_registry_remove(reg, "a");
        result = d_assert_standalone(
            d_registry_count(reg) == 1,
            "count_after_remove",
            "Count should be 1 after removing one",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_capacity_fn
  Tests the d_registry_capacity function.
  Tests the following:
  - NULL registry returns 0
  - reflects initial allocation
  - capacity >= count invariant
*/
bool
d_tests_sa_registry_capacity_fn
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;

    result = true;

    // test 1: NULL
    result = d_assert_standalone(
        d_registry_capacity(NULL) == 0,
        "cap_null",
        "NULL registry should return 0",
        _counter) && result;

    // test 2: reflects initial
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 64);

    if (reg)
    {
        result = d_assert_standalone(
            d_registry_capacity(reg) >= 64,
            "cap_initial",
            "Capacity should be >= 64",
            _counter) && result;

        // test 3: capacity >= count invariant
        result = d_assert_standalone(
            d_registry_capacity(reg) >= d_registry_count(reg),
            "cap_ge_count",
            "Capacity should always be >= count",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_is_empty
  Tests the d_registry_is_empty function.
  Tests the following:
  - NULL registry returns true
  - new registry is empty
  - after add, not empty
  - after clear, empty again
*/
bool
d_tests_sa_registry_is_empty
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
        d_registry_is_empty(NULL) == true,
        "empty_null",
        "NULL registry should return true",
        _counter) && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: new
        result = d_assert_standalone(
            d_registry_is_empty(reg) == true,
            "empty_new",
            "New registry should be empty",
            _counter) && result;

        // test 3: after add
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);

        result = d_assert_standalone(
            d_registry_is_empty(reg) == false,
            "empty_after_add",
            "Registry with rows should not be empty",
            _counter) && result;

        // test 4: after clear
        d_registry_clear(reg);
        result = d_assert_standalone(
            d_registry_is_empty(reg) == true,
            "empty_after_clear",
            "Cleared registry should be empty",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_queries_all
  Aggregation function that runs all query tests.
*/
bool
d_tests_sa_registry_queries_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Query Functions\n");
    printf("  --------------------------\n");

    result = d_tests_sa_registry_contains(_counter) && result;
    result = d_tests_sa_registry_index_of(_counter) && result;
    result = d_tests_sa_registry_at(_counter) && result;
    result = d_tests_sa_registry_count(_counter) && result;
    result = d_tests_sa_registry_capacity_fn(_counter) && result;
    result = d_tests_sa_registry_is_empty(_counter) && result;

    return result;
}
