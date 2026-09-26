/*******************************************************************************
* djinterp [env]                                                 env_printer.hpp
*
* djinterp environment printer.
*   Compile-time-aware printing of the build environment detected by env.h and
* env_cpp_features.h, to any target print.hpp's writers accept: console, file,
* string, or buffer.
*   Sections printed: the language standard (C / C++ version); the compiler
* and its version; the operating system and platform; the CPU architecture,
* bit width, and endianness; the POSIX level and features; the preprocessor
* translation limits; the build configuration (Debug / Release); and, when
* enabled, C runtime and C++ feature availability.
*   Usage:
*     djinterp::print_env(std::cout);              // to console
*     djinterp::print_env(my_file_ptr);            // to FILE*
*     std::string s; djinterp::print_env(s);       // to string
*     char buf[4096];                              // to buffer
*     auto bs = djinterp::make_buffer_state(buf, sizeof(buf));
*     djinterp::print_env(bs);
*   Configuration: D_ENV_PRINTER_INCLUDE_FEATURES set to 1 prints the
* individual C++ feature flags, and D_ENV_PRINTER_INCLUDE_C_FEATURES set to 1
* prints the individual C runtime feature flags; both default to 0, as the
* lists are long.
*   print.hpp's writers must be declared before this header is included; it
* does not include print.hpp itself.
*
* path:      /inc/djinterp/env/env_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.22
*                                                            revised: 2026.09.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONFIGURATION
    -------------
    1.  Output options
         1.  D_ENV_PRINTER_INCLUDE_FEATURES
         2.  D_ENV_PRINTER_INCLUDE_C_FEATURES
2.  SECTION PRINTERS
    ----------------
    1.  Language
    2.  Compiler
    3.  Operating system
    4.  Architecture
    5.  POSIX
    6.  Preprocessor limits
    7.  Build configuration
3.  OPTIONAL FEATURE PRINTERS
    -------------------------
    1.  C runtime features
    2.  C++ features
4.  MASTER PRINTER
    --------------
    1.  Full report
5.  FILE* OVERLOADS
    ---------------
    1.  FILE* printers
*/

#ifndef DJINTERP_ENV_ENV_PRINTER_HPP
#define DJINTERP_ENV_ENV_PRINTER_HPP 1

// std
#include <cstddef>                   // std::size_t
#include <cstdio>                    // std::FILE
// djinterp
#include "../djinterp.hpp"           // NS_DJINTERP, NS_END
#include "./env.h"                   // D_ENV_*
#include "./cpp/env_cpp_features.h"  // D_ENV_CPP_FEATURE_*


//==============================================================================
// 1.  CONFIGURATION
//==============================================================================


// 1.1    Output options
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_PRINTER_INCLUDE_FEATURES
//   configuration: when set to 1, print_env will include individual
// C++ feature test macro results. Disabled by default due to the
// volume of output.
#ifndef D_ENV_PRINTER_INCLUDE_FEATURES
    #define D_ENV_PRINTER_INCLUDE_FEATURES 0
#endif  // D_ENV_PRINTER_INCLUDE_FEATURES

// 1.1.2
// D_ENV_PRINTER_INCLUDE_C_FEATURES
//   configuration: when set to 1, print_env will include individual
// C runtime / standard-library feature results (the D_ENV_C_HAS_*
// family). Disabled by default due to the volume of output.
#ifndef D_ENV_PRINTER_INCLUDE_C_FEATURES
    #define D_ENV_PRINTER_INCLUDE_C_FEATURES 0
#endif  // D_ENV_PRINTER_INCLUDE_C_FEATURES


NS_DJINTERP


//==============================================================================
// 2.  SECTION PRINTERS
//==============================================================================
// One template per section of the report, each writing a header line followed
// by one key / value line per detected property.


// 2.1    Language
//------------------------------------------------------------------------------
/**
 * @brief Prints the detected language standards.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_language(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Language",
                                               _indent);

#ifdef D_ENV_LANG_CPP_STANDARD_NAME
    written += write_kv(_target,
                        "C++ Standard",
                        D_ENV_LANG_CPP_STANDARD_NAME,
                        _indent + 1);
#endif  // D_ENV_LANG_CPP_STANDARD_NAME

#ifdef D_ENV_LANG_C_STANDARD_NAME
    written += write_kv(_target,
                        "C Standard",
                        D_ENV_LANG_C_STANDARD_NAME,
                        _indent + 1);
#endif  // D_ENV_LANG_C_STANDARD_NAME

    written += write_kv(_target,
                        "Using C++",
                        static_cast<bool>(D_ENV_LANG_USING_CPP),
                        _indent + 1);

    written += write_kv(_target,
                        "Using C",
                        static_cast<bool>(D_ENV_LANG_USING_C),
                        _indent + 1);

    written += write_kv(_target,
                        "long long",
                        static_cast<bool>(D_ENV_HAS_LONG_LONG),
                        _indent + 1);

    return written;
}

// 2.2    Compiler
//------------------------------------------------------------------------------
/**
 * @brief Prints the detected compiler and its version.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_compiler(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Compiler",
                                               _indent);

#ifdef D_ENV_COMPILER_NAME
    written += write_kv(_target,
                        "Compiler",
                        D_ENV_COMPILER_NAME,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "Compiler",
                        "Unknown",
                        _indent + 1);
#endif  // D_ENV_COMPILER_NAME

#ifdef D_ENV_COMPILER_FULL_NAME
    written += write_kv(_target,
                        "Full Name",
                        D_ENV_COMPILER_FULL_NAME,
                        _indent + 1);
#endif  // D_ENV_COMPILER_FULL_NAME

#ifdef D_ENV_COMPILER_VERSION_STRING
    written += write_kv(_target,
                        "Version",
                        D_ENV_COMPILER_VERSION_STRING,
                        _indent + 1);
#endif  // D_ENV_COMPILER_VERSION_STRING

#ifdef D_ENV_COMPILER_MAJOR
    written += write_kv(_target,
                        "Major",
                        static_cast<long long>(D_ENV_COMPILER_MAJOR),
                        _indent + 1);
#endif  // D_ENV_COMPILER_MAJOR

#ifdef D_ENV_COMPILER_MINOR
    written += write_kv(_target,
                        "Minor",
                        static_cast<long long>(D_ENV_COMPILER_MINOR),
                        _indent + 1);
#endif  // D_ENV_COMPILER_MINOR

#ifdef D_ENV_COMPILER_PATCHLEVEL
    written += write_kv(_target,
                        "Patch",
                        static_cast<long long>(D_ENV_COMPILER_PATCHLEVEL),
                        _indent + 1);
#endif  // D_ENV_COMPILER_PATCHLEVEL

    return written;
}

// 2.3    Operating system
//------------------------------------------------------------------------------
/**
 * @brief Prints the detected operating system and platform.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_os(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Operating System",
                                               _indent);

#ifdef D_ENV_OS_NAME
    written += write_kv(_target,
                        "OS",
                        D_ENV_OS_NAME,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "OS",
                        "Unknown",
                        _indent + 1);
#endif  // D_ENV_OS_NAME

#ifdef D_ENV_PLATFORM_NAME
    written += write_kv(_target,
                        "Platform",
                        D_ENV_PLATFORM_NAME,
                        _indent + 1);
#endif  // D_ENV_PLATFORM_NAME

#ifdef D_ENV_OS_ID
    written += write_kv(_target,
                        "OS Flag",
                        static_cast<long long>(D_ENV_OS_ID),
                        _indent + 1);

    written += write_kv(_target,
                        "POSIX-like",
                        static_cast<bool>(D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)),
                        _indent + 1);

    written += write_kv(_target,
                        "Windows",
                        static_cast<bool>(D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)),
                        _indent + 1);
#endif  // D_ENV_OS_ID

    return written;
}

// 2.4    Architecture
//------------------------------------------------------------------------------
/**
 * @brief Prints the detected CPU architecture, bit width, and endianness.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_arch(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Architecture",
                                               _indent);

#ifdef D_ENV_ARCH_NAME
    written += write_kv(_target,
                        "Architecture",
                        D_ENV_ARCH_NAME,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "Architecture",
                        "Unknown",
                        _indent + 1);
#endif  // D_ENV_ARCH_NAME

#ifdef D_ENV_ARCH_BITS
    written += write_kv(_target,
                        "Bit Width",
                        static_cast<long long>(D_ENV_ARCH_BITS),
                        _indent + 1);
#endif  // D_ENV_ARCH_BITS

    written += write_kv(_target,
                        "Endianness",
                        D_ENV_ARCH_IS_LITTLE_ENDIAN ? "Little"
                      : D_ENV_ARCH_IS_BIG_ENDIAN    ? "Big"
                                                    : "Unknown",
                        _indent + 1);

    return written;
}

// 2.5    POSIX
//------------------------------------------------------------------------------
/**
 * @brief Prints the detected POSIX level and feature availability.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_posix(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "POSIX",
                                               _indent);

#ifdef D_ENV_POSIX_NAME
    written += write_kv(_target,
                        "POSIX",
                        D_ENV_POSIX_NAME,
                        _indent + 1);
#endif  // D_ENV_POSIX_NAME

    written += write_kv(_target,
                        "Available",
                        static_cast<bool>(D_ENV_POSIX_IS_AVAILABLE),
                        _indent + 1);

#ifdef D_ENV_POSIX_XSI_NAME
    written += write_kv(_target,
                        "XSI",
                        D_ENV_POSIX_XSI_NAME,
                        _indent + 1);
#endif  // D_ENV_POSIX_XSI_NAME

    written += write_kv(_target,
                        "Threads",
                        static_cast<bool>(D_ENV_POSIX_FEATURE_THREADS),
                        _indent + 1);

    written += write_kv(_target,
                        "Realtime",
                        static_cast<bool>(D_ENV_POSIX_FEATURE_REALTIME),
                        _indent + 1);

    written += write_kv(_target,
                        "Sockets",
                        static_cast<bool>(D_ENV_POSIX_FEATURE_SOCKETS),
                        _indent + 1);

    return written;
}

// 2.6    Preprocessor limits
//------------------------------------------------------------------------------
/**
 * @brief Prints the detected preprocessor translation limits.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_pp_limits(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Preprocessor Limits",
                                               _indent);

#ifdef D_ENV_PP_LIMIT_SOURCE
    written += write_kv(_target,
                        "Source",
                        D_ENV_PP_LIMIT_SOURCE,
                        _indent + 1);
#endif  // D_ENV_PP_LIMIT_SOURCE

    written += write_kv(_target,
                        "Max Macro Args",
                        static_cast<long long>(D_ENV_PP_MAX_MACRO_ARGS),
                        _indent + 1);

    written += write_kv(_target,
                        "Max Nesting Depth",
                        static_cast<long long>(D_ENV_PP_MAX_NESTING_DEPTH),
                        _indent + 1);

    written += write_kv(_target,
                        "Has __VA_OPT__",
                        static_cast<bool>(D_ENV_PP_HAS_VA_OPT),
                        _indent + 1);

    return written;
}

// 2.7    Build configuration
//------------------------------------------------------------------------------
/**
 * @brief Prints the build configuration (Debug / Release).
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_build(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Build",
                                               _indent);

#ifdef D_ENV_BUILD_TYPE
    written += write_kv(_target,
                        "Build Type",
                        D_ENV_BUILD_TYPE,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "Build Type",
                        "Unknown",
                        _indent + 1);
#endif  // D_ENV_BUILD_TYPE

#ifdef D_ENV_BUILD_DEBUG
    written += write_kv(_target,
                        "Debug",
                        true,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "Debug",
                        false,
                        _indent + 1);
#endif  // D_ENV_BUILD_DEBUG

    return written;
}


//==============================================================================
// 3.  OPTIONAL FEATURE PRINTERS
//==============================================================================
// Compiled only when the matching D_ENV_PRINTER_INCLUDE_* switch is 1, since
// each list is long.


// 3.1    C runtime features
//------------------------------------------------------------------------------
#if D_ENV_PRINTER_INCLUDE_C_FEATURES

/**
 * @brief Prints C runtime feature availability (the D_ENV_C_HAS_* family).
 *
 * @note declared only when D_ENV_PRINTER_INCLUDE_C_FEATURES is 1.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_c_features(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = 0;

#ifdef __STDC_HOSTED__
    written += write_section_header(_target,
                                    "C Runtime Features",
                                    _indent);

    written += write_kv(_target,
                        "C11 threads",
                        static_cast<bool>(D_ENV_C_HAS_C11_THREADS),
                        _indent + 1);
    written += write_kv(_target,
                        "pthread",
                        static_cast<bool>(D_ENV_C_HAS_PTHREAD),
                        _indent + 1);
    written += write_kv(_target,
                        "stdatomic",
                        static_cast<bool>(D_ENV_C_HAS_STDATOMIC),
                        _indent + 1);
    written += write_kv(_target,
                        "stdint.h",
                        static_cast<bool>(D_ENV_C_HAS_STDINT_H),
                        _indent + 1);
    written += write_kv(_target,
                        "unistd.h",
                        static_cast<bool>(D_ENV_C_HAS_UNISTD_H),
                        _indent + 1);
    written += write_kv(_target,
                        "mmap",
                        static_cast<bool>(D_ENV_C_HAS_MMAP),
                        _indent + 1);
    written += write_kv(_target,
                        "fork",
                        static_cast<bool>(D_ENV_C_HAS_FORK),
                        _indent + 1);
    written += write_kv(_target,
                        "VLA",
                        static_cast<bool>(D_ENV_C_HAS_VLA),
                        _indent + 1);
    written += write_kv(_target,
                        "SSE",
                        static_cast<bool>(D_ENV_C_HAS_SSE),
                        _indent + 1);
    written += write_kv(_target,
                        "AVX",
                        static_cast<bool>(D_ENV_C_HAS_AVX),
                        _indent + 1);
    written += write_kv(_target,
                        "NEON",
                        static_cast<bool>(D_ENV_C_HAS_NEON),
                        _indent + 1);
#endif  // __STDC_HOSTED__

    return written;
}

#endif  // D_ENV_PRINTER_INCLUDE_C_FEATURES

// 3.2    C++ features
//------------------------------------------------------------------------------
#if D_ENV_PRINTER_INCLUDE_FEATURES

/**
 * @brief Prints one C++ feature-test result as a status line.
 *
 * @note declared only when D_ENV_PRINTER_INCLUDE_FEATURES is 1.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target   the output target.
 * @param[in]     _name     the feature-test macro's name.
 * @param[in]     _desc     a human-readable description of the feature.
 * @param[in]     _enabled  nonzero if the feature is available.
 * @param[in]     _version  the standard that introduced the feature.
 * @param[in]     _indent   the indentation level of the line.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_cpp_feature_line(
    Target&     _target,
    const char* _name,
    const char* _desc,
    int         _enabled,
    const char* _version,
    std::size_t _indent
)
{
    std::size_t written = write_indent(_target,
                                       _indent);
    written += write_to(_target, _enabled ? "[+] " : "[-] ");
    written += write_to(_target,
                        _desc);
    written += write_to(_target,
                        " ");
    written += write_to(_target,
                        _version);
    written += write_to(_target,
                        " ("); written += write_to(_target,
                        _name); written += write_to(_target,
                        ")");
    written += write_newline(_target);

    return written;
}

/**
 * @brief Prints the C++11 language feature-test results.
 *
 * @note declared only when D_ENV_PRINTER_INCLUDE_FEATURES is 1.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_cpp_features_lang_cpp11(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_line_to(_target,
                                        "C++11 Language Features:");

    written += print_env_cpp_feature_line(_target,
                                          D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_NAME,
                                          D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_DESC,
                                          D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES,
                                          D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VERS,
                                          _indent + 1);

    written += print_env_cpp_feature_line(_target,
                                          D_ENV_CPP_FEATURE_LANG_CONSTEXPR_NAME,
                                          D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DESC,
                                          D_ENV_CPP_FEATURE_LANG_CONSTEXPR,
                                          D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VERS,
                                          _indent + 1);

    written += print_env_cpp_feature_line(_target,
                                          D_ENV_CPP_FEATURE_LANG_DECLTYPE_NAME,
                                          D_ENV_CPP_FEATURE_LANG_DECLTYPE_DESC,
                                          D_ENV_CPP_FEATURE_LANG_DECLTYPE,
                                          D_ENV_CPP_FEATURE_LANG_DECLTYPE_VERS,
                                          _indent + 1);

    written += print_env_cpp_feature_line(_target,
                                          D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_NAME,
                                          D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_DESC,
                                          D_ENV_CPP_FEATURE_LANG_ATTRIBUTES,
                                          D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VERS,
                                          _indent + 1);

    return written;
}

/**
 * @brief Prints the aggregate C++ feature availability flags.
 *
 * @note declared only when D_ENV_PRINTER_INCLUDE_FEATURES is 1.
 * @note prints nothing below C++11.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env_cpp_features_aggregate(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = 0;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    written += write_section_header(_target,
                                    "C++ Feature Aggregates",
                                    _indent);

    written += write_kv(_target,
                        "All C++14 Lang Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++17 Lang Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++20 Lang Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++23 Lang Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++14 STL Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++17 STL Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++20 STL Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++23 STL Features",
                        static_cast<bool>(D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23),
                        _indent + 1);
#endif

    return written;
}

#endif  // D_ENV_PRINTER_INCLUDE_FEATURES


//==============================================================================
// 4.  MASTER PRINTER
//==============================================================================


// 4.1    Full report
//------------------------------------------------------------------------------
/**
 * @brief Prints every section of the detected environment.
 *
 * @tparam Target  any output target print.hpp's writers accept.
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
template<typename Target>
inline std::size_t
print_env(
    Target&     _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "djinterp Environment",
                                               _indent);
    written += write_newline(_target);

    written += print_env_language(_target,
                                  _indent);
    written += write_newline(_target);

    written += print_env_compiler(_target,
                                  _indent);
    written += write_newline(_target);

    written += print_env_os(_target,
                            _indent);
    written += write_newline(_target);

    written += print_env_arch(_target,
                              _indent);
    written += write_newline(_target);

    written += print_env_posix(_target,
                               _indent);
    written += write_newline(_target);

    written += print_env_pp_limits(_target,
                                   _indent);
    written += write_newline(_target);

    written += print_env_build(_target,
                               _indent);

#if D_ENV_PRINTER_INCLUDE_C_FEATURES
    written += write_newline(_target);
    written += print_env_c_features(_target,
                                    _indent);
#endif

#if D_ENV_PRINTER_INCLUDE_FEATURES
    written += write_newline(_target);
    written += print_env_cpp_features_aggregate(_target,
                                                _indent);
#endif

    return written;
}


//==============================================================================
// 5.  FILE* OVERLOADS
//==============================================================================
// Non-template overloads for FILE*, which overload resolution prefers to the
// templates when the argument is a FILE*. They print a reduced report; see
// each note.


// 5.1    FILE* printers
//------------------------------------------------------------------------------
/**
 * @brief Prints the detected language standards.
 *
 * @note `FILE*` overload, preferred over the template for a `FILE*` argument;
 *       it prints the standard names only: unlike the template, it omits the
 *       using-C / using-C++ and long long lines.
 *
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
inline std::size_t
print_env_language(
    std::FILE*  _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Language",
                                               _indent);

#ifdef D_ENV_LANG_CPP_STANDARD_NAME
    written += write_kv(_target,
                        "C++ Standard",
                        D_ENV_LANG_CPP_STANDARD_NAME,
                        _indent + 1);
#endif  // D_ENV_LANG_CPP_STANDARD_NAME

#ifdef D_ENV_LANG_C_STANDARD_NAME
    written += write_kv(_target,
                        "C Standard",
                        D_ENV_LANG_C_STANDARD_NAME,
                        _indent + 1);
#endif  // D_ENV_LANG_C_STANDARD_NAME

    return written;
}

/**
 * @brief Prints the detected compiler and its version.
 *
 * @note `FILE*` overload, preferred over the template for a `FILE*` argument;
 *       it prints the name and version string only: unlike the template, it
 *       omits the full name and the numeric version parts.
 *
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
inline std::size_t
print_env_compiler(
    std::FILE*  _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Compiler",
                                               _indent);

#ifdef D_ENV_COMPILER_NAME
    written += write_kv(_target,
                        "Compiler",
                        D_ENV_COMPILER_NAME,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "Compiler",
                        "Unknown",
                        _indent + 1);
#endif  // D_ENV_COMPILER_NAME

#ifdef D_ENV_COMPILER_VERSION_STRING
    written += write_kv(_target,
                        "Version",
                        D_ENV_COMPILER_VERSION_STRING,
                        _indent + 1);
#endif  // D_ENV_COMPILER_VERSION_STRING

    return written;
}

/**
 * @brief Prints the detected operating system and platform.
 *
 * @note `FILE*` overload, preferred over the template for a `FILE*` argument;
 *       it prints the OS name only: unlike the template, it omits the platform,
 *       OS flag, and POSIX-like / Windows lines.
 *
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
inline std::size_t
print_env_os(
    std::FILE*  _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "Operating System",
                                               _indent);

#ifdef D_ENV_OS_NAME
    written += write_kv(_target,
                        "OS",
                        D_ENV_OS_NAME,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "OS",
                        "Unknown",
                        _indent + 1);
#endif  // D_ENV_OS_NAME

    return written;
}

/**
 * @brief Prints every section of the detected environment.
 *
 * @note `FILE*` overload, preferred over the template for a `FILE*` argument;
 *       it prints the language, compiler, and OS sections only; the template
 *       also prints architecture, POSIX, preprocessor limits, build, and the
 *       optional feature sections.
 *
 * @param[in,out] _target  the output target.
 * @param[in]     _indent  the indentation level of the section header; its
 *                         entries are indented one level further.
 * @return the number of characters written, as print.hpp's writers count them.
 */
inline std::size_t
print_env(
    std::FILE*  _target,
    std::size_t _indent = 0
)
{
    std::size_t written = write_section_header(_target,
                                               "djinterp Environment",
                                               _indent);
    written += write_newline(_target);

    written += print_env_language(_target,
                                  _indent);
    written += write_newline(_target);

    written += print_env_compiler(_target,
                                  _indent);
    written += write_newline(_target);

    written += print_env_os(_target,
                            _indent);

    return written;
}


NS_END  // djinterp


#endif  // DJINTERP_ENV_ENV_PRINTER_HPP
