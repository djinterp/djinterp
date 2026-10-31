/*******************************************************************************
* djinterp [test]                                           string_fn_tests_sa.h
*
*   Unit tests for the string_fn module (cross-platform string operations).
*   Tests cover secure string copying, duplication, case operations,
* tokenization, length limiting, searching, error handling, length-aware
* comparison, validation, counting, hashing, prefix/suffix/containment,
* index-returning search, and in-place character replacement.
*
*
* path:      /tests/djinterp/c/string_fn_tests_sa.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_STRING_FN_TESTS_STANDALONE_
#define DJINTERP_STRING_FN_TESTS_STANDALONE_ 1

// djinterp
#include "../../../inc/djinterp/c/string_fn.h"


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_DSTRING_SHORT_STR
//   constant: short test string.
#define D_TEST_DSTRING_SHORT_STR        "Hello"

// D_TEST_DSTRING_MEDIUM_STR
//   constant: medium test string.
#define D_TEST_DSTRING_MEDIUM_STR       "Hello, World! Testing 123"

// D_TEST_DSTRING_LONG_STR
//   constant: long test string.
#define D_TEST_DSTRING_LONG_STR         "The quick brown fox jumps over the lazy dog. " \
                                        "Pack my box with five dozen liquor jugs."

// D_TEST_DSTRING_MIXED_CASE
//   constant: mixed case string for case operations.
#define D_TEST_DSTRING_MIXED_CASE       "HeLLo WoRLd 123 !@#"

// D_TEST_DSTRING_UNICODE_STR
//   constant: string with unicode/special characters.
#define D_TEST_DSTRING_UNICODE_STR      "CafÃ© naÃ¯ve rÃ©sumÃ©"

// D_TEST_DSTRING_BUFFER_SIZE
//   constant: standard buffer size for tests.
#define D_TEST_DSTRING_BUFFER_SIZE      256

// D_TEST_DSTRING_SMALL_BUFFER
//   constant: small buffer size for overflow tests.
#define D_TEST_DSTRING_SMALL_BUFFER     10


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

bool d_tests_sa_string_fn_setup(void);
bool d_tests_sa_string_fn_teardown(void);
void d_tests_sa_string_fn_fill_buffer(
    char*  _buffer,
    size_t _size,
    char   _pattern
);
bool d_tests_sa_string_fn_compare_buffers(
    const char* _buf1,
    const char* _buf2,
    size_t      _size
);

// safe string copy tests
struct dj_test_object* d_tests_sa_string_fn_strcpy_s(void);
struct dj_test_object* d_tests_sa_string_fn_strncpy_s(void);
struct dj_test_object* d_tests_sa_string_fn_strcat_s(void);
struct dj_test_object* d_tests_sa_string_fn_strncat_s(void);
struct dj_test_object* d_tests_sa_string_fn_safe_copy_all(void);

// string duplication tests
struct dj_test_object* d_tests_sa_string_fn_strdup(void);
struct dj_test_object* d_tests_sa_string_fn_strndup(void);
struct dj_test_object* d_tests_sa_string_fn_duplication_all(void);

// case-insensitive comparison tests
struct dj_test_object* d_tests_sa_string_fn_strcasecmp(void);
struct dj_test_object* d_tests_sa_string_fn_strncasecmp(void);
struct dj_test_object* d_tests_sa_string_fn_case_comparison_all(void);

// string tokenization tests
struct dj_test_object* d_tests_sa_string_fn_strtok_r(void);
struct dj_test_object* d_tests_sa_string_fn_tokenization_all(void);

// string length tests
struct dj_test_object* d_tests_sa_string_fn_strnlen(void);
struct dj_test_object* d_tests_sa_string_fn_length_all(void);

// string search tests
struct dj_test_object* d_tests_sa_string_fn_strcasestr(void);
struct dj_test_object* d_tests_sa_string_fn_strchrnul(void);
struct dj_test_object* d_tests_sa_string_fn_search_all(void);

// case conversion tests
struct dj_test_object* d_tests_sa_string_fn_strlwr(void);
struct dj_test_object* d_tests_sa_string_fn_strupr(void);
struct dj_test_object* d_tests_sa_string_fn_case_conversion_all(void);

// string manipulation tests
struct dj_test_object* d_tests_sa_string_fn_strrev(void);
struct dj_test_object* d_tests_sa_string_fn_manipulation_all(void);

// error handling tests
struct dj_test_object* d_tests_sa_string_fn_strerror_r(void);
struct dj_test_object* d_tests_sa_string_fn_error_handling_all(void);

// null parameter tests
struct dj_test_object* d_tests_sa_string_fn_null_params_all(void);

// boundary condition tests
struct dj_test_object* d_tests_sa_string_fn_boundary_conditions_all(void);

// length-aware comparison tests
struct dj_test_object* d_tests_sa_string_fn_strcmp_n(void);
struct dj_test_object* d_tests_sa_string_fn_strncmp_n(void);
struct dj_test_object* d_tests_sa_string_fn_strcasecmp_n(void);
struct dj_test_object* d_tests_sa_string_fn_strncasecmp_n(void);
struct dj_test_object* d_tests_sa_string_fn_strequals(void);
struct dj_test_object* d_tests_sa_string_fn_strequals_nocase(void);
struct dj_test_object* d_tests_sa_string_fn_length_aware_cmp_all(void);

// validation tests
struct dj_test_object* d_tests_sa_string_fn_str_is_valid(void);
struct dj_test_object* d_tests_sa_string_fn_str_is_ascii(void);
struct dj_test_object* d_tests_sa_string_fn_str_is_numeric(void);
struct dj_test_object* d_tests_sa_string_fn_str_is_alpha(void);
struct dj_test_object* d_tests_sa_string_fn_str_is_alnum(void);
struct dj_test_object* d_tests_sa_string_fn_str_is_whitespace(void);
struct dj_test_object* d_tests_sa_string_fn_validation_all(void);

// counting tests
struct dj_test_object* d_tests_sa_string_fn_strcount_char(void);
struct dj_test_object* d_tests_sa_string_fn_strcount_substr(void);
struct dj_test_object* d_tests_sa_string_fn_counting_all(void);

// hash tests
struct dj_test_object* d_tests_sa_string_fn_strhash(void);
struct dj_test_object* d_tests_sa_string_fn_hash_all(void);

// prefix, suffix, and containment tests
struct dj_test_object* d_tests_sa_string_fn_strstartswith(void);
struct dj_test_object* d_tests_sa_string_fn_strendswith(void);
struct dj_test_object* d_tests_sa_string_fn_strcontains(void);
struct dj_test_object* d_tests_sa_string_fn_strcontains_char(void);
struct dj_test_object* d_tests_sa_string_fn_prefix_suffix_all(void);

// index-returning search tests
struct dj_test_object* d_tests_sa_string_fn_strchr_index(void);
struct dj_test_object* d_tests_sa_string_fn_strchr_index_from(void);
struct dj_test_object* d_tests_sa_string_fn_strrchr_index(void);
struct dj_test_object* d_tests_sa_string_fn_strstr_index(void);
struct dj_test_object* d_tests_sa_string_fn_strstr_index_from(void);
struct dj_test_object* d_tests_sa_string_fn_strrstr_index(void);
struct dj_test_object* d_tests_sa_string_fn_strcasestr_index(void);
struct dj_test_object* d_tests_sa_string_fn_index_search_all(void);

// in-place character replacement tests
struct dj_test_object* d_tests_sa_string_fn_strreplace_char(void);
struct dj_test_object* d_tests_sa_string_fn_replace_all(void);

// master test runner
struct dj_test_object* d_tests_sa_string_fn_run_all(void);


#endif  // DJINTERP_STRING_FN_TESTS_STANDALONE_
