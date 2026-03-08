/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for cli_option module unit tests.
*   Executes comprehensive standalone tests for convention-agnostic
* string-to-void* lookup and CLI argument parsing including d_string_cli_entry
* lookup, generic field-based search, custom lookup delegation, argv parsing,
* and table macros.
*
*
* path:      /.config/.msvs/testing/c/test/djinterp-c-test-test-tests-sa/main.c
* link(s)    TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.26
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/test/test_session_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_status_notes[] =
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

static const struct d_test_sa_note_item g_coverage_notes[] =
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

static const struct d_test_sa_note_section g_note_sections[] =
{
    {
        "CURRENT STATUS",
        sizeof(g_status_notes) / sizeof(g_status_notes[0]),
        g_status_notes
    },
    {
        "FUNCTIONS COVERED",
        sizeof(g_coverage_notes) / sizeof(g_coverage_notes[0]),
        g_coverage_notes
    }
};

static const size_t g_note_section_count =
    sizeof(g_note_sections) / sizeof(g_note_sections[0]);


/******************************************************************************
 * MAIN
 *****************************************************************************/

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

    // initialize runner
    d_test_sa_runner_init(&runner,
                          "test_session",
                          "Standalone unit tests for test_session.h");

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

    // register the test_session test module
    d_test_sa_runner_add_module_counter(
        &runner,
        "test_session",
        "Unit tests for the d_test_session structure and API",
        d_tests_sa_test_session_all,
        g_note_section_count,
        g_note_sections
    );

    // execute all registered modules
    exit_code = d_test_sa_runner_execute(&runner);

    // cleanup
    d_test_sa_runner_cleanup(&runner);

    return exit_code;
}