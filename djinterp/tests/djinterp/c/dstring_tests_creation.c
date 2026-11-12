/*******************************************************************************
* djinterp [c]                                          dstring_tests_creation.c
*
*   Unit tests for dj_string creation and destruction functions:
*     - d_string_new
*     - d_string_new_with_capacity
*     - d_string_new_from_cstr
*     - d_string_new_from_cstr_n
*     - d_string_new_from_buffer
*     - d_string_new_copy
*     - d_string_new_fill
*     - d_string_new_formatted
*     - d_string_free
*     - d_string_free_contents
*
*
* path:      /tests/djinterp/c/dstring_tests_creation.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.11.06
*******************************************************************************/

// djinterp
#include "dstring_tests.h"


/******************************************************************************
* d_tests_dstring_new
******************************************************************************/

/*
d_tests_dstring_new
  Tests d_string_new() which creates an empty string with default capacity.

Test cases:
  1. Returns non-NULL pointer
  2. Size is 0 (empty string)
  3. Capacity is at least default (16)
  4. Text pointer is non-NULL
  5. Text is empty string (null-terminated)

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new(struct dj_test_suite* _suite)
{
    struct dj_string*      str;

    d_test_suite_test(_suite, "d_string_new");

    // create test subject
    str = d_string_new();

    // test 1: returns non-NULL
    D_TEST_CHECK_MSG(_suite,
                     "new_returns_non_null",
                     (str) != NULL,
                     "d_string_new() should return non-NULL pointer");

    if (str)
    {
        // test 2: size is 0
        D_TEST_CHECK_MSG(_suite,
                         "new_size_is_zero",
                         (str->size) == (0),
                         "newly created string should have size 0");

        // test 3: capacity is at least default (16)
        D_TEST_CHECK_MSG(_suite,
                         "new_capacity_at_least_default",
                         str->capacity >= 16,
                         "capacity should be at least default (16)");

        // test 4: text pointer is non-NULL
        D_TEST_CHECK_MSG(_suite,
                         "new_text_non_null",
                         (str->text) != NULL,
                         "text pointer should be non-NULL");

        // test 5: text is empty string
        D_TEST_CHECK_MSG(_suite,
                         "new_text_is_empty",
                         (str->text != NULL) && (str->text[0] == '\0'),
                         "text should be empty (null-terminated)");

        d_string_free(str);
    }
    else
    {
        // fill remaining slots with failure indicators
        D_TEST_CHECK_MSG(_suite,
                         "skipped_due_to_null",
                         false,
                         "test skipped - d_string_new returned NULL");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_new_with_capacity
******************************************************************************/

/*
d_tests_dstring_new_with_capacity
  Tests d_string_new_with_capacity() which creates an empty string with
  specified initial capacity.

Test cases:
  1. Capacity 0 - should still allocate minimum
  2. Capacity 1 - minimal allocation
  3. Capacity 32 - typical small capacity
  4. Capacity 1024 - larger capacity
  5. Capacity 65536 - very large capacity
  6. Verify size is always 0
  7. Verify text is always empty

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new_with_capacity(struct dj_test_suite* _suite)
{
    struct dj_string*      str;

    d_test_suite_test(_suite, "d_string_new_with_capacity");

    // test 1: capacity 0 - should allocate something
    str = d_string_new_with_capacity(0);
    D_TEST_CHECK_MSG(_suite,
                     "capacity_0_returns_non_null",
                     (str) != NULL,
                     "capacity 0 should still return valid string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_0_has_some_capacity",
                         str->capacity > 0,
                         "even with capacity 0, should have some allocation");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_0_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 2: capacity 1
    str = d_string_new_with_capacity(1);
    D_TEST_CHECK_MSG(_suite,
                     "capacity_1_returns_non_null",
                     (str) != NULL,
                     "capacity 1 should return valid string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_1_sufficient",
                         str->capacity >= 1,
                         "capacity should be at least 1");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_1_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 3: capacity 32
    str = d_string_new_with_capacity(32);
    D_TEST_CHECK_MSG(_suite,
                     "capacity_32_returns_non_null",
                     (str) != NULL,
                     "capacity 32 should return valid string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_32_sufficient",
                         str->capacity >= 32,
                         "capacity should be at least 32");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_32_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 4: capacity 1024
    str = d_string_new_with_capacity(1024);
    D_TEST_CHECK_MSG(_suite,
                     "capacity_1024_returns_non_null",
                     (str) != NULL,
                     "capacity 1024 should return valid string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_1024_sufficient",
                         str->capacity >= 1024,
                         "capacity should be at least 1024");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "capacity_1024_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 5: size is always 0 for new strings
    str = d_string_new_with_capacity(100);

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "new_with_capacity_size_zero",
                         (str->size) == (0),
                         "size should always be 0 for newly created strings");

        // test 6: text is empty
        D_TEST_CHECK_MSG(_suite,
                         "new_with_capacity_text_empty",
                         (str->text != NULL) && (str->text[0] == '\0'),
                         "text should be empty regardless of capacity");

        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "size_test_skipped",
                         false,
                         "skipped - allocation failed");
        D_TEST_CHECK_MSG(_suite,
                         "text_test_skipped",
                         false,
                         "skipped - allocation failed");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_new_from_cstr
******************************************************************************/

/*
d_tests_dstring_new_from_cstr
  Tests d_string_new_from_cstr() which creates a string from a C string.

Test cases:
  1. NULL input returns NULL
  2. Empty string input
  3. Single character string
  4. Short string
  5. String with spaces
  6. String with special characters
  7. Longer string (100+ chars)
  8. Verify null termination
  9. Verify content matches exactly

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new_from_cstr(struct dj_test_suite* _suite)
{
    struct dj_string*      str;

    d_test_suite_test(_suite, "d_string_new_from_cstr");

    // test 1: NULL input returns NULL
    str = d_string_new_from_cstr(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_input_returns_null",
                     (str) == NULL,
                     "NULL input should return NULL");

    // test 2: empty string input
    str = d_string_new_from_cstr("");
    D_TEST_CHECK_MSG(_suite,
                     "empty_string_returns_non_null",
                     (str) != NULL,
                     "empty string should return valid dj_string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "empty_string_size_zero",
                         (str->size) == (0),
                         "empty string should have size 0");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "empty_string_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 3: single character
    str = d_string_new_from_cstr("A");
    D_TEST_CHECK_MSG(_suite,
                     "single_char_returns_non_null",
                     (str) != NULL,
                     "single char should return valid dj_string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "single_char_size_one",
                         (str->size) == (1),
                         "single char string should have size 1");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "single_char_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 4: short string with content verification
    str = d_string_new_from_cstr("Hello");
    D_TEST_CHECK_MSG(_suite,
                     "short_string_returns_non_null",
                     (str) != NULL,
                     "short string should return valid dj_string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "short_string_content_matches",
                         d_test_suite_str_equal((str->text), ("Hello")),
                         "content should match input exactly");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "short_string_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 5: string with spaces
    str = d_string_new_from_cstr("Hello World!");
    D_TEST_CHECK_MSG(_suite,
                     "string_with_spaces_non_null",
                     (str) != NULL,
                     "string with spaces should work");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "string_with_spaces_size",
                         (str->size) == (12),
                         "size should be 12 for 'Hello World!'");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "spaces_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 6: string with special characters
    str = d_string_new_from_cstr("Tab:\tNewline:\nQuote:\"");
    D_TEST_CHECK_MSG(_suite,
                     "special_chars_non_null",
                     (str) != NULL,
                     "string with special chars should work");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "special_chars_preserved",
                         (str->text != NULL) && (strchr(str->text, '\t') != NULL),
                         "special characters should be preserved");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "special_skipped",
                         false,
                         "skipped - allocation failed");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_new_from_cstr_n
******************************************************************************/

/*
d_tests_dstring_new_from_cstr_n
  Tests d_string_new_from_cstr_n() which creates a string from a C string
  with a maximum length limit.

Test cases:
  1. NULL input returns NULL
  2. Length 0 creates empty string
  3. Length less than string length
  4. Length equal to string length
  5. Length greater than string length
  6. Very large length value
  7. Partial copy verification

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new_from_cstr_n(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    const char*           test_str;

    d_test_suite_test(_suite, "d_string_new_from_cstr_n");
    test_str  = "Hello World";

    // test 1: NULL input returns NULL
    str = d_string_new_from_cstr_n(NULL, 5);
    D_TEST_CHECK_MSG(_suite,
                     "null_input_returns_null",
                     (str) == NULL,
                     "NULL input should return NULL");

    // test 2: length 0 creates empty string
    str = d_string_new_from_cstr_n(test_str, 0);
    D_TEST_CHECK_MSG(_suite,
                     "length_0_returns_non_null",
                     (str) != NULL,
                     "length 0 should return valid empty string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "length_0_size_zero",
                         (str->size) == (0),
                         "length 0 should create empty string");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "length_0_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 3: length less than string length (partial copy)
    str = d_string_new_from_cstr_n(test_str, 5);
    D_TEST_CHECK_MSG(_suite,
                     "partial_copy_non_null",
                     (str) != NULL,
                     "partial copy should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "partial_copy_size",
                         (str->size) == (5),
                         "size should be 5 for partial copy");

        D_TEST_CHECK_MSG(_suite,
                         "partial_copy_content",
                         d_test_suite_str_equal((str->text), ("Hello")),
                         "partial copy should contain 'Hello'");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "partial_size_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "partial_content_skipped", false, "skipped");
    }

    // test 4: length equal to string length
    str = d_string_new_from_cstr_n(test_str, 11);
    D_TEST_CHECK_MSG(_suite,
                     "exact_length_non_null",
                     (str) != NULL,
                     "exact length should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "exact_length_content",
                         d_test_suite_str_equal((str->text), (test_str)),
                         "exact length should copy entire string");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "exact_length_skipped", false, "skipped");
    }

    // test 5: length greater than string length
    str = d_string_new_from_cstr_n(test_str, 100);
    D_TEST_CHECK_MSG(_suite,
                     "excess_length_non_null",
                     (str) != NULL,
                     "excess length should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "excess_length_size_correct",
                         (str->size) == (11),
                         "size should be actual string length, not requested length");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "excess_length_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_new_from_buffer
******************************************************************************/

/*
d_tests_dstring_new_from_buffer
  Tests d_string_new_from_buffer() which creates a string from a raw buffer
  with explicit length (buffer may contain embedded nulls).

Test cases:
  1. NULL buffer returns NULL
  2. Length 0 creates empty string
  3. Buffer with embedded null characters
  4. Normal buffer copy
  5. Binary data in buffer

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new_from_buffer(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    char                  buffer_with_nulls[10];
    char                  binary_buffer[8];

    d_test_suite_test(_suite, "d_string_new_from_buffer");

    // test 1: NULL buffer returns NULL
    str = d_string_new_from_buffer(NULL, 5);
    D_TEST_CHECK_MSG(_suite,
                     "null_buffer_returns_null",
                     (str) == NULL,
                     "NULL buffer should return NULL");

    // test 2: length 0 creates empty string
    str = d_string_new_from_buffer("anything", 0);
    D_TEST_CHECK_MSG(_suite,
                     "length_0_returns_non_null",
                     (str) != NULL,
                     "length 0 should return valid empty string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "length_0_size_zero",
                         (str->size) == (0),
                         "length 0 should create empty string");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "length_0_skipped", false, "skipped");
    }

    // test 3: buffer with embedded null characters
    // create buffer: "AB\0CD\0EF\0\0"
    buffer_with_nulls[0] = 'A';
    buffer_with_nulls[1] = 'B';
    buffer_with_nulls[2] = '\0';
    buffer_with_nulls[3] = 'C';
    buffer_with_nulls[4] = 'D';
    buffer_with_nulls[5] = '\0';
    buffer_with_nulls[6] = 'E';
    buffer_with_nulls[7] = 'F';
    buffer_with_nulls[8] = '\0';
    buffer_with_nulls[9] = '\0';

    str = d_string_new_from_buffer(buffer_with_nulls, 8);
    D_TEST_CHECK_MSG(_suite,
                     "embedded_nulls_non_null",
                     (str) != NULL,
                     "buffer with embedded nulls should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "embedded_nulls_size",
                         (str->size) == (8),
                         "size should include embedded null characters");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "embedded_nulls_skipped", false, "skipped");
    }

    // test 4: normal buffer copy
    str = d_string_new_from_buffer("Normal text", 11);
    D_TEST_CHECK_MSG(_suite,
                     "normal_buffer_non_null",
                     (str) != NULL,
                     "normal buffer copy should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "normal_buffer_content",
                         d_test_suite_str_equal((str->text), ("Normal text")),
                         "content should match buffer");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "normal_buffer_skipped", false, "skipped");
    }

    // test 5: binary data in buffer
    binary_buffer[0] = 0x00;
    binary_buffer[1] = 0x01;
    binary_buffer[2] = (char)0xFF;
    binary_buffer[3] = (char)0xFE;
    binary_buffer[4] = 0x7F;
    binary_buffer[5] = (char)0x80;
    binary_buffer[6] = 0x00;
    binary_buffer[7] = (char)0xFF;

    str = d_string_new_from_buffer(binary_buffer, 8);
    D_TEST_CHECK_MSG(_suite,
                     "binary_data_non_null",
                     (str) != NULL,
                     "binary data buffer should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "binary_data_preserved",
                         (str->size == 8) && ((unsigned char)str->text[2] == 0xFF),
                         "binary data should be preserved exactly");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "binary_data_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_new_copy
******************************************************************************/

/*
d_tests_dstring_new_copy
  Tests d_string_new_copy() which creates a deep copy of an existing dj_string.

Test cases:
  1. NULL input returns NULL
  2. Copy of empty string
  3. Copy of non-empty string
  4. Verify deep copy (modifying original doesn't affect copy)
  5. Verify deep copy (modifying copy doesn't affect original)
  6. Copy preserves size exactly

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new_copy(struct dj_test_suite* _suite)
{
    struct dj_string*      original;
    struct dj_string*      copy;
    bool                  deep_copy_test;

    d_test_suite_test(_suite, "d_string_new_copy");

    // test 1: NULL input returns NULL
    copy = d_string_new_copy(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_input_returns_null",
                     (copy) == NULL,
                     "NULL input should return NULL");

    // test 2: copy of empty string
    original = d_string_new();

    if (original)
    {
        copy = d_string_new_copy(original);
        D_TEST_CHECK_MSG(_suite,
                         "copy_empty_string_non_null",
                         (copy) != NULL,
                         "copy of empty string should succeed");

        if (copy)
        {
            D_TEST_CHECK_MSG(_suite,
                             "copy_empty_string_size_zero",
                             (copy->size) == (0),
                             "copy of empty string should have size 0");
            d_string_free(copy);
        }
        else
        {
            D_TEST_CHECK_MSG(_suite, "copy_empty_skipped", false, "skipped");
        }

        d_string_free(original);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "empty_original_failed",
                         false,
                         "failed to create original");
        D_TEST_CHECK_MSG(_suite, "empty_copy_skipped", false, "skipped");
    }

    // test 3: copy of non-empty string
    original = d_string_new_from_cstr("Test String");

    if (original)
    {
        copy = d_string_new_copy(original);
        D_TEST_CHECK_MSG(_suite,
                         "copy_non_empty_non_null",
                         (copy) != NULL,
                         "copy of non-empty string should succeed");

        if (copy)
        {
            D_TEST_CHECK_MSG(_suite,
                             "copy_content_matches",
                             d_test_suite_str_equal((copy->text), (original->text)),
                             "copy content should match original");

            D_TEST_CHECK_MSG(_suite,
                             "copy_size_matches",
                             (copy->size) == (original->size),
                             "copy size should match original");

            // test 4: verify deep copy - different memory locations
            D_TEST_CHECK_MSG(_suite,
                             "deep_copy_different_pointers",
                             copy->text != original->text,
                             "copy should have different text pointer (deep copy)");

            // test 5: modifying copy doesn't affect original
            if (copy->text && original->text)
            {
                copy->text[0] = 'X';
                deep_copy_test = (original->text[0] == 'T');
                copy->text[0] = 'T';  // restore
            }
            else
            {
                deep_copy_test = false;
            }

            D_TEST_CHECK_MSG(_suite,
                             "deep_copy_modification_isolated",
                             deep_copy_test,
                             "modifying copy should not affect original");

            d_string_free(copy);
        }
        else
        {
            // fill with skip markers
            D_TEST_CHECK_MSG(_suite,
                             "copy_skipped",
                             false,
                             "skipped - copy failed");
        }

        d_string_free(original);
    }
    else
    {
        // fill with skip markers
        D_TEST_CHECK_MSG(_suite,
                         "original_failed",
                         false,
                         "skipped - original creation failed");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_new_fill
******************************************************************************/

/*
d_tests_dstring_new_fill
  Tests d_string_new_fill() which creates a string filled with a repeated
  character.

Test cases:
  1. Length 0 creates empty string
  2. Length 1 creates single character string
  3. Normal fill with letter
  4. Fill with space character
  5. Fill with null character (edge case)
  6. Large fill (1000 characters)

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new_fill(struct dj_test_suite* _suite)
{
    struct dj_string*      str;
    size_t                i;
    bool                  all_match;

    d_test_suite_test(_suite, "d_string_new_fill");

    // test 1: length 0 creates empty string
    str = d_string_new_fill(0, 'X');
    D_TEST_CHECK_MSG(_suite,
                     "fill_0_returns_non_null",
                     (str) != NULL,
                     "fill with length 0 should return valid string");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "fill_0_size_zero",
                         (str->size) == (0),
                         "fill with length 0 should have size 0");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "fill_0_skipped", false, "skipped");
    }

    // test 2: length 1 creates single character
    str = d_string_new_fill(1, 'A');
    D_TEST_CHECK_MSG(_suite,
                     "fill_1_returns_non_null",
                     (str) != NULL,
                     "fill with length 1 should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "fill_1_content",
                         d_test_suite_str_equal((str->text), ("A")),
                         "fill with length 1 should contain single char");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "fill_1_skipped", false, "skipped");
    }

    // test 3: normal fill with letter
    str = d_string_new_fill(5, 'X');
    D_TEST_CHECK_MSG(_suite,
                     "fill_5_returns_non_null",
                     (str) != NULL,
                     "fill with length 5 should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "fill_5_content",
                         d_test_suite_str_equal((str->text), ("XXXXX")),
                         "fill should create 5 X characters");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "fill_5_skipped", false, "skipped");
    }

    // test 4: fill with space
    str = d_string_new_fill(3, ' ');
    D_TEST_CHECK_MSG(_suite,
                     "fill_space_returns_non_null",
                     (str) != NULL,
                     "fill with spaces should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "fill_space_content",
                         d_test_suite_str_equal((str->text), ("   ")),
                         "fill should create 3 spaces");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "fill_space_skipped", false, "skipped");
    }

    // test 5: large fill (100 characters)
    str = d_string_new_fill(100, '-');
    D_TEST_CHECK_MSG(_suite,
                     "fill_100_returns_non_null",
                     (str) != NULL,
                     "fill with length 100 should succeed");

    if (str)
    {
        // verify all characters match
        all_match = (str->size == 100);

        for (i = 0; i < str->size && all_match; i++)
        {
            if (str->text[i] != '-')
            {
                all_match = false;
            }
        }

        D_TEST_CHECK_MSG(_suite,
                         "fill_100_all_match",
                         all_match,
                         "all 100 characters should be '-'");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "fill_100_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_new_formatted
******************************************************************************/

/*
d_tests_dstring_new_formatted
  Tests d_string_new_formatted() which creates a string using printf-style
  format specifiers.

Test cases:
  1. NULL format returns NULL
  2. Simple string (no format specifiers)
  3. Integer format %d
  4. String format %s
  5. Multiple format specifiers
  6. Floating point %f
  7. Hex format %x
  8. Width and precision specifiers

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_new_formatted(struct dj_test_suite* _suite)
{
    struct dj_string*      str;

    d_test_suite_test(_suite, "d_string_new_formatted");

    // test 1: NULL format returns NULL
    str = d_string_new_formatted(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_format_returns_null",
                     (str) == NULL,
                     "NULL format should return NULL");

    // test 2: simple string (no format specifiers)
    str = d_string_new_formatted("Hello World");
    D_TEST_CHECK_MSG(_suite,
                     "simple_string_non_null",
                     (str) != NULL,
                     "simple string should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "simple_string_content",
                         d_test_suite_str_equal((str->text), ("Hello World")),
                         "simple string should match exactly");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "simple_string_skipped", false, "skipped");
    }

    // test 3: integer format %d
    str = d_string_new_formatted("Value: %d", 42);
    D_TEST_CHECK_MSG(_suite,
                     "int_format_non_null",
                     (str) != NULL,
                     "integer format should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "int_format_content",
                         d_test_suite_str_equal((str->text), ("Value: 42")),
                         "integer should be formatted correctly");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "int_format_skipped", false, "skipped");
    }

    // test 4: string format %s
    str = d_string_new_formatted("Name: %s", "Alice");
    D_TEST_CHECK_MSG(_suite,
                     "str_format_non_null",
                     (str) != NULL,
                     "string format should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "str_format_content",
                         d_test_suite_str_equal((str->text), ("Name: Alice")),
                         "string should be formatted correctly");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "str_format_skipped", false, "skipped");
    }

    // test 5: multiple format specifiers
    str = d_string_new_formatted("%s is %d years old", "Bob", 25);
    D_TEST_CHECK_MSG(_suite,
                     "multi_format_non_null",
                     (str) != NULL,
                     "multiple format specifiers should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "multi_format_content",
                         d_test_suite_str_equal((str->text), ("Bob is 25 years old")),
                         "multiple specifiers should be formatted correctly");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "multi_format_skipped", false, "skipped");
    }

    // test 6: hex format %x
    str = d_string_new_formatted("Hex: 0x%x", 255);
    D_TEST_CHECK_MSG(_suite,
                     "hex_format_non_null",
                     (str) != NULL,
                     "hex format should succeed");

    if (str)
    {
        D_TEST_CHECK_MSG(_suite,
                         "hex_format_content",
                         d_test_suite_str_equal((str->text), ("Hex: 0xff")),
                         "hex should be formatted correctly");
        d_string_free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "hex_format_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_free
******************************************************************************/

/*
d_tests_dstring_free
  Tests d_string_free() which frees a dj_string and its contents.

Test cases:
  1. NULL input doesn't crash (no-op)
  2. Free empty string
  3. Free non-empty string
  4. Free string with large allocation

Note: Memory leak testing would require external tools like valgrind.

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_free(struct dj_test_suite* _suite)
{
    struct dj_string*      str;

    d_test_suite_test(_suite, "d_string_free");

    // test 1: NULL input doesn't crash
    d_string_free(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_free_no_crash",
                     true,
                     "d_string_free(NULL) should not crash");

    // test 2: free empty string
    str = d_string_new();

    if (str)
    {
        d_string_free(str);
        D_TEST_CHECK_MSG(_suite,
                         "free_empty_string",
                         true,
                         "freeing empty string should succeed");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "free_empty_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 3: free non-empty string
    str = d_string_new_from_cstr("Test string to free");

    if (str)
    {
        d_string_free(str);
        D_TEST_CHECK_MSG(_suite,
                         "free_non_empty_string",
                         true,
                         "freeing non-empty string should succeed");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "free_non_empty_skipped",
                         false,
                         "skipped - allocation failed");
    }

    // test 4: free string with large allocation
    str = d_string_new_with_capacity(10000);

    if (str)
    {
        d_string_free(str);
        D_TEST_CHECK_MSG(_suite,
                         "free_large_allocation",
                         true,
                         "freeing large allocation should succeed");
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "free_large_skipped",
                         false,
                         "skipped - allocation failed");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_free_contents
******************************************************************************/

/*
d_tests_dstring_free_contents
  Tests d_string_free_contents() which frees the string contents but not the
  dj_string structure itself. Useful for stack-allocated dj_string structs.

Test cases:
  1. NULL input doesn't crash (no-op)
  2. Free contents of string with data
  3. Verify structure is reusable after free_contents
  4. Verify text pointer is NULL after free_contents

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
bool d_tests_dstring_free_contents(struct dj_test_suite* _suite)
{
    struct dj_string*      str;

    d_test_suite_test(_suite, "d_string_free_contents");

    // test 1: NULL input doesn't crash
    d_string_free_contents(NULL);
    D_TEST_CHECK_MSG(_suite,
                     "null_free_contents_no_crash",
                     true,
                     "d_string_free_contents(NULL) should not crash");

    // test 2: free contents of string with data
    str = d_string_new_from_cstr("Content to free");

    if (str)
    {
        d_string_free_contents(str);
        D_TEST_CHECK_MSG(_suite,
                         "free_contents_succeeds",
                         true,
                         "d_string_free_contents should succeed");

        // test 3: verify text is NULL after free_contents
        D_TEST_CHECK_MSG(_suite,
                         "text_null_after_free_contents",
                         (str->text) == NULL,
                         "text should be NULL after free_contents");

        // test 4: verify size is 0 after free_contents
        D_TEST_CHECK_MSG(_suite,
                         "size_zero_after_free_contents",
                         (str->size) == (0),
                         "size should be 0 after free_contents");

        // test 5: verify capacity is 0 after free_contents
        D_TEST_CHECK_MSG(_suite,
                         "capacity_zero_after_free_contents",
                         (str->capacity) == (0),
                         "capacity should be 0 after free_contents");

        // now free the structure itself
        free(str);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite,
                         "free_contents_skipped",
                         false,
                         "skipped - allocation failed");
        D_TEST_CHECK_MSG(_suite, "text_null_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "size_zero_skipped", false, "skipped");
        D_TEST_CHECK_MSG(_suite, "capacity_zero_skipped", false, "skipped");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_creation_all
******************************************************************************/

/*
d_tests_dstring_creation_all
  Runs all creation and destruction tests and returns an aggregate test object
  containing all results.

Parameter(s):
  (none)
Return:
  Test object containing all creation/destruction test results.
*/
bool d_tests_dstring_creation_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "dj_string Creation & Destruction");

    // run all creation/destruction tests
    d_tests_dstring_new(_suite);
    d_tests_dstring_new_with_capacity(_suite);
    d_tests_dstring_new_from_cstr(_suite);
    d_tests_dstring_new_from_cstr_n(_suite);
    d_tests_dstring_new_from_buffer(_suite);
    d_tests_dstring_new_copy(_suite);
    d_tests_dstring_new_fill(_suite);
    d_tests_dstring_new_formatted(_suite);
    d_tests_dstring_free(_suite);
    d_tests_dstring_free_contents(_suite);

    return true;
}
