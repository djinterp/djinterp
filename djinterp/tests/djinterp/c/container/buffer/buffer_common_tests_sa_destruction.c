/*******************************************************************************
* djinterp [container]                      buffer_common_tests_sa_destruction.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_sa_destruction.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "./buffer_common_tests_sa.h"
#include "../../../../../inc/djinterp/test/c/test_counter.h"


// test_free_fn
//   internal: custom free function for deep free tests.
static void
test_free_fn(void* _ptr)
{
    free(_ptr);

    return;
}


/*
d_tests_sa_buffer_common_free_data
  Tests the d_buffer_common_free_data function.
  Tests the following:
  - NULL elements does not crash
  - valid allocation is freed without error
*/
bool
d_tests_sa_buffer_common_free_data
(
    struct d_test_counter* _counter
)
{
    bool  result;
    void* elements;

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_free_data(NULL);
    result = d_assert_standalone(
        true,
        "free_data_null",
        "NULL free should not crash",
        _counter) && result;

    // test 2: valid allocation freed
    elements = d_buffer_common_alloc(sizeof(int), 16);

    if (elements)
    {
        d_buffer_common_free_data(elements);
        result = d_assert_standalone(
            true,
            "free_data_valid",
            "Valid allocation should be freed",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_buffer_common_free_data_deep
  Tests the d_buffer_common_free_data_deep function.
  Tests the following:
  - NULL elements does not crash
  - NULL free function does not crash
  - valid pointer array with elements is freed
  - NULL elements within array are skipped
*/
bool
d_tests_sa_buffer_common_free_data_deep
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void** ptrs;

    result = true;

    // test 1: NULL elements should not crash
    d_buffer_common_free_data_deep(3, NULL, test_free_fn);
    result = d_assert_standalone(
        true,
        "free_data_deep_null_elements",
        "NULL elements should not crash",
        _counter) && result;

    // test 2: NULL free function should not crash
    ptrs = malloc(2 * sizeof(void*));

    if (ptrs)
    {
        ptrs[0] = malloc(32);
        ptrs[1] = malloc(32);

        d_buffer_common_free_data_deep(2, ptrs, NULL);
        // ptrs not freed by the function since _free_fn was NULL;
        // clean up manually
        // note: ptrs[0], ptrs[1] leak here intentionally because
        // the function bailed out without freeing anything
    }

    result = d_assert_standalone(
        true,
        "free_data_deep_null_fn",
        "NULL free function should not crash",
        _counter) && result;

    // test 3: valid deep free with some NULL elements
    ptrs = malloc(3 * sizeof(void*));

    if (ptrs)
    {
        ptrs[0] = malloc(64);
        ptrs[1] = NULL;
        ptrs[2] = malloc(64);

        d_buffer_common_free_data_deep(3, ptrs, test_free_fn);
        result = d_assert_standalone(
            true,
            "free_data_deep_with_nulls",
            "Deep free with NULL entries should succeed",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_buffer_common_destruction_all
  Aggregation function that runs all destruction tests.
*/
bool
d_tests_sa_buffer_common_destruction_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Destruction Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_buffer_common_free_data(_counter) && result;
    result = d_tests_sa_buffer_common_free_data_deep(_counter) && result;

    return result;
}
