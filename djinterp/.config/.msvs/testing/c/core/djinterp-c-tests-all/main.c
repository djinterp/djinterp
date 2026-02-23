/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Master test runner for the djinterp project.
*   Executes every registered standalone test module across all subsystems:
* core, functional, container, and test framework. Intended as the single
* top-level entry point for full project validation.
*
*
* path:      .config/.msvs/testing/c/djinterp-c-master-tests-sa/main.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.22
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"

// core test headers
#include "../../../../../../tests/c/datomic_tests_sa.h"
#include "../../../../../../tests/c/dfile_tests_sa.h"
#include "../../../../../../tests/c/dio_tests_sa.h"
#include "../../../../../../tests/c/djinterp_tests_sa.h"
#include "../../../../../../tests/c/dmacro_tests_sa.h"
#include "../../../../../../tests/c/dmemory_tests_sa.h"
#include "../../../../../../tests/c/dmutex_tests_sa.h"
#include "../../../../../../tests/c/dstring_tests_sa.h"
#include "../../../../../../tests/c/dtime_tests_sa.h"
#include "../../../../../../tests/c/env_tests_sa.h"
#include "../../../../../../tests/c/string_fn_tests_sa.h"

// functional test headers
#include "../../../../../../tests/c/functional/compose_tests_sa.h"
#include "../../../../../../tests/c/functional/filter_tests_sa.h"
#include "../../../../../../tests/c/functional/fn_builder_tests_sa.h"
#include "../../../../../../tests/c/functional/functional_common_tests_sa.h"
#include "../../../../../../tests/c/functional/pipeline_tests_sa.h"
#include "../../../../../../tests/c/functional/predicate_tests_sa.h"

// container test headers
#include "../../../../../../tests/c/container/map/enum_map_entry_tests_sa.h"
#include "../../../../../../tests/c/container/map/min_enum_map_tests_sa.h"

// test framework test headers
#include "../../../../../../tests/c/test/test_common_tests_sa.h"
#include "../../../../../../tests/c/test/test_standalone_tests_sa.h"


/*
main
  Entry point for the master test suite.
  Sets up the test runner, registers every test module across all subsystems,
executes all tests, and displays comprehensive results. This is the definitive
top-level runner for full project validation.

Parameter(s):
  _argc: argument count (unused)
  _argv: argument values (unused)
Return:
  0 on success, 1 on failure.
*/
int
main
(
    int   _argc,
    char* _argv[]
)
{
    struct d_test_sa_runner runner;
    int                     exit_code;

    (void)_argc;
    (void)_argv;

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "djinterp Master Test Suite",
                          "Full project validation: all core, "
                          "functional, container, and test "
                          "framework modules");

    ///////////////////////////////////////////////////////////////////////////
    //  core modules
    ///////////////////////////////////////////////////////////////////////////

    // register env module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Environment Detection",
        "Tests for env.h environment detection macros",
        d_tests_sa_env_all,
        0,
        NULL);

    // register djinterp module
    d_test_sa_runner_add_module_counter(
        &runner,
        "djinterp Core",
        "Tests for djinterp.h core types, constants, and macros",
        d_tests_sa_run_all,
        0,
        NULL);

    // register dmacro module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Macro Utilities",
        "Tests for dmacro.h macro utilities",
        d_tests_sa_dmacro_all,
        0,
        NULL);

    // register dmemory module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Memory Operations",
        "Tests for dmemory.h memory operations",
        d_tests_sa_dmemory_run_all,
        0,
        NULL);

    // register datomic module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Atomic Operations",
        "Tests for datomic.h atomic operations",
        d_tests_sa_datomic_run_all,
        0,
        NULL);

    // register dmutex module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Mutex and Threading",
        "Tests for dmutex.h mutex and threading operations",
        d_tests_sa_dmutex_run_all,
        0,
        NULL);

    // register dtime module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Time Operations",
        "Tests for dtime.h time operations",
        d_tests_sa_dtime_run_all,
        0,
        NULL);

    // register string_fn module
    d_test_sa_runner_add_module_counter(
        &runner,
        "String Functions",
        "Tests for string_fn.h raw string operations",
        d_tests_sa_string_fn_run_all,
        0,
        NULL);

    // register dstring module
    d_test_sa_runner_add_module_counter(
        &runner,
        "String Type",
        "Tests for dstring.h safe string type",
        d_tests_sa_dstring_all,
        0,
        NULL);

    // register dfile module
    d_test_sa_runner_add_module_counter(
        &runner,
        "File Operations",
        "Tests for dfile.h file I/O operations",
        d_tests_dfile_run_all,
        0,
        NULL);

    // register dio module
    d_test_sa_runner_add_module_counter(
        &runner,
        "I/O Operations",
        "Tests for dio.h formatted I/O operations",
        d_tests_sa_dio_run_all,
        0,
        NULL);

    ///////////////////////////////////////////////////////////////////////////
    //  functional modules
    ///////////////////////////////////////////////////////////////////////////

    // register functional_common module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Functional Common",
        "Tests for functional_common.h shared utilities",
        d_tests_sa_functional_common_all,
        0,
        NULL);

    // register compose module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Compose Operations",
        "Tests for compose.h function composition",
        d_tests_sa_compose_run_all,
        0,
        NULL);

    // register filter module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Filter Operations",
        "Tests for filter.h filter operations",
        d_tests_sa_filter_run_all,
        0,
        NULL);

    // register fn_builder module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Function Builder",
        "Tests for fn_builder.h function builder",
        d_tests_sa_fn_builder_all,
        0,
        NULL);

    // register pipeline module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Pipeline Operations",
        "Tests for pipeline.h pipeline operations",
        d_tests_sa_pipeline_all,
        0,
        NULL);

    // register predicate module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Predicate Operations",
        "Tests for predicate.h predicate operations",
        d_tests_sa_predicate_run_all,
        0,
        NULL);

    ///////////////////////////////////////////////////////////////////////////
    //  container modules
    ///////////////////////////////////////////////////////////////////////////

    // register enum_map_entry module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Enum Map Entry",
        "Tests for enum_map_entry.h map entry operations",
        d_tests_enum_map_entry_run_all,
        0,
        NULL);

    // register min_enum_map module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Min Enum Map",
        "Tests for min_enum_map.h minimal enum map",
        d_tests_min_enum_map_run_all,
        0,
        NULL);

    ///////////////////////////////////////////////////////////////////////////
    //  test framework modules
    ///////////////////////////////////////////////////////////////////////////

    /* register assert module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Assert",
        "Tests for assert.h assertion macros and functions",
        d_tests_sa_assert_run_all,
        0,
        NULL);*/

    // register test_common module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Test Common",
        "Tests for test_common.h shared test utilities",
        d_tests_sa_test_common_run_all,
        0,
        NULL);

    /* register test_config module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Test Config",
        "Tests for test_config.h test configuration",
        d_tests_sa_test_config_run_all,
        0,
        NULL);

    // register test_cvar module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Test CVar",
        "Tests for test_cvar.h test console variables",
        d_tests_sa_test_cvar_run_all,
        0,
        NULL);*/

    // register test_standalone module
    d_test_sa_runner_add_module_counter(
        &runner,
        "Test Standalone",
        "Tests for test_standalone.h standalone test runner",
        d_tests_sa_standalone_run_all,
        0,
        NULL);

    // configure runner options
    d_test_sa_runner_set_wait_for_input(&runner, false);
    d_test_sa_runner_set_show_notes(&runner, false);

    // execute all tests
    exit_code = d_test_sa_runner_execute(&runner);

    // cleanup
    d_test_sa_runner_cleanup(&runner);

    return exit_code;
}