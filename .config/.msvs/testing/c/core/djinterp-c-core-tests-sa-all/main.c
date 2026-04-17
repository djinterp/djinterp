/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Unified test runner for all core module standalone tests.
*   Runs every core module test suite in a single executable:
*     env, dmacro, djinterp, dmemory, string_fn, dstring, dfile, dio,
*     dtime, datomic, dmutex
*
*
* path:      \.config\.msvs\testing\core\djinterp-c-core-tests-sa\main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "..\..\..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\..\..\tests\c\env_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\dmacro_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\djinterp_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\dmemory_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\string_fn_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\dstring_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\dfile_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\dio_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\dtime_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\datomic_tests_sa.h"
#include "..\..\..\..\..\..\tests\c\dmutex_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_core_status_items[] =
{
    { "[INFO]", "Environment detection (env) validated" },
    { "[INFO]", "Preprocessor macro utilities (dmacro) tested" },
    { "[INFO]", "Core types and indexing (djinterp) verified" },
    { "[INFO]", "Memory operations (dmemory) functional" },
    { "[INFO]", "C string functions (string_fn) working" },
    { "[INFO]", "Safe string type (dstring) validated" },
    { "[INFO]", "File I/O operations (dfile) tested" },
    { "[INFO]", "Formatted I/O operations (dio) verified" },
    { "[INFO]", "Time utilities (dtime) functional" },
    { "[INFO]", "Atomic operations (datomic) validated" },
    { "[INFO]", "Mutex and threading (dmutex) tested" }
};

static const struct d_test_sa_note_item g_core_issues_items[] =
{
    { "[WARN]", "MSVC preprocessor requires /Zc:preprocessor for dmacro" },
    { "[WARN]", "Some secure variants are no-ops on POSIX" },
    { "[NOTE]", "dio gets_s tests redirect stdin; restored after tests" },
    { "[NOTE]", "dtime sleep tests have platform-dependent precision" },
    { "[NOTE]", "dmutex tests require threading support" }
};

static const struct d_test_sa_note_section g_core_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_core_status_items) / sizeof(g_core_status_items[0]),
      g_core_status_items },
    { "KNOWN ISSUES",
      sizeof(g_core_issues_items) / sizeof(g_core_issues_items[0]),
      g_core_issues_items }
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
                          "djinterp Core Modules",
                          "Comprehensive Testing of All Core Module "
                          "Standalone Test Suites");

    //
    // counter-based modules: bool (*)(struct d_test_counter*)
    //

    d_test_sa_runner_add_module_counter(&runner,
                                        "env",
                                        "Environment detection and platform "
                                        "configuration",
                                        d_tests_sa_env_all,
                                        0, NULL);

    d_test_sa_runner_add_module_counter(&runner,
                                        "dmacro",
                                        "Preprocessor macro utilities and "
                                        "metaprogramming tools",
                                        d_tests_sa_dmacro_all,
                                        0, NULL);

    d_test_sa_runner_add_module_counter(&runner,
                                        "djinterp",
                                        "Core types, indexing, and fundamental "
                                        "definitions",
                                        d_tests_sa_run_all,
                                        0, NULL);

    d_test_sa_runner_add_module_counter(&runner,
                                        "dio",
                                        "Formatted input/output, secure variants, "
                                        "character and string I/O, large file "
                                        "stream positioning, and error handling",
                                        d_tests_sa_dio_run_all,
                                        0, NULL);

    d_test_sa_runner_add_module_counter(&runner,
                                        "datomic",
                                        "Atomic operations, memory fences, and "
                                        "lock-free primitives",
                                        d_tests_sa_atomic_run_all,
                                        0, NULL);

    d_test_sa_runner_add_module_counter(&runner,
                                        "dmutex",
                                        "Mutex, threading, condition variables, "
                                        "and read-write locks",
                                        d_tests_sa_dmutex_run_all,
                                        0, NULL);

    //
    // tree-based modules: struct d_test_object* (*)(void)
    //

    d_test_sa_runner_add_module(&runner,
                                "dmemory",
                                "Memory copy, duplication, set, and "
                                "boundary conditions",
                                d_tests_dmemory_run_all,
                                0, NULL);

    d_test_sa_runner_add_module(&runner,
                                "string_fn",
                                "C string operations: copy, compare, "
                                "tokenize, search, case conversion",
                                d_tests_string_fn_run_all,
                                0, NULL);

    d_test_sa_runner_add_module(&runner,
                                "dstring",
                                "Safe string type: creation, modification, "
                                "search, comparison, formatting",
                                d_tests_sa_dstring_all,
                                0, NULL);

    d_test_sa_runner_add_module(&runner,
                                "dfile",
                                "File I/O: secure open, large file support, "
                                "descriptors, locking, metadata, paths",
                                d_tests_dfile_run_all,
                                0, NULL);

    d_test_sa_runner_add_module(&runner,
                                "dtime",
                                "Time utilities: thread-safe conversion, "
                                "high-resolution, sleep, arithmetic",
                                d_tests_dtime_run_all,
                                0, NULL);

    // configuration
    d_test_sa_runner_set_wait_for_input(&runner, true);
    d_test_sa_runner_set_show_notes(&runner, true);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}