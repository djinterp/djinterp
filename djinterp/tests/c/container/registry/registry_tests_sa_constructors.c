#include ".\registry_tests_sa.h"


/*
d_tests_sa_registry_new
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
bool
d_tests_sa_registry_new
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;

    result = true;

    // test 1: zero row_size
    reg = d_registry_new(0);
    result = d_assert_standalone(
        reg == NULL,
        "new_zero_rowsize",
        "Zero row_size should return NULL",
        _counter) && result;

    // test 2: valid row_size
    reg = d_registry_new(sizeof(struct test_row));
    result = d_assert_standalone(
        reg != NULL,
        "new_valid",
        "Valid row_size should return non-NULL",
        _counter) && result;

    if (reg)
    {
        // test 3: zero count
        result = d_assert_standalone(
            d_registry_count(reg) == 0,
            "new_count_zero",
            "New registry should have zero count",
            _counter) && result;

        // test 4: default capacity
        result = d_assert_standalone(
            d_registry_capacity(reg) >= D_REGISTRY_DEFAULT_CAPACITY,
            "new_default_cap",
            "Capacity should be >= D_REGISTRY_DEFAULT_CAPACITY",
            _counter) && result;

        // test 5: is empty
        result = d_assert_standalone(
            d_registry_is_empty(reg) == true,
            "new_is_empty",
            "New registry should be empty",
            _counter) && result;

        // test 6: flags
        result = d_assert_standalone(
            reg->flags == (uint8_t)D_REGISTRY_FLAG_DEFAULT,
            "new_flags_default",
            "Flags should be D_REGISTRY_FLAG_DEFAULT",
            _counter) && result;

        // test 7: row_size stored
        result = d_assert_standalone(
            reg->row_size == sizeof(struct test_row),
            "new_row_size",
            "row_size should equal sizeof(struct test_row)",
            _counter) && result;

        // test 8: row_free NULL
        result = d_assert_standalone(
            reg->row_free == NULL,
            "new_row_free_null",
            "row_free should be NULL on new registry",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_new_with_capacity
  Tests the d_registry_new_with_capacity function.
  Tests the following:
  - zero row_size returns NULL
  - zero capacity returns valid empty registry with no pre-allocation
  - custom capacity is met or exceeded
  - large capacity allocates successfully
  - row and lookup capacity are both reserved
*/
bool
d_tests_sa_registry_new_with_capacity
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;

    result = true;

    // test 1: zero row_size
    reg = d_registry_new_with_capacity(0, 64);
    result = d_assert_standalone(
        reg == NULL,
        "new_cap_zero_rowsize",
        "Zero row_size should return NULL",
        _counter) && result;

    // test 2: zero capacity
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 0);
    result = d_assert_standalone(
        reg != NULL,
        "new_cap_zero_cap",
        "Zero capacity should return valid registry",
        _counter) && result;

    if (reg)
    {
        result = d_assert_standalone(
            d_registry_count(reg) == 0,
            "new_cap_zero_count",
            "Zero-capacity registry should have zero count",
            _counter) && result;

        d_registry_free(reg);
    }

    // test 3: custom capacity
    reg = d_registry_new_with_capacity(sizeof(struct test_row), 128);
    result = d_assert_standalone(
        reg != NULL,
        "new_cap_custom",
        "Custom capacity 128 should succeed",
        _counter) && result;

    if (reg)
    {
        // test 4: capacity met
        result = d_assert_standalone(
            d_registry_capacity(reg) >= 128,
            "new_cap_custom_value",
            "Row capacity should be >= 128",
            _counter) && result;

        // test 5: lookup capacity also reserved
        result = d_assert_standalone(
            reg->lookup_capacity >= 128,
            "new_cap_lookup",
            "Lookup capacity should also be >= 128",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_new_copy
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
bool
d_tests_sa_registry_new_copy
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* src;
    struct d_registry* cpy;
    struct test_row    row;
    struct test_row*   found;

    result = true;

    // test 1: NULL source
    result = d_assert_standalone(
        d_registry_new_copy(NULL) == NULL,
        "copy_null",
        "NULL source should return NULL",
        _counter) && result;

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
        result = d_assert_standalone(
            cpy != NULL,
            "copy_success",
            "Copy should succeed",
            _counter) && result;

        if (cpy)
        {
            // test 3: count matches
            result = d_assert_standalone(
                d_registry_count(cpy) == 3,
                "copy_count",
                "Copy count should be 3",
                _counter) && result;

            // test 4: data accessible by key
            found = (struct test_row*)d_registry_get(cpy, "beta");
            result = d_assert_standalone(
                found != NULL && found->value == 20,
                "copy_data_beta",
                "Copy should contain 'beta' with value 20",
                _counter) && result;

            // test 5: copy is independent
            row.key   = "delta";
            row.value = 40;
            d_registry_add(cpy, &row);

            result = d_assert_standalone(
                d_registry_count(cpy) == 4
                && d_registry_count(src) == 3,
                "copy_independent",
                "Adding to copy should not affect source",
                _counter) && result;

            // test 6: STATIC_ROWS flag cleared
            result = d_assert_standalone(
                (cpy->flags & D_REGISTRY_FLAG_STATIC_ROWS) == 0,
                "copy_not_static",
                "Copy should not have STATIC_ROWS flag",
                _counter) && result;

            // test 7: CASE_INSENSITIVE flag preserved
            result = d_assert_standalone(
                (cpy->flags & D_REGISTRY_FLAG_CASE_INSENSITIVE) != 0,
                "copy_flags_preserved",
                "CASE_INSENSITIVE flag should be preserved in copy",
                _counter) && result;

            d_registry_free(cpy);
        }

        d_registry_free(src);
    }

    return result;
}

/*
d_tests_sa_registry_new_from_array
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
bool
d_tests_sa_registry_new_from_array
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    rows[3];
    struct test_row*   found;

    result = true;

    rows[0].key = "cherry";   rows[0].value = 1;
    rows[1].key = "apple";    rows[1].value = 2;
    rows[2].key = "banana";   rows[2].value = 3;

    // test 1: NULL rows
    result = d_assert_standalone(
        d_registry_new_from_array(NULL,
            sizeof(struct test_row), 3) == NULL,
        "from_arr_null_rows",
        "NULL rows should return NULL",
        _counter) && result;

    // test 2: zero row_size
    result = d_assert_standalone(
        d_registry_new_from_array(rows, 0, 3) == NULL,
        "from_arr_zero_rowsize",
        "Zero row_size should return NULL",
        _counter) && result;

    // test 3: valid array
    reg = d_registry_new_from_array(rows,
                                     sizeof(struct test_row), 3);
    result = d_assert_standalone(
        reg != NULL,
        "from_arr_valid",
        "Valid array should return non-NULL",
        _counter) && result;

    if (reg)
    {
        // test 4: count
        result = d_assert_standalone(
            d_registry_count(reg) == 3,
            "from_arr_count",
            "Count should be 3",
            _counter) && result;

        // test 5: lookup built and sorted (all keys findable)
        found = (struct test_row*)d_registry_get(reg, "apple");
        result = d_assert_standalone(
            found != NULL && found->value == 2,
            "from_arr_lookup_apple",
            "'apple' should be found with value 2",
            _counter) && result;

        found = (struct test_row*)d_registry_get(reg, "banana");
        result = d_assert_standalone(
            found != NULL && found->value == 3,
            "from_arr_lookup_banana",
            "'banana' should be found with value 3",
            _counter) && result;

        found = (struct test_row*)d_registry_get(reg, "cherry");
        result = d_assert_standalone(
            found != NULL && found->value == 1,
            "from_arr_lookup_cherry",
            "'cherry' should be found with value 1",
            _counter) && result;

        // test 6: non-existent key
        result = d_assert_standalone(
            d_registry_get(reg, "grape") == NULL,
            "from_arr_not_found",
            "Non-existent key should return NULL",
            _counter) && result;

        d_registry_free(reg);
    }

    return result;
}

/*
d_tests_sa_registry_constructors_all
  Aggregation function that runs all constructor tests.
*/
bool
d_tests_sa_registry_constructors_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_registry_new(_counter) && result;
    result = d_tests_sa_registry_new_with_capacity(_counter) && result;
    result = d_tests_sa_registry_new_copy(_counter) && result;
    result = d_tests_sa_registry_new_from_array(_counter) && result;

    return result;
}
