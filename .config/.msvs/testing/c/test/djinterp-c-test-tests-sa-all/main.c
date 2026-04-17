/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Unified test runner for all djinterp standalone test modules.
*   Registers and executes comprehensive standalone tests for: test_common,
* test_standalone, test_cvar, assert, compose, test_printer, test_handler,
* test_session, test_module, test_block, test_config, and cli_option.
*
*
* path:      /.config/.msvs/testing/c/djinterp-c-test-all-sa/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"

#include "../../../../../../tests/c/test/test_common_tests_sa.h"
#include "../../../../../../tests/c/test/test_standalone_tests_sa.h"
#include "../../../../../../tests/c/test/assert_tests_sa.h"
#include "../../../../../../tests/c/test/test_cvar_tests_sa.h"
#include "../../../../../../tests/c/test/test_options_tests_sa.h"
#include "../../../../../../tests/c/test/test_tests_sa.h"
#include "../../../../../../tests/c/test/test_block_tests_sa.h"
#include "../../../../../../tests/c/test/test_module_tests_sa.h"
#include "../../../../../../tests/c/test/test_session_tests_sa.h"
#include "../../../../../../tests/c/test/test_handler_tests_sa.h"
//#include "../../../../../../test/test/test_printer_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES: test_common
 *****************************************************************************/

static const struct d_test_sa_note_item g_test_common_status_items[] =
{
    { "[INFO]", "Keyword macros (ASSERTION, TEST_FN, TEST, BLOCK, MODULE) validated" },
    { "[INFO]", "Pass/fail macros (D_TEST_PASS, D_TEST_FAIL) verified against D_SUCCESS/D_FAILURE" },
    { "[INFO]", "Symbol macros (PASS, FAIL, SUCCESS, INFO) tested for both emoji and ASCII modes" },
    { "[INFO]", "Tree structure symbols (LEAF, INTERIOR, MODULE, WARNING, UNKNOWN) validated" },
    { "[INFO]", "Type definitions (d_test_id, fn_test, fn_stage) size and behavior tested" },
    { "[INFO]", "Argument structures (d_test_arg, d_test_arg_list) member access verified" },
    { "[INFO]", "Test function wrapper (d_test_fn) structure validated" },
    { "[INFO]", "Lifecycle stages (DTestStage) enum values and sequencing verified" },
    { "[INFO]", "Type discriminators (DTestTypeFlag) uniqueness and hierarchy validated" }
};

static const struct d_test_sa_note_item g_test_common_issues_items[] =
{
    { "[NOTE]", "d_test_id is typedef'd to unsigned long long (64-bit minimum)" },
    { "[NOTE]", "fn_test returns bool and takes no parameters" },
    { "[NOTE]", "fn_stage returns bool and takes struct d_test* parameter" },
    { "[NOTE]", "Symbol macros use conditional compilation for emoji vs ASCII" },
    { "[WARN]", "D_EMOJIS must be defined and equal D_ENABLED for emoji symbols" }
};

static const struct d_test_sa_note_item g_test_common_steps_items[] =
{
    { "[TODO]", "Add integration tests with actual test framework" },
    { "[TODO]", "Add stress tests for d_test_arg_list with large argument counts" },
    { "[TODO]", "Test symbol macros on non-UTF8 terminals" },
    { "[TODO]", "Add validation for min_enum_map dependency" },
    { "[TODO]", "Create mock d_test structure for fn_stage testing" }
};

static const struct d_test_sa_note_item g_test_common_guidelines_items[] =
{
    { "[BEST]", "Use D_TEST_PASS and D_TEST_FAIL instead of raw true/false in tests" },
    { "[BEST]", "Check fn_test and fn_stage pointers for NULL before calling" },
    { "[BEST]", "Use DTestTypeFlag for discriminated union pattern" },
    { "[BEST]", "Iterate d_test_arg_list using count member, not null-termination" },
    { "[BEST]", "Use DTestStage values as array indices for lifecycle hook arrays" }
};

static const struct d_test_sa_note_section g_test_common_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_test_common_status_items) / sizeof(g_test_common_status_items[0]),
      g_test_common_status_items },
    { "KNOWN ISSUES",
      sizeof(g_test_common_issues_items) / sizeof(g_test_common_issues_items[0]),
      g_test_common_issues_items },
    { "NEXT STEPS",
      sizeof(g_test_common_steps_items) / sizeof(g_test_common_steps_items[0]),
      g_test_common_steps_items },
    { "BEST PRACTICES",
      sizeof(g_test_common_guidelines_items) / sizeof(g_test_common_guidelines_items[0]),
      g_test_common_guidelines_items }
};


/******************************************************************************
 * IMPLEMENTATION NOTES: test_standalone
 *****************************************************************************/

static const struct d_test_sa_note_item g_test_standalone_status_items[] =
{
    { "[INFO]", "Assertion macros (TRUE, FALSE, NULL, NOT_NULL, EQUAL, STR_EQUAL) validated" },
    { "[INFO]", "Object constants (LEAF, INTERIOR) and formatting constants verified"        },
    { "[INFO]", "Test counter struct and operations (reset, add) tested"                     },
    { "[INFO]", "Test object creation (new_leaf, new_interior), add_child, and free tested"  },
    { "[INFO]", "Results structures (module_results, suite_results) validated" },
    { "[INFO]", "Note structures (note_item, note_section) validated" },
    { "[INFO]", "Module entry and runner structures tested" },
    { "[INFO]", "Function pointer types (fn_print_object, fn_print_object_file) tested" },
    { "[INFO]", "d_assert_standalone function behavior verified" },
    { "[INFO]", "Template substitution with various scenarios tested" },
    { "[INFO]", "Runner functions (init, add_module, set_wait, set_notes, cleanup) tested" },
    { "[INFO]", "Utility functions (get_elapsed_time) validated" }
};

static const struct d_test_sa_note_item g_test_standalone_issues_items[] =
{
    { "[NOTE]", "D_ASSERT_STR_EQUAL uses strcmp() - ensure strings are valid" },
    { "[NOTE]", "d_test_object_free recursively frees all children" },
    { "[NOTE]", "Runner defaults: wait_for_input=true, show_notes=true" },
    { "[NOTE]", "D_TEST_SA_MAX_MODULES limits total registered modules to 64" },
    { "[WARN]", "Template substitution allocates memory - caller must free" }
};

static const struct d_test_sa_note_item g_test_standalone_steps_items[] =
{
    { "[TODO]", "Add tests for d_test_sa_runner_execute (complex integration)" },
    { "[TODO]", "Add tests for output formatting functions" },
    { "[TODO]", "Add tests for d_test_standalone_output_console" },
    { "[TODO]", "Add tests for d_test_standalone_output_file" },
    { "[TODO]", "Add tests for d_test_sa_print_timestamp" }
};

static const struct d_test_sa_note_item g_test_standalone_guidelines_items[] =
{
    { "[BEST]", "Use D_ASSERT_* macros for tree-based test objects" },
    { "[BEST]", "Use d_assert_standalone for counter-based tests" },
    { "[BEST]", "Always free test objects with d_test_object_free" },
    { "[BEST]", "Initialize runners with d_test_sa_runner_init before use" },
    { "[BEST]", "Call d_test_sa_runner_cleanup after execution" }
};

static const struct d_test_sa_note_section g_test_standalone_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_test_standalone_status_items) / sizeof(g_test_standalone_status_items[0]),
      g_test_standalone_status_items },
    { "KNOWN ISSUES",
      sizeof(g_test_standalone_issues_items) / sizeof(g_test_standalone_issues_items[0]),
      g_test_standalone_issues_items },
    { "NEXT STEPS",
      sizeof(g_test_standalone_steps_items) / sizeof(g_test_standalone_steps_items[0]),
      g_test_standalone_steps_items },
    { "BEST PRACTICES",
      sizeof(g_test_standalone_guidelines_items) / sizeof(g_test_standalone_guidelines_items[0]),
      g_test_standalone_guidelines_items }
};


/******************************************************************************
 * IMPLEMENTATION NOTES: test_cvar
 *****************************************************************************/

static const struct d_test_sa_note_item g_cvar_status_items[] =
{
    { "[INFO]", "Registry row flags validated (9 unique power-of-2 flags)" },
    { "[INFO]", "Value union member access for all 6 types verified" },
    { "[INFO]", "Row structure layout confirmed (key at offset 0)" },
    { "[INFO]", "Find by key and find by flag operations working correctly" },
    { "[INFO]", "All 8 aliases resolve to expected primary keys" },
    { "[INFO]", "Get/set/reset roundtrip operations validated" },
    { "[INFO]", "Table integrity: 25 rows (8 config + 17 metadata)" }
};

static const struct d_test_sa_note_item g_cvar_issues_items[] =
{
    { "[WARN]", "get_default/get_default_by_key declared but not implemented" },
    { "[WARN]", "Predicate functions are static inline (tested via flags)" },
    { "[NOTE]", "Frozen registry behavior not tested (requires freeze API)" }
};

static const struct d_test_sa_note_item g_cvar_steps_items[] =
{
    { "[TODO]", "Add tests for frozen registry set/reset rejection" },
    { "[TODO]", "Add tests for d_registry_add_alias error paths" },
    { "[TODO]", "Add concurrency tests for registry access" },
    { "[TODO]", "Benchmark registry lookup performance" }
};

static const struct d_test_sa_note_item g_cvar_guidelines_items[] =
{
    { "[BEST]", "Always call d_test_registry_init before registry access" },
    { "[BEST]", "Use d_test_registry_reset_all in test teardown" },
    { "[BEST]", "Prefer typed access macros over raw find + cast" },
    { "[BEST]", "Use aliases for user-facing key names" }
};

static const struct d_test_sa_note_section g_cvar_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_cvar_status_items) / sizeof(g_cvar_status_items[0]),
      g_cvar_status_items },
    { "KNOWN ISSUES",
      sizeof(g_cvar_issues_items) / sizeof(g_cvar_issues_items[0]),
      g_cvar_issues_items },
    { "NEXT STEPS",
      sizeof(g_cvar_steps_items) / sizeof(g_cvar_steps_items[0]),
      g_cvar_steps_items },
    { "BEST PRACTICES",
      sizeof(g_cvar_guidelines_items) / sizeof(g_cvar_guidelines_items[0]),
      g_cvar_guidelines_items }
};


/******************************************************************************
 * IMPLEMENTATION NOTES: assert
 *****************************************************************************/

static const struct d_test_sa_note_item g_assert_status_items[] =
{
    { "[INFO]", "d_assert type creation and destruction validated" },
    { "[INFO]", "Comparison operations (eq, neq, lt, gt) working correctly" },
    { "[INFO]", "String and array assertions verified" },
    { "[INFO]", "Memory management validated for all assertion operations" },
    { "[INFO]", "Edge cases for NULL and empty inputs handled properly" }
};

static const struct d_test_sa_note_item g_assert_issues_items[] =
{
    { "[NOTE]", "This module is a dependency of DTest framework" },
    { "[NOTE]", "Uses test_standalone.h rather than DTest for testing" }
};

static const struct d_test_sa_note_item g_assert_steps_items[] =
{
    { "[TODO]", "Add floating-point comparison assertions" },
    { "[TODO]", "Add range-based assertions (between, within)" },
    { "[TODO]", "Add collection assertions (contains, all_match)" },
    { "[TODO]", "Create performance benchmarks for assertion overhead" }
};

static const struct d_test_sa_note_item g_assert_guidelines_items[] =
{
    { "[BEST]", "Always free d_assert objects when no longer needed" },
    { "[BEST]", "Use appropriate assertion function for the data type" },
    { "[BEST]", "Provide descriptive pass and fail messages" },
    { "[BEST]", "Check assertion return value before accessing result" }
};

static const struct d_test_sa_note_section g_assert_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_assert_status_items) / sizeof(g_assert_status_items[0]),
      g_assert_status_items },
    { "KNOWN ISSUES",
      sizeof(g_assert_issues_items) / sizeof(g_assert_issues_items[0]),
      g_assert_issues_items },
    { "NEXT STEPS",
      sizeof(g_assert_steps_items) / sizeof(g_assert_steps_items[0]),
      g_assert_steps_items },
    { "BEST PRACTICES",
      sizeof(g_assert_guidelines_items) / sizeof(g_assert_guidelines_items[0]),
      g_assert_guidelines_items }
};


/******************************************************************************
 * IMPLEMENTATION NOTES: test_handler
 *****************************************************************************/

static const struct d_test_sa_note_item g_handler_status_items[] =
{
    { "[INFO]", "d_test_handler creation and destruction validated" },
    { "[INFO]", "Flag management (set, clear, has) working correctly" },
    { "[INFO]", "Event listener registration and lifecycle verified" },
    { "[INFO]", "Test, block, module, tree, session execution validated" },
    { "[INFO]", "Assertion tracking and statistics accumulation verified" },
    { "[INFO]", "Stack operations (push/pop result and context) validated" },
    { "[INFO]", "Result queries and rate calculations working correctly" },
    { "[INFO]", "Context helper allocation, init, and free validated" },
    { "[INFO]", "DSL helper functions for module/block creation verified" },
    { "[INFO]", "Edge cases for NULL inputs handled properly" }
};

static const struct d_test_sa_note_item g_handler_issues_items[] =
{
    { "[NOTE]", "This module is a dependency of DTest framework" },
    { "[NOTE]", "Uses test_standalone.h rather than DTest for testing" },
    { "[NOTE]", "Uses string_fn.h instead of string.h for safe ops" }
};

static const struct d_test_sa_note_item g_handler_steps_items[] =
{
    { "[TODO]", "Add thread-safety tests for concurrent handler access" },
    { "[TODO]", "Add stress tests for very deep nesting levels" },
    { "[TODO]", "Add output buffer management tests" },
    { "[TODO]", "Add performance benchmarks for handler overhead" }
};

static const struct d_test_sa_note_item g_handler_guidelines_items[] =
{
    { "[BEST]", "Always free d_test_handler when no longer needed" },
    { "[BEST]", "Use d_test_handler_new_full for precise control" },
    { "[BEST]", "Reset results between independent test batches" },
    { "[BEST]", "Check return values from stack push/pop operations" },
    { "[BEST]", "Use TRACK_STACK flag for execution context tracing" }
};

static const struct d_test_sa_note_section g_handler_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_handler_status_items) / sizeof(g_handler_status_items[0]),
      g_handler_status_items },
    { "KNOWN ISSUES",
      sizeof(g_handler_issues_items) / sizeof(g_handler_issues_items[0]),
      g_handler_issues_items },
    { "NEXT STEPS",
      sizeof(g_handler_steps_items) / sizeof(g_handler_steps_items[0]),
      g_handler_steps_items },
    { "BEST PRACTICES",
      sizeof(g_handler_guidelines_items) / sizeof(g_handler_guidelines_items[0]),
      g_handler_guidelines_items }
};


/******************************************************************************
 * IMPLEMENTATION NOTES: test_session
 *****************************************************************************/

static const struct d_test_sa_note_item g_session_status_notes[] =
{
    { "[INFO]", "Tests updated for new d_test_session API with "
                "d_ptr_vector children, d_test_options, and "
                "d_test_session_output." },
    { "[INFO]", "Uses string_fn.h safe functions; no strcmp, strcpy, "
                "or fopen." },
    { "[INFO]", "Covers constructor/destructor, configuration, child "
                "management, execution, output, statistics, status, "
                "and utility." }
};

static const struct d_test_sa_note_item g_session_coverage_notes[] =
{
    { "[INFO]", "d_test_session_new" },
    { "[INFO]", "d_test_session_new_with_config" },
    { "[INFO]", "d_test_session_new_with_modules" },
    { "[INFO]", "d_test_session_new_with_modules_and_config" },
    { "[INFO]", "d_test_session_validate_args" },
    { "[INFO]", "d_test_session_free" },
    { "[INFO]", "d_test_session_set_option / get_option" },
    { "[INFO]", "d_test_session_set_output_format" },
    { "[INFO]", "d_test_session_set_verbosity" },
    { "[INFO]", "d_test_session_enable_color" },
    { "[INFO]", "d_test_session_add_child / add_children" },
    { "[INFO]", "d_test_session_child_count / get_child_at" },
    { "[INFO]", "d_test_session_clear_children" },
    { "[INFO]", "d_test_session_run / run_child / run_by_name" },
    { "[INFO]", "d_test_session_abort / reset / pause / resume" },
    { "[INFO]", "d_test_session_write_header / footer / summary" },
    { "[INFO]", "d_test_session_flush" },
    { "[INFO]", "d_test_session_get_stats / all_passed" },
    { "[INFO]", "d_test_session_total_run/passed/failed/skipped" },
    { "[INFO]", "d_test_session_duration_ms" },
    { "[INFO]", "d_test_session_get_status / status_to_string" },
    { "[INFO]", "d_test_session_is_running / is_complete" },
    { "[INFO]", "d_test_session_print / exit_code" }
};

static const struct d_test_sa_note_section g_session_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_session_status_notes) / sizeof(g_session_status_notes[0]),
      g_session_status_notes },
    { "FUNCTIONS COVERED",
      sizeof(g_session_coverage_notes) / sizeof(g_session_coverage_notes[0]),
      g_session_coverage_notes }
};


/******************************************************************************
 * IMPLEMENTATION NOTES: test_module
 *****************************************************************************/

static const struct d_test_sa_note_item g_module_status_notes[] =
{
    { "[INFO]", "Tests updated for new d_test_module API with "
                "d_ptr_vector children and d_test_config." },
    { "[INFO]", "Uses string_fn.h safe functions; no strcmp, strcpy, "
                "or fopen." },
    { "[INFO]", "Covers constructor/destructor, child management, "
                "config, hooks, execution, results, and output." }
};

static const struct d_test_sa_note_item g_module_coverage_notes[] =
{
    { "[INFO]", "d_test_module_new" },
    { "[INFO]", "d_test_module_new_args" },
    { "[INFO]", "d_test_module_validate_args" },
    { "[INFO]", "d_test_module_free" },
    { "[INFO]", "d_test_module_add_child" },
    { "[INFO]", "d_test_module_child_count" },
    { "[INFO]", "d_test_module_get_child_at" },
    { "[INFO]", "d_test_module_get_effective_settings" },
    { "[INFO]", "d_test_module_get_name" },
    { "[INFO]", "d_test_module_set_stage_hook" },
    { "[INFO]", "d_test_module_get_stage_hook" },
    { "[INFO]", "d_test_module_run" },
    { "[INFO]", "d_test_module_run_child" },
    { "[INFO]", "d_test_module_get_result" },
    { "[INFO]", "d_test_module_reset_result" },
    { "[INFO]", "d_test_module_get_pass_rate" },
    { "[INFO]", "d_test_module_print" },
    { "[INFO]", "d_test_module_print_result" },
    { "[INFO]", "d_test_module_status_to_string" }
};

static const struct d_test_sa_note_section g_module_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_module_status_notes) / sizeof(g_module_status_notes[0]),
      g_module_status_notes },
    { "FUNCTIONS COVERED",
      sizeof(g_module_coverage_notes) / sizeof(g_module_coverage_notes[0]),
      g_module_coverage_notes }
};


/******************************************************************************
 * IMPLEMENTATION NOTES: test_config (test_options)
 *****************************************************************************/

static const struct d_test_sa_note_item g_config_status_items[] =
{
    { "[INFO]", "Flag manipulation macros (shift, to_flags, from_flags) validated" },
    { "[INFO]", "Mask definitions verified for non-overlapping bit ranges" },
    { "[INFO]", "All enum values confirmed unique and sequential" },
    { "[INFO]", "Message flag combinations produce correct composites" },
    { "[INFO]", "Mode presets match their documented flag layouts" },
    { "[INFO]", "Utility and semantic macros operate correctly on configs" },
    { "[INFO]", "Constructor/destructor lifecycle management verified" },
    { "[INFO]", "Typed getters/setters with schema defaults validated" },
    { "[INFO]", "Key lookup with aliases and type-safety confirmed" }
};

static const struct d_test_sa_note_item g_config_issues_items[] =
{
    { "[WARN]", "Override map stores values via pointer-sized casts" },
    { "[NOTE]", "MESSAGE_FLAGS key modifies config->flags directly" }
};

static const struct d_test_sa_note_item g_config_steps_items[] =
{
    { "[TODO]", "Add thread-safety tests for concurrent config access" },
    { "[TODO]", "Benchmark config lookup performance under load" },
    { "[TODO]", "Add serialization/deserialization round-trip tests" },
    { "[TODO]", "Test config inheritance and override cascading" }
};

static const struct d_test_sa_note_item g_config_guidelines_items[] =
{
    { "[BEST]", "Always free d_test_config objects when no longer needed" },
    { "[BEST]", "Use typed setters to ensure schema type compatibility" },
    { "[BEST]", "Prefer d_test_options_key_from_string for user input" },
    { "[BEST]", "Use presets for standard mode configurations" }
};

static const struct d_test_sa_note_section g_config_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_config_status_items) / sizeof(g_config_status_items[0]),
      g_config_status_items },
    { "KNOWN ISSUES",
      sizeof(g_config_issues_items) / sizeof(g_config_issues_items[0]),
      g_config_issues_items },
    { "NEXT STEPS",
      sizeof(g_config_steps_items) / sizeof(g_config_steps_items[0]),
      g_config_steps_items },
    { "BEST PRACTICES",
      sizeof(g_config_guidelines_items) / sizeof(g_config_guidelines_items[0]),
      g_config_guidelines_items }
};


/******************************************************************************
 * MAIN ENTRY POINT
 *****************************************************************************/

/*
main
  Unified entry point for all djinterp standalone test suites.
  Initializes the test registry and runner, parses CLI options, registers
every test module, executes all tests, and returns the aggregate exit status.

Parameter(s):
  _argc: argument count from the command line.
  _argv: argument vector from the command line.
Return:
  EXIT_SUCCESS (0) if all tests passed, EXIT_FAILURE (1) otherwise.
*/
int
main
(
    int   _argc,
    char* _argv[]
)
{
    struct d_test_sa_runner  runner;
    struct d_test_sa_options options;
    int                      exit_code;

    // initialize the test registry
    d_test_registry_init();

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "djinterp All Modules",
                          "Unified Standalone Test Suite for All "
                          "djinterp Modules");

    // parse CLI options
    d_test_sa_options_init(&options);

    if (_argc > 1)
    {
        if (!d_test_sa_options_parse(&options, _argc, _argv))
        {
            d_test_sa_options_print_usage(_argv[0]);

            return EXIT_FAILURE;
        }
    }

    d_test_sa_runner_set_options(&runner, &options);

    // enable features
    d_test_sa_runner_set_show_notes(&runner, true);
    d_test_sa_runner_set_wait_for_input(&runner, false);

    // ---- register all test modules ----

    // 1. test_common
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_common",
        "test_common types and structures including "
        "macro definitions, type definitions, "
        "argument structures, test function wrappers, "
        "lifecycle stages, and type discriminators",
        d_tests_sa_test_common_run_all,
        ( sizeof(g_test_common_notes) / sizeof(g_test_common_notes[0]) ),
        g_test_common_notes);

    // 2. test_standalone
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_standalone",
        "test_standalone framework including "
        "assertion macros, constants, counter "
        "operations, test objects, results and "
        "note structures, runner structures and "
        "functions, function pointers, template "
        "substitution, and utility functions",
        d_tests_sa_standalone_run_all,
        sizeof(g_test_standalone_notes) /
            sizeof(g_test_standalone_notes[0]),
        g_test_standalone_notes);

    // 3. test_cvar
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_cvar",
        "Registry row flags, value union, "
        "row structure, init, find, aliases, "
        "get/set, reset, validation, macros, "
        "predicates, integrity, and defaults",
        d_tests_sa_cvar_run_all,
        sizeof(g_cvar_notes) /
            sizeof(g_cvar_notes[0]),
        g_cvar_notes);

    // 4. assert (uses add_module, not add_module_counter)
    d_test_sa_runner_add_module(
        &runner,
        "assert",
        "d_assert type creation, comparison, and "
        "memory management",
        d_tests_sa_assert_all,
        sizeof(g_assert_notes) /
            sizeof(g_assert_notes[0]),
        g_assert_notes);

    // 6. test_printer
    //d_test_sa_runner_add_module_counter(
    //    &runner,
    //    "test_printer",
    //    "Test output and reporting - string "
    //    "generation, file I/O, formatting",
    //    d_tests_sa_test_printer_all,
    //    0,
    //    NULL);

    // 7. test_handler
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_handler",
        "d_test_handler creation, execution, events, "
        "statistics, and memory management",
        d_tests_sa_test_handler_all,
        sizeof(g_handler_notes) /
            sizeof(g_handler_notes[0]),
        g_handler_notes);

    // 8. test_session
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_session",
        "Unit tests for the d_test_session structure "
        "and API",
        d_tests_sa_test_session_all,
        sizeof(g_session_notes) /
            sizeof(g_session_notes[0]),
        g_session_notes);

    // 9. test_module
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_module",
        "Unit tests for the d_test_module structure "
        "and API",
        d_tests_sa_test_module_all,
        sizeof(g_module_notes) /
            sizeof(g_module_notes[0]),
        g_module_notes);

    // 10. test_block
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_block",
        "Tests for d_test_block_new, child mgmt, "
        "hooks, run, and utilities",
        d_tests_sa_test_block_all,
        0,
        NULL);

    // 11. test_config (test_options)
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_config",
        "d_test_config creation, flag "
        "manipulation, typed accessors, "
        "and key lookup",
        d_tests_sa_config_run_all,
        sizeof(g_config_notes) /
            sizeof(g_config_notes[0]),
        g_config_notes);

    // 12. cli_option
    d_test_sa_runner_add_module_counter(
        &runner,
        "cli_option",
        "Tests for d_string_cli_entry lookup, "
        "field search, and argv parsing",
        d_tests_sa_test_run_all,
        0,
        NULL);

    // execute all registered modules
    exit_code = d_test_sa_runner_execute(&runner);

    // cleanup
    d_test_sa_runner_cleanup(&runner);

    return exit_code;
}