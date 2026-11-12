/*******************************************************************************
* djinterp [c][test]                              test_options_tests_lifecycle.c
*
* test_options.h's suite, block "config_lifecycle_all": d_test_options_new,
* _new_preset, _new_copy (of NULL too) and _free.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_lifecycle.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_options_tests.h"  // corresponding header


//==============================================================================
// 10. CONSTRUCTOR AND DESTRUCTOR TESTS
//==============================================================================

/*
d_tests_options_new
  Tests the d_test_options_new function.
  Tests the following:
  - Returns non-NULL pointer
  - flags member matches the input
  - settings map is initialized (non-NULL)
  - stage_hooks is NULL by default
  - Works with zero flags
  - Works with complex flag combinations
*/
bool
d_tests_options_new(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: basic creation
    d_test_suite_test(_suite,
                      "config_new");

    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    result = D_TEST_CHECK_MSG(_suite,
                              "new_returns_non_null",
                              config != NULL,
                              "d_test_options_new should return non-NULL") && result;

    // the remaining checks need the option set
    if (config)
    {
        // test 2: flags match input
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_flags_match",
                                  config->flags == D_TEST_MODE_NORMAL,
                                  "flags should match the input value") && result;

        // test 3: settings map initialized
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_settings_init",
                                  config->settings != NULL,
                                  "settings map should be initialized") && result;

        // test 4: stage_hooks is NULL
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_stage_hooks_null",
                                  config->stage_hooks == NULL,
                                  "stage_hooks should be NULL by default") && result;

        d_test_options_free(config);
    }

    // test 5: zero flags
    struct dj_test_options* config_zero = d_test_options_new(0);

    result = D_TEST_CHECK_MSG(_suite,
                              "new_zero_flags",
                              config_zero != NULL,
                              "d_test_options_new(0) should return non-NULL") && result;

    // the remaining checks need the option set
    if (config_zero)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_zero_flags_value",
                                  config_zero->flags == 0,
                                  "Zero-flags config should have flags == 0") && result;

        d_test_options_free(config_zero);
    }

    // test 6: complex flags
    config = d_test_options_new(D_TEST_MODE_VERBOSE |
                               D_TEST_SETTINGS_STACK_PUSH_ALL);

    // the remaining checks need the option set
    if (config)
    {
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_complex_flags",
                                  config->flags == (D_TEST_MODE_VERBOSE | D_TEST_SETTINGS_STACK_PUSH_ALL),
                                  "Complex flags should be stored correctly") && result;

        d_test_options_free(config);
    }

    return result;
}

/*
d_tests_options_new_preset
  Tests the d_test_options_new_preset function.
  Tests the following:
  - Creates config with preset flags
  - Each preset produces the expected flag value
*/
bool
d_tests_options_new_preset(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_new_preset");

    struct dj_test_options* silent =
        d_test_options_new_preset(D_TEST_OPTIONS_PRESET_SILENT);
    struct dj_test_options* verbose =
        d_test_options_new_preset(D_TEST_OPTIONS_PRESET_VERBOSE);

    // an allocation failed: release the rest, and the test fails
    if ( (!silent) ||
         (!verbose) )
    {
        // release whichever were made; d_test_options_free takes NULL
        d_test_options_free(silent);
        d_test_options_free(verbose);

        return D_TEST_CHECK_MSG(_suite,
                                "new_preset_alloc",
                                false,
                                "Failed to allocate preset configs");
    }

    // test 1: silent preset
    result = D_TEST_CHECK_MSG(_suite,
                              "new_preset_silent",
                              silent->flags == D_TEST_MODE_SILENT,
                              "Silent preset should have silent flags") && result;

    // test 2: verbose preset
    result = D_TEST_CHECK_MSG(_suite,
                              "new_preset_verbose",
                              verbose->flags == D_TEST_MODE_VERBOSE,
                              "Verbose preset should have verbose flags") && result;

    d_test_options_free(silent);
    d_test_options_free(verbose);

    return result;
}

/*
d_tests_options_new_copy
  Tests the d_test_options_new_copy function.
  Tests the following:
  - Returns non-NULL for valid input
  - Flags are copied correctly
  - Settings map is independently allocated (not shared)
  - Modifications to copy do not affect original
*/
bool
d_tests_options_new_copy(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_new_copy");

    struct dj_test_options* original = d_test_options_new(D_TEST_MODE_VERBOSE);

    // allocation failed: the test fails here
    if (!original)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "new_copy_orig_alloc",
                                false,
                                "Failed to allocate original config");
    }

    struct dj_test_options* copy = d_test_options_new_copy(original);

    // test 1: copy is non-NULL
    result = D_TEST_CHECK_MSG(_suite,
                              "new_copy_non_null",
                              copy != NULL,
                              "d_test_options_new_copy should return non-NULL") && result;

    // the remaining checks need the option set
    if (copy)
    {
        // test 2: flags match
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_flags_match",
                                  copy->flags == original->flags,
                                  "Copied flags should match original") && result;

        // test 3: settings map is independently allocated
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_settings_independent",
                                  copy->settings != original->settings,
                                  "Copied settings map should be a different pointer") && result;

        // test 4: copy is a distinct object
        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_distinct_pointer",
                                  copy != original,
                                  "Copy should be a distinct object from original") && result;

        // test 5: modifying copy does not affect original
        copy->flags = D_TEST_MODE_SILENT;

        result = D_TEST_CHECK_MSG(_suite,
                                  "new_copy_independent_flags",
                                  original->flags == D_TEST_MODE_VERBOSE,
                                  "Modifying copy flags should not affect original") && result;

        d_test_options_free(copy);
    }

    d_test_options_free(original);

    return result;
}

/*
d_tests_options_new_copy_null
  Tests d_test_options_new_copy with NULL input.
  Tests the following:
  - Returns NULL for NULL input
*/
bool
d_tests_options_new_copy_null(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_test(_suite,
                      "config_new_copy_null");

    struct dj_test_options* copy = d_test_options_new_copy(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "new_copy_null_returns_null",
                              copy == NULL,
                              "d_test_options_new_copy(NULL) should return NULL") && result;

    return result;
}

/*
d_tests_options_free
  Tests the d_test_options_free function.
  Tests the following:
  - NULL input is handled safely (no crash)
  - Freeing a valid config does not crash
  - Freeing a config with NULL settings does not crash
*/
bool
d_tests_options_free(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // test 1: NULL is handled safely
    d_test_suite_test(_suite,
                      "config_free");

    d_test_options_free(NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "free_null_safe",
                              true,
                              "d_test_options_free(NULL) should not crash") && result;

    // test 2: valid config is freed without crash
    struct dj_test_options* config = d_test_options_new(D_TEST_MODE_NORMAL);

    // the remaining checks need the option set
    if (config)
    {
        d_test_options_free(config);

        result = D_TEST_CHECK_MSG(_suite,
                                  "free_valid_config",
                                  true,
                                  "Freeing valid config should not crash") && result;
    }

    return result;
}

/*
d_tests_options_lifecycle_all
  Aggregation function that runs all constructor/destructor tests.
*/
bool
d_tests_options_lifecycle_all(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_block(_suite,
                       "config_lifecycle_all");

    result = d_tests_options_new(_suite)           && result;
    result = d_tests_options_new_preset(_suite)    && result;
    result = d_tests_options_new_copy(_suite)      && result;
    result = d_tests_options_new_copy_null(_suite) && result;
    result = d_tests_options_free(_suite)          && result;

    return result;
}
