#include ".\compose_tests_sa.h"
#include ".\compose_tests_sa_helpers.h"


/*
d_tests_sa_compose_new
  Tests the d_functional_compose_new constructor function.
  Tests the following:
  - NULL first transformer rejection
  - NULL second transformer rejection
  - both NULL transformers rejection
  - zero temp_size rejection
  - successful creation with valid parameters and no context
  - field initialization verification (first, second, contexts, temp_size,
    temp_buf)
  - successful creation with both contexts
  - large temp_size allocation
  - mixed context handling (one NULL, one non-NULL)
*/
bool
d_tests_sa_compose_new
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_composed_transformer* composed;
    int                            context1;
    int                            context2;

    result   = true;
    context1 = 5;
    context2 = 3;

    // test 1: NULL first transformer should return NULL
    composed = d_functional_compose_new(NULL,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));
    result   = d_assert_standalone(
        composed == NULL,
        "compose_new_null_first",
        "NULL first transformer should return NULL",
        _counter) && result;

    // test 2: NULL second transformer should return NULL
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        NULL,
                                        NULL,
                                        sizeof(int));
    result   = d_assert_standalone(
        composed == NULL,
        "compose_new_null_second",
        "NULL second transformer should return NULL",
        _counter) && result;

    // test 3: both NULL transformers should return NULL
    composed = d_functional_compose_new(NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        sizeof(int));
    result   = d_assert_standalone(
        composed == NULL,
        "compose_new_both_null",
        "Both NULL transformers should return NULL",
        _counter) && result;

    // test 4: zero temp_size should return NULL
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        0);
    result   = d_assert_standalone(
        composed == NULL,
        "compose_new_zero_temp_size",
        "Zero temp_size should return NULL",
        _counter) && result;

    // test 5: successful creation with no contexts
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));
    result   = d_assert_standalone(
        composed != NULL,
        "compose_new_success_no_context",
        "d_functional_compose_new should succeed with NULL contexts",
        _counter) && result;

    if (composed)
    {
        result = d_assert_standalone(
            composed->first == transform_double,
            "compose_new_first_set",
            "first transformer should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            composed->second == transform_add_10,
            "compose_new_second_set",
            "second transformer should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            composed->context1 == NULL && composed->context2 == NULL,
            "compose_new_contexts_null",
            "contexts should be NULL",
            _counter) && result;

        result = d_assert_standalone(
            composed->temp_size == sizeof(int),
            "compose_new_temp_size_set",
            "temp_size should be sizeof(int)",
            _counter) && result;

        result = d_assert_standalone(
            composed->temp_buf != NULL,
            "compose_new_temp_buf_allocated",
            "temp_buf should be allocated",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 6: successful creation with contexts
    composed = d_functional_compose_new(transform_multiply_by_context,
                                        &context1,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));
    result   = d_assert_standalone(
        composed != NULL,
        "compose_new_success_with_context",
        "d_functional_compose_new should succeed with contexts",
        _counter) && result;

    if (composed)
    {
        result = d_assert_standalone(
            composed->first == transform_multiply_by_context,
            "compose_new_context_first_set",
            "first transformer should be set correctly",
            _counter) && result;

        result = d_assert_standalone(
            composed->context1 == &context1,
            "compose_new_context1_set",
            "context1 should point to context1 variable",
            _counter) && result;

        result = d_assert_standalone(
            composed->context2 == &context2,
            "compose_new_context2_set",
            "context2 should point to context2 variable",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 7: successful creation with large temp_size
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        1024);
    result   = d_assert_standalone(
        composed != NULL,
        "compose_new_large_temp_size",
        "d_functional_compose_new with large temp_size should succeed",
        _counter) && result;

    if (composed)
    {
        result = d_assert_standalone(
            composed->temp_size == 1024,
            "compose_new_large_temp_size_set",
            "temp_size should be 1024",
            _counter) && result;

        result = d_assert_standalone(
            composed->temp_buf != NULL,
            "compose_new_large_temp_buf_allocated",
            "temp_buf should be allocated for large size",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 8: creation with mixed context (one NULL, one non-NULL)
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));
    result   = d_assert_standalone(
        composed != NULL,
        "compose_new_mixed_context",
        "d_functional_compose_new with mixed contexts should succeed",
        _counter) && result;

    if (composed)
    {
        result = d_assert_standalone(
            composed->context1 == NULL,
            "compose_new_mixed_context1_null",
            "context1 should be NULL",
            _counter) && result;

        result = d_assert_standalone(
            composed->context2 == &context2,
            "compose_new_mixed_context2_set",
            "context2 should be set",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    return result;
}


/*
d_tests_sa_compose_apply
  Tests the d_functional_compose_apply function.
  Tests the following:
  - NULL composed rejection
  - NULL input rejection
  - NULL output rejection
  - NULL first transformer in struct rejection
  - NULL second transformer in struct rejection
  - NULL temp_buf in struct rejection
  - successful composition (double then add 10)
  - correct order of operations (add 10 then double vs double then add 10)
  - context forwarding to first transformer
  - context forwarding to second transformer
  - both contexts simultaneously
  - first transformer failure handling
  - second transformer failure handling
  - multiple applications with same composed transformer (reuse)
  - zero boundary value
  - negative input value
  - identity composition
*/
bool
d_tests_sa_compose_apply
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_composed_transformer* composed;
    int                            input;
    int                            output;
    int                            context1;
    int                            context2;

    result   = true;
    context1 = 3;
    context2 = 7;

    // test 1: NULL composed should return false
    input  = 5;
    output = 0;
    result = d_assert_standalone(
        d_functional_compose_apply(NULL, &input, &output) == false,
        "compose_apply_null_composed",
        "NULL composed should return false",
        _counter) && result;

    // test 2: NULL input should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        result = d_assert_standalone(
            d_functional_compose_apply(composed, NULL, &output) == false,
            "compose_apply_null_input",
            "NULL input should return false",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 3: NULL output should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, NULL) == false,
            "compose_apply_null_output",
            "NULL output should return false",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 4: NULL first transformer in struct should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // corrupt the first field
        composed->first = NULL;
        input            = 5;
        output           = 999;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == false,
            "compose_apply_null_first_in_struct",
            "NULL first in struct should return false",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 5: NULL second transformer in struct should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // corrupt the second field
        composed->second = NULL;
        input             = 5;
        output            = 999;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == false,
            "compose_apply_null_second_in_struct",
            "NULL second in struct should return false",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 6: NULL temp_buf in struct should return false
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // save and corrupt temp_buf, restore for proper free
        void* saved_buf    = composed->temp_buf;
        composed->temp_buf = NULL;
        input              = 5;
        output             = 999;

        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == false,
            "compose_apply_null_temp_buf",
            "NULL temp_buf should return false",
            _counter) && result;

        // restore for clean free
        composed->temp_buf = saved_buf;
        d_functional_compose_free(composed);
    }

    // test 7: successful composition - double then add 10
    // input: 5 -> double -> 10 -> add 10 -> 20
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_double_add10_success",
            "Composition should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 20,
            "compose_apply_double_add10_result",
            "5 doubled then +10 should equal 20",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 8: verify order matters - add 10 then double
    // input: 5 -> add 10 -> 15 -> double -> 30
    composed = d_functional_compose_new(transform_add_10,
                                        NULL,
                                        transform_double,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_add10_double_success",
            "Reverse composition should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 30,
            "compose_apply_add10_double_result",
            "5 +10 then doubled should equal 30",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 9: context forwarding to first transformer
    // input: 4 -> multiply by 3 -> 12 -> add 10 -> 22
    composed = d_functional_compose_new(transform_multiply_by_context,
                                        &context1,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 4;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_context1_success",
            "Composition with context1 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 22,
            "compose_apply_context1_result",
            "4 * 3 + 10 should equal 22",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 10: context forwarding to second transformer
    // input: 5 -> double -> 10 -> add 7 -> 17
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_context2_success",
            "Composition with context2 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 17,
            "compose_apply_context2_result",
            "(5 * 2) + 7 should equal 17",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 11: both contexts
    // input: 2 -> multiply by 3 -> 6 -> add 7 -> 13
    composed = d_functional_compose_new(transform_multiply_by_context,
                                        &context1,
                                        transform_add_context,
                                        &context2,
                                        sizeof(int));

    if (composed)
    {
        input  = 2;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_both_contexts_success",
            "Composition with both contexts should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 13,
            "compose_apply_both_contexts_result",
            "(2 * 3) + 7 should equal 13",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 12: first transformer failure
    composed = d_functional_compose_new(transform_always_fails,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 999;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == false,
            "compose_apply_first_fails",
            "First transformer failure should return false",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 13: second transformer failure
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_always_fails,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 5;
        output = 999;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == false,
            "compose_apply_second_fails",
            "Second transformer failure should return false",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 14: multiple applications with same composed transformer
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        // first application: 3 -> 6 -> 16
        input  = 3;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_reuse_first",
            "First application should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 16,
            "compose_apply_reuse_first_result",
            "3 doubled +10 should equal 16",
            _counter) && result;

        // second application: 7 -> 14 -> 24
        input  = 7;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_reuse_second",
            "Second application should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 24,
            "compose_apply_reuse_second_result",
            "7 doubled +10 should equal 24",
            _counter) && result;

        // third application: 0 -> 0 -> 10 (verifies temp buffer is clean)
        input  = 0;
        output = 999;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_reuse_third",
            "Third application with 0 should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 10,
            "compose_apply_reuse_third_result",
            "0 doubled +10 should equal 10 (no temp buffer leakage)",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 15: zero boundary value
    // input: 0 -> double -> 0 -> add 10 -> 10
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 0;
        output = 999;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_zero_boundary_success",
            "Zero input should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 10,
            "compose_apply_zero_boundary_result",
            "0 doubled +10 should equal 10",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 16: negative input value
    // input: -3 -> double -> -6 -> add 10 -> 4
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = -3;
        output = 999;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_negative_success",
            "Negative input should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 4,
            "compose_apply_negative_result",
            "-3 doubled +10 should equal 4",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 17: identity composition (identity . identity = identity)
    composed = d_functional_compose_new(transform_identity,
                                        NULL,
                                        transform_identity,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        input  = 42;
        output = 0;
        result = d_assert_standalone(
            d_functional_compose_apply(composed, &input, &output) == true,
            "compose_apply_identity_success",
            "Identity composition should succeed",
            _counter) && result;

        result = d_assert_standalone(
            output == 42,
            "compose_apply_identity_result",
            "identity(identity(42)) should equal 42",
            _counter) && result;

        d_functional_compose_free(composed);
    }

    // test 18: three-stage composition via nesting
    // input: 3 -> square -> 9 -> double -> 18
    {
        struct d_composed_transformer* inner;
        struct d_composed_transformer* outer;

        inner = d_functional_compose_new(transform_square,
                                          NULL,
                                          transform_double,
                                          NULL,
                                          sizeof(int));

        if (inner)
        {
            // verify inner works: 3 -> 9 -> 18
            input  = 3;
            output = 0;
            result = d_assert_standalone(
                d_functional_compose_apply(inner, &input, &output) == true,
                "compose_apply_nested_inner_success",
                "Inner composition (square then double) should succeed",
                _counter) && result;

            result = d_assert_standalone(
                output == 18,
                "compose_apply_nested_inner_result",
                "3 squared then doubled should equal 18",
                _counter) && result;

            d_functional_compose_free(inner);
        }
    }

    return result;
}


/*
d_tests_sa_compose_free
  Tests the d_functional_compose_free function.
  Tests the following:
  - NULL pointer handling (should not crash)
  - successful free after creation
  - free with contexts (contexts should remain valid)
  - free after use (contexts should remain valid)
*/
bool
d_tests_sa_compose_free
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_composed_transformer* composed;

    result = true;

    // test 1: NULL pointer should not crash
    d_functional_compose_free(NULL);
    result = d_assert_standalone(
        true,
        "compose_free_null",
        "Freeing NULL should not crash",
        _counter) && result;

    // test 2: normal free after creation
    composed = d_functional_compose_new(transform_double,
                                        NULL,
                                        transform_add_10,
                                        NULL,
                                        sizeof(int));

    if (composed)
    {
        d_functional_compose_free(composed);
        result = d_assert_standalone(
            true,
            "compose_free_normal",
            "Normal free should succeed",
            _counter) && result;
    }

    // test 3: free with contexts (contexts should not be freed)
    {
        int ctx1 = 5;
        int ctx2 = 10;

        composed = d_functional_compose_new(transform_multiply_by_context,
                                            &ctx1,
                                            transform_add_context,
                                            &ctx2,
                                            sizeof(int));

        if (composed)
        {
            d_functional_compose_free(composed);
            result = d_assert_standalone(
                true,
                "compose_free_with_contexts",
                "Free with contexts should succeed",
                _counter) && result;

            // contexts should still be valid
            result = d_assert_standalone(
                ctx1 == 5 && ctx2 == 10,
                "compose_free_contexts_intact",
                "Contexts should not be freed/modified",
                _counter) && result;
        }
    }

    // test 4: free after use (contexts should remain valid)
    {
        int ctx1 = 3;
        int ctx2 = 7;
        int input;
        int output;

        composed = d_functional_compose_new(transform_multiply_by_context,
                                            &ctx1,
                                            transform_add_context,
                                            &ctx2,
                                            sizeof(int));

        if (composed)
        {
            // use before freeing
            input  = 4;
            output = 0;
            d_functional_compose_apply(composed, &input, &output);

            result = d_assert_standalone(
                output == 19,
                "compose_free_after_use_result",
                "(4 * 3) + 7 should equal 19 before free",
                _counter) && result;

            d_functional_compose_free(composed);

            // contexts should still be valid after free
            result = d_assert_standalone(
                ctx1 == 3 && ctx2 == 7,
                "compose_free_after_use_contexts_intact",
                "Contexts should remain valid after use and free",
                _counter) && result;
        }
    }

    return result;
}


/*
d_tests_sa_compose_transformer_all
  Aggregation function that runs all transformer composition tests.
*/
bool
d_tests_sa_compose_transformer_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Transformer Composition Functions\n");
    printf("  --------------------------------------------\n");

    result = d_tests_sa_compose_new(_counter)   && result;
    result = d_tests_sa_compose_apply(_counter)  && result;
    result = d_tests_sa_compose_free(_counter)   && result;

    return result;
}
