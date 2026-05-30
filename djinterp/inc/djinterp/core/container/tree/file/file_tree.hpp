/******************************************************************************
* djinterp [fs]                                                   file_tree.hpp
*
* File tree umbrella:
*   The single public entry point.  Including this header pulls in the
* OS-independent core (file_tree_common.hpp) and every scanner backend
* that is valid to compile on the current host, then exposes a single
* selectable type:
*
*       file_tree<operating_system OS = operating_system::automatic>
*
*   The OS template parameter chooses the scan backend.  Passing no
* argument selects operating_system::automatic, which resolves at
* compile time to whichever backend matches the detected D_ENV_OS_ID.
*
* Usage:
*
*   #include "file_tree.hpp"
*
*   using namespace djinterp::fs;
*
*   // explicit backend
*   file_tree<operating_system::windows10> tree;
*   tree.scan("C:\\Users\\me\\project");
*
*   // detected backend (no argument -> automatic)
*   file_tree<> ft;
*   ft.scan("/home/me/project");
*
*   node_id n = ft.resolve("src/core/main.cpp");
*   std::string p = ft.full_path(n);
*
* Backend selection (os_scanner):
*   Every operating_system value names a policy.  On a host where that
* policy's real implementation cannot compile (e.g. the Win32 backend
* on Linux), the umbrella still lets you *name* the type - the backend
* degrades to a stub or null_scanner so file_tree<that_os> always
* instantiates.  Only the backend matching the build target performs
* real OS calls.  This keeps the selector total: any enum value yields
* a usable type on any platform.
*
*
* path:      /inc/cpp/fs/file_tree.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_
#define DJINTERP_FS_FILE_TREE_ 1

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "file_tree.hpp can only be used in C++ compilation mode"
#endif

#include "./file_tree_common.hpp"


// ----------------------------------------------------------------
//  backend inclusion
// ----------------------------------------------------------------
// Each platform family pulls in its own real backend plus the
// portable POSIX baseline.  Headers not included here still have
// their *selector* mapped to null_scanner below, so naming any
// operating_system value compiles everywhere; only the headers that
// can compile on this host are brought in.

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #include "./file_tree_windows.hpp"
    #define D_FS_BACKEND_WINDOWS 1
#elif defined(__APPLE__)
    #include "./file_tree_apple.hpp"
    #include "./file_tree_ios.hpp"
    #include "./file_tree_posix.hpp"
    #include "./file_tree_bsd.hpp"
    #define D_FS_BACKEND_APPLE 1
#elif defined(__linux__)
    #include "./file_tree_linux.hpp"
    #include "./file_tree_bsd.hpp"
    #include "./file_tree_posix.hpp"
    #define D_FS_BACKEND_LINUX 1
#elif D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)   // BSD family
    #include "./file_tree_bsd.hpp"
    #include "./file_tree_posix.hpp"
    #define D_FS_BACKEND_BSD 1
#elif D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
    #include "./file_tree_posix.hpp"
    #define D_FS_BACKEND_POSIX 1
#endif


NS_DJINTERP
NS_FS


// ================================================================
//  os_scanner  (operating_system -> scanner policy)
// ================================================================

// os_scanner
//   trait: maps an operating_system selector onto a scanner policy
// type via the nested `type` alias.  The primary template maps to
// null_scanner; specializations below bind each selector to a real
// backend *when that backend is compiled in*, and otherwise leave
// the null_scanner mapping in place.
template<operating_system _OS>
struct os_scanner
{
    using type = null_scanner;
};


// --- detected-backend resolution for `automatic` -----------------
//
// `automatic` resolves to the concrete selector matching the build
// target.  We define a constant that names that selector, then map
// `automatic` onto the same policy as the target selector.

#if   defined(D_FS_BACKEND_WINDOWS)
    #if (D_ENV_OS_ID == D_ENV_OS_FLAG_WIN_PC_10) || \
        (D_ENV_OS_ID == D_ENV_OS_FLAG_WIN_PC_11)
        D_STATIC_CONSTEXPR operating_system detected_os =
            operating_system::windows10;
    #else
        D_STATIC_CONSTEXPR operating_system detected_os =
            operating_system::windows;
    #endif
#elif defined(D_FS_BACKEND_APPLE)
    #if defined(D_ENV_OS_ID) && (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS)
        D_STATIC_CONSTEXPR operating_system detected_os =
            operating_system::ios;
    #else
        D_STATIC_CONSTEXPR operating_system detected_os =
            operating_system::apple;
    #endif
#elif defined(D_FS_BACKEND_LINUX)
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::linux_generic;
#elif defined(D_FS_BACKEND_BSD)
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::bsd;
#elif defined(D_FS_BACKEND_POSIX)
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::posix;
#else
    D_STATIC_CONSTEXPR operating_system detected_os =
        operating_system::none;
#endif


// --- per-selector specializations --------------------------------
// Only define a mapping to a real policy when its header was
// compiled in; otherwise the primary template (null_scanner) stands.

#if defined(D_FS_BACKEND_WINDOWS)
    template<> struct os_scanner<operating_system::windows>
    { using type = windows_scanner; };

    template<> struct os_scanner<operating_system::windows10>
    { using type = windows10_scanner; };

    template<> struct os_scanner<operating_system::windows11>
    { using type = windows10_scanner; };
#endif

#if defined(D_FS_BACKEND_APPLE)
    template<> struct os_scanner<operating_system::apple>
    { using type = apple_scanner; };

    template<> struct os_scanner<operating_system::ios>
    { using type = ios_scanner; };

    template<> struct os_scanner<operating_system::bsd>
    { using type = bsd_scanner; };

    template<> struct os_scanner<operating_system::posix>
    { using type = posix_scanner; };
#endif

#if defined(D_FS_BACKEND_LINUX)
    template<> struct os_scanner<operating_system::linux_generic>
    { using type = linux_scanner; };

    template<> struct os_scanner<operating_system::bsd>
    { using type = bsd_scanner; };

    template<> struct os_scanner<operating_system::posix>
    { using type = posix_scanner; };
#endif

#if defined(D_FS_BACKEND_BSD)
    template<> struct os_scanner<operating_system::bsd>
    { using type = bsd_scanner; };

    template<> struct os_scanner<operating_system::posix>
    { using type = posix_scanner; };
#endif

#if defined(D_FS_BACKEND_POSIX)
    template<> struct os_scanner<operating_system::posix>
    { using type = posix_scanner; };
#endif


NS_INTERNAL

    // automatic_policy
    //   resolves operating_system::automatic onto the detected
    // backend's policy.  Kept in internal:: so the file_tree alias
    // below stays a one-liner.
    template<operating_system _OS>
    struct resolve_os
    {
        using type = typename os_scanner<_OS>::type;
    };

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
//   alias: the public, OS-parameterized file tree.  Selects the
// scanner backend named by _OS (defaulting to the detected backend),
// and inherits the full OS-independent surface from file_tree_core:
// scan / resolve / name / name_str / full_path / visit_* / add_child
// / clear / operator[] / size / empty / nodes.
template<operating_system _OS = operating_system::automatic>
using file_tree =
    file_tree_core<typename internal::resolve_os<_OS>::type>;


// file_tree_default
//   type: the detected-backend file tree, for code that wants a
// plain non-template name.
using file_tree_default = file_tree<operating_system::automatic>;


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_
