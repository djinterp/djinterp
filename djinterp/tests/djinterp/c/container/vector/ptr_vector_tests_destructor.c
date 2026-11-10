/*******************************************************************************
* djinterp [test]                                  ptr_vector_tests_destructor.c
*
*   Unit tests for ptr_vector destructor functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_destructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

static int g_free_count = 0;

static
void mock_free_fn(
    void* _ptr
)
{
    if (_ptr)
    {
        g_free_count++;
        free(_ptr);
    }
}


bool d_tests_ptr_vector_free(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    static int           test_values[] = {10, 20, 30};

    d_test_suite_test(_suite, "ptr_vector_free");

    result = true;

    /* test 1: NULL should not crash */
    d_ptr_vector_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_null",
                              true,
                              "Free NULL vector should not crash") && result;

    /* test 2: free non-empty vector */
    vec = d_ptr_vector_new_from_args(3,
        &test_values[0], &test_values[1], &test_values[2]);
    if (vec)
    {
        d_ptr_vector_free(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_nonempty",
                                  true,
                                  "Free non-empty vector should work") && result;
    }

    /* test 3: free empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_free(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_empty",
                                  true,
                                  "Free empty vector should work") && result;
    }

    return result;
}


bool d_tests_ptr_vector_free_deep(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int*                 ptr1;
    int*                 ptr2;
    int*                 ptr3;

    d_test_suite_test(_suite, "ptr_vector_free_deep");

    result = true;

    /* test 1: NULL vector should not crash */
    d_ptr_vector_free_deep(NULL, mock_free_fn);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_deep_null",
                              true,
                              "Free deep NULL vector should not crash") && result;

    /* test 2: free deep with allocated elements */
    g_free_count = 0;
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        ptr1 = (int*)malloc(sizeof(int)); *ptr1 = 100;
        ptr2 = (int*)malloc(sizeof(int)); *ptr2 = 200;
        ptr3 = (int*)malloc(sizeof(int)); *ptr3 = 300;

        d_ptr_vector_push_back(vec, ptr1);
        d_ptr_vector_push_back(vec, ptr2);
        d_ptr_vector_push_back(vec, ptr3);

        d_ptr_vector_free_deep(vec, mock_free_fn);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_deep_elements",
                                  g_free_count == 3,
                                  "Free deep should call free function for each element") && result;
    }

    /* test 3: free deep with NULL elements */
    g_free_count = 0;
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, NULL);
        d_ptr_vector_push_back(vec, NULL);
        d_ptr_vector_free_deep(vec, mock_free_fn);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_deep_null_elements",
                                  g_free_count == 0,
                                  "Free deep should skip NULL elements") && result;
    }

    /* test 4: free deep with NULL free function */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_free_deep(vec, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_deep_null_fn",
                                  true,
                                  "Free deep with NULL function should not crash") && result;

        /* with no free function nothing was freed: the vector is still ours */
        d_ptr_vector_free(vec);
    }

    return result;
}


bool d_tests_ptr_vector_clear_deep(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;
    int*                 ptr1;
    int*                 ptr2;

    d_test_suite_test(_suite, "ptr_vector_clear_deep");

    result = true;

    /* test 1: NULL vector should not crash */
    d_ptr_vector_clear_deep(NULL, mock_free_fn);
    result = D_TEST_CHECK_MSG(_suite,
                              "clear_deep_null",
                              true,
                              "Clear deep NULL vector should not crash") && result;

    /* test 2: clear deep with allocated elements */
    g_free_count = 0;
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        ptr1 = (int*)malloc(sizeof(int)); *ptr1 = 100;
        ptr2 = (int*)malloc(sizeof(int)); *ptr2 = 200;

        d_ptr_vector_push_back(vec, ptr1);
        d_ptr_vector_push_back(vec, ptr2);

        d_ptr_vector_clear_deep(vec, mock_free_fn);

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_deep_elements",
                                  g_free_count == 2 && vec->count == 0,
                                  "Clear deep should free elements and reset count") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_deep_preserves_capacity",
                                  vec->capacity > 0 && vec->elements != NULL,
                                  "Clear deep should preserve capacity") && result;

        /* Can still use the vector after clear_deep */
        ptr1 = (int*)malloc(sizeof(int)); *ptr1 = 300;
        d_ptr_vector_push_back(vec, ptr1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "clear_deep_reusable",
                                  vec->count == 1,
                                  "Vector should be reusable after clear_deep") && result;

        d_ptr_vector_free_deep(vec, mock_free_fn);
    }

    return result;
}


bool d_tests_ptr_vector_destructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "ptr_vector_destructor_all");

    result = true;

    result = d_tests_ptr_vector_free(_suite) && result;
    result = d_tests_ptr_vector_free_deep(_suite) && result;
    result = d_tests_ptr_vector_clear_deep(_suite) && result;

    return result;
}
