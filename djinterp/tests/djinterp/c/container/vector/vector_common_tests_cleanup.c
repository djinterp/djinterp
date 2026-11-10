/*******************************************************************************
* djinterp [container]                             vector_common_tests_cleanup.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_common_tests_cleanup.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_common_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_common_free_elements
  Tests the d_vector_common_free_elements function for freeing memory.
  Tests the following:
  - NULL elements handling (should not crash)
  - successful free of allocated memory
*/
bool d_tests_vector_common_free_elements(
    struct dj_test_suite* _suite
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    d_test_suite_test(_suite, "vector_common_free_elements");

    result = true;

    // test 1: NULL elements should be handled gracefully (no crash)
    d_vector_common_free_elements(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_elements_null",
                              true,
                              "NULL elements should not crash") && result;

    // test 2: successful free of allocated memory
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    // populate with some data
    {
        int* arr = (int*)elements;
        size_t i;
        for (i = 0; i < 10; i++)
        {
            arr[i] = (int)(i * 10);
        }
        count = 10;
    }

    // free should not crash and memory should be released
    d_vector_common_free_elements(elements);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_elements_valid",
                              true,
                              "Valid elements should be freed without crash") && result;

    // Note: we cannot directly test that memory is freed, but we can verify
    // the function doesn't crash

    // test 3: free of small allocation
    elements = malloc(sizeof(int));
    d_vector_common_free_elements(elements);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_elements_small",
                              true,
                              "Small allocation should be freed without crash") && result;

    // test 4: free of larger allocation with struct type
    {
        struct large_struct
        {
            int    data[100];
            char   name[256];
            double values[50];
        };

        elements = malloc(10 * sizeof(struct large_struct));
        d_vector_common_free_elements(elements);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_elements_large",
                                  true,
                                  "Large allocation should be freed without crash") && result;
    }

    return result;
}


/*
d_tests_vector_common_cleanup_all
  Aggregation function that runs all cleanup tests.
*/
bool d_tests_vector_common_cleanup_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_common_cleanup_all");

    result = true;

    result = d_tests_vector_common_free_elements(_suite) && result;

    return result;
}
