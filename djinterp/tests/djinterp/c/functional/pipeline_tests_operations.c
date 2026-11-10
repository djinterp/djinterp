/*******************************************************************************
* djinterp [functional]                              pipeline_tests_operations.c
*
*
* path:      /tests/djinterp/c/functional/pipeline_tests_operations.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.19
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./pipeline_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * TEST HELPERS: callbacks for pipeline operations
 *****************************************************************************/

/*
test_helper_double_int
  Transformer: multiplies each int by 2.
*/
static
bool test_helper_double_int(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = (*(const int*)_input) * 2;

    return true;
}


/*
test_helper_add_context_int
  Transformer: adds the int pointed to by _context to the input.
*/
static
bool test_helper_add_context_int(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    int offset;

    offset = _context ? *(int*)_context : 0;
    *(int*)_output = (*(const int*)_input) + offset;

    return true;
}


/*
test_helper_fail_transformer
  Transformer: always fails (returns false).
*/
static
bool test_helper_fail_transformer(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_input;
    (void)_output;
    (void)_context;

    return false;
}


/*
test_helper_fail_on_third
  Transformer: fails on the third invocation.
*/
static
bool test_helper_fail_on_third(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    size_t* call_count;

    call_count = (size_t*)_context;
    (*call_count)++;

    if (*call_count == 3)
    {
        return false;
    }

    *(int*)_output = *(const int*)_input;

    return true;
}


/*
test_helper_is_even
  Predicate: returns true if the int is even.
*/
static
bool test_helper_is_even(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) % 2 == 0;
}


/*
test_helper_greater_than_context
  Predicate: returns true if the int is greater than *_context.
*/
static
bool test_helper_greater_than_context(
    const void* _element,
    void*       _context
)
{
    int threshold;

    threshold = *(int*)_context;

    return (*(const int*)_element) > threshold;
}


/*
test_helper_always_true
  Predicate: always returns true.
*/
static
bool test_helper_always_true(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return true;
}


/*
test_helper_always_false
  Predicate: always returns false.
*/
static
bool test_helper_always_false(
    const void* _element,
    void*       _context
)
{
    (void)_element;
    (void)_context;

    return false;
}


/*
test_helper_sum_accumulator
  Accumulator: adds each int element to the int accumulator.
*/
static
bool test_helper_sum_accumulator(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_context;

    *(int*)_accumulated += *(const int*)_element;

    return true;
}


/*
test_helper_fail_accumulator
  Accumulator: always fails.
*/
static
bool test_helper_fail_accumulator(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_accumulated;
    (void)_element;
    (void)_context;

    return false;
}


/*
test_helper_product_accumulator
  Accumulator: multiplies accumulator by each int element.
*/
static
bool test_helper_product_accumulator(
    void*       _accumulated,
    const void* _element,
    void*       _context
)
{
    (void)_context;

    *(int*)_accumulated *= *(const int*)_element;

    return true;
}


/*
test_helper_count_consumer
  Consumer: increments a counter pointed to by _context.
*/
static
void test_helper_count_consumer(
    void* _element,
    void* _context
)
{
    (void)_element;

    (*(size_t*)_context)++;

    return;
}


/*
test_helper_sum_consumer
  Consumer: adds each element to an int pointed to by _context.
*/
static
void test_helper_sum_consumer(
    void* _element,
    void* _context
)
{
    *(int*)_context += *(int*)_element;

    return;
}


/*
test_helper_negate_consumer
  Consumer: negates each int in-place.
*/
static
void test_helper_negate_consumer(
    void* _element,
    void* _context
)
{
    (void)_context;

    *(int*)_element = -(*(int*)_element);

    return;
}


/*
d_tests_pipeline_map
  Tests d_functional_pipeline_map.
  Tests the following:
  - mapping with a valid transformer doubles each element
  - mapped pipeline owns its data (owns_data == true)
  - original data is unchanged after mapping a non-owned pipeline
  - mapping propagates prior error (no-op)
  - NULL transformer produces error
  - context parameter is forwarded to the transformer
  - transformer failure mid-stream produces error and frees new buffer
  - mapping a begin_copy pipeline frees the old owned buffer
*/
bool d_tests_pipeline_map(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    struct dj_functional_pipeline mapped;
    int                          data[] = { 1, 2, 3, 4, 5 };
    int*                         result;
    size_t                       count;
    bool                         all_passed;
    int                          offset;
    size_t                       call_count;

    d_test_suite_test(_suite, "pipeline_map");

    all_passed = true;

    // ---- basic map: double each element ----
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped = d_functional_pipeline_map(pipe,
                                       test_helper_double_int,
                                       NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: valid transform produces error_code 0",
                                   mapped.error_code == 0,
                                   "mapping should succeed");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: result pipeline owns data",
                                   mapped.owns_data == true,
                                   "map always allocates a new buffer");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: count preserved",
                                   mapped.count == 5,
                                   "element count should not change");

    result = (int*)mapped.data;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: each element doubled correctly",
                                   result[0] == 2 && result[1] == 4 && result[2] == 6 && result[3] == 8 && result[4] == 10,
                                   "values should be 2,4,6,8,10");

    // original data unchanged
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: original data unchanged",
                                   data[0] == 1 && data[4] == 5,
                                   "begin (non-owning) data should not be modified");

    d_functional_pipeline_free(&mapped);

    // ---- map with context ----
    offset = 100;
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped = d_functional_pipeline_map(pipe,
                                       test_helper_add_context_int,
                                       &offset);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: context forwarding succeeds",
                                   mapped.error_code == 0,
                                   "mapping with context should succeed");

    result = (int*)mapped.data;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: context value applied correctly",
                                   result[0] == 101 && result[4] == 105,
                                   "each element should have 100 added");

    d_functional_pipeline_free(&mapped);

    // ---- NULL transformer ----
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped = d_functional_pipeline_map(pipe, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: NULL transformer returns error",
                                   mapped.error_code == EINVAL,
                                   "error_code should be EINVAL for NULL transformer");

    // ---- propagate prior error ----
    {
        struct dj_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        mapped = d_functional_pipeline_map(err_pipe,
                                           test_helper_double_int,
                                           NULL);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "map: propagates prior error unchanged",
                                       mapped.error_code == -1,
                                       "error should pass through without calling transformer");
    }

    // ---- transformer failure mid-stream ----
    call_count = 0;
    pipe       = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped     = d_functional_pipeline_map(pipe,
                                           test_helper_fail_on_third,
                                           &call_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "map: transformer failure mid-stream returns error",
                                   mapped.error_code == EINVAL,
                                   "error_code should be EINVAL when transformer fails");

    // ---- map on owned data (begin_copy): old buffer freed ----
    {
        struct dj_functional_pipeline copy_pipe;
        struct dj_functional_pipeline mapped2;
        int                          src[] = { 7, 8, 9 };

        copy_pipe = d_functional_pipeline_begin_copy(src,
                                                     3,
                                                     sizeof(int));

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "map: begin_copy pipeline owns data before map",
                                       copy_pipe.owns_data == true,
                                       "precondition: copy pipeline should own data");

        mapped2 = d_functional_pipeline_map(copy_pipe,
                                            test_helper_double_int,
                                            NULL);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "map: mapping a copy pipeline succeeds and owns new data",
                                       mapped2.error_code == 0 && mapped2.owns_data == true,
                                       "old buffer freed, new buffer owned");

        result = (int*)mapped2.data;

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "map: copy-then-map values correct",
                                       result[0] == 14 && result[1] == 16 && result[2] == 18,
                                       "values should be 14,16,18");

        d_functional_pipeline_free(&mapped2);
    }

    return all_passed;
}


/*
d_tests_pipeline_filter
  Tests d_functional_pipeline_filter.
  Tests the following:
  - filtering keeps only elements where predicate returns true
  - filtered pipeline owns its data
  - original data is unchanged
  - filtering with all-pass predicate keeps all elements
  - filtering with all-fail predicate produces count 0
  - NULL predicate returns error
  - propagates prior error
  - context parameter forwarded to predicate
  - filter on owned data frees old buffer
*/
bool d_tests_pipeline_filter(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    struct dj_functional_pipeline filtered;
    int                          data[] = { 1, 2, 3, 4, 5, 6 };
    int*                         result;
    bool                         all_passed;
    int                          threshold;

    d_test_suite_test(_suite, "pipeline_filter");

    all_passed = true;

    // ---- basic filter: keep even numbers ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe,
                                            test_helper_is_even,
                                            NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: valid predicate produces error_code 0",
                                   filtered.error_code == 0,
                                   "filtering should succeed");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: count is 3 (evens from 1..6)",
                                   filtered.count == 3,
                                   "only 2, 4, 6 should pass");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: result pipeline owns data",
                                   filtered.owns_data == true,
                                   "filter always allocates a new buffer");

    result = (int*)filtered.data;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: even values correct",
                                   result[0] == 2 && result[1] == 4 && result[2] == 6,
                                   "filtered values should be 2, 4, 6");

    // original unchanged
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: original data unchanged",
                                   data[0] == 1 && data[5] == 6,
                                   "source array should not be modified");

    d_functional_pipeline_free(&filtered);

    // ---- filter with context: greater than threshold ----
    threshold = 3;
    pipe      = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered  = d_functional_pipeline_filter(pipe,
                                             test_helper_greater_than_context,
                                             &threshold);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: context threshold keeps 4,5,6",
                                   filtered.error_code == 0 && filtered.count == 3,
                                   "3 elements should be > 3");

    result = (int*)filtered.data;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: context threshold values correct",
                                   result[0] == 4 && result[1] == 5 && result[2] == 6,
                                   "values should be 4, 5, 6");

    d_functional_pipeline_free(&filtered);

    // ---- all-pass predicate ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe,
                                            test_helper_always_true,
                                            NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: all-pass keeps all elements",
                                   filtered.count == 6,
                                   "count should still be 6");

    d_functional_pipeline_free(&filtered);

    // ---- all-fail predicate ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe,
                                            test_helper_always_false,
                                            NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: all-fail produces empty pipeline",
                                   filtered.error_code == 0 && filtered.count == 0,
                                   "count should be 0, error_code still 0");

    d_functional_pipeline_free(&filtered);

    // ---- NULL predicate ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "filter: NULL predicate returns error",
                                   filtered.error_code == EINVAL,
                                   "error_code should be EINVAL");

    // ---- propagate prior error ----
    {
        struct dj_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        filtered = d_functional_pipeline_filter(err_pipe,
                                                test_helper_is_even,
                                                NULL);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "filter: propagates prior error unchanged",
                                       filtered.error_code == -1,
                                       "error should pass through");
    }

    // ---- filter on owned data (begin_copy) ----
    {
        struct dj_functional_pipeline copy_pipe;
        int                          src[] = { 10, 15, 20, 25, 30 };

        copy_pipe = d_functional_pipeline_begin_copy(src,
                                                     5,
                                                     sizeof(int));
        filtered  = d_functional_pipeline_filter(copy_pipe,
                                                 test_helper_is_even,
                                                 NULL);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "filter: on copy pipeline succeeds (10,20,30)",
                                       filtered.error_code == 0 && filtered.count == 3,
                                       "old buffer freed, new buffer owned");

        result = (int*)filtered.data;

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "filter: copy-then-filter values correct",
                                       result[0] == 10 && result[1] == 20 && result[2] == 30,
                                       "values should be 10, 20, 30");

        d_functional_pipeline_free(&filtered);
    }

    return all_passed;
}


/*
d_tests_pipeline_fold_int
  Folds _count ints, viewed, from *_init (NULL: from zero) with _combine,
frees the result, and gives back the folded int. *_ok is whether the fold
gave one int of its own: count 1, an int's size, an owned block that is not
*_init.
*/
static
int d_tests_pipeline_fold_int(
    int*           _data,
    size_t         _count,
    int*           _init,
    fn_accumulator _combine,
    bool*          _ok
)
{
    struct dj_functional_pipeline folded;
    int                          value;

    folded = d_functional_pipeline_fold(
                 d_functional_pipeline_begin(_data, _count, sizeof(int)),
                 _init,
                 sizeof(int),
                 _combine,
                 NULL);

    *_ok  = ( (folded.error_code == 0)             &&
              (folded.count == 1)                  &&
              (folded.element_size == sizeof(int)) &&
              (folded.owns_data)                   &&
              (folded.data != (void*)_init) );
    value = (*_ok) ? *(int*)folded.data : 0;

    d_functional_pipeline_free(&folded);

    return value;
}


/*
d_tests_pipeline_fold
  Tests d_functional_pipeline_fold's results: one owned element of
_accumulator_size bytes, started from *_initial, which is read and never
written.
  Tests the following:
  - a sum, into one int of its own, *_initial unchanged
  - *_initial is the starting value
  - a product, from 1
  - NULL _initial starts from zero
  - an owned input is released when the fold replaces it
  - a single element
*/
bool d_tests_pipeline_fold(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    struct dj_functional_pipeline folded;
    int                          data[] = { 1, 2, 3, 4, 5 };
    int                          src[]  = { 10, 20, 30 };
    int                          init;
    int                          value;
    bool                         ok;
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_fold");

    all_passed = true;

    init = 0;
    value = d_tests_pipeline_fold_int(data,
                                      5,
                                      &init,
                                      test_helper_sum_accumulator,
                                      &ok);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: sum of 1..5 is 15",
                                   ok && (value == 15) && (init == 0),
                                   "one owned int; *_initial unchanged");

    init = 100;
    value = d_tests_pipeline_fold_int(data,
                                      5,
                                      &init,
                                      test_helper_sum_accumulator,
                                      &ok);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: starts from *_initial",
                                   ok && (value == 115),
                                   "100+1+2+3+4+5 = 115");

    init = 1;
    value = d_tests_pipeline_fold_int(data,
                                      5,
                                      &init,
                                      test_helper_product_accumulator,
                                      &ok);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: product of 1..5 is 120",
                                   ok && (value == 120),
                                   "1*2*3*4*5 = 120");

    value = d_tests_pipeline_fold_int(data,
                                      5,
                                      NULL,
                                      test_helper_sum_accumulator,
                                      &ok);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: NULL initial starts from zero",
                                   ok && (value == 15),
                                   "0+1+2+3+4+5 = 15");

    init = 0;
    value = d_tests_pipeline_fold_int(data + 4,
                                      1,
                                      &init,
                                      test_helper_sum_accumulator,
                                      &ok);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: single element fold works",
                                   ok && (value == 5),
                                   "0 + 5 = 5");

    // an owned input is released when the fold replaces it (ASan sees it)
    init   = 0;
    pipe   = d_functional_pipeline_begin_copy(src, 3, sizeof(int));
    folded = d_functional_pipeline_fold(pipe,
                                        &init,
                                        sizeof(int),
                                        test_helper_sum_accumulator,
                                        NULL);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: owned input replaced, sum 60",
                                   (folded.error_code == 0) &&
                                   (*(int*)folded.data == 60),
                                   "10+20+30 = 60");
    d_functional_pipeline_free(&folded);

    return all_passed;
}


/*
d_tests_pipeline_fold_errors
  Tests d_functional_pipeline_fold's failures, recorded as EINVAL.
  Tests the following:
  - NULL _combine, or a zero _accumulator_size
  - an accumulator that fails part way
  - a prior error passed through unchanged, *_initial untouched
*/
bool d_tests_pipeline_fold_errors(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    struct dj_functional_pipeline folded;
    int                          data[] = { 1, 2, 3, 4, 5 };
    int                          init;
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_fold_errors");

    all_passed = true;
    init       = 0;

    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    folded = d_functional_pipeline_fold(pipe,
                                        &init,
                                        sizeof(int),
                                        NULL,
                                        NULL);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: NULL combine returns error",
                                   folded.error_code == EINVAL,
                                   "error_code should be EINVAL");

    folded = d_functional_pipeline_fold(pipe,
                                        &init,
                                        0,
                                        test_helper_sum_accumulator,
                                        NULL);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: zero accumulator_size returns error",
                                   folded.error_code == EINVAL,
                                   "error_code should be EINVAL");

    folded = d_functional_pipeline_fold(pipe,
                                        &init,
                                        sizeof(int),
                                        test_helper_fail_accumulator,
                                        NULL);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: accumulator failure returns error",
                                   (folded.error_code == EINVAL) &&
                                   (!folded.owns_data),
                                   "EINVAL, the view kept");

    // ---- a prior error passes through ----
    pipe.error_code = ENOMEM;
    folded          = d_functional_pipeline_fold(pipe,
                                                 &init,
                                                 sizeof(int),
                                                 test_helper_sum_accumulator,
                                                 NULL);
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "fold: propagates prior error unchanged",
                                   (folded.error_code == ENOMEM) &&
                                   (init == 0),
                                   "the first error stays; nothing folded");

    return all_passed;
}


/*
d_tests_pipeline_for_each
  Tests d_functional_pipeline_for_each.
  Tests the following:
  - consumer is called for each element
  - context is forwarded to consumer
  - pipeline is returned unchanged (data, count, owns_data)
  - consumer can modify elements in-place
  - NULL consumer returns error
  - propagates prior error
*/
bool d_tests_pipeline_for_each(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    struct dj_functional_pipeline result;
    int                          data[] = { 1, 2, 3, 4, 5 };
    size_t                       visit_count;
    int                          running_sum;
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_for_each");

    all_passed = true;

    // ---- count visits ----
    visit_count = 0;
    pipe        = d_functional_pipeline_begin(data, 5, sizeof(int));
    result      = d_functional_pipeline_for_each(pipe,
                                                 test_helper_count_consumer,
                                                 &visit_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "for_each: valid consumer produces error_code 0",
                                   result.error_code == 0,
                                   "for_each should succeed");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "for_each: consumer called 5 times",
                                   visit_count == 5,
                                   "each of the 5 elements should be visited");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "for_each: pipeline passed through unchanged",
                                   result.data == data && result.count == 5,
                                   "data pointer and count should be same as input");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "for_each: ownership unchanged",
                                   result.owns_data == false,
                                   "for_each does not allocate or transfer ownership");

    // ---- sum via consumer context ----
    running_sum = 0;
    pipe        = d_functional_pipeline_begin(data, 5, sizeof(int));
    result      = d_functional_pipeline_for_each(pipe,
                                                 test_helper_sum_consumer,
                                                 &running_sum);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "for_each: sum via context is 15",
                                   running_sum == 15,
                                   "1+2+3+4+5 = 15");

    // ---- in-place mutation via consumer ----
    {
        int mutable_data[] = { 10, 20, 30 };

        pipe   = d_functional_pipeline_begin(mutable_data,
                                             3,
                                             sizeof(int));
        result = d_functional_pipeline_for_each(pipe,
                                                test_helper_negate_consumer,
                                                NULL);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "for_each: negate consumer succeeds",
                                       result.error_code == 0,
                                       "in-place mutation should work");

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "for_each: elements negated in-place",
                                       mutable_data[0] == -10 && mutable_data[1] == -20 && mutable_data[2] == -30,
                                       "values should be -10, -20, -30");
    }

    // ---- NULL consumer ----
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    result = d_functional_pipeline_for_each(pipe, NULL, NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "for_each: NULL consumer returns error",
                                   result.error_code == EINVAL,
                                   "error_code should be EINVAL");

    // ---- propagate prior error ----
    {
        struct dj_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        visit_count = 0;
        result = d_functional_pipeline_for_each(err_pipe,
                                                test_helper_count_consumer,
                                                &visit_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "for_each: propagates prior error",
                                       result.error_code == -1,
                                       "error should pass through");

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "for_each: consumer not called on prior error",
                                       visit_count == 0,
                                       "consumer should never fire if pipe has error");
    }

    // ---- single element ----
    {
        int single[] = { 99 };

        visit_count = 0;
        pipe   = d_functional_pipeline_begin(single, 1, sizeof(int));
        result = d_functional_pipeline_for_each(pipe,
                                                test_helper_count_consumer,
                                                &visit_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "for_each: single element visited once",
                                       visit_count == 1,
                                       "consumer should be called exactly once");
    }

    return all_passed;
}


/*
d_tests_pipeline_take
  Tests d_functional_pipeline_take.
  Tests the following:
  - take(n) where n < count reduces count to n
  - take(n) where n >= count leaves count unchanged
  - take(0) produces count 0
  - data pointer unchanged (no reallocation)
  - propagates prior error
*/
bool d_tests_pipeline_take(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    struct dj_functional_pipeline taken;
    int                          data[] = { 10, 20, 30, 40, 50 };
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_take");

    all_passed = true;

    // ---- take fewer than count ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 3);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take: valid take produces error_code 0",
                                   taken.error_code == 0,
                                   "take should succeed");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take(3): count reduced to 3",
                                   taken.count == 3,
                                   "count should be clamped to 3");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take: data pointer unchanged",
                                   taken.data == data,
                                   "take does not reallocate");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take: first 3 elements accessible",
                                   ((int*)taken.data)[0] == 10 && ((int*)taken.data)[2] == 30,
                                   "values should be 10, 20, 30");

    // ---- take exactly count ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 5);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take(5): count unchanged when n == count",
                                   taken.count == 5,
                                   "count should remain 5");

    // ---- take more than count ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 100);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take(100): count unchanged when n > count",
                                   taken.count == 5,
                                   "count should remain 5");

    // ---- take 0 ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 0);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take(0): count becomes 0",
                                   taken.error_code == 0 && taken.count == 0,
                                   "taking 0 should empty the pipeline");

    // ---- take 1 ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 1);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "take(1): count is 1, first element correct",
                                   taken.count == 1 && ((int*)taken.data)[0] == 10,
                                   "only the first element should remain");

    // ---- propagate prior error ----
    {
        struct dj_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        taken = d_functional_pipeline_take(err_pipe, 3);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "take: propagates prior error",
                                       taken.error_code == -1,
                                       "error should pass through");
    }

    return all_passed;
}


/*
d_tests_pipeline_skip
  Tests d_functional_pipeline_skip.
  Tests the following:
  - skip(n) where n < count advances pointer and reduces count
  - skip(n) where n >= count produces count 0
  - skip(0) is a no-op
  - data pointer moves forward by n * element_size
  - propagates prior error
*/
bool d_tests_pipeline_skip(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    struct dj_functional_pipeline skipped;
    int                          data[] = { 10, 20, 30, 40, 50 };
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_skip");

    all_passed = true;

    // ---- skip fewer than count ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 2);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip: valid skip produces error_code 0",
                                   skipped.error_code == 0,
                                   "skip should succeed");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(2): count reduced from 5 to 3",
                                   skipped.count == 3,
                                   "5 - 2 = 3 elements remaining");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(2): data pointer advanced by 2 elements",
                                   skipped.data == (unsigned char*)data + (2 * sizeof(int)),
                                   "pointer should advance by 2 * sizeof(int)");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(2): remaining elements are 30,40,50",
                                   ((int*)skipped.data)[0] == 30 && ((int*)skipped.data)[1] == 40 && ((int*)skipped.data)[2] == 50,
                                   "first visible element should be data[2]");

    // ---- skip 0 (no-op) ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 0);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(0): no-op, count and data unchanged",
                                   skipped.count == 5 && skipped.data == data,
                                   "skipping 0 should change nothing");

    // ---- skip exactly count ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 5);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(5): skipping all produces count 0",
                                   skipped.count == 0,
                                   "no elements should remain");

    // ---- skip more than count ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 100);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(100): skipping past all produces count 0",
                                   skipped.count == 0,
                                   "no elements should remain");

    // ---- skip 1 ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 1);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(1): first element removed, count is 4",
                                   skipped.count == 4 && ((int*)skipped.data)[0] == 20,
                                   "first visible element should be 20");

    // ---- skip all but 1 ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 4);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "skip(4): only last element remains",
                                   skipped.count == 1 && ((int*)skipped.data)[0] == 50,
                                   "only 50 should be left");

    // ---- propagate prior error ----
    {
        struct dj_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        skipped = d_functional_pipeline_skip(err_pipe, 2);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "skip: propagates prior error",
                                       skipped.error_code == -1,
                                       "error should pass through");
    }

    return all_passed;
}


/*
d_tests_pipeline_chaining
  Tests chaining multiple pipeline operations together.
  Tests the following:
  - map then filter produces correct subset of transformed values
  - filter then map operates on the filtered subset
  - filter then fold accumulates only matching elements
  - take then map processes only the first N elements
  - skip then take produces a slice
  - map then for_each visits all mapped elements
  - error in first stage propagates through entire chain
  - complex multi-stage chain: copy -> map -> filter -> fold
  - D_FUNCTIONAL_PIPELINE and D_FUNCTIONAL_PIPE macros work
*/
bool d_tests_pipeline_chaining(
    struct dj_test_suite* _suite
)
{
    struct dj_functional_pipeline pipe;
    int                          data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int*                         result;
    size_t                       out_count;
    int                          sum;
    bool                         all_passed;

    d_test_suite_test(_suite, "pipeline_chaining");

    all_passed = true;

    // ---- map then filter: double, then keep evens ----
    // doubling any int gives an even, so all should pass
    pipe = d_functional_pipeline_begin(data, 8, sizeof(int));
    pipe = d_functional_pipeline_map(pipe,
                                     test_helper_double_int,
                                     NULL);
    pipe = d_functional_pipeline_filter(pipe,
                                        test_helper_is_even,
                                        NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: map(double) -> filter(even) keeps all 8",
                                   pipe.error_code == 0 && pipe.count == 8,
                                   "doubling always gives evens");

    result = (int*)d_functional_pipeline_end(pipe, &out_count);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: end returns 8 elements",
                                   out_count == 8 && result != NULL,
                                   "pipeline end should return all");

    // clean up (map+filter allocated buffers; the last one is owned)
    free(result);

    // ---- filter then fold: keep evens, sum them ----
    sum  = 0;
    pipe = d_functional_pipeline_begin(data, 8, sizeof(int));
    pipe = d_functional_pipeline_filter(pipe,
                                        test_helper_is_even,
                                        NULL);
    pipe = d_functional_pipeline_fold(pipe,
                                      &sum,
                                      sizeof(int),
                                      test_helper_sum_accumulator,
                                      NULL);

    //   the fold's result is a block of its own; sum, the starting value,
    // stays 0
    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: filter(even) -> fold(sum) = 20",
                                   (pipe.error_code == 0) &&
                                   (*(int*)pipe.data == 20) &&
                                   (sum == 0),
                                   "2+4+6+8 = 20");

    d_functional_pipeline_free(&pipe);

    // ---- take then map: take first 3, double them ----
    pipe = d_functional_pipeline_begin(data, 8, sizeof(int));
    pipe = d_functional_pipeline_take(pipe, 3);
    pipe = d_functional_pipeline_map(pipe,
                                     test_helper_double_int,
                                     NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: take(3) -> map(double) has 3 elements",
                                   pipe.error_code == 0 && pipe.count == 3,
                                   "only first 3 should be processed");

    result = (int*)pipe.data;

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: take(3) -> map(double) values 2,4,6",
                                   result[0] == 2 && result[1] == 4 && result[2] == 6,
                                   "1*2=2, 2*2=4, 3*2=6");

    d_functional_pipeline_free(&pipe);

    // ---- skip then take: slice [2..4) ----
    pipe = d_functional_pipeline_begin(data, 8, sizeof(int));
    pipe = d_functional_pipeline_skip(pipe, 2);
    pipe = d_functional_pipeline_take(pipe, 2);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: skip(2) -> take(2) gives slice of 2",
                                   pipe.error_code == 0 && pipe.count == 2,
                                   "elements [2] and [3] of original");

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: skip(2) -> take(2) values are 3, 4",
                                   ((int*)pipe.data)[0] == 3 && ((int*)pipe.data)[1] == 4,
                                   "data[2]=3, data[3]=4");

    // ---- complex chain: copy -> map -> filter -> fold ----
    {
        const int src[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

        sum  = 0;
        pipe = d_functional_pipeline_begin_copy(src,
                                                10,
                                                sizeof(int));
        pipe = d_functional_pipeline_map(pipe,
                                         test_helper_double_int,
                                         NULL);
        pipe = d_functional_pipeline_filter(pipe,
                                            test_helper_is_even,
                                            NULL);
        pipe = d_functional_pipeline_fold(pipe,
                                          &sum,
                                          sizeof(int),
                                          test_helper_sum_accumulator,
                                          NULL);

        // all doubled values are even, so filter keeps all 10
        // sum of 2+4+6+8+10+12+14+16+18+20 = 110
        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "chain: copy -> map -> filter -> fold",
                                       (pipe.error_code == 0) &&
                                       (*(int*)pipe.data == 110),
                                       "sum of doubled 1..10 = 110");

        d_functional_pipeline_free(&pipe);
    }

    // ---- error propagation through chain ----
    pipe = d_functional_pipeline_begin(NULL, 5, sizeof(int));
    pipe = d_functional_pipeline_map(pipe,
                                     test_helper_double_int,
                                     NULL);
    pipe = d_functional_pipeline_filter(pipe,
                                        test_helper_is_even,
                                        NULL);

    all_passed &= D_TEST_CHECK_MSG(_suite,
                                   "chain: error propagates through map and filter",
                                   pipe.error_code == EINVAL,
                                   "begin's EINVAL should survive the chain");

    // ---- D_FUNCTIONAL_PIPELINE macro ----
    {
        struct dj_functional_pipeline macro_pipe;

        macro_pipe = D_FUNCTIONAL_PIPELINE(data, 8, sizeof(int));

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "D_FUNCTIONAL_PIPELINE macro works",
                                       macro_pipe.error_code == 0 && macro_pipe.count == 8,
                                       "macro should produce same result as begin()");
    }

    // ---- map then for_each ----
    {
        size_t visit_count;

        visit_count = 0;
        pipe = d_functional_pipeline_begin(data, 8, sizeof(int));
        pipe = d_functional_pipeline_map(pipe,
                                         test_helper_double_int,
                                         NULL);
        pipe = d_functional_pipeline_for_each(pipe,
                                              test_helper_count_consumer,
                                              &visit_count);

        all_passed &= D_TEST_CHECK_MSG(_suite,
                                       "chain: map -> for_each visits all 8 elements",
                                       pipe.error_code == 0 && visit_count == 8,
                                       "for_each should see all mapped elements");

        d_functional_pipeline_free(&pipe);
    }

    return all_passed;
}


/*
d_tests_pipeline_operations_all
  Runs all pipeline operation tests.
  Tests the following:
  - d_functional_pipeline_map
  - d_functional_pipeline_filter
  - d_functional_pipeline_fold
  - d_functional_pipeline_for_each
  - d_functional_pipeline_take
  - d_functional_pipeline_skip
  - chaining multiple operations
*/
bool d_tests_pipeline_operations_all(
    struct dj_test_suite* _suite
)
{
    bool all_passed;

    d_test_suite_block(_suite, "pipeline_operations_all");

    all_passed = true;

    all_passed &= d_tests_pipeline_map(_suite);
    all_passed &= d_tests_pipeline_filter(_suite);
    all_passed &= d_tests_pipeline_fold(_suite);
    all_passed &= d_tests_pipeline_fold_errors(_suite);
    all_passed &= d_tests_pipeline_for_each(_suite);
    all_passed &= d_tests_pipeline_take(_suite);
    all_passed &= d_tests_pipeline_skip(_suite);
    all_passed &= d_tests_pipeline_chaining(_suite);

    return all_passed;
}
