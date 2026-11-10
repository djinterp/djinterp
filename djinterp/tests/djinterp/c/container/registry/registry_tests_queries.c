/*******************************************************************************
* djinterp [container]                                  registry_tests_queries.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_queries.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_contains
  Tests the d_registry_contains function.
  Tests the following:
  - NULL registry returns false
  - NULL key returns false
  - empty registry returns false
  - finds existing key
  - does not find non-existent key
  - finds via alias
*/
bool d_tests_registry_contains(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_contains");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "contains_null_reg",
                              d_registry_contains(NULL, "key") == false,
                              "NULL registry should return false") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_null_key",
                                  d_registry_contains(reg, NULL) == false,
                                  "NULL key should return false") && result;

        // test 3: empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_empty",
                                  d_registry_contains(reg, "alpha") == false,
                                  "Empty registry should return false") && result;

        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);

        // test 4: found
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_found",
                                  d_registry_contains(reg, "alpha") == true,
                                  "'alpha' should be found") && result;

        // test 5: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_not_found",
                                  d_registry_contains(reg, "beta") == false,
                                  "'beta' should not be found") && result;

        // test 6: via alias
        d_registry_add_alias(reg, "alpha", "a");
        result = D_TEST_CHECK_MSG(_suite,
                                  "contains_alias",
                                  d_registry_contains(reg, "a") == true,
                                  "Alias 'a' should be found") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_index_of
  Tests the d_registry_index_of function.
  Tests the following:
  - NULL registry returns -1
  - NULL key returns -1
  - key not found returns -1
  - returns correct row index for existing key
  - returns correct index via alias (same row)
*/
bool d_tests_registry_index_of(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    ssize_t            idx;

    d_test_suite_test(_suite, "registry_index_of");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "indexof_null_reg",
                              d_registry_index_of(NULL, "key") == (ssize_t)-1,
                              "NULL registry should return -1") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: NULL key
        result = D_TEST_CHECK_MSG(_suite,
                                  "indexof_null_key",
                                  d_registry_index_of(reg, NULL) == (ssize_t)-1,
                                  "NULL key should return -1") && result;

        // test 3: not found
        result = D_TEST_CHECK_MSG(_suite,
                                  "indexof_not_found",
                                  d_registry_index_of(reg, "x") == (ssize_t)-1,
                                  "Non-existent key should return -1") && result;

        // add rows
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);
        row.key = "gamma";  row.value = 3;
        d_registry_add(reg, &row);

        // test 4: correct index
        idx = d_registry_index_of(reg, "alpha");
        result = D_TEST_CHECK_MSG(_suite,
                                  "indexof_alpha",
                                  idx == 0,
                                  "'alpha' should be at index 0") && result;

        idx = d_registry_index_of(reg, "gamma");
        result = D_TEST_CHECK_MSG(_suite,
                                  "indexof_gamma",
                                  idx == 2,
                                  "'gamma' should be at index 2") && result;

        // test 5: alias returns same row index
        d_registry_add_alias(reg, "beta", "b");
        idx = d_registry_index_of(reg, "b");
        result = D_TEST_CHECK_MSG(_suite,
                                  "indexof_alias",
                                  idx == d_registry_index_of(reg, "beta"),
                                  "Alias 'b' should return same index as 'beta'") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_at
  Tests the d_registry_at function.
  Tests the following:
  - NULL registry returns NULL
  - index out of range returns NULL
  - valid index returns correct row pointer
  - returned pointer allows reading row data
*/
bool d_tests_registry_at(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_at");

    result = true;

    // test 1: NULL registry
    result = D_TEST_CHECK_MSG(_suite,
                              "at_null_reg",
                              d_registry_at(NULL, 0) == NULL,
                              "NULL registry should return NULL") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: out of range on empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_empty",
                                  d_registry_at(reg, 0) == NULL,
                                  "Index 0 on empty registry should return NULL") && result;

        row.key = "alpha";  row.value = 10;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 20;
        d_registry_add(reg, &row);

        // test 3: out of range
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_oob",
                                  d_registry_at(reg, 5) == NULL,
                                  "Index 5 should return NULL (only 2 rows)") && result;

        // test 4: valid index
        found = (struct test_row*)d_registry_at(reg, 0);
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_valid",
                                  found != NULL && d_strcmp_n(found->key, strlen(found->key), "alpha", 5) == 0 && found->value == 10,
                                  "Index 0 should be 'alpha' with value 10") && result;

        found = (struct test_row*)d_registry_at(reg, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "at_valid_1",
                                  found != NULL && d_strcmp_n(found->key, strlen(found->key), "beta", 4) == 0 && found->value == 20,
                                  "Index 1 should be 'beta' with value 20") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_count
  Tests the d_registry_count function.
  Tests the following:
  - NULL registry returns 0
  - empty registry returns 0
  - count reflects number of rows added
  - count decrements after remove
*/
bool d_tests_registry_count(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_count");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "count_null",
                              d_registry_count(NULL) == 0,
                              "NULL registry should return 0") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_empty",
                                  d_registry_count(reg) == 0,
                                  "Empty registry should return 0") && result;

        // test 3: after adds
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);

        result = D_TEST_CHECK_MSG(_suite,
                                  "count_two",
                                  d_registry_count(reg) == 2,
                                  "Count should be 2 after adding 2 rows") && result;

        // test 4: after remove
        d_registry_remove(reg, "a");
        result = D_TEST_CHECK_MSG(_suite,
                                  "count_after_remove",
                                  d_registry_count(reg) == 1,
                                  "Count should be 1 after removing one") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_capacity_fn
  Tests the d_registry_capacity function.
  Tests the following:
  - NULL registry returns 0
  - reflects initial allocation
  - capacity >= count invariant
*/
bool d_tests_registry_capacity_fn(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;

    d_test_suite_test(_suite, "registry_capacity_fn");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "cap_null",
                              d_registry_capacity(NULL) == 0,
                              "NULL registry should return 0") && result;

    // test 2: reflects initial
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 64);

    if (reg)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "cap_initial",
                                  d_registry_capacity(reg) >= 64,
                                  "Capacity should be >= 64") && result;

        // test 3: capacity >= count invariant
        result = D_TEST_CHECK_MSG(_suite,
                                  "cap_ge_count",
                                  d_registry_capacity(reg) >= d_registry_count(reg),
                                  "Capacity should always be >= count") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_is_empty
  Tests the d_registry_is_empty function.
  Tests the following:
  - NULL registry returns true
  - new registry is empty
  - after add, not empty
  - after clear, empty again
*/
bool d_tests_registry_is_empty(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_is_empty");

    result = true;

    // test 1: NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "empty_null",
                              d_registry_is_empty(NULL) == true,
                              "NULL registry should return true") && result;

    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        // test 2: new
        result = D_TEST_CHECK_MSG(_suite,
                                  "empty_new",
                                  d_registry_is_empty(reg) == true,
                                  "New registry should be empty") && result;

        // test 3: after add
        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);

        result = D_TEST_CHECK_MSG(_suite,
                                  "empty_after_add",
                                  d_registry_is_empty(reg) == false,
                                  "Registry with rows should not be empty") && result;

        // test 4: after clear
        d_registry_clear(reg);
        result = D_TEST_CHECK_MSG(_suite,
                                  "empty_after_clear",
                                  d_registry_is_empty(reg) == true,
                                  "Cleared registry should be empty") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_queries_all
  Aggregation function that runs all query tests.
*/
bool d_tests_registry_queries_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_queries_all");

    result = true;

    result = d_tests_registry_contains(_suite) && result;
    result = d_tests_registry_index_of(_suite) && result;
    result = d_tests_registry_at(_suite) && result;
    result = d_tests_registry_count(_suite) && result;
    result = d_tests_registry_capacity_fn(_suite) && result;
    result = d_tests_registry_is_empty(_suite) && result;

    return result;
}
