/*******************************************************************************
* djinterp [container]                              text_buffer_tests_creation.c
*
*
* path:      /tests/djinterp/c/container/buffer/text_buffer_tests_creation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_buffer_tests.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/*
d_tests_text_buffer_new
  Tests the d_text_buffer_new function.
  Tests the following:
  - zero capacity creates a valid buffer
  - small capacity creates a buffer with correct capacity
  - large capacity creates a buffer with correct capacity
  - returned buffer has zero count
  - returned buffer data is non-NULL and null-terminated
*/
bool d_tests_text_buffer_new(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_new");

    result = true;

    // test 1: create with zero capacity
    buffer = d_text_buffer_new(0);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_cap",
                              buffer != NULL,
                              "Zero capacity should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_zero_cap_count",
                                  d_text_buffer_length(buffer) == 0,
                                  "New buffer should have zero length") && result;

        d_text_buffer_free(buffer);
    }

    // test 2: create with small capacity
    buffer = d_text_buffer_new(16);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_small_cap",
                              buffer != NULL,
                              "Small capacity should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_small_cap_value",
                                  d_text_buffer_capacity(buffer) >= 16,
                                  "Capacity should be at least 16") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_small_cap_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "New buffer should have zero length") && result;

        // verify null-termination
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_small_cap_null_term",
                                  d_text_buffer_get_string(buffer) != NULL && d_text_buffer_get_string(buffer)[0] == '\0',
                                  "New buffer data should be null-terminated") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: create with large capacity
    buffer = d_text_buffer_new(4096);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_large_cap",
                              buffer != NULL,
                              "Large capacity should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_large_cap_value",
                                  d_text_buffer_capacity(buffer) >= 4096,
                                  "Capacity should be at least 4096") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_new_default_capacity
  Tests the d_text_buffer_new_default_capacity function.
  Tests the following:
  - returned buffer is non-NULL
  - capacity is at least D_BUFFER_DEFAULT_CAPACITY
  - length is zero
  - data is null-terminated
*/
bool d_tests_text_buffer_new_default_capacity(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;

    d_test_suite_test(_suite, "text_buffer_new_default_capacity");

    result = true;

    buffer = d_text_buffer_new_default_capacity();

    // test 1: non-NULL result
    result = D_TEST_CHECK_MSG(_suite,
                              "new_default_nonnull",
                              buffer != NULL,
                              "Default capacity should return non-NULL") && result;

    if (buffer)
    {
        // test 2: capacity
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_cap",
                                  d_text_buffer_capacity(buffer) >= D_BUFFER_DEFAULT_CAPACITY,
                                  "Capacity should be at least D_BUFFER_DEFAULT_CAPACITY") && result;

        // test 3: empty
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "Default buffer should have zero length") && result;

        // test 4: null-terminated
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_default_null_term",
                                  d_text_buffer_get_string(buffer)[0] == '\0',
                                  "Default buffer should be null-terminated") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_new_from_string
  Tests the d_text_buffer_new_from_string function.
  Tests the following:
  - NULL string returns NULL
  - empty string creates empty buffer
  - normal string is copied correctly
  - buffer is null-terminated
  - length matches original string
*/
bool d_tests_text_buffer_new_from_string(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_new_from_string");

    result = true;

    // test 1: NULL string -> returns empty buffer (not NULL)
    buffer = d_text_buffer_new_from_string(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_string_null",
                              buffer != NULL,
                              "NULL string should return empty buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_null_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "NULL string buffer should have zero length") && result;

        d_text_buffer_free(buffer);
    }

    // test 2: empty string
    buffer = d_text_buffer_new_from_string("");
    result = D_TEST_CHECK_MSG(_suite,
                              "from_string_empty",
                              buffer != NULL,
                              "Empty string should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_empty_len",
                                  d_text_buffer_length(buffer) == 0,
                                  "Empty string buffer should have zero length") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: normal string
    buffer = d_text_buffer_new_from_string("Hello, World!");
    result = D_TEST_CHECK_MSG(_suite,
                              "from_string_normal",
                              buffer != NULL,
                              "Normal string should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_normal_len",
                                  d_text_buffer_length(buffer) == 13,
                                  "Length should be 13") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_normal_content",
                                  str != NULL && d_strncasecmp(str, "Hello, World!", 13) == 0,
                                  "Content should match 'Hello, World!'") && result;

        // test 4: null-termination
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_null_term",
                                  str[13] == '\0',
                                  "Buffer data should be null-terminated") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_new_from_string_n
  Tests the d_text_buffer_new_from_string_n function.
  Tests the following:
  - NULL string returns NULL
  - zero length creates empty buffer
  - partial copy of string with correct length
  - content matches first _length characters
*/
bool d_tests_text_buffer_new_from_string_n(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_new_from_string_n");

    result = true;

    // test 1: NULL string -> returns empty buffer (not NULL)
    buffer = d_text_buffer_new_from_string_n(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_string_n_null",
                              buffer != NULL,
                              "NULL string should return empty buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_n_null_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "NULL string buffer should have zero length") && result;

        d_text_buffer_free(buffer);
    }

    // test 2: zero length
    buffer = d_text_buffer_new_from_string_n("Hello", 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_string_n_zero",
                              buffer != NULL,
                              "Zero length should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_n_zero_len",
                                  d_text_buffer_length(buffer) == 0,
                                  "Zero length buffer should have zero length") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: partial copy
    buffer = d_text_buffer_new_from_string_n("Hello, World!", 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_string_n_partial",
                              buffer != NULL,
                              "Partial copy should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_n_partial_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should be 5") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_string_n_partial_content",
                                  str != NULL && d_strncasecmp(str, "Hello", 5) == 0 && str[5] == '\0',
                                  "Content should be 'Hello' and null-terminated") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_new_from_strings
  Tests the d_text_buffer_new_from_strings function.
  Tests the following:
  - zero count returns valid empty buffer
  - single string concatenation
  - multiple string concatenation
  - result is null-terminated with correct total length
*/
bool d_tests_text_buffer_new_from_strings(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_new_from_strings");

    result = true;

    // test 1: single string
    buffer = d_text_buffer_new_from_strings(1, "Hello");
    result = D_TEST_CHECK_MSG(_suite,
                              "from_strings_single",
                              buffer != NULL,
                              "Single string should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_strings_single_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Single string length should be 5") && result;

        d_text_buffer_free(buffer);
    }

    // test 2: multiple strings
    buffer = d_text_buffer_new_from_strings(3, "Hello", ", ", "World!");
    result = D_TEST_CHECK_MSG(_suite,
                              "from_strings_multi",
                              buffer != NULL,
                              "Multiple strings should return non-NULL buffer") && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_strings_multi_len",
                                  d_text_buffer_length(buffer) == 13,
                                  "Concatenated length should be 13") && result;

        result = D_TEST_CHECK_MSG(_suite,
                                  "from_strings_multi_content",
                                  str != NULL && d_strncasecmp(str, "Hello, World!", 13) == 0,
                                  "Content should be 'Hello, World!'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_new_from_buffer
  Tests the d_text_buffer_new_from_buffer function.
  Tests the following:
  - NULL buffer returns NULL
  - zero length creates empty buffer
  - data with embedded nulls copies correct number of bytes
  - result is null-terminated
*/
bool d_tests_text_buffer_new_from_buffer(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    char                  raw_data[8];
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_new_from_buffer");

    result = true;

    // test 1: NULL buffer -> returns empty buffer (not NULL)
    buffer = d_text_buffer_new_from_buffer(NULL, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_buffer_null",
                              buffer != NULL,
                              "NULL buffer data should return empty buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_buffer_null_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "NULL buffer data should have zero length") && result;

        d_text_buffer_free(buffer);
    }

    // test 2: zero length
    buffer = d_text_buffer_new_from_buffer("data", 0);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_buffer_zero_len",
                              buffer != NULL,
                              "Zero length should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_buffer_zero_len_empty",
                                  d_text_buffer_length(buffer) == 0,
                                  "Zero length buffer should have zero length") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: raw data copy
    d_memset(raw_data, 0, sizeof(raw_data));
    raw_data[0] = 'A';
    raw_data[1] = 'B';
    raw_data[2] = '\0';
    raw_data[3] = 'C';
    raw_data[4] = 'D';

    buffer = d_text_buffer_new_from_buffer(raw_data, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "from_buffer_raw",
                              buffer != NULL,
                              "Raw data copy should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_buffer_raw_len",
                                  d_text_buffer_length(buffer) == 5,
                                  "Length should be 5 (including embedded null)") && result;

        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "from_buffer_raw_content",
                                  str != NULL && str[0] == 'A' && str[1] == 'B' && str[3] == 'C' && str[4] == 'D',
                                  "Raw data content should match") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_new_copy
  Tests the d_text_buffer_new_copy function.
  Tests the following:
  - NULL other returns NULL
  - successful deep copy of non-empty buffer
  - copied buffer is independent (modifying copy does not affect original)
  - lengths and contents match
*/
bool d_tests_text_buffer_new_copy(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* original;
    struct dj_text_buffer* copy;
    const char*           orig_str;
    const char*           copy_str;

    d_test_suite_test(_suite, "text_buffer_new_copy");

    result = true;

    // test 1: NULL other
    copy = d_text_buffer_new_copy(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "new_copy_null",
                              copy == NULL,
                              "NULL other should return NULL") && result;

    // test 2: successful deep copy
    original = d_text_buffer_new_from_string("Test String");

    if (original)
    {
        copy = d_text_buffer_new_copy(original);
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_success",
                                  copy != NULL,
                                  "Copy should return non-NULL buffer") && result;

        if (copy)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_length",
                                      d_text_buffer_length(copy) == d_text_buffer_length(original),
                                      "Copy length should match original") && result;

            orig_str = d_text_buffer_get_string(original);
            copy_str = d_text_buffer_get_string(copy);
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_independent_ptrs",
                                      orig_str != copy_str,
                                      "Copy data pointer should differ from original") && result;

            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_content",
                                      d_strcasecmp(orig_str, copy_str) == 0,
                                      "Copy content should match original") && result;

            // test 3: independence — modify copy
            d_text_buffer_set_char(copy, 0, 'X');
            result = D_TEST_CHECK_MSG(_suite,
                                      "new_copy_independent",
                                      d_text_buffer_get_char(original, 0) == 'T',
                                      "Modifying copy should not affect original") && result;

            d_text_buffer_free(copy);
        }

        d_text_buffer_free(original);
    }

    return result;
}

/*
d_tests_text_buffer_new_copy_range
  Tests the d_text_buffer_new_copy_range function.
  Tests the following:
  - NULL other returns NULL
  - out-of-bounds range returns NULL
  - start > end returns NULL
  - valid range extracts correct substring
  - result is null-terminated
*/
bool d_tests_text_buffer_new_copy_range(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* original;
    struct dj_text_buffer* sub;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_new_copy_range");

    result = true;

    // test 1: NULL other
    sub = d_text_buffer_new_copy_range(NULL, 0, 5);
    result = D_TEST_CHECK_MSG(_suite,
                              "copy_range_null",
                              sub == NULL,
                              "NULL other should return NULL") && result;

    original = d_text_buffer_new_from_string("Hello, World!");

    if (original)
    {
        // test 2: valid range [7, 12) => "World"
        sub = d_text_buffer_new_copy_range(original, 7, 12);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_range_valid",
                                  sub != NULL,
                                  "Valid range should return non-NULL buffer") && result;

        if (sub)
        {
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_range_len",
                                      d_text_buffer_length(sub) == 5,
                                      "Range length should be 5") && result;

            str = d_text_buffer_get_string(sub);
            result = D_TEST_CHECK_MSG(_suite,
                                      "copy_range_content",
                                      str != NULL && d_strncasecmp(str, "World", 5) == 0,
                                      "Content should be 'World'") && result;

            d_text_buffer_free(sub);
        }

        // test 3: start == end => returns NULL (empty range)
        sub = d_text_buffer_new_copy_range(original, 3, 3);
        result = D_TEST_CHECK_MSG(_suite,
                                  "copy_range_empty",
                                  sub == NULL,
                                  "start == end should return NULL") && result;

        if (sub)
        {
            d_text_buffer_free(sub);
        }

        d_text_buffer_free(original);
    }

    return result;
}

/*
d_tests_text_buffer_new_fill
  Tests the d_text_buffer_new_fill function.
  Tests the following:
  - zero length creates empty buffer
  - fill creates buffer with correct length and character
  - result is null-terminated
*/
bool d_tests_text_buffer_new_fill(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;
    size_t                i;
    bool                  all_match;

    d_test_suite_test(_suite, "text_buffer_new_fill");

    result = true;

    // test 1: zero length
    buffer = d_text_buffer_new_fill(0, 'X');
    result = D_TEST_CHECK_MSG(_suite,
                              "fill_zero",
                              buffer != NULL && d_text_buffer_length(buffer) == 0,
                              "Zero length fill should create empty buffer") && result;

    if (buffer)
    {
        d_text_buffer_free(buffer);
    }

    // test 2: fill with character
    buffer = d_text_buffer_new_fill(10, 'A');
    result = D_TEST_CHECK_MSG(_suite,
                              "fill_normal",
                              buffer != NULL,
                              "Fill should return non-NULL buffer") && result;

    if (buffer)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_normal_len",
                                  d_text_buffer_length(buffer) == 10,
                                  "Fill length should be 10") && result;

        str = d_text_buffer_get_string(buffer);
        all_match = true;
        for (i = 0; i < 10; i++)
        {
            if (str[i] != 'A')
            {
                all_match = false;
            }
        }

        result = D_TEST_CHECK_MSG(_suite,
                                  "fill_normal_content",
                                  all_match && str[10] == '\0',
                                  "All 10 chars should be 'A' and null-terminated") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_new_formatted
  Tests the d_text_buffer_new_formatted function.
  Tests the following:
  - NULL format returns NULL
  - simple format string
  - format with integer substitution
  - format with multiple arguments
  - result is null-terminated with correct length
*/
bool d_tests_text_buffer_new_formatted(
    struct dj_test_suite* _suite
)
{
    bool                  result;
    struct dj_text_buffer* buffer;
    const char*           str;

    d_test_suite_test(_suite, "text_buffer_new_formatted");

    result = true;

    // test 1: NULL format
    buffer = d_text_buffer_new_formatted(NULL);
    result = D_TEST_CHECK_MSG(_suite,
                              "formatted_null",
                              buffer == NULL,
                              "NULL format should return NULL") && result;

    // test 2: simple format
    buffer = d_text_buffer_new_formatted("Hello");
    result = D_TEST_CHECK_MSG(_suite,
                              "formatted_simple",
                              buffer != NULL,
                              "Simple format should return non-NULL buffer") && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "formatted_simple_content",
                                  d_text_buffer_length(buffer) == 5 && d_strcasecmp(str, "Hello") == 0,
                                  "Content should be 'Hello'") && result;

        d_text_buffer_free(buffer);
    }

    // test 3: format with integer
    buffer = d_text_buffer_new_formatted("Value: %d", 42);
    result = D_TEST_CHECK_MSG(_suite,
                              "formatted_int",
                              buffer != NULL,
                              "Integer format should return non-NULL buffer") && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "formatted_int_content",
                                  d_strcasecmp(str, "Value: 42") == 0,
                                  "Content should be 'Value: 42'") && result;

        d_text_buffer_free(buffer);
    }

    // test 4: format with multiple arguments
    buffer = d_text_buffer_new_formatted("%s=%d, %s=%d",
                                      "x", 10, "y", 20);
    result = D_TEST_CHECK_MSG(_suite,
                              "formatted_multi",
                              buffer != NULL,
                              "Multi-arg format should return non-NULL buffer") && result;

    if (buffer)
    {
        str = d_text_buffer_get_string(buffer);
        result = D_TEST_CHECK_MSG(_suite,
                                  "formatted_multi_content",
                                  d_strcasecmp(str, "x=10, y=20") == 0,
                                  "Content should be 'x=10, y=20'") && result;

        d_text_buffer_free(buffer);
    }

    return result;
}

/*
d_tests_text_buffer_creation_all
  Aggregation function that runs all creation tests.
*/
bool d_tests_text_buffer_creation_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "text_buffer_creation_all");

    result = true;

    result = d_tests_text_buffer_new(_suite) && result;
    result = d_tests_text_buffer_new_default_capacity(_suite)
             && result;
    result = d_tests_text_buffer_new_from_string(_suite) && result;
    result = d_tests_text_buffer_new_from_string_n(_suite) && result;
    result = d_tests_text_buffer_new_from_strings(_suite) && result;
    result = d_tests_text_buffer_new_from_buffer(_suite) && result;
    result = d_tests_text_buffer_new_copy(_suite) && result;
    result = d_tests_text_buffer_new_copy_range(_suite) && result;
    result = d_tests_text_buffer_new_fill(_suite) && result;
    result = d_tests_text_buffer_new_formatted(_suite) && result;

    return result;
}
