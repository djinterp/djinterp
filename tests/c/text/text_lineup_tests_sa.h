/******************************************************************************
* djinterp [test]                                      text_lineup_tests_sa.h
*
*   Unit tests for the text_lineup module (text lineup and formatting
* utilities). Tests cover the lightweight d_text_buffer used for output
* accumulation, the d_string_array token container, and the core
* d_text_lineup formatting engine with its full set of behavior flags.
*
*
* path:      \test\text\text_lineup_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.01.01
******************************************************************************/

#ifndef DJINTERP_TEXT_LINEUP_TESTS_STANDALONE_
#define DJINTERP_TEXT_LINEUP_TESTS_STANDALONE_ 1

#include "../../../inc/djinterp/c/test/test_standalone.h"
#include "../../../inc/djinterp/c/text/text_lineup.h"


/******************************************************************************
 * TEST CONFIGURATION
 *****************************************************************************/

// D_TEST_LINEUP_BUF_CAPACITY
//   constant: default initial capacity for test text buffers.
#define D_TEST_LINEUP_BUF_CAPACITY      256

// D_TEST_LINEUP_SMALL_CAPACITY
//   constant: small capacity for growth and overflow tests.
#define D_TEST_LINEUP_SMALL_CAPACITY    8

// D_TEST_LINEUP_LINE_WIDTH
//   constant: default line width for lineup formatting tests.
#define D_TEST_LINEUP_LINE_WIDTH        60

// D_TEST_LINEUP_NARROW_WIDTH
//   constant: narrow line width to force multi-line wrapping.
#define D_TEST_LINEUP_NARROW_WIDTH      20

// D_TEST_LINEUP_TOKEN_SHORT
//   constant: short token string used in array and lineup tests.
#define D_TEST_LINEUP_TOKEN_SHORT       "abc"

// D_TEST_LINEUP_TOKEN_LONG
//   constant: longer token string for max-length calculations.
#define D_TEST_LINEUP_TOKEN_LONG        "abcdefgh"

// D_TEST_LINEUP_SEPARATOR
//   constant: separator string used in lineup tests.
#define D_TEST_LINEUP_SEPARATOR         ", "

// D_TEST_LINEUP_PREFIX
//   constant: prefix string for first-line tests.
#define D_TEST_LINEUP_PREFIX            "{ "

// D_TEST_LINEUP_SUFFIX
//   constant: suffix string for last-line tests.
#define D_TEST_LINEUP_SUFFIX            " }"

// D_TEST_LINEUP_LINE_PREFIX
//   constant: per-line prefix used in multi-line tests.
#define D_TEST_LINEUP_LINE_PREFIX       "  "

// D_TEST_LINEUP_LINE_SUFFIX
//   constant: per-line suffix (e.g., trailing backslash).
#define D_TEST_LINEUP_LINE_SUFFIX       " \\"

// D_TEST_LINEUP_STRNLEN_MAX
//   constant: upper bound passed to d_strnlen in test assertions.
#define D_TEST_LINEUP_STRNLEN_MAX       4096


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

bool                   d_tests_sa_text_lineup_setup(void);
bool                   d_tests_sa_text_lineup_teardown(void);
struct d_text_buffer*  d_tests_sa_text_lineup_make_buffer(size_t _capacity);
struct d_string_array* d_tests_sa_text_lineup_make_token_array(size_t _count,
                                                               ...);
struct d_text_template d_tests_sa_text_lineup_make_template(
                           struct d_string_array* _tokens,
                           const char*            _separator,
                           bool                   _sep_before,
                           bool                   _sep_after,
                           const char*            _line_prefix,
                           const char*            _line_suffix,
                           size_t                 _min_token_width,
                           uint32_t               _flags);


/******************************************************************************
 * SECTION I: d_text_buffer OPERATIONS
 *****************************************************************************/

// d_text_buffer_new tests
struct d_test_object* d_tests_sa_text_lineup_text_buffer_new(void);

// d_text_buffer_ensure_capacity tests
struct d_test_object* d_tests_sa_text_lineup_text_buffer_ensure_capacity(void);

// d_text_buffer_append tests
struct d_test_object* d_tests_sa_text_lineup_text_buffer_append(void);

// d_text_buffer_append_s tests
struct d_test_object* d_tests_sa_text_lineup_text_buffer_append_s(void);

// d_text_buffer_prepend tests
struct d_test_object* d_tests_sa_text_lineup_text_buffer_prepend(void);

// d_text_buffer_write_over tests
struct d_test_object* d_tests_sa_text_lineup_text_buffer_write_over(void);

// section I aggregator
struct d_test_object* d_tests_sa_text_lineup_text_buffer_all(void);


/******************************************************************************
 * SECTION II: d_string_array OPERATIONS
 *****************************************************************************/

// d_string_array_new tests
struct d_test_object* d_tests_sa_text_lineup_string_array_new(void);

// d_string_array_reserve tests
struct d_test_object* d_tests_sa_text_lineup_string_array_reserve(void);

// d_string_array_append tests
struct d_test_object* d_tests_sa_text_lineup_string_array_append(void);

// d_string_array_generate_series tests
struct d_test_object* d_tests_sa_text_lineup_string_array_generate_series(void);

// d_string_array_generate_series_capacity tests
struct d_test_object*
    d_tests_sa_text_lineup_string_array_generate_series_capacity(void);

// section II aggregator
struct d_test_object* d_tests_sa_text_lineup_string_array_all(void);


/******************************************************************************
 * SECTION III: d_text_lineup
 *****************************************************************************/

// null / invalid input rejection
struct d_test_object* d_tests_sa_text_lineup_null_inputs(void);

// zero line_width (no-wrap) output
struct d_test_object* d_tests_sa_text_lineup_no_wrap(void);

// line-wrapping over multiple rows
struct d_test_object* d_tests_sa_text_lineup_wrapping(void);

// first-line prefix / last-line suffix
struct d_test_object* d_tests_sa_text_lineup_prefix_suffix(void);

// per-line prefix / per-line suffix
struct d_test_object* d_tests_sa_text_lineup_line_prefix_suffix(void);

// separator_before / separator_after
struct d_test_object* d_tests_sa_text_lineup_separators(void);

// token alignment flags (left, center, right/default)
struct d_test_object* d_tests_sa_text_lineup_token_alignment(void);

// behavior flags (right-align line suffix, suffix after suffix, etc.)
struct d_test_object* d_tests_sa_text_lineup_flags(void);

// min_token_width enforcement
struct d_test_object* d_tests_sa_text_lineup_min_token_width(void);

// indent_prefix / outdent_suffix parameters
struct d_test_object* d_tests_sa_text_lineup_indent_outdent(void);

// section III aggregator
struct d_test_object* d_tests_sa_text_lineup_lineup_all(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// master runner for all text_lineup module tests
struct d_test_object* d_tests_sa_text_lineup_run_all(void);


#endif  // DJINTERP_TEXT_LINEUP_TESTS_STANDALONE_
