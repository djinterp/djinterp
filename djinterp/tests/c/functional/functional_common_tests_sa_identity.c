#include ".\functional_common_tests_sa.h"


/*
d_tests_sa_functional_identity_transformer
  Tests d_functional_identity_transformer for correctness.
  Tests the following:
  - valid input/output copies the input pointer into the output
  - context parameter is ignored (NULL context succeeds)
  - non-NULL context is accepted and ignored
  - NULL input returns false
  - NULL output returns false
  - both NULL returns false
  - output receives the exact pointer value of input
*/
bool
d_tests_sa_functional_identity_transformer
(
    struct d_test_counter* _counter
)
{
    bool           all_passed;
    int            value;
    const void*    output;
    const void*    input_ptr;
    int            dummy_context;
    bool           result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: valid input and output with NULL context ---
    value      = 42;
    output     = NULL;
    input_ptr  = &value;
    result     = d_functional_identity_transformer(input_ptr,
                                                   &output,
                                                   NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "identity_transformer: valid input/output returns true",
        "expected true for valid non-NULL input and output",
        _counter);

    all_passed &= d_assert_standalone(
        output == input_ptr,
        "identity_transformer: output receives input pointer",
        "expected output to hold the same pointer as input",
        _counter);

    // --- test: non-NULL context is accepted and ignored ---
    output        = NULL;
    dummy_context = 99;
    result        = d_functional_identity_transformer(input_ptr,
                                                     &output,
                                                     &dummy_context);

    all_passed &= d_assert_standalone(
        result == true,
        "identity_transformer: non-NULL context accepted",
        "expected true when context is non-NULL",
        _counter);

    all_passed &= d_assert_standalone(
        output == input_ptr,
        "identity_transformer: output correct with non-NULL context",
        "expected output pointer unchanged regardless of context",
        _counter);

    // --- test: NULL input returns false ---
    output = NULL;
    result = d_functional_identity_transformer(NULL,
                                               &output,
                                               NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "identity_transformer: NULL input returns false",
        "expected false when input is NULL",
        _counter);

    // --- test: NULL output returns false ---
    result = d_functional_identity_transformer(input_ptr,
                                               NULL,
                                               NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "identity_transformer: NULL output returns false",
        "expected false when output is NULL",
        _counter);

    // --- test: both NULL returns false ---
    result = d_functional_identity_transformer(NULL,
                                               NULL,
                                               NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "identity_transformer: both NULL returns false",
        "expected false when both input and output are NULL",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_identity_predicate
  Tests d_functional_identity_predicate for correctness.
  Tests the following:
  - non-NULL element returns true
  - NULL element returns false
  - context parameter is ignored (NULL context)
  - non-NULL context is accepted and ignored
  - various non-NULL pointer types all return true
*/
bool
d_tests_sa_functional_identity_predicate
(
    struct d_test_counter* _counter
)
{
    bool   all_passed;
    int    value;
    char   ch;
    double dbl;
    int    dummy_context;
    bool   result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: non-NULL int pointer returns true ---
    value  = 42;
    result = d_functional_identity_predicate(&value, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "identity_predicate: non-NULL int pointer returns true",
        "expected true for valid non-NULL element",
        _counter);

    // --- test: non-NULL char pointer returns true ---
    ch     = 'A';
    result = d_functional_identity_predicate(&ch, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "identity_predicate: non-NULL char pointer returns true",
        "expected true for valid non-NULL char element",
        _counter);

    // --- test: non-NULL double pointer returns true ---
    dbl    = 3.14;
    result = d_functional_identity_predicate(&dbl, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "identity_predicate: non-NULL double pointer returns true",
        "expected true for valid non-NULL double element",
        _counter);

    // --- test: NULL element returns false ---
    result = d_functional_identity_predicate(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "identity_predicate: NULL element returns false",
        "expected false when element is NULL",
        _counter);

    // --- test: non-NULL context is ignored ---
    dummy_context = 99;
    result        = d_functional_identity_predicate(&value,
                                                    &dummy_context);

    all_passed &= d_assert_standalone(
        result == true,
        "identity_predicate: non-NULL context is ignored",
        "expected true; context does not affect result",
        _counter);

    // --- test: NULL element with non-NULL context still false ---
    result = d_functional_identity_predicate(NULL,
                                             &dummy_context);

    all_passed &= d_assert_standalone(
        result == false,
        "identity_predicate: NULL element with context returns false",
        "expected false; context does not override NULL element",
        _counter);

    return all_passed;
}
