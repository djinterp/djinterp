/*******************************************************************************
* djinterp [test]                                                  dtime_tests.h
*
* dtime.h's suite in DTest's C face (c/modules/test/test_suite.h).
*   The first block holds the register's decisions on dtime (#14, #15, #25 to
* #29 of docs/djinterp_decisions.md, Part B), each with a regression test.
* The standalone suite (dtime_tests_sa*.c) moves in as further blocks.
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
* path:      /tests/djinterp/c/dtime_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_DTIME_TESTS_H
#define DJINTERP_TESTS_C_DTIME_TESTS_H 1

// djinterp
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <string.h>  // memset, strcmp, strlen, strstr
    // djinterp
    #include "../../../inc/djinterp/c/dtime.h"  // d_timegm, d_timespec_*
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    The register's decisions
//------------------------------------------------------------------------------
bool d_tests_dtime_timespec_to_ns(void);
bool d_tests_dtime_nanosleep(void);
bool d_tests_dtime_timegm_normalizes(void);
bool d_tests_dtime_clocks(void);
bool d_tests_dtime_strftime_s(void);
bool d_tests_dtime_timespec_cmp_null(void);
bool d_tests_dtime_timespec_type(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_dtime_register_rows
//   constant: the register block's rows.
static const struct dj_test_case_spec d_tests_dtime_register_rows[] =
{
    { "d_tests_dtime_timespec_to_ns",
      "#14: whole and negative values convert within the precondition",
      d_tests_dtime_timespec_to_ns },
    { "d_tests_dtime_nanosleep",
      "#15: a short sleep succeeds; a NULL or out-of-range request is EINVAL",
      d_tests_dtime_nanosleep },
    { "d_tests_dtime_timegm_normalizes",
      "#25: timegm, or its fallback, normalizes the broken-down time",
      d_tests_dtime_timegm_normalizes },
    { "d_tests_dtime_clocks",
      "#26: the monotonic clock never goes back; CPU time advances",
      d_tests_dtime_clocks },
    { "d_tests_dtime_strftime_s",
      "#27: strftime_s formats, and empties its buffer on a bad argument",
      d_tests_dtime_strftime_s },
    { "d_tests_dtime_timespec_cmp_null",
      "#28: NULL orders first on both sides",
      d_tests_dtime_timespec_cmp_null },
    { "d_tests_dtime_timespec_type",
      "#29: struct timespec is the platform's, with no shim of its own",
      d_tests_dtime_timespec_type }
};

// d_tests_dtime_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_dtime_blocks[] =
{
    { "register",
      "the register's decisions on dtime, each with its regression test",
      d_tests_dtime_register_rows,
      sizeof(d_tests_dtime_register_rows) /
          sizeof(d_tests_dtime_register_rows[0]) }
};

// d_tests_dtime_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_dtime_module =
{
    "dtime",
    "dtime.h: portable clocks, sleeping, conversion and formatting",
    d_tests_dtime_blocks,
    sizeof(d_tests_dtime_blocks) / sizeof(d_tests_dtime_blocks[0])
};

// d_tests_dtime_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_dtime_spec(void)
{
    return &d_tests_dtime_module;
}

// 2.2    The sections' entry points
//------------------------------------------------------------------------------
// The tests built call by call (section 3), which the runner calls after
// adding the spec.
bool d_tests_dtime_run_all(struct dj_test_suite* _suite);


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

// D_TEST_DTIME_SLEEP_TOLERANCE_MS
//   constant: tolerance in milliseconds for sleep timing tests.
// Sleep functions are inherently imprecise; this allows for OS scheduling.
#define D_TEST_DTIME_SLEEP_TOLERANCE_MS     50

// D_TEST_DTIME_SLEEP_TEST_MS
//   constant: duration in milliseconds for sleep tests.
#define D_TEST_DTIME_SLEEP_TEST_MS          100

// D_TEST_DTIME_CTIME_BUF_SIZE
//   constant: buffer size for ctime/asctime output (minimum 26 bytes required).
#define D_TEST_DTIME_CTIME_BUF_SIZE         32

// D_TEST_DTIME_STRFTIME_BUF_SIZE
//   constant: buffer size for strftime output.
#define D_TEST_DTIME_STRFTIME_BUF_SIZE      128

// D_TEST_DTIME_KNOWN_TIMESTAMP
//   constant: known Unix timestamp for validation (2000-01-01 00:00:00 UTC).
#define D_TEST_DTIME_KNOWN_TIMESTAMP        946684800

// D_TEST_DTIME_EPOCH_YEAR
//   constant: Unix epoch year for validation.
#define D_TEST_DTIME_EPOCH_YEAR             1970

// D_TEST_DTIME_Y2K_YEAR
//   constant: Y2K year for timestamp validation.
#define D_TEST_DTIME_Y2K_YEAR               2000

// D_INTERNAL_TEST_PATH_BUF_SIZE
//   constant: buffer size for test path construction.
#define D_INTERNAL_TEST_PATH_BUF_SIZE       512


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

// d_tests_dtime_setup
//   function: initializes test environment for dtime tests.
// returns true on success, false on failure.
bool d_tests_dtime_setup(void);

// d_tests_dtime_teardown
//   function: cleans up test environment after dtime tests.
// returns true on success, false on failure.
bool d_tests_dtime_teardown(void);


/******************************************************************************
 * III. THREAD-SAFE TIME CONVERSION TESTS
 *****************************************************************************/

// III. thread-safe time conversion tests
bool d_tests_dtime_localtime(struct dj_test_suite* _suite);
bool d_tests_dtime_gmtime(struct dj_test_suite* _suite);
bool d_tests_dtime_ctime(struct dj_test_suite* _suite);
bool d_tests_dtime_asctime(struct dj_test_suite* _suite);
bool d_tests_dtime_thread_safe_conversion_all(struct dj_test_suite* _suite);


/******************************************************************************
 * IV. HIGH-RESOLUTION TIME TESTS
 *****************************************************************************/

// IV. high-resolution time tests
bool d_tests_dtime_clock_gettime(struct dj_test_suite* _suite);
bool d_tests_dtime_clock_getres(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_get(struct dj_test_suite* _suite);
bool d_tests_dtime_high_resolution_time_all(struct dj_test_suite* _suite);


/******************************************************************************
 * V. SLEEP FUNCTIONS TESTS
 *****************************************************************************/

// V. sleep functions tests
bool d_tests_dtime_nanosleep_cases(struct dj_test_suite* _suite);
bool d_tests_dtime_usleep(struct dj_test_suite* _suite);
bool d_tests_dtime_sleep_ms(struct dj_test_suite* _suite);
bool d_tests_dtime_sleep_functions_all(struct dj_test_suite* _suite);


/******************************************************************************
 * VI. TIMEZONE UTILITIES TESTS
 *****************************************************************************/

// VI. timezone utilities tests
bool d_tests_dtime_timegm(struct dj_test_suite* _suite);
bool d_tests_dtime_tzset(struct dj_test_suite* _suite);
bool d_tests_dtime_timezone_utilities_all(struct dj_test_suite* _suite);


/******************************************************************************
 * VII. STRING PARSING AND FORMATTING TESTS
 *****************************************************************************/

// VII. string parsing and formatting tests
bool d_tests_dtime_strptime(struct dj_test_suite* _suite);
bool d_tests_dtime_strftime_s_cases(struct dj_test_suite* _suite);
bool d_tests_dtime_string_parsing_formatting_all(struct dj_test_suite* _suite);


/******************************************************************************
 * VIII. TIME ARITHMETIC TESTS
 *****************************************************************************/

// VIII. time arithmetic tests
bool d_tests_dtime_timespec_add(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_sub(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_cmp(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_to_ms(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_to_us(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_to_ns_cases(struct dj_test_suite* _suite);
bool d_tests_dtime_ms_to_timespec(struct dj_test_suite* _suite);
bool d_tests_dtime_us_to_timespec(struct dj_test_suite* _suite);
bool d_tests_dtime_ns_to_timespec(struct dj_test_suite* _suite);
bool d_tests_dtime_time_arithmetic_all(struct dj_test_suite* _suite);


/******************************************************************************
 * IX. MONOTONIC TIME UTILITIES TESTS
 *****************************************************************************/

// IX. monotonic time utilities tests
bool d_tests_dtime_monotonic_time_ms(struct dj_test_suite* _suite);
bool d_tests_dtime_monotonic_time_us(struct dj_test_suite* _suite);
bool d_tests_dtime_monotonic_time_ns(struct dj_test_suite* _suite);
bool d_tests_dtime_monotonic_time_all(struct dj_test_suite* _suite);


/******************************************************************************
 * X. TIMESPEC NORMALIZATION TESTS
 *****************************************************************************/

// X. timespec normalization tests
bool d_tests_dtime_timespec_normalize(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_is_valid(struct dj_test_suite* _suite);
bool d_tests_dtime_timespec_normalization_all(struct dj_test_suite* _suite);


/******************************************************************************
 * XI. NULL PARAMETER TESTS
 *****************************************************************************/

// XI. null parameter tests
bool d_tests_dtime_null_params_all(struct dj_test_suite* _suite);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// d_tests_dtime_run_all
//   function: runs all dtime tests and returns the root test object tree.
// returns pointer to root test object containing all test results, or NULL
// on failure.
bool d_tests_dtime_run_all(struct dj_test_suite* _suite);

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_DTIME_TESTS_H
