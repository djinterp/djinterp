/*******************************************************************************
* djinterp [test]                                              env_tests_posix.c
*
* Unit tests for `env.h` POSIX standards detection section (Section III).
* Tests POSIX version detection, XSI detection, feature macros, and utilities.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_posix.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.09.26
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <string.h>
// djinterp
#include "env_tests.h"


/******************************************************************************
* POSIX VERSION CONSTANT TESTS
******************************************************************************/

/*
d_tests_env_posix_version_constants
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
bool d_tests_env_posix_version_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_version_constants");

    all_assertions_passed = true;

    // verify each constant matches the official _POSIX_VERSION value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_1988 == 198808L",
                          D_ENV_POSIX_VERSION_1988 == 198808L,
                          "POSIX.1-1988 constant should match IEEE value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_1990 == 199009L",
                          D_ENV_POSIX_VERSION_1990 == 199009L,
                          "POSIX.1-1990 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_1993 == 199309L",
                          D_ENV_POSIX_VERSION_1993 == 199309L,
                          "POSIX.1b-1993 constant should match value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_1996 == 199506L",
                          D_ENV_POSIX_VERSION_1996 == 199506L,
                          "POSIX.1c-1995 constant should match value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_2001 == 200112L",
                          D_ENV_POSIX_VERSION_2001 == 200112L,
                          "POSIX.1-2001 constant should match SUSv3 value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_2008 == 200809L",
                          D_ENV_POSIX_VERSION_2008 == 200809L,
                          "POSIX.1-2008 constant should match SUSv4 value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_2017 == 201700L",
                          D_ENV_POSIX_VERSION_2017 == 201700L,
                          "POSIX.1-2017 constant should match value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_VERSION_2024 == 202405L",
                          D_ENV_POSIX_VERSION_2024 == 202405L,
                          "POSIX.1-2024 constant should match value"))
    {
        all_assertions_passed = false;
    }

    // verify strictly ascending chronological order
    if (!D_TEST_CHECK_MSG(_suite,
                          "1988 < 1990",
                          D_ENV_POSIX_VERSION_1988 < D_ENV_POSIX_VERSION_1990,
                          "versions must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "1990 < 1993",
                          D_ENV_POSIX_VERSION_1990 < D_ENV_POSIX_VERSION_1993,
                          "versions must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "1993 < 1996",
                          D_ENV_POSIX_VERSION_1993 < D_ENV_POSIX_VERSION_1996,
                          "versions must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "1996 < 2001",
                          D_ENV_POSIX_VERSION_1996 < D_ENV_POSIX_VERSION_2001,
                          "versions must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "2001 < 2008",
                          D_ENV_POSIX_VERSION_2001 < D_ENV_POSIX_VERSION_2008,
                          "versions must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "2008 < 2017",
                          D_ENV_POSIX_VERSION_2008 < D_ENV_POSIX_VERSION_2017,
                          "versions must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "2017 < 2024",
                          D_ENV_POSIX_VERSION_2017 < D_ENV_POSIX_VERSION_2024,
                          "versions must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    // verify all constants are positive
    if (!D_TEST_CHECK_MSG(_suite,
                          "POSIX 1988 constant is positive",
                          D_ENV_POSIX_VERSION_1988 > 0,
                          "version constants must be positive"))
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

    return all_assertions_passed;
}


/*
d_tests_env_posix_c_source_constants
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
bool d_tests_env_posix_c_source_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_c_source_constants");

    all_assertions_passed = true;

    // verify each constant
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_C_SOURCE_1 == 1L",
                          D_ENV_POSIX_C_SOURCE_1 == 1L,
                          "basic POSIX.1 should be 1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_C_SOURCE_2 == 2L",
                          D_ENV_POSIX_C_SOURCE_2 == 2L,
                          "POSIX.2 should be 2"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_C_SOURCE_199309L == 199309L",
                          D_ENV_POSIX_C_SOURCE_199309L == 199309L,
                          "real-time extensions should match"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_C_SOURCE_199506L == 199506L",
                          D_ENV_POSIX_C_SOURCE_199506L == 199506L,
                          "threads extensions should match"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_C_SOURCE_200112L == 200112L",
                          D_ENV_POSIX_C_SOURCE_200112L == 200112L,
                          "POSIX.1-2001 should match"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_C_SOURCE_200809L == 200809L",
                          D_ENV_POSIX_C_SOURCE_200809L == 200809L,
                          "POSIX.1-2008 should match"))
    {
        all_assertions_passed = false;
    }

    // verify ascending order
    if (!D_TEST_CHECK_MSG(_suite,
                          "C_SOURCE constants in ascending order",
                          (D_ENV_POSIX_C_SOURCE_1 < D_ENV_POSIX_C_SOURCE_2) && (D_ENV_POSIX_C_SOURCE_2 < D_ENV_POSIX_C_SOURCE_199309L) && (D_ENV_POSIX_C_SOURCE_199309L < D_ENV_POSIX_C_SOURCE_199506L) && (D_ENV_POSIX_C_SOURCE_199506L < D_ENV_POSIX_C_SOURCE_200112L) && (D_ENV_POSIX_C_SOURCE_200112L < D_ENV_POSIX_C_SOURCE_200809L),
                          "feature levels should be ordered"))
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

    return all_assertions_passed;
}


/*
d_tests_env_posix_xsi_version_constants
  Tests XSI (X/Open System Interface) version constants.
  Tests the following:
  - D_ENV_POSIX_XSI_VERSION_3 equals 3L (XPG3)
  - D_ENV_POSIX_XSI_VERSION_4 equals 4L (XPG4)
  - D_ENV_POSIX_XSI_VERSION_500 equals 500L (SUSv2/Unix 98)
  - D_ENV_POSIX_XSI_VERSION_600 equals 600L (SUSv3/Unix 03)
  - D_ENV_POSIX_XSI_VERSION_700 equals 700L (SUSv4/Unix 08)
  - constants are in ascending order
*/
bool d_tests_env_posix_xsi_version_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_xsi_version_constants");

    all_assertions_passed = true;

    // verify each constant
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_XSI_VERSION_3 == 3L",
                          D_ENV_POSIX_XSI_VERSION_3 == 3L,
                          "XPG3 should be 3"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_XSI_VERSION_4 == 4L",
                          D_ENV_POSIX_XSI_VERSION_4 == 4L,
                          "XPG4 should be 4"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_XSI_VERSION_500 == 500L",
                          D_ENV_POSIX_XSI_VERSION_500 == 500L,
                          "SUSv2 should be 500"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_XSI_VERSION_600 == 600L",
                          D_ENV_POSIX_XSI_VERSION_600 == 600L,
                          "SUSv3 should be 600"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_XSI_VERSION_700 == 700L",
                          D_ENV_POSIX_XSI_VERSION_700 == 700L,
                          "SUSv4 should be 700"))
    {
        all_assertions_passed = false;
    }

    // verify ascending order
    if (!D_TEST_CHECK_MSG(_suite,
                          "XSI constants in ascending order",
                          (D_ENV_POSIX_XSI_VERSION_3 < D_ENV_POSIX_XSI_VERSION_4) && (D_ENV_POSIX_XSI_VERSION_4 < D_ENV_POSIX_XSI_VERSION_500) && (D_ENV_POSIX_XSI_VERSION_500 < D_ENV_POSIX_XSI_VERSION_600) && (D_ENV_POSIX_XSI_VERSION_600 < D_ENV_POSIX_XSI_VERSION_700),
                          "XSI levels should be ordered"))
    {
        all_assertions_passed = false;
    }

    // print values
    printf("%s    XSI_VERSION_3:   %ldL (XPG3)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_3);
    printf("%s    XSI_VERSION_4:   %ldL (XPG4)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_4);
    printf("%s    XSI_VERSION_500: %ldL (SUSv2)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_500);
    printf("%s    XSI_VERSION_600: %ldL (SUSv3)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_600);
    printf("%s    XSI_VERSION_700: %ldL (SUSv4)\n", D_INDENT, D_ENV_POSIX_XSI_VERSION_700);

    return all_assertions_passed;
}


/******************************************************************************
* POSIX VERSION DETECTION TESTS
******************************************************************************/

/*
d_tests_env_posix_version_detection
  Tests POSIX version detection and resulting macros.
  Tests the following:
  - D_ENV_POSIX_VERSION is always defined (may be 0)
  - D_ENV_POSIX_NAME is always defined
  - exactly one version flag is defined (2024, 2017, 2008, etc. or NONE/LIKELY)
  - version and name are consistent
*/
bool d_tests_env_posix_version_detection(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    version_flag_count;
    long   detected_version;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_version_detection");

    all_assertions_passed = true;
    version_flag_count    = 0;

    // verify D_ENV_POSIX_VERSION is defined
    #ifdef D_ENV_POSIX_VERSION
        detected_version = D_ENV_POSIX_VERSION;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_VERSION is defined",
                              true,
                              "POSIX version macro must always be defined"))
        {
            all_assertions_passed = false;
        }

        // verify non-negative value
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_VERSION >= 0",
                              detected_version >= 0,
                              "version should be non-negative"))
        {
            all_assertions_passed = false;
        }
    #else
        detected_version = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_VERSION should be defined",
                              false,
                              "POSIX version macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_POSIX_NAME is defined
    #ifdef D_ENV_POSIX_NAME
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_NAME is defined",
                              true,
                              "POSIX name macro must always be defined"))
        {
            all_assertions_passed = false;
        }

        // verify name is not empty
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_NAME is not empty",
                              (D_ENV_POSIX_NAME != NULL) && (D_ENV_POSIX_NAME[0] != '\0'),
                              "name should be non-empty string"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_NAME should be defined",
                              false,
                              "POSIX name macro must always be defined"))
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
    if (!D_TEST_CHECK_MSG(_suite,
                          "exactly one POSIX version flag defined",
                          version_flag_count == 1,
                          "detection should set exactly one flag"))
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

    return all_assertions_passed;
}


/*
d_tests_env_posix_version_name_consistency
  Tests that POSIX version and name are consistent.
  Tests the following:
  - version flag matches D_ENV_POSIX_VERSION value
  - D_ENV_POSIX_NAME matches the detected version
  - edge cases like NONE and LIKELY have version 0
*/
bool d_tests_env_posix_version_name_consistency(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   version_name_match;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_version_name_consistency");

    all_assertions_passed = true;
    version_name_match    = false;

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

        if (!D_TEST_CHECK_MSG(_suite,
                              "POSIX version and name are consistent",
                              version_name_match,
                              "name should match detected version"))
        {
            all_assertions_passed = false;
            printf("%s    ERROR: version=%ldL name=\"%s\"\n",
                   D_INDENT,
                   D_ENV_POSIX_VERSION,
                   D_ENV_POSIX_NAME);
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "both VERSION and NAME should be defined",
                              false,
                              "consistency check requires both macros"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify NONE implies version 0
    #ifdef D_ENV_POSIX_NONE
        if (!D_TEST_CHECK_MSG(_suite,
                              "POSIX_NONE implies VERSION == 0",
                              D_ENV_POSIX_VERSION == 0,
                              "no POSIX should have version 0"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify LIKELY implies version 0
    #ifdef D_ENV_POSIX_LIKELY
        if (!D_TEST_CHECK_MSG(_suite,
                              "POSIX_LIKELY implies VERSION == 0",
                              D_ENV_POSIX_VERSION == 0,
                              "unconfirmed POSIX should have version 0"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* XSI DETECTION TESTS
******************************************************************************/

/*
d_tests_env_posix_xsi_detection
  Tests XSI (X/Open System Interface) detection.
  Tests the following:
  - D_ENV_POSIX_XSI_VERSION is always defined (may be 0)
  - D_ENV_POSIX_XSI_NAME is always defined
  - exactly one XSI flag is defined (700, 600, 500, etc. or NONE)
  - version and name are consistent
*/
bool d_tests_env_posix_xsi_detection(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    xsi_flag_count;
    long   detected_version;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_xsi_detection");

    all_assertions_passed = true;
    xsi_flag_count        = 0;

    // verify D_ENV_POSIX_XSI_VERSION is defined
    #ifdef D_ENV_POSIX_XSI_VERSION
        detected_version = D_ENV_POSIX_XSI_VERSION;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_XSI_VERSION is defined",
                              true,
                              "XSI version macro must always be defined"))
        {
            all_assertions_passed = false;
        }

        // verify non-negative value
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_XSI_VERSION >= 0",
                              detected_version >= 0,
                              "version should be non-negative"))
        {
            all_assertions_passed = false;
        }
    #else
        detected_version = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_XSI_VERSION should be defined",
                              false,
                              "XSI version macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_POSIX_XSI_NAME is defined
    #ifdef D_ENV_POSIX_XSI_NAME
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_XSI_NAME is defined",
                              true,
                              "XSI name macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_XSI_NAME should be defined",
                              false,
                              "XSI name macro must always be defined"))
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
    if (!D_TEST_CHECK_MSG(_suite,
                          "exactly one XSI flag defined",
                          xsi_flag_count == 1,
                          "detection should set exactly one flag"))
    {
        all_assertions_passed = false;
        printf("%s    ERROR: found %d XSI flags\n", D_INDENT, xsi_flag_count);
    }

    // verify NONE implies version 0
    #ifdef D_ENV_POSIX_XSI_NONE
        if (!D_TEST_CHECK_MSG(_suite,
                              "XSI_NONE implies VERSION == 0",
                              D_ENV_POSIX_XSI_VERSION == 0,
                              "no XSI should have version 0"))
        {
            all_assertions_passed = false;
        }
    #endif

    // report detected values
    #if defined(D_ENV_POSIX_XSI_VERSION) && defined(D_ENV_POSIX_XSI_NAME)
        printf("%s    Detected: %s (%ldL)\n",
               D_INDENT,
               D_ENV_POSIX_XSI_NAME,
               (long)D_ENV_POSIX_XSI_VERSION);
    #endif

    // suppress unused warning
    (void)detected_version;

    return all_assertions_passed;
}


/******************************************************************************
* POSIX FEATURE DETECTION TESTS
******************************************************************************/

/*
d_tests_env_posix_feature_macros
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
bool d_tests_env_posix_feature_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    threads_val;
    int    realtime_val;
    int    sockets_val;
    int    shared_mem_val;
    int    semaphores_val;
    int    msg_queues_val;
    int    mem_mapping_val;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_feature_macros");

    all_assertions_passed = true;

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
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_FEATURE_THREADS is defined",
                          threads_val >= 0,
                          "threads feature macro must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FEATURE_THREADS is 0 or 1",
                          (threads_val == 0) || (threads_val == 1),
                          "feature macro should be boolean"))
    {
        all_assertions_passed = false;
    }

    // verify REALTIME is defined and boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_FEATURE_REALTIME is defined",
                          realtime_val >= 0,
                          "realtime feature macro must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FEATURE_REALTIME is 0 or 1",
                          (realtime_val == 0) || (realtime_val == 1),
                          "feature macro should be boolean"))
    {
        all_assertions_passed = false;
    }

    // verify SOCKETS is defined and boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_FEATURE_SOCKETS is defined",
                          sockets_val >= 0,
                          "sockets feature macro must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FEATURE_SOCKETS is 0 or 1",
                          (sockets_val == 0) || (sockets_val == 1),
                          "feature macro should be boolean"))
    {
        all_assertions_passed = false;
    }

    // verify SHARED_MEMORY is defined and boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_FEATURE_SHARED_MEMORY is defined",
                          shared_mem_val >= 0,
                          "shared memory feature macro must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FEATURE_SHARED_MEMORY is 0 or 1",
                          (shared_mem_val == 0) || (shared_mem_val == 1),
                          "feature macro should be boolean"))
    {
        all_assertions_passed = false;
    }

    // verify SEMAPHORES is defined and boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_FEATURE_SEMAPHORES is defined",
                          semaphores_val >= 0,
                          "semaphores feature macro must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FEATURE_SEMAPHORES is 0 or 1",
                          (semaphores_val == 0) || (semaphores_val == 1),
                          "feature macro should be boolean"))
    {
        all_assertions_passed = false;
    }

    // verify MESSAGE_QUEUES is defined and boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_FEATURE_MESSAGE_QUEUES is defined",
                          msg_queues_val >= 0,
                          "message queues feature macro must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FEATURE_MESSAGE_QUEUES is 0 or 1",
                          (msg_queues_val == 0) || (msg_queues_val == 1),
                          "feature macro should be boolean"))
    {
        all_assertions_passed = false;
    }

    // verify MEMORY_MAPPING is defined and boolean
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_POSIX_FEATURE_MEMORY_MAPPING is defined",
                          mem_mapping_val >= 0,
                          "memory mapping feature macro must be defined"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "FEATURE_MEMORY_MAPPING is 0 or 1",
                          (mem_mapping_val == 0) || (mem_mapping_val == 1),
                          "feature macro should be boolean"))
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

    return all_assertions_passed;
}


/******************************************************************************
* POSIX UTILITY MACRO TESTS
******************************************************************************/

/*
d_tests_env_posix_utility_is_available
  Tests D_ENV_POSIX_IS_AVAILABLE and D_ENV_XSI_IS_AVAILABLE macros.
  Tests the following:
  - D_ENV_POSIX_IS_AVAILABLE is defined
  - evaluates to 0 when POSIX_NONE, 1 otherwise (when version > 0)
  - D_ENV_XSI_IS_AVAILABLE is defined
  - evaluates to 0 when XSI_NONE, 1 otherwise
*/
bool d_tests_env_posix_utility_is_available(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    posix_available;
    int    xsi_available;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_utility_is_available");

    all_assertions_passed = true;

    // verify D_ENV_POSIX_IS_AVAILABLE is defined
    #ifdef D_ENV_POSIX_IS_AVAILABLE
        posix_available = D_ENV_POSIX_IS_AVAILABLE;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_IS_AVAILABLE is defined",
                              true,
                              "availability macro must be defined"))
        {
            all_assertions_passed = false;
        }

        // verify boolean value
        if (!D_TEST_CHECK_MSG(_suite,
                              "POSIX_IS_AVAILABLE is 0 or 1",
                              (posix_available == 0) || (posix_available == 1),
                              "availability should be boolean"))
        {
            all_assertions_passed = false;
        }

        // verify consistency with POSIX_NONE
        #ifdef D_ENV_POSIX_NONE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "IS_AVAILABLE == 0 when POSIX_NONE",
                                  posix_available == 0,
                                  "no POSIX means not available"))
            {
                all_assertions_passed = false;
            }
        #else
            // if not NONE, should match (VERSION > 0)
            if (!D_TEST_CHECK_MSG(_suite,
                                  "IS_AVAILABLE matches (VERSION > 0)",
                                  posix_available == (D_ENV_POSIX_VERSION > 0),
                                  "availability should match version check"))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        posix_available = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_IS_AVAILABLE should be defined",
                              false,
                              "availability macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_XSI_IS_AVAILABLE is defined
    #ifdef D_ENV_XSI_IS_AVAILABLE
        xsi_available = D_ENV_XSI_IS_AVAILABLE;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_XSI_IS_AVAILABLE is defined",
                              true,
                              "XSI availability macro must be defined"))
        {
            all_assertions_passed = false;
        }

        // verify boolean value
        if (!D_TEST_CHECK_MSG(_suite,
                              "XSI_IS_AVAILABLE is 0 or 1",
                              (xsi_available == 0) || (xsi_available == 1),
                              "availability should be boolean"))
        {
            all_assertions_passed = false;
        }

        // verify consistency with XSI_NONE
        #ifdef D_ENV_POSIX_XSI_NONE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "XSI_IS_AVAILABLE == 0 when XSI_NONE",
                                  xsi_available == 0,
                                  "no XSI means not available"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "XSI_IS_AVAILABLE matches (VERSION > 0)",
                                  xsi_available == (D_ENV_POSIX_XSI_VERSION > 0),
                                  "availability should match version check"))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        xsi_available = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_XSI_IS_AVAILABLE should be defined",
                              false,
                              "XSI availability macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    POSIX_IS_AVAILABLE: %d\n", D_INDENT, posix_available);
    printf("%s    XSI_IS_AVAILABLE:   %d\n", D_INDENT, xsi_available);

    return all_assertions_passed;
}


/*
d_tests_env_posix_utility_is_modern
  Tests D_ENV_POSIX_IS_MODERN macro.
  Tests the following:
  - D_ENV_POSIX_IS_MODERN is defined
  - evaluates to 1 when version >= 2001, 0 otherwise
  - correctly identifies pre-2001 vs modern POSIX
*/
bool d_tests_env_posix_utility_is_modern(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    is_modern;
    int    expected_modern;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_utility_is_modern");

    all_assertions_passed = true;

    // verify D_ENV_POSIX_IS_MODERN is defined
    #ifdef D_ENV_POSIX_IS_MODERN
        is_modern = D_ENV_POSIX_IS_MODERN;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_IS_MODERN is defined",
                              true,
                              "modern check macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        is_modern = -1;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_POSIX_IS_MODERN should be defined",
                              false,
                              "modern check macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // calculate expected value
    expected_modern = (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001) ? 1 : 0;

    // verify matches expected
    if (!D_TEST_CHECK_MSG(_suite,
                          "IS_MODERN matches (VERSION >= 2001)",
                          is_modern == expected_modern,
                          "modern should mean POSIX.1-2001 or later"))
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
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_MODERN == 1 for 2001/2008/2017/2024",
                              is_modern == 1,
                              "these versions are modern"))
        {
            all_assertions_passed = false;
        }
    #endif

    #if defined(D_ENV_POSIX_1988) || defined(D_ENV_POSIX_1990) || \
        defined(D_ENV_POSIX_1993) || defined(D_ENV_POSIX_1996)
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_MODERN == 0 for 1988/1990/1993/1996",
                              is_modern == 0,
                              "these versions are not modern"))
        {
            all_assertions_passed = false;
        }
    #endif

    printf("%s    D_ENV_POSIX_IS_MODERN: %d (expected %d)\n",
           D_INDENT,
           is_modern,
           expected_modern);

    return all_assertions_passed;
}


/*
d_tests_env_posix_utility_version_at_least
  Tests D_ENV_POSIX_VERSION_AT_LEAST and D_ENV_XSI_VERSION_AT_LEAST macros.
  Tests the following:
  - macros evaluate correctly for various version thresholds
  - boundary conditions work correctly
  - macros are usable in #if directives
*/
bool d_tests_env_posix_utility_version_at_least(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   at_least_1988;
    bool   at_least_2001;
    bool   at_least_2008;
    bool   at_least_2024;
    bool   xsi_at_least_500;
    bool   xsi_at_least_700;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_utility_version_at_least");

    all_assertions_passed = true;

    // test POSIX VERSION_AT_LEAST
    at_least_1988 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_1988);
    at_least_2001 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_2001);
    at_least_2008 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_2008);
    at_least_2024 = D_ENV_POSIX_VERSION_AT_LEAST(D_ENV_POSIX_VERSION_2024);

    // verify macro expands correctly
    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(1988) matches direct comparison",
                          at_least_1988 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_1988),
                          "macro should equal >= comparison"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(2001) matches direct comparison",
                          at_least_2001 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001),
                          "macro should equal >= comparison"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(2008) matches direct comparison",
                          at_least_2008 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2008),
                          "macro should equal >= comparison"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "VERSION_AT_LEAST(2024) matches direct comparison",
                          at_least_2024 == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2024),
                          "macro should equal >= comparison"))
    {
        all_assertions_passed = false;
    }

    // verify monotonic property
    if (at_least_2024)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "2024 implies 2008, 2001, 1988",
                              at_least_2008 && at_least_2001 && at_least_1988,
                              "higher version implies all lower versions"))
        {
            all_assertions_passed = false;
        }
    }

    if (at_least_2008)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "2008 implies 2001, 1988",
                              at_least_2001 && at_least_1988,
                              "higher version implies all lower versions"))
        {
            all_assertions_passed = false;
        }
    }

    // test XSI VERSION_AT_LEAST
    xsi_at_least_500 = D_ENV_XSI_VERSION_AT_LEAST(D_ENV_POSIX_XSI_VERSION_500);
    xsi_at_least_700 = D_ENV_XSI_VERSION_AT_LEAST(D_ENV_POSIX_XSI_VERSION_700);

    if (!D_TEST_CHECK_MSG(_suite,
                          "XSI_VERSION_AT_LEAST(500) matches comparison",
                          xsi_at_least_500 == (D_ENV_POSIX_XSI_VERSION >= D_ENV_POSIX_XSI_VERSION_500),
                          "macro should equal >= comparison"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "XSI_VERSION_AT_LEAST(700) matches comparison",
                          xsi_at_least_700 == (D_ENV_POSIX_XSI_VERSION >= D_ENV_POSIX_XSI_VERSION_700),
                          "macro should equal >= comparison"))
    {
        all_assertions_passed = false;
    }

    // verify XSI monotonic property
    if (xsi_at_least_700)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "XSI 700 implies 500",
                              xsi_at_least_500,
                              "higher XSI implies lower XSI"))
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

    return all_assertions_passed;
}


/*
d_tests_env_posix_utility_has_feature
  Tests D_ENV_POSIX_HAS_FEATURE macro.
  Tests the following:
  - macro correctly passes through feature values
  - can be used with all feature macros
  - usable in boolean context
*/
bool d_tests_env_posix_utility_has_feature(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_threads;
    bool   has_realtime;
    bool   has_sockets;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_utility_has_feature");

    all_assertions_passed = true;

    // test HAS_FEATURE with various features
    has_threads  = D_ENV_POSIX_HAS_FEATURE(D_ENV_POSIX_FEATURE_THREADS);
    has_realtime = D_ENV_POSIX_HAS_FEATURE(D_ENV_POSIX_FEATURE_REALTIME);
    has_sockets  = D_ENV_POSIX_HAS_FEATURE(D_ENV_POSIX_FEATURE_SOCKETS);

    // verify macro passes through value correctly
    if (!D_TEST_CHECK_MSG(_suite,
                          "HAS_FEATURE(THREADS) matches direct check",
                          has_threads == (D_ENV_POSIX_FEATURE_THREADS != 0),
                          "macro should pass through value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "HAS_FEATURE(REALTIME) matches direct check",
                          has_realtime == (D_ENV_POSIX_FEATURE_REALTIME != 0),
                          "macro should pass through value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "HAS_FEATURE(SOCKETS) matches direct check",
                          has_sockets == (D_ENV_POSIX_FEATURE_SOCKETS != 0),
                          "macro should pass through value"))
    {
        all_assertions_passed = false;
    }

    // verify usable in boolean context
    if (!D_TEST_CHECK_MSG(_suite,
                          "HAS_FEATURE usable in boolean context",
                          true,
                          "macro should work in if statements"))
    {
        all_assertions_passed = false;
    }

    printf("%s    HAS_FEATURE results:\n", D_INDENT);
    printf("%s      THREADS:  %s\n", D_INDENT, has_threads ? "true" : "false");
    printf("%s      REALTIME: %s\n", D_INDENT, has_realtime ? "true" : "false");
    printf("%s      SOCKETS:  %s\n", D_INDENT, has_sockets ? "true" : "false");

    return all_assertions_passed;
}


/******************************************************************************
* MANUAL DETECTION TESTS
******************************************************************************/

/*
d_tests_env_posix_manual_detection
  Tests manual POSIX detection via D_ENV_DETECTED_POSIX_* variables.
  Tests the following:
  - manual detection variables are mutually exclusive
  - correct mapping when manual mode is active
  - feature detection variables work in manual mode
*/
bool d_tests_env_posix_manual_detection(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    version_count;
    int    feature_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_manual_detection");

    all_assertions_passed = true;
    version_count         = 0;
    feature_count         = 0;

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
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one POSIX version detection var",
                          version_count <= 1,
                          "version detection should be mutually exclusive"))
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
            if (!D_TEST_CHECK_MSG(_suite,
                                  "DETECTED_2008 maps to correct version",
                                  D_ENV_POSIX_VERSION == D_ENV_POSIX_VERSION_2008,
                                  "manual detection should set correct value"))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_DETECTED_POSIX_NONE
            if (!D_TEST_CHECK_MSG(_suite,
                                  "DETECTED_NONE maps to version 0",
                                  D_ENV_POSIX_VERSION == 0,
                                  "no POSIX should have version 0"))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        printf("%s    Automatic POSIX detection mode\n", D_INDENT);

        if (!D_TEST_CHECK_MSG(_suite,
                              "automatic detection mode active",
                              true,
                              "manual vars are informational only"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* EDGE CASE AND INTEGRATION TESTS
******************************************************************************/

/*
d_tests_env_posix_consistency_check
  Tests overall consistency of POSIX detection.
  Tests the following:
  - version/name pairs are both defined
  - feature macros are consistent with version
  - XSI detection is consistent with POSIX detection
  - no contradictory states
*/
bool d_tests_env_posix_consistency_check(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   posix_version_defined;
    bool   posix_name_defined;
    bool   xsi_version_defined;
    bool   xsi_name_defined;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_consistency_check");

    all_assertions_passed  = true;
    posix_version_defined  = false;
    posix_name_defined     = false;
    xsi_version_defined    = false;
    xsi_name_defined       = false;

    // check POSIX version/name pair
    #ifdef D_ENV_POSIX_VERSION
        posix_version_defined = true;
    #endif

    #ifdef D_ENV_POSIX_NAME
        posix_name_defined = true;
    #endif

    if (!D_TEST_CHECK_MSG(_suite,
                          "POSIX version and name both defined or both not",
                          posix_version_defined == posix_name_defined,
                          "version/name pair should be consistent"))
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

    if (!D_TEST_CHECK_MSG(_suite,
                          "XSI version and name both defined or both not",
                          xsi_version_defined == xsi_name_defined,
                          "version/name pair should be consistent"))
    {
        all_assertions_passed = false;
    }

    // verify IS_AVAILABLE is consistent with NONE flag
    #ifdef D_ENV_POSIX_NONE
        if (!D_TEST_CHECK_MSG(_suite,
                              "NONE implies IS_AVAILABLE == 0",
                              D_ENV_POSIX_IS_AVAILABLE == 0,
                              "no POSIX means not available"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify IS_MODERN is consistent with version
    #ifdef D_ENV_POSIX_VERSION
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_MODERN consistent with VERSION",
                              D_ENV_POSIX_IS_MODERN == (D_ENV_POSIX_VERSION >= D_ENV_POSIX_VERSION_2001),
                              "modern check should match version comparison"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify feature macros exist when version indicates support
    #if defined(D_ENV_POSIX_2001) || defined(D_ENV_POSIX_2008) || \
        defined(D_ENV_POSIX_2017) || defined(D_ENV_POSIX_2024)
        // modern POSIX should have feature macros defined
        #ifndef D_ENV_POSIX_FEATURE_THREADS
            if (!D_TEST_CHECK_MSG(_suite,
                                  "FEATURE_THREADS defined for modern POSIX",
                                  false,
                                  "modern POSIX should have feature macros"))
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

    return all_assertions_passed;
}


/*
d_tests_env_posix_feature_test_patterns
  Tests practical use of POSIX macros for feature testing.
  Tests the following:
  - macros can be used in #if directives
  - common POSIX feature gate patterns work
  - combination of version and feature checks
*/
bool d_tests_env_posix_feature_test_patterns(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   can_use_pthreads;
    bool   can_use_realtime;
    bool   is_unix_like;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_posix_feature_test_patterns");

    all_assertions_passed = true;

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
    if (!D_TEST_CHECK_MSG(_suite,
                          "POSIX feature patterns compile successfully",
                          true,
                          "#if directives with POSIX macros work"))
    {
        all_assertions_passed = false;
    }

    // verify consistency between patterns and individual checks
    if (can_use_pthreads)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "pthreads pattern consistent",
                              D_ENV_POSIX_IS_AVAILABLE && D_ENV_POSIX_FEATURE_THREADS,
                              "pattern should match individual checks"))
        {
            all_assertions_passed = false;
        }
    }

    // suppress unused warnings
    (void)can_use_pthreads;
    (void)can_use_realtime;
    (void)is_unix_like;

    return all_assertions_passed;
}


/******************************************************************************
* MODULE TEST AGGREGATOR
******************************************************************************/

/*
d_tests_env_posix_all
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
bool d_tests_env_posix_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_posix_all");

    d_tests_env_posix_version_constants(_suite);
    d_tests_env_posix_c_source_constants(_suite);
    d_tests_env_posix_xsi_version_constants(_suite);
    d_tests_env_posix_version_detection(_suite);
    d_tests_env_posix_version_name_consistency(_suite);
    d_tests_env_posix_xsi_detection(_suite);
    d_tests_env_posix_feature_macros(_suite);
    d_tests_env_posix_utility_is_available(_suite);
    d_tests_env_posix_utility_is_modern(_suite);
    d_tests_env_posix_utility_version_at_least(_suite);
    d_tests_env_posix_utility_has_feature(_suite);
    d_tests_env_posix_manual_detection(_suite);
    d_tests_env_posix_consistency_check(_suite);
    d_tests_env_posix_feature_test_patterns(_suite);

    return true;
}
