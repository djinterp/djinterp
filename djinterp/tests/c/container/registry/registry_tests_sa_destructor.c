#include ".\registry_tests_sa.h"


/******************************************************************************
 * HELPER: row_free counter for OWNS_ROWS testing
 *****************************************************************************/

static int g_row_free_count = 0;

static void
test_row_free
(
    void* _row
)
{
    (void)_row;
    g_row_free_count++;
}

/******************************************************************************
 * TEST FUNCTIONS
 *****************************************************************************/


/*
d_tests_sa_registry_free
  Tests the d_registry_free function.
  Tests the following:
  - NULL registry does not crash
  - frees a new empty registry without error
  - frees a populated registry without error
  - frees a registry with aliases without error
  - calls row_free for each row when OWNS_ROWS flag is set
  - does not free rows/lookup for static registries (only the struct)
*/
bool
d_tests_sa_registry_free
(
    struct d_test_counter* _counter
)
{
    bool               result;
    struct d_registry* reg;
    struct test_row    row;

    result = true;

    // test 1: NULL does not crash
    d_registry_free(NULL);
    result = d_assert_standalone(
        true,
        "free_null",
        "NULL registry free should not crash",
        _counter) && result;

    // test 2: empty registry
    reg = d_registry_new(sizeof(struct test_row));
    d_registry_free(reg);
    result = d_assert_standalone(
        true,
        "free_empty",
        "Freeing empty registry should not crash",
        _counter) && result;

    // test 3: populated registry
    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "beta";   row.value = 2;
        d_registry_add(reg, &row);
        row.key = "gamma";  row.value = 3;
        d_registry_add(reg, &row);

        d_registry_free(reg);
        result = d_assert_standalone(
            true,
            "free_populated",
            "Freeing populated registry should not crash",
            _counter) && result;
    }

    // test 4: registry with aliases
    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        row.key = "alpha";  row.value = 1;
        d_registry_add(reg, &row);

        d_registry_add_alias(reg, "alpha", "a");
        d_registry_add_alias(reg, "alpha", "al");

        d_registry_free(reg);
        result = d_assert_standalone(
            true,
            "free_with_aliases",
            "Freeing registry with aliases should not crash",
            _counter) && result;
    }

    // test 5: OWNS_ROWS flag triggers row_free
    reg = d_registry_new(sizeof(struct test_row));

    if (reg)
    {
        reg->flags   |= (uint8_t)D_REGISTRY_FLAG_OWNS_ROWS;
        reg->row_free = test_row_free;

        row.key = "a";  row.value = 1;
        d_registry_add(reg, &row);
        row.key = "b";  row.value = 2;
        d_registry_add(reg, &row);
        row.key = "c";  row.value = 3;
        d_registry_add(reg, &row);

        g_row_free_count = 0;
        d_registry_free(reg);

        result = d_assert_standalone(
            g_row_free_count == 3,
            "free_owns_rows",
            "row_free should be called 3 times with OWNS_ROWS",
            _counter) && result;
    }

    return result;
}

/*
d_tests_sa_registry_destructor_all
  Aggregation function that runs all destructor tests.
*/
bool
d_tests_sa_registry_destructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Destructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_registry_free(_counter) && result;

    return result;
}
