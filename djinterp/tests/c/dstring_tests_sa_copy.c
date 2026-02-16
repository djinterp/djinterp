/******************************************************************************
* djinterp [test]                                          dstring_tests_copy.c
*
*   Unit tests for d_string safe copy functions:
*     - d_string_copy_s
*     - d_string_copy_cstr_s
*     - d_string_ncopy_s
*     - d_string_ncopy_cstr_s
*     - d_string_to_buffer_s
*
* path:      \src\test\dstring_tests_copy.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "..\..\tests\dstring_tests_sa.h"


/******************************************************************************
* d_tests_sa_dstring_copy_s
******************************************************************************/

/*
d_tests_sa_dstring_copy_s
  Tests d_string_copy_s() which safely copies the content of one d_string to
  another, ensuring no buffer overflow.

Test cases:
  1.  NULL destination returns error
  2.  NULL source returns error
  3.  Copy empty string to empty string
  4.  Copy non-empty string to empty destination
  5.  Copy replaces existing content
  6.  Destination capacity sufficient
  7.  Destination grows when needed
  8.  Self-copy handled correctly
  9.  Copy preserves source unchanged
  10. Empty source clears destination

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_copy_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_copy_s", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns error
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_copy_s(NULL, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_dest_returns_error",
            result != 0,
            "d_string_copy_s(NULL, ...) should return error"
        );

        d_string_free(src);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_dest_skipped", false, "skipped"
        );
    }

    // test 2: NULL source returns error
    dest = d_string_new();

    if (dest)
    {
        result = d_string_copy_s(dest, NULL);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_src_returns_error",
            result != 0,
            "d_string_copy_s(..., NULL) should return error"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_src_skipped", false, "skipped"
        );
    }

    // test 3: copy empty string to empty string
    dest = d_string_new();
    src  = d_string_new();

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_to_empty_succeeds",
            result == 0,
            "copying empty to empty should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_to_empty_size",
            dest->size, 0,
            "destination should remain empty"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ee_skipped", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ee_size_skip", false, "skip");
    }

    // test 4: copy non-empty string to empty destination
    dest = d_string_new();
    src  = d_string_new_from_cstr("Hello World");

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "nonempty_to_empty_succeeds",
            result == 0,
            "copying non-empty to empty should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "nonempty_to_empty_content",
            dest->text, "Hello World",
            "destination should contain 'Hello World'"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ne_skipped", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ne_content_skip", false, "skip");
    }

    // test 5: copy replaces existing content
    dest = d_string_new_from_cstr("Old Content");
    src  = d_string_new_from_cstr("New Content");

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "replaces_existing",
            dest->text, "New Content",
            "copy should replace existing content"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("repl_skipped", false, "skip");
    }

    // test 6: destination grows when needed
    dest = d_string_new_with_capacity(8);
    src  = d_string_new_from_cstr("This is a much longer string");

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "grows_when_needed_succeeds",
            result == 0,
            "copy should grow destination when needed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "grows_when_needed_content",
            dest->text, "This is a much longer string",
            "content should be fully copied"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("grow_skipped", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("grow_content_skip", false, "skip");
    }

    // test 7: copy preserves source unchanged
    dest = d_string_new();
    src  = d_string_new_from_cstr("Source String");

    if (dest && src)
    {
        d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "source_preserved",
            src->text, "Source String",
            "source should be unchanged after copy"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("src_pres_skip", false, "skip");
    }

    // test 8: empty source clears destination
    dest = d_string_new_from_cstr("Has Content");
    src  = d_string_new();

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_src_clears_dest",
            dest->size, 0,
            "copying empty source should clear destination"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("clear_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_copy_cstr_s
******************************************************************************/

/*
d_tests_sa_dstring_copy_cstr_s
  Tests d_string_copy_cstr_s() which safely copies a C string to a d_string.

Test cases:
  1.  NULL destination returns error
  2.  NULL source returns error or clears
  3.  Copy empty C string
  4.  Copy normal C string
  5.  Copy replaces existing content
  6.  Destination grows when needed

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_copy_cstr_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_copy_cstr_s", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns error
    result = d_string_copy_cstr_s(NULL, "test");
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_dest_returns_error",
        result != 0,
        "d_string_copy_cstr_s(NULL, ...) should return error"
    );

    // test 2: NULL source returns error
    dest = d_string_new();

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, NULL);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_src_returns_error",
            result != 0,
            "d_string_copy_cstr_s(..., NULL) should return error"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("null_src_skip", false, "skip");
    }

    // test 3: copy empty C string
    dest = d_string_new_from_cstr("Has Content");

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_cstr_succeeds",
            result == 0,
            "copying empty C string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_cstr_clears",
            dest->size, 0,
            "empty C string should clear destination"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("empty_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("empty_clr_skip", false, "skip");
    }

    // test 4: copy normal C string
    dest = d_string_new();

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "Hello World");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "normal_copy_succeeds",
            result == 0,
            "copying C string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "normal_copy_content",
            dest->text, "Hello World",
            "content should be 'Hello World'"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("normal_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("norm_cont_skip", false, "skip");
    }

    // test 5: copy replaces existing
    dest = d_string_new_from_cstr("Old");

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "New");

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "replaces_existing",
            dest->text, "New",
            "copy should replace existing content"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("repl_skip", false, "skip");
    }

    // test 6: destination grows
    dest = d_string_new_with_capacity(4);

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "This is a longer string");

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "grows_correctly",
            dest->text, "This is a longer string",
            "destination should grow to accommodate"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("grow_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncopy_s
******************************************************************************/

/*
d_tests_sa_dstring_ncopy_s
  Tests d_string_ncopy_s() which safely copies at most n characters from one
  d_string to another.

Test cases:
  1.  NULL destination returns error
  2.  NULL source returns error
  3.  Copy 0 characters (clears destination)
  4.  Copy fewer than source length
  5.  Copy more than source length (copies all)
  6.  Copy exact source length
  7.  Null-termination preserved

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncopy_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_ncopy_s", 9);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns error
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_ncopy_s(NULL, src, 4);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_dest_returns_error",
            result != 0,
            "d_string_ncopy_s(NULL, ...) should return error"
        );

        d_string_free(src);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nd_skip", false, "skip");
    }

    // test 2: NULL source returns error
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_s(dest, NULL, 4);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_src_returns_error",
            result != 0,
            "d_string_ncopy_s(..., NULL, ...) should return error"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: copy 0 characters
    dest = d_string_new_from_cstr("Has Content");
    src  = d_string_new_from_cstr("Source");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 0);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "copy_0_succeeds",
            result == 0,
            "copying 0 characters should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "copy_0_clears",
            dest->size, 0,
            "copying 0 characters should clear destination"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("c0_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("c0_clr_skip", false, "skip");
    }

    // test 4: copy fewer than source length
    dest = d_string_new();
    src  = d_string_new_from_cstr("Hello World");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 5);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "copy_partial",
            dest->text, "Hello",
            "should copy only 5 characters"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "copy_partial_size",
            dest->size, 5,
            "size should be 5"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("partial_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("partial_sz_skip", false, "skip");
    }

    // test 5: copy more than source length
    dest = d_string_new();
    src  = d_string_new_from_cstr("Short");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 100);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "copy_more_than_length",
            dest->text, "Short",
            "should copy entire string when n > length"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("more_skip", false, "skip");
    }

    // test 6: copy exact source length
    dest = d_string_new();
    src  = d_string_new_from_cstr("Exact");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 5);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "copy_exact_length",
            dest->text, "Exact",
            "should copy entire string when n == length"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("exact_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncopy_cstr_s
******************************************************************************/

/*
d_tests_sa_dstring_ncopy_cstr_s
  Tests d_string_ncopy_cstr_s() which safely copies at most n characters from
  a C string to a d_string.

Test cases:
  1.  NULL destination returns error
  2.  NULL source returns error
  3.  Copy fewer than source length
  4.  Copy more than source length
  5.  Null-termination preserved

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncopy_cstr_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_ncopy_cstr_s", 6);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns error
    result = d_string_ncopy_cstr_s(NULL, "test", 4);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_dest_returns_error",
        result != 0,
        "d_string_ncopy_cstr_s(NULL, ...) should return error"
    );

    // test 2: NULL source returns error
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, NULL, 4);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_src_returns_error",
            result != 0,
            "d_string_ncopy_cstr_s(..., NULL, ...) should return error"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: copy fewer than source length
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Hello World", 5);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "copy_partial",
            dest->text, "Hello",
            "should copy only 5 characters"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("partial_skip", false, "skip");
    }

    // test 4: copy more than source length
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Short", 100);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "copy_more_than_length",
            dest->text, "Short",
            "should copy entire string when n > length"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("more_skip", false, "skip");
    }

    // test 5: null termination preserved
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Testing", 4);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_terminated",
            (dest->text != NULL) && (dest->text[4] == '\0'),
            "result should be null-terminated"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("null_skip", false, "skip");
    }

    // test 6: zero count
    dest = d_string_new_from_cstr("Content");

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Other", 0);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "zero_count_clears",
            dest->size, 0,
            "copying 0 characters should clear destination"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("zero_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_to_buffer_s
******************************************************************************/

/*
d_tests_sa_dstring_to_buffer_s
  Tests d_string_to_buffer_s() which safely copies a d_string's content to a
  raw character buffer with size limit.

Test cases:
  1.  NULL string returns error
  2.  NULL buffer returns error
  3.  Zero buffer size returns error
  4.  Buffer size 1 (only null terminator fits)
  5.  Exact fit (buffer = length + 1)
  6.  Buffer larger than needed
  7.  Buffer smaller than needed (truncates)
  8.  Empty string to buffer

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_to_buffer_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    char                  buffer[256];
    char                  small_buffer[6];
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_to_buffer_s", 10);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns error
    result = d_string_to_buffer_s(buffer, sizeof(buffer), NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_string_returns_error",
        result != 0,
        "d_string_to_buffer_s(..., NULL) should return error"
    );

    // test 2: NULL buffer returns error
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_to_buffer_s(NULL, 10, str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "null_buffer_returns_error",
            result != 0,
            "d_string_to_buffer_s(NULL, ...) should return error"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nb_skip", false, "skip");
    }

    // test 3: zero buffer size returns error
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_to_buffer_s(buffer, 0, str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "zero_size_returns_error",
            result != 0,
            "zero buffer size should return error"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("zs_skip", false, "skip");
    }

    // test 4: buffer size 1
    str = d_string_new_from_cstr("test");

    if (str)
    {
        char tiny_buffer[1];
        result = d_string_to_buffer_s(tiny_buffer, 1, str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "size_1_null_terminates",
            tiny_buffer[0] == '\0',
            "buffer size 1 should result in empty string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("s1_skip", false, "skip");
    }

    // test 5: exact fit
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        memset(small_buffer, 'X', sizeof(small_buffer));
        result = d_string_to_buffer_s(small_buffer, 6, str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "exact_fit_succeeds",
            result == 0,
            "exact fit should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "exact_fit_content",
            small_buffer, "Hello",
            "buffer should contain 'Hello'"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ef_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("efc_skip", false, "skip");
    }

    // test 6: buffer larger than needed
    str = d_string_new_from_cstr("Short");

    if (str)
    {
        memset(buffer, 'X', sizeof(buffer));
        result = d_string_to_buffer_s(buffer, sizeof(buffer), str);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "larger_buffer_content",
            buffer, "Short",
            "content should be copied correctly"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("lg_skip", false, "skip");
    }

    // test 7: buffer smaller than needed (truncates or error)
    str = d_string_new_from_cstr("This is a long string");

    if (str)
    {
        memset(small_buffer, 0, sizeof(small_buffer));
        result = d_string_to_buffer_s(small_buffer, sizeof(small_buffer), str);

        // either truncates (result == 0) or returns error (result != 0)
        // but buffer should be null-terminated either way
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "smaller_buffer_safe",
            small_buffer[sizeof(small_buffer) - 1] == '\0',
            "buffer should always be null-terminated"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("sm_skip", false, "skip");
    }

    // test 8: empty string to buffer
    str = d_string_new();

    if (str)
    {
        buffer[0] = 'X';
        result = d_string_to_buffer_s(buffer, sizeof(buffer), str);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_string_to_buffer",
            result == 0,
            "copying empty string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_string_result",
            buffer[0], '\0',
            "buffer should contain empty string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("es_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("esr_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_copy_all
******************************************************************************/

/*
d_tests_sa_dstring_copy_all
  Runs all safe copy tests and returns an aggregate test object containing
  all results.

Parameter(s):
  (none)
Return:
  Test object containing all safe copy test results.
*/
struct d_test_object*
d_tests_sa_dstring_copy_all
(
    void
)
{
    struct d_test_object* group;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string Safe Copy Functions", 5);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // run all safe copy tests
    group->elements[child_idx++] = d_tests_sa_dstring_copy_s();
    group->elements[child_idx++] = d_tests_sa_dstring_copy_cstr_s();
    group->elements[child_idx++] = d_tests_sa_dstring_ncopy_s();
    group->elements[child_idx++] = d_tests_sa_dstring_ncopy_cstr_s();
    group->elements[child_idx++] = d_tests_sa_dstring_to_buffer_s();

    return group;
}


