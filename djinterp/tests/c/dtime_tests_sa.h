/******************************************************************************
* djinterp [test]                                              dtime_tests_sa.h
*
*   Unit tests for the dtime module (cross-platform time utilities).
*   Tests cover thread-safe time conversion, high-resolution time, sleep
* functions, timezone utilities, string parsing/formatting, time arithmetic,
* monotonic time utilities, and timespec normalization.
*
*
* path:      \inc\c\test\dtime_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/

#ifndef DJINTERP_DTIME_TESTS_SA_
#define DJINTERP_DTIME_TESTS_SA_ 1

#include "..\..\inc\c\test\test_standalone.h"
#include "..\..\inc\c\dtime.h"


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
struct d_test_object* d_tests_dtime_localtime(void);
struct d_test_object* d_tests_dtime_gmtime(void);
struct d_test_object* d_tests_dtime_ctime(void);
struct d_test_object* d_tests_dtime_asctime(void);
struct d_test_object* d_tests_dtime_thread_safe_conversion_all(void);


/******************************************************************************
 * IV. HIGH-RESOLUTION TIME TESTS
 *****************************************************************************/

// IV. high-resolution time tests
struct d_test_object* d_tests_dtime_clock_gettime(void);
struct d_test_object* d_tests_dtime_clock_getres(void);
struct d_test_object* d_tests_dtime_timespec_get(void);
struct d_test_object* d_tests_dtime_high_resolution_time_all(void);


/******************************************************************************
 * V. SLEEP FUNCTIONS TESTS
 *****************************************************************************/

// V. sleep functions tests
struct d_test_object* d_tests_dtime_nanosleep(void);
struct d_test_object* d_tests_dtime_usleep(void);
struct d_test_object* d_tests_dtime_sleep_ms(void);
struct d_test_object* d_tests_dtime_sleep_functions_all(void);


/******************************************************************************
 * VI. TIMEZONE UTILITIES TESTS
 *****************************************************************************/

// VI. timezone utilities tests
struct d_test_object* d_tests_dtime_timegm(void);
struct d_test_object* d_tests_dtime_tzset(void);
struct d_test_object* d_tests_dtime_timezone_utilities_all(void);


/******************************************************************************
 * VII. STRING PARSING AND FORMATTING TESTS
 *****************************************************************************/

// VII. string parsing and formatting tests
struct d_test_object* d_tests_dtime_strptime(void);
struct d_test_object* d_tests_dtime_strftime_s(void);
struct d_test_object* d_tests_dtime_string_parsing_formatting_all(void);


/******************************************************************************
 * VIII. TIME ARITHMETIC TESTS
 *****************************************************************************/

// VIII. time arithmetic tests
struct d_test_object* d_tests_dtime_timespec_add(void);
struct d_test_object* d_tests_dtime_timespec_sub(void);
struct d_test_object* d_tests_dtime_timespec_cmp(void);
struct d_test_object* d_tests_dtime_timespec_to_ms(void);
struct d_test_object* d_tests_dtime_timespec_to_us(void);
struct d_test_object* d_tests_dtime_timespec_to_ns(void);
struct d_test_object* d_tests_dtime_ms_to_timespec(void);
struct d_test_object* d_tests_dtime_us_to_timespec(void);
struct d_test_object* d_tests_dtime_ns_to_timespec(void);
struct d_test_object* d_tests_dtime_time_arithmetic_all(void);


/******************************************************************************
 * IX. MONOTONIC TIME UTILITIES TESTS
 *****************************************************************************/

// IX. monotonic time utilities tests
struct d_test_object* d_tests_dtime_monotonic_time_ms(void);
struct d_test_object* d_tests_dtime_monotonic_time_us(void);
struct d_test_object* d_tests_dtime_monotonic_time_ns(void);
struct d_test_object* d_tests_dtime_monotonic_time_all(void);


/******************************************************************************
 * X. TIMESPEC NORMALIZATION TESTS
 *****************************************************************************/

// X. timespec normalization tests
struct d_test_object* d_tests_dtime_timespec_normalize(void);
struct d_test_object* d_tests_dtime_timespec_is_valid(void);
struct d_test_object* d_tests_dtime_timespec_normalization_all(void);


/******************************************************************************
 * XI. NULL PARAMETER TESTS
 *****************************************************************************/

// XI. null parameter tests
struct d_test_object* d_tests_dtime_null_params_all(void);


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

// d_tests_dtime_run_all
//   function: runs all dtime tests and returns the root test object tree.
// returns pointer to root test object containing all test results, or NULL
// on failure.
struct d_test_object* d_tests_dtime_run_all(void);


#endif  // DJINTERP_DTIME_TESTS_SA_





