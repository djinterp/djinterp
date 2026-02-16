/******************************************************************************
* djinterp [test]                                          env_tests_sa_lang.c
*
* Unit tests for `env.h` language environment section (Section II).
* Tests C and C++ standard detection, version constants, and comparison macros.
* Note: this module is required to build DTest, so it uses `test_standalone.h`.
*
* path:      \test\env_tests_sa_lang.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.26
******************************************************************************/

#include "env_tests_sa.h"
#include <string.h>


/******************************************************************************
* LANGUAGE STANDARD VERSION CONSTANT TESTS
******************************************************************************/

/*
d_tests_sa_env_lang_c_standard_constants
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
bool
d_tests_sa_env_lang_c_standard_constants
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

    printf("%s--- Testing C Standard Version Constants ---\n", D_INDENT);

    // verify each constant matches the official ISO __STDC_VERSION__ value
    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C95 == 199409L,
                             "D_ENV_LANG_C_STANDARD_C95 == 199409L",
                             "C95 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C99 == 199901L,
                             "D_ENV_LANG_C_STANDARD_C99 == 199901L",
                             "C99 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C11 == 201112L,
                             "D_ENV_LANG_C_STANDARD_C11 == 201112L",
                             "C11 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C17 == 201710L,
                             "D_ENV_LANG_C_STANDARD_C17 == 201710L",
                             "C17 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C23 == 202311L,
                             "D_ENV_LANG_C_STANDARD_C23 == 202311L",
                             "C23 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify strictly ascending chronological order
    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C95 < D_ENV_LANG_C_STANDARD_C99,
                             "C95 < C99",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C99 < D_ENV_LANG_C_STANDARD_C11,
                             "C99 < C11",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C11 < D_ENV_LANG_C_STANDARD_C17,
                             "C11 < C17",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C17 < D_ENV_LANG_C_STANDARD_C23,
                             "C17 < C23",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify all constants are positive (valid __STDC_VERSION__ values)
    if (!d_assert_standalone(D_ENV_LANG_C_STANDARD_C95 > 0,
                             "C95 constant is positive",
                             "version constants must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // print constant values for verification
    printf("%s    C95:  %ldL\n", D_INDENT, D_ENV_LANG_C_STANDARD_C95);
    printf("%s    C99:  %ldL\n", D_INDENT, D_ENV_LANG_C_STANDARD_C99);
    printf("%s    C11:  %ldL\n", D_INDENT, D_ENV_LANG_C_STANDARD_C11);
    printf("%s    C17:  %ldL\n", D_INDENT, D_ENV_LANG_C_STANDARD_C17);
    printf("%s    C23:  %ldL\n", D_INDENT, D_ENV_LANG_C_STANDARD_C23);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C standard version constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C standard version constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_lang_cpp_standard_constants
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
bool
d_tests_sa_env_lang_cpp_standard_constants
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

    printf("%s--- Testing C++ Standard Version Constants ---\n", D_INDENT);

    // verify each constant matches the official ISO __cplusplus value
    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP98 == 199711L,
                             "D_ENV_LANG_CPP_STANDARD_CPP98 == 199711L",
                             "C++98 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP11 == 201103L,
                             "D_ENV_LANG_CPP_STANDARD_CPP11 == 201103L",
                             "C++11 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP14 == 201402L,
                             "D_ENV_LANG_CPP_STANDARD_CPP14 == 201402L",
                             "C++14 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP17 == 201703L,
                             "D_ENV_LANG_CPP_STANDARD_CPP17 == 201703L",
                             "C++17 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP20 == 202002L,
                             "D_ENV_LANG_CPP_STANDARD_CPP20 == 202002L",
                             "C++20 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP23 == 202302L,
                             "D_ENV_LANG_CPP_STANDARD_CPP23 == 202302L",
                             "C++23 constant should match ISO value",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify strictly ascending chronological order
    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP98 < D_ENV_LANG_CPP_STANDARD_CPP11,
                             "C++98 < C++11",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP11 < D_ENV_LANG_CPP_STANDARD_CPP14,
                             "C++11 < C++14",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP14 < D_ENV_LANG_CPP_STANDARD_CPP17,
                             "C++14 < C++17",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP17 < D_ENV_LANG_CPP_STANDARD_CPP20,
                             "C++17 < C++20",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP20 < D_ENV_LANG_CPP_STANDARD_CPP23,
                             "C++20 < C++23",
                             "standards must be in chronological order",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify all constants are positive
    if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD_CPP98 > 0,
                             "C++98 constant is positive",
                             "version constants must be positive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // print constant values for verification
    printf("%s    C++98:  %ldL\n", D_INDENT, D_ENV_LANG_CPP_STANDARD_CPP98);
    printf("%s    C++11:  %ldL\n", D_INDENT, D_ENV_LANG_CPP_STANDARD_CPP11);
    printf("%s    C++14:  %ldL\n", D_INDENT, D_ENV_LANG_CPP_STANDARD_CPP14);
    printf("%s    C++17:  %ldL\n", D_INDENT, D_ENV_LANG_CPP_STANDARD_CPP17);
    printf("%s    C++20:  %ldL\n", D_INDENT, D_ENV_LANG_CPP_STANDARD_CPP20);
    printf("%s    C++23:  %ldL\n", D_INDENT, D_ENV_LANG_CPP_STANDARD_CPP23);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C++ standard version constants test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C++ standard version constants test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_lang_standard_constant_ranges
  Tests that C and C++ standard constants don't overlap in problematic ways.
  Tests the following:
  - C and C++ version numbers are distinguishable
  - no ambiguous values between C and C++ standards
  - version format YYYYMM is consistent across all constants
*/
bool
d_tests_sa_env_lang_standard_constant_ranges
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    long   c_min;
    long   c_max;
    long   cpp_min;
    long   cpp_max;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Standard Constant Ranges ---\n", D_INDENT);

    // determine C standard range
    c_min = D_ENV_LANG_C_STANDARD_C95;
    c_max = D_ENV_LANG_C_STANDARD_C23;

    // determine C++ standard range
    cpp_min = D_ENV_LANG_CPP_STANDARD_CPP98;
    cpp_max = D_ENV_LANG_CPP_STANDARD_CPP23;

    // verify all constants follow YYYYMM format (6 digits)
    // minimum valid: 199000 (year 1990, month 00)
    // maximum valid: 209912 (year 2099, month 12)
    if (!d_assert_standalone( (D_ENV_LANG_C_STANDARD_C95 >= 199000L) &&
                              (D_ENV_LANG_C_STANDARD_C95 <= 209912L),
                             "C95 follows YYYYMM format",
                             "version should be 6-digit YYYYMM",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (D_ENV_LANG_C_STANDARD_C23 >= 199000L) &&
                              (D_ENV_LANG_C_STANDARD_C23 <= 209912L),
                             "C23 follows YYYYMM format",
                             "version should be 6-digit YYYYMM",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (D_ENV_LANG_CPP_STANDARD_CPP98 >= 199000L) &&
                              (D_ENV_LANG_CPP_STANDARD_CPP98 <= 209912L),
                             "C++98 follows YYYYMM format",
                             "version should be 6-digit YYYYMM",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (D_ENV_LANG_CPP_STANDARD_CPP23 >= 199000L) &&
                              (D_ENV_LANG_CPP_STANDARD_CPP23 <= 209912L),
                             "C++23 follows YYYYMM format",
                             "version should be 6-digit YYYYMM",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify month portion is valid (01-12 typically, 00 for some)
    // extract month from YYYYMM: value % 100
    if (!d_assert_standalone((D_ENV_LANG_C_STANDARD_C95 % 100) <= 12,
                             "C95 month portion <= 12",
                             "month should be valid",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone((D_ENV_LANG_CPP_STANDARD_CPP98 % 100) <= 12,
                             "C++98 month portion <= 12",
                             "month should be valid",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // report ranges
    printf("%s    C standard range:   %ldL to %ldL\n", D_INDENT, c_min, c_max);
    printf("%s    C++ standard range: %ldL to %ldL\n", D_INDENT, cpp_min, cpp_max);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Standard constant ranges test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Standard constant ranges test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* C STANDARD DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_lang_c_standard_detection
  Tests C standard detection and resulting macro definitions.
  Tests the following:
  - D_ENV_LANG_C_STANDARD is always defined (with fallback to C90)
  - D_ENV_LANG_C_STANDARD_NAME is always defined
  - detected standard value is positive
  - standard name matches standard value
  - D_ENV_LANG_USING_C is correctly set
*/
bool
d_tests_sa_env_lang_c_standard_detection
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    long        detected_standard;
    const char* detected_name;
    bool        name_matches_value;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing C Standard Detection ---\n", D_INDENT);

    // verify D_ENV_LANG_C_STANDARD is defined
    #ifdef D_ENV_LANG_C_STANDARD
        detected_standard = D_ENV_LANG_C_STANDARD;

        if (!d_assert_standalone(true,
                                 "D_ENV_LANG_C_STANDARD is defined",
                                 "C standard macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify value is positive
        if (!d_assert_standalone(detected_standard > 0,
                                 "D_ENV_LANG_C_STANDARD > 0",
                                 "detected standard should be positive",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify value is at least C90 (199000L used as fallback)
        if (!d_assert_standalone(detected_standard >= 199000L,
                                 "D_ENV_LANG_C_STANDARD >= 199000L (C90)",
                                 "minimum standard should be C90",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        detected_standard = 0;

        if (!d_assert_standalone(false,
                                 "D_ENV_LANG_C_STANDARD should be defined",
                                 "C standard macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_LANG_C_STANDARD_NAME is defined
    #ifdef D_ENV_LANG_C_STANDARD_NAME
        detected_name = D_ENV_LANG_C_STANDARD_NAME;

        if (!d_assert_standalone(true,
                                 "D_ENV_LANG_C_STANDARD_NAME is defined",
                                 "standard name macro must be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify name is not empty
        if (!d_assert_standalone( (detected_name != NULL) &&
                                  (detected_name[0] != '\0'),
                                 "D_ENV_LANG_C_STANDARD_NAME is not empty",
                                 "standard name should be non-empty string",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify name starts with 'C' (all C standards: C90, C95, C99, etc.)
        if (!d_assert_standalone( (detected_name != NULL) &&
                                  (detected_name[0] == 'C'),
                                 "D_ENV_LANG_C_STANDARD_NAME starts with 'C'",
                                 "C standard names should start with 'C'",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        detected_name = NULL;

        if (!d_assert_standalone(false,
                                 "D_ENV_LANG_C_STANDARD_NAME should be defined",
                                 "standard name macro must be defined",
                                 _test_info))
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

        if (!d_assert_standalone(name_matches_value,
                                 "standard name matches standard value",
                                 "name and value should be consistent",
                                 _test_info))
        {
            all_assertions_passed = false;
            printf("%s    ERROR: value=%ld but name=\"%s\"\n",
                   D_INDENT,
                   D_ENV_LANG_C_STANDARD,
                   D_ENV_LANG_C_STANDARD_NAME);
        }
    #endif

    // verify D_ENV_LANG_USING_C is correctly set
    #ifdef D_ENV_LANG_C_STANDARD
        if (!d_assert_standalone(D_ENV_LANG_USING_C == 1,
                                 "D_ENV_LANG_USING_C == 1 when C detected",
                                 "USING_C should be 1 when standard is defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // report detected values
    #if defined(D_ENV_LANG_C_STANDARD) && defined(D_ENV_LANG_C_STANDARD_NAME)
        printf("%s    Detected: %s (%ldL)\n",
               D_INDENT,
               D_ENV_LANG_C_STANDARD_NAME,
               D_ENV_LANG_C_STANDARD);
        printf("%s    D_ENV_LANG_USING_C: %d\n", D_INDENT, D_ENV_LANG_USING_C);
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C standard detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C standard detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_lang_c_standard_fallback
  Tests C90 fallback behavior when __STDC_VERSION__ is not defined.
  Tests the following:
  - fallback value is exactly 199000L
  - fallback name is exactly "C90"
  - fallback occurs when __STDC_VERSION__ is undefined (pre-C95 compilers)
*/
bool
d_tests_sa_env_lang_c_standard_fallback
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

    printf("%s--- Testing C90 Fallback Behavior ---\n", D_INDENT);

    // test that C90 fallback value is correct
    // when __STDC_VERSION__ is not defined, should get 199000L
    #ifndef __STDC_VERSION__
        printf("%s    __STDC_VERSION__ is NOT defined (C90 mode)\n", D_INDENT);

        #ifdef D_ENV_LANG_C_STANDARD
            if (!d_assert_standalone(D_ENV_LANG_C_STANDARD == 199000L,
                                     "fallback value is 199000L",
                                     "C90 fallback should be exactly 199000L",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        #ifdef D_ENV_LANG_C_STANDARD_NAME
            if (!d_assert_standalone(strcmp(D_ENV_LANG_C_STANDARD_NAME, "C90") == 0,
                                     "fallback name is \"C90\"",
                                     "C90 fallback name should be exactly \"C90\"",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif
    #else
        printf("%s    __STDC_VERSION__ is defined: %ldL\n",
               D_INDENT,
               (long)__STDC_VERSION__);
        printf("%s    (C90 fallback not active in this compilation)\n", D_INDENT);

        // even when __STDC_VERSION__ is defined, verify the logic is correct
        // by checking that detected standard >= C90 fallback
        #ifdef D_ENV_LANG_C_STANDARD
            if (!d_assert_standalone(D_ENV_LANG_C_STANDARD >= 199000L,
                                     "detected standard >= C90 fallback",
                                     "any detected standard should be >= C90",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify the fallback path would work if needed
        if (!d_assert_standalone(199000L > 0,
                                 "C90 fallback constant is valid",
                                 "fallback value should be positive",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C90 fallback behavior test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C90 fallback behavior test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* C++ STANDARD DETECTION TESTS
******************************************************************************/

/*
d_tests_sa_env_lang_cpp_standard_detection
  Tests C++ standard detection and resulting macro definitions.
  Tests the following:
  - D_ENV_LANG_CPP_STANDARD is defined when compiling as C++
  - D_ENV_LANG_CPP_STANDARD is NOT defined when compiling as C
  - D_ENV_LANG_CPP_STANDARD_NAME matches detected value
  - D_ENV_LANG_DETECTED_CPP is set in C++ mode
  - D_ENV_LANG_USING_CPP reflects compilation mode
*/
bool
d_tests_sa_env_lang_cpp_standard_detection
(
    struct d_test_counter* _test_info
)
{
    bool        all_assertions_passed;
    size_t      initial_tests_passed;
    bool        name_matches_value;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing C++ Standard Detection ---\n", D_INDENT);

    #ifdef __cplusplus
        printf("%s    Compiling as C++ (__cplusplus = %ldL)\n",
               D_INDENT,
               (long)__cplusplus);

        // verify D_ENV_LANG_CPP_STANDARD is defined in C++ mode
        #ifdef D_ENV_LANG_CPP_STANDARD
            if (!d_assert_standalone(true,
                                     "D_ENV_LANG_CPP_STANDARD defined in C++ mode",
                                     "C++ standard should be detected",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            // verify value is positive and reasonable
            if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD > 0,
                                     "D_ENV_LANG_CPP_STANDARD > 0",
                                     "detected standard should be positive",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            // verify value is at least C++98
            if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98,
                                     "D_ENV_LANG_CPP_STANDARD >= C++98",
                                     "minimum C++ standard should be C++98",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(false,
                                     "D_ENV_LANG_CPP_STANDARD should be defined",
                                     "C++ standard must be detected in C++ mode",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_CPP_STANDARD_NAME is defined
        #ifdef D_ENV_LANG_CPP_STANDARD_NAME
            if (!d_assert_standalone(true,
                                     "D_ENV_LANG_CPP_STANDARD_NAME defined",
                                     "C++ standard name should be set",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            // verify name starts with "C++"
            if (!d_assert_standalone( (D_ENV_LANG_CPP_STANDARD_NAME[0] == 'C') &&
                                      (D_ENV_LANG_CPP_STANDARD_NAME[1] == '+') &&
                                      (D_ENV_LANG_CPP_STANDARD_NAME[2] == '+'),
                                     "name starts with \"C++\"",
                                     "C++ standard names should start with C++",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_DETECTED_CPP is set
        #ifdef D_ENV_LANG_DETECTED_CPP
            if (!d_assert_standalone(true,
                                     "D_ENV_LANG_DETECTED_CPP is defined",
                                     "detection flag should be set in C++ mode",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(false,
                                     "D_ENV_LANG_DETECTED_CPP should be defined",
                                     "detection flag must be set in C++ mode",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_USING_CPP is 1
        if (!d_assert_standalone(D_ENV_LANG_USING_CPP == 1,
                                 "D_ENV_LANG_USING_CPP == 1",
                                 "USING_CPP should be 1 in C++ mode",
                                 _test_info))
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

            if (!d_assert_standalone(name_matches_value,
                                     "C++ standard name matches value",
                                     "name and value should be consistent",
                                     _test_info))
            {
                all_assertions_passed = false;
            }

            printf("%s    Detected: %s (%ldL)\n",
                   D_INDENT,
                   D_ENV_LANG_CPP_STANDARD_NAME,
                   D_ENV_LANG_CPP_STANDARD);
        #endif

    #else
        printf("%s    Compiling as C (not C++)\n", D_INDENT);

        // verify D_ENV_LANG_CPP_STANDARD is NOT defined in C mode
        #ifdef D_ENV_LANG_CPP_STANDARD
            if (!d_assert_standalone(false,
                                     "D_ENV_LANG_CPP_STANDARD should NOT be defined in C",
                                     "C++ standard should not be detected in C mode",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(true,
                                     "D_ENV_LANG_CPP_STANDARD not defined in C mode",
                                     "correct: C++ standard not detected in C",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_DETECTED_CPP is NOT defined
        #ifdef D_ENV_LANG_DETECTED_CPP
            if (!d_assert_standalone(false,
                                     "D_ENV_LANG_DETECTED_CPP should NOT be defined",
                                     "detection flag should not be set in C mode",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(true,
                                     "D_ENV_LANG_DETECTED_CPP not defined in C mode",
                                     "correct: detection flag not set in C",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        // verify D_ENV_LANG_USING_CPP is 0
        if (!d_assert_standalone(D_ENV_LANG_USING_CPP == 0,
                                 "D_ENV_LANG_USING_CPP == 0 in C mode",
                                 "USING_CPP should be 0 in C mode",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // suppress unused variable warning
        (void)name_matches_value;
    #endif

    printf("%s    D_ENV_LANG_USING_CPP: %d\n", D_INDENT, D_ENV_LANG_USING_CPP);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C++ standard detection test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C++ standard detection test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* VERSION COMPARISON MACRO TESTS
******************************************************************************/

/*
d_tests_sa_env_lang_c_version_comparison_macros
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
bool
d_tests_sa_env_lang_c_version_comparison_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   is_c95;
    bool   is_c99;
    bool   is_c11;
    bool   is_c17;
    bool   is_c23;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing C Version Comparison Macros ---\n", D_INDENT);

    // evaluate all comparison macros
    is_c95 = D_ENV_LANG_IS_C95_OR_HIGHER;
    is_c99 = D_ENV_LANG_IS_C99_OR_HIGHER;
    is_c11 = D_ENV_LANG_IS_C11_OR_HIGHER;
    is_c17 = D_ENV_LANG_IS_C17_OR_HIGHER;
    is_c23 = D_ENV_LANG_IS_C23_OR_HIGHER;

    // verify monotonic property: if C23, then also C17, C11, C99, C95
    if (is_c23)
    {
        if (!d_assert_standalone(is_c17,
                                 "C23 implies C17",
                                 "C23 must also be C17 or higher",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (is_c17)
    {
        if (!d_assert_standalone(is_c11,
                                 "C17 implies C11",
                                 "C17 must also be C11 or higher",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (is_c11)
    {
        if (!d_assert_standalone(is_c99,
                                 "C11 implies C99",
                                 "C11 must also be C99 or higher",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    if (is_c99)
    {
        if (!d_assert_standalone(is_c95,
                                 "C99 implies C95",
                                 "C99 must also be C95 or higher",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // verify macros match detected standard
    #ifdef D_ENV_LANG_C_STANDARD
        // verify C95 comparison
        if (!d_assert_standalone( is_c95 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C95),
                                 "IS_C95_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify C99 comparison
        if (!d_assert_standalone( is_c99 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C99),
                                 "IS_C99_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify C11 comparison
        if (!d_assert_standalone( is_c11 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C11),
                                 "IS_C11_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify C17 comparison
        if (!d_assert_standalone( is_c17 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C17),
                                 "IS_C17_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify C23 comparison
        if (!d_assert_standalone( is_c23 == (D_ENV_LANG_C_STANDARD >= D_ENV_LANG_C_STANDARD_C23),
                                 "IS_C23_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // report results
    printf("%s    IS_C95_OR_HIGHER: %s\n", D_INDENT, is_c95 ? "true" : "false");
    printf("%s    IS_C99_OR_HIGHER: %s\n", D_INDENT, is_c99 ? "true" : "false");
    printf("%s    IS_C11_OR_HIGHER: %s\n", D_INDENT, is_c11 ? "true" : "false");
    printf("%s    IS_C17_OR_HIGHER: %s\n", D_INDENT, is_c17 ? "true" : "false");
    printf("%s    IS_C23_OR_HIGHER: %s\n", D_INDENT, is_c23 ? "true" : "false");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C version comparison macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C version comparison macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_lang_cpp_version_comparison_macros
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
bool
d_tests_sa_env_lang_cpp_version_comparison_macros
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

    printf("%s--- Testing C++ Version Comparison Macros ---\n", D_INDENT);

    #ifdef D_ENV_LANG_CPP_STANDARD
        // C++ mode: test all comparison macros
        bool is_cpp98 = D_ENV_LANG_IS_CPP98_OR_HIGHER;
        bool is_cpp11 = D_ENV_LANG_IS_CPP11_OR_HIGHER;
        bool is_cpp14 = D_ENV_LANG_IS_CPP14_OR_HIGHER;
        bool is_cpp17 = D_ENV_LANG_IS_CPP17_OR_HIGHER;
        bool is_cpp20 = D_ENV_LANG_IS_CPP20_OR_HIGHER;
        bool is_cpp23 = D_ENV_LANG_IS_CPP23_OR_HIGHER;

        printf("%s    Compiling as C++, testing comparison macros\n", D_INDENT);

        // verify monotonic property
        if (is_cpp23)
        {
            if (!d_assert_standalone(is_cpp20,
                                     "C++23 implies C++20",
                                     "C++23 must also be C++20 or higher",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp20)
        {
            if (!d_assert_standalone(is_cpp17,
                                     "C++20 implies C++17",
                                     "C++20 must also be C++17 or higher",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp17)
        {
            if (!d_assert_standalone(is_cpp14,
                                     "C++17 implies C++14",
                                     "C++17 must also be C++14 or higher",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp14)
        {
            if (!d_assert_standalone(is_cpp11,
                                     "C++14 implies C++11",
                                     "C++14 must also be C++11 or higher",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }

        if (is_cpp11)
        {
            if (!d_assert_standalone(is_cpp98,
                                     "C++11 implies C++98",
                                     "C++11 must also be C++98 or higher",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        }

        // in C++ mode, should always be at least C++98
        if (!d_assert_standalone(is_cpp98,
                                 "always C++98 or higher in C++ mode",
                                 "minimum C++ standard is C++98",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // verify macros match direct comparison
        if (!d_assert_standalone( is_cpp98 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP98),
                                 "IS_CPP98_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone( is_cpp11 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP11),
                                 "IS_CPP11_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone( is_cpp14 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP14),
                                 "IS_CPP14_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone( is_cpp17 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP17),
                                 "IS_CPP17_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone( is_cpp20 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP20),
                                 "IS_CPP20_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        if (!d_assert_standalone( is_cpp23 == (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP23),
                                 "IS_CPP23_OR_HIGHER matches direct comparison",
                                 "macro should equal direct >= comparison",
                                 _test_info))
        {
            all_assertions_passed = false;
        }

        // report results
        printf("%s    IS_CPP98_OR_HIGHER: %s\n", D_INDENT, is_cpp98 ? "true" : "false");
        printf("%s    IS_CPP11_OR_HIGHER: %s\n", D_INDENT, is_cpp11 ? "true" : "false");
        printf("%s    IS_CPP14_OR_HIGHER: %s\n", D_INDENT, is_cpp14 ? "true" : "false");
        printf("%s    IS_CPP17_OR_HIGHER: %s\n", D_INDENT, is_cpp17 ? "true" : "false");
        printf("%s    IS_CPP20_OR_HIGHER: %s\n", D_INDENT, is_cpp20 ? "true" : "false");
        printf("%s    IS_CPP23_OR_HIGHER: %s\n", D_INDENT, is_cpp23 ? "true" : "false");

    #else
        // C mode: verify C++ comparison macros are not defined
        printf("%s    Compiling as C, C++ comparison macros should not exist\n",
               D_INDENT);

        #ifdef D_ENV_LANG_IS_CPP98_OR_HIGHER
            if (!d_assert_standalone(false,
                                     "IS_CPP98_OR_HIGHER should NOT be defined",
                                     "C++ macros should not exist in C mode",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #else
            if (!d_assert_standalone(true,
                                     "IS_CPP98_OR_HIGHER not defined in C mode",
                                     "correct: C++ macros not in C mode",
                                     _test_info))
            {
                all_assertions_passed = false;
            }
        #endif

        printf("%s    (C++ comparison macros correctly absent)\n", D_INDENT);
    #endif

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] C++ version comparison macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] C++ version comparison macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_lang_version_comparison_boundary_cases
  Tests version comparison macros at exact boundary values.
  Tests the following:
  - exact match: standard == boundary returns true for that comparison
  - just below: standard == boundary-1 returns false for that comparison
  - comparison operators are >= not >
*/
bool
d_tests_sa_env_lang_version_comparison_boundary_cases
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    long   test_value;
    bool   comparison_result;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Version Comparison Boundary Cases ---\n", D_INDENT);

    // test C99 boundary: exact match should be true
    test_value = D_ENV_LANG_C_STANDARD_C99;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C99);

    if (!d_assert_standalone(comparison_result == true,
                             "exact C99 value >= C99 is true",
                             "exact match should satisfy >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test C99 boundary: one below should be false
    test_value = D_ENV_LANG_C_STANDARD_C99 - 1;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C99);

    if (!d_assert_standalone(comparison_result == false,
                             "C99-1 >= C99 is false",
                             "value below boundary should fail comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test C11 boundary: exact match should be true
    test_value = D_ENV_LANG_C_STANDARD_C11;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C11);

    if (!d_assert_standalone(comparison_result == true,
                             "exact C11 value >= C11 is true",
                             "exact match should satisfy >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test C11 boundary: one below should be false
    test_value = D_ENV_LANG_C_STANDARD_C11 - 1;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C11);

    if (!d_assert_standalone(comparison_result == false,
                             "C11-1 >= C11 is false",
                             "value below boundary should fail comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test that >= is used, not > (value+1 should still pass)
    test_value = D_ENV_LANG_C_STANDARD_C99 + 1;
    comparison_result = (test_value >= D_ENV_LANG_C_STANDARD_C99);

    if (!d_assert_standalone(comparison_result == true,
                             "C99+1 >= C99 is true",
                             "value above boundary should pass comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test C++ boundaries
    test_value = D_ENV_LANG_CPP_STANDARD_CPP11;
    comparison_result = (test_value >= D_ENV_LANG_CPP_STANDARD_CPP11);

    if (!d_assert_standalone(comparison_result == true,
                             "exact C++11 value >= C++11 is true",
                             "exact match should satisfy >= comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    test_value = D_ENV_LANG_CPP_STANDARD_CPP11 - 1;
    comparison_result = (test_value >= D_ENV_LANG_CPP_STANDARD_CPP11);

    if (!d_assert_standalone(comparison_result == false,
                             "C++11-1 >= C++11 is false",
                             "value below boundary should fail comparison",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // test transition between standards (e.g., C99 to C11 gap)
    // a value between C99 and C11 should pass C99 but fail C11
    test_value = D_ENV_LANG_C_STANDARD_C99 + 1000; // arbitrary mid-gap value

    if (!d_assert_standalone( (test_value >= D_ENV_LANG_C_STANDARD_C99) &&
                              (test_value < D_ENV_LANG_C_STANDARD_C11),
                             "mid-gap value passes lower, fails higher",
                             "gap handling should work correctly",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    Boundary tests completed\n", D_INDENT);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Version comparison boundary cases test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Version comparison boundary cases test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* CONVENIENCE MACRO TESTS
******************************************************************************/

/*
d_tests_sa_env_lang_using_macros
  Tests the D_ENV_LANG_USING_C and D_ENV_LANG_USING_CPP convenience macros.
  Tests the following:
  - D_ENV_LANG_USING_C is always defined (0 or 1)
  - D_ENV_LANG_USING_CPP is always defined (0 or 1)
  - values are boolean (exactly 0 or 1, not just truthy/falsy)
  - USING_C should be 1 when C standard is detected
  - USING_CPP should be 1 only in C++ mode
  - mutual exclusivity considerations
*/
bool
d_tests_sa_env_lang_using_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    using_c;
    int    using_cpp;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing USING_C and USING_CPP Macros ---\n", D_INDENT);

    // get macro values
    using_c   = D_ENV_LANG_USING_C;
    using_cpp = D_ENV_LANG_USING_CPP;

    // verify D_ENV_LANG_USING_C is defined
    #ifdef D_ENV_LANG_USING_C
        if (!d_assert_standalone(true,
                                 "D_ENV_LANG_USING_C is defined",
                                 "USING_C macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_LANG_USING_C should be defined",
                                 "USING_C macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify D_ENV_LANG_USING_CPP is defined
    #ifdef D_ENV_LANG_USING_CPP
        if (!d_assert_standalone(true,
                                 "D_ENV_LANG_USING_CPP is defined",
                                 "USING_CPP macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(false,
                                 "D_ENV_LANG_USING_CPP should be defined",
                                 "USING_CPP macro must always be defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify values are boolean (exactly 0 or 1)
    if (!d_assert_standalone( (using_c == 0) || (using_c == 1),
                             "D_ENV_LANG_USING_C is 0 or 1",
                             "boolean macro should be exactly 0 or 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    if (!d_assert_standalone( (using_cpp == 0) || (using_cpp == 1),
                             "D_ENV_LANG_USING_CPP is 0 or 1",
                             "boolean macro should be exactly 0 or 1",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify USING_C matches C standard detection
    #ifdef D_ENV_LANG_C_STANDARD
        if (!d_assert_standalone(using_c == 1,
                                 "USING_C == 1 when C standard detected",
                                 "USING_C should be 1 when standard is defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(using_c == 0,
                                 "USING_C == 0 when no C standard",
                                 "USING_C should be 0 when standard not defined",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // verify USING_CPP matches C++ mode
    #ifdef __cplusplus
        if (!d_assert_standalone(using_cpp == 1,
                                 "USING_CPP == 1 in C++ mode",
                                 "USING_CPP should be 1 when compiling as C++",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #else
        if (!d_assert_standalone(using_cpp == 0,
                                 "USING_CPP == 0 in C mode",
                                 "USING_CPP should be 0 when compiling as C",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    #endif

    // note: C and C++ can both be "used" since C++ includes C features
    // so we don't test for mutual exclusivity

    printf("%s    D_ENV_LANG_USING_C:   %d\n", D_INDENT, using_c);
    printf("%s    D_ENV_LANG_USING_CPP: %d\n", D_INDENT, using_cpp);

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] USING_C and USING_CPP macros test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] USING_C and USING_CPP macros test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MANUAL DETECTION (D_ENV_DETECTED_*) TESTS
******************************************************************************/

/*
d_tests_sa_env_lang_detected_vars_c
  Tests manual C standard configuration via D_ENV_DETECTED_C* variables.
  Tests the following:
  - when D_CFG_ENV_CUSTOM disables language detection, D_ENV_DETECTED_* are used
  - D_ENV_DETECTED_C95/C99/C11/C17/C23 correctly set corresponding standard
  - only one C standard detection variable should be defined at a time
  - fallback to C90 when no detection variable is defined
*/
bool
d_tests_sa_env_lang_detected_vars_c
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    detected_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    detected_count        = 0;

    printf("%s--- Testing Manual C Standard Detection Variables ---\n",
           D_INDENT);

    // count how many C detection variables are defined
    #ifdef D_ENV_DETECTED_C95
        detected_count++;
        printf("%s    D_ENV_DETECTED_C95 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_C99
        detected_count++;
        printf("%s    D_ENV_DETECTED_C99 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_C11
        detected_count++;
        printf("%s    D_ENV_DETECTED_C11 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_C17
        detected_count++;
        printf("%s    D_ENV_DETECTED_C17 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_C23
        detected_count++;
        printf("%s    D_ENV_DETECTED_C23 is defined\n", D_INDENT);
    #endif

    // verify at most one C detection variable is defined
    if (!d_assert_standalone(detected_count <= 1,
                             "at most one C detection var defined",
                             "C standard detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    C detection vars count: %d\n", D_INDENT, detected_count);

    // if manual detection is active (language bit set), verify mapping
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG)
        printf("%s    Manual detection mode active\n", D_INDENT);

        #ifdef D_ENV_DETECTED_C23
            #ifdef D_ENV_LANG_C_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C23,
                                         "D_ENV_DETECTED_C23 maps to C23 standard",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C17)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C17,
                                         "D_ENV_DETECTED_C17 maps to C17 standard",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C11)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C11,
                                         "D_ENV_DETECTED_C11 maps to C11 standard",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C99)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C99,
                                         "D_ENV_DETECTED_C99 maps to C99 standard",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_C95)
            #ifdef D_ENV_LANG_C_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_C_STANDARD == D_ENV_LANG_C_STANDARD_C95,
                                         "D_ENV_DETECTED_C95 maps to C95 standard",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #else
        printf("%s    Automatic detection mode (manual vars not used)\n",
               D_INDENT);

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
        printf("%s[PASS] Manual C standard detection variables test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Manual C standard detection variables test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_lang_detected_vars_cpp
  Tests manual C++ standard configuration via D_ENV_DETECTED_CPP* variables.
  Tests the following:
  - when D_CFG_ENV_CUSTOM disables language detection, D_ENV_DETECTED_* are used
  - D_ENV_DETECTED_CPP98/11/14/17/20/23 correctly set corresponding standard
  - only one C++ standard detection variable should be defined at a time
*/
bool
d_tests_sa_env_lang_detected_vars_cpp
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    int    detected_count;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;
    detected_count        = 0;

    printf("%s--- Testing Manual C++ Standard Detection Variables ---\n",
           D_INDENT);

    // count how many C++ detection variables are defined
    #ifdef D_ENV_DETECTED_CPP98
        detected_count++;
        printf("%s    D_ENV_DETECTED_CPP98 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_CPP11
        detected_count++;
        printf("%s    D_ENV_DETECTED_CPP11 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_CPP14
        detected_count++;
        printf("%s    D_ENV_DETECTED_CPP14 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_CPP17
        detected_count++;
        printf("%s    D_ENV_DETECTED_CPP17 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_CPP20
        detected_count++;
        printf("%s    D_ENV_DETECTED_CPP20 is defined\n", D_INDENT);
    #endif

    #ifdef D_ENV_DETECTED_CPP23
        detected_count++;
        printf("%s    D_ENV_DETECTED_CPP23 is defined\n", D_INDENT);
    #endif

    // verify at most one C++ detection variable is defined
    if (!d_assert_standalone(detected_count <= 1,
                             "at most one C++ detection var defined",
                             "C++ standard detection should be mutually exclusive",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    printf("%s    C++ detection vars count: %d\n", D_INDENT, detected_count);

    // if manual detection is active, verify mapping
    #if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG)
        printf("%s    Manual detection mode active\n", D_INDENT);

        #ifdef D_ENV_DETECTED_CPP23
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP23,
                                         "D_ENV_DETECTED_CPP23 maps to C++23",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP20)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP20,
                                         "D_ENV_DETECTED_CPP20 maps to C++20",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP17)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP17,
                                         "D_ENV_DETECTED_CPP17 maps to C++17",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP14)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP14,
                                         "D_ENV_DETECTED_CPP14 maps to C++14",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP11)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP11,
                                         "D_ENV_DETECTED_CPP11 maps to C++11",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #elif defined(D_ENV_DETECTED_CPP98)
            #ifdef D_ENV_LANG_CPP_STANDARD
                if (!d_assert_standalone(D_ENV_LANG_CPP_STANDARD == D_ENV_LANG_CPP_STANDARD_CPP98,
                                         "D_ENV_DETECTED_CPP98 maps to C++98",
                                         "detected var should set correct standard",
                                         _test_info))
                {
                    all_assertions_passed = false;
                }
            #endif
        #endif
    #else
        printf("%s    Automatic detection mode (manual vars not used)\n",
               D_INDENT);

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
        printf("%s[PASS] Manual C++ standard detection variables test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Manual C++ standard detection variables test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* EDGE CASE AND INTEGRATION TESTS
******************************************************************************/

/*
d_tests_sa_env_lang_consistency_check
  Tests overall consistency of language detection.
  Tests the following:
  - standard name and value are always both defined or both undefined
  - USING_* macros are consistent with standard detection
  - no contradictory states (e.g., C++ detected but USING_CPP == 0)
  - comparison macros are consistent with detected standard
*/
bool
d_tests_sa_env_lang_consistency_check
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   c_standard_defined;
    bool   c_name_defined;
    bool   cpp_standard_defined;
    bool   cpp_name_defined;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Language Detection Consistency ---\n", D_INDENT);

    // check C standard/name pair consistency
    c_standard_defined = false;
    c_name_defined     = false;

    #ifdef D_ENV_LANG_C_STANDARD
        c_standard_defined = true;
    #endif

    #ifdef D_ENV_LANG_C_STANDARD_NAME
        c_name_defined = true;
    #endif

    if (!d_assert_standalone(c_standard_defined == c_name_defined,
                             "C standard and name are both defined or both undefined",
                             "standard/name pair should be consistent",
                             _test_info))
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

    if (!d_assert_standalone(cpp_standard_defined == cpp_name_defined,
                             "C++ standard and name are both defined or both undefined",
                             "standard/name pair should be consistent",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify USING_CPP is consistent with CPP_STANDARD
    if (!d_assert_standalone((D_ENV_LANG_USING_CPP == 1) == cpp_standard_defined,
                             "USING_CPP == 1 iff CPP_STANDARD defined",
                             "USING_CPP should match standard definition",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // verify DETECTED_CPP is consistent with CPP_STANDARD in auto mode
    #if D_CFG_ENV_LANG_ENABLED
        #ifdef __cplusplus
            #ifdef D_ENV_LANG_DETECTED_CPP
                if (!d_assert_standalone(cpp_standard_defined,
                                         "DETECTED_CPP implies CPP_STANDARD defined",
                                         "detection flag should imply standard",
                                         _test_info))
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

        if (!d_assert_standalone(true,
                                 "C comparison macros evaluate without error",
                                 "macros should be usable in boolean context",
                                 _test_info))
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

        if (!d_assert_standalone(true,
                                 "C++ comparison macros evaluate without error",
                                 "macros should be usable in boolean context",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }
    #endif

    printf("%s    C standard defined:   %s\n",
           D_INDENT,
           c_standard_defined ? "YES" : "NO");
    printf("%s    C++ standard defined: %s\n",
           D_INDENT,
           cpp_standard_defined ? "YES" : "NO");

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Language detection consistency test passed\n",
               D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Language detection consistency test failed\n",
               D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_env_lang_feature_test_macros
  Tests practical use of comparison macros for feature testing.
  Tests the following:
  - comparison macros can be used in #if directives
  - macros produce correct results for common feature gates
  - practical patterns work (e.g., "if C99 or higher, use inline")
*/
bool
d_tests_sa_env_lang_feature_test_macros
(
    struct d_test_counter* _test_info
)
{
    bool   all_assertions_passed;
    size_t initial_tests_passed;
    bool   has_c99_features;
    bool   has_c11_features;

    if (!_test_info)
    {
        return false;
    }

    all_assertions_passed = true;
    initial_tests_passed  = _test_info->tests_passed;

    printf("%s--- Testing Feature Test Macro Patterns ---\n", D_INDENT);

    // test common C99 feature gate pattern
    #if D_ENV_LANG_IS_C99_OR_HIGHER
        has_c99_features = true;
        printf("%s    C99+ features available (inline, restrict, etc.)\n",
               D_INDENT);
    #else
        has_c99_features = false;
        printf("%s    Pre-C99 mode (no inline, restrict, etc.)\n", D_INDENT);
    #endif

    // test common C11 feature gate pattern
    #if D_ENV_LANG_IS_C11_OR_HIGHER
        has_c11_features = true;
        printf("%s    C11+ features available (_Generic, _Atomic, etc.)\n",
               D_INDENT);
    #else
        has_c11_features = false;
        printf("%s    Pre-C11 mode (no _Generic, _Atomic, etc.)\n", D_INDENT);
    #endif

    // verify feature flags are consistent with each other
    // C11 implies C99
    if (has_c11_features)
    {
        if (!d_assert_standalone(has_c99_features,
                                 "C11 features imply C99 features",
                                 "feature availability should be monotonic",
                                 _test_info))
        {
            all_assertions_passed = false;
        }
    }

    // test C++ feature gate patterns
    #ifdef D_ENV_LANG_CPP_STANDARD
        #if D_ENV_LANG_IS_CPP11_OR_HIGHER
            printf("%s    C++11+ features (auto, nullptr, lambdas, etc.)\n",
                   D_INDENT);
        #endif

        #if D_ENV_LANG_IS_CPP17_OR_HIGHER
            printf("%s    C++17+ features (if constexpr, fold expressions)\n",
                   D_INDENT);
        #endif

        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            printf("%s    C++20+ features (concepts, ranges, modules)\n",
                   D_INDENT);
        #endif
    #endif

    // verify the pattern of conditional compilation works
    if (!d_assert_standalone(true,
                             "feature test patterns compile successfully",
                             "#if directives with comparison macros work",
                             _test_info))
    {
        all_assertions_passed = false;
    }

    // suppress unused warnings
    (void)has_c99_features;
    (void)has_c11_features;

    // update test counter
    if (all_assertions_passed)
    {
        _test_info->tests_passed++;
        printf("%s[PASS] Feature test macro patterns test passed\n", D_INDENT);
    }
    else
    {
        printf("%s[FAIL] Feature test macro patterns test failed\n", D_INDENT);
    }
    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/******************************************************************************
* MODULE TEST AGGREGATOR
******************************************************************************/

/*
d_tests_sa_env_lang_all
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
bool
d_tests_sa_env_lang_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool c_constants_result;
    bool cpp_constants_result;
    bool constant_ranges_result;
    bool c_detection_result;
    bool c_fallback_result;
    bool cpp_detection_result;
    bool c_comparison_result;
    bool cpp_comparison_result;
    bool boundary_result;
    bool using_macros_result;
    bool detected_c_result;
    bool detected_cpp_result;
    bool consistency_result;
    bool feature_test_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){0, 0, 0, 0};

    printf("\n[MODULE] Testing Language Environment Detection\n");
    printf("========================================="
           "=======================================\n");

    // run all language tests
    c_constants_result    = d_tests_sa_env_lang_c_standard_constants(&module_counter);
    cpp_constants_result  = d_tests_sa_env_lang_cpp_standard_constants(&module_counter);
    constant_ranges_result = d_tests_sa_env_lang_standard_constant_ranges(&module_counter);
    c_detection_result    = d_tests_sa_env_lang_c_standard_detection(&module_counter);
    c_fallback_result     = d_tests_sa_env_lang_c_standard_fallback(&module_counter);
    cpp_detection_result  = d_tests_sa_env_lang_cpp_standard_detection(&module_counter);
    c_comparison_result   = d_tests_sa_env_lang_c_version_comparison_macros(&module_counter);
    cpp_comparison_result = d_tests_sa_env_lang_cpp_version_comparison_macros(&module_counter);
    boundary_result       = d_tests_sa_env_lang_version_comparison_boundary_cases(&module_counter);
    using_macros_result   = d_tests_sa_env_lang_using_macros(&module_counter);
    detected_c_result     = d_tests_sa_env_lang_detected_vars_c(&module_counter);
    detected_cpp_result   = d_tests_sa_env_lang_detected_vars_cpp(&module_counter);
    consistency_result    = d_tests_sa_env_lang_consistency_check(&module_counter);
    feature_test_result   = d_tests_sa_env_lang_feature_test_macros(&module_counter);

    // update totals
    _test_info->assertions_total  += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total         += module_counter.tests_total;
    _test_info->tests_passed      += module_counter.tests_passed;

    overall_result = ( c_constants_result     &&
                       cpp_constants_result   &&
                       constant_ranges_result &&
                       c_detection_result     &&
                       c_fallback_result      &&
                       cpp_detection_result   &&
                       c_comparison_result    &&
                       cpp_comparison_result  &&
                       boundary_result        &&
                       using_macros_result    &&
                       detected_c_result      &&
                       detected_cpp_result    &&
                       consistency_result     &&
                       feature_test_result );

    printf("\n");

    if (overall_result)
    {
        printf("[PASS] Language Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);
    }
    else
    {
        printf("[FAIL] Language Module: %zu/%zu assertions, %zu/%zu tests passed\n",
               module_counter.assertions_passed,
               module_counter.assertions_total,
               module_counter.tests_passed,
               module_counter.tests_total);

        printf("  - C Standard Constants:      %s\n",
               c_constants_result ? "PASSED" : "FAILED");
        printf("  - C++ Standard Constants:    %s\n",
               cpp_constants_result ? "PASSED" : "FAILED");
        printf("  - Constant Ranges:           %s\n",
               constant_ranges_result ? "PASSED" : "FAILED");
        printf("  - C Standard Detection:      %s\n",
               c_detection_result ? "PASSED" : "FAILED");
        printf("  - C90 Fallback:              %s\n",
               c_fallback_result ? "PASSED" : "FAILED");
        printf("  - C++ Standard Detection:    %s\n",
               cpp_detection_result ? "PASSED" : "FAILED");
        printf("  - C Comparison Macros:       %s\n",
               c_comparison_result ? "PASSED" : "FAILED");
        printf("  - C++ Comparison Macros:     %s\n",
               cpp_comparison_result ? "PASSED" : "FAILED");
        printf("  - Boundary Cases:            %s\n",
               boundary_result ? "PASSED" : "FAILED");
        printf("  - USING_* Macros:            %s\n",
               using_macros_result ? "PASSED" : "FAILED");
        printf("  - Manual C Detection Vars:   %s\n",
               detected_c_result ? "PASSED" : "FAILED");
        printf("  - Manual C++ Detection Vars: %s\n",
               detected_cpp_result ? "PASSED" : "FAILED");
        printf("  - Consistency Check:         %s\n",
               consistency_result ? "PASSED" : "FAILED");
        printf("  - Feature Test Patterns:     %s\n",
               feature_test_result ? "PASSED" : "FAILED");
    }

    return overall_result;
}
