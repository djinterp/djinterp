/*******************************************************************************
* djinterp [test]                                         dstring_tests_concat.c
*
*
* path:      /tests/djinterp/c/dstring_tests_concat.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./dstring_tests.h"


/*******************************************************************************
*                                                      dstring_tests_sa_concat.c
*
*
* path:      /tests/djinterp/c/dstring_tests_sa_concat.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
d_tests_dstring_cat_s
  Tests d_string_cat_s() which safely concatenates one dj_string to another,
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
bool d_tests_dstring_cat_s(struct dj_test_suite* _suite)
{
    struct dj_string*      dest;
    struct dj_string*      src;
    int                   result;

    d_test_suite_test(_suite, "d_string_cat_s");

    // test 1: NULL destination returns false
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_cat_s(NULL, src);

        D_TEST_CHECK_MSG(_suite,
                         "null_dest_returns_false",
                         (result == EINVAL),
                         "d_string_cat_s(NULL, src) should return false");

        d_string_free(src);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "nd_skip", false, "skip");
    }

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_s(dest, NULL);

        D_TEST_CHECK_MSG(_suite,
                         "null_src_returns_false",
                         (result == EINVAL),
                         "d_string_cat_s(dest, NULL) should return false");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "ns_skip", false, "skip");
    }

    // test 3: concatenate to empty destination
    dest = d_string_new();
    src  = d_string_new_from_cstr("Hello");

    if (dest && src)
    {
        result = d_string_cat_s(dest, src);

        D_TEST_CHECK_MSG(_suite,
                         "cat_to_empty_succeeds",
                         (result == 0),
                         "concatenate to empty should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "cat_to_empty_content",
                         d_test_suite_str_equal((dest->text), ("Hello")),
                         "destination should contain 'Hello'");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "ce_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "cec_skip", false, "skip");
    }

    // test 4: concatenate empty source (no change)
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new();

    if (dest && src)
    {
        result = d_string_cat_s(dest, src);

        D_TEST_CHECK_MSG(_suite,
                         "cat_empty_src_succeeds",
                         (result == 0),
                         "concatenate empty source should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "cat_empty_src_unchanged",
                         d_test_suite_str_equal((dest->text), ("Hello")),
                         "destination should be unchanged");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "ces_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "cesu_skip", false, "skip");
    }

    // test 5: normal concatenation
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new_from_cstr(" World");

    if (dest && src)
    {
        result = d_string_cat_s(dest, src);

        D_TEST_CHECK_MSG(_suite,
                         "normal_cat_succeeds",
                         (result == 0),
                         "normal concatenation should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "normal_cat_content",
                         d_test_suite_str_equal((dest->text), ("Hello World")),
                         "destination should be 'Hello World'");

        D_TEST_CHECK_MSG(_suite,
                         "normal_cat_size",
                         (dest->size) == (11),
                         "size should be 11");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "nc_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "ncc_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "ncs_skip", false, "skip");
    }

    // test 6: capacity expands if needed
    dest = d_string_new_with_capacity(8);
    src  = d_string_new_from_cstr("This is a much longer string");

    if (dest && src)
    {
        d_string_assign_cstr(dest, "Start: ");
        result = d_string_cat_s(dest, src);

        D_TEST_CHECK_MSG(_suite,
                         "capacity_expands_succeeds",
                         (result == 0),
                         "concatenation should expand capacity");

        D_TEST_CHECK_MSG(_suite,
                         "capacity_expands_content",
                         d_test_suite_str_equal((dest->text), ("Start: This is a much longer string")),
                         "content should be fully concatenated");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "cex_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "cexc_skip", false, "skip");
    }

    // test 7: source unchanged after concatenation
    dest = d_string_new_from_cstr("A");
    src  = d_string_new_from_cstr("B");

    if (dest && src)
    {
        d_string_cat_s(dest, src);

        D_TEST_CHECK_MSG(_suite,
                         "source_unchanged",
                         d_test_suite_str_equal((src->text), ("B")),
                         "source should be unchanged after concatenation");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "su_skip", false, "skip");
    }

    // test 8: multiple concatenations
    dest = d_string_new_from_cstr("A");
    src  = d_string_new_from_cstr("B");

    if (dest && src)
    {
        d_string_cat_s(dest, src);
        d_string_cat_s(dest, src);
        d_string_cat_s(dest, src);

        D_TEST_CHECK_MSG(_suite,
                         "multiple_cats",
                         d_test_suite_str_equal((dest->text), ("ABBB")),
                         "multiple concatenations should work");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "mc_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_cat_cstr_s
******************************************************************************/

/*
d_tests_dstring_cat_cstr_s
  Tests d_string_cat_cstr_s() which safely concatenates a C string to a
  dj_string.

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
bool d_tests_dstring_cat_cstr_s(struct dj_test_suite* _suite)
{
    struct dj_string*      dest;
    int                   result;

    d_test_suite_test(_suite, "d_string_cat_cstr_s");

    // test 1: NULL destination returns false
    result = d_string_cat_cstr_s(NULL, "test");
    D_TEST_CHECK_MSG(_suite,
                     "null_dest_returns_false",
                     (result == EINVAL),
                     "d_string_cat_cstr_s(NULL, ...) should return false");

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, NULL);

        D_TEST_CHECK_MSG(_suite,
                         "null_src_returns_false",
                         (result == EINVAL),
                         "d_string_cat_cstr_s(dest, NULL) should return false");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "ns_skip", false, "skip");
    }

    // test 3: concatenate to empty destination
    dest = d_string_new();

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, "Hello");

        D_TEST_CHECK_MSG(_suite,
                         "cat_to_empty_succeeds",
                         (result == 0),
                         "concatenate to empty should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "cat_to_empty_content",
                         d_test_suite_str_equal((dest->text), ("Hello")),
                         "destination should contain 'Hello'");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "ce_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "cec_skip", false, "skip");
    }

    // test 4: concatenate empty C string (no change)
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, "");

        D_TEST_CHECK_MSG(_suite,
                         "cat_empty_succeeds",
                         (result == 0),
                         "concatenate empty string should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "cat_empty_unchanged",
                         d_test_suite_str_equal((dest->text), ("Hello")),
                         "destination should be unchanged");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "cem_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "cemu_skip", false, "skip");
    }

    // test 5: normal concatenation
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_cat_cstr_s(dest, " World!");

        D_TEST_CHECK_MSG(_suite,
                         "normal_cat_content",
                         d_test_suite_str_equal((dest->text), ("Hello World!")),
                         "destination should be 'Hello World!'");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "nc_skip", false, "skip");
    }

    // test 6: capacity expands
    dest = d_string_new_with_capacity(4);

    if (dest)
    {
        d_string_assign_cstr(dest, "AB");
        result = d_string_cat_cstr_s(dest, "CDEFGHIJKLMNOP");

        D_TEST_CHECK_MSG(_suite,
                         "capacity_expands_succeeds",
                         (result == 0),
                         "concatenation should expand capacity");

        D_TEST_CHECK_MSG(_suite,
                         "capacity_expands_content",
                         d_test_suite_str_equal((dest->text), ("ABCDEFGHIJKLMNOP")),
                         "content should be fully concatenated");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "cex_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "cexc_skip", false, "skip");
    }

    // test 7: multiple concatenations
    dest = d_string_new_from_cstr("A");

    if (dest)
    {
        d_string_cat_cstr_s(dest, "B");
        d_string_cat_cstr_s(dest, "C");
        d_string_cat_cstr_s(dest, "D");

        D_TEST_CHECK_MSG(_suite,
                         "multiple_cats",
                         d_test_suite_str_equal((dest->text), ("ABCD")),
                         "multiple concatenations should work");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "mc_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_ncat_s
******************************************************************************/

/*
d_tests_dstring_ncat_s
  Tests d_string_ncat_s() which safely concatenates up to n characters from
  one dj_string to another.

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
bool d_tests_dstring_ncat_s(struct dj_test_suite* _suite)
{
    struct dj_string*      dest;
    struct dj_string*      src;
    int                   result;

    d_test_suite_test(_suite, "d_string_ncat_s");

    // test 1: NULL destination returns false
    src = d_string_new_from_cstr("test");

    if (src)
    {
        result = d_string_ncat_s(NULL, src, 4);

        D_TEST_CHECK_MSG(_suite,
                         "null_dest_returns_false",
                         (result == EINVAL),
                         "d_string_ncat_s(NULL, ...) should return false");

        d_string_free(src);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "nd_skip", false, "skip");
    }

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_s(dest, NULL, 4);

        D_TEST_CHECK_MSG(_suite,
                         "null_src_returns_false",
                         (result == EINVAL),
                         "d_string_ncat_s(dest, NULL, ...) should return false");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "ns_skip", false, "skip");
    }

    // test 3: n = 0 (no change)
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new_from_cstr(" World");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 0);

        D_TEST_CHECK_MSG(_suite,
                         "n_0_succeeds",
                         (result == 0),
                         "ncat with n=0 should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "n_0_unchanged",
                         d_test_suite_str_equal((dest->text), ("Hello")),
                         "destination should be unchanged when n=0");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "n0_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "n0u_skip", false, "skip");
    }

    // test 4: n less than source length (partial concatenation)
    dest = d_string_new_from_cstr("Hello");
    src  = d_string_new_from_cstr(" World!");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 6);

        D_TEST_CHECK_MSG(_suite,
                         "partial_cat_succeeds",
                         (result == 0),
                         "partial concatenation should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "partial_cat_content",
                         d_test_suite_str_equal((dest->text), ("Hello World")),
                         "should concatenate first 6 chars of source");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "pc_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "pcc_skip", false, "skip");
    }

    // test 5: n equal to source length
    dest = d_string_new_from_cstr("AB");
    src  = d_string_new_from_cstr("CD");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 2);

        D_TEST_CHECK_MSG(_suite,
                         "exact_n_content",
                         d_test_suite_str_equal((dest->text), ("ABCD")),
                         "exact n should concatenate full source");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "exn_skip", false, "skip");
    }

    // test 6: n greater than source length
    dest = d_string_new_from_cstr("AB");
    src  = d_string_new_from_cstr("CD");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 100);

        D_TEST_CHECK_MSG(_suite,
                         "excess_n_succeeds",
                         (result == 0),
                         "n > source length should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "excess_n_content",
                         d_test_suite_str_equal((dest->text), ("ABCD")),
                         "should concatenate full source when n > length");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "en_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "enc_skip", false, "skip");
    }

    // test 7: concatenate to empty destination
    dest = d_string_new();
    src  = d_string_new_from_cstr("Test");

    if (dest && src)
    {
        result = d_string_ncat_s(dest, src, 2);

        D_TEST_CHECK_MSG(_suite,
                         "to_empty_content",
                         d_test_suite_str_equal((dest->text), ("Te")),
                         "should concatenate to empty destination");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "te_skip", false, "skip");
    }

    // test 8: size updated correctly
    dest = d_string_new_from_cstr("123");
    src  = d_string_new_from_cstr("456789");

    if (dest && src)
    {
        d_string_ncat_s(dest, src, 3);

        D_TEST_CHECK_MSG(_suite,
                         "size_updated",
                         (dest->size) == (6),
                         "size should be 6 after concatenating 3 chars");

        d_string_free(dest);
        d_string_free(src);
    }
    else
    {
        if (dest) d_string_free(dest);
        if (src) d_string_free(src);
        D_TEST_CHECK_MSG(_suite, "su_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_ncat_cstr_s
******************************************************************************/

/*
d_tests_dstring_ncat_cstr_s
  Tests d_string_ncat_cstr_s() which safely concatenates up to n characters
  from a C string to a dj_string.

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
bool d_tests_dstring_ncat_cstr_s(struct dj_test_suite* _suite)
{
    struct dj_string*      dest;
    int                   result;

    d_test_suite_test(_suite, "d_string_ncat_cstr_s");

    // test 1: NULL destination returns false
    result = d_string_ncat_cstr_s(NULL, "test", 4);
    D_TEST_CHECK_MSG(_suite,
                     "null_dest_returns_false",
                     (result == EINVAL),
                     "d_string_ncat_cstr_s(NULL, ...) should return false");

    // test 2: NULL source returns false
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, NULL, 4);

        D_TEST_CHECK_MSG(_suite,
                         "null_src_returns_false",
                         (result == EINVAL),
                         "d_string_ncat_cstr_s(dest, NULL, ...) should return false");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "ns_skip", false, "skip");
    }

    // test 3: n = 0 (no change)
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, " World", 0);

        D_TEST_CHECK_MSG(_suite, "n_0_succeeds", (result == 0), "n=0 should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "n_0_unchanged",
                         d_test_suite_str_equal((dest->text), ("Hello")),
                         "destination should be unchanged when n=0");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "n0_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "n0u_skip", false, "skip");
    }

    // test 4: partial concatenation
    dest = d_string_new_from_cstr("Hello");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, " World!", 6);

        D_TEST_CHECK_MSG(_suite,
                         "partial_succeeds",
                         (result == 0),
                         "partial concatenation should succeed");

        D_TEST_CHECK_MSG(_suite,
                         "partial_content",
                         d_test_suite_str_equal((dest->text), ("Hello World")),
                         "should concatenate first 6 chars");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "pc_skip", false, "skip");
        D_TEST_CHECK_MSG(_suite, "pcc_skip", false, "skip");
    }

    // test 5: full concatenation with excess n
    dest = d_string_new_from_cstr("AB");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, "CD", 100);

        D_TEST_CHECK_MSG(_suite,
                         "excess_n_content",
                         d_test_suite_str_equal((dest->text), ("ABCD")),
                         "should concatenate full string when n > length");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "en_skip", false, "skip");
    }

    // test 6: exact n
    dest = d_string_new_from_cstr("12");

    if (dest)
    {
        result = d_string_ncat_cstr_s(dest, "34", 2);

        D_TEST_CHECK_MSG(_suite,
                         "exact_n_content",
                         d_test_suite_str_equal((dest->text), ("1234")),
                         "exact n should work correctly");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "exn_skip", false, "skip");
    }

    // test 7: multiple partial concatenations
    dest = d_string_new_from_cstr("A");

    if (dest)
    {
        d_string_ncat_cstr_s(dest, "BCD", 1);
        d_string_ncat_cstr_s(dest, "CDE", 1);
        d_string_ncat_cstr_s(dest, "DEF", 1);

        D_TEST_CHECK_MSG(_suite,
                         "multiple_partial",
                         d_test_suite_str_equal((dest->text), ("ABCD")),
                         "multiple partial concatenations should work");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "mp_skip", false, "skip");
    }

    // test 8: size updated
    dest = d_string_new_from_cstr("XX");

    if (dest)
    {
        d_string_ncat_cstr_s(dest, "YYYY", 2);

        D_TEST_CHECK_MSG(_suite,
                         "size_updated",
                         (dest->size) == (4),
                         "size should be 4");

        d_string_free(dest);
    }
    else
    {
        D_TEST_CHECK_MSG(_suite, "su_skip", false, "skip");
    }

    return true;
}


/******************************************************************************
* d_tests_dstring_concat_all
******************************************************************************/

/*
d_tests_dstring_concat_all
  Runs all concatenation tests and returns an aggregate test object
  containing all results.

Parameter(s):
  (none)
Return:
  Test object containing all concatenation test results.
*/
bool d_tests_dstring_concat_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "dj_string Concatenation Functions");

    // run all concatenation tests
    d_tests_dstring_cat_s(_suite);
    d_tests_dstring_cat_cstr_s(_suite);
    d_tests_dstring_ncat_s(_suite);
    d_tests_dstring_ncat_cstr_s(_suite);

    return true;
}
