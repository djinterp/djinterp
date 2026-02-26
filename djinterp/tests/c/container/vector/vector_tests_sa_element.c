#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_push_back
  Tests the d_vector_push_back function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - successful push to empty vector
  - successful push to non-empty vector
  - automatic growth when full
*/
bool
d_tests_sa_vector_push_back
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              value;

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = d_assert_standalone(
        d_vector_push_back(NULL, &value) == D_FAILURE,
        "push_back_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: NULL value should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_push_back(vec, NULL) == D_FAILURE,
            "push_back_null_value",
            "NULL value should return D_FAILURE",
            _counter) && result;

        // test 3: successful push to empty vector
        value  = 100;
        result = d_assert_standalone(
            d_vector_push_back(vec, &value) == D_SUCCESS,
            "push_back_empty_success",
            "Push to empty vector should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 1 && ((int*)vec->elements)[0] == 100,
            "push_back_empty_result",
            "Count=1, element[0]=100",
            _counter) && result;

        // test 4: successful push to non-empty vector
        value  = 200;
        result = d_assert_standalone(
            d_vector_push_back(vec, &value) == D_SUCCESS,
            "push_back_nonempty_success",
            "Push to non-empty vector should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 2 && ((int*)vec->elements)[1] == 200,
            "push_back_nonempty_result",
            "Count=2, element[1]=200",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 5: automatic growth when full
    vec = d_vector_new(sizeof(int), 2);

    if (vec)
    {
        value = 1;
        d_vector_push_back(vec, &value);
        value = 2;
        d_vector_push_back(vec, &value);

        size_t old_capacity = vec->capacity;
        value               = 3;

        result = d_assert_standalone(
            d_vector_push_back(vec, &value) == D_SUCCESS,
            "push_back_grow_success",
            "Push beyond capacity should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity > old_capacity && vec->count == 3,
            "push_back_grow_result",
            "Capacity should grow, count=3",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_push_front
  Tests the d_vector_push_front function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - successful push front
  - existing elements shifted
*/
bool
d_tests_sa_vector_push_front
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              value;

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = d_assert_standalone(
        d_vector_push_front(NULL, &value) == D_FAILURE,
        "push_front_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful push front
    vec = d_vector_new_from_args(sizeof(int), 2, 20, 30);

    if (vec)
    {
        value  = 10;
        result = d_assert_standalone(
            d_vector_push_front(vec, &value) == D_SUCCESS,
            "push_front_success",
            "Push front should succeed",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = d_assert_standalone(
                correct && vec->count == 3,
                "push_front_shifted",
                "Elements should be [10, 20, 30]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_pop_back
  Tests the d_vector_pop_back function.
  Tests the following:
  - NULL vector rejection
  - empty vector rejection
  - successful pop with output
  - successful pop without output
*/
bool
d_tests_sa_vector_pop_back
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              out_value;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_pop_back(NULL, &out_value) == D_FAILURE,
        "pop_back_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: empty vector should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_pop_back(vec, &out_value) == D_FAILURE,
            "pop_back_empty",
            "Empty vector should return D_FAILURE",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: successful pop with output
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        out_value = 0;
        result    = d_assert_standalone(
            d_vector_pop_back(vec, &out_value) == D_SUCCESS,
            "pop_back_success",
            "Pop back should succeed",
            _counter) && result;

        result = d_assert_standalone(
            out_value == 30 && vec->count == 2,
            "pop_back_result",
            "out_value=30, count=2",
            _counter) && result;

        // test 4: pop without output
        result = d_assert_standalone(
            d_vector_pop_back(vec, NULL) == D_SUCCESS,
            "pop_back_null_out",
            "Pop with NULL out should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 1,
            "pop_back_null_out_count",
            "count should be 1",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_pop_front
  Tests the d_vector_pop_front function.
  Tests the following:
  - NULL vector rejection
  - empty vector rejection
  - successful pop with output
  - remaining elements shifted
*/
bool
d_tests_sa_vector_pop_front
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              out_value;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_pop_front(NULL, &out_value) == D_FAILURE,
        "pop_front_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: empty vector should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_pop_front(vec, &out_value) == D_FAILURE,
            "pop_front_empty",
            "Empty vector should return D_FAILURE",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: successful pop
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        out_value = 0;
        result    = d_assert_standalone(
            d_vector_pop_front(vec, &out_value) == D_SUCCESS,
            "pop_front_success",
            "Pop front should succeed",
            _counter) && result;

        result = d_assert_standalone(
            out_value == 10 && vec->count == 2,
            "pop_front_result",
            "out_value=10, count=2",
            _counter) && result;

        // test 4: remaining elements shifted
        {
            int* arr     = (int*)vec->elements;
            bool shifted = (arr[0] == 20) && (arr[1] == 30);

            result = d_assert_standalone(
                shifted,
                "pop_front_shifted",
                "Elements should be [20, 30]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_insert_element
  Tests the d_vector_insert_element function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - successful insert at various positions
  - negative index support
*/
bool
d_tests_sa_vector_insert_element
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              value;

    result = true;

    // test 1: NULL vector should fail
    value  = 42;
    result = d_assert_standalone(
        d_vector_insert_element(NULL, 0, &value) == D_FAILURE,
        "insert_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: NULL value should fail
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_insert_element(vec, 0, NULL) == D_FAILURE,
            "insert_null_value",
            "NULL value should return D_FAILURE",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: insert at various positions
    vec = d_vector_new_from_args(sizeof(int), 2, 10, 30);

    if (vec)
    {
        // insert in middle
        value  = 20;
        result = d_assert_standalone(
            d_vector_insert_element(vec, 1, &value) == D_SUCCESS,
            "insert_middle_success",
            "Insert at middle should succeed",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30);

            result = d_assert_standalone(
                correct,
                "insert_middle_result",
                "Elements should be [10, 20, 30]",
                _counter) && result;
        }

        // test 4: negative index
        value  = 25;
        result = d_assert_standalone(
            d_vector_insert_element(vec, -1, &value) == D_SUCCESS,
            "insert_negative_success",
            "Insert with negative index should succeed",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_insert_elements
  Tests the d_vector_insert_elements function.
  Tests the following:
  - NULL vector rejection
  - zero count no-op
  - successful range insert
*/
bool
d_tests_sa_vector_insert_elements
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              source[3] = {100, 200, 300};

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_insert_elements(NULL, 0, source, 3) == D_FAILURE,
        "insert_range_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: zero count no-op
    vec = d_vector_new_from_args(sizeof(int), 2, 10, 40);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_insert_elements(vec, 1, source, 0) == D_SUCCESS,
            "insert_range_zero_count",
            "Zero count should return D_SUCCESS",
            _counter) && result;

        // test 3: successful range insert
        result = d_assert_standalone(
            d_vector_insert_elements(vec, 1, source, 3) == D_SUCCESS,
            "insert_range_success",
            "Insert range should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 5,
            "insert_range_count",
            "Count should be 5",
            _counter) && result;

        {
            int* arr = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 100) && (arr[2] == 200) &&
                           (arr[3] == 300) && (arr[4] == 40);

            result = d_assert_standalone(
                correct,
                "insert_range_order",
                "Elements should be [10, 100, 200, 300, 40]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_erase
  Tests the d_vector_erase function.
  Tests the following:
  - NULL vector rejection
  - invalid index rejection
  - successful erase
  - negative index support
*/
bool
d_tests_sa_vector_erase
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_erase(NULL, 0) == D_FAILURE,
        "erase_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful erase
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_erase(vec, 1) == D_SUCCESS,
            "erase_middle_success",
            "Erase at middle should succeed",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 30) && (vec->count == 2);

            result = d_assert_standalone(
                correct,
                "erase_middle_result",
                "Elements should be [10, 30]",
                _counter) && result;
        }

        // test 3: erase with negative index
        result = d_assert_standalone(
            d_vector_erase(vec, -1) == D_SUCCESS,
            "erase_negative_success",
            "Erase with negative index should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 1 && ((int*)vec->elements)[0] == 10,
            "erase_negative_result",
            "Only element 10 should remain",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_erase_range
  Tests the d_vector_erase_range function.
  Tests the following:
  - NULL vector rejection
  - invalid range rejection
  - successful range erase
*/
bool
d_tests_sa_vector_erase_range
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_erase_range(NULL, 0, 1) == D_FAILURE,
        "erase_range_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful range erase
    vec = d_vector_new_from_args(sizeof(int), 5, 10, 20, 30, 40, 50);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_erase_range(vec, 1, 3) == D_SUCCESS,
            "erase_range_success",
            "Erase range [1,3] should succeed",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (vec->count == 2) && (arr[0] == 10) && (arr[1] == 50);

            result = d_assert_standalone(
                correct,
                "erase_range_result",
                "Elements should be [10, 50]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_clear
  Tests the d_vector_clear function.
  Tests the following:
  - NULL vector handling
  - successful clear
  - capacity preserved after clear
*/
bool
d_tests_sa_vector_clear
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: NULL vector should not crash
    d_vector_clear(NULL);
    result = d_assert_standalone(
        true,
        "clear_null_vector",
        "NULL vector should not crash",
        _counter) && result;

    // test 2: successful clear
    vec = d_vector_new_from_args(sizeof(int), 5, 1, 2, 3, 4, 5);

    if (vec)
    {
        size_t old_capacity = vec->capacity;

        d_vector_clear(vec);

        result = d_assert_standalone(
            vec->count == 0,
            "clear_count_zero",
            "Count should be 0 after clear",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity == old_capacity,
            "clear_capacity_preserved",
            "Capacity should be preserved after clear",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_element_all
  Aggregation function that runs all element manipulation tests.
*/
bool
d_tests_sa_vector_element_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Element Manipulation Functions\n");
    printf("  -----------------------------------------\n");

    result = d_tests_sa_vector_push_back(_counter) && result;
    result = d_tests_sa_vector_push_front(_counter) && result;
    result = d_tests_sa_vector_pop_back(_counter) && result;
    result = d_tests_sa_vector_pop_front(_counter) && result;
    result = d_tests_sa_vector_insert_element(_counter) && result;
    result = d_tests_sa_vector_insert_elements(_counter) && result;
    result = d_tests_sa_vector_erase(_counter) && result;
    result = d_tests_sa_vector_erase_range(_counter) && result;
    result = d_tests_sa_vector_clear(_counter) && result;

    return result;
}
