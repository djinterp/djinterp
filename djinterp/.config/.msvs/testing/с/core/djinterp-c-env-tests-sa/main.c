/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for env module standalone tests.
*   Tests compile-time environment detection macros.
*
*
* path:      \.config\.msvs\testing\core\djinterp-c-env-tests-sa\main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "..\..\..\..\..\inc\c\test\test_standalone.h
#include "..\..\..\..\..\tests\c\env_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_env_status_items[] =
{
    { "[INFO]", "Environment detection macros validated and ready for use" },
    { "[INFO]", "Language standard detection working correctly across "
                "C90-C23 and C++98-C++23" },
    { "[INFO]", "Compiler detection and version parsing thoroughly tested" },
    { "[INFO]", "Architecture and operating system detection established" },
    { "[INFO]", "Build configuration detection functioning properly" }
};

static const struct d_test_sa_note_item g_env_issues_items[] =
{
    { "[WARN]", "D_ENV_LANG_C_STANDARD may be undefined due to "
                "configuration logic issues" },
    { "[WARN]", "Custom environment simulation bitfield logic needs "
                "verification" },
    { "[WARN]", "Some edge cases in compiler version detection may need "
                "refinement" },
    { "[WARN]", "Legacy platform support may require additional testing" }
};

static const struct d_test_sa_note_item g_env_steps_items[] =
{
    { "[TODO]", "Fix D_CFG_ENV_LANG_ENABLED macro logic to enable "
                "automatic detection by default" },
    { "[TODO]", "Correct typo in D_CFG_ENV_USING_C_ONLY to "
                "D_CFG_ENV_DETECTED_C_ONLY" },
    { "[TODO]", "Add fallback definitions in manual detection paths" },
    { "[TODO]", "Extend testing coverage for uncommon compiler/platform "
                "combinations" },
    { "[TODO]", "Add runtime environment validation utilities" },
    { "[TODO]", "Integrate environment detection with build system "
                "configuration" }
};

static const struct d_test_sa_note_item g_env_guidelines_items[] =
{
    { "[NOTE]", "Always use D_ENV_* macros for portable "
                "environment-dependent code" },
    { "[NOTE]", "Use D_ENV_COMPILER_VERSION_AT_LEAST for compiler-specific "
                "feature detection" },
    { "[NOTE]", "Test environment detection on all target platforms during "
                "development" },
    { "[NOTE]", "Document platform-specific behaviors clearly in code "
                "comments" },
    { "[NOTE]", "Use D_CFG_ENV_CUSTOM for testing different environment "
                "scenarios" },
    { "[NOTE]", "Ensure environment detection works correctly in both C "
                "and C++ compilation modes" }
};

static const struct d_test_sa_note_section g_env_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_env_status_items) / sizeof(g_env_status_items[0]),
      g_env_status_items },
    { "KNOWN ISSUES",
      sizeof(g_env_issues_items) / sizeof(g_env_issues_items[0]),
      g_env_issues_items },
    { "NEXT STEPS",
      sizeof(g_env_steps_items) / sizeof(g_env_steps_items[0]),
      g_env_steps_items },
    { "USAGE GUIDELINES",
      sizeof(g_env_guidelines_items) / sizeof(g_env_guidelines_items[0]),
      g_env_guidelines_items }
};


/******************************************************************************
 * MAIN ENTRY POINT
 *****************************************************************************/

int
main
(
    int    _argc,
    char** _argv
)
{
    struct d_test_sa_runner runner;

    // suppress unused parameter warnings
    (void)_argc;
    (void)_argv;

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "djinterp Environment Detection",
                          "Comprehensive Testing of Compile-Time "
                          "Environment Detection");

    // register the env module (uses counter-based test function)
    d_test_sa_runner_add_module_counter(&runner,
                                        "env",
                                        "Compile-time detection of language "
                                        "standards, compilers, architectures, "
                                        "operating systems, and build "
                                        "configurations",
                                        d_tests_sa_env_all,
                                        sizeof(g_env_notes) /
                                            sizeof(g_env_notes[0]),
                                        g_env_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}