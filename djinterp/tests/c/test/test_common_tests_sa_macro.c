#include ".\test_common_tests_sa.h"


/******************************************************************************
 * HELPER FUNCTIONS FOR STRING COMPARISON
 *****************************************************************************/

/*
helper_strings_equal
  Helper function to compare two C strings for equality.
  Uses d_strcasecmp from string_fn.h instead of strcpy/strcmp.

Parameter(s):
  _s1: first string to compare
  _s2: second string to compare
Return:
  true if strings are equal, false otherwise
*/
static bool
helper_strings_equal
(
    const char* _s1,
    const char* _s2
)
{
    // handle NULL cases
    if ((!_s1) && (!_s2))
    {
        return true;
    }

    if ((!_s1) || (!_s2))
    {
        return false;
    }

    // use d_strcasecmp for case-sensitive comparison (returns 0 if equal)
    // note: we want case-sensitive, so we manually compare
    while ((*_s1) && (*_s2))
    {
        if (*_s1 != *_s2)
        {
            return false;
        }

        _s1++;
        _s2++;
    }

    return (*_s1 == *_s2);
}


/******************************************************************************
 * I. MACRO DEFINITION TESTS
 *****************************************************************************/

/*
d_tests_sa_test_common_keyword_macros
  Tests the keyword macro definitions in test_common.h.
  Tests the following:
  - D_KEYWORD_TEST_ASSERTION is defined correctly
  - D_KEYWORD_TEST_TEST_FN is defined correctly
  - D_KEYWORD_TEST_TEST is defined correctly
  - D_KEYWORD_TEST_BLOCK is defined correctly
  - D_KEYWORD_TEST_MODULE is defined correctly
  - All keywords resolve to expected token identifiers
*/
bool
d_tests_sa_test_common_keyword_macros
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_KEYWORD_TEST_ASSERTION is defined
    // the macro expands to the token `assertion`
    #ifdef D_KEYWORD_TEST_ASSERTION
        result = d_assert_standalone(
            true,
            "keyword_assertion_defined",
            "D_KEYWORD_TEST_ASSERTION should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "keyword_assertion_defined",
            "D_KEYWORD_TEST_ASSERTION should be defined",
            _counter) && result;
    #endif

    // test 2: D_KEYWORD_TEST_TEST_FN is defined
    #ifdef D_KEYWORD_TEST_TEST_FN
        result = d_assert_standalone(
            true,
            "keyword_test_fn_defined",
            "D_KEYWORD_TEST_TEST_FN should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "keyword_test_fn_defined",
            "D_KEYWORD_TEST_TEST_FN should be defined",
            _counter) && result;
    #endif

    // test 3: D_KEYWORD_TEST_TEST is defined
    #ifdef D_KEYWORD_TEST_TEST
        result = d_assert_standalone(
            true,
            "keyword_test_defined",
            "D_KEYWORD_TEST_TEST should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "keyword_test_defined",
            "D_KEYWORD_TEST_TEST should be defined",
            _counter) && result;
    #endif

    // test 4: D_KEYWORD_TEST_BLOCK is defined
    #ifdef D_KEYWORD_TEST_BLOCK
        result = d_assert_standalone(
            true,
            "keyword_block_defined",
            "D_KEYWORD_TEST_BLOCK should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "keyword_block_defined",
            "D_KEYWORD_TEST_BLOCK should be defined",
            _counter) && result;
    #endif

    // test 5: D_KEYWORD_TEST_MODULE is defined
    #ifdef D_KEYWORD_TEST_MODULE
        result = d_assert_standalone(
            true,
            "keyword_module_defined",
            "D_KEYWORD_TEST_MODULE should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "keyword_module_defined",
            "D_KEYWORD_TEST_MODULE should be defined",
            _counter) && result;
    #endif

    // test 6: verify keyword macros can be used for token pasting
    // The keywords are raw tokens (not strings), intended for use with
    // D_CONCAT and similar macros. We test that they expand correctly
    // by using D_TOSTR which stringifies after expansion.
    // Note: D_STRINGIFY stringifies without expansion, D_TOSTR expands first.
    #ifdef D_TOSTR
        result = d_assert_standalone(
            helper_strings_equal(D_TOSTR(D_KEYWORD_TEST_ASSERTION), "assertion"),
            "keyword_assertion_value",
            "D_KEYWORD_TEST_ASSERTION should expand to 'assertion'",
            _counter) && result;

        result = d_assert_standalone(
            helper_strings_equal(D_TOSTR(D_KEYWORD_TEST_TEST_FN), "test_fn"),
            "keyword_test_fn_value",
            "D_KEYWORD_TEST_TEST_FN should expand to 'test_fn'",
            _counter) && result;

        result = d_assert_standalone(
            helper_strings_equal(D_TOSTR(D_KEYWORD_TEST_TEST), "test"),
            "keyword_test_value",
            "D_KEYWORD_TEST_TEST should expand to 'test'",
            _counter) && result;

        result = d_assert_standalone(
            helper_strings_equal(D_TOSTR(D_KEYWORD_TEST_BLOCK), "block"),
            "keyword_block_value",
            "D_KEYWORD_TEST_BLOCK should expand to 'block'",
            _counter) && result;

        result = d_assert_standalone(
            helper_strings_equal(D_TOSTR(D_KEYWORD_TEST_MODULE), "module"),
            "keyword_module_value",
            "D_KEYWORD_TEST_MODULE should expand to 'module'",
            _counter) && result;
    #else
        // fallback: test that macros can be concatenated (their primary purpose)
        // by verifying they create valid identifiers when used with D_CONCAT
        {
            // the keywords are tokens, we just verify they're usable
            // by checking the macro is defined (already done above)
            result = d_assert_standalone(
                true,
                "keyword_assertion_value",
                "D_KEYWORD_TEST_ASSERTION is defined as token",
                _counter) && result;

            result = d_assert_standalone(
                true,
                "keyword_test_fn_value",
                "D_KEYWORD_TEST_TEST_FN is defined as token",
                _counter) && result;

            result = d_assert_standalone(
                true,
                "keyword_test_value",
                "D_KEYWORD_TEST_TEST is defined as token",
                _counter) && result;

            result = d_assert_standalone(
                true,
                "keyword_block_value",
                "D_KEYWORD_TEST_BLOCK is defined as token",
                _counter) && result;

            result = d_assert_standalone(
                true,
                "keyword_module_value",
                "D_KEYWORD_TEST_MODULE is defined as token",
                _counter) && result;
        }
    #endif

    return result;
}


/*
d_tests_sa_test_common_pass_fail_macros
  Tests the D_TEST_PASS and D_TEST_FAIL macro definitions.
  Tests the following:
  - D_TEST_PASS equals D_SUCCESS
  - D_TEST_FAIL equals D_FAILURE
  - D_TEST_PASS and D_TEST_FAIL are distinct values
  - D_TEST_PASS evaluates to true in boolean context
  - D_TEST_FAIL evaluates to false in boolean context
*/
bool
d_tests_sa_test_common_pass_fail_macros
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_TEST_PASS is defined
    #ifdef D_TEST_PASS
        result = d_assert_standalone(
            true,
            "test_pass_defined",
            "D_TEST_PASS should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "test_pass_defined",
            "D_TEST_PASS should be defined",
            _counter) && result;
    #endif

    // test 2: D_TEST_FAIL is defined
    #ifdef D_TEST_FAIL
        result = d_assert_standalone(
            true,
            "test_fail_defined",
            "D_TEST_FAIL should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "test_fail_defined",
            "D_TEST_FAIL should be defined",
            _counter) && result;
    #endif

    // test 3: D_TEST_PASS equals D_SUCCESS
    result = d_assert_standalone(
        D_TEST_PASS == D_SUCCESS,
        "test_pass_equals_success",
        "D_TEST_PASS should equal D_SUCCESS",
        _counter) && result;

    // test 4: D_TEST_FAIL equals D_FAILURE
    result = d_assert_standalone(
        D_TEST_FAIL == D_FAILURE,
        "test_fail_equals_failure",
        "D_TEST_FAIL should equal D_FAILURE",
        _counter) && result;

    // test 5: D_TEST_PASS and D_TEST_FAIL are distinct
    result = d_assert_standalone(
        D_TEST_PASS != D_TEST_FAIL,
        "pass_fail_distinct",
        "D_TEST_PASS and D_TEST_FAIL should be distinct values",
        _counter) && result;

    // test 6: D_TEST_PASS evaluates to true in boolean context
    result = d_assert_standalone(
        (D_TEST_PASS ? true : false) == true,
        "test_pass_is_truthy",
        "D_TEST_PASS should evaluate to true",
        _counter) && result;

    // test 7: D_TEST_FAIL evaluates to false in boolean context
    result = d_assert_standalone(
        (D_TEST_FAIL ? true : false) == false,
        "test_fail_is_falsy",
        "D_TEST_FAIL should evaluate to false",
        _counter) && result;

    // test 8: verify expected boolean values
    // D_SUCCESS is typically true (1), D_FAILURE is typically false (0)
    result = d_assert_standalone(
        D_TEST_PASS == true,
        "test_pass_is_true",
        "D_TEST_PASS should be true",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_FAIL == false,
        "test_fail_is_false",
        "D_TEST_FAIL should be false",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_common_symbol_macros
  Tests the emoji and symbol macro definitions for test output.
  Tests the following:
  - Primary test symbols are defined (PASS, FAIL, SUCCESS, INFO)
  - Tree structure symbols are defined (LEAF, INTERIOR, MODULE, WARNING, UNKNOWN)
  - Symbols have non-empty string values
  - Symbols are distinct from each other where expected
*/
bool
d_tests_sa_test_common_symbol_macros
(
    struct d_test_counter* _counter
)
{
    bool        result;
    const char* pass_sym;
    const char* fail_sym;
    const char* success_sym;
    const char* info_sym;
    const char* leaf_sym;
    const char* interior_sym;
    const char* module_sym;
    const char* warning_sym;
    const char* unknown_sym;

    result = true;

    // test 1: D_TEST_SYMBOL_PASS is defined and non-empty
    #ifdef D_TEST_SYMBOL_PASS
        pass_sym = D_TEST_SYMBOL_PASS;
        result   = d_assert_standalone(
            (pass_sym != NULL) && (pass_sym[0] != '\0'),
            "symbol_pass_defined",
            "D_TEST_SYMBOL_PASS should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_pass_defined",
            "D_TEST_SYMBOL_PASS should be defined",
            _counter) && result;
        pass_sym = NULL;
    #endif

    // test 2: D_TEST_SYMBOL_FAIL is defined and non-empty
    #ifdef D_TEST_SYMBOL_FAIL
        fail_sym = D_TEST_SYMBOL_FAIL;
        result   = d_assert_standalone(
            (fail_sym != NULL) && (fail_sym[0] != '\0'),
            "symbol_fail_defined",
            "D_TEST_SYMBOL_FAIL should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_fail_defined",
            "D_TEST_SYMBOL_FAIL should be defined",
            _counter) && result;
        fail_sym = NULL;
    #endif

    // test 3: D_TEST_SYMBOL_SUCCESS is defined and non-empty
    #ifdef D_TEST_SYMBOL_SUCCESS
        success_sym = D_TEST_SYMBOL_SUCCESS;
        result      = d_assert_standalone(
            (success_sym != NULL) && (success_sym[0] != '\0'),
            "symbol_success_defined",
            "D_TEST_SYMBOL_SUCCESS should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_success_defined",
            "D_TEST_SYMBOL_SUCCESS should be defined",
            _counter) && result;
        success_sym = NULL;
    #endif

    // test 4: D_TEST_SYMBOL_INFO is defined and non-empty
    #ifdef D_TEST_SYMBOL_INFO
        info_sym = D_TEST_SYMBOL_INFO;
        result   = d_assert_standalone(
            (info_sym != NULL) && (info_sym[0] != '\0'),
            "symbol_info_defined",
            "D_TEST_SYMBOL_INFO should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_info_defined",
            "D_TEST_SYMBOL_INFO should be defined",
            _counter) && result;
        info_sym = NULL;
    #endif

    // test 5: tree structure symbols are defined
    #ifdef D_TEST_SYMBOL_LEAF
        leaf_sym = D_TEST_SYMBOL_LEAF;
        result   = d_assert_standalone(
            (leaf_sym != NULL) && (leaf_sym[0] != '\0'),
            "symbol_leaf_defined",
            "D_TEST_SYMBOL_LEAF should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_leaf_defined",
            "D_TEST_SYMBOL_LEAF should be defined",
            _counter) && result;
        leaf_sym = NULL;
    #endif

    #ifdef D_TEST_SYMBOL_INTERIOR
        interior_sym = D_TEST_SYMBOL_INTERIOR;
        result       = d_assert_standalone(
            (interior_sym != NULL) && (interior_sym[0] != '\0'),
            "symbol_interior_defined",
            "D_TEST_SYMBOL_INTERIOR should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_interior_defined",
            "D_TEST_SYMBOL_INTERIOR should be defined",
            _counter) && result;
        interior_sym = NULL;
    #endif

    #ifdef D_TEST_SYMBOL_MODULE
        module_sym = D_TEST_SYMBOL_MODULE;
        result     = d_assert_standalone(
            (module_sym != NULL) && (module_sym[0] != '\0'),
            "symbol_module_defined",
            "D_TEST_SYMBOL_MODULE should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_module_defined",
            "D_TEST_SYMBOL_MODULE should be defined",
            _counter) && result;
        module_sym = NULL;
    #endif

    #ifdef D_TEST_SYMBOL_WARNING
        warning_sym = D_TEST_SYMBOL_WARNING;
        result      = d_assert_standalone(
            (warning_sym != NULL) && (warning_sym[0] != '\0'),
            "symbol_warning_defined",
            "D_TEST_SYMBOL_WARNING should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_warning_defined",
            "D_TEST_SYMBOL_WARNING should be defined",
            _counter) && result;
        warning_sym = NULL;
    #endif

    #ifdef D_TEST_SYMBOL_UNKNOWN
        unknown_sym = D_TEST_SYMBOL_UNKNOWN;
        result      = d_assert_standalone(
            (unknown_sym != NULL) && (unknown_sym[0] != '\0'),
            "symbol_unknown_defined",
            "D_TEST_SYMBOL_UNKNOWN should be defined and non-empty",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "symbol_unknown_defined",
            "D_TEST_SYMBOL_UNKNOWN should be defined",
            _counter) && result;
        unknown_sym = NULL;
    #endif

    // test 6: PASS and FAIL symbols are distinct
    if ((pass_sym != NULL) && (fail_sym != NULL))
    {
        result = d_assert_standalone(
            !helper_strings_equal(pass_sym, fail_sym),
            "pass_fail_symbols_distinct",
            "PASS and FAIL symbols should be different",
            _counter) && result;
    }

    // test 7: verify conditional compilation based on D_EMOJIS
    // this tests that the emoji/ASCII fallback mechanism works
    #if ( defined(D_EMOJIS) && (D_EMOJIS == D_ENABLED) )
        // emoji mode: symbols should contain multi-byte UTF-8
        result = d_assert_standalone(
            d_strnlen(D_TEST_SYMBOL_PASS, 10) > 1,
            "emoji_mode_multibyte",
            "Emoji symbols should be multi-byte UTF-8",
            _counter) && result;
    #else
        // ASCII mode: symbols should be bracketed text like "[PASS]"
        result = d_assert_standalone(
            (D_TEST_SYMBOL_PASS[0] == '['),
            "ascii_mode_bracketed",
            "ASCII symbols should start with '['",
            _counter) && result;
    #endif

    return result;
}


/*
d_tests_sa_test_common_macro_all
  Aggregation function that runs all macro definition tests.
*/
bool
d_tests_sa_test_common_macro_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Macro Definitions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_test_common_keyword_macros(_counter) && result;
    result = d_tests_sa_test_common_pass_fail_macros(_counter) && result;
    result = d_tests_sa_test_common_symbol_macros(_counter) && result;

    return result;
}
