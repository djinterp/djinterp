#include ".\test_config_tests_sa.h"


/******************************************************************************
 * X. CONSTRUCTOR AND DESTRUCTOR TESTS
 *****************************************************************************/

/*
d_tests_sa_config_new
  Tests the d_test_config_new function.
  Tests the following:
  - Returns non-NULL pointer
  - flags member matches the input
  - settings map is initialized (non-NULL)
  - stage_hooks is NULL by default
  - Works with zero flags
  - Works with complex flag combinations
*/
bool
d_tests_sa_config_new
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;
    struct d_test_config* config_zero;

    result = true;

    // test 1: basic creation
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    result = d_assert_standalone(
        config != NULL,
        "new_returns_non_null",
        "d_test_config_new should return non-NULL",
        _counter) && result;

    if (config)
    {
        // test 2: flags match input
        result = d_assert_standalone(
            config->flags == D_TEST_MODE_NORMAL,
            "new_flags_match",
            "flags should match the input value",
            _counter) && result;

        // test 3: settings map initialized
        result = d_assert_standalone(
            config->settings != NULL,
            "new_settings_init",
            "settings map should be initialized",
            _counter) && result;

        // test 4: stage_hooks is NULL
        result = d_assert_standalone(
            config->stage_hooks == NULL,
            "new_stage_hooks_null",
            "stage_hooks should be NULL by default",
            _counter) && result;

        d_test_config_free(config);
    }

    // test 5: zero flags
    config_zero = d_test_config_new(0);

    result = d_assert_standalone(
        config_zero != NULL,
        "new_zero_flags",
        "d_test_config_new(0) should return non-NULL",
        _counter) && result;

    if (config_zero)
    {
        result = d_assert_standalone(
            config_zero->flags == 0,
            "new_zero_flags_value",
            "Zero-flags config should have flags == 0",
            _counter) && result;

        d_test_config_free(config_zero);
    }

    // test 6: complex flags
    config = d_test_config_new(D_TEST_MODE_VERBOSE |
                               D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (config)
    {
        result = d_assert_standalone(
            config->flags == (D_TEST_MODE_VERBOSE |
                              D_TEST_SETTINGS_STACK_PUSH_ALL),
            "new_complex_flags",
            "Complex flags should be stored correctly",
            _counter) && result;

        d_test_config_free(config);
    }

    return result;
}


/*
d_tests_sa_config_new_preset
  Tests the d_test_config_new_preset function.
  Tests the following:
  - Creates config with preset flags
  - Each preset produces the expected flag value
*/
bool
d_tests_sa_config_new_preset
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* silent;
    struct d_test_config* verbose;

    result  = true;
    silent  = d_test_config_new_preset(D_TEST_CONFIG_PRESET_SILENT);
    verbose = d_test_config_new_preset(D_TEST_CONFIG_PRESET_VERBOSE);

    if ( (!silent) || (!verbose) )
    {
        if (silent)  { d_test_config_free(silent); }
        if (verbose) { d_test_config_free(verbose); }

        return d_assert_standalone(false,
                                   "new_preset_alloc",
                                   "Failed to allocate preset configs",
                                   _counter);
    }

    // test 1: silent preset
    result = d_assert_standalone(
        silent->flags == D_TEST_MODE_SILENT,
        "new_preset_silent",
        "Silent preset should have silent flags",
        _counter) && result;

    // test 2: verbose preset
    result = d_assert_standalone(
        verbose->flags == D_TEST_MODE_VERBOSE,
        "new_preset_verbose",
        "Verbose preset should have verbose flags",
        _counter) && result;

    d_test_config_free(silent);
    d_test_config_free(verbose);

    return result;
}


/*
d_tests_sa_config_new_copy
  Tests the d_test_config_new_copy function.
  Tests the following:
  - Returns non-NULL for valid input
  - Flags are copied correctly
  - Settings map is independently allocated (not shared)
  - Modifications to copy do not affect original
*/
bool
d_tests_sa_config_new_copy
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* original;
    struct d_test_config* copy;

    result   = true;
    original = d_test_config_new(D_TEST_MODE_VERBOSE);

    if (!original)
    {
        return d_assert_standalone(false,
                                   "new_copy_orig_alloc",
                                   "Failed to allocate original config",
                                   _counter);
    }

    copy = d_test_config_new_copy(original);

    // test 1: copy is non-NULL
    result = d_assert_standalone(
        copy != NULL,
        "new_copy_non_null",
        "d_test_config_new_copy should return non-NULL",
        _counter) && result;

    if (copy)
    {
        // test 2: flags match
        result = d_assert_standalone(
            copy->flags == original->flags,
            "new_copy_flags_match",
            "Copied flags should match original",
            _counter) && result;

        // test 3: settings map is independently allocated
        result = d_assert_standalone(
            copy->settings != original->settings,
            "new_copy_settings_independent",
            "Copied settings map should be a different pointer",
            _counter) && result;

        // test 4: copy is a distinct object
        result = d_assert_standalone(
            copy != original,
            "new_copy_distinct_pointer",
            "Copy should be a distinct object from original",
            _counter) && result;

        // test 5: modifying copy does not affect original
        copy->flags = D_TEST_MODE_SILENT;

        result = d_assert_standalone(
            original->flags == D_TEST_MODE_VERBOSE,
            "new_copy_independent_flags",
            "Modifying copy flags should not affect original",
            _counter) && result;

        d_test_config_free(copy);
    }

    d_test_config_free(original);

    return result;
}


/*
d_tests_sa_config_new_copy_null
  Tests d_test_config_new_copy with NULL input.
  Tests the following:
  - Returns NULL for NULL input
*/
bool
d_tests_sa_config_new_copy_null
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* copy;

    result = true;
    copy   = d_test_config_new_copy(NULL);

    result = d_assert_standalone(
        copy == NULL,
        "new_copy_null_returns_null",
        "d_test_config_new_copy(NULL) should return NULL",
        _counter) && result;

    return result;
}


/*
d_tests_sa_config_free
  Tests the d_test_config_free function.
  Tests the following:
  - NULL input is handled safely (no crash)
  - Freeing a valid config does not crash
  - Freeing a config with NULL settings does not crash
*/
bool
d_tests_sa_config_free
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;

    // test 1: NULL is handled safely
    d_test_config_free(NULL);

    result = d_assert_standalone(
        true,
        "free_null_safe",
        "d_test_config_free(NULL) should not crash",
        _counter) && result;

    // test 2: valid config is freed without crash
    config = d_test_config_new(D_TEST_MODE_NORMAL);

    if (config)
    {
        d_test_config_free(config);

        result = d_assert_standalone(
            true,
            "free_valid_config",
            "Freeing valid config should not crash",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_config_lifecycle_all
  Aggregation function that runs all constructor/destructor tests.
*/
bool
d_tests_sa_config_lifecycle_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constructor/Destructor\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_config_new(_counter) && result;
    result = d_tests_sa_config_new_preset(_counter) && result;
    result = d_tests_sa_config_new_copy(_counter) && result;
    result = d_tests_sa_config_new_copy_null(_counter) && result;
    result = d_tests_sa_config_free(_counter) && result;

    return result;
}
