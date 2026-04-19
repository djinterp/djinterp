#include "./vector_tests_sa.h"


/*
d_tests_sa_vector_new
  Tests the d_vector_new constructor function.
  Tests the following:
  - zero element_size rejection
  - successful creation with various element sizes
  - zero initial capacity handling
  - struct initialization verification
*/
bool
d_tests_sa_vector_new
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new(0, 10);
    result = d_assert_standalone(
        vec == NULL,
        "new_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 2: successful creation with int elements
    vec    = d_vector_new(sizeof(int), 10);
    result = d_assert_standalone(
        vec != NULL,
        "new_int_success",
        "d_vector_new with sizeof(int) should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->element_size == sizeof(int),
            "new_int_element_size",
            "element_size should be sizeof(int)",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity == 10,
            "new_int_capacity",
            "capacity should be 10",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 0,
            "new_int_count",
            "count should be 0",
            _counter) && result;

        result = d_assert_standalone(
            vec->elements != NULL,
            "new_int_elements_allocated",
            "elements should be allocated",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: zero initial capacity should succeed
    vec    = d_vector_new(sizeof(int), 0);
    result = d_assert_standalone(
        vec != NULL,
        "new_zero_capacity_success",
        "Zero initial capacity should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->capacity == 0 && vec->count == 0,
            "new_zero_capacity_values",
            "Zero capacity should have count=0, capacity=0",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 4: large struct element size
    {
        struct large_struct
        {
            int    data[100];
            double values[50];
        };

        vec    = d_vector_new(sizeof(struct large_struct), 5);
        result = d_assert_standalone(
            vec != NULL,
            "new_large_struct_success",
            "d_vector_new with large struct should succeed",
            _counter) && result;

        if (vec)
        {
            result = d_assert_standalone(
                vec->element_size == sizeof(struct large_struct),
                "new_large_struct_element_size",
                "element_size should match large struct size",
                _counter) && result;

            d_vector_free(vec);
        }
    }

    return result;
}


/*
d_tests_sa_vector_new_default
  Tests the d_vector_new_default constructor function.
  Tests the following:
  - zero element_size rejection
  - successful creation with default capacity
  - default capacity value verification
*/
bool
d_tests_sa_vector_new_default
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new_default(0);
    result = d_assert_standalone(
        vec == NULL,
        "new_default_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 2: successful creation with default capacity
    vec    = d_vector_new_default(sizeof(int));
    result = d_assert_standalone(
        vec != NULL,
        "new_default_success",
        "d_vector_new_default should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->capacity == D_VECTOR_DEFAULT_CAPACITY,
            "new_default_capacity",
            "capacity should be D_VECTOR_DEFAULT_CAPACITY",
            _counter) && result;

        result = d_assert_standalone(
            vec->count == 0,
            "new_default_count",
            "count should be 0",
            _counter) && result;

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_new_from_array
  Tests the d_vector_new_from_array constructor function.
  Tests the following:
  - zero element_size rejection
  - NULL source with non-zero count rejection
  - NULL source with zero count handling
  - successful creation from valid array
  - elements correctly copied
*/
bool
d_tests_sa_vector_new_from_array
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              source[5] = {10, 20, 30, 40, 50};

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new_from_array(0, source, 5);
    result = d_assert_standalone(
        vec == NULL,
        "new_from_array_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 2: NULL source with non-zero count should return NULL
    vec    = d_vector_new_from_array(sizeof(int), NULL, 5);
    result = d_assert_standalone(
        vec == NULL,
        "new_from_array_null_source",
        "NULL source with non-zero count should return NULL",
        _counter) && result;

    // test 3: NULL source with zero count should succeed
    vec    = d_vector_new_from_array(sizeof(int), NULL, 0);
    result = d_assert_standalone(
        vec != NULL,
        "new_from_array_null_zero_count",
        "NULL source with zero count should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 0,
            "new_from_array_null_zero_count_empty",
            "Should create empty vector",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 4: successful creation from valid array
    vec    = d_vector_new_from_array(sizeof(int), source, 5);
    result = d_assert_standalone(
        vec != NULL,
        "new_from_array_valid_success",
        "d_vector_new_from_array with valid array should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 5,
            "new_from_array_valid_count",
            "count should be 5",
            _counter) && result;

        result = d_assert_standalone(
            vec->capacity >= 5,
            "new_from_array_valid_capacity",
            "capacity should be at least 5",
            _counter) && result;

        // test 5: verify elements copied correctly
        {
            int*   arr       = (int*)vec->elements;
            bool   all_match = true;
            size_t i;

            for (i = 0; i < 5; i++)
            {
                if (arr[i] != source[i])
                {
                    all_match = false;

                    break;
                }
            }

            result = d_assert_standalone(
                all_match,
                "new_from_array_elements_copied",
                "All elements should be correctly copied",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_new_from_args
  Tests the d_vector_new_from_args constructor function.
  Tests the following:
  - zero element_size rejection
  - zero arg_count handling
  - successful creation with variadic arguments
*/
bool
d_tests_sa_vector_new_from_args
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;

    result = true;

    // test 1: zero element_size should return NULL
    vec    = d_vector_new_from_args(0, 3, 1, 2, 3);
    result = d_assert_standalone(
        vec == NULL,
        "new_from_args_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 2: zero arg_count should succeed with empty vector
    vec    = d_vector_new_from_args(sizeof(int), 0);
    result = d_assert_standalone(
        vec != NULL,
        "new_from_args_zero_count_success",
        "Zero arg_count should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 0,
            "new_from_args_zero_count_empty",
            "Should create empty vector",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: successful creation with int arguments
    vec    = d_vector_new_from_args(sizeof(int), 3, 100, 200, 300);
    result = d_assert_standalone(
        vec != NULL,
        "new_from_args_int_success",
        "d_vector_new_from_args with ints should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 3,
            "new_from_args_int_count",
            "count should be 3",
            _counter) && result;

        {
            int* arr     = (int*)vec->elements;
            bool correct = (arr[0] == 100) && (arr[1] == 200) && (arr[2] == 300);

            result = d_assert_standalone(
                correct,
                "new_from_args_int_values",
                "Values should be [100, 200, 300]",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_new_copy
  Tests the d_vector_new_copy constructor function.
  Tests the following:
  - NULL source rejection
  - successful deep copy creation
  - all properties copied correctly
  - elements independently copied (not shared)
*/
bool
d_tests_sa_vector_new_copy
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* original;
    struct d_vector* copy;

    result = true;

    // test 1: NULL source should return NULL
    copy   = d_vector_new_copy(NULL);
    result = d_assert_standalone(
        copy == NULL,
        "new_copy_null_source",
        "NULL source should return NULL",
        _counter) && result;

    // test 2: successful copy of non-empty vector
    original = d_vector_new_from_args(sizeof(int), 3, 10, 20, 30);

    if (original)
    {
        copy   = d_vector_new_copy(original);
        result = d_assert_standalone(
            copy != NULL,
            "new_copy_success",
            "d_vector_new_copy should succeed",
            _counter) && result;

        if (copy)
        {
            result = d_assert_standalone(
                copy->count == original->count,
                "new_copy_count_match",
                "copy count should match original",
                _counter) && result;

            result = d_assert_standalone(
                copy->element_size == original->element_size,
                "new_copy_element_size_match",
                "copy element_size should match original",
                _counter) && result;

            result = d_assert_standalone(
                copy->elements != original->elements,
                "new_copy_elements_independent",
                "copy elements should be independent allocation",
                _counter) && result;

            // verify elements match
            {
                int* orig_arr = (int*)original->elements;
                int* copy_arr = (int*)copy->elements;
                bool match    = (copy_arr[0] == orig_arr[0]) &&
                                (copy_arr[1] == orig_arr[1]) &&
                                (copy_arr[2] == orig_arr[2]);

                result = d_assert_standalone(
                    match,
                    "new_copy_elements_match",
                    "copy elements should match original values",
                    _counter) && result;
            }

            d_vector_free(copy);
        }

        d_vector_free(original);
    }

    // test 3: copy of empty vector
    original = d_vector_new(sizeof(int), 10);

    if (original)
    {
        copy   = d_vector_new_copy(original);
        result = d_assert_standalone(
            copy != NULL,
            "new_copy_empty_success",
            "Copy of empty vector should succeed",
            _counter) && result;

        if (copy)
        {
            result = d_assert_standalone(
                copy->count == 0,
                "new_copy_empty_count",
                "Copy of empty vector should have count 0",
                _counter) && result;

            d_vector_free(copy);
        }

        d_vector_free(original);
    }

    return result;
}


/*
d_tests_sa_vector_new_fill
  Tests the d_vector_new_fill constructor function.
  Tests the following:
  - zero element_size rejection
  - zero count handling
  - NULL fill value handling
  - successful fill creation
  - all elements match fill value
*/
bool
d_tests_sa_vector_new_fill
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct d_vector* vec;
    int              fill_value;

    result = true;

    // test 1: zero element_size should return NULL
    fill_value = 42;
    vec        = d_vector_new_fill(0, 5, &fill_value);
    result     = d_assert_standalone(
        vec == NULL,
        "new_fill_zero_element_size",
        "Zero element_size should return NULL",
        _counter) && result;

    // test 2: zero count should succeed with empty vector
    vec    = d_vector_new_fill(sizeof(int), 0, &fill_value);
    result = d_assert_standalone(
        vec != NULL,
        "new_fill_zero_count_success",
        "Zero count should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 0,
            "new_fill_zero_count_empty",
            "Should create empty vector",
            _counter) && result;

        d_vector_free(vec);
    }

    // test 3: NULL fill value with non-zero count should return NULL
    vec    = d_vector_new_fill(sizeof(int), 5, NULL);
    result = d_assert_standalone(
        vec == NULL,
        "new_fill_null_value",
        "NULL fill value with non-zero count should return NULL",
        _counter) && result;

    // test 4: successful fill creation
    fill_value = 777;
    vec        = d_vector_new_fill(sizeof(int), 5, &fill_value);
    result     = d_assert_standalone(
        vec != NULL,
        "new_fill_valid_success",
        "d_vector_new_fill with valid params should succeed",
        _counter) && result;

    if (vec)
    {
        result = d_assert_standalone(
            vec->count == 5,
            "new_fill_valid_count",
            "count should be 5",
            _counter) && result;

        // verify all elements match fill value
        {
            int*   arr       = (int*)vec->elements;
            bool   all_match = true;
            size_t i;

            for (i = 0; i < 5; i++)
            {
                if (arr[i] != fill_value)
                {
                    all_match = false;

                    break;
                }
            }

            result = d_assert_standalone(
                all_match,
                "new_fill_all_elements_match",
                "All elements should match fill value (777)",
                _counter) && result;
        }

        d_vector_free(vec);
    }

    return result;
}


/*
d_tests_sa_vector_constructor_all
  Aggregation function that runs all constructor tests.
*/
bool
d_tests_sa_vector_constructor_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constructor Functions\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_vector_new(_counter) && result;
    result = d_tests_sa_vector_new_default(_counter) && result;
    result = d_tests_sa_vector_new_from_array(_counter) && result;
    result = d_tests_sa_vector_new_from_args(_counter) && result;
    result = d_tests_sa_vector_new_copy(_counter) && result;
    result = d_tests_sa_vector_new_fill(_counter) && result;

    return result;
}
