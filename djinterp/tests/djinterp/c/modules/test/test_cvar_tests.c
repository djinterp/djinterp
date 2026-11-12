/*******************************************************************************
* djinterp [c][test]                                           test_cvar_tests.c
*
* test_cvar.h's suite: d_tests_cvar_run_all, which runs the thirteen blocks of
* test_cvar_tests.h in order.
*
*
* path:      /tests/djinterp/c/modules/test/test_cvar_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_cvar_tests.h"  // corresponding header


/*
d_tests_cvar_run_all
  Module-level aggregation function that runs all test_cvar tests.
  Executes tests for all categories:
  - Registry row flags (IS_REQUIRED, IS_CONFIG, IS_METADATA, unique bits)
  - Value union (ptr, z, u32, u16, i32, b)
  - Row structure (key, flag, command_flags, value_type, value, help)
  - Initialization and registry access (init, registry, row count, flags)
  - Row find (find by key, find by flag, NULL/invalid handling)
  - Alias lookup (enabled, indent, max-indent, timeout-ms, name shortcuts)
  - Value get/set (defaults, roundtrip, invalid flags)
  - Reset functions (single reset, reset_all, idempotent)
  - Arg validation (config, metadata, wrong flag, NULL, invalid)
  - Typed access macros (GET, VALUE_BOOL/SIZE_T/UINT32/PTR, HELP/FLAG/TYPE)
  - Row kinds (each row's IS_CONFIG, IS_METADATA and IS_REQUIRED flags)
  - Registry table integrity (keys, help, config/metadata flags, counts)
  - Defaults (after reset_all, by flag and by key, after a set)
*/
bool
d_tests_cvar_run_all(
    struct dj_test_suite* _suite
)
{
    bool result = true;

    // run all test categories
    result = d_tests_cvar_row_flag_all(_suite)        && result;
    result = d_tests_cvar_value_union_all(_suite)     && result;
    result = d_tests_cvar_row_struct_all(_suite)      && result;
    result = d_tests_cvar_init_all(_suite)            && result;
    result = d_tests_cvar_find_all(_suite)            && result;
    result = d_tests_cvar_alias_all(_suite)           && result;
    result = d_tests_cvar_get_set_all(_suite)         && result;
    result = d_tests_cvar_reset_all_fn(_suite)        && result;
    result = d_tests_cvar_valid_arg_all(_suite)       && result;
    result = d_tests_cvar_typed_macro_all(_suite)     && result;
    result = d_tests_cvar_predicate_all(_suite)       && result;
    result = d_tests_cvar_table_integrity_all(_suite) && result;
    result = d_tests_cvar_default_all(_suite)         && result;

    return result;
}
