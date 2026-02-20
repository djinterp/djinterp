/******************************************************************************
* djinterp [test]                                      test_config_tests_sa.h
*
*   Unit test declarations for `test_config.h` module.
*   Provides comprehensive testing of all test_config types, macros, and
* functions including flag manipulation macros, mask definitions, enum values,
* message flag combinations, mode definitions, utility macros, semantic check
* macros, default values, core structures, constructors/destructors, typed
* getters, typed setters, and key lookup.
*
*
* path:      \tests\test\test_config_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.03
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_CONFIG_SA_
#define DJINTERP_TESTS_TEST_CONFIG_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\inc\test\test_standalone.h"
#include "..\..\inc\test\test_config.h"
#include "..\..\inc\test\test_cvar.h"
#include "..\..\inc\string_fn.h"


/******************************************************************************
 * I. FLAG MANIPULATION MACRO TESTS
 *****************************************************************************/
// D_TEST_SETTINGS_SHIFT constant
bool d_tests_sa_config_settings_shift(struct d_test_counter* _counter);
// D_TEST_SETTINGS_TO_FLAGS macro
bool d_tests_sa_config_settings_to_flags(struct d_test_counter* _counter);
// D_TEST_FLAGS_TO_SETTINGS macro
bool d_tests_sa_config_flags_to_settings(struct d_test_counter* _counter);
// round-trip conversion: TO_FLAGS then FLAGS_TO_SETTINGS
bool d_tests_sa_config_flag_roundtrip(struct d_test_counter* _counter);

// I.   aggregation function
bool d_tests_sa_config_flag_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. MASK DEFINITION TESTS
 *****************************************************************************/
// D_TEST_MASK_MESSAGE_FLAGS
bool d_tests_sa_config_mask_message(struct d_test_counter* _counter);
// D_TEST_MASK_SETTINGS_FLAGS
bool d_tests_sa_config_mask_settings(struct d_test_counter* _counter);
// D_TEST_MASK_COUNTER_FLAGS
bool d_tests_sa_config_mask_counter(struct d_test_counter* _counter);
// D_TEST_MASK_PRINT_FLAGS
bool d_tests_sa_config_mask_print(struct d_test_counter* _counter);
// D_TEST_MASK_STACK_FLAGS
bool d_tests_sa_config_mask_stack(struct d_test_counter* _counter);
// masks are mutually exclusive (no overlap between message and settings)
bool d_tests_sa_config_mask_no_overlap(struct d_test_counter* _counter);

// II.  aggregation function
bool d_tests_sa_config_mask_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. ENUM VALUE TESTS
 *****************************************************************************/
// DTestMessageFlag enum values
bool d_tests_sa_config_enum_message_flag(struct d_test_counter* _counter);
// DTestSettingsFlag enum values
bool d_tests_sa_config_enum_settings_flag(struct d_test_counter* _counter);
// DTestConfigKey enum values
bool d_tests_sa_config_enum_config_key(struct d_test_counter* _counter);
// DTestMetadataFlag enum values
bool d_tests_sa_config_enum_metadata_flag(struct d_test_counter* _counter);
// DTestEvent enum values
bool d_tests_sa_config_enum_event(struct d_test_counter* _counter);

// III. aggregation function
bool d_tests_sa_config_enum_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. MESSAGE FLAG COMBINATION TESTS
 *****************************************************************************/
// D_TEST_MSG_COUNT_FAIL_ALL, D_TEST_MSG_COUNT_PASS_ALL, D_TEST_MSG_COUNT_ALL
bool d_tests_sa_config_msg_count_combinations(struct d_test_counter* _counter);
// D_TEST_MSG_PRINT_FAIL_ALL, D_TEST_MSG_PRINT_PASS_ALL, D_TEST_MSG_PRINT_ALL
bool d_tests_sa_config_msg_print_combinations(struct d_test_counter* _counter);
// D_TEST_MSG_ALL
bool d_tests_sa_config_msg_all(struct d_test_counter* _counter);
// category-specific combinations (asserts, tests, blocks, modules)
bool d_tests_sa_config_msg_category_combinations(struct d_test_counter* _counter);

// IV.  aggregation function
bool d_tests_sa_config_combination_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. MODE DEFINITION AND PRESET TESTS
 *****************************************************************************/
// D_TEST_MODE_SILENT
bool d_tests_sa_config_mode_silent(struct d_test_counter* _counter);
// D_TEST_MODE_MINIMAL
bool d_tests_sa_config_mode_minimal(struct d_test_counter* _counter);
// D_TEST_MODE_NORMAL
bool d_tests_sa_config_mode_normal(struct d_test_counter* _counter);
// D_TEST_MODE_VERBOSE
bool d_tests_sa_config_mode_verbose(struct d_test_counter* _counter);
// D_TEST_CONFIG_PRESET_* aliases match corresponding modes
bool d_tests_sa_config_presets(struct d_test_counter* _counter);
// D_TEST_SETTINGS_STACK_PUSH_ALL
bool d_tests_sa_config_settings_stack_push_all(struct d_test_counter* _counter);

// V.   aggregation function
bool d_tests_sa_config_mode_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. UTILITY MACRO TESTS
 *****************************************************************************/
// D_TEST_HAS_FLAG macro
bool d_tests_sa_config_has_flag(struct d_test_counter* _counter);
// D_TEST_HAS_ANY_FLAG macro
bool d_tests_sa_config_has_any_flag(struct d_test_counter* _counter);
// D_TEST_GET_MESSAGE_FLAGS macro
bool d_tests_sa_config_get_message_flags(struct d_test_counter* _counter);
// D_TEST_GET_SETTINGS_FLAGS macro
bool d_tests_sa_config_get_settings_flags(struct d_test_counter* _counter);
// D_TEST_IS_SILENT macro
bool d_tests_sa_config_is_silent(struct d_test_counter* _counter);
// D_TEST_IS_VERBOSE macro
bool d_tests_sa_config_is_verbose(struct d_test_counter* _counter);
// D_TEST_IS_MODE macro
bool d_tests_sa_config_is_mode(struct d_test_counter* _counter);

// VI.  aggregation function
bool d_tests_sa_config_utility_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. SEMANTIC CHECK MACRO TESTS
 *****************************************************************************/
// counter semantic checks (SHOULD_COUNT_*)
bool d_tests_sa_config_semantic_count(struct d_test_counter* _counter);
// print semantic checks (SHOULD_PRINT_*)
bool d_tests_sa_config_semantic_print(struct d_test_counter* _counter);
// settings semantic checks (SHOULD_PUSH_*)
bool d_tests_sa_config_semantic_push(struct d_test_counter* _counter);
// legacy aliases (SHOULD_STACK_PUSH_*)
bool d_tests_sa_config_semantic_legacy_aliases(struct d_test_counter* _counter);

// VII. aggregation function
bool d_tests_sa_config_semantic_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. DEFAULT VALUE TESTS
 *****************************************************************************/
// D_TEST_DEFAULT_INDENT
bool d_tests_sa_config_default_indent(struct d_test_counter* _counter);
// D_TEST_DEFAULT_MAX_INDENT
bool d_tests_sa_config_default_max_indent(struct d_test_counter* _counter);
// D_TEST_DEFAULT_MAX_FAILURES
bool d_tests_sa_config_default_max_failures(struct d_test_counter* _counter);
// D_TEST_DEFAULT_TIMEOUT
bool d_tests_sa_config_default_timeout(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_config_default_all(struct d_test_counter* _counter);


/******************************************************************************
 * IX. CORE STRUCTURE TESTS
 *****************************************************************************/
// d_test_config structure members
bool d_tests_sa_config_struct_members(struct d_test_counter* _counter);
// d_test_config size and layout
bool d_tests_sa_config_struct_size(struct d_test_counter* _counter);

// IX.  aggregation function
bool d_tests_sa_config_struct_all(struct d_test_counter* _counter);


/******************************************************************************
 * X. CONSTRUCTOR AND DESTRUCTOR TESTS
 *****************************************************************************/
// d_test_config_new function
bool d_tests_sa_config_new(struct d_test_counter* _counter);
// d_test_config_new_preset function
bool d_tests_sa_config_new_preset(struct d_test_counter* _counter);
// d_test_config_new_copy function
bool d_tests_sa_config_new_copy(struct d_test_counter* _counter);
// d_test_config_new_copy with NULL
bool d_tests_sa_config_new_copy_null(struct d_test_counter* _counter);
// d_test_config_free function
bool d_tests_sa_config_free(struct d_test_counter* _counter);

// X.   aggregation function
bool d_tests_sa_config_lifecycle_all(struct d_test_counter* _counter);


/******************************************************************************
 * XI. GETTER FUNCTION TESTS
 *****************************************************************************/
// d_test_config_get_bool function
bool d_tests_sa_config_get_bool(struct d_test_counter* _counter);
// d_test_config_get_size_t function
bool d_tests_sa_config_get_size_t(struct d_test_counter* _counter);
// d_test_config_get_int32 function
bool d_tests_sa_config_get_int32(struct d_test_counter* _counter);
// d_test_config_get_uint32 function
bool d_tests_sa_config_get_uint32(struct d_test_counter* _counter);
// d_test_config_get_string function
bool d_tests_sa_config_get_string(struct d_test_counter* _counter);
// d_test_config_get_ptr function
bool d_tests_sa_config_get_ptr(struct d_test_counter* _counter);
// getters with NULL config (schema defaults)
bool d_tests_sa_config_get_null_config(struct d_test_counter* _counter);

// XI.  aggregation function
bool d_tests_sa_config_getter_all(struct d_test_counter* _counter);


/******************************************************************************
 * XII. SETTER FUNCTION TESTS
 *****************************************************************************/
// d_test_config_set_bool function
bool d_tests_sa_config_set_bool(struct d_test_counter* _counter);
// d_test_config_set_size_t function
bool d_tests_sa_config_set_size_t(struct d_test_counter* _counter);
// d_test_config_set_int32 function
bool d_tests_sa_config_set_int32(struct d_test_counter* _counter);
// d_test_config_set_uint32 function
bool d_tests_sa_config_set_uint32(struct d_test_counter* _counter);
// d_test_config_set_string function
bool d_tests_sa_config_set_string(struct d_test_counter* _counter);
// d_test_config_set_ptr function
bool d_tests_sa_config_set_ptr(struct d_test_counter* _counter);
// setters with NULL config
bool d_tests_sa_config_set_null_config(struct d_test_counter* _counter);
// setters with invalid keys (type mismatch)
bool d_tests_sa_config_set_type_mismatch(struct d_test_counter* _counter);

// XII. aggregation function
bool d_tests_sa_config_setter_all(struct d_test_counter* _counter);


/******************************************************************************
 * XIII. KEY LOOKUP TESTS
 *****************************************************************************/
// d_test_config_key_from_string with valid keys
bool d_tests_sa_config_key_from_string_valid(struct d_test_counter* _counter);
// d_test_config_key_from_string with NULL
bool d_tests_sa_config_key_from_string_null(struct d_test_counter* _counter);
// d_test_config_key_from_string with invalid/metadata keys
bool d_tests_sa_config_key_from_string_invalid(struct d_test_counter* _counter);
// d_test_config_key_from_string: aliases return INVALID (primary keys only)
bool d_tests_sa_config_key_from_string_alias(struct d_test_counter* _counter);

// XIII. aggregation function
bool d_tests_sa_config_key_lookup_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/
bool d_tests_sa_config_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_TEST_CONFIG_SA_
