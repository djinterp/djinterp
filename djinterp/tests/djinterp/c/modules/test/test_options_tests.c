/*******************************************************************************
* djinterp [test]                                           test_options_tests.c
*
*
* path:      /tests/djinterp/c/modules/test/test_options_tests.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.30
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "test_options_tests.h"


/*
d_tests_options_run_all
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
  - Core structure (dj_test_config members and layout)
  - Constructor/destructor (new, new_preset, new_copy, free)
  - Getter functions (get_bool, get_size_t, get_int32, get_uint32,
    get_string, get_ptr)
  - Setter functions (set_bool, set_size_t, set_int32, set_uint32,
    set_string, set_ptr)
  - Key lookup (d_test_options_key_from_string)
*/
bool d_tests_options_run_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    result = true;

    // run all test categories
    result = d_tests_options_flag_macro_all(_suite) && result;
    result = d_tests_options_mask_all(_suite) && result;
    result = d_tests_options_enum_all(_suite) && result;
    result = d_tests_options_combination_all(_suite) && result;
    result = d_tests_options_mode_all(_suite) && result;
    result = d_tests_options_utility_macro_all(_suite) && result;
    result = d_tests_options_semantic_all(_suite) && result;
    result = d_tests_options_default_all(_suite) && result;
    result = d_tests_options_struct_all(_suite) && result;
    result = d_tests_options_lifecycle_all(_suite) && result;
    result = d_tests_options_getter_all(_suite) && result;
    result = d_tests_options_setter_all(_suite) && result;
    result = d_tests_options_key_lookup_all(_suite) && result;

    return result;
}
