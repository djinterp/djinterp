/*******************************************************************************
* djinterp [config]                                            cfg_file_common.h
*
* Build-time configuration shared by every module of the djinterp fs
* subframework (c/fs/file_*.h).  It owns the knobs that are meaningful to
* more than one module -- backend selection, parameter validation,
* diagnostics, path limits, allocation hooks, descriptor hygiene -- and it
* owns the platform feature gates that used to live inside dfile.h.  It
* publishes the D_INTERNAL_FILE_* "effective" values that the modules read.
*
*   Per-module knobs (buffer sizes, module-local behaviour) live in that
* module's own cfg_file_<module>.h, which includes this file first.
*
*   THREE CONFIGURATION MODELS
*     gate        D_CFG_FILE_<KNOB> == 0 / 1.  A feature is on or off.
*                 Validated as a strict boolean.
*     selection   D_CFG_FILE_<KNOB> == one of a small set of D_CFG_FILE_*
*                 sentinels (e.g. the BACKEND family).  AUTO defers to
*                 env.h detection; anything else pins the choice.
*     value       D_CFG_FILE_<KNOB> == an integer or an identifier (sizes,
*                 allocator names, stream names).  NOT boolean, NOT
*                 validated as one.
*
*   Every knob is #ifndef-guarded, so the cascade holds:
*     -D on the command line  >  cfg_user.h  >  cfg_testing.h  >  this file.
*
*   targets:  c/fs/file_common.h and every c/fs/file_*.h -> D_INTERNAL_FILE_*
*   requires: cfg_common.h, env/env.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Master switches
         1.  D_CFG_FILE_ALL
         2.  D_CFG_FILE_OPTIMIZE
    2.  Backend selection
         1.  D_CFG_FILE_BACKEND_AUTO
         2.  D_CFG_FILE_BACKEND_STDC
         3.  D_CFG_FILE_BACKEND_POSIX
         4.  D_CFG_FILE_BACKEND_NATIVE
         5.  D_CFG_FILE_BACKEND
         6.  D_CFG_FILE_PREFER_ANNEX_K
    3.  Parameter validation and error reporting
         1.  D_CFG_FILE_VALIDATE_PARAMS
         2.  D_CFG_FILE_SET_ERRNO
         3.  D_CFG_FILE_ASSERT_PARAMS
    4.  Notifications / diagnostics
         1.  D_CFG_FILE_NOTIFY
         2.  D_CFG_FILE_NOTIFY_LEVEL
         3.  D_CFG_FILE_NOTIFY_DEFAULT_HANDLER
         4.  D_CFG_FILE_NOTIFY_STREAM
         5.  D_CFG_FILE_NOTIFY_PATHS
    5.  Path limits and encoding
         1.  D_CFG_FILE_PATH_MAX
         2.  D_CFG_FILE_NAME_MAX
         3.  D_CFG_FILE_LONG_PATHS
         4.  D_CFG_FILE_WIDE_PATHS
    6.  Allocation hooks
         1.  D_CFG_FILE_MALLOC
         2.  D_CFG_FILE_REALLOC
         3.  D_CFG_FILE_FREE
         4.  D_CFG_FILE_MAX_ALLOC
    7.  Descriptor hygiene and robustness
         1.  D_CFG_FILE_CLOEXEC_DEFAULT
         2.  D_CFG_FILE_EINTR_RETRY
         3.  D_CFG_FILE_THREADSAFE
    8.  Environment optimization
         1.  D_CFG_FILE_USE_RESTRICT
         2.  D_CFG_FILE_INLINE_PREDICATES
         3.  D_CFG_FILE_FADVISE
    9.  Platform feature gates
         1.  D_CFG_FILE_HAS_POSIX
         2.  D_CFG_FILE_HAS_WIN32
         3.  D_CFG_FILE_HAS_FOPEN_S
         4.  D_CFG_FILE_HAS_FSEEKO
         5.  D_CFG_FILE_HAS_MKSTEMP
         6.  D_CFG_FILE_HAS_REALPATH
         7.  D_CFG_FILE_HAS_SYMLINKS
         8.  D_CFG_FILE_HAS_PIPES
         9.  D_CFG_FILE_HAS_FLOCK
         10. D_CFG_FILE_HAS_PREAD
         11. D_CFG_FILE_HAS_COPY_FILE_RANGE
         12. D_CFG_FILE_HAS_FCOPYFILE
         13. D_CFG_FILE_HAS_STATX
         14. D_CFG_FILE_HAS_FADVISE
         15. D_CFG_FILE_HAS_FALLOCATE
         16. D_CFG_FILE_HAS_STAT_NSEC
         17. D_CFG_FILE_HAS_BIRTHTIME
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_BACKEND
         2.  D_INTERNAL_FILE_VALIDATE
         3.  D_INTERNAL_FILE_SET_ERRNO
         4.  D_INTERNAL_FILE_NOTIFY_LEVEL
         5.  D_INTERNAL_FILE_PATH_MAX
         6.  D_INTERNAL_FILE_NAME_MAX
         7.  D_INTERNAL_FILE_HAS_FOPEN_S
         8.  D_INTERNAL_FILE_HAS_FSEEKO
         9.  D_INTERNAL_FILE_HAS_MKSTEMP
         10. D_INTERNAL_FILE_HAS_REALPATH
         11. D_INTERNAL_FILE_HAS_SYMLINKS
         12. D_INTERNAL_FILE_HAS_PIPES
         13. D_INTERNAL_FILE_HAS_PREAD
         14. D_INTERNAL_FILE_HAS_FADVISE
         15. D_INTERNAL_FILE_USE_NATIVE_COPY
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_BACKEND_IS_NATIVE / _IS_POSIX / _IS_STDC
         2.  D_FILE_NOTIFY_IS_ACTIVE
         3.  D_FILE_VALIDATE_IS_ACTIVE
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_COMMON_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_COMMON_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_ON, D_CFG_NORM, D_CFG_TESTING
#include "../../../env/env.h"  // D_ENV_* platform detection


//==============================================================================
// 1.  KNOBS
//==============================================================================
//   Pre-define any knob before including a c/fs header to override it;
// otherwise it takes the default resolved here.


// 1.1    Master switches
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_ALL
//   knob: optional aggregate for the fs subframework's boolean gates. It is
// NOT defaulted here -- its absence is meaningful. Define it to 0 or 1 and
// every gate that has no explicit value of its own falls back to it, letting
// you turn the whole subframework's optional behaviour on or off in one
// place. Individual knobs still win over it.

// 1.1.2
// D_CFG_FILE_OPTIMIZE
//   knob: allow the fs modules to take platform fast paths (native copy
// engines, positional I/O, single-syscall metadata, kernel hints) when the
// environment offers them. 0 forces the portable path everywhere, which is
// what you want when bisecting a platform-specific bug or when byte-for-byte
// identical behaviour across targets matters more than throughput.
#ifndef D_CFG_FILE_OPTIMIZE
    #define D_CFG_FILE_OPTIMIZE 1
#endif  // D_CFG_FILE_OPTIMIZE

// 1.2    Backend selection
//------------------------------------------------------------------------------
//   Selection model. The sentinels are ordered by increasing platform
// specificity; they are values, not gates.

// 1.2.1
// D_CFG_FILE_BACKEND_AUTO
//   constant: pick the most capable backend env.h can prove is present.
#define D_CFG_FILE_BACKEND_AUTO   0

// 1.2.2
// D_CFG_FILE_BACKEND_STDC
//   constant: ISO C only (fopen/fseek/remove/rename). Maximum portability,
// minimum capability -- no descriptors, no locking, no directory walk.
#define D_CFG_FILE_BACKEND_STDC   1

// 1.2.3
// D_CFG_FILE_BACKEND_POSIX
//   constant: POSIX.1 calls (open/read/write/stat/opendir/flock). On Windows
// this means the CRT's _open/_read compatibility layer.
#define D_CFG_FILE_BACKEND_POSIX  2

// 1.2.4
// D_CFG_FILE_BACKEND_NATIVE
//   constant: the OS's own API (Win32 CreateFileA/ReadFile, Linux syscalls).
// Best throughput and the only way to reach platform-only semantics.
#define D_CFG_FILE_BACKEND_NATIVE 3

// 1.2.5
// D_CFG_FILE_BACKEND
//   knob: which implementation family the fs modules call into. Defaults to
// AUTO, which resolves to NATIVE on Windows (the CRT layer costs a
// translation on every call), POSIX on any POSIX host, and STDC otherwise.
// Pin it when cross-compiling to a host whose headers advertise more than
// the target actually implements.
#ifndef D_CFG_FILE_BACKEND
    #define D_CFG_FILE_BACKEND D_CFG_FILE_BACKEND_AUTO
#endif  // D_CFG_FILE_BACKEND

#if !D_CFG_IS_INT_LITERAL(D_CFG_FILE_BACKEND)
    #error "D_CFG_FILE_BACKEND must name one of its values; a misspelled name would read as 0"
#endif

// 1.2.6
// D_CFG_FILE_PREFER_ANNEX_K
//   knob: call the C11 Annex K bounds-checked functions (fopen_s, tmpfile_s)
// when the environment provides them. On by default: where Annex K exists it
// is the platform's own hardened path. Set 0 to always take djinterp's
// portable emulation, which behaves identically on every target.
#ifndef D_CFG_FILE_PREFER_ANNEX_K
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_PREFER_ANNEX_K D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_PREFER_ANNEX_K 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_PREFER_ANNEX_K

// 1.3    Parameter validation and error reporting
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_FILE_VALIDATE_PARAMS
//   knob: check pointer/range arguments at every public entry point. On by
// default -- the whole point of these wrappers is that d_file_open_stream(NULL,
// "r") returns NULL instead of trapping. Set 0 only for a measured hot path
// where the caller has already proven its arguments; the functions then inherit
// the underlying platform's undefined behaviour for bad input.
#ifndef D_CFG_FILE_VALIDATE_PARAMS
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_VALIDATE_PARAMS D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_VALIDATE_PARAMS 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_VALIDATE_PARAMS

// 1.3.2
// D_CFG_FILE_SET_ERRNO
//   knob: set errno (EINVAL, ENOMEM, EOVERFLOW, ...) when a call fails a
// djinterp-level check rather than a platform call. Keeps the failure mode
// indistinguishable from the native one. Set 0 on freestanding targets with
// no errno.
#ifndef D_CFG_FILE_SET_ERRNO
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_SET_ERRNO D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_SET_ERRNO 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_SET_ERRNO

// 1.3.3
// D_CFG_FILE_ASSERT_PARAMS
//   knob: additionally trip an assertion on a failed parameter check, so a
// contract violation is loud in a debug build instead of silently returning
// an error the caller ignores. Off by default; cfg_testing.h turns it on.
#ifndef D_CFG_FILE_ASSERT_PARAMS
    #define D_CFG_FILE_ASSERT_PARAMS 0
#endif  // D_CFG_FILE_ASSERT_PARAMS

// 1.4    Notifications / diagnostics
//------------------------------------------------------------------------------
//   The notification layer is how an fs module reports "what happened and
// why" without inventing a return channel. It is compiled out entirely when
// D_CFG_FILE_NOTIFY is 0 -- no code, no strings, no per-call branch.

// 1.4.1
// D_CFG_FILE_NOTIFY
//   knob: master enable for the notification hook. Off by default: it costs
// a branch per failure path and, more importantly, it retains the __func__
// and message string literals in the binary.
#ifndef D_CFG_FILE_NOTIFY
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_NOTIFY D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_NOTIFY 0
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_NOTIFY

// 1.4.2
// D_CFG_FILE_NOTIFY_LEVEL
//   knob: compile-time severity ceiling (value model). Notifications above
// this level are removed by the preprocessor, not filtered at runtime, so
// the strings they carry never reach the binary.
//   0 = none, 1 = error, 2 = warn, 3 = info, 4 = trace.
#ifndef D_CFG_FILE_NOTIFY_LEVEL
    #define D_CFG_FILE_NOTIFY_LEVEL 1
#endif  // D_CFG_FILE_NOTIFY_LEVEL

#if !D_CFG_IS_INT_LITERAL(D_CFG_FILE_NOTIFY_LEVEL)
    #error "D_CFG_FILE_NOTIFY_LEVEL must name one of its values; a misspelled name would read as 0"
#endif

// 1.4.3
// D_CFG_FILE_NOTIFY_DEFAULT_HANDLER
//   knob: install djinterp's built-in handler (writes a one-line record to
// D_CFG_FILE_NOTIFY_STREAM) as the starting handler. Off by default: a
// library must not write to a stream its host did not ask it to write to.
// Leave it off and call d_file_notify_set_handler() instead.
#ifndef D_CFG_FILE_NOTIFY_DEFAULT_HANDLER
    #define D_CFG_FILE_NOTIFY_DEFAULT_HANDLER 0
#endif  // D_CFG_FILE_NOTIFY_DEFAULT_HANDLER

// 1.4.4
// D_CFG_FILE_NOTIFY_STREAM
//   knob: FILE* expression the built-in handler writes to (value model).
#ifndef D_CFG_FILE_NOTIFY_STREAM
    #define D_CFG_FILE_NOTIFY_STREAM stderr
#endif  // D_CFG_FILE_NOTIFY_STREAM

// 1.4.5
// D_CFG_FILE_NOTIFY_PATHS
//   knob: include the offending path in the notice record. Off by default:
// paths are frequently sensitive (home directories, tenant identifiers) and
// a notification handler is often a log sink.
#ifndef D_CFG_FILE_NOTIFY_PATHS
    #define D_CFG_FILE_NOTIFY_PATHS 0
#endif  // D_CFG_FILE_NOTIFY_PATHS

// 1.5    Path limits and encoding
//------------------------------------------------------------------------------
// 1.5.1
// D_CFG_FILE_PATH_MAX
//   knob: maximum path length the fs modules will construct (value model).
// Not defaulted here -- 0.11.d derives it from the platform unless you set
// it. It sizes caller-visible buffers, so raising it is an ABI change for
// anything that embeds a path array.

// 1.5.2
// D_CFG_FILE_NAME_MAX
//   knob: maximum single filename length (value model). Sizes
// d_dirent_t.d_name; see the ABI note above.

// 1.5.3
// D_CFG_FILE_LONG_PATHS
//   knob: on Windows, transparently prefix absolute paths with \\?\ so they
// escape the 260-character MAX_PATH limit. Off by default because the prefix
// also disables path normalization by the OS: "..", "." and trailing dots
// stop being interpreted, which surprises code that leans on that behaviour.
// No effect on POSIX.
#ifndef D_CFG_FILE_LONG_PATHS
    #define D_CFG_FILE_LONG_PATHS 0
#endif  // D_CFG_FILE_LONG_PATHS

// 1.5.4
// D_CFG_FILE_WIDE_PATHS
//   knob: on Windows, treat incoming char* paths as UTF-8, convert to
// UTF-16 and call the wide API (_wfopen, CreateFileW). Off by default: it
// changes how non-ASCII paths are interpreted, which is a behavioural break
// for a program already passing ANSI-codepage bytes. Turn it on for any new
// program that wants to open a path outside the active codepage. No effect
// on POSIX, where paths are already opaque bytes.
#ifndef D_CFG_FILE_WIDE_PATHS
    #define D_CFG_FILE_WIDE_PATHS 0
#endif  // D_CFG_FILE_WIDE_PATHS

// 1.6    Allocation hooks
//------------------------------------------------------------------------------
//   Value model: these expand to callable expressions, not to 0/1. Every fs
// allocation goes through them, so a host with its own arena can capture all
// of it without patching sources.

// 1.6.1
// D_CFG_FILE_MALLOC
//   knob: allocation entry point; must match malloc's contract.
#ifndef D_CFG_FILE_MALLOC
    #define D_CFG_FILE_MALLOC(_size) malloc(_size)
#endif  // D_CFG_FILE_MALLOC

// 1.6.2
// D_CFG_FILE_REALLOC
//   knob: reallocation entry point; must match realloc's contract.
#ifndef D_CFG_FILE_REALLOC
    #define D_CFG_FILE_REALLOC(_ptr, _size) realloc(_ptr, _size)
#endif  // D_CFG_FILE_REALLOC

// 1.6.3
// D_CFG_FILE_FREE
//   knob: release entry point; must tolerate NULL, as free does.
#ifndef D_CFG_FILE_FREE
    #define D_CFG_FILE_FREE(_ptr) free(_ptr)
#endif  // D_CFG_FILE_FREE

// 1.6.4
// D_CFG_FILE_MAX_ALLOC
//   knob: refuse any single fs allocation larger than this many bytes (value
// model). This is the guard that stops d_file_read_all() on a 40 GiB file from
// being an out-of-memory event in a process that only wanted a config file. 0
// disables the ceiling. Default 1 GiB.
#ifndef D_CFG_FILE_MAX_ALLOC
    #define D_CFG_FILE_MAX_ALLOC (1024u * 1024u * 1024u)
#endif  // D_CFG_FILE_MAX_ALLOC

// 1.7    Descriptor hygiene and robustness
//------------------------------------------------------------------------------
// 1.7.1
// D_CFG_FILE_CLOEXEC_DEFAULT
//   knob: open descriptors close-on-exec unless the caller asks otherwise.
// On by default: a descriptor leaked across fork/exec into a child process
// is both a handle leak and a capability leak. Set 0 if you deliberately
// pass descriptors to children by inheritance.
#ifndef D_CFG_FILE_CLOEXEC_DEFAULT
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_CLOEXEC_DEFAULT D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_CLOEXEC_DEFAULT 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_CLOEXEC_DEFAULT

// 1.7.2
// D_CFG_FILE_EINTR_RETRY
//   knob: transparently retry a call the kernel interrupted with EINTR. On
// by default: on a POSIX host any signal (a profiler's SIGPROF, SIGWINCH on
// a resize) can interrupt a blocking read, and almost no caller wants to see
// that. Set 0 if your program installs handlers and needs to observe EINTR.
#ifndef D_CFG_FILE_EINTR_RETRY
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_EINTR_RETRY D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_EINTR_RETRY 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_EINTR_RETRY

// 1.7.3
// D_CFG_FILE_THREADSAFE
//   knob: use the reentrant platform variants and per-handle state so two
// threads may safely drive two different handles. On by default. It does NOT
// promise that one handle may be shared unsynchronized between threads -- no
// setting here provides that.
#ifndef D_CFG_FILE_THREADSAFE
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_THREADSAFE D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_THREADSAFE 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_THREADSAFE

// 1.8    Environment optimization
//------------------------------------------------------------------------------
// 1.8.1
// D_CFG_FILE_USE_RESTRICT
//   knob: qualify non-aliasing pointer parameters with restrict. Follows
// D_CFG_FILE_OPTIMIZE and the language level (C99+ only; a C++ consumer of
// these headers gets the compiler's __restrict spelling or nothing).
#ifndef D_CFG_FILE_USE_RESTRICT
    #define D_CFG_FILE_USE_RESTRICT D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_USE_RESTRICT

// 1.8.2
// D_CFG_FILE_INLINE_PREDICATES
//   knob: publish the trivial predicates (d_file_exists, d_dir_exists, ...) as
// header inlines instead of out-of-line calls. Follows D_CFG_FILE_OPTIMIZE.
// Set 0 to keep every symbol out-of-line, which is what a coverage or
// interposition build wants.
#ifndef D_CFG_FILE_INLINE_PREDICATES
    #define D_CFG_FILE_INLINE_PREDICATES D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_INLINE_PREDICATES

// 1.8.3
// D_CFG_FILE_FADVISE
//   knob: hand the kernel access-pattern hints (posix_fadvise on POSIX,
// FILE_FLAG_SEQUENTIAL_SCAN on Win32) when a module knows the pattern up
// front, as whole-file reads do. Follows D_CFG_FILE_OPTIMIZE; ignored where
// the platform has no equivalent.
#ifndef D_CFG_FILE_FADVISE
    #define D_CFG_FILE_FADVISE D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_FADVISE

// 1.9    Platform feature gates
//------------------------------------------------------------------------------
//   Each gate answers "does this target have the call at all". They are
// env-detected but user-overridable, which is the escape hatch for a cross
// build whose host headers lie about the target. A module reads the
// D_INTERNAL_FILE_HAS_* form (0.11.e), never these.
//
//   INVARIANT: a gate whose call is declared in a POSIX header must include
// D_CFG_FILE_HAS_POSIX in its own condition. file_common.h includes
// <unistd.h>, <fcntl.h>, <dirent.h> and friends ONLY when HAS_POSIX is 1, so
// a gate that says "this target has posix_fadvise" while HAS_POSIX is 0 is
// claiming a capability whose declaration the build will never see -- and the
// module that believes it fails to compile.
//   This is the same error as gating on the platform instead of the
// capability, wearing a different hat: `defined(D_ENV_PLATFORM_LINUX)` is not
// "has POSIX", it is "is Linux", and a build can be told it is neither.
// Setting -DD_CFG_FILE_HAS_POSIX=0 on a Linux host is exactly that, and it is
// what caught five of these.

// 1.9.1
// D_CFG_FILE_HAS_POSIX
//   knob: the POSIX file API is present.
#ifndef D_CFG_FILE_HAS_POSIX
    #if ( (defined(D_ENV_PLATFORM_LINUX)) ||                                   \
          (defined(D_ENV_PLATFORM_MACOS)) ||                                   \
          (defined(D_ENV_PLATFORM_UNIX)) )
        #define D_CFG_FILE_HAS_POSIX 1
    #else
        #define D_CFG_FILE_HAS_POSIX 0
    #endif
#endif  // D_CFG_FILE_HAS_POSIX

// 1.9.2
// D_CFG_FILE_HAS_WIN32
//   knob: the Win32 file API is present.
#ifndef D_CFG_FILE_HAS_WIN32
    #if defined(D_ENV_PLATFORM_WINDOWS)
        #define D_CFG_FILE_HAS_WIN32 1
    #else
        #define D_CFG_FILE_HAS_WIN32 0
    #endif
#endif  // D_CFG_FILE_HAS_WIN32

// 1.9.3
// D_CFG_FILE_HAS_FOPEN_S
//   knob: C11 Annex K / MSVC secure fopen_s is available.
#ifndef D_CFG_FILE_HAS_FOPEN_S
    #if ( (defined(__STDC_LIB_EXT1__)) ||                                      \
          (defined(__STDC_SECURE_LIB__)) )
        #define D_CFG_FILE_HAS_FOPEN_S 1
    #elif ( (defined(D_ENV_MSC_VER)) &&                                        \
            (D_ENV_MSC_VER >= 1400) )
        #define D_CFG_FILE_HAS_FOPEN_S 1
    #else
        #define D_CFG_FILE_HAS_FOPEN_S 0
    #endif
#endif  // D_CFG_FILE_HAS_FOPEN_S

// 1.9.4
// D_CFG_FILE_HAS_FSEEKO
//   knob: 64-bit fseeko/ftello are available.
#ifndef D_CFG_FILE_HAS_FSEEKO
    #if ( (defined(_POSIX_C_SOURCE)) &&                                        \
          (_POSIX_C_SOURCE >= D_ENV_POSIX_C_SOURCE_200112L) )
        #define D_CFG_FILE_HAS_FSEEKO 1
    #elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
        #define D_CFG_FILE_HAS_FSEEKO 1
    #else
        #define D_CFG_FILE_HAS_FSEEKO 0
    #endif
#endif  // D_CFG_FILE_HAS_FSEEKO

// 1.9.5
// D_CFG_FILE_HAS_MKSTEMP
//   knob: mkstemp is available.
#ifndef D_CFG_FILE_HAS_MKSTEMP
    #define D_CFG_FILE_HAS_MKSTEMP D_CFG_FILE_HAS_POSIX
#endif  // D_CFG_FILE_HAS_MKSTEMP

// 1.9.6
// D_CFG_FILE_HAS_REALPATH
//   knob: realpath is available.
#ifndef D_CFG_FILE_HAS_REALPATH
    #define D_CFG_FILE_HAS_REALPATH D_CFG_FILE_HAS_POSIX
#endif  // D_CFG_FILE_HAS_REALPATH

// 1.9.7
// D_CFG_FILE_HAS_SYMLINKS
//   knob: the target supports symbolic links. True on Windows Vista+, where
// creating one still needs SeCreateSymbolicLinkPrivilege -- presence here is
// a compile-time claim, not a runtime permission.
#ifndef D_CFG_FILE_HAS_SYMLINKS
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) ||                               \
          (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
        #define D_CFG_FILE_HAS_SYMLINKS 1
    #else
        #define D_CFG_FILE_HAS_SYMLINKS 0
    #endif
#endif  // D_CFG_FILE_HAS_SYMLINKS

// 1.9.8
// D_CFG_FILE_HAS_PIPES
//   knob: popen/pclose (or _popen/_pclose) are available.
#ifndef D_CFG_FILE_HAS_PIPES
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) ||                               \
          (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
        #define D_CFG_FILE_HAS_PIPES 1
    #else
        #define D_CFG_FILE_HAS_PIPES 0
    #endif
#endif  // D_CFG_FILE_HAS_PIPES

// 1.9.9
// D_CFG_FILE_HAS_FLOCK
//   knob: BSD flock() is available. Not implied by POSIX -- Solaris and
// some embedded libcs ship fcntl locking only.
#ifndef D_CFG_FILE_HAS_FLOCK
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          ( (defined(D_ENV_PLATFORM_LINUX)) ||                                 \
            (defined(D_ENV_PLATFORM_MACOS)) ) )
        #define D_CFG_FILE_HAS_FLOCK 1
    #else
        #define D_CFG_FILE_HAS_FLOCK 0
    #endif
#endif  // D_CFG_FILE_HAS_FLOCK

// 1.9.10
// D_CFG_FILE_HAS_PREAD
//   knob: positional pread/pwrite are available. They are the only way to
// read at an offset without a seek+read race on a shared descriptor.
#ifndef D_CFG_FILE_HAS_PREAD
    #define D_CFG_FILE_HAS_PREAD D_CFG_FILE_HAS_POSIX
#endif  // D_CFG_FILE_HAS_PREAD

// 1.9.11
// D_CFG_FILE_HAS_COPY_FILE_RANGE
//   knob: Linux copy_file_range(2) -- an in-kernel copy that can offload to
// the filesystem (reflink on btrfs/XFS) and never touches user space.
#ifndef D_CFG_FILE_HAS_COPY_FILE_RANGE
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (defined(D_ENV_PLATFORM_LINUX)) &&                                   \
          (defined(__GLIBC__)) )
        #define D_CFG_FILE_HAS_COPY_FILE_RANGE 1
    #else
        #define D_CFG_FILE_HAS_COPY_FILE_RANGE 0
    #endif
#endif  // D_CFG_FILE_HAS_COPY_FILE_RANGE

// 1.9.12
// D_CFG_FILE_HAS_FCOPYFILE
//   knob: macOS fcopyfile(3) -- the Apple equivalent, and the only portable
// way to bring resource forks and metadata along.
#ifndef D_CFG_FILE_HAS_FCOPYFILE
    #if defined(D_ENV_PLATFORM_MACOS)
        #define D_CFG_FILE_HAS_FCOPYFILE 1
    #else
        #define D_CFG_FILE_HAS_FCOPYFILE 0
    #endif
#endif  // D_CFG_FILE_HAS_FCOPYFILE

// 1.9.13
// D_CFG_FILE_HAS_STATX
//   knob: Linux statx(2) -- one syscall for metadata that stat() reports in
// pieces, including true creation time.
#ifndef D_CFG_FILE_HAS_STATX
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (defined(D_ENV_PLATFORM_LINUX)) &&                                   \
          (defined(__GLIBC__)) )
        #define D_CFG_FILE_HAS_STATX 1
    #else
        #define D_CFG_FILE_HAS_STATX 0
    #endif
#endif  // D_CFG_FILE_HAS_STATX

// 1.9.14
// D_CFG_FILE_HAS_FADVISE
//   knob: posix_fadvise is available.
#ifndef D_CFG_FILE_HAS_FADVISE
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (defined(D_ENV_PLATFORM_LINUX)) )
        #define D_CFG_FILE_HAS_FADVISE 1
    #else
        #define D_CFG_FILE_HAS_FADVISE 0
    #endif
#endif  // D_CFG_FILE_HAS_FADVISE

// 1.9.15
// D_CFG_FILE_HAS_FALLOCATE
//   knob: preallocation is available (posix_fallocate / SetFileValidData).
// Lets a known-size write reserve extents up front instead of growing the
// file one write at a time.
#ifndef D_CFG_FILE_HAS_FALLOCATE
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (defined(D_ENV_PLATFORM_LINUX)) )
        #define D_CFG_FILE_HAS_FALLOCATE 1
    #else
        #define D_CFG_FILE_HAS_FALLOCATE 0
    #endif
#endif  // D_CFG_FILE_HAS_FALLOCATE

// 1.9.16
// D_CFG_FILE_HAS_STAT_NSEC
//   knob: the platform reports sub-second timestamps, and by which spelling.
// POSIX.1-2008 named the member st_mtim (a struct timespec); the BSDs and
// macOS got there first with st_mtimespec; older hosts have neither and
// report whole seconds only.
//   This is what env CAN do about the timestamp problem: it cannot make a
// field named st_mtime reachable (see the note on d_stat_t in file_common.h),
// but it can tell file_stat.c which member to READ when filling
// d_stat_t.st_modified. Detection here, spelling in the module.
#ifndef D_CFG_FILE_HAS_STAT_NSEC
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (defined(D_ENV_PLATFORM_LINUX)) &&                                   \
          (defined(_POSIX_C_SOURCE)) &&                                        \
          (_POSIX_C_SOURCE >= 200809L) )
        #define D_CFG_FILE_HAS_STAT_NSEC 1   // st_mtim.tv_nsec
    #elif ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                             \
            (defined(D_ENV_PLATFORM_MACOS)) )
        #define D_CFG_FILE_HAS_STAT_NSEC 2   // st_mtimespec.tv_nsec
    #elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        #define D_CFG_FILE_HAS_STAT_NSEC 3   // FILETIME, 100ns ticks
    #else
        #define D_CFG_FILE_HAS_STAT_NSEC 0   // whole seconds only
    #endif
#endif  // D_CFG_FILE_HAS_STAT_NSEC

// 1.9.17
// D_CFG_FILE_HAS_BIRTHTIME
//   knob: the platform reports a true CREATION time, as distinct from
// POSIX's ctime (which is the metadata-change time and is not it).
// Windows has always had it; the BSDs and macOS expose st_birthtime; Linux
// only through statx(2), and only on filesystems that store it.
//   d_stat_t.st_changed carries ctime everywhere; a creation timestamp, where
// one exists, is a separate concern and does not get silently substituted --
// that conflation is exactly what the old st_ctime field name encouraged.
#ifndef D_CFG_FILE_HAS_BIRTHTIME
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) ||                               \
          (defined(D_ENV_PLATFORM_MACOS)) ||                                   \
          (defined(D_ENV_PLATFORM_BSD)) )
        #define D_CFG_FILE_HAS_BIRTHTIME 1
    #elif D_CFG_IS_ON(D_CFG_FILE_HAS_STATX)
        #define D_CFG_FILE_HAS_BIRTHTIME 1
    #else
        #define D_CFG_FILE_HAS_BIRTHTIME 0
    #endif
#endif  // D_CFG_FILE_HAS_BIRTHTIME


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
//   Cheap, and it catches a typo before it silently mis-gates a definition.
//   Only the boolean gates are checked here; selection and value knobs are
// validated by their own rules below.

#if !D_CFG_IS_BOOL(D_CFG_FILE_OPTIMIZE)
    #error "D_CFG_FILE_OPTIMIZE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_PREFER_ANNEX_K)
    #error "D_CFG_FILE_PREFER_ANNEX_K must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_VALIDATE_PARAMS)
    #error "D_CFG_FILE_VALIDATE_PARAMS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_SET_ERRNO)
    #error "D_CFG_FILE_SET_ERRNO must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_ASSERT_PARAMS)
    #error "D_CFG_FILE_ASSERT_PARAMS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_NOTIFY)
    #error "D_CFG_FILE_NOTIFY must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_NOTIFY_DEFAULT_HANDLER)
    #error "D_CFG_FILE_NOTIFY_DEFAULT_HANDLER must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_NOTIFY_PATHS)
    #error "D_CFG_FILE_NOTIFY_PATHS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_LONG_PATHS)
    #error "D_CFG_FILE_LONG_PATHS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_WIDE_PATHS)
    #error "D_CFG_FILE_WIDE_PATHS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_CLOEXEC_DEFAULT)
    #error "D_CFG_FILE_CLOEXEC_DEFAULT must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_EINTR_RETRY)
    #error "D_CFG_FILE_EINTR_RETRY must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_THREADSAFE)
    #error "D_CFG_FILE_THREADSAFE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_USE_RESTRICT)
    #error "D_CFG_FILE_USE_RESTRICT must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_INLINE_PREDICATES)
    #error "D_CFG_FILE_INLINE_PREDICATES must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_FADVISE)
    #error "D_CFG_FILE_FADVISE must be literally 0 or 1"
#endif

// selection knob: must name a backend that exists.
#if ( (D_CFG_NORM(D_CFG_FILE_BACKEND) < D_CFG_FILE_BACKEND_AUTO) ||            \
      (D_CFG_NORM(D_CFG_FILE_BACKEND) > D_CFG_FILE_BACKEND_NATIVE) )
    #error "D_CFG_FILE_BACKEND must be one of the D_CFG_FILE_BACKEND_* values"
#endif

// value knob: severity is a closed range.
#if ( (D_CFG_NORM(D_CFG_FILE_NOTIFY_LEVEL) < 0) ||                             \
      (D_CFG_NORM(D_CFG_FILE_NOTIFY_LEVEL) > 4) )
    #error "D_CFG_FILE_NOTIFY_LEVEL must be 0..4"
#endif

// a pinned backend the target cannot supply is a build error, not a silent
// downgrade -- a silent downgrade is how you ship a binary that is missing
// the semantics you pinned it for.
#if ( (D_CFG_NORM(D_CFG_FILE_BACKEND) == D_CFG_FILE_BACKEND_POSIX) &&          \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_POSIX)) )
    #error "D_CFG_FILE_BACKEND == POSIX but D_CFG_FILE_HAS_POSIX is 0"
#endif
#if ( (D_CFG_NORM(D_CFG_FILE_BACKEND) == D_CFG_FILE_BACKEND_NATIVE) &&         \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_POSIX)) &&                                  \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_WIN32)) )
    #error "D_CFG_FILE_BACKEND == NATIVE but no native API was detected"
#endif
#if ( (D_CFG_IS_ON(D_CFG_FILE_WIDE_PATHS)) &&                                  \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_WIN32)) )
    #error "D_CFG_FILE_WIDE_PATHS is Windows-only"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
//   What the modules actually read. Do not override these; set the D_CFG_*
// knobs above instead.

// 3.1.1
// D_INTERNAL_FILE_BACKEND
//   resolved: the resolved backend -- one of the D_CFG_FILE_BACKEND_*
// sentinels, never AUTO. Computed from the requested backend and the detected
// platform.
#if (D_CFG_NORM(D_CFG_FILE_BACKEND) != D_CFG_FILE_BACKEND_AUTO)
    #define D_INTERNAL_FILE_BACKEND D_CFG_NORM(D_CFG_FILE_BACKEND)
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_FILE_BACKEND D_CFG_FILE_BACKEND_NATIVE
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    #define D_INTERNAL_FILE_BACKEND D_CFG_FILE_BACKEND_POSIX
#else
    #define D_INTERNAL_FILE_BACKEND D_CFG_FILE_BACKEND_STDC
#endif

// 3.1.2
// D_INTERNAL_FILE_VALIDATE
//   resolved: 1 when public entry points check their arguments.
#if D_CFG_IS_ON(D_CFG_FILE_VALIDATE_PARAMS)
    #define D_INTERNAL_FILE_VALIDATE 1
#else
    #define D_INTERNAL_FILE_VALIDATE 0
#endif

// 3.1.3
// D_INTERNAL_FILE_SET_ERRNO
//   resolved: 1 when a djinterp-level failure sets errno.
//   NOT folded with VALIDATE, though it used to be, on the reasoning that
// "setting errno for a check that never runs is dead code". That holds for
// D_INTERNAL_FILE_REQUIRE, whose checks do vanish with validation off -- but
// D_INTERNAL_FILE_SET_ERR is also reached from D_INTERNAL_FILE_FAIL, which is
// not a parameter check and always runs: EEXIST from a non-overwrite rename,
// ENOSYS from an unsupported backend, ERANGE from a buffer too small, "source
// is not a regular file". Folding left every one of those returning -1 with
// errno untouched, so a caller could see the failure but not what it was --
// and the knob's own description promises the opposite, that the failure mode
// stays "indistinguishable from the native one".
//   There is no dead code to remove either way: where SET_ERR is reached only
// through REQUIRE, the REQUIRE macro compiles to nothing and takes the SET_ERR
// inside it along.
#if D_CFG_IS_ON(D_CFG_FILE_SET_ERRNO)
    #define D_INTERNAL_FILE_SET_ERRNO 1
#else
    #define D_INTERNAL_FILE_SET_ERRNO 0
#endif

// 3.1.4
// D_INTERNAL_FILE_NOTIFY_LEVEL
//   resolved: the effective severity ceiling. Collapses to 0 (fully compiled
// out) whenever the master switch is off, so a module only has to test this
// one value.
#if D_CFG_IS_ON(D_CFG_FILE_NOTIFY)
    #define D_INTERNAL_FILE_NOTIFY_LEVEL D_CFG_NORM(D_CFG_FILE_NOTIFY_LEVEL)
#else
    #define D_INTERNAL_FILE_NOTIFY_LEVEL 0
#endif

// 3.1.5
// D_INTERNAL_FILE_PATH_MAX
//   resolved: the effective maximum path length. Honours an explicit
// D_CFG_FILE_PATH_MAX, else derives it: Windows is 260 unless long paths are
// enabled (then 32767, the \\?\ ceiling); POSIX takes PATH_MAX when the
// headers name it; everything else gets 4096.
#ifdef D_CFG_FILE_PATH_MAX
    #define D_INTERNAL_FILE_PATH_MAX D_CFG_FILE_PATH_MAX
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #if D_CFG_IS_ON(D_CFG_FILE_LONG_PATHS)
        #define D_INTERNAL_FILE_PATH_MAX 32767
    #else
        #define D_INTERNAL_FILE_PATH_MAX 260
    #endif
#elif defined(PATH_MAX)
    #define D_INTERNAL_FILE_PATH_MAX PATH_MAX
#else
    #define D_INTERNAL_FILE_PATH_MAX 4096
#endif  // D_CFG_FILE_PATH_MAX

// 3.1.6
// D_INTERNAL_FILE_NAME_MAX
//   resolved: the effective maximum single-filename length.
#ifdef D_CFG_FILE_NAME_MAX
    #define D_INTERNAL_FILE_NAME_MAX D_CFG_FILE_NAME_MAX
#elif defined(NAME_MAX)
    #define D_INTERNAL_FILE_NAME_MAX NAME_MAX
#else
    #define D_INTERNAL_FILE_NAME_MAX 255
#endif  // D_CFG_FILE_NAME_MAX

// 3.1.7
// D_INTERNAL_FILE_HAS_FOPEN_S
//   resolved: 1 when the modules should call Annex K's fopen_s -- it must both
// exist and be preferred.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_FOPEN_S)) &&                                 \
      (D_CFG_IS_ON(D_CFG_FILE_PREFER_ANNEX_K)) )
    #define D_INTERNAL_FILE_HAS_FOPEN_S 1
#else
    #define D_INTERNAL_FILE_HAS_FOPEN_S 0
#endif

// 3.1.8
// D_INTERNAL_FILE_HAS_FSEEKO
//   resolved: 1 when fseeko/ftello may be called. Requires a backend above
// STDC: the ISO C backend has only fseek/ftell by definition.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_FSEEKO)) &&                                  \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_HAS_FSEEKO 1
#else
    #define D_INTERNAL_FILE_HAS_FSEEKO 0
#endif

// 3.1.9
// D_INTERNAL_FILE_HAS_MKSTEMP
//   resolved: 1 when mkstemp may be called.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_MKSTEMP)) &&                                 \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_HAS_MKSTEMP 1
#else
    #define D_INTERNAL_FILE_HAS_MKSTEMP 0
#endif

// 3.1.10
// D_INTERNAL_FILE_HAS_REALPATH
//   resolved: 1 when realpath may be called.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_REALPATH)) &&                                \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_HAS_REALPATH 1
#else
    #define D_INTERNAL_FILE_HAS_REALPATH 0
#endif

// 3.1.11
// D_INTERNAL_FILE_HAS_SYMLINKS
//   resolved: 1 when file_link.h publishes its API.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_SYMLINKS)) &&                                \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_HAS_SYMLINKS 1
#else
    #define D_INTERNAL_FILE_HAS_SYMLINKS 0
#endif

// 3.1.12
// D_INTERNAL_FILE_HAS_PIPES
//   resolved: 1 when file_pipe.h publishes its API.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_PIPES)) &&                                   \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_HAS_PIPES 1
#else
    #define D_INTERNAL_FILE_HAS_PIPES 0
#endif

// 3.1.13
// D_INTERNAL_FILE_HAS_PREAD
//   resolved: 1 when positional reads/writes may be called.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_PREAD)) &&                                   \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_HAS_PREAD 1
#else
    #define D_INTERNAL_FILE_HAS_PREAD 0
#endif

// 3.1.14
// D_INTERNAL_FILE_HAS_FADVISE
//   resolved: 1 when the modules should issue kernel access hints.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_FADVISE)) &&                                 \
      (D_CFG_IS_ON(D_CFG_FILE_FADVISE)) )
    #define D_INTERNAL_FILE_HAS_FADVISE 1
#else
    #define D_INTERNAL_FILE_HAS_FADVISE 0
#endif

// 3.1.15
// D_INTERNAL_FILE_USE_NATIVE_COPY
//   resolved: 1 when file_ops may offload a whole-file copy to the platform
// (copy_file_range / fcopyfile / CopyFileEx) instead of shuttling the bytes
// through a user-space buffer.
#if ( (D_CFG_IS_ON(D_CFG_FILE_OPTIMIZE)) &&                                    \
      ( (D_CFG_IS_ON(D_CFG_FILE_HAS_COPY_FILE_RANGE)) ||                       \
        (D_CFG_IS_ON(D_CFG_FILE_HAS_FCOPYFILE)) ||                             \
        (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) ) )
    #define D_INTERNAL_FILE_USE_NATIVE_COPY 1
#else
    #define D_INTERNAL_FILE_USE_NATIVE_COPY 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
//   #if-safe answers to "how was this build configured", for user code and
// for the test suite, which must skip what the build cannot do.

// 4.1.1
// D_FILE_BACKEND_IS_NATIVE / _IS_POSIX / _IS_STDC
//   query: 1 when the named backend is the resolved one, else 0.
#define D_FILE_BACKEND_IS_NATIVE                                               \
    (D_INTERNAL_FILE_BACKEND == D_CFG_FILE_BACKEND_NATIVE)
#define D_FILE_BACKEND_IS_POSIX                                                \
    (D_INTERNAL_FILE_BACKEND == D_CFG_FILE_BACKEND_POSIX)
#define D_FILE_BACKEND_IS_STDC                                                 \
    (D_INTERNAL_FILE_BACKEND == D_CFG_FILE_BACKEND_STDC)

// 4.1.2
// D_FILE_NOTIFY_IS_ACTIVE
//   query: 1 when any notification survives the preprocessor, else 0.
#define D_FILE_NOTIFY_IS_ACTIVE (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)

// 4.1.3
// D_FILE_VALIDATE_IS_ACTIVE
//   query: 1 when the modules check their caller's parameters, else 0.
//   Published because "does this build reject a NULL path" is a question a
// CONSUMER has to be able to ask, and D_INTERNAL_FILE_VALIDATE is not theirs
// to read. With validation off, passing NULL is undefined by contract -- the
// knob is the caller promising not to -- so a test asserting a specific return
// value for it is asserting something this build never promised. Without a
// published macro the suite could not tell the two builds apart, and the only
// evidence was a segfault.
#define D_FILE_VALIDATE_IS_ACTIVE (D_INTERNAL_FILE_VALIDATE == 1)


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_COMMON_H
