/*******************************************************************************
* djinterp [test]                                         dstring_tests_search.c
*
*
* path:      /tests/djinterp/c/dstring_tests_search.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dstring_tests.h"


// D_INTERNAL_INDEX_NOT_FOUND
//   macro: sentinel value for "not found" in search functions.
// Using (dj_index)-1 which is the maximum value for the unsigned type.
#define D_INTERNAL_INDEX_NOT_FOUND ((dj_index)-1)


/******************************************************************************
 * SECTION 8: SEARCH FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * I. CHARACTER SEARCH TESTS
 *****************************************************************************/

/*
d_tests_dstring_find_char
  Tests d_string_find_char function.
  Tests the following:
  - finding character at beginning of string
  - finding character at end of string
  - finding character in middle of string
  - finding first occurrence when multiple exist
  - character not found returns not-found sentinel
  - NULL string handling
  - empty string handling
*/
bool d_tests_dstring_find_char(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_find_char");

    // test: find character at beginning
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_char(str, 'h');
        D_TEST_CHECK_MSG(_suite,
                         "find_char_at_beginning",
                         (result) == (0),
                         "should find 'h' at index 0");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_at_beginning",
                         false,
                         "failed to allocate test string");
    }

    // test: find character at end
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_char(str, 'd');
        D_TEST_CHECK_MSG(_suite,
                         "find_char_at_end",
                         (result) == (10),
                         "should find 'd' at index 10");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_at_end",
                         false,
                         "failed to allocate test string");
    }

    // test: find character in middle
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_char(str, ' ');
        D_TEST_CHECK_MSG(_suite,
                         "find_char_in_middle",
                         (result) == (5),
                         "should find space at index 5");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_in_middle",
                         false,
                         "failed to allocate test string");
    }

    // test: find first occurrence when multiple exist
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_find_char(str, 'l');
        D_TEST_CHECK_MSG(_suite,
                         "find_char_first_occurrence",
                         (result) == (2),
                         "should find first 'l' at index 2");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_first_occurrence",
                         false,
                         "failed to allocate test string");
    }

    // test: character not found
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_char(str, 'z');
        D_TEST_CHECK_MSG(_suite,
                         "find_char_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found sentinel for missing char");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_find_char(NULL, 'a');
    D_TEST_CHECK_MSG(_suite,
                     "find_char_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found sentinel for NULL string");

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_find_char(str, 'a');
        D_TEST_CHECK_MSG(_suite,
                         "find_char_empty_string",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found sentinel for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_empty_string",
                         false,
                         "failed to allocate test string");
    }

    return true;
}

/*
d_tests_dstring_find_char_from
  Tests d_string_find_char_from function.
  Tests the following:
  - finding character from start index
  - finding second occurrence by starting after first
  - start index at exact match position
  - start index beyond string length
  - NULL string handling
*/
bool d_tests_dstring_find_char_from(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_find_char_from");

    // test: find character from start index
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_char_from(str, 'o', 0);
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_start",
                         (result) == (4),
                         "should find 'o' at index 4");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_start",
                         false,
                         "failed to allocate test string");
    }

    // test: find second occurrence by starting after first
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_char_from(str, 'o', 5);
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_skip_first",
                         (result) == (7),
                         "should find second 'o' at index 7");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_skip_first",
                         false,
                         "failed to allocate test string");
    }

    // test: start index at exact match position
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_find_char_from(str, 'l', 2);
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_exact_position",
                         (result) == (2),
                         "should find 'l' at start index 2");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_exact_position",
                         false,
                         "failed to allocate test string");
    }

    // test: start index beyond string length
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_find_char_from(str, 'l', 100);
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_beyond_length",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for out-of-bounds start");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_char_from_beyond_length",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_find_char_from(NULL, 'a', 0);
    D_TEST_CHECK_MSG(_suite,
                     "find_char_from_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    return true;
}

/*
d_tests_dstring_rfind_char
  Tests d_string_rfind_char function.
  Tests the following:
  - finding last occurrence of character
  - finding character when only one exists
  - character not found returns not-found sentinel
  - NULL string handling
  - empty string handling
*/
bool d_tests_dstring_rfind_char(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_rfind_char");

    // test: find last occurrence of character
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_rfind_char(str, 'o');
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_last_occurrence",
                         (result) == (7),
                         "should find last 'o' at index 7");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_last_occurrence",
                         false,
                         "failed to allocate test string");
    }

    // test: find character when only one exists
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_rfind_char(str, 'h');
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_single_occurrence",
                         (result) == (0),
                         "should find 'h' at index 0");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_single_occurrence",
                         false,
                         "failed to allocate test string");
    }

    // test: character not found
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_rfind_char(str, 'z');
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for missing char");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_rfind_char(NULL, 'a');
    D_TEST_CHECK_MSG(_suite,
                     "rfind_char_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_rfind_char(str, 'a');
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_empty_string",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for empty string");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_char_empty_string",
                         false,
                         "failed to allocate test string");
    }

    return true;
}

/*
d_tests_dstring_chr
  Tests d_string_chr function (strchr equivalent).
  Tests the following:
  - finding character returns pointer to it
  - finding null terminator
  - character not found returns NULL
  - NULL string handling
*/
bool d_tests_dstring_chr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    char*                 result;

    d_test_suite_test(_suite, "d_string_chr");

    // test: finding character returns pointer to it
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_chr(str, 'w');
        D_TEST_CHECK_MSG(_suite,
                         "chr_find_character",
                         (result != NULL) && (*result == 'w'),
                         "should return pointer to 'w'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "chr_find_character",
                         false,
                         "failed to allocate test string");
    }

    // test: finding null terminator
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_chr(str, '\0');
        D_TEST_CHECK_MSG(_suite,
                         "chr_find_null_terminator",
                         (result != NULL) && (*result == '\0'),
                         "should return pointer to null terminator");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "chr_find_null_terminator",
                         false,
                         "failed to allocate test string");
    }

    // test: character not found returns NULL
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_chr(str, 'z');
        D_TEST_CHECK_MSG(_suite,
                         "chr_not_found",
                         (result) == NULL,
                         "should return NULL for missing char");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "chr_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_chr(NULL, 'a');
    D_TEST_CHECK_MSG(_suite,
                     "chr_null_string",
                     (result) == NULL,
                     "should return NULL for NULL string");

    return true;
}

/*
d_tests_dstring_rchr
  Tests d_string_rchr function (strrchr equivalent).
  Tests the following:
  - finding last occurrence returns pointer to it
  - finding null terminator
  - character not found returns NULL
  - NULL string handling
*/
bool d_tests_dstring_rchr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    char*                 result;

    d_test_suite_test(_suite, "d_string_rchr");

    // test: finding last occurrence returns pointer to it
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_rchr(str, 'l');
        D_TEST_CHECK_MSG(_suite,
                         "rchr_find_last_occurrence",
                         (result != NULL) && (*result == 'l') && (result[1] == 'd'),
                         "should return pointer to last 'l'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rchr_find_last_occurrence",
                         false,
                         "failed to allocate test string");
    }

    // test: finding null terminator
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_rchr(str, '\0');
        D_TEST_CHECK_MSG(_suite,
                         "rchr_find_null_terminator",
                         (result != NULL) && (*result == '\0'),
                         "should return pointer to null terminator");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rchr_find_null_terminator",
                         false,
                         "failed to allocate test string");
    }

    // test: character not found returns NULL
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_rchr(str, 'z');
        D_TEST_CHECK_MSG(_suite,
                         "rchr_not_found",
                         (result) == NULL,
                         "should return NULL for missing char");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rchr_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_rchr(NULL, 'a');
    D_TEST_CHECK_MSG(_suite,
                     "rchr_null_string",
                     (result) == NULL,
                     "should return NULL for NULL string");

    return true;
}

/*
d_tests_dstring_chrnul
  Tests d_string_chrnul function (GNU strchrnul equivalent).
  Tests the following:
  - finding character returns pointer to it
  - character not found returns pointer to null terminator
  - NULL string handling
*/
bool d_tests_dstring_chrnul(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    char*                 result;

    d_test_suite_test(_suite, "d_string_chrnul");

    // test: finding character returns pointer to it
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_chrnul(str, 'w');
        D_TEST_CHECK_MSG(_suite,
                         "chrnul_find_character",
                         (result != NULL) && (*result == 'w'),
                         "should return pointer to 'w'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "chrnul_find_character",
                         false,
                         "failed to allocate test string");
    }

    // test: character not found returns pointer to null terminator
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_chrnul(str, 'z');
        D_TEST_CHECK_MSG(_suite,
                         "chrnul_not_found_returns_end",
                         (result != NULL) && (*result == '\0'),
                         "should return pointer to null terminator");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "chrnul_not_found_returns_end",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_chrnul(NULL, 'a');
    D_TEST_CHECK_MSG(_suite,
                     "chrnul_null_string",
                     (result) == NULL,
                     "should return NULL for NULL string");

    return true;
}


/******************************************************************************
 * II. SUBSTRING SEARCH TESTS
 *****************************************************************************/

/*
d_tests_dstring_find
  Tests d_string_find function.
  Tests the following:
  - finding substring at beginning
  - finding substring at end
  - finding substring in middle
  - finding first occurrence when multiple exist
  - substring not found returns not-found sentinel
  - NULL string handling
  - NULL substring handling
  - empty substring handling
*/
bool d_tests_dstring_find(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      substr;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_find");

    // test: find substring at beginning
    str    = d_string_new_from_cstr("hello world");
    substr = d_string_new_from_cstr("hello");

    if (str && substr)
    {
        result = d_string_find(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_at_beginning",
                         (result) == (0),
                         "should find 'hello' at index 0");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_at_beginning",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: find substring at end
    str    = d_string_new_from_cstr("hello world");
    substr = d_string_new_from_cstr("world");

    if (str && substr)
    {
        result = d_string_find(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_at_end",
                         (result) == (6),
                         "should find 'world' at index 6");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_at_end",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: find substring in middle
    str    = d_string_new_from_cstr("the quick brown fox");
    substr = d_string_new_from_cstr("quick");

    if (str && substr)
    {
        result = d_string_find(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_in_middle",
                         (result) == (4),
                         "should find 'quick' at index 4");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_in_middle",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: find first occurrence when multiple exist
    str    = d_string_new_from_cstr("abcabc");
    substr = d_string_new_from_cstr("abc");

    if (str && substr)
    {
        result = d_string_find(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_first_occurrence",
                         (result) == (0),
                         "should find first 'abc' at index 0");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_first_occurrence",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: substring not found
    str    = d_string_new_from_cstr("hello world");
    substr = d_string_new_from_cstr("xyz");

    if (str && substr)
    {
        result = d_string_find(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for missing substr");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_substr_not_found",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: NULL string handling
    substr = d_string_new_from_cstr("test");
    result = d_string_find(NULL, substr);
    D_TEST_CHECK_MSG(_suite,
                     "find_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    d_string_free(substr);

    // test: NULL substring handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_find(str, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "find_null_substr",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL substr");

    d_string_free(str);

    // test: empty substring handling
    str    = d_string_new_from_cstr("hello");
    substr = d_string_new_from_cstr("");

    if (str && substr)
    {
        result = d_string_find(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "find_empty_substr",
                         (result) == (0),
                         "should return 0 for empty substring");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_empty_substr",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    return true;
}

/*
d_tests_dstring_find_cstr
  Tests d_string_find_cstr function.
  Tests the following:
  - finding C string substring at beginning
  - finding C string substring at end
  - finding C string substring in middle
  - substring not found returns not-found sentinel
  - NULL string handling
  - NULL C string handling
*/
bool d_tests_dstring_find_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_find_cstr");

    // test: find C string at beginning
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_cstr(str, "hello");
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_at_beginning",
                         (result) == (0),
                         "should find 'hello' at index 0");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_at_beginning",
                         false,
                         "failed to allocate test string");
    }

    // test: find C string at end
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_cstr(str, "world");
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_at_end",
                         (result) == (6),
                         "should find 'world' at index 6");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_at_end",
                         false,
                         "failed to allocate test string");
    }

    // test: find C string in middle
    str = d_string_new_from_cstr("the quick brown fox");

    if (str)
    {
        result = d_string_find_cstr(str, "quick");
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_in_middle",
                         (result) == (4),
                         "should find 'quick' at index 4");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_in_middle",
                         false,
                         "failed to allocate test string");
    }

    // test: C string not found
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_find_cstr(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for missing cstr");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_find_cstr(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "find_cstr_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    // test: NULL C string handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_find_cstr(str, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "find_cstr_null_cstr",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL cstr");

    d_string_free(str);

    return true;
}

/*
d_tests_dstring_find_from
  Tests d_string_find_from function.
  Tests the following:
  - finding substring from start index
  - finding second occurrence by starting after first
  - start index at exact match position
  - start index beyond string length
  - NULL string handling
*/
bool d_tests_dstring_find_from(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      substr;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_find_from");

    // test: find substring from start index
    str    = d_string_new_from_cstr("abcabcabc");
    substr = d_string_new_from_cstr("abc");

    if (str && substr)
    {
        result = d_string_find_from(str, substr, 0);
        D_TEST_CHECK_MSG(_suite,
                         "find_from_start",
                         (result) == (0),
                         "should find 'abc' at index 0");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_from_start",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: find second occurrence by starting after first
    str    = d_string_new_from_cstr("abcabcabc");
    substr = d_string_new_from_cstr("abc");

    if (str && substr)
    {
        result = d_string_find_from(str, substr, 1);
        D_TEST_CHECK_MSG(_suite,
                         "find_from_skip_first",
                         (result) == (3),
                         "should find second 'abc' at index 3");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_from_skip_first",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: start index at exact match position
    str    = d_string_new_from_cstr("hello");
    substr = d_string_new_from_cstr("llo");

    if (str && substr)
    {
        result = d_string_find_from(str, substr, 2);
        D_TEST_CHECK_MSG(_suite,
                         "find_from_exact_position",
                         (result) == (2),
                         "should find 'llo' at start index 2");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_from_exact_position",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: start index beyond string length
    str    = d_string_new_from_cstr("hello");
    substr = d_string_new_from_cstr("llo");

    if (str && substr)
    {
        result = d_string_find_from(str, substr, 100);
        D_TEST_CHECK_MSG(_suite,
                         "find_from_beyond_length",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for out-of-bounds start");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_from_beyond_length",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: NULL string handling
    substr = d_string_new_from_cstr("test");
    result = d_string_find_from(NULL, substr, 0);
    D_TEST_CHECK_MSG(_suite,
                     "find_from_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    d_string_free(substr);

    return true;
}

/*
d_tests_dstring_find_cstr_from
  Tests d_string_find_cstr_from function.
  Tests the following:
  - finding C string from start index
  - finding second occurrence by starting after first
  - start index beyond string length
  - NULL string handling
*/
bool d_tests_dstring_find_cstr_from(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_find_cstr_from");

    // test: find C string from start index
    str = d_string_new_from_cstr("abcabcabc");

    if (str)
    {
        result = d_string_find_cstr_from(str, "abc", 0);
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_from_start",
                         (result) == (0),
                         "should find 'abc' at index 0");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_from_start",
                         false,
                         "failed to allocate test string");
    }

    // test: find second occurrence by starting after first
    str = d_string_new_from_cstr("abcabcabc");

    if (str)
    {
        result = d_string_find_cstr_from(str, "abc", 1);
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_from_skip_first",
                         (result) == (3),
                         "should find second 'abc' at index 3");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_from_skip_first",
                         false,
                         "failed to allocate test string");
    }

    // test: start index beyond string length
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_find_cstr_from(str, "llo", 100);
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_from_beyond_length",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for out-of-bounds");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "find_cstr_from_beyond_length",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_find_cstr_from(NULL, "test", 0);
    D_TEST_CHECK_MSG(_suite,
                     "find_cstr_from_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    return true;
}

/*
d_tests_dstring_rfind
  Tests d_string_rfind function.
  Tests the following:
  - finding last occurrence of substring
  - finding substring when only one exists
  - substring not found returns not-found sentinel
  - NULL string handling
  - NULL substring handling
*/
bool d_tests_dstring_rfind(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      substr;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_rfind");

    // test: find last occurrence of substring
    str    = d_string_new_from_cstr("abcabcabc");
    substr = d_string_new_from_cstr("abc");

    if (str && substr)
    {
        result = d_string_rfind(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "rfind_last_occurrence",
                         (result) == (6),
                         "should find last 'abc' at index 6");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_last_occurrence",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: find substring when only one exists
    str    = d_string_new_from_cstr("hello world");
    substr = d_string_new_from_cstr("world");

    if (str && substr)
    {
        result = d_string_rfind(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "rfind_single_occurrence",
                         (result) == (6),
                         "should find 'world' at index 6");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_single_occurrence",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: substring not found
    str    = d_string_new_from_cstr("hello world");
    substr = d_string_new_from_cstr("xyz");

    if (str && substr)
    {
        result = d_string_rfind(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "rfind_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for missing substr");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_not_found",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: NULL string handling
    substr = d_string_new_from_cstr("test");
    result = d_string_rfind(NULL, substr);
    D_TEST_CHECK_MSG(_suite,
                     "rfind_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    d_string_free(substr);

    // test: NULL substring handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_rfind(str, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "rfind_null_substr",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL substr");

    d_string_free(str);

    return true;
}

/*
d_tests_dstring_rfind_cstr
  Tests d_string_rfind_cstr function.
  Tests the following:
  - finding last occurrence of C string
  - finding C string when only one exists
  - C string not found returns not-found sentinel
  - NULL string handling
*/
bool d_tests_dstring_rfind_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_rfind_cstr");

    // test: find last occurrence of C string
    str = d_string_new_from_cstr("abcabcabc");

    if (str)
    {
        result = d_string_rfind_cstr(str, "abc");
        D_TEST_CHECK_MSG(_suite,
                         "rfind_cstr_last_occurrence",
                         (result) == (6),
                         "should find last 'abc' at index 6");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_cstr_last_occurrence",
                         false,
                         "failed to allocate test string");
    }

    // test: find C string when only one exists
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_rfind_cstr(str, "world");
        D_TEST_CHECK_MSG(_suite,
                         "rfind_cstr_single_occurrence",
                         (result) == (6),
                         "should find 'world' at index 6");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_cstr_single_occurrence",
                         false,
                         "failed to allocate test string");
    }

    // test: C string not found
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_rfind_cstr(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "rfind_cstr_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for missing cstr");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "rfind_cstr_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_rfind_cstr(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "rfind_cstr_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    return true;
}

/*
d_tests_dstring_str
  Tests d_string_str function (strstr equivalent).
  Tests the following:
  - finding substring returns pointer to it
  - substring not found returns NULL
  - NULL string handling
*/
bool d_tests_dstring_str(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    char*                 result;

    d_test_suite_test(_suite, "d_string_str");

    // test: finding substring returns pointer to it
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_str(str, "world");
        D_TEST_CHECK_MSG(_suite,
                         "str_find_substring",
                         (result != NULL) && (strncmp(result, "world", 5) == 0),
                         "should return pointer to 'world'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "str_find_substring",
                         false,
                         "failed to allocate test string");
    }

    // test: substring not found returns NULL
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_str(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "str_not_found",
                         (result) == NULL,
                         "should return NULL for missing substr");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "str_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_str(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "str_null_string",
                     (result) == NULL,
                     "should return NULL for NULL string");

    return true;
}


/******************************************************************************
 * III. CASE-INSENSITIVE SEARCH TESTS
 *****************************************************************************/

/*
d_tests_dstring_casefind
  Tests d_string_casefind function.
  Tests the following:
  - finding substring with matching case
  - finding substring with different case
  - finding mixed case substring
  - substring not found returns not-found sentinel
  - NULL string handling
*/
bool d_tests_dstring_casefind(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      substr;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_casefind");

    // test: find substring with matching case
    str    = d_string_new_from_cstr("Hello World");
    substr = d_string_new_from_cstr("World");

    if (str && substr)
    {
        result = d_string_casefind(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "casefind_matching_case",
                         (result) == (6),
                         "should find 'World' at index 6");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casefind_matching_case",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: find substring with different case
    str    = d_string_new_from_cstr("Hello World");
    substr = d_string_new_from_cstr("WORLD");

    if (str && substr)
    {
        result = d_string_casefind(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "casefind_different_case",
                         (result) == (6),
                         "should find 'WORLD' at index 6 (case-insensitive)");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casefind_different_case",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: find mixed case substring
    str    = d_string_new_from_cstr("HeLLo WoRLd");
    substr = d_string_new_from_cstr("world");

    if (str && substr)
    {
        result = d_string_casefind(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "casefind_mixed_case",
                         (result) == (6),
                         "should find 'world' at index 6 (case-insensitive)");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casefind_mixed_case",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: substring not found
    str    = d_string_new_from_cstr("Hello World");
    substr = d_string_new_from_cstr("xyz");

    if (str && substr)
    {
        result = d_string_casefind(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "casefind_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for missing substr");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casefind_not_found",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: NULL string handling
    substr = d_string_new_from_cstr("test");
    result = d_string_casefind(NULL, substr);
    D_TEST_CHECK_MSG(_suite,
                     "casefind_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    d_string_free(substr);

    return true;
}

/*
d_tests_dstring_casefind_cstr
  Tests d_string_casefind_cstr function.
  Tests the following:
  - finding C string with matching case
  - finding C string with different case
  - C string not found returns not-found sentinel
  - NULL string handling
*/
bool d_tests_dstring_casefind_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    dj_index               result;

    d_test_suite_test(_suite, "d_string_casefind_cstr");

    // test: find C string with matching case
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_casefind_cstr(str, "World");
        D_TEST_CHECK_MSG(_suite,
                         "casefind_cstr_matching_case",
                         (result) == (6),
                         "should find 'World' at index 6");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casefind_cstr_matching_case",
                         false,
                         "failed to allocate test string");
    }

    // test: find C string with different case
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_casefind_cstr(str, "WORLD");
        D_TEST_CHECK_MSG(_suite,
                         "casefind_cstr_different_case",
                         (result) == (6),
                         "should find 'WORLD' at index 6 (case-insensitive)");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casefind_cstr_different_case",
                         false,
                         "failed to allocate test string");
    }

    // test: C string not found
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_casefind_cstr(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "casefind_cstr_not_found",
                         (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                         "should return not-found for missing cstr");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casefind_cstr_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_casefind_cstr(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "casefind_cstr_null_string",
                     (result) == (D_INTERNAL_INDEX_NOT_FOUND),
                     "should return not-found for NULL string");

    return true;
}

/*
d_tests_dstring_casestr
  Tests d_string_casestr function (strcasestr equivalent).
  Tests the following:
  - finding substring returns pointer (case-insensitive)
  - substring not found returns NULL
  - NULL string handling
*/
bool d_tests_dstring_casestr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    char*                 result;

    d_test_suite_test(_suite, "d_string_casestr");

    // test: finding substring returns pointer (case-insensitive)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_casestr(str, "WORLD");
        D_TEST_CHECK_MSG(_suite,
                         "casestr_find_substring",
                         (result) != NULL,
                         "should return pointer to match (case-insensitive)");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casestr_find_substring",
                         false,
                         "failed to allocate test string");
    }

    // test: substring not found returns NULL
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        result = d_string_casestr(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "casestr_not_found",
                         (result) == NULL,
                         "should return NULL for missing substr");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "casestr_not_found",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_casestr(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "casestr_null_string",
                     (result) == NULL,
                     "should return NULL for NULL string");

    return true;
}


/******************************************************************************
 * IV. CONTAINMENT CHECK TESTS
 *****************************************************************************/

/*
d_tests_dstring_contains
  Tests d_string_contains function.
  Tests the following:
  - string contains substring returns true
  - string does not contain substring returns false
  - NULL string handling
  - NULL substring handling
  - empty substring handling
*/
bool d_tests_dstring_contains(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      substr;
    bool                  result;

    d_test_suite_test(_suite, "d_string_contains");

    // test: string contains substring
    str    = d_string_new_from_cstr("hello world");
    substr = d_string_new_from_cstr("world");

    if (str && substr)
    {
        result = d_string_contains(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "contains_true",
                         result == true,
                         "should return true when substring exists");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "contains_true",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: string does not contain substring
    str    = d_string_new_from_cstr("hello world");
    substr = d_string_new_from_cstr("xyz");

    if (str && substr)
    {
        result = d_string_contains(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "contains_false",
                         !(result),
                         "should return false when substring missing");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "contains_false",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    // test: NULL string handling
    substr = d_string_new_from_cstr("test");
    result = d_string_contains(NULL, substr);
    D_TEST_CHECK_MSG(_suite,
                     "contains_null_string",
                     !(result),
                     "should return false for NULL string");

    d_string_free(substr);

    // test: NULL substring handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_contains(str, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "contains_null_substr",
                     !(result),
                     "should return false for NULL substr");

    d_string_free(str);

    // test: empty substring handling
    str    = d_string_new_from_cstr("hello");
    substr = d_string_new_from_cstr("");

    if (str && substr)
    {
        result = d_string_contains(str, substr);
        D_TEST_CHECK_MSG(_suite,
                         "contains_empty_substr",
                         result == true,
                         "should return true for empty substring");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "contains_empty_substr",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(substr);

    return true;
}

/*
d_tests_dstring_contains_cstr
  Tests d_string_contains_cstr function.
  Tests the following:
  - string contains C string returns true
  - string does not contain C string returns false
  - NULL string handling
  - NULL C string handling
*/
bool d_tests_dstring_contains_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_contains_cstr");

    // test: string contains C string
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_contains_cstr(str, "world");
        D_TEST_CHECK_MSG(_suite,
                         "contains_cstr_true",
                         result == true,
                         "should return true when cstr exists");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "contains_cstr_true",
                         false,
                         "failed to allocate test string");
    }

    // test: string does not contain C string
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_contains_cstr(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "contains_cstr_false",
                         !(result),
                         "should return false when cstr missing");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "contains_cstr_false",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_contains_cstr(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "contains_cstr_null_string",
                     !(result),
                     "should return false for NULL string");

    // test: NULL C string handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_contains_cstr(str, NULL);
    D_TEST_CHECK_MSG(_suite,
                     "contains_cstr_null_cstr",
                     !(result),
                     "should return false for NULL cstr");

    d_string_free(str);

    return true;
}

/*
d_tests_dstring_contains_char
  Tests d_string_contains_char function.
  Tests the following:
  - string contains character returns true
  - string does not contain character returns false
  - NULL string handling
*/
bool d_tests_dstring_contains_char(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_contains_char");

    // test: string contains character
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_contains_char(str, 'w');
        D_TEST_CHECK_MSG(_suite,
                         "contains_char_true",
                         result == true,
                         "should return true when char exists");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "contains_char_true",
                         false,
                         "failed to allocate test string");
    }

    // test: string does not contain character
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_contains_char(str, 'z');
        D_TEST_CHECK_MSG(_suite,
                         "contains_char_false",
                         !(result),
                         "should return false when char missing");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "contains_char_false",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_contains_char(NULL, 'a');
    D_TEST_CHECK_MSG(_suite,
                     "contains_char_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_starts_with
  Tests d_string_starts_with function.
  Tests the following:
  - string starts with prefix returns true
  - string does not start with prefix returns false
  - exact match returns true
  - empty prefix returns true
  - NULL string handling
*/
bool d_tests_dstring_starts_with(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      prefix;
    bool                  result;

    d_test_suite_test(_suite, "d_string_starts_with");

    // test: string starts with prefix
    str    = d_string_new_from_cstr("hello world");
    prefix = d_string_new_from_cstr("hello");

    if (str && prefix)
    {
        result = d_string_starts_with(str, prefix);
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_true",
                         result == true,
                         "should return true when prefix matches");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_true",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(prefix);

    // test: string does not start with prefix
    str    = d_string_new_from_cstr("hello world");
    prefix = d_string_new_from_cstr("world");

    if (str && prefix)
    {
        result = d_string_starts_with(str, prefix);
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_false",
                         !(result),
                         "should return false when prefix doesn't match");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_false",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(prefix);

    // test: exact match
    str    = d_string_new_from_cstr("hello");
    prefix = d_string_new_from_cstr("hello");

    if (str && prefix)
    {
        result = d_string_starts_with(str, prefix);
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_exact_match",
                         result == true,
                         "should return true for exact match");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_exact_match",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(prefix);

    // test: empty prefix
    str    = d_string_new_from_cstr("hello");
    prefix = d_string_new_from_cstr("");

    if (str && prefix)
    {
        result = d_string_starts_with(str, prefix);
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_empty_prefix",
                         result == true,
                         "should return true for empty prefix");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_empty_prefix",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(prefix);

    // test: NULL string handling
    prefix = d_string_new_from_cstr("test");
    result = d_string_starts_with(NULL, prefix);
    D_TEST_CHECK_MSG(_suite,
                     "starts_with_null_string",
                     !(result),
                     "should return false for NULL string");

    d_string_free(prefix);

    return true;
}

/*
d_tests_dstring_starts_with_cstr
  Tests d_string_starts_with_cstr function.
  Tests the following:
  - string starts with C string prefix returns true
  - string does not start with C string prefix returns false
  - NULL string handling
*/
bool d_tests_dstring_starts_with_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_starts_with_cstr");

    // test: string starts with C string prefix
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_starts_with_cstr(str, "hello");
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_cstr_true",
                         result == true,
                         "should return true when prefix matches");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_cstr_true",
                         false,
                         "failed to allocate test string");
    }

    // test: string does not start with C string prefix
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_starts_with_cstr(str, "world");
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_cstr_false",
                         !(result),
                         "should return false when prefix doesn't match");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "starts_with_cstr_false",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_starts_with_cstr(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "starts_with_cstr_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}

/*
d_tests_dstring_ends_with
  Tests d_string_ends_with function.
  Tests the following:
  - string ends with suffix returns true
  - string does not end with suffix returns false
  - exact match returns true
  - empty suffix returns true
  - NULL string handling
*/
bool d_tests_dstring_ends_with(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    struct dj_string*      suffix;
    bool                  result;

    d_test_suite_test(_suite, "d_string_ends_with");

    // test: string ends with suffix
    str    = d_string_new_from_cstr("hello world");
    suffix = d_string_new_from_cstr("world");

    if ( (str) &&
         (suffix) )
    {
        result = d_string_ends_with(str, suffix);
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_true",
                         result == true,
                         "should return true when suffix matches");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_true",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(suffix);

    // test: string does not end with suffix
    str    = d_string_new_from_cstr("hello world");
    suffix = d_string_new_from_cstr("hello");

    if ( (str) &&
         (suffix) )
    {
        result = d_string_ends_with(str, suffix);
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_false",
                         !(result),
                         "should return false when suffix doesn't match");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_false",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(suffix);

    // test: exact match
    str    = d_string_new_from_cstr("hello");
    suffix = d_string_new_from_cstr("hello");

    if ( (str) &&
         (suffix) )
    {
        result = d_string_ends_with(str, suffix);
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_exact_match",
                         result == true,
                         "should return true for exact match");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_exact_match",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(suffix);

    // test: empty suffix
    str    = d_string_new_from_cstr("hello");
    suffix = d_string_new_from_cstr("");

    if ( (str) &&
         (suffix) )
    {
        result = d_string_ends_with(str, suffix);
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_empty_suffix",
                         result == true,
                         "should return true for empty suffix");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_empty_suffix",
                         false,
                         "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(suffix);

    // test: NULL string handling
    suffix = d_string_new_from_cstr("test");
    result = d_string_ends_with(NULL, suffix);
    D_TEST_CHECK_MSG(_suite,
                     "ends_with_null_string",
                     !(result),
                     "should return false for NULL string");

    d_string_free(suffix);

    return true;
}

/*
d_tests_dstring_ends_with_cstr
  Tests d_string_ends_with_cstr function.
  Tests the following:
  - string ends with C string suffix returns true
  - string does not end with C string suffix returns false
  - NULL string handling
*/
bool d_tests_dstring_ends_with_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    bool                  result;

    d_test_suite_test(_suite, "d_string_ends_with_cstr");

    // test: string ends with C string suffix
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_ends_with_cstr(str, "world");
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_cstr_true",
                         result == true,
                         "should return true when suffix matches");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_cstr_true",
                         false,
                         "failed to allocate test string");
    }

    // test: string does not end with C string suffix
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_ends_with_cstr(str, "hello");
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_cstr_false",
                         !(result),
                         "should return false when suffix doesn't match");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "ends_with_cstr_false",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_ends_with_cstr(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "ends_with_cstr_null_string",
                     !(result),
                     "should return false for NULL string");

    return true;
}


/******************************************************************************
 * V. SPAN FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_dstring_spn
  Tests d_string_spn function (strspn equivalent).
  Tests the following:
  - counting initial span of matching characters
  - no matching characters returns 0
  - all characters match
  - NULL string handling
*/
bool d_tests_dstring_spn(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                result;

    d_test_suite_test(_suite, "d_string_spn");

    // test: counting initial span of matching characters
    str = d_string_new_from_cstr("aabbccxyz");

    if (str)
    {
        result = d_string_spn(str, "abc");
        D_TEST_CHECK_MSG(_suite,
                         "spn_initial_span",
                         (result) == (6),
                         "should count 6 initial matching chars");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "spn_initial_span",
                         false,
                         "failed to allocate test string");
    }

    // test: no matching characters returns 0
    str = d_string_new_from_cstr("xyz");

    if (str)
    {
        result = d_string_spn(str, "abc");
        D_TEST_CHECK_MSG(_suite,
                         "spn_no_match",
                         (result) == (0),
                         "should return 0 when no initial match");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "spn_no_match",
                         false,
                         "failed to allocate test string");
    }

    // test: all characters match
    str = d_string_new_from_cstr("aabbcc");

    if (str)
    {
        result = d_string_spn(str, "abc");
        D_TEST_CHECK_MSG(_suite,
                         "spn_all_match",
                         (result) == (6),
                         "should return full length when all match");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "spn_all_match",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_spn(NULL, "abc");
    D_TEST_CHECK_MSG(_suite,
                     "spn_null_string",
                     (result) == (0),
                     "should return 0 for NULL string");

    return true;
}

/*
d_tests_dstring_cspn
  Tests d_string_cspn function (strcspn equivalent).
  Tests the following:
  - counting initial span of non-matching characters
  - first character matches returns 0
  - no matching characters returns full length
  - NULL string handling
*/
bool d_tests_dstring_cspn(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                result;

    d_test_suite_test(_suite, "d_string_cspn");

    // test: counting initial span of non-matching characters
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_cspn(str, " ");
        D_TEST_CHECK_MSG(_suite,
                         "cspn_initial_span",
                         (result) == (5),
                         "should count 5 chars before space");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "cspn_initial_span",
                         false,
                         "failed to allocate test string");
    }

    // test: first character matches returns 0
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_cspn(str, "h");
        D_TEST_CHECK_MSG(_suite,
                         "cspn_first_match",
                         (result) == (0),
                         "should return 0 when first char matches");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "cspn_first_match",
                         false,
                         "failed to allocate test string");
    }

    // test: no matching characters returns full length
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_cspn(str, "xyz");
        D_TEST_CHECK_MSG(_suite,
                         "cspn_no_match",
                         (result) == (5),
                         "should return full length when no match");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "cspn_no_match",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_cspn(NULL, "abc");
    D_TEST_CHECK_MSG(_suite,
                     "cspn_null_string",
                     (result) == (0),
                     "should return 0 for NULL string");

    return true;
}

/*
d_tests_dstring_pbrk
  Tests d_string_pbrk function (strpbrk equivalent).
  Tests the following:
  - finding first matching character returns pointer
  - no matching character returns NULL
  - NULL string handling
*/
bool d_tests_dstring_pbrk(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    char*                 result;

    d_test_suite_test(_suite, "d_string_pbrk");

    // test: finding first matching character returns pointer
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_pbrk(str, "aeiou");
        D_TEST_CHECK_MSG(_suite,
                         "pbrk_find_match",
                         (result != NULL) && (*result == 'e'),
                         "should return pointer to first vowel 'e'");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "pbrk_find_match",
                         false,
                         "failed to allocate test string");
    }

    // test: no matching character returns NULL
    str = d_string_new_from_cstr("xyz");

    if (str)
    {
        result = d_string_pbrk(str, "abc");
        D_TEST_CHECK_MSG(_suite,
                         "pbrk_no_match",
                         (result) == NULL,
                         "should return NULL when no match");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "pbrk_no_match",
                         false,
                         "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_pbrk(NULL, "abc");
    D_TEST_CHECK_MSG(_suite,
                     "pbrk_null_string",
                     (result) == NULL,
                     "should return NULL for NULL string");

    return true;
}


/******************************************************************************
 * SEARCH ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_dstring_search_all
  Runs all search function tests for dstring module.
  Tests the following:
  - character search functions (find_char, find_char_from, rfind_char,
    chr, rchr, chrnul)
  - substring search functions (find, find_cstr, find_from, find_cstr_from,
    rfind, rfind_cstr, str)
  - case-insensitive search functions (casefind, casefind_cstr, casestr)
  - containment check functions (contains, contains_cstr, contains_char,
    starts_with, starts_with_cstr, ends_with, ends_with_cstr)
  - span functions (spn, cspn, pbrk)
*/
bool d_tests_dstring_search_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Search Functions");

    // I. character search tests
    d_tests_dstring_find_char(_suite);
    d_tests_dstring_find_char_from(_suite);
    d_tests_dstring_rfind_char(_suite);
    d_tests_dstring_chr(_suite);
    d_tests_dstring_rchr(_suite);
    d_tests_dstring_chrnul(_suite);

    // II. substring search tests
    d_tests_dstring_find(_suite);
    d_tests_dstring_find_cstr(_suite);
    d_tests_dstring_find_from(_suite);
    d_tests_dstring_find_cstr_from(_suite);
    d_tests_dstring_rfind(_suite);
    d_tests_dstring_rfind_cstr(_suite);
    d_tests_dstring_str(_suite);

    // III. case-insensitive search tests
    d_tests_dstring_casefind(_suite);
    d_tests_dstring_casefind_cstr(_suite);
    d_tests_dstring_casestr(_suite);

    // IV. containment check tests
    d_tests_dstring_contains(_suite);
    d_tests_dstring_contains_cstr(_suite);
    d_tests_dstring_contains_char(_suite);
    d_tests_dstring_starts_with(_suite);
    d_tests_dstring_starts_with_cstr(_suite);
    d_tests_dstring_ends_with(_suite);
    d_tests_dstring_ends_with_cstr(_suite);

    // V. span function tests
    d_tests_dstring_spn(_suite);
    d_tests_dstring_cspn(_suite);
    d_tests_dstring_pbrk(_suite);

    return true;
}
