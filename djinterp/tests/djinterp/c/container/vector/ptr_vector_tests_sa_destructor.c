/******************************************************************************
* djinterp [test]                                 ptr_vector_tests_sa_destructor.c
*
*   Unit tests for ptr_vector destructor functions.
*
* path:      \tests\container\vector\ptr_vector_tests_sa_destructor.c
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.01.29
******************************************************************************/

#include "./ptr_vector_tests_sa.h"

static int g_free_count = 0;

static void
mock_free_fn(void* _ptr)
{
    if (_ptr)
    {
        g_free_count++;
        free(_ptr);
    }
}


bool
d_tests_sa_ptr_vector_free
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    static int           test_values[] = {10, 20, 30};

    result = true;

    /* test 1: NULL should not crash */
    d_ptr_vector_free(NULL);
    result = d_assert_standalone(true,
        "free_null",
        "Free NULL vector should not crash",
        _counter) && result;

    /* test 2: free non-empty vector */
    vec = d_ptr_vector_new_from_args(3,
        &test_values[0], &test_values[1], &test_values[2]);
    if (vec)
    {
        d_ptr_vector_free(vec);
        result = d_assert_standalone(true,
            "free_nonempty",
            "Free non-empty vector should work",
            _counter) && result;
    }

    /* test 3: free empty vector */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_free(vec);
        result = d_assert_standalone(true,
            "free_empty",
            "Free empty vector should work",
            _counter) && result;
    }

    return result;
}


bool
d_tests_sa_ptr_vector_free_deep
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int*                 ptr1;
    int*                 ptr2;
    int*                 ptr3;

    result = true;

    /* test 1: NULL vector should not crash */
    d_ptr_vector_free_deep(NULL, mock_free_fn);
    result = d_assert_standalone(true,
        "free_deep_null",
        "Free deep NULL vector should not crash",
        _counter) && result;

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
        result = d_assert_standalone(g_free_count == 3,
            "free_deep_elements",
            "Free deep should call free function for each element",
            _counter) && result;
    }

    /* test 3: free deep with NULL elements */
    g_free_count = 0;
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_push_back(vec, NULL);
        d_ptr_vector_push_back(vec, NULL);
        d_ptr_vector_free_deep(vec, mock_free_fn);
        result = d_assert_standalone(g_free_count == 0,
            "free_deep_null_elements",
            "Free deep should skip NULL elements",
            _counter) && result;
    }

    /* test 4: free deep with NULL free function */
    vec = d_ptr_vector_new(10);
    if (vec)
    {
        d_ptr_vector_free_deep(vec, NULL);
        result = d_assert_standalone(true,
            "free_deep_null_fn",
            "Free deep with NULL function should not crash",
            _counter) && result;
    }

    return result;
}


bool
d_tests_sa_ptr_vector_clear_deep
(
    struct d_test_counter* _counter
)
{
    bool                 result;
    struct d_ptr_vector* vec;
    int*                 ptr1;
    int*                 ptr2;

    result = true;

    /* test 1: NULL vector should not crash */
    d_ptr_vector_clear_deep(NULL, mock_free_fn);
    result = d_assert_standalone(true,
        "clear_deep_null",
        "Clear deep NULL vector should not crash",
        _counter) && result;

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

        result = d_assert_standalone(
            g_free_count == 2 && vec->count == 0,
            "clear_deep_elements",
            "Clear deep should free elements and reset count",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity > 0 && vec->elements != NULL,
            "clear_deep_preserves_capacity",
            "Clear deep should preserve capacity",
            _counter) && result;

        /* Can still use the vector after clear_deep */
        ptr1 = (int*)malloc(sizeof(int)); *ptr1 = 300;
        d_ptr_vector_push_back(vec, ptr1);
        result = d_assert_standalone(vec->count == 1,
            "clear_deep_reusable",
            "Vector should be reusable after clear_deep",
            _counter) && result;

        d_ptr_vector_free_deep(vec, mock_free_fn);
    }

    return result;
}


bool
d_tests_sa_ptr_vector_destructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Destructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_ptr_vector_free(_counter) && result;
    result = d_tests_sa_ptr_vector_free_deep(_counter) && result;
    result = d_tests_sa_ptr_vector_clear_deep(_counter) && result;

    return result;
}
