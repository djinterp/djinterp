/*******************************************************************************
* djinterp [core]                                                  file_tree.hpp
*
* File tree umbrella:
*   The single public entry point.  Including this header pulls in the
* OS-independent core (file_tree_common.hpp), the backend configuration
* (cfg_filesys.h), and every scanner backend that has been ENABLED for
* this build, then exposes one selectable type:
*
*       file_tree<operating_system OS = operating_system::automatic>
*
*   The OS template parameter chooses the scan backend.  Passing no
* argument selects operating_system::automatic, which resolves at
* compile time to the backend matching the detected D_ENV_OS_ID.  A trailing
* Options... pack carries the framework's per-axis options
* (container_options.hpp) through to file_tree_core; it is trailing, so every
* existing instantiation keeps compiling untouched.
*
* Usage (native build):
*
*   #include "file_tree.hpp"
*   using namespace djinterp;
*
*   file_tree<> ft;                       // automatic -> detected backend
*   ft.scan("/home/me/project");
*   file_node_id n = ft.resolve("src/core/main.cpp");
*   std::string p = ft.full_path(n);
*
* Fail-safe backend selection:
*   A foreign backend is HIDDEN by default.  Naming a backend that has
* not been enabled is a COMPILE ERROR, not a silent no-op - you cannot
* accidentally build code that can never run on the current host:
*
*   // on a Linux build, with no config flags set:
*   file_tree<operating_system::windows10> t;   // ERROR: backend disabled
*
*   To deliberately allow a foreign backend (e.g. for a cross-compile or
* a host abstraction layer), opt in BEFORE including this header - either
* with a #define or a -D on the command line.  See cfg_filesys.h:
*
*   #define D_CFG_FILESYS_ALLOW_WINDOWS 1
*   #include "file_tree.hpp"
*   file_tree<operating_system::windows10> t;   // now permitted
*
*   The native backend (matching D_ENV_OS_ID) is always enabled; you can
* always build for the platform you are on.
*
*
* path:      /inc/djinterp/core/container/tree/file/file_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.22
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_HPP
#define DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "file_tree.hpp can only be used in C++ compilation mode"
#endif

// djinterp
#include "./file_tree_common.hpp"   // also pulls in cfg_filesys.h


// ----------------------------------------------------------------
//  backend inclusion
// ----------------------------------------------------------------
// Include each backend header that cfg_filesys.h has enabled for this
// build.  The native backend is always enabled; foreign backends only
// when the user opted in.  A disabled backend's header is NOT included,
// so its selector has no os_scanner specialization and naming it is a
// compile error (see the primary os_scanner template below).
//
// Inclusion order respects the layering (posix < bsd < {linux, apple} <
// ios); each header also includes what it needs, and the include guards
// make the repetition harmless.

#if D_FILESYS_ENABLE_POSIX
    #include "./file_tree_posix.hpp"
#endif

#if D_FILESYS_ENABLE_BSD
    #include "./file_tree_bsd.hpp"
#endif

#if D_FILESYS_ENABLE_LINUX
    #include "./file_tree_linux.hpp"
#endif

#if D_FILESYS_ENABLE_APPLE
    #include "./file_tree_apple.hpp"
#endif

#if D_FILESYS_ENABLE_IOS
    #include "./file_tree_ios.hpp"
#endif

#if D_FILESYS_ENABLE_WINDOWS
    #include "./file_tree_windows.hpp"
#endif


NS_DJINTERP


// ================================================================
//  os_scanner  (operating_system -> scanner policy)
// ================================================================

// disabled_backend_selected
//   trait: dependent-false helper so the primary os_scanner template
// can static_assert with a readable message only when actually instantiated
// for a disabled selector.
template<operating_system OS>
struct disabled_backend_selected
{
    static constexpr bool value = false;
};

// os_scanner (primary)
//   Reached only for selectors whose backend was NOT enabled for this build.
// Instantiating it is a hard error carrying remediation guidance. Enabled
// selectors hit one of the specializations below instead.
template<operating_system OS>
struct os_scanner
{
    static_assert(
        disabled_backend_selected<OS>::value,
        "file_tree: the requested operating_system backend is not enabled "
        "for this build. It is foreign to the current target and hidden by "
        "default. Opt in before including file_tree.hpp by defining the "
        "matching D_CFG_FILESYS_ALLOW_* flag (per-OS, OS-group, or the "
        "master D_CFG_FILESYS_ALLOW_FOREIGN). See cfg_filesys.h.");

    // Provide a type so that, past the static_assert, dependent code
    // still names something and the diagnostic stays focused on the assertion
    // rather than a cascade of 'no member type' errors.
    using type = null_scanner;
};


// --- detected-backend selector for `automatic` ------------------- Resolve
// the detected target onto its concrete selector. The native
// backend is always enabled, so the corresponding specialization below always
// exists.

#if   D_FILESYS_NATIVE_WINDOWS
    #if (D_ENV_OS_ID == D_ENV_OS_FLAG_WIN_PC_10) || \
        (D_ENV_OS_ID == D_ENV_OS_FLAG_WIN_PC_11)
        D_STATIC_CONSTEXPR operating_system detected_os =
            operating_system::windows10;
    #else
        D_STATIC_CONSTEXPR operating_system detected_os =
            operating_system::windows;
    #endif
#elif D_FILESYS_NATIVE_IOS
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::ios;
#elif D_FILESYS_NATIVE_APPLE
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::apple;
#elif D_FILESYS_NATIVE_LINUX
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::linux_generic;
#elif D_FILESYS_NATIVE_BSD
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::bsd;
#elif D_FILESYS_NATIVE_POSIX
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::posix;
#else
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::none;
#endif


// --- per-selector specializations (only for enabled backends) ---- Each is
// defined iff its backend header was included above. A
// disabled selector therefore has no specialization and falls through to the
// asserting primary template.

#if D_FILESYS_ENABLE_POSIX
    template<> struct os_scanner<operating_system::posix>
    { using type = posix_scanner; };
#endif

#if D_FILESYS_ENABLE_BSD
    template<> struct os_scanner<operating_system::bsd>
    { using type = bsd_scanner; };
#endif

#if D_FILESYS_ENABLE_LINUX
    template<> struct os_scanner<operating_system::linux_generic>
    { using type = linux_scanner; };
#endif

#if D_FILESYS_ENABLE_APPLE
    template<> struct os_scanner<operating_system::apple>
    { using type = apple_scanner; };
#endif

#if D_FILESYS_ENABLE_IOS
    template<> struct os_scanner<operating_system::ios>
    { using type = ios_scanner; };
#endif

#if D_FILESYS_ENABLE_WINDOWS
    template<> struct os_scanner<operating_system::windows>
    { using type = windows_scanner; };

    template<> struct os_scanner<operating_system::windows10>
    { using type = windows10_scanner; };

    template<> struct os_scanner<operating_system::windows11>
    { using type = windows10_scanner; };
#endif

// `none` is always available: an explicit, portable no-op tree.
template<> struct os_scanner<operating_system::none>
{ using type = null_scanner; };


NS_INTERNAL

    // resolve_os
    //   maps an operating_system selector onto its scanner policy, routing
    // `automatic` through detected_os.
    template<operating_system OS>
    struct resolve_os
    {
        using type = typename os_scanner<OS>::type;
    };

    // resolve_os<operating_system::automatic>
    //   trait: the `operating_system::automatic` case; it maps to `typename
    // os_scanner<detected_os>::type`.
    template<>
    struct resolve_os<operating_system::automatic>
    {
        using type = typename os_scanner<detected_os>::type;
    };

NS_END  // internal


// ================================================================
//  file_tree
// ================================================================

// file_tree
//   alias: the public, OS-parameterized file tree. Selects the scanner backend
// named by OS (defaulting to the detected backend) and inherits the full
// OS-independent surface from file_tree_core: scan / resolve / name / name_str
// / full_path / visit_* / add_child / clear / operator[] / size / empty /
// nodes.
//
//   Naming a disabled foreign backend is a compile error; see cfg_filesys.h to
// opt in. Pool selects the node store. It sits BEFORE Options because
// file_tree_core takes it there, and it must be pointer-stable: every node
// link and every file_node_id is a raw pointer into it. A djinterp pool
// (memory/pool.hpp) never moves a slot; any other pool is a compile error (see
// the guard in file_tree_common.hpp), not a crash later.
template<operating_system OS    = operating_system::automatic,
         typename         Pool = ::djinterp::pool<file_node>,
         typename...      Options>
using file_tree =
    file_tree_core<typename internal::resolve_os<OS>::type,
                   Pool,
                   Options...>;


// file_tree_default
//   type: the detected-backend file tree, for code that wants a plain
// non-template name. NOTE that `file_tree` is a template ALIAS -- naming it
// bare, as `const file_tree&`, is ill-formed. Use this, or file_tree<>, or
// template on the tree type.
using file_tree_default = file_tree<operating_system::automatic>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_FILE_FILE_TREE_HPP
