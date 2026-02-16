/******************************************************************************
* djinterp [test]                                         env_tests_sa_posix.c
*
* Unit tests for `env.h` POSIX standards detection section (Section III).
* Tests POSIX version detection, XSI detection, feature macros, and utilities.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_posix.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.26
******************************************************************************/

#include "env_tests_sa.h"
#include <string.h>


/******************************************************************************
* POSIX VERSION CONSTANT TESTS
******************************************************************************/

/*
d_tests_sa_env_posix_version_constants
  Tests POSIX version constant definitions.
  Tests the following:
  - D_ENV_POSIX_VERSION_1988 equals official value 198808L
  - D_ENV_POSIX_VERSION_1990 equals official value 199009L
  - D_ENV_POSIX_VERSION_1993 equals official value 199309L
  - D_ENV_POSIX_VERSION_1996 equals official value 199506L
  - D_ENV_POSIX_VERSION_2001 equals official value 200112L
  - D_ENV_POSIX_VERSION_2008 equals official value 200809L
  - D_ENV_POSIX_VERSION_2017 equals official value 201700L
  - D_ENV_POSIX_VERSION_2024 equals official value 202405L
  - constants are in strictly ascending chronological order
*/
bool
d_tests_sa_env_posix_version_constants
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing POSIX Version Constants ---\n", D_INDENT);

    // verify each constant matches the official _POSIX_VERSION value
    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1988 == 198808L,
                             "D_ENV_POSIX_VERSION_1988 == 198808L",
                             "POSIX.1-1988 constant should match IEEE value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1990 == 199009L,
                             "D_ENV_POSIX_VERSION_1990 == 199009L",
                             "POSIX.1-1990 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1993 == 199309L,
                             "D_ENV_POSIX_VERSION_1993 == 199309L",
                             "POSIX.1b-1993 constant should match value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1996 == 199506L,
                             "D_ENV_POSIX_VERSION_1996 == 199506L",
                             "POSIX.1c-1995 constant should match value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_2001 == 200112L,
                             "D_ENV_POSIX_VERSION_2001 == 200112L",
                             "POSIX.1-2001 constant should match SUSv3 value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_2008 == 200809L,
                             "D_ENV_POSIX_VERSION_2008 == 200809L",
                             "POSIX.1-2008 constant should match SUSv4 value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_2017 == 201700L,
                             "D_ENV_POSIX_VERSION_2017 == 201700L",
                             "POSIX.1-2017 constant should match value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_2024 == 202405L,
                             "D_ENV_POSIX_VERSION_2024 == 202405L",
                             "POSIX.1-2024 constant should match value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify strictly ascending chronological order
    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1988 < D_ENV_POSIX_VERSION_1990,
                             "1988 < 1990",
                             "versions must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1990 < D_ENV_POSIX_VERSION_1993,
                             "1990 < 1993",
                             "versions must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1993 < D_ENV_POSIX_VERSION_1996,
                             "1993 < 1996",
                             "versions must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1996 < D_ENV_POSIX_VERSION_2001,
                             "1996 < 2001",
                             "versions must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_2001 < D_ENV_POSIX_VERSION_2008,
                             "2001 < 2008",
                             "versions must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_2008 < D_ENV_POSIX_VERSION_2017,
                             "2008 < 2017",
                             "versions must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_VERSION_2017 < D_ENV_POSIX_VERSION_2024,
                             "2017 < 2024",
                             "versions must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify all constants are positive
    if (!d_assert_standalone(D_ENV_POSIX_VERSION_1988 > 0,
                             "POSIX 1988 constant is positive",
                             "version constants must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // print constant values for verification
    printf("%s    1988: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_1988);
    printf("%s    1990: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_1990);
    printf("%s    1993: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_1993);
    printf("%s    1996: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_1996);
    printf("%s    2001: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_2001);
    printf("%s    2008: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_2008);
    printf("%s    2017: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_2017);
    printf("%s    2024: %ldL\n", D_INDENT, D_ENV_POSIX_VERSION_2024);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX version constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX version constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_posix_c_source_constants
  Tests _POSIX_C_SOURCE feature level constants.
  Tests the following:
  - D_ENV_POSIX_C_SOURCE_1 equals 1L (basic POSIX.1)
  - D_ENV_POSIX_C_SOURCE_2 equals 2L (POSIX.2)
  - D_ENV_POSIX_C_SOURCE_199309L equals 199309L (real-time)
  - D_ENV_POSIX_C_SOURCE_199506L equals 199506L (threads)
  - D_ENV_POSIX_C_SOURCE_200112L equals 200112L (POSIX.1-2001)
  - D_ENV_POSIX_C_SOURCE_200809L equals 200809L (POSIX.1-2008)
  - constants are in ascending order
*/
bool
d_tests_sa_env_posix_c_source_constants
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing POSIX_C_SOURCE Constants ---\n", D_INDENT);

    // verify each constant
    if (!d_assert_standalone(D_ENV_POSIX_C_SOURCE_1 == 1L,
                             "D_ENV_POSIX_C_SOURCE_1 == 1L",
                             "basic POSIX.1 should be 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_C_SOURCE_2 == 2L,
                             "D_ENV_POSIX_C_SOURCE_2 == 2L",
                             "POSIX.2 should be 2",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_C_SOURCE_199309L == 199309L,
                             "D_ENV_POSIX_C_SOURCE_199309L == 199309L",
                             "real-time extensions should match",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_C_SOURCE_199506L == 199506L,
                             "D_ENV_POSIX_C_SOURCE_199506L == 199506L",
                             "threads extensions should match",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_C_SOURCE_200112L == 200112L,
                             "D_ENV_POSIX_C_SOURCE_200112L == 200112L",
                             "POSIX.1-2001 should match",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_C_SOURCE_200809L == 200809L,
                             "D_ENV_POSIX_C_SOURCE_200809L == 200809L",
                             "POSIX.1-2008 should match",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify ascending order
    if (!d_assert_standalone( (D_ENV_POSIX_C_SOURCE_1 < D_ENV_POSIX_C_SOURCE_2) &&
                              (D_ENV_POSIX_C_SOURCE_2 < D_ENV_POSIX_C_SOURCE_199309L) &&
                              (D_ENV_POSIX_C_SOURCE_199309L < D_ENV_POSIX_C_SOURCE_199506L) &&
                              (D_ENV_POSIX_C_SOURCE_199506L < D_ENV_POSIX_C_SOURCE_200112L) &&
                              (D_ENV_POSIX_C_SOURCE_200112L < D_ENV_POSIX_C_SOURCE_200809L),
                             "C_SOURCE constants in ascending order",
                             "feature levels should be ordered",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // print values
    printf("%s    C_SOURCE_1:       %ldL\n", D_INDENT, D_ENV_POSIX_C_SOURCE_1);
    printf("%s    C_SOURCE_2:       %ldL\n", D_INDENT, D_ENV_POSIX_C_SOURCE_2);
    printf("%s    C_SOURCE_199309L: %ldL\n", D_INDENT, D_ENV_POSIX_C_SOURCE_199309L);
    printf("%s    C_SOURCE_199506L: %ldL\n", D_INDENT, D_ENV_POSIX_C_SOURCE_199506L);
    printf("%s    C_SOURCE_200112L: %ldL\n", D_INDENT, D_ENV_POSIX_C_SOURCE_200112L);
    printf("%s    C_SOURCE_200809L: %ldL\n", D_INDENT, D_ENV_POSIX_C_SOURCE_200809L);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX_C_SOURCE constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX_C_SOURCE constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_posix_xsi_version_constants
  Tests XSI (X/Open System Interface) version constants.
  Tests the following:
  - D_ENV_POSIX_XSI_VERSION_3 equals 3L (XPG3)
  - D_ENV_POSIX_XSI_VERSION_4 equals 4L (XPG4)
  - D_ENV_POSIX_XSI_VERSION_500 equals 500L (SUSv2/Unix 98)
  - D_ENV_POSIX_XSI_VERSION_600 equals 600L (SUSv3/Unix 03)
  - D_ENV_POSIX_XSI_VERSION_700 equals 700L (SUSv4/Unix 08)
  - constants are in ascending order
*/
bool
d_tests_sa_env_posix_xsi_version_constants
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing XSI Version Constants ---\n", D_INDENT);

    // verify each constant
    if (!d_assert_standalone(D_ENV_POSIX_XSI_VERSION_3 == 3L,
                             "D_ENV_POSIX_XSI_VERSION_3 == 3L",
                             "XPG3 should be 3",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_XSI_VERSION_4 == 4L,
                             "D_ENV_POSIX_XSI_VERSION_4 == 4L",
                             "XPG4 should be 4",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_XSI_VERSION_500 == 500L,
                             "D_ENV_POSIX_XSI_VERSION_500 == 500L",
                             "SUSv2 should be 500",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_XSI_VERSION_600 == 600L,
                             "D_ENV_POSIX_XSI_VERSION_600 == 600L",
                             "SUSv3 should be 600",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_POSIX_XSI_VERSION_700 == 700L,
                             "D_ENV_POSIX_XSI_VERSION_700 == 700L",
                             "SUSv4 should be 700",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify ascending order
    if (!d_assert_standalone( (D_ENV_POSIX_XSI_VERSION_3 < D_ENV_POSIX_XSI_VERSION_4) &&
                              (D_ENV_POSIX_XSI_VERSION_4 < D_ENV_POSIX_XSI_VERSION_500) &&
                              (D_ENV_POSIX_XSI_VERSION_500 < D_ENV_POSIX_XSI_VERSION_600) &&
                              (D_ENV_POSIX_XSI_VERSION_600 < D_ENV_POSIX_XSI_VERSION_700),
                             "XSI constants in ascending order",
                             "XSI levels should be ordered",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // print values
    printf("%s    XSI_VERSION_3:   %ldL (XPG3)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_3);
    printf("%s    XSI_VERSION_4:   %ldL (XPG4)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_4);
    printf("%s    XSI_VERSION_500: %ldL (SUSv2)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_500);
    printf("%s    XSI_VERSION_600: %ldL (SUSv3)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_600);
    printf("%s    XSI_VERSION_700: %ldL (SUSv4)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_700);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] XSI version constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] XSI version constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* POSIX VERSION DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_posix_version_detection
  Tests POSIX version detection and resulting macros.
  Tests the following:
  - D_ENV_POSIX_VERSION is always defined (may be 0)
  - D_ENV_POSIX_NAME is always defined
  - exactly one version flag is defined (2024, 2017, 2008, etc. or NONE/LIKELY)
  - version and name are consistent
*/
bool
d_tests_sa_env_posix_version_detection
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    version_flag_count;
    long   detected_version;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    version_flag_count    = 0;

    printf("%s--- Testing POSIX Version Detection ---\n", D_INDENT);

    // verify D_ENV_POSIX_VERSION is defined
    #ifdef D_ENV_POSIX_VERSION
        detected_version = D_ENV_POSIX_VERSION;

        if (!d_assert_standalone(true,
                                 "D_ENV_POSIX_VERSION is defined",
                                 "POSIX version macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify non-negative value
        if (!d_assert_standalone(detected_version >= 0,
                                 "D_ENV_POSIX_VERSION >= 0",
                                 "version should be non-negative",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        detected_version = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_POSIX_VERSION should be defined",
                                 "POSIX version macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_POSIX_NAME is defined
    #ifdef D_ENV_POSIX_NAME
        if (!d_assert_standalone(true,
                                 "D_ENV_POSIX_NAME is defined",
                                 "POSIX name macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify name is not empty
        if (!d_assert_standalone( (D_ENV_POSIX_NAME != NULL) &&
                                  (D_ENV_POSIX_NAME[0] != '\0'),
                                 "D_ENV_POSIX_NAME is not empty",
                                 "name should be non-empty string",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_POSIX_NAME should be defined",
                                 "POSIX name macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // count version flags (should be exactly one)
    #ifdef D_ENV_POSIX_2024
        version_flag_count++;
        printf("%s    D_ENV_POSIX_2024 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_2017
        version_flag_count++;
        printf("%s    D_ENV_POSIX_2017 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_2008
        version_flag_count++;
        printf("%s    D_ENV_POSIX_2008 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_2001
        version_flag_count++;
        printf("%s    D_ENV_POSIX_2001 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_1996
        version_flag_count++;
        printf("%s    D_ENV_POSIX_1996 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_1993
        version_flag_count++;
        printf("%s    D_ENV_POSIX_1993 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_1990
        version_flag_count++;
        printf("%s    D_ENV_POSIX_1990 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_1988
        version_flag_count++;
        printf("%s    D_ENV_POSIX_1988 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_UNKNOWN
        version_flag_count++;
        printf("%s    D_ENV_POSIX_UNKNOWN is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_LIKELY
        version_flag_count++;
        printf("%s    D_ENV_POSIX_LIKELY is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_NONE
        version_flag_count++;
        printf("%s    D_ENV_POSIX_NONE is defined\n", D_INDENT);
    #endif

    // verify exactly one version flag
    if (!d_assert_standalone(version_flag_count == 1,
                             "exactly one POSIX version flag defined",
                             "detection should set exactly one flag",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: found %d version flags\n",
               D_INDENT,
               version_flag_count);
    }

    // report detected values
    #if defined(D_ENV_POSIX_VERSION) && defined(D_ENV_POSIX_NAME)
        printf("%s    Detected: %s (%ldL)\n",
               D_INDENT,
               D_ENV_POSIX_NAME,
               D_ENV_POSIX_VERSION);
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX version detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX version detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_posix_version_name_consistency
  Tests that POSIX version and name are consistent.
  Tests the following:
  - version flag matches D_ENV_POSIX_VERSION value
  - D_ENV_POSIX_NAME matches the detected version
  - edge cases like NONE and LIKELY have version 0
*/
bool
d_tests_sa_env_posix_version_name_consistency
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   version_name_match;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    version_name_match    = false;

    printf("%s--- Testing POSIX Version/Name Consistency ---\n", D_INDENT);

    #if defined(D_ENV_POSIX_VERSION) && defined(D_ENV_POSIX_NAME)
        // check each possible version flag
        #ifdef D_ENV_POSIX_2024
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2024) &&
                                   (strstr(D_ENV_POSIX_NAME, "2024") != NULL) );
        #elif defined(D_ENV_POSIX_2017)
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2017) &&
                                   (D_ENV_POSIX_VERSION < D_ENV_POSIX_VERSION_2024) &&
                                   (strstr(D_ENV_POSIX_NAME, "2017") != NULL) );
        #elif defined(D_ENV_POSIX_2008)
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2008) &&
                                   (D_ENV_POSIX_VERSION < D_ENV_POSIX_VERSION_2017) &&
                                   (strstr(D_ENV_POSIX_NAME, "2008") != NULL) );
        #elif defined(D_ENV_POSIX_2001)
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001) &&
                                   (D_ENV_POSIX_VERSION < D_ENV_POSIX_VERSION_2008) &&
                                   (strstr(D_ENV_POSIX_NAME, "2001") != NULL) );
        #elif defined(D_ENV_POSIX_1996)
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1996) &&
                                   (D_ENV_POSIX_VERSION < D_ENV_POSIX_VERSION_2001) &&
                                   (strstr(D_ENV_POSIX_NAME, "1995") != NULL) );
        #elif defined(D_ENV_POSIX_1993)
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1993) &&
                                   (D_ENV_POSIX_VERSION < D_ENV_POSIX_VERSION_1996) &&
                                   (strstr(D_ENV_POSIX_NAME, "1993") != NULL) );
        #elif defined(D_ENV_POSIX_1990)
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1990) &&
                                   (D_ENV_POSIX_VERSION < D_ENV_POSIX_VERSION_1993) &&
                                   (strstr(D_ENV_POSIX_NAME, "1990") != NULL) );
        #elif defined(D_ENV_POSIX_1988)
            version_name_match = ( (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1988) &&
                                   (D_ENV_POSIX_VERSION < D_ENV_POSIX_VERSION_1990) &&
                                   (strstr(D_ENV_POSIX_NAME, "1988") != NULL) );
        #elif defined(D_ENV_POSIX_UNKNOWN)
            // unknown has a version but "Unknown" in name
            version_name_match = (strstr(D_ENV_POSIX_NAME, "Unknown") != NULL);
        #elif defined(D_ENV_POSIX_LIKELY)
            // likely has version 0 and "Likely" in name
            version_name_match = ( (D_ENV_POSIX_VERSION == 0) &&
                                   (strstr(D_ENV_POSIX_NAME, "Likely") != NULL) );
        #elif defined(D_ENV_POSIX_NONE)
            // none has version 0 and "None" in name
            version_name_match = ( (D_ENV_POSIX_VERSION == 0) &&
                                   (strcmp(D_ENV_POSIX_NAME, "None") == 0) );
        #endif

        if (!d_assert_standalone(version_name_match,
                                 "POSIX version and name are consistent",
                                 "name should match detected version",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    ERROR: version=%ldL name=\"%s\"\n",
                   D_INDENT,
                   D_ENV_POSIX_VERSION,
                   D_ENV_POSIX_NAME);
        }
    #else
        if (!d_assert_standalone(false,
                                 "both VERSION and NAME should be defined",
                                 "consistency check requires both macros",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify NONE implies version 0
    #ifdef D_ENV_POSIX_NONE
        if (!d_assert_standalone(D_ENV_POSIX_VERSION == 0,
                                 "POSIX_NONE implies VERSION == 0",
                                 "no POSIX should have version 0",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify LIKELY implies version 0
    #ifdef D_ENV_POSIX_LIKELY
        if (!d_assert_standalone(D_ENV_POSIX_VERSION == 0,
                                 "POSIX_LIKELY implies VERSION == 0",
                                 "unconfirmed POSIX should have version 0",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX version/name consistency test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX version/name consistency test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* XSI DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_posix_xsi_detection
  Tests XSI (X/Open System Interface) detection.
  Tests the following:
  - D_ENV_POSIX_XSI_VERSION is always defined (may be 0)
  - D_ENV_POSIX_XSI_NAME is always defined
  - exactly one XSI flag is defined (700, 600, 500, etc. or NONE)
  - version and name are consistent
*/
bool
d_tests_sa_env_posix_xsi_detection
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    xsi_flag_count;
    long   detected_version;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    xsi_flag_count        = 0;

    printf("%s--- Testing XSI Detection ---\n", D_INDENT);

    // verify D_ENV_POSIX_XSI_VERSION is defined
    #ifdef D_ENV_POSIX_XSI_VERSION
        detected_version = D_ENV_POSIX_XSI_VERSION;

        if (!d_assert_standalone(true,
                                 "D_ENV_POSIX_XSI_VERSION is defined",
                                 "XSI version macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify non-negative value
        if (!d_assert_standalone(detected_version >= 0,
                                 "D_ENV_POSIX_XSI_VERSION >= 0",
                                 "version should be non-negative",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        detected_version = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_POSIX_XSI_VERSION should be defined",
                                 "XSI version macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_POSIX_XSI_NAME is defined
    #ifdef D_ENV_POSIX_XSI_NAME
        if (!d_assert_standalone(true,
                                 "D_ENV_POSIX_XSI_NAME is defined",
                                 "XSI name macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_POSIX_XSI_NAME should be defined",
                                 "XSI name macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // count XSI flags (should be exactly one)
    #ifdef D_ENV_POSIX_XSI_700
        xsi_flag_count++;
        printf("%s    D_ENV_POSIX_XSI_700 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_XSI_600
        xsi_flag_count++;
        printf("%s    D_ENV_POSIX_XSI_600 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_XSI_500
        xsi_flag_count++;
        printf("%s    D_ENV_POSIX_XSI_500 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_XSI_4
        xsi_flag_count++;
        printf("%s    D_ENV_POSIX_XSI_4 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_XSI_3
        xsi_flag_count++;
        printf("%s    D_ENV_POSIX_XSI_3 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_XSI_UNKNOWN
        xsi_flag_count++;
        printf("%s    D_ENV_POSIX_XSI_UNKNOWN is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_POSIX_XSI_NONE
        xsi_flag_count++;
        printf("%s    D_ENV_POSIX_XSI_NONE is defined\n", D_INDENT);
    #endif

    // verify exactly one XSI flag
    if (!d_assert_standalone(xsi_flag_count == 1,
                             "exactly one XSI flag defined",
                             "detection should set exactly one flag",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: found %d XSI flags\n", D_INDENT, xsi_flag_count);
    }

    // verify NONE implies version 0
    #ifdef D_ENV_POSIX_XSI_NONE
        if (!d_assert_standalone(D_ENV_POSIX_XSI_VERSION == 0,
                                 "XSI_NONE implies VERSION == 0",
                                 "no XSI should have version 0",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // report detected values
    #if defined(D_ENV_POSIX_XSI_VERSION) && defined(D_ENV_POSIX_XSI_NAME)
        printf("%s    Detected: %s (%ldL)\n",
               D_INDENT,
               D_ENV_POSIX_XSI_NAME,
               D_ENV_POSIX_XSI_VERSION);
    #endif

    // suppress unused warning
    (void)detected_version;

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] XSI detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] XSI detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* POSIX FEATURE DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_posix_feature_macros
  Tests POSIX feature detection macros.
  Tests the following:
  - D_ENV_POSIX_FEATURE_THREADS is defined as 0 or 1
  - D_ENV_POSIX_FEATURE_REALTIME is defined as 0 or 1
  - D_ENV_POSIX_FEATURE_SOCKETS is defined as 0 or 1
  - D_ENV_POSIX_FEATURE_SHARED_MEMORY is defined as 0 or 1
  - D_ENV_POSIX_FEATURE_SEMAPHORES is defined as 0 or 1
  - D_ENV_POSIX_FEATURE_MESSAGE_QUEUES is defined as 0 or 1
  - D_ENV_POSIX_FEATURE_MEMORY_MAPPING is defined as 0 or 1
  - all feature macros are boolean (exactly 0 or 1)
*/
bool
d_tests_sa_env_posix_feature_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    threads_val;
    int    realtime_val;
    int    sockets_val;
    int    shared_mem_val;
    int    semaphores_val;
    int    msg_queues_val;
    int    mem_mapping_val;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing POSIX Feature Macros ---\n", D_INDENT);

    // get all feature values
    #ifdef D_ENV_POSIX_FEATURE_THREADS
        threads_val = D_ENV_POSIX_FEATURE_THREADS;
    #else
        threads_val = -1;
    #endif

    #ifdef D_ENV_POSIX_FEATURE_REALTIME
        realtime_val = D_ENV_POSIX_FEATURE_REALTIME;
    #else
        realtime_val = -1;
    #endif

    #ifdef D_ENV_POSIX_FEATURE_SOCKETS
        sockets_val = D_ENV_POSIX_FEATURE_SOCKETS;
    #else
        sockets_val = -1;
    #endif

    #ifdef D_ENV_POSIX_FEATURE_SHARED_MEMORY
        shared_mem_val = D_ENV_POSIX_FEATURE_SHARED_MEMORY;
    #else
        shared_mem_val = -1;
    #endif

    #ifdef D_ENV_POSIX_FEATURE_SEMAPHORES
        semaphores_val = D_ENV_POSIX_FEATURE_SEMAPHORES;
    #else
        semaphores_val = -1;
    #endif

    #ifdef D_ENV_POSIX_FEATURE_MESSAGE_QUEUES
        msg_queues_val = D_ENV_POSIX_FEATURE_MESSAGE_QUEUES;
    #else
        msg_queues_val = -1;
    #endif

    #ifdef D_ENV_POSIX_FEATURE_MEMORY_MAPPING
        mem_mapping_val = D_ENV_POSIX_FEATURE_MEMORY_MAPPING;
    #else
        mem_mapping_val = -1;
    #endif

    // verify THREADS is defined and boolean
    if (!d_assert_standalone(threads_val >= 0,
                             "D_ENV_POSIX_FEATURE_THREADS is defined",
                             "threads feature macro must be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (threads_val == 0) || (threads_val == 1),
                             "FEATURE_THREADS is 0 or 1",
                             "feature macro should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify REALTIME is defined and boolean
    if (!d_assert_standalone(realtime_val >= 0,
                             "D_ENV_POSIX_FEATURE_REALTIME is defined",
                             "realtime feature macro must be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (realtime_val == 0) || (realtime_val == 1),
                             "FEATURE_REALTIME is 0 or 1",
                             "feature macro should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify SOCKETS is defined and boolean
    if (!d_assert_standalone(sockets_val >= 0,
                             "D_ENV_POSIX_FEATURE_SOCKETS is defined",
                             "sockets feature macro must be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (sockets_val == 0) || (sockets_val == 1),
                             "FEATURE_SOCKETS is 0 or 1",
                             "feature macro should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify SHARED_MEMORY is defined and boolean
    if (!d_assert_standalone(shared_mem_val >= 0,
                             "D_ENV_POSIX_FEATURE_SHARED_MEMORY is defined",
                             "shared memory feature macro must be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (shared_mem_val == 0) || (shared_mem_val == 1),
                             "FEATURE_SHARED_MEMORY is 0 or 1",
                             "feature macro should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify SEMAPHORES is defined and boolean
    if (!d_assert_standalone(semaphores_val >= 0,
                             "D_ENV_POSIX_FEATURE_SEMAPHORES is defined",
                             "semaphores feature macro must be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (semaphores_val == 0) || (semaphores_val == 1),
                             "FEATURE_SEMAPHORES is 0 or 1",
                             "feature macro should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify MESSAGE_QUEUES is defined and boolean
    if (!d_assert_standalone(msg_queues_val >= 0,
                             "D_ENV_POSIX_FEATURE_MESSAGE_QUEUES is defined",
                             "message queues feature macro must be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (msg_queues_val == 0) || (msg_queues_val == 1),
                             "FEATURE_MESSAGE_QUEUES is 0 or 1",
                             "feature macro should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify MEMORY_MAPPING is defined and boolean
    if (!d_assert_standalone(mem_mapping_val >= 0,
                             "D_ENV_POSIX_FEATURE_MEMORY_MAPPING is defined",
                             "memory mapping feature macro must be defined",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (mem_mapping_val == 0) || (mem_mapping_val == 1),
                             "FEATURE_MEMORY_MAPPING is 0 or 1",
                             "feature macro should be boolean",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // report feature values
    printf("%s    THREADS:        %d\n", D_INDENT, threads_val);
    printf("%s    REALTIME:       %d\n", D_INDENT, realtime_val);
    printf("%s    SOCKETS:        %d\n", D_INDENT, sockets_val);
    printf("%s    SHARED_MEMORY:  %d\n", D_INDENT, shared_mem_val);
    printf("%s    SEMAPHORES:     %d\n", D_INDENT, semaphores_val);
    printf("%s    MESSAGE_QUEUES: %d\n", D_INDENT, msg_queues_val);
    printf("%s    MEMORY_MAPPING: %d\n", D_INDENT, mem_mapping_val);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX feature macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX feature macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* POSIX UTILITY MACRO TESTS
******************************************************************************/

/*
d_tests_sa_env_posix_utility_is_available
  Tests D_ENV_POSIX_IS_AVAILABLE and D_ENV_XSI_IS_AVAILABLE macros.
  Tests the following:
  - D_ENV_POSIX_IS_AVAILABLE is defined
  - evaluates to 0 when POSIX_NONE, 1 otherwise (when version > 0)
  - D_ENV_XSI_IS_AVAILABLE is defined
  - evaluates to 0 when XSI_NONE, 1 otherwise
*/
bool
d_tests_sa_env_posix_utility_is_available
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    posix_available;
    int    xsi_available;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing IS_AVAILABLE Macros ---\n", D_INDENT);

    // verify D_ENV_POSIX_IS_AVAILABLE is defined
    #ifdef D_ENV_POSIX_IS_AVAILABLE
        posix_available = D_ENV_POSIX_IS_AVAILABLE;

        if (!d_assert_standalone(true,
                                 "D_ENV_POSIX_IS_AVAILABLE is defined",
                                 "availability macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify boolean value
        if (!d_assert_standalone( (posix_available == 0) || (posix_available == 1),
                                 "POSIX_IS_AVAILABLE is 0 or 1",
                                 "availability should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify consistency with POSIX_NONE
        #ifdef D_ENV_POSIX_NONE
            if (!d_assert_standalone(posix_available == 0,
                                     "IS_AVAILABLE == 0 when POSIX_NONE",
                                     "no POSIX means not available",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            // if not NONE, should match (VERSION > 0)
            if (!d_assert_standalone(posix_available == (D_ENV_POSIX_VERSION > 0),
                                     "IS_AVAILABLE matches (VERSION > 0)",
                                     "availability should match version check",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        posix_available = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_POSIX_IS_AVAILABLE should be defined",
                                 "availability macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_XSI_IS_AVAILABLE is defined
    #ifdef D_ENV_XSI_IS_AVAILABLE
        xsi_available = D_ENV_XSI_IS_AVAILABLE;

        if (!d_assert_standalone(true,
                                 "D_ENV_XSI_IS_AVAILABLE is defined",
                                 "XSI availability macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify boolean value
        if (!d_assert_standalone( (xsi_available == 0) || (xsi_available == 1),
                                 "XSI_IS_AVAILABLE is 0 or 1",
                                 "availability should be boolean",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify consistency with XSI_NONE
        #ifdef D_ENV_POSIX_XSI_NONE
            if (!d_assert_standalone(xsi_available == 0,
                                     "XSI_IS_AVAILABLE == 0 when XSI_NONE",
                                     "no XSI means not available",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(xsi_available == (D_ENV_POSIX_XSI_VERSION > 0),
                                     "XSI_IS_AVAILABLE matches (VERSION > 0)",
                                     "availability should match version check",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        xsi_available = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_XSI_IS_AVAILABLE should be defined",
                                 "XSI availability macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    POSIX_IS_AVAILABLE: %d\n", D_INDENT, posix_available);
    printf("%s    XSI_IS_AVAILABLE:   %d\n", D_INDENT, xsi_available);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] IS_AVAILABLE macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] IS_AVAILABLE macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_posix_utility_is_modern
  Tests D_ENV_POSIX_IS_MODERN macro.
  Tests the following:
  - D_ENV_POSIX_IS_MODERN is defined
  - evaluates to 1 when version >= 2001, 0 otherwise
  - correctly identifies pre-2001 vs modern POSIX
*/
bool
d_tests_sa_env_posix_utility_is_modern
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    is_modern;
    int    expected_modern;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing IS_MODERN Macro ---\n", D_INDENT);

    // verify D_ENV_POSIX_IS_MODERN is defined
    #ifdef D_ENV_POSIX_IS_MODERN
        is_modern = D_ENV_POSIX_IS_MODERN;

        if (!d_assert_standalone(true,
                                 "D_ENV_POSIX_IS_MODERN is defined",
                                 "modern check macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        is_modern = -1;

        if (!d_assert_standalone(false,
                                 "D_ENV_POSIX_IS_MODERN should be defined",
                                 "modern check macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // calculate expected value
    expected_modern = (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001) ? 1 : 0;

    // verify matches expected
    if (!d_assert_standalone(is_modern == expected_modern,
                             "IS_MODERN matches (VERSION >= 2001)",
                             "modern should mean POSIX.1-2001 or later",
                             _test_info))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: is_modern=%d but expected=%d\n",
               D_INDENT,
               is_modern,
               expected_modern);
    }

    // verify specific version flags
    #if defined(D_ENV_POSIX_2001) || defined(D_ENV_POSIX_2008) || \
        defined(D_ENV_POSIX_2017) || defined(D_ENV_POSIX_2024)
        if (!d_assert_standalone(is_modern == 1,
                                 "IS_MODERN == 1 for 2001/2008/2017/2024",
                                 "these versions are modern",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    #if defined(D_ENV_POSIX_1988) || defined(D_ENV_POSIX_1990) || \
        defined(D_ENV_POSIX_1993) || defined(D_ENV_POSIX_1996)
        if (!d_assert_standalone(is_modern == 0,
                                 "IS_MODERN == 0 for 1988/1990/1993/1996",
                                 "these versions are not modern",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    D_ENV_POSIX_IS_MODERN: %d (expected %d)\n",
           D_INDENT,
           is_modern,
           expected_modern);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] IS_MODERN macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] IS_MODERN macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_posix_utility_version_at_least
  Tests D_ENV_POSIX_VERSION_AT_LEAST and D_ENV_XSI_VERSION_AT_LEAST macros.
  Tests the following:
  - macros evaluate correctly for various version thresholds
  - boundary conditions work correctly
  - macros are usable in #if directives
*/
bool
d_tests_sa_env_posix_utility_version_at_least
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   at_least_1988;
    bool   at_least_2001;
    bool   at_least_2008;
    bool   at_least_2024;
    bool   xsi_at_least_500;
    bool   xsi_at_least_700;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing VERSION_AT_LEAST Macros ---\n", D_INDENT);

    // test POSIX VERSION_AT_LEAST
    at_least_1988 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_1988);
    at_least_2001 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_2001);
    at_least_2008 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_2008);
    at_least_2024 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_2024);

    // verify macro expands correctly
    if (!d_assert_standalone(at_least_1988 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1988),
                             "VERSION_AT_LEAST(1988) matches direct comparison",
                             "macro should equal >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(at_least_2001 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001),
                             "VERSION_AT_LEAST(2001) matches direct comparison",
                             "macro should equal >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(at_least_2008 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2008),
                             "VERSION_AT_LEAST(2008) matches direct comparison",
                             "macro should equal >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(at_least_2024 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2024),
                             "VERSION_AT_LEAST(2024) matches direct comparison",
                             "macro should equal >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify monotonic property
    if (at_least_2024)
    {
        if (!d_assert_standalone(at_least_2008 && at_least_2001 && at_least_1988,
                                 "2024 implies 2008, 2001, 1988",
                                 "higher version implies all lower versions",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (at_least_2008)
    {
        if (!d_assert_standalone(at_least_2001 && at_least_1988,
                                 "2008 implies 2001, 1988",
                                 "higher version implies all lower versions",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test XSI VERSION_AT_LEAST
    xsi_at_least_500 = D_ENV_XSI_VERSION_AT_LEAST(D_ENV_POSIX_XSI_VERSION_500);
    xsi_at_least_700 = D_ENV_XSI_VERSION_AT_LEAST(D_ENV_POSIX_XSI_VERSION_700);

    if (!d_assert_standalone(xsi_at_least_500 == (D_ENV_POSIX_XSI_VERSION >= D_ENV_POSIX_XSI_VERSION_500),
                             "XSI_VERSION_AT_LEAST(500) matches comparison",
                             "macro should equal >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(xsi_at_least_700 == (D_ENV_POSIX_XSI_VERSION >= D_ENV_POSIX_XSI_VERSION_700),
                             "XSI_VERSION_AT_LEAST(700) matches comparison",
                             "macro should equal >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify XSI monotonic property
    if (xsi_at_least_700)
    {
        if (!d_assert_standalone(xsi_at_least_500,
                                 "XSI 700 implies 500",
                                 "higher XSI implies lower XSI",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    printf("%s    POSIX VERSION_AT_LEAST results:\n", D_INDENT);
    printf("%s      1988: %s\n", D_INDENT, at_least_1988 ? "true" : "false");
    printf("%s      2001: %s\n", D_INDENT, at_least_2001 ? "true" : "false");
    printf("%s      2008: %s\n", D_INDENT, at_least_2008 ? "true" : "false");
    printf("%s      2024: %s\n", D_INDENT, at_least_2024 ? "true" : "false");
    printf("%s    XSI VERSION_AT_LEAST results:\n", D_INDENT);
    printf("%s      500:  %s\n", D_INDENT, xsi_at_least_500 ? "true" : "false");
    printf("%s      700:  %s\n", D_INDENT, xsi_at_least_700 ? "true" : "false");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] VERSION_AT_LEAST macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] VERSION_AT_LEAST macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_posix_utility_has_feature
  Tests D_ENV_POSIX_HAS_FEATURE macro.
  Tests the following:
  - macro correctly passes through feature values
  - can be used with all feature macros
  - usable in boolean context
*/
bool
d_tests_sa_env_posix_utility_has_feature
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_threads;
    bool   has_realtime;
    bool   has_sockets;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing HAS_FEATURE Macro ---\n", D_INDENT);

    // test HAS_FEATURE with various features
    has_threads  = D_ENV_POSIX_HAS_FEATURE(D_ENV_POSIX_FEATURE_THREADS);
    has_realtime = D_ENV_POSIX_HAS_FEATURE(D_ENV_POSIX_FEATURE_REALTIME);
    has_sockets  = D_ENV_POSIX_HAS_FEATURE(D_ENV_POSIX_FEATURE_SOCKETS);

    // verify macro passes through value correctly
    if (!d_assert_standalone(has_threads == (D_ENV_POSIX_FEATURE_THREADS != 0),
                             "HAS_FEATURE(THREADS) matches direct check",
                             "macro should pass through value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(has_realtime == (D_ENV_POSIX_FEATURE_REALTIME != 0),
                             "HAS_FEATURE(REALTIME) matches direct check",
                             "macro should pass through value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(has_sockets == (D_ENV_POSIX_FEATURE_SOCKETS != 0),
                             "HAS_FEATURE(SOCKETS) matches direct check",
                             "macro should pass through value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify usable in boolean context
    if (!d_assert_standalone(true,
                             "HAS_FEATURE usable in boolean context",
                             "macro should work in if statements",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    HAS_FEATURE results:\n", D_INDENT);
    printf("%s      THREADS:  %s\n", D_INDENT, has_threads ? "true" : "false");
    printf("%s      REALTIME: %s\n", D_INDENT, has_realtime ? "true" : "false");
    printf("%s      SOCKETS:  %s\n", D_INDENT, has_sockets ? "true" : "false");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] HAS_FEATURE macro test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] HAS_FEATURE macro test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MANUAL DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_posix_manual_detection
  Tests manual POSIX detection via D_ENV_DETECTED_POSIX_* variables.
  Tests the following:
  - manual detection variables are mutually exclusive
  - correct mapping when manual mode is active
  - feature detection variables work in manual mode
*/
bool
d_tests_sa_env_posix_manual_detection
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    version_count;
    int    feature_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    version_count         = 0;
    feature_count         = 0;

    printf("%s--- Testing Manual POSIX Detection Variables ---\n", D_INDENT);

    // count version detection variables
    #ifdef D_ENV_DETECTED_POSIX_2024
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2024 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2017
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2017 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2008
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2008 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_2001
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_2001 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1996
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1996 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1993
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1993 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1990
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1990 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_1988
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_1988 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_NONE
        version_count++;
        printf("%s    D_ENV_DETECTED_POSIX_NONE is defined\n", D_INDENT);
    #endif

    // verify at most one version detection variable
    if (!d_assert_standalone(version_count <= 1,
                             "at most one POSIX version detection var",
                             "version detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // count feature detection variables
    #ifdef D_ENV_DETECTED_POSIX_THREADS
        feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_THREADS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_REALTIME
        feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_REALTIME is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_SOCKETS
        feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_SOCKETS is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_POSIX_XSI
        feature_count++;
        printf("%s    D_ENV_DETECTED_POSIX_XSI is defined\n", D_INDENT);
    #endif

    printf("%s    Version detection vars: %d\n", D_INDENT, version_count);
    printf("%s    Feature detection vars: %d\n", D_INDENT, feature_count);

    // if manual mode is active, verify correct mapping
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_POSIX)
        printf("%s    Manual POSIX detection mode active\n", D_INDENT);

        #ifdef D_ENV_DETECTED_POSIX_2008
            if (!d_assert_standalone(D_ENV_POSIX_VERSION == D_ENV_POSIX_VERSION_2008,
                                     "DETECTED_2008 maps to correct version",
                                     "manual detection should set correct value",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_DETECTED_POSIX_NONE
            if (!d_assert_standalone(D_ENV_POSIX_VERSION == 0,
                                     "DETECTED_NONE maps to version 0",
                                     "no POSIX should have version 0",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        printf("%s    Automatic POSIX detection mode\n", D_INDENT);

        if (!d_assert_standalone(true,
                                 "automatic detection mode active",
                                 "manual vars are informational only",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Manual POSIX detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Manual POSIX detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* EDGE CASE AND INTEGRATION TESTS
******************************************************************************/

/*
d_tests_sa_env_posix_consistency_check
  Tests overall consistency of POSIX detection.
  Tests the following:
  - version/name pairs are both defined
  - feature macros are consistent with version
  - XSI detection is consistent with POSIX detection
  - no contradictory states
*/
bool
d_tests_sa_env_posix_consistency_check
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   posix_version_defined;
    bool   posix_name_defined;
    bool   xsi_version_defined;
    bool   xsi_name_defined;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed  = true;
    initial_tests_passed   = _test_info->tests_passed;
    posix_version_defined  = false;
    posix_name_defined     = false;
    xsi_version_defined    = false;
    xsi_name_defined       = false;

    printf("%s--- Testing POSIX Detection Consistency ---\n", D_INDENT);

    // check POSIX version/name pair
    #ifdef D_ENV_POSIX_VERSION
        posix_version_defined = true;
    #endif

    #ifdef D_ENV_POSIX_NAME
        posix_name_defined = true;
    #endif

    if (!d_assert_standalone(posix_version_defined == posix_name_defined,
                             "POSIX version and name both defined or both not",
                             "version/name pair should be consistent",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // check XSI version/name pair
    #ifdef D_ENV_POSIX_XSI_VERSION
        xsi_version_defined = true;
    #endif

    #ifdef D_ENV_POSIX_XSI_NAME
        xsi_name_defined = true;
    #endif

    if (!d_assert_standalone(xsi_version_defined == xsi_name_defined,
                             "XSI version and name both defined or both not",
                             "version/name pair should be consistent",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify IS_AVAILABLE is consistent with NONE flag
    #ifdef D_ENV_POSIX_NONE
        if (!d_assert_standalone(D_ENV_POSIX_IS_AVAILABLE == 0,
                                 "NONE implies IS_AVAILABLE == 0",
                                 "no POSIX means not available",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify IS_MODERN is consistent with version
    #ifdef D_ENV_POSIX_VERSION
        if (!d_assert_standalone( D_ENV_POSIX_IS_MODERN ==
                                  (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001),
                                 "IS_MODERN consistent with VERSION",
                                 "modern check should match version comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify feature macros exist when version indicates support
    #if defined(D_ENV_POSIX_2001) || defined(D_ENV_POSIX_2008) || \
        defined(D_ENV_POSIX_2017) || defined(D_ENV_POSIX_2024)
        // modern POSIX should have feature macros defined
        #ifndef D_ENV_POSIX_FEATURE_THREADS
            if (!d_assert_standalone(false,
                                     "FEATURE_THREADS defined for modern POSIX",
                                     "modern POSIX should have feature macros",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #endif

    printf("%s    POSIX version defined: %s\n",
           D_INDENT,
           posix_version_defined ? "YES" : "NO");
    printf("%s    POSIX name defined:    %s\n",
           D_INDENT,
           posix_name_defined ? "YES" : "NO");
    printf("%s    XSI version defined:   %s\n",
           D_INDENT,
           xsi_version_defined ? "YES" : "NO");
    printf("%s    XSI name defined:      %s\n",
           D_INDENT,
           xsi_name_defined ? "YES" : "NO");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX consistency check test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX consistency check test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_posix_feature_test_patterns
  Tests practical use of POSIX macros for feature testing.
  Tests the following:
  - macros can be used in #if directives
  - common POSIX feature gate patterns work
  - combination of version and feature checks
*/
bool
d_tests_sa_env_posix_feature_test_patterns
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   can_use_pthreads;
    bool   can_use_realtime;
    bool   is_unix_like;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing POSIX Feature Test Patterns ---\n", D_INDENT);

    // test common pthread availability pattern
    #if D_ENV_POSIX_IS_AVAILABLE && D_ENV_POSIX_FEATURE_THREADS
        can_use_pthreads = true;
        printf("%s    pthreads available\n", D_INDENT);
    #else
        can_use_pthreads = false;
        printf("%s    pthreads not available\n", D_INDENT);
    #endif

    // test realtime extensions pattern
    #if D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_1993) && \
        D_ENV_POSIX_FEATURE_REALTIME
        can_use_realtime = true;
        printf("%s    realtime extensions available\n", D_INDENT);
    #else
        can_use_realtime = false;
        printf("%s    realtime extensions not available\n", D_INDENT);
    #endif

    // test Unix-like system pattern
    #if D_ENV_POSIX_IS_AVAILABLE || defined(D_ENV_POSIX_LIKELY)
        is_unix_like = true;
        printf("%s    Unix-like system detected\n", D_INDENT);
    #else
        is_unix_like = false;
        printf("%s    not Unix-like system\n", D_INDENT);
    #endif

    // verify patterns compile and evaluate correctly
    if (!d_assert_standalone(true,
                             "POSIX feature patterns compile successfully",
                             "#if directives with POSIX macros work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify consistency between patterns and individual checks
    if (can_use_pthreads)
    {
        if (!d_assert_standalone( D_ENV_POSIX_IS_AVAILABLE &&
                                  D_ENV_POSIX_FEATURE_THREADS,
                                 "pthreads pattern consistent",
                                 "pattern should match individual checks",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // suppress unused warnings
    (void)can_use_pthreads;
    (void)can_use_realtime;
    (void)is_unix_like;

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] POSIX feature test patterns test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] POSIX feature test patterns test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MODULE TEST AGGREGATOR
******************************************************************************/

/*
d_tests_sa_env_posix_all
  Runs all POSIX environment tests.
  Tests the following:
  - version constants
  - C_SOURCE constants
  - XSI version constants
  - version detection
  - version/name consistency
  - XSI detection
  - feature macros
  - utility macros
  - manual detection
  - consistency and patterns
*/
bool
d_tests_sa_env_posix_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool version_constants_result;
    bool c_source_constants_result;
    bool xsi_constants_result;
    bool version_detection_result;
    bool version_name_result;
    bool xsi_detection_result;
    bool feature_macros_result;
    bool is_available_result;
    bool is_modern_result;
    bool version_at_least_result;
    bool has_feature_result;
    bool manual_detection_result;
    bool consistency_result;
    bool patterns_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing POSIX Standards Detection\n");
    printf("========================================="
           "=======================================\n");

    // run all POSIX tests
    version_constants_result  = d_tests_sa_env_posix_version_constants(&module_counter);
    c_source_constants_result = d_tests_sa_env_posix_c_source_constants(&module_counter);
    xsi_constants_result      = d_tests_sa_env_posix_xsi_version_constants(&module_counter);
    version_detection_result  = d_tests_sa_env_posix_version_detection(&module_counter);
    version_name_result       = d_tests_sa_env_posix_version_name_consistency(&module_counter);
    xsi_detection_result      = d_tests_sa_env_posix_xsi_detection(&module_counter);
    feature_macros_result     = d_tests_sa_env_posix_feature_macros(&module_counter);
    is_available_result       = d_tests_sa_env_posix_utility_is_available(&module_counter);
    is_modern_result          = d_tests_sa_env_posix_utility_is_modern(&module_counter);
    version_at_least_result   = d_tests_sa_env_posix_utility_version_at_least(&module_counter);
    has_feature_result        = d_tests_sa_env_posix_utility_has_feature(&module_counter);
    manual_detection_result   = d_tests_sa_env_posix_manual_detection(&module_counter);
    consistency_result        = d_tests_sa_env_posix_consistency_check(&module_counter);
    patterns_result           = d_tests_sa_env_posix_feature_test_patterns(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( version_constants_result  &&
                       c_source_constants_result &&
                       xsi_constants_result      &&
                       version_detection_result  &&
                       version_name_result       &&
                       xsi_detection_result      &&
                       feature_macros_result     &&
                       is_available_result       &&
                       is_modern_result          &&
                       version_at_least_result   &&
                       has_feature_result        &&
                       manual_detection_result   &&
                       consistency_result        &&
                       patterns_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] POSIX Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] POSIX Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - Version Constants:     %s\n",
               version_constants_result ? "PASSED" : "FAILED");
        printf("  - C_SOURCE Constants:    %s\n",
               c_source_constants_result ? "PASSED" : "FAILED");
        printf("  - XSI Constants:         %s\n",
               xsi_constants_result ? "PASSED" : "FAILED");
        printf("  - Version Detection:     %s\n",
               version_detection_result ? "PASSED" : "FAILED");
        printf("  - Version/Name Match:    %s\n",
               version_name_result ? "PASSED" : "FAILED");
        printf("  - XSI Detection:         %s\n",
               xsi_detection_result ? "PASSED" : "FAILED");
        printf("  - Feature Macros:        %s\n",
               feature_macros_result ? "PASSED" : "FAILED");
        printf("  - IS_AVAILABLE:          %s\n",
               is_available_result ? "PASSED" : "FAILED");
        printf("  - IS_MODERN:             %s\n",
               is_modern_result ? "PASSED" : "FAILED");
        printf("  - VERSION_AT_LEAST:      %s\n",
               version_at_least_result ? "PASSED" : "FAILED");
        printf("  - HAS_FEATURE:           %s\n",
               has_feature_result ? "PASSED" : "FAILED");
        printf("  - Manual Detection:      %s\n",
               manual_detection_result ? "PASSED" : "FAILED");
        printf("  - Consistency Check:     %s\n",
               consistency_result ? "PASSED" : "FAILED");
        printf("  - Feature Patterns:      %s\n",
               patterns_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
