#include ".\functional_common_tests_sa.h"


// helper functions for testing

static bool pred_is_even(const void* _element, void* _context)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value % 2 == 0);
}

static bool pred_is_positive(const void* _element, void* _context)
{
    const int* value;

    (void)_context;

    if (!_element)
    {
        return false;
    }

    value = (const int*)_element;

    return (*value > 0);
}

static bool transform_double(const void* _input,
                              void*       _output,
                              void*       _context)
{
    const int* in;
    int*       out;

    (void)_context;

    if ( (!_input) ||
         (!_output) )
    {
        return false;
    }

    in  = (const int*)_input;
    out = (int*)_output;

    *out = (*in) * 2;

    return true;
}

static bool accumulate_sum(void*       _accumulated,
                            const void* _element,
                            void*       _context)
{
    int*       acc;
    const int* elem;

    (void)_context;

    if ( (!_accumulated) ||
         (!_element) )
    {
        return false;
    }

    acc  = (int*)_accumulated;
    elem = (const int*)_element;

    *acc += *elem;

    return true;
}

static void consumer_increment(void* _element, void* _context)
{
    int* value;

    (void)_context;

    if (!_element)
    {
        return;
    }

    value = (int*)_element;
    (*value)++;

    return;
}


/*
d_tests_sa_fc_map
  Tests the d_functional_map function.
  Tests the following:
  - NULL input rejection
  - NULL output rejection
  - NULL transform rejection
  - zero count/element_size rejection
  - successful transformation
  - output count verification
*/
bool
d_tests_sa_fc_map
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    input[5]  = {1, 2, 3, 4, 5};
    int    output[5] = {0, 0, 0, 0, 0};
    size_t out_count;

    result = true;

    // test 1: NULL input should return NULL
    result = d_assert_standalone(
        d_functional_map(NULL,
                         output,
                         5,
                         sizeof(int),
                         transform_double,
                         NULL) == NULL,
        "map_null_input",
        "NULL input should return NULL",
        _counter) && result;

    // test 2: NULL output should return NULL
    result = d_assert_standalone(
        d_functional_map(input,
                         NULL,
                         5,
                         sizeof(int),
                         transform_double,
                         NULL) == NULL,
        "map_null_output",
        "NULL output should return NULL",
        _counter) && result;

    // test 3: NULL transform should return NULL
    result = d_assert_standalone(
        d_functional_map(input,
                         output,
                         5,
                         sizeof(int),
                         NULL,
                         NULL) == NULL,
        "map_null_transform",
        "NULL transform should return NULL",
        _counter) && result;

    // test 4: zero count should return NULL
    result = d_assert_standalone(
        d_functional_map(input,
                         output,
                         0,
                         sizeof(int),
                         transform_double,
                         NULL) == NULL,
        "map_zero_count",
        "Zero count should return NULL",
        _counter) && result;

    // test 5: successful map - double each element
    out_count = 0;
    result    = d_assert_standalone(
        d_functional_map(input,
                         output,
                         5,
                         sizeof(int),
                         transform_double,
                         &out_count) == output,
        "map_success",
        "Map should succeed and return output pointer",
        _counter) && result;

    result = d_assert_standalone(
        out_count == 5,
        "map_output_count",
        "Output count should be 5",
        _counter) && result;

    {
        bool correct = (output[0] == 2)  && (output[1] == 4)  &&
                       (output[2] == 6)  && (output[3] == 8)  &&
                       (output[4] == 10);

        result = d_assert_standalone(
            correct,
            "map_output_values",
            "Output should be [2, 4, 6, 8, 10]",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_fc_filter
  Tests the d_functional_filter function.
  Tests the following:
  - NULL input rejection
  - NULL output rejection
  - NULL test rejection
  - successful filtering
  - output count verification
*/
bool
d_tests_sa_fc_filter
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    input[5]  = {1, 2, 3, 4, 5};
    int    output[5] = {0, 0, 0, 0, 0};
    size_t out_count;

    result = true;

    // test 1: NULL input should return NULL
    result = d_assert_standalone(
        d_functional_filter(NULL,
                            output,
                            5,
                            sizeof(int),
                            pred_is_even,
                            NULL) == NULL,
        "filter_null_input",
        "NULL input should return NULL",
        _counter) && result;

    // test 2: NULL output should return NULL
    result = d_assert_standalone(
        d_functional_filter(input,
                            NULL,
                            5,
                            sizeof(int),
                            pred_is_even,
                            NULL) == NULL,
        "filter_null_output",
        "NULL output should return NULL",
        _counter) && result;

    // test 3: successful filter - only even numbers
    out_count = 0;
    result    = d_assert_standalone(
        d_functional_filter(input,
                            output,
                            5,
                            sizeof(int),
                            pred_is_even,
                            &out_count) == output,
        "filter_success",
        "Filter should succeed",
        _counter) && result;

    result = d_assert_standalone(
        out_count == 2,
        "filter_output_count",
        "Output count should be 2 (only 2 and 4 are even)",
        _counter) && result;

    {
        bool correct = (output[0] == 2) && (output[1] == 4);

        result = d_assert_standalone(
            correct,
            "filter_output_values",
            "Output should be [2, 4]",
            _counter) && result;
    }

    // test 4: filter with all passing
    {
        int    all_pos[3] = {1, 2, 3};
        int    out[3]     = {0, 0, 0};

        out_count = 0;
        result    = d_assert_standalone(
            d_functional_filter(all_pos,
                                out,
                                3,
                                sizeof(int),
                                pred_is_positive,
                                &out_count) == out,
            "filter_all_pass",
            "Filter with all passing should succeed",
            _counter) && result;

        result = d_assert_standalone(
            out_count == 3,
            "filter_all_pass_count",
            "All 3 elements should pass",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_fc_fold_left
  Tests the d_functional_fold_left function.
  Tests the following:
  - NULL input rejection
  - NULL accumulator rejection
  - NULL combine rejection
  - successful left fold (sum)
  - accumulator updates
*/
bool
d_tests_sa_fc_fold_left
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  input[5] = {1, 2, 3, 4, 5};
    int  acc;

    result = true;

    // test 1: NULL input should return false
    acc    = 0;
    result = d_assert_standalone(
        d_functional_fold_left(NULL,
                               &acc,
                               5,
                               sizeof(int),
                               accumulate_sum,
                               NULL) == false,
        "fold_left_null_input",
        "NULL input should return false",
        _counter) && result;

    // test 2: NULL accumulator should return false
    result = d_assert_standalone(
        d_functional_fold_left(input,
                               NULL,
                               5,
                               sizeof(int),
                               accumulate_sum,
                               NULL) == false,
        "fold_left_null_accumulator",
        "NULL accumulator should return false",
        _counter) && result;

    // test 3: successful fold_left - sum all elements
    acc    = 0;
    result = d_assert_standalone(
        d_functional_fold_left(input,
                               &acc,
                               5,
                               sizeof(int),
                               accumulate_sum,
                               NULL) == true,
        "fold_left_success",
        "Fold left should succeed",
        _counter) && result;

    result = d_assert_standalone(
        acc == 15,
        "fold_left_sum_result",
        "Sum should be 15 (1+2+3+4+5)",
        _counter) && result;

    // test 4: fold_left with initial accumulator value
    acc    = 100;
    result = d_assert_standalone(
        d_functional_fold_left(input,
                               &acc,
                               5,
                               sizeof(int),
                               accumulate_sum,
                               NULL) == true,
        "fold_left_with_init",
        "Fold left with initial value should succeed",
        _counter) && result;

    result = d_assert_standalone(
        acc == 115,
        "fold_left_with_init_result",
        "Sum should be 115 (100+1+2+3+4+5)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_fold_right
  Tests the d_functional_fold_right function.
  Tests the following:
  - successful right fold
  - accumulator updates from right to left
*/
bool
d_tests_sa_fc_fold_right
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  input[5] = {1, 2, 3, 4, 5};
    int  acc;

    result = true;

    // test 1: successful fold_right - sum all elements
    acc    = 0;
    result = d_assert_standalone(
        d_functional_fold_right(input,
                                &acc,
                                5,
                                sizeof(int),
                                accumulate_sum,
                                NULL) == true,
        "fold_right_success",
        "Fold right should succeed",
        _counter) && result;

    result = d_assert_standalone(
        acc == 15,
        "fold_right_sum_result",
        "Sum should be 15 (5+4+3+2+1)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_for_each
  Tests the d_functional_for_each and d_functional_for_each_const functions.
  Tests the following:
  - NULL input rejection
  - NULL apply rejection
  - successful for_each application
  - side effects verification
*/
bool
d_tests_sa_fc_for_each
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  input[5] = {1, 2, 3, 4, 5};

    result = true;

    // test 1: NULL input should not crash
    d_functional_for_each(NULL, 5, sizeof(int), consumer_increment, NULL);
    result = d_assert_standalone(
        true,
        "for_each_null_input",
        "NULL input should not crash",
        _counter) && result;

    // test 2: NULL apply should not crash
    d_functional_for_each(input, 5, sizeof(int), NULL, NULL);
    result = d_assert_standalone(
        true,
        "for_each_null_apply",
        "NULL apply should not crash",
        _counter) && result;

    // test 3: successful for_each - increment all elements
    d_functional_for_each(input, 5, sizeof(int), consumer_increment, NULL);

    {
        bool correct = (input[0] == 2) && (input[1] == 3) &&
                       (input[2] == 4) && (input[3] == 5) &&
                       (input[4] == 6);

        result = d_assert_standalone(
            correct,
            "for_each_increment",
            "All elements should be incremented to [2, 3, 4, 5, 6]",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_fc_higher_order_all
  Aggregation function that runs all higher-order function tests.
*/
bool
d_tests_sa_fc_higher_order_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Higher-Order Functions\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_fc_map(_counter) && result;
    result = d_tests_sa_fc_filter(_counter) && result;
    result = d_tests_sa_fc_fold_left(_counter) && result;
    result = d_tests_sa_fc_fold_right(_counter) && result;
    result = d_tests_sa_fc_for_each(_counter) && result;

    return result;
}
