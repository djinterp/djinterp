#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_free
  Tests the d_vector_free function.
  Tests the following:
  - NULL vector handling (should not crash)
  - successful free of valid vector
*/
bool
d_tests_sa_vector_free
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should not crash
    d_vector_free(NULL);
    result = d_assert_standalone(
        true,
        "free_null_vector",
        "NULL vector should not crash",
        _counter) && result;

    // test 2: successful free of valid vector
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        d_vector_free(vec);
        result = d_assert_standalone(
            true,
            "free_valid_vector",
            "Valid vector should be freed without crash",
            _counter) && result;
    }

    // test 3: free of empty vector
    vec = d_vector_new(sizeof(int), 0);

    if (vec)
    {
        d_vector_free(vec);
        result = d_assert_standalone(
            true,
            "free_empty_vector",
            "Empty vector should be freed without crash",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_vector_free_deep
  Tests the d_vector_free_deep function.
  Tests the following:
  - NULL vector handling (should not crash)
  - NULL free_fn handling (should not crash)
  - successful deep free of pointer vector
*/
bool
d_tests_sa_vector_free_deep
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    void*            ptr1;
    void*            ptr2;
    void*            ptr3;

    result = true;

    // test 1: NULL vector should not crash
    d_vector_free_deep(NULL, free);
    result = d_assert_standalone(
        true,
        "free_deep_null_vector",
        "NULL vector should not crash",
        _counter) && result;

    // test 2: NULL free_fn should not crash (but won't free elements)
    vec = d_vector_new(sizeof(void*), 10);

    if (vec)
    {
        d_vector_free_deep(vec, NULL);
        result = d_assert_standalone(
            true,
            "free_deep_null_fn",
            "NULL free_fn should not crash",
            _counter) && result;
    }

    // test 3: successful deep free of pointer vector
    vec = d_vector_new(sizeof(void*), 10);

    if (vec)
    {
        // allocate some memory and store pointers
        ptr1 = malloc(100);
        ptr2 = malloc(200);
        ptr3 = malloc(300);

        if ( (ptr1) &&
             (ptr2) &&
             (ptr3) )
        {
            d_vector_push_back(vec, &ptr1);
            d_vector_push_back(vec, &ptr2);
            d_vector_push_back(vec, &ptr3);

            d_vector_free_deep(vec, free);
            result = d_assert_standalone(
                true,
                "free_deep_valid",
                "Deep free should succeed without crash",
                _counter) && result;
        }
        else
        {
            // cleanup if allocation failed
            if (ptr1)
            {
                free(ptr1);
            }

            if (ptr2)
            {
                free(ptr2);
            }

            if (ptr3)
            {
                free(ptr3);
            }

            d_vector_free(vec);
        }
    }

    return result;
}


/*
d_tests_sa_vector_destructor_all
  Aggregation function that runs all destructor tests.
*/
bool
d_tests_sa_vector_destructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Destructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_vector_free(_counter) && result;
    result = d_tests_sa_vector_free_deep(_counter) && result;

    return result;
}
