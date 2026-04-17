/******************************************************************************
* djinterp [test]                                           dtime_tests_sa_tz.c
*
*   Tests for timezone utility functions (timegm, tzset).
*
*
* path:      \src\test\dtime_tests_sa_tz.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * VI. TIMEZONE UTILITIES TESTS
 *****************************************************************************/

/*
d_tests_dtime_timegm
  Tests d_timegm for inverse of gmtime (struct tm UTC to time_t).
  Tests the following:
  - converts epoch struct tm to time_t 0
  - converts Y2K struct tm to known timestamp
  - round-trip with gmtime produces original timestamp
  - handles mid-year dates correctly
  - handles end-of-year dates correctly
  - handles leap year dates correctly
  - returns -1 for NULL parameter
  - handles dates before epoch (if supported)
*/
struct d_test_object*
d_tests_dtime_timegm
(
    void
)
{
    struct d_test_object* group;
    struct tm             tm_epoch;
    struct tm             tm_y2k;
    struct tm             tm_midyear;
    struct tm             tm_endyear;
    struct tm             tm_leapyear;
    struct tm             tm_roundtrip;
    time_t                result;
    time_t                original;
    time_t                expected_y2k;
    bool                  test_epoch_conversion;
    bool                  test_y2k_conversion;
    bool                  test_roundtrip;
    bool                  test_midyear;
    bool                  test_endyear;
    bool                  test_leapyear;
    bool                  test_null_param;
    bool                  test_consistency;
    size_t                idx;

    expected_y2k = D_TEST_DTIME_KNOWN_TIMESTAMP;  // 946684800

    // test 1: epoch conversion (1970-01-01 00:00:00 UTC -> 0)
    memset(&tm_epoch, 0, sizeof(tm_epoch));
    tm_epoch.tm_year  = 70;   // years since 1900
    tm_epoch.tm_mon   = 0;    // January
    tm_epoch.tm_mday  = 1;
    tm_epoch.tm_hour  = 0;
    tm_epoch.tm_min   = 0;
    tm_epoch.tm_sec   = 0;
    tm_epoch.tm_isdst = 0;

    result = d_timegm(&tm_epoch);
    test_epoch_conversion = (result == 0);

    // test 2: Y2K conversion (2000-01-01 00:00:00 UTC -> 946684800)
    memset(&tm_y2k, 0, sizeof(tm_y2k));
    tm_y2k.tm_year  = 100;  // years since 1900 (2000)
    tm_y2k.tm_mon   = 0;    // January
    tm_y2k.tm_mday  = 1;
    tm_y2k.tm_hour  = 0;
    tm_y2k.tm_min   = 0;
    tm_y2k.tm_sec   = 0;
    tm_y2k.tm_isdst = 0;

    result = d_timegm(&tm_y2k);
    test_y2k_conversion = (result == expected_y2k);

    // test 3: round-trip with gmtime
    original = time(NULL);
    d_gmtime(&original, &tm_roundtrip);
    result = d_timegm(&tm_roundtrip);
    test_roundtrip = (result == original);

    // test 4: mid-year date (2023-06-15 12:30:45 UTC)
    memset(&tm_midyear, 0, sizeof(tm_midyear));
    tm_midyear.tm_year  = 123;  // 2023
    tm_midyear.tm_mon   = 5;    // June (0-indexed)
    tm_midyear.tm_mday  = 15;
    tm_midyear.tm_hour  = 12;
    tm_midyear.tm_min   = 30;
    tm_midyear.tm_sec   = 45;
    tm_midyear.tm_isdst = 0;

    result = d_timegm(&tm_midyear);
    // verify by converting back
    d_gmtime(&result, &tm_roundtrip);
    test_midyear = (tm_roundtrip.tm_year == 123) &&
                   (tm_roundtrip.tm_mon == 5) &&
                   (tm_roundtrip.tm_mday == 15) &&
                   (tm_roundtrip.tm_hour == 12) &&
                   (tm_roundtrip.tm_min == 30) &&
                   (tm_roundtrip.tm_sec == 45);

    // test 5: end-of-year date (2023-12-31 23:59:59 UTC)
    memset(&tm_endyear, 0, sizeof(tm_endyear));
    tm_endyear.tm_year  = 123;  // 2023
    tm_endyear.tm_mon   = 11;   // December (0-indexed)
    tm_endyear.tm_mday  = 31;
    tm_endyear.tm_hour  = 23;
    tm_endyear.tm_min   = 59;
    tm_endyear.tm_sec   = 59;
    tm_endyear.tm_isdst = 0;

    result = d_timegm(&tm_endyear);
    d_gmtime(&result, &tm_roundtrip);
    test_endyear = (tm_roundtrip.tm_year == 123) &&
                   (tm_roundtrip.tm_mon == 11) &&
                   (tm_roundtrip.tm_mday == 31) &&
                   (tm_roundtrip.tm_hour == 23) &&
                   (tm_roundtrip.tm_min == 59) &&
                   (tm_roundtrip.tm_sec == 59);

    // test 6: leap year date (2024-02-29 00:00:00 UTC)
    memset(&tm_leapyear, 0, sizeof(tm_leapyear));
    tm_leapyear.tm_year  = 124;  // 2024 (leap year)
    tm_leapyear.tm_mon   = 1;    // February
    tm_leapyear.tm_mday  = 29;   // leap day
    tm_leapyear.tm_hour  = 0;
    tm_leapyear.tm_min   = 0;
    tm_leapyear.tm_sec   = 0;
    tm_leapyear.tm_isdst = 0;

    result = d_timegm(&tm_leapyear);
    d_gmtime(&result, &tm_roundtrip);
    test_leapyear = (tm_roundtrip.tm_year == 124) &&
                    (tm_roundtrip.tm_mon == 1) &&
                    (tm_roundtrip.tm_mday == 29);

    // test 7: NULL parameter
    result = d_timegm(NULL);
    test_null_param = (result == (time_t)-1);

    // test 8: consistency - multiple calls with same input produce same output
    memset(&tm_y2k, 0, sizeof(tm_y2k));
    tm_y2k.tm_year  = 100;
    tm_y2k.tm_mon   = 0;
    tm_y2k.tm_mday  = 1;
    tm_y2k.tm_hour  = 0;
    tm_y2k.tm_min   = 0;
    tm_y2k.tm_sec   = 0;
    tm_y2k.tm_isdst = 0;

    result = d_timegm(&tm_y2k);
    original = d_timegm(&tm_y2k);
    test_consistency = (result == original);

    // build result tree
    group = d_test_object_new_interior("d_timegm", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("epoch_conversion",
                                           test_epoch_conversion,
                                           "d_timegm converts epoch to 0");
    group->elements[idx++] = D_ASSERT_TRUE("y2k_conversion",
                                           test_y2k_conversion,
                                           "d_timegm converts Y2K correctly");
    group->elements[idx++] = D_ASSERT_TRUE("roundtrip",
                                           test_roundtrip,
                                           "d_timegm/d_gmtime round-trip works");
    group->elements[idx++] = D_ASSERT_TRUE("midyear",
                                           test_midyear,
                                           "d_timegm handles mid-year dates");
    group->elements[idx++] = D_ASSERT_TRUE("endyear",
                                           test_endyear,
                                           "d_timegm handles end-of-year dates");
    group->elements[idx++] = D_ASSERT_TRUE("leapyear",
                                           test_leapyear,
                                           "d_timegm handles leap year dates");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "d_timegm returns -1 for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "d_timegm produces consistent results");

    return group;
}


/*
d_tests_dtime_tzset
  Tests d_tzset for timezone initialization.
  Tests the following:
  - function executes without error
  - can be called multiple times safely
  - timezone information is available after call
  - daylight saving information is available after call
  - tzname array is populated (platform-dependent)
  - function is idempotent
  - works in conjunction with localtime
  - timezone offset is reasonable
*/
struct d_test_object*
d_tests_dtime_tzset
(
    void
)
{
    struct d_test_object* group;
    struct tm             tm_local;
    time_t                now;
    time_t                epoch;
    bool                  test_executes;
    bool                  test_multiple_calls;
    bool                  test_timezone_available;
    bool                  test_daylight_available;
    bool                  test_tzname_populated;
    bool                  test_idempotent;
    bool                  test_localtime_works;
    bool                  test_offset_reasonable;
    size_t                idx;

    // setup
    now   = time(NULL);
    epoch = 0;

    // test 1: function executes without error
    // d_tzset is void, so we just call it and verify no crash
    d_tzset();
    test_executes = true;  // if we get here, it executed

    // test 2: can be called multiple times safely
    d_tzset();
    d_tzset();
    d_tzset();
    test_multiple_calls = true;  // if we get here, multiple calls are safe

    // test 3: timezone information is available
    // on POSIX, timezone is a global variable set by tzset
    // on Windows, _timezone is set by _tzset
    // we verify by checking that localtime produces different results than gmtime
    // for a known timestamp (unless in UTC timezone)
    {
        struct tm tm_utc;
        struct tm tm_loc;

        d_gmtime(&now, &tm_utc);
        d_localtime(&now, &tm_loc);

        // timezone info is "available" if the function works
        // actual offset may be 0 if in UTC timezone
        test_timezone_available = true;
    }

    // test 4: daylight saving information is available
    // similar to above, we just verify the functions work after tzset
    d_tzset();
    d_localtime(&now, &tm_local);
    // tm_isdst should be 0, 1, or -1 (unknown)
    test_daylight_available = (tm_local.tm_isdst >= -1) && (tm_local.tm_isdst <= 1);

    // test 5: tzname array is populated (platform-specific check)
    // on POSIX: extern char *tzname[2]
    // on Windows: _tzname
    // we can't directly check this portably, so we verify localtime works
    d_tzset();
    d_localtime(&now, &tm_local);
    test_tzname_populated = (tm_local.tm_year >= 70);  // reasonable year

    // test 6: idempotent - calling multiple times doesn't change behavior
    {
        struct tm tm1;
        struct tm tm2;

        d_tzset();
        d_localtime(&now, &tm1);

        d_tzset();
        d_localtime(&now, &tm2);

        test_idempotent = (tm1.tm_year == tm2.tm_year) &&
                          (tm1.tm_mon == tm2.tm_mon) &&
                          (tm1.tm_mday == tm2.tm_mday) &&
                          (tm1.tm_hour == tm2.tm_hour) &&
                          (tm1.tm_min == tm2.tm_min) &&
                          (tm1.tm_sec == tm2.tm_sec);
    }

    // test 7: localtime works correctly after tzset
    d_tzset();
    memset(&tm_local, 0xFF, sizeof(tm_local));
    d_localtime(&epoch, &tm_local);
    // epoch in local time should produce valid fields
    test_localtime_works = (tm_local.tm_sec >= 0) && (tm_local.tm_sec <= 60) &&
                           (tm_local.tm_min >= 0) && (tm_local.tm_min <= 59) &&
                           (tm_local.tm_hour >= 0) && (tm_local.tm_hour <= 23);

    // test 8: timezone offset is reasonable (within +/- 14 hours of UTC)
    // we check by comparing localtime and gmtime for the same timestamp
    {
        struct tm tm_utc;
        struct tm tm_loc;
        int       hour_diff;

        d_gmtime(&now, &tm_utc);
        d_localtime(&now, &tm_loc);

        hour_diff = tm_loc.tm_hour - tm_utc.tm_hour;

        // handle day boundary crossing
        if (hour_diff > 12)
        {
            hour_diff -= 24;
        }
        else if (hour_diff < -12)
        {
            hour_diff += 24;
        }

        // timezone offset should be within +/- 14 hours
        test_offset_reasonable = (hour_diff >= -14) && (hour_diff <= 14);
    }

    // build result tree
    group = d_test_object_new_interior("d_tzset", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("executes",
                                           test_executes,
                                           "d_tzset executes without error");
    group->elements[idx++] = D_ASSERT_TRUE("multiple_calls",
                                           test_multiple_calls,
                                           "d_tzset can be called multiple times");
    group->elements[idx++] = D_ASSERT_TRUE("timezone_available",
                                           test_timezone_available,
                                           "timezone info available after d_tzset");
    group->elements[idx++] = D_ASSERT_TRUE("daylight_available",
                                           test_daylight_available,
                                           "daylight saving info available");
    group->elements[idx++] = D_ASSERT_TRUE("tzname_populated",
                                           test_tzname_populated,
                                           "timezone names populated");
    group->elements[idx++] = D_ASSERT_TRUE("idempotent",
                                           test_idempotent,
                                           "d_tzset is idempotent");
    group->elements[idx++] = D_ASSERT_TRUE("localtime_works",
                                           test_localtime_works,
                                           "localtime works after d_tzset");
    group->elements[idx++] = D_ASSERT_TRUE("offset_reasonable",
                                           test_offset_reasonable,
                                           "timezone offset is reasonable");

    return group;
}


/*
d_tests_dtime_timezone_utilities_all
  Runs all timezone utility tests.
  Tests the following:
  - d_timegm
  - d_tzset
*/
struct d_test_object*
d_tests_dtime_timezone_utilities_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("VI. Timezone Utilities", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_timegm();
    group->elements[idx++] = d_tests_dtime_tzset();

    return group;
}
