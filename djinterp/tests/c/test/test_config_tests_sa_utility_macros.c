#include ".\test_config_tests_sa.h"


/******************************************************************************
 * VI. UTILITY MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_config_has_flag
  Tests the D_TEST_HAS_FLAG macro.
  Tests the following:
  - Returns true when all bits of flag_mask are set
  - Returns false when only some bits of flag_mask are set
  - Returns false when no bits of flag_mask are set
*/
bool
d_tests_sa_config_has_flag
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |
                               D_TEST_MSG_FLAG_COUNT_TESTS_FAIL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "has_flag_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: exact single flag match
    result = d_assert_standalone(
        D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL) == true,
        "has_flag_single_set",
        "HAS_FLAG should be true when single flag is set",
        _counter) && result;

    // test 2: multi-bit flag_mask - all bits present
    result = d_assert_standalone(
        D_TEST_HAS_FLAG(config,
            D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |
            D_TEST_MSG_FLAG_COUNT_TESTS_FAIL) == true,
        "has_flag_multi_all_set",
        "HAS_FLAG should be true when all mask bits are set",
        _counter) && result;

    // test 3: multi-bit flag_mask - only some bits present
    result = d_assert_standalone(
        D_TEST_HAS_FLAG(config,
            D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |
            D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL) == false,
        "has_flag_multi_partial",
        "HAS_FLAG should be false when only some mask bits set",
        _counter) && result;

    // test 4: flag not set at all
    result = d_assert_standalone(
        D_TEST_HAS_FLAG(config, D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS) == false,
        "has_flag_not_set",
        "HAS_FLAG should be false when flag is not set",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_has_any_flag
  Tests the D_TEST_HAS_ANY_FLAG macro.
  Tests the following:
  - Returns true when any bit of flag_mask is set
  - Returns false when no bits of flag_mask are set
*/
bool
d_tests_sa_config_has_any_flag
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "has_any_flag_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: any match succeeds
    result = d_assert_standalone(
        D_TEST_HAS_ANY_FLAG(config,
            D_TEST_MSG_FLAG_COUNT_ASSERTS_FAIL |
            D_TEST_MSG_FLAG_COUNT_BLOCKS_FAIL) == true,
        "has_any_flag_partial_match",
        "HAS_ANY_FLAG should be true when any bit matches",
        _counter) && result;

    // test 2: no match fails
    result = d_assert_standalone(
        D_TEST_HAS_ANY_FLAG(config,
            D_TEST_MSG_FLAG_PRINT_ASSERTS_PASS |
            D_TEST_MSG_FLAG_PRINT_TESTS_PASS) == false,
        "has_any_flag_no_match",
        "HAS_ANY_FLAG should be false when no bits match",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_message_flags
  Tests the D_TEST_GET_MESSAGE_FLAGS macro.
  Tests the following:
  - Extracts only lower 16 bits
  - Strips upper settings bits
*/
bool
d_tests_sa_config_get_message_flags
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;
    uint32_t              msg_flags;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL |
                               D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_msg_flags_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    msg_flags = D_TEST_GET_MESSAGE_FLAGS(config);

    // test 1: extracts only message portion
    result = d_assert_standalone(
        msg_flags == D_TEST_MODE_NORMAL,
        "get_msg_flags_value",
        "GET_MESSAGE_FLAGS should extract only message portion",
        _counter) && result;

    // test 2: no settings bits in result
    result = d_assert_standalone(
        (msg_flags & D_TEST_MASK_SETTINGS_FLAGS) == 0,
        "get_msg_flags_no_settings",
        "GET_MESSAGE_FLAGS result should have no settings bits",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_get_settings_flags
  Tests the D_TEST_GET_SETTINGS_FLAGS macro.
  Tests the following:
  - Extracts only upper 16 bits
  - Strips lower message bits
*/
bool
d_tests_sa_config_get_settings_flags
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;
    uint32_t              settings_flags;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL |
                               D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "get_settings_flags_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    settings_flags = D_TEST_GET_SETTINGS_FLAGS(config);

    // test 1: extracts only settings portion
    result = d_assert_standalone(
        settings_flags == D_TEST_SETTINGS_STACK_PUSH_ALL,
        "get_settings_flags_value",
        "GET_SETTINGS_FLAGS should extract only settings portion",
        _counter) && result;

    // test 2: no message bits in result
    result = d_assert_standalone(
        (settings_flags & D_TEST_MASK_MESSAGE_FLAGS) == 0,
        "get_settings_flags_no_message",
        "GET_SETTINGS_FLAGS result should have no message bits",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_is_silent
  Tests the D_TEST_IS_SILENT macro.
  Tests the following:
  - True for silent mode
  - False for non-silent modes
  - True for settings-only (no message flags)
*/
bool
d_tests_sa_config_is_silent
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* silent;
    struct d_test_config* normal;
    struct d_test_config* settings_only;

    result = true;

    silent        = d_test_config_new(D_TEST_MODE_SILENT);
    normal        = d_test_config_new(D_TEST_MODE_NORMAL);
    settings_only = d_test_config_new(D_TEST_SETTINGS_STACK_PUSH_ALL);

    if ( (!silent) || (!normal) || (!settings_only) )
    {
        if (silent)        { d_test_config_free(silent); }
        if (normal)        { d_test_config_free(normal); }
        if (settings_only) { d_test_config_free(settings_only); }

        return d_assert_standalone(false,
                                   "is_silent_alloc",
                                   "Failed to allocate configs",
                                   _counter);
    }

    result = d_assert_standalone(
        D_TEST_IS_SILENT(silent) == true,
        "is_silent_true",
        "IS_SILENT should be true for silent mode",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_IS_SILENT(normal) == false,
        "is_silent_false_normal",
        "IS_SILENT should be false for normal mode",
        _counter) && result;

    // settings-only has no message flags, so IS_SILENT is true
    result = d_assert_standalone(
        D_TEST_IS_SILENT(settings_only) == true,
        "is_silent_settings_only",
        "IS_SILENT should be true when only settings set",
        _counter) && result;

    d_test_config_free(silent);
    d_test_config_free(normal);
    d_test_config_free(settings_only);

    return result;
}


/*
d_tests_sa_config_is_verbose
  Tests the D_TEST_IS_VERBOSE macro.
  Tests the following:
  - True for verbose mode
  - False for non-verbose modes
*/
bool
d_tests_sa_config_is_verbose
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* verbose;
    struct d_test_config* normal;
    struct d_test_config* silent;

    result = true;

    verbose = d_test_config_new(D_TEST_MODE_VERBOSE);
    normal  = d_test_config_new(D_TEST_MODE_NORMAL);
    silent  = d_test_config_new(D_TEST_MODE_SILENT);

    if ( (!verbose) || (!normal) || (!silent) )
    {
        if (verbose) { d_test_config_free(verbose); }
        if (normal)  { d_test_config_free(normal); }
        if (silent)  { d_test_config_free(silent); }

        return d_assert_standalone(false,
                                   "is_verbose_alloc",
                                   "Failed to allocate configs",
                                   _counter);
    }

    result = d_assert_standalone(
        D_TEST_IS_VERBOSE(verbose) == true,
        "is_verbose_true",
        "IS_VERBOSE should be true for verbose mode",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_IS_VERBOSE(normal) == false,
        "is_verbose_false_normal",
        "IS_VERBOSE should be false for normal mode",
        _counter) && result;

    result = d_assert_standalone(
        D_TEST_IS_VERBOSE(silent) == false,
        "is_verbose_false_silent",
        "IS_VERBOSE should be false for silent mode",
        _counter) && result;

    d_test_config_free(verbose);
    d_test_config_free(normal);
    d_test_config_free(silent);

    return result;
}


/*
d_tests_sa_config_is_mode
  Tests the D_TEST_IS_MODE macro.
  Tests the following:
  - Returns true for exact mode match
  - Returns false for mode mismatch
  - Ignores settings flags when checking mode
*/
bool
d_tests_sa_config_is_mode
(
    struct d_test_counter* _counter
)
{
    bool                  result;
    struct d_test_config* config;

    result = true;
    config = d_test_config_new(D_TEST_MODE_NORMAL |
                               D_TEST_SETTINGS_STACK_PUSH_ALL);

    if (!config)
    {
        return d_assert_standalone(false,
                                   "is_mode_alloc",
                                   "Failed to allocate config",
                                   _counter);
    }

    // test 1: exact match (ignoring settings)
    result = d_assert_standalone(
        D_TEST_IS_MODE(config, D_TEST_MODE_NORMAL) == true,
        "is_mode_exact_match",
        "IS_MODE should match NORMAL even with settings",
        _counter) && result;

    // test 2: mismatch
    result = d_assert_standalone(
        D_TEST_IS_MODE(config, D_TEST_MODE_VERBOSE) == false,
        "is_mode_mismatch",
        "IS_MODE should not match VERBOSE for NORMAL config",
        _counter) && result;

    // test 3: silent mode mismatch
    result = d_assert_standalone(
        D_TEST_IS_MODE(config, D_TEST_MODE_SILENT) == false,
        "is_mode_silent_mismatch",
        "IS_MODE should not match SILENT for NORMAL config",
        _counter) && result;

    d_test_config_free(config);

    return result;
}


/*
d_tests_sa_config_utility_macro_all
  Aggregation function that runs all utility macro tests.
*/
bool
d_tests_sa_config_utility_macro_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Macros\n");
    printf("  --------------------------\n");

    result = d_tests_sa_config_has_flag(_counter) && result;
    result = d_tests_sa_config_has_any_flag(_counter) && result;
    result = d_tests_sa_config_get_message_flags(_counter) && result;
    result = d_tests_sa_config_get_settings_flags(_counter) && result;
    result = d_tests_sa_config_is_silent(_counter) && result;
    result = d_tests_sa_config_is_verbose(_counter) && result;
    result = d_tests_sa_config_is_mode(_counter) && result;

    return result;
}
