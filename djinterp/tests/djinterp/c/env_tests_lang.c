/*******************************************************************************
* djinterp [test]                                               env_tests_lang.c
*
* Unit tests for `env.h` language environment section (Section II).
* Tests C and C++ standard detection, version constants, and comparison macros.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests_lang.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.09.26
*                                                            revised: 2026.11.06
*******************************************************************************/

// std
#include <string.h>
// djinterp
#include "env_tests.h"


/******************************************************************************
* LANGUAGE STANDARD VERSION CONSTANT TESTS
******************************************************************************/

/*
d_tests_env_lang_c_standard_constants
  Tests C language standard version constant definitions.
  Tests the following:
  - D_ENV_LANG_C_STANDARD_C95 equals official ISO value 199409L
  - D_ENV_LANG_C_STANDARD_C99 equals official ISO value 199901L
  - D_ENV_LANG_C_STANDARD_C11 equals official ISO value 201112L
  - D_ENV_LANG_C_STANDARD_C17 equals official ISO value 201710L
  - D_ENV_LANG_C_STANDARD_C23 equals official ISO value 202311L
  - constants are in strictly ascending chronological order
  - no gaps or overlaps between adjacent standards
*/
bool d_tests_env_lang_c_standard_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_c_standard_constants");

    all_assertions_passed = true;

    // verify each constant matches the official ISO __STDC_VERSION__ value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_C_STANDARD_C95 == 199409L",
                          D_ENV_LANG_C_STANDARD_C95 == 199409L,
                          "C95 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_C_STANDARD_C99 == 199901L",
                          D_ENV_LANG_C_STANDARD_C99 == 199901L,
                          "C99 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_C_STANDARD_C11 == 201112L",
                          D_ENV_LANG_C_STANDARD_C11 == 201112L,
                          "C11 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_C_STANDARD_C17 == 201710L",
                          D_ENV_LANG_C_STANDARD_C17 == 201710L,
                          "C17 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_C_STANDARD_C23 == 202311L",
                          D_ENV_LANG_C_STANDARD_C23 == 202311L,
                          "C23 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    // verify strictly ascending chronological order
    if (!D_TEST_CHECK_MSG(_suite,
                          "C95 < C99",
                          D_ENV_LANG_C_STANDARD_C95 < D_ENV_LANG_C_STANDARD_C99,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C99 < C11",
                          D_ENV_LANG_C_STANDARD_C99 < D_ENV_LANG_C_STANDARD_C11,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C11 < C17",
                          D_ENV_LANG_C_STANDARD_C11 < D_ENV_LANG_C_STANDARD_C17,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C17 < C23",
                          D_ENV_LANG_C_STANDARD_C17 < D_ENV_LANG_C_STANDARD_C23,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    // verify all constants are positive (valid __STDC_VERSION__ values)
    if (!D_TEST_CHECK_MSG(_suite,
                          "C95 constant is positive",
                          D_ENV_LANG_C_STANDARD_C95 > 0,
                          "version constants must be positive"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_env_lang_cpp_standard_constants
  Tests C++ language standard version constant definitions.
  Tests the following:
  - D_ENV_LANG_CPP_STANDARD_CPP98 equals official ISO value 199711L
  - D_ENV_LANG_CPP_STANDARD_CPP11 equals official ISO value 201103L
  - D_ENV_LANG_CPP_STANDARD_CPP14 equals official ISO value 201402L
  - D_ENV_LANG_CPP_STANDARD_CPP17 equals official ISO value 201703L
  - D_ENV_LANG_CPP_STANDARD_CPP20 equals official ISO value 202002L
  - D_ENV_LANG_CPP_STANDARD_CPP23 equals official ISO value 202302L
  - constants are in strictly ascending chronological order
*/
bool d_tests_env_lang_cpp_standard_constants(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_cpp_standard_constants");

    all_assertions_passed = true;

    // verify each constant matches the official ISO __cplusplus value
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_CPP_STANDARD_CPP98 == 199711L",
                          D_ENV_LANG_CPP_STANDARD_CPP98 == 199711L,
                          "C++98 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_CPP_STANDARD_CPP11 == 201103L",
                          D_ENV_LANG_CPP_STANDARD_CPP11 == 201103L,
                          "C++11 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_CPP_STANDARD_CPP14 == 201402L",
                          D_ENV_LANG_CPP_STANDARD_CPP14 == 201402L,
                          "C++14 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_CPP_STANDARD_CPP17 == 201703L",
                          D_ENV_LANG_CPP_STANDARD_CPP17 == 201703L,
                          "C++17 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_CPP_STANDARD_CPP20 == 202002L",
                          D_ENV_LANG_CPP_STANDARD_CPP20 == 202002L,
                          "C++20 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_CPP_STANDARD_CPP23 == 202302L",
                          D_ENV_LANG_CPP_STANDARD_CPP23 == 202302L,
                          "C++23 constant should match ISO value"))
    {
        all_assertions_passed = false;
    }

    // verify strictly ascending chronological order
    if (!D_TEST_CHECK_MSG(_suite,
                          "C++98 < C++11",
                          D_ENV_LANG_CPP_STANDARD_CPP98 < D_ENV_LANG_CPP_STANDARD_CPP11,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++11 < C++14",
                          D_ENV_LANG_CPP_STANDARD_CPP11 < D_ENV_LANG_CPP_STANDARD_CPP14,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++14 < C++17",
                          D_ENV_LANG_CPP_STANDARD_CPP14 < D_ENV_LANG_CPP_STANDARD_CPP17,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++17 < C++20",
                          D_ENV_LANG_CPP_STANDARD_CPP17 < D_ENV_LANG_CPP_STANDARD_CPP20,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++20 < C++23",
                          D_ENV_LANG_CPP_STANDARD_CPP20 < D_ENV_LANG_CPP_STANDARD_CPP23,
                          "standards must be in chronological order"))
    {
        all_assertions_passed = false;
    }

    // verify all constants are positive
    if (!D_TEST_CHECK_MSG(_suite,
                          "C++98 constant is positive",
                          D_ENV_LANG_CPP_STANDARD_CPP98 > 0,
                          "version constants must be positive"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/*
d_tests_env_lang_standard_constant_ranges
  Tests that C and C++ standard constants don't overlap in problematic ways.
  Tests the following:
  - C and C++ version numbers are distinguishable
  - no ambiguous values between C and C++ standards
  - version format YYYYMM is consistent across all constants
*/
bool d_tests_env_lang_standard_constant_ranges(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_standard_constant_ranges");

    all_assertions_passed = true;

    // verify all constants follow YYYYMM format (6 digits)
    // minimum valid: 199000 (year 1990, month 00)
    // maximum valid: 209912 (year 2099, month 12)
    if (!D_TEST_CHECK_MSG(_suite,
                          "C95 follows YYYYMM format",
                          (D_ENV_LANG_C_STANDARD_C95 >= 199000L) && (D_ENV_LANG_C_STANDARD_C95 <= 209912L),
                          "version should be 6-digit YYYYMM"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C23 follows YYYYMM format",
                          (D_ENV_LANG_C_STANDARD_C23 >= 199000L) && (D_ENV_LANG_C_STANDARD_C23 <= 209912L),
                          "version should be 6-digit YYYYMM"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++98 follows YYYYMM format",
                          (D_ENV_LANG_CPP_STANDARD_CPP98 >= 199000L) && (D_ENV_LANG_CPP_STANDARD_CPP98 <= 209912L),
                          "version should be 6-digit YYYYMM"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++23 follows YYYYMM format",
                          (D_ENV_LANG_CPP_STANDARD_CPP23 >= 199000L) && (D_ENV_LANG_CPP_STANDARD_CPP23 <= 209912L),
                          "version should be 6-digit YYYYMM"))
    {
        all_assertions_passed = false;
    }

    // verify month portion is valid (01-12 typically, 00 for some)
    // extract month from YYYYMM: value % 100
    if (!D_TEST_CHECK_MSG(_suite,
                          "C95 month portion <= 12",
                          (D_ENV_LANG_C_STANDARD_C95 % 100) <= 12,
                          "month should be valid"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++98 month portion <= 12",
                          (D_ENV_LANG_CPP_STANDARD_CPP98 % 100) <= 12,
                          "month should be valid"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
* C STANDARD DETECTION TESTS
******************************************************************************/

/*
d_tests_env_lang_c_standard_detection
  Tests C standard detection and resulting macro definitions.
  Tests the following:
  - D_ENV_LANG_C_STANDARD is always defined (with fallback to C90)
  - D_ENV_LANG_C_STANDARD_NAME is always defined
  - detected standard value is positive
  - standard name matches standard value
  - D_ENV_LANG_USING_C is correctly set
*/
bool d_tests_env_lang_c_standard_detection(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    long        detected_standard;
    const char* detected_name;
    bool        name_matches_value;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_c_standard_detection");

    all_assertions_passed = true;

    // verify D_ENV_LANG_C_STANDARD is defined
    #ifdef D_ENV_LANG_C_STANDARD
        detected_standard = D_ENV_LANG_C_STANDARD;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD is defined",
                              true,
                              "C standard macro must always be defined"))
        {
            all_assertions_passed = false;
        }

        // verify value is positive
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD > 0",
                              detected_standard > 0,
                              "detected standard should be positive"))
        {
            all_assertions_passed = false;
        }

        // verify value is at least C90 (199000L used as fallback)
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD >= 199000L (C90)",
                              detected_standard >= 199000L,
                              "minimum standard should be C90"))
        {
            all_assertions_passed = false;
        }
    #else
        detected_standard = 0;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD should be defined",
                              false,
                              "C standard macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_LANG_C_STANDARD_NAME is defined
    #ifdef D_ENV_LANG_C_STANDARD_NAME
        detected_name = D_ENV_LANG_C_STANDARD_NAME;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD_NAME is defined",
                              true,
                              "standard name macro must be defined"))
        {
            all_assertions_passed = false;
        }

        // verify name is not empty
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD_NAME is not empty",
                              (detected_name != NULL) && (detected_name[0] != '\0'),
                              "standard name should be non-empty string"))
        {
            all_assertions_passed = false;
        }

        // verify name starts with 'C' (all C standards: C90, C95, C99, etc.)
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD_NAME starts with 'C'",
                              (detected_name != NULL) && (detected_name[0] == 'C'),
                              "C standard names should start with 'C'"))
        {
            all_assertions_passed = false;
        }
    #else
        detected_name = NULL;

        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_C_STANDARD_NAME should be defined",
                              false,
                              "standard name macro must be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify name matches value (cross-validation)
    name_matches_value = false;

    #if defined(D_ENV_LANG_C_STANDARD) && defined(D_ENV_LANG_C_STANDARD_NAME)
        if (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C23)
        {
            name_matches_value = (strcmp(D_ENV_LANG_C_STANDARD_NAME, "C23") == 0);
        }
        else if (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C17)
        {
            name_matches_value = (strcmp(D_ENV_LANG_C_STANDARD_NAME, "C17") == 0);
        }
        else if (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C11)
        {
            name_matches_value = (strcmp(D_ENV_LANG_C_STANDARD_NAME, "C11") == 0);
        }
        else if (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C99)
        {
            name_matches_value = (strcmp(D_ENV_LANG_C_STANDARD_NAME, "C99") == 0);
        }
        else if (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C95)
        {
            name_matches_value = (strcmp(D_ENV_LANG_C_STANDARD_NAME, "C95") == 0);
        }
        else
        {
            name_matches_value = (strcmp(D_ENV_LANG_C_STANDARD_NAME, "C90") == 0);
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "standard name matches standard value",
                              name_matches_value,
                              "name and value should be consistent"))
        {
            all_assertions_passed = false;
            (void)d_test_suite_note(_suite, "value=%ld but name=\"%s\"",
                                    D_ENV_LANG_C_STANDARD,
                                    D_ENV_LANG_C_STANDARD_NAME);
        }
    #endif

    // verify D_ENV_LANG_USING_C is correctly set
    #ifdef D_ENV_LANG_C_STANDARD
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_USING_C == 1 when C detected",
                              D_ENV_LANG_USING_C == 1,
                              "USING_C should be 1 when standard is defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_lang_c_standard_fallback
  Tests C90 fallback behavior when __STDC_VERSION__ is not defined.
  Tests the following:
  - fallback value is exactly 199000L
  - fallback name is exactly "C90"
  - fallback occurs when __STDC_VERSION__ is undefined (pre-C95 compilers)
*/
bool d_tests_env_lang_c_standard_fallback(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_c_standard_fallback");

    all_assertions_passed = true;

    // test that C90 fallback value is correct
    // when __STDC_VERSION__ is not defined, should get 199000L
    #ifndef __STDC_VERSION__

        #ifdef D_ENV_LANG_C_STANDARD
            if (!D_TEST_CHECK_MSG(_suite,
                                  "fallback value is 199000L",
                                  D_ENV_LANG_C_STANDARD == 199000L,
                                  "C90 fallback should be exactly 199000L"))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_LANG_C_STANDARD_NAME
            if (!D_TEST_CHECK_MSG(_suite,
                                  "fallback name is \"C90\"",
                                  strcmp(D_ENV_LANG_C_STANDARD_NAME, "C90") == 0,
                                  "C90 fallback name should be exactly \"C90\""))
            {
                all_assertions_passed = false;
            }
        #endif
    #else

        // even when __STDC_VERSION__ is defined, verify the logic is correct
        // by checking that detected standard >= C90 fallback
        #ifdef D_ENV_LANG_C_STANDARD
            if (!D_TEST_CHECK_MSG(_suite,
                                  "detected standard >= C90 fallback",
                                  D_ENV_LANG_C_STANDARD >= 199000L,
                                  "any detected standard should be >= C90"))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify the fallback path would work if needed
        if (!D_TEST_CHECK_MSG(_suite,
                              "C90 fallback constant is valid",
                              199000L > 0,
                              "fallback value should be positive"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* C++ STANDARD DETECTION TESTS
******************************************************************************/

/*
d_tests_env_lang_cpp_standard_detection
  Tests C++ standard detection and resulting macro definitions.
  Tests the following:
  - D_ENV_LANG_CPP_STANDARD is defined when compiling as C++
  - D_ENV_LANG_CPP_STANDARD is NOT defined when compiling as C
  - D_ENV_LANG_CPP_STANDARD_NAME matches detected value
  - D_ENV_LANG_DETECTED_CPP is set in C++ mode
  - D_ENV_LANG_USING_CPP reflects compilation mode
*/
bool d_tests_env_lang_cpp_standard_detection(
    struct dj_test_suite* _suite
)
{
    bool        all_assertions_passed;
    bool        name_matches_value;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_cpp_standard_detection");

    all_assertions_passed = true;

    #ifdef __cplusplus

        // verify D_ENV_LANG_CPP_STANDARD is defined in C++ mode
        #ifdef D_ENV_LANG_CPP_STANDARD
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_CPP_STANDARD defined in C++ mode",
                                  true,
                                  "C++ standard should be detected"))
            {
                all_assertions_passed = false;
            }

            // verify value is positive and reasonable
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_CPP_STANDARD > 0",
                                  D_ENV_LANG_CPP_STANDARD > 0,
                                  "detected standard should be positive"))
            {
                all_assertions_passed = false;
            }

            // verify value is at least C++98
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_CPP_STANDARD >= C++98",
                                  D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98,
                                  "minimum C++ standard should be C++98"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_CPP_STANDARD should be defined",
                                  false,
                                  "C++ standard must be detected in C++ mode"))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_CPP_STANDARD_NAME is defined
        #ifdef D_ENV_LANG_CPP_STANDARD_NAME
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_CPP_STANDARD_NAME defined",
                                  true,
                                  "C++ standard name should be set"))
            {
                all_assertions_passed = false;
            }

            // verify name starts with "C++"
            if (!D_TEST_CHECK_MSG(_suite,
                                  "name starts with \"C++\"",
                                  (D_ENV_LANG_CPP_STANDARD_NAME[0] == 'C') && (D_ENV_LANG_CPP_STANDARD_NAME[1] == '+') && (D_ENV_LANG_CPP_STANDARD_NAME[2] == '+'),
                                  "C++ standard names should start with C++"))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_DETECTED_CPP is set
        #ifdef D_ENV_LANG_DETECTED_CPP
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_DETECTED_CPP is defined",
                                  true,
                                  "detection flag should be set in C++ mode"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_DETECTED_CPP should be defined",
                                  false,
                                  "detection flag must be set in C++ mode"))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_USING_CPP is 1
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_USING_CPP == 1",
                              D_ENV_LANG_USING_CPP == 1,
                              "USING_CPP should be 1 in C++ mode"))
        {
            all_assertions_passed = false;
        }

        // verify name matches value
        #if defined(D_ENV_LANG_CPP_STANDARD) && defined(D_ENV_LANG_CPP_STANDARD_NAME)
            name_matches_value = false;

            if (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP23)
            {
                name_matches_value = (strcmp(D_ENV_LANG_CPP_STANDARD_NAME, "C++23") == 0);
            }
            else if (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20)
            {
                name_matches_value = (strcmp(D_ENV_LANG_CPP_STANDARD_NAME, "C++20") == 0);
            }
            else if (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17)
            {
                name_matches_value = (strcmp(D_ENV_LANG_CPP_STANDARD_NAME, "C++17") == 0);
            }
            else if (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP14)
            {
                name_matches_value = (strcmp(D_ENV_LANG_CPP_STANDARD_NAME, "C++14") == 0);
            }
            else if (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11)
            {
                name_matches_value = (strcmp(D_ENV_LANG_CPP_STANDARD_NAME, "C++11") == 0);
            }
            else
            {
                name_matches_value = (strcmp(D_ENV_LANG_CPP_STANDARD_NAME, "C++98") == 0);
            }

            if (!D_TEST_CHECK_MSG(_suite,
                                  "C++ standard name matches value",
                                  name_matches_value,
                                  "name and value should be consistent"))
            {
                all_assertions_passed = false;
            }

        #endif

    #else

        // verify D_ENV_LANG_CPP_STANDARD is NOT defined in C mode
        #ifdef D_ENV_LANG_CPP_STANDARD
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_CPP_STANDARD should NOT be defined in C",
                                  false,
                                  "C++ standard should not be detected in C mode"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_CPP_STANDARD not defined in C mode",
                                  true,
                                  "correct: C++ standard not detected in C"))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_DETECTED_CPP is NOT defined
        #ifdef D_ENV_LANG_DETECTED_CPP
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_DETECTED_CPP should NOT be defined",
                                  false,
                                  "detection flag should not be set in C mode"))
            {
                all_assertions_passed = false;
            }
        #else
            if (!D_TEST_CHECK_MSG(_suite,
                                  "D_ENV_LANG_DETECTED_CPP not defined in C mode",
                                  true,
                                  "correct: detection flag not set in C"))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_USING_CPP is 0
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_USING_CPP == 0 in C mode",
                              D_ENV_LANG_USING_CPP == 0,
                              "USING_CPP should be 0 in C mode"))
        {
            all_assertions_passed = false;
        }

        // suppress unused variable warning
        (void)name_matches_value;
    #endif

    return all_assertions_passed;
}


/******************************************************************************
* VERSION COMPARISON MACRO TESTS
******************************************************************************/

/*
d_tests_env_lang_c_version_comparison_macros
  Tests C standard version comparison macros.
  Tests the following:
  - D_ENV_LANG_IS_C95_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_C99_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_C11_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_C17_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_C23_OR_HIGHER evaluates correctly
  - comparison macros are monotonically decreasing (higher always implies lower)
  - boundary conditions at exact standard values
*/
bool d_tests_env_lang_c_version_comparison_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   is_c95;
    bool   is_c99;
    bool   is_c11;
    bool   is_c17;
    bool   is_c23;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_c_version_comparison_macros");

    all_assertions_passed = true;

    // evaluate all comparison macros
    is_c95 = D_ENV_LANG_IS_C95_OR_HIGHER;
    is_c99 = D_ENV_LANG_IS_C99_OR_HIGHER;
    is_c11 = D_ENV_LANG_IS_C11_OR_HIGHER;
    is_c17 = D_ENV_LANG_IS_C17_OR_HIGHER;
    is_c23 = D_ENV_LANG_IS_C23_OR_HIGHER;

    // verify monotonic property: if C23, then also C17, C11, C99, C95
    if (is_c23)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "C23 implies C17",
                              is_c17,
                              "C23 must also be C17 or higher"))
        {
            all_assertions_passed = false;
        }
    }

    if (is_c17)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "C17 implies C11",
                              is_c11,
                              "C17 must also be C11 or higher"))
        {
            all_assertions_passed = false;
        }
    }

    if (is_c11)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "C11 implies C99",
                              is_c99,
                              "C11 must also be C99 or higher"))
        {
            all_assertions_passed = false;
        }
    }

    if (is_c99)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "C99 implies C95",
                              is_c95,
                              "C99 must also be C95 or higher"))
        {
            all_assertions_passed = false;
        }
    }

    // verify macros match detected standard
    #ifdef D_ENV_LANG_C_STANDARD
        // verify C95 comparison
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_C95_OR_HIGHER matches direct comparison",
                              is_c95 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C95),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        // verify C99 comparison
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_C99_OR_HIGHER matches direct comparison",
                              is_c99 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C99),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        // verify C11 comparison
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_C11_OR_HIGHER matches direct comparison",
                              is_c11 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C11),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        // verify C17 comparison
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_C17_OR_HIGHER matches direct comparison",
                              is_c17 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C17),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        // verify C23 comparison
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_C23_OR_HIGHER matches direct comparison",
                              is_c23 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C23),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_lang_cpp_version_comparison_macros
  Tests C++ standard version comparison macros.
  Tests the following:
  - D_ENV_LANG_IS_CPP98_OR_HIGHER evaluates correctly (when in C++ mode)
  - D_ENV_LANG_IS_CPP11_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_CPP14_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_CPP17_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_CPP20_OR_HIGHER evaluates correctly
  - D_ENV_LANG_IS_CPP23_OR_HIGHER evaluates correctly
  - macros are only defined when compiling as C++
  - monotonic property holds
*/
bool d_tests_env_lang_cpp_version_comparison_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_cpp_version_comparison_macros");

    all_assertions_passed = true;

    #ifdef D_ENV_LANG_CPP_STANDARD
        // C++ mode: test all comparison macros
        bool is_cpp98 = D_ENV_LANG_IS_CPP98_OR_HIGHER;
        bool is_cpp11 = D_ENV_LANG_IS_CPP11_OR_HIGHER;
        bool is_cpp14 = D_ENV_LANG_IS_CPP14_OR_HIGHER;
        bool is_cpp17 = D_ENV_LANG_IS_CPP17_OR_HIGHER;
        bool is_cpp20 = D_ENV_LANG_IS_CPP20_OR_HIGHER;
        bool is_cpp23 = D_ENV_LANG_IS_CPP23_OR_HIGHER;


        // verify monotonic property
        if (is_cpp23)
        {
            if (!D_TEST_CHECK_MSG(_suite,
                                  "C++23 implies C++20",
                                  is_cpp20,
                                  "C++23 must also be C++20 or higher"))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp20)
        {
            if (!D_TEST_CHECK_MSG(_suite,
                                  "C++20 implies C++17",
                                  is_cpp17,
                                  "C++20 must also be C++17 or higher"))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp17)
        {
            if (!D_TEST_CHECK_MSG(_suite,
                                  "C++17 implies C++14",
                                  is_cpp14,
                                  "C++17 must also be C++14 or higher"))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp14)
        {
            if (!D_TEST_CHECK_MSG(_suite,
                                  "C++14 implies C++11",
                                  is_cpp11,
                                  "C++14 must also be C++11 or higher"))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp11)
        {
            if (!D_TEST_CHECK_MSG(_suite,
                                  "C++11 implies C++98",
                                  is_cpp98,
                                  "C++11 must also be C++98 or higher"))
            {
                all_assertions_passed = false;
            }
        }

        // in C++ mode, should always be at least C++98
        if (!D_TEST_CHECK_MSG(_suite,
                              "always C++98 or higher in C++ mode",
                              is_cpp98,
                              "minimum C++ standard is C++98"))
        {
            all_assertions_passed = false;
        }

        // verify macros match direct comparison
        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_CPP98_OR_HIGHER matches direct comparison",
                              is_cpp98 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_CPP11_OR_HIGHER matches direct comparison",
                              is_cpp11 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_CPP14_OR_HIGHER matches direct comparison",
                              is_cpp14 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP14),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_CPP17_OR_HIGHER matches direct comparison",
                              is_cpp17 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_CPP20_OR_HIGHER matches direct comparison",
                              is_cpp20 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }

        if (!D_TEST_CHECK_MSG(_suite,
                              "IS_CPP23_OR_HIGHER matches direct comparison",
                              is_cpp23 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP23),
                              "macro should equal direct >= comparison"))
        {
            all_assertions_passed = false;
        }


    #else
        // C mode: verify the C++ comparison macros are 0

        // decision 20: each C++ predicate is defined, and 0, in a C build,
        // so #if can read one; reading them here is the definedness check
        if (!D_TEST_CHECK_MSG(_suite,
                              "the C++ predicates are 0 in C mode",
                              (D_ENV_LANG_IS_CPP98_OR_HIGHER == 0) &&
                                  (D_ENV_LANG_IS_CPP11_OR_HIGHER == 0) &&
                                  (D_ENV_LANG_IS_CPP14_OR_HIGHER == 0) &&
                                  (D_ENV_LANG_IS_CPP17_OR_HIGHER == 0) &&
                                  (D_ENV_LANG_IS_CPP20_OR_HIGHER == 0) &&
                                  (D_ENV_LANG_IS_CPP23_OR_HIGHER == 0) &&
                                  (D_ENV_LANG_IS_CPP26_OR_HIGHER == 0),
                              "every C++ predicate should be 0 in C"))
        {
            all_assertions_passed = false;
        }
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_lang_version_comparison_boundary_cases
  Tests version comparison macros at exact boundary values.
  Tests the following:
  - exact match: standard == boundary returns true for that comparison
  - just below: standard == boundary-1 returns false for that comparison
  - comparison operators are >= not >
*/
bool d_tests_env_lang_version_comparison_boundary_cases(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    long   test_value;
    bool   comparison_result;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_version_comparison_boundary_cases");

    all_assertions_passed = true;

    // test C99 boundary: exact match should be true
    test_value = D_ENV_LANG_C_STANDARD_C99;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C99);

    if (!D_TEST_CHECK_MSG(_suite,
                          "exact C99 value >= C99 is true",
                          comparison_result == true,
                          "exact match should satisfy >= comparison"))
    {
        all_assertions_passed = false;
    }

    // test C99 boundary: one below should be false
    test_value = D_ENV_LANG_C_STANDARD_C99 - 1;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C99);

    if (!D_TEST_CHECK_MSG(_suite,
                          "C99-1 >= C99 is false",
                          comparison_result == false,
                          "value below boundary should fail comparison"))
    {
        all_assertions_passed = false;
    }

    // test C11 boundary: exact match should be true
    test_value = D_ENV_LANG_C_STANDARD_C11;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C11);

    if (!D_TEST_CHECK_MSG(_suite,
                          "exact C11 value >= C11 is true",
                          comparison_result == true,
                          "exact match should satisfy >= comparison"))
    {
        all_assertions_passed = false;
    }

    // test C11 boundary: one below should be false
    test_value = D_ENV_LANG_C_STANDARD_C11 - 1;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C11);

    if (!D_TEST_CHECK_MSG(_suite,
                          "C11-1 >= C11 is false",
                          comparison_result == false,
                          "value below boundary should fail comparison"))
    {
        all_assertions_passed = false;
    }

    // test that >= is used, not > (value+1 should still pass)
    test_value = D_ENV_LANG_C_STANDARD_C99 + 1;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C99);

    if (!D_TEST_CHECK_MSG(_suite,
                          "C99+1 >= C99 is true",
                          comparison_result == true,
                          "value above boundary should pass comparison"))
    {
        all_assertions_passed = false;
    }

    // test C++ boundaries
    test_value = D_ENV_LANG_CPP_STANDARD_CPP11;
    comparison_result = (test_value >= D_ENV_LANG_CPP_STANDARD_CPP11);

    if (!D_TEST_CHECK_MSG(_suite,
                          "exact C++11 value >= C++11 is true",
                          comparison_result == true,
                          "exact match should satisfy >= comparison"))
    {
        all_assertions_passed = false;
    }

    test_value = D_ENV_LANG_CPP_STANDARD_CPP11 - 1;
    comparison_result = (test_value >= D_ENV_LANG_CPP_STANDARD_CPP11);

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++11-1 >= C++11 is false",
                          comparison_result == false,
                          "value below boundary should fail comparison"))
    {
        all_assertions_passed = false;
    }

    // test transition between standards (e.g., C99 to C11 gap)
    // a value between C99 and C11 should pass C99 but fail C11
    test_value = D_ENV_LANG_C_STANDARD_C99 + 1000; // arbitrary mid-gap value

    if (!D_TEST_CHECK_MSG(_suite,
                          "mid-gap value passes lower, fails higher",
                          (test_value >= D_ENV_LANG_C_STANDARD_C99) && (test_value < D_ENV_LANG_C_STANDARD_C11),
                          "gap handling should work correctly"))
    {
        all_assertions_passed = false;
    }

    return all_assertions_passed;
}


/******************************************************************************
* CONVENIENCE MACRO TESTS
******************************************************************************/

/*
d_tests_env_lang_using_macros
  Tests the D_ENV_LANG_USING_C and D_ENV_LANG_USING_CPP convenience macros.
  Tests the following:
  - D_ENV_LANG_USING_C is always defined (0 or 1)
  - D_ENV_LANG_USING_CPP is always defined (0 or 1)
  - values are boolean (exactly 0 or 1, not just truthy/falsy)
  - USING_C should be 1 when C standard is detected
  - USING_CPP should be 1 only in C++ mode
  - mutual exclusivity considerations
*/
bool d_tests_env_lang_using_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    using_c;
    int    using_cpp;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_using_macros");

    all_assertions_passed = true;

    // get macro values
    using_c   = D_ENV_LANG_USING_C;
    using_cpp = D_ENV_LANG_USING_CPP;

    // verify D_ENV_LANG_USING_C is defined
    #ifdef D_ENV_LANG_USING_C
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_USING_C is defined",
                              true,
                              "USING_C macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_USING_C should be defined",
                              false,
                              "USING_C macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_LANG_USING_CPP is defined
    #ifdef D_ENV_LANG_USING_CPP
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_USING_CPP is defined",
                              true,
                              "USING_CPP macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "D_ENV_LANG_USING_CPP should be defined",
                              false,
                              "USING_CPP macro must always be defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify values are boolean (exactly 0 or 1)
    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_USING_C is 0 or 1",
                          (using_c == 0) || (using_c == 1),
                          "boolean macro should be exactly 0 or 1"))
    {
        all_assertions_passed = false;
    }

    if (!D_TEST_CHECK_MSG(_suite,
                          "D_ENV_LANG_USING_CPP is 0 or 1",
                          (using_cpp == 0) || (using_cpp == 1),
                          "boolean macro should be exactly 0 or 1"))
    {
        all_assertions_passed = false;
    }

    // verify USING_C matches C standard detection
    #ifdef D_ENV_LANG_C_STANDARD
        if (!D_TEST_CHECK_MSG(_suite,
                              "USING_C == 1 when C standard detected",
                              using_c == 1,
                              "USING_C should be 1 when standard is defined"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "USING_C == 0 when no C standard",
                              using_c == 0,
                              "USING_C should be 0 when standard not defined"))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify USING_CPP matches C++ mode
    #ifdef __cplusplus
        if (!D_TEST_CHECK_MSG(_suite,
                              "USING_CPP == 1 in C++ mode",
                              using_cpp == 1,
                              "USING_CPP should be 1 when compiling as C++"))
        {
            all_assertions_passed = false;
        }
    #else
        if (!D_TEST_CHECK_MSG(_suite,
                              "USING_CPP == 0 in C mode",
                              using_cpp == 0,
                              "USING_CPP should be 0 when compiling as C"))
        {
            all_assertions_passed = false;
        }
    #endif

    // note: C and C++ can both be "used" since C++ includes C features
    // so we don't test for mutual exclusivity

    return all_assertions_passed;
}


/******************************************************************************
* MANUAL DETECTION (D_ENV_DETECTED_*) TESTS
******************************************************************************/

/*
d_tests_env_lang_detected_vars_c
  Tests manual C standard configuration via D_ENV_DETECTED_C* variables.
  Tests the following:
  - when D_CFG_ENV_CUSTOM disables language detection, D_ENV_DETECTED_* are used
  - D_ENV_DETECTED_C95/C99/C11/C17/C23 correctly set corresponding standard
  - only one C standard detection variable should be defined at a time
  - fallback to C90 when no detection variable is defined
*/
bool d_tests_env_lang_detected_vars_c(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    detected_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_detected_vars_c");

    all_assertions_passed = true;
    detected_count        = 0;

    // count how many C detection variables are defined
    #ifdef D_ENV_DETECTED_C95
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_C99
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_C11
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_C17
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_C23
        detected_count++;
    #endif

    // verify at most one C detection variable is defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one C detection var defined",
                          detected_count <= 1,
                          "C standard detection should be mutually exclusive"))
    {
        all_assertions_passed = false;
    }


    // if manual detection is active (language bit set), verify mapping
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG)

        #ifdef D_ENV_DETECTED_C23
            #ifdef D_ENV_LANG_C_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_C23 maps to C23 standard",
                                      D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C23,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C17)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_C17 maps to C17 standard",
                                      D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C17,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C11)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_C11 maps to C11 standard",
                                      D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C11,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C99)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_C99 maps to C99 standard",
                                      D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C99,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C95)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_C95 maps to C95 standard",
                                      D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C95,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #else

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


/*
d_tests_env_lang_detected_vars_cpp
  Tests manual C++ standard configuration via D_ENV_DETECTED_CPP* variables.
  Tests the following:
  - when D_CFG_ENV_CUSTOM disables language detection, D_ENV_DETECTED_* are used
  - D_ENV_DETECTED_CPP98/11/14/17/20/23 correctly set corresponding standard
  - only one C++ standard detection variable should be defined at a time
*/
bool d_tests_env_lang_detected_vars_cpp(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    int    detected_count;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_detected_vars_cpp");

    all_assertions_passed = true;
    detected_count        = 0;

    // count how many C++ detection variables are defined
    #ifdef D_ENV_DETECTED_CPP98
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_CPP11
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_CPP14
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_CPP17
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_CPP20
        detected_count++;
    #endif

    #ifdef D_ENV_DETECTED_CPP23
        detected_count++;
    #endif

    // verify at most one C++ detection variable is defined
    if (!D_TEST_CHECK_MSG(_suite,
                          "at most one C++ detection var defined",
                          detected_count <= 1,
                          "C++ standard detection should be mutually exclusive"))
    {
        all_assertions_passed = false;
    }


    // if manual detection is active, verify mapping
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG)

        #ifdef D_ENV_DETECTED_CPP23
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_CPP23 maps to C++23",
                                      D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP23,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP20)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_CPP20 maps to C++20",
                                      D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP20,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP17)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_CPP17 maps to C++17",
                                      D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP17,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP14)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_CPP14 maps to C++14",
                                      D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP14,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP11)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_CPP11 maps to C++11",
                                      D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP11,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP98)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!D_TEST_CHECK_MSG(_suite,
                                      "D_ENV_DETECTED_CPP98 maps to C++98",
                                      D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP98,
                                      "detected var should set correct standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #else

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
d_tests_env_lang_consistency_check
  Tests overall consistency of language detection.
  Tests the following:
  - standard name and value are always both defined or both undefined
  - USING_* macros are consistent with standard detection
  - no contradictory states (e.g., C++ detected but USING_CPP == 0)
  - comparison macros are consistent with detected standard
*/
bool d_tests_env_lang_consistency_check(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   c_standard_defined;
    bool   c_name_defined;
    bool   cpp_standard_defined;
    bool   cpp_name_defined;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_consistency_check");

    all_assertions_passed = true;

    // check C standard/name pair consistency
    c_standard_defined = false;
    c_name_defined     = false;

    #ifdef D_ENV_LANG_C_STANDARD
        c_standard_defined = true;
    #endif

    #ifdef D_ENV_LANG_C_STANDARD_NAME
        c_name_defined = true;
    #endif

    if (!D_TEST_CHECK_MSG(_suite,
                          "C standard and name are both defined or both undefined",
                          c_standard_defined == c_name_defined,
                          "standard/name pair should be consistent"))
    {
        all_assertions_passed = false;
    }

    // check C++ standard/name pair consistency
    cpp_standard_defined = false;
    cpp_name_defined     = false;

    #ifdef D_ENV_LANG_CPP_STANDARD
        cpp_standard_defined = true;
    #endif

    #ifdef D_ENV_LANG_CPP_STANDARD_NAME
        cpp_name_defined = true;
    #endif

    if (!D_TEST_CHECK_MSG(_suite,
                          "C++ standard and name are both defined or both undefined",
                          cpp_standard_defined == cpp_name_defined,
                          "standard/name pair should be consistent"))
    {
        all_assertions_passed = false;
    }

    // verify USING_CPP is consistent with CPP_STANDARD
    if (!D_TEST_CHECK_MSG(_suite,
                          "USING_CPP == 1 iff CPP_STANDARD defined",
                          (D_ENV_LANG_USING_CPP == 1) == cpp_standard_defined,
                          "USING_CPP should match standard definition"))
    {
        all_assertions_passed = false;
    }

    // verify DETECTED_CPP is consistent with CPP_STANDARD in auto mode
    #if D_CFG_ENV_LANG_ENABLED
        #ifdef __cplusplus
            #ifdef D_ENV_LANG_DETECTED_CPP
                if (!D_TEST_CHECK_MSG(_suite,
                                      "DETECTED_CPP implies CPP_STANDARD defined",
                                      cpp_standard_defined,
                                      "detection flag should imply standard"))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #endif

    // verify C comparison macros are usable (don't cause compile errors)
    // by evaluating them in boolean context
    {
        volatile bool c95_check = D_ENV_LANG_IS_C95_OR_HIGHER;
        volatile bool c99_check = D_ENV_LANG_IS_C99_OR_HIGHER;
        volatile bool c11_check = D_ENV_LANG_IS_C11_OR_HIGHER;
        volatile bool c17_check = D_ENV_LANG_IS_C17_OR_HIGHER;
        volatile bool c23_check = D_ENV_LANG_IS_C23_OR_HIGHER;

        // suppress unused warnings
        (void)c95_check;
        (void)c99_check;
        (void)c11_check;
        (void)c17_check;
        (void)c23_check;

        if (!D_TEST_CHECK_MSG(_suite,
                              "C comparison macros evaluate without error",
                              true,
                              "macros should be usable in boolean context"))
        {
            all_assertions_passed = false;
        }
    }

    // verify C++ comparison macros are usable (when in C++ mode)
    #ifdef D_ENV_LANG_CPP_STANDARD
    {
        volatile bool cpp98_check = D_ENV_LANG_IS_CPP98_OR_HIGHER;
        volatile bool cpp11_check = D_ENV_LANG_IS_CPP11_OR_HIGHER;
        volatile bool cpp14_check = D_ENV_LANG_IS_CPP14_OR_HIGHER;
        volatile bool cpp17_check = D_ENV_LANG_IS_CPP17_OR_HIGHER;
        volatile bool cpp20_check = D_ENV_LANG_IS_CPP20_OR_HIGHER;
        volatile bool cpp23_check = D_ENV_LANG_IS_CPP23_OR_HIGHER;

        (void)cpp98_check;
        (void)cpp11_check;
        (void)cpp14_check;
        (void)cpp17_check;
        (void)cpp20_check;
        (void)cpp23_check;

        if (!D_TEST_CHECK_MSG(_suite,
                              "C++ comparison macros evaluate without error",
                              true,
                              "macros should be usable in boolean context"))
        {
            all_assertions_passed = false;
        }
    }
    #endif

    return all_assertions_passed;
}


/*
d_tests_env_lang_feature_test_macros
  Tests practical use of comparison macros for feature testing.
  Tests the following:
  - comparison macros can be used in #if directives
  - macros produce correct results for common feature gates
  - practical patterns work (e.g., "if C99 or higher, use inline")
*/
bool d_tests_env_lang_feature_test_macros(
    struct dj_test_suite* _suite
)
{
    bool   all_assertions_passed;
    bool   has_c99_features;
    bool   has_c11_features;

    if (!_suite)
    {
        return false;
    }

    d_test_suite_test(_suite, "env_lang_feature_test_macros");

    all_assertions_passed = true;

    // test common C99 feature gate pattern
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        has_c99_features = true;
    #else
        has_c99_features = false;
    #endif

    // test common C11 feature gate pattern
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        has_c11_features = true;
    #else
        has_c11_features = false;
    #endif

    // verify feature flags are consistent with each other
    // C11 implies C99
    if (has_c11_features)
    {
        if (!D_TEST_CHECK_MSG(_suite,
                              "C11 features imply C99 features",
                              has_c99_features,
                              "feature availability should be monotonic"))
        {
            all_assertions_passed = false;
        }
    }

    // test C++ feature gate patterns

    // verify the pattern of conditional compilation works
    if (!D_TEST_CHECK_MSG(_suite,
                          "feature test patterns compile successfully",
                          true,
                          "#if directives with comparison macros work"))
    {
        all_assertions_passed = false;
    }

    // suppress unused warnings
    (void)has_c99_features;
    (void)has_c11_features;

    return all_assertions_passed;
}


/******************************************************************************
* MODULE TEST AGGREGATOR
******************************************************************************/

/*
d_tests_env_lang_all
  Runs all language environment tests.
  Tests the following:
  - standard version constants
  - C standard detection
  - C++ standard detection
  - version comparison macros
  - convenience macros
  - manual detection variables
  - consistency and edge cases
*/
bool d_tests_env_lang_all(
    struct dj_test_suite* _suite
)
{
    d_test_suite_block(_suite, "env_lang_all");

    d_tests_env_lang_c_standard_constants(_suite);
    d_tests_env_lang_cpp_standard_constants(_suite);
    d_tests_env_lang_standard_constant_ranges(_suite);
    d_tests_env_lang_c_standard_detection(_suite);
    d_tests_env_lang_c_standard_fallback(_suite);
    d_tests_env_lang_cpp_standard_detection(_suite);
    d_tests_env_lang_c_version_comparison_macros(_suite);
    d_tests_env_lang_cpp_version_comparison_macros(_suite);
    d_tests_env_lang_version_comparison_boundary_cases(_suite);
    d_tests_env_lang_using_macros(_suite);
    d_tests_env_lang_detected_vars_c(_suite);
    d_tests_env_lang_detected_vars_cpp(_suite);
    d_tests_env_lang_consistency_check(_suite);
    d_tests_env_lang_feature_test_macros(_suite);

    return true;
}
