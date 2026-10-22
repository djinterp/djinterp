/*******************************************************************************
* djinterp [config]                                            cfg_file_common.h
*
* Build-time configuration shared by every module of the djinterp filesys
* subframework (c/filesys/file_*.h).  It owns the knobs that are meaningful to
* more than one module -- backend selection, parameter validation,
* diagnostics, path limits, allocation hooks, descriptor hygiene -- and it
* owns the platform feature gates that used to live inside the old,
* monolithic dfile.h.  It publishes the D_INTERNAL_FILE_* "effective" values
* that the modules read.
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
*   targets:  c/filesys/file_common.h and every c/filesys/file_*.h ->
*             D_INTERNAL_FILE_*
*   requires: cfg_common.h, env/env.h, and re_std's dstdint.h (INT64_MAX)
*
*
* path:      /inc/djinterp/config/c/filesys/cfg_file_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.20
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
         2.  D_CFG_FILE_FADVISE
    9.  Platform feature gates
         0.  D_INTERNAL_FILE_OS_* (families; constants, not knobs)
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
         18. D_CFG_FILE_HAS_TMPFILE
         19. D_CFG_FILE_HAS_CLOCK
    10. Width
         1.  D_CFG_FILE_WIDTH_AUTO
         2.  D_CFG_FILE_WIDTH
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
         16. D_INTERNAL_FILE_HAS_INT64
         17. D_INTERNAL_FILE_NATURAL_WIDTH
         18. D_INTERNAL_FILE_WIDTH
         19. D_INTERNAL_FILE_HAS_TIME_BITS
         20. D_INTERNAL_FILE_WIDE
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_BACKEND_IS_NATIVE / _IS_POSIX / _IS_STDC
         2.  D_FILE_NOTIFY_IS_ACTIVE
         3.  D_FILE_VALIDATE_IS_ACTIVE
         4.  D_FILE_WIDTH
         5.  D_FILE_64_IS_AVAILABLE
         6.  D_FILE_WIDTH_IS_NARROWED
*/

#ifndef DJINTERP_CONFIG_C_FILESYS_CFG_FILE_COMMON_H
#define DJINTERP_CONFIG_C_FILESYS_CFG_FILE_COMMON_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_IS_ON, D_CFG_NORM, D_CFG_TESTING
#include "../../../env/env.h"  // D_ENV_* platform detection
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: the 64-bit floor


//==============================================================================
// 1.  KNOBS
//==============================================================================
//   Pre-define any knob before including a c/filesys header to override it;
// otherwise it takes the default resolved here.


// 1.1    Master switches
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_ALL
//   knob: optional aggregate for the filesys subframework's behaviour gates.
// It is NOT defaulted here -- its absence is meaningful. Define it to 0 or 1
// and the gates whose defaults are written to follow it -- validation, errno,
// notifications, descriptor hygiene and Annex K here, and some of the
// modules' own -- take its value unless given one of their own; the #ifndef
// block under each knob shows which. Individual knobs still win over it. The
// platform gates (D_CFG_FILE_HAS_*) never follow it: they say what the target
// has, not what a program wants.

// 1.1.2
// D_CFG_FILE_OPTIMIZE
//   knob: allow the filesys modules to take platform fast paths (native copy
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
// specificity; they are values, not gates. The modules tell only two of them
// apart: the ISO C backend, and the platform's. POSIX and NATIVE build the
// same code -- each platform's own calls -- and differ only in the name
// d_file_backend_name reports.

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
//   constant: the platform's calls on a POSIX host -- open, read, stat,
// opendir, flock and the rest. It needs D_CFG_FILE_HAS_POSIX, so it cannot be
// pinned on Windows (2.1).
#define D_CFG_FILE_BACKEND_POSIX  2

// 1.2.4
// D_CFG_FILE_BACKEND_NATIVE
//   constant: the platform's calls, as AUTO names them on Windows: Win32
// (CreateFileA, FindFirstFileA, LockFileEx) beside the CRT's descriptor layer
// (_open, _read). Pinned on a POSIX host, it builds the same code as POSIX.
#define D_CFG_FILE_BACKEND_NATIVE 3

// 1.2.5
// D_CFG_FILE_BACKEND
//   knob: which implementation family the filesys modules call into. Defaults
// to AUTO, which resolves to NATIVE on Windows, POSIX on any POSIX host, and
// STDC otherwise. Pin STDC when cross-compiling to a target whose headers
// advertise more than it implements, or for ISO C behaviour everywhere;
// pinning POSIX or NATIVE only checks that the target has the API (2.1).
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
//   knob: trip an assertion on a failed parameter check, so that a contract
// violation is loud in a debug build instead of an error the caller ignores.
// A placeholder: nothing reads it yet, so a failed check returns its error
// whatever this is. It waits on the framework's assertion subframework
// (D_CFG_DEBUG_ASSERTS); when a module reads it, the tests that pass invalid
// parameters on purpose must expect the assertion, or set it to 0. Off by
// default; cfg_testing.h turns it on.
#ifndef D_CFG_FILE_ASSERT_PARAMS
    #define D_CFG_FILE_ASSERT_PARAMS 0
#endif  // D_CFG_FILE_ASSERT_PARAMS

// 1.4    Notifications / diagnostics
//------------------------------------------------------------------------------
//   The notification layer is how a filesys module reports "what happened and
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
//   knob: maximum path length the filesys modules will construct (value model).
// Not defaulted here -- 3.1.5 derives it from the platform unless you set
// it. It sizes caller-visible buffers, so raising it is an ABI change for
// anything that embeds a path array.

// 1.5.2
// D_CFG_FILE_NAME_MAX
//   knob: maximum single filename length (value model). Sizes
// d_dirent_t.d_name; see the ABI note above.

// 1.5.3
// D_CFG_FILE_LONG_PATHS
//   knob: on Windows, take paths past MAX_PATH's 260 characters. The paths
// this build constructs are sized for 32767 (D_FILE_PATH_MAX, 3.1.5), and a
// path at or past MAX_PATH less 12 -- the limit CreateDirectoryW keeps, the
// shortest a path call has -- is made absolute with GetFullPathNameW and
// handed to Windows behind the \\?\ prefix (\\?\UNC\ for a share), which
// lifts the limit. A device path (\\.\), or one already in that form, goes
// as it is, and d_path_resolve gives a path back without a prefix it added.
// The prefix is the wide calls' alone, so it takes D_CFG_FILE_WIDE_PATHS
// (1.5.4); without that, a longer path fits this build's buffers, and
// whether it opens is the platform's answer. Off by default: every path
// buffer grows from 260 characters to 32767, and is kept on the heap rather
// than the stack -- a path's UTF-16 copy, 64 KiB, and a path built in UTF-8,
// 32 KiB (d_internal_file_native and d_internal_file_path, file_common.h);
// a default build keeps its 260-character ones on the stack. No effect on
// POSIX.
#ifndef D_CFG_FILE_LONG_PATHS
    #define D_CFG_FILE_LONG_PATHS 0
#endif  // D_CFG_FILE_LONG_PATHS

// 1.5.4
// D_CFG_FILE_WIDE_PATHS
//   knob: on Windows, read every char* path as UTF-8, and hand it to the
// wide API (_wfopen, _wopen, CreateFileW) as UTF-16, so a name outside the
// active code page reaches the filesystem as it was spelled. Text that is not
// UTF-8 fails with EILSEQ, and a path whose UTF-16 form will not fit
// D_FILE_PATH_MAX characters with ENAMETOOLONG. The conversions are
// file_common.h's (6.6).
//   Every module takes it: a path goes in as UTF-8, and a name the platform
// hands back -- an entry d_dir_read64 reads, the working directory, a full
// path, d_dir_temp's directory, a link's target -- comes back as UTF-8,
// D_FILE_NAME_MAX growing to hold any name Windows can (3.1.6). A name with
// a lone surrogate, which Windows allows and UTF-8 cannot carry, fails with
// EILSEQ, and a listing skips it, as it skips a name past D_FILE_NAME_MAX.
//   On by default, since every module takes it (2026.10.20): a path is read
// as UTF-8 on Windows as everywhere else. A program passing code-page bytes
// sets it to 0, and gets the A calls, and the code page, back. No effect on
// POSIX, where a path is bytes and UTF-8 is already what reaches the
// filesystem.
#ifndef D_CFG_FILE_WIDE_PATHS
    #define D_CFG_FILE_WIDE_PATHS 1
#endif  // D_CFG_FILE_WIDE_PATHS

// 1.6    Allocation hooks
//------------------------------------------------------------------------------
//   Value model: these expand to callable expressions, not to 0/1. Every
// filesys allocation goes through them, so a host with its own arena can
// capture all of it without patching sources.

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
//   knob: refuse any single filesys allocation larger than this many bytes
// (value model). This is the guard that stops d_file_read_all() on a 40 GiB
// file from being an out-of-memory event in a process that only wanted a
// config file. 0 disables the ceiling, and so does a ceiling past what a
// size_t holds, which refuses nothing a size_t can ask for: one value serves a
// 32-bit build and a 64-bit one alike. A negative one is refused (2.1).
// Default 1 GiB.
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
//   knob: two threads may safely drive two different handles. Nothing reads
// it, because that is so whatever it is set to: the modules keep no state
// between calls except the notification hook, set once at startup by
// contract, and file_stat's statx memo, which is atomic. So 0 changes
// nothing. It does NOT promise that one handle may be shared unsynchronized
// between threads -- no setting here provides that.
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
// build whose host headers lie about the target. Where section 3 resolves a
// gate into a D_INTERNAL_FILE_HAS_* value, folding in the backend or a knob, a
// module reads that value and never the gate; the gates with no resolved form
// -- D_CFG_FILE_HAS_POSIX and D_CFG_FILE_HAS_WIN32 above all -- are read as
// they stand.
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
//   And a gate names its platforms through the families in 1.9.0, never
// through D_ENV_PLATFORM_*: env names one platform per build, and iOS, the
// BSDs and Android each fall outside the one a gate would think to name --
// which is how iOS and Android came to build every module on the ISO C
// backend, and how two gates came to test a D_ENV_PLATFORM_BSD that env has
// never defined.

// 1.9.0
// D_INTERNAL_FILE_OS_LINUX
//   constant: 1 on Linux, glibc's or musl's. Android is not Linux here: it is
// a family of its own.
#if ( (defined(D_ENV_OS_ID)) &&                                                \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_LINUX) )
    #define D_INTERNAL_FILE_OS_LINUX 1
#else
    #define D_INTERNAL_FILE_OS_LINUX 0
#endif

// D_INTERNAL_FILE_OS_ANDROID
//   constant: 1 on Android, whose bionic libc has the POSIX file API but not
// every Linux extension at every API level, so it is given none of them.
#if ( (defined(D_ENV_OS_ID)) &&                                                \
      (D_ENV_OS_ID == D_ENV_OS_FLAG_ANDROID) )
    #define D_INTERNAL_FILE_OS_ANDROID 1
#else
    #define D_INTERNAL_FILE_OS_ANDROID 0
#endif

// D_INTERNAL_FILE_OS_DARWIN
//   constant: 1 on Apple's systems -- macOS; iOS, with the systems env
// reports as iOS; a build told only that it is on an Apple system -- which
// share Darwin's libc. The first test keeps out a build with no OS identity,
// whose D_ENV_OS_ID would read as 0, the generic Apple flag.
#if ( (defined(D_ENV_OS_ID)) &&                                                \
      ( (D_ENV_OS_ID == D_ENV_OS_FLAG_APPLE) ||                                \
        (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) ||                                \
        (D_ENV_OS_ID == D_ENV_OS_FLAG_IOS) ) )
    #define D_INTERNAL_FILE_OS_DARWIN 1
#else
    #define D_INTERNAL_FILE_OS_DARWIN 0
#endif

// D_INTERNAL_FILE_OS_BSD
//   constant: 1 on the BSDs: FreeBSD, NetBSD, OpenBSD, DragonFly BSD and
// BSD/OS.
#if ( (defined(D_ENV_OS_ID)) &&                                                \
      (D_ENV_IS_OS_BSD(D_ENV_OS_ID)) )
    #define D_INTERNAL_FILE_OS_BSD 1
#else
    #define D_INTERNAL_FILE_OS_BSD 0
#endif

// D_INTERNAL_FILE_OS_WASI
//   constant: 1 on WASI, the WebAssembly system interface. Its libc lacks two
// of the ISO C calls c/filesys makes (1.9.18, 1.9.19), and env counts it as no
// POSIX system, so it builds on the ISO C backend. env has no identity for it
// yet, so the compiler's own __wasi__ names it here.
#if defined(__wasi__)
    #define D_INTERNAL_FILE_OS_WASI 1
#else
    #define D_INTERNAL_FILE_OS_WASI 0
#endif

// D_INTERNAL_FILE_OS_EXTENDED
//   constant: 1 when Darwin's and the BSDs' headers declare their own
// extensions -- flock, F_FULLFSYNC, st_mtimespec. They hide them from a build
// that asks for a POSIX level, as the C guide's -D_XOPEN_SOURCE=700 does;
// on Darwin _DARWIN_C_SOURCE brings them back, as Darwin's <sys/cdefs.h>
// decides it.
#if ( ( (!defined(_POSIX_C_SOURCE)) &&                                         \
        (!defined(_XOPEN_SOURCE)) ) ||                                         \
      ( (D_INTERNAL_FILE_OS_DARWIN == 1) &&                                    \
        (defined(_DARWIN_C_SOURCE)) ) )
    #define D_INTERNAL_FILE_OS_EXTENDED 1
#else
    #define D_INTERNAL_FILE_OS_EXTENDED 0
#endif

// 1.9.1
// D_CFG_FILE_HAS_POSIX
//   knob: the POSIX file API is present: on every system env counts as
// POSIX-compliant -- Linux and the other Unix systems, the BSDs, Solaris,
// macOS, Android -- and on the rest of Apple's.
#ifndef D_CFG_FILE_HAS_POSIX
    #if ( ( (defined(D_ENV_OS_ID)) &&                                          \
            (D_ENV_IS_OS_POSIX_COMPLIANT(D_ENV_OS_ID)) ) ||                    \
          (D_INTERNAL_FILE_OS_DARWIN == 1) )
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
// some embedded libcs ship fcntl locking only. Darwin and the BSDs declare it
// only where they declare their extensions (D_INTERNAL_FILE_OS_EXTENDED), so
// a build at the C guide's POSIX level locks with fcntl there; Linux and
// Android always declare it.
#ifndef D_CFG_FILE_HAS_FLOCK
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          ( (D_INTERNAL_FILE_OS_LINUX == 1)   ||                               \
            (D_INTERNAL_FILE_OS_ANDROID == 1) ||                               \
            ( (D_INTERNAL_FILE_OS_EXTENDED == 1) &&                            \
              ( (D_INTERNAL_FILE_OS_DARWIN == 1) ||                            \
                (D_INTERNAL_FILE_OS_BSD == 1) ) ) ) )
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
// the filesystem (reflink on btrfs/XFS) and never touches user space. glibc
// declares it from 2.27, and only under _GNU_SOURCE, which a Linux build
// gives file_ops.c (its banner; the C guide's rule). Without either the call
// is not there to make, and a copy takes the portable loop.
#ifndef D_CFG_FILE_HAS_COPY_FILE_RANGE
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (D_INTERNAL_FILE_OS_LINUX == 1) &&                                   \
          (defined(_GNU_SOURCE)) &&                                            \
          (defined(__GLIBC__)) &&                                              \
          (defined(__GLIBC_MINOR__)) &&                                        \
          ( (__GLIBC__ > 2) ||                                                 \
            ( (__GLIBC__ == 2) &&                                              \
              (__GLIBC_MINOR__ >= 27) ) ) )
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
    #if ( (defined(D_ENV_OS_ID)) &&                                          \
          (D_ENV_OS_ID == D_ENV_OS_FLAG_MACOS) )
        #define D_CFG_FILE_HAS_FCOPYFILE 1
    #else
        #define D_CFG_FILE_HAS_FCOPYFILE 0
    #endif
#endif  // D_CFG_FILE_HAS_FCOPYFILE

// 1.9.13
// D_CFG_FILE_HAS_STATX
//   knob: Linux statx(2) -- one syscall for metadata that stat() reports in
// pieces, including true creation time. glibc declares it from 2.28 under
// _GNU_SOURCE, which a Linux build gives file_stat.c (its banner);
// cfg_file_stat.h asks for the declaration itself before using it.
#ifndef D_CFG_FILE_HAS_STATX
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (D_INTERNAL_FILE_OS_LINUX == 1) &&                                   \
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
          (D_INTERNAL_FILE_OS_LINUX == 1) )
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
          (D_INTERNAL_FILE_OS_LINUX == 1) )
        #define D_CFG_FILE_HAS_FALLOCATE 1
    #else
        #define D_CFG_FILE_HAS_FALLOCATE 0
    #endif
#endif  // D_CFG_FILE_HAS_FALLOCATE

// 1.9.16
// D_CFG_FILE_HAS_STAT_NSEC
//   knob: the platform reports sub-second timestamps, and by which spelling.
// POSIX.1-2008 named the member st_mtim (a struct timespec); the BSDs and
// macOS got there first with st_mtimespec, and Darwin, built at a POSIX level
// without _DARWIN_C_SOURCE, spells it st_mtimensec, a long beside the
// seconds; older hosts have none and report whole seconds only.
//   This is what env CAN do about the timestamp problem: it cannot make a
// field named st_mtime reachable (see the note on d_stat_t in file_common.h),
// but it can tell file_stat.c which member to READ when filling
// d_stat_t.st_modified. Detection here, spelling in the module.
#ifndef D_CFG_FILE_HAS_STAT_NSEC
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                               \
          (D_INTERNAL_FILE_OS_DARWIN == 1) &&                                  \
          (D_INTERNAL_FILE_OS_EXTENDED == 1) )
        #define D_CFG_FILE_HAS_STAT_NSEC 2   // st_mtimespec.tv_nsec
    #elif ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                             \
            (D_INTERNAL_FILE_OS_DARWIN == 1) )
        #define D_CFG_FILE_HAS_STAT_NSEC 4   // st_mtimensec
    #elif ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                             \
            ( (D_INTERNAL_FILE_OS_LINUX == 1)   ||                             \
              (D_INTERNAL_FILE_OS_ANDROID == 1) ||                             \
              (D_INTERNAL_FILE_OS_BSD == 1) ) &&                               \
            ( ( (defined(_POSIX_C_SOURCE)) &&                                  \
                (_POSIX_C_SOURCE >= 200809L) ) ||                              \
              ( (defined(_XOPEN_SOURCE)) &&                                    \
                (_XOPEN_SOURCE >= 700) ) ||                                    \
              ( (D_INTERNAL_FILE_OS_BSD == 1) &&                               \
                (D_INTERNAL_FILE_OS_EXTENDED == 1) ) ) )
        #define D_CFG_FILE_HAS_STAT_NSEC 1   // st_mtim.tv_nsec
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
//   st_birthtime is <sys/stat.h>'s, so the Apple and BSD terms need the POSIX
// API, as 1.9's invariant says.
#ifndef D_CFG_FILE_HAS_BIRTHTIME
    #if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) ||                               \
          ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                             \
            ( (D_INTERNAL_FILE_OS_DARWIN == 1) ||                              \
              (D_INTERNAL_FILE_OS_BSD == 1) ) ) )
        #define D_CFG_FILE_HAS_BIRTHTIME 1
    #elif D_CFG_IS_ON(D_CFG_FILE_HAS_STATX)
        #define D_CFG_FILE_HAS_BIRTHTIME 1
    #else
        #define D_CFG_FILE_HAS_BIRTHTIME 0
    #endif
#endif  // D_CFG_FILE_HAS_BIRTHTIME

// 1.9.18
// D_CFG_FILE_HAS_TMPFILE
//   knob: ISO C's tmpfile is available. Every hosted C library has it but
// WASI's, which declares it only to deprecate it: a WASI program reaches no
// directory its host has not granted, so there is no temporary directory for
// tmpfile to use. Without it d_file_temp_stream fails with ENOSYS.
#ifndef D_CFG_FILE_HAS_TMPFILE
    #if (D_INTERNAL_FILE_OS_WASI == 1)
        #define D_CFG_FILE_HAS_TMPFILE 0
    #else
        #define D_CFG_FILE_HAS_TMPFILE 1
    #endif
#endif  // D_CFG_FILE_HAS_TMPFILE

// 1.9.19
// D_CFG_FILE_HAS_CLOCK
//   knob: ISO C's clock, the processor time used, is available. WASI has no
// process clocks: its libc emulates clock on the wall clock only for a build
// that defines _WASI_EMULATED_PROCESS_CLOCKS and links
// -lwasi-emulated-process-clocks, and otherwise declares it only to deprecate
// it. Only the seed of the temporary names reads it, and does without.
#ifndef D_CFG_FILE_HAS_CLOCK
    #if ( (D_INTERNAL_FILE_OS_WASI == 1) &&                                    \
          (!defined(_WASI_EMULATED_PROCESS_CLOCKS)) )
        #define D_CFG_FILE_HAS_CLOCK 0
    #else
        #define D_CFG_FILE_HAS_CLOCK 1
    #endif
#endif  // D_CFG_FILE_HAS_CLOCK

// 1.10   Width
//------------------------------------------------------------------------------
//   Selection model. Every declaration that holds a file offset, a size, a
// timestamp, an inode number or a capacity comes in two widths, each with a
// fixed layout: a 32-bit twin (d_stat32_t, d_file_stat32) and a 64-bit one
// (d_stat64_t, d_file_stat64). The unsuffixed name -- d_stat_t, d_file_stat --
// is an alias of one of them, and this chooses which. Both twins are declared
// whatever it says, so code that needs one width names that twin.

// 1.10.1
// D_CFG_FILE_WIDTH_AUTO
//   constant: the width that suits the build. 64 where 64-bit arithmetic is
// native -- a 64-bit architecture -- or where the build asked for large files
// by setting _FILE_OFFSET_BITS to 64; 32 otherwise, and always 32 below the
// 64-bit floor (3.1.16), where the 64-bit twins do not exist.
#define D_CFG_FILE_WIDTH_AUTO 0

// 1.10.2
// D_CFG_FILE_WIDTH
//   knob: the width of the unsuffixed names -- D_CFG_FILE_WIDTH_AUTO, 32 or
// 64. Pin 32 to keep a 64-bit build's layouts as narrow as a 32-bit one's, or
// 64 to reach files past 2 GiB from a 32-bit build; 64 below the 64-bit floor
// is a build error.
#ifndef D_CFG_FILE_WIDTH
    #define D_CFG_FILE_WIDTH D_CFG_FILE_WIDTH_AUTO
#endif  // D_CFG_FILE_WIDTH

#if !D_CFG_IS_INT_LITERAL(D_CFG_FILE_WIDTH)
    #error "D_CFG_FILE_WIDTH must name one of its values; a misspelled name would read as 0"
#endif


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
#if !D_CFG_IS_BOOL(D_CFG_FILE_FADVISE)
    #error "D_CFG_FILE_FADVISE must be literally 0 or 1"
#endif

// selection knob: must name a backend that exists.
#if ( (D_CFG_NORM(D_CFG_FILE_BACKEND) < D_CFG_FILE_BACKEND_AUTO) ||            \
      (D_CFG_NORM(D_CFG_FILE_BACKEND) > D_CFG_FILE_BACKEND_NATIVE) )
    #error "D_CFG_FILE_BACKEND must be one of the D_CFG_FILE_BACKEND_* values"
#endif

// selection knob: AUTO or one of the two widths.
#if ( (D_CFG_NORM(D_CFG_FILE_WIDTH) != D_CFG_FILE_WIDTH_AUTO) &&               \
      (D_CFG_NORM(D_CFG_FILE_WIDTH) != 32)                    &&               \
      (D_CFG_NORM(D_CFG_FILE_WIDTH) != 64) )
    #error "D_CFG_FILE_WIDTH must be D_CFG_FILE_WIDTH_AUTO, 32 or 64"
#endif

// value knob: severity is a closed range.
#if ( (D_CFG_NORM(D_CFG_FILE_NOTIFY_LEVEL) < 0) ||                             \
      (D_CFG_NORM(D_CFG_FILE_NOTIFY_LEVEL) > 4) )
    #error "D_CFG_FILE_NOTIFY_LEVEL must be 0..4"
#endif

// value knob: an allocation ceiling counts bytes, and 0 is none.
#if (D_CFG_NORM(D_CFG_FILE_MAX_ALLOC) < 0)
    #error "D_CFG_FILE_MAX_ALLOC must be 0 or a number of bytes"
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
// the same rule for a pinned width: 64 needs a 64-bit integer type, which ISO
// strict C++98 cannot spell on a 32-bit target, on Windows or on macOS.
#if ( (D_CFG_NORM(D_CFG_FILE_WIDTH) == 64) &&                                  \
      (!defined(INT64_MAX)) )
    #error "D_CFG_FILE_WIDTH == 64 but this build has no 64-bit integer type"
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
#elif ( (D_CFG_IS_ON(D_CFG_FILE_WIDE_PATHS)) &&                                \
        (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    // a build reading its names as UTF-8 (D_INTERNAL_FILE_WIDE, 3.1.20, whose
    // test this repeats, as it is resolved after this): Windows' 255 UTF-16
    // units, at the three bytes UTF-8 spends on one at most -- a surrogate
    // pair spends four on two -- so every name a directory holds can be read.
    // Ahead of NAME_MAX, which MinGW's <limits.h> defines as 255 bytes of the
    // code page, and only in a unit that has included it by now: the size of
    // d_name cannot hang on what a unit included first.
    #define D_INTERNAL_FILE_NAME_MAX 765
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
// (copy_file_range or fcopyfile) instead of shuttling the bytes through a
// user-space buffer. Win32 has no engine here: CopyFileEx copies between
// names, and d_file_copy works on the descriptors it has already checked.
#if ( (D_CFG_IS_ON(D_CFG_FILE_OPTIMIZE)) &&                                    \
      ( (D_CFG_IS_ON(D_CFG_FILE_HAS_COPY_FILE_RANGE)) ||                       \
        (D_CFG_IS_ON(D_CFG_FILE_HAS_FCOPYFILE)) ) )
    #define D_INTERNAL_FILE_USE_NATIVE_COPY 1
#else
    #define D_INTERNAL_FILE_USE_NATIVE_COPY 0
#endif

// 3.1.16
// D_INTERNAL_FILE_HAS_INT64
//   resolved: 1 where re_std's dstdint.h declares the 64-bit integer types,
// else 0 -- the 64-bit floor. ISO strict C++98 has none on a 32-bit target,
// on Windows or on macOS; every other build does. Below the floor the 64-bit
// twins are absent and the 32-bit ones stand alone (the owner's rulings of
// 2026.10.13: the floor gates declarations, not headers, and a 32-bit twin
// exists for each).
#if defined(INT64_MAX)
    #define D_INTERNAL_FILE_HAS_INT64 1
#else
    #define D_INTERNAL_FILE_HAS_INT64 0
#endif

// 3.1.17
// D_INTERNAL_FILE_NATURAL_WIDTH
//   resolved: the width every language level of this build agrees on, 32 or
// 64 -- what D_INTERNAL_FILE_WIDTH is wherever the 64-bit floor does not
// intervene. A pinned width is taken as it stands. AUTO is 64 on a 64-bit
// architecture, where the wider twins cost nothing; 64 on a 32-bit one built
// with _FILE_OFFSET_BITS set to 64, which has asked to reach files past 2 GiB
// and could not with a 32-bit offset; and 32 otherwise, where every 64-bit
// quantity would take two registers. Nothing here reads the language level.
#if (D_CFG_NORM(D_CFG_FILE_WIDTH) == 32)
    #define D_INTERNAL_FILE_NATURAL_WIDTH 32
#elif (D_CFG_NORM(D_CFG_FILE_WIDTH) == 64)
    #define D_INTERNAL_FILE_NATURAL_WIDTH 64
#elif (D_ENV_ARCH_IS_64BIT == 1)
    #define D_INTERNAL_FILE_NATURAL_WIDTH 64
#elif ( (defined(_FILE_OFFSET_BITS)) &&                                        \
        (_FILE_OFFSET_BITS == 64) )
    #define D_INTERNAL_FILE_NATURAL_WIDTH 64
#else
    #define D_INTERNAL_FILE_NATURAL_WIDTH 32
#endif

// 3.1.18
// D_INTERNAL_FILE_WIDTH
//   resolved: the width of the unsuffixed names at this language level, 32 or
// 64, never AUTO: the natural width (3.1.17), except 32 below the 64-bit
// floor, where no 64-bit twin exists to name. 2.1 has already refused a
// width pinned at 64 there.
#if (D_INTERNAL_FILE_HAS_INT64 == 0)
    #define D_INTERNAL_FILE_WIDTH 32
#else
    #define D_INTERNAL_FILE_WIDTH D_INTERNAL_FILE_NATURAL_WIDTH
#endif

// 3.1.19
// D_INTERNAL_FILE_HAS_TIME_BITS
//   resolved: 1 where the C library gives a 32-bit target a 64-bit time_t on
// request (_TIME_BITS set to 64: glibc 2.34 and later), else 0. The c/filesys
// sources assert the request where it can be made, so that a 64-bit twin
// reports a time past 2038 from a 32-bit process; where it cannot, such a
// time fails with EOVERFLOW, as the platform's own stat does.
#if ( (defined(__GLIBC__))         &&                                          \
      (defined(__GLIBC_MINOR__))   &&                                          \
      ( (__GLIBC__ > 2) ||                                                     \
        ( (__GLIBC__ == 2) &&                                                  \
          (__GLIBC_MINOR__ >= 34) ) ) )
    #define D_INTERNAL_FILE_HAS_TIME_BITS 1
#else
    #define D_INTERNAL_FILE_HAS_TIME_BITS 0
#endif

// 3.1.20
// D_INTERNAL_FILE_WIDE
//   resolved: 1 when this build reads its paths as UTF-8 and hands them to
// Windows' wide API (1.5.4): the knob, on a target with the Win32 API. 0 on
// every other target, the knob's setting notwithstanding.
#if ( (D_CFG_IS_ON(D_CFG_FILE_WIDE_PATHS)) &&                                  \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #define D_INTERNAL_FILE_WIDE 1
#else
    #define D_INTERNAL_FILE_WIDE 0
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

// 4.1.4
// D_FILE_WIDTH
//   query: the width of the unsuffixed names, 32 or 64: which twin d_off_t,
// d_stat_t, d_dirent_t and d_space_t name, and which every unsuffixed call
// that takes or fills one -- d_file_seek_stream, d_file_stat, d_dir_read,
// d_file_space, ... -- forwards to. Safe in #if.
#define D_FILE_WIDTH D_INTERNAL_FILE_WIDTH

// 4.1.5
// D_FILE_64_IS_AVAILABLE
//   query: 1 when this build declares the 64-bit twins -- d_off64_t,
// d_stat64_t, d_dirent64_t, d_space64_t and every call suffixed 64 -- and 0
// below the 64-bit floor, where only the 32-bit twins exist; those are
// declared on every build. Safe in #if.
#define D_FILE_64_IS_AVAILABLE D_INTERNAL_FILE_HAS_INT64

// 4.1.6
// D_FILE_WIDTH_IS_NARROWED
//   query: 1 when this language level has had to resolve D_FILE_WIDTH to 32
// where the build's other levels resolve it to 64 -- below the 64-bit floor,
// on a target that is otherwise 64 (ISO strict C++98 on Windows or macOS, or
// on a 32-bit target built for large files) -- and 0 everywhere else. The C
// names are safe either way, each twin being a symbol of its own. The C++
// layer's types that carry a width have one name at every level, so where
// this is 1 they are absent rather than narrowed: no level changes what a
// shared type is (the C++ guide). Safe in #if.
#define D_FILE_WIDTH_IS_NARROWED                                               \
    (D_INTERNAL_FILE_WIDTH != D_INTERNAL_FILE_NATURAL_WIDTH)


#endif  // DJINTERP_CONFIG_C_FILESYS_CFG_FILE_COMMON_H
