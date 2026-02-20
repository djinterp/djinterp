/******************************************************************************
* djinterp [test]                                         test_cvar_tests_sa.h
*
*   Unit test declarations for `test_cvar.h` module.
*   Provides comprehensive testing of the registry-based configuration and
* metadata schema including: registry row flags, value union, row structure,
* initialization, row find functions, alias lookup, value get/set, reset
* functions, argument validation, typed access macros, predicate functions,
* registry table integrity, and default value helpers.
*
*
* path:      \tests\test\test_cvar_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.03
******************************************************************************/

#ifndef DJINTERP_TESTS_TEST_CVAR_SA_
#define DJINTERP_TESTS_TEST_CVAR_SA_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "..\..\inc\test\test_standalone.h"
#include "..\..\inc\test\test_cvar.h"
#include "..\..\inc\string_fn.h"


/******************************************************************************
 * I. REGISTRY ROW FLAG TESTS
 *****************************************************************************/
// DTestRegistryRowFlag: IS_REQUIRED is bit 0
bool d_tests_sa_cvar_row_flag_is_required(struct d_test_counter* _counter);
// DTestRegistryRowFlag: IS_CONFIG is bit 1
bool d_tests_sa_cvar_row_flag_is_config(struct d_test_counter* _counter);
// DTestRegistryRowFlag: IS_METADATA is bit 2
bool d_tests_sa_cvar_row_flag_is_metadata(struct d_test_counter* _counter);
// DTestRegistryRowFlag: all flags are unique powers of 2
bool d_tests_sa_cvar_row_flag_unique_bits(struct d_test_counter* _counter);
// DTestRegistryRowFlag: no overlapping bits between any two flags
bool d_tests_sa_cvar_row_flag_no_overlap(struct d_test_counter* _counter);

// I.    aggregation function
bool d_tests_sa_cvar_row_flag_all(struct d_test_counter* _counter);


/******************************************************************************
 * II. VALUE UNION TESTS
 *****************************************************************************/
// d_test_value: pointer member accessible
bool d_tests_sa_cvar_value_union_ptr(struct d_test_counter* _counter);
// d_test_value: size_t member accessible
bool d_tests_sa_cvar_value_union_size_t(struct d_test_counter* _counter);
// d_test_value: uint32_t member accessible
bool d_tests_sa_cvar_value_union_uint32(struct d_test_counter* _counter);
// d_test_value: uint16_t member accessible
bool d_tests_sa_cvar_value_union_uint16(struct d_test_counter* _counter);
// d_test_value: int32_t member accessible
bool d_tests_sa_cvar_value_union_int32(struct d_test_counter* _counter);
// d_test_value: bool member accessible
bool d_tests_sa_cvar_value_union_bool(struct d_test_counter* _counter);

// II.   aggregation function
bool d_tests_sa_cvar_value_union_all(struct d_test_counter* _counter);


/******************************************************************************
 * III. ROW STRUCTURE TESTS
 *****************************************************************************/
// d_test_registry_row: key member is accessible and first
bool d_tests_sa_cvar_row_struct_key(struct d_test_counter* _counter);
// d_test_registry_row: flag member is accessible
bool d_tests_sa_cvar_row_struct_flag(struct d_test_counter* _counter);
// d_test_registry_row: command_flags member is accessible
bool d_tests_sa_cvar_row_struct_command_flags(struct d_test_counter* _counter);
// d_test_registry_row: value_type member is accessible
bool d_tests_sa_cvar_row_struct_value_type(struct d_test_counter* _counter);
// d_test_registry_row: value and help members accessible
bool d_tests_sa_cvar_row_struct_value_help(struct d_test_counter* _counter);

// III.  aggregation function
bool d_tests_sa_cvar_row_struct_all(struct d_test_counter* _counter);


/******************************************************************************
 * IV. INITIALIZATION AND REGISTRY ACCESS TESTS
 *****************************************************************************/
// d_test_registry_init: does not crash
bool d_tests_sa_cvar_init_safe(struct d_test_counter* _counter);
// d_test_registry_init: idempotent (double init)
bool d_tests_sa_cvar_init_idempotent(struct d_test_counter* _counter);
// d_test_registry_registry: returns non-NULL
bool d_tests_sa_cvar_registry_non_null(struct d_test_counter* _counter);
// d_test_registry_registry: has expected row count
bool d_tests_sa_cvar_registry_row_count(struct d_test_counter* _counter);
// d_test_registry_registry: has static rows flag
bool d_tests_sa_cvar_registry_static_flag(struct d_test_counter* _counter);

// IV.   aggregation function
bool d_tests_sa_cvar_init_all(struct d_test_counter* _counter);


/******************************************************************************
 * V. ROW FIND TESTS
 *****************************************************************************/
// d_test_registry_find: valid key returns non-NULL
bool d_tests_sa_cvar_find_valid_key(struct d_test_counter* _counter);
// d_test_registry_find: returned row has matching key
bool d_tests_sa_cvar_find_key_matches(struct d_test_counter* _counter);
// d_test_registry_find: NULL key returns NULL
bool d_tests_sa_cvar_find_null_key(struct d_test_counter* _counter);
// d_test_registry_find: invalid key returns NULL
bool d_tests_sa_cvar_find_invalid_key(struct d_test_counter* _counter);
// d_test_registry_find_by_flag: valid flag returns correct row
bool d_tests_sa_cvar_find_by_flag_valid(struct d_test_counter* _counter);
// d_test_registry_find_by_flag: invalid flag returns NULL
bool d_tests_sa_cvar_find_by_flag_invalid(struct d_test_counter* _counter);

// V.    aggregation function
bool d_tests_sa_cvar_find_all(struct d_test_counter* _counter);


/******************************************************************************
 * VI. ALIAS LOOKUP TESTS
 *****************************************************************************/
// alias: "enabled" resolves to "config-enabled" row
bool d_tests_sa_cvar_alias_enabled(struct d_test_counter* _counter);
// alias: "indent" resolves to "indent-string" row
bool d_tests_sa_cvar_alias_indent(struct d_test_counter* _counter);
// alias: "indent-max" and "indent-level" resolve to "max-indent" row
bool d_tests_sa_cvar_alias_max_indent(struct d_test_counter* _counter);
// alias: "timeout-ms" resolves to "timeout" row
bool d_tests_sa_cvar_alias_timeout(struct d_test_counter* _counter);
// alias: "framework", "module", "submodule" resolve correctly
bool d_tests_sa_cvar_alias_name_shortcuts(struct d_test_counter* _counter);

// VI.   aggregation function
bool d_tests_sa_cvar_alias_all(struct d_test_counter* _counter);


/******************************************************************************
 * VII. VALUE GET/SET TESTS
 *****************************************************************************/
// d_test_registry_get: returns default value initially
bool d_tests_sa_cvar_get_default_value(struct d_test_counter* _counter);
// d_test_registry_set: changes value and get reads it back
bool d_tests_sa_cvar_set_then_get(struct d_test_counter* _counter);
// d_test_registry_set: invalid flag returns false
bool d_tests_sa_cvar_set_invalid_flag(struct d_test_counter* _counter);
// d_test_registry_get: invalid flag returns zeroed value
bool d_tests_sa_cvar_get_invalid_flag(struct d_test_counter* _counter);
// d_test_registry_set: set bool then get bool roundtrip
bool d_tests_sa_cvar_set_get_bool(struct d_test_counter* _counter);

// VII.  aggregation function
bool d_tests_sa_cvar_get_set_all(struct d_test_counter* _counter);


/******************************************************************************
 * VIII. RESET FUNCTION TESTS
 *****************************************************************************/
// d_test_registry_reset: restores single value to default
bool d_tests_sa_cvar_reset_single(struct d_test_counter* _counter);
// d_test_registry_reset: invalid flag does not crash
bool d_tests_sa_cvar_reset_invalid_flag(struct d_test_counter* _counter);
// d_test_registry_reset_all: restores all values to defaults
bool d_tests_sa_cvar_reset_all_values(struct d_test_counter* _counter);
// d_test_registry_reset_all: idempotent (double reset)
bool d_tests_sa_cvar_reset_all_idempotent(struct d_test_counter* _counter);

// VIII. aggregation function
bool d_tests_sa_cvar_reset_all_fn(struct d_test_counter* _counter);


/******************************************************************************
 * IX. ARG VALIDATION TESTS
 *****************************************************************************/
// d_test_registry_is_valid_arg: config key with IS_CONFIG returns true
bool d_tests_sa_cvar_valid_arg_config(struct d_test_counter* _counter);
// d_test_registry_is_valid_arg: metadata key with IS_METADATA returns true
bool d_tests_sa_cvar_valid_arg_metadata(struct d_test_counter* _counter);
// d_test_registry_is_valid_arg: key with wrong flag returns false
bool d_tests_sa_cvar_valid_arg_wrong_flag(struct d_test_counter* _counter);
// d_test_registry_is_valid_arg: NULL key returns false
bool d_tests_sa_cvar_valid_arg_null_key(struct d_test_counter* _counter);
// d_test_registry_is_valid_arg: invalid key returns false
bool d_tests_sa_cvar_valid_arg_invalid_key(struct d_test_counter* _counter);

// IX.   aggregation function
bool d_tests_sa_cvar_valid_arg_all(struct d_test_counter* _counter);


/******************************************************************************
 * X. TYPED ACCESS MACRO TESTS
 *****************************************************************************/
// D_TEST_REGISTRY_GET: returns row for valid key
bool d_tests_sa_cvar_macro_get_row(struct d_test_counter* _counter);
// D_TEST_REGISTRY_VALUE_BOOL: returns boolean value
bool d_tests_sa_cvar_macro_value_bool(struct d_test_counter* _counter);
// D_TEST_REGISTRY_VALUE_SIZE_T: returns size_t value
bool d_tests_sa_cvar_macro_value_size_t(struct d_test_counter* _counter);
// D_TEST_REGISTRY_VALUE_UINT32/UINT16/INT32 macros
bool d_tests_sa_cvar_macro_value_numeric(struct d_test_counter* _counter);
// D_TEST_REGISTRY_VALUE_PTR: returns pointer value
bool d_tests_sa_cvar_macro_value_ptr(struct d_test_counter* _counter);
// D_TEST_REGISTRY_HELP/FLAG/TYPE: returns metadata fields
bool d_tests_sa_cvar_macro_metadata_fields(struct d_test_counter* _counter);
// Typed macros with invalid key return zero/NULL/false
bool d_tests_sa_cvar_macro_invalid_key(struct d_test_counter* _counter);

// X.    aggregation function
bool d_tests_sa_cvar_typed_macro_all(struct d_test_counter* _counter);


/******************************************************************************
 * XI. PREDICATE FUNCTION TESTS
 *****************************************************************************/
// command_flags: config entries have IS_CONFIG set
bool d_tests_sa_cvar_predicate_config_true(struct d_test_counter* _counter);
// command_flags: metadata entries do not have IS_CONFIG
bool d_tests_sa_cvar_predicate_config_false(struct d_test_counter* _counter);
// command_flags: metadata entries have IS_METADATA set
bool d_tests_sa_cvar_predicate_metadata_true(struct d_test_counter* _counter);
// command_flags: config entries do not have IS_METADATA
bool d_tests_sa_cvar_predicate_metadata_false(struct d_test_counter* _counter);
// command_flags: no current rows have IS_REQUIRED
bool d_tests_sa_cvar_predicate_required_false(struct d_test_counter* _counter);

// XI.   aggregation function
bool d_tests_sa_cvar_predicate_all(struct d_test_counter* _counter);


/******************************************************************************
 * XII. REGISTRY TABLE INTEGRITY TESTS
 *****************************************************************************/
// all rows have non-NULL key strings
bool d_tests_sa_cvar_table_keys_non_null(struct d_test_counter* _counter);
// all rows have non-NULL help text
bool d_tests_sa_cvar_table_help_non_null(struct d_test_counter* _counter);
// config rows have IS_CONFIG command flag
bool d_tests_sa_cvar_table_config_flags(struct d_test_counter* _counter);
// metadata rows have IS_METADATA command flag
bool d_tests_sa_cvar_table_metadata_flags(struct d_test_counter* _counter);
// total row count is 25 (8 config + 17 metadata)
bool d_tests_sa_cvar_table_row_counts(struct d_test_counter* _counter);

// XII.  aggregation function
bool d_tests_sa_cvar_table_integrity_all(struct d_test_counter* _counter);


/******************************************************************************
 * XIII. DEFAULT VALUE HELPER TESTS
 *****************************************************************************/
// defaults via reset_all + get: config flag returns expected default
bool d_tests_sa_cvar_default_by_flag(struct d_test_counter* _counter);
// defaults via reset_all + macro: string key returns expected default
bool d_tests_sa_cvar_default_by_key(struct d_test_counter* _counter);
// defaults restored after set + reset_all
bool d_tests_sa_cvar_default_after_set(struct d_test_counter* _counter);
// defaults match known compile-time constants
bool d_tests_sa_cvar_default_known_values(struct d_test_counter* _counter);

// XIII. aggregation function
bool d_tests_sa_cvar_default_all(struct d_test_counter* _counter);


/******************************************************************************
 * MODULE AGGREGATION
 *****************************************************************************/

// d_tests_sa_cvar_run_all
//   Module-level aggregation function that runs all test_cvar tests.
bool d_tests_sa_cvar_run_all(struct d_test_counter* _counter);


#endif  // DJINTERP_TESTS_TEST_CVAR_SA_
