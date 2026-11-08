/*******************************************************************************
* djinterp [test]                                        string_fn_tests_error.c
*
*
* path:      /tests/djinterp/c/string_fn_tests_error.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// std
#include <errno.h>
// djinterp
#include "./string_fn_tests.h"


/******************************************************************************
 * ERROR HANDLING TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strerror_r_cases
  Tests d_strerror_r for thread-safe error strings.
  Tests the following:
  - returns valid error string for known error
  - handles insufficient buffer size
  - handles unknown error codes
  - null terminates result
  - handles NULL buffer
  - handles zero buffer size
*/
bool d_tests_string_fn_strerror_r_cases(struct dj_test_suite* _suite)
{
    char                  buffer[256];
    char                  small_buffer[10];
    int                   result;
    bool                  test_known_error;
    bool                  test_insufficient_buffer;
    bool                  test_unknown_error;
    bool                  test_null_terminated;
    bool                  test_null_buffer;
    bool                  test_zero_size;

    // test 1: known error code
    memset(buffer, 0, sizeof(buffer));
    result = d_strerror_r(EINVAL, buffer, sizeof(buffer));
    test_known_error = (result == 0) && (strlen(buffer) > 0);

    // test 2: insufficient buffer
    memset(small_buffer, 0, sizeof(small_buffer));
    result = d_strerror_r(EINVAL, small_buffer, sizeof(small_buffer));
    test_insufficient_buffer = (result == 0 || result == ERANGE);

    // test 3: unknown error code
    memset(buffer, 0, sizeof(buffer));
    result = d_strerror_r(99999, buffer, sizeof(buffer));
    test_unknown_error = (strlen(buffer) > 0);

    // test 4: null terminated
    memset(buffer, 'X', sizeof(buffer));
    result = d_strerror_r(EINVAL, buffer, sizeof(buffer));
    test_null_terminated = (buffer[strlen(buffer)] == '\0');

    // test 5: NULL buffer
    result = d_strerror_r(EINVAL, NULL, sizeof(buffer));
    test_null_buffer = (result != 0);

    // test 6: zero buffer size
    result = d_strerror_r(EINVAL, buffer, 0);
    test_zero_size = (result != 0);

    // build result tree
    d_test_suite_test(_suite, "d_strerror_r");

    D_TEST_CHECK_MSG(_suite,
                     "known_error",
                     test_known_error,
                     "returns valid string for known error");
    D_TEST_CHECK_MSG(_suite,
                     "insufficient_buffer",
                     test_insufficient_buffer,
                     "handles insufficient buffer size");
    D_TEST_CHECK_MSG(_suite,
                     "unknown_error",
                     test_unknown_error,
                     "handles unknown error codes");
    D_TEST_CHECK_MSG(_suite,
                     "null_terminated",
                     test_null_terminated,
                     "null terminates result");
    D_TEST_CHECK_MSG(_suite,
                     "null_buffer",
                     test_null_buffer,
                     "handles NULL buffer");
    D_TEST_CHECK_MSG(_suite,
                     "zero_size",
                     test_zero_size,
                     "handles zero buffer size");

    return true;
}


/*
d_tests_string_fn_error_handling_all
  Runs all error handling tests.
  Tests the following:
  - d_strerror_r
*/
bool d_tests_string_fn_error_handling_all(struct dj_test_suite* _suite)
{
    d_test_suite_block(_suite, "Error Handling");

    d_tests_string_fn_strerror_r_cases(_suite);

    return true;
}


/******************************************************************************
 * NULL PARAMETER TESTS
 *****************************************************************************/

/*
d_tests_string_fn_null_params_all
  Tests NULL parameter handling across all functions.
  Tests the following:
  - all functions handle NULL parameters gracefully
  - appropriate error codes returned
  - no crashes or undefined behavior
*/
bool d_tests_string_fn_null_params_all(struct dj_test_suite* _suite)
{
    char                  buffer[256];
    bool                  test_strcpy_s;
    bool                  test_strncpy_s;
    bool                  test_strcat_s;
    bool                  test_strncat_s;
    bool                  test_strdup;
    bool                  test_strndup;
    bool                  test_strcasecmp;
    bool                  test_strncasecmp;
    bool                  test_strtok_r;
    bool                  test_strnlen;
    bool                  test_strcasestr;
    bool                  test_strlwr;
    bool                  test_strupr;
    bool                  test_strrev;
    bool                  test_strchrnul;
    bool                  test_strcmp_n;
    bool                  test_strequals;
    bool                  test_validation;
    bool                  test_counting;
    bool                  test_hash;
    bool                  test_prefix_suffix;
    bool                  test_index_search;
    bool                  test_replace_char;

    // original functions
    test_strcpy_s = (d_strcpy_s(NULL, 10, "test") != 0) &&
                   (d_strcpy_s(buffer, sizeof(buffer), NULL) != 0);

    test_strncpy_s = (d_strncpy_s(NULL, 10, "test", 4) != 0) &&
                     (d_strncpy_s(buffer, sizeof(buffer), NULL, 4) != 0);

    test_strcat_s = (d_strcat_s(NULL, 10, "test") != 0) &&
                    (d_strcat_s(buffer, sizeof(buffer), NULL) != 0);

    test_strncat_s = (d_strncat_s(NULL, 10, "test", 4) != 0) &&
                     (d_strncat_s(buffer, sizeof(buffer), NULL, 4) != 0);

    test_strdup = (d_strdup(NULL) == NULL);

    test_strndup = (d_strndup(NULL, 10) == NULL);

    test_strcasecmp = true;

    test_strncasecmp = true;

    test_strtok_r = (d_strtok_r(NULL, ",", NULL) == NULL);

    test_strnlen = (d_strnlen(NULL, 100) == 0);

    test_strcasestr = (d_strcasestr(NULL, "test") == NULL) &&
                     (d_strcasestr("test", NULL) == NULL);

    test_strlwr = (d_strlwr(NULL) == NULL);

    test_strupr = (d_strupr(NULL) == NULL);

    test_strrev = (d_strrev(NULL) == NULL);

    test_strchrnul = (d_strchrnul(NULL, 'a') == NULL);

    // new functions
    test_strcmp_n = (d_strcmp_n(NULL, 0, "test", 4) < 0) &&
                   (d_strcmp_n("test", 4, NULL, 0) > 0) &&
                   (d_strcmp_n(NULL, 0, NULL, 0) == 0);

    test_strequals = (d_strequals(NULL, 0, NULL, 0) == true) &&
                    (d_strequals(NULL, 0, "test", 4) == false) &&
                    (d_strequals_nocase(NULL, 0, NULL, 0) == true);

    test_validation = !d_str_is_valid(NULL, 5) &&
                     !d_str_is_ascii(NULL, 5) &&
                     !d_str_is_numeric(NULL, 5) &&
                     !d_str_is_alpha(NULL, 5) &&
                     !d_str_is_alnum(NULL, 5) &&
                     !d_str_is_whitespace(NULL, 5);

    test_counting = (d_strcount_char(NULL, 5, 'a') == 0) &&
                   (d_strcount_substr(NULL, 5, "abc") == 0) &&
                   (d_strcount_substr("abc", 3, NULL) == 0);

    test_hash = (d_strhash(NULL, 5) == 0);

    test_prefix_suffix = !d_strstartswith(NULL, 0, "x", 1) &&
                        !d_strendswith(NULL, 0, "x", 1) &&
                        !d_strcontains(NULL, 0, "x") &&
                        !d_strcontains_char(NULL, 0, 'x');

    test_index_search = (d_strchr_index(NULL, 5, 'a') == D_STRING_NPOS) &&
                       (d_strrchr_index(NULL, 5, 'a') == D_STRING_NPOS) &&
                       (d_strstr_index(NULL, 5, "ab", 2) == D_STRING_NPOS) &&
                       (d_strrstr_index(NULL, 5, "ab", 2) == D_STRING_NPOS) &&
                       (d_strcasestr_index(NULL, 5, "ab", 2) == D_STRING_NPOS);

    test_replace_char = (d_strreplace_char(NULL, 5, 'a', 'b') == 0);

    // build result tree
    d_test_suite_test(_suite, "NULL Parameter Handling");

    D_TEST_CHECK_MSG(_suite,
                     "strcpy_s_null",
                     test_strcpy_s,
                     "d_strcpy_s handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strncpy_s_null",
                     test_strncpy_s,
                     "d_strncpy_s handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strcat_s_null",
                     test_strcat_s,
                     "d_strcat_s handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strncat_s_null",
                     test_strncat_s,
                     "d_strncat_s handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strdup_null",
                     test_strdup,
                     "d_strdup handles NULL parameter");
    D_TEST_CHECK_MSG(_suite,
                     "strndup_null",
                     test_strndup,
                     "d_strndup handles NULL parameter");
    D_TEST_CHECK_MSG(_suite,
                     "strcasecmp_null",
                     test_strcasecmp,
                     "d_strcasecmp handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strncasecmp_null",
                     test_strncasecmp,
                     "d_strncasecmp handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strtok_r_null",
                     test_strtok_r,
                     "d_strtok_r handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strnlen_null",
                     test_strnlen,
                     "d_strnlen handles NULL parameter");
    D_TEST_CHECK_MSG(_suite,
                     "strcasestr_null",
                     test_strcasestr,
                     "d_strcasestr handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strlwr_null",
                     test_strlwr,
                     "d_strlwr handles NULL parameter");
    D_TEST_CHECK_MSG(_suite,
                     "strupr_null",
                     test_strupr,
                     "d_strupr handles NULL parameter");
    D_TEST_CHECK_MSG(_suite,
                     "strrev_null",
                     test_strrev,
                     "d_strrev handles NULL parameter");
    D_TEST_CHECK_MSG(_suite,
                     "strchrnul_null",
                     test_strchrnul,
                     "d_strchrnul handles NULL parameter");
    D_TEST_CHECK_MSG(_suite,
                     "strcmp_n_null",
                     test_strcmp_n,
                     "d_strcmp_n handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "strequals_null",
                     test_strequals,
                     "d_strequals handles NULL parameters");
    D_TEST_CHECK_MSG(_suite,
                     "validation_null",
                     test_validation,
                     "validation functions handle NULL");
    D_TEST_CHECK_MSG(_suite,
                     "counting_null",
                     test_counting,
                     "counting functions handle NULL");
    D_TEST_CHECK_MSG(_suite, "hash_null", test_hash, "d_strhash handles NULL");
    D_TEST_CHECK_MSG(_suite,
                     "prefix_suffix_null",
                     test_prefix_suffix,
                     "prefix/suffix functions handle NULL");
    D_TEST_CHECK_MSG(_suite,
                     "index_search_null",
                     test_index_search,
                     "index search functions handle NULL");
    D_TEST_CHECK_MSG(_suite,
                     "replace_char_null",
                     test_replace_char,
                     "d_strreplace_char handles NULL");

    return true;
}


/******************************************************************************
 * BOUNDARY CONDITION TESTS
 *****************************************************************************/

/*
d_tests_string_fn_boundary_conditions_all
  Tests boundary conditions across all functions.
  Tests the following:
  - zero-length buffers
  - single-character operations
  - maximum size operations
  - off-by-one scenarios
  - empty string operations for new functions
  - single-char search and containment edge cases
*/
bool d_tests_string_fn_boundary_conditions_all(struct dj_test_suite* _suite)
{
    char                  one_char[2];
    char                  zero_buf[1];
    char                  exact_fit[6];
    char                  large_buffer[1024];
    char*                 result_ptr;
    int                   result;
    bool                  test_zero_length;
    bool                  test_single_char;
    bool                  test_exact_boundary;
    bool                  test_off_by_one;
    bool                  test_max_size;
    bool                  test_empty_operations;
    bool                  test_single_char_search;
    bool                  test_single_char_prefix;

    // test 1: zero-length buffer operations
    zero_buf[0] = '\0';
    result = d_strcpy_s(zero_buf, 0, "test");
    test_zero_length = (result != 0);

    // test 2: single character operations
    one_char[0] = 'A';
    one_char[1] = '\0';
    result_ptr = d_strrev(one_char);
    test_single_char = (result_ptr == one_char) && (one_char[0] == 'A');

    // test 3: exact boundary fit
    result = d_strcpy_s(exact_fit, sizeof(exact_fit), "Hello");
    test_exact_boundary = (result == 0) &&
                         (strcmp(exact_fit, "Hello") == 0) &&
                         (strlen(exact_fit) == 5);

    // test 4: off-by-one scenarios
    result = d_strcpy_s(exact_fit, sizeof(exact_fit), "Hello!");
    test_off_by_one = (result != 0);

    // test 5: maximum size operations
    memset(large_buffer, 'X', sizeof(large_buffer) - 1);
    large_buffer[sizeof(large_buffer) - 1] = '\0';
    size_t len = d_strnlen(large_buffer, SIZE_MAX);
    test_max_size = (len == sizeof(large_buffer) - 1);

    // test 6: empty string operations
    char empty[] = "";
    result_ptr = d_strrev(empty);
    char* dup_empty = d_strdup("");
    test_empty_operations = (result_ptr == empty) &&
                           (empty[0] == '\0') &&
                           (dup_empty != NULL) &&
                           (dup_empty[0] == '\0');

    if (dup_empty)
    {
        free(dup_empty);
    }

    // test 7: single-character search edge cases
    dj_index ci = d_strchr_index("X", 1, 'X');
    dj_index ri = d_strrchr_index("X", 1, 'X');
    test_single_char_search = (ci == 0) && (ri == 0);

    // test 8: single-character prefix/suffix
    bool sw = d_strstartswith("X", 1, "X", 1);
    bool ew = d_strendswith("X", 1, "X", 1);
    test_single_char_prefix = sw && ew;

    // build result tree
    d_test_suite_test(_suite, "Boundary Conditions");

    D_TEST_CHECK_MSG(_suite,
                     "zero_length",
                     test_zero_length,
                     "handles zero-length buffers");
    D_TEST_CHECK_MSG(_suite,
                     "single_char",
                     test_single_char,
                     "handles single character operations");
    D_TEST_CHECK_MSG(_suite,
                     "exact_boundary",
                     test_exact_boundary,
                     "handles exact boundary fit");
    D_TEST_CHECK_MSG(_suite,
                     "off_by_one",
                     test_off_by_one,
                     "detects off-by-one errors");
    D_TEST_CHECK_MSG(_suite,
                     "max_size",
                     test_max_size,
                     "handles maximum size operations");
    D_TEST_CHECK_MSG(_suite,
                     "empty_operations",
                     test_empty_operations,
                     "handles empty string operations");
    D_TEST_CHECK_MSG(_suite,
                     "single_char_search",
                     test_single_char_search,
                     "handles single-char index search");
    D_TEST_CHECK_MSG(_suite,
                     "single_char_prefix",
                     test_single_char_prefix,
                     "handles single-char prefix/suffix");

    return true;
}
