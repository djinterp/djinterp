/******************************************************************************
* djinterp [core] unit tests                                test_env_config.h
* 
* test environment configuration helper
* This header demonstrates how to simulate different environments for testing
* env.h detection logic using the D_CFG_ENV_CUSTOM system.
*
* usage:
*   define one of the test scenario macros before including this header,
*   then include env.h:
*   
*   #define TEST_SCENARIO_WINDOWS_MSVC
*   #include "test_env_config.h"
*   #include "../env.h"
*
* path:      \tests\test_env_config.h
* author(s): [Your Name]                                      date: 2024.12.24
******************************************************************************/

#ifndef DJINTERP_TEST_ENV_CONFIG_
#define DJINTERP_TEST_ENV_CONFIG_ 1

// =============================================================================
// I. COMMON TEST SCENARIOS
// =============================================================================

// TEST_SCENARIO_LINUX_GCC
//   configuration: simulates a Linux system with GCC compiler.
// test coverage:
//   - Unix-like OS detection
//   - GCC compiler detection
//   - POSIX environment
//   - x86-64 architecture (typical)
#ifdef TEST_SCENARIO_LINUX_GCC
    // OS detection
    #define D_ENV_DETECTED_OS_LINUX     1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_GCC 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_X64     1
    
    // POSIX detection
    #define D_ENV_DETECTED_POSIX_2008   1
    #define D_ENV_DETECTED_POSIX_THREADS 1
    #define D_ENV_DETECTED_POSIX_REALTIME 1
    
    // language detection
    #define D_ENV_DETECTED_C11          1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SCENARIO_LINUX_GCC

// TEST_SCENARIO_WINDOWS_MSVC
//   configuration: simulates a Windows system with MSVC compiler.
// test coverage:
//   - Windows OS detection
//   - MSVC compiler detection
//   - x86-64 architecture
//   - no POSIX environment
#ifdef TEST_SCENARIO_WINDOWS_MSVC
    // OS detection
    #define D_ENV_DETECTED_OS_WINDOWS   1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_MSVC 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_X64     1
    
    // POSIX detection (none on Windows)
    #define D_ENV_DETECTED_POSIX_NONE   1
    
    // language detection (C++ compilation)
    #define D_ENV_DETECTED_CPP17        1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_RELEASE 1
#endif  // TEST_SCENARIO_WINDOWS_MSVC

// TEST_SCENARIO_MACOS_CLANG
//   configuration: simulates macOS with Apple Clang compiler.
// test coverage:
//   - Apple OS detection
//   - Clang compiler detection
//   - ARM64 architecture (Apple Silicon)
//   - POSIX environment
#ifdef TEST_SCENARIO_MACOS_CLANG
    // OS detection
    #define D_ENV_DETECTED_OS_MACOS     1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_APPLE_CLANG 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_ARM64   1
    
    // POSIX detection
    #define D_ENV_DETECTED_POSIX_2008   1
    #define D_ENV_DETECTED_POSIX_THREADS 1
    
    // language detection
    #define D_ENV_DETECTED_C17          1
    #define D_ENV_DETECTED_CPP20        1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SCENARIO_MACOS_CLANG

// TEST_SCENARIO_EMBEDDED_ARM
//   configuration: simulates an embedded ARM system.
// test coverage:
//   - minimal OS (bare metal or RTOS)
//   - GCC cross-compiler
//   - ARM 32-bit architecture
//   - no POSIX
#ifdef TEST_SCENARIO_EMBEDDED_ARM
    // OS detection
    #define D_ENV_DETECTED_OS_UNKNOWN   1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_GCC 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_ARM     1
    
    // POSIX detection (none in embedded)
    #define D_ENV_DETECTED_POSIX_NONE   1
    
    // language detection
    #define D_ENV_DETECTED_C99          1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_RELEASE 1
#endif  // TEST_SCENARIO_EMBEDDED_ARM

// TEST_SCENARIO_BSD_CLANG
//   configuration: simulates FreeBSD with Clang compiler.
// test coverage:
//   - BSD OS detection
//   - Clang compiler detection
//   - x86-64 architecture
//   - full POSIX support
#ifdef TEST_SCENARIO_BSD_CLANG
    // OS detection
    #define D_ENV_DETECTED_OS_BSD       1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_CLANG 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_X64     1
    
    // POSIX detection
    #define D_ENV_DETECTED_POSIX_2017   1
    #define D_ENV_DETECTED_POSIX_THREADS 1
    #define D_ENV_DETECTED_POSIX_REALTIME 1
    #define D_ENV_DETECTED_POSIX_SOCKETS 1
    
    // language detection
    #define D_ENV_DETECTED_C11          1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SCENARIO_BSD_CLANG


// =============================================================================
// II. SPECIALIZED TEST SCENARIOS
// =============================================================================

// TEST_SCENARIO_OLD_COMPILER
//   configuration: simulates an older compiler environment.
// test coverage:
//   - legacy C standard (C95)
//   - older POSIX version
//   - testing backward compatibility
#ifdef TEST_SCENARIO_OLD_COMPILER
    // OS detection
    #define D_ENV_DETECTED_OS_LINUX     1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_GCC 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_X86     1
    
    // POSIX detection
    #define D_ENV_DETECTED_POSIX_1993   1
    
    // language detection
    #define D_ENV_DETECTED_C95          1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SCENARIO_OLD_COMPILER

// TEST_SCENARIO_MODERN_CPP
//   configuration: simulates modern C++ development environment.
// test coverage:
//   - latest C++ standard (C++23)
//   - modern POSIX
//   - testing cutting-edge features
#ifdef TEST_SCENARIO_MODERN_CPP
    // OS detection
    #define D_ENV_DETECTED_OS_LINUX     1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_CLANG 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_X64     1
    
    // POSIX detection
    #define D_ENV_DETECTED_POSIX_2024   1
    #define D_ENV_DETECTED_POSIX_THREADS 1
    #define D_ENV_DETECTED_POSIX_REALTIME 1
    #define D_ENV_DETECTED_POSIX_SOCKETS 1
    
    // language detection
    #define D_ENV_DETECTED_CPP23        1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_RELEASE 1
#endif  // TEST_SCENARIO_MODERN_CPP

// TEST_SCENARIO_CROSS_PLATFORM
//   configuration: simulates a cross-platform development setup.
// test coverage:
//   - testing with multiple configurations
//   - validating platform abstraction
#ifdef TEST_SCENARIO_CROSS_PLATFORM
    // OS detection (can be overridden)
    #ifndef D_ENV_DETECTED_OS_LINUX
        #define D_ENV_DETECTED_OS_LINUX 1
    #endif
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_GCC 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_X64     1
    
    // POSIX detection
    #define D_ENV_DETECTED_POSIX_2008   1
    
    // language detection (both C and C++)
    #define D_ENV_DETECTED_C11          1
    #define D_ENV_DETECTED_CPP17        1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SCENARIO_CROSS_PLATFORM

// TEST_SCENARIO_MINIMAL
//   configuration: simulates minimal environment with basic detection only.
// test coverage:
//   - testing with minimal features
//   - validating fallback paths
#ifdef TEST_SCENARIO_MINIMAL
    // OS detection
    #define D_ENV_DETECTED_OS_UNKNOWN   1
    
    // compiler detection
    #define D_ENV_DETECTED_COMPILER_UNKNOWN 1
    
    // architecture detection
    #define D_ENV_DETECTED_ARCH_UNKNOWN 1
    
    // POSIX detection (none)
    #define D_ENV_DETECTED_POSIX_NONE   1
    
    // language detection (basic C)
    #define D_ENV_DETECTED_C95          1
    
    // build configuration
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SCENARIO_MINIMAL


// =============================================================================
// III. SELECTIVE DETECTION TESTING
// =============================================================================

// TEST_SELECTIVE_LANG_ONLY
//   configuration: tests with only language detection enabled.
// test coverage:
//   - selective section enable/disable
//   - D_CFG_ENV_CUSTOM bitfield functionality
#ifdef TEST_SELECTIVE_LANG_ONLY
    #define D_CFG_ENV_CUSTOM (D_CFG_ENV_BIT_POSIX  | \
                              D_CFG_ENV_BIT_COMPILER | \
                              D_CFG_ENV_BIT_OS     | \
                              D_CFG_ENV_BIT_ARCH   | \
                              D_CFG_ENV_BIT_BUILD)
    
    // manually define non-language sections
    #define D_ENV_DETECTED_COMPILER_GCC 1
    #define D_ENV_DETECTED_OS_LINUX     1
    #define D_ENV_DETECTED_ARCH_X64     1
    #define D_ENV_DETECTED_POSIX_2008   1
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SELECTIVE_LANG_ONLY

// TEST_SELECTIVE_COMPILER_ONLY
//   configuration: tests with only compiler detection enabled.
#ifdef TEST_SELECTIVE_COMPILER_ONLY
    #define D_CFG_ENV_CUSTOM (D_CFG_ENV_BIT_LANG  | \
                              D_CFG_ENV_BIT_POSIX | \
                              D_CFG_ENV_BIT_OS    | \
                              D_CFG_ENV_BIT_ARCH  | \
                              D_CFG_ENV_BIT_BUILD)
    
    // manually define non-compiler sections
    #define D_ENV_DETECTED_C11          1
    #define D_ENV_DETECTED_OS_LINUX     1
    #define D_ENV_DETECTED_ARCH_X64     1
    #define D_ENV_DETECTED_POSIX_2008   1
    #define D_ENV_DETECTED_BUILD_DEBUG  1
#endif  // TEST_SELECTIVE_COMPILER_ONLY


// =============================================================================
// IV. VALIDATION HELPERS
// =============================================================================

// D_TEST_VALIDATE_CONFIG
//   macro: validates that test configuration is properly set up.
// usage:
//   call this at the start of test functions to verify environment is
//   correctly configured.
#define D_TEST_VALIDATE_CONFIG() \
    do { \
        if (!D_CFG_ENV_CUSTOM) { \
            printf("WARNING: using automatic detection, " \
                   "not simulated environment\n"); \
        } \
    } while(0)

// D_TEST_PRINT_CONFIG
//   macro: prints the current test configuration for debugging.
#define D_TEST_PRINT_CONFIG() \
    do { \
        printf("Test Configuration:\n"); \
        printf("  D_CFG_ENV_CUSTOM: 0x%02X\n", D_CFG_ENV_CUSTOM); \
        printf("  Sections disabled:\n"); \
        if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_LANG) \
            printf("    - Language\n"); \
        if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_POSIX) \
            printf("    - POSIX\n"); \
        if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_COMPILER) \
            printf("    - Compiler\n"); \
        if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_OS) \
            printf("    - OS\n"); \
        if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_ARCH) \
            printf("    - Architecture\n"); \
        if (D_CFG_ENV_CUSTOM & D_CFG_ENV_BIT_BUILD) \
            printf("    - Build\n"); \
    } while(0)


#endif  // DJINTERP_TEST_ENV_CONFIG_
