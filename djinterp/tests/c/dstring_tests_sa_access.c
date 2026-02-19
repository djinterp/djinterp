/******************************************************************************
* djinterp [test]                                        dstring_tests_access.c
*
*   Unit tests for d_string access functions:
*     - d_string_length
*     - d_string_size
*     - d_string_cstr
*     - d_string_data
*     - d_string_is_empty
*     - d_string_char_at
*     - d_string_set_char
*     - d_string_front
*     - d_string_back
*
* path:      \src\test\dstring_tests_access.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "..\tests\dstring_tests_sa.h"


/******************************************************************************
* d_tests_sa_dstring_length
******************************************************************************/

/*
d_tests_sa_dstring_length
  Tests d_string_length() which returns the length of the string (number of
  characters, not including null terminator).

Test cases:
  1. NULL string returns 0
  2. Empty string returns 0
  3. Single character returns 1
  4. Normal string returns correct length
  5. String with spaces counted correctly
  6. Length matches strlen of text
  7. Length after modification

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_length
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    size_t                len;

    group     = d_test_object_new_interior("d_string_length", 9);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns 0
    len = d_string_length(NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "null_string_returns_0",
        len, 0,
        "d_string_length(NULL) should return 0"
    );

    // test 2: empty string returns 0
    str = d_string_new();

    if (str)
    {
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_string_length_0",
            len, 0,
            "empty string should have length 0"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_skipped", false, "skipped"
        );
    }

    // test 3: single character returns 1
    str = d_string_new_from_cstr("A");

    if (str)
    {
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "single_char_length_1",
            len, 1,
            "single character string should have length 1"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "single_skipped", false, "skipped"
        );
    }

    // test 4: normal string returns correct length
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "normal_string_length",
            len, 5,
            "'Hello' should have length 5"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "normal_skipped", false, "skipped"
        );
    }

    // test 5: string with spaces counted correctly
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "spaces_counted",
            len, 11,
            "'Hello World' should have length 11 (space counted)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "spaces_skipped", false, "skipped"
        );
    }

    // test 6: length matches strlen of text
    str = d_string_new_from_cstr("Test string for strlen comparison");

    if (str)
    {
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "length_matches_strlen",
            len, strlen(str->text),
            "d_string_length should match strlen(text)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "strlen_skipped", false, "skipped"
        );
    }

    // test 7: length matches size field
    str = d_string_new_from_cstr("Size test");

    if (str)
    {
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "length_matches_size_field",
            len, str->size,
            "d_string_length should match size field"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "size_field_skipped", false, "skipped"
        );
    }

    // test 8: length after append
    str = d_string_new_from_cstr("Start");

    if (str)
    {
        d_string_append_cstr(str, " End");
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "length_after_append",
            len, 9,
            "length should be 9 after appending ' End' to 'Start'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "append_skipped", false, "skipped"
        );
    }

    // test 9: length of long string
    str = d_string_new_fill(1000, 'x');

    if (str)
    {
        len = d_string_length(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "long_string_length",
            len, 1000,
            "string filled with 1000 chars should have length 1000"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "long_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_size
******************************************************************************/

/*
d_tests_sa_dstring_size
  Tests d_string_size() which returns the size of the string (same as length).

Test cases:
  1. NULL string returns 0
  2. Empty string returns 0
  3. Normal string returns correct size
  4. Size equals length
  5. Size matches struct field

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_size
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    size_t                sz;

    group     = d_test_object_new_interior("d_string_size", 6);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns 0
    sz = d_string_size(NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "null_string_returns_0",
        sz, 0,
        "d_string_size(NULL) should return 0"
    );

    // test 2: empty string returns 0
    str = d_string_new();

    if (str)
    {
        sz = d_string_size(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_string_size_0",
            sz, 0,
            "empty string should have size 0"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_skipped", false, "skipped"
        );
    }

    // test 3: normal string returns correct size
    str = d_string_new_from_cstr("Testing");

    if (str)
    {
        sz = d_string_size(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "normal_string_size",
            sz, 7,
            "'Testing' should have size 7"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "normal_skipped", false, "skipped"
        );
    }

    // test 4: size equals length
    str = d_string_new_from_cstr("Compare size and length");

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "size_equals_length",
            d_string_size(str), d_string_length(str),
            "d_string_size should equal d_string_length"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "compare_skipped", false, "skipped"
        );
    }

    // test 5: size matches struct field
    str = d_string_new_from_cstr("Field test");

    if (str)
    {
        sz = d_string_size(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "size_matches_field",
            sz, str->size,
            "d_string_size should match struct size field"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "field_skipped", false, "skipped"
        );
    }

    // test 6: size of string with special chars
    str = d_string_new_from_cstr("Tab:\tNewline:\n");

    if (str)
    {
        sz = d_string_size(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "special_chars_size",
            sz, 14,
            "special chars should be counted (tab and newline = 1 char each)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "special_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_cstr
******************************************************************************/

/*
d_tests_sa_dstring_cstr
  Tests d_string_cstr() which returns a pointer to the null-terminated C string.

Test cases:
  1. NULL string returns NULL
  2. Returns valid pointer for non-NULL string
  3. Returned string is null-terminated
  4. Content matches expected
  5. Pointer equals text field
  6. Can be used with standard C functions

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    const char*           cstr;

    group     = d_test_object_new_interior("d_string_cstr", 7);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns NULL
    cstr = d_string_cstr(NULL);
    group->elements[child_idx++] = D_ASSERT_NULL(
        "null_string_returns_null",
        cstr,
        "d_string_cstr(NULL) should return NULL"
    );

    // test 2: returns valid pointer
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        cstr = d_string_cstr(str);

        group->elements[child_idx++] = D_ASSERT_NOT_NULL(
            "returns_valid_pointer",
            cstr,
            "d_string_cstr should return non-NULL for valid string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "valid_skipped", false, "skipped"
        );
    }

    // test 3: returned string is null-terminated
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        cstr = d_string_cstr(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_terminated",
            (cstr != NULL) && (cstr[5] == '\0'),
            "returned string should be null-terminated"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_term_skipped", false, "skipped"
        );
    }

    // test 4: content matches expected
    str = d_string_new_from_cstr("Content Test");

    if (str)
    {
        cstr = d_string_cstr(str);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "content_matches",
            cstr, "Content Test",
            "returned C string should match content"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "content_skipped", false, "skipped"
        );
    }

    // test 5: pointer equals text field
    str = d_string_new_from_cstr("Pointer Test");

    if (str)
    {
        cstr = d_string_cstr(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "pointer_equals_text",
            cstr == str->text,
            "d_string_cstr should return same pointer as text field"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "pointer_skipped", false, "skipped"
        );
    }

    // test 6: works with strlen
    str = d_string_new_from_cstr("strlen test");

    if (str)
    {
        cstr = d_string_cstr(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "works_with_strlen",
            strlen(cstr), 11,
            "returned C string should work with strlen"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "strlen_skipped", false, "skipped"
        );
    }

    // test 7: works with strcmp
    str = d_string_new_from_cstr("compare");

    if (str)
    {
        cstr = d_string_cstr(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "works_with_strcmp",
            strcmp(cstr, "compare") == 0,
            "returned C string should work with strcmp"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "strcmp_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_data
******************************************************************************/

/*
d_tests_sa_dstring_data
  Tests d_string_data() which returns a mutable pointer to the string data.

Test cases:
  1. NULL string returns NULL
  2. Returns valid pointer
  3. Same as cstr for non-const access
  4. Can modify through returned pointer
  5. Modifications reflected in string

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_data
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    char*                 data;

    group     = d_test_object_new_interior("d_string_data", 6);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns NULL
    data = d_string_data(NULL);
    group->elements[child_idx++] = D_ASSERT_NULL(
        "null_string_returns_null",
        data,
        "d_string_data(NULL) should return NULL"
    );

    // test 2: returns valid pointer
    str = d_string_new_from_cstr("Data");

    if (str)
    {
        data = d_string_data(str);

        group->elements[child_idx++] = D_ASSERT_NOT_NULL(
            "returns_valid_pointer",
            data,
            "d_string_data should return non-NULL for valid string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "valid_skipped", false, "skipped"
        );
    }

    // test 3: pointer equals text field
    str = d_string_new_from_cstr("Pointer");

    if (str)
    {
        data = d_string_data(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "pointer_equals_text",
            data == str->text,
            "d_string_data should return same pointer as text field"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "pointer_skipped", false, "skipped"
        );
    }

    // test 4: can modify through returned pointer
    str = d_string_new_from_cstr("Modify");

    if (str)
    {
        data = d_string_data(str);

        if (data)
        {
            data[0] = 'X';

            group->elements[child_idx++] = D_ASSERT_TRUE(
                "can_modify",
                str->text[0] == 'X',
                "modification through data pointer should affect string"
            );
        }
        else
        {
            group->elements[child_idx++] = D_ASSERT_TRUE(
                "modify_skipped", false, "skipped - NULL data"
            );
        }

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "modify_skipped", false, "skipped"
        );
    }

    // test 5: modifications reflected in cstr
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        data = d_string_data(str);

        if (data)
        {
            data[0] = 'B';
            data[1] = 'e';
            data[2] = 's';
            data[3] = 't';

            group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
                "modifications_reflected",
                d_string_cstr(str), "Best",
                "modifications should be reflected in cstr"
            );
        }
        else
        {
            group->elements[child_idx++] = D_ASSERT_TRUE(
                "reflected_skipped", false, "skipped"
            );
        }

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "reflected_skipped", false, "skipped"
        );
    }

    // test 6: empty string data
    str = d_string_new();

    if (str)
    {
        data = d_string_data(str);

        group->elements[child_idx++] = D_ASSERT_NOT_NULL(
            "empty_string_data_not_null",
            data,
            "empty string should still return valid data pointer"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_is_empty
******************************************************************************/

/*
d_tests_sa_dstring_is_empty
  Tests d_string_is_empty() which returns true if the string has length 0.

Test cases:
  1. NULL string returns true (considered empty)
  2. New empty string returns true
  3. String with content returns false
  4. Single character returns false
  5. String with only spaces returns false
  6. After clearing string returns true
  7. String with only whitespace returns false

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_is_empty
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    bool                  is_empty;

    group     = d_test_object_new_interior("d_string_is_empty", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns true
    is_empty = d_string_is_empty(NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_string_is_empty",
        is_empty,
        "d_string_is_empty(NULL) should return true"
    );

    // test 2: new empty string returns true
    str = d_string_new();

    if (str)
    {
        is_empty = d_string_is_empty(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "new_string_is_empty",
            is_empty,
            "newly created string should be empty"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "new_skipped", false, "skipped"
        );
    }

    // test 3: string from "" returns true
    str = d_string_new_from_cstr("");

    if (str)
    {
        is_empty = d_string_is_empty(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_cstr_is_empty",
            is_empty,
            "string created from empty C string should be empty"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_cstr_skipped", false, "skipped"
        );
    }

    // test 4: string with content returns false
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        is_empty = d_string_is_empty(str);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "content_not_empty",
            is_empty,
            "string with content should not be empty"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "content_skipped", false, "skipped"
        );
    }

    // test 5: single character returns false
    str = d_string_new_from_cstr("X");

    if (str)
    {
        is_empty = d_string_is_empty(str);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "single_char_not_empty",
            is_empty,
            "single character string should not be empty"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "single_skipped", false, "skipped"
        );
    }

    // test 6: string with only space returns false
    str = d_string_new_from_cstr(" ");

    if (str)
    {
        is_empty = d_string_is_empty(str);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "space_not_empty",
            is_empty,
            "string with space should not be empty (space is a character)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "space_skipped", false, "skipped"
        );
    }

    // test 7: after clearing string returns true
    str = d_string_new_from_cstr("Content");

    if (str)
    {
        d_string_clear(str);
        is_empty = d_string_is_empty(str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "after_clear_is_empty",
            is_empty,
            "string should be empty after clearing"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "clear_skipped", false, "skipped"
        );
    }

    // test 8: whitespace string returns false
    str = d_string_new_from_cstr("\t\n ");

    if (str)
    {
        is_empty = d_string_is_empty(str);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "whitespace_not_empty",
            is_empty,
            "string with whitespace chars should not be empty"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "whitespace_skipped", false, "skipped"
        );
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_char_at
******************************************************************************/

/*
d_tests_sa_dstring_char_at
  Tests d_string_char_at() which returns the character at a given index,
  supporting negative indices for reverse indexing.

Test cases:
  1.  NULL string returns '\0'
  2.  Index 0 returns first character
  3.  Positive index in range
  4.  Last character (length - 1)
  5.  Index out of bounds returns '\0'
  6.  Negative index -1 returns last character
  7.  Negative index -2 returns second to last
  8.  Negative index out of bounds returns '\0'
  9.  Empty string any index returns '\0'
  10. Various positions in longer string

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_char_at
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    char                  ch;

    group     = d_test_object_new_interior("d_string_char_at", 14);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns '\0'
    ch = d_string_char_at(NULL, 0);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "null_string_returns_nul",
        ch, '\0',
        "d_string_char_at(NULL, ...) should return '\\0'"
    );

    // test 2: index 0 returns first character
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        ch = d_string_char_at(str, 0);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "index_0_first_char",
            ch, 'H',
            "index 0 should return first character 'H'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "index_0_skipped", false, "skipped"
        );
    }

    // test 3: positive index in range
    str = d_string_new_from_cstr("ABCDE");

    if (str)
    {
        ch = d_string_char_at(str, 2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "positive_index_in_range",
            ch, 'C',
            "index 2 in 'ABCDE' should return 'C'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "positive_skipped", false, "skipped"
        );
    }

    // test 4: last character
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        ch = d_string_char_at(str, 3);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "last_char",
            ch, 't',
            "index 3 in 'Test' should return 't'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "last_skipped", false, "skipped"
        );
    }

    // test 5: index out of bounds returns '\0'
    str = d_string_new_from_cstr("Short");

    if (str)
    {
        ch = d_string_char_at(str, 100);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "out_of_bounds_returns_nul",
            ch, '\0',
            "out of bounds index should return '\\0'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "oob_skipped", false, "skipped"
        );
    }

    // test 6: negative index -1 returns last character
    str = d_string_new_from_cstr("End");

    if (str)
    {
        ch = d_string_char_at(str, -1);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "negative_1_last_char",
            ch, 'd',
            "index -1 in 'End' should return last char 'd'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "neg1_skipped", false, "skipped"
        );
    }

    // test 7: negative index -2 returns second to last
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        ch = d_string_char_at(str, -2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "negative_2_second_to_last",
            ch, 's',
            "index -2 in 'Test' should return 's'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "neg2_skipped", false, "skipped"
        );
    }

    // test 8: negative index equals first char
    str = d_string_new_from_cstr("First");

    if (str)
    {
        ch = d_string_char_at(str, -5);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "negative_equals_first",
            ch, 'F',
            "index -5 in 5-char string should return first char"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "neg_first_skipped", false, "skipped"
        );
    }

    // test 9: negative index out of bounds
    str = d_string_new_from_cstr("ABC");

    if (str)
    {
        ch = d_string_char_at(str, -10);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "negative_out_of_bounds",
            ch, '\0',
            "negative index out of bounds should return '\\0'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "neg_oob_skipped", false, "skipped"
        );
    }

    // test 10: empty string any index returns '\0'
    str = d_string_new();

    if (str)
    {
        ch = d_string_char_at(str, 0);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_string_returns_nul",
            ch, '\0',
            "any index in empty string should return '\\0'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_skipped", false, "skipped"
        );
    }

    // test 11-14: various positions in longer string
    str = d_string_new_from_cstr("0123456789");

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "digit_index_0",
            d_string_char_at(str, 0), '0',
            "index 0 should be '0'"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "digit_index_5",
            d_string_char_at(str, 5), '5',
            "index 5 should be '5'"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "digit_index_neg_1",
            d_string_char_at(str, -1), '9',
            "index -1 should be '9'"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "digit_index_neg_5",
            d_string_char_at(str, -5), '5',
            "index -5 should be '5'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("d0_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("d5_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("dn1_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("dn5_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_set_char
******************************************************************************/

/*
d_tests_sa_dstring_set_char
  Tests d_string_set_char() which sets the character at a given index,
  supporting negative indices for reverse indexing.

Test cases:
  1. NULL string returns false
  2. Set character at index 0
  3. Set character at middle index
  4. Set character at last position
  5. Index out of bounds returns false
  6. Negative index -1 sets last character
  7. Negative index out of bounds returns false
  8. Empty string returns false
  9. Setting null character (edge case)
  10. Multiple sets on same string

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_set_char
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_set_char", 14);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns false
    result = d_string_set_char(NULL, 0, 'X');
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_string_returns_false",
        result,
        "d_string_set_char(NULL, ...) should return false"
    );

    // test 2: set character at index 0
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        result = d_string_set_char(str, 0, 'J');

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "set_index_0_succeeds",
            result,
            "setting character at index 0 should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "set_index_0_content",
            str->text, "Jello",
            "'Hello' with index 0 set to 'J' should be 'Jello'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("i0_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("i0c_skip", false, "skip");
    }

    // test 3: set character at middle index
    str = d_string_new_from_cstr("ABCDE");

    if (str)
    {
        result = d_string_set_char(str, 2, 'X');

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "set_middle_succeeds",
            result,
            "setting character at middle index should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "set_middle_content",
            str->text, "ABXDE",
            "'ABCDE' with index 2 set to 'X' should be 'ABXDE'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("mid_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("midc_skip", false, "skip");
    }

    // test 4: set character at last position
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        result = d_string_set_char(str, 3, '!');

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "set_last_content",
            str->text, "Tes!",
            "'Test' with last char set to '!' should be 'Tes!'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("last_skip", false, "skip");
    }

    // test 5: index out of bounds returns false
    str = d_string_new_from_cstr("Short");

    if (str)
    {
        result = d_string_set_char(str, 100, 'X');

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "out_of_bounds_fails",
            result,
            "setting character out of bounds should return false"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "out_of_bounds_unchanged",
            str->text, "Short",
            "string should be unchanged after failed set"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("oob_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("oobc_skip", false, "skip");
    }

    // test 6: negative index -1 sets last character
    str = d_string_new_from_cstr("End");

    if (str)
    {
        result = d_string_set_char(str, -1, 'X');

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "negative_1_succeeds",
            result,
            "setting at index -1 should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "negative_1_content",
            str->text, "EnX",
            "'End' with index -1 set to 'X' should be 'EnX'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("n1_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("n1c_skip", false, "skip");
    }

    // test 7: negative index out of bounds
    str = d_string_new_from_cstr("ABC");

    if (str)
    {
        result = d_string_set_char(str, -10, 'X');

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "negative_oob_fails",
            result,
            "negative index out of bounds should return false"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("noob_skip", false, "skip");
    }

    // test 8: empty string returns false
    str = d_string_new();

    if (str)
    {
        result = d_string_set_char(str, 0, 'X');

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "empty_string_fails",
            result,
            "setting character in empty string should return false"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("empty_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_front
******************************************************************************/

/*
d_tests_sa_dstring_front
  Tests d_string_front() which returns the first character of the string.

Test cases:
  1. NULL string returns '\0'
  2. Empty string returns '\0'
  3. Single character string
  4. Normal string returns first char
  5. String starting with space
  6. String starting with number

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_front
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    char                  ch;

    group     = d_test_object_new_interior("d_string_front", 7);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns '\0'
    ch = d_string_front(NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "null_string_returns_nul",
        ch, '\0',
        "d_string_front(NULL) should return '\\0'"
    );

    // test 2: empty string returns '\0'
    str = d_string_new();

    if (str)
    {
        ch = d_string_front(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_string_returns_nul",
            ch, '\0',
            "front of empty string should return '\\0'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("empty_skip", false, "skip");
    }

    // test 3: single character string
    str = d_string_new_from_cstr("X");

    if (str)
    {
        ch = d_string_front(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "single_char_front",
            ch, 'X',
            "front of 'X' should be 'X'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("single_skip", false, "skip");
    }

    // test 4: normal string returns first char
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        ch = d_string_front(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "normal_front",
            ch, 'H',
            "front of 'Hello' should be 'H'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("normal_skip", false, "skip");
    }

    // test 5: string starting with space
    str = d_string_new_from_cstr(" Space");

    if (str)
    {
        ch = d_string_front(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "space_front",
            ch, ' ',
            "front of ' Space' should be ' '"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("space_skip", false, "skip");
    }

    // test 6: string starting with number
    str = d_string_new_from_cstr("123abc");

    if (str)
    {
        ch = d_string_front(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "number_front",
            ch, '1',
            "front of '123abc' should be '1'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("number_skip", false, "skip");
    }

    // test 7: front equals char_at(0)
    str = d_string_new_from_cstr("Compare");

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "front_equals_char_at_0",
            d_string_front(str), d_string_char_at(str, 0),
            "front() should equal char_at(0)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("compare_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_back
******************************************************************************/

/*
d_tests_sa_dstring_back
  Tests d_string_back() which returns the last character of the string.

Test cases:
  1. NULL string returns '\0'
  2. Empty string returns '\0'
  3. Single character string
  4. Normal string returns last char
  5. String ending with space
  6. String ending with number
  7. Back equals char_at(-1)

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_back
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    char                  ch;

    group     = d_test_object_new_interior("d_string_back", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns '\0'
    ch = d_string_back(NULL);
    group->elements[child_idx++] = D_ASSERT_EQUAL(
        "null_string_returns_nul",
        ch, '\0',
        "d_string_back(NULL) should return '\\0'"
    );

    // test 2: empty string returns '\0'
    str = d_string_new();

    if (str)
    {
        ch = d_string_back(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_string_returns_nul",
            ch, '\0',
            "back of empty string should return '\\0'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("empty_skip", false, "skip");
    }

    // test 3: single character string
    str = d_string_new_from_cstr("Y");

    if (str)
    {
        ch = d_string_back(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "single_char_back",
            ch, 'Y',
            "back of 'Y' should be 'Y'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("single_skip", false, "skip");
    }

    // test 4: normal string returns last char
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        ch = d_string_back(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "normal_back",
            ch, 'o',
            "back of 'Hello' should be 'o'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("normal_skip", false, "skip");
    }

    // test 5: string ending with space
    str = d_string_new_from_cstr("Space ");

    if (str)
    {
        ch = d_string_back(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "space_back",
            ch, ' ',
            "back of 'Space ' should be ' '"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("space_skip", false, "skip");
    }

    // test 6: string ending with number
    str = d_string_new_from_cstr("abc123");

    if (str)
    {
        ch = d_string_back(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "number_back",
            ch, '3',
            "back of 'abc123' should be '3'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("number_skip", false, "skip");
    }

    // test 7: back equals char_at(-1)
    str = d_string_new_from_cstr("Compare");

    if (str)
    {
        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "back_equals_char_at_neg1",
            d_string_back(str), d_string_char_at(str, -1),
            "back() should equal char_at(-1)"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("compare_skip", false, "skip");
    }

    // test 8: string ending with punctuation
    str = d_string_new_from_cstr("Hello!");

    if (str)
    {
        ch = d_string_back(str);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "punctuation_back",
            ch, '!',
            "back of 'Hello!' should be '!'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("punct_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_access_all
******************************************************************************/

/*
d_tests_sa_dstring_access_all
  Runs all access function tests and returns an aggregate test object
  containing all results.

Parameter(s):
  (none)
Return:
  Test object containing all access function test results.
*/
struct d_test_object*
d_tests_sa_dstring_access_all
(
    void
)
{
    struct d_test_object* group;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string Access Functions", 9);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // run all access function tests
    group->elements[child_idx++] = d_tests_sa_dstring_length();
    group->elements[child_idx++] = d_tests_sa_dstring_size();
    group->elements[child_idx++] = d_tests_sa_dstring_cstr();
    group->elements[child_idx++] = d_tests_sa_dstring_data();
    group->elements[child_idx++] = d_tests_sa_dstring_is_empty();
    group->elements[child_idx++] = d_tests_sa_dstring_char_at();
    group->elements[child_idx++] = d_tests_sa_dstring_set_char();
    group->elements[child_idx++] = d_tests_sa_dstring_front();
    group->elements[child_idx++] = d_tests_sa_dstring_back();

    return group;
}
