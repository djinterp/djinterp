#include ".\functional_common_tests_sa.h"


/*
d_tests_sa_fc_identity_functions
  Tests identity transformer and predicate functions.
  Tests the following:
  - d_functional_identity_transformer with valid inputs
  - d_functional_identity_transformer with NULL inputs
  - d_functional_identity_predicate with non-NULL element
  - d_functional_identity_predicate with NULL element
*/
bool
d_tests_sa_fc_identity_functions
(
    struct d_test_counter* _counter
)
{
    bool        result;
    const void* input;
    const void* output;
    int         value;

    result = true;
    value  = 42;
    input  = &value;

    // test 1: identity transformer with valid input/output
    result = d_assert_standalone(
        d_functional_identity_transformer(input, (void*)&output, NULL) == true,
        "identity_transformer_valid",
        "Identity transformer should succeed with valid params",
        _counter) && result;

    result = d_assert_standalone(
        output == input,
        "identity_transformer_pointer_copy",
        "Output should point to input",
        _counter) && result;

    // test 2: identity transformer with NULL input
    result = d_assert_standalone(
        d_functional_identity_transformer(NULL, (void*)&output, NULL) == false,
        "identity_transformer_null_input",
        "NULL input should return false",
        _counter) && result;

    // test 3: identity transformer with NULL output
    result = d_assert_standalone(
        d_functional_identity_transformer(input, NULL, NULL) == false,
        "identity_transformer_null_output",
        "NULL output should return false",
        _counter) && result;

    // test 4: identity predicate with non-NULL element
    result = d_assert_standalone(
        d_functional_identity_predicate(&value, NULL) == true,
        "identity_predicate_non_null",
        "Non-NULL element should return true",
        _counter) && result;

    // test 5: identity predicate with NULL element
    result = d_assert_standalone(
        d_functional_identity_predicate(NULL, NULL) == false,
        "identity_predicate_null",
        "NULL element should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_constant_functions
  Tests constant true/false functions.
  Tests the following:
  - d_functional_constant_true always returns true
  - d_functional_constant_false always returns false
  - NULL parameters handled correctly
*/
bool
d_tests_sa_fc_constant_functions
(
    struct d_test_counter* _counter
)
{
    bool result;
    int  value;

    result = true;
    value  = 42;

    // test 1: constant_true with non-NULL element
    result = d_assert_standalone(
        d_functional_constant_true(&value, NULL) == true,
        "constant_true_non_null",
        "constant_true should return true with non-NULL",
        _counter) && result;

    // test 2: constant_true with NULL element
    result = d_assert_standalone(
        d_functional_constant_true(NULL, NULL) == true,
        "constant_true_null",
        "constant_true should return true with NULL",
        _counter) && result;

    // test 3: constant_false with non-NULL element
    result = d_assert_standalone(
        d_functional_constant_false(&value, NULL) == false,
        "constant_false_non_null",
        "constant_false should return false with non-NULL",
        _counter) && result;

    // test 4: constant_false with NULL element
    result = d_assert_standalone(
        d_functional_constant_false(NULL, NULL) == false,
        "constant_false_null",
        "constant_false should return false with NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_comparison_utilities
  Tests comparison and equality functions.
  Tests the following:
  - int comparison (less than, equal, greater than)
  - size_t comparison
  - double comparison
  - int equality
  - size_t equality
  - NULL parameter handling
*/
bool
d_tests_sa_fc_comparison_utilities
(
    struct d_test_counter* _counter
)
{
    bool   result;
    int    int_a;
    int    int_b;
    size_t size_a;
    size_t size_b;
    double double_a;
    double double_b;

    result   = true;
    int_a    = 5;
    int_b    = 10;
    size_a   = 100;
    size_b   = 200;
    double_a = 3.14;
    double_b = 2.71;

    // test 1: compare_int - less than
    result = d_assert_standalone(
        d_functional_compare_int(&int_a, &int_b, NULL) < 0,
        "compare_int_less_than",
        "5 < 10 should return negative",
        _counter) && result;

    // test 2: compare_int - equal
    int_b  = 5;
    result = d_assert_standalone(
        d_functional_compare_int(&int_a, &int_b, NULL) == 0,
        "compare_int_equal",
        "5 == 5 should return 0",
        _counter) && result;

    // test 3: compare_int - greater than
    int_b  = 3;
    result = d_assert_standalone(
        d_functional_compare_int(&int_a, &int_b, NULL) > 0,
        "compare_int_greater_than",
        "5 > 3 should return positive",
        _counter) && result;

    // test 4: compare_int with NULL
    result = d_assert_standalone(
        d_functional_compare_int(NULL, &int_b, NULL) == 0,
        "compare_int_null_a",
        "NULL first param should return 0",
        _counter) && result;

    // test 5: compare_size_t - less than
    result = d_assert_standalone(
        d_functional_compare_size_t(&size_a, &size_b, NULL) < 0,
        "compare_size_t_less_than",
        "100 < 200 should return negative",
        _counter) && result;

    // test 6: compare_size_t - equal
    size_b = 100;
    result = d_assert_standalone(
        d_functional_compare_size_t(&size_a, &size_b, NULL) == 0,
        "compare_size_t_equal",
        "100 == 100 should return 0",
        _counter) && result;

    // test 7: compare_double - greater than
    result = d_assert_standalone(
        d_functional_compare_double(&double_a, &double_b, NULL) > 0,
        "compare_double_greater_than",
        "3.14 > 2.71 should return positive",
        _counter) && result;

    // test 8: compare_double - less than
    double_a = 1.5;
    double_b = 2.71;
    result   = d_assert_standalone(
        d_functional_compare_double(&double_a, &double_b, NULL) < 0,
        "compare_double_less_than",
        "1.5 < 2.71 should return negative",
        _counter) && result;

    // test 9: equal_int - true
    int_a  = 42;
    int_b  = 42;
    result = d_assert_standalone(
        d_functional_equal_int(&int_a, &int_b, NULL) == true,
        "equal_int_true",
        "42 == 42 should return true",
        _counter) && result;

    // test 10: equal_int - false
    int_b  = 43;
    result = d_assert_standalone(
        d_functional_equal_int(&int_a, &int_b, NULL) == false,
        "equal_int_false",
        "42 != 43 should return false",
        _counter) && result;

    // test 11: equal_int with NULL
    result = d_assert_standalone(
        d_functional_equal_int(NULL, &int_b, NULL) == false,
        "equal_int_null",
        "NULL param should return false",
        _counter) && result;

    // test 12: equal_size_t - true
    size_a = 1000;
    size_b = 1000;
    result = d_assert_standalone(
        d_functional_equal_size_t(&size_a, &size_b, NULL) == true,
        "equal_size_t_true",
        "1000 == 1000 should return true",
        _counter) && result;

    // test 13: equal_size_t - false
    size_b = 999;
    result = d_assert_standalone(
        d_functional_equal_size_t(&size_a, &size_b, NULL) == false,
        "equal_size_t_false",
        "1000 != 999 should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_predicate_utilities
  Tests null-checking predicates.
  Tests the following:
  - d_functional_is_null with NULL pointer
  - d_functional_is_null with non-NULL pointer
  - d_functional_is_null with NULL element
  - d_functional_is_not_null with non-NULL pointer
  - d_functional_is_not_null with NULL pointer
  - d_functional_is_not_null with NULL element
*/
bool
d_tests_sa_fc_predicate_utilities
(
    struct d_test_counter* _counter
)
{
    bool   result;
    void*  null_ptr;
    void*  non_null_ptr;
    int    value;

    result      = true;
    null_ptr    = NULL;
    value       = 42;
    non_null_ptr = &value;

    // test 1: is_null with NULL pointer
    result = d_assert_standalone(
        d_functional_is_null(&null_ptr, NULL) == true,
        "is_null_with_null_ptr",
        "Pointer to NULL should return true",
        _counter) && result;

    // test 2: is_null with non-NULL pointer
    result = d_assert_standalone(
        d_functional_is_null(&non_null_ptr, NULL) == false,
        "is_null_with_non_null_ptr",
        "Pointer to non-NULL should return false",
        _counter) && result;

    // test 3: is_null with NULL element
    result = d_assert_standalone(
        d_functional_is_null(NULL, NULL) == true,
        "is_null_with_null_element",
        "NULL element should return true",
        _counter) && result;

    // test 4: is_not_null with non-NULL pointer
    result = d_assert_standalone(
        d_functional_is_not_null(&non_null_ptr, NULL) == true,
        "is_not_null_with_non_null_ptr",
        "Pointer to non-NULL should return true",
        _counter) && result;

    // test 5: is_not_null with NULL pointer
    result = d_assert_standalone(
        d_functional_is_not_null(&null_ptr, NULL) == false,
        "is_not_null_with_null_ptr",
        "Pointer to NULL should return false",
        _counter) && result;

    // test 6: is_not_null with NULL element
    result = d_assert_standalone(
        d_functional_is_not_null(NULL, NULL) == false,
        "is_not_null_with_null_element",
        "NULL element should return false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_fc_utility_all
  Aggregation function that runs all utility function tests.
*/
bool
d_tests_sa_fc_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_fc_identity_functions(_counter) && result;
    result = d_tests_sa_fc_constant_functions(_counter) && result;
    result = d_tests_sa_fc_comparison_utilities(_counter) && result;
    result = d_tests_sa_fc_predicate_utilities(_counter) && result;

    return result;
}
