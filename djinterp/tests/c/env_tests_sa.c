/******************************************************************************
* djinterp [test]                                           env_tests_sa_all.c
*
*   Master test suite aggregator for all env module tests.
*   Calls all submodule test functions and aggregates results.
*
*   NOTE: This file contains ONLY d_tests_sa_env_all (the master function).
*         d_tests_sa_env_build_all belongs in env_tests_sa_build.c
*
*
* path:      \tests\env_tests_sa_all.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include ".\env_tests_sa.h"


/*
d_tests_sa_env_all
  Master test suite that runs all environment detection tests.
  Executes tests for:
  - Configuration system (cfg)
  - Language detection (lang)
  - POSIX detection (posix)
  - Compiler detection (compiler)
  - Preprocessor limits (pp_limits)
  - Architecture detection (arch)
  - Operating system detection (os)
  - Build configuration (build)

Parameter(s):
  _test_info: pointer to test counter structure to accumulate results
Return:
  true if all tests passed, false otherwise
*/
bool
d_tests_sa_env_all
(
    struct d_test_counter* _test_info
)
{
    struct d_test_counter module_counter;
    bool cfg_result;
    bool lang_result;
    bool posix_result;
    bool compiler_result;
    bool pp_limits_result;
    bool arch_result;
    bool os_result;
    bool build_result;
    bool overall_result;

    if (!_test_info)
    {
        return false;
    }

    module_counter = (struct d_test_counter){ 0, 0, 0, 0 };

    printf("\n");
    printf("================================================================================\n");
    printf("ENVIRONMENT DETECTION TEST SUITE\n");
    printf("================================================================================\n");

    // run all submodule tests
    cfg_result = d_tests_sa_env_cfg_all(&module_counter);
    lang_result = d_tests_sa_env_lang_all(&module_counter);
    posix_result = d_tests_sa_env_posix_all(&module_counter);
    compiler_result = d_tests_sa_env_compiler_all(&module_counter);
    pp_limits_result = d_tests_sa_env_pp_limits_all(&module_counter);
    arch_result = d_tests_sa_env_arch_all(&module_counter);
    os_result = d_tests_sa_env_os_all(&module_counter);
    build_result = d_tests_sa_env_build_all(&module_counter);

    // update totals
    _test_info->assertions_total += module_counter.assertions_total;
    _test_info->assertions_passed += module_counter.assertions_passed;
    _test_info->tests_total += module_counter.tests_total;
    _test_info->tests_passed += module_counter.tests_passed;

    overall_result = (cfg_result &&
        lang_result &&
        posix_result &&
        compiler_result &&
        pp_limits_result &&
        arch_result &&
        os_result &&
        build_result);

    // print suite summary
    printf("\n");
    printf("================================================================================\n");
    printf("ENVIRONMENT DETECTION TEST SUITE SUMMARY\n");
    printf("================================================================================\n");
    printf("  Configuration (cfg):     %s\n", cfg_result ? "PASSED" : "FAILED");
    printf("  Language (lang):         %s\n", lang_result ? "PASSED" : "FAILED");
    printf("  POSIX (posix):           %s\n", posix_result ? "PASSED" : "FAILED");
    printf("  Compiler (compiler):     %s\n", compiler_result ? "PASSED" : "FAILED");
    printf("  PP Limits (pp_limits):   %s\n", pp_limits_result ? "PASSED" : "FAILED");
    printf("  Architecture (arch):     %s\n", arch_result ? "PASSED" : "FAILED");
    printf("  Operating System (os):   %s\n", os_result ? "PASSED" : "FAILED");
    printf("  Build (build):           %s\n", build_result ? "PASSED" : "FAILED");
    printf("--------------------------------------------------------------------------------\n");
    printf("  Total Assertions: %zu/%zu passed\n",
        module_counter.assertions_passed,
        module_counter.assertions_total);
    printf("  Total Tests:      %zu/%zu passed\n",
        module_counter.tests_passed,
        module_counter.tests_total);
    printf("================================================================================\n");

    if (overall_result)
    {
        printf("[PASS] Environment Detection Test Suite PASSED\n");
    }
    else
    {
        printf("[FAIL] Environment Detection Test Suite FAILED\n");
    }

    printf("================================================================================\n");

    return overall_result;
}