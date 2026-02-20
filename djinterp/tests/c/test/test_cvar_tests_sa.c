#include ".\test_cvar_tests_sa.h"


/*
d_tests_sa_cvar_run_all
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
  - Predicate functions (is_config_row, is_metadata_row, is_required_row)
  - Registry table integrity (keys, help, config/metadata flags, counts)
  - Default value helpers (get_default, get_default_by_key, persistence)
*/
bool
d_tests_sa_cvar_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_cvar_row_flag_all(_counter)        && result;
    result = d_tests_sa_cvar_value_union_all(_counter)     && result;
    result = d_tests_sa_cvar_row_struct_all(_counter)      && result;
    result = d_tests_sa_cvar_init_all(_counter)            && result;
    result = d_tests_sa_cvar_find_all(_counter)            && result;
    result = d_tests_sa_cvar_alias_all(_counter)           && result;
    result = d_tests_sa_cvar_get_set_all(_counter)         && result;
    result = d_tests_sa_cvar_reset_all_fn(_counter)        && result;
    result = d_tests_sa_cvar_valid_arg_all(_counter)       && result;
    result = d_tests_sa_cvar_typed_macro_all(_counter)     && result;
    result = d_tests_sa_cvar_predicate_all(_counter)       && result;
    result = d_tests_sa_cvar_table_integrity_all(_counter) && result;
    result = d_tests_sa_cvar_default_all(_counter)         && result;

    return result;
}
