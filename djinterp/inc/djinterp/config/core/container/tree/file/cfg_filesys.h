/******************************************************************************
* djinterp [fs]                                                  cfg_filesys.h
*
* Filesystem backend configuration:
*   This header is the single control surface for which file_tree scan
* backends are compiled into a translation unit.  Its guiding rule is
* fail-safe: a backend for a foreign operating system is HIDDEN by
* default, so a user cannot accidentally compile code that can never run
* on the current host.  Naming file_tree<operating_system::windows10> on
* a Linux build is a compile error unless the user has explicitly opted
* into the Windows backend here (or via -D on the command line).
*
*   The native backend - the one matching the detected D_ENV_OS_ID - is
* ALWAYS enabled.  You can always build for the platform you are on.
*
*   Opt-in is hierarchical.  A backend is enabled when ANY of the
* following is true:
*       1. it is the native backend, or
*       2. the master flag D_CFG_FILESYS_ALLOW_FOREIGN is set, or
*       3. its OS-group flag is set, or
*       4. its own per-OS flag is set.
*
*   CONFIGURATION FLAGS (define to 1 before including file_tree.hpp, or
*   pass -D on the compiler command line):
*
*     master
*       D_CFG_FILESYS_ALLOW_FOREIGN          unlock every foreign backend
*
*     OS groups
*       D_CFG_FILESYS_ALLOW_POSIX_FAMILY     posix + linux + bsd + apple + ios
*       D_CFG_FILESYS_ALLOW_APPLE_FAMILY     apple + ios
*       D_CFG_FILESYS_ALLOW_WINDOWS_FAMILY   windows (all versions)
*
*     per-OS
*       D_CFG_FILESYS_ALLOW_POSIX            portable POSIX baseline
*       D_CFG_FILESYS_ALLOW_LINUX            Linux (statx / d_type)
*       D_CFG_FILESYS_ALLOW_BSD              BSD family (d_type)
*       D_CFG_FILESYS_ALLOW_APPLE            macOS (getattrlistbulk)
*       D_CFG_FILESYS_ALLOW_IOS             iOS (sandbox-aware Darwin)
*       D_CFG_FILESYS_ALLOW_WINDOWS          Win32 (FindFirstFileExW)
*
*   This header emits a set of derived, read-only macros consumed by the
*   per-OS headers and the umbrella:
*
*       D_FILESYS_ENABLE_POSIX
*       D_FILESYS_ENABLE_LINUX
*       D_FILESYS_ENABLE_BSD
*       D_FILESYS_ENABLE_APPLE
*       D_FILESYS_ENABLE_IOS
*       D_FILESYS_ENABLE_WINDOWS
*
*   Each is 1 if that backend is permitted in this build, else 0.
*   Do not define the D_FILESYS_ENABLE_* macros yourself; set the
*   D_CFG_FILESYS_* inputs and let this header derive them.
*
*
* path:      /inc/cpp/fs/cfg_filesys.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_CFG_FILESYS_
#define DJINTERP_FS_CFG_FILESYS_ 1

// env.h must be visible: this header keys off D_ENV_OS_ID and the
// D_ENV_IS_OS_* / D_ENV_OS_FLAG_* surface for native detection.
#ifndef DJINTERP_ENVIRONMENT_
    #error "cfg_filesys.h requires env.h (D_ENV_OS_ID) to be included first"
#endif


// ============================================================================
// I.   USER-FACING CONFIGURATION INPUTS  (all default OFF)
// ============================================================================
// These are the ONLY macros a user should set.  Each defaults to 0
// (disabled).  Setting any to 1 widens what is allowed to compile; none
// of them can ever DISABLE the native backend.

// --- master ----------------------------------------------------------------

// D_CFG_FILESYS_ALLOW_FOREIGN
//   master switch: when 1, every foreign backend is permitted.
#ifndef D_CFG_FILESYS_ALLOW_FOREIGN
    #define D_CFG_FILESYS_ALLOW_FOREIGN 0
#endif

// --- OS groups --------------------------------------------------------------

// D_CFG_FILESYS_ALLOW_POSIX_FAMILY
//   group: permits posix, linux, bsd, apple, and ios backends.
#ifndef D_CFG_FILESYS_ALLOW_POSIX_FAMILY
    #define D_CFG_FILESYS_ALLOW_POSIX_FAMILY 0
#endif

// D_CFG_FILESYS_ALLOW_APPLE_FAMILY
//   group: permits apple and ios backends.
#ifndef D_CFG_FILESYS_ALLOW_APPLE_FAMILY
    #define D_CFG_FILESYS_ALLOW_APPLE_FAMILY 0
#endif

// D_CFG_FILESYS_ALLOW_WINDOWS_FAMILY
//   group: permits all Windows backends.
#ifndef D_CFG_FILESYS_ALLOW_WINDOWS_FAMILY
    #define D_CFG_FILESYS_ALLOW_WINDOWS_FAMILY 0
#endif

// --- per-OS -----------------------------------------------------------------

// D_CFG_FILESYS_ALLOW_POSIX
//   permits the portable POSIX baseline backend.
#ifndef D_CFG_FILESYS_ALLOW_POSIX
    #define D_CFG_FILESYS_ALLOW_POSIX 0
#endif

// D_CFG_FILESYS_ALLOW_LINUX
//   permits the Linux backend.
#ifndef D_CFG_FILESYS_ALLOW_LINUX
    #define D_CFG_FILESYS_ALLOW_LINUX 0
#endif

// D_CFG_FILESYS_ALLOW_BSD
//   permits the BSD-family backend.
#ifndef D_CFG_FILESYS_ALLOW_BSD
    #define D_CFG_FILESYS_ALLOW_BSD 0
#endif

// D_CFG_FILESYS_ALLOW_APPLE
//   permits the macOS backend.
#ifndef D_CFG_FILESYS_ALLOW_APPLE
    #define D_CFG_FILESYS_ALLOW_APPLE 0
#endif

// D_CFG_FILESYS_ALLOW_IOS
//   permits the iOS backend.
#ifndef D_CFG_FILESYS_ALLOW_IOS
    #define D_CFG_FILESYS_ALLOW_IOS 0
#endif

// D_CFG_FILESYS_ALLOW_WINDOWS
//   permits the Win32 backend.
#ifndef D_CFG_FILESYS_ALLOW_WINDOWS
    #define D_CFG_FILESYS_ALLOW_WINDOWS 0
#endif


// ============================================================================
// II.  NATIVE BACKEND DETECTION  (always enabled)
// ============================================================================
// Exactly one family below is native to the detected D_ENV_OS_ID.  The
// native per-OS backend is forced on regardless of the D_CFG_* inputs.
//
// Detection keys ONLY off D_ENV_OS_ID (and its env.h companions), never
// off raw compiler predefines such as __linux__ / __APPLE__ / _WIN32.
// This keeps native selection consistent with env.h's centralized,
// simulatable model: overriding D_ENV_OS_ID (e.g. via D_CFG_ENV_CUSTOM)
// re-targets the filesystem layer in lockstep with the rest of djinterp.

#if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
    #define D_FILESYS_NATIVE_WINDOWS 1
#else
    #define D_FILESYS_NATIVE_WINDOWS 0
#endif

#if (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS)
    #define D_FILESYS_NATIVE_IOS 1
#else
    #define D_FILESYS_NATIVE_IOS 0
#endif

// Apple desktop = macOS (and the generic Apple block), excluding iOS.
#if !D_FILESYS_NATIVE_IOS && \
    ( (D_ENV_OS_ID == D_ENV_OS_FLAG_APPLE) || \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) )
    #define D_FILESYS_NATIVE_APPLE 1
#else
    #define D_FILESYS_NATIVE_APPLE 0
#endif

#if (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX)
    #define D_FILESYS_NATIVE_LINUX 1
#else
    #define D_FILESYS_NATIVE_LINUX 0
#endif

#if !D_FILESYS_NATIVE_WINDOWS && !D_FILESYS_NATIVE_APPLE && \
    !D_FILESYS_NATIVE_IOS     && !D_FILESYS_NATIVE_LINUX && \
    D_ENV_IS_OS_FLAG_IN_BLOCK(D_ENV_OS_ID, 0x4)
    #define D_FILESYS_NATIVE_BSD 1
#else
    #define D_FILESYS_NATIVE_BSD 0
#endif

// POSIX baseline is native on any POSIX-like host that is not already
// served by a more specific native family above.
#if !D_FILESYS_NATIVE_WINDOWS && !D_FILESYS_NATIVE_APPLE && \
    !D_FILESYS_NATIVE_IOS     && !D_FILESYS_NATIVE_LINUX && \
    !D_FILESYS_NATIVE_BSD     && D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
    #define D_FILESYS_NATIVE_POSIX 1
#else
    #define D_FILESYS_NATIVE_POSIX 0
#endif


// ============================================================================
// III. DERIVED ENABLE MACROS  (read-only; consumed downstream)
// ============================================================================
// A backend is enabled iff it is native OR explicitly opted into by the
// master flag, its group flag, or its own flag.
//
// Group membership:
//   posix family  = posix, linux, bsd, apple, ios
//   apple family  = apple, ios
//   windows family= windows
//
// The POSIX baseline is treated as a member of the POSIX family, so any
// posix-family opt-in also permits the portable baseline (handy for
// cross-builds that just want *a* working unix walk).
//
// DEPENDENCY PROPAGATION:
//   The unix backends are layered - each includes the one below it for
//   shared helpers (classify, is_dot_entry, d_type mapping, fallback):
//       ios   -> apple -> bsd -> posix
//       linux ->          bsd -> posix
//   So enabling a higher backend must also enable everything it includes,
//   or that dependency's header would #error.  The "_REQ" intermediate
//   captures the direct opt-in; the final macro ORs in its dependents.

// --- direct opt-in (native or explicitly allowed) --------------------------

#define D_FILESYS_REQ_POSIX                             \
    ( D_FILESYS_NATIVE_POSIX                         || \
      D_CFG_FILESYS_ALLOW_FOREIGN                    || \
      D_CFG_FILESYS_ALLOW_POSIX_FAMILY               || \
      D_CFG_FILESYS_ALLOW_POSIX )

#define D_FILESYS_REQ_BSD                               \
    ( D_FILESYS_NATIVE_BSD                           || \
      D_CFG_FILESYS_ALLOW_FOREIGN                    || \
      D_CFG_FILESYS_ALLOW_POSIX_FAMILY               || \
      D_CFG_FILESYS_ALLOW_BSD )

#define D_FILESYS_REQ_LINUX                             \
    ( D_FILESYS_NATIVE_LINUX                         || \
      D_CFG_FILESYS_ALLOW_FOREIGN                    || \
      D_CFG_FILESYS_ALLOW_POSIX_FAMILY               || \
      D_CFG_FILESYS_ALLOW_LINUX )

#define D_FILESYS_REQ_APPLE                             \
    ( D_FILESYS_NATIVE_APPLE                         || \
      D_CFG_FILESYS_ALLOW_FOREIGN                    || \
      D_CFG_FILESYS_ALLOW_POSIX_FAMILY               || \
      D_CFG_FILESYS_ALLOW_APPLE_FAMILY               || \
      D_CFG_FILESYS_ALLOW_APPLE )

#define D_FILESYS_REQ_IOS                               \
    ( D_FILESYS_NATIVE_IOS                           || \
      D_CFG_FILESYS_ALLOW_FOREIGN                    || \
      D_CFG_FILESYS_ALLOW_POSIX_FAMILY               || \
      D_CFG_FILESYS_ALLOW_APPLE_FAMILY               || \
      D_CFG_FILESYS_ALLOW_IOS )

// --- final enable macros (direct opt-in + propagated dependents) -----------
// ios depends on apple; apple and linux depend on bsd; bsd depends on posix.

// D_FILESYS_ENABLE_IOS
#define D_FILESYS_ENABLE_IOS    ( D_FILESYS_REQ_IOS )

// D_FILESYS_ENABLE_APPLE  (+ ios)
#define D_FILESYS_ENABLE_APPLE  ( D_FILESYS_REQ_APPLE || D_FILESYS_ENABLE_IOS )

// D_FILESYS_ENABLE_LINUX
#define D_FILESYS_ENABLE_LINUX  ( D_FILESYS_REQ_LINUX )

// D_FILESYS_ENABLE_BSD    (+ linux, + apple)
#define D_FILESYS_ENABLE_BSD                            \
    ( D_FILESYS_REQ_BSD   ||                            \
      D_FILESYS_ENABLE_LINUX ||                         \
      D_FILESYS_ENABLE_APPLE )

// D_FILESYS_ENABLE_POSIX  (+ bsd, and thus everything above it)
#define D_FILESYS_ENABLE_POSIX                          \
    ( D_FILESYS_REQ_POSIX ||                            \
      D_FILESYS_ENABLE_BSD )

// D_FILESYS_ENABLE_WINDOWS  (standalone; no unix dependency)
#define D_FILESYS_ENABLE_WINDOWS                        \
    ( D_FILESYS_NATIVE_WINDOWS                       || \
      D_CFG_FILESYS_ALLOW_FOREIGN                    || \
      D_CFG_FILESYS_ALLOW_WINDOWS_FAMILY             || \
      D_CFG_FILESYS_ALLOW_WINDOWS )


// ============================================================================
// IV.  CONVENIENCE: anything-foreign-enabled flag
// ============================================================================
// True when at least one NON-native backend has been opted into.  Used
// only for diagnostics / static_assert messaging downstream.

#define D_FILESYS_ANY_FOREIGN_ENABLED                       \
    ( ( D_FILESYS_ENABLE_POSIX   && !D_FILESYS_NATIVE_POSIX   ) || \
      ( D_FILESYS_ENABLE_LINUX   && !D_FILESYS_NATIVE_LINUX   ) || \
      ( D_FILESYS_ENABLE_BSD     && !D_FILESYS_NATIVE_BSD     ) || \
      ( D_FILESYS_ENABLE_APPLE   && !D_FILESYS_NATIVE_APPLE   ) || \
      ( D_FILESYS_ENABLE_IOS     && !D_FILESYS_NATIVE_IOS     ) || \
      ( D_FILESYS_ENABLE_WINDOWS && !D_FILESYS_NATIVE_WINDOWS ) )


#endif  // DJINTERP_FS_CFG_FILESYS_
