#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_free_elements
  Tests the d_vector_common_free_elements function for freeing memory.
  Tests the following:
  - NULL elements handling (should not crash)
  - successful free of allocated memory
*/
bool
d_tests_sa_vector_common_free_elements
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;

    result = true;

    // test 1: NULL elements should be handled gracefully (no crash)
    d_vector_common_free_elements(NULL);
    result = d_assert_standalone(
        true,
        "free_elements_null",
        "NULL elements should not crash",
        _counter) && result;

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
    result = d_assert_standalone(
        true,
        "free_elements_valid",
        "Valid elements should be freed without crash",
        _counter) && result;

    // Note: we cannot directly test that memory is freed, but we can verify
    // the function doesn't crash

    // test 3: free of small allocation
    elements = malloc(sizeof(int));
    d_vector_common_free_elements(elements);
    result = d_assert_standalone(
        true,
        "free_elements_small",
        "Small allocation should be freed without crash",
        _counter) && result;

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
        result = d_assert_standalone(
            true,
            "free_elements_large",
            "Large allocation should be freed without crash",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_vector_common_cleanup_all
  Aggregation function that runs all cleanup tests.
*/
bool
d_tests_sa_vector_common_cleanup_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Cleanup Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_vector_common_free_elements(_counter) && result;

    return result;
}
