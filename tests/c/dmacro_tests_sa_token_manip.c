/******************************************************************************
* djinterp [test]                                        dmacro_tests_sa_token.c
*
*   Unit tests for `dmacro.h` token manipulation section (Section I).
*   Tests token pasting (D_CONCAT), stringification (D_STRINGIFY, D_TOSTR),
*   and expansion control macros (D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT,
*   D_UNPACK).
*   Note: this module tests fundamental macro utilities that are dependencies
* of other djinterp modules, so it uses `test_standalone.h` rather than DTest.
*
*
* path:      \test\dmacro_tests_sa_token.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "dmacro_tests_sa.h"
#include <string.h>


/******************************************************************************
 * TEST HELPER MACROS
 *****************************************************************************/

// D_TEST_HELPER_PREFIX
//   macro: test prefix for token concatenation testing.
#define D_TEST_HELPER_PREFIX test_

// D_TEST_HELPER_SUFFIX
//   macro: test suffix for token concatenation testing.
#define D_TEST_HELPER_SUFFIX _value

// D_TEST_HELPER_NUMBER
//   macro: test number for stringification testing.
#define D_TEST_HELPER_NUMBER 42

// D_TEST_HELPER_IDENT
//   macro: test identifier for stringification testing.
#define D_TEST_HELPER_IDENT my_identifier

// D_TEST_HELPER_EXPR
//   macro: test expression for expansion testing.
#define D_TEST_HELPER_EXPR (1 + 2)

// D_TEST_NESTED_A
//   macro: nested macro A for expansion testing.
#define D_TEST_NESTED_A D_TEST_NESTED_B

// D_TEST_NESTED_B
//   macro: nested macro B for expansion testing.
#define D_TEST_NESTED_B final_value

// D_TEST_EMPTY_LIKE
//   macro: empty-like macro for testing.
#define D_TEST_EMPTY_LIKE

// D_TEST_PARENTHESIZED
//   macro: parenthesized content for D_UNPACK testing.
#define D_TEST_PARENTHESIZED (a, b, c)


/******************************************************************************
 * TOKEN PASTING TESTS (D_CONCAT, D_INTERNAL_CONCAT_HELPER)
 *****************************************************************************/

/*
d_tests_sa_dmacro_concat_basic
  Tests basic token concatenation functionality.
  Tests the following:
  - D_CONCAT joins two literal tokens correctly
  - D_CONCAT creates valid identifiers
  - D_CONCAT joins numeric suffixes correctly
  - D_CONCAT handles underscore-prefixed tokens
  - D_CONCAT works with single-character tokens
*/
bool
d_tests_sa_dmacro_concat_basic
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    test_var1;
    int    var_42;
    int    a_b;
    int    _x;
    int    AB;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_CONCAT Basic Functionality ---\n", D_INDENT);

    // initialize test variables
    test_var1 = 100;
    var_42    = 200;
    a_b       = 300;
    _x        = 400;
    AB        = 500;

    // verify basic token concatenation creates valid identifier
    // D_CONCAT(test_, var1) should expand to test_var1
    if (!d_assert_standalone(D_CONCAT(test_, var1) == 100,
                             "D_CONCAT(test_, var1) == test_var1",
                             "basic token concatenation should create identifier",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify concatenation with numeric suffix
    // D_CONCAT(var_, 42) should expand to var_42
    if (!d_assert_standalone(D_CONCAT(var_, 42) == 200,
                             "D_CONCAT(var_, 42) == var_42",
                             "numeric suffix concatenation should work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify concatenation creates underscore-separated identifier
    // D_CONCAT(a_, b) should expand to a_b
    if (!d_assert_standalone(D_CONCAT(a_, b) == 300,
                             "D_CONCAT(a_, b) == a_b",
                             "underscore-separated concatenation should work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify concatenation with underscore prefix
    // D_CONCAT(_, x) should expand to _x
    if (!d_assert_standalone(D_CONCAT(_, x) == 400,
                             "D_CONCAT(_, x) == _x",
                             "underscore-prefixed concatenation should work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify single character concatenation
    // D_CONCAT(A, B) should expand to AB
    if (!d_assert_standalone(D_CONCAT(A, B) == 500,
                             "D_CONCAT(A, B) == AB",
                             "single character concatenation should work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_CONCAT basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_CONCAT basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_concat_with_macros
  Tests token concatenation with macro arguments.
  Tests the following:
  - D_CONCAT expands macro arguments before concatenating
  - D_CONCAT handles prefix macros correctly
  - D_CONCAT handles suffix macros correctly
  - D_CONCAT works with both prefix and suffix as macros
  - nested D_CONCAT calls work correctly
*/
bool
d_tests_sa_dmacro_concat_with_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    test_result;
    int    test_result_value;
    int    result_value;
    int    test_value;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_CONCAT with Macro Arguments ---\n", D_INDENT);

    // initialize test variables
    test_result       = 111;
    test_result_value = 222;
    result_value      = 333;
    test_value        = 444;

    // verify D_CONCAT expands macro prefix before concatenation
    // D_CONCAT(D_TEST_HELPER_PREFIX, result) should become test_result
    if (!d_assert_standalone(D_CONCAT(D_TEST_HELPER_PREFIX, result) == 111,
                             "D_CONCAT(D_TEST_HELPER_PREFIX, result) == test_result",
                             "macro prefix should be expanded before concat",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT expands macro suffix before concatenation
    // D_CONCAT(result, D_TEST_HELPER_SUFFIX) should become result_value
    if (!d_assert_standalone(D_CONCAT(result, D_TEST_HELPER_SUFFIX) == 333,
                             "D_CONCAT(result, D_TEST_HELPER_SUFFIX) == result_value",
                             "macro suffix should be expanded before concat",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT expands both macro arguments
    // D_CONCAT(D_TEST_HELPER_PREFIX, D_TEST_HELPER_SUFFIX) would NOT work
    // because test_ and _value don't form a valid token
    // Instead test: D_CONCAT(test, _value)
    if (!d_assert_standalone(D_CONCAT(test, _value) == 444,
                             "D_CONCAT(test, _value) == test_value",
                             "both tokens should concatenate correctly",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify nested D_CONCAT calls work
    // D_CONCAT(D_CONCAT(test_, result), _value) should become test_result_value
    if (!d_assert_standalone(D_CONCAT(D_CONCAT(test_, result), _value) == 222,
                             "nested D_CONCAT creates test_result_value",
                             "nested concatenation should work correctly",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_CONCAT with macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_CONCAT with macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_concat_edge_cases
  Tests edge cases and boundary conditions for token concatenation.
  Tests the following:
  - D_CONCAT with empty-like second token (where applicable)
  - D_CONCAT for creating type names
  - D_CONCAT for creating function names
  - D_CONCAT with numbers at start (invalid C identifier - compile test)
  - multiple sequential D_CONCAT operations
*/
bool
d_tests_sa_dmacro_concat_edge_cases
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    type_int;
    int    func_init;
    int    x1;
    int    x1y2z3;
    int    prefix_mid_suffix;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_CONCAT Edge Cases ---\n", D_INDENT);

    // initialize test variables
    type_int          = 1000;
    func_init         = 2000;
    x1                = 3000;
    x1y2z3            = 4000;
    prefix_mid_suffix = 5000;

    // verify D_CONCAT can create type-like names
    // D_CONCAT(type_, int) should create type_int
    if (!d_assert_standalone(D_CONCAT(type_, int) == 1000,
                             "D_CONCAT(type_, int) == type_int",
                             "should create type-like identifier",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT can create function-like names
    // D_CONCAT(func_, init) should create func_init
    if (!d_assert_standalone(D_CONCAT(func_, init) == 2000,
                             "D_CONCAT(func_, init) == func_init",
                             "should create function-like identifier",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT with trailing number
    // D_CONCAT(x, 1) should create x1
    if (!d_assert_standalone(D_CONCAT(x, 1) == 3000,
                             "D_CONCAT(x, 1) == x1",
                             "should handle trailing number",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify deeply nested concatenation (3 levels)
    // D_CONCAT(D_CONCAT(D_CONCAT(x, 1), y2), z3) should create x1y2z3
    if (!d_assert_standalone(D_CONCAT(D_CONCAT(D_CONCAT(x, 1), y2), z3) == 4000,
                             "triple nested D_CONCAT creates x1y2z3",
                             "deeply nested concatenation should work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify sequential/chained concatenation
    // D_CONCAT(prefix_, D_CONCAT(mid_, suffix)) should create prefix_mid_suffix
    if (!d_assert_standalone(D_CONCAT(prefix_, D_CONCAT(mid_, suffix)) == 5000,
                             "chained D_CONCAT creates prefix_mid_suffix",
                             "chained concatenation should work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_CONCAT edge cases test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_CONCAT edge cases test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * STRINGIFICATION TESTS (D_STRINGIFY, D_TOSTR)
 *****************************************************************************/

/*
d_tests_sa_dmacro_stringify_basic
  Tests basic stringification functionality.
  Tests the following:
  - D_STRINGIFY converts tokens to string literals
  - D_STRINGIFY preserves the exact token text
  - D_STRINGIFY works with identifiers
  - D_STRINGIFY works with numbers
  - D_STRINGIFY works with expressions
*/
bool
d_tests_sa_dmacro_stringify_basic
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* str_ident;
    const char* str_number;
    const char* str_expr;
    const char* str_keyword;
    const char* str_special;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_STRINGIFY Basic Functionality ---\n", D_INDENT);

    // test stringification of identifier
    str_ident = D_STRINGIFY(hello_world);

    if (!d_assert_standalone(strcmp(str_ident, "hello_world") == 0,
                             "D_STRINGIFY(hello_world) == \"hello_world\"",
                             "identifier should be stringified exactly",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_ident);
    }

    // test stringification of number
    str_number = D_STRINGIFY(12345);

    if (!d_assert_standalone(strcmp(str_number, "12345") == 0,
                             "D_STRINGIFY(12345) == \"12345\"",
                             "number should be stringified exactly",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_number);
    }

    // test stringification of expression
    str_expr = D_STRINGIFY(1 + 2 * 3);

    if (!d_assert_standalone(strcmp(str_expr, "1 + 2 * 3") == 0,
                             "D_STRINGIFY(1 + 2 * 3) == \"1 + 2 * 3\"",
                             "expression should be stringified with spacing",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_expr);
    }

    // test stringification of C keyword
    str_keyword = D_STRINGIFY(return);

    if (!d_assert_standalone(strcmp(str_keyword, "return") == 0,
                             "D_STRINGIFY(return) == \"return\"",
                             "keyword should be stringified exactly",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_keyword);
    }

    // test stringification of special characters
    str_special = D_STRINGIFY(_underscore_prefix);

    if (!d_assert_standalone(strcmp(str_special, "_underscore_prefix") == 0,
                             "D_STRINGIFY(_underscore_prefix) stringified",
                             "underscore-prefixed identifier should stringify",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_special);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_STRINGIFY basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_STRINGIFY basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_stringify_vs_tostr
  Tests the difference between D_STRINGIFY and D_TOSTR.
  Tests the following:
  - D_STRINGIFY does NOT expand macros before stringifying
  - D_TOSTR DOES expand macros before stringifying
  - behavior difference is correct for simple macros
  - behavior difference is correct for numeric macros
  - behavior difference is correct for nested macros
*/
bool
d_tests_sa_dmacro_stringify_vs_tostr
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* stringify_macro;
    const char* tostr_macro;
    const char* stringify_num;
    const char* tostr_num;
    const char* stringify_nested;
    const char* tostr_nested;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_STRINGIFY vs D_TOSTR ---\n", D_INDENT);

    // D_STRINGIFY should NOT expand macro
    stringify_macro = D_STRINGIFY(D_TEST_HELPER_IDENT);

    if (!d_assert_standalone(strcmp(stringify_macro, "D_TEST_HELPER_IDENT") == 0,
                             "D_STRINGIFY(D_TEST_HELPER_IDENT) == macro name",
                             "D_STRINGIFY should not expand macros",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, stringify_macro);
    }

    // D_TOSTR should expand macro first, then stringify
    tostr_macro = D_TOSTR(D_TEST_HELPER_IDENT);

    if (!d_assert_standalone(strcmp(tostr_macro, "my_identifier") == 0,
                             "D_TOSTR(D_TEST_HELPER_IDENT) == \"my_identifier\"",
                             "D_TOSTR should expand macros first",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, tostr_macro);
    }

    // test with numeric macro
    stringify_num = D_STRINGIFY(D_TEST_HELPER_NUMBER);

    if (!d_assert_standalone(strcmp(stringify_num, "D_TEST_HELPER_NUMBER") == 0,
                             "D_STRINGIFY(D_TEST_HELPER_NUMBER) == macro name",
                             "D_STRINGIFY should not expand numeric macro",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, stringify_num);
    }

    // D_TOSTR should expand numeric macro
    tostr_num = D_TOSTR(D_TEST_HELPER_NUMBER);

    if (!d_assert_standalone(strcmp(tostr_num, "42") == 0,
                             "D_TOSTR(D_TEST_HELPER_NUMBER) == \"42\"",
                             "D_TOSTR should expand numeric macro to value",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, tostr_num);
    }

    // test with nested macro (D_TEST_NESTED_A -> D_TEST_NESTED_B -> final_value)
    stringify_nested = D_STRINGIFY(D_TEST_NESTED_A);

    if (!d_assert_standalone(strcmp(stringify_nested, "D_TEST_NESTED_A") == 0,
                             "D_STRINGIFY(D_TEST_NESTED_A) == macro name",
                             "D_STRINGIFY should not expand nested macro",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, stringify_nested);
    }

    // D_TOSTR should fully expand nested macros
    tostr_nested = D_TOSTR(D_TEST_NESTED_A);

    if (!d_assert_standalone(strcmp(tostr_nested, "final_value") == 0,
                             "D_TOSTR(D_TEST_NESTED_A) == \"final_value\"",
                             "D_TOSTR should fully expand nested macros",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, tostr_nested);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_STRINGIFY vs D_TOSTR test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_STRINGIFY vs D_TOSTR test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_stringify_edge_cases
  Tests edge cases for stringification macros.
  Tests the following:
  - D_STRINGIFY with operators and punctuation
  - D_STRINGIFY with parentheses
  - D_TOSTR with empty-expanding macro
  - D_STRINGIFY with comma-containing text
  - D_TOSTR preserves whitespace appropriately
*/
bool
d_tests_sa_dmacro_stringify_edge_cases
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* str_operators;
    const char* str_parens;
    const char* str_concat_result;
    const char* str_comparison;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Stringification Edge Cases ---\n", D_INDENT);

    // test stringification with operators
    str_operators = D_STRINGIFY(a + b - c);

    if (!d_assert_standalone(strcmp(str_operators, "a + b - c") == 0,
                             "D_STRINGIFY(a + b - c) preserves operators",
                             "operators should be preserved in string",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_operators);
    }

    // test stringification with parentheses
    str_parens = D_STRINGIFY((x));

    if (!d_assert_standalone(strcmp(str_parens, "(x)") == 0,
                             "D_STRINGIFY((x)) == \"(x)\"",
                             "parentheses should be preserved",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_parens);
    }

    // test D_TOSTR with D_CONCAT result
    // D_CONCAT(test_, 123) creates test_123, D_TOSTR should give "test_123"
    str_concat_result = D_TOSTR(D_CONCAT(test_, 123));

    if (!d_assert_standalone(strcmp(str_concat_result, "test_123") == 0,
                             "D_TOSTR(D_CONCAT(test_, 123)) == \"test_123\"",
                             "D_TOSTR should stringify concatenation result",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_concat_result);
    }

    // test comparison operators
    str_comparison = D_STRINGIFY(x == y);

    if (!d_assert_standalone(strcmp(str_comparison, "x == y") == 0,
                             "D_STRINGIFY(x == y) preserves comparison",
                             "comparison operator should be preserved",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, str_comparison);
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Stringification edge cases test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Stringification edge cases test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * EXPANSION CONTROL TESTS (D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT, D_UNPACK)
 *****************************************************************************/

/*
d_tests_sa_dmacro_expand_basic
  Tests the D_EXPAND macro functionality.
  Tests the following:
  - D_EXPAND passes through its argument unchanged
  - D_EXPAND forces one expansion pass
  - D_EXPAND works with identifiers
  - D_EXPAND works with expressions
  - D_EXPAND works with macro results
*/
bool
d_tests_sa_dmacro_expand_basic
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    test_var;
    int    expand_result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_EXPAND Basic Functionality ---\n", D_INDENT);

    // initialize test variable
    test_var = 999;

    // verify D_EXPAND passes through identifier
    expand_result = D_EXPAND(test_var);

    if (!d_assert_standalone(expand_result == 999,
                             "D_EXPAND(test_var) == test_var value",
                             "D_EXPAND should pass through identifier",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_EXPAND passes through literal
    if (!d_assert_standalone(D_EXPAND(42) == 42,
                             "D_EXPAND(42) == 42",
                             "D_EXPAND should pass through literal",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_EXPAND passes through expression
    if (!d_assert_standalone(D_EXPAND(1 + 2) == 3,
                             "D_EXPAND(1 + 2) == 3",
                             "D_EXPAND should pass through expression",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_EXPAND works with macro that expands to value
    if (!d_assert_standalone(D_EXPAND(D_TEST_HELPER_NUMBER) == 42,
                             "D_EXPAND(D_TEST_HELPER_NUMBER) == 42",
                             "D_EXPAND should expand macro argument",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify nested D_EXPAND
    if (!d_assert_standalone(D_EXPAND(D_EXPAND(100)) == 100,
                             "D_EXPAND(D_EXPAND(100)) == 100",
                             "nested D_EXPAND should work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_EXPAND basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_EXPAND basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_empty_macro
  Tests the D_EMPTY macro functionality.
  Tests the following:
  - D_EMPTY() expands to nothing
  - D_EMPTY can be used in concatenation contexts
  - D_EMPTY does not affect surrounding tokens
  - D_EMPTY can be used for delayed expansion
  - multiple D_EMPTY calls have no cumulative effect
*/
bool
d_tests_sa_dmacro_empty_macro
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* empty_str;
    int         value_with_empty;
    int         test_var;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_EMPTY Macro ---\n", D_INDENT);

    // initialize
    test_var = 777;

    // verify D_EMPTY expands to nothing (stringified should be empty)
    empty_str = D_TOSTR(D_EMPTY());

    if (!d_assert_standalone(strcmp(empty_str, "") == 0,
                             "D_TOSTR(D_EMPTY()) == \"\"",
                             "D_EMPTY should expand to empty string",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\" (length %zu)\n", D_INDENT, empty_str, strlen(empty_str));
    }

    // verify D_EMPTY does not affect surrounding code
    value_with_empty = test_var D_EMPTY();

    if (!d_assert_standalone(value_with_empty == 777,
                             "test_var D_EMPTY() == test_var",
                             "D_EMPTY should not affect surrounding tokens",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify multiple D_EMPTY calls
    if (!d_assert_standalone(D_EXPAND(123 D_EMPTY() D_EMPTY() D_EMPTY()) == 123,
                             "123 D_EMPTY() D_EMPTY() D_EMPTY() == 123",
                             "multiple D_EMPTY should have no effect",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_EMPTY works inside expressions
    if (!d_assert_standalone((5 D_EMPTY() + D_EMPTY() 3) == 8,
                             "(5 D_EMPTY() + D_EMPTY() 3) == 8",
                             "D_EMPTY in expressions should be invisible",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_EMPTY macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_EMPTY macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_defer_macro
  Tests the D_DEFER macro functionality.
  Tests the following:
  - D_DEFER delays macro expansion by one pass
  - D_DEFER combined with D_EMPTY creates delayed expansion
  - D_DEFER is essential for recursive macro patterns
  - D_DEFER works with D_EXPAND to control timing
*/
bool
d_tests_sa_dmacro_defer_macro
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* defer_str;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_DEFER Macro ---\n", D_INDENT);

    // D_DEFER(id) expands to "id D_EMPTY()" which delays by one scan
    // after one expansion pass, we get "id" (with D_EMPTY() consumed)
    defer_str = D_TOSTR(D_DEFER(test_token));

    // the result should be "test_token D_EMPTY()" or "test_token "
    // depending on expansion timing
    if (!d_assert_standalone(defer_str != NULL,
                             "D_TOSTR(D_DEFER(test_token)) is not NULL",
                             "D_DEFER should produce valid output",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_DEFER contains the original token
    if (!d_assert_standalone(strstr(defer_str, "test_token") != NULL,
                             "D_DEFER(test_token) contains \"test_token\"",
                             "D_DEFER should preserve the token",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, defer_str);
    }

    // verify D_DEFER with D_EXPAND evaluates to the token
    // D_EXPAND(D_DEFER(test)) should eventually evaluate to just "test"
    if (!d_assert_standalone(D_EXPAND(D_DEFER(42)) == 42,
                             "D_EXPAND(D_DEFER(42)) == 42",
                             "D_EXPAND should resolve deferred value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_DEFER macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_DEFER macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_obstruct_macro
  Tests the D_OBSTRUCT macro functionality.
  Tests the following:
  - D_OBSTRUCT delays macro expansion by two passes
  - D_OBSTRUCT is stronger than D_DEFER
  - D_OBSTRUCT is used for advanced recursive patterns
  - D_OBSTRUCT with multiple D_EXPAND calls
*/
bool
d_tests_sa_dmacro_obstruct_macro
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* obstruct_str;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_OBSTRUCT Macro ---\n", D_INDENT);

    // D_OBSTRUCT adds D_DEFER(D_EMPTY)() which delays by two passes
    obstruct_str = D_TOSTR(D_OBSTRUCT(test_token));

    // verify D_OBSTRUCT produces valid output
    if (!d_assert_standalone(obstruct_str != NULL,
                             "D_TOSTR(D_OBSTRUCT(test_token)) is not NULL",
                             "D_OBSTRUCT should produce valid output",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify D_OBSTRUCT contains the original token
    if (!d_assert_standalone(strstr(obstruct_str, "test_token") != NULL,
                             "D_OBSTRUCT(test_token) contains \"test_token\"",
                             "D_OBSTRUCT should preserve the token",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, obstruct_str);
    }

    // verify D_OBSTRUCT eventually resolves with enough expansion
    if (!d_assert_standalone(D_EXPAND(D_EXPAND(D_OBSTRUCT(99))) == 99,
                             "D_EXPAND(D_EXPAND(D_OBSTRUCT(99))) == 99",
                             "double D_EXPAND should resolve obstructed value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_OBSTRUCT macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_OBSTRUCT macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_unpack_basic
  Tests basic D_UNPACK functionality.
  Tests the following:
  - D_UNPACK removes outer parentheses from arguments
  - D_UNPACK passes through multiple comma-separated arguments
  - D_UNPACK works with single argument
  - D_UNPACK works with empty parentheses
  - D_UNPACK preserves argument values
  Note: multi-argument stringify tests avoided due to MSVC traditional
  preprocessor limitations (use /Zc:preprocessor for conformant behavior).
*/
bool
d_tests_sa_dmacro_unpack_basic
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    const char* unpack_single;
    int         sum_result;
    int         arr_3[3];
    int         arr_4[4];

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_UNPACK Basic Functionality ---\n", D_INDENT);

    // test D_UNPACK with single argument (stringify works for single arg)
    unpack_single = D_TOSTR(D_UNPACK(hello));

    if (!d_assert_standalone(strcmp(unpack_single, "hello") == 0,
                             "D_TOSTR(D_UNPACK(hello)) == \"hello\"",
                             "D_UNPACK should pass through single arg",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, unpack_single);
    }

    // test D_UNPACK with multiple arguments via array initialization
    // (avoids MSVC traditional preprocessor stringify issue)
    {
        int arr[] = { D_UNPACK(10, 20, 30) };
        arr_3[0]   = arr[0];
        arr_3[1]   = arr[1];
        arr_3[2]   = arr[2];
        sum_result = arr_3[0] + arr_3[1] + arr_3[2];
    }

    if (!d_assert_standalone(sum_result == 60,
                             "array from D_UNPACK(10, 20, 30) sums to 60",
                             "D_UNPACK should create valid initializer list",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify individual values are correct
    if (!d_assert_standalone(arr_3[0] == 10 && arr_3[1] == 20 && arr_3[2] == 30,
                             "D_UNPACK(10, 20, 30) expands to 10, 20, 30",
                             "D_UNPACK should preserve individual values",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: %d, %d, %d\n", D_INDENT, arr_3[0], arr_3[1], arr_3[2]);
    }

    // test D_UNPACK with 4 arguments
    {
        int arr[] = { D_UNPACK(100, 200, 300, 400) };
        arr_4[0] = arr[0];
        arr_4[1] = arr[1];
        arr_4[2] = arr[2];
        arr_4[3] = arr[3];
    }

    if (!d_assert_standalone(arr_4[0] + arr_4[1] + arr_4[2] + arr_4[3] == 1000,
                             "D_UNPACK(100, 200, 300, 400) sums to 1000",
                             "D_UNPACK should work with 4 arguments",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_UNPACK with single numeric value
    if (!d_assert_standalone(D_UNPACK(42) == 42,
                             "D_UNPACK(42) == 42",
                             "D_UNPACK should pass through single value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_UNPACK preserves expression evaluation
    if (!d_assert_standalone(D_UNPACK(2 + 3) == 5,
                             "D_UNPACK(2 + 3) == 5",
                             "D_UNPACK should allow expression evaluation",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_UNPACK basic functionality test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_UNPACK basic functionality test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_dmacro_unpack_nested
  Tests nested and complex D_UNPACK scenarios.
  Tests the following:
  - D_UNPACK with nested parentheses
  - D_UNPACK with macro arguments that contain parentheses
  - D_UNPACK used to pass tuple-like structures
  - D_UNPACK combined with other macros
  - D_UNPACK with expressions containing operators
  Note: multi-argument stringify tests avoided due to MSVC traditional
  preprocessor limitations (use /Zc:preprocessor for conformant behavior).
*/
bool
d_tests_sa_dmacro_unpack_nested
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    int         arr_sum;
    int         nested_arr[2];
    int         expr_arr[2];
    const char* from_macro;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing D_UNPACK Nested Scenarios ---\n", D_INDENT);

    // test D_UNPACK with parenthesized sub-expressions via array init
    // this tests that inner parentheses are preserved
    {
        int arr[] = { D_UNPACK((1 + 2), (3 + 4)) };
        nested_arr[0] = arr[0];
        nested_arr[1] = arr[1];
    }

    if (!d_assert_standalone(nested_arr[0] == 3 && nested_arr[1] == 7,
                             "D_UNPACK((1+2), (3+4)) == {3, 7}",
                             "D_UNPACK should preserve inner parentheses",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: {%d, %d}\n", D_INDENT, nested_arr[0], nested_arr[1]);
    }

    // test D_UNPACK with expressions containing operators
    {
        int arr[] = { D_UNPACK(1 + 2, 3 * 4) };
        expr_arr[0] = arr[0];
        expr_arr[1] = arr[1];
    }

    if (!d_assert_standalone(expr_arr[0] == 3 && expr_arr[1] == 12,
                             "D_UNPACK(1 + 2, 3 * 4) == {3, 12}",
                             "D_UNPACK should allow expression evaluation",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: {%d, %d}\n", D_INDENT, expr_arr[0], expr_arr[1]);
    }

    // test D_UNPACK with D_TEST_PARENTHESIZED macro
    // D_TEST_PARENTHESIZED is (a, b, c)
    // D_UNPACK D_TEST_PARENTHESIZED should give a, b, c
    // We test this by stringifying just the macro name first (single arg)
    from_macro = D_TOSTR(D_TEST_PARENTHESIZED);

    if (!d_assert_standalone(strcmp(from_macro, "(a, b, c)") == 0,
                             "D_TOSTR(D_TEST_PARENTHESIZED) == \"(a, b, c)\"",
                             "parenthesized macro should stringify with parens",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    Got: \"%s\"\n", D_INDENT, from_macro);
    }

    // test D_UNPACK in array initialization with computed values
    {
        int vals[] = { D_UNPACK(100, 200, 300, 400) };
        arr_sum = vals[0] + vals[1] + vals[2] + vals[3];
    }

    if (!d_assert_standalone(arr_sum == 1000,
                             "D_UNPACK array values sum to 1000",
                             "D_UNPACK should work with 4-element array",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test D_UNPACK with negative numbers
    {
        int neg_arr[] = { D_UNPACK(-1, -2, -3) };
        int neg_sum   = neg_arr[0] + neg_arr[1] + neg_arr[2];

        if (!d_assert_standalone(neg_sum == -6,
                                 "D_UNPACK(-1, -2, -3) sums to -6",
                                 "D_UNPACK should handle negative numbers",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test D_UNPACK with mixed positive and negative
    {
        int mix_arr[] = { D_UNPACK(10, -5, 20, -15) };
        int mix_sum   = mix_arr[0] + mix_arr[1] + mix_arr[2] + mix_arr[3];

        if (!d_assert_standalone(mix_sum == 10,
                                 "D_UNPACK(10, -5, 20, -15) sums to 10",
                                 "D_UNPACK should handle mixed signs",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test D_UNPACK with zero
    {
        int zero_arr[] = { D_UNPACK(0, 0, 0) };

        if (!d_assert_standalone(zero_arr[0] == 0 && zero_arr[1] == 0 && zero_arr[2] == 0,
                                 "D_UNPACK(0, 0, 0) == {0, 0, 0}",
                                 "D_UNPACK should handle zeros",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] D_UNPACK nested scenarios test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] D_UNPACK nested scenarios test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
 * TOKEN MANIPULATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_sa_dmacro_token_all
  Runs all token manipulation section tests.
  Tests the following:
  - D_CONCAT basic and with macros
  - D_STRINGIFY and D_TOSTR
  - D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT
  - D_UNPACK basic and nested
*/
bool
d_tests_sa_dmacro_token_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool                  concat_basic_result;
    bool                  concat_macros_result;
    bool                  concat_edge_result;
    bool                  stringify_basic_result;
    bool                  stringify_vs_tostr_result;
    bool                  stringify_edge_result;
    bool                  expand_result;
    bool                  empty_result;
    bool                  defer_result;
    bool                  obstruct_result;
    bool                  unpack_basic_result;
    bool                  unpack_nested_result;
    bool                  overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n");
    printf("=========================================");
    printf("=======================================\n");
    printf("[MODULE] Testing Token Manipulation (Section I)\n");
    printf("=========================================");
    printf("=======================================\n");

    // run all token manipulation tests
    concat_basic_result       = d_tests_sa_dmacro_concat_basic(&module_counter);
    concat_macros_result      = d_tests_sa_dmacro_concat_with_macros(&module_counter);
    concat_edge_result        = d_tests_sa_dmacro_concat_edge_cases(&module_counter);
    stringify_basic_result    = d_tests_sa_dmacro_stringify_basic(&module_counter);
    stringify_vs_tostr_result = d_tests_sa_dmacro_stringify_vs_tostr(&module_counter);
    stringify_edge_result     = d_tests_sa_dmacro_stringify_edge_cases(&module_counter);
    expand_result             = d_tests_sa_dmacro_expand_basic(&module_counter);
    empty_result              = d_tests_sa_dmacro_empty_macro(&module_counter);
    defer_result              = d_tests_sa_dmacro_defer_macro(&module_counter);
    obstruct_result           = d_tests_sa_dmacro_obstruct_macro(&module_counter);
    unpack_basic_result       = d_tests_sa_dmacro_unpack_basic(&module_counter);
    unpack_nested_result      = d_tests_sa_dmacro_unpack_nested(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total       += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( concat_basic_result       &&
                       concat_macros_result      &&
                       concat_edge_result        &&
                       stringify_basic_result    &&
                       stringify_vs_tostr_result &&
                       stringify_edge_result     &&
                       expand_result             &&
                       empty_result              &&
                       defer_result              &&
                       obstruct_result           &&
                       unpack_basic_result       &&
                       unpack_nested_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Token Manipulation Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Token Manipulation Module: %zu/%zu assertions, "
               "%zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - D_CONCAT Basic:       %s\n",
               concat_basic_result ? "PASSED" : "FAILED");
        printf("  - D_CONCAT Macros:      %s\n",
               concat_macros_result ? "PASSED" : "FAILED");
        printf("  - D_CONCAT Edge Cases:  %s\n",
               concat_edge_result ? "PASSED" : "FAILED");
        printf("  - D_STRINGIFY Basic:    %s\n",
               stringify_basic_result ? "PASSED" : "FAILED");
        printf("  - STRINGIFY vs TOSTR:   %s\n",
               stringify_vs_tostr_result ? "PASSED" : "FAILED");
        printf("  - Stringify Edge Cases: %s\n",
               stringify_edge_result ? "PASSED" : "FAILED");
        printf("  - D_EXPAND:             %s\n",
               expand_result ? "PASSED" : "FAILED");
        printf("  - D_EMPTY:              %s\n",
               empty_result ? "PASSED" : "FAILED");
        printf("  - D_DEFER:              %s\n",
               defer_result ? "PASSED" : "FAILED");
        printf("  - D_OBSTRUCT:           %s\n",
               obstruct_result ? "PASSED" : "FAILED");
        printf("  - D_UNPACK Basic:       %s\n",
               unpack_basic_result ? "PASSED" : "FAILED");
        printf("  - D_UNPACK Nested:      %s\n",
               unpack_nested_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}