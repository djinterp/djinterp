/*******************************************************************************
* djinterp [c][env]                                                  env_tests.h
*
* This is a test file for `env.h` unit tests.
* For the file itself, go to `\inc\c\env.h`.
* Note: DTest's C face stands on this module (through djinterp.h), so a
* fault here can stop the runner itself, not just fail a test.
*
*
* path:      /tests/djinterp/c/env_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.09.26
*                                                            revised: 2026.11.06
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_ENV_TESTS_H
#define DJINTERP_TESTS_C_ENV_TESTS_H 1

// std
#include "../../../inc/re_std/c/cstdint/dstdint.h"  // fixed-width integers
#include <stdlib.h>
// djinterp
#include "../../../inc/djinterp/c/djinterp.h"
#include "../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's C face


// =============================================================================
// I.   CFG CONFIGURATION SYSTEM TEST FUNCTIONS
// =============================================================================

// cfg bit flag tests
bool d_tests_env_cfg_bit_flags(
    struct dj_test_suite* _suite
);

bool d_tests_env_cfg_custom_value(
    struct dj_test_suite* _suite
);

// cfg section enable macro tests
bool d_tests_env_cfg_enabled_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_cfg_enabled_zero_case(
    struct dj_test_suite* _suite
);

// cfg detected variable tests
bool d_tests_env_cfg_detected_vars_lang(
    struct dj_test_suite* _suite
);

bool d_tests_env_cfg_detected_vars_posix(
    struct dj_test_suite* _suite
);

bool d_tests_env_cfg_detected_vars_compiler(
    struct dj_test_suite* _suite
);

bool d_tests_env_cfg_detected_vars_os(
    struct dj_test_suite* _suite
);

bool d_tests_env_cfg_detected_vars_arch(
    struct dj_test_suite* _suite
);

bool d_tests_env_cfg_detected_vars_build(
    struct dj_test_suite* _suite
);

// cfg module aggregator
bool d_tests_env_cfg_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// II.  LANGUAGE ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// standard version constant tests
bool d_tests_env_lang_c_standard_constants(
    struct dj_test_suite* _suite
);

bool d_tests_env_lang_cpp_standard_constants(
    struct dj_test_suite* _suite
);

bool d_tests_env_lang_standard_constant_ranges(
    struct dj_test_suite* _suite
);

// C standard detection tests
bool d_tests_env_lang_c_standard_detection(
    struct dj_test_suite* _suite
);

bool d_tests_env_lang_c_standard_fallback(
    struct dj_test_suite* _suite
);

// C++ standard detection tests
bool d_tests_env_lang_cpp_standard_detection(
    struct dj_test_suite* _suite
);

// version comparison macro tests
bool d_tests_env_lang_c_version_comparison_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_lang_cpp_version_comparison_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_lang_version_comparison_boundary_cases(
    struct dj_test_suite* _suite
);

// convenience macro tests
bool d_tests_env_lang_using_macros(
    struct dj_test_suite* _suite
);

// manual detection variable tests
bool d_tests_env_lang_detected_vars_c(
    struct dj_test_suite* _suite
);

bool d_tests_env_lang_detected_vars_cpp(
    struct dj_test_suite* _suite
);

// consistency and edge case tests
bool d_tests_env_lang_consistency_check(
    struct dj_test_suite* _suite
);

bool d_tests_env_lang_feature_test_macros(
    struct dj_test_suite* _suite
);

// language module aggregator
bool d_tests_env_lang_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// III. POSIX ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// POSIX version constant tests
bool d_tests_env_posix_version_constants(
    struct dj_test_suite* _suite
);

bool d_tests_env_posix_c_source_constants(
    struct dj_test_suite* _suite
);

bool d_tests_env_posix_xsi_version_constants(
    struct dj_test_suite* _suite
);

// POSIX version detection tests
bool d_tests_env_posix_version_detection(
    struct dj_test_suite* _suite
);

bool d_tests_env_posix_version_name_consistency(
    struct dj_test_suite* _suite
);

// XSI detection tests
bool d_tests_env_posix_xsi_detection(
    struct dj_test_suite* _suite
);

// POSIX feature macro tests
bool d_tests_env_posix_feature_macros(
    struct dj_test_suite* _suite
);

// POSIX utility macro tests
bool d_tests_env_posix_utility_is_available(
    struct dj_test_suite* _suite
);

bool d_tests_env_posix_utility_is_modern(
    struct dj_test_suite* _suite
);

bool d_tests_env_posix_utility_version_at_least(
    struct dj_test_suite* _suite
);

bool d_tests_env_posix_utility_has_feature(
    struct dj_test_suite* _suite
);

// manual detection tests
bool d_tests_env_posix_manual_detection(
    struct dj_test_suite* _suite
);

// consistency and integration tests
bool d_tests_env_posix_consistency_check(
    struct dj_test_suite* _suite
);

bool d_tests_env_posix_feature_test_patterns(
    struct dj_test_suite* _suite
);

// POSIX module aggregator
bool d_tests_env_posix_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// IV.  COMPILER ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// compiler detection tests
bool d_tests_env_compiler_detection_flags(
    struct dj_test_suite* _suite
);

bool d_tests_env_compiler_name_macros(
    struct dj_test_suite* _suite
);

// compiler version tests
bool d_tests_env_compiler_version_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_compiler_version_at_least(
    struct dj_test_suite* _suite
);

bool d_tests_env_compiler_version_at_most(
    struct dj_test_suite* _suite
);

// VA_OPT detection tests
bool d_tests_env_compiler_va_opt_detection(
    struct dj_test_suite* _suite
);

// preprocessor limit tests
bool d_tests_env_compiler_pp_limit_constants(
    struct dj_test_suite* _suite
);

bool d_tests_env_compiler_pp_min_limits(
    struct dj_test_suite* _suite
);

bool d_tests_env_compiler_pp_max_limits(
    struct dj_test_suite* _suite
);

bool d_tests_env_compiler_pp_limit_utility_macros(
    struct dj_test_suite* _suite
);

// manual detection tests
bool d_tests_env_compiler_manual_detection(
    struct dj_test_suite* _suite
);

// consistency tests
bool d_tests_env_compiler_consistency_check(
    struct dj_test_suite* _suite
);

// compiler module aggregator
bool d_tests_env_compiler_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// IV-A. PREPROCESSOR LIMITS TEST FUNCTIONS
// =============================================================================

// standard limit constant tests
bool d_tests_env_pp_limit_constants_c89(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_limit_constants_c99(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_limit_constants_cpp(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_limit_constants_all(
    struct dj_test_suite* _suite
);

// standard-based minimum limit tests
bool d_tests_env_pp_min_limits_definition(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_min_limits_values(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_min_limits_all(
    struct dj_test_suite* _suite
);

// compiler-specific maximum limit tests
bool d_tests_env_pp_max_limits_definition(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_max_limits_values(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_max_limits_all(
    struct dj_test_suite* _suite
);

// utility macro tests
bool d_tests_env_pp_utility_macros_definition(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_utility_macros_functionality(
    struct dj_test_suite* _suite
);

bool d_tests_env_pp_utility_macros_all(
    struct dj_test_suite* _suite
);

// __VA_OPT__ detection tests
bool d_tests_env_pp_va_opt_detection(
    struct dj_test_suite* _suite
);

// preprocessor limits module aggregator
bool d_tests_env_pp_limits_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// VI.   ARCHITECTURE ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// architecture constant tests
bool d_tests_env_arch_type_constants(
    struct dj_test_suite* _suite
);

bool d_tests_env_arch_endian_constants(
    struct dj_test_suite* _suite
);

// architecture detection tests
bool d_tests_env_arch_detection_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_arch_property_values(
    struct dj_test_suite* _suite
);

bool d_tests_env_arch_detection_all(
    struct dj_test_suite* _suite
);

// architecture utility macro tests
bool d_tests_env_arch_family_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_arch_bitwidth_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_arch_endianness_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_arch_utility_macros_all(
    struct dj_test_suite* _suite
);

// architecture consistency tests
bool d_tests_env_arch_consistency_check(
    struct dj_test_suite* _suite
);

// architecture module aggregator
bool d_tests_env_arch_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// VIII.  OPERATING SYSTEM ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// OS flag constant tests
bool d_tests_env_os_flag_constants_apple(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_flag_constants_unix(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_flag_constants_bsd(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_flag_constants_windows(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_flag_constants_mobile(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_flag_constants_legacy(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_flag_constants_all(
    struct dj_test_suite* _suite
);

// OS utility constant tests
bool d_tests_env_os_utility_constants(
    struct dj_test_suite* _suite
);

// OS utility macro tests
bool d_tests_env_os_utility_macro_block(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_utility_macro_unix(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_utility_macro_mobile(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_utility_macro_windows(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_utility_macro_discontinued(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_utility_macro_posix(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_utility_macros_all(
    struct dj_test_suite* _suite
);

// OS detection tests
bool d_tests_env_os_detection_macros(
    struct dj_test_suite* _suite
);

bool d_tests_env_os_platform_macros(
    struct dj_test_suite* _suite
);

// OS module aggregator
bool d_tests_env_os_all(
    struct dj_test_suite* _suite
);

// =============================================================================
// IX. STANDARD LIBRARY FEATURE DETECTION TEST FUNCTIONS
// =============================================================================

// C standard header availability tests
bool d_tests_env_stdlib_c_standard_headers(
    struct dj_test_suite* _suite
);

bool d_tests_env_stdlib_posix_headers(
    struct dj_test_suite* _suite
);

// string and memory function tests
bool d_tests_env_stdlib_string_functions(
    struct dj_test_suite* _suite
);

// file I/O feature tests
bool d_tests_env_stdlib_file_io_features(
    struct dj_test_suite* _suite
);

// time and date feature tests
bool d_tests_env_stdlib_time_features(
    struct dj_test_suite* _suite
);

// math feature tests
bool d_tests_env_stdlib_math_features(
    struct dj_test_suite* _suite
);

// network feature tests
bool d_tests_env_stdlib_network_features(
    struct dj_test_suite* _suite
);

// process and system feature tests
bool d_tests_env_stdlib_process_features(
    struct dj_test_suite* _suite
);

// memory management feature tests
bool d_tests_env_stdlib_memory_features(
    struct dj_test_suite* _suite
);

// SIMD and hardware intrinsics tests
bool d_tests_env_stdlib_simd_features(
    struct dj_test_suite* _suite
);

// VLA and security feature tests
bool d_tests_env_stdlib_vla_and_security(
    struct dj_test_suite* _suite
);

// consistency tests
bool d_tests_env_stdlib_consistency_check(
    struct dj_test_suite* _suite
);

// stdlib module aggregator
bool d_tests_env_stdlib_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// X. BUILD CONFIGURATION ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// build configuration tests
bool d_tests_env_build_config_defined(
    struct dj_test_suite* _suite
);

bool d_tests_env_build_config_values(
    struct dj_test_suite* _suite
);

bool d_tests_env_build_config_consistency(
    struct dj_test_suite* _suite
);

// build detection logic tests
bool d_tests_env_build_detection_logic(
    struct dj_test_suite* _suite
);

// build type string tests
bool d_tests_env_build_type_string(
    struct dj_test_suite* _suite
);

// conditional compilation tests
bool d_tests_env_build_conditional_compilation(
    struct dj_test_suite* _suite
);

// build usage tests
bool d_tests_env_build_usage_examples(
    struct dj_test_suite* _suite
);

// build module aggregator
bool d_tests_env_build_all(
    struct dj_test_suite* _suite
);


// =============================================================================
// XI.   MASTER TEST SUITE
// =============================================================================

bool d_tests_env_all(
    struct dj_test_suite* _suite
);

// the build profile, d_env_report_here's report as section.name=value lines,
// noted on the node opened last: the module (env_tests_profile.c)
bool d_tests_env_profile(
    struct dj_test_suite* _suite
);


#endif  // DJINTERP_TESTS_C_ENV_TESTS_H
