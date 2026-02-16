/******************************************************************************
* djinterp [test]                                               env_tests_sa.h
*
* This is a test file for `env.h` unit tests.
* For the file itself, go to `\inc\c\env.h`.
* Note: this module is required to build DTest, so it uses `test_standalone.h`,
* rather than DTest for unit testing. Any modules that are not dependencies of
* DTest should use DTest for unit tests.
* 
*
* path:      \test\env_tests_sa.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.09.26
******************************************************************************/

#ifndef DJINTERP_TESTING_ENV_
#define DJINTERP_TESTING_ENV_ 1

#include <stdlib.h>
#include <stdint.h>
#include "..\..\inc\c\djinterp.h"
#include "..\..\inc\c\test\test_standalone.h"


// =============================================================================
// I.   CFG CONFIGURATION SYSTEM TEST FUNCTIONS
// =============================================================================

// cfg bit flag tests
bool d_tests_sa_env_cfg_bit_flags(struct d_test_counter* _test_info);
bool d_tests_sa_env_cfg_custom_value(struct d_test_counter* _test_info);

// cfg section enable macro tests
bool d_tests_sa_env_cfg_enabled_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_cfg_enabled_zero_case(struct d_test_counter* _test_info);

// cfg detected variable tests
bool d_tests_sa_env_cfg_detected_vars_lang(struct d_test_counter* _test_info);
bool d_tests_sa_env_cfg_detected_vars_posix(struct d_test_counter* _test_info);
bool d_tests_sa_env_cfg_detected_vars_compiler(struct d_test_counter* _test_info);
bool d_tests_sa_env_cfg_detected_vars_os(struct d_test_counter* _test_info);
bool d_tests_sa_env_cfg_detected_vars_arch(struct d_test_counter* _test_info);
bool d_tests_sa_env_cfg_detected_vars_build(struct d_test_counter* _test_info);

// cfg module aggregator
bool d_tests_sa_env_cfg_all(struct d_test_counter* _test_info);


// =============================================================================
// II.  LANGUAGE ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// standard version constant tests
bool d_tests_sa_env_lang_c_standard_constants(struct d_test_counter* _test_info);
bool d_tests_sa_env_lang_cpp_standard_constants(struct d_test_counter* _test_info);
bool d_tests_sa_env_lang_standard_constant_ranges(struct d_test_counter* _test_info);

// C standard detection tests
bool d_tests_sa_env_lang_c_standard_detection(struct d_test_counter* _test_info);
bool d_tests_sa_env_lang_c_standard_fallback(struct d_test_counter* _test_info);

// C++ standard detection tests
bool d_tests_sa_env_lang_cpp_standard_detection(struct d_test_counter* _test_info);

// version comparison macro tests
bool d_tests_sa_env_lang_c_version_comparison_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_lang_cpp_version_comparison_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_lang_version_comparison_boundary_cases(struct d_test_counter* _test_info);

// convenience macro tests
bool d_tests_sa_env_lang_using_macros(struct d_test_counter* _test_info);

// manual detection variable tests
bool d_tests_sa_env_lang_detected_vars_c(struct d_test_counter* _test_info);
bool d_tests_sa_env_lang_detected_vars_cpp(struct d_test_counter* _test_info);

// consistency and edge case tests
bool d_tests_sa_env_lang_consistency_check(struct d_test_counter* _test_info);
bool d_tests_sa_env_lang_feature_test_macros(struct d_test_counter* _test_info);

// language module aggregator
bool d_tests_sa_env_lang_all(struct d_test_counter* _test_info);


// =============================================================================
// III. POSIX ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// POSIX version constant tests
bool d_tests_sa_env_posix_version_constants(struct d_test_counter* _test_info);
bool d_tests_sa_env_posix_c_source_constants(struct d_test_counter* _test_info);
bool d_tests_sa_env_posix_xsi_version_constants(struct d_test_counter* _test_info);

// POSIX version detection tests
bool d_tests_sa_env_posix_version_detection(struct d_test_counter* _test_info);
bool d_tests_sa_env_posix_version_name_consistency(struct d_test_counter* _test_info);

// XSI detection tests
bool d_tests_sa_env_posix_xsi_detection(struct d_test_counter* _test_info);

// POSIX feature macro tests
bool d_tests_sa_env_posix_feature_macros(struct d_test_counter* _test_info);

// POSIX utility macro tests
bool d_tests_sa_env_posix_utility_is_available(struct d_test_counter* _test_info);
bool d_tests_sa_env_posix_utility_is_modern(struct d_test_counter* _test_info);
bool d_tests_sa_env_posix_utility_version_at_least(struct d_test_counter* _test_info);
bool d_tests_sa_env_posix_utility_has_feature(struct d_test_counter* _test_info);

// manual detection tests
bool d_tests_sa_env_posix_manual_detection(struct d_test_counter* _test_info);

// consistency and integration tests
bool d_tests_sa_env_posix_consistency_check(struct d_test_counter* _test_info);
bool d_tests_sa_env_posix_feature_test_patterns(struct d_test_counter* _test_info);

// POSIX module aggregator
bool d_tests_sa_env_posix_all(struct d_test_counter* _test_info);


// =============================================================================
// IV.  COMPILER ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// compiler detection tests
bool d_tests_sa_env_compiler_detection_flags(struct d_test_counter* _test_info);
bool d_tests_sa_env_compiler_name_macros(struct d_test_counter* _test_info);

// compiler version tests
bool d_tests_sa_env_compiler_version_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_compiler_version_at_least(struct d_test_counter* _test_info);
bool d_tests_sa_env_compiler_version_at_most(struct d_test_counter* _test_info);

// VA_OPT detection tests
bool d_tests_sa_env_compiler_va_opt_detection(struct d_test_counter* _test_info);

// preprocessor limit tests
bool d_tests_sa_env_compiler_pp_limit_constants(struct d_test_counter* _test_info);
bool d_tests_sa_env_compiler_pp_min_limits(struct d_test_counter* _test_info);
bool d_tests_sa_env_compiler_pp_max_limits(struct d_test_counter* _test_info);
bool d_tests_sa_env_compiler_pp_limit_utility_macros(struct d_test_counter* _test_info);

// manual detection tests
bool d_tests_sa_env_compiler_manual_detection(struct d_test_counter* _test_info);

// consistency tests
bool d_tests_sa_env_compiler_consistency_check(struct d_test_counter* _test_info);

// compiler module aggregator
bool d_tests_sa_env_compiler_all(struct d_test_counter* _test_info);


// =============================================================================
// IV-A. PREPROCESSOR LIMITS TEST FUNCTIONS
// =============================================================================

// standard limit constant tests
bool d_tests_sa_env_pp_limit_constants_c89(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_limit_constants_c99(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_limit_constants_cpp(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_limit_constants_all(struct d_test_counter* _test_info);

// standard-based minimum limit tests
bool d_tests_sa_env_pp_min_limits_definition(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_min_limits_values(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_min_limits_all(struct d_test_counter* _test_info);

// compiler-specific maximum limit tests
bool d_tests_sa_env_pp_max_limits_definition(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_max_limits_values(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_max_limits_all(struct d_test_counter* _test_info);

// utility macro tests
bool d_tests_sa_env_pp_utility_macros_definition(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_utility_macros_functionality(struct d_test_counter* _test_info);
bool d_tests_sa_env_pp_utility_macros_all(struct d_test_counter* _test_info);

// __VA_OPT__ detection tests
bool d_tests_sa_env_pp_va_opt_detection(struct d_test_counter* _test_info);

// preprocessor limits module aggregator
bool d_tests_sa_env_pp_limits_all(struct d_test_counter* _test_info);


// =============================================================================
// VI.   ARCHITECTURE ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// architecture constant tests
bool d_tests_sa_env_arch_type_constants(struct d_test_counter* _test_info);
bool d_tests_sa_env_arch_endian_constants(struct d_test_counter* _test_info);

// architecture detection tests
bool d_tests_sa_env_arch_detection_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_arch_property_values(struct d_test_counter* _test_info);
bool d_tests_sa_env_arch_detection_all(struct d_test_counter* _test_info);

// architecture utility macro tests
bool d_tests_sa_env_arch_family_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_arch_bitwidth_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_arch_endianness_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_arch_utility_macros_all(struct d_test_counter* _test_info);

// architecture consistency tests
bool d_tests_sa_env_arch_consistency_check(struct d_test_counter* _test_info);

// architecture module aggregator
bool d_tests_sa_env_arch_all(struct d_test_counter* _test_info);


// =============================================================================
// VIII.  OPERATING SYSTEM ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// OS flag constant tests
bool d_tests_sa_env_os_flag_constants_apple(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_flag_constants_unix(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_flag_constants_bsd(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_flag_constants_windows(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_flag_constants_mobile(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_flag_constants_legacy(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_flag_constants_all(struct d_test_counter* _test_info);

// OS utility constant tests
bool d_tests_sa_env_os_utility_constants(struct d_test_counter* _test_info);

// OS utility macro tests
bool d_tests_sa_env_os_utility_macro_block(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_utility_macro_unix(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_utility_macro_mobile(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_utility_macro_windows(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_utility_macro_discontinued(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_utility_macro_posix(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_utility_macros_all(struct d_test_counter* _test_info);

// OS detection tests
bool d_tests_sa_env_os_detection_macros(struct d_test_counter* _test_info);
bool d_tests_sa_env_os_platform_macros(struct d_test_counter* _test_info);

// OS module aggregator
bool d_tests_sa_env_os_all(struct d_test_counter* _test_info);

// =============================================================================
// IX. STANDARD LIBRARY FEATURE DETECTION TEST FUNCTIONS
// =============================================================================

// C standard header availability tests
bool d_tests_sa_env_stdlib_c_standard_headers(struct d_test_counter* _test_info);
bool d_tests_sa_env_stdlib_posix_headers(struct d_test_counter* _test_info);

// string and memory function tests
bool d_tests_sa_env_stdlib_string_functions(struct d_test_counter* _test_info);

// file I/O feature tests
bool d_tests_sa_env_stdlib_file_io_features(struct d_test_counter* _test_info);

// time and date feature tests
bool d_tests_sa_env_stdlib_time_features(struct d_test_counter* _test_info);

// math feature tests
bool d_tests_sa_env_stdlib_math_features(struct d_test_counter* _test_info);

// network feature tests
bool d_tests_sa_env_stdlib_network_features(struct d_test_counter* _test_info);

// process and system feature tests
bool d_tests_sa_env_stdlib_process_features(struct d_test_counter* _test_info);

// memory management feature tests
bool d_tests_sa_env_stdlib_memory_features(struct d_test_counter* _test_info);

// SIMD and hardware intrinsics tests
bool d_tests_sa_env_stdlib_simd_features(struct d_test_counter* _test_info);

// VLA and security feature tests
bool d_tests_sa_env_stdlib_vla_and_security(struct d_test_counter* _test_info);

// consistency tests
bool d_tests_sa_env_stdlib_consistency_check(struct d_test_counter* _test_info);

// stdlib module aggregator
bool d_tests_sa_env_stdlib_all(struct d_test_counter* _test_info);


// =============================================================================
// X. BUILD CONFIGURATION ENVIRONMENT TEST FUNCTIONS
// =============================================================================

// build configuration tests
bool d_tests_sa_env_build_config_defined(struct d_test_counter* _test_info);
bool d_tests_sa_env_build_config_values(struct d_test_counter* _test_info);
bool d_tests_sa_env_build_config_consistency(struct d_test_counter* _test_info);

// build detection logic tests
bool d_tests_sa_env_build_detection_logic(struct d_test_counter* _test_info);

// build type string tests
bool d_tests_sa_env_build_type_string(struct d_test_counter* _test_info);

// conditional compilation tests
bool d_tests_sa_env_build_conditional_compilation(struct d_test_counter* _test_info);

// build usage tests
bool d_tests_sa_env_build_usage_examples(struct d_test_counter* _test_info);

// build module aggregator
bool d_tests_sa_env_build_all(struct d_test_counter* _test_info);


// =============================================================================
// XI.   MASTER TEST SUITE
// =============================================================================

bool d_tests_sa_env_all(struct d_test_counter* _test_info);


#endif	// DJINTERP_TESTING_ENV_





