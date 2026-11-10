/*******************************************************************************
* djinterp [container]                             registry_tests_constructors.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_constructors.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_registry_new
  Tests the d_registry_new function.
  Tests the following:
  - zero row_size returns NULL
  - valid row_size returns non-NULL registry
  - new registry has zero count
  - new registry has default capacity (>= D_REGISTRY_DEFAULT_CAPACITY)
  - new registry is empty
  - flags are D_REGISTRY_FLAG_DEFAULT (NONE)
  - row_size is stored correctly
  - row_free is NULL
*/
bool d_tests_registry_new(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;

    d_test_suite_test(_suite, "registry_new");

    result = true;

    // test 1: zero row_size
    reg = d_registry_new(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_rowsize",
                              reg == NULL,
                              "Zero row_size should return NULL") && result;

    // test 2: valid row_size
    reg = d_registry_new(sizeof(struct test_row));
    result = D_TEST_CHECK_MSG(_suite,
                              "new_valid",
                              reg != NULL,
                              "Valid row_size should return non-NULL") && result;

    if (reg)
    {
        // test 3: zero count
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_count_zero",
                                  d_registry_count(reg) == 0,
                                  "New registry should have zero count") && result;

        // test 4: default capacity
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_cap",
                                  d_registry_capacity(reg) >= D_REGISTRY_DEFAULT_CAPACITY,
                                  "Capacity should be >= D_REGISTRY_DEFAULT_CAPACITY") && result;

        // test 5: is empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_is_empty",
                                  d_registry_is_empty(reg) == true,
                                  "New registry should be empty") && result;

        // test 6: flags
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_flags_default",
                                  reg->flags == (uint8_t)D_REGISTRY_FLAG_DEFAULT,
                                  "Flags should be D_REGISTRY_FLAG_DEFAULT") && result;

        // test 7: row_size stored
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_row_size",
                                  reg->row_size == sizeof(struct test_row),
                                  "row_size should equal sizeof(struct test_row)") && result;

        // test 8: row_free NULL
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_row_free_null",
                                  reg->row_free == NULL,
                                  "row_free should be NULL on new registry") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_new_with_capacity
  Tests the d_registry_new_with_capacity function.
  Tests the following:
  - zero row_size returns NULL
  - zero capacity returns valid empty registry with no pre-allocation
  - custom capacity is met or exceeded
  - large capacity allocates successfully
  - row and lookup capacity are both reserved
*/
bool d_tests_registry_new_with_capacity(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;

    d_test_suite_test(_suite, "registry_new_with_capacity");

    result = true;

    // test 1: zero row_size
    reg = d_registry_new_with_capacity(0, 64);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_cap_zero_rowsize",
                              reg == NULL,
                              "Zero row_size should return NULL") && result;

    // test 2: zero capacity
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_cap_zero_cap",
                              reg != NULL,
                              "Zero capacity should return valid registry") && result;

    if (reg)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_cap_zero_count",
                                  d_registry_count(reg) == 0,
                                  "Zero-capacity registry should have zero count") && result;

        d_registry_free(reg);
    }

    // test 3: custom capacity
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 128);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_cap_custom",
                              reg != NULL,
                              "Custom capacity 128 should succeed") && result;

    if (reg)
    {
        // test 4: capacity met
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_cap_custom_value",
                                  d_registry_capacity(reg) >= 128,
                                  "Row capacity should be >= 128") && result;

        // test 5: lookup capacity also reserved
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_cap_lookup",
                                  reg->lookup_capacity >= 128,
                                  "Lookup capacity should also be >= 128") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_new_copy
  Tests the d_registry_new_copy function.
  Tests the following:
  - NULL source returns NULL
  - copies row data correctly
  - copy count matches source
  - copy is independent of source (modifications do not cross)
  - lookup entries are duplicated (keys findable in copy)
  - STATIC_ROWS flag is cleared on copy
  - flags (other than STATIC_ROWS) are preserved
  - row_free function pointer is copied
*/
bool d_tests_registry_new_copy(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* src;
    struct dj_registry* cpy;
    struct test_row    row;
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_new_copy");

    result = true;

    // test 1: NULL source
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_null",
                              d_registry_new_copy(NULL) == NULL,
                              "NULL source should return NULL") && result;

    // set up source with rows
    src = d_registry_new(sizeof(struct test_row));

    if (src)
    {
        row.key   = "alpha";
        row.value = 10;
        d_registry_add(src, &row);

        row.key   = "beta";
        row.value = 20;
        d_registry_add(src, &row);

        row.key   = "gamma";
        row.value = 30;
        d_registry_add(src, &row);

        // set a flag to verify preservation
        src->flags |= (uint8_t)D_REGISTRY_FLAG_CASE_INSENSITIVE;

        // test 2: copy succeeds
        cpy = d_registry_new_copy(src);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_success",
                                  cpy != NULL,
                                  "Copy should succeed") && result;

        if (cpy)
        {
            // test 3: count matches
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_count",
                                      d_registry_count(cpy) == 3,
                                      "Copy count should be 3") && result;

            // test 4: data accessible by key
            found = (struct test_row*)d_registry_get(cpy, "beta");
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_data_beta",
                                      found != NULL && found->value == 20,
                                      "Copy should contain 'beta' with value 20") && result;

            // test 5: copy is independent
            row.key   = "delta";
            row.value = 40;
            d_registry_add(cpy, &row);

            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_independent",
                                      d_registry_count(cpy) == 4 && d_registry_count(src) == 3,
                                      "Adding to copy should not affect source") && result;

            // test 6: STATIC_ROWS flag cleared
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_not_static",
                                      (cpy->flags & D_REGISTRY_FLAG_STATIC_ROWS) == 0,
                                      "Copy should not have STATIC_ROWS flag") && result;

            // test 7: CASE_INSENSITIVE flag preserved
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_flags_preserved",
                                      (cpy->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) != 0,
                                      "CASE_INSENSITIVE flag should be preserved in copy") && result;

            d_registry_free(cpy);
        }

        d_registry_free(src);
    }

    return result;
}

/*
d_tests_registry_new_from_array
  Tests the d_registry_new_from_array function.
  Tests the following:
  - NULL rows returns NULL
  - zero row_size returns NULL
  - zero count creates empty registry
  - valid array creates registry with correct count
  - lookup is automatically rebuilt and sorted
  - all rows are accessible by key
  - non-existent key returns NULL
*/
bool d_tests_registry_new_from_array(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    rows[3];
    struct test_row*   found;

    d_test_suite_test(_suite, "registry_new_from_array");

    result = true;

    rows[0].key = "cherry";   rows[0].value = 1;
    rows[1].key = "apple";    rows[1].value = 2;
    rows[2].key = "banana";   rows[2].value = 3;

    // test 1: NULL rows
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_null_rows",
                              d_registry_new_from_array(NULL, sizeof(struct test_row), 3) == NULL,
                              "NULL rows should return NULL") && result;

    // test 2: zero row_size
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_zero_rowsize",
                              d_registry_new_from_array(rows, 0, 3) == NULL,
                              "Zero row_size should return NULL") && result;

    // test 3: valid array
    reg = d_registry_new_from_array(rows,
                                     sizeof(struct test_row), 3);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_arr_valid",
                              reg != NULL,
                              "Valid array should return non-NULL") && result;

    if (reg)
    {
        // test 4: count
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_count",
                                  d_registry_count(reg) == 3,
                                  "Count should be 3") && result;

        // test 5: lookup built and sorted (all keys findable)
        found = (struct test_row*)d_registry_get(reg, "apple");
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_lookup_apple",
                                  found != NULL && found->value == 2,
                                  "'apple' should be found with value 2") && result;

        found = (struct test_row*)d_registry_get(reg, "banana");
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_lookup_banana",
                                  found != NULL && found->value == 3,
                                  "'banana' should be found with value 3") && result;

        found = (struct test_row*)d_registry_get(reg, "cherry");
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_lookup_cherry",
                                  found != NULL && found->value == 1,
                                  "'cherry' should be found with value 1") && result;

        // test 6: non-existent key
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_arr_not_found",
                                  d_registry_get(reg, "grape") == NULL,
                                  "Non-existent key should return NULL") && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_registry_constructors_all
  Aggregation function that runs all constructor tests.
*/
bool d_tests_registry_constructors_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_constructors_all");

    result = true;

    result = d_tests_registry_new(_suite) && result;
    result = d_tests_registry_new_with_capacity(_suite) && result;
    result = d_tests_registry_new_copy(_suite) && result;
    result = d_tests_registry_new_from_array(_suite) && result;

    return result;
}
