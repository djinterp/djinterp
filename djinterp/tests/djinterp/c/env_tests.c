/*******************************************************************************
* djinterp [test]                                                    env_tests.c
*
*   Master test suite aggregator for all env module tests.
*   Calls all submodule test functions and aggregates results.
*
*   NOTE: This file contains ONLY d_tests_env_all (the master function).
*         d_tests_env_build_all belongs in env_tests_sa_build.c
*
*
* path:      /tests/djinterp/c/env_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.19
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "./env_tests.h"


/*
d_tests_env_all
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
  _suite: pointer to test counter structure to accumulate results
Return:
  true if all tests passed, false otherwise
*/
bool d_tests_env_all(
    struct dj_test_suite* _suite
)
{
    d_tests_env_cfg_all(_suite);
    d_tests_env_lang_all(_suite);
    d_tests_env_posix_all(_suite);
    d_tests_env_compiler_all(_suite);
    d_tests_env_pp_limits_all(_suite);
    d_tests_env_arch_all(_suite);
    d_tests_env_os_all(_suite);
    d_tests_env_build_all(_suite);

    return true;
}
