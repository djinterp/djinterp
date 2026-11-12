/*******************************************************************************
* djinterp [c]                                          string_fn_tests_prefix.c
*
*
* path:      /tests/djinterp/c/string_fn_tests_prefix.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./string_fn_tests.h"


/******************************************************************************
 * PREFIX, SUFFIX, AND CONTAINMENT TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strstartswith
  Tests d_strstartswith for prefix checking.
  Tests the following:
  - returns true for matching prefix
  - returns false for non-matching prefix
  - returns true for empty prefix
  - returns false when prefix longer than string
  - handles NULL inputs
  - returns true when prefix equals string
*/
bool d_tests_string_fn_strstartswith(struct dj_test_suite* _suite)
{
    bool                  test_match;
    bool                  test_no_match;
    bool                  test_empty_prefix;
    bool                  test_prefix_longer;
    bool                  test_null;
    bool                  test_exact;

    test_match        = d_strstartswith("Hello World", 11, "Hello", 5);
    test_no_match     = !d_strstartswith("Hello World", 11, "World", 5);
    test_empty_prefix = d_strstartswith("Hello", 5, "", 0);
    test_prefix_longer = !d_strstartswith("Hi", 2, "Hello", 5);
    test_null         = !d_strstartswith(NULL, 0, "Hi", 2) &&
                        !d_strstartswith("Hi", 2, NULL, 0);
    test_exact        = d_strstartswith("Hello", 5, "Hello", 5);

    // build result tree
    d_test_suite_test(_suite, "d_strstartswith");

    D_TEST_CHECK_MSG(_suite,
                     "match",
                     test_match,
                     "returns true for matching prefix");
    D_TEST_CHECK_MSG(_suite,
                     "no_match",
                     test_no_match,
                     "returns false for non-matching");
    D_TEST_CHECK_MSG(_suite,
                     "empty_prefix",
                     test_empty_prefix,
                     "returns true for empty prefix");
    D_TEST_CHECK_MSG(_suite,
                     "prefix_longer",
                     test_prefix_longer,
                     "returns false when prefix longer");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "handles NULL inputs");
    D_TEST_CHECK_MSG(_suite,
                     "exact",
                     test_exact,
                     "returns true when prefix equals string");

    return true;
}

/*
d_tests_string_fn_strendswith
  Tests d_strendswith for suffix checking.
  Tests the following:
  - returns true for matching suffix
  - returns false for non-matching suffix
  - returns true for empty suffix
  - returns false when suffix longer than string
  - handles NULL inputs
  - returns true when suffix equals string
*/
bool d_tests_string_fn_strendswith(struct dj_test_suite* _suite)
{
    bool                  test_match;
    bool                  test_no_match;
    bool                  test_empty_suffix;
    bool                  test_suffix_longer;
    bool                  test_null;
    bool                  test_exact;

    test_match         = d_strendswith("Hello World", 11, "World", 5);
    test_no_match      = !d_strendswith("Hello World", 11, "Hello", 5);
    test_empty_suffix  = d_strendswith("Hello", 5, "", 0);
    test_suffix_longer = !d_strendswith("Hi", 2, "Hello", 5);
    test_null          = !d_strendswith(NULL, 0, "Hi", 2) &&
                         !d_strendswith("Hi", 2, NULL, 0);
    test_exact         = d_strendswith("Hello", 5, "Hello", 5);

    // build result tree
    d_test_suite_test(_suite, "d_strendswith");

    D_TEST_CHECK_MSG(_suite,
                     "match",
                     test_match,
                     "returns true for matching suffix");
    D_TEST_CHECK_MSG(_suite,
                     "no_match",
                     test_no_match,
                     "returns false for non-matching");
    D_TEST_CHECK_MSG(_suite,
                     "empty_suffix",
                     test_empty_suffix,
                     "returns true for empty suffix");
    D_TEST_CHECK_MSG(_suite,
                     "suffix_longer",
                     test_suffix_longer,
                     "returns false when suffix longer");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "handles NULL inputs");
    D_TEST_CHECK_MSG(_suite,
                     "exact",
                     test_exact,
                     "returns true when suffix equals string");

    return true;
}

/*
d_tests_string_fn_strcontains
  Tests d_strcontains for substring containment.
  Tests the following:
  - finds substring in middle
  - finds substring at beginning
  - finds substring at end
  - returns false when not found
  - handles NULL inputs
  - handles empty substring
*/
bool d_tests_string_fn_strcontains(struct dj_test_suite* _suite)
{
    bool                  test_middle;
    bool                  test_begin;
    bool                  test_end;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty;

    test_middle    = d_strcontains("Hello World", 11, "lo Wo");
    test_begin     = d_strcontains("Hello World", 11, "Hello");
    test_end       = d_strcontains("Hello World", 11, "World");
    test_not_found = !d_strcontains("Hello World", 11, "xyz");
    test_null      = !d_strcontains(NULL, 0, "test") &&
                     !d_strcontains("test", 4, NULL);
    test_empty     = d_strcontains("Hello", 5, "");

    // build result tree
    d_test_suite_test(_suite, "d_strcontains");

    D_TEST_CHECK_MSG(_suite,
                     "middle",
                     test_middle,
                     "finds substring in middle");
    D_TEST_CHECK_MSG(_suite,
                     "begin",
                     test_begin,
                     "finds substring at beginning");
    D_TEST_CHECK_MSG(_suite, "end", test_end, "finds substring at end");
    D_TEST_CHECK_MSG(_suite,
                     "not_found",
                     test_not_found,
                     "returns false when not found");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "handles NULL inputs");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty substring");

    return true;
}

/*
d_tests_string_fn_strcontains_char
  Tests d_strcontains_char for character containment.
  Tests the following:
  - finds character present
  - returns false when not found
  - handles NULL
  - handles empty string
*/
bool d_tests_string_fn_strcontains_char(struct dj_test_suite* _suite)
{
    bool                  test_found;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty;

    test_found     = d_strcontains_char("Hello", 5, 'l');
    test_not_found = !d_strcontains_char("Hello", 5, 'z');
    test_null      = !d_strcontains_char(NULL, 0, 'a');
    test_empty     = !d_strcontains_char("", 0, 'a');

    // build result tree
    d_test_suite_test(_suite, "d_strcontains_char");

    D_TEST_CHECK_MSG(_suite, "found", test_found, "finds character present");
    D_TEST_CHECK_MSG(_suite,
                     "not_found",
                     test_not_found,
                     "returns false when not found");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "handles NULL");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty string");

    return true;
}

/*
d_tests_string_fn_prefix_suffix_all
  Runs all prefix, suffix, and containment tests.
  Tests the following:
  - d_strstartswith
  - d_strendswith
  - d_strcontains
  - d_strcontains_char
*/
bool d_tests_string_fn_prefix_suffix_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Prefix, Suffix, Containment");

    d_tests_string_fn_strstartswith(_suite);
    d_tests_string_fn_strendswith(_suite);
    d_tests_string_fn_strcontains(_suite);
    d_tests_string_fn_strcontains_char(_suite);

    return true;
}
