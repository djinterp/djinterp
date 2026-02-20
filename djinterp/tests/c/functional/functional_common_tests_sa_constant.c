#include ".\functional_common_tests_sa.h"


/*
d_tests_sa_functional_constant_true
  Tests d_functional_constant_true for correctness.
  Tests the following:
  - returns true with non-NULL element and NULL context
  - returns true with NULL element and NULL context
  - returns true with non-NULL element and non-NULL context
  - returns true with NULL element and non-NULL context
  - always returns true regardless of any argument combination
*/
bool
d_tests_sa_functional_constant_true
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  value;
    int  dummy_context;
    bool result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed    = true;
    value         = 42;
    dummy_context = 99;

    // --- test: non-NULL element, NULL context ---
    result = d_functional_constant_true(&value, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "constant_true: non-NULL element, NULL context",
        "expected true regardless of arguments",
        _counter);

    // --- test: NULL element, NULL context ---
    result = d_functional_constant_true(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "constant_true: NULL element, NULL context",
        "expected true even with NULL element",
        _counter);

    // --- test: non-NULL element, non-NULL context ---
    result = d_functional_constant_true(&value, &dummy_context);

    all_passed &= d_assert_standalone(
        result == true,
        "constant_true: non-NULL element, non-NULL context",
        "expected true regardless of context",
        _counter);

    // --- test: NULL element, non-NULL context ---
    result = d_functional_constant_true(NULL, &dummy_context);

    all_passed &= d_assert_standalone(
        result == true,
        "constant_true: NULL element, non-NULL context",
        "expected true regardless of all arguments",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_constant_false
  Tests d_functional_constant_false for correctness.
  Tests the following:
  - returns false with non-NULL element and NULL context
  - returns false with NULL element and NULL context
  - returns false with non-NULL element and non-NULL context
  - returns false with NULL element and non-NULL context
  - always returns false regardless of any argument combination
*/
bool
d_tests_sa_functional_constant_false
(
    struct d_test_counter* _counter
)
{
    bool all_passed;
    int  value;
    int  dummy_context;
    bool result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed    = true;
    value         = 42;
    dummy_context = 99;

    // --- test: non-NULL element, NULL context ---
    result = d_functional_constant_false(&value, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "constant_false: non-NULL element, NULL context",
        "expected false regardless of arguments",
        _counter);

    // --- test: NULL element, NULL context ---
    result = d_functional_constant_false(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "constant_false: NULL element, NULL context",
        "expected false even with NULL element",
        _counter);

    // --- test: non-NULL element, non-NULL context ---
    result = d_functional_constant_false(&value, &dummy_context);

    all_passed &= d_assert_standalone(
        result == false,
        "constant_false: non-NULL element, non-NULL context",
        "expected false regardless of context",
        _counter);

    // --- test: NULL element, non-NULL context ---
    result = d_functional_constant_false(NULL, &dummy_context);

    all_passed &= d_assert_standalone(
        result == false,
        "constant_false: NULL element, non-NULL context",
        "expected false regardless of all arguments",
        _counter);

    return all_passed;
}
