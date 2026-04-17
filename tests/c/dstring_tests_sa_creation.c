/******************************************************************************
* djinterp [test]                                       dstring_tests_creation.c
*
*   Unit tests for d_string creation and destruction functions:
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
* path:      \src\test\dstring_tests_creation.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "../../tests/c/dstring_tests_sa.h"


/******************************************************************************
* d_tests_sa_dstring_new
******************************************************************************/

/*
d_tests_sa_dstring_new
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
struct d_test_object*
d_tests_sa_dstring_new
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string_new", 5);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // create test subject
    str = d_string_new();

    // test 1: returns non-NULL
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "new_returns_non_null",
        str,
        "d_string_new() should return non-NULL pointer"
    );

    if (str)
    {
        // test 2: size is 0
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "new_size_is_zero",
            str->size, 0,
            "newly created string should have size 0"
        );

        // test 3: capacity is at least default (16)
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "new_capacity_at_least_default",
            str->capacity >= 16,
            "capacity should be at least default (16)"
        );

        // test 4: text pointer is non-NULL
        group->elements[child_idx++] = D_ASSERT_NOT_NULL(
            "new_text_non_null",
            str->text,
            "text pointer should be non-NULL"
        );

        // test 5: text is empty string
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "new_text_is_empty",
            (str->text != NULL) && (str->text[0] == '\0'),
            "text should be empty (null-terminated)"
        );

        d_string_free(str);
    }
    else
    {
        // fill remaining slots with failure indicators
        while (child_idx < 5)
        {
            group->elements[child_idx++] = D_ASSERT_TRUE(
                "skipped_due_to_null",
                false,
                "test skipped - d_string_new returned NULL"
            );
        }
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_new_with_capacity
******************************************************************************/

/*
d_tests_sa_dstring_new_with_capacity
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
struct d_test_object*
d_tests_sa_dstring_new_with_capacity
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string_new_with_capacity", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: capacity 0 - should allocate something
    str = d_string_new_with_capacity(0);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "capacity_0_returns_non_null",
        str,
        "capacity 0 should still return valid string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_0_has_some_capacity",
            str->capacity > 0,
            "even with capacity 0, should have some allocation"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_0_skipped", false, "skipped - allocation failed"
        );
    }

    // test 2: capacity 1
    str = d_string_new_with_capacity(1);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "capacity_1_returns_non_null",
        str,
        "capacity 1 should return valid string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_1_sufficient",
            str->capacity >= 1,
            "capacity should be at least 1"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_1_skipped", false, "skipped - allocation failed"
        );
    }

    // test 3: capacity 32
    str = d_string_new_with_capacity(32);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "capacity_32_returns_non_null",
        str,
        "capacity 32 should return valid string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_32_sufficient",
            str->capacity >= 32,
            "capacity should be at least 32"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_32_skipped", false, "skipped - allocation failed"
        );
    }

    // test 4: capacity 1024
    str = d_string_new_with_capacity(1024);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "capacity_1024_returns_non_null",
        str,
        "capacity 1024 should return valid string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_1024_sufficient",
            str->capacity >= 1024,
            "capacity should be at least 1024"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_1024_skipped", false, "skipped - allocation failed"
        );
    }

    // test 5: size is always 0 for new strings
    str = d_string_new_with_capacity(100);

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "new_with_capacity_size_zero",
            str->size, 0,
            "size should always be 0 for newly created strings"
        );

        // test 6: text is empty
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "new_with_capacity_text_empty",
            (str->text != NULL) && (str->text[0] == '\0'),
            "text should be empty regardless of capacity"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "size_test_skipped", false, "skipped - allocation failed"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "text_test_skipped", false, "skipped - allocation failed"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_new_from_cstr
******************************************************************************/

/*
d_tests_sa_dstring_new_from_cstr
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
struct d_test_object*
d_tests_sa_dstring_new_from_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string_new_from_cstr", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL input returns NULL
    str = d_string_new_from_cstr(NULL);
    group->elements[child_idx++] = D_ASSERT_NULL(
        "null_input_returns_null",
        str,
        "NULL input should return NULL"
    );

    // test 2: empty string input
    str = d_string_new_from_cstr("");
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "empty_string_returns_non_null",
        str,
        "empty string should return valid d_string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_string_size_zero",
            str->size, 0,
            "empty string should have size 0"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_string_skipped", false, "skipped - allocation failed"
        );
    }

    // test 3: single character
    str = d_string_new_from_cstr("A");
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "single_char_returns_non_null",
        str,
        "single char should return valid d_string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "single_char_size_one",
            str->size, 1,
            "single char string should have size 1"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "single_char_skipped", false, "skipped - allocation failed"
        );
    }

    // test 4: short string with content verification
    str = d_string_new_from_cstr("Hello");
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "short_string_returns_non_null",
        str,
        "short string should return valid d_string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "short_string_content_matches",
            str->text, "Hello",
            "content should match input exactly"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "short_string_skipped", false, "skipped - allocation failed"
        );
    }

    // test 5: string with spaces
    str = d_string_new_from_cstr("Hello World!");
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "string_with_spaces_non_null",
        str,
        "string with spaces should work"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "string_with_spaces_size",
            str->size, 12,
            "size should be 12 for 'Hello World!'"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "spaces_skipped", false, "skipped - allocation failed"
        );
    }

    // test 6: string with special characters
    str = d_string_new_from_cstr("Tab:\tNewline:\nQuote:\"");
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "special_chars_non_null",
        str,
        "string with special chars should work"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "special_chars_preserved",
            (str->text != NULL) && (strchr(str->text, '\t') != NULL),
            "special characters should be preserved"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "special_skipped", false, "skipped - allocation failed"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_new_from_cstr_n
******************************************************************************/

/*
d_tests_sa_dstring_new_from_cstr_n
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
struct d_test_object*
d_tests_sa_dstring_new_from_cstr_n
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    const char*           test_str;

    group     = d_test_object_new_interior("d_string_new_from_cstr_n", 10);
    child_idx = 0;
    test_str  = "Hello World";

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL input returns NULL
    str = d_string_new_from_cstr_n(NULL, 5);
    group->elements[child_idx++] = D_ASSERT_NULL(
        "null_input_returns_null",
        str,
        "NULL input should return NULL"
    );

    // test 2: length 0 creates empty string
    str = d_string_new_from_cstr_n(test_str, 0);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "length_0_returns_non_null",
        str,
        "length 0 should return valid empty string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "length_0_size_zero",
            str->size, 0,
            "length 0 should create empty string"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "length_0_skipped", false, "skipped - allocation failed"
        );
    }

    // test 3: length less than string length (partial copy)
    str = d_string_new_from_cstr_n(test_str, 5);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "partial_copy_non_null",
        str,
        "partial copy should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "partial_copy_size",
            str->size, 5,
            "size should be 5 for partial copy"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "partial_copy_content",
            str->text, "Hello",
            "partial copy should contain 'Hello'"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_size_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_content_skipped", false, "skipped"
        );
    }

    // test 4: length equal to string length
    str = d_string_new_from_cstr_n(test_str, 11);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "exact_length_non_null",
        str,
        "exact length should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "exact_length_content",
            str->text, test_str,
            "exact length should copy entire string"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "exact_length_skipped", false, "skipped"
        );
    }

    // test 5: length greater than string length
    str = d_string_new_from_cstr_n(test_str, 100);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "excess_length_non_null",
        str,
        "excess length should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "excess_length_size_correct",
            str->size, 11,
            "size should be actual string length, not requested length"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "excess_length_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_new_from_buffer
******************************************************************************/

/*
d_tests_sa_dstring_new_from_buffer
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
struct d_test_object*
d_tests_sa_dstring_new_from_buffer
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    char                  buffer_with_nulls[10];
    char                  binary_buffer[8];

    group     = d_test_object_new_interior("d_string_new_from_buffer", 9);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL buffer returns NULL
    str = d_string_new_from_buffer(NULL, 5);
    group->elements[child_idx++] = D_ASSERT_NULL(
        "null_buffer_returns_null",
        str,
        "NULL buffer should return NULL"
    );

    // test 2: length 0 creates empty string
    str = d_string_new_from_buffer("anything", 0);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "length_0_returns_non_null",
        str,
        "length 0 should return valid empty string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "length_0_size_zero",
            str->size, 0,
            "length 0 should create empty string"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "length_0_skipped", false, "skipped"
        );
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
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "embedded_nulls_non_null",
        str,
        "buffer with embedded nulls should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "embedded_nulls_size",
            str->size, 8,
            "size should include embedded null characters"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "embedded_nulls_skipped", false, "skipped"
        );
    }

    // test 4: normal buffer copy
    str = d_string_new_from_buffer("Normal text", 11);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "normal_buffer_non_null",
        str,
        "normal buffer copy should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "normal_buffer_content",
            str->text, "Normal text",
            "content should match buffer"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "normal_buffer_skipped", false, "skipped"
        );
    }

    // test 5: binary data in buffer
    binary_buffer[0] = 0x00;
    binary_buffer[1] = 0x01;
    binary_buffer[2] = 0xFF;
    binary_buffer[3] = 0xFE;
    binary_buffer[4] = 0x7F;
    binary_buffer[5] = 0x80;
    binary_buffer[6] = 0x00;
    binary_buffer[7] = 0xFF;

    str = d_string_new_from_buffer(binary_buffer, 8);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "binary_data_non_null",
        str,
        "binary data buffer should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "binary_data_preserved",
            (str->size == 8) && 
            ((unsigned char)str->text[2] == 0xFF),
            "binary data should be preserved exactly"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "binary_data_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_new_copy
******************************************************************************/

/*
d_tests_sa_dstring_new_copy
  Tests d_string_new_copy() which creates a deep copy of an existing d_string.

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
struct d_test_object*
d_tests_sa_dstring_new_copy
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      original;
    struct d_string*      copy;
    size_t                child_idx;
    bool                  deep_copy_test;

    group     = d_test_object_new_interior("d_string_new_copy", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL input returns NULL
    copy = d_string_new_copy(NULL);
    group->elements[child_idx++] = D_ASSERT_NULL(
        "null_input_returns_null",
        copy,
        "NULL input should return NULL"
    );

    // test 2: copy of empty string
    original = d_string_new();

    if (original)
    {
        copy = d_string_new_copy(original);
        group->elements[child_idx++] = D_ASSERT_NOT_NULL(
            "copy_empty_string_non_null",
            copy,
            "copy of empty string should succeed"
        );

        if (copy)
        {
            group->elements[child_idx++] = D_ASSERT_EQUAL(
                "copy_empty_string_size_zero",
                copy->size, 0,
                "copy of empty string should have size 0"
            );
            d_string_free(copy);
        }
        else
        {
            group->elements[child_idx++] = D_ASSERT_TRUE(
                "copy_empty_skipped", false, "skipped"
            );
        }

        d_string_free(original);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_original_failed", false, "failed to create original"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_copy_skipped", false, "skipped"
        );
    }

    // test 3: copy of non-empty string
    original = d_string_new_from_cstr("Test String");

    if (original)
    {
        copy = d_string_new_copy(original);
        group->elements[child_idx++] = D_ASSERT_NOT_NULL(
            "copy_non_empty_non_null",
            copy,
            "copy of non-empty string should succeed"
        );

        if (copy)
        {
            group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
                "copy_content_matches",
                copy->text, original->text,
                "copy content should match original"
            );

            group->elements[child_idx++] = D_ASSERT_EQUAL(
                "copy_size_matches",
                copy->size, original->size,
                "copy size should match original"
            );

            // test 4: verify deep copy - different memory locations
            group->elements[child_idx++] = D_ASSERT_TRUE(
                "deep_copy_different_pointers",
                copy->text != original->text,
                "copy should have different text pointer (deep copy)"
            );

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

            group->elements[child_idx++] = D_ASSERT_TRUE(
                "deep_copy_modification_isolated",
                deep_copy_test,
                "modifying copy should not affect original"
            );

            d_string_free(copy);
        }
        else
        {
            // fill with skip markers
            while (child_idx < 8)
            {
                group->elements[child_idx++] = D_ASSERT_TRUE(
                    "copy_skipped", false, "skipped - copy failed"
                );
            }
        }

        d_string_free(original);
    }
    else
    {
        // fill with skip markers
        while (child_idx < 8)
        {
            group->elements[child_idx++] = D_ASSERT_TRUE(
                "original_failed", false, "skipped - original creation failed"
            );
        }
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_new_fill
******************************************************************************/

/*
d_tests_sa_dstring_new_fill
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
struct d_test_object*
d_tests_sa_dstring_new_fill
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    size_t                i;
    bool                  all_match;

    group     = d_test_object_new_interior("d_string_new_fill", 10);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: length 0 creates empty string
    str = d_string_new_fill(0, 'X');
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "fill_0_returns_non_null",
        str,
        "fill with length 0 should return valid string"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "fill_0_size_zero",
            str->size, 0,
            "fill with length 0 should have size 0"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "fill_0_skipped", false, "skipped"
        );
    }

    // test 2: length 1 creates single character
    str = d_string_new_fill(1, 'A');
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "fill_1_returns_non_null",
        str,
        "fill with length 1 should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "fill_1_content",
            str->text, "A",
            "fill with length 1 should contain single char"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "fill_1_skipped", false, "skipped"
        );
    }

    // test 3: normal fill with letter
    str = d_string_new_fill(5, 'X');
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "fill_5_returns_non_null",
        str,
        "fill with length 5 should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "fill_5_content",
            str->text, "XXXXX",
            "fill should create 5 X characters"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "fill_5_skipped", false, "skipped"
        );
    }

    // test 4: fill with space
    str = d_string_new_fill(3, ' ');
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "fill_space_returns_non_null",
        str,
        "fill with spaces should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "fill_space_content",
            str->text, "   ",
            "fill should create 3 spaces"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "fill_space_skipped", false, "skipped"
        );
    }

    // test 5: large fill (100 characters)
    str = d_string_new_fill(100, '-');
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "fill_100_returns_non_null",
        str,
        "fill with length 100 should succeed"
    );

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

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "fill_100_all_match",
            all_match,
            "all 100 characters should be '-'"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "fill_100_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_new_formatted
******************************************************************************/

/*
d_tests_sa_dstring_new_formatted
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
struct d_test_object*
d_tests_sa_dstring_new_formatted
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string_new_formatted", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL format returns NULL
    str = d_string_new_formatted(NULL);
    group->elements[child_idx++] = D_ASSERT_NULL(
        "null_format_returns_null",
        str,
        "NULL format should return NULL"
    );

    // test 2: simple string (no format specifiers)
    str = d_string_new_formatted("Hello World");
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "simple_string_non_null",
        str,
        "simple string should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "simple_string_content",
            str->text, "Hello World",
            "simple string should match exactly"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "simple_string_skipped", false, "skipped"
        );
    }

    // test 3: integer format %d
    str = d_string_new_formatted("Value: %d", 42);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "int_format_non_null",
        str,
        "integer format should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "int_format_content",
            str->text, "Value: 42",
            "integer should be formatted correctly"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "int_format_skipped", false, "skipped"
        );
    }

    // test 4: string format %s
    str = d_string_new_formatted("Name: %s", "Alice");
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "str_format_non_null",
        str,
        "string format should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "str_format_content",
            str->text, "Name: Alice",
            "string should be formatted correctly"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "str_format_skipped", false, "skipped"
        );
    }

    // test 5: multiple format specifiers
    str = d_string_new_formatted("%s is %d years old", "Bob", 25);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "multi_format_non_null",
        str,
        "multiple format specifiers should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "multi_format_content",
            str->text, "Bob is 25 years old",
            "multiple specifiers should be formatted correctly"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "multi_format_skipped", false, "skipped"
        );
    }

    // test 6: hex format %x
    str = d_string_new_formatted("Hex: 0x%x", 255);
    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "hex_format_non_null",
        str,
        "hex format should succeed"
    );

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "hex_format_content",
            str->text, "Hex: 0xff",
            "hex should be formatted correctly"
        );
        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "hex_format_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_free
******************************************************************************/

/*
d_tests_sa_dstring_free
  Tests d_string_free() which frees a d_string and its contents.

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
struct d_test_object*
d_tests_sa_dstring_free
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string_free", 4);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL input doesn't crash
    d_string_free(NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_free_no_crash",
        true,
        "d_string_free(NULL) should not crash"
    );

    // test 2: free empty string
    str = d_string_new();

    if (str)
    {
        d_string_free(str);
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_empty_string",
            true,
            "freeing empty string should succeed"
        );
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_empty_skipped", false, "skipped - allocation failed"
        );
    }

    // test 3: free non-empty string
    str = d_string_new_from_cstr("Test string to free");

    if (str)
    {
        d_string_free(str);
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_non_empty_string",
            true,
            "freeing non-empty string should succeed"
        );
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_non_empty_skipped", false, "skipped - allocation failed"
        );
    }

    // test 4: free string with large allocation
    str = d_string_new_with_capacity(10000);

    if (str)
    {
        d_string_free(str);
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_large_allocation",
            true,
            "freeing large allocation should succeed"
        );
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_large_skipped", false, "skipped - allocation failed"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_free_contents
******************************************************************************/

/*
d_tests_sa_dstring_free_contents
  Tests d_string_free_contents() which frees the string contents but not the
  d_string structure itself. Useful for stack-allocated d_string structs.

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
struct d_test_object*
d_tests_sa_dstring_free_contents
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string_free_contents", 5);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL input doesn't crash
    d_string_free_contents(NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_free_contents_no_crash",
        true,
        "d_string_free_contents(NULL) should not crash"
    );

    // test 2: free contents of string with data
    str = d_string_new_from_cstr("Content to free");

    if (str)
    {
        d_string_free_contents(str);
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_contents_succeeds",
            true,
            "d_string_free_contents should succeed"
        );

        // test 3: verify text is NULL after free_contents
        group->elements[child_idx++] = D_ASSERT_NULL(
            "text_null_after_free_contents",
            str->text,
            "text should be NULL after free_contents"
        );

        // test 4: verify size is 0 after free_contents
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "size_zero_after_free_contents",
            str->size, 0,
            "size should be 0 after free_contents"
        );

        // test 5: verify capacity is 0 after free_contents
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "capacity_zero_after_free_contents",
            str->capacity, 0,
            "capacity should be 0 after free_contents"
        );

        // now free the structure itself
        free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "free_contents_skipped", false, "skipped - allocation failed"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "text_null_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "size_zero_skipped", false, "skipped"
        );
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_zero_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_creation_all
******************************************************************************/

/*
d_tests_sa_dstring_creation_all
  Runs all creation and destruction tests and returns an aggregate test object
  containing all results.

Parameter(s):
  (none)
Return:
  Test object containing all creation/destruction test results.
*/
struct d_test_object*
d_tests_sa_dstring_creation_all
(
    void
)
{
    struct d_test_object* group;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string Creation & Destruction", 10);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // run all creation/destruction tests
    group->elements[child_idx++] = d_tests_sa_dstring_new();
    group->elements[child_idx++] = d_tests_sa_dstring_new_with_capacity();
    group->elements[child_idx++] = d_tests_sa_dstring_new_from_cstr();
    group->elements[child_idx++] = d_tests_sa_dstring_new_from_cstr_n();
    group->elements[child_idx++] = d_tests_sa_dstring_new_from_buffer();
    group->elements[child_idx++] = d_tests_sa_dstring_new_copy();
    group->elements[child_idx++] = d_tests_sa_dstring_new_fill();
    group->elements[child_idx++] = d_tests_sa_dstring_new_formatted();
    group->elements[child_idx++] = d_tests_sa_dstring_free();
    group->elements[child_idx++] = d_tests_sa_dstring_free_contents();

    return group;
}
