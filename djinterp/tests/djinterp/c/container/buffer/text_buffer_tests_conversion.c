/*******************************************************************************
* djinterp [container]                            text_buffer_tests_conversion.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_conversion.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_to_cstring
  Tests the d_text_buffer_to_cstring function.
  Tests the following:
  - NULL buffer returns NULL
  - returns newly allocated null-terminated string
  - returned string content matches buffer
  - returned string is independent (modifying it does not affect buffer)
  - caller must free returned string
*/
bool d_tests_text_buffer_to_cstring(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    char*                 str;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_to_cstring");

    result = D_TEST_CHECK_MSG(_suite,
                              "to_cstr_null",
                              d_text_buffer_to_cstring(NULL) == NULL,
                              "NULL buffer should return NULL") && result;

    // test 2: normal conversion
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        str = d_text_buffer_to_cstring(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_cstr_not_null",
                                  str != NULL,
                                  "Returned string should not be NULL") && result;

        if (str)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "to_cstr_content",
                                      d_strcasecmp(str, "Hello, World!") == 0,
                                      "Returned string should match buffer content") && result;

            // test 3: independence - modify returned string
            str[0] = 'X';

            result = D_TEST_CHECK_MSG(_suite,
                                      "to_cstr_independent",
                                      buffer->data[0] == 'H',
                                      "Modifying returned string should not affect buffer") && result;

            free(str);
        }

        d_text_buffer_free(buffer);
    }

    // test 4: empty buffer
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        str = d_text_buffer_to_cstring(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_cstr_empty_not_null",
                                  (str != NULL),
                                  "Empty buffer should return non-NULL string") && result;

        if (str)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "to_cstr_empty_content",
                                      (str[0] == '\0'),
                                      "Empty buffer should produce empty string") && result;

            free(str);
        }

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_copy_to_buffer
  Tests the d_text_buffer_copy_to_buffer function.
  Tests the following:
  - NULL source returns false
  - NULL destination returns false
  - zero destination size returns false
  - successful copy with sufficient space
  - destination is null-terminated
  - copy truncates when destination too small
*/
bool d_tests_text_buffer_copy_to_buffer(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    char                  dest[64];
    bool                  result = true;

    // test 1: NULL source
    d_test_suite_test(_suite, "text_buffer_copy_to_buffer");

    d_memset(dest, 0, sizeof(dest));

    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_buf_null_src",
                              d_text_buffer_copy_to_buffer(NULL, dest, sizeof(dest)) == false,
                              "NULL source should return false") && result;

    // test 2: NULL destination
    buffer = d_text_buffer_new_from_string("test");

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_null_dest",
                                  d_text_buffer_copy_to_buffer(buffer, NULL, 64) == false,
                                  "NULL destination should return false") && result;

        // test 3: zero size
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_zero_size",
                                  d_text_buffer_copy_to_buffer(buffer, dest, 0) == false,
                                  "Zero destination size should return false") && result;

        // test 4: successful copy
        d_memset(dest, 0, sizeof(dest));

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_success",
                                  d_text_buffer_copy_to_buffer(buffer, dest, sizeof(dest)) == true,
                                  "Copy should succeed with sufficient space") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_content",
                                  d_strcasecmp(dest, "test") == 0,
                                  "Destination should contain 'test'") && result;

        d_text_buffer_free(buffer);
    }

    // test 5: copy with exact-fit destination
    buffer = d_text_buffer_new_from_string("abc");

    if (buffer)
    {
        d_memset(dest, 'X', sizeof(dest));

        // destination size 4 = 3 chars + null terminator
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_exact",
                                  d_text_buffer_copy_to_buffer(buffer, dest, 4) == true,
                                  "Copy with exact fit should succeed") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_exact_null",
                                  dest[3] == '\0',
                                  "Destination should be null-terminated") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_copy_to_buffer_n
  Tests the d_text_buffer_copy_to_buffer_n function.
  Tests the following:
  - NULL source returns 0
  - NULL destination returns 0
  - copies at most max_chars characters
  - does not exceed destination size
  - returns number of characters actually copied
*/
bool d_tests_text_buffer_copy_to_buffer_n(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    char                  dest[64];
    size_t                copied;
    bool                  result = true;

    // test 1: NULL source
    d_test_suite_test(_suite, "text_buffer_copy_to_buffer_n");

    result = D_TEST_CHECK_MSG(_suite,
                              "copy_to_buf_n_null_src",
                              d_text_buffer_copy_to_buffer_n(NULL, dest, sizeof(dest), 5) == 0,
                              "NULL source should return 0") && result;

    // test 2: copy with max_chars limit
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        d_memset(dest, 0, sizeof(dest));
        copied = d_text_buffer_copy_to_buffer_n(buffer, dest,
                                                  sizeof(dest), 5);

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_n_count",
                                  copied == 5,
                                  "Should copy exactly 5 characters") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_n_content",
                                  d_strncasecmp(dest, "Hello", 5) == 0,
                                  "Destination should contain 'Hello'") && result;

        // test 3: max_chars exceeds buffer length
        d_memset(dest, 0, sizeof(dest));
        copied = d_text_buffer_copy_to_buffer_n(buffer, dest,
                                                  sizeof(dest), 100);

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_n_clamp",
                                  copied == 13,
                                  "Should clamp to buffer length (13)") && result;

        // test 4: destination too small
        d_memset(dest, 0, sizeof(dest));
        copied = d_text_buffer_copy_to_buffer_n(buffer, dest, 4, 10);

        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_to_buf_n_small_dest",
                                  copied <= 3,
                                  "Should not exceed destination size minus null") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_to_d_string
  Tests the d_text_buffer_to_d_string function.
  Tests the following:
  - NULL buffer returns NULL
  - returns a new dj_string with matching content
  - returned dj_string has correct size
  - returned dj_string is independent of original buffer
*/
bool d_tests_text_buffer_to_d_string(
    struct dj_test_suite* _suite
)
{
    struct dj_text_buffer* buffer;
    struct dj_string*      str;
    bool                  result = true;

    // test 1: NULL buffer
    d_test_suite_test(_suite, "text_buffer_to_d_string");

    result = D_TEST_CHECK_MSG(_suite,
                              "to_dstr_null",
                              d_text_buffer_to_d_string(NULL) == NULL,
                              "NULL buffer should return NULL") && result;

    // test 2: normal conversion
    buffer = d_text_buffer_new_from_string("Hello, World!");

    if (buffer)
    {
        str = d_text_buffer_to_d_string(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_dstr_not_null",
                                  str != NULL,
                                  "Returned dj_string should not be NULL") && result;

        if (str)
        {
            // test 3: content match
            result = D_TEST_CHECK_MSG(_suite,
                                      "to_dstr_content",
                                      str->text != NULL && d_strcasecmp(str->text, "Hello, World!") == 0,
                                      "dj_string text should match buffer content") && result;

            // test 4: size match
            result = D_TEST_CHECK_MSG(_suite,
                                      "to_dstr_size",
                                      str->size == 13,
                                      "dj_string size should be 13") && result;

            // NOTE: already know str->text is non-NULL due to previous assert
            // gets rid of persistent compiler warning
            if (str->text)
            {
                // test 5: independence
                str->text[0] = 'X';

                result = D_TEST_CHECK_MSG(_suite,
                                          "to_dstr_independent",
                                          buffer->data[0] == 'H',
                                          "Modifying dj_string should not affect buffer") && result;
            }

            d_string_free(str);
        }

        d_text_buffer_free(buffer);
    }

    // test 6: empty buffer conversion
    buffer = d_text_buffer_new_default_capacity();

    if (buffer)
    {
        str = d_text_buffer_to_d_string(buffer);

        result = D_TEST_CHECK_MSG(_suite,
                                  "to_dstr_empty_not_null",
                                  (str != NULL),
                                  "Empty buffer should produce non-NULL dj_string") && result;

        if (str)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "to_dstr_empty_size",
                                      (str->size == 0),
                                      "Empty buffer dj_string size should be 0") && result;

            d_string_free(str);
        }

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_conversion_all
  Aggregation function that runs all conversion tests.
*/
bool d_tests_text_buffer_conversion_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "text_buffer_conversion_all");

    return d_tests_text_buffer_to_cstring(_suite)       &&
           d_tests_text_buffer_copy_to_buffer(_suite)   &&
           d_tests_text_buffer_copy_to_buffer_n(_suite) &&
           d_tests_text_buffer_to_d_string(_suite);
}
