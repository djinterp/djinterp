/*******************************************************************************
* djinterp [container]                         buffer_common_tests_destruction.c
*
*
* path:      /tests/djinterp/c/container/buffer/buffer_common_tests_destruction.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./buffer_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// test_free_fn
//   internal: custom free function for deep free tests.
static
void test_free_fn(
    void* _ptr
)
{
    free(_ptr);

    return;
}


/*
d_tests_buffer_common_free_data
  Tests the d_buffer_common_free_data function.
  Tests the following:
  - NULL elements does not crash
  - valid allocation is freed without error
*/
bool d_tests_buffer_common_free_data(
    struct dj_test_suite* _suite
)
{
    bool  result;
    void* elements;

    d_test_suite_test(_suite, "buffer_common_free_data");

    result = true;

    // test 1: NULL should not crash
    d_buffer_common_free_data(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_data_null",
                              true,
                              "NULL free should not crash") && result;

    // test 2: valid allocation freed
    elements = d_buffer_common_alloc(sizeof(int), 16);

    if (elements)
    {
        d_buffer_common_free_data(elements);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_data_valid",
                                  true,
                                  "Valid allocation should be freed") && result;
    }

    return result;
}


/*
d_tests_buffer_common_free_data_deep
  Tests the d_buffer_common_free_data_deep function.
  Tests the following:
  - NULL elements does not crash
  - NULL free function does not crash
  - valid pointer array with elements is freed
  - NULL elements within array are skipped
*/
bool d_tests_buffer_common_free_data_deep(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void** ptrs;

    d_test_suite_test(_suite, "buffer_common_free_data_deep");

    result = true;

    // test 1: NULL elements should not crash
    d_buffer_common_free_data_deep(3, NULL, test_free_fn);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_data_deep_null_elements",
                              true,
                              "NULL elements should not crash") && result;

    // test 2: NULL free function should not crash
    ptrs = malloc(2 * sizeof(void*));

    if (ptrs)
    {
        ptrs[0] = malloc(32);
        ptrs[1] = malloc(32);

        d_buffer_common_free_data_deep(2, ptrs, NULL);

        // with no free function nothing was freed: clean up here
        free(ptrs[0]);
        free(ptrs[1]);
        free(ptrs);
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "free_data_deep_null_fn",
                              true,
                              "NULL free function should not crash") && result;

    // test 3: valid deep free with some NULL elements
    ptrs = malloc(3 * sizeof(void*));

    if (ptrs)
    {
        ptrs[0] = malloc(64);
        ptrs[1] = NULL;
        ptrs[2] = malloc(64);

        d_buffer_common_free_data_deep(3, ptrs, test_free_fn);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_data_deep_with_nulls",
                                  true,
                                  "Deep free with NULL entries should succeed") && result;
    }

    return result;
}


/*
d_tests_buffer_common_destruction_all
  Aggregation function that runs all destruction tests.
*/
bool d_tests_buffer_common_destruction_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "buffer_common_destruction_all");

    result = true;

    result = d_tests_buffer_common_free_data(_suite) && result;
    result = d_tests_buffer_common_free_data_deep(_suite) && result;

    return result;
}
