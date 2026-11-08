/*******************************************************************************
* djinterp [test]                            test_options_tests_utility_macros.c
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests_utility_macros.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "test_options_tests.h"


/******************************************************************************
 * VI. UTILITY MACRO TESTS
 *****************************************************************************/

/*
d_tests_options_has_flag
  Tests the D_TEST_HAS_FLAG macro.
  Tests the following:
  - Returns true when all bits of flag_mask are set
  - Returns false when only some bits of flag_mask are set
  - Returns false when no bits of flag_mask are set
*/
bool d_tests_options_has_flag(
    struct dj_test_suite* _suite
)
{
    struct dj_test_options* config;
    bool                   result = true;

    d_test_suite_test(_suite, "config_has_flag");

    config = d_test_options_new(D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |
                               D_TEST_MSG_FLAG_COUNT_TESTS_FAIL);

    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "has_flag_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: exact single flag match
    result = D_TEST_CHECK_MSG(_suite,
                              "has_flag_single_set",
                              D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL) == true,
                              "HAS_FLAG should be true when single flag is set") && result;

    // test 2: multi-bit flag_mask - all bits present
    result = D_TEST_CHECK_MSG(_suite,
                              "has_flag_multi_all_set",
                              D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL | D_TEST_MSG_FLAG_COUNT_TESTS_FAIL) == true,
                              "HAS_FLAG should be true when all mask bits are set") && result;

    // test 3: multi-bit flag_mask - only some bits present
    result = D_TEST_CHECK_MSG(_suite,
                              "has_flag_multi_partial",
                              D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL | D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL) == false,
                              "HAS_FLAG should be false when only some mask bits set") && result;

    // test 4: flag not set at all
    result = D_TEST_CHECK_MSG(_suite,
                              "has_flag_not_set",
                              D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS) == false,
                              "HAS_FLAG should be false when flag is not set") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_has_any_flag
  Tests the D_TEST_HAS_ANY_FLAG macro.
  Tests the following:
  - Returns true when any bit of flag_mask is set
  - Returns false when no bits of flag_mask are set
*/
bool d_tests_options_has_any_flag(
    struct dj_test_suite* _suite
)
{
    struct dj_test_options* config;
    bool                   result = true;

    d_test_suite_test(_suite, "config_has_any_flag");

    config = d_test_options_new(D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL);

    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "has_any_flag_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: any match succeeds
    result = D_TEST_CHECK_MSG(_suite,
                              "has_any_flag_partial_match",
                              D_TEST_HAS_ANY_FLAG(config, D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL | D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL) == true,
                              "HAS_ANY_FLAG should be true when any bit matches") && result;

    // test 2: no match fails
    result = D_TEST_CHECK_MSG(_suite,
                              "has_any_flag_no_match",
                              D_TEST_HAS_ANY_FLAG(config, D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS | D_TEST_MSG_FLAG_PRINT_TESTS_PASS) == false,
                              "HAS_ANY_FLAG should be false when no bits match") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_message_flags
  Tests the D_TEST_GET_MESSAGE_FLAGS macro.
  Tests the following:
  - Extracts only lower 16 bits
  - Strips upper settings bits
*/
bool d_tests_options_get_message_flags(
    struct dj_test_suite* _suite
)
{
    struct dj_test_options* config;
    uint32_t               msg_flags;
    bool                   result = true;

    d_test_suite_test(_suite, "config_get_message_flags");

    config = d_test_options_new(D_TEST_MODE_NORMAL |
                               D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_msg_flags_alloc",
                                false,
                                "Failed to allocate config");
    }

    msg_flags = D_TEST_GET_MESSAGE_FLAGS(config);

    // test 1: extracts only message portion
    result = D_TEST_CHECK_MSG(_suite,
                              "get_msg_flags_value",
                              msg_flags == D_TEST_MODE_NORMAL,
                              "GET_MESSAGE_FLAGS should extract only message portion") && result;

    // test 2: no settings bits in result
    result = D_TEST_CHECK_MSG(_suite,
                              "get_msg_flags_no_settings",
                              (msg_flags & D_TEST_MASK_SETTINGS_FLAGS) == 0,
                              "GET_MESSAGE_FLAGS result should have no settings bits") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_get_settings_flags
  Tests the D_TEST_GET_SETTINGS_FLAGS macro.
  Tests the following:
  - Extracts only upper 16 bits
  - Strips lower message bits
*/
bool d_tests_options_get_settings_flags(
    struct dj_test_suite* _suite
)
{
    struct dj_test_options* config;
    uint32_t               settings_flags;
    bool                   result = true;

    d_test_suite_test(_suite, "config_get_settings_flags");

    config = d_test_options_new(D_TEST_MODE_NORMAL |
                               D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "get_settings_flags_alloc",
                                false,
                                "Failed to allocate config");
    }

    settings_flags = D_TEST_GET_SETTINGS_FLAGS(config);

    // test 1: extracts only settings portion
    result = D_TEST_CHECK_MSG(_suite,
                              "get_settings_flags_value",
                              settings_flags == D_TEST_SETTINGS_STACK_PUSH_ALL,
                              "GET_SETTINGS_FLAGS should extract only settings portion") && result;

    // test 2: no message bits in result
    result = D_TEST_CHECK_MSG(_suite,
                              "get_settings_flags_no_message",
                              (settings_flags & D_TEST_MASK_MESSAGE_FLAGS) == 0,
                              "GET_SETTINGS_FLAGS result should have no message bits") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_is_silent
  Tests the D_TEST_IS_SILENT macro.
  Tests the following:
  - True for silent mode
  - False for non-silent modes
  - True for settings-only (no message flags)
*/
bool d_tests_options_is_silent(
    struct dj_test_suite* _suite
)
{
    struct dj_test_options* silent;
    struct dj_test_options* normal;
    struct dj_test_options* settings_only;
    bool                   result = true;

    d_test_suite_test(_suite, "config_is_silent");

    silent        = d_test_options_new(D_TEST_MODE_SILENT);
    normal        = d_test_options_new(D_TEST_MODE_NORMAL);
    settings_only = d_test_options_new(D_TEST_SETTINGS_STACK_PUSH_ALL);

    if ( (!silent) || (!normal) || (!settings_only) )
    {
        if (silent)        { d_test_options_free(silent); }
        if (normal)        { d_test_options_free(normal); }
        if (settings_only) { d_test_options_free(settings_only); }

        return D_TEST_CHECK_MSG(_suite,
                                "is_silent_alloc",
                                false,
                                "Failed to allocate configs");
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "is_silent_true",
                              D_TEST_IS_SILENT(silent) == true,
                              "IS_SILENT should be true for silent mode") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "is_silent_false_normal",
                              D_TEST_IS_SILENT(normal) == false,
                              "IS_SILENT should be false for normal mode") && result;

    // settings-only has no message flags, so IS_SILENT is true
    result = D_TEST_CHECK_MSG(_suite,
                              "is_silent_settings_only",
                              D_TEST_IS_SILENT(settings_only) == true,
                              "IS_SILENT should be true when only settings set") && result;

    d_test_options_free(silent);
    d_test_options_free(normal);
    d_test_options_free(settings_only);

    return result;
}

/*
d_tests_options_is_verbose
  Tests the D_TEST_IS_VERBOSE macro.
  Tests the following:
  - True for verbose mode
  - False for non-verbose modes
*/
bool d_tests_options_is_verbose(
    struct dj_test_suite* _suite
)
{
    struct dj_test_options* verbose;
    struct dj_test_options* normal;
    struct dj_test_options* silent;
    bool                   result = true;

    d_test_suite_test(_suite, "config_is_verbose");

    verbose = d_test_options_new(D_TEST_MODE_VERBOSE);
    normal  = d_test_options_new(D_TEST_MODE_NORMAL);
    silent  = d_test_options_new(D_TEST_MODE_SILENT);

    if ( (!verbose) || (!normal) || (!silent) )
    {
        if (verbose) { d_test_options_free(verbose); }
        if (normal)  { d_test_options_free(normal); }
        if (silent)  { d_test_options_free(silent); }

        return D_TEST_CHECK_MSG(_suite,
                                "is_verbose_alloc",
                                false,
                                "Failed to allocate configs");
    }

    result = D_TEST_CHECK_MSG(_suite,
                              "is_verbose_true",
                              D_TEST_IS_VERBOSE(verbose) == true,
                              "IS_VERBOSE should be true for verbose mode") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "is_verbose_false_normal",
                              D_TEST_IS_VERBOSE(normal) == false,
                              "IS_VERBOSE should be false for normal mode") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "is_verbose_false_silent",
                              D_TEST_IS_VERBOSE(silent) == false,
                              "IS_VERBOSE should be false for silent mode") && result;

    d_test_options_free(verbose);
    d_test_options_free(normal);
    d_test_options_free(silent);

    return result;
}

/*
d_tests_options_is_mode
  Tests the D_TEST_IS_MODE macro.
  Tests the following:
  - Returns true for exact mode match
  - Returns false for mode mismatch
  - Ignores settings flags when checking mode
*/
bool d_tests_options_is_mode(
    struct dj_test_suite* _suite
)
{
    struct dj_test_options* config;
    bool                   result = true;

    d_test_suite_test(_suite, "config_is_mode");

    config = d_test_options_new(D_TEST_MODE_NORMAL |
                                D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (!config)
    {
        return D_TEST_CHECK_MSG(_suite,
                                "is_mode_alloc",
                                false,
                                "Failed to allocate config");
    }

    // test 1: exact match (ignoring settings)
    result = D_TEST_CHECK_MSG(_suite,
                              "is_mode_exact_match",
                              D_TEST_IS_MODE(config, D_TEST_MODE_NORMAL) == true,
                              "IS_MODE should match NORMAL even with settings") && result;

    // test 2: mismatch
    result = D_TEST_CHECK_MSG(_suite,
                              "is_mode_mismatch",
                              D_TEST_IS_MODE(config, D_TEST_MODE_VERBOSE) == false,
                              "IS_MODE should not match VERBOSE for NORMAL config") && result;

    // test 3: silent mode mismatch
    result = D_TEST_CHECK_MSG(_suite,
                              "is_mode_silent_mismatch",
                              D_TEST_IS_MODE(config, D_TEST_MODE_SILENT) == false,
                              "IS_MODE should not match SILENT for NORMAL config") && result;

    d_test_options_free(config);

    return result;
}

/*
d_tests_options_utility_macro_all
  Aggregation function that runs all utility macro tests.
*/
bool d_tests_options_utility_macro_all(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    d_test_suite_block(_suite, "config_utility_macro_all");

    result = d_tests_options_has_flag(_suite)           && result;
    result = d_tests_options_has_any_flag(_suite)       && result;
    result = d_tests_options_get_message_flags(_suite)  && result;
    result = d_tests_options_get_settings_flags(_suite) && result;
    result = d_tests_options_is_silent(_suite)          && result;
    result = d_tests_options_is_verbose(_suite)         && result;
    result = d_tests_options_is_mode(_suite)            && result;

    return result;
}
