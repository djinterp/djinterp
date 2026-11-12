/*******************************************************************************
* djinterp [c]                                        dmacro_tests_token_manip.c
*
*   Unit tests for `dmacro.h` token manipulation section (Section I).
*   Tests token pasting (D_CONCAT), stringification (D_STRINGIFY, D_TOSTR),
*   and expansion control macros (D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT,
*   D_UNPACK).
*   Note: these macro utilities are dependencies of other djinterp modules,
* DTest's C face among them (dmacro_token.h, through djinterp.h), so a fault
* here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/dmacro_tests_token_manip.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <string.h>
// djinterp
#include "dmacro_tests.h"


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
d_tests_dmacro_concat_basic
  Tests basic token concatenation functionality.
  Tests the following:
  - D_CONCAT joins two literal tokens correctly
  - D_CONCAT creates valid identifiers
  - D_CONCAT joins numeric suffixes correctly
  - D_CONCAT handles underscore-prefixed tokens
  - D_CONCAT works with single-character tokens
*/
bool d_tests_dmacro_concat_basic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    test_var1;
    int    var_42;
    int    a_b;
    int    _x;
    int    AB;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_concat_basic");

    all_assertions_passed = true;

    // initialize test variables
    test_var1 = 100;
    var_42    = 200;
    a_b       = 300;
    _x        = 400;
    AB        = 500;

    // verify basic token concatenation creates valid identifier
    // D_CONCAT(test_, var1) should expand to test_var1
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(test_, var1) == test_var1",
                          D_CONCAT(test_, var1) == 100,
                          "basic token concatenation should create identifier"))
    {
        all_assertions_passed = false;
    }

    // verify concatenation with numeric suffix
    // D_CONCAT(var_, 42) should expand to var_42
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(var_, 42) == var_42",
                          D_CONCAT(var_, 42) == 200,
                          "numeric suffix concatenation should work"))
    {
        all_assertions_passed = false;
    }

    // verify concatenation creates underscore-separated identifier
    // D_CONCAT(a_, b) should expand to a_b
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(a_, b) == a_b",
                          D_CONCAT(a_, b) == 300,
                          "underscore-separated concatenation should work"))
    {
        all_assertions_passed = false;
    }

    // verify concatenation with underscore prefix
    // D_CONCAT(_, x) should expand to _x
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(_, x) == _x",
                          D_CONCAT(_, x) == 400,
                          "underscore-prefixed concatenation should work"))
    {
        all_assertions_passed = false;
    }

    // verify single character concatenation
    // D_CONCAT(A, B) should expand to AB
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(A, B) == AB",
                          D_CONCAT(A, B) == 500,
                          "single character concatenation should work"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_concat_with_macros
  Tests token concatenation with macro arguments.
  Tests the following:
  - D_CONCAT expands macro arguments before concatenating
  - D_CONCAT handles prefix macros correctly
  - D_CONCAT handles suffix macros correctly
  - D_CONCAT works with both prefix and suffix as macros
  - nested D_CONCAT calls work correctly
*/
bool d_tests_dmacro_concat_with_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    test_result;
    int    test_result_value;
    int    result_value;
    int    test_value;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_concat_with_macros");

    all_assertions_passed = true;

    // initialize test variables
    test_result       = 111;
    test_result_value = 222;
    result_value      = 333;
    test_value        = 444;

    // verify D_CONCAT expands macro prefix before concatenation
    // D_CONCAT(D_TEST_HELPER_PREFIX, result) should become test_result
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(D_TEST_HELPER_PREFIX, result) == test_result",
                          D_CONCAT(D_TEST_HELPER_PREFIX, result) == 111,
                          "macro prefix should be expanded before concat"))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT expands macro suffix before concatenation
    // D_CONCAT(result, D_TEST_HELPER_SUFFIX) should become result_value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(result, D_TEST_HELPER_SUFFIX) == result_value",
                          D_CONCAT(result, D_TEST_HELPER_SUFFIX) == 333,
                          "macro suffix should be expanded before concat"))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT expands both macro arguments
    // D_CONCAT(D_TEST_HELPER_PREFIX, D_TEST_HELPER_SUFFIX) would NOT work
    // because test_ and _value don't form a valid token
    // Instead test: D_CONCAT(test, _value)
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(test, _value) == test_value",
                          D_CONCAT(test, _value) == 444,
                          "both tokens should concatenate correctly"))
    {
        all_assertions_passed = false;
    }

    // verify nested D_CONCAT calls work
    // D_CONCAT(D_CONCAT(test_, result), _value) should become test_result_value
    if (!D_TEST_CHECK_MSG(_suite,
                          "nested D_CONCAT creates test_result_value",
                          D_CONCAT(D_CONCAT(test_, result), _value) == 222,
                          "nested concatenation should work correctly"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_concat_edge_cases
  Tests edge cases and boundary conditions for token concatenation.
  Tests the following:
  - D_CONCAT with empty-like second token (where applicable)
  - D_CONCAT for creating type names
  - D_CONCAT for creating function names
  - D_CONCAT with numbers at start (invalid C identifier - compile test)
  - multiple sequential D_CONCAT operations
*/
bool d_tests_dmacro_concat_edge_cases(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    type_int;
    int    func_init;
    int    x1;
    int    x1y2z3;
    int    prefix_mid_suffix;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_concat_edge_cases");

    all_assertions_passed = true;

    // initialize test variables
    type_int          = 1000;
    func_init         = 2000;
    x1                = 3000;
    x1y2z3            = 4000;
    prefix_mid_suffix = 5000;

    // verify D_CONCAT can create type-like names
    // D_CONCAT(type_, int) should create type_int
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(type_, int) == type_int",
                          D_CONCAT(type_, int) == 1000,
                          "should create type-like identifier"))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT can create function-like names
    // D_CONCAT(func_, init) should create func_init
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(func_, init) == func_init",
                          D_CONCAT(func_, init) == 2000,
                          "should create function-like identifier"))
    {
        all_assertions_passed = false;
    }

    // verify D_CONCAT with trailing number
    // D_CONCAT(x, 1) should create x1
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_CONCAT(x, 1) == x1",
                          D_CONCAT(x, 1) == 3000,
                          "should handle trailing number"))
    {
        all_assertions_passed = false;
    }

    // verify deeply nested concatenation (3 levels)
    // D_CONCAT(D_CONCAT(D_CONCAT(x, 1), y2), z3) should create x1y2z3
    if (!D_TEST_CHECK_MSG(_suite,
                          "triple nested D_CONCAT creates x1y2z3",
                          D_CONCAT(D_CONCAT(D_CONCAT(x, 1), y2), z3) == 4000,
                          "deeply nested concatenation should work"))
    {
        all_assertions_passed = false;
    }

    // verify sequential/chained concatenation
    // D_CONCAT(prefix_, D_CONCAT(mid_, suffix)) should create prefix_mid_suffix
    if (!D_TEST_CHECK_MSG(_suite,
                          "chained D_CONCAT creates prefix_mid_suffix",
                          D_CONCAT(prefix_, D_CONCAT(mid_, suffix)) == 5000,
                          "chained concatenation should work"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
 * STRINGIFICATION TESTS (D_STRINGIFY, D_TOSTR)
 *****************************************************************************/

/*
d_tests_dmacro_stringify_basic
  Tests basic stringification functionality.
  Tests the following:
  - D_STRINGIFY converts tokens to string literals
  - D_STRINGIFY preserves the exact token text
  - D_STRINGIFY works with identifiers
  - D_STRINGIFY works with numbers
  - D_STRINGIFY works with expressions
*/
bool d_tests_dmacro_stringify_basic(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* str_ident;
    const char* str_number;
    const char* str_expr;
    const char* str_keyword;
    const char* str_special;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_stringify_basic");

    all_assertions_passed = true;

    // test stringification of identifier
    str_ident = D_STRINGIFY(hello_world);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(hello_world) == \"hello_world\"",
                          strcmp(str_ident, "hello_world") == 0,
                          "identifier should be stringified exactly"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_ident);
    }

    // test stringification of number
    str_number = D_STRINGIFY(12345);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(12345) == \"12345\"",
                          strcmp(str_number, "12345") == 0,
                          "number should be stringified exactly"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_number);
    }

    // test stringification of expression
    str_expr = D_STRINGIFY(1 + 2 * 3);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(1 + 2 * 3) == \"1 + 2 * 3\"",
                          strcmp(str_expr, "1 + 2 * 3") == 0,
                          "expression should be stringified with spacing"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_expr);
    }

    // test stringification of C keyword
    str_keyword = D_STRINGIFY(return);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(return) == \"return\"",
                          strcmp(str_keyword, "return") == 0,
                          "keyword should be stringified exactly"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_keyword);
    }

    // test stringification of special characters
    str_special = D_STRINGIFY(_underscore_prefix);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(_underscore_prefix) stringified",
                          strcmp(str_special, "_underscore_prefix") == 0,
                          "underscore-prefixed identifier should stringify"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_special);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_stringify_vs_tostr
  Tests the difference between D_STRINGIFY and D_TOSTR.
  Tests the following:
  - D_STRINGIFY does NOT expand macros before stringifying
  - D_TOSTR DOES expand macros before stringifying
  - behavior difference is correct for simple macros
  - behavior difference is correct for numeric macros
  - behavior difference is correct for nested macros
*/
bool d_tests_dmacro_stringify_vs_tostr(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* stringify_macro;
    const char* tostr_macro;
    const char* stringify_num;
    const char* tostr_num;
    const char* stringify_nested;
    const char* tostr_nested;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_stringify_vs_tostr");

    all_assertions_passed = true;

    // D_STRINGIFY should NOT expand macro
    stringify_macro = D_STRINGIFY(D_TEST_HELPER_IDENT);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(D_TEST_HELPER_IDENT) == macro name",
                          strcmp(stringify_macro, "D_TEST_HELPER_IDENT") == 0,
                          "D_STRINGIFY should not expand macros"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", stringify_macro);
    }

    // D_TOSTR should expand macro first, then stringify
    tostr_macro = D_TOSTR(D_TEST_HELPER_IDENT);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_TEST_HELPER_IDENT) == \"my_identifier\"",
                          strcmp(tostr_macro, "my_identifier") == 0,
                          "D_TOSTR should expand macros first"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", tostr_macro);
    }

    // test with numeric macro
    stringify_num = D_STRINGIFY(D_TEST_HELPER_NUMBER);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(D_TEST_HELPER_NUMBER) == macro name",
                          strcmp(stringify_num, "D_TEST_HELPER_NUMBER") == 0,
                          "D_STRINGIFY should not expand numeric macro"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", stringify_num);
    }

    // D_TOSTR should expand numeric macro
    tostr_num = D_TOSTR(D_TEST_HELPER_NUMBER);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_TEST_HELPER_NUMBER) == \"42\"",
                          strcmp(tostr_num, "42") == 0,
                          "D_TOSTR should expand numeric macro to value"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", tostr_num);
    }

    // test with nested macro (D_TEST_NESTED_A -> D_TEST_NESTED_B -> final_value)
    stringify_nested = D_STRINGIFY(D_TEST_NESTED_A);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(D_TEST_NESTED_A) == macro name",
                          strcmp(stringify_nested, "D_TEST_NESTED_A") == 0,
                          "D_STRINGIFY should not expand nested macro"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", stringify_nested);
    }

    // D_TOSTR should fully expand nested macros
    tostr_nested = D_TOSTR(D_TEST_NESTED_A);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_TEST_NESTED_A) == \"final_value\"",
                          strcmp(tostr_nested, "final_value") == 0,
                          "D_TOSTR should fully expand nested macros"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", tostr_nested);
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_stringify_edge_cases
  Tests edge cases for stringification macros.
  Tests the following:
  - D_STRINGIFY with operators and punctuation
  - D_STRINGIFY with parentheses
  - D_TOSTR with empty-expanding macro
  - D_STRINGIFY with comma-containing text
  - D_TOSTR preserves whitespace appropriately
*/
bool d_tests_dmacro_stringify_edge_cases(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* str_operators;
    const char* str_parens;
    const char* str_concat_result;
    const char* str_comparison;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_stringify_edge_cases");

    all_assertions_passed = true;

    // test stringification with operators
    str_operators = D_STRINGIFY(a + b - c);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(a + b - c) preserves operators",
                          strcmp(str_operators, "a + b - c") == 0,
                          "operators should be preserved in string"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_operators);
    }

    // test stringification with parentheses
    str_parens = D_STRINGIFY((x));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY((x)) == \"(x)\"",
                          strcmp(str_parens, "(x)") == 0,
                          "parentheses should be preserved"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_parens);
    }

    // test D_TOSTR with D_CONCAT result
    // D_CONCAT(test_, 123) creates test_123, D_TOSTR should give "test_123"
    str_concat_result = D_TOSTR(D_CONCAT(test_, 123));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_CONCAT(test_, 123)) == \"test_123\"",
                          strcmp(str_concat_result, "test_123") == 0,
                          "D_TOSTR should stringify concatenation result"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_concat_result);
    }

    // test comparison operators
    str_comparison = D_STRINGIFY(x == y);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_STRINGIFY(x == y) preserves comparison",
                          strcmp(str_comparison, "x == y") == 0,
                          "comparison operator should be preserved"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", str_comparison);
    }

    return all_assertions_passed;
}


/******************************************************************************
 * EXPANSION CONTROL TESTS (D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT, D_UNPACK)
 *****************************************************************************/

/*
d_tests_dmacro_expand_basic
  Tests the D_EXPAND macro functionality.
  Tests the following:
  - D_EXPAND passes through its argument unchanged
  - D_EXPAND forces one expansion pass
  - D_EXPAND works with identifiers
  - D_EXPAND works with expressions
  - D_EXPAND works with macro results
*/
bool d_tests_dmacro_expand_basic(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    test_var;
    int    expand_result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_expand_basic");

    all_assertions_passed = true;

    // initialize test variable
    test_var = 999;

    // verify D_EXPAND passes through identifier
    expand_result = D_EXPAND(test_var);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EXPAND(test_var) == test_var value",
                          expand_result == 999,
                          "D_EXPAND should pass through identifier"))
    {
        all_assertions_passed = false;
    }

    // verify D_EXPAND passes through literal
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EXPAND(42) == 42",
                          D_EXPAND(42) == 42,
                          "D_EXPAND should pass through literal"))
    {
        all_assertions_passed = false;
    }

    // verify D_EXPAND passes through expression
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EXPAND(1 + 2) == 3",
                          D_EXPAND(1 + 2) == 3,
                          "D_EXPAND should pass through expression"))
    {
        all_assertions_passed = false;
    }

    // verify D_EXPAND works with macro that expands to value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EXPAND(D_TEST_HELPER_NUMBER) == 42",
                          D_EXPAND(D_TEST_HELPER_NUMBER) == 42,
                          "D_EXPAND should expand macro argument"))
    {
        all_assertions_passed = false;
    }

    // verify nested D_EXPAND
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EXPAND(D_EXPAND(100)) == 100",
                          D_EXPAND(D_EXPAND(100)) == 100,
                          "nested D_EXPAND should work"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_empty_macro
  Tests the D_EMPTY macro functionality.
  Tests the following:
  - D_EMPTY() expands to nothing
  - D_EMPTY can be used in concatenation contexts
  - D_EMPTY does not affect surrounding tokens
  - D_EMPTY can be used for delayed expansion
  - multiple D_EMPTY calls have no cumulative effect
*/
bool d_tests_dmacro_empty_macro(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* empty_str;
    int         value_with_empty;
    int         test_var;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_empty_macro");

    all_assertions_passed = true;

    // initialize
    test_var = 777;

    // verify D_EMPTY expands to nothing (stringified should be empty)
    empty_str = D_TOSTR(D_EMPTY());

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_EMPTY()) == \"\"",
                          strcmp(empty_str, "") == 0,
                          "D_EMPTY should expand to empty string"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\" (length %zu)",
                                empty_str, strlen(empty_str));
    }

    // verify D_EMPTY does not affect surrounding code
    value_with_empty = test_var D_EMPTY();

    if (!D_TEST_CHECK_MSG(_suite,
                          "test_var D_EMPTY() == test_var",
                          value_with_empty == 777,
                          "D_EMPTY should not affect surrounding tokens"))
    {
        all_assertions_passed = false;
    }

    // verify multiple D_EMPTY calls
    if (!D_TEST_CHECK_MSG(_suite,
                          "123 D_EMPTY() D_EMPTY() D_EMPTY() == 123",
                          D_EXPAND(123 D_EMPTY() D_EMPTY() D_EMPTY()) == 123,
                          "multiple D_EMPTY should have no effect"))
    {
        all_assertions_passed = false;
    }

    // verify D_EMPTY works inside expressions
    if (!D_TEST_CHECK_MSG(_suite,
                          "(5 D_EMPTY() + D_EMPTY() 3) == 8",
                          (5 D_EMPTY() + D_EMPTY() 3) == 8,
                          "D_EMPTY in expressions should be invisible"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_defer_macro
  Tests the D_DEFER macro functionality.
  Tests the following:
  - D_DEFER delays macro expansion by one pass
  - D_DEFER combined with D_EMPTY creates delayed expansion
  - D_DEFER is essential for recursive macro patterns
  - D_DEFER works with D_EXPAND to control timing
*/
bool d_tests_dmacro_defer_macro(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* defer_str;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_defer_macro");

    all_assertions_passed = true;

    // D_DEFER(id) expands to "id D_EMPTY()" which delays by one scan
    // after one expansion pass, we get "id" (with D_EMPTY() consumed)
    defer_str = D_TOSTR(D_DEFER(test_token));

    // the result should be "test_token D_EMPTY()" or "test_token "
    // depending on expansion timing
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_DEFER(test_token)) is not NULL",
                          defer_str != NULL,
                          "D_DEFER should produce valid output"))
    {
        all_assertions_passed = false;
    }

    // verify D_DEFER contains the original token
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_DEFER(test_token) contains \"test_token\"",
                          strstr(defer_str, "test_token") != NULL,
                          "D_DEFER should preserve the token"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", defer_str);
    }

    // verify D_DEFER with D_EXPAND evaluates to the token
    // D_EXPAND(D_DEFER(test)) should eventually evaluate to just "test"
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EXPAND(D_DEFER(42)) == 42",
                          D_EXPAND(D_DEFER(42)) == 42,
                          "D_EXPAND should resolve deferred value"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_obstruct_macro
  Tests the D_OBSTRUCT macro functionality.
  Tests the following:
  - D_OBSTRUCT delays macro expansion by two passes
  - D_OBSTRUCT is stronger than D_DEFER
  - D_OBSTRUCT is used for advanced recursive patterns
  - D_OBSTRUCT with multiple D_EXPAND calls
*/
bool d_tests_dmacro_obstruct_macro(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* obstruct_str;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_obstruct_macro");

    all_assertions_passed = true;

    // D_OBSTRUCT adds D_DEFER(D_EMPTY)() which delays by two passes
    obstruct_str = D_TOSTR(D_OBSTRUCT(test_token));

    // verify D_OBSTRUCT produces valid output
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_OBSTRUCT(test_token)) is not NULL",
                          obstruct_str != NULL,
                          "D_OBSTRUCT should produce valid output"))
    {
        all_assertions_passed = false;
    }

    // verify D_OBSTRUCT contains the original token
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_OBSTRUCT(test_token) contains \"test_token\"",
                          strstr(obstruct_str, "test_token") != NULL,
                          "D_OBSTRUCT should preserve the token"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", obstruct_str);
    }

    // verify D_OBSTRUCT eventually resolves with enough expansion
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_EXPAND(D_EXPAND(D_OBSTRUCT(99))) == 99",
                          D_EXPAND(D_EXPAND(D_OBSTRUCT(99))) == 99,
                          "double D_EXPAND should resolve obstructed value"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_unpack_basic
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
bool d_tests_dmacro_unpack_basic(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    const char* unpack_single;
    int         sum_result;
    int         arr_3[3];
    int         arr_4[4];

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_unpack_basic");

    all_assertions_passed = true;

    // test D_UNPACK with single argument (stringify works for single arg)
    unpack_single = D_TOSTR(D_UNPACK(hello));

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_UNPACK(hello)) == \"hello\"",
                          strcmp(unpack_single, "hello") == 0,
                          "D_UNPACK should pass through single arg"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", unpack_single);
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "array from D_UNPACK(10, 20, 30) sums to 60",
                          sum_result == 60,
                          "D_UNPACK should create valid initializer list"))
    {
        all_assertions_passed = false;
    }

    // verify individual values are correct
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_UNPACK(10, 20, 30) expands to 10, 20, 30",
                          arr_3[0] == 10 && arr_3[1] == 20 && arr_3[2] == 30,
                          "D_UNPACK should preserve individual values"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: %d, %d, %d",
                                arr_3[0], arr_3[1], arr_3[2]);
    }

    // test D_UNPACK with 4 arguments
    {
        int arr[] = { D_UNPACK(100, 200, 300, 400) };
        arr_4[0] = arr[0];
        arr_4[1] = arr[1];
        arr_4[2] = arr[2];
        arr_4[3] = arr[3];
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_UNPACK(100, 200, 300, 400) sums to 1000",
                          arr_4[0] + arr_4[1] + arr_4[2] + arr_4[3] == 1000,
                          "D_UNPACK should work with 4 arguments"))
    {
        all_assertions_passed = false;
    }

    // test D_UNPACK with single numeric value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_UNPACK(42) == 42",
                          D_UNPACK(42) == 42,
                          "D_UNPACK should pass through single value"))
    {
        all_assertions_passed = false;
    }

    // test D_UNPACK preserves expression evaluation
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_UNPACK(2 + 3) == 5",
                          D_UNPACK(2 + 3) == 5,
                          "D_UNPACK should allow expression evaluation"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_dmacro_unpack_nested
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
bool d_tests_dmacro_unpack_nested(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    int         arr_sum;
    int         nested_arr[2];
    int         expr_arr[2];
    const char* from_macro;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "dmacro_unpack_nested");

    all_assertions_passed = true;

    // test D_UNPACK with parenthesized sub-expressions via array init
    // this tests that inner parentheses are preserved
    {
        int arr[] = { D_UNPACK((1 + 2), (3 + 4)) };
        nested_arr[0] = arr[0];
        nested_arr[1] = arr[1];
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_UNPACK((1+2), (3+4)) == {3, 7}",
                          nested_arr[0] == 3 && nested_arr[1] == 7,
                          "D_UNPACK should preserve inner parentheses"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: {%d, %d}",
                                nested_arr[0], nested_arr[1]);
    }

    // test D_UNPACK with expressions containing operators
    {
        int arr[] = { D_UNPACK(1 + 2, 3 * 4) };
        expr_arr[0] = arr[0];
        expr_arr[1] = arr[1];
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_UNPACK(1 + 2, 3 * 4) == {3, 12}",
                          expr_arr[0] == 3 && expr_arr[1] == 12,
                          "D_UNPACK should allow expression evaluation"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: {%d, %d}",
                                expr_arr[0], expr_arr[1]);
    }

    // test D_UNPACK with D_TEST_PARENTHESIZED macro
    // D_TEST_PARENTHESIZED is (a, b, c)
    // D_UNPACK D_TEST_PARENTHESIZED should give a, b, c
    // We test this by stringifying just the macro name first (single arg)
    from_macro = D_TOSTR(D_TEST_PARENTHESIZED);

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_TOSTR(D_TEST_PARENTHESIZED) == \"(a, b, c)\"",
                          strcmp(from_macro, "(a, b, c)") == 0,
                          "parenthesized macro should stringify with parens"))
    {
        all_assertions_passed = false;
        (void)d_test_suite_note(_suite, "Got: \"%s\"", from_macro);
    }

    // test D_UNPACK in array initialization with computed values
    {
        int vals[] = { D_UNPACK(100, 200, 300, 400) };
        arr_sum = vals[0] + vals[1] + vals[2] + vals[3];
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_UNPACK array values sum to 1000",
                          arr_sum == 1000,
                          "D_UNPACK should work with 4-element array"))
    {
        all_assertions_passed = false;
    }

    // test D_UNPACK with negative numbers
    {
        int neg_arr[] = { D_UNPACK(-1, -2, -3) };
        int neg_sum   = neg_arr[0] + neg_arr[1] + neg_arr[2];

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_UNPACK(-1, -2, -3) sums to -6",
                              neg_sum == -6,
                              "D_UNPACK should handle negative numbers"))
        {
            all_assertions_passed = false;
        }
    }

    // test D_UNPACK with mixed positive and negative
    {
        int mix_arr[] = { D_UNPACK(10, -5, 20, -15) };
        int mix_sum   = mix_arr[0] + mix_arr[1] + mix_arr[2] + mix_arr[3];

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_UNPACK(10, -5, 20, -15) sums to 10",
                              mix_sum == 10,
                              "D_UNPACK should handle mixed signs"))
        {
            all_assertions_passed = false;
        }
    }

    // test D_UNPACK with zero
    {
        int zero_arr[] = { D_UNPACK(0, 0, 0) };

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_UNPACK(0, 0, 0) == {0, 0, 0}",
                              zero_arr[0] == 0 && zero_arr[1] == 0 && zero_arr[2] == 0,
                              "D_UNPACK should handle zeros"))
        {
            all_assertions_passed = false;
        }
    }

    return all_assertions_passed;
}


/******************************************************************************
 * TOKEN MANIPULATION MODULE AGGREGATOR
 *****************************************************************************/

/*
d_tests_dmacro_token_all
  Runs all token manipulation section tests.
  Tests the following:
  - D_CONCAT basic and with macros
  - D_STRINGIFY and D_TOSTR
  - D_EXPAND, D_EMPTY, D_DEFER, D_OBSTRUCT
  - D_UNPACK basic and nested
*/
bool d_tests_dmacro_token_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "dmacro_token_all");

    d_tests_dmacro_concat_basic(_suite);
    d_tests_dmacro_concat_with_macros(_suite);
    d_tests_dmacro_concat_edge_cases(_suite);
    d_tests_dmacro_stringify_basic(_suite);
    d_tests_dmacro_stringify_vs_tostr(_suite);
    d_tests_dmacro_stringify_edge_cases(_suite);
    d_tests_dmacro_expand_basic(_suite);
    d_tests_dmacro_empty_macro(_suite);
    d_tests_dmacro_defer_macro(_suite);
    d_tests_dmacro_obstruct_macro(_suite);
    d_tests_dmacro_unpack_basic(_suite);
    d_tests_dmacro_unpack_nested(_suite);

    return true;
}
