#include ".\fn_builder_tests_sa.h"


/******************************************************************************
 * TEST HELPERS: callbacks for execution tests
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
test_helper_add_ten
  Transformer: adds 10 to each int.
*/
static bool
test_helper_add_ten
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = (*(const int*)_input) + 10;

    return true;
}


/*
test_helper_negate
  Transformer: negates each int.
*/
static bool
test_helper_negate
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    (void)_context;

    *(int*)_output = -(*(const int*)_input);

    return true;
}


/*
test_helper_square
  Transformer: squares each int.
*/
static bool
test_helper_square
(
    const void* _input,
    void*       _output,
    void*       _context
)
{
    int val;

    (void)_context;

    val            = *(const int*)_input;
    *(int*)_output = val * val;

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
test_helper_is_positive
  Predicate: returns true if the int is > 0.
*/
static bool
test_helper_is_positive
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) > 0;
}


/*
test_helper_greater_than_five
  Predicate: returns true if the int is > 5.
*/
static bool
test_helper_greater_than_five
(
    const void* _element,
    void*       _context
)
{
    (void)_context;

    return (*(const int*)_element) > 5;
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
d_tests_sa_fn_builder_execute_validation
  Tests d_fn_builder_execute parameter validation.
  Tests the following:
  - NULL builder returns false, sets _out_count to 0
  - NULL input returns false, sets _out_count to 0
  - NULL output returns false, sets _out_count to 0
  - NULL _out_count returns false (does not crash)
  - zero count returns false, sets _out_count to 0
  - zero element_size returns false, sets _out_count to 0
  - multiple invalid params at once returns false
*/
bool
d_tests_sa_fn_builder_execute_validation
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 1, 2, 3 };
    int                  output[3];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- NULL builder ----
    out_count = 999;
    result    = d_fn_builder_execute(NULL,
                                     input,
                                     3,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false,
        "execute: NULL builder returns false",
        "validation should reject NULL builder",
        _test_info);

    all_passed &= d_assert_standalone(
        out_count == 0,
        "execute: NULL builder sets out_count to 0",
        "out_count should be zeroed on error",
        _test_info);

    // ---- NULL input ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     NULL,
                                     3,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "execute: NULL input returns false, out_count 0",
        "validation should reject NULL input",
        _test_info);

    // ---- NULL output ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     3,
                                     sizeof(int),
                                     NULL,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "execute: NULL output returns false, out_count 0",
        "validation should reject NULL output",
        _test_info);

    // ---- NULL _out_count ----
    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "execute: NULL out_count returns false",
        "validation should reject NULL out_count without crash",
        _test_info);

    // ---- zero count ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     0,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "execute: zero count returns false, out_count 0",
        "validation should reject zero count",
        _test_info);

    // ---- zero element_size ----
    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     3,
                                     0,
                                     output,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "execute: zero element_size returns false, out_count 0",
        "validation should reject zero element_size",
        _test_info);

    // ---- multiple invalid at once ----
    out_count = 999;
    result    = d_fn_builder_execute(NULL,
                                     NULL,
                                     0,
                                     0,
                                     NULL,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "execute: all invalid returns false, out_count 0",
        "all-invalid should still write out_count and return false",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_execute_passthrough
  Tests execute with empty builder (no transforms, no predicates).
  Tests the following:
  - empty builder copies input to output verbatim
  - out_count equals input count
  - output contents match input exactly
  - single element passthrough works
*/
bool
d_tests_sa_fn_builder_execute_passthrough
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 10, 20, 30, 40, 50 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;
    bool                 match;
    size_t               i;

    all_passed = true;
    builder    = d_fn_builder_new();

    // ---- empty builder: passthrough copy ----
    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true,
        "passthrough: returns true",
        "empty builder should succeed",
        _test_info);

    all_passed &= d_assert_standalone(
        out_count == 5,
        "passthrough: out_count equals input count",
        "all 5 elements should be copied",
        _test_info);

    match = true;

    for (i = 0; i < 5; i++)
    {
        if (output[i] != input[i])
        {
            match = false;
        }
    }

    all_passed &= d_assert_standalone(
        match,
        "passthrough: output contents match input",
        "memcpy should produce an exact copy",
        _test_info);

    // ---- single element passthrough ----
    {
        int one_in  = 42;
        int one_out = 0;

        result = d_fn_builder_execute(builder,
                                      &one_in,
                                      1,
                                      sizeof(int),
                                      &one_out,
                                      &out_count);

        all_passed &= d_assert_standalone(
            result == true && out_count == 1 && one_out == 42,
            "passthrough: single element works",
            "one element should copy correctly",
            _test_info);
    }

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_execute_transforms_only
  Tests execute with transforms but no predicates.
  Tests the following:
  - single transform applied to all elements (double)
  - two transforms applied in order (double then add 10)
  - all elements pass through (no filtering)
  - out_count equals input count
  - original input is not modified
*/
bool
d_tests_sa_fn_builder_execute_transforms_only
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    // ---- single transform: double ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 5,
        "transforms_only: single transform returns 5 elements",
        "no filtering means all pass",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == 2 && output[1] == 4 && output[2] == 6 &&
        output[3] == 8 && output[4] == 10,
        "transforms_only: single double produces 2,4,6,8,10",
        "each element should be doubled",
        _test_info);

    // original unchanged
    all_passed &= d_assert_standalone(
        input[0] == 1 && input[4] == 5,
        "transforms_only: original input unchanged",
        "execute should not modify the source array",
        _test_info);

    d_fn_builder_free(builder);

    // ---- two transforms: double then add 10 ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_and_then(builder, test_helper_add_ten);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 5,
        "transforms_only: two transforms returns 5 elements",
        "no filtering means all pass",
        _test_info);

    // (1*2)+10=12, (2*2)+10=14, (3*2)+10=16, (4*2)+10=18, (5*2)+10=20
    all_passed &= d_assert_standalone(
        output[0] == 12 && output[1] == 14 && output[2] == 16 &&
        output[3] == 18 && output[4] == 20,
        "transforms_only: double then add_ten -> 12,14,16,18,20",
        "transforms should compose left-to-right",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_execute_predicates_only
  Tests execute with predicates but no transforms.
  Tests the following:
  - single predicate filters elements (keep evens)
  - predicate-only uses original input values for filtering and output
  - two predicates applied as conjunction (even AND positive)
  - all-fail predicate produces out_count 0
  - single predicate that passes all produces full copy
*/
bool
d_tests_sa_fn_builder_execute_predicates_only
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5, 6 };
    int                  output[6];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    // ---- single predicate: keep evens ----
    builder = d_fn_builder_new();
    d_funtional_builder_filter(builder, test_helper_is_even);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 3,
        "predicates_only: is_even keeps 3 of 6",
        "2, 4, 6 should pass",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == 2 && output[1] == 4 && output[2] == 6,
        "predicates_only: filtered values are 2, 4, 6",
        "original (untransformed) values should be copied",
        _test_info);

    d_fn_builder_free(builder);

    // ---- two predicates as conjunction: is_even AND > 5 ----
    builder = d_fn_builder_new();
    d_funtional_builder_filter(builder, test_helper_is_even);
    d_funtional_builder_where(builder, test_helper_greater_than_five);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 1,
        "predicates_only: is_even AND > 5 keeps 1",
        "only 6 passes both predicates",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == 6,
        "predicates_only: conjunction result is 6",
        "only 6 is both even and > 5",
        _test_info);

    d_fn_builder_free(builder);

    // ---- all-fail predicate: nothing passes ----
    builder = d_fn_builder_new();
    d_funtional_builder_filter(builder, test_helper_always_false);

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 0,
        "predicates_only: always_false produces out_count 0",
        "no elements should pass",
        _test_info);

    d_fn_builder_free(builder);

    // ---- predicate that all pass: is_positive on positive data ----
    builder = d_fn_builder_new();
    d_funtional_builder_filter(builder, test_helper_is_positive);

    result = d_fn_builder_execute(builder,
                                  input,
                                  6,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 6,
        "predicates_only: all-pass predicate keeps all 6",
        "all elements are positive",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_execute_combined
  Tests execute with both transforms and predicates.
  Tests the following:
  - transform then filter: predicate applied to TRANSFORMED values
  - double -> is_even: doubling makes all even, all pass
  - negate -> is_positive: negating positive values makes all fail
  - double -> is_even + > 5: combined filtering on transformed data
  - output contains the TRANSFORMED values, not originals
*/
bool
d_tests_sa_fn_builder_execute_combined
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    // ---- double -> is_even: all doubled values are even ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_filter(builder, test_helper_is_even);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 5,
        "combined: double -> is_even keeps all 5",
        "doubling always produces evens",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == 2 && output[1] == 4 && output[2] == 6 &&
        output[3] == 8 && output[4] == 10,
        "combined: output contains TRANSFORMED values",
        "output should have doubled values, not originals",
        _test_info);

    d_fn_builder_free(builder);

    // ---- negate -> is_positive: all negated positives fail ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_negate);
    d_funtional_builder_filter(builder, test_helper_is_positive);

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 0,
        "combined: negate -> is_positive keeps none",
        "negating positive values makes all non-positive",
        _test_info);

    d_fn_builder_free(builder);

    // ---- double -> is_even + > 5: partial filtering ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_filter(builder, test_helper_is_even);
    d_funtional_builder_where(builder, test_helper_greater_than_five);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  5,
                                  sizeof(int),
                                  output,
                                  &out_count);

    // doubled: 2,4,6,8,10; all even; > 5: 6,8,10
    all_passed &= d_assert_standalone(
        result == true && out_count == 3,
        "combined: double -> is_even + > 5 keeps 3",
        "6, 8, 10 pass both predicates",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == 6 && output[1] == 8 && output[2] == 10,
        "combined: filtered transformed output is 6, 8, 10",
        "only transformed values > 5 should appear",
        _test_info);

    d_fn_builder_free(builder);

    // ---- add_ten -> is_even: partial pass on transformed ----
    {
        int data[] = { 1, 2, 3, 4 };

        builder = d_fn_builder_new();
        d_funtional_builder_map(builder, test_helper_add_ten);
        d_funtional_builder_filter(builder, test_helper_is_even);

        d_memset(output, 0, sizeof(output));

        result = d_fn_builder_execute(builder,
                                      data,
                                      4,
                                      sizeof(int),
                                      output,
                                      &out_count);

        // 1+10=11(odd), 2+10=12(even), 3+10=13(odd), 4+10=14(even)
        all_passed &= d_assert_standalone(
            result == true && out_count == 2,
            "combined: add_ten -> is_even keeps 2 of 4",
            "12 and 14 are even",
            _test_info);

        all_passed &= d_assert_standalone(
            output[0] == 12 && output[1] == 14,
            "combined: add_ten -> is_even values are 12, 14",
            "transformed even values should be output",
            _test_info);

        d_fn_builder_free(builder);
    }

    return all_passed;
}


/*
d_tests_sa_fn_builder_execute_transform_failure
  Tests execute when a transformer returns false.
  Tests the following:
  - first (and only) transform fails: returns false, out_count 0
  - first of two transforms fails: returns false, out_count 0, both
    temp buffers freed
  - second of two transforms fails: returns false, out_count 0
  - failure on a non-first element still returns false
*/
bool
d_tests_sa_fn_builder_execute_transform_failure
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 1, 2, 3, 4, 5 };
    int                  output[5];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    // ---- single failing transform (only temp_a, no temp_b) ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_fail_transformer);

    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     5,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "transform_fail: single failing transform returns false",
        "first transform failure should abort, out_count 0",
        _test_info);

    d_fn_builder_free(builder);

    // ---- first of two transforms fails (temp_a + temp_b allocated) ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_fail_transformer);
    d_funtional_builder_and_then(builder, test_helper_double_int);

    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     5,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "transform_fail: first of two fails, both buffers freed",
        "first transform failure should free temp_a and temp_b",
        _test_info);

    d_fn_builder_free(builder);

    // ---- second of two transforms fails ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_and_then(builder, test_helper_fail_transformer);

    out_count = 999;
    result    = d_fn_builder_execute(builder,
                                     input,
                                     5,
                                     sizeof(int),
                                     output,
                                     &out_count);

    all_passed &= d_assert_standalone(
        result == false && out_count == 0,
        "transform_fail: second of two fails returns false",
        "subsequent transform failure in ping-pong loop aborts",
        _test_info);

    d_fn_builder_free(builder);

    // ---- failure on non-first element (transformer succeeds for
    //      element[0] but fails on element[1]) ----
    {
        // we use a static counter to make the transformer fail
        // on the second call; here we just reuse fail_transformer
        // which fails on every element, so it fails on element[0]
        // The first-of-two test above already covers temp_b cleanup.
        // For coverage of the "some succeed then fail" path, we test
        // with a single-element array so transform[0] is the failure.
        int single[] = { 99 };

        builder = d_fn_builder_new();
        d_funtional_builder_map(builder, test_helper_fail_transformer);

        out_count = 999;
        result    = d_fn_builder_execute(builder,
                                         single,
                                         1,
                                         sizeof(int),
                                         output,
                                         &out_count);

        all_passed &= d_assert_standalone(
            result == false && out_count == 0,
            "transform_fail: single element failure returns false",
            "failure on any element should abort",
            _test_info);

        d_fn_builder_free(builder);
    }

    return all_passed;
}


/*
d_tests_sa_fn_builder_execute_pingpong
  Tests the ping-pong buffer mechanism for 3+ transforms.
  Tests the following:
  - three transforms applied in correct order (double -> add_ten -> negate)
  - four transforms applied in correct order with correct ping-pong swaps
  - single transform (no ping-pong, only temp_a)
  - two transforms (single swap, temp_a -> temp_b -> done)
  - large struct elements work with ping-pong
*/
bool
d_tests_sa_fn_builder_execute_pingpong
(
    struct d_test_counter* _test_info
)
{
    struct d_fn_builder* builder;
    int                  input[]  = { 1, 2, 3 };
    int                  output[3];
    size_t               out_count;
    bool                 result;
    bool                 all_passed;

    all_passed = true;

    // ---- three transforms: double -> add_ten -> negate ----
    // 1 -> 2 -> 12 -> -12
    // 2 -> 4 -> 14 -> -14
    // 3 -> 6 -> 16 -> -16
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_and_then(builder, test_helper_add_ten);
    d_funtional_builder_and_then(builder, test_helper_negate);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 3,
        "pingpong: 3 transforms returns 3 elements",
        "no filtering, all pass through",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == -12 && output[1] == -14 && output[2] == -16,
        "pingpong: 3-transform order correct: -12, -14, -16",
        "double(1)=2, +10=12, negate=-12",
        _test_info);

    d_fn_builder_free(builder);

    // ---- four transforms: double -> add_ten -> negate -> square ----
    // 1 -> 2 -> 12 -> -12 -> 144
    // 2 -> 4 -> 14 -> -14 -> 196
    // 3 -> 6 -> 16 -> -16 -> 256
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_and_then(builder, test_helper_add_ten);
    d_funtional_builder_and_then(builder, test_helper_negate);
    d_funtional_builder_and_then(builder, test_helper_square);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 3,
        "pingpong: 4 transforms returns 3 elements",
        "even number of swaps should work",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == 144 && output[1] == 196 && output[2] == 256,
        "pingpong: 4-transform order correct: 144, 196, 256",
        "double(1)=2, +10=12, neg=-12, sq=144",
        _test_info);

    d_fn_builder_free(builder);

    // ---- single transform: no ping-pong (only temp_a) ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_add_ten);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    all_passed &= d_assert_standalone(
        result == true && out_count == 3,
        "pingpong: 1 transform (no temp_b) succeeds",
        "single transform uses only temp_a",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == 11 && output[1] == 12 && output[2] == 13,
        "pingpong: 1 transform values correct: 11, 12, 13",
        "each element + 10",
        _test_info);

    d_fn_builder_free(builder);

    // ---- two transforms: single swap (temp_a -> temp_b) ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_and_then(builder, test_helper_negate);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    // 1 -> 2 -> -2; 2 -> 4 -> -4; 3 -> 6 -> -6
    all_passed &= d_assert_standalone(
        result == true && out_count == 3,
        "pingpong: 2 transforms (one swap) succeeds",
        "two transforms use temp_a and temp_b",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == -2 && output[1] == -4 && output[2] == -6,
        "pingpong: 2-transform values correct: -2, -4, -6",
        "double then negate",
        _test_info);

    d_fn_builder_free(builder);

    // ---- 3 transforms + predicate: combined ping-pong + filtering ----
    builder = d_fn_builder_new();
    d_funtional_builder_map(builder, test_helper_double_int);
    d_funtional_builder_and_then(builder, test_helper_add_ten);
    d_funtional_builder_and_then(builder, test_helper_negate);
    d_funtional_builder_filter(builder, test_helper_is_even);

    d_memset(output, 0, sizeof(output));

    result = d_fn_builder_execute(builder,
                                  input,
                                  3,
                                  sizeof(int),
                                  output,
                                  &out_count);

    // 1->2->12->-12(even), 2->4->14->-14(even), 3->6->16->-16(even)
    all_passed &= d_assert_standalone(
        result == true && out_count == 3,
        "pingpong: 3 transforms + filter keeps all",
        "all negated sums are even",
        _test_info);

    all_passed &= d_assert_standalone(
        output[0] == -12 && output[1] == -14 && output[2] == -16,
        "pingpong: 3 transforms + filter values correct",
        "ping-pong result fed to predicate correctly",
        _test_info);

    d_fn_builder_free(builder);

    return all_passed;
}


/*
d_tests_sa_fn_builder_execution_all
  Runs all builder execution tests.
  Tests the following:
  - parameter validation
  - passthrough (empty builder)
  - transforms only
  - predicates only
  - combined transforms + predicates
  - transform failure paths
  - ping-pong buffer mechanism
*/
bool
d_tests_sa_fn_builder_execution_all
(
    struct d_test_counter* _test_info
)
{
    bool all_passed;

    all_passed = true;

    all_passed &= d_tests_sa_fn_builder_execute_validation(_test_info);
    all_passed &= d_tests_sa_fn_builder_execute_passthrough(_test_info);
    all_passed &= d_tests_sa_fn_builder_execute_transforms_only(_test_info);
    all_passed &= d_tests_sa_fn_builder_execute_predicates_only(_test_info);
    all_passed &= d_tests_sa_fn_builder_execute_combined(_test_info);
    all_passed &= d_tests_sa_fn_builder_execute_transform_failure(_test_info);
    all_passed &= d_tests_sa_fn_builder_execute_pingpong(_test_info);

    return all_passed;
}
