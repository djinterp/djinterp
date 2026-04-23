/******************************************************************************
* djinterp [core]                                           env_printer.hpp
*
* djinterp environment printer header:
*   This header provides compile-time-aware printing of the detected build
* environment gathered by env.h and cpp_features.h. Output can be directed
* to any target supported by print.hpp (console, file, string, buffer).
*
*   SECTIONS PRINTED:
*     - Language standard (C / C++ version)
*     - Compiler identification and version
*     - Operating system and platform
*     - CPU architecture and bit width
*     - Build configuration (Debug / Release)
*     - C++ feature availability (optional, controlled by macro)
*
*   USAGE:
*     djinterp::print_env(std::cout);              // to console
*     djinterp::print_env(my_file_ptr);            // to FILE*
*     std::string s; djinterp::print_env(s);       // to string
*     char buf[4096];                              // to buffer
*     auto bs = djinterp::make_buffer_state(buf, sizeof(buf));
*     djinterp::print_env(bs);
*
*   CONFIGURATION:
*     D_ENV_PRINTER_INCLUDE_FEATURES  : if 1, print individual C++ feature
*                                       flags (default: 0, as the list is
*                                       very long)
*
* path:      /inc/cpp/io/env_printer.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.03.22
******************************************************************************/

#ifndef DJINTERP_ENV_PRINTER_
#define DJINTERP_ENV_PRINTER_ 1

#include "../djinterp.hpp"
#include "../text/print.hpp"
#include "../text/printer_traits.hpp"


// D_ENV_PRINTER_INCLUDE_FEATURES
//   configuration: when set to 1, print_env will include individual
// C++ feature test macro results. Disabled by default due to the
// volume of output.
#ifndef D_ENV_PRINTER_INCLUDE_FEATURES
    #define D_ENV_PRINTER_INCLUDE_FEATURES 0
#endif


NS_DJINTERP


// =============================================================================
// I.   LANGUAGE SECTION
// =============================================================================

// print_env_language
//   function: prints detected language standard information to the
// given target.
template<typename _Target>
inline std::size_t
print_env_language(_Target& _target, std::size_t _indent = 0)
{
    std::size_t written;

    written = write_section_header(_target, "Language", _indent);

#ifdef D_ENV_LANG_CPP_STANDARD_NAME
    written += write_kv(_target,
                        "C++ Standard",
                        D_ENV_LANG_CPP_STANDARD_NAME,
                        _indent + 1);
#endif

#ifdef D_ENV_LANG_C_STANDARD_NAME
    written += write_kv(_target,
                        "C Standard",
                        D_ENV_LANG_C_STANDARD_NAME,
                        _indent + 1);
#endif

#if D_ENV_LANG_USING_CPP
    written += write_kv(_target,
                        "Using C++",
                        true,
                        _indent + 1);
#else
    written += write_kv(_target,
                        "Using C++",
                        false,
                        _indent + 1);
#endif

    return written;
}


// =============================================================================
// II.  COMPILER SECTION
// =============================================================================

// print_env_compiler
//   function: prints detected compiler identification and version
// information to the given target.
template<typename _Target>
inline std::size_t
print_env_compiler(_Target& _target, std::size_t _indent = 0)
{
    std::size_t written;

    written = write_section_header(_target, "Compiler", _indent);

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
#endif

#ifdef D_ENV_COMPILER_VERSION_STRING
    written += write_kv(_target,
                        "Version",
                        D_ENV_COMPILER_VERSION_STRING,
                        _indent + 1);
#endif

#ifdef D_ENV_COMPILER_VERSION_MAJOR
    written += write_kv(_target,
                        "Major",
                        static_cast<long long>(D_ENV_COMPILER_VERSION_MAJOR),
                        _indent + 1);
#endif

#ifdef D_ENV_COMPILER_VERSION_MINOR
    written += write_kv(_target,
                        "Minor",
                        static_cast<long long>(D_ENV_COMPILER_VERSION_MINOR),
                        _indent + 1);
#endif

#ifdef D_ENV_COMPILER_VERSION_PATCH
    written += write_kv(_target,
                        "Patch",
                        static_cast<long long>(D_ENV_COMPILER_VERSION_PATCH),
                        _indent + 1);
#endif

    return written;
}


// =============================================================================
// III. OPERATING SYSTEM SECTION
// =============================================================================

// print_env_os
//   function: prints detected operating system and platform
// information to the given target.
template<typename _Target>
inline std::size_t
print_env_os(_Target& _target, std::size_t _indent = 0)
{
    std::size_t written;

    written = write_section_header(_target, "Operating System", _indent);

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
#endif

#ifdef D_ENV_PLATFORM_BITS
    written += write_kv(_target,
                        "Platform Bits",
                        static_cast<long long>(D_ENV_PLATFORM_BITS),
                        _indent + 1);
#endif

#ifdef D_ENV_ENDIANNESS_NAME
    written += write_kv(_target,
                        "Endianness",
                        D_ENV_ENDIANNESS_NAME,
                        _indent + 1);
#endif

    return written;
}


// =============================================================================
// IV.  ARCHITECTURE SECTION
// =============================================================================

// print_env_arch
//   function: prints detected CPU architecture information to the
// given target.
template<typename _Target>
inline std::size_t
print_env_arch(_Target& _target, std::size_t _indent = 0)
{
    std::size_t written;

    written = write_section_header(_target, "Architecture", _indent);

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
#endif

    return written;
}


// =============================================================================
// V.   BUILD CONFIGURATION SECTION
// =============================================================================

// print_env_build
//   function: prints detected build configuration (Debug/Release)
// to the given target.
template<typename _Target>
inline std::size_t
print_env_build(_Target& _target, std::size_t _indent = 0)
{
    std::size_t written;

    written = write_section_header(_target, "Build", _indent);

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
#endif

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
#endif

    return written;
}


// =============================================================================
// VI.  C++ FEATURE SECTION (OPTIONAL)
// =============================================================================

#if D_ENV_PRINTER_INCLUDE_FEATURES

// print_env_cpp_feature_line
//   function: prints a single feature detection result as a
// key-value line with enabled/disabled status.
template<typename _Target>
inline std::size_t
print_env_cpp_feature_line(_Target&    _target,
                           const char* _name,
                           const char* _desc,
                           int         _enabled,
                           const char* _version,
                           std::size_t _indent)
{
    std::size_t written;

    written  = write_indent(_target, _indent);
    written += write_to(_target, _enabled ? "[+] " : "[-] ");
    written += write_to(_target, _desc);
    written += write_to(_target, " ");
    written += write_to(_target, _version);
    written += write_to(_target, " (");
    written += write_to(_target, _name);
    written += write_to(_target, ")");
    written += write_newline(_target);

    return written;
}


// print_env_cpp_features_lang_cpp11
//   function: prints C++11 language feature detection results.
template<typename _Target>
inline std::size_t
print_env_cpp_features_lang_cpp11(_Target&    _target,
                                  std::size_t _indent = 0)
{
    std::size_t written;

    written = write_line_to(_target, "C++11 Language Features:");

    written += print_env_cpp_feature_line(
        _target,
        D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_NAME,
        D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_DESC,
        D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES,
        D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES_VERS,
        _indent + 1);

    written += print_env_cpp_feature_line(
        _target,
        D_ENV_CPP_FEATURE_LANG_CONSTEXPR_NAME,
        D_ENV_CPP_FEATURE_LANG_CONSTEXPR_DESC,
        D_ENV_CPP_FEATURE_LANG_CONSTEXPR,
        D_ENV_CPP_FEATURE_LANG_CONSTEXPR_VERS,
        _indent + 1);

    written += print_env_cpp_feature_line(
        _target,
        D_ENV_CPP_FEATURE_LANG_DECLTYPE_NAME,
        D_ENV_CPP_FEATURE_LANG_DECLTYPE_DESC,
        D_ENV_CPP_FEATURE_LANG_DECLTYPE,
        D_ENV_CPP_FEATURE_LANG_DECLTYPE_VERS,
        _indent + 1);

    written += print_env_cpp_feature_line(
        _target,
        D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_NAME,
        D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_DESC,
        D_ENV_CPP_FEATURE_LANG_ATTRIBUTES,
        D_ENV_CPP_FEATURE_LANG_ATTRIBUTES_VERS,
        _indent + 1);

    return written;
}


// print_env_cpp_features_aggregate
//   function: prints aggregate C++ feature availability flags.
template<typename _Target>
inline std::size_t
print_env_cpp_features_aggregate(_Target&    _target,
                                 std::size_t _indent = 0)
{
    std::size_t written;

    written = 0;

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    written += write_section_header(_target,
                                    "C++ Feature Aggregates",
                                    _indent);

    written += write_kv(_target,
                        "All C++14 Lang Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP14),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++17 Lang Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP17),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++20 Lang Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP20),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++23 Lang Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_LANG_CPP23),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++14 STL Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP14),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++17 STL Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP17),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++20 STL Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP20),
                        _indent + 1);

    written += write_kv(_target,
                        "All C++23 STL Features",
                        static_cast<bool>(
                            D_ENV_CPP_FEATURE_HAS_ALL_STL_CPP23),
                        _indent + 1);
#endif

    return written;
}

#endif  // D_ENV_PRINTER_INCLUDE_FEATURES


// =============================================================================
// VII. MASTER PRINT FUNCTION
// =============================================================================

// print_env
//   function: prints all detected environment information to the
// given target. Calls each section printer in order.
template<typename _Target>
inline std::size_t
print_env(_Target& _target, std::size_t _indent = 0)
{
    std::size_t written;

    written  = write_section_header(_target,
                                     "djinterp Environment",
                                     _indent);
    written += write_newline(_target);

    written += print_env_language(_target, _indent);
    written += write_newline(_target);

    written += print_env_compiler(_target, _indent);
    written += write_newline(_target);

    written += print_env_os(_target, _indent);
    written += write_newline(_target);

    written += print_env_arch(_target, _indent);
    written += write_newline(_target);

    written += print_env_build(_target, _indent);

#if D_ENV_PRINTER_INCLUDE_FEATURES
    written += write_newline(_target);
    written += print_env_cpp_features_aggregate(_target, _indent);
#endif

    return written;
}


// =============================================================================
// VIII. FILE* OVERLOADS
// =============================================================================
// Explicit FILE* overloads to avoid template-vs-pointer ambiguity.

// print_env_language
//   function: FILE* overload for language section printing.
inline std::size_t
print_env_language(std::FILE* _target, std::size_t _indent = 0)
{
    std::size_t written;

    written = write_section_header(_target, "Language", _indent);

#ifdef D_ENV_LANG_CPP_STANDARD_NAME
    written += write_kv(_target,
                        "C++ Standard",
                        D_ENV_LANG_CPP_STANDARD_NAME,
                        _indent + 1);
#endif

#ifdef D_ENV_LANG_C_STANDARD_NAME
    written += write_kv(_target,
                        "C Standard",
                        D_ENV_LANG_C_STANDARD_NAME,
                        _indent + 1);
#endif

    return written;
}

// print_env_compiler
//   function: FILE* overload for compiler section printing.
inline std::size_t
print_env_compiler(std::FILE* _target, std::size_t _indent = 0)
{
    std::size_t written;

    written = write_section_header(_target, "Compiler", _indent);

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
#endif

#ifdef D_ENV_COMPILER_VERSION_STRING
    written += write_kv(_target,
                        "Version",
                        D_ENV_COMPILER_VERSION_STRING,
                        _indent + 1);
#endif

    return written;
}

// print_env
//   function: FILE* overload for the master environment printer.
inline std::size_t
print_env(std::FILE* _target, std::size_t _indent = 0)
{
    std::size_t written;

    written  = write_section_header(_target,
                                     "djinterp Environment",
                                     _indent);
    written += write_newline(_target);

    written += print_env_language(_target, _indent);
    written += write_newline(_target);

    written += print_env_compiler(_target, _indent);

    return written;
}


NS_END  // djinterp


#endif  // DJINTERP_ENV_PRINTER_
