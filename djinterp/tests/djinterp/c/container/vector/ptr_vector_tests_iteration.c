/*******************************************************************************
* djinterp [tests]                                  ptr_vector_tests_iteration.c
*
*   Unit tests for ptr_vector iteration helper functions.
*
*
* path:      /tests/djinterp/c/container/vector/ptr_vector_tests_iteration.c
* link(s):   TBA
*                djinterp-c-container-ptr-vector-tests-sa/main.c
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.29
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./ptr_vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


static int g_iter_test_values[] = {1, 2, 3, 4, 5};
static int g_apply_sum = 0;
static int g_apply_count = 0;

static
void apply_sum_fn(
    void* _element
)
{
    if (_element)
    {
        g_apply_sum += *(int*)_element;
    }
    g_apply_count++;
}

static
void apply_ctx_fn(
    void* _element,
    void* _context
)
{
    if (_element && _context)
    {
        *(int*)_context += *(int*)_element;
    }
}


bool d_tests_ptr_vector_foreach(
    struct dj_test_suite* _suite
)
{
    bool                 result;
    struct dj_ptr_vector* vec;

    d_test_suite_test(_suite, "ptr_vector_foreach");

    result = true;

    /* Reset globals */
    g_apply_sum = 0;
    g_apply_count = 0;

    /* test 1: NULL checks - foreach returns void */
    d_ptr_vector_foreach(NULL, apply_sum_fn);
    result = D_TEST_CHECK_MSG(_suite,
                              "foreach_null_vector",
                              g_apply_count == 0,
                              "Foreach NULL vector should not call function") && result;

    /* test 2: foreach with NULL function */
    vec = d_ptr_vector_new_from_args(3,
        &g_iter_test_values[0], &g_iter_test_values[1], &g_iter_test_values[2]);
    if (vec)
    {
        d_ptr_vector_foreach(vec, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_null_fn",
                                  true,
                                  "Foreach with NULL function should not crash") && result;

        d_ptr_vector_free(vec);
    }

    /* test 3: foreach applies to all elements */
    vec = d_ptr_vector_new_from_args(3,
        &g_iter_test_values[0], &g_iter_test_values[1], &g_iter_test_values[2]);
    if (vec)
    {
        g_apply_sum = 0;
        g_apply_count = 0;
        d_ptr_vector_foreach(vec, apply_sum_fn);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_success",
                                  g_apply_count == 3 && g_apply_sum == 6,
                                  "Foreach should apply function to all elements (sum=6)") && result;

        d_ptr_vector_free(vec);
    }

    /* test 4: empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        g_apply_count = 0;
        d_ptr_vector_foreach(vec, apply_sum_fn);
        result = D_TEST_CHECK_MSG(_suite,
                                  "foreach_empty",
                                  g_apply_count == 0,
                                  "Foreach on empty vector should not call function") && result;

        d_ptr_vector_free(vec);
    }

    return result;
}

bool d_tests_ptr_vector_iteration_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "ptr_vector_iteration_all");

    return d_tests_ptr_vector_foreach(_suite);
}
