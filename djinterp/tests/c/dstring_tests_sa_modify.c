#include ".\dstring_tests_sa.h"


/******************************************************************************
 * SECTION 9: MODIFICATION FUNCTIONS (IN-PLACE)
 *****************************************************************************/

/******************************************************************************
 * I. ASSIGNMENT TESTS
 *****************************************************************************/

/*
d_tests_sa_dstring_assign
  Tests d_string_assign function.
  Tests the following:
  - assigning from one d_string to another
  - assigning to empty string
  - assigning empty string
  - self-assignment handling
  - NULL destination handling
  - NULL source handling
*/
struct d_test_object*
d_tests_sa_dstring_assign
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_assign", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: assigning from one d_string to another
    dest = d_string_new_from_cstr("original");
    src  = d_string_new_from_cstr("replacement");

    if (dest && src)
    {
        result = d_string_assign(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_basic",
            result && d_string_equals_cstr(dest, "replacement"),
            "should assign 'replacement' to dest");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_basic",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: assigning to empty string
    dest = d_string_new();
    src  = d_string_new_from_cstr("content");

    if (dest && src)
    {
        result = d_string_assign(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_to_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should assign to empty string");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_to_empty",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: assigning empty string
    dest = d_string_new_from_cstr("content");
    src  = d_string_new_from_cstr("");

    if (dest && src)
    {
        result = d_string_assign(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_empty_string",
            result && d_string_is_empty(dest),
            "should assign empty string");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_empty_string",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: self-assignment handling
    dest = d_string_new_from_cstr("self");

    if (dest)
    {
        result = d_string_assign(dest, dest);
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_self",
            result && d_string_equals_cstr(dest, "self"),
            "should handle self-assignment");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_self",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    src    = d_string_new_from_cstr("test");
    result = d_string_assign(NULL, src);
    group->elements[idx++] = D_ASSERT_FALSE(
        "assign_null_dest",
        result,
        "should return false for NULL destination");

    d_string_free(src);

    // test: NULL source handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_assign(dest, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "assign_null_src",
        result,
        "should return false for NULL source");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_assign_cstr
  Tests d_string_assign_cstr function.
  Tests the following:
  - assigning C string to d_string
  - assigning to empty string
  - assigning empty C string
  - NULL destination handling
  - NULL C string handling
*/
struct d_test_object*
d_tests_sa_dstring_assign_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_assign_cstr", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: assigning C string to d_string
    dest = d_string_new_from_cstr("original");

    if (dest)
    {
        result = d_string_assign_cstr(dest, "replacement");
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_cstr_basic",
            result && d_string_equals_cstr(dest, "replacement"),
            "should assign 'replacement' to dest");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_cstr_basic",
            false,
            "failed to allocate test string");
    }

    // test: assigning to empty string
    dest = d_string_new();

    if (dest)
    {
        result = d_string_assign_cstr(dest, "content");
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_cstr_to_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should assign to empty string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_cstr_to_empty",
            false,
            "failed to allocate test string");
    }

    // test: assigning empty C string
    dest = d_string_new_from_cstr("content");

    if (dest)
    {
        result = d_string_assign_cstr(dest, "");
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_cstr_empty",
            result && d_string_is_empty(dest),
            "should assign empty C string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_cstr_empty",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_assign_cstr(NULL, "test");
    group->elements[idx++] = D_ASSERT_FALSE(
        "assign_cstr_null_dest",
        result,
        "should return false for NULL destination");

    // test: NULL C string handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_assign_cstr(dest, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "assign_cstr_null_cstr",
        result,
        "should return false for NULL C string");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_assign_buffer
  Tests d_string_assign_buffer function.
  Tests the following:
  - assigning buffer with length to d_string
  - assigning partial buffer
  - assigning buffer with embedded null
  - NULL destination handling
  - NULL buffer handling
*/
struct d_test_object*
d_tests_sa_dstring_assign_buffer
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    const char*           buffer;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_assign_buffer", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: assigning buffer with length to d_string
    dest   = d_string_new_from_cstr("original");
    buffer = "replacement";

    if (dest)
    {
        result = d_string_assign_buffer(dest, buffer, 11);
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_buffer_basic",
            result && d_string_equals_cstr(dest, "replacement"),
            "should assign buffer to dest");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_buffer_basic",
            false,
            "failed to allocate test string");
    }

    // test: assigning partial buffer
    dest   = d_string_new_from_cstr("original");
    buffer = "hello world";

    if (dest)
    {
        result = d_string_assign_buffer(dest, buffer, 5);
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_buffer_partial",
            result && d_string_equals_cstr(dest, "hello"),
            "should assign partial buffer");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_buffer_partial",
            false,
            "failed to allocate test string");
    }

    // test: assigning buffer with embedded null
    dest   = d_string_new_from_cstr("original");
    buffer = "hel\0lo";

    if (dest)
    {
        result = d_string_assign_buffer(dest, buffer, 6);
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_buffer_embedded_null",
            result && (d_string_length(dest) == 6),
            "should assign buffer with embedded null");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_buffer_embedded_null",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_assign_buffer(NULL, "test", 4);
    group->elements[idx++] = D_ASSERT_FALSE(
        "assign_buffer_null_dest",
        result,
        "should return false for NULL destination");

    // test: NULL buffer handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_assign_buffer(dest, NULL, 4);
    group->elements[idx++] = D_ASSERT_FALSE(
        "assign_buffer_null_buffer",
        result,
        "should return false for NULL buffer");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_assign_char
  Tests d_string_assign_char function.
  Tests the following:
  - assigning repeated character
  - assigning single character
  - assigning zero count (empty result)
  - NULL destination handling
*/
struct d_test_object*
d_tests_sa_dstring_assign_char
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_assign_char", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: assigning repeated character
    dest = d_string_new_from_cstr("original");

    if (dest)
    {
        result = d_string_assign_char(dest, 5, 'x');
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_char_repeated",
            result && d_string_equals_cstr(dest, "xxxxx"),
            "should assign 5 'x' characters");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_char_repeated",
            false,
            "failed to allocate test string");
    }

    // test: assigning single character
    dest = d_string_new_from_cstr("original");

    if (dest)
    {
        result = d_string_assign_char(dest, 1, 'a');
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_char_single",
            result && d_string_equals_cstr(dest, "a"),
            "should assign single 'a' character");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_char_single",
            false,
            "failed to allocate test string");
    }

    // test: assigning zero count
    dest = d_string_new_from_cstr("original");

    if (dest)
    {
        result = d_string_assign_char(dest, 0, 'x');
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_char_zero_count",
            result && d_string_is_empty(dest),
            "should result in empty string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "assign_char_zero_count",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_assign_char(NULL, 5, 'x');
    group->elements[idx++] = D_ASSERT_FALSE(
        "assign_char_null_dest",
        result,
        "should return false for NULL destination");

    return group;
}


/******************************************************************************
 * II. APPEND TESTS
 *****************************************************************************/

/*
d_tests_sa_dstring_append
  Tests d_string_append function.
  Tests the following:
  - appending d_string to another
  - appending to empty string
  - appending empty string
  - NULL destination handling
  - NULL source handling
*/
struct d_test_object*
d_tests_sa_dstring_append
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_append", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: appending d_string to another
    dest = d_string_new_from_cstr("hello");
    src  = d_string_new_from_cstr(" world");

    if (dest && src)
    {
        result = d_string_append(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_basic",
            result && d_string_equals_cstr(dest, "hello world"),
            "should append ' world' to 'hello'");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_basic",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: appending to empty string
    dest = d_string_new();
    src  = d_string_new_from_cstr("content");

    if (dest && src)
    {
        result = d_string_append(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_to_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should append to empty string");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_to_empty",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: appending empty string
    dest = d_string_new_from_cstr("content");
    src  = d_string_new_from_cstr("");

    if (dest && src)
    {
        result = d_string_append(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_empty_string",
            result && d_string_equals_cstr(dest, "content"),
            "should remain unchanged after appending empty");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_empty_string",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: NULL destination handling
    src    = d_string_new_from_cstr("test");
    result = d_string_append(NULL, src);
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_null_dest",
        result,
        "should return false for NULL destination");

    d_string_free(src);

    // test: NULL source handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_append(dest, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_null_src",
        result,
        "should return false for NULL source");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_append_cstr
  Tests d_string_append_cstr function.
  Tests the following:
  - appending C string to d_string
  - appending to empty string
  - appending empty C string
  - NULL destination handling
  - NULL C string handling
*/
struct d_test_object*
d_tests_sa_dstring_append_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_append_cstr", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: appending C string to d_string
    dest = d_string_new_from_cstr("hello");

    if (dest)
    {
        result = d_string_append_cstr(dest, " world");
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_cstr_basic",
            result && d_string_equals_cstr(dest, "hello world"),
            "should append ' world' to 'hello'");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_cstr_basic",
            false,
            "failed to allocate test string");
    }

    // test: appending to empty string
    dest = d_string_new();

    if (dest)
    {
        result = d_string_append_cstr(dest, "content");
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_cstr_to_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should append to empty string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_cstr_to_empty",
            false,
            "failed to allocate test string");
    }

    // test: appending empty C string
    dest = d_string_new_from_cstr("content");

    if (dest)
    {
        result = d_string_append_cstr(dest, "");
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_cstr_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should remain unchanged after appending empty");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_cstr_empty",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_append_cstr(NULL, "test");
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_cstr_null_dest",
        result,
        "should return false for NULL destination");

    // test: NULL C string handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_append_cstr(dest, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_cstr_null_cstr",
        result,
        "should return false for NULL C string");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_append_buffer
  Tests d_string_append_buffer function.
  Tests the following:
  - appending buffer with length
  - appending partial buffer
  - appending zero-length buffer
  - NULL destination handling
  - NULL buffer handling
*/
struct d_test_object*
d_tests_sa_dstring_append_buffer
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    const char*           buffer;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_append_buffer", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: appending buffer with length
    dest   = d_string_new_from_cstr("hello");
    buffer = " world";

    if (dest)
    {
        result = d_string_append_buffer(dest, buffer, 6);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_buffer_basic",
            result && d_string_equals_cstr(dest, "hello world"),
            "should append buffer to dest");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_buffer_basic",
            false,
            "failed to allocate test string");
    }

    // test: appending partial buffer
    dest   = d_string_new_from_cstr("hello");
    buffer = " world!!!";

    if (dest)
    {
        result = d_string_append_buffer(dest, buffer, 6);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_buffer_partial",
            result && d_string_equals_cstr(dest, "hello world"),
            "should append partial buffer");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_buffer_partial",
            false,
            "failed to allocate test string");
    }

    // test: appending zero-length buffer
    dest   = d_string_new_from_cstr("hello");
    buffer = " world";

    if (dest)
    {
        result = d_string_append_buffer(dest, buffer, 0);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_buffer_zero_length",
            result && d_string_equals_cstr(dest, "hello"),
            "should remain unchanged after zero-length append");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_buffer_zero_length",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_append_buffer(NULL, "test", 4);
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_buffer_null_dest",
        result,
        "should return false for NULL destination");

    // test: NULL buffer handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_append_buffer(dest, NULL, 4);
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_buffer_null_buffer",
        result,
        "should return false for NULL buffer");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_append_char
  Tests d_string_append_char function.
  Tests the following:
  - appending single character
  - appending to empty string
  - appending multiple characters sequentially
  - NULL destination handling
*/
struct d_test_object*
d_tests_sa_dstring_append_char
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_append_char", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: appending single character
    dest = d_string_new_from_cstr("hello");

    if (dest)
    {
        result = d_string_append_char(dest, '!');
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_char_basic",
            result && d_string_equals_cstr(dest, "hello!"),
            "should append '!' to 'hello'");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_char_basic",
            false,
            "failed to allocate test string");
    }

    // test: appending to empty string
    dest = d_string_new();

    if (dest)
    {
        result = d_string_append_char(dest, 'a');
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_char_to_empty",
            result && d_string_equals_cstr(dest, "a"),
            "should append to empty string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_char_to_empty",
            false,
            "failed to allocate test string");
    }

    // test: appending multiple characters sequentially
    dest = d_string_new_from_cstr("ab");

    if (dest)
    {
        d_string_append_char(dest, 'c');
        d_string_append_char(dest, 'd');
        result = d_string_append_char(dest, 'e');
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_char_sequential",
            result && d_string_equals_cstr(dest, "abcde"),
            "should append multiple chars sequentially");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_char_sequential",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_append_char(NULL, 'x');
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_char_null_dest",
        result,
        "should return false for NULL destination");

    return group;
}

/*
d_tests_sa_dstring_append_formatted
  Tests d_string_append_formatted function.
  Tests the following:
  - appending formatted string with integers
  - appending formatted string with strings
  - appending formatted string with mixed types
  - NULL destination handling
*/
struct d_test_object*
d_tests_sa_dstring_append_formatted
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_append_formatted", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: appending formatted string with integers
    dest = d_string_new_from_cstr("value: ");

    if (dest)
    {
        result = d_string_append_formatted(dest, "%d", 42);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_formatted_int",
            result && d_string_equals_cstr(dest, "value: 42"),
            "should append formatted integer");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_formatted_int",
            false,
            "failed to allocate test string");
    }

    // test: appending formatted string with strings
    dest = d_string_new_from_cstr("hello ");

    if (dest)
    {
        result = d_string_append_formatted(dest, "%s", "world");
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_formatted_string",
            result && d_string_equals_cstr(dest, "hello world"),
            "should append formatted string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_formatted_string",
            false,
            "failed to allocate test string");
    }

    // test: appending formatted string with mixed types
    dest = d_string_new_from_cstr("result: ");

    if (dest)
    {
        result = d_string_append_formatted(dest, "%s=%d", "count", 10);
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_formatted_mixed",
            result && d_string_equals_cstr(dest, "result: count=10"),
            "should append formatted mixed types");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "append_formatted_mixed",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_append_formatted(NULL, "%d", 42);
    group->elements[idx++] = D_ASSERT_FALSE(
        "append_formatted_null_dest",
        result,
        "should return false for NULL destination");

    return group;
}


/******************************************************************************
 * III. PREPEND TESTS
 *****************************************************************************/

/*
d_tests_sa_dstring_prepend
  Tests d_string_prepend function.
  Tests the following:
  - prepending d_string to another
  - prepending to empty string
  - prepending empty string
  - NULL destination handling
  - NULL source handling
*/
struct d_test_object*
d_tests_sa_dstring_prepend
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_prepend", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: prepending d_string to another
    dest = d_string_new_from_cstr("world");
    src  = d_string_new_from_cstr("hello ");

    if (dest && src)
    {
        result = d_string_prepend(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_basic",
            result && d_string_equals_cstr(dest, "hello world"),
            "should prepend 'hello ' to 'world'");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_basic",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: prepending to empty string
    dest = d_string_new();
    src  = d_string_new_from_cstr("content");

    if (dest && src)
    {
        result = d_string_prepend(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_to_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should prepend to empty string");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_to_empty",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: prepending empty string
    dest = d_string_new_from_cstr("content");
    src  = d_string_new_from_cstr("");

    if (dest && src)
    {
        result = d_string_prepend(dest, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_empty_string",
            result && d_string_equals_cstr(dest, "content"),
            "should remain unchanged after prepending empty");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_empty_string",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: NULL destination handling
    src    = d_string_new_from_cstr("test");
    result = d_string_prepend(NULL, src);
    group->elements[idx++] = D_ASSERT_FALSE(
        "prepend_null_dest",
        result,
        "should return false for NULL destination");

    d_string_free(src);

    // test: NULL source handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_prepend(dest, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "prepend_null_src",
        result,
        "should return false for NULL source");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_prepend_cstr
  Tests d_string_prepend_cstr function.
  Tests the following:
  - prepending C string to d_string
  - prepending to empty string
  - prepending empty C string
  - NULL destination handling
  - NULL C string handling
*/
struct d_test_object*
d_tests_sa_dstring_prepend_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_prepend_cstr", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: prepending C string to d_string
    dest = d_string_new_from_cstr("world");

    if (dest)
    {
        result = d_string_prepend_cstr(dest, "hello ");
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_cstr_basic",
            result && d_string_equals_cstr(dest, "hello world"),
            "should prepend 'hello ' to 'world'");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_cstr_basic",
            false,
            "failed to allocate test string");
    }

    // test: prepending to empty string
    dest = d_string_new();

    if (dest)
    {
        result = d_string_prepend_cstr(dest, "content");
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_cstr_to_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should prepend to empty string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_cstr_to_empty",
            false,
            "failed to allocate test string");
    }

    // test: prepending empty C string
    dest = d_string_new_from_cstr("content");

    if (dest)
    {
        result = d_string_prepend_cstr(dest, "");
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_cstr_empty",
            result && d_string_equals_cstr(dest, "content"),
            "should remain unchanged after prepending empty");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_cstr_empty",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_prepend_cstr(NULL, "test");
    group->elements[idx++] = D_ASSERT_FALSE(
        "prepend_cstr_null_dest",
        result,
        "should return false for NULL destination");

    // test: NULL C string handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_prepend_cstr(dest, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "prepend_cstr_null_cstr",
        result,
        "should return false for NULL C string");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_prepend_char
  Tests d_string_prepend_char function.
  Tests the following:
  - prepending single character
  - prepending to empty string
  - prepending multiple characters sequentially
  - NULL destination handling
*/
struct d_test_object*
d_tests_sa_dstring_prepend_char
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_prepend_char", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: prepending single character
    dest = d_string_new_from_cstr("ello");

    if (dest)
    {
        result = d_string_prepend_char(dest, 'h');
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_char_basic",
            result && d_string_equals_cstr(dest, "hello"),
            "should prepend 'h' to 'ello'");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_char_basic",
            false,
            "failed to allocate test string");
    }

    // test: prepending to empty string
    dest = d_string_new();

    if (dest)
    {
        result = d_string_prepend_char(dest, 'a');
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_char_to_empty",
            result && d_string_equals_cstr(dest, "a"),
            "should prepend to empty string");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_char_to_empty",
            false,
            "failed to allocate test string");
    }

    // test: prepending multiple characters sequentially
    dest = d_string_new_from_cstr("de");

    if (dest)
    {
        d_string_prepend_char(dest, 'c');
        d_string_prepend_char(dest, 'b');
        result = d_string_prepend_char(dest, 'a');
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_char_sequential",
            result && d_string_equals_cstr(dest, "abcde"),
            "should prepend multiple chars sequentially");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "prepend_char_sequential",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_prepend_char(NULL, 'x');
    group->elements[idx++] = D_ASSERT_FALSE(
        "prepend_char_null_dest",
        result,
        "should return false for NULL destination");

    return group;
}


/******************************************************************************
 * IV. INSERT TESTS
 *****************************************************************************/

/*
d_tests_sa_dstring_insert
  Tests d_string_insert function.
  Tests the following:
  - inserting at beginning (index 0)
  - inserting at end
  - inserting in middle
  - inserting empty string
  - NULL destination handling
  - NULL source handling
*/
struct d_test_object*
d_tests_sa_dstring_insert
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_insert", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: inserting at beginning
    dest = d_string_new_from_cstr("world");
    src  = d_string_new_from_cstr("hello ");

    if (dest && src)
    {
        result = d_string_insert(dest, 0, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_at_beginning",
            result && d_string_equals_cstr(dest, "hello world"),
            "should insert at beginning");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_at_beginning",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: inserting at end
    dest = d_string_new_from_cstr("hello");
    src  = d_string_new_from_cstr(" world");

    if (dest && src)
    {
        result = d_string_insert(dest, 5, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_at_end",
            result && d_string_equals_cstr(dest, "hello world"),
            "should insert at end");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_at_end",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: inserting in middle
    dest = d_string_new_from_cstr("helloworld");
    src  = d_string_new_from_cstr(" ");

    if (dest && src)
    {
        result = d_string_insert(dest, 5, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_in_middle",
            result && d_string_equals_cstr(dest, "hello world"),
            "should insert in middle");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_in_middle",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: inserting empty string
    dest = d_string_new_from_cstr("hello");
    src  = d_string_new_from_cstr("");

    if (dest && src)
    {
        result = d_string_insert(dest, 2, src);
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_empty_string",
            result && d_string_equals_cstr(dest, "hello"),
            "should remain unchanged after inserting empty");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_empty_string",
            false,
            "failed to allocate test strings");
    }

    d_string_free(dest);
    d_string_free(src);

    // test: NULL destination handling
    src    = d_string_new_from_cstr("test");
    result = d_string_insert(NULL, 0, src);
    group->elements[idx++] = D_ASSERT_FALSE(
        "insert_null_dest",
        result,
        "should return false for NULL destination");

    d_string_free(src);

    // test: NULL source handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_insert(dest, 0, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "insert_null_src",
        result,
        "should return false for NULL source");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_insert_cstr
  Tests d_string_insert_cstr function.
  Tests the following:
  - inserting C string at beginning
  - inserting C string at end
  - inserting C string in middle
  - NULL destination handling
  - NULL C string handling
*/
struct d_test_object*
d_tests_sa_dstring_insert_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_insert_cstr", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: inserting C string at beginning
    dest = d_string_new_from_cstr("world");

    if (dest)
    {
        result = d_string_insert_cstr(dest, 0, "hello ");
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_cstr_at_beginning",
            result && d_string_equals_cstr(dest, "hello world"),
            "should insert at beginning");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_cstr_at_beginning",
            false,
            "failed to allocate test string");
    }

    // test: inserting C string at end
    dest = d_string_new_from_cstr("hello");

    if (dest)
    {
        result = d_string_insert_cstr(dest, 5, " world");
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_cstr_at_end",
            result && d_string_equals_cstr(dest, "hello world"),
            "should insert at end");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_cstr_at_end",
            false,
            "failed to allocate test string");
    }

    // test: inserting C string in middle
    dest = d_string_new_from_cstr("helloworld");

    if (dest)
    {
        result = d_string_insert_cstr(dest, 5, " ");
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_cstr_in_middle",
            result && d_string_equals_cstr(dest, "hello world"),
            "should insert in middle");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_cstr_in_middle",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_insert_cstr(NULL, 0, "test");
    group->elements[idx++] = D_ASSERT_FALSE(
        "insert_cstr_null_dest",
        result,
        "should return false for NULL destination");

    // test: NULL C string handling
    dest   = d_string_new_from_cstr("test");
    result = d_string_insert_cstr(dest, 0, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "insert_cstr_null_cstr",
        result,
        "should return false for NULL C string");

    d_string_free(dest);

    return group;
}

/*
d_tests_sa_dstring_insert_char
  Tests d_string_insert_char function.
  Tests the following:
  - inserting char at beginning
  - inserting char at end
  - inserting char in middle
  - NULL destination handling
*/
struct d_test_object*
d_tests_sa_dstring_insert_char
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_insert_char", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: inserting char at beginning
    dest = d_string_new_from_cstr("ello");

    if (dest)
    {
        result = d_string_insert_char(dest, 0, 'h');
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_char_at_beginning",
            result && d_string_equals_cstr(dest, "hello"),
            "should insert at beginning");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_char_at_beginning",
            false,
            "failed to allocate test string");
    }

    // test: inserting char at end
    dest = d_string_new_from_cstr("hell");

    if (dest)
    {
        result = d_string_insert_char(dest, 4, 'o');
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_char_at_end",
            result && d_string_equals_cstr(dest, "hello"),
            "should insert at end");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_char_at_end",
            false,
            "failed to allocate test string");
    }

    // test: inserting char in middle
    dest = d_string_new_from_cstr("hllo");

    if (dest)
    {
        result = d_string_insert_char(dest, 1, 'e');
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_char_in_middle",
            result && d_string_equals_cstr(dest, "hello"),
            "should insert in middle");

        d_string_free(dest);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "insert_char_in_middle",
            false,
            "failed to allocate test string");
    }

    // test: NULL destination handling
    result = d_string_insert_char(NULL, 0, 'x');
    group->elements[idx++] = D_ASSERT_FALSE(
        "insert_char_null_dest",
        result,
        "should return false for NULL destination");

    return group;
}


/******************************************************************************
 * V. ERASE AND CLEAR TESTS
 *****************************************************************************/

/*
d_tests_sa_dstring_erase
  Tests d_string_erase function.
  Tests the following:
  - erasing from beginning
  - erasing from end
  - erasing from middle
  - erasing entire string
  - erasing zero characters
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_erase
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_erase", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: erasing from beginning
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_erase(str, 0, 6);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_from_beginning",
            result && d_string_equals_cstr(str, "world"),
            "should erase 'hello ' from beginning");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_from_beginning",
            false,
            "failed to allocate test string");
    }

    // test: erasing from end
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_erase(str, 5, 6);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_from_end",
            result && d_string_equals_cstr(str, "hello"),
            "should erase ' world' from end");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_from_end",
            false,
            "failed to allocate test string");
    }

    // test: erasing from middle
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_erase(str, 5, 1);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_from_middle",
            result && d_string_equals_cstr(str, "helloworld"),
            "should erase space from middle");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_from_middle",
            false,
            "failed to allocate test string");
    }

    // test: erasing entire string
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_erase(str, 0, 5);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_entire_string",
            result && d_string_is_empty(str),
            "should result in empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_entire_string",
            false,
            "failed to allocate test string");
    }

    // test: erasing zero characters
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_erase(str, 2, 0);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_zero_chars",
            result && d_string_equals_cstr(str, "hello"),
            "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_zero_chars",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_erase(NULL, 0, 1);
    group->elements[idx++] = D_ASSERT_FALSE(
        "erase_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_erase_char
  Tests d_string_erase_char function.
  Tests the following:
  - erasing char at beginning
  - erasing char at end
  - erasing char in middle
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_erase_char
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_erase_char", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: erasing char at beginning
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_erase_char(str, 0);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_char_at_beginning",
            result && d_string_equals_cstr(str, "ello"),
            "should erase first char");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_char_at_beginning",
            false,
            "failed to allocate test string");
    }

    // test: erasing char at end
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_erase_char(str, 4);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_char_at_end",
            result && d_string_equals_cstr(str, "hell"),
            "should erase last char");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_char_at_end",
            false,
            "failed to allocate test string");
    }

    // test: erasing char in middle
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_erase_char(str, 2);
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_char_in_middle",
            result && d_string_equals_cstr(str, "helo"),
            "should erase middle char");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "erase_char_in_middle",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_erase_char(NULL, 0);
    group->elements[idx++] = D_ASSERT_FALSE(
        "erase_char_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_clear
  Tests d_string_clear function.
  Tests the following:
  - clearing non-empty string
  - clearing empty string
  - NULL string handling (should not crash)
*/
struct d_test_object*
d_tests_sa_dstring_clear
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                idx;

    group = d_test_object_new_interior("d_string_clear", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: clearing non-empty string
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        d_string_clear(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "clear_non_empty",
            d_string_is_empty(str),
            "should result in empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "clear_non_empty",
            false,
            "failed to allocate test string");
    }

    // test: clearing empty string
    str = d_string_new();

    if (str)
    {
        d_string_clear(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "clear_empty",
            d_string_is_empty(str),
            "should remain empty");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "clear_empty",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling (should not crash)
    d_string_clear(NULL);
    group->elements[idx++] = D_ASSERT_TRUE(
        "clear_null_string",
        true,
        "should handle NULL without crashing");

    return group;
}


/******************************************************************************
 * VI. REPLACE TESTS
 *****************************************************************************/

/*
d_tests_sa_dstring_replace
  Tests d_string_replace function.
  Tests the following:
  - replacing at beginning
  - replacing at end
  - replacing in middle
  - replacing with shorter string
  - replacing with longer string
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_replace
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string*      replacement;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_replace", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: replacing at beginning
    str         = d_string_new_from_cstr("hello world");
    replacement = d_string_new_from_cstr("hi");

    if (str && replacement)
    {
        result = d_string_replace(str, 0, 5, replacement);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_at_beginning",
            result && d_string_equals_cstr(str, "hi world"),
            "should replace 'hello' with 'hi'");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_at_beginning",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(replacement);

    // test: replacing at end
    str         = d_string_new_from_cstr("hello world");
    replacement = d_string_new_from_cstr("there");

    if (str && replacement)
    {
        result = d_string_replace(str, 6, 5, replacement);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_at_end",
            result && d_string_equals_cstr(str, "hello there"),
            "should replace 'world' with 'there'");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_at_end",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(replacement);

    // test: replacing in middle
    str         = d_string_new_from_cstr("hello world");
    replacement = d_string_new_from_cstr("_");

    if (str && replacement)
    {
        result = d_string_replace(str, 5, 1, replacement);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_in_middle",
            result && d_string_equals_cstr(str, "hello_world"),
            "should replace space with underscore");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_in_middle",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(replacement);

    // test: replacing with shorter string
    str         = d_string_new_from_cstr("hello world");
    replacement = d_string_new_from_cstr("x");

    if (str && replacement)
    {
        result = d_string_replace(str, 0, 5, replacement);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_with_shorter",
            result && d_string_equals_cstr(str, "x world"),
            "should replace with shorter string");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_with_shorter",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(replacement);

    // test: replacing with longer string
    str         = d_string_new_from_cstr("hello world");
    replacement = d_string_new_from_cstr("greetings");

    if (str && replacement)
    {
        result = d_string_replace(str, 0, 5, replacement);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_with_longer",
            result && d_string_equals_cstr(str, "greetings world"),
            "should replace with longer string");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_with_longer",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(replacement);

    // test: NULL string handling
    replacement = d_string_new_from_cstr("test");
    result      = d_string_replace(NULL, 0, 1, replacement);
    group->elements[idx++] = D_ASSERT_FALSE(
        "replace_null_string",
        result,
        "should return false for NULL string");

    d_string_free(replacement);

    return group;
}

/*
d_tests_sa_dstring_replace_cstr
  Tests d_string_replace_cstr function.
  Tests the following:
  - replacing with C string at beginning
  - replacing with C string in middle
  - replacing with C string at end
  - NULL string handling
  - NULL C string handling
*/
struct d_test_object*
d_tests_sa_dstring_replace_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_replace_cstr", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: replacing with C string at beginning
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_replace_cstr(str, 0, 5, "hi");
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_cstr_at_beginning",
            result && d_string_equals_cstr(str, "hi world"),
            "should replace 'hello' with 'hi'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_cstr_at_beginning",
            false,
            "failed to allocate test string");
    }

    // test: replacing with C string in middle
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_replace_cstr(str, 5, 1, "_");
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_cstr_in_middle",
            result && d_string_equals_cstr(str, "hello_world"),
            "should replace space with underscore");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_cstr_in_middle",
            false,
            "failed to allocate test string");
    }

    // test: replacing with C string at end
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_replace_cstr(str, 6, 5, "there");
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_cstr_at_end",
            result && d_string_equals_cstr(str, "hello there"),
            "should replace 'world' with 'there'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_cstr_at_end",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_replace_cstr(NULL, 0, 1, "test");
    group->elements[idx++] = D_ASSERT_FALSE(
        "replace_cstr_null_string",
        result,
        "should return false for NULL string");

    // test: NULL C string handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_replace_cstr(str, 0, 1, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "replace_cstr_null_cstr",
        result,
        "should return false for NULL C string");

    d_string_free(str);

    return group;
}

/*
d_tests_sa_dstring_replace_all
  Tests d_string_replace_all function.
  Tests the following:
  - replacing all occurrences
  - replacing when pattern appears once
  - replacing when pattern not found
  - replacing overlapping patterns
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_replace_all
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string*      old_str;
    struct d_string*      new_str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_replace_all", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: replacing all occurrences
    str     = d_string_new_from_cstr("hello hello hello");
    old_str = d_string_new_from_cstr("hello");
    new_str = d_string_new_from_cstr("hi");

    if (str && old_str && new_str)
    {
        result = d_string_replace_all(str, old_str, new_str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_multiple",
            result && d_string_equals_cstr(str, "hi hi hi"),
            "should replace all occurrences");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_multiple",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(old_str);
    d_string_free(new_str);

    // test: replacing when pattern appears once
    str     = d_string_new_from_cstr("hello world");
    old_str = d_string_new_from_cstr("world");
    new_str = d_string_new_from_cstr("there");

    if (str && old_str && new_str)
    {
        result = d_string_replace_all(str, old_str, new_str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_single",
            result && d_string_equals_cstr(str, "hello there"),
            "should replace single occurrence");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_single",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(old_str);
    d_string_free(new_str);

    // test: replacing when pattern not found
    str     = d_string_new_from_cstr("hello world");
    old_str = d_string_new_from_cstr("xyz");
    new_str = d_string_new_from_cstr("abc");

    if (str && old_str && new_str)
    {
        result = d_string_replace_all(str, old_str, new_str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_not_found",
            result && d_string_equals_cstr(str, "hello world"),
            "should remain unchanged when pattern not found");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_not_found",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(old_str);
    d_string_free(new_str);

    // test: replacing with empty string (deletion)
    str     = d_string_new_from_cstr("a-b-c-d");
    old_str = d_string_new_from_cstr("-");
    new_str = d_string_new_from_cstr("");

    if (str && old_str && new_str)
    {
        result = d_string_replace_all(str, old_str, new_str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_with_empty",
            result && d_string_equals_cstr(str, "abcd"),
            "should delete all occurrences");
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_with_empty",
            false,
            "failed to allocate test strings");
    }

    d_string_free(str);
    d_string_free(old_str);
    d_string_free(new_str);

    // test: NULL string handling
    old_str = d_string_new_from_cstr("old");
    new_str = d_string_new_from_cstr("new");
    result  = d_string_replace_all(NULL, old_str, new_str);
    group->elements[idx++] = D_ASSERT_FALSE(
        "replace_all_null_string",
        result,
        "should return false for NULL string");

    d_string_free(old_str);
    d_string_free(new_str);

    return group;
}

/*
d_tests_sa_dstring_replace_all_cstr
  Tests d_string_replace_all_cstr function.
  Tests the following:
  - replacing all occurrences with C strings
  - replacing when pattern appears once
  - replacing when pattern not found
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_replace_all_cstr
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_replace_all_cstr", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: replacing all occurrences with C strings
    str = d_string_new_from_cstr("hello hello hello");

    if (str)
    {
        result = d_string_replace_all_cstr(str, "hello", "hi");
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_cstr_multiple",
            result && d_string_equals_cstr(str, "hi hi hi"),
            "should replace all occurrences");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_cstr_multiple",
            false,
            "failed to allocate test string");
    }

    // test: replacing when pattern appears once
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_replace_all_cstr(str, "world", "there");
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_cstr_single",
            result && d_string_equals_cstr(str, "hello there"),
            "should replace single occurrence");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_cstr_single",
            false,
            "failed to allocate test string");
    }

    // test: replacing when pattern not found
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_replace_all_cstr(str, "xyz", "abc");
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_cstr_not_found",
            result && d_string_equals_cstr(str, "hello world"),
            "should remain unchanged when pattern not found");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_all_cstr_not_found",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_replace_all_cstr(NULL, "old", "new");
    group->elements[idx++] = D_ASSERT_FALSE(
        "replace_all_cstr_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_replace_char
  Tests d_string_replace_char function.
  Tests the following:
  - replacing all occurrences of a character
  - replacing when character appears once
  - replacing when character not found
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_replace_char
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_replace_char", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: replacing all occurrences of a character
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_replace_char(str, 'l', 'x');
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_char_multiple",
            result && d_string_equals_cstr(str, "hexxo"),
            "should replace all 'l' with 'x'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_char_multiple",
            false,
            "failed to allocate test string");
    }

    // test: replacing when character appears once
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_replace_char(str, 'h', 'j');
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_char_single",
            result && d_string_equals_cstr(str, "jello"),
            "should replace 'h' with 'j'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_char_single",
            false,
            "failed to allocate test string");
    }

    // test: replacing when character not found
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_replace_char(str, 'z', 'x');
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_char_not_found",
            result && d_string_equals_cstr(str, "hello"),
            "should remain unchanged when char not found");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "replace_char_not_found",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_replace_char(NULL, 'a', 'b');
    group->elements[idx++] = D_ASSERT_FALSE(
        "replace_char_null_string",
        result,
        "should return false for NULL string");

    return group;
}


/******************************************************************************
 * VII. MODIFY ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_sa_dstring_modify_all
  Runs all modification function tests for dstring module.
  Tests the following:
  - assignment functions (assign, assign_cstr, assign_buffer, assign_char)
  - append functions (append, append_cstr, append_buffer, append_char, 
    append_formatted)
  - prepend functions (prepend, prepend_cstr, prepend_char)
  - insert functions (insert, insert_cstr, insert_char)
  - erase functions (erase, erase_char, clear)
  - replace functions (replace, replace_cstr, replace_all, replace_all_cstr,
    replace_char)
*/
struct d_test_object*
d_tests_sa_dstring_modify_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Modification Functions", 23);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // I. assignment tests
    group->elements[idx++] = d_tests_sa_dstring_assign();
    group->elements[idx++] = d_tests_sa_dstring_assign_cstr();
    group->elements[idx++] = d_tests_sa_dstring_assign_buffer();
    group->elements[idx++] = d_tests_sa_dstring_assign_char();

    // II. append tests
    group->elements[idx++] = d_tests_sa_dstring_append();
    group->elements[idx++] = d_tests_sa_dstring_append_cstr();
    group->elements[idx++] = d_tests_sa_dstring_append_buffer();
    group->elements[idx++] = d_tests_sa_dstring_append_char();
    group->elements[idx++] = d_tests_sa_dstring_append_formatted();

    // III. prepend tests
    group->elements[idx++] = d_tests_sa_dstring_prepend();
    group->elements[idx++] = d_tests_sa_dstring_prepend_cstr();
    group->elements[idx++] = d_tests_sa_dstring_prepend_char();

    // IV. insert tests
    group->elements[idx++] = d_tests_sa_dstring_insert();
    group->elements[idx++] = d_tests_sa_dstring_insert_cstr();
    group->elements[idx++] = d_tests_sa_dstring_insert_char();

    // V. erase and clear tests
    group->elements[idx++] = d_tests_sa_dstring_erase();
    group->elements[idx++] = d_tests_sa_dstring_erase_char();
    group->elements[idx++] = d_tests_sa_dstring_clear();

    // VI. replace tests
    group->elements[idx++] = d_tests_sa_dstring_replace();
    group->elements[idx++] = d_tests_sa_dstring_replace_cstr();
    group->elements[idx++] = d_tests_sa_dstring_replace_all();
    group->elements[idx++] = d_tests_sa_dstring_replace_all_cstr();
    group->elements[idx++] = d_tests_sa_dstring_replace_char();

    return group;
}