/******************************************************************************
* djinterp [test]                                         dtime_tests_sa_null.c
*
*   Comprehensive NULL parameter tests for all dtime functions.
*
*
* path:      \src\test\dtime_tests_sa_null.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * XI. NULL PARAMETER TESTS
 *****************************************************************************/

/*
d_tests_dtime_null_params_all
  Comprehensive NULL parameter tests for all dtime functions.
  Tests the following:
  - d_localtime with NULL timer returns NULL
  - d_localtime with NULL result returns NULL
  - d_gmtime with NULL timer returns NULL
  - d_gmtime with NULL result returns NULL
  - d_ctime with NULL timer returns NULL
  - d_ctime with NULL buffer returns NULL
  - d_asctime with NULL tm returns NULL
  - d_asctime with NULL buffer returns NULL
  - d_clock_gettime with NULL timespec returns -1
  - d_clock_getres with NULL resolution returns -1
  - d_timespec_get with NULL timespec returns 0
  - d_nanosleep with NULL request returns -1
  - d_timegm with NULL tm returns -1
  - d_strptime with NULL string returns NULL
  - d_strptime with NULL format returns NULL
  - d_strftime_s with NULL buffer returns 0
  - d_strftime_s with NULL format returns 0
  - d_strftime_s with NULL tm returns 0
  - d_timespec_to_ms with NULL returns 0
  - d_timespec_to_us with NULL returns 0
  - d_timespec_to_ns with NULL returns 0
  - d_timespec_is_valid with NULL returns 0
*/
struct d_test_object*
d_tests_dtime_null_params_all
(
    void
)
{
    struct d_test_object* group;
    struct tm*            tm_result;
    char*                 str_result;
    time_t                timer;
    struct tm             tm_valid;
    struct timespec       ts_valid;
    char                  buf[D_TEST_DTIME_STRFTIME_BUF_SIZE];
    int                   int_result;
    int64_t               int64_result;
    bool                  test_localtime_null_timer;
    bool                  test_localtime_null_result;
    bool                  test_gmtime_null_timer;
    bool                  test_gmtime_null_result;
    bool                  test_ctime_null_timer;
    bool                  test_ctime_null_buffer;
    bool                  test_asctime_null_tm;
    bool                  test_asctime_null_buffer;
    bool                  test_clock_gettime_null_ts;
    bool                  test_clock_getres_null_res;
    bool                  test_timespec_get_null_ts;
    bool                  test_nanosleep_null_req;
    bool                  test_timegm_null_tm;
    bool                  test_strptime_null_str;
    bool                  test_strptime_null_fmt;
    bool                  test_strftime_null_buf;
    bool                  test_strftime_null_fmt;
    bool                  test_strftime_null_tm;
    bool                  test_timespec_to_ms_null;
    bool                  test_timespec_to_us_null;
    bool                  test_timespec_to_ns_null;
    bool                  test_timespec_is_valid_null;
    size_t                idx;

    // setup valid structures for tests that need one valid parameter
    timer = time(NULL);

    memset(&tm_valid, 0, sizeof(tm_valid));
    tm_valid.tm_year  = 123;
    tm_valid.tm_mon   = 5;
    tm_valid.tm_mday  = 15;
    tm_valid.tm_hour  = 12;
    tm_valid.tm_min   = 30;
    tm_valid.tm_sec   = 45;

    memset(&ts_valid, 0, sizeof(ts_valid));
    ts_valid.tv_sec  = 1;
    ts_valid.tv_nsec = 500000000L;

    // test 1: d_localtime with NULL timer
    tm_result = d_localtime(NULL, &tm_valid);
    test_localtime_null_timer = (tm_result == NULL);

    // test 2: d_localtime with NULL result
    tm_result = d_localtime(&timer, NULL);
    test_localtime_null_result = (tm_result == NULL);

    // test 3: d_gmtime with NULL timer
    tm_result = d_gmtime(NULL, &tm_valid);
    test_gmtime_null_timer = (tm_result == NULL);

    // test 4: d_gmtime with NULL result
    tm_result = d_gmtime(&timer, NULL);
    test_gmtime_null_result = (tm_result == NULL);

    // test 5: d_ctime with NULL timer
    // d_ctime signature: char* d_ctime(const time_t* timer, char* buf)
    str_result = d_ctime(NULL, buf);
    test_ctime_null_timer = (str_result == NULL);

    // test 6: d_ctime with NULL buffer
    str_result = d_ctime(&timer, NULL);
    test_ctime_null_buffer = (str_result == NULL);

    // test 7: d_asctime with NULL tm
    // d_asctime signature: char* d_asctime(const struct tm* tm, char* buf)
    str_result = d_asctime(NULL, buf);
    test_asctime_null_tm = (str_result == NULL);

    // test 8: d_asctime with NULL buffer
    str_result = d_asctime(&tm_valid, NULL);
    test_asctime_null_buffer = (str_result == NULL);

    // test 9: d_clock_gettime with NULL timespec
    int_result = d_clock_gettime(CLOCK_REALTIME, NULL);
    test_clock_gettime_null_ts = (int_result == -1);

    // test 10: d_clock_getres with NULL resolution
    int_result = d_clock_getres(CLOCK_REALTIME, NULL);
    test_clock_getres_null_res = (int_result == -1);

    // test 11: d_timespec_get with NULL timespec
    int_result = d_timespec_get(NULL, TIME_UTC);
    test_timespec_get_null_ts = (int_result == 0);

    // test 12: d_nanosleep with NULL request
    int_result = d_nanosleep(NULL, NULL);
    test_nanosleep_null_req = (int_result == -1);

    // test 13: d_timegm with NULL tm
    {
        time_t timegm_result;

        timegm_result = d_timegm(NULL);
        test_timegm_null_tm = (timegm_result == (time_t)-1);
    }

    // test 14: d_strptime with NULL string
    str_result = d_strptime(NULL, "%Y-%m-%d", &tm_valid);
    test_strptime_null_str = (str_result == NULL);

    // test 15: d_strptime with NULL format
    str_result = d_strptime("2023-06-15", NULL, &tm_valid);
    test_strptime_null_fmt = (str_result == NULL);

    // test 16: d_strftime_s with NULL buffer
    int_result = d_strftime_s(NULL, sizeof(buf), "%Y-%m-%d", &tm_valid);
    test_strftime_null_buf = (int_result == 0);

    // test 17: d_strftime_s with NULL format
    int_result = d_strftime_s(buf, sizeof(buf), NULL, &tm_valid);
    test_strftime_null_fmt = (int_result == 0);

    // test 18: d_strftime_s with NULL tm
    int_result = d_strftime_s(buf, sizeof(buf), "%Y-%m-%d", NULL);
    test_strftime_null_tm = (int_result == 0);

    // test 19: d_timespec_to_ms with NULL
    int64_result = d_timespec_to_ms(NULL);
    test_timespec_to_ms_null = (int64_result == 0);

    // test 20: d_timespec_to_us with NULL
    int64_result = d_timespec_to_us(NULL);
    test_timespec_to_us_null = (int64_result == 0);

    // test 21: d_timespec_to_ns with NULL
    int64_result = d_timespec_to_ns(NULL);
    test_timespec_to_ns_null = (int64_result == 0);

    // test 22: d_timespec_is_valid with NULL
    int_result = d_timespec_is_valid(NULL);
    test_timespec_is_valid_null = (int_result == 0);

    // build result tree
    group = d_test_object_new_interior("XI. NULL Parameter Tests", 22);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("localtime_null_timer",
                                           test_localtime_null_timer,
                                           "d_localtime NULL timer returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("localtime_null_result",
                                           test_localtime_null_result,
                                           "d_localtime NULL result returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("gmtime_null_timer",
                                           test_gmtime_null_timer,
                                           "d_gmtime NULL timer returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("gmtime_null_result",
                                           test_gmtime_null_result,
                                           "d_gmtime NULL result returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("ctime_null_timer",
                                           test_ctime_null_timer,
                                           "d_ctime NULL timer returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("ctime_null_buffer",
                                           test_ctime_null_buffer,
                                           "d_ctime NULL buffer returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("asctime_null_tm",
                                           test_asctime_null_tm,
                                           "d_asctime NULL tm returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("asctime_null_buffer",
                                           test_asctime_null_buffer,
                                           "d_asctime NULL buffer returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("clock_gettime_null_ts",
                                           test_clock_gettime_null_ts,
                                           "d_clock_gettime NULL ts returns -1");
    group->elements[idx++] = D_ASSERT_TRUE("clock_getres_null_res",
                                           test_clock_getres_null_res,
                                           "d_clock_getres NULL res returns -1");
    group->elements[idx++] = D_ASSERT_TRUE("timespec_get_null_ts",
                                           test_timespec_get_null_ts,
                                           "d_timespec_get NULL ts returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("nanosleep_null_req",
                                           test_nanosleep_null_req,
                                           "d_nanosleep NULL req returns -1");
    group->elements[idx++] = D_ASSERT_TRUE("timegm_null_tm",
                                           test_timegm_null_tm,
                                           "d_timegm NULL tm returns -1");
    group->elements[idx++] = D_ASSERT_TRUE("strptime_null_str",
                                           test_strptime_null_str,
                                           "d_strptime NULL str returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("strptime_null_fmt",
                                           test_strptime_null_fmt,
                                           "d_strptime NULL fmt returns NULL");
    group->elements[idx++] = D_ASSERT_TRUE("strftime_null_buf",
                                           test_strftime_null_buf,
                                           "d_strftime_s NULL buf returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("strftime_null_fmt",
                                           test_strftime_null_fmt,
                                           "d_strftime_s NULL fmt returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("strftime_null_tm",
                                           test_strftime_null_tm,
                                           "d_strftime_s NULL tm returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("timespec_to_ms_null",
                                           test_timespec_to_ms_null,
                                           "d_timespec_to_ms NULL returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("timespec_to_us_null",
                                           test_timespec_to_us_null,
                                           "d_timespec_to_us NULL returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("timespec_to_ns_null",
                                           test_timespec_to_ns_null,
                                           "d_timespec_to_ns NULL returns 0");
    group->elements[idx++] = D_ASSERT_TRUE("timespec_is_valid_null",
                                           test_timespec_is_valid_null,
                                           "d_timespec_is_valid NULL returns 0");

    return group;
}