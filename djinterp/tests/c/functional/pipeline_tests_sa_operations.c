#include ".\pipeline_tests_sa.h"


/******************************************************************************
 * TEST HELPERS: callbacks for pipeline operations
 *****************************************************************************/

/*
test_helper_double_int
  Transformer: multiplies each int by 2.
*/
static bool
test_helper_double_int
(
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
static bool
test_helper_add_context_int
(
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
static bool
test_helper_fail_transformer
(
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
static bool
test_helper_fail_on_third
(
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
static bool
test_helper_is_even
(
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
static bool
test_helper_greater_than_context
(
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
static bool
test_helper_always_true
(
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
static bool
test_helper_always_false
(
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
static bool
test_helper_sum_accumulator
(
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
static bool
test_helper_fail_accumulator
(
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
static bool
test_helper_product_accumulator
(
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
static void
test_helper_count_consumer
(
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
static void
test_helper_sum_consumer
(
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
static void
test_helper_negate_consumer
(
    void* _element,
    void* _context
)
{
    (void)_context;

    *(int*)_element = -(*(int*)_element);

    return;
}


/*
d_tests_sa_pipeline_map
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
bool
d_tests_sa_pipeline_map
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    struct d_functional_pipeline mapped;
    int                          data[] = { 1, 2, 3, 4, 5 };
    int*                         result;
    size_t                       count;
    bool                         all_passed;
    int                          offset;
    size_t                       call_count;

    all_passed = true;

    // ---- basic map: double each element ----
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped = d_functional_pipeline_map(pipe,
                                       test_helper_double_int,
                                       NULL);

    all_passed &= d_assert_standalone(
        mapped.error_code == 0,
        "map: valid transform produces error_code 0",
        "mapping should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        mapped.owns_data == true,
        "map: result pipeline owns data",
        "map always allocates a new buffer",
        _test_info);

    all_passed &= d_assert_standalone(
        mapped.count == 5,
        "map: count preserved",
        "element count should not change",
        _test_info);

    result = (int*)mapped.data;

    all_passed &= d_assert_standalone(
        result[0] == 2 && result[1] == 4 && result[2] == 6 &&
        result[3] == 8 && result[4] == 10,
        "map: each element doubled correctly",
        "values should be 2,4,6,8,10",
        _test_info);

    // original data unchanged
    all_passed &= d_assert_standalone(
        data[0] == 1 && data[4] == 5,
        "map: original data unchanged",
        "begin (non-owning) data should not be modified",
        _test_info);

    d_functional_pipeline_free(&mapped);

    // ---- map with context ----
    offset = 100;
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped = d_functional_pipeline_map(pipe,
                                       test_helper_add_context_int,
                                       &offset);

    all_passed &= d_assert_standalone(
        mapped.error_code == 0,
        "map: context forwarding succeeds",
        "mapping with context should succeed",
        _test_info);

    result = (int*)mapped.data;

    all_passed &= d_assert_standalone(
        result[0] == 101 && result[4] == 105,
        "map: context value applied correctly",
        "each element should have 100 added",
        _test_info);

    d_functional_pipeline_free(&mapped);

    // ---- NULL transformer ----
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped = d_functional_pipeline_map(pipe, NULL, NULL);

    all_passed &= d_assert_standalone(
        mapped.error_code == -1,
        "map: NULL transformer returns error",
        "error_code should be -1 for NULL transformer",
        _test_info);

    // ---- propagate prior error ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        mapped = d_functional_pipeline_map(err_pipe,
                                           test_helper_double_int,
                                           NULL);

        all_passed &= d_assert_standalone(
            mapped.error_code == -1,
            "map: propagates prior error unchanged",
            "error should pass through without calling transformer",
            _test_info);
    }

    // ---- transformer failure mid-stream ----
    call_count = 0;
    pipe       = d_functional_pipeline_begin(data, 5, sizeof(int));
    mapped     = d_functional_pipeline_map(pipe,
                                           test_helper_fail_on_third,
                                           &call_count);

    all_passed &= d_assert_standalone(
        mapped.error_code == -1,
        "map: transformer failure mid-stream returns error",
        "error_code should be -1 when transformer fails",
        _test_info);

    // ---- map on owned data (begin_copy): old buffer freed ----
    {
        struct d_functional_pipeline copy_pipe;
        struct d_functional_pipeline mapped2;
        int                          src[] = { 7, 8, 9 };

        copy_pipe = d_functional_pipeline_begin_copy(src,
                                                     3,
                                                     sizeof(int));

        all_passed &= d_assert_standalone(
            copy_pipe.owns_data == true,
            "map: begin_copy pipeline owns data before map",
            "precondition: copy pipeline should own data",
            _test_info);

        mapped2 = d_functional_pipeline_map(copy_pipe,
                                            test_helper_double_int,
                                            NULL);

        all_passed &= d_assert_standalone(
            mapped2.error_code == 0 && mapped2.owns_data == true,
            "map: mapping a copy pipeline succeeds and owns new data",
            "old buffer freed, new buffer owned",
            _test_info);

        result = (int*)mapped2.data;

        all_passed &= d_assert_standalone(
            result[0] == 14 && result[1] == 16 && result[2] == 18,
            "map: copy-then-map values correct",
            "values should be 14,16,18",
            _test_info);

        d_functional_pipeline_free(&mapped2);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_filter
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
bool
d_tests_sa_pipeline_filter
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    struct d_functional_pipeline filtered;
    int                          data[] = { 1, 2, 3, 4, 5, 6 };
    int*                         result;
    bool                         all_passed;
    int                          threshold;

    all_passed = true;

    // ---- basic filter: keep even numbers ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe,
                                            test_helper_is_even,
                                            NULL);

    all_passed &= d_assert_standalone(
        filtered.error_code == 0,
        "filter: valid predicate produces error_code 0",
        "filtering should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        filtered.count == 3,
        "filter: count is 3 (evens from 1..6)",
        "only 2, 4, 6 should pass",
        _test_info);

    all_passed &= d_assert_standalone(
        filtered.owns_data == true,
        "filter: result pipeline owns data",
        "filter always allocates a new buffer",
        _test_info);

    result = (int*)filtered.data;

    all_passed &= d_assert_standalone(
        result[0] == 2 && result[1] == 4 && result[2] == 6,
        "filter: even values correct",
        "filtered values should be 2, 4, 6",
        _test_info);

    // original unchanged
    all_passed &= d_assert_standalone(
        data[0] == 1 && data[5] == 6,
        "filter: original data unchanged",
        "source array should not be modified",
        _test_info);

    d_functional_pipeline_free(&filtered);

    // ---- filter with context: greater than threshold ----
    threshold = 3;
    pipe      = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered  = d_functional_pipeline_filter(pipe,
                                             test_helper_greater_than_context,
                                             &threshold);

    all_passed &= d_assert_standalone(
        filtered.error_code == 0 && filtered.count == 3,
        "filter: context threshold keeps 4,5,6",
        "3 elements should be > 3",
        _test_info);

    result = (int*)filtered.data;

    all_passed &= d_assert_standalone(
        result[0] == 4 && result[1] == 5 && result[2] == 6,
        "filter: context threshold values correct",
        "values should be 4, 5, 6",
        _test_info);

    d_functional_pipeline_free(&filtered);

    // ---- all-pass predicate ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe,
                                            test_helper_always_true,
                                            NULL);

    all_passed &= d_assert_standalone(
        filtered.count == 6,
        "filter: all-pass keeps all elements",
        "count should still be 6",
        _test_info);

    d_functional_pipeline_free(&filtered);

    // ---- all-fail predicate ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe,
                                            test_helper_always_false,
                                            NULL);

    all_passed &= d_assert_standalone(
        filtered.error_code == 0 && filtered.count == 0,
        "filter: all-fail produces empty pipeline",
        "count should be 0, error_code still 0",
        _test_info);

    d_functional_pipeline_free(&filtered);

    // ---- NULL predicate ----
    pipe     = d_functional_pipeline_begin(data, 6, sizeof(int));
    filtered = d_functional_pipeline_filter(pipe, NULL, NULL);

    all_passed &= d_assert_standalone(
        filtered.error_code == -1,
        "filter: NULL predicate returns error",
        "error_code should be -1",
        _test_info);

    // ---- propagate prior error ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        filtered = d_functional_pipeline_filter(err_pipe,
                                                test_helper_is_even,
                                                NULL);

        all_passed &= d_assert_standalone(
            filtered.error_code == -1,
            "filter: propagates prior error unchanged",
            "error should pass through",
            _test_info);
    }

    // ---- filter on owned data (begin_copy) ----
    {
        struct d_functional_pipeline copy_pipe;
        int                          src[] = { 10, 15, 20, 25, 30 };

        copy_pipe = d_functional_pipeline_begin_copy(src,
                                                     5,
                                                     sizeof(int));
        filtered  = d_functional_pipeline_filter(copy_pipe,
                                                 test_helper_is_even,
                                                 NULL);

        all_passed &= d_assert_standalone(
            filtered.error_code == 0 && filtered.count == 3,
            "filter: on copy pipeline succeeds (10,20,30)",
            "old buffer freed, new buffer owned",
            _test_info);

        result = (int*)filtered.data;

        all_passed &= d_assert_standalone(
            result[0] == 10 && result[1] == 20 && result[2] == 30,
            "filter: copy-then-filter values correct",
            "values should be 10, 20, 30",
            _test_info);

        d_functional_pipeline_free(&filtered);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_fold
  Tests d_functional_pipeline_fold.
  Tests the following:
  - folding with sum accumulator produces correct total
  - result pipeline wraps _initial, has count 1
  - result pipeline does NOT own _initial
  - fold frees old data if owned
  - NULL _initial returns error
  - NULL _combine returns error
  - zero _accumulator_size returns error
  - propagates prior error
  - context forwarded to accumulator
  - accumulator failure mid-stream returns error
  - product fold works correctly
*/
bool
d_tests_sa_pipeline_fold
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    struct d_functional_pipeline folded;
    int                          data[] = { 1, 2, 3, 4, 5 };
    int                          sum;
    int                          product;
    bool                         all_passed;

    all_passed = true;

    // ---- basic sum fold ----
    sum  = 0;
    pipe = d_functional_pipeline_begin(data, 5, sizeof(int));
    folded = d_functional_pipeline_fold(pipe,
                                        &sum,
                                        sizeof(int),
                                        test_helper_sum_accumulator,
                                        NULL);

    all_passed &= d_assert_standalone(
        folded.error_code == 0,
        "fold: valid accumulator produces error_code 0",
        "folding should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        sum == 15,
        "fold: sum of 1..5 is 15",
        "1+2+3+4+5 = 15",
        _test_info);

    all_passed &= d_assert_standalone(
        folded.data == &sum,
        "fold: result wraps _initial pointer",
        "data should point to the sum variable",
        _test_info);

    all_passed &= d_assert_standalone(
        folded.count == 1,
        "fold: result count is 1",
        "folded pipeline always has count 1",
        _test_info);

    all_passed &= d_assert_standalone(
        folded.element_size == sizeof(int),
        "fold: element_size is _accumulator_size",
        "element_size should match accumulator size",
        _test_info);

    all_passed &= d_assert_standalone(
        folded.owns_data == false,
        "fold: result does NOT own _initial",
        "pipeline should not own caller-provided accumulator",
        _test_info);

    // ---- product fold ----
    product = 1;
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    folded  = d_functional_pipeline_fold(pipe,
                                         &product,
                                         sizeof(int),
                                         test_helper_product_accumulator,
                                         NULL);

    all_passed &= d_assert_standalone(
        folded.error_code == 0 && product == 120,
        "fold: product of 1..5 is 120",
        "1*2*3*4*5 = 120",
        _test_info);

    // ---- fold frees owned data ----
    {
        struct d_functional_pipeline copy_pipe;
        int                          src[] = { 10, 20, 30 };
        int                          total;

        total     = 0;
        copy_pipe = d_functional_pipeline_begin_copy(src,
                                                     3,
                                                     sizeof(int));
        folded    = d_functional_pipeline_fold(copy_pipe,
                                               &total,
                                               sizeof(int),
                                               test_helper_sum_accumulator,
                                               NULL);

        all_passed &= d_assert_standalone(
            folded.error_code == 0 && total == 60,
            "fold: owned data freed, sum correct (60)",
            "10+20+30 = 60, old buffer should be freed",
            _test_info);
    }

    // ---- NULL _initial ----
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    folded = d_functional_pipeline_fold(pipe,
                                        NULL,
                                        sizeof(int),
                                        test_helper_sum_accumulator,
                                        NULL);

    all_passed &= d_assert_standalone(
        folded.error_code == -1,
        "fold: NULL initial returns error",
        "error_code should be -1",
        _test_info);

    // ---- NULL _combine ----
    sum    = 0;
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    folded = d_functional_pipeline_fold(pipe,
                                        &sum,
                                        sizeof(int),
                                        NULL,
                                        NULL);

    all_passed &= d_assert_standalone(
        folded.error_code == -1,
        "fold: NULL combine returns error",
        "error_code should be -1",
        _test_info);

    // ---- zero _accumulator_size ----
    sum    = 0;
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    folded = d_functional_pipeline_fold(pipe,
                                        &sum,
                                        0,
                                        test_helper_sum_accumulator,
                                        NULL);

    all_passed &= d_assert_standalone(
        folded.error_code == -1,
        "fold: zero accumulator_size returns error",
        "error_code should be -1",
        _test_info);

    // ---- propagate prior error ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        sum    = 0;
        folded = d_functional_pipeline_fold(err_pipe,
                                            &sum,
                                            sizeof(int),
                                            test_helper_sum_accumulator,
                                            NULL);

        all_passed &= d_assert_standalone(
            folded.error_code == -1,
            "fold: propagates prior error unchanged",
            "error should pass through",
            _test_info);

        all_passed &= d_assert_standalone(
            sum == 0,
            "fold: accumulator untouched on prior error",
            "sum should remain 0",
            _test_info);
    }

    // ---- accumulator failure ----
    sum    = 0;
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    folded = d_functional_pipeline_fold(pipe,
                                        &sum,
                                        sizeof(int),
                                        test_helper_fail_accumulator,
                                        NULL);

    all_passed &= d_assert_standalone(
        folded.error_code == -1,
        "fold: accumulator failure returns error",
        "error_code should be -1 when combine fails",
        _test_info);

    // ---- single element fold ----
    {
        int single[] = { 42 };
        int acc;

        acc  = 0;
        pipe = d_functional_pipeline_begin(single, 1, sizeof(int));
        folded = d_functional_pipeline_fold(pipe,
                                            &acc,
                                            sizeof(int),
                                            test_helper_sum_accumulator,
                                            NULL);

        all_passed &= d_assert_standalone(
            folded.error_code == 0 && acc == 42,
            "fold: single element fold works",
            "folding 1 element should just add it to acc",
            _test_info);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_for_each
  Tests d_functional_pipeline_for_each.
  Tests the following:
  - consumer is called for each element
  - context is forwarded to consumer
  - pipeline is returned unchanged (data, count, owns_data)
  - consumer can modify elements in-place
  - NULL consumer returns error
  - propagates prior error
*/
bool
d_tests_sa_pipeline_for_each
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    struct d_functional_pipeline result;
    int                          data[] = { 1, 2, 3, 4, 5 };
    size_t                       visit_count;
    int                          running_sum;
    bool                         all_passed;

    all_passed = true;

    // ---- count visits ----
    visit_count = 0;
    pipe        = d_functional_pipeline_begin(data, 5, sizeof(int));
    result      = d_functional_pipeline_for_each(pipe,
                                                 test_helper_count_consumer,
                                                 &visit_count);

    all_passed &= d_assert_standalone(
        result.error_code == 0,
        "for_each: valid consumer produces error_code 0",
        "for_each should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        visit_count == 5,
        "for_each: consumer called 5 times",
        "each of the 5 elements should be visited",
        _test_info);

    all_passed &= d_assert_standalone(
        result.data == data && result.count == 5,
        "for_each: pipeline passed through unchanged",
        "data pointer and count should be same as input",
        _test_info);

    all_passed &= d_assert_standalone(
        result.owns_data == false,
        "for_each: ownership unchanged",
        "for_each does not allocate or transfer ownership",
        _test_info);

    // ---- sum via consumer context ----
    running_sum = 0;
    pipe        = d_functional_pipeline_begin(data, 5, sizeof(int));
    result      = d_functional_pipeline_for_each(pipe,
                                                 test_helper_sum_consumer,
                                                 &running_sum);

    all_passed &= d_assert_standalone(
        running_sum == 15,
        "for_each: sum via context is 15",
        "1+2+3+4+5 = 15",
        _test_info);

    // ---- in-place mutation via consumer ----
    {
        int mutable_data[] = { 10, 20, 30 };

        pipe   = d_functional_pipeline_begin(mutable_data,
                                             3,
                                             sizeof(int));
        result = d_functional_pipeline_for_each(pipe,
                                                test_helper_negate_consumer,
                                                NULL);

        all_passed &= d_assert_standalone(
            result.error_code == 0,
            "for_each: negate consumer succeeds",
            "in-place mutation should work",
            _test_info);

        all_passed &= d_assert_standalone(
            mutable_data[0] == -10 && mutable_data[1] == -20 &&
            mutable_data[2] == -30,
            "for_each: elements negated in-place",
            "values should be -10, -20, -30",
            _test_info);
    }

    // ---- NULL consumer ----
    pipe   = d_functional_pipeline_begin(data, 5, sizeof(int));
    result = d_functional_pipeline_for_each(pipe, NULL, NULL);

    all_passed &= d_assert_standalone(
        result.error_code == -1,
        "for_each: NULL consumer returns error",
        "error_code should be -1",
        _test_info);

    // ---- propagate prior error ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        visit_count = 0;
        result = d_functional_pipeline_for_each(err_pipe,
                                                test_helper_count_consumer,
                                                &visit_count);

        all_passed &= d_assert_standalone(
            result.error_code == -1,
            "for_each: propagates prior error",
            "error should pass through",
            _test_info);

        all_passed &= d_assert_standalone(
            visit_count == 0,
            "for_each: consumer not called on prior error",
            "consumer should never fire if pipe has error",
            _test_info);
    }

    // ---- single element ----
    {
        int single[] = { 99 };

        visit_count = 0;
        pipe   = d_functional_pipeline_begin(single, 1, sizeof(int));
        result = d_functional_pipeline_for_each(pipe,
                                                test_helper_count_consumer,
                                                &visit_count);

        all_passed &= d_assert_standalone(
            visit_count == 1,
            "for_each: single element visited once",
            "consumer should be called exactly once",
            _test_info);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_take
  Tests d_functional_pipeline_take.
  Tests the following:
  - take(n) where n < count reduces count to n
  - take(n) where n >= count leaves count unchanged
  - take(0) produces count 0
  - data pointer unchanged (no reallocation)
  - propagates prior error
*/
bool
d_tests_sa_pipeline_take
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    struct d_functional_pipeline taken;
    int                          data[] = { 10, 20, 30, 40, 50 };
    bool                         all_passed;

    all_passed = true;

    // ---- take fewer than count ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 3);

    all_passed &= d_assert_standalone(
        taken.error_code == 0,
        "take: valid take produces error_code 0",
        "take should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        taken.count == 3,
        "take(3): count reduced to 3",
        "count should be clamped to 3",
        _test_info);

    all_passed &= d_assert_standalone(
        taken.data == data,
        "take: data pointer unchanged",
        "take does not reallocate",
        _test_info);

    all_passed &= d_assert_standalone(
        ((int*)taken.data)[0] == 10 && ((int*)taken.data)[2] == 30,
        "take: first 3 elements accessible",
        "values should be 10, 20, 30",
        _test_info);

    // ---- take exactly count ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 5);

    all_passed &= d_assert_standalone(
        taken.count == 5,
        "take(5): count unchanged when n == count",
        "count should remain 5",
        _test_info);

    // ---- take more than count ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 100);

    all_passed &= d_assert_standalone(
        taken.count == 5,
        "take(100): count unchanged when n > count",
        "count should remain 5",
        _test_info);

    // ---- take 0 ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 0);

    all_passed &= d_assert_standalone(
        taken.error_code == 0 && taken.count == 0,
        "take(0): count becomes 0",
        "taking 0 should empty the pipeline",
        _test_info);

    // ---- take 1 ----
    pipe  = d_functional_pipeline_begin(data, 5, sizeof(int));
    taken = d_functional_pipeline_take(pipe, 1);

    all_passed &= d_assert_standalone(
        taken.count == 1 && ((int*)taken.data)[0] == 10,
        "take(1): count is 1, first element correct",
        "only the first element should remain",
        _test_info);

    // ---- propagate prior error ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        taken = d_functional_pipeline_take(err_pipe, 3);

        all_passed &= d_assert_standalone(
            taken.error_code == -1,
            "take: propagates prior error",
            "error should pass through",
            _test_info);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_skip
  Tests d_functional_pipeline_skip.
  Tests the following:
  - skip(n) where n < count advances pointer and reduces count
  - skip(n) where n >= count produces count 0
  - skip(0) is a no-op
  - data pointer moves forward by n * element_size
  - propagates prior error
*/
bool
d_tests_sa_pipeline_skip
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    struct d_functional_pipeline skipped;
    int                          data[] = { 10, 20, 30, 40, 50 };
    bool                         all_passed;

    all_passed = true;

    // ---- skip fewer than count ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 2);

    all_passed &= d_assert_standalone(
        skipped.error_code == 0,
        "skip: valid skip produces error_code 0",
        "skip should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        skipped.count == 3,
        "skip(2): count reduced from 5 to 3",
        "5 - 2 = 3 elements remaining",
        _test_info);

    all_passed &= d_assert_standalone(
        skipped.data == (unsigned char*)data + (2 * sizeof(int)),
        "skip(2): data pointer advanced by 2 elements",
        "pointer should advance by 2 * sizeof(int)",
        _test_info);

    all_passed &= d_assert_standalone(
        ((int*)skipped.data)[0] == 30 &&
        ((int*)skipped.data)[1] == 40 &&
        ((int*)skipped.data)[2] == 50,
        "skip(2): remaining elements are 30,40,50",
        "first visible element should be data[2]",
        _test_info);

    // ---- skip 0 (no-op) ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 0);

    all_passed &= d_assert_standalone(
        skipped.count == 5 && skipped.data == data,
        "skip(0): no-op, count and data unchanged",
        "skipping 0 should change nothing",
        _test_info);

    // ---- skip exactly count ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 5);

    all_passed &= d_assert_standalone(
        skipped.count == 0,
        "skip(5): skipping all produces count 0",
        "no elements should remain",
        _test_info);

    // ---- skip more than count ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 100);

    all_passed &= d_assert_standalone(
        skipped.count == 0,
        "skip(100): skipping past all produces count 0",
        "no elements should remain",
        _test_info);

    // ---- skip 1 ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 1);

    all_passed &= d_assert_standalone(
        skipped.count == 4 && ((int*)skipped.data)[0] == 20,
        "skip(1): first element removed, count is 4",
        "first visible element should be 20",
        _test_info);

    // ---- skip all but 1 ----
    pipe    = d_functional_pipeline_begin(data, 5, sizeof(int));
    skipped = d_functional_pipeline_skip(pipe, 4);

    all_passed &= d_assert_standalone(
        skipped.count == 1 && ((int*)skipped.data)[0] == 50,
        "skip(4): only last element remains",
        "only 50 should be left",
        _test_info);

    // ---- propagate prior error ----
    {
        struct d_functional_pipeline err_pipe;

        err_pipe.data         = NULL;
        err_pipe.element_size = sizeof(int);
        err_pipe.count        = 0;
        err_pipe.owns_data    = false;
        err_pipe.error_code   = -1;

        skipped = d_functional_pipeline_skip(err_pipe, 2);

        all_passed &= d_assert_standalone(
            skipped.error_code == -1,
            "skip: propagates prior error",
            "error should pass through",
            _test_info);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_chaining
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
bool
d_tests_sa_pipeline_chaining
(
    struct d_test_counter* _test_info
)
{
    struct d_functional_pipeline pipe;
    int                          data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int*                         result;
    size_t                       out_count;
    int                          sum;
    bool                         all_passed;

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

    all_passed &= d_assert_standalone(
        pipe.error_code == 0 && pipe.count == 8,
        "chain: map(double) -> filter(even) keeps all 8",
        "doubling always gives evens",
        _test_info);

    result = (int*)d_functional_pipeline_end(pipe, &out_count);

    all_passed &= d_assert_standalone(
        out_count == 8 && result != NULL,
        "chain: end returns 8 elements",
        "pipeline end should return all",
        _test_info);

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

    all_passed &= d_assert_standalone(
        pipe.error_code == 0 && sum == 20,
        "chain: filter(even) -> fold(sum) = 20",
        "2+4+6+8 = 20",
        _test_info);

    // ---- take then map: take first 3, double them ----
    pipe = d_functional_pipeline_begin(data, 8, sizeof(int));
    pipe = d_functional_pipeline_take(pipe, 3);
    pipe = d_functional_pipeline_map(pipe,
                                     test_helper_double_int,
                                     NULL);

    all_passed &= d_assert_standalone(
        pipe.error_code == 0 && pipe.count == 3,
        "chain: take(3) -> map(double) has 3 elements",
        "only first 3 should be processed",
        _test_info);

    result = (int*)pipe.data;

    all_passed &= d_assert_standalone(
        result[0] == 2 && result[1] == 4 && result[2] == 6,
        "chain: take(3) -> map(double) values 2,4,6",
        "1*2=2, 2*2=4, 3*2=6",
        _test_info);

    d_functional_pipeline_free(&pipe);

    // ---- skip then take: slice [2..4) ----
    pipe = d_functional_pipeline_begin(data, 8, sizeof(int));
    pipe = d_functional_pipeline_skip(pipe, 2);
    pipe = d_functional_pipeline_take(pipe, 2);

    all_passed &= d_assert_standalone(
        pipe.error_code == 0 && pipe.count == 2,
        "chain: skip(2) -> take(2) gives slice of 2",
        "elements [2] and [3] of original",
        _test_info);

    all_passed &= d_assert_standalone(
        ((int*)pipe.data)[0] == 3 && ((int*)pipe.data)[1] == 4,
        "chain: skip(2) -> take(2) values are 3, 4",
        "data[2]=3, data[3]=4",
        _test_info);

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
        all_passed &= d_assert_standalone(
            pipe.error_code == 0 && sum == 110,
            "chain: copy -> map(x2) -> filter(even) -> fold(sum) = 110",
            "sum of doubled 1..10 = 110",
            _test_info);
    }

    // ---- error propagation through chain ----
    pipe = d_functional_pipeline_begin(NULL, 5, sizeof(int));
    pipe = d_functional_pipeline_map(pipe,
                                     test_helper_double_int,
                                     NULL);
    pipe = d_functional_pipeline_filter(pipe,
                                        test_helper_is_even,
                                        NULL);

    all_passed &= d_assert_standalone(
        pipe.error_code == -1,
        "chain: error propagates through map and filter",
        "initial error should survive the entire chain",
        _test_info);

    // ---- D_FUNCTIONAL_PIPELINE macro ----
    {
        struct d_functional_pipeline macro_pipe;

        macro_pipe = D_FUNCTIONAL_PIPELINE(data, 8, sizeof(int));

        all_passed &= d_assert_standalone(
            macro_pipe.error_code == 0 && macro_pipe.count == 8,
            "D_FUNCTIONAL_PIPELINE macro works",
            "macro should produce same result as begin()",
            _test_info);
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

        all_passed &= d_assert_standalone(
            pipe.error_code == 0 && visit_count == 8,
            "chain: map -> for_each visits all 8 elements",
            "for_each should see all mapped elements",
            _test_info);

        d_functional_pipeline_free(&pipe);
    }

    return all_passed;
}


/*
d_tests_sa_pipeline_operations_all
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
bool
d_tests_sa_pipeline_operations_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_pipeline_map(_test_info);
    all_passed &= d_tests_sa_pipeline_filter(_test_info);
    all_passed &= d_tests_sa_pipeline_fold(_test_info);
    all_passed &= d_tests_sa_pipeline_for_each(_test_info);
    all_passed &= d_tests_sa_pipeline_take(_test_info);
    all_passed &= d_tests_sa_pipeline_skip(_test_info);
    all_passed &= d_tests_sa_pipeline_chaining(_test_info);

    return all_passed;
}
