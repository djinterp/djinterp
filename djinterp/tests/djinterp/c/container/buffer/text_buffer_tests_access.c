/*******************************************************************************
* djinterp [container]                                text_buffer_tests_access.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_access.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_get_char
  Tests the d_text_buffer_get_char function.
  Tests the following:
  - NULL buffer returns '\0'
  - out-of-bounds index returns '\0'
  - correct character at first, middle, and last positions
  - negative indexing (if supported by dj_index)
*/
bool d_tests_text_buffer_get_char(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_get_char");

    result = D_TEST_CHECK_MSG(_suite,
                              "get_char_null",
                              d_text_buffer_get_char(NULL, 0) == '\0',
                              "NULL buffer should return '\\0'") && result;

    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        // test 2: first character
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_char_first",
                                  d_text_buffer_get_char(buffer, 0) == 'A',
                                  "First character should be 'A'") && result;

        // test 3: middle character
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_char_middle",
                                  d_text_buffer_get_char(buffer, 2) == 'C',
                                  "Middle character should be 'C'") && result;

        // test 4: last character
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_char_last",
                                  d_text_buffer_get_char(buffer, 4) == 'E',
                                  "Last character should be 'E'") && result;

        // test 5: negative index (last element)
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_char_neg_1",
                                  d_text_buffer_get_char(buffer, -1) == 'E',
                                  "Index -1 should return 'E'") && result;

        // test 6: negative index (first element)
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_char_neg_5",
                                  d_text_buffer_get_char(buffer, -5) == 'A',
                                  "Index -5 should return 'A'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_set_char
  Tests the d_text_buffer_set_char function.
  Tests the following:
  - NULL buffer returns false
  - out-of-bounds index returns false
  - successful set at beginning, middle, and end
  - neighbouring characters unchanged
*/
bool d_tests_text_buffer_set_char(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    const char*           str;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_set_char");

    result = D_TEST_CHECK_MSG(_suite,
                              "set_char_null",
                              d_text_buffer_set_char(NULL, 0, 'X') == false,
                              "NULL buffer should return false") && result;

    buffer = d_text_buffer_new_from_string("ABCDE");

    if (buffer)
    {
        // test 2: set first character
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_char_first",
                                  d_text_buffer_set_char(buffer, 0, 'X') == true,
                                  "Set first char should succeed") && result;

        // test 3: set last character
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_char_last",
                                  d_text_buffer_set_char(buffer, 4, 'Z') == true,
                                  "Set last char should succeed") && result;

        // test 4: verify content
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_char_content",
                                  str[0] == 'X' && str[1] == 'B' && str[2] == 'C' && str[3] == 'D' && str[4] == 'Z',
                                  "Content should be 'XBCDZ'") && result;

        // test 5: set with negative index
        result = D_TEST_CHECK_MSG(_suite,
                                  "set_char_neg",
                                  d_text_buffer_set_char(buffer, -1, '!') == true,
                                  "Set with negative index should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "set_char_neg_verify",
                                  d_text_buffer_get_char(buffer, 4) == '!',
                                  "Last char should now be '!'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_get_string
  Tests the d_text_buffer_get_string function.
  Tests the following:
  - NULL buffer returns NULL
  - empty buffer returns pointer to null terminator
  - non-empty buffer returns pointer to data
  - returned string is null-terminated
*/
bool d_tests_text_buffer_get_string(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    const char*           str;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_get_string");

    result = D_TEST_CHECK_MSG(_suite,
                              "get_string_null",
                              d_text_buffer_get_string(NULL) == NULL,
                              "NULL buffer should return NULL") && result;

    // test 2: empty buffer
    buffer = d_text_buffer_new(16);

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_string_empty",
                                  str != NULL && str[0] == '\0',
                                  "Empty buffer should return empty string") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: non-empty buffer
    buffer = d_text_buffer_new_from_string("Test Data");

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_string_nonempty",
                                  str != NULL && d_strcasecmp(str, "Test Data") == 0,
                                  "Should return 'Test Data'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_get_range_string
  Tests the d_text_buffer_get_range_string function.
  Tests the following:
  - NULL buffer returns NULL
  - out-of-bounds range returns NULL
  - start == end returns empty string (or NULL)
  - valid range returns correct substring
  - returned string is independently allocated and null-terminated
*/
bool d_tests_text_buffer_get_range_string(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    char*                 substr;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_get_range_string");

    result = D_TEST_CHECK_MSG(_suite,
                              "get_range_null",
                              d_text_buffer_get_range_string(NULL, 0, 5) == NULL,
                              "NULL buffer should return NULL") && result;

    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        // test 2: valid range
        substr = d_text_buffer_get_range_string(buffer, 7, 12);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_range_valid",
                                  substr != NULL && d_strcasecmp(substr, "World") == 0,
                                  "Range [7,12) should be 'World'") && result;

        if (substr)
        {
            free(substr);
        }

        // test 3: single character range
        substr = d_text_buffer_get_range_string(buffer, 0, 1);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_range_single",
                                  substr != NULL && substr[0] == 'H' && substr[1] == '\0',
                                  "Range [0,1) should be 'H'") && result;

        if (substr)
        {
            free(substr);
        }

        // test 4: full range
        substr = d_text_buffer_get_range_string(buffer, 0, 13);
        result = D_TEST_CHECK_MSG(_suite,
                                  "get_range_full",
                                  substr != NULL && d_strcasecmp(substr, "Hello, World!") == 0,
                                  "Full range should return complete string") && result;

        if (substr)
        {
            free(substr);
        }

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_access_all
  Aggregation function that runs all access operation tests.
*/
bool d_tests_text_buffer_access_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "text_buffer_access_all");

    return d_tests_text_buffer_get_char(_suite)   &&
           d_tests_text_buffer_set_char(_suite)   &&
           d_tests_text_buffer_get_string(_suite) &&
           d_tests_text_buffer_get_range_string(_suite);
}
