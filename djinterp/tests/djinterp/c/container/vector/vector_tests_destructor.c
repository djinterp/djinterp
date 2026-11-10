/*******************************************************************************
* djinterp [container]                                 vector_tests_destructor.c
*
*
* path:      /tests/djinterp/c/container/vector/vector_tests_destructor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.25
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./vector_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_vector_free
  Tests the d_vector_free function.
  Tests the following:
  - NULL vector handling (should not crash)
  - successful free of valid vector
*/
bool d_tests_vector_free(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;

    d_test_suite_test(_suite, "vector_free");

    result = true;

    // test 1: NULL vector should not crash
    d_vector_free(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_null_vector",
                              true,
                              "NULL vector should not crash") && result;

    // test 2: successful free of valid vector
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        d_vector_free(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_valid_vector",
                                  true,
                                  "Valid vector should be freed without crash") && result;
    }

    // test 3: free of empty vector
    vec = d_vector_new(sizeof(int), 0);

    if (vec)
    {
        d_vector_free(vec);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_empty_vector",
                                  true,
                                  "Empty vector should be freed without crash") && result;
    }

    return result;
}


/*
d_tests_vector_free_deep
  Tests the d_vector_free_deep function.
  Tests the following:
  - NULL vector handling (should not crash)
  - NULL free_fn handling (should not crash)
  - successful deep free of pointer vector
*/
bool d_tests_vector_free_deep(
    struct dj_test_suite* _suite
)
{
    bool             result;
    struct dj_vector* vec;
    void*            ptr1;
    void*            ptr2;
    void*            ptr3;

    d_test_suite_test(_suite, "vector_free_deep");

    result = true;

    // test 1: NULL vector should not crash
    d_vector_free_deep(NULL, free);
    result = D_TEST_CHECK_MSG(_suite,
                              "free_deep_null_vector",
                              true,
                              "NULL vector should not crash") && result;

    // test 2: NULL free_fn should not crash (but won't free elements)
    vec = d_vector_new(sizeof(void*), 10);

    if (vec)
    {
        d_vector_free_deep(vec, NULL);
        result = D_TEST_CHECK_MSG(_suite,
                                  "free_deep_null_fn",
                                  true,
                                  "NULL free_fn should not crash") && result;

        // with no free function nothing was freed: the vector is still ours
        d_vector_free(vec);
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
            result = D_TEST_CHECK_MSG(_suite,
                                      "free_deep_valid",
                                      true,
                                      "Deep free should succeed without crash") && result;
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
d_tests_vector_destructor_all
  Aggregation function that runs all destructor tests.
*/
bool d_tests_vector_destructor_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "vector_destructor_all");

    result = true;

    result = d_tests_vector_free(_suite) && result;
    result = d_tests_vector_free_deep(_suite) && result;

    return result;
}
