/*******************************************************************************
* djinterp [c]                                            dtime_tests_register.c
*
* dtime.h's suite, block "register": the register's decisions on dtime.
*   #26 (the Windows clocks) and #27's MSVC branch are platform paths: on
* Linux and FreeBSD these tests exercise the POSIX path, and on Windows the
* same tests exercise the fixed code.
*
*
* path:      /tests/djinterp/c/dtime_tests_register.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "./dtime_tests.h"  // corresponding header

// std
#include <errno.h>   // EINVAL
#include <string.h>  // memset, strcmp
#include <time.h>    // struct tm, struct timespec


/*
d_tests_dtime_timespec_to_ns
  Decision 14: the precondition (|tv_sec| below about 9.2e9), with a debug
assert; within it the conversion is exact.
  Tests the following:
  - {1, 5} is 1000000005 ns; {-2, 0} is -2e9 ns
  - nine billion seconds, inside the precondition, converts exactly
  - NULL is 0
*/
bool
d_tests_dtime_timespec_to_ns(void)
{
    struct timespec ts;

    ts.tv_sec  = 1;
    ts.tv_nsec = 5;
    D_TEST_REQUIRE(d_timespec_to_ns(&ts) == (int64_t)1000000005);

    ts.tv_sec  = -2;
    ts.tv_nsec = 0;
    D_TEST_REQUIRE(d_timespec_to_ns(&ts) == -(int64_t)2000000000);

    if (sizeof(ts.tv_sec) >= 8)
    {
        ts.tv_sec  = (time_t)9000000000.0;
        ts.tv_nsec = 0;
        D_TEST_REQUIRE(d_timespec_to_ns(&ts) ==
                       (int64_t)9000000000 * (int64_t)1000000000);
    }

    D_TEST_REQUIRE(d_timespec_to_ns(NULL) == 0);

    return true;
}

/*
d_tests_dtime_nanosleep
  Decision 15: the fallback sleeps with thrd_sleep or sleep(), or fails with
ENOSYS; every path checks its request the same way.
  Tests the following:
  - a one-millisecond sleep succeeds (or, on a platform that cannot sleep,
    fails with ENOSYS and nothing else)
  - a NULL request, or a tv_nsec of 1e9, is -1 with EINVAL
*/
bool
d_tests_dtime_nanosleep(void)
{
    struct timespec req;
    int             result;

    req.tv_sec  = 0;
    req.tv_nsec = 1000000;
    errno       = 0;
    result      = d_nanosleep(&req, NULL);
    D_TEST_REQUIRE( (result == 0) ||
                    ( (result == -1) && (errno == ENOSYS) ) );

    errno = 0;
    D_TEST_REQUIRE(d_nanosleep(NULL, NULL) == -1);
    D_TEST_REQUIRE(errno == EINVAL);

    req.tv_nsec = 1000000000L;
    errno       = 0;
    D_TEST_REQUIRE(d_nanosleep(&req, NULL) == -1);
    D_TEST_REQUIRE(errno == EINVAL);

    return true;
}

/*
d_tests_dtime_timegm_normalizes
  Decision 25: timegm where the platform declares it, and a fallback that
normalizes the struct tm as timegm does.
  Tests the following:
  - 2026-01-32 00:00 is 2026-02-01, and the fields are normalized
  - 25:00 on a day is 01:00 the next
  - the epoch is 0, and NULL is -1
*/
bool
d_tests_dtime_timegm_normalizes(void)
{
    struct tm tm;
    struct tm feb;
    time_t    a;
    time_t    b;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 2026 - 1900;
    tm.tm_mon  = 0;
    tm.tm_mday = 32;
    a          = d_timegm(&tm);

    memset(&feb, 0, sizeof(feb));
    feb.tm_year = 2026 - 1900;
    feb.tm_mon  = 1;
    feb.tm_mday = 1;
    b           = d_timegm(&feb);

    D_TEST_REQUIRE(a != (time_t)-1);
    D_TEST_REQUIRE(a == b);
    D_TEST_REQUIRE( (tm.tm_mon == 1) && (tm.tm_mday == 1) );

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 2026 - 1900;
    tm.tm_mon  = 10;
    tm.tm_mday = 5;
    tm.tm_hour = 25;
    a          = d_timegm(&tm);
    D_TEST_REQUIRE( (tm.tm_mday == 6) && (tm.tm_hour == 1) );
    D_TEST_REQUIRE(a == b + (time_t)(278 * 86400 + 3600));

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 70;
    tm.tm_mday = 1;
    D_TEST_REQUIRE(d_timegm(&tm) == 0);
    D_TEST_REQUIRE(d_timegm(NULL) == (time_t)-1);

    return true;
}

/*
d_tests_dtime_clocks
  Decision 26: the clocks cache their frequency and add CPU times as 64-bit
counts (Windows); every platform keeps the contract checked here.
  Tests the following:
  - CLOCK_MONOTONIC never goes back across many reads
  - CLOCK_PROCESS_CPUTIME_ID advances over a busy loop
  - a NULL result is -1 with EINVAL
*/
bool
d_tests_dtime_clocks(void)
{
    struct timespec before;
    struct timespec after;
    volatile long   sink;
    long            i;

    D_TEST_REQUIRE(d_clock_gettime(CLOCK_MONOTONIC, &before) == 0);

    for (i = 0; i < 1000; i++)
    {
        D_TEST_REQUIRE(d_clock_gettime(CLOCK_MONOTONIC, &after) == 0);
        D_TEST_REQUIRE(d_timespec_cmp(&before, &after) <= 0);
        before = after;
    }

    D_TEST_REQUIRE(d_clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &before) == 0);

    for (sink = 0, i = 0; i < 50000000L; i++)
    {
        sink = sink + i;
    }

    D_TEST_REQUIRE(d_clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &after) == 0);
    D_TEST_REQUIRE(d_timespec_cmp(&before, &after) < 0);

    errno = 0;
    D_TEST_REQUIRE(d_clock_gettime(CLOCK_MONOTONIC, NULL) == -1);
    D_TEST_REQUIRE(errno == EINVAL);

    return true;
}

/*
d_tests_dtime_strftime_s
  Decision 27: d_strftime_s is strftime with its arguments checked, on every
platform (the branch to a function that did not exist is gone).
  Tests the following:
  - a date formats, and the count excludes the terminator
  - a NULL format writes the empty string and returns 0
  - a buffer too small returns 0
*/
bool
d_tests_dtime_strftime_s(void)
{
    struct tm tm;
    char      out[32];

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 2026 - 1900;
    tm.tm_mon  = 10;
    tm.tm_mday = 5;

    D_TEST_REQUIRE(d_strftime_s(out, sizeof(out), "%Y-%m-%d", &tm) == 10);
    D_TEST_REQUIRE(strcmp(out, "2026-11-05") == 0);

    out[0] = 'X';
    D_TEST_REQUIRE(d_strftime_s(out, sizeof(out), NULL, &tm) == 0);
    D_TEST_REQUIRE(out[0] == '\0');

    D_TEST_REQUIRE(d_strftime_s(out, 4, "%Y-%m-%d", &tm) == 0);
    D_TEST_REQUIRE(d_strftime_s(out, sizeof(out), "%Y", NULL) == 0);

    return true;
}

/*
d_tests_dtime_timespec_cmp_null
  Decision 28: NULL orders before every value, on both sides, so the
comparison is safe to sort with.
  Tests the following:
  - NULL against NULL is 0; NULL first is -1; NULL second is 1
  - values order by seconds, then nanoseconds
*/
bool
d_tests_dtime_timespec_cmp_null(void)
{
    struct timespec a;
    struct timespec b;

    a.tv_sec  = 1;
    a.tv_nsec = 500;
    b.tv_sec  = 1;
    b.tv_nsec = 600;

    D_TEST_REQUIRE(d_timespec_cmp(NULL, NULL) == 0);
    D_TEST_REQUIRE(d_timespec_cmp(NULL, &a) == -1);
    D_TEST_REQUIRE(d_timespec_cmp(&a, NULL) == 1);
    D_TEST_REQUIRE(d_timespec_cmp(&a, &b) < 0);
    D_TEST_REQUIRE(d_timespec_cmp(&b, &a) > 0);
    D_TEST_REQUIRE(d_timespec_cmp(&a, &a) == 0);

    b.tv_sec  = 0;
    b.tv_nsec = 999999999L;
    D_TEST_REQUIRE(d_timespec_cmp(&b, &a) < 0);

    return true;
}

/*
d_tests_dtime_timespec_type
  Decision 29: the shim that defined _TIMESPEC_DEFINED for MSVC before 2015
is gone; struct timespec is the platform's.
  Tests the following:
  - dtime.h's struct timespec has tv_sec and tv_nsec, usable as time_t and
    long
  - outside Windows, dtime.h has not defined the shim's reserved name
*/
bool
d_tests_dtime_timespec_type(void)
{
    struct timespec ts;

    ts.tv_sec  = (time_t)3;
    ts.tv_nsec = 4L;
    D_TEST_REQUIRE( (ts.tv_sec == 3) && (ts.tv_nsec == 4L) );

#if ( !defined(_WIN32) &&                                                      \
      defined(_TIMESPEC_DEFINED) )
    D_TEST_REQUIRE(!"dtime.h defines _TIMESPEC_DEFINED outside Windows");
#endif

    return true;
}
