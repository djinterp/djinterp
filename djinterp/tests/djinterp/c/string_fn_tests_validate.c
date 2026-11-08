/*******************************************************************************
* djinterp [test]                                     string_fn_tests_validate.c
*
*
* path:      /tests/djinterp/c/string_fn_tests_validate.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./string_fn_tests.h"


/******************************************************************************
 * VALIDATION TESTS
 *****************************************************************************/

/*
d_tests_string_fn_str_is_valid
  Tests d_str_is_valid for string validity checking.
  Tests the following:
  - returns true for normal string
  - returns false for NULL
  - returns false for embedded null
  - handles empty string (length 0)
*/
bool d_tests_string_fn_str_is_valid(struct dj_test_suite* _suite)
{
    bool                  test_normal;
    bool                  test_null;
    bool                  test_embedded_null;
    bool                  test_empty;

    // test 1: normal string
    test_normal = d_str_is_valid("Hello", 5);

    // test 2: NULL pointer
    test_null = !d_str_is_valid(NULL, 5);

    // test 3: embedded null
    char buf[] = "He\0lo";
    test_embedded_null = !d_str_is_valid(buf, 5);

    // test 4: empty (length 0 is vacuously true)
    test_empty = d_str_is_valid("anything", 0);

    // build result tree
    d_test_suite_test(_suite, "d_str_is_valid");

    D_TEST_CHECK_MSG(_suite,
                     "normal",
                     test_normal,
                     "returns true for normal string");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "returns false for NULL");
    D_TEST_CHECK_MSG(_suite,
                     "embedded_null",
                     test_embedded_null,
                     "returns false for embedded null");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty string");

    return true;
}

/*
d_tests_string_fn_str_is_ascii
  Tests d_str_is_ascii for ASCII validation.
  Tests the following:
  - returns true for pure ASCII
  - returns false for non-ASCII
  - returns false for NULL
  - handles empty string
*/
bool d_tests_string_fn_str_is_ascii(struct dj_test_suite* _suite)
{
    bool                  test_ascii;
    bool                  test_non_ascii;
    bool                  test_null;
    bool                  test_empty;

    // test 1: pure ASCII
    test_ascii = d_str_is_ascii("Hello 123!@#", 12);

    // test 2: non-ASCII (high byte)
    char non_ascii[] = { 'H', 'i', (char)0x80, '\0' };
    test_non_ascii = !d_str_is_ascii(non_ascii, 3);

    // test 3: NULL
    test_null = !d_str_is_ascii(NULL, 5);

    // test 4: empty
    test_empty = d_str_is_ascii("", 0);

    // build result tree
    d_test_suite_test(_suite, "d_str_is_ascii");

    D_TEST_CHECK_MSG(_suite,
                     "ascii",
                     test_ascii,
                     "returns true for pure ASCII");
    D_TEST_CHECK_MSG(_suite,
                     "non_ascii",
                     test_non_ascii,
                     "returns false for non-ASCII");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "returns false for NULL");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty string");

    return true;
}

/*
d_tests_string_fn_str_is_numeric
  Tests d_str_is_numeric for numeric validation.
  Tests the following:
  - returns true for all digits
  - returns false for letters
  - returns false for mixed
  - returns false for empty
  - returns false for NULL
*/
bool d_tests_string_fn_str_is_numeric(struct dj_test_suite* _suite)
{
    bool                  test_digits;
    bool                  test_letters;
    bool                  test_mixed;
    bool                  test_empty;
    bool                  test_null;

    test_digits  = d_str_is_numeric("1234567890", 10);
    test_letters = !d_str_is_numeric("abcdef", 6);
    test_mixed   = !d_str_is_numeric("abc123", 6);
    test_empty   = !d_str_is_numeric("", 0);
    test_null    = !d_str_is_numeric(NULL, 5);

    // build result tree
    d_test_suite_test(_suite, "d_str_is_numeric");

    D_TEST_CHECK_MSG(_suite,
                     "digits",
                     test_digits,
                     "returns true for all digits");
    D_TEST_CHECK_MSG(_suite,
                     "letters",
                     test_letters,
                     "returns false for letters");
    D_TEST_CHECK_MSG(_suite, "mixed", test_mixed, "returns false for mixed");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "returns false for empty");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "returns false for NULL");

    return true;
}

/*
d_tests_string_fn_str_is_alpha
  Tests d_str_is_alpha for alphabetic validation.
  Tests the following:
  - returns true for all letters
  - returns false for digits
  - returns false for mixed
  - returns false for empty
  - handles both cases
*/
bool d_tests_string_fn_str_is_alpha(struct dj_test_suite* _suite)
{
    bool                  test_alpha;
    bool                  test_digits;
    bool                  test_mixed;
    bool                  test_empty;
    bool                  test_both_cases;

    test_alpha      = d_str_is_alpha("abcdef", 6);
    test_digits     = !d_str_is_alpha("123456", 6);
    test_mixed      = !d_str_is_alpha("abc123", 6);
    test_empty      = !d_str_is_alpha("", 0);
    test_both_cases = d_str_is_alpha("ABCdef", 6);

    // build result tree
    d_test_suite_test(_suite, "d_str_is_alpha");

    D_TEST_CHECK_MSG(_suite,
                     "alpha",
                     test_alpha,
                     "returns true for all letters");
    D_TEST_CHECK_MSG(_suite, "digits", test_digits, "returns false for digits");
    D_TEST_CHECK_MSG(_suite, "mixed", test_mixed, "returns false for mixed");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "returns false for empty");
    D_TEST_CHECK_MSG(_suite,
                     "both_cases",
                     test_both_cases,
                     "handles both cases");

    return true;
}

/*
d_tests_string_fn_str_is_alnum
  Tests d_str_is_alnum for alphanumeric validation.
  Tests the following:
  - returns true for letters and digits
  - returns false for special characters
  - returns false for empty
  - handles pure digits
  - handles pure letters
*/
bool d_tests_string_fn_str_is_alnum(struct dj_test_suite* _suite)
{
    bool                  test_alnum;
    bool                  test_special;
    bool                  test_empty;
    bool                  test_digits_only;
    bool                  test_alpha_only;

    test_alnum       = d_str_is_alnum("abc123DEF", 9);
    test_special     = !d_str_is_alnum("abc!@#", 6);
    test_empty       = !d_str_is_alnum("", 0);
    test_digits_only = d_str_is_alnum("12345", 5);
    test_alpha_only  = d_str_is_alnum("abcXYZ", 6);

    // build result tree
    d_test_suite_test(_suite, "d_str_is_alnum");

    D_TEST_CHECK_MSG(_suite,
                     "alnum",
                     test_alnum,
                     "returns true for alphanumeric");
    D_TEST_CHECK_MSG(_suite,
                     "special",
                     test_special,
                     "returns false for special chars");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "returns false for empty");
    D_TEST_CHECK_MSG(_suite,
                     "digits_only",
                     test_digits_only,
                     "handles pure digits");
    D_TEST_CHECK_MSG(_suite,
                     "alpha_only",
                     test_alpha_only,
                     "handles pure letters");

    return true;
}

/*
d_tests_string_fn_str_is_whitespace
  Tests d_str_is_whitespace for whitespace validation.
  Tests the following:
  - returns true for all whitespace
  - returns false for non-whitespace
  - returns false for empty
  - handles mixed whitespace types
*/
bool d_tests_string_fn_str_is_whitespace(struct dj_test_suite* _suite)
{
    bool                  test_whitespace;
    bool                  test_non_ws;
    bool                  test_empty;
    bool                  test_mixed_ws;

    test_whitespace = d_str_is_whitespace("   ", 3);
    test_non_ws     = !d_str_is_whitespace("hello", 5);
    test_empty      = !d_str_is_whitespace("", 0);
    test_mixed_ws   = d_str_is_whitespace(" \t\n\r ", 5);

    // build result tree
    d_test_suite_test(_suite, "d_str_is_whitespace");

    D_TEST_CHECK_MSG(_suite,
                     "whitespace",
                     test_whitespace,
                     "returns true for all whitespace");
    D_TEST_CHECK_MSG(_suite,
                     "non_ws",
                     test_non_ws,
                     "returns false for non-whitespace");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "returns false for empty");
    D_TEST_CHECK_MSG(_suite,
                     "mixed_ws",
                     test_mixed_ws,
                     "handles mixed whitespace types");

    return true;
}

/*
d_tests_string_fn_validation_all
  Runs all validation tests.
  Tests the following:
  - d_str_is_valid
  - d_str_is_ascii
  - d_str_is_numeric
  - d_str_is_alpha
  - d_str_is_alnum
  - d_str_is_whitespace
*/
bool d_tests_string_fn_validation_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Validation");

    d_tests_string_fn_str_is_valid(_suite);
    d_tests_string_fn_str_is_ascii(_suite);
    d_tests_string_fn_str_is_numeric(_suite);
    d_tests_string_fn_str_is_alpha(_suite);
    d_tests_string_fn_str_is_alnum(_suite);
    d_tests_string_fn_str_is_whitespace(_suite);

    return true;
}
