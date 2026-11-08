/*******************************************************************************
* djinterp [test]                                              string_fn_tests.h
*
* string_fn.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   The first block holds the register's decisions on string_fn (#11, #12 and
* #30 of docs/djinterp_decisions.md, Part B), each with a regression test.
*
*
* TABLE OF CONTENTS
* =================
* 1.  TESTS
*     -----
*     1.  The register's decisions
* 2.  SPEC
*     ----
*     1.  The provider
*     2.  The sections' entry points
* 3.  SECTIONS
*     --------
*
*
* path:      /tests/djinterp/c/string_fn_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_STRING_FN_TESTS_H
#define DJINTERP_TESTS_C_STRING_FN_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <stdlib.h>  // free
    #include <string.h>  // memset, strchr, strcmp
    // re_std
    #include "../../../inc/re_std/c/cstdint/dstdint.h"  // SIZE_MAX
    // djinterp
    #include "../../../inc/djinterp/c/string_fn.h"  // d_strerror_r, d_str*case*
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The register's decisions
//------------------------------------------------------------------------------
bool d_tests_string_fn_strerror_r(void);
bool d_tests_string_fn_ascii_only(void);
bool d_tests_string_fn_casestr_embedded_nul(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_string_fn_register_rows
//   constant: the register block's rows.
static const struct dj_test_case_spec d_tests_string_fn_register_rows[] =
{
    { "d_tests_string_fn_strerror_r",
      "#11: the platform's description, with EINVAL and ERANGE as documented",
      d_tests_string_fn_strerror_r },
    { "d_tests_string_fn_ascii_only",
      "#12: classes and case folding are ASCII, whatever the locale",
      d_tests_string_fn_ascii_only },
    { "d_tests_string_fn_casestr_embedded_nul",
      "#30: a case-insensitive search reads embedded NULs",
      d_tests_string_fn_casestr_embedded_nul }
};

// d_tests_string_fn_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_string_fn_blocks[] =
{
    { "register",
      "the register's decisions on string_fn, each with its regression test",
      d_tests_string_fn_register_rows,
      sizeof(d_tests_string_fn_register_rows) /
          sizeof(d_tests_string_fn_register_rows[0]) }
};

// d_tests_string_fn_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_string_fn_module =
{
    "string_fn",
    "string_fn.h: raw-buffer string primitives",
    d_tests_string_fn_blocks,
    sizeof(d_tests_string_fn_blocks) / sizeof(d_tests_string_fn_blocks[0])
};

// d_tests_string_fn_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_string_fn_spec(void)
{
    return &d_tests_string_fn_module;
}

// 2.2    The sections' entry points
//------------------------------------------------------------------------------
// The tests built call by call (section 3), which the runner calls after
// adding the spec.
bool d_tests_string_fn_run_all(struct dj_test_suite* _suite);
bool d_tests_string_fn_length_aware_cmp_all(struct dj_test_suite* _suite);
bool d_tests_string_fn_counting_all(struct dj_test_suite* _suite);
bool d_tests_string_fn_hash_all(struct dj_test_suite* _suite);
bool d_tests_string_fn_index_search_all(struct dj_test_suite* _suite);
bool d_tests_string_fn_replace_all(struct dj_test_suite* _suite);
bool d_tests_string_fn_prefix_suffix_all(struct dj_test_suite* _suite);
bool d_tests_string_fn_validation_all(struct dj_test_suite* _suite);


#endif  // DTEST_SPEC_MODE


//==============================================================================
// 3.  SECTIONS
//==============================================================================
// The module's tests built call by call (test_suite.h 3.2), outside spec
// mode only, since they read the module's own types.
#ifndef DTEST_SPEC_MODE

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

bool d_tests_string_fn_setup(void);
bool d_tests_string_fn_teardown(void);
void d_tests_string_fn_fill_buffer(
    char*  _buffer,
    size_t _size,
    char   _pattern
);
bool d_tests_string_fn_compare_buffers(
    const char* _buf1,
    const char* _buf2,
    size_t      _size
);

// safe string copy tests
bool d_tests_string_fn_strcpy_s(struct dj_test_suite* _suite);
bool d_tests_string_fn_strncpy_s(struct dj_test_suite* _suite);
bool d_tests_string_fn_strcat_s(struct dj_test_suite* _suite);
bool d_tests_string_fn_strncat_s(struct dj_test_suite* _suite);
bool d_tests_string_fn_safe_copy_all(struct dj_test_suite* _suite);

// string duplication tests
bool d_tests_string_fn_strdup(struct dj_test_suite* _suite);
bool d_tests_string_fn_strndup(struct dj_test_suite* _suite);
bool d_tests_string_fn_duplication_all(struct dj_test_suite* _suite);

// case-insensitive comparison tests
bool d_tests_string_fn_strcasecmp(struct dj_test_suite* _suite);
bool d_tests_string_fn_strncasecmp(struct dj_test_suite* _suite);
bool d_tests_string_fn_case_comparison_all(struct dj_test_suite* _suite);

// string tokenization tests
bool d_tests_string_fn_strtok_r(struct dj_test_suite* _suite);
bool d_tests_string_fn_tokenization_all(struct dj_test_suite* _suite);

// string length tests
bool d_tests_string_fn_strnlen(struct dj_test_suite* _suite);
bool d_tests_string_fn_length_all(struct dj_test_suite* _suite);

// string search tests
bool d_tests_string_fn_strcasestr(struct dj_test_suite* _suite);
bool d_tests_string_fn_strchrnul(struct dj_test_suite* _suite);
bool d_tests_string_fn_search_all(struct dj_test_suite* _suite);

// case conversion tests
bool d_tests_string_fn_strlwr(struct dj_test_suite* _suite);
bool d_tests_string_fn_strupr(struct dj_test_suite* _suite);
bool d_tests_string_fn_case_conversion_all(struct dj_test_suite* _suite);

// string manipulation tests
bool d_tests_string_fn_strrev(struct dj_test_suite* _suite);
bool d_tests_string_fn_manipulation_all(struct dj_test_suite* _suite);

// error handling tests
bool d_tests_string_fn_strerror_r_cases(struct dj_test_suite* _suite);
bool d_tests_string_fn_error_handling_all(struct dj_test_suite* _suite);

// null parameter tests
bool d_tests_string_fn_null_params_all(struct dj_test_suite* _suite);

// boundary condition tests
bool d_tests_string_fn_boundary_conditions_all(struct dj_test_suite* _suite);

// length-aware comparison tests
bool d_tests_string_fn_strcmp_n(struct dj_test_suite* _suite);
bool d_tests_string_fn_strncmp_n(struct dj_test_suite* _suite);
bool d_tests_string_fn_strcasecmp_n(struct dj_test_suite* _suite);
bool d_tests_string_fn_strncasecmp_n(struct dj_test_suite* _suite);
bool d_tests_string_fn_strequals(struct dj_test_suite* _suite);
bool d_tests_string_fn_strequals_nocase(struct dj_test_suite* _suite);
bool d_tests_string_fn_length_aware_cmp_all(struct dj_test_suite* _suite);

// validation tests
bool d_tests_string_fn_str_is_valid(struct dj_test_suite* _suite);
bool d_tests_string_fn_str_is_ascii(struct dj_test_suite* _suite);
bool d_tests_string_fn_str_is_numeric(struct dj_test_suite* _suite);
bool d_tests_string_fn_str_is_alpha(struct dj_test_suite* _suite);
bool d_tests_string_fn_str_is_alnum(struct dj_test_suite* _suite);
bool d_tests_string_fn_str_is_whitespace(struct dj_test_suite* _suite);
bool d_tests_string_fn_validation_all(struct dj_test_suite* _suite);

// counting tests
bool d_tests_string_fn_strcount_char(struct dj_test_suite* _suite);
bool d_tests_string_fn_strcount_substr(struct dj_test_suite* _suite);
bool d_tests_string_fn_counting_all(struct dj_test_suite* _suite);

// hash tests
bool d_tests_string_fn_strhash(struct dj_test_suite* _suite);
bool d_tests_string_fn_hash_all(struct dj_test_suite* _suite);

// prefix, suffix, and containment tests
bool d_tests_string_fn_strstartswith(struct dj_test_suite* _suite);
bool d_tests_string_fn_strendswith(struct dj_test_suite* _suite);
bool d_tests_string_fn_strcontains(struct dj_test_suite* _suite);
bool d_tests_string_fn_strcontains_char(struct dj_test_suite* _suite);
bool d_tests_string_fn_prefix_suffix_all(struct dj_test_suite* _suite);

// index-returning search tests
bool d_tests_string_fn_strchr_index(struct dj_test_suite* _suite);
bool d_tests_string_fn_strchr_index_from(struct dj_test_suite* _suite);
bool d_tests_string_fn_strrchr_index(struct dj_test_suite* _suite);
bool d_tests_string_fn_strstr_index(struct dj_test_suite* _suite);
bool d_tests_string_fn_strstr_index_from(struct dj_test_suite* _suite);
bool d_tests_string_fn_strrstr_index(struct dj_test_suite* _suite);
bool d_tests_string_fn_strcasestr_index(struct dj_test_suite* _suite);
bool d_tests_string_fn_index_search_all(struct dj_test_suite* _suite);

// in-place character replacement tests
bool d_tests_string_fn_strreplace_char(struct dj_test_suite* _suite);
bool d_tests_string_fn_replace_all(struct dj_test_suite* _suite);

// master test runner
bool d_tests_string_fn_run_all(struct dj_test_suite* _suite);

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_STRING_FN_TESTS_H
