/*******************************************************************************
* djinterp [c]                                           string_fn_tests_count.c
*
*
* path:      /tests/djinterp/c/string_fn_tests_count.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./string_fn_tests.h"


/******************************************************************************
 * COUNTING TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strcount_char
  Tests d_strcount_char for character counting.
  Tests the following:
  - counts single occurrence
  - counts multiple occurrences
  - returns 0 when not found
  - handles NULL string
  - handles empty string
  - counts at boundaries
*/
bool d_tests_string_fn_strcount_char(struct dj_test_suite* _suite)
{
    size_t                result;
    bool                  test_single;
    bool                  test_multiple;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty;
    bool                  test_boundaries;

    // test 1: single occurrence
    result = d_strcount_char("Hello", 5, 'H');
    test_single = (result == 1);

    // test 2: multiple occurrences
    result = d_strcount_char("aababcabc", 9, 'a');
    test_multiple = (result == 4);

    // test 3: not found
    result = d_strcount_char("Hello", 5, 'z');
    test_not_found = (result == 0);

    // test 4: NULL
    result = d_strcount_char(NULL, 5, 'a');
    test_null = (result == 0);

    // test 5: empty
    result = d_strcount_char("", 0, 'a');
    test_empty = (result == 0);

    // test 6: count at first and last positions
    result = d_strcount_char("abba", 4, 'a');
    test_boundaries = (result == 2);

    // build result tree
    d_test_suite_test(_suite, "d_strcount_char");

    D_TEST_CHECK_MSG(_suite, "single", test_single, "counts single occurrence");
    D_TEST_CHECK_MSG(_suite,
                     "multiple",
                     test_multiple,
                     "counts multiple occurrences");
    D_TEST_CHECK_MSG(_suite,
                     "not_found",
                     test_not_found,
                     "returns 0 when not found");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "handles NULL string");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty string");
    D_TEST_CHECK_MSG(_suite,
                     "boundaries",
                     test_boundaries,
                     "counts at boundaries");

    return true;
}

/*
d_tests_string_fn_strcount_substr
  Tests d_strcount_substr for substring counting.
  Tests the following:
  - counts non-overlapping occurrences
  - returns 0 when not found
  - handles NULL inputs
  - handles empty substring
  - handles substring longer than string
  - counts adjacent occurrences
*/
bool d_tests_string_fn_strcount_substr(struct dj_test_suite* _suite)
{
    size_t                result;
    bool                  test_non_overlapping;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty_substr;
    bool                  test_longer_substr;
    bool                  test_adjacent;

    // test 1: non-overlapping
    result = d_strcount_substr("abcabcabc", 9, "abc");
    test_non_overlapping = (result == 3);

    // test 2: not found
    result = d_strcount_substr("Hello World", 11, "xyz");
    test_not_found = (result == 0);

    // test 3: NULL inputs
    size_t r1 = d_strcount_substr(NULL, 5, "abc");
    size_t r2 = d_strcount_substr("abc", 3, NULL);
    test_null = (r1 == 0) && (r2 == 0);

    // test 4: empty substring
    result = d_strcount_substr("Hello", 5, "");
    test_empty_substr = (result == 0);

    // test 5: substring longer than string
    result = d_strcount_substr("Hi", 2, "Hello");
    test_longer_substr = (result == 0);

    // test 6: adjacent occurrences
    result = d_strcount_substr("aaaa", 4, "aa");
    test_adjacent = (result == 2);

    // build result tree
    d_test_suite_test(_suite, "d_strcount_substr");

    D_TEST_CHECK_MSG(_suite,
                     "non_overlapping",
                     test_non_overlapping,
                     "counts non-overlapping occurrences");
    D_TEST_CHECK_MSG(_suite,
                     "not_found",
                     test_not_found,
                     "returns 0 when not found");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "handles NULL inputs");
    D_TEST_CHECK_MSG(_suite,
                     "empty_substr",
                     test_empty_substr,
                     "handles empty substring");
    D_TEST_CHECK_MSG(_suite,
                     "longer_substr",
                     test_longer_substr,
                     "handles longer substring");
    D_TEST_CHECK_MSG(_suite,
                     "adjacent",
                     test_adjacent,
                     "counts adjacent non-overlapping");

    return true;
}

/*
d_tests_string_fn_counting_all
  Runs all counting tests.
  Tests the following:
  - d_strcount_char
  - d_strcount_substr
*/
bool d_tests_string_fn_counting_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Counting");

    d_tests_string_fn_strcount_char(_suite);
    d_tests_string_fn_strcount_substr(_suite);

    return true;
}


/******************************************************************************
 * HASH TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strhash
  Tests d_strhash for string hashing.
  Tests the following:
  - produces consistent hash for same input
  - different strings produce different hashes
  - handles NULL
  - handles empty string
  - hash changes with length
*/
bool d_tests_string_fn_strhash(struct dj_test_suite* _suite)
{
    size_t                hash1;
    size_t                hash2;
    bool                  test_consistent;
    bool                  test_different;
    bool                  test_null;
    bool                  test_empty;
    bool                  test_length_sensitive;

    // test 1: consistent
    hash1 = d_strhash("Hello", 5);
    hash2 = d_strhash("Hello", 5);
    test_consistent = (hash1 == hash2);

    // test 2: different strings => different hashes (very likely)
    hash1 = d_strhash("Hello", 5);
    hash2 = d_strhash("World", 5);
    test_different = (hash1 != hash2);

    // test 3: NULL
    hash1 = d_strhash(NULL, 5);
    test_null = (hash1 == 0);

    // test 4: empty
    hash1 = d_strhash("", 0);
    test_empty = (hash1 == 5381);  // djb2 initial value

    // test 5: same prefix but different length
    hash1 = d_strhash("Hello", 3);
    hash2 = d_strhash("Hello", 5);
    test_length_sensitive = (hash1 != hash2);

    // build result tree
    d_test_suite_test(_suite, "d_strhash");

    D_TEST_CHECK_MSG(_suite,
                     "consistent",
                     test_consistent,
                     "produces consistent hash");
    D_TEST_CHECK_MSG(_suite,
                     "different",
                     test_different,
                     "different strings produce different hashes");
    D_TEST_CHECK_MSG(_suite, "null", test_null, "handles NULL");
    D_TEST_CHECK_MSG(_suite, "empty", test_empty, "handles empty string");
    D_TEST_CHECK_MSG(_suite,
                     "length_sensitive",
                     test_length_sensitive,
                     "hash changes with length");

    return true;
}

/*
d_tests_string_fn_hash_all
  Runs all hash tests.
  Tests the following:
  - d_strhash
*/
bool d_tests_string_fn_hash_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Hash");

    d_tests_string_fn_strhash(_suite);

    return true;
}
