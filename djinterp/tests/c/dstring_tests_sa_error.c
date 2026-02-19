/******************************************************************************
* djinterp [test]                                          dstring_tests_error.c
*
*   Unit tests for d_string error functions:
*     - d_string_error
*     - d_string_error_r
*
* path:      \src\test\dstring_tests_error.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/

#include "..\tests\dstring_tests_sa.h"

#include <errno.h>


/******************************************************************************
* d_tests_sa_dstring_error
******************************************************************************/

/*
d_tests_sa_dstring_error
  Tests d_string_error() which returns a d_string containing the error message
  for the given error number.

Test cases:
  1.  Known error code (EINVAL)
  2.  Known error code (ENOMEM)
  3.  Known error code (ENOENT)
  4.  Error code 0 (success)
  5.  Unknown/invalid error code
  6.  Returned string is non-empty for known errors
  7.  Multiple calls return independent strings

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_error
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      error_str;
    struct d_string*      error_str2;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string_error", 9);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: EINVAL - known error code
    error_str = d_string_error(EINVAL);

    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "einval_not_null",
        error_str,
        "d_string_error(EINVAL) should return non-NULL"
    );

    if (error_str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "einval_not_empty",
            error_str->size > 0,
            "error message for EINVAL should not be empty"
        );

        d_string_free(error_str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("einval_skip", false, "skip");
    }

    // test 2: ENOMEM - another known error
    error_str = d_string_error(ENOMEM);

    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "enomem_not_null",
        error_str,
        "d_string_error(ENOMEM) should return non-NULL"
    );

    if (error_str)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "enomem_not_empty",
            error_str->size > 0,
            "error message for ENOMEM should not be empty"
        );

        d_string_free(error_str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("enomem_skip", false, "skip");
    }

    // test 3: ENOENT - file not found
    error_str = d_string_error(ENOENT);

    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "enoent_not_null",
        error_str,
        "d_string_error(ENOENT) should return non-NULL"
    );

    if (error_str)
    {
        d_string_free(error_str);
    }

    // test 4: error code 0 (success - may or may not have message)
    error_str = d_string_error(0);

    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "zero_not_null",
        error_str,
        "d_string_error(0) should return non-NULL"
    );

    if (error_str)
    {
        d_string_free(error_str);
    }

    // test 5: unknown error code
    error_str = d_string_error(99999);

    group->elements[child_idx++] = D_ASSERT_NOT_NULL(
        "unknown_not_null",
        error_str,
        "unknown error should still return a string"
    );

    if (error_str)
    {
        // should have some kind of message, even if generic
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "unknown_has_message",
            error_str->size > 0,
            "unknown error should have some message"
        );

        d_string_free(error_str);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("unknown_skip", false, "skip");
    }

    // test 6: multiple calls return independent strings
    error_str  = d_string_error(EINVAL);
    error_str2 = d_string_error(ENOMEM);

    if (error_str && error_str2)
    {
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "independent_strings",
            error_str->text != error_str2->text,
            "multiple calls should return independent strings"
        );

        d_string_free(error_str);
        d_string_free(error_str2);
    }
    else
    {
        if (error_str) d_string_free(error_str);
        if (error_str2) d_string_free(error_str2);
        group->elements[child_idx++] = D_ASSERT_TRUE("ind_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_error_r
******************************************************************************/

/*
d_tests_sa_dstring_error_r
  Tests d_string_error_r() which is the reentrant/thread-safe version that
  copies the error message to a provided d_string.

Test cases:
  1.  NULL destination returns error
  2.  Known error code (EINVAL)
  3.  Known error code (ENOMEM)
  4.  Error code 0
  5.  Unknown error code
  6.  Destination is modified in place
  7.  Previous content replaced

Parameter(s):
  (none)
Return:
  Test object containing all assertion results.
*/
struct d_test_object*
d_tests_sa_dstring_error_r
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      dest;
    size_t                child_idx;
    int                   result;

    group     = d_test_object_new_interior("d_string_error_r", 8);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // test 1: NULL destination returns error
    result = d_string_error_r(EINVAL, NULL);
    group->elements[child_idx++] = D_ASSERT_TRUE(
        "null_dest_error",
        result != 0,
        "d_string_error_r(..., NULL) should return error"
    );

    // test 2: EINVAL
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(EINVAL, dest);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "einval_succeeds",
            result == 0,
            "d_string_error_r(EINVAL, ...) should succeed"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "einval_not_empty",
            dest->size > 0,
            "error message should not be empty"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("einval_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("einval_ne_skip", false, "skip");
    }

    // test 3: ENOMEM
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(ENOMEM, dest);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "enomem_succeeds",
            result == 0,
            "d_string_error_r(ENOMEM, ...) should succeed"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("enomem_skip", false, "skip");
    }

    // test 4: error code 0
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(0, dest);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "zero_succeeds",
            result == 0,
            "d_string_error_r(0, ...) should succeed"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("zero_skip", false, "skip");
    }

    // test 5: unknown error code
    dest = d_string_new();

    if (dest)
    {
        result = d_string_error_r(99999, dest);

        // should still succeed and provide some message
        group->elements[child_idx++] = D_ASSERT_TRUE(
            "unknown_succeeds",
            result == 0,
            "unknown error should still succeed"
        );

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "unknown_has_message",
            dest->size > 0,
            "unknown error should have some message"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("unknown_skip", false, "skip");
        group->elements[child_idx++] = D_ASSERT_TRUE("unknown_msg_skip", false, "skip");
    }

    // test 6: previous content replaced
    dest = d_string_new_from_cstr("Previous content that should be replaced");

    if (dest)
    {
        result = d_string_error_r(EINVAL, dest);

        group->elements[child_idx++] = D_ASSERT_TRUE(
            "content_replaced",
            strstr(dest->text, "Previous") == NULL,
            "previous content should be replaced"
        );

        d_string_free(dest);
    }
    else
    {
        group->elements[child_idx++] = D_ASSERT_TRUE("replace_skip", false, "skip");
    }

    return group;
}


/******************************************************************************
* d_tests_sa_dstring_error_all
******************************************************************************/

/*
d_tests_sa_dstring_error_all
  Runs all error string tests and returns an aggregate test object containing
  all results.

Parameter(s):
  (none)
Return:
  Test object containing all error string test results.
*/
struct d_test_object*
d_tests_sa_dstring_error_all
(
    void
)
{
    struct d_test_object* group;
    size_t                child_idx;

    group     = d_test_object_new_interior("d_string Error Functions", 2);
    child_idx = 0;

    if (!group)
    {
        return NULL;
    }

    // run all error string tests
    group->elements[child_idx++] = d_tests_sa_dstring_error();
    group->elements[child_idx++] = d_tests_sa_dstring_error_r();

    return group;
}