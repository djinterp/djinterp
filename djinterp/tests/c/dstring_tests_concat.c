#include ".\dstring_tests_sa.h"


/******************************************************************************
* d_tests_sa_dstring_cat_s
******************************************************************************/

/*
d_tests_sa_dstring_cat_s
  Tests d_string_cat_s() which safely concatenates one d_string to another,
  similar to strcat_s.

Test cases:
  1.  NULL destination returns false
  2.  NULL source returns false
  3.  Concatenate to empty destination
  4.  Concatenate empty source (no change)
  5.  Normal concatenation
  6.  Capacity expands if needed
  7.  Multiple concatenations
  8.  Source unchanged after concatenation
  9.  Concatenate to self (same object)
  10. Concatenate long strings

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_cat_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_cat_s", 14);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_cat_s(NULL, src);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_dest_returns_false",
            result,
            "d_string_cat_s(NULL, src) should return false"
        );

        d_string_free(src);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nd_skip", false, "skip");
    }

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_s(dest, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_cat_s(dest, NULL) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: concatenate to empty destination
    dest = d_string_new();
    src  = d_string_new_from_cstr("Hello");

    if (dest && src)
    {
        result = d_string_cat_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "cat_to_empty_succeeds",
            result,
            "concatenate to empty should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "cat_to_empty_content",
            dest->text, "Hello",
            "destination should contain 'Hello'"
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

    // test 4: concatenate empty source (no change)
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new();

    if (dest && src)
    {
        result = d_string_cat_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "cat_empty_src_succeeds",
            result,
            "concatenate empty source should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "cat_empty_src_unchanged",
            dest->text, "Hello",
            "destination should be unchanged"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("ces_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("cesu_skip", false, "skip");
    }

    // test 5: normal concatenation
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new_from_cstr(" World");

    if (dest && src)
    {
        result = d_string_cat_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "normal_cat_succeeds",
            result,
            "normal concatenation should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "normal_cat_content",
            dest->text, "Hello World",
            "destination should be 'Hello World'"
        );

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "normal_cat_size",
            dest->size, 11,
            "size should be 11"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("nc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ncc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("ncs_skip", false, "skip");
    }

    // test 6: capacity expands if needed
    dest = d_string_new_with_capacity(8);
    src  = d_string_new_from_cstr("This is a much longer string");

    if (dest && src)
    {
        d_string_assign_cstr(dest, "Start: ");
        result = d_string_cat_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_expands_succeeds",
            result,
            "concatenation should expand capacity"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "capacity_expands_content",
            dest->text, "Start: This is a much longer string",
            "content should be fully concatenated"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("cex_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("cexc_skip", false, "skip");
    }

    // test 7: source unchanged after concatenation
    dest = d_string_new_from_cstr("A");
    src  = d_string_new_from_cstr("B");

    if (dest && src)
    {
        d_string_cat_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "source_unchanged",
            src->text, "B",
            "source should be unchanged after concatenation"
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

    // test 8: multiple concatenations
    dest = d_string_new_from_cstr("A");
    src  = d_string_new_from_cstr("B");

    if (dest && src)
    {
        d_string_cat_s(dest, src);
        d_string_cat_s(dest, src);
        d_string_cat_s(dest, src);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "multiple_cats",
            dest->text, "ABBB",
            "multiple concatenations should work"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("mc_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_cat_cstr_s
******************************************************************************/

/*
d_tests_sa_dstring_cat_cstr_s
  Tests d_string_cat_cstr_s() which safely concatenates a C string to a
  d_string.

Test cases:
  1. NULL destination returns false
  2. NULL source returns false
  3. Concatenate to empty destination
  4. Concatenate empty C string
  5. Normal concatenation
  6. Capacity expands
  7. Concatenate special characters
  8. Multiple concatenations

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_cat_cstr_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_cat_cstr_s", 11);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    result = d_string_cat_cstr_s(NULL, "test");
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_dest_returns_false",
        result,
        "d_string_cat_cstr_s(NULL, ...) should return false"
    );

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, NULL);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_cat_cstr_s(dest, NULL) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: concatenate to empty destination
    dest = d_string_new();

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, "Hello");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "cat_to_empty_succeeds",
            result,
            "concatenate to empty should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "cat_to_empty_content",
            dest->text, "Hello",
            "destination should contain 'Hello'"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ce_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("cec_skip", false, "skip");
    }

    // test 4: concatenate empty C string (no change)
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, "");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "cat_empty_succeeds",
            result,
            "concatenate empty string should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "cat_empty_unchanged",
            dest->text, "Hello",
            "destination should be unchanged"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("cem_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("cemu_skip", false, "skip");
    }

    // test 5: normal concatenation
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, " World!");

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "normal_cat_content",
            dest->text, "Hello World!",
            "destination should be 'Hello World!'"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nc_skip", false, "skip");
    }

    // test 6: capacity expands
    dest = d_string_new_with_capacity(4);

    if (dest)
    {
        d_string_assign_cstr(dest, "AB");
        result = d_string_cat_cstr_s(dest, "CDEFGHIJKLMNOP");

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "capacity_expands_succeeds",
            result,
            "concatenation should expand capacity"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "capacity_expands_content",
            dest->text, "ABCDEFGHIJKLMNOP",
            "content should be fully concatenated"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("cex_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("cexc_skip", false, "skip");
    }

    // test 7: multiple concatenations
    dest = d_string_new_from_cstr("A");

    if (dest)
    {
        d_string_cat_cstr_s(dest, "B");
        d_string_cat_cstr_s(dest, "C");
        d_string_cat_cstr_s(dest, "D");

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "multiple_cats",
            dest->text, "ABCD",
            "multiple concatenations should work"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("mc_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncat_s
******************************************************************************/

/*
d_tests_sa_dstring_ncat_s
  Tests d_string_ncat_s() which safely concatenates up to n characters from
  one d_string to another.

Test cases:
  1. NULL destination returns false
  2. NULL source returns false
  3. n = 0 (no change)
  4. n less than source length (partial)
  5. n equal to source length
  6. n greater than source length
  7. Concatenate to empty destination
  8. Size updated correctly

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncat_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    struct d_string*      src;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_ncat_s", 12);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_ncat_s(NULL, src, 4);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_dest_returns_false",
            result,
            "d_string_ncat_s(NULL, ...) should return false"
        );

        d_string_free(src);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("nd_skip", false, "skip");
    }

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_s(dest, NULL, 4);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_ncat_s(dest, NULL, ...) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: n = 0 (no change)
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new_from_cstr(" World");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 0);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "n_0_succeeds",
            result,
            "ncat with n=0 should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "n_0_unchanged",
            dest->text, "Hello",
            "destination should be unchanged when n=0"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("n0u_skip", false, "skip");
    }

    // test 4: n less than source length (partial concatenation)
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new_from_cstr(" World!");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 6);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_cat_succeeds",
            result,
            "partial concatenation should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "partial_cat_content",
            dest->text, "Hello World",
            "should concatenate first 6 chars of source"
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
    }

    // test 5: n equal to source length
    dest = d_string_new_from_cstr("AB");
    src  = d_string_new_from_cstr("CD");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 2);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "exact_n_content",
            dest->text, "ABCD",
            "exact n should concatenate full source"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("exn_skip", false, "skip");
    }

    // test 6: n greater than source length
    dest = d_string_new_from_cstr("AB");
    src  = d_string_new_from_cstr("CD");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 100);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "excess_n_succeeds",
            result,
            "n > source length should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "excess_n_content",
            dest->text, "ABCD",
            "should concatenate full source when n > length"
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
    }

    // test 7: concatenate to empty destination
    dest = d_string_new();
    src  = d_string_new_from_cstr("Test");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 2);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "to_empty_content",
            dest->text, "Te",
            "should concatenate to empty destination"
        );

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        group->elements[child_idx++] = D_ASSERT_TRUE("te_skip", false, "skip");
    }

    // test 8: size updated correctly
    dest = d_string_new_from_cstr("123");
    src  = d_string_new_from_cstr("456789");

    if (dest && src)
    {
        d_string_ncat_s(dest, src, 3);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "size_updated",
            dest->size, 6,
            "size should be 6 after concatenating 3 chars"
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

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_ncat_cstr_s
******************************************************************************/

/*
d_tests_sa_dstring_ncat_cstr_s
  Tests d_string_ncat_cstr_s() which safely concatenates up to n characters
  from a C string to a d_string.

Test cases:
  1. NULL destination returns false
  2. NULL source returns false
  3. n = 0 (no change)
  4. Partial concatenation
  5. Full concatenation with excess n
  6. Exact n
  7. Multiple partial concatenations

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_ncat_cstr_s
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    size_t                child_idx;
    bool                  result;

    group     = d_test_object_new_interior("d_string_ncat_cstr_s", 10);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns false
    result = d_string_ncat_cstr_s(NULL, "test", 4);
    group->elements[child_idx++] = D_ASSERT_FALSE(
        "null_dest_returns_false",
        result,
        "d_string_ncat_cstr_s(NULL, ...) should return false"
    );

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, NULL, 4);

        group->elements[child_idx++] = D_ASSERT_FALSE(
            "null_src_returns_false",
            result,
            "d_string_ncat_cstr_s(dest, NULL, ...) should return false"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("ns_skip", false, "skip");
    }

    // test 3: n = 0 (no change)
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, " World", 0);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "n_0_succeeds",
            result,
            "n=0 should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "n_0_unchanged",
            dest->text, "Hello",
            "destination should be unchanged when n=0"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("n0_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("n0u_skip", false, "skip");
    }

    // test 4: partial concatenation
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, " World!", 6);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "partial_succeeds",
            result,
            "partial concatenation should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "partial_content",
            dest->text, "Hello World",
            "should concatenate first 6 chars"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("pc_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("pcc_skip", false, "skip");
    }

    // test 5: full concatenation with excess n
    dest = d_string_new_from_cstr("AB");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, "CD", 100);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "excess_n_content",
            dest->text, "ABCD",
            "should concatenate full string when n > length"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("en_skip", false, "skip");
    }

    // test 6: exact n
    dest = d_string_new_from_cstr("12");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, "34", 2);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "exact_n_content",
            dest->text, "1234",
            "exact n should work correctly"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("exn_skip", false, "skip");
    }

    // test 7: multiple partial concatenations
    dest = d_string_new_from_cstr("A");

    if (dest)
    {
        d_string_ncat_cstr_s(dest, "BCD", 1);
        d_string_ncat_cstr_s(dest, "CDE", 1);
        d_string_ncat_cstr_s(dest, "DEF", 1);

        group->elements[child_idx++] = D_ASSERT_STR_EQUAL(
            "multiple_partial",
            dest->text, "ABCD",
            "multiple partial concatenations should work"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("mp_skip", false, "skip");
    }

    // test 8: size updated
    dest = d_string_new_from_cstr("XX");

    if (dest)
    {
        d_string_ncat_cstr_s(dest, "YYYY", 2);

        group->elements[child_idx++] = D_ASSERT_EQUAL(
            "size_updated",
            dest->size, 4,
            "size should be 4"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("su_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_concat_all
******************************************************************************/

/*
d_tests_sa_dstring_concat_all
  Runs all concatenation tests and returns an aggregate test object
  containing all results.

Parameter(s):
  (none)
Return:
  Test object containing all concatenation test results.
*/
struct d_test_object*
d_tests_sa_dstring_concat_all
(
    void
)
{
    struct d_test_object* group;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string Concatenation Functions", 4);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // run all concatenation tests
    group->elements[child_idx++] = d_tests_sa_dstring_cat_s();
    group->elements[child_idx++] = d_tests_sa_dstring_cat_cstr_s();
    group->elements[child_idx++] = d_tests_sa_dstring_ncat_s();
    group->elements[child_idx++] = d_tests_sa_dstring_ncat_cstr_s();

    return group;
}
