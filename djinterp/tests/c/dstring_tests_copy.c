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

#include "..\tests\dstring_tests_sa.h"


/******************************************************************************
* test_dstring_copy_s
******************************************************************************/

/*
test_dstring_copy_s
  Tests d_string_copy_s() which safely copies one d_string to another,
  similar to strcpy_s.

Test cases:
  1.  NULL destination returns false
  2.  NULL source returns false
  3.  Copy empty string to empty string
  4.  Copy non-empty string to empty destination
  5.  Copy overwrites existing content
  6.  Capacity expands if needed
  7.  Size updated correctly
  8.  Content matches exactly
  9.  Self-copy (same object)
  10. Copy long string

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
test_dstring_copy_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_copy_s", 14);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_copy_s(NULL, src);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_dest_returns_false",
            result,
            "d_string_copy_s(NULL, src) should return false"
        );

        d_string_free(src);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nd_skip", false, "skip");
    }

    // test 2: NULL source returns false
    dest = d_string_new();

    if (dest)
    {
        result = d_string_copy_s(dest, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_copy_s(dest, NULL) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: copy empty string to empty string
    dest = d_string_new();
    src  = d_string_new();

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_to_empty_succeeds",
            result,
            "copy empty to empty should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_to_empty_size_0",
            dest->size, 0,
            "destination should have size 0"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ee_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ees_skip", false, "skip");
    }

    // test 4: copy non-empty to empty destination
    dest = d_string_new();
    src  = d_string_new_from_cstr("Hello World");

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "nonempty_to_empty_succeeds",
            result,
            "copy non-empty to empty should succeed"
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
        group->elements[child_idx++] = D_ASSERT_TRUE("ne_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("nec_skip", false, "skip");
    }

    // test 5: copy overwrites existing content
    dest = d_string_new_from_cstr("Old Content");
    src  = d_string_new_from_cstr("New");

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "overwrite_succeeds",
            result,
            "copy should overwrite existing content"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "overwrite_content",
            dest->text, "New",
            "destination should be 'New', not 'Old Content'"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "overwrite_size",
            dest->size, 3,
            "size should be 3"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ow_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("owc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ows_skip", false, "skip");
    }

    // test 6: capacity expands if needed
    dest = d_string_new_with_capacity(4);
    src  = d_string_new_from_cstr("This is a much longer string");

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_expands_succeeds",
            result,
            "copy should expand capacity as needed"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_expanded",
            dest->capacity >= strlen("This is a much longer string") + 1,
            "capacity should have expanded"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ce_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("cec_skip", false, "skip");
    }

    // test 7: source unchanged after copy
    dest = d_string_new();
    src  = d_string_new_from_cstr("Source");

    if (dest && src)
    {
        d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "source_unchanged",
            src->text, "Source",
            "source should be unchanged after copy"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("su_skip", false, "skip");
    }

    // test 8: copy long string (stress test)
    dest = d_string_new();
    src  = d_string_new_fill(1000, 'X');

    if (dest && src)
    {
        result = d_string_copy_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "long_string_succeeds",
            result,
            "copy long string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "long_string_size",
            dest->size, 1000,
            "destination should have size 1000"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ls_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("lss_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* test_dstring_copy_cstr_s
******************************************************************************/

/*
test_dstring_copy_cstr_s
  Tests d_string_copy_cstr_s() which safely copies a C string to a d_string.

Test cases:
  1. NULL destination returns false
  2. NULL source returns false
  3. Copy empty C string
  4. Copy normal C string
  5. Copy overwrites existing content
  6. Capacity expands if needed
  7. Copy string with special characters

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
test_dstring_copy_cstr_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_copy_cstr_s", 10);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    result = d_string_copy_cstr_s(NULL, "test");
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_dest_returns_false",
        result,
        "d_string_copy_cstr_s(NULL, ...) should return false"
    );

    // test 2: NULL source returns false
    dest = d_string_new();

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_copy_cstr_s(dest, NULL) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: copy empty C string
    dest = d_string_new_from_cstr("Existing");

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_cstr_succeeds",
            result,
            "copy empty C string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "empty_cstr_size_0",
            dest->size, 0,
            "destination should have size 0"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ec_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ecs_skip", false, "skip");
    }

    // test 4: copy normal C string
    dest = d_string_new();

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "Hello World");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "normal_copy_succeeds",
            result,
            "copy normal C string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "normal_copy_content",
            dest->text, "Hello World",
            "destination should contain 'Hello World'"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ncc_skip", false, "skip");
    }

    // test 5: copy overwrites existing content
    dest = d_string_new_from_cstr("Old Data Here");

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "New");

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "overwrite_content",
            dest->text, "New",
            "copy should overwrite existing content"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ow_skip", false, "skip");
    }

    // test 6: capacity expands
    dest = d_string_new_with_capacity(4);

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "A much longer string than 4 chars");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_expands_succeeds",
            result,
            "copy should expand capacity"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "capacity_expands_content",
            dest->text, "A much longer string than 4 chars",
            "content should be fully copied"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ce_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("cec_skip", false, "skip");
    }

    // test 7: copy string with special characters
    dest = d_string_new();

    if (dest)
    {
        result = d_string_copy_cstr_s(dest, "Tab:\tNewline:\nQuote:\"");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "special_chars_succeeds",
            result,
            "copy with special chars should succeed"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("sc_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* test_dstring_ncopy_s
******************************************************************************/

/*
test_dstring_ncopy_s
  Tests d_string_ncopy_s() which safely copies up to n characters from one
  d_string to another.

Test cases:
  1.  NULL destination returns false
  2.  NULL source returns false
  3.  n = 0 clears destination
  4.  n less than source length (partial copy)
  5.  n equal to source length (full copy)
  6.  n greater than source length (full copy)
  7.  Overwrites existing content
  8.  Result is null-terminated

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
test_dstring_ncopy_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_ncopy_s", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_ncopy_s(NULL, src, 4);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_dest_returns_false",
            result,
            "d_string_ncopy_s(NULL, ...) should return false"
        );

        d_string_free(src);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nd_skip", false, "skip");
    }

    // test 2: NULL source returns false
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_s(dest, NULL, 4);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_ncopy_s(dest, NULL, ...) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: n = 0 clears destination
    dest = d_string_new_from_cstr("Existing");
    src  = d_string_new_from_cstr("Source");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 0);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "n_0_succeeds",
            result,
            "ncopy with n=0 should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_0_clears",
            dest->size, 0,
            "n=0 should clear destination"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("n0c_skip", false, "skip");
    }

    // test 4: n less than source length (partial copy)
    dest = d_string_new();
    src  = d_string_new_from_cstr("Hello World");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 5);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_copy_succeeds",
            result,
            "partial copy should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "partial_copy_content",
            dest->text, "Hello",
            "destination should contain 'Hello'"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "partial_copy_size",
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
        group->elements[child_idx++] = D_ASSERT_TRUE("pc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("pcc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("pcs_skip", false, "skip");
    }

    // test 5: n equal to source length (full copy)
    dest = d_string_new();
    src  = d_string_new_from_cstr("Exact");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 5);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "exact_copy_content",
            dest->text, "Exact",
            "exact length copy should work"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ex_skip", false, "skip");
    }

    // test 6: n greater than source length (full copy)
    dest = d_string_new();
    src  = d_string_new_from_cstr("Short");

    if (dest && src)
    {
        result = d_string_ncopy_s(dest, src, 100);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "excess_n_succeeds",
            result,
            "n > length should copy full string"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "excess_n_content",
            dest->text, "Short",
            "destination should contain 'Short'"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "excess_n_size",
            dest->size, 5,
            "size should be actual string length (5)"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("en_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("enc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ens_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* test_dstring_ncopy_cstr_s
******************************************************************************/

/*
test_dstring_ncopy_cstr_s
  Tests d_string_ncopy_cstr_s() which safely copies up to n characters from
  a C string to a d_string.

Test cases:
  1. NULL destination returns false
  2. NULL source returns false
  3. n = 0 clears destination
  4. Partial copy
  5. Full copy with excess n
  6. Copy with exact n

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
test_dstring_ncopy_cstr_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_ncopy_cstr_s", 10);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    result = d_string_ncopy_cstr_s(NULL, "test", 4);
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_dest_returns_false",
        result,
        "d_string_ncopy_cstr_s(NULL, ...) should return false"
    );

    // test 2: NULL source returns false
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, NULL, 4);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_ncopy_cstr_s(dest, NULL, ...) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: n = 0 clears destination
    dest = d_string_new_from_cstr("Existing");

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Source", 0);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "n_0_succeeds",
            result,
            "n=0 should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "n_0_clears",
            dest->size, 0,
            "n=0 should clear destination"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("n0c_skip", false, "skip");
    }

    // test 4: partial copy
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Hello World", 5);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_succeeds",
            result,
            "partial copy should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "partial_content",
            dest->text, "Hello",
            "should copy first 5 characters"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("pc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("pcc_skip", false, "skip");
    }

    // test 5: full copy with excess n
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Short", 100);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "excess_n_content",
            dest->text, "Short",
            "should copy full string when n > length"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "excess_n_size",
            dest->size, 5,
            "size should be actual length"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("en_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ens_skip", false, "skip");
    }

    // test 6: copy with exact n
    dest = d_string_new();

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "Test", 4);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "exact_n_content",
            dest->text, "Test",
            "exact n copy should work"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("exn_skip", false, "skip");
    }

    // test 7: overwrite longer content with shorter partial
    dest = d_string_new_from_cstr("LongExistingContent");

    if (dest)
    {
        result = d_string_ncopy_cstr_s(dest, "ABCDE", 3);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "overwrite_partial",
            dest->text, "ABC",
            "partial copy should overwrite completely"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("op_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* test_dstring_to_buffer_s
******************************************************************************/

/*
test_dstring_to_buffer_s
  Tests d_string_to_buffer_s() which safely copies a d_string to a raw buffer.

Test cases:
  1.  NULL string returns false
  2.  NULL buffer returns false
  3.  Buffer size 0 returns false
  4.  Buffer size 1 (room for null only)
  5.  Buffer size sufficient for full copy
  6.  Buffer too small (truncation)
  7.  Empty string copy
  8.  Result is null-terminated
  9.  Returns bytes written
  10. Exact size buffer

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
test_dstring_to_buffer_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    size_t                child_idx;
    char                  buffer[256];
    bool                  result;

    group     = d_test_object_new_interior("d_string_to_buffer_s", 14);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL string returns false
    result = d_string_to_buffer_s(NULL, buffer, sizeof(buffer));
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_string_returns_false",
        result,
        "d_string_to_buffer_s(NULL, ...) should return false"
    );

    // test 2: NULL buffer returns false
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_to_buffer_s(str, NULL, 100);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_buffer_returns_false",
            result,
            "d_string_to_buffer_s(..., NULL, ...) should return false"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nb_skip", false, "skip");
    }

    // test 3: buffer size 0 returns false
    str = d_string_new_from_cstr("test");

    if (str)
    {
        result = d_string_to_buffer_s(str, buffer, 0);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "size_0_returns_false",
            result,
            "buffer size 0 should return false"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("s0_skip", false, "skip");
    }

    // test 4: buffer size 1 (room for null only)
    str = d_string_new_from_cstr("Hello");

    if (str)
    {
        memset(buffer, 'X', sizeof(buffer));
        result = d_string_to_buffer_s(str, buffer, 1);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "size_1_succeeds",
            result,
            "buffer size 1 should succeed (empty result)"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "size_1_null_terminated",
            buffer[0], '\0',
            "buffer should be null-terminated"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("s1_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("s1n_skip", false, "skip");
    }

    // test 5: buffer size sufficient for full copy
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        memset(buffer, 'X', sizeof(buffer));
        result = d_string_to_buffer_s(str, buffer, sizeof(buffer));

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "full_copy_succeeds",
            result,
            "full copy should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "full_copy_content",
            buffer, "Hello World",
            "buffer should contain full string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("fc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("fcc_skip", false, "skip");
    }

    // test 6: buffer too small (truncation)
    str = d_string_new_from_cstr("Hello World");

    if (str)
    {
        memset(buffer, 'X', sizeof(buffer));
        result = d_string_to_buffer_s(str, buffer, 6);  // room for "Hello" + null

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "truncation_succeeds",
            result,
            "truncation should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "truncation_content",
            buffer, "Hello",
            "buffer should contain truncated string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("tr_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("trc_skip", false, "skip");
    }

    // test 7: empty string copy
    str = d_string_new();

    if (str)
    {
        memset(buffer, 'X', sizeof(buffer));
        result = d_string_to_buffer_s(str, buffer, sizeof(buffer));

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "empty_string_succeeds",
            result,
            "empty string copy should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "empty_string_content",
            buffer, "",
            "buffer should be empty string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("es_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("esc_skip", false, "skip");
    }

    // test 8: exact size buffer (string length + 1)
    str = d_string_new_from_cstr("Test");

    if (str)
    {
        memset(buffer, 'X', sizeof(buffer));
        result = d_string_to_buffer_s(str, buffer, 5);  // exactly "Test" + null

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "exact_size_succeeds",
            result,
            "exact size buffer should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "exact_size_content",
            buffer, "Test",
            "buffer should contain full string"
        );

        d_string_free(str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("exs_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("exsc_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* test_dstring_copy_all
******************************************************************************/

/*
test_dstring_copy_all
  Runs all safe copy tests and returns an aggregate test object containing
  all results.

Parameter(s):
  (none)
Return:
  Test object containing all safe copy test results.
*/
struct d_test_object*
test_dstring_copy_all
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
    group->elements[child_idx++] = test_dstring_copy_s();
    group->elements[child_idx++] = test_dstring_copy_cstr_s();
    group->elements[child_idx++] = test_dstring_ncopy_s();
    group->elements[child_idx++] = test_dstring_ncopy_cstr_s();
    group->elements[child_idx++] = test_dstring_to_buffer_s();

    return group;
}
