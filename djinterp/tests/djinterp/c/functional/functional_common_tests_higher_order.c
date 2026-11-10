/*******************************************************************************
* djinterp [functional]                   functional_common_tests_higher_order.c
*
*
* path:      /tests/djinterp/c/functional/functional_common_tests_higher_order.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./functional_common_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// helper functions for testing

static
bool pred_is_even(
    const void* _element,
    void*       _context
)
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

static
bool pred_is_positive(
    const void* _element,
    void*       _context
)
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

static
bool transform_double(
    const void* _input,
    void*       _output,
    void*       _context
)
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

static
bool accumulate_sum(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
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

static
bool transform_fail_on_3(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    if ( (!_input) ||
         (!_output) ||
         (*(const int*)_input == 3) )
    {
        return false;
    }

    *(int*)_output = *(const int*)_input;

    return true;
}

static
bool accumulate_digits(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_context;

    if ( (!_accumulated) ||
         (!_element) )
    {
        return false;
    }

    // acc * 10 + element: the order of the elements shows in the digits
    *(int*)_accumulated = (*(int*)_accumulated * 10) + *(const int*)_element;

    return true;
}

static
void consumer_increment(
    void* _element,
    void* _context
)
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
d_tests_fc_map
  Tests the d_functional_map function.
  Tests the following:
  - NULL input, output or transform rejected while count > 0
  - an empty input accepted with NULL arrays
  - each element transformed into its slot, in order
  - a failing transform stops the map, keeping what it wrote
*/
bool d_tests_fc_map(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  input[5]  = {1, 2, 3, 4, 5};
    int  output[5] = {0, 0, 0, 0, 0};
    int  part[5]   = {0, 0, 0, 0, 0};

    d_test_suite_test(_suite, "fc_map");

    result = true;

    // test 1: each missing argument is refused
    result = D_TEST_CHECK_MSG(_suite,
                              "map_null_input",
                              !d_functional_map(NULL, output, 5, sizeof(int),
                                                transform_double, NULL),
                              "NULL input should return false") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "map_null_output",
                              !d_functional_map(input, NULL, 5, sizeof(int),
                                                transform_double, NULL),
                              "NULL output should return false") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "map_null_transform",
                              !d_functional_map(input, output, 5, sizeof(int),
                                                NULL, NULL),
                              "NULL transform should return false") && result;

    // test 2: nothing to map needs no arrays
    result = D_TEST_CHECK_MSG(_suite,
                              "map_zero_count",
                              d_functional_map(NULL, NULL, 0, sizeof(int),
                                               transform_double, NULL),
                              "an empty input should succeed") && result;

    // test 3: double each element
    result = D_TEST_CHECK_MSG(_suite,
                              "map_success",
                              d_functional_map(input, output, 5, sizeof(int),
                                               transform_double, NULL),
                              "map should succeed") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "map_output_values",
                              (output[0] == 2) && (output[1] == 4) &&
                              (output[2] == 6) && (output[3] == 8) &&
                              (output[4] == 10),
                              "output should be [2, 4, 6, 8, 10]") && result;

    // test 4: the transform fails at 3, after writing 1 and 2
    result = D_TEST_CHECK_MSG(_suite,
                              "map_stops_on_failure",
                              (!d_functional_map(input, part, 5, sizeof(int),
                                                 transform_fail_on_3, NULL)) &&
                              (part[0] == 1) && (part[1] == 2) &&
                              (part[2] == 0),
                              "should fail, keeping [1, 2]") && result;

    return result;
}


/*
d_tests_fc_filter
  Tests the d_functional_filter function.
  Tests the following:
  - NULL input, output, test or out_count rejected while count > 0, with
    *_out_count untouched
  - an empty input accepted with NULL arrays, keeping 0
  - the kept elements packed in order, and their count
  - every element kept
  - filtering in place (output is input)
*/
bool d_tests_fc_filter(
    struct dj_test_suite* _suite
)
{
    bool   result;
    int    input[5]    = {1, 2, 3, 4, 5};
    int    output[5]   = {0, 0, 0, 0, 0};
    int    all_pos[3]  = {1, 2, 3};
    int    in_place[6] = {1, 2, 3, 4, 5, 6};
    size_t out_count;

    d_test_suite_test(_suite, "fc_filter");

    result = true;

    // test 1: each missing argument is refused, out_count untouched
    out_count = 99u;
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_null_input",
                              (!d_functional_filter(NULL, output, 5,
                                                    sizeof(int), pred_is_even,
                                                    NULL, &out_count)) &&
                              (out_count == 99u),
                              "NULL input refused, count kept") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_null_output",
                              (!d_functional_filter(input, NULL, 5,
                                                    sizeof(int), pred_is_even,
                                                    NULL, &out_count)) &&
                              (out_count == 99u),
                              "NULL output refused, count kept") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_null_test",
                              (!d_functional_filter(input, output, 5,
                                                    sizeof(int), NULL,
                                                    NULL, &out_count)) &&
                              (out_count == 99u),
                              "NULL test refused, count kept") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_null_out_count",
                              !d_functional_filter(input, output, 5,
                                                   sizeof(int), pred_is_even,
                                                   NULL, NULL),
                              "a NULL out_count is refused") && result;

    // test 2: nothing to filter needs no arrays
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_empty",
                              (d_functional_filter(NULL, NULL, 0,
                                                   sizeof(int), pred_is_even,
                                                   NULL, &out_count)) &&
                              (out_count == 0u),
                              "empty input keeps 0") && result;

    // test 3: only the even numbers, packed in order
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_success",
                              d_functional_filter(input, output, 5,
                                                  sizeof(int), pred_is_even,
                                                  NULL, &out_count),
                              "filtering the evens succeeds") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_output_count",
                              out_count == 2u,
                              "2 of the 5 are even") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_output_values",
                              (output[0] == 2) &&
                              (output[1] == 4),
                              "the output is [2, 4]") && result;

    // test 4: every element kept
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_all_pass",
                              (d_functional_filter(all_pos, output, 3,
                                                   sizeof(int),
                                                   pred_is_positive,
                                                   NULL, &out_count)) &&
                              (out_count == 3u) &&
                              (output[0] == 1) &&
                              (output[2] == 3),
                              "all 3 positives are kept, in order") && result;

    // test 5: in place, the input becomes its kept prefix
    result = D_TEST_CHECK_MSG(_suite,
                              "filter_in_place",
                              (d_functional_filter(in_place, in_place, 6,
                                                   sizeof(int), pred_is_even,
                                                   NULL, &out_count)) &&
                              (out_count == 3u) &&
                              (in_place[0] == 2) &&
                              (in_place[1] == 4) &&
                              (in_place[2] == 6),
                              "in place, [1..6] becomes [2, 4, 6]") && result;

    return result;
}


/*
d_tests_fc_fold_left
  Tests the d_functional_fold_left function.
  Tests the following:
  - NULL input, accumulator or combine rejected
  - successful left fold (sum), from a given initial value
  - the elements combined first to last
*/
bool d_tests_fc_fold_left(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  input[5] = {1, 2, 3, 4, 5};
    int  acc;

    d_test_suite_test(_suite, "fc_fold_left");

    result = true;

    // test 1: each missing argument is refused
    acc = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_left_null_input",
                              !d_functional_fold_left(NULL, 5, sizeof(int),
                                                      &acc, accumulate_sum,
                                                      NULL),
                              "NULL input should return false") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_left_null_accumulator",
                              !d_functional_fold_left(input, 5, sizeof(int),
                                                      NULL, accumulate_sum,
                                                      NULL),
                              "NULL accumulator should return false") && result;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_left_null_combine",
                              !d_functional_fold_left(input, 5, sizeof(int),
                                                      &acc, NULL, NULL),
                              "NULL combine should return false") && result;

    // test 2: sum all elements, from 0 and from 100
    acc = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_left_sum",
                              (d_functional_fold_left(input, 5, sizeof(int),
                                                      &acc, accumulate_sum,
                                                      NULL)) &&
                              (acc == 15),
                              "sum should be 15 (1+2+3+4+5)") && result;
    acc = 100;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_left_with_init",
                              (d_functional_fold_left(input, 5, sizeof(int),
                                                      &acc, accumulate_sum,
                                                      NULL)) &&
                              (acc == 115),
                              "sum should be 115 (100+1+2+3+4+5)") && result;

    // test 3: first to last
    acc = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_left_order",
                              (d_functional_fold_left(input, 5, sizeof(int),
                                                      &acc, accumulate_digits,
                                                      NULL)) &&
                              (acc == 12345),
                              "digits should read 12345") && result;

    return result;
}


/*
d_tests_fc_fold_right
  Tests the d_functional_fold_right function.
  Tests the following:
  - successful right fold (sum)
  - the elements combined last to first
*/
bool d_tests_fc_fold_right(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  input[5] = {1, 2, 3, 4, 5};
    int  acc;

    d_test_suite_test(_suite, "fc_fold_right");

    result = true;

    // test 1: sum all elements
    acc = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_right_sum",
                              (d_functional_fold_right(input, 5, sizeof(int),
                                                       &acc, accumulate_sum,
                                                       NULL)) &&
                              (acc == 15),
                              "sum should be 15 (5+4+3+2+1)") && result;

    // test 2: last to first
    acc = 0;
    result = D_TEST_CHECK_MSG(_suite,
                              "fold_right_order",
                              (d_functional_fold_right(input, 5, sizeof(int),
                                                       &acc, accumulate_digits,
                                                       NULL)) &&
                              (acc == 54321),
                              "digits should read 54321") && result;

    return result;
}


/*
d_tests_fc_for_each
  Tests the d_functional_for_each and d_functional_for_each_const functions.
  Tests the following:
  - NULL input rejection
  - NULL apply rejection
  - successful for_each application
  - side effects verification
*/
bool d_tests_fc_for_each(
    struct dj_test_suite* _suite
)
{
    bool result;
    int  input[5] = {1, 2, 3, 4, 5};

    d_test_suite_test(_suite, "fc_for_each");

    result = true;

    // test 1: NULL input should not crash
    d_functional_for_each(NULL, 5, sizeof(int), consumer_increment, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "for_each_null_input",
                              true,
                              "NULL input should not crash") && result;

    // test 2: NULL apply should not crash
    d_functional_for_each(input, 5, sizeof(int), NULL, NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "for_each_null_apply",
                              true,
                              "NULL apply should not crash") && result;

    // test 3: successful for_each - increment all elements
    d_functional_for_each(input, 5, sizeof(int), consumer_increment, NULL);

    {
        bool correct = (input[0] == 2) && (input[1] == 3) &&
                       (input[2] == 4) && (input[3] == 5) &&
                       (input[4] == 6);

        result = D_TEST_CHECK_MSG(_suite,
                                  "for_each_increment",
                                  correct,
                                  "elements should be [2, 3, 4, 5, 6]") && result;
    }

    return result;
}


/*
d_tests_fc_higher_order_all
  Aggregation function that runs all higher-order function tests.
*/
bool d_tests_fc_higher_order_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "fc_higher_order_all");

    result = true;

    result = d_tests_fc_map(_suite) && result;
    result = d_tests_fc_filter(_suite) && result;
    result = d_tests_fc_fold_left(_suite) && result;
    result = d_tests_fc_fold_right(_suite) && result;
    result = d_tests_fc_for_each(_suite) && result;

    return result;
}
