#include ".\string_fn_tests_sa.h"


/******************************************************************************
 * SAFE STRING COPY TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strcpy_s
  Tests d_strcpy_s for safe string copying.
  Tests the following:
  - copies string correctly to adequate buffer
  - handles exact-fit buffer
  - returns error for insufficient buffer
  - returns error for NULL destination
  - returns error for NULL source
  - returns error for zero-sized destination
  - handles empty source string
  - preserves destination on error
*/
struct d_test_object*
d_tests_string_fn_strcpy_s
(
    void
)
{
    struct d_test_object* group;
    char                  dest[D_TEST_DSTRING_BUFFER_SIZE];
    char                  small_dest[D_TEST_DSTRING_SMALL_BUFFER];
    char                  exact_dest[6];  // exactly for "Hello" + null
    int                   result;
    bool                  test_normal_copy;
    bool                  test_exact_fit;
    bool                  test_overflow;
    bool                  test_null_dest;
    bool                  test_null_src;
    bool                  test_zero_size;
    bool                  test_empty_src;
    bool                  test_preserve_on_error;
    size_t                idx;

    // test 1: normal copy
    memset(dest, 0, sizeof(dest));
    result = d_strcpy_s(dest, sizeof(dest), D_TEST_DSTRING_SHORT_STR);
    test_normal_copy = (result == 0) && 
                      (strcmp(dest, D_TEST_DSTRING_SHORT_STR) == 0);

    // test 2: exact-fit buffer
    memset(exact_dest, 0, sizeof(exact_dest));
    result = d_strcpy_s(exact_dest, sizeof(exact_dest), D_TEST_DSTRING_SHORT_STR);
    test_exact_fit = (result == 0) && 
                     (strcmp(exact_dest, D_TEST_DSTRING_SHORT_STR) == 0);

    // test 3: insufficient buffer
    memset(small_dest, 'X', sizeof(small_dest));
    small_dest[sizeof(small_dest) - 1] = '\0';
    result = d_strcpy_s(small_dest, sizeof(small_dest), D_TEST_DSTRING_LONG_STR);
    test_overflow = (result != 0);

    // test 4: NULL destination
    result = d_strcpy_s(NULL, sizeof(dest), D_TEST_DSTRING_SHORT_STR);
    test_null_dest = (result != 0);

    // test 5: NULL source
    memset(dest, 0, sizeof(dest));
    result = d_strcpy_s(dest, sizeof(dest), NULL);
    test_null_src = (result != 0);

    // test 6: zero-sized destination
    result = d_strcpy_s(dest, 0, D_TEST_DSTRING_SHORT_STR);
    test_zero_size = (result != 0);

    // test 7: empty source string
    memset(dest, 'Y', sizeof(dest));
    result = d_strcpy_s(dest, sizeof(dest), "");
    test_empty_src = (result == 0) && (dest[0] == '\0');

    // test 8: preserve destination on error
    strcpy(dest, "Original");
    result = d_strcpy_s(dest, 5, D_TEST_DSTRING_LONG_STR);  // too small
    test_preserve_on_error = (result != 0) && (dest[0] == '\0');  // should be nulled

    // build result tree
    group = d_test_object_new_interior("d_strcpy_s", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("normal_copy",
                                           test_normal_copy,
                                           "copies string to adequate buffer");
    group->elements[idx++] = D_ASSERT_TRUE("exact_fit",
                                           test_exact_fit,
                                           "handles exact-fit buffer");
    group->elements[idx++] = D_ASSERT_TRUE("overflow",
                                           test_overflow,
                                           "returns error for insufficient buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_dest",
                                           test_null_dest,
                                           "returns error for NULL destination");
    group->elements[idx++] = D_ASSERT_TRUE("null_src",
                                           test_null_src,
                                           "returns error for NULL source");
    group->elements[idx++] = D_ASSERT_TRUE("zero_size",
                                           test_zero_size,
                                           "returns error for zero-sized dest");
    group->elements[idx++] = D_ASSERT_TRUE("empty_src",
                                           test_empty_src,
                                           "handles empty source string");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_on_error",
                                           test_preserve_on_error,
                                           "nulls destination on error");

    return group;
}


/*
d_tests_string_fn_strncpy_s
  Tests d_strncpy_s for safe counted string copying.
  Tests the following:
  - copies n characters correctly
  - null-terminates when copying less than source length
  - handles count larger than source
  - returns error for insufficient buffer
  - returns error for NULL parameters
  - handles zero count
  - handles count equal to buffer size
*/
struct d_test_object*
d_tests_string_fn_strncpy_s
(
    void
)
{
    struct d_test_object* group;
    char                  dest[D_TEST_DSTRING_BUFFER_SIZE];
    char                  small_dest[D_TEST_DSTRING_SMALL_BUFFER];
    int                   result;
    bool                  test_partial_copy;
    bool                  test_null_termination;
    bool                  test_count_larger;
    bool                  test_overflow;
    bool                  test_null_params;
    bool                  test_zero_count;
    bool                  test_count_equal_size;
    size_t                idx;

    // test 1: partial copy (first 5 chars)
    memset(dest, 0, sizeof(dest));
    result = d_strncpy_s(dest, sizeof(dest), D_TEST_DSTRING_MEDIUM_STR, 5);
    test_partial_copy = (result == 0) && 
                       (strncmp(dest, D_TEST_DSTRING_MEDIUM_STR, 5) == 0) &&
                       (strlen(dest) == 5);

    // test 2: null termination
    memset(dest, 'Z', sizeof(dest));
    result = d_strncpy_s(dest, sizeof(dest), D_TEST_DSTRING_SHORT_STR, 3);
    test_null_termination = (result == 0) && 
                           (dest[3] == '\0') && 
                           (strlen(dest) == 3);

    // test 3: count larger than source
    memset(dest, 0, sizeof(dest));
    result = d_strncpy_s(dest, sizeof(dest), D_TEST_DSTRING_SHORT_STR, 100);
    test_count_larger = (result == 0) && 
                       (strcmp(dest, D_TEST_DSTRING_SHORT_STR) == 0);

    // test 4: insufficient buffer
    result = d_strncpy_s(small_dest, sizeof(small_dest), D_TEST_DSTRING_LONG_STR, 50);
    test_overflow = (result != 0);

    // test 5: NULL parameters
    result = d_strncpy_s(NULL, sizeof(dest), D_TEST_DSTRING_SHORT_STR, 5);
    test_null_params = (result != 0);

    // test 6: zero count
    memset(dest, 'A', sizeof(dest));
    dest[sizeof(dest) - 1] = '\0';
    result = d_strncpy_s(dest, sizeof(dest), D_TEST_DSTRING_SHORT_STR, 0);
    test_zero_count = (result == 0) && (dest[0] == '\0');

    // test 7: count equal to buffer size
    memset(small_dest, 0, sizeof(small_dest));
    result = d_strncpy_s(small_dest, sizeof(small_dest), D_TEST_DSTRING_LONG_STR, 
                         sizeof(small_dest) - 1);
    test_count_equal_size = (result == 0) && 
                            (strlen(small_dest) == sizeof(small_dest) - 1);

    // build result tree
    group = d_test_object_new_interior("d_strncpy_s", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("partial_copy",
                                           test_partial_copy,
                                           "copies n characters correctly");
    group->elements[idx++] = D_ASSERT_TRUE("null_termination",
                                           test_null_termination,
                                           "null-terminates partial copy");
    group->elements[idx++] = D_ASSERT_TRUE("count_larger",
                                           test_count_larger,
                                           "handles count larger than source");
    group->elements[idx++] = D_ASSERT_TRUE("overflow",
                                           test_overflow,
                                           "returns error for insufficient buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_params",
                                           test_null_params,
                                           "returns error for NULL parameters");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "handles zero count");
    group->elements[idx++] = D_ASSERT_TRUE("count_equal_size",
                                           test_count_equal_size,
                                           "handles count equal to buffer size");

    return group;
}


/*
d_tests_string_fn_strcat_s
  Tests d_strcat_s for safe string concatenation.
  Tests the following:
  - concatenates strings correctly
  - handles empty destination
  - handles empty source
  - returns error for insufficient buffer
  - returns error for NULL parameters
  - preserves original on error
  - handles exact-fit scenario
*/
struct d_test_object*
d_tests_string_fn_strcat_s
(
    void
)
{
    struct d_test_object* group;
    char                  dest[D_TEST_DSTRING_BUFFER_SIZE];
    char                  small_dest[D_TEST_DSTRING_SMALL_BUFFER];
    char                  exact_dest[11];  // for "Hello" + "World" + null
    int                   result;
    bool                  test_normal_concat;
    bool                  test_empty_dest;
    bool                  test_empty_src;
    bool                  test_overflow;
    bool                  test_null_params;
    bool                  test_preserve_on_error;
    bool                  test_exact_fit;
    size_t                idx;

    // test 1: normal concatenation
    strcpy(dest, D_TEST_DSTRING_SHORT_STR);
    result = d_strcat_s(dest, sizeof(dest), " World");
    test_normal_concat = (result == 0) && 
                        (strcmp(dest, "Hello World") == 0);

    // test 2: empty destination
    dest[0] = '\0';
    result = d_strcat_s(dest, sizeof(dest), D_TEST_DSTRING_SHORT_STR);
    test_empty_dest = (result == 0) && 
                     (strcmp(dest, D_TEST_DSTRING_SHORT_STR) == 0);

    // test 3: empty source
    strcpy(dest, D_TEST_DSTRING_SHORT_STR);
    result = d_strcat_s(dest, sizeof(dest), "");
    test_empty_src = (result == 0) && 
                    (strcmp(dest, D_TEST_DSTRING_SHORT_STR) == 0);

    // test 4: insufficient buffer
    strcpy(small_dest, "12345678");  // 8 chars
    result = d_strcat_s(small_dest, sizeof(small_dest), "ABCDEF");  // would overflow
    test_overflow = (result != 0);

    // test 5: NULL parameters
    result = d_strcat_s(NULL, sizeof(dest), "test");
    test_null_params = (result != 0);

    // test 6: preserve on error
    strcpy(small_dest, "Original");
    result = d_strcat_s(small_dest, sizeof(small_dest), D_TEST_DSTRING_LONG_STR);
    test_preserve_on_error = (result != 0) && (small_dest[0] == '\0');

    // test 7: exact fit
    strcpy(exact_dest, "Hello");
    result = d_strcat_s(exact_dest, sizeof(exact_dest), "World");
    test_exact_fit = (result == 0) && 
                    (strcmp(exact_dest, "HelloWorld") == 0);

    // build result tree
    group = d_test_object_new_interior("d_strcat_s", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("normal_concat",
                                           test_normal_concat,
                                           "concatenates strings correctly");
    group->elements[idx++] = D_ASSERT_TRUE("empty_dest",
                                           test_empty_dest,
                                           "handles empty destination");
    group->elements[idx++] = D_ASSERT_TRUE("empty_src",
                                           test_empty_src,
                                           "handles empty source");
    group->elements[idx++] = D_ASSERT_TRUE("overflow",
                                           test_overflow,
                                           "returns error for insufficient buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_params",
                                           test_null_params,
                                           "returns error for NULL parameters");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_on_error",
                                           test_preserve_on_error,
                                           "nulls destination on error");
    group->elements[idx++] = D_ASSERT_TRUE("exact_fit",
                                           test_exact_fit,
                                           "handles exact-fit scenario");

    return group;
}

/*
d_tests_string_fn_strncat_s
  Tests d_strncat_s for safe counted string concatenation.
  Tests the following:
  - concatenates n characters correctly
  - handles count larger than source
  - returns error for insufficient buffer
  - returns error for NULL parameters
  - handles zero count
  - preserves termination
*/
struct d_test_object*
d_tests_string_fn_strncat_s
(
    void
)
{
    struct d_test_object* group;
    char                  dest[D_TEST_DSTRING_BUFFER_SIZE];
    char                  small_dest[D_TEST_DSTRING_SMALL_BUFFER];
    int                   result;
    bool                  test_partial_concat;
    bool                  test_count_larger;
    bool                  test_overflow;
    bool                  test_null_params;
    bool                  test_zero_count;
    bool                  test_termination;
    size_t                idx;

    // test 1: partial concatenation
    strcpy(dest, "Hello");
    result = d_strncat_s(dest, sizeof(dest), " World!", 6);  // just " World"
    test_partial_concat = (result == 0) && 
                         (strcmp(dest, "Hello World") == 0);

    // test 2: count larger than source
    strcpy(dest, "Test");
    result = d_strncat_s(dest, sizeof(dest), "123", 100);
    test_count_larger = (result == 0) && 
                       (strcmp(dest, "Test123") == 0);

    // test 3: insufficient buffer
    strcpy(small_dest, "12345678");
    result = d_strncat_s(small_dest, sizeof(small_dest), "ABCDEF", 5);
    test_overflow = (result != 0);

    // test 4: NULL parameters
    result = d_strncat_s(NULL, sizeof(dest), "test", 4);
    test_null_params = (result != 0);

    // test 5: zero count
    strcpy(dest, "Original");
    result = d_strncat_s(dest, sizeof(dest), "Should not appear", 0);
    test_zero_count = (result == 0) && 
                     (strcmp(dest, "Original") == 0);

    // test 6: preserves termination
    memset(dest, 'X', sizeof(dest));
    strcpy(dest, "Start");
    result = d_strncat_s(dest, sizeof(dest), "End", 3);
    test_termination = (result == 0) && 
                      (dest[strlen("StartEnd")] == '\0');

    // build result tree
    group = d_test_object_new_interior("d_strncat_s", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("partial_concat",
                                           test_partial_concat,
                                           "concatenates n characters correctly");
    group->elements[idx++] = D_ASSERT_TRUE("count_larger",
                                           test_count_larger,
                                           "handles count larger than source");
    group->elements[idx++] = D_ASSERT_TRUE("overflow",
                                           test_overflow,
                                           "returns error for insufficient buffer");
    group->elements[idx++] = D_ASSERT_TRUE("null_params",
                                           test_null_params,
                                           "returns error for NULL parameters");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "handles zero count");
    group->elements[idx++] = D_ASSERT_TRUE("termination",
                                           test_termination,
                                           "preserves null termination");

    return group;
}

/*
d_tests_string_fn_safe_copy_all
  Runs all safe string copy tests.
  Tests the following:
  - d_strcpy_s
  - d_strncpy_s
  - d_strcat_s
  - d_strncat_s
*/
struct d_test_object*
d_tests_string_fn_safe_copy_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Safe String Copy Operations", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strcpy_s();
    group->elements[idx++] = d_tests_string_fn_strncpy_s();
    group->elements[idx++] = d_tests_string_fn_strcat_s();
    group->elements[idx++] = d_tests_string_fn_strncat_s();

    return group;
}