#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_at
  Tests the d_vector_at function.
  Tests the following:
  - NULL vector returns NULL
  - empty vector returns NULL
  - valid positive index
  - valid negative index
  - out-of-bounds returns NULL
*/
bool
d_tests_sa_vector_at
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    void*            ptr;

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_at(NULL, 0);
    result = d_assert_standalone(
        ptr == NULL,
        "at_null_vector",
        "NULL vector should return NULL",
        _counter) && result;

    // test 2: empty vector returns NULL
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        ptr    = d_vector_at(vec, 0);
        result = d_assert_standalone(
            ptr == NULL,
            "at_empty_vector",
            "Empty vector should return NULL",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: valid positive index
    vec = d_vector_new_from_args(sizeof(int), 3, 100, 200, 300);

    if (vec)
    {
        ptr    = d_vector_at(vec, 1);
        result = d_assert_standalone(
            ptr != NULL && *(int*)ptr == 200,
            "at_positive_index",
            "Index 1 should return pointer to 200",
            _counter) && result;

        // test 4: valid negative index
        ptr    = d_vector_at(vec, -1);
        result = d_assert_standalone(
            ptr != NULL && *(int*)ptr == 300,
            "at_negative_index",
            "Index -1 should return pointer to 300",
            _counter) && result;

        // test 5: out-of-bounds returns NULL
        ptr    = d_vector_at(vec, 10);
        result = d_assert_standalone(
            ptr == NULL,
            "at_out_of_bounds",
            "Out-of-bounds should return NULL",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_front
  Tests the d_vector_front function.
  Tests the following:
  - NULL vector returns NULL
  - empty vector returns NULL
  - valid front access
*/
bool
d_tests_sa_vector_front
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    void*            ptr;

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_front(NULL);
    result = d_assert_standalone(
        ptr == NULL,
        "front_null_vector",
        "NULL vector should return NULL",
        _counter) && result;

    // test 2: empty vector returns NULL
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        ptr    = d_vector_front(vec);
        result = d_assert_standalone(
            ptr == NULL,
            "front_empty_vector",
            "Empty vector should return NULL",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: valid front access
    vec = d_vector_new_from_args(sizeof(int), 3, 111, 222, 333);

    if (vec)
    {
        ptr    = d_vector_front(vec);
        result = d_assert_standalone(
            ptr != NULL && *(int*)ptr == 111,
            "front_valid",
            "Front should return pointer to 111",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_back
  Tests the d_vector_back function.
  Tests the following:
  - NULL vector returns NULL
  - empty vector returns NULL
  - valid back access
*/
bool
d_tests_sa_vector_back
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    void*            ptr;

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_back(NULL);
    result = d_assert_standalone(
        ptr == NULL,
        "back_null_vector",
        "NULL vector should return NULL",
        _counter) && result;

    // test 2: empty vector returns NULL
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        ptr    = d_vector_back(vec);
        result = d_assert_standalone(
            ptr == NULL,
            "back_empty_vector",
            "Empty vector should return NULL",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: valid back access
    vec = d_vector_new_from_args(sizeof(int), 3, 111, 222, 333);

    if (vec)
    {
        ptr    = d_vector_back(vec);
        result = d_assert_standalone(
            ptr != NULL && *(int*)ptr == 333,
            "back_valid",
            "Back should return pointer to 333",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_data
  Tests the d_vector_data function.
  Tests the following:
  - NULL vector returns NULL
  - valid data access
*/
bool
d_tests_sa_vector_data
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    void*            ptr;

    result = true;

    // test 1: NULL vector returns NULL
    ptr    = d_vector_data(NULL);
    result = d_assert_standalone(
        ptr == NULL,
        "data_null_vector",
        "NULL vector should return NULL",
        _counter) && result;

    // test 2: valid data access
    vec = d_vector_new_from_args(sizeof(int), 3, 1, 2, 3);

    if (vec)
    {
        ptr    = d_vector_data(vec);
        result = d_assert_standalone(
            ptr == vec->elements,
            "data_valid",
            "Data should return elements pointer",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_get
  Tests the d_vector_get function.
  Tests the following:
  - NULL vector rejection
  - NULL out_value rejection
  - empty vector rejection
  - successful get
*/
bool
d_tests_sa_vector_get
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
        d_vector_get(NULL, 0, &out_value) == D_FAILURE,
        "get_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: NULL out_value should fail
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_get(vec, 0, NULL) == D_FAILURE,
            "get_null_out",
            "NULL out_value should return D_FAILURE",
            _counter) && result;

        // test 3: successful get
        out_value = 0;
        result    = d_assert_standalone(
            d_vector_get(vec, 1, &out_value) == D_SUCCESS,
            "get_success",
            "Get should succeed",
            _counter) && result;

        result = d_assert_standalone(
            out_value == 20,
            "get_value",
            "out_value should be 20",
            _counter) && result;

        // test 4: negative index
        out_value = 0;
        result    = d_assert_standalone(
            d_vector_get(vec, -1, &out_value) == D_SUCCESS && out_value == 30,
            "get_negative",
            "Get with -1 should return 30",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_set
  Tests the d_vector_set function.
  Tests the following:
  - NULL vector rejection
  - NULL value rejection
  - empty vector rejection
  - successful set
*/
bool
d_tests_sa_vector_set
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
        d_vector_set(NULL, 0, &value) == D_FAILURE,
        "set_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: NULL value should fail
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_set(vec, 0, NULL) == D_FAILURE,
            "set_null_value",
            "NULL value should return D_FAILURE",
            _counter) && result;

        // test 3: successful set
        value  = 999;
        result = d_assert_standalone(
            d_vector_set(vec, 1, &value) == D_SUCCESS,
            "set_success",
            "Set should succeed",
            _counter) && result;

        result = d_assert_standalone(
            ((int*)vec->elements)[1] == 999,
            "set_value",
            "Element at index 1 should be 999",
            _counter) && result;

        // test 4: negative index
        value  = 888;
        result = d_assert_standalone(
            d_vector_set(vec, -1, &value) == D_SUCCESS,
            "set_negative",
            "Set with negative index should succeed",
            _counter) && result;

        result = d_assert_standalone(
            ((int*)vec->elements)[2] == 888,
            "set_negative_value",
            "Last element should be 888",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_access_all
  Aggregation function that runs all access tests.
*/
bool
d_tests_sa_vector_access_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Access Functions\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_vector_at(_counter) && result;
    result = d_tests_sa_vector_front(_counter) && result;
    result = d_tests_sa_vector_back(_counter) && result;
    result = d_tests_sa_vector_data(_counter) && result;
    result = d_tests_sa_vector_get(_counter) && result;
    result = d_tests_sa_vector_set(_counter) && result;

    return result;
}
