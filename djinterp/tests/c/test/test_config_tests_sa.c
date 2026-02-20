#include ".\test_config_tests_sa.h"


/*
d_tests_sa_config_run_all
  Module-level aggregation function that runs all test_config tests.
  Executes tests for all categories:
  - Flag manipulation macros (SETTINGS_SHIFT, SETTINGS_TO_FLAGS, FLAGS_TO_SETTINGS)
  - Mask definitions (MESSAGE, SETTINGS, COUNTER, PRINT, STACK)
  - Enum values (DTestMessageFlag, DTestSettingsFlag, DTestConfigKey,
    DTestMetadataFlag, DTestEvent)
  - Message flag combinations (COUNT_*, PRINT_*, MSG_ALL, category combos)
  - Mode definitions and presets (SILENT, MINIMAL, NORMAL, VERBOSE)
  - Utility macros (HAS_FLAG, HAS_ANY_FLAG, GET_*, IS_SILENT, IS_VERBOSE,
    IS_MODE)
  - Semantic check macros (SHOULD_COUNT_*, SHOULD_PRINT_*, SHOULD_PUSH_*)
  - Default values (INDENT, MAX_INDENT, MAX_FAILURES, TIMEOUT)
  - Core structure (d_test_config members and layout)
  - Constructor/destructor (new, new_preset, new_copy, free)
  - Getter functions (get_bool, get_size_t, get_int32, get_uint32,
    get_string, get_ptr)
  - Setter functions (set_bool, set_size_t, set_int32, set_uint32,
    set_string, set_ptr)
  - Key lookup (d_test_config_key_from_string)
*/
bool
d_tests_sa_config_run_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_sa_config_flag_macro_all(_counter) && result;
    result = d_tests_sa_config_mask_all(_counter) && result;
    result = d_tests_sa_config_enum_all(_counter) && result;
    result = d_tests_sa_config_combination_all(_counter) && result;
    result = d_tests_sa_config_mode_all(_counter) && result;
    result = d_tests_sa_config_utility_macro_all(_counter) && result;
    result = d_tests_sa_config_semantic_all(_counter) && result;
    result = d_tests_sa_config_default_all(_counter) && result;
    result = d_tests_sa_config_struct_all(_counter) && result;
    result = d_tests_sa_config_lifecycle_all(_counter) && result;
    result = d_tests_sa_config_getter_all(_counter) && result;
    result = d_tests_sa_config_setter_all(_counter) && result;
    result = d_tests_sa_config_key_lookup_all(_counter) && result;

    return result;
}
