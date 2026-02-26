#include "./vector_common_tests_sa.h"


/*
d_tests_sa_vector_common_append_element
  Tests the d_vector_common_append_element function for appending a single element.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - successful append of single element
  - automatic capacity growth
*/
bool
d_tests_sa_vector_common_append_element
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = d_assert_standalone(
        d_vector_common_append_element(NULL,
                                       &count,
                                       &capacity,
                                       sizeof(int),
                                       &value) == D_FAILURE,
        "append_element_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_append_element(&elements,
                                       NULL,
                                       &capacity,
                                       sizeof(int),
                                       &value) == D_FAILURE,
        "append_element_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = d_assert_standalone(
        d_vector_common_append_element(&elements,
                                       &count,
                                       NULL,
                                       sizeof(int),
                                       &value) == D_FAILURE,
        "append_element_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_append_element(&elements,
                                       &count,
                                       &capacity,
                                       0,
                                       &value) == D_FAILURE,
        "append_element_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 5: successful append to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    value  = 100;
    result = d_assert_standalone(
        d_vector_common_append_element(&elements,
                                       &count,
                                       &capacity,
                                       sizeof(int),
                                       &value) == D_SUCCESS,
        "append_element_empty_success",
        "Append to empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 1 && ((int*)elements)[0] == 100,
        "append_element_empty_result",
        "Count=1, element[0]=100",
        _counter) && result;

    // test 6: append to non-empty vector
    value  = 200;
    result = d_assert_standalone(
        d_vector_common_append_element(&elements,
                                       &count,
                                       &capacity,
                                       sizeof(int),
                                       &value) == D_SUCCESS,
        "append_element_nonempty_success",
        "Append to non-empty vector should succeed",
        _counter) && result;

    result = d_assert_standalone(
        count == 2 && ((int*)elements)[1] == 200,
        "append_element_nonempty_result",
        "Count=2, element[1]=200",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_append_elements
  Tests the d_vector_common_append_elements function for appending multiple elements.
  Tests the following:
  - NULL parameter handling
  - zero element_size rejection
  - zero source_count (no-op success)
  - NULL source with non-zero count failure
  - successful append to empty vector
  - successful append to non-empty vector
  - automatic capacity growth
*/
bool
d_tests_sa_vector_common_append_elements
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {100, 200, 300};

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_append_elements(NULL,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        source,
                                        3) == D_FAILURE,
        "append_elements_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        NULL,
                                        &capacity,
                                        sizeof(int),
                                        source,
                                        3) == D_FAILURE,
        "append_elements_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: NULL capacity pointer should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    result   = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        &count,
                                        NULL,
                                        sizeof(int),
                                        source,
                                        3) == D_FAILURE,
        "append_elements_null_capacity",
        "NULL capacity pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 4: zero element_size should fail
    elements = malloc(10 * sizeof(int));
    count    = 0;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        &count,
                                        &capacity,
                                        0,
                                        source,
                                        3) == D_FAILURE,
        "append_elements_zero_element_size",
        "Zero element_size should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 5: zero source_count is no-op success
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        source,
                                        0) == D_SUCCESS,
        "append_elements_zero_count",
        "Zero source_count should return D_SUCCESS (no-op)",
        _counter) && result;

    result = d_assert_standalone(
        count == 0,
        "append_elements_zero_count_unchanged",
        "Count should remain unchanged",
        _counter) && result;

    // test 6: NULL source with non-zero count should fail
    result = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        NULL,
                                        3) == D_FAILURE,
        "append_elements_null_source",
        "NULL source with non-zero count should return D_FAILURE",
        _counter) && result;

    // test 7: successful append to empty vector
    result = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        source,
                                        3) == D_SUCCESS,
        "append_elements_empty_success",
        "Append to empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 3,
        "append_elements_empty_count",
        "Count should be 3",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300);

        result = d_assert_standalone(
            correct,
            "append_elements_empty_values",
            "Elements should be [100, 200, 300]",
            _counter) && result;
    }

    // test 8: successful append to non-empty vector
    int more_source[2] = {400, 500};
    result             = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        more_source,
                                        2) == D_SUCCESS,
        "append_elements_nonempty_success",
        "Append to non-empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 5,
        "append_elements_nonempty_count",
        "Count should be 5",
        _counter) && result;

    {
        int* arr = (int*)elements;
        bool correct = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300) &&
                       (arr[3] == 400) && (arr[4] == 500);

        result = d_assert_standalone(
            correct,
            "append_elements_nonempty_values",
            "Elements should be [100, 200, 300, 400, 500]",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    // test 9: automatic capacity growth
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 2);

    int large_source[10];
    size_t i;
    for (i = 0; i < 10; i++)
    {
        large_source[i] = (int)(i * 10);
    }

    result = d_assert_standalone(
        d_vector_common_append_elements(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        large_source,
                                        10) == D_SUCCESS,
        "append_elements_grow_success",
        "Append beyond capacity should succeed (with growth)",
        _counter) && result;

    result = d_assert_standalone(
        count == 10,
        "append_elements_grow_count",
        "Count should be 10",
        _counter) && result;

    result = d_assert_standalone(
        capacity >= 10,
        "append_elements_grow_capacity",
        "Capacity should be at least 10",
        _counter) && result;

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_prepend_element
  Tests the d_vector_common_prepend_element function for prepending a single element.
  Tests the following:
  - NULL parameter handling
  - successful prepend to empty vector
  - successful prepend to non-empty vector (shifts existing)
*/
bool
d_tests_sa_vector_common_prepend_element
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    value;

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    value    = 42;
    result   = d_assert_standalone(
        d_vector_common_prepend_element(NULL,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        &value) == D_FAILURE,
        "prepend_element_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: successful prepend to empty vector
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    value  = 100;
    result = d_assert_standalone(
        d_vector_common_prepend_element(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        &value) == D_SUCCESS,
        "prepend_element_empty_success",
        "Prepend to empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 1 && ((int*)elements)[0] == 100,
        "prepend_element_empty_result",
        "Count=1, element[0]=100",
        _counter) && result;

    // test 3: prepend to non-empty vector (shifts existing)
    value  = 50;
    result = d_assert_standalone(
        d_vector_common_prepend_element(&elements,
                                        &count,
                                        &capacity,
                                        sizeof(int),
                                        &value) == D_SUCCESS,
        "prepend_element_nonempty_success",
        "Prepend to non-empty vector should succeed",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 50) && (arr[1] == 100);

        result = d_assert_standalone(
            correct && count == 2,
            "prepend_element_shifted",
            "Elements should be [50, 100]",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_prepend_elements
  Tests the d_vector_common_prepend_elements function for prepending multiple elements.
  Tests the following:
  - NULL parameter handling
  - zero source_count (no-op success)
  - NULL source with non-zero count failure
  - successful prepend to empty vector
  - successful prepend to non-empty vector
  - elements correctly shifted
*/
bool
d_tests_sa_vector_common_prepend_elements
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  elements;
    size_t count;
    size_t capacity;
    int    source[3] = {10, 20, 30};

    result = true;

    // test 1: NULL elements pointer should fail
    count    = 0;
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_prepend_elements(NULL,
                                         &count,
                                         &capacity,
                                         sizeof(int),
                                         source,
                                         3) == D_FAILURE,
        "prepend_elements_null_elements",
        "NULL elements pointer should return D_FAILURE",
        _counter) && result;

    // test 2: NULL count pointer should fail
    elements = malloc(10 * sizeof(int));
    capacity = 10;
    result   = d_assert_standalone(
        d_vector_common_prepend_elements(&elements,
                                         NULL,
                                         &capacity,
                                         sizeof(int),
                                         source,
                                         3) == D_FAILURE,
        "prepend_elements_null_count",
        "NULL count pointer should return D_FAILURE",
        _counter) && result;

    free(elements);

    // test 3: zero source_count is no-op success
    elements = NULL;
    count    = 0;
    capacity = 0;
    d_vector_common_init(&elements, &count, &capacity, sizeof(int), 10);

    result = d_assert_standalone(
        d_vector_common_prepend_elements(&elements,
                                         &count,
                                         &capacity,
                                         sizeof(int),
                                         source,
                                         0) == D_SUCCESS,
        "prepend_elements_zero_count",
        "Zero source_count should return D_SUCCESS (no-op)",
        _counter) && result;

    // test 4: NULL source with non-zero count should fail
    result = d_assert_standalone(
        d_vector_common_prepend_elements(&elements,
                                         &count,
                                         &capacity,
                                         sizeof(int),
                                         NULL,
                                         3) == D_FAILURE,
        "prepend_elements_null_source",
        "NULL source with non-zero count should fail",
        _counter) && result;

    // test 5: successful prepend to empty vector
    result = d_assert_standalone(
        d_vector_common_prepend_elements(&elements,
                                         &count,
                                         &capacity,
                                         sizeof(int),
                                         source,
                                         3) == D_SUCCESS,
        "prepend_elements_empty_success",
        "Prepend to empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 3,
        "prepend_elements_empty_count",
        "Count should be 3",
        _counter) && result;

    {
        int* arr     = (int*)elements;
        bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

        result = d_assert_standalone(
            correct,
            "prepend_elements_empty_values",
            "Elements should be [10, 20, 30]",
            _counter) && result;
    }

    // test 6: successful prepend to non-empty vector (shifts existing)
    int prepend_source[2] = {1, 2};
    result                = d_assert_standalone(
        d_vector_common_prepend_elements(&elements,
                                         &count,
                                         &capacity,
                                         sizeof(int),
                                         prepend_source,
                                         2) == D_SUCCESS,
        "prepend_elements_nonempty_success",
        "Prepend to non-empty vector should return D_SUCCESS",
        _counter) && result;

    result = d_assert_standalone(
        count == 5,
        "prepend_elements_nonempty_count",
        "Count should be 5",
        _counter) && result;

    {
        int* arr = (int*)elements;
        bool correct = (arr[0] == 1) && (arr[1] == 2) && (arr[2] == 10) &&
                       (arr[3] == 20) && (arr[4] == 30);

        result = d_assert_standalone(
            correct,
            "prepend_elements_nonempty_values",
            "Elements should be [1, 2, 10, 20, 30]",
            _counter) && result;
    }

    if (elements)
    {
        free(elements);
    }

    return result;
}


/*
d_tests_sa_vector_common_append_all
  Aggregation function that runs all append/extend tests.
*/
bool
d_tests_sa_vector_common_append_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Append/Extend Functions\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_vector_common_append_element(_counter) && result;
    result = d_tests_sa_vector_common_append_elements(_counter) && result;
    result = d_tests_sa_vector_common_prepend_element(_counter) && result;
    result = d_tests_sa_vector_common_prepend_elements(_counter) && result;

    return result;
}
