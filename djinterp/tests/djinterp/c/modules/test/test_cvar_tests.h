/*******************************************************************************
* djinterp [test]                                              test_cvar_tests.h
*
*   Unit test declarations for `test_cvar.h` module.
*   Provides comprehensive testing of the registry-based configuration and
* metadata schema including: registry row flags, value union, row structure,
* initialization, row find functions, alias lookup, value get/set, reset
* functions, argument validation, typed access macros, predicate functions,
* registry table integrity, and default value helpers.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.03
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_MODULES_TEST_TEST_CVAR_TESTS_H
#define DJINTERP_TESTS_C_MODULES_TEST_TEST_CVAR_TESTS_H 1

// std
#include <stddef.h>
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"
#include "../../../../../inc/djinterp/c/modules/test/test_cvar.h"
#include "../../../../../inc/djinterp/c/string_fn.h"
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


/******************************************************************************
 * I. REGISTRY ROW FLAG TESTS
 *****************************************************************************/
// DTestRegistryRowFlag: IS_REQUIRED is bit 0
bool d_tests_cvar_row_flag_is_required(
    struct dj_test_suite* _suite
);
// DTestRegistryRowFlag: IS_CONFIG is bit 1
bool d_tests_cvar_row_flag_is_config(
    struct dj_test_suite* _suite
);
// DTestRegistryRowFlag: IS_METADATA is bit 2
bool d_tests_cvar_row_flag_is_metadata(
    struct dj_test_suite* _suite
);
// DTestRegistryRowFlag: all flags are unique powers of 2
bool d_tests_cvar_row_flag_unique_bits(
    struct dj_test_suite* _suite
);
// DTestRegistryRowFlag: no overlapping bits between any two flags
bool d_tests_cvar_row_flag_no_overlap(
    struct dj_test_suite* _suite
);

// I.    aggregation function
bool d_tests_cvar_row_flag_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * II. VALUE UNION TESTS
 *****************************************************************************/
// dj_test_value: pointer member accessible
bool d_tests_cvar_value_union_ptr(
    struct dj_test_suite* _suite
);
// dj_test_value: size_t member accessible
bool d_tests_cvar_value_union_size_t(
    struct dj_test_suite* _suite
);
// dj_test_value: uint32_t member accessible
bool d_tests_cvar_value_union_uint32(
    struct dj_test_suite* _suite
);
// dj_test_value: uint16_t member accessible
bool d_tests_cvar_value_union_uint16(
    struct dj_test_suite* _suite
);
// dj_test_value: int32_t member accessible
bool d_tests_cvar_value_union_int32(
    struct dj_test_suite* _suite
);
// dj_test_value: bool member accessible
bool d_tests_cvar_value_union_bool(
    struct dj_test_suite* _suite
);

// II.   aggregation function
bool d_tests_cvar_value_union_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * III. ROW STRUCTURE TESTS
 *****************************************************************************/
// dj_test_registry_row: key member is accessible and first
bool d_tests_cvar_row_struct_key(
    struct dj_test_suite* _suite
);
// dj_test_registry_row: flag member is accessible
bool d_tests_cvar_row_struct_flag(
    struct dj_test_suite* _suite
);
// dj_test_registry_row: command_flags member is accessible
bool d_tests_cvar_row_struct_command_flags(
    struct dj_test_suite* _suite
);
// dj_test_registry_row: value_type member is accessible
bool d_tests_cvar_row_struct_value_type(
    struct dj_test_suite* _suite
);
// dj_test_registry_row: value and help members accessible
bool d_tests_cvar_row_struct_value_help(
    struct dj_test_suite* _suite
);

// III.  aggregation function
bool d_tests_cvar_row_struct_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IV. INITIALIZATION AND REGISTRY ACCESS TESTS
 *****************************************************************************/
// d_test_registry_init: does not crash
bool d_tests_cvar_init_safe(
    struct dj_test_suite* _suite
);
// d_test_registry_init: idempotent (double init)
bool d_tests_cvar_init_idempotent(
    struct dj_test_suite* _suite
);
// d_test_registry_registry: returns non-NULL
bool d_tests_cvar_registry_non_null(
    struct dj_test_suite* _suite
);
// d_test_registry_registry: has expected row count
bool d_tests_cvar_registry_row_count(
    struct dj_test_suite* _suite
);
// d_test_registry_registry: has static rows flag
bool d_tests_cvar_registry_static_flag(
    struct dj_test_suite* _suite
);

// IV.   aggregation function
bool d_tests_cvar_init_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * V. ROW FIND TESTS
 *****************************************************************************/
// d_test_registry_find: valid key returns non-NULL
bool d_tests_cvar_find_valid_key(
    struct dj_test_suite* _suite
);
// d_test_registry_find: returned row has matching key
bool d_tests_cvar_find_key_matches(
    struct dj_test_suite* _suite
);
// d_test_registry_find: NULL key returns NULL
bool d_tests_cvar_find_null_key(
    struct dj_test_suite* _suite
);
// d_test_registry_find: invalid key returns NULL
bool d_tests_cvar_find_invalid_key(
    struct dj_test_suite* _suite
);
// d_test_registry_find_by_flag: valid flag returns correct row
bool d_tests_cvar_find_by_flag_valid(
    struct dj_test_suite* _suite
);
// d_test_registry_find_by_flag: invalid flag returns NULL
bool d_tests_cvar_find_by_flag_invalid(
    struct dj_test_suite* _suite
);

// V.    aggregation function
bool d_tests_cvar_find_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VI. ALIAS LOOKUP TESTS
 *****************************************************************************/
// alias: "enabled" resolves to "config-enabled" row
bool d_tests_cvar_alias_enabled(
    struct dj_test_suite* _suite
);
// alias: "indent" resolves to "indent-string" row
bool d_tests_cvar_alias_indent(
    struct dj_test_suite* _suite
);
// alias: "indent-max" and "indent-level" resolve to "max-indent" row
bool d_tests_cvar_alias_max_indent(
    struct dj_test_suite* _suite
);
// alias: "timeout-ms" resolves to "timeout" row
bool d_tests_cvar_alias_timeout(
    struct dj_test_suite* _suite
);
// alias: "framework", "module", "submodule" resolve correctly
bool d_tests_cvar_alias_name_shortcuts(
    struct dj_test_suite* _suite
);

// VI.   aggregation function
bool d_tests_cvar_alias_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VII. VALUE GET/SET TESTS
 *****************************************************************************/
// d_test_registry_get: returns default value initially
bool d_tests_cvar_get_default_value(
    struct dj_test_suite* _suite
);
// d_test_registry_set: changes value and get reads it back
bool d_tests_cvar_set_then_get(
    struct dj_test_suite* _suite
);
// d_test_registry_set: invalid flag returns false
bool d_tests_cvar_set_invalid_flag(
    struct dj_test_suite* _suite
);
// d_test_registry_get: invalid flag returns zeroed value
bool d_tests_cvar_get_invalid_flag(
    struct dj_test_suite* _suite
);
// d_test_registry_set: set bool then get bool roundtrip
bool d_tests_cvar_set_get_bool(
    struct dj_test_suite* _suite
);

// VII.  aggregation function
bool d_tests_cvar_get_set_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * VIII. RESET FUNCTION TESTS
 *****************************************************************************/
// d_test_registry_reset: restores single value to default
bool d_tests_cvar_reset_single(
    struct dj_test_suite* _suite
);
// d_test_registry_reset: invalid flag does not crash
bool d_tests_cvar_reset_invalid_flag(
    struct dj_test_suite* _suite
);
// d_test_registry_reset_all: restores all values to defaults
bool d_tests_cvar_reset_all_values(
    struct dj_test_suite* _suite
);
// d_test_registry_reset_all: idempotent (double reset)
bool d_tests_cvar_reset_all_idempotent(
    struct dj_test_suite* _suite
);

// VIII. aggregation function
bool d_tests_cvar_reset_all_fn(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * IX. ARG VALIDATION TESTS
 *****************************************************************************/
// d_test_registry_is_valid_arg: config key with IS_CONFIG returns true
bool d_tests_cvar_valid_arg_config(
    struct dj_test_suite* _suite
);
// d_test_registry_is_valid_arg: metadata key with IS_METADATA returns true
bool d_tests_cvar_valid_arg_metadata(
    struct dj_test_suite* _suite
);
// d_test_registry_is_valid_arg: key with wrong flag returns false
bool d_tests_cvar_valid_arg_wrong_flag(
    struct dj_test_suite* _suite
);
// d_test_registry_is_valid_arg: NULL key returns false
bool d_tests_cvar_valid_arg_null_key(
    struct dj_test_suite* _suite
);
// d_test_registry_is_valid_arg: invalid key returns false
bool d_tests_cvar_valid_arg_invalid_key(
    struct dj_test_suite* _suite
);

// IX.   aggregation function
bool d_tests_cvar_valid_arg_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * X. TYPED ACCESS MACRO TESTS
 *****************************************************************************/
// D_TEST_REGISTRY_GET: returns row for valid key
bool d_tests_cvar_macro_get_row(
    struct dj_test_suite* _suite
);
// D_TEST_REGISTRY_VALUE_BOOL: returns boolean value
bool d_tests_cvar_macro_value_bool(
    struct dj_test_suite* _suite
);
// D_TEST_REGISTRY_VALUE_SIZE_T: returns size_t value
bool d_tests_cvar_macro_value_size_t(
    struct dj_test_suite* _suite
);
// D_TEST_REGISTRY_VALUE_UINT32/UINT16/INT32 macros
bool d_tests_cvar_macro_value_numeric(
    struct dj_test_suite* _suite
);
// D_TEST_REGISTRY_VALUE_PTR: returns pointer value
bool d_tests_cvar_macro_value_ptr(
    struct dj_test_suite* _suite
);
// D_TEST_REGISTRY_HELP/FLAG/TYPE: returns metadata fields
bool d_tests_cvar_macro_metadata_fields(
    struct dj_test_suite* _suite
);
// Typed macros with invalid key return zero/NULL/false
bool d_tests_cvar_macro_invalid_key(
    struct dj_test_suite* _suite
);

// X.    aggregation function
bool d_tests_cvar_typed_macro_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * XI. PREDICATE FUNCTION TESTS
 *****************************************************************************/
// command_flags: config entries have IS_CONFIG set
bool d_tests_cvar_predicate_config_true(
    struct dj_test_suite* _suite
);
// command_flags: metadata entries do not have IS_CONFIG
bool d_tests_cvar_predicate_config_false(
    struct dj_test_suite* _suite
);
// command_flags: metadata entries have IS_METADATA set
bool d_tests_cvar_predicate_metadata_true(
    struct dj_test_suite* _suite
);
// command_flags: config entries do not have IS_METADATA
bool d_tests_cvar_predicate_metadata_false(
    struct dj_test_suite* _suite
);
// command_flags: no current rows have IS_REQUIRED
bool d_tests_cvar_predicate_required_false(
    struct dj_test_suite* _suite
);

// XI.   aggregation function
bool d_tests_cvar_predicate_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * XII. REGISTRY TABLE INTEGRITY TESTS
 *****************************************************************************/
// all rows have non-NULL key strings
bool d_tests_cvar_table_keys_non_null(
    struct dj_test_suite* _suite
);
// all rows have non-NULL help text
bool d_tests_cvar_table_help_non_null(
    struct dj_test_suite* _suite
);
// config rows have IS_CONFIG command flag
bool d_tests_cvar_table_config_flags(
    struct dj_test_suite* _suite
);
// metadata rows have IS_METADATA command flag
bool d_tests_cvar_table_metadata_flags(
    struct dj_test_suite* _suite
);
// total row count is 25 (8 config + 17 metadata)
bool d_tests_cvar_table_row_counts(
    struct dj_test_suite* _suite
);

// XII.  aggregation function
bool d_tests_cvar_table_integrity_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * XIII. DEFAULT VALUE HELPER TESTS
 *****************************************************************************/
// defaults via reset_all + get: config flag returns expected default
bool d_tests_cvar_default_by_flag(
    struct dj_test_suite* _suite
);
// defaults via reset_all + macro: string key returns expected default
bool d_tests_cvar_default_by_key(
    struct dj_test_suite* _suite
);
// defaults restored after set + reset_all
bool d_tests_cvar_default_after_set(
    struct dj_test_suite* _suite
);
// defaults match known compile-time constants
bool d_tests_cvar_default_known_values(
    struct dj_test_suite* _suite
);

// XIII. aggregation function
bool d_tests_cvar_default_all(
    struct dj_test_suite* _suite
);


/******************************************************************************
 * MODULE AGGREGATION
 *****************************************************************************/

// d_tests_cvar_run_all
//   Module-level aggregation function that runs all test_cvar tests.
bool d_tests_cvar_run_all(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_MODULES_TEST_TEST_CVAR_TESTS_H
