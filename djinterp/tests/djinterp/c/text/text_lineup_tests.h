/*******************************************************************************
* djinterp [test]                                            text_lineup_tests.h
*
*   Unit tests for the text_lineup module (text lineup and formatting
* utilities). Tests cover the lightweight dj_text_buffer used for output
* accumulation, the dj_string_array token container, and the core
* d_text_lineup formatting engine with its full set of behavior flags.
* Converted from the standalone tier to DTest's C face on 2026.11.09; the
* entry point is d_tests_text_lineup_run_all.
*
*   BUILD PREREQUISITE. The module under test is not in the tree: there is no
* inc/djinterp/c/text/text_lineup.h and no src/djinterp/c/text/text_lineup.c,
* though an earlier tree had both (scripts/trees2.txt, under c/text/), and
* this header includes neither. Until they are restored, the suite does not
* compile -- dj_text_buffer is incomplete here and d_text_lineup is
* undeclared -- so it has no CMake leaf. Restored, it needs:
*     #include "../../../../inc/djinterp/c/text/text_lineup.h"
* below DTest's include, a leaf like text_template/'s with text_lineup.c and
* c/container/buffer/text_buffer.c among its sources, and <stdint.h> or
* re_std's dstdint.h for the uint32_t this header uses (lane D, 2026.11.09).
*
*
* path:      /tests/djinterp/c/text/text_lineup_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.01.01
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TEXT_LINEUP_TESTS_STANDALONE_
#define DJINTERP_TEXT_LINEUP_TESTS_STANDALONE_ 1

#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face

// djinterp


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
#define D_TEST_LINEUP_LINE_SUFFIX       " //"

// D_TEST_LINEUP_STRNLEN_MAX
//   constant: upper bound passed to d_strnlen in test assertions.
#define D_TEST_LINEUP_STRNLEN_MAX       4096


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

bool d_tests_text_lineup_setup(void);
bool d_tests_text_lineup_teardown(void);
struct dj_text_buffer* d_tests_text_lineup_make_buffer(
    size_t _capacity
);
struct dj_string_array* d_tests_text_lineup_make_token_array(
    size_t _count,
    ...
);
struct dj_text_template d_tests_text_lineup_make_template(
    struct dj_string_array* _tokens,
    const char*             _separator,
    bool                    _sep_before,
    bool                    _sep_after,
    const char*             _line_prefix,
    const char*             _line_suffix,
    size_t                  _min_token_width,
    uint32_t                _flags
);


/******************************************************************************
 * SECTION I: dj_text_buffer OPERATIONS
 *****************************************************************************/

// d_text_buffer_new tests
bool d_tests_text_lineup_text_buffer_new(struct dj_test_suite* _suite);

// d_text_buffer_ensure_capacity tests
bool d_tests_text_lineup_text_buffer_ensure_capacity(struct dj_test_suite* _suite);

// d_text_buffer_append tests
bool d_tests_text_lineup_text_buffer_append(struct dj_test_suite* _suite);

// d_text_buffer_append_s tests
bool d_tests_text_lineup_text_buffer_append_s(struct dj_test_suite* _suite);

// d_text_buffer_prepend tests
bool d_tests_text_lineup_text_buffer_prepend(struct dj_test_suite* _suite);

// d_text_buffer_write_over tests
bool d_tests_text_lineup_text_buffer_write_over(struct dj_test_suite* _suite);

// section I aggregator
bool d_tests_text_lineup_text_buffer_all(struct dj_test_suite* _suite);


/******************************************************************************
 * SECTION II: dj_string_array OPERATIONS
 *****************************************************************************/

// d_string_array_new tests
bool d_tests_text_lineup_string_array_new(struct dj_test_suite* _suite);

// d_string_array_reserve tests
bool d_tests_text_lineup_string_array_reserve(struct dj_test_suite* _suite);

// d_string_array_append tests
bool d_tests_text_lineup_string_array_append(struct dj_test_suite* _suite);

// d_string_array_generate_series tests
bool d_tests_text_lineup_string_array_generate_series(struct dj_test_suite* _suite);

// d_string_array_generate_series_capacity tests
bool d_tests_text_lineup_string_array_generate_series_capacity(struct dj_test_suite* _suite);

// section II aggregator
bool d_tests_text_lineup_string_array_all(struct dj_test_suite* _suite);


/******************************************************************************
 * SECTION III: d_text_lineup
 *****************************************************************************/

// null / invalid input rejection
bool d_tests_text_lineup_null_inputs(struct dj_test_suite* _suite);

// zero line_width (no-wrap) output
bool d_tests_text_lineup_no_wrap(struct dj_test_suite* _suite);

// line-wrapping over multiple rows
bool d_tests_text_lineup_wrapping(struct dj_test_suite* _suite);

// first-line prefix / last-line suffix
bool d_tests_text_lineup_prefix_suffix(struct dj_test_suite* _suite);

// per-line prefix / per-line suffix
bool d_tests_text_lineup_line_prefix_suffix(struct dj_test_suite* _suite);

// separator_before / separator_after
bool d_tests_text_lineup_separators(struct dj_test_suite* _suite);

// token alignment flags (left, center, right/default)
bool d_tests_text_lineup_token_alignment(struct dj_test_suite* _suite);

// behavior flags (right-align line suffix, suffix after suffix, etc.)
bool d_tests_text_lineup_flags(struct dj_test_suite* _suite);

// min_token_width enforcement
bool d_tests_text_lineup_min_token_width(struct dj_test_suite* _suite);

// indent_prefix / outdent_suffix parameters
bool d_tests_text_lineup_indent_outdent(struct dj_test_suite* _suite);

// section III aggregator
bool d_tests_text_lineup_lineup_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// master runner for all text_lineup module tests
bool d_tests_text_lineup_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_TEXT_LINEUP_TESTS_STANDALONE_
