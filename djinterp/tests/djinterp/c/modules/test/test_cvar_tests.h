/*******************************************************************************
* djinterp [c][test]                                           test_cvar_tests.h
*
* test_cvar.h's suite, written against DTest's C face (test_suite.h).
*   One section a block: the row flags, the value union, the row, the
* registry's initialization, find by key and by flag, aliases, get and set,
* reset, argument validation, the typed access macros, the rows' command flags,
* the table's integrity and the defaults. d_tests_cvar_run_all runs the
* thirteen in order.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.03
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  REGISTRY ROW FLAG TESTS
    -----------------------
2.  VALUE UNION TESTS
    -----------------
3.  ROW STRUCTURE TESTS
    -------------------
4.  INITIALIZATION AND REGISTRY ACCESS TESTS
    ----------------------------------------
5.  ROW FIND TESTS
    --------------
6.  ALIAS LOOKUP TESTS
    ------------------
7.  VALUE GET/SET TESTS
    -------------------
8.  RESET FUNCTION TESTS
    --------------------
9.  ARG VALIDATION TESTS
    --------------------
10. TYPED ACCESS MACRO TESTS
    ------------------------
11. PREDICATE FUNCTION TESTS
    ------------------------
12. REGISTRY TABLE INTEGRITY TESTS
    ------------------------------
13. DEFAULT VALUE HELPER TESTS
    --------------------------
14. MODULE AGGREGATION
    ------------------
*/

#ifndef DJINTERP_TESTS_C_TEST_CVAR_TESTS_H
#define DJINTERP_TESTS_C_TEST_CVAR_TESTS_H 1

// std
#include <stddef.h>  // size_t
#include <stdlib.h>  // free
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"                 // framework root
#include "../../../../../inc/djinterp/c/modules/test/test_cvar.h"   // the module under test
#include "../../../../../inc/djinterp/c/string_fn.h"                // d_strcasecmp
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint16_t, uint32_t, int32_t


//==============================================================================
// 1.  REGISTRY ROW FLAG TESTS
//==============================================================================


// DTestRegistryRowFlag: IS_REQUIRED, IS_CONFIG and IS_METADATA's bits,
// each a distinct power of two, no two overlapping
bool d_tests_cvar_row_flag_is_required(struct dj_test_suite* _suite);

bool d_tests_cvar_row_flag_is_config(struct dj_test_suite* _suite);

bool d_tests_cvar_row_flag_is_metadata(struct dj_test_suite* _suite);

bool d_tests_cvar_row_flag_unique_bits(struct dj_test_suite* _suite);

bool d_tests_cvar_row_flag_no_overlap(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_row_flag_all(struct dj_test_suite* _suite);


//==============================================================================
// 2.  VALUE UNION TESTS
//==============================================================================


// dj_test_value: each member (pointer, size_t, uint32_t, uint16_t,
// int32_t, bool) holds its value
bool d_tests_cvar_value_union_ptr(struct dj_test_suite* _suite);

bool d_tests_cvar_value_union_size_t(struct dj_test_suite* _suite);

bool d_tests_cvar_value_union_uint32(struct dj_test_suite* _suite);

bool d_tests_cvar_value_union_uint16(struct dj_test_suite* _suite);

bool d_tests_cvar_value_union_int32(struct dj_test_suite* _suite);

bool d_tests_cvar_value_union_bool(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_value_union_all(struct dj_test_suite* _suite);


//==============================================================================
// 3.  ROW STRUCTURE TESTS
//==============================================================================


// dj_test_registry_row: key first, then flag, command_flags,
// value_type, value and help
bool d_tests_cvar_row_struct_key(struct dj_test_suite* _suite);

bool d_tests_cvar_row_struct_flag(struct dj_test_suite* _suite);

bool d_tests_cvar_row_struct_command_flags(struct dj_test_suite* _suite);

bool d_tests_cvar_row_struct_value_type(struct dj_test_suite* _suite);

bool d_tests_cvar_row_struct_value_help(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_row_struct_all(struct dj_test_suite* _suite);


//==============================================================================
// 4.  INITIALIZATION AND REGISTRY ACCESS TESTS
//==============================================================================


// d_test_registry_init, run twice, and d_test_registry_registry:
// non-NULL, the expected row count, the static rows flag
bool d_tests_cvar_init_safe(struct dj_test_suite* _suite);

bool d_tests_cvar_init_idempotent(struct dj_test_suite* _suite);

bool d_tests_cvar_registry_non_null(struct dj_test_suite* _suite);

bool d_tests_cvar_registry_row_count(struct dj_test_suite* _suite);

bool d_tests_cvar_registry_static_flag(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_init_all(struct dj_test_suite* _suite);


//==============================================================================
// 5.  ROW FIND TESTS
//==============================================================================


// d_test_registry_find by key (valid, NULL, unknown) and
// d_test_registry_find_by_flag (valid, unknown)
bool d_tests_cvar_find_valid_key(struct dj_test_suite* _suite);

bool d_tests_cvar_find_key_matches(struct dj_test_suite* _suite);

bool d_tests_cvar_find_null_key(struct dj_test_suite* _suite);

bool d_tests_cvar_find_invalid_key(struct dj_test_suite* _suite);

bool d_tests_cvar_find_by_flag_valid(struct dj_test_suite* _suite);

bool d_tests_cvar_find_by_flag_invalid(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_find_all(struct dj_test_suite* _suite);


//==============================================================================
// 6.  ALIAS LOOKUP TESTS
//==============================================================================


// the aliases: enabled, indent, indent-max and indent-level,
// timeout-ms, framework, module and submodule
bool d_tests_cvar_alias_enabled(struct dj_test_suite* _suite);

bool d_tests_cvar_alias_indent(struct dj_test_suite* _suite);

bool d_tests_cvar_alias_max_indent(struct dj_test_suite* _suite);

bool d_tests_cvar_alias_timeout(struct dj_test_suite* _suite);

bool d_tests_cvar_alias_name_shortcuts(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_alias_all(struct dj_test_suite* _suite);


//==============================================================================
// 7.  VALUE GET/SET TESTS
//==============================================================================


// d_test_registry_get and _set: the default, a set read back, an
// unknown flag, a bool round trip
bool d_tests_cvar_get_default_value(struct dj_test_suite* _suite);

bool d_tests_cvar_set_then_get(struct dj_test_suite* _suite);

bool d_tests_cvar_set_invalid_flag(struct dj_test_suite* _suite);

bool d_tests_cvar_get_invalid_flag(struct dj_test_suite* _suite);

bool d_tests_cvar_set_get_bool(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_get_set_all(struct dj_test_suite* _suite);


//==============================================================================
// 8.  RESET FUNCTION TESTS
//==============================================================================


// d_test_registry_reset, of an unknown flag too, and _reset_all, run
// twice
bool d_tests_cvar_reset_single(struct dj_test_suite* _suite);

bool d_tests_cvar_reset_invalid_flag(struct dj_test_suite* _suite);

bool d_tests_cvar_reset_all_values(struct dj_test_suite* _suite);

bool d_tests_cvar_reset_all_idempotent(struct dj_test_suite* _suite);

bool d_tests_cvar_reset_all_fn(struct dj_test_suite* _suite);

// the section's tests, in order


//==============================================================================
// 9.  ARG VALIDATION TESTS
//==============================================================================


// d_test_registry_is_valid_arg: config and metadata keys, the wrong
// flag, NULL and unknown keys
bool d_tests_cvar_valid_arg_config(struct dj_test_suite* _suite);

bool d_tests_cvar_valid_arg_metadata(struct dj_test_suite* _suite);

bool d_tests_cvar_valid_arg_wrong_flag(struct dj_test_suite* _suite);

bool d_tests_cvar_valid_arg_null_key(struct dj_test_suite* _suite);

bool d_tests_cvar_valid_arg_invalid_key(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_valid_arg_all(struct dj_test_suite* _suite);


//==============================================================================
// 10. TYPED ACCESS MACRO TESTS
//==============================================================================


// the D_TEST_REGISTRY_* typed access macros, also with an unknown key
bool d_tests_cvar_macro_get_row(struct dj_test_suite* _suite);

bool d_tests_cvar_macro_value_bool(struct dj_test_suite* _suite);

bool d_tests_cvar_macro_value_size_t(struct dj_test_suite* _suite);

bool d_tests_cvar_macro_value_numeric(struct dj_test_suite* _suite);

bool d_tests_cvar_macro_value_ptr(struct dj_test_suite* _suite);

bool d_tests_cvar_macro_metadata_fields(struct dj_test_suite* _suite);

bool d_tests_cvar_macro_invalid_key(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_typed_macro_all(struct dj_test_suite* _suite);


//==============================================================================
// 11. PREDICATE FUNCTION TESTS
//==============================================================================


// each row's command_flags: IS_CONFIG and IS_METADATA where they
// belong and nowhere else, IS_REQUIRED on none
bool d_tests_cvar_predicate_config_true(struct dj_test_suite* _suite);

bool d_tests_cvar_predicate_config_false(struct dj_test_suite* _suite);

bool d_tests_cvar_predicate_metadata_true(struct dj_test_suite* _suite);

bool d_tests_cvar_predicate_metadata_false(struct dj_test_suite* _suite);

bool d_tests_cvar_predicate_required_false(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_predicate_all(struct dj_test_suite* _suite);


//==============================================================================
// 12. REGISTRY TABLE INTEGRITY TESTS
//==============================================================================


// the table: every key and help text set, the command flags right,
// 25 rows
bool d_tests_cvar_table_keys_non_null(struct dj_test_suite* _suite);

bool d_tests_cvar_table_help_non_null(struct dj_test_suite* _suite);

bool d_tests_cvar_table_config_flags(struct dj_test_suite* _suite);

bool d_tests_cvar_table_metadata_flags(struct dj_test_suite* _suite);

bool d_tests_cvar_table_row_counts(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_table_integrity_all(struct dj_test_suite* _suite);


//==============================================================================
// 13. DEFAULT VALUE HELPER TESTS
//==============================================================================


// the defaults: after reset_all, by flag and by key, after a set,
// and against the compile-time constants
bool d_tests_cvar_default_by_flag(struct dj_test_suite* _suite);

bool d_tests_cvar_default_by_key(struct dj_test_suite* _suite);

bool d_tests_cvar_default_after_set(struct dj_test_suite* _suite);

bool d_tests_cvar_default_known_values(struct dj_test_suite* _suite);

// the section's tests, in order
bool d_tests_cvar_default_all(struct dj_test_suite* _suite);


//==============================================================================
// 14. MODULE AGGREGATION
//==============================================================================


// every section's aggregate, in order
bool d_tests_cvar_run_all(struct dj_test_suite* _suite);


#endif  // DJINTERP_TESTS_C_TEST_CVAR_TESTS_H
