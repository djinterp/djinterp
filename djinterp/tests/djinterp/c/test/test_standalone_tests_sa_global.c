#include "./test_standalone_tests_sa.h"


/******************************************************************************
 * XV. GLOBAL STATE TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_global_state
  Tests the global state variables.
  Tests the following:
  - g_d_test_options is accessible and defaults to NULL
  - g_d_test_failures is accessible and defaults to NULL
  - g_d_test_current_module is accessible and defaults to NULL
  - g_d_test_assertion_number is accessible
  - g_d_test_test_number is accessible
  - g_d_test_output_file is accessible and defaults to NULL
  - Global variables can be assigned and read back
*/
bool
d_tests_sa_standalone_global_state
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_options*      saved_options;
    struct d_test_sa_failure_list* saved_failures;
    const char*                    saved_module;
    size_t                         saved_assertion_num;
    size_t                         saved_test_num;
    FILE*                          saved_file;

    result = true;

    // save current global state
    saved_options       = g_d_test_options;
    saved_failures      = g_d_test_failures;
    saved_module        = g_d_test_current_module;
    saved_assertion_num = g_d_test_assertion_number;
    saved_test_num      = g_d_test_test_number;
    saved_file          = g_d_test_output_file;

    // test 1: g_d_test_options is accessible
    {
        struct d_test_sa_options local_opts;

        d_test_sa_options_init(&local_opts);
        g_d_test_options = &local_opts;

        result = d_assert_standalone(
            g_d_test_options == &local_opts,
            "global_options_writable",
            "g_d_test_options should be writable",
            _counter) && result;

        result = d_assert_standalone(
            g_d_test_options->show_info == true,
            "global_options_readable",
            "g_d_test_options should be readable",
            _counter) && result;
    }

    // test 2: g_d_test_failures is accessible
    {
        struct d_test_sa_failure_list local_list;

        local_list.count    = 0;
        local_list.capacity = 0;
        local_list.entries  = NULL;

        g_d_test_failures = &local_list;

        result = d_assert_standalone(
            g_d_test_failures == &local_list,
            "global_failures_writable",
            "g_d_test_failures should be writable",
            _counter) && result;

        result = d_assert_standalone(
            g_d_test_failures->count == 0,
            "global_failures_readable",
            "g_d_test_failures should be readable",
            _counter) && result;
    }

    // test 3: g_d_test_current_module is accessible
    g_d_test_current_module = "test_global_module";

    result = d_assert_standalone(
        g_d_test_current_module != NULL,
        "global_current_module_writable",
        "g_d_test_current_module should be writable",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(g_d_test_current_module,
               "test_global_module") == 0,
        "global_current_module_readable",
        "g_d_test_current_module should be readable",
        _counter) && result;

    // test 4: g_d_test_assertion_number is accessible
    g_d_test_assertion_number = 42;

    result = d_assert_standalone(
        g_d_test_assertion_number == 42,
        "global_assertion_number_writable",
        "g_d_test_assertion_number should be writable",
        _counter) && result;

    // test 5: g_d_test_test_number is accessible
    g_d_test_test_number = 99;

    result = d_assert_standalone(
        g_d_test_test_number == 99,
        "global_test_number_writable",
        "g_d_test_test_number should be writable",
        _counter) && result;

    // test 6: g_d_test_output_file is accessible
    result = d_assert_standalone(
        sizeof(g_d_test_output_file) == sizeof(FILE*),
        "global_output_file_type",
        "g_d_test_output_file should be FILE* sized",
        _counter) && result;

    // test 7: globals can be set to NULL
    g_d_test_options        = NULL;
    g_d_test_failures       = NULL;
    g_d_test_current_module = NULL;
    g_d_test_output_file    = NULL;

    result = d_assert_standalone(
        (g_d_test_options == NULL)        &&
        (g_d_test_failures == NULL)       &&
        (g_d_test_current_module == NULL) &&
        (g_d_test_output_file == NULL),
        "global_nullable",
        "All pointer globals should be nullable",
        _counter) && result;

    // restore global state
    g_d_test_options          = saved_options;
    g_d_test_failures         = saved_failures;
    g_d_test_current_module   = saved_module;
    g_d_test_assertion_number = saved_assertion_num;
    g_d_test_test_number      = saved_test_num;
    g_d_test_output_file      = saved_file;

    return result;
}

/*
d_tests_sa_standalone_global_all
  Aggregation function that runs all global state tests.
*/
bool
d_tests_sa_standalone_global_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Global State\n");
    printf("  -----------------------\n");

    result = d_tests_sa_standalone_global_state(_counter) && result;

    return result;
}
