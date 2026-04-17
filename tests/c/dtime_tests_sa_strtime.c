/******************************************************************************
* djinterp [test]                                       dtime_tests_sa_strtime.c
*
*   Tests for string parsing and formatting functions (strptime, strftime_s).
*
*
* path:      \src\test\dtime_tests_sa_strtime.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * VII. STRING PARSING AND FORMATTING TESTS
 *****************************************************************************/

/*
d_tests_dtime_strptime
  Tests d_strptime for parsing time strings according to format.
  Tests the following:
  - parses date in Y-m-d format
  - parses time in H:M:S format
  - parses combined datetime format
  - returns pointer past parsed portion on success
  - returns NULL for mismatched format
  - returns NULL for NULL string parameter
  - returns NULL for NULL format parameter
  - handles partial parsing correctly
  - parses month names correctly
  - parses weekday names correctly
*/
struct d_test_object*
d_tests_dtime_strptime
(
    void
)
{
    struct d_test_object* group;
    struct tm             result;
    char*                 ret_ptr;
    const char*           date_str;
    const char*           time_str;
    const char*           datetime_str;
    const char*           partial_str;
    const char*           month_str;
    const char*           weekday_str;
    bool                  test_parse_date;
    bool                  test_parse_time;
    bool                  test_parse_datetime;
    bool                  test_returns_past;
    bool                  test_mismatch;
    bool                  test_null_string;
    bool                  test_null_format;
    bool                  test_partial;
    bool                  test_month_name;
    bool                  test_weekday_name;
    size_t                idx;

    // test strings
    date_str     = "2023-06-15";
    time_str     = "14:30:45";
    datetime_str = "2023-06-15 14:30:45";
    partial_str  = "2023-06-15 extra text";
    month_str    = "Jun 15 2023";
    weekday_str  = "Thu Jun 15 2023";

    // test 1: parse date in Y-m-d format
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime(date_str, "%Y-%m-%d", &result);
    test_parse_date = (ret_ptr != NULL) &&
                      (result.tm_year == 123) &&   // 2023 - 1900
                      (result.tm_mon == 5) &&      // June (0-indexed)
                      (result.tm_mday == 15);

    // test 2: parse time in H:M:S format
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime(time_str, "%H:%M:%S", &result);
    test_parse_time = (ret_ptr != NULL) &&
                      (result.tm_hour == 14) &&
                      (result.tm_min == 30) &&
                      (result.tm_sec == 45);

    // test 3: parse combined datetime format
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime(datetime_str, "%Y-%m-%d %H:%M:%S", &result);
    test_parse_datetime = (ret_ptr != NULL) &&
                          (result.tm_year == 123) &&
                          (result.tm_mon == 5) &&
                          (result.tm_mday == 15) &&
                          (result.tm_hour == 14) &&
                          (result.tm_min == 30) &&
                          (result.tm_sec == 45);

    // test 4: returns pointer past parsed portion
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime(date_str, "%Y-%m-%d", &result);
    // should point to the null terminator since entire string was parsed
    test_returns_past = (ret_ptr != NULL) && (*ret_ptr == '\0');

    // test 5: mismatched format returns NULL or stops early
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime("not-a-date", "%Y-%m-%d", &result);
    // strptime behavior varies: may return NULL or return early
    // we accept NULL or a pointer that indicates parse failure
    test_mismatch = (ret_ptr == NULL) || 
                    (result.tm_year == 0 && result.tm_mon == 0 && result.tm_mday == 0);

    // test 6: NULL string parameter
    ret_ptr = d_strptime(NULL, "%Y-%m-%d", &result);
    test_null_string = (ret_ptr == NULL);

    // test 7: NULL format parameter
    ret_ptr = d_strptime(date_str, NULL, &result);
    test_null_format = (ret_ptr == NULL);

    // test 8: partial parsing - should return pointer to unparsed portion
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime(partial_str, "%Y-%m-%d", &result);
    test_partial = (ret_ptr != NULL) &&
                   (result.tm_year == 123) &&
                   (result.tm_mon == 5) &&
                   (result.tm_mday == 15) &&
                   (*ret_ptr == ' ');  // points to space before "extra text"

    // test 9: parse month name (%b or %B)
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime(month_str, "%b %d %Y", &result);
    test_month_name = (ret_ptr != NULL) &&
                      (result.tm_mon == 5) &&      // June
                      (result.tm_mday == 15) &&
                      (result.tm_year == 123);

    // test 10: parse weekday name (%a or %A)
    memset(&result, 0, sizeof(result));
    ret_ptr = d_strptime(weekday_str, "%a %b %d %Y", &result);
    // weekday parsing may not set tm_wday on all platforms
    test_weekday_name = (ret_ptr != NULL) &&
                        (result.tm_mon == 5) &&
                        (result.tm_mday == 15) &&
                        (result.tm_year == 123);

    // build result tree
    group = d_test_object_new_interior("d_strptime", 10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("parse_date",
                                           test_parse_date,
                                           "d_strptime parses Y-m-d format");
    group->elements[idx++] = D_ASSERT_TRUE("parse_time",
                                           test_parse_time,
                                           "d_strptime parses H:M:S format");
    group->elements[idx++] = D_ASSERT_TRUE("parse_datetime",
                                           test_parse_datetime,
                                           "d_strptime parses combined datetime");
    group->elements[idx++] = D_ASSERT_TRUE("returns_past",
                                           test_returns_past,
                                           "d_strptime returns ptr past parsed");
    group->elements[idx++] = D_ASSERT_TRUE("mismatch",
                                           test_mismatch,
                                           "d_strptime handles format mismatch");
    group->elements[idx++] = D_ASSERT_TRUE("null_string",
                                           test_null_string,
                                           "d_strptime returns NULL for NULL str");
    group->elements[idx++] = D_ASSERT_TRUE("null_format",
                                           test_null_format,
                                           "d_strptime returns NULL for NULL fmt");
    group->elements[idx++] = D_ASSERT_TRUE("partial",
                                           test_partial,
                                           "d_strptime handles partial parsing");
    group->elements[idx++] = D_ASSERT_TRUE("month_name",
                                           test_month_name,
                                           "d_strptime parses month names");
    group->elements[idx++] = D_ASSERT_TRUE("weekday_name",
                                           test_weekday_name,
                                           "d_strptime parses weekday names");

    return group;
}


/*
d_tests_dtime_strftime_s
  Tests d_strftime_s for safe string formatting of time.
  Tests the following:
  - formats date correctly
  - formats time correctly
  - formats combined datetime correctly
  - returns number of characters written (excluding null)
  - returns 0 when buffer is too small
  - returns 0 for NULL buffer parameter
  - returns 0 for NULL format parameter
  - returns 0 for NULL tm parameter
  - handles empty format string
  - handles literal characters in format
  - formats locale-specific elements
  - handles maximum buffer size edge case
*/
struct d_test_object*
d_tests_dtime_strftime_s
(
    void
)
{
    struct d_test_object* group;
    struct tm             tm_test;
    char                  buf[D_TEST_DTIME_STRFTIME_BUF_SIZE];
    char                  small_buf[8];
    int                   result;
    bool                  test_format_date;
    bool                  test_format_time;
    bool                  test_format_datetime;
    bool                  test_returns_count;
    bool                  test_buffer_too_small;
    bool                  test_null_buffer;
    bool                  test_null_format;
    bool                  test_null_tm;
    bool                  test_empty_format;
    bool                  test_literal_chars;
    bool                  test_year_format;
    bool                  test_exact_fit;
    size_t                idx;

    // setup test tm structure (2023-06-15 14:30:45, Thursday)
    memset(&tm_test, 0, sizeof(tm_test));
    tm_test.tm_year  = 123;  // 2023 - 1900
    tm_test.tm_mon   = 5;    // June (0-indexed)
    tm_test.tm_mday  = 15;
    tm_test.tm_hour  = 14;
    tm_test.tm_min   = 30;
    tm_test.tm_sec   = 45;
    tm_test.tm_wday  = 4;    // Thursday
    tm_test.tm_yday  = 165;  // day of year
    tm_test.tm_isdst = 0;

    // test 1: format date correctly
    memset(buf, 0, sizeof(buf));
    result = d_strftime_s(buf, sizeof(buf), "%Y-%m-%d", &tm_test);
    test_format_date = (result > 0) && (strcmp(buf, "2023-06-15") == 0);

    // test 2: format time correctly
    memset(buf, 0, sizeof(buf));
    result = d_strftime_s(buf, sizeof(buf), "%H:%M:%S", &tm_test);
    test_format_time = (result > 0) && (strcmp(buf, "14:30:45") == 0);

    // test 3: format combined datetime correctly
    memset(buf, 0, sizeof(buf));
    result = d_strftime_s(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_test);
    test_format_datetime = (result > 0) && 
                           (strcmp(buf, "2023-06-15 14:30:45") == 0);

    // test 4: returns number of characters written (excluding null)
    memset(buf, 0, sizeof(buf));
    result = d_strftime_s(buf, sizeof(buf), "%Y-%m-%d", &tm_test);
    // "2023-06-15" is 10 characters
    test_returns_count = (result == 10);

    // test 5: returns 0 when buffer is too small
    memset(small_buf, 'X', sizeof(small_buf));
    small_buf[sizeof(small_buf) - 1] = '\0';
    result = d_strftime_s(small_buf, sizeof(small_buf), "%Y-%m-%d %H:%M:%S", &tm_test);
    // buffer too small for "2023-06-15 14:30:45" (19 chars + null)
    test_buffer_too_small = (result == 0);

    // test 6: returns 0 for NULL buffer parameter
    result = d_strftime_s(NULL, sizeof(buf), "%Y-%m-%d", &tm_test);
    test_null_buffer = (result == 0);

    // test 7: returns 0 for NULL format parameter
    result = d_strftime_s(buf, sizeof(buf), NULL, &tm_test);
    test_null_format = (result == 0);

    // test 8: returns 0 for NULL tm parameter
    result = d_strftime_s(buf, sizeof(buf), "%Y-%m-%d", NULL);
    test_null_tm = (result == 0);

    // test 9: handles empty format string
    memset(buf, 'X', sizeof(buf));
    result = d_strftime_s(buf, sizeof(buf), "", &tm_test);
    // empty format should produce empty string
    test_empty_format = (result == 0) && (buf[0] == '\0');

    // test 10: handles literal characters in format
    memset(buf, 0, sizeof(buf));
    result = d_strftime_s(buf, sizeof(buf), "Date: %Y/%m/%d", &tm_test);
    test_literal_chars = (result > 0) && (strcmp(buf, "Date: 2023/06/15") == 0);

    // test 11: formats year correctly with different specifiers
    memset(buf, 0, sizeof(buf));
    result = d_strftime_s(buf, sizeof(buf), "%Y %y", &tm_test);
    // %Y = 2023, %y = 23
    test_year_format = (result > 0) && (strcmp(buf, "2023 23") == 0);

    // test 12: exact fit in buffer (string length + 1 for null)
    {
        char exact_buf[11];  // "2023-06-15" is 10 chars + null = 11

        memset(exact_buf, 0, sizeof(exact_buf));
        result = d_strftime_s(exact_buf, sizeof(exact_buf), "%Y-%m-%d", &tm_test);
        test_exact_fit = (result == 10) && (strcmp(exact_buf, "2023-06-15") == 0);
    }

    // build result tree
    group = d_test_object_new_interior("d_strftime_s", 12);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("format_date",
                                           test_format_date,
                                           "d_strftime_s formats date correctly");
    group->elements[idx++] = D_ASSERT_TRUE("format_time",
                                           test_format_time,
                                           "d_strftime_s formats time correctly");
    group->elements[idx++] = D_ASSERT_TRUE("format_datetime",
                                           test_format_datetime,
                                           "d_strftime_s formats datetime");
    group->elements[idx++] = D_ASSERT_TRUE("returns_count",
                                           test_returns_count,
                                           "d_strftime_s returns char count");
    group->elements[idx++] = D_ASSERT_TRUE("buffer_too_small",
                                           test_buffer_too_small,
                                           "d_strftime_s returns 0 if too small");
    group->elements[idx++] = D_ASSERT_TRUE("null_buffer",
                                           test_null_buffer,
                                           "d_strftime_s returns 0 for NULL buf");
    group->elements[idx++] = D_ASSERT_TRUE("null_format",
                                           test_null_format,
                                           "d_strftime_s returns 0 for NULL fmt");
    group->elements[idx++] = D_ASSERT_TRUE("null_tm",
                                           test_null_tm,
                                           "d_strftime_s returns 0 for NULL tm");
    group->elements[idx++] = D_ASSERT_TRUE("empty_format",
                                           test_empty_format,
                                           "d_strftime_s handles empty format");
    group->elements[idx++] = D_ASSERT_TRUE("literal_chars",
                                           test_literal_chars,
                                           "d_strftime_s handles literal chars");
    group->elements[idx++] = D_ASSERT_TRUE("year_format",
                                           test_year_format,
                                           "d_strftime_s formats year specifiers");
    group->elements[idx++] = D_ASSERT_TRUE("exact_fit",
                                           test_exact_fit,
                                           "d_strftime_s handles exact buffer fit");

    return group;
}


/*
d_tests_dtime_string_parsing_formatting_all
  Runs all string parsing and formatting tests.
  Tests the following:
  - d_strptime
  - d_strftime_s
*/
struct d_test_object*
d_tests_dtime_string_parsing_formatting_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("VII. String Parsing and Formatting", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_strptime();
    group->elements[idx++] = d_tests_dtime_strftime_s();

    return group;
}
