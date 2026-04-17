#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_append_element
  Tests the d_vector_append_element function.
  Tests the following:
  - NULL vector rejection
  - successful append of single element
*/
bool
d_tests_sa_vector_append_element
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
        d_vector_append_element(NULL, &value) == D_FAILURE,
        "append_element_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful append to empty vector
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        value  = 100;
        result = d_assert_standalone(
            d_vector_append_element(vec, &value) == D_SUCCESS,
            "append_element_success",
            "Append element should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 1 && ((int*)vec->elements)[0] == 100,
            "append_element_result",
            "Count=1, element[0]=100",
            _counter) && result;

        // test 3: append to non-empty vector
        value  = 200;
        result = d_assert_standalone(
            d_vector_append_element(vec, &value) == D_SUCCESS,
            "append_element_nonempty_success",
            "Append to non-empty should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 2 && ((int*)vec->elements)[1] == 200,
            "append_element_nonempty_result",
            "Count=2, element[1]=200",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_append_elements
  Tests the d_vector_append_elements function.
  Tests the following:
  - NULL vector rejection
  - zero count no-op
  - NULL source with non-zero count failure
  - successful append
*/
bool
d_tests_sa_vector_append_elements
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              source[3] = {40, 50, 60};

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_append_elements(NULL, source, 3) == D_FAILURE,
        "append_elements_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: zero count no-op
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_append_elements(vec, source, 0) == D_SUCCESS,
            "append_elements_zero_count",
            "Zero count should return D_SUCCESS",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 3,
            "append_elements_zero_count_unchanged",
            "Count should remain unchanged",
            _counter) && result;

        // test 3: successful append
        result = d_assert_standalone(
            d_vector_append_elements(vec, source, 3) == D_SUCCESS,
            "append_elements_success",
            "Append should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 6,
            "append_elements_count",
            "Count should be 6",
            _counter) && result;

        {
            int* arr = (int*)vec->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30) &&
                           (arr[3] == 40) && (arr[4] == 50) && (arr[5] == 60);

            result = d_assert_standalone(
                correct,
                "append_elements_values",
                "Elements should be [10, 20, 30, 40, 50, 60]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_append_vector
  Tests the d_vector_append_vector function.
  Tests the following:
  - NULL destination rejection
  - NULL source rejection
  - element size mismatch rejection
  - successful vector append
*/
bool
d_tests_sa_vector_append_vector
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* dest;
    struct d_vector* src;
    struct d_vector* wrong_size;

    result = true;

    // test 1: NULL destination should fail
    src    = d_vector_new_from_args(sizeof(int), 2, 40, 50);
    result = d_assert_standalone(
        d_vector_append_vector(NULL, src) == D_FAILURE,
        "append_vector_null_dest",
        "NULL destination should return D_FAILURE",
        _counter) && result;

    if (src)
    {
        d_vector_free(src);
    }

    // test 2: NULL source should fail
    dest   = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    result = d_assert_standalone(
        d_vector_append_vector(dest, NULL) == D_FAILURE,
        "append_vector_null_src",
        "NULL source should return D_FAILURE",
        _counter) && result;

    if (dest)
    {
        d_vector_free(dest);
    }

    // test 3: element size mismatch should fail
    dest       = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    wrong_size = d_vector_new(sizeof(double), 5);

    if ( (dest) &&
         (wrong_size) )
    {
        result = d_assert_standalone(
            d_vector_append_vector(dest, wrong_size) == D_FAILURE,
            "append_vector_size_mismatch",
            "Element size mismatch should return D_FAILURE",
            _counter) && result;

        d_vector_free(dest);
        d_vector_free(wrong_size);
    }
    else
    {
        if (dest)
        {
            d_vector_free(dest);
        }

        if (wrong_size)
        {
            d_vector_free(wrong_size);
        }
    }

    // test 4: successful vector append
    dest = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    src  = d_vector_new_from_args(sizeof(int), 3, 30, 40, 50);

    if ( (dest) &&
         (src) )
    {
        result = d_assert_standalone(
            d_vector_append_vector(dest, src) == D_SUCCESS,
            "append_vector_success",
            "Append vector should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest->count == 5,
            "append_vector_count",
            "Destination count should be 5",
            _counter) && result;

        {
            int* arr = (int*)dest->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) && (arr[2] == 30) &&
                           (arr[3] == 40) && (arr[4] == 50);

            result = d_assert_standalone(
                correct,
                "append_vector_values",
                "Elements should be [10, 20, 30, 40, 50]",
                _counter) && result;
        }

        d_vector_free(dest);
        d_vector_free(src);
    }

    return result;
}


/*
d_tests_sa_vector_prepend_element
  Tests the d_vector_prepend_element function.
  Tests the following:
  - NULL vector rejection
  - successful prepend of single element
*/
bool
d_tests_sa_vector_prepend_element
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
        d_vector_prepend_element(NULL, &value) == D_FAILURE,
        "prepend_element_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: successful prepend to empty vector
    vec = d_vector_new(sizeof(int), 10);

    if (vec)
    {
        value  = 100;
        result = d_assert_standalone(
            d_vector_prepend_element(vec, &value) == D_SUCCESS,
            "prepend_element_success",
            "Prepend element should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 1 && ((int*)vec->elements)[0] == 100,
            "prepend_element_result",
            "Count=1, element[0]=100",
            _counter) && result;

        // test 3: prepend to non-empty vector (shifts existing)
        value  = 50;
        result = d_assert_standalone(
            d_vector_prepend_element(vec, &value) == D_SUCCESS,
            "prepend_element_nonempty_success",
            "Prepend to non-empty should succeed",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 50) && (arr[1] == 100);

            result = d_assert_standalone(
                correct && vec->count == 2,
                "prepend_element_shifted",
                "Elements should be [50, 100]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_prepend_elements
  Tests the d_vector_prepend_elements function.
  Tests the following:
  - NULL vector rejection
  - zero count no-op
  - successful prepend
*/
bool
d_tests_sa_vector_prepend_elements
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              source[2] = {1, 2};

    result = true;

    // test 1: NULL vector should fail
    result = d_assert_standalone(
        d_vector_prepend_elements(NULL, source, 2) == D_FAILURE,
        "prepend_elements_null_vector",
        "NULL vector should return D_FAILURE",
        _counter) && result;

    // test 2: zero count no-op
    vec = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (vec)
    {
        result = d_assert_standalone(
            d_vector_prepend_elements(vec, source, 0) == D_SUCCESS,
            "prepend_elements_zero_count",
            "Zero count should return D_SUCCESS",
            _counter) && result;

        // test 3: successful prepend
        result = d_assert_standalone(
            d_vector_prepend_elements(vec, source, 2) == D_SUCCESS,
            "prepend_elements_success",
            "Prepend should succeed",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 5,
            "prepend_elements_count",
            "Count should be 5",
            _counter) && result;

        {
            int* arr = (int*)vec->elements;
            bool correct = (arr[0] == 1) && (arr[1] == 2) && (arr[2] == 10) &&
                           (arr[3] == 20) && (arr[4] == 30);

            result = d_assert_standalone(
                correct,
                "prepend_elements_values",
                "Elements should be [1, 2, 10, 20, 30]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_prepend_vector
  Tests the d_vector_prepend_vector function.
  Tests the following:
  - NULL destination rejection
  - NULL source rejection
  - successful vector prepend
*/
bool
d_tests_sa_vector_prepend_vector
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* dest;
    struct d_vector* src;

    result = true;

    // test 1: NULL destination should fail
    src    = d_vector_new_from_args(sizeof(int), 2, 1, 2);
    result = d_assert_standalone(
        d_vector_prepend_vector(NULL, src) == D_FAILURE,
        "prepend_vector_null_dest",
        "NULL destination should return D_FAILURE",
        _counter) && result;

    if (src)
    {
        d_vector_free(src);
    }

    // test 2: NULL source should fail
    dest   = d_vector_new_from_args(sizeof(int), 2, 10, 20);
    result = d_assert_standalone(
        d_vector_prepend_vector(dest, NULL) == D_FAILURE,
        "prepend_vector_null_src",
        "NULL source should return D_FAILURE",
        _counter) && result;

    if (dest)
    {
        d_vector_free(dest);
    }

    // test 3: successful vector prepend
    dest = d_vector_new_from_args(sizeof(int), 2, 30, 40);
    src  = d_vector_new_from_args(sizeof(int), 2, 10, 20);

    if ( (dest) &&
         (src) )
    {
        result = d_assert_standalone(
            d_vector_prepend_vector(dest, src) == D_SUCCESS,
            "prepend_vector_success",
            "Prepend vector should succeed",
            _counter) && result;

        result = d_assert_standalone(
            dest->count == 4,
            "prepend_vector_count",
            "Destination count should be 4",
            _counter) && result;

        {
            int* arr = (int*)dest->elements;
            bool correct = (arr[0] == 10) && (arr[1] == 20) &&
                           (arr[2] == 30) && (arr[3] == 40);

            result = d_assert_standalone(
                correct,
                "prepend_vector_values",
                "Elements should be [10, 20, 30, 40]",
                _counter) && result;
        }

        d_vector_free(dest);
        d_vector_free(src);
    }

    return result;
}


/*
d_tests_sa_vector_append_all
  Aggregation function that runs all append/prepend tests.
*/
bool
d_tests_sa_vector_append_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Append/Prepend Functions\n");
    printf("  -----------------------------------\n");

    result = d_tests_sa_vector_append_element(_counter) && result;
    result = d_tests_sa_vector_append_elements(_counter) && result;
    result = d_tests_sa_vector_append_vector(_counter) && result;
    result = d_tests_sa_vector_prepend_element(_counter) && result;
    result = d_tests_sa_vector_prepend_elements(_counter) && result;
    result = d_tests_sa_vector_prepend_vector(_counter) && result;

    return result;
}
