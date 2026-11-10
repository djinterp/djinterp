/*******************************************************************************
* djinterp [container]                               registry_tests_destructor.c
*
*
* path:      /tests/djinterp/c/container/registry/registry_tests_destructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./registry_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * HELPER: row_free counter for OWNS_ROWS testing
 *****************************************************************************/

static int g_row_free_count = 0;

static
void test_row_free(
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
d_tests_registry_free
  Tests the d_registry_free function.
  Tests the following:
  - NULL registry does not crash
  - frees a new empty registry without error
  - frees a populated registry without error
  - frees a registry with aliases without error
  - calls row_free for each row when OWNS_ROWS flag is set
  - does not free rows/lookup for static registries (only the struct)
*/
bool d_tests_registry_free(
    struct dj_test_suite* _suite
)
{
    bool               result;
    struct dj_registry* reg;
    struct test_row    row;

    d_test_suite_test(_suite, "registry_free");

    result = true;

    // test 1: NULL does not crash
    d_registry_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_null",
                              true,
                              "NULL registry free should not crash") && result;

    // test 2: empty registry
    reg = d_registry_new(sizeof(struct test_row));
    d_registry_free(reg);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_empty",
                              true,
                              "Freeing empty registry should not crash") && result;

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
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_populated",
                                  true,
                                  "Freeing populated registry should not crash") && result;
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
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_with_aliases",
                                  true,
                                  "Freeing registry with aliases should not crash") && result;
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

        result = D_TEST_CHECK_MSG(_suite,
                                  "free_owns_rows",
                                  g_row_free_count == 3,
                                  "row_free should be called 3 times with OWNS_ROWS") && result;
    }

    return result;
}

/*
d_tests_registry_destructor_all
  Aggregation function that runs all destructor tests.
*/
bool d_tests_registry_destructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "registry_destructor_all");

    result = true;

    result = d_tests_registry_free(_suite) && result;

    return result;
}
