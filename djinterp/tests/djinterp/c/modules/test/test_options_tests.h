/*******************************************************************************
* djinterp [test]                                           test_options_tests.h
*
*   Unit test declarations for `test_options.h` module.
*   Provides comprehensive testing of all test_config types, macros, and
* functions including flag manipulation macros, mask definitions, enum values,
* message flag combinations, mode definitions, utility macros, semantic check
* macros, default values, core structures, constructors/destructors, typed
* getters, typed setters, and key lookup.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.03
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_MODULES_TEST_TEST_OPTIONS_TESTS_H
#define DJINTERP_TESTS_C_MODULES_TEST_TEST_OPTIONS_TESTS_H 1

// std
#include <stddef.h>
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_options.h"
#include "../../../../../inc/djinterp/c/modules/test/test_cvar.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. FLAG MANIPULATION MACRO TESTS
 *****************************************************************************/
// D_TEST_SETTINGS_SHIFT constant
bool d_tests_options_settings_shift(
    struct dj_test_suite* _suite
);
// D_TEST_SETTINGS_TO_FLAGS macro
bool d_tests_options_settings_to_flags(
    struct dj_test_suite* _suite
);
// D_TEST_FLAGS_TO_SETTINGS macro
bool d_tests_options_flags_to_settings(
    struct dj_test_suite* _suite
);
// round-trip conversion: TO_FLAGS then FLAGS_TO_SETTINGS
bool d_tests_options_flag_roundtrip(
    struct dj_test_suite* _suite
);

// I.   aggregation function
bool d_tests_options_flag_macro_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. MASK DEFINITION TESTS
 *****************************************************************************/
// D_TEST_MASK_MESSAGE_FLAGS
bool d_tests_options_mask_message(
    struct dj_test_suite* _suite
);
// D_TEST_MASK_SETTINGS_FLAGS
bool d_tests_options_mask_settings(
    struct dj_test_suite* _suite
);
// D_TEST_MASK_COUNTER_FLAGS
bool d_tests_options_mask_counter(
    struct dj_test_suite* _suite
);
// D_TEST_MASK_PRINT_FLAGS
bool d_tests_options_mask_print(
    struct dj_test_suite* _suite
);
// D_TEST_MASK_STACK_FLAGS
bool d_tests_options_mask_stack(
    struct dj_test_suite* _suite
);
// masks are mutually exclusive (no overlap between message and settings)
bool d_tests_options_mask_no_overlap(
    struct dj_test_suite* _suite
);

// II.  aggregation function
bool d_tests_options_mask_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. ENUM VALUE TESTS
 *****************************************************************************/
// DTestMessageFlag enum values
bool d_tests_options_enum_message_flag(
    struct dj_test_suite* _suite
);
// DTestSettingsFlag enum values
bool d_tests_options_enum_settings_flag(
    struct dj_test_suite* _suite
);
// DTestConfigKey enum values
bool d_tests_options_enum_config_key(
    struct dj_test_suite* _suite
);
// DTestMetadataFlag enum values
bool d_tests_options_enum_metadata_flag(
    struct dj_test_suite* _suite
);
// DTestEvent enum values
bool d_tests_options_enum_event(
    struct dj_test_suite* _suite
);

// III. aggregation function
bool d_tests_options_enum_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. MESSAGE FLAG COMBINATION TESTS
 *****************************************************************************/
// D_TEST_MSG_COUNT_FAIL_ALL, D_TEST_MSG_COUNT_PASS_ALL, D_TEST_MSG_COUNT_ALL
bool d_tests_options_msg_count_combinations(
    struct dj_test_suite* _suite
);
// D_TEST_MSG_PRINT_FAIL_ALL, D_TEST_MSG_PRINT_PASS_ALL, D_TEST_MSG_PRINT_ALL
bool d_tests_options_msg_print_combinations(
    struct dj_test_suite* _suite
);
// D_TEST_MSG_ALL
bool d_tests_options_msg_all(
    struct dj_test_suite* _suite
);
// category-specific combinations (asserts, tests, blocks, modules)
bool d_tests_options_msg_category_combinations(
    struct dj_test_suite* _suite
);

// IV.  aggregation function
bool d_tests_options_combination_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. MODE DEFINITION AND PRESET TESTS
 *****************************************************************************/
// D_TEST_MODE_SILENT
bool d_tests_options_mode_silent(
    struct dj_test_suite* _suite
);
// D_TEST_MODE_MINIMAL
bool d_tests_options_mode_minimal(
    struct dj_test_suite* _suite
);
// D_TEST_MODE_NORMAL
bool d_tests_options_mode_normal(
    struct dj_test_suite* _suite
);
// D_TEST_MODE_VERBOSE
bool d_tests_options_mode_verbose(
    struct dj_test_suite* _suite
);
// D_TEST_OPTIONS_PRESET_* aliases match corresponding modes
bool d_tests_options_presets(
    struct dj_test_suite* _suite
);
// D_TEST_SETTINGS_STACK_PUSH_ALL
bool d_tests_options_settings_stack_push_all(
    struct dj_test_suite* _suite
);

// V.   aggregation function
bool d_tests_options_mode_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. UTILITY MACRO TESTS
 *****************************************************************************/
// D_TEST_HAS_FLAG macro
bool d_tests_options_has_flag(
    struct dj_test_suite* _suite
);
// D_TEST_HAS_ANY_FLAG macro
bool d_tests_options_has_any_flag(
    struct dj_test_suite* _suite
);
// D_TEST_GET_MESSAGE_FLAGS macro
bool d_tests_options_get_message_flags(
    struct dj_test_suite* _suite
);
// D_TEST_GET_SETTINGS_FLAGS macro
bool d_tests_options_get_settings_flags(
    struct dj_test_suite* _suite
);
// D_TEST_IS_SILENT macro
bool d_tests_options_is_silent(
    struct dj_test_suite* _suite
);
// D_TEST_IS_VERBOSE macro
bool d_tests_options_is_verbose(
    struct dj_test_suite* _suite
);
// D_TEST_IS_MODE macro
bool d_tests_options_is_mode(
    struct dj_test_suite* _suite
);

// VI.  aggregation function
bool d_tests_options_utility_macro_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. SEMANTIC CHECK MACRO TESTS
 *****************************************************************************/
// counter semantic checks (SHOULD_COUNT_*)
bool d_tests_options_semantic_count(
    struct dj_test_suite* _suite
);
// print semantic checks (SHOULD_PRINT_*)
bool d_tests_options_semantic_print(
    struct dj_test_suite* _suite
);
// settings semantic checks (SHOULD_PUSH_*)
bool d_tests_options_semantic_push(
    struct dj_test_suite* _suite
);

// VII. aggregation function
bool d_tests_options_semantic_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. DEFAULT VALUE TESTS
 *****************************************************************************/
// D_TEST_DEFAULT_INDENT
bool d_tests_options_default_indent(
    struct dj_test_suite* _suite
);
// D_TEST_DEFAULT_MAX_INDENT
bool d_tests_options_default_max_indent(
    struct dj_test_suite* _suite
);
// D_TEST_DEFAULT_MAX_FAILURES
bool d_tests_options_default_max_failures(
    struct dj_test_suite* _suite
);
// D_TEST_DEFAULT_TIMEOUT
bool d_tests_options_default_timeout(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_options_default_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IX. CORE STRUCTURE TESTS
 *****************************************************************************/
// dj_test_config structure members
bool d_tests_options_struct_members(
    struct dj_test_suite* _suite
);
// dj_test_config size and layout
bool d_tests_options_struct_size(
    struct dj_test_suite* _suite
);

// IX.  aggregation function
bool d_tests_options_struct_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * X. CONSTRUCTOR AND DESTRUCTOR TESTS
 *****************************************************************************/
// d_test_options_new function
bool d_tests_options_new(
    struct dj_test_suite* _suite
);
// d_test_options_new_preset function
bool d_tests_options_new_preset(
    struct dj_test_suite* _suite
);
// d_test_options_new_copy function
bool d_tests_options_new_copy(
    struct dj_test_suite* _suite
);
// d_test_options_new_copy with NULL
bool d_tests_options_new_copy_null(
    struct dj_test_suite* _suite
);
// d_test_options_free function
bool d_tests_options_free(
    struct dj_test_suite* _suite
);

// X.   aggregation function
bool d_tests_options_lifecycle_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * XI. GETTER FUNCTION TESTS
 *****************************************************************************/
// d_test_options_get_bool function
bool d_tests_options_get_bool(
    struct dj_test_suite* _suite
);
// d_test_options_get_size_t function
bool d_tests_options_get_size_t(
    struct dj_test_suite* _suite
);
// d_test_options_get_int32 function
bool d_tests_options_get_int32(
    struct dj_test_suite* _suite
);
// d_test_options_get_uint32 function
bool d_tests_options_get_uint32(
    struct dj_test_suite* _suite
);
// d_test_options_get_string function
bool d_tests_options_get_string(
    struct dj_test_suite* _suite
);
// d_test_options_get_ptr function
bool d_tests_options_get_ptr(
    struct dj_test_suite* _suite
);
// getters with NULL config (schema defaults)
bool d_tests_options_get_null_config(
    struct dj_test_suite* _suite
);

// XI.  aggregation function
bool d_tests_options_getter_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * XII. SETTER FUNCTION TESTS
 *****************************************************************************/
// d_test_options_set_bool function
bool d_tests_options_set_bool(
    struct dj_test_suite* _suite
);
// d_test_options_set_size_t function
bool d_tests_options_set_size_t(
    struct dj_test_suite* _suite
);
// d_test_options_set_int32 function
bool d_tests_options_set_int32(
    struct dj_test_suite* _suite
);
// d_test_options_set_uint32 function
bool d_tests_options_set_uint32(
    struct dj_test_suite* _suite
);
// d_test_options_set_string function
bool d_tests_options_set_string(
    struct dj_test_suite* _suite
);
// d_test_options_set_ptr function
bool d_tests_options_set_ptr(
    struct dj_test_suite* _suite
);
// setters with NULL config
bool d_tests_options_set_null_config(
    struct dj_test_suite* _suite
);
// setters with invalid keys (type mismatch)
bool d_tests_options_set_type_mismatch(
    struct dj_test_suite* _suite
);

// XII. aggregation function
bool d_tests_options_setter_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * XIII. KEY LOOKUP TESTS
 *****************************************************************************/
// d_test_options_key_from_string with valid keys
bool d_tests_options_key_from_string_valid(
    struct dj_test_suite* _suite
);
// d_test_options_key_from_string with NULL
bool d_tests_options_key_from_string_null(
    struct dj_test_suite* _suite
);
// d_test_options_key_from_string with invalid/metadata keys
bool d_tests_options_key_from_string_invalid(
    struct dj_test_suite* _suite
);
// d_test_options_key_from_string: aliases return INVALID (primary keys only)
bool d_tests_options_key_from_string_alias(
    struct dj_test_suite* _suite
);

// XIII. aggregation function
bool d_tests_options_key_lookup_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_options_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_MODULES_TEST_TEST_OPTIONS_TESTS_H
