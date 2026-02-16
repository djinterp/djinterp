/******************************************************************************
* djinterp [test]                                       dtime_tests_sa_convert.c
*
*   Tests for thread-safe time conversion functions (localtime, gmtime,
* ctime, asctime).
*
*
* path:      \src\test\dtime_tests_sa_convert.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * III. THREAD-SAFE TIME CONVERSION TESTS
 *****************************************************************************/

/*
d_tests_dtime_localtime
  Tests d_localtime for thread-safe local time conversion.
  Tests the following:
  - converts valid timestamp to local time
  - populates all struct tm fields correctly
  - returns result pointer on success
  - returns NULL for NULL timer parameter
  - returns NULL for NULL result parameter
  - handles epoch timestamp (0)
  - handles known timestamp (Y2K)
  - handles negative timestamp (pre-epoch, if supported)
*/
struct d_test_object*
d_tests_dtime_localtime
(
    void
)
{
    struct d_test_object* group;
    time_t                now;
    time_t                epoch;
    time_t                y2k;
    struct tm             result;
    struct tm*            ret_ptr;
    bool                  test_valid_conversion;
    bool                  test_fields_populated;
    bool                  test_returns_result;
    bool                  test_null_timer;
    bool                  test_null_result;
    bool                  test_epoch;
    bool                  test_y2k;
    bool                  test_y2k_year;
    size_t                idx;

    // setup known timestamps
    now   = time(NULL);
    epoch = 0;
    y2k   = D_TEST_DTIME_KNOWN_TIMESTAMP;

    // test 1: valid conversion
    memset(&result, 0, sizeof(result));
    ret_ptr = d_localtime(&now, &result);
    test_valid_conversion = (ret_ptr != NULL);

    // test 2: fields populated (check reasonable ranges)
    test_fields_populated = (result.tm_sec >= 0)   && (result.tm_sec <= 60)   &&
                            (result.tm_min >= 0)   && (result.tm_min <= 59)   &&
                            (result.tm_hour >= 0)  && (result.tm_hour <= 23)  &&
                            (result.tm_mday >= 1)  && (result.tm_mday <= 31)  &&
                            (result.tm_mon >= 0)   && (result.tm_mon <= 11)   &&
                            (result.tm_year >= 70) &&  // years since 1900
                            (result.tm_wday >= 0)  && (result.tm_wday <= 6)   &&
                            (result.tm_yday >= 0)  && (result.tm_yday <= 365);

    // test 3: returns the result pointer
    test_returns_result = (ret_ptr == &result);

    // test 4: NULL timer parameter
    ret_ptr = d_localtime(NULL, &result);
    test_null_timer = (ret_ptr == NULL);

    // test 5: NULL result parameter
    ret_ptr = d_localtime(&now, NULL);
    test_null_result = (ret_ptr == NULL);

    // test 6: epoch timestamp (1970-01-01 00:00:00 UTC, adjusted for local TZ)
    memset(&result, 0xFF, sizeof(result));
    ret_ptr = d_localtime(&epoch, &result);
    test_epoch = (ret_ptr != NULL) &&
                 (result.tm_year == 70 || result.tm_year == 69);  // may be 69 in
                                                                  // western TZ

    // test 7 & 8: Y2K timestamp (2000-01-01 00:00:00 UTC)
    memset(&result, 0xFF, sizeof(result));
    ret_ptr = d_localtime(&y2k, &result);
    test_y2k = (ret_ptr != NULL);
    test_y2k_year = (result.tm_year == 100 || result.tm_year == 99);  // 2000 or 
                                                                       // 1999
                                                                       // dep. TZ

    // build result tree
    group = d_test_object_new_interior("d_localtime", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_conversion",
                                           test_valid_conversion,
                                           "d_localtime converts valid timestamp");
    group->elements[idx++] = D_ASSERT_TRUE("fields_populated",
                                           test_fields_populated,
                                           "d_localtime populates all tm fields");
    group->elements[idx++] = D_ASSERT_TRUE("returns_result",
                                           test_returns_result,
                                           "d_localtime returns result pointer");
    group->elements[idx++] = D_ASSERT_TRUE("null_timer",
                                           test_null_timer,
                                           "d_localtime returns NULL for NULL timer");
    group->elements[idx++] = D_ASSERT_TRUE("null_result",
                                           test_null_result,
                                           "d_localtime returns NULL for NULL result");
    group->elements[idx++] = D_ASSERT_TRUE("epoch",
                                           test_epoch,
                                           "d_localtime handles epoch timestamp");
    group->elements[idx++] = D_ASSERT_TRUE("y2k",
                                           test_y2k,
                                           "d_localtime handles Y2K timestamp");
    group->elements[idx++] = D_ASSERT_TRUE("y2k_year",
                                           test_y2k_year,
                                           "d_localtime Y2K year is correct");

    return group;
}


/*
d_tests_dtime_gmtime
  Tests d_gmtime for thread-safe UTC time conversion.
  Tests the following:
  - converts valid timestamp to UTC
  - epoch timestamp produces 1970-01-01 00:00:00
  - Y2K timestamp produces 2000-01-01 00:00:00
  - returns result pointer on success
  - returns NULL for NULL timer parameter
  - returns NULL for NULL result parameter
  - all struct tm fields are in valid ranges
*/
struct d_test_object*
d_tests_dtime_gmtime
(
    void
)
{
    struct d_test_object* group;
    time_t                now;
    time_t                epoch;
    time_t                y2k;
    struct tm             result;
    struct tm*            ret_ptr;
    bool                  test_valid_conversion;
    bool                  test_epoch_date;
    bool                  test_epoch_time;
    bool                  test_y2k_date;
    bool                  test_y2k_time;
    bool                  test_returns_result;
    bool                  test_null_timer;
    bool                  test_null_result;
    size_t                idx;

    // setup known timestamps
    now   = time(NULL);
    epoch = 0;
    y2k   = D_TEST_DTIME_KNOWN_TIMESTAMP;

    // test 1: valid conversion
    memset(&result, 0, sizeof(result));
    ret_ptr = d_gmtime(&now, &result);
    test_valid_conversion = (ret_ptr != NULL);

    // test 2 & 3: epoch timestamp should produce exact 1970-01-01 00:00:00 UTC
    memset(&result, 0xFF, sizeof(result));
    ret_ptr = d_gmtime(&epoch, &result);
    test_epoch_date = (ret_ptr != NULL) &&
                      (result.tm_year == 70) &&
                      (result.tm_mon == 0) &&
                      (result.tm_mday == 1);
    test_epoch_time = (ret_ptr != NULL) &&
                      (result.tm_hour == 0) &&
                      (result.tm_min == 0) &&
                      (result.tm_sec == 0);

    // test 4 & 5: Y2K timestamp should produce exact 2000-01-01 00:00:00 UTC
    memset(&result, 0xFF, sizeof(result));
    ret_ptr = d_gmtime(&y2k, &result);
    test_y2k_date = (ret_ptr != NULL) &&
                    (result.tm_year == 100) &&
                    (result.tm_mon == 0) &&
                    (result.tm_mday == 1);
    test_y2k_time = (ret_ptr != NULL) &&
                    (result.tm_hour == 0) &&
                    (result.tm_min == 0) &&
                    (result.tm_sec == 0);

    // test 6: returns the result pointer
    memset(&result, 0, sizeof(result));
    ret_ptr = d_gmtime(&now, &result);
    test_returns_result = (ret_ptr == &result);

    // test 7: NULL timer parameter
    ret_ptr = d_gmtime(NULL, &result);
    test_null_timer = (ret_ptr == NULL);

    // test 8: NULL result parameter
    ret_ptr = d_gmtime(&now, NULL);
    test_null_result = (ret_ptr == NULL);

    // build result tree
    group = d_test_object_new_interior("d_gmtime", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_conversion",
                                           test_valid_conversion,
                                           "d_gmtime converts valid timestamp");
    group->elements[idx++] = D_ASSERT_TRUE("epoch_date",
                                           test_epoch_date,
                                           "d_gmtime epoch date is 1970-01-01");
    group->elements[idx++] = D_ASSERT_TRUE("epoch_time",
                                           test_epoch_time,
                                           "d_gmtime epoch time is 00:00:00");
    group->elements[idx++] = D_ASSERT_TRUE("y2k_date",
                                           test_y2k_date,
                                           "d_gmtime Y2K date is 2000-01-01");
    group->elements[idx++] = D_ASSERT_TRUE("y2k_time",
                                           test_y2k_time,
                                           "d_gmtime Y2K time is 00:00:00");
    group->elements[idx++] = D_ASSERT_TRUE("returns_result",
                                           test_returns_result,
                                           "d_gmtime returns result pointer");
    group->elements[idx++] = D_ASSERT_TRUE("null_timer",
                                           test_null_timer,
                                           "d_gmtime returns NULL for NULL timer");
    group->elements[idx++] = D_ASSERT_TRUE("null_result",
                                           test_null_result,
                                           "d_gmtime returns NULL for NULL result");

    return group;
}


/*
d_tests_dtime_ctime
  Tests d_ctime for thread-safe time-to-string conversion.
  Tests the following:
  - converts valid timestamp to string
  - string has expected format (26 chars including newline and null)
  - string contains year
  - returns buffer pointer on success
  - returns NULL for NULL timer parameter
  - returns NULL for NULL buffer parameter
  - handles epoch timestamp
*/
struct d_test_object*
d_tests_dtime_ctime
(
    void
)
{
    struct d_test_object* group;
    time_t                now;
    time_t                epoch;
    time_t                y2k;
    char                  buf[D_TEST_DTIME_CTIME_BUF_SIZE];
    char*                 ret_ptr;
    size_t                len;
    bool                  test_valid_conversion;
    bool                  test_string_length;
    bool                  test_contains_year;
    bool                  test_returns_buffer;
    bool                  test_null_timer;
    bool                  test_null_buffer;
    bool                  test_epoch;
    bool                  test_y2k_contains_2000;
    size_t                idx;

    // setup
    now   = time(NULL);
    epoch = 0;
    y2k   = D_TEST_DTIME_KNOWN_TIMESTAMP;

    // test 1: valid conversion
    memset(buf, 0, sizeof(buf));
    ret_ptr = d_ctime(&now, buf);
    test_valid_conversion = (ret_ptr != NULL) && (buf[0] != '\0');

    // test 2: string length (ctime produces "Day Mon DD HH:MM:SS YYYY\n\0")
    // standard format is 25 characters plus newline plus null = 26 total used
    len = strlen(buf);
    test_string_length = (len >= 24) && (len <= 26);

    // test 3: contains current year (check for 4-digit year pattern)
    // we look for "20" to handle years 2000-2099
    test_contains_year = (strstr(buf, "20") != NULL) || 
                         (strstr(buf, "19") != NULL);

    // test 4: returns the buffer pointer
    test_returns_buffer = (ret_ptr == buf);

    // test 5: NULL timer parameter
    ret_ptr = d_ctime(NULL, buf);
    test_null_timer = (ret_ptr == NULL);

    // test 6: NULL buffer parameter
    ret_ptr = d_ctime(&now, NULL);
    test_null_buffer = (ret_ptr == NULL);

    // test 7: epoch timestamp
    memset(buf, 0, sizeof(buf));
    ret_ptr = d_ctime(&epoch, buf);
    test_epoch = (ret_ptr != NULL) && (strlen(buf) >= 24);

    // test 8: Y2K timestamp
    // Note: d_ctime converts to LOCAL time, not UTC. The Y2K timestamp (946684800)
    // is 2000-01-01 00:00:00 UTC, but in timezones behind UTC (e.g., PST = UTC-8),
    // this will display as 1999-12-31 in local time. Therefore, we check for
    // either "2000" or "1999" to handle all timezones correctly.
    memset(buf, 0, sizeof(buf));
    ret_ptr = d_ctime(&y2k, buf);
    test_y2k_contains_2000 = (ret_ptr != NULL) && 
                             ((strstr(buf, "2000") != NULL) || 
                              (strstr(buf, "1999") != NULL));

    // build result tree
    group = d_test_object_new_interior("d_ctime", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_conversion",
                                           test_valid_conversion,
                                           "d_ctime converts timestamp to string");
    group->elements[idx++] = D_ASSERT_TRUE("string_length",
                                           test_string_length,
                                           "d_ctime string has expected length");
    group->elements[idx++] = D_ASSERT_TRUE("contains_year",
                                           test_contains_year,
                                           "d_ctime string contains year");
    group->elements[idx++] = D_ASSERT_TRUE("returns_buffer",
                                           test_returns_buffer,
                                           "d_ctime returns buffer pointer");
    group->elements[idx++] = D_ASSERT_TRUE("null_timer",
                                           test_null_timer,
                                           "d_ctime returns NULL for NULL timer");
    group->elements[idx++] = D_ASSERT_TRUE("null_buffer",
                                           test_null_buffer,
                                           "d_ctime returns NULL for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("epoch",
                                           test_epoch,
                                           "d_ctime handles epoch timestamp");
    group->elements[idx++] = D_ASSERT_TRUE("y2k_year",
                                           test_y2k_contains_2000,
                                           "d_ctime Y2K string has valid year");

    return group;
}


/*
d_tests_dtime_asctime
  Tests d_asctime for thread-safe tm-to-string conversion.
  Tests the following:
  - converts valid struct tm to string
  - string has expected format
  - returns buffer pointer on success
  - returns NULL for NULL tm parameter
  - returns NULL for NULL buffer parameter
  - handles edge case dates (first of month, last of year)
  - produces consistent output for same input
*/
struct d_test_object*
d_tests_dtime_asctime
(
    void
)
{
    struct d_test_object* group;
    struct tm             tm_now;
    struct tm             tm_y2k;
    time_t                now;
    time_t                y2k;
    char                  buf[D_TEST_DTIME_CTIME_BUF_SIZE];
    char                  buf2[D_TEST_DTIME_CTIME_BUF_SIZE];
    char*                 ret_ptr;
    size_t                len;
    bool                  test_valid_conversion;
    bool                  test_string_length;
    bool                  test_returns_buffer;
    bool                  test_null_tm;
    bool                  test_null_buffer;
    bool                  test_y2k_conversion;
    bool                  test_consistency;
    bool                  test_contains_weekday;
    size_t                idx;

    // setup
    now = time(NULL);
    y2k = D_TEST_DTIME_KNOWN_TIMESTAMP;

    // get struct tm for current time
    d_gmtime(&now, &tm_now);
    d_gmtime(&y2k, &tm_y2k);

    // test 1: valid conversion
    memset(buf, 0, sizeof(buf));
    ret_ptr = d_asctime(&tm_now, buf);
    test_valid_conversion = (ret_ptr != NULL) && (buf[0] != '\0');

    // test 2: string length (same format as ctime)
    len = strlen(buf);
    test_string_length = (len >= 24) && (len <= 26);

    // test 3: returns the buffer pointer
    test_returns_buffer = (ret_ptr == buf);

    // test 4: NULL tm parameter
    ret_ptr = d_asctime(NULL, buf);
    test_null_tm = (ret_ptr == NULL);

    // test 5: NULL buffer parameter
    ret_ptr = d_asctime(&tm_now, NULL);
    test_null_buffer = (ret_ptr == NULL);

    // test 6: Y2K conversion
    memset(buf, 0, sizeof(buf));
    ret_ptr = d_asctime(&tm_y2k, buf);
    test_y2k_conversion = (ret_ptr != NULL) && 
                          (strstr(buf, "2000") != NULL) &&
                          (strstr(buf, "Jan") != NULL);

    // test 7: consistency - same input produces same output
    memset(buf, 0, sizeof(buf));
    memset(buf2, 0, sizeof(buf2));
    d_asctime(&tm_y2k, buf);
    d_asctime(&tm_y2k, buf2);
    test_consistency = (strcmp(buf, buf2) == 0);

    // test 8: output contains weekday abbreviation (3 letters)
    memset(buf, 0, sizeof(buf));
    d_asctime(&tm_now, buf);
    // weekday abbreviations: Sun, Mon, Tue, Wed, Thu, Fri, Sat
    test_contains_weekday = (strstr(buf, "Sun") != NULL) ||
                            (strstr(buf, "Mon") != NULL) ||
                            (strstr(buf, "Tue") != NULL) ||
                            (strstr(buf, "Wed") != NULL) ||
                            (strstr(buf, "Thu") != NULL) ||
                            (strstr(buf, "Fri") != NULL) ||
                            (strstr(buf, "Sat") != NULL);

    // build result tree
    group = d_test_object_new_interior("d_asctime", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_conversion",
                                           test_valid_conversion,
                                           "d_asctime converts tm to string");
    group->elements[idx++] = D_ASSERT_TRUE("string_length",
                                           test_string_length,
                                           "d_asctime string has expected length");
    group->elements[idx++] = D_ASSERT_TRUE("returns_buffer",
                                           test_returns_buffer,
                                           "d_asctime returns buffer pointer");
    group->elements[idx++] = D_ASSERT_TRUE("null_tm",
                                           test_null_tm,
                                           "d_asctime returns NULL for NULL tm");
    group->elements[idx++] = D_ASSERT_TRUE("null_buffer",
                                           test_null_buffer,
                                           "d_asctime returns NULL for NULL buffer");
    group->elements[idx++] = D_ASSERT_TRUE("y2k_conversion",
                                           test_y2k_conversion,
                                           "d_asctime handles Y2K date correctly");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "d_asctime produces consistent output");
    group->elements[idx++] = D_ASSERT_TRUE("contains_weekday",
                                           test_contains_weekday,
                                           "d_asctime output contains weekday");

    return group;
}


/*
d_tests_dtime_thread_safe_conversion_all
  Runs all thread-safe time conversion tests.
  Tests the following:
  - d_localtime
  - d_gmtime
  - d_ctime
  - d_asctime
*/
struct d_test_object*
d_tests_dtime_thread_safe_conversion_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("III. Thread-Safe Time Conversion", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_localtime();
    group->elements[idx++] = d_tests_dtime_gmtime();
    group->elements[idx++] = d_tests_dtime_ctime();
    group->elements[idx++] = d_tests_dtime_asctime();

    return group;
}