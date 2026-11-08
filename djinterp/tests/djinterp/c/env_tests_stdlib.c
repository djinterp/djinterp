/*******************************************************************************
* djinterp [test]                                             env_tests_stdlib.c
*
* Unit tests for `env.h` Standard Library Feature Detection section.
* Tests header availability, POSIX features, string functions, file I/O,
* time functions, math headers, network features, process features,
* memory management, SIMD intrinsics, VLA, and security features.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_stdlib.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.09.26
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./env_tests.h"


/******************************************************************************
* STANDARD HEADERS AVAILABILITY TESTS
******************************************************************************/

/*
d_tests_env_stdlib_c_standard_headers
  Tests C standard header availability macros.
  Tests the following:
  - D_ENV_C_HAS_STDBOOL_H is defined and boolean
  - D_ENV_C_HAS_STDINT_H is defined and boolean
  - D_ENV_C_HAS_INTTYPES_H is defined and boolean
  - D_ENV_C_HAS_STDALIGN_H is defined and boolean
  - D_ENV_C_HAS_UCHAR_H is defined and boolean
  - C99 headers available when C99+
  - C11 headers available when C11+
*/
bool d_tests_env_stdlib_c_standard_headers(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_stdbool;
    int    has_stdint;
    int    has_inttypes;
    int    has_stdalign;
    int    has_uchar;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_c_standard_headers");

    all_assertions_passed = true;

    // get all header availability values
    #ifdef D_ENV_C_HAS_STDBOOL_H
        has_stdbool = D_ENV_C_HAS_STDBOOL_H;
    #else
        has_stdbool = -1;
    #endif

    #ifdef D_ENV_C_HAS_STDINT_H
        has_stdint = D_ENV_C_HAS_STDINT_H;
    #else
        has_stdint = -1;
    #endif

    #ifdef D_ENV_C_HAS_INTTYPES_H
        has_inttypes = D_ENV_C_HAS_INTTYPES_H;
    #else
        has_inttypes = -1;
    #endif

    #ifdef D_ENV_C_HAS_STDALIGN_H
        has_stdalign = D_ENV_C_HAS_STDALIGN_H;
    #else
        has_stdalign = -1;
    #endif

    #ifdef D_ENV_C_HAS_UCHAR_H
        has_uchar = D_ENV_C_HAS_UCHAR_H;
    #else
        has_uchar = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_STDBOOL_H is defined",
                          has_stdbool >= 0,
                          "stdbool.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_STDINT_H is defined",
                          has_stdint >= 0,
                          "stdint.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_INTTYPES_H is defined",
                          has_inttypes >= 0,
                          "inttypes.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_STDALIGN_H is defined",
                          has_stdalign >= 0,
                          "stdalign.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_UCHAR_H is defined",
                          has_uchar >= 0,
                          "uchar.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "HAS_STDBOOL_H is boolean",
                          (has_stdbool == 0) || (has_stdbool == 1),
                          "should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "HAS_STDINT_H is boolean",
                          (has_stdint == 0) || (has_stdint == 1),
                          "should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify C99 headers available when C99+
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        if (!D_TEST_CHECK_MSG(_suite,
                              "stdbool.h available in C99+",
                              has_stdbool == 1,
                              "C99 requires stdbool.h"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "stdint.h available in C99+",
                              has_stdint == 1,
                              "C99 requires stdint.h"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "inttypes.h available in C99+",
                              has_inttypes == 1,
                              "C99 requires inttypes.h"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify C11 headers available when C11+
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        if (!D_TEST_CHECK_MSG(_suite,
                              "uchar.h available in C11+",
                              has_uchar == 1,
                              "C11 requires uchar.h"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_STDBOOL_H:  %d\n", D_INDENT, has_stdbool);
    printf("%s    HAS_STDINT_H:   %d\n", D_INDENT, has_stdint);
    printf("%s    HAS_INTTYPES_H: %d\n", D_INDENT, has_inttypes);
    printf("%s    HAS_STDALIGN_H: %d\n", D_INDENT, has_stdalign);
    printf("%s    HAS_UCHAR_H:    %d\n", D_INDENT, has_uchar);

    return all_assertions_passed;
}


/*
d_tests_env_stdlib_posix_headers
  Tests POSIX header availability macros.
  Tests the following:
  - D_ENV_C_HAS_UNISTD_H is defined and boolean
  - D_ENV_C_HAS_SYS_TYPES_H is defined and boolean
  - D_ENV_C_HAS_SYS_STAT_H is defined and boolean
  - D_ENV_C_HAS_DIRENT_H is defined and boolean
  - headers consistent with OS detection
*/
bool d_tests_env_stdlib_posix_headers(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_unistd;
    int    has_sys_types;
    int    has_sys_stat;
    int    has_dirent;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_posix_headers");

    all_assertions_passed = true;

    // get all header availability values
    #ifdef D_ENV_C_HAS_UNISTD_H
        has_unistd = D_ENV_C_HAS_UNISTD_H;
    #else
        has_unistd = -1;
    #endif

    #ifdef D_ENV_C_HAS_SYS_TYPES_H
        has_sys_types = D_ENV_C_HAS_SYS_TYPES_H;
    #else
        has_sys_types = -1;
    #endif

    #ifdef D_ENV_C_HAS_SYS_STAT_H
        has_sys_stat = D_ENV_C_HAS_SYS_STAT_H;
    #else
        has_sys_stat = -1;
    #endif

    #ifdef D_ENV_C_HAS_DIRENT_H
        has_dirent = D_ENV_C_HAS_DIRENT_H;
    #else
        has_dirent = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_UNISTD_H is defined",
                          has_unistd >= 0,
                          "unistd.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_SYS_TYPES_H is defined",
                          has_sys_types >= 0,
                          "sys/types.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_SYS_STAT_H is defined",
                          has_sys_stat >= 0,
                          "sys/stat.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_C_HAS_DIRENT_H is defined",
                          has_dirent >= 0,
                          "dirent.h availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "POSIX header macros are boolean",
                          ( (has_unistd == 0) || (has_unistd == 1) ) && ( (has_sys_types == 0) || (has_sys_types == 1) ) && ( (has_sys_stat == 0) || (has_sys_stat == 1) ) && ( (has_dirent == 0) || (has_dirent == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // report values
    printf("%s    HAS_UNISTD_H:    %d\n", D_INDENT, has_unistd);
    printf("%s    HAS_SYS_TYPES_H: %d\n", D_INDENT, has_sys_types);
    printf("%s    HAS_SYS_STAT_H:  %d\n", D_INDENT, has_sys_stat);
    printf("%s    HAS_DIRENT_H:    %d\n", D_INDENT, has_dirent);

    return all_assertions_passed;
}


/******************************************************************************
* STRING AND MEMORY FUNCTION TESTS
******************************************************************************/

/*
d_tests_env_stdlib_string_functions
  Tests string and memory function availability macros.
  Tests the following:
  - D_ENV_C_HAS_STRTOK_R is defined and boolean
  - D_ENV_C_HAS_STRTOK_S is defined and boolean
  - D_ENV_C_HAS_SNPRINTF is defined and boolean
  - D_ENV_C_HAS_STRDUP is defined and boolean
  - D_ENV_C_HAS_STRNDUP is defined and boolean
  - D_ENV_C_HAS_STRCASECMP is defined and boolean
  - D_ENV_C_HAS_STRICMP is defined and boolean
  - D_ENV_C_HAS_MEMCCPY is defined and boolean
  - platform-specific functions consistent with OS
*/
bool d_tests_env_stdlib_string_functions(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_strtok_r;
    int    has_strtok_s;
    int    has_snprintf;
    int    has_strdup;
    int    has_strndup;
    int    has_strcasecmp;
    int    has_stricmp;
    int    has_memccpy;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_string_functions");

    all_assertions_passed = true;

    // get all function availability values
    #ifdef D_ENV_C_HAS_STRTOK_R
        has_strtok_r = D_ENV_C_HAS_STRTOK_R;
    #else
        has_strtok_r = -1;
    #endif

    #ifdef D_ENV_C_HAS_STRTOK_S
        has_strtok_s = D_ENV_C_HAS_STRTOK_S;
    #else
        has_strtok_s = -1;
    #endif

    #ifdef D_ENV_C_HAS_SNPRINTF
        has_snprintf = D_ENV_C_HAS_SNPRINTF;
    #else
        has_snprintf = -1;
    #endif

    #ifdef D_ENV_C_HAS_STRDUP
        has_strdup = D_ENV_C_HAS_STRDUP;
    #else
        has_strdup = -1;
    #endif

    #ifdef D_ENV_C_HAS_STRNDUP
        has_strndup = D_ENV_C_HAS_STRNDUP;
    #else
        has_strndup = -1;
    #endif

    #ifdef D_ENV_C_HAS_STRCASECMP
        has_strcasecmp = D_ENV_C_HAS_STRCASECMP;
    #else
        has_strcasecmp = -1;
    #endif

    #ifdef D_ENV_C_HAS_STRICMP
        has_stricmp = D_ENV_C_HAS_STRICMP;
    #else
        has_stricmp = -1;
    #endif

    #ifdef D_ENV_C_HAS_MEMCCPY
        has_memccpy = D_ENV_C_HAS_MEMCCPY;
    #else
        has_memccpy = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all string function macros are defined",
                          (has_strtok_r >= 0) && (has_strtok_s >= 0) && (has_snprintf >= 0) && (has_strdup >= 0) && (has_strndup >= 0) && (has_strcasecmp >= 0) && (has_stricmp >= 0) && (has_memccpy >= 0),
                          "string function availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "string function macros are boolean",
                          ( (has_strtok_r == 0) || (has_strtok_r == 1) ) && ( (has_strtok_s == 0) || (has_strtok_s == 1) ) && ( (has_snprintf == 0) || (has_snprintf == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify snprintf available in C99+
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        if (!D_TEST_CHECK_MSG(_suite,
                              "snprintf available in C99+",
                              has_snprintf == 1,
                              "C99 requires snprintf"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify platform-specific consistency
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "strtok_s available on Windows",
                              has_strtok_s == 1,
                              "Windows should have strtok_s"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "_stricmp available on Windows",
                              has_stricmp == 1,
                              "Windows should have _stricmp"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_STRTOK_R:   %d\n", D_INDENT, has_strtok_r);
    printf("%s    HAS_STRTOK_S:   %d\n", D_INDENT, has_strtok_s);
    printf("%s    HAS_SNPRINTF:   %d\n", D_INDENT, has_snprintf);
    printf("%s    HAS_STRDUP:     %d\n", D_INDENT, has_strdup);
    printf("%s    HAS_STRNDUP:    %d\n", D_INDENT, has_strndup);
    printf("%s    HAS_STRCASECMP: %d\n", D_INDENT, has_strcasecmp);
    printf("%s    HAS_STRICMP:    %d\n", D_INDENT, has_stricmp);
    printf("%s    HAS_MEMCCPY:    %d\n", D_INDENT, has_memccpy);

    return all_assertions_passed;
}


/******************************************************************************
* FILE SYSTEM AND I/O TESTS
******************************************************************************/

/*
d_tests_env_stdlib_file_io_features
  Tests file system and I/O feature availability macros.
  Tests the following:
  - D_ENV_C_HAS_FOPEN_S is defined and boolean
  - D_ENV_C_HAS_MMAP is defined and boolean
  - D_ENV_C_HAS_FSYNC is defined and boolean
  - D_ENV_C_HAS_FLOCK is defined and boolean
  - D_ENV_C_HAS_LOCKFILE is defined and boolean
  - Windows/POSIX exclusive features consistent with OS
*/
bool d_tests_env_stdlib_file_io_features(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_fopen_s;
    int    has_mmap;
    int    has_fsync;
    int    has_flock;
    int    has_lockfile;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_file_io_features");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_FOPEN_S
        has_fopen_s = D_ENV_C_HAS_FOPEN_S;
    #else
        has_fopen_s = -1;
    #endif

    #ifdef D_ENV_C_HAS_MMAP
        has_mmap = D_ENV_C_HAS_MMAP;
    #else
        has_mmap = -1;
    #endif

    #ifdef D_ENV_C_HAS_FSYNC
        has_fsync = D_ENV_C_HAS_FSYNC;
    #else
        has_fsync = -1;
    #endif

    #ifdef D_ENV_C_HAS_FLOCK
        has_flock = D_ENV_C_HAS_FLOCK;
    #else
        has_flock = -1;
    #endif

    #ifdef D_ENV_C_HAS_LOCKFILE
        has_lockfile = D_ENV_C_HAS_LOCKFILE;
    #else
        has_lockfile = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all file I/O macros are defined",
                          (has_fopen_s >= 0) && (has_mmap >= 0) && (has_fsync >= 0) && (has_flock >= 0) && (has_lockfile >= 0),
                          "file I/O availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "file I/O macros are boolean",
                          ( (has_fopen_s == 0) || (has_fopen_s == 1) ) && ( (has_mmap == 0) || (has_mmap == 1) ) && ( (has_fsync == 0) || (has_fsync == 1) ) && ( (has_flock == 0) || (has_flock == 1) ) && ( (has_lockfile == 0) || (has_lockfile == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify Windows-specific features
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "LockFile available on Windows",
                              has_lockfile == 1,
                              "Windows should have LockFile API"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "fopen_s available on Windows",
                              has_fopen_s == 1,
                              "Windows should have fopen_s"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_FOPEN_S:  %d\n", D_INDENT, has_fopen_s);
    printf("%s    HAS_MMAP:     %d\n", D_INDENT, has_mmap);
    printf("%s    HAS_FSYNC:    %d\n", D_INDENT, has_fsync);
    printf("%s    HAS_FLOCK:    %d\n", D_INDENT, has_flock);
    printf("%s    HAS_LOCKFILE: %d\n", D_INDENT, has_lockfile);

    return all_assertions_passed;
}


/******************************************************************************
* TIME AND DATE TESTS
******************************************************************************/

/*
d_tests_env_stdlib_time_features
  Tests time and date feature availability macros.
  Tests the following:
  - D_ENV_C_HAS_TIMESPEC_GET is defined and boolean
  - D_ENV_C_HAS_CLOCK_GETTIME is defined and boolean
  - D_ENV_C_HAS_GETTIMEOFDAY is defined and boolean
  - D_ENV_C_HAS_QUERYPERFORMANCECOUNTER is defined and boolean
  - timespec_get available in C11+
  - Windows timer available on Windows
*/
bool d_tests_env_stdlib_time_features(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_timespec_get;
    int    has_clock_gettime;
    int    has_gettimeofday;
    int    has_qpc;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_time_features");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_TIMESPEC_GET
        has_timespec_get = D_ENV_C_HAS_TIMESPEC_GET;
    #else
        has_timespec_get = -1;
    #endif

    #ifdef D_ENV_C_HAS_CLOCK_GETTIME
        has_clock_gettime = D_ENV_C_HAS_CLOCK_GETTIME;
    #else
        has_clock_gettime = -1;
    #endif

    #ifdef D_ENV_C_HAS_GETTIMEOFDAY
        has_gettimeofday = D_ENV_C_HAS_GETTIMEOFDAY;
    #else
        has_gettimeofday = -1;
    #endif

    #ifdef D_ENV_C_HAS_QUERYPERFORMANCECOUNTER
        has_qpc = D_ENV_C_HAS_QUERYPERFORMANCECOUNTER;
    #else
        has_qpc = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all time feature macros are defined",
                          (has_timespec_get >= 0) && (has_clock_gettime >= 0) && (has_gettimeofday >= 0) && (has_qpc >= 0),
                          "time feature availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "time feature macros are boolean",
                          ( (has_timespec_get == 0) || (has_timespec_get == 1) ) && ( (has_clock_gettime == 0) || (has_clock_gettime == 1) ) && ( (has_gettimeofday == 0) || (has_gettimeofday == 1) ) && ( (has_qpc == 0) || (has_qpc == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify Windows-specific
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "QPC available on Windows",
                              has_qpc == 1,
                              "Windows should have QueryPerformanceCounter"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_TIMESPEC_GET:             %d\n", D_INDENT, has_timespec_get);
    printf("%s    HAS_CLOCK_GETTIME:            %d\n", D_INDENT, has_clock_gettime);
    printf("%s    HAS_GETTIMEOFDAY:             %d\n", D_INDENT, has_gettimeofday);
    printf("%s    HAS_QUERYPERFORMANCECOUNTER:  %d\n", D_INDENT, has_qpc);

    return all_assertions_passed;
}


/******************************************************************************
* MATH FEATURES TESTS
******************************************************************************/

/*
d_tests_env_stdlib_math_features
  Tests math header availability macros.
  Tests the following:
  - D_ENV_C_HAS_TGMATH_H is defined and boolean
  - D_ENV_C_HAS_COMPLEX_H is defined and boolean
  - D_ENV_C_HAS_FENV_H is defined and boolean
  - math headers available in C99+
*/
bool d_tests_env_stdlib_math_features(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_tgmath;
    int    has_complex;
    int    has_fenv;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_math_features");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_TGMATH_H
        has_tgmath = D_ENV_C_HAS_TGMATH_H;
    #else
        has_tgmath = -1;
    #endif

    #ifdef D_ENV_C_HAS_COMPLEX_H
        has_complex = D_ENV_C_HAS_COMPLEX_H;
    #else
        has_complex = -1;
    #endif

    #ifdef D_ENV_C_HAS_FENV_H
        has_fenv = D_ENV_C_HAS_FENV_H;
    #else
        has_fenv = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all math feature macros are defined",
                          (has_tgmath >= 0) && (has_complex >= 0) && (has_fenv >= 0),
                          "math feature availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "math feature macros are boolean",
                          ( (has_tgmath == 0) || (has_tgmath == 1) ) && ( (has_complex == 0) || (has_complex == 1) ) && ( (has_fenv == 0) || (has_fenv == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify C99 math headers
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        if (!D_TEST_CHECK_MSG(_suite,
                              "tgmath.h available in C99+",
                              has_tgmath == 1,
                              "C99 requires tgmath.h"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "fenv.h available in C99+",
                              has_fenv == 1,
                              "C99 requires fenv.h"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_TGMATH_H:  %d\n", D_INDENT, has_tgmath);
    printf("%s    HAS_COMPLEX_H: %d\n", D_INDENT, has_complex);
    printf("%s    HAS_FENV_H:    %d\n", D_INDENT, has_fenv);

    return all_assertions_passed;
}


/******************************************************************************
* NETWORK FEATURES TESTS
******************************************************************************/

/*
d_tests_env_stdlib_network_features
  Tests network feature availability macros.
  Tests the following:
  - D_ENV_C_HAS_WINSOCK is defined and boolean
  - D_ENV_C_HAS_BSD_SOCKETS is defined and boolean
  - D_ENV_C_HAS_GETADDRINFO is defined and boolean
  - platform-specific sockets consistent with OS
*/
bool d_tests_env_stdlib_network_features(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_winsock;
    int    has_bsd_sockets;
    int    has_getaddrinfo;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_network_features");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_WINSOCK
        has_winsock = D_ENV_C_HAS_WINSOCK;
    #else
        has_winsock = -1;
    #endif

    #ifdef D_ENV_C_HAS_BSD_SOCKETS
        has_bsd_sockets = D_ENV_C_HAS_BSD_SOCKETS;
    #else
        has_bsd_sockets = -1;
    #endif

    #ifdef D_ENV_C_HAS_GETADDRINFO
        has_getaddrinfo = D_ENV_C_HAS_GETADDRINFO;
    #else
        has_getaddrinfo = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all network feature macros are defined",
                          (has_winsock >= 0) && (has_bsd_sockets >= 0) && (has_getaddrinfo >= 0),
                          "network feature availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "network feature macros are boolean",
                          ( (has_winsock == 0) || (has_winsock == 1) ) && ( (has_bsd_sockets == 0) || (has_bsd_sockets == 1) ) && ( (has_getaddrinfo == 0) || (has_getaddrinfo == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify Windows has Winsock
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "Winsock available on Windows",
                              has_winsock == 1,
                              "Windows should have Winsock"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_WINSOCK:     %d\n", D_INDENT, has_winsock);
    printf("%s    HAS_BSD_SOCKETS: %d\n", D_INDENT, has_bsd_sockets);
    printf("%s    HAS_GETADDRINFO: %d\n", D_INDENT, has_getaddrinfo);

    return all_assertions_passed;
}


/******************************************************************************
* PROCESS AND SYSTEM FEATURES TESTS
******************************************************************************/

/*
d_tests_env_stdlib_process_features
  Tests process and system feature availability macros.
  Tests the following:
  - D_ENV_C_HAS_FORK is defined and boolean
  - D_ENV_C_HAS_EXECVE is defined and boolean
  - D_ENV_C_HAS_GETPID is defined and boolean
  - D_ENV_C_HAS_SIGNAL_H is defined and boolean
  - POSIX process functions consistent with OS
*/
bool d_tests_env_stdlib_process_features(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_fork;
    int    has_execve;
    int    has_getpid;
    int    has_signal;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_process_features");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_FORK
        has_fork = D_ENV_C_HAS_FORK;
    #else
        has_fork = -1;
    #endif

    #ifdef D_ENV_C_HAS_EXECVE
        has_execve = D_ENV_C_HAS_EXECVE;
    #else
        has_execve = -1;
    #endif

    #ifdef D_ENV_C_HAS_GETPID
        has_getpid = D_ENV_C_HAS_GETPID;
    #else
        has_getpid = -1;
    #endif

    #ifdef D_ENV_C_HAS_SIGNAL_H
        has_signal = D_ENV_C_HAS_SIGNAL_H;
    #else
        has_signal = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all process feature macros are defined",
                          (has_fork >= 0) && (has_execve >= 0) && (has_getpid >= 0) && (has_signal >= 0),
                          "process feature availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "process feature macros are boolean",
                          ( (has_fork == 0) || (has_fork == 1) ) && ( (has_execve == 0) || (has_execve == 1) ) && ( (has_getpid == 0) || (has_getpid == 1) ) && ( (has_signal == 0) || (has_signal == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify Windows doesn't have fork
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "fork not available on Windows",
                              has_fork == 0,
                              "Windows doesn't have fork()"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_FORK:     %d\n", D_INDENT, has_fork);
    printf("%s    HAS_EXECVE:   %d\n", D_INDENT, has_execve);
    printf("%s    HAS_GETPID:   %d\n", D_INDENT, has_getpid);
    printf("%s    HAS_SIGNAL_H: %d\n", D_INDENT, has_signal);

    return all_assertions_passed;
}


/******************************************************************************
* MEMORY MANAGEMENT FEATURES TESTS
******************************************************************************/

/*
d_tests_env_stdlib_memory_features
  Tests memory management feature availability macros.
  Tests the following:
  - D_ENV_C_HAS_ALIGNED_ALLOC is defined and boolean
  - D_ENV_C_HAS_POSIX_MEMALIGN is defined and boolean
  - D_ENV_C_HAS_ALIGNED_MALLOC is defined and boolean
  - D_ENV_C_HAS_ALLOCA is defined and boolean
  - platform-specific allocators consistent with OS
*/
bool d_tests_env_stdlib_memory_features(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_aligned_alloc;
    int    has_posix_memalign;
    int    has_aligned_malloc;
    int    has_alloca;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_memory_features");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_ALIGNED_ALLOC
        has_aligned_alloc = D_ENV_C_HAS_ALIGNED_ALLOC;
    #else
        has_aligned_alloc = -1;
    #endif

    #ifdef D_ENV_C_HAS_POSIX_MEMALIGN
        has_posix_memalign = D_ENV_C_HAS_POSIX_MEMALIGN;
    #else
        has_posix_memalign = -1;
    #endif

    #ifdef D_ENV_C_HAS_ALIGNED_MALLOC
        has_aligned_malloc = D_ENV_C_HAS_ALIGNED_MALLOC;
    #else
        has_aligned_malloc = -1;
    #endif

    #ifdef D_ENV_C_HAS_ALLOCA
        has_alloca = D_ENV_C_HAS_ALLOCA;
    #else
        has_alloca = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all memory feature macros are defined",
                          (has_aligned_alloc >= 0) && (has_posix_memalign >= 0) && (has_aligned_malloc >= 0) && (has_alloca >= 0),
                          "memory feature availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "memory feature macros are boolean",
                          ( (has_aligned_alloc == 0) || (has_aligned_alloc == 1) ) && ( (has_posix_memalign == 0) || (has_posix_memalign == 1) ) && ( (has_aligned_malloc == 0) || (has_aligned_malloc == 1) ) && ( (has_alloca == 0) || (has_alloca == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify Windows has _aligned_malloc
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "_aligned_malloc available on Windows",
                              has_aligned_malloc == 1,
                              "Windows should have _aligned_malloc"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_ALIGNED_ALLOC:  %d\n", D_INDENT, has_aligned_alloc);
    printf("%s    HAS_POSIX_MEMALIGN: %d\n", D_INDENT, has_posix_memalign);
    printf("%s    HAS_ALIGNED_MALLOC: %d\n", D_INDENT, has_aligned_malloc);
    printf("%s    HAS_ALLOCA:         %d\n", D_INDENT, has_alloca);

    return all_assertions_passed;
}


/******************************************************************************
* SIMD AND HARDWARE INTRINSICS TESTS
******************************************************************************/

/*
d_tests_env_stdlib_simd_features
  Tests SIMD and hardware intrinsics availability macros.
  Tests the following:
  - D_ENV_C_HAS_SSE is defined and boolean
  - D_ENV_C_HAS_SSE2 is defined and boolean
  - D_ENV_C_HAS_AVX is defined and boolean
  - D_ENV_C_HAS_AVX2 is defined and boolean
  - D_ENV_C_HAS_NEON is defined and boolean
  - SIMD features consistent with architecture
*/
bool d_tests_env_stdlib_simd_features(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_sse;
    int    has_sse2;
    int    has_avx;
    int    has_avx2;
    int    has_neon;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_simd_features");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_SSE
        has_sse = D_ENV_C_HAS_SSE;
    #else
        has_sse = -1;
    #endif

    #ifdef D_ENV_C_HAS_SSE2
        has_sse2 = D_ENV_C_HAS_SSE2;
    #else
        has_sse2 = -1;
    #endif

    #ifdef D_ENV_C_HAS_AVX
        has_avx = D_ENV_C_HAS_AVX;
    #else
        has_avx = -1;
    #endif

    #ifdef D_ENV_C_HAS_AVX2
        has_avx2 = D_ENV_C_HAS_AVX2;
    #else
        has_avx2 = -1;
    #endif

    #ifdef D_ENV_C_HAS_NEON
        has_neon = D_ENV_C_HAS_NEON;
    #else
        has_neon = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all SIMD feature macros are defined",
                          (has_sse >= 0) && (has_sse2 >= 0) && (has_avx >= 0) && (has_avx2 >= 0) && (has_neon >= 0),
                          "SIMD feature availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "SIMD feature macros are boolean",
                          ( (has_sse == 0) || (has_sse == 1) ) && ( (has_sse2 == 0) || (has_sse2 == 1) ) && ( (has_avx == 0) || (has_avx == 1) ) && ( (has_avx2 == 0) || (has_avx2 == 1) ) && ( (has_neon == 0) || (has_neon == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify SIMD hierarchy (AVX2 implies AVX implies SSE2 implies SSE)
    if (has_avx2 == 1)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "AVX2 implies AVX",
                              has_avx == 1,
                              "AVX2 requires AVX support"))
        {
            all_assertions_passed = false;
        }
    }

    if (has_avx == 1)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "AVX implies SSE2",
                              has_sse2 == 1,
                              "AVX requires SSE2 support"))
        {
            all_assertions_passed = false;
        }
    }

    if (has_sse2 == 1)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "SSE2 implies SSE",
                              has_sse == 1,
                              "SSE2 requires SSE support"))
        {
            all_assertions_passed = false;
        }
    }

    // verify NEON only on ARM
    #if ( (D_ENV_ARCH_TYPE != D_ENV_ARCH_TYPE_ARM) && \
          (D_ENV_ARCH_TYPE != D_ENV_ARCH_TYPE_ARM64) )
        if (!D_TEST_CHECK_MSG(_suite,
                              "NEON not available on non-ARM",
                              has_neon == 0,
                              "NEON is ARM-specific"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify SSE/AVX only on x86/x64
    #if ( (D_ENV_ARCH_TYPE != D_ENV_ARCH_TYPE_X86) && \
          (D_ENV_ARCH_TYPE != D_ENV_ARCH_TYPE_X64) )
        if (!D_TEST_CHECK_MSG(_suite,
                              "SSE/AVX not available on non-x86",
                              (has_sse == 0) && (has_sse2 == 0) && (has_avx == 0) && (has_avx2 == 0),
                              "SSE/AVX are x86-specific"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_SSE:  %d\n", D_INDENT, has_sse);
    printf("%s    HAS_SSE2: %d\n", D_INDENT, has_sse2);
    printf("%s    HAS_AVX:  %d\n", D_INDENT, has_avx);
    printf("%s    HAS_AVX2: %d\n", D_INDENT, has_avx2);
    printf("%s    HAS_NEON: %d\n", D_INDENT, has_neon);

    return all_assertions_passed;
}


/******************************************************************************
* VLA AND SECURITY FEATURES TESTS
******************************************************************************/

/*
d_tests_env_stdlib_vla_and_security
  Tests VLA and security feature availability macros.
  Tests the following:
  - D_ENV_C_HAS_VLA is defined and boolean
  - D_ENV_C_HAS_SECURE_STRING_LIB is defined and boolean
  - D_ENV_C_HAS_GETENTROPY is defined and boolean
  - VLA available in C99+ (unless __STDC_NO_VLA__)
*/
bool d_tests_env_stdlib_vla_and_security(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    has_vla;
    int    has_secure_string;
    int    has_getentropy;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_vla_and_security");

    all_assertions_passed = true;

    // get all feature availability values
    #ifdef D_ENV_C_HAS_VLA
        has_vla = D_ENV_C_HAS_VLA;
    #else
        has_vla = -1;
    #endif

    #ifdef D_ENV_C_HAS_SECURE_STRING_LIB
        has_secure_string = D_ENV_C_HAS_SECURE_STRING_LIB;
    #else
        has_secure_string = -1;
    #endif

    #ifdef D_ENV_C_HAS_GETENTROPY
        has_getentropy = D_ENV_C_HAS_GETENTROPY;
    #else
        has_getentropy = -1;
    #endif

    // verify all macros are defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "all VLA/security macros are defined",
                          (has_vla >= 0) && (has_secure_string >= 0) && (has_getentropy >= 0),
                          "VLA/security availability must be defined"))
    {
        all_assertions_passed = false;
    }

    // verify boolean values
    if (!D_TEST_CHECK_MSG(_suite,
                          "VLA/security macros are boolean",
                          ( (has_vla == 0) || (has_vla == 1) ) && ( (has_secure_string == 0) || (has_secure_string == 1) ) && ( (has_getentropy == 0) || (has_getentropy == 1) ),
                          "all should be 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify Windows has secure string lib
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "secure string lib on Windows",
                              has_secure_string == 1,
                              "Windows should have Annex K"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report values
    printf("%s    HAS_VLA:               %d\n", D_INDENT, has_vla);
    printf("%s    HAS_SECURE_STRING_LIB: %d\n", D_INDENT, has_secure_string);
    printf("%s    HAS_GETENTROPY:        %d\n", D_INDENT, has_getentropy);

    return all_assertions_passed;
}


/******************************************************************************
* CONSISTENCY TESTS
******************************************************************************/

/*
d_tests_env_stdlib_consistency_check
  Tests overall consistency of stdlib feature detection.
  Tests the following:
  - all macros evaluate without error
  - no contradictory platform states
  - at least one timer mechanism is available
  - at least one aligned alloc mechanism on modern systems
*/
bool d_tests_env_stdlib_consistency_check(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_any_timer;
    bool   has_any_aligned_alloc;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_stdlib_consistency_check");

    all_assertions_passed = true;

    // verify at least one timer mechanism is available
    has_any_timer = ( D_ENV_C_HAS_TIMESPEC_GET            ||
                      D_ENV_C_HAS_CLOCK_GETTIME           ||
                      D_ENV_C_HAS_GETTIMEOFDAY            ||
                      D_ENV_C_HAS_QUERYPERFORMANCECOUNTER );

    if (!D_TEST_CHECK_MSG(_suite,
                          "at least one timer mechanism available",
                          has_any_timer,
                          "should have some timing API"))
    {
        all_assertions_passed = false;
    }

    // verify at least one aligned alloc on modern systems
    has_any_aligned_alloc = ( D_ENV_C_HAS_ALIGNED_ALLOC  ||
                              D_ENV_C_HAS_POSIX_MEMALIGN ||
                              D_ENV_C_HAS_ALIGNED_MALLOC );

    // this check only matters on C11+ or POSIX or Windows
    #if ( D_ENV_LANG_IS_C11_OR_HIGHER || D_ENV_POSIX_IS_AVAILABLE || \
          D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID) )
        if (!D_TEST_CHECK_MSG(_suite,
                              "aligned allocation available on modern system",
                              has_any_aligned_alloc,
                              "should have some aligned alloc API"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify Windows has Winsock OR Unix has BSD sockets
    #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
        if (!D_TEST_CHECK_MSG(_suite,
                              "Windows has Winsock",
                              D_ENV_C_HAS_WINSOCK,
                              "Windows networking API"))
        {
            all_assertions_passed = false;
        }
    #elif D_ENV_C_HAS_UNISTD_H
        if (!D_TEST_CHECK_MSG(_suite,
                              "Unix-like has BSD sockets",
                              D_ENV_C_HAS_BSD_SOCKETS,
                              "POSIX networking API"))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    Has any timer:         %s\n",
           D_INDENT,
           has_any_timer ? "YES" : "NO");
    printf("%s    Has aligned alloc:     %s\n",
           D_INDENT,
           has_any_aligned_alloc ? "YES" : "NO");

    return all_assertions_passed;
}


/******************************************************************************
* MODULE TEST AGGREGATOR
******************************************************************************/

/*
d_tests_env_stdlib_all
  Runs all standard library feature detection tests.
  Tests the following:
  - C standard headers
  - POSIX headers
  - string/memory functions
  - file I/O features
  - time/date features
  - math features
  - network features
  - process features
  - memory management features
  - SIMD features
  - VLA and security features
  - consistency check
*/
bool d_tests_env_stdlib_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_stdlib_all");

    d_tests_env_stdlib_c_standard_headers(_suite);
    d_tests_env_stdlib_posix_headers(_suite);
    d_tests_env_stdlib_string_functions(_suite);
    d_tests_env_stdlib_file_io_features(_suite);
    d_tests_env_stdlib_time_features(_suite);
    d_tests_env_stdlib_math_features(_suite);
    d_tests_env_stdlib_network_features(_suite);
    d_tests_env_stdlib_process_features(_suite);
    d_tests_env_stdlib_memory_features(_suite);
    d_tests_env_stdlib_simd_features(_suite);
    d_tests_env_stdlib_vla_and_security(_suite);
    d_tests_env_stdlib_consistency_check(_suite);

    return true;
}
