/*******************************************************************************
* djinterp [c][test]                                        test_options_tests.h
*
* test_options.h's suite, written against DTest's C face (test_suite.h).
*   One section a block, in test_options.h's order: the flag macros, masks,
* enums, message and category combinations, modes and presets, utility and
* semantic macros, defaults, dj_test_options, its creation and destruction, its
* getters and setters, and key lookup. d_tests_options_run_all runs the
* thirteen in order.
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.03
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FLAG MANIPULATION MACRO TESTS
    -----------------------------
2.  MASK DEFINITION TESTS
    ---------------------
3.  ENUM VALUE TESTS
    ----------------
4.  MESSAGE FLAG COMBINATION TESTS
    ------------------------------
5.  MODE DEFINITION AND PRESET TESTS
    --------------------------------
6.  UTILITY MACRO TESTS
    -------------------
7.  SEMANTIC CHECK MACRO TESTS
    --------------------------
8.  DEFAULT VALUE TESTS
    -------------------
9.  CORE STRUCTURE TESTS
    --------------------
10. CONSTRUCTOR AND DESTRUCTOR TESTS
    --------------------------------
11. GETTER FUNCTION TESTS
    ---------------------
12. SETTER FUNCTION TESTS
    ---------------------
13. KEY LOOKUP TESTS
    ----------------
14. MODULE-LEVEL AGGREGATION
    ------------------------
*/

#ifndef DJINTERP_TESTS_C_TEST_OPTIONS_TESTS_H
#define DJINTERP_TESTS_C_TEST_OPTIONS_TESTS_H 1

// std
#include <stddef.h>  // size_t
#include <stdlib.h>  // free
// djinterp
#include "../../../../../inc/djinterp/c/modules/test/test_options.h"  // the module under test
#include "../../../../../inc/djinterp/c/modules/test/test_cvar.h"     // the registry it reads
#include "../../../../../inc/djinterp/c/string_fn.h"                  // d_strcasecmp, d_strnlen
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"    // DTest's C face
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint16_t, uint32_t


//==============================================================================
// 1.  FLAG MANIPULATION MACRO TESTS
//==============================================================================


// D_TEST_SETTINGS_SHIFT, D_TEST_SETTINGS_TO_FLAGS,
// D_TEST_FLAGS_TO_SETTINGS, and the round trip through the last two
bool d_tests_options_settings_shift(struct dj_test_suite* _suite);

bool d_tests_options_settings_to_flags(struct dj_test_suite* _suite);

bool d_tests_options_flags_to_settings(struct dj_test_suite* _suite);

bool d_tests_options_flag_roundtrip(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_flag_macro_all(struct dj_test_suite* _suite);


//==============================================================================
// 2.  MASK DEFINITION TESTS
//==============================================================================


// each D_TEST_MASK_*, and that the message and settings masks do not
// overlap
bool d_tests_options_mask_message(struct dj_test_suite* _suite);

bool d_tests_options_mask_settings(struct dj_test_suite* _suite);

bool d_tests_options_mask_counter(struct dj_test_suite* _suite);

bool d_tests_options_mask_print(struct dj_test_suite* _suite);

bool d_tests_options_mask_stack(struct dj_test_suite* _suite);

bool d_tests_options_mask_no_overlap(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_mask_all(struct dj_test_suite* _suite);


//==============================================================================
// 3.  ENUM VALUE TESTS
//==============================================================================


// the values of DTestMessageFlag, DTestSettingsFlag, DTestConfigKey,
// DTestMetadataFlag and DTestEvent
bool d_tests_options_enum_message_flag(struct dj_test_suite* _suite);

bool d_tests_options_enum_settings_flag(struct dj_test_suite* _suite);

bool d_tests_options_enum_config_key(struct dj_test_suite* _suite);

bool d_tests_options_enum_metadata_flag(struct dj_test_suite* _suite);

bool d_tests_options_enum_event(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_enum_all(struct dj_test_suite* _suite);


//==============================================================================
// 4.  MESSAGE FLAG COMBINATION TESTS
//==============================================================================


// the D_TEST_MSG_COUNT_*, D_TEST_MSG_PRINT_* and D_TEST_MSG_ALL
// combinations, and those for asserts, tests, blocks and modules
bool d_tests_options_msg_count_combinations(struct dj_test_suite* _suite);

bool d_tests_options_msg_print_combinations(struct dj_test_suite* _suite);

bool d_tests_options_msg_category_combinations(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_msg_all(struct dj_test_suite* _suite);

bool d_tests_options_combination_all(struct dj_test_suite* _suite);


//==============================================================================
// 5.  MODE DEFINITION AND PRESET TESTS
//==============================================================================


// the D_TEST_MODE_* modes, the D_TEST_OPTIONS_PRESET_* aliases of them,
// and D_TEST_SETTINGS_STACK_PUSH_ALL
bool d_tests_options_mode_silent(struct dj_test_suite* _suite);

bool d_tests_options_mode_minimal(struct dj_test_suite* _suite);

bool d_tests_options_mode_normal(struct dj_test_suite* _suite);

bool d_tests_options_mode_verbose(struct dj_test_suite* _suite);

bool d_tests_options_presets(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_settings_stack_push_all(struct dj_test_suite* _suite);

bool d_tests_options_mode_all(struct dj_test_suite* _suite);


//==============================================================================
// 6.  UTILITY MACRO TESTS
//==============================================================================


// D_TEST_HAS_FLAG, _HAS_ANY_FLAG, _GET_MESSAGE_FLAGS,
// _GET_SETTINGS_FLAGS, _IS_SILENT, _IS_VERBOSE and _IS_MODE
bool d_tests_options_has_flag(struct dj_test_suite* _suite);

bool d_tests_options_has_any_flag(struct dj_test_suite* _suite);

bool d_tests_options_get_message_flags(struct dj_test_suite* _suite);

bool d_tests_options_get_settings_flags(struct dj_test_suite* _suite);

bool d_tests_options_is_silent(struct dj_test_suite* _suite);

bool d_tests_options_is_verbose(struct dj_test_suite* _suite);

bool d_tests_options_is_mode(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_utility_macro_all(struct dj_test_suite* _suite);


//==============================================================================
// 7.  SEMANTIC CHECK MACRO TESTS
//==============================================================================


// the SHOULD_COUNT_*, SHOULD_PRINT_* and SHOULD_PUSH_* checks
bool d_tests_options_semantic_count(struct dj_test_suite* _suite);

bool d_tests_options_semantic_print(struct dj_test_suite* _suite);

bool d_tests_options_semantic_push(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_semantic_all(struct dj_test_suite* _suite);


//==============================================================================
// 8.  DEFAULT VALUE TESTS
//==============================================================================


// D_TEST_DEFAULT_INDENT, _MAX_INDENT, _MAX_FAILURES and _TIMEOUT
bool d_tests_options_default_indent(struct dj_test_suite* _suite);

bool d_tests_options_default_max_indent(struct dj_test_suite* _suite);

bool d_tests_options_default_max_failures(struct dj_test_suite* _suite);

bool d_tests_options_default_timeout(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_default_all(struct dj_test_suite* _suite);


//==============================================================================
// 9.  CORE STRUCTURE TESTS
//==============================================================================


// dj_test_options' members, its size and its layout
bool d_tests_options_struct_members(struct dj_test_suite* _suite);

bool d_tests_options_struct_size(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_struct_all(struct dj_test_suite* _suite);


//==============================================================================
// 10. CONSTRUCTOR AND DESTRUCTOR TESTS
//==============================================================================


// d_test_options_new, _new_preset, _new_copy (of NULL too) and _free
bool d_tests_options_new(struct dj_test_suite* _suite);

bool d_tests_options_new_preset(struct dj_test_suite* _suite);

bool d_tests_options_new_copy(struct dj_test_suite* _suite);

bool d_tests_options_new_copy_null(struct dj_test_suite* _suite);

bool d_tests_options_free(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_lifecycle_all(struct dj_test_suite* _suite);


//==============================================================================
// 11. GETTER FUNCTION TESTS
//==============================================================================


// each d_test_options_get_* getter, and the getters on a NULL config,
// which read the schema's defaults
bool d_tests_options_get_bool(struct dj_test_suite* _suite);

bool d_tests_options_get_size_t(struct dj_test_suite* _suite);

bool d_tests_options_get_int32(struct dj_test_suite* _suite);

bool d_tests_options_get_uint32(struct dj_test_suite* _suite);

bool d_tests_options_get_string(struct dj_test_suite* _suite);

bool d_tests_options_get_ptr(struct dj_test_suite* _suite);

bool d_tests_options_get_null_config(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_getter_all(struct dj_test_suite* _suite);


//==============================================================================
// 12. SETTER FUNCTION TESTS
//==============================================================================


// each d_test_options_set_* setter, on a NULL config, and with a key
// of the wrong type
bool d_tests_options_set_bool(struct dj_test_suite* _suite);

bool d_tests_options_set_size_t(struct dj_test_suite* _suite);

bool d_tests_options_set_int32(struct dj_test_suite* _suite);

bool d_tests_options_set_uint32(struct dj_test_suite* _suite);

bool d_tests_options_set_string(struct dj_test_suite* _suite);

bool d_tests_options_set_ptr(struct dj_test_suite* _suite);

bool d_tests_options_set_null_config(struct dj_test_suite* _suite);

bool d_tests_options_set_type_mismatch(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_setter_all(struct dj_test_suite* _suite);


//==============================================================================
// 13. KEY LOOKUP TESTS
//==============================================================================


// d_test_options_key_from_string: config keys and their aliases resolve;
// NULL, unknown, empty and metadata keys do not
bool d_tests_options_key_from_string_valid(struct dj_test_suite* _suite);

bool d_tests_options_key_from_string_null(struct dj_test_suite* _suite);

bool d_tests_options_key_from_string_invalid(struct dj_test_suite* _suite);

bool d_tests_options_key_from_string_alias(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_options_key_lookup_all(struct dj_test_suite* _suite);


//==============================================================================
// 14. MODULE-LEVEL AGGREGATION
//==============================================================================


// every section's aggregate, in order
bool d_tests_options_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_TESTS_C_TEST_OPTIONS_TESTS_H
