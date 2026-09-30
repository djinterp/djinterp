/*******************************************************************************
* djinterp [c]                                                     file_common.h
*
* Shared foundation for the djinterp fs subframework.
*   Every c/fs/file_*.h includes this one and nothing else of djinterp's fs
* tree, so a program that wants only one capability -- reading a file, say --
* compiles and links exactly this plus that one module.
*   It owns what genuinely crosses module boundaries: the platform includes,
* the portable types (d_off_t, d_stat_t, d_dirent_t, d_dir_t), the POSIX
* constants that a non-POSIX target lacks, and the notification hook. It
* contains no configuration logic -- every D_CFG_FILE_* knob is resolved in
* cfg_file_common.h and only read here.
*
*
* path:      /inc/djinterp/c/fs/file_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.29
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PLATFORM SUPPORT
    ----------------
    1.  MSVC deprecation warnings
         1.  _CRT_SECURE_NO_WARNINGS
    2.  Windows headers
         1.  FSCTL_GET_REPARSE_POINT
         2.  MAXIMUM_REPARSE_DATA_BUFFER_SIZE
         3.  SYMBOLIC_LINK_FLAG_DIRECTORY
         4.  SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
    3.  POSIX headers
2.  LANGUAGE SUPPORT
    ----------------
    1.  Linkage
    2.  Qualifiers
         1.  D_FILE_RESTRICT
         2.  D_FILE_INLINE_PRED
3.  TYPES
    -----
    1.  Portable types
         1.  d_off_t
         2.  d_stat_t
         3.  d_dirent_t
         4.  d_dir_t
4.  CONSTANTS
    ---------
    1.  Path limits and separators
         1.  D_FILE_PATH_MAX
         2.  D_FILE_NAME_MAX
         3.  D_FILE_PATH_SEP / D_FILE_PATH_SEP_STR / D_FILE_PATH_SEP_ALT
    2.  POSIX compatibility constants
         1.  DT_UNKNOWN .. DT_SOCK
         2.  F_OK / R_OK / W_OK / X_OK
         3.  S_IRUSR .. S_IXOTH
         4.  S_IRWXU / S_IRWXG / S_IRWXO
         5.  S_ISREG .. S_ISSOCK
    3.  Lock operations
         1.  D_LOCK_SH / D_LOCK_EX / D_LOCK_NB / D_LOCK_UN
    4.  Seek origins
         1.  SEEK_SET / SEEK_CUR / SEEK_END
5.  NOTIFICATIONS
    -------------
    1.  Notification types
         1.  d_file_notify_level
         2.  d_file_notice
         3.  fn_file_notify
    2.  Handler management
    3.  Diagnostic names
6.  INTERNAL SUPPORT
    ----------------
    1.  Allocation and emission
    2.  Failure reporting
         1.  D_INTERNAL_FILE_SET_ERR
         2.  D_INTERNAL_FILE_NOTIFY
         3.  D_INTERNAL_FILE_NOTIFY_PATH
         4.  D_INTERNAL_FILE_FAIL
         5.  D_INTERNAL_FILE_REQUIRE
    3.  Interrupted calls
         1.  D_INTERNAL_FILE_RETRY_EINTR
*/

#ifndef DJINTERP_C_FS_FILE_COMMON_H
#define DJINTERP_C_FS_FILE_COMMON_H 1

// std
#include <errno.h>   // errno, EINTR
#include <stdarg.h>  // va_list, for file_desc's variadic open
#include <stddef.h>  // size_t, NULL
#include <stdint.h>  // int64_t, uint8_t, uint32_t, uint64_t
#include <stdio.h>   // FILE, SEEK_SET / SEEK_CUR / SEEK_END
#include <stdlib.h>  // malloc, realloc, free: the allocator defaults
#include <string.h>  // memcpy, memset, strlen, for the fs sources
// djinterp
#include "../djinterp.h"                        // framework root
#include "../memory/dmemory.h"                  // nothing in fs uses it
#include "../../config/c/fs/cfg_file_common.h"  // fs knobs, D_INTERNAL_FILE_*


//==============================================================================
// 1.  PLATFORM SUPPORT
//==============================================================================
// The operating-system headers the fs modules call into, included once here
// behind the detected backend so that no module repeats the platform gate.


// 1.1    MSVC deprecation warnings
//------------------------------------------------------------------------------
// 1.1.1
// _CRT_SECURE_NO_WARNINGS
//   macro: silences MSVC's deprecation of the ISO calls this library wraps;
// the warning fires on the very calls the wrappers exist to make safe.
#if ( (D_ENV_CRT_MSVC) &&                                                      \
      (!defined(_CRT_SECURE_NO_WARNINGS)) )
    #define _CRT_SECURE_NO_WARNINGS 1
#endif

// 1.2    Windows headers
//------------------------------------------------------------------------------
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif  // WIN32_LEAN_AND_MEAN

    // windows
    #include <windows.h>    // HANDLE, DWORD, CreateFileA, LockFileEx
    #include <winioctl.h>   // FSCTL_GET_REPARSE_POINT
    #include <io.h>         // _open, _read, _write, _close, _commit, _dup
    #include <fcntl.h>      // O_* open flags
    #include <sys/types.h>  // the types <sys/stat.h> needs
    #include <sys/stat.h>   // _stat64 and the S_* mode bits
    #include <direct.h>     // _mkdir, _rmdir, _getcwd, _chdir
    #include <share.h>      // _SH_* sharing modes, for _fsopen

    // 1.2.1
    // FSCTL_GET_REPARSE_POINT
    //   constant: the reparse-point ioctl, for SDKs whose <winioctl.h>
    // predates it.
    #ifndef FSCTL_GET_REPARSE_POINT
        #define FSCTL_GET_REPARSE_POINT 0x000900A8
    #endif  // FSCTL_GET_REPARSE_POINT

    // 1.2.2
    // MAXIMUM_REPARSE_DATA_BUFFER_SIZE
    //   constant: the largest reparse-point buffer, for SDKs that omit it.
    #ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
        #define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
    #endif  // MAXIMUM_REPARSE_DATA_BUFFER_SIZE

    // 1.2.3
    // SYMBOLIC_LINK_FLAG_DIRECTORY
    //   constant: CreateSymbolicLink's directory-link flag, for SDKs that
    // omit it.
    #ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
        #define SYMBOLIC_LINK_FLAG_DIRECTORY 0x1
    #endif  // SYMBOLIC_LINK_FLAG_DIRECTORY

    // 1.2.4
    // SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
    //   constant: CreateSymbolicLink's Developer Mode flag, for SDKs that
    // predate Windows 10 1703.
    #ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
        #define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
    #endif  // SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#endif  // D_CFG_FILE_HAS_WIN32

// 1.3    POSIX headers
//------------------------------------------------------------------------------
#if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    // posix
    #include <unistd.h>     // read, write, close, fsync, ftruncate, readlink
    #include <fcntl.h>      // open, fcntl and the O_* / F_* constants
    #include <sys/types.h>  // off_t, mode_t
    #include <sys/stat.h>   // stat, lstat, fstat, mkdir, chmod, umask
    #include <dirent.h>     // DIR, opendir, readdir, closedir
    #include <libgen.h>     // nothing: file_path is lexical and uses none

    #if D_CFG_IS_ON(D_CFG_FILE_HAS_FLOCK)
        // posix
        #include <sys/file.h>  // flock and the LOCK_* operations
    #endif
#endif  // D_CFG_FILE_HAS_POSIX


//==============================================================================
// 2.  LANGUAGE SUPPORT
//==============================================================================


// 2.1    Linkage
//------------------------------------------------------------------------------
//   Linkage comes from the framework -- djinterp.h defines the D_EXTERN_C
// family, config/cfg_qualifiers.h gates it. An fs-local `extern "C"` block
// would be a second answer to a question the framework already answers, and
// the two would drift apart the first time one of them changed.
//   The check below turns "D_CFG_DEFINE_EXTERN_C is 0 and you forgot to
// supply your own" from a cascade of syntax errors into one sentence.
#if !defined(D_EXTERN_C_BEGIN)
    #error "D_EXTERN_C_BEGIN undefined: D_CFG_DEFINE_EXTERN_C is 0 "           \
           "and no replacement was supplied"
#endif

// 2.2    Qualifiers
//------------------------------------------------------------------------------
// 2.2.1
// D_FILE_RESTRICT
//   macro: the target's spelling of `restrict` for non-aliasing pointer
// parameters, or nothing where the qualifier is unavailable or unwanted.
// C++ has no standard restrict, hence the vendor spellings.
#if D_CFG_IS_OFF(D_CFG_FILE_USE_RESTRICT)
    #define D_FILE_RESTRICT
#elif ( (!defined(__cplusplus)) &&                                             \
        (D_ENV_LANG_IS_C99_OR_HIGHER) )
    #define D_FILE_RESTRICT restrict
#elif ( (defined(__GNUC__))  ||                                                \
        (defined(__clang__)) ||                                                \
        (defined(_MSC_VER)) )
    #define D_FILE_RESTRICT __restrict
#else
    #define D_FILE_RESTRICT
#endif

// 2.2.2
// D_FILE_INLINE_PRED
//   macro: storage class for the trivial predicates. They collapse to a
// header inline when D_CFG_FILE_INLINE_PREDICATES is on, and stay ordinary
// out-of-line functions otherwise.
#if D_CFG_IS_ON(D_CFG_FILE_INLINE_PREDICATES)
    #define D_FILE_INLINE_PRED D_STATIC_INLINE
#else
    #define D_FILE_INLINE_PRED
#endif


D_EXTERN_C_BEGIN


//==============================================================================
// 3.  TYPES
//==============================================================================


// 3.1    Portable types
//------------------------------------------------------------------------------
// 3.1.1
// d_off_t
//   type: 64-bit file offset. Windows has no off_t worth using, so it gets
// int64_t; a POSIX host whose off_t is already 64-bit keeps off_t so the
// type matches the calls it is handed to.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    typedef int64_t d_off_t;
#elif ( (defined(_FILE_OFFSET_BITS)) &&                                        \
        (_FILE_OFFSET_BITS == 64) )
    typedef off_t d_off_t;
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    typedef off_t d_off_t;
#else
    typedef int64_t d_off_t;
#endif

// 3.1.2
// d_stat_t
//   struct: portable file status. Fixed-width throughout so the layout does
// not shift between a 32- and 64-bit build of the same program.
//   The timestamps are deliberately NOT called st_mtime / st_atime /
// st_ctime. Those are not member names on a modern POSIX host, they are
// MACROS -- glibc has `#define st_mtime st_mtim.tv_sec`, macOS has
// `#define st_mtime st_mtimespec.tv_sec` -- so a struct declaring them
// expands to `uint64_t st_mtim.tv_sec;` and does not compile, and any code
// reading the field hits the same expansion. The macros only appear once
// _POSIX_C_SOURCE >= 200809L or _GNU_SOURCE is set, which is why the old
// dfile.h got away with it: the moment this subframework asks for a Linux
// fast path it must set _GNU_SOURCE, and the collision goes live.
//   st_changed is the honest name for what POSIX calls ctime. It is the
// metadata change time, NOT the creation time; only Windows (and Linux via
// statx) reports true creation, and calling the field st_ctime invited
// everyone to read it as the wrong one. So creation gets its own field and
// the two never substitute for each other -- a fabricated timestamp is a lie
// the caller cannot detect. Where a platform lacks one, it reports 0.
struct d_stat_t
{
    uint64_t st_size;           // file size in bytes
    int64_t  st_modified;       // last content modification (Unix seconds)
    int64_t  st_accessed;       // last access (Unix seconds)
    int64_t  st_changed;        // last metadata change; 0 on Windows, which
                                // has no such concept
    int64_t  st_created;        // true creation; 0 where unavailable (most
                                // POSIX filesystems). NEVER falls back to
                                // st_changed
    uint32_t st_modified_nsec;  // sub-second part of st_modified; 0 if the
    uint32_t st_accessed_nsec;  // platform reports whole seconds only, or
    uint32_t st_changed_nsec;   // D_CFG_FILE_STAT_NSEC is off
    uint32_t st_mode;           // file mode and permissions
    uint32_t st_nlink;          // number of hard links
    uint32_t st_uid;            // owner user ID
    uint32_t st_gid;            // owner group ID
    uint64_t st_dev;            // device ID
    uint64_t st_ino;            // inode number
};

// 3.1.3
// d_dirent_t
//   struct: portable directory entry. d_name is sized from the resolved
// D_CFG_FILE_NAME_MAX, so a target with longer names is a rebuild rather
// than a patch -- but note it is an ABI break for anything that embeds one.
struct d_dirent_t
{
    char     d_name[D_INTERNAL_FILE_NAME_MAX + 1];  // filename, NUL-terminated
    uint64_t d_ino;                                 // inode (0 on Windows)
    uint8_t  d_type;                                // file type (DT_*)
};

// 3.1.4
// d_dir_t
//   struct: opaque directory handle; defined by file_dir.c.
struct d_dir_t;


//==============================================================================
// 4.  CONSTANTS
//==============================================================================


// 4.1    Path limits and separators
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_PATH_MAX
//   constant: maximum path length this build will construct.
#define D_FILE_PATH_MAX D_INTERNAL_FILE_PATH_MAX

// 4.1.2
// D_FILE_NAME_MAX
//   constant: maximum single-filename length this build will construct.
#define D_FILE_NAME_MAX D_INTERNAL_FILE_NAME_MAX

// 4.1.3
// D_FILE_PATH_SEP / D_FILE_PATH_SEP_STR / D_FILE_PATH_SEP_ALT
//   constant: the target's path separator, its string form, and the
// alternate form the target also accepts on input.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_FILE_PATH_SEP      '\\'
    #define D_FILE_PATH_SEP_STR  "\\"
    #define D_FILE_PATH_SEP_ALT  '/'
#else
    // POSIX has no alternate separator; _ALT repeats _SEP so that code
    // testing both characters needs no platform branch.
    #define D_FILE_PATH_SEP      '/'
    #define D_FILE_PATH_SEP_STR  "/"
    #define D_FILE_PATH_SEP_ALT  '/'
#endif

// 4.2    POSIX compatibility constants
//------------------------------------------------------------------------------
// 4.2.1
// DT_UNKNOWN .. DT_SOCK
//   constant: d_dirent_t.d_type values, matching the POSIX numbering so a
// POSIX build can hand the kernel's value straight through.
#ifndef DT_UNKNOWN
    #define DT_UNKNOWN  0
#endif  // DT_UNKNOWN
#ifndef DT_FIFO
    #define DT_FIFO     1   // named pipe (FIFO)
#endif  // DT_FIFO
#ifndef DT_CHR
    #define DT_CHR      2   // character device
#endif  // DT_CHR
#ifndef DT_DIR
    #define DT_DIR      4   // directory
#endif  // DT_DIR
#ifndef DT_BLK
    #define DT_BLK      6   // block device
#endif  // DT_BLK
#ifndef DT_REG
    #define DT_REG      8   // regular file
#endif  // DT_REG
#ifndef DT_LNK
    #define DT_LNK      10  // symbolic link
#endif  // DT_LNK
#ifndef DT_SOCK
    #define DT_SOCK     12  // socket
#endif  // DT_SOCK

// 4.2.2
// F_OK / R_OK / W_OK / X_OK
//   constant: access mode bits for d_file_access.
#ifndef F_OK
    #define F_OK        0   // existence
#endif  // F_OK
#ifndef X_OK
    #define X_OK        1   // execute permission
#endif  // X_OK
#ifndef W_OK
    #define W_OK        2   // write permission
#endif  // W_OK
#ifndef R_OK
    #define R_OK        4   // read permission
#endif  // R_OK

// 4.2.3
// S_IRUSR .. S_IXOTH
//   constant: permission bits, defined here for targets whose <sys/stat.h>
// omits them (MSVC ships only S_IREAD/S_IWRITE).
#ifndef S_IRUSR
    #define S_IRUSR 0400    // owner read
#endif  // S_IRUSR
#ifndef S_IWUSR
    #define S_IWUSR 0200    // owner write
#endif  // S_IWUSR
#ifndef S_IXUSR
    #define S_IXUSR 0100    // owner execute
#endif  // S_IXUSR
#ifndef S_IRGRP
    #define S_IRGRP 0040    // group read
#endif  // S_IRGRP
#ifndef S_IWGRP
    #define S_IWGRP 0020    // group write
#endif  // S_IWGRP
#ifndef S_IXGRP
    #define S_IXGRP 0010    // group execute
#endif  // S_IXGRP
#ifndef S_IROTH
    #define S_IROTH 0004    // others read
#endif  // S_IROTH
#ifndef S_IWOTH
    #define S_IWOTH 0002    // others write
#endif  // S_IWOTH
#ifndef S_IXOTH
    #define S_IXOTH 0001    // others execute
#endif  // S_IXOTH

// 4.2.4
// S_IRWXU / S_IRWXG / S_IRWXO
//   constant: combined per-class permission masks.
#ifndef S_IRWXU
    #define S_IRWXU (S_IRUSR |                                                 \
                     S_IWUSR |                                                 \
                     S_IXUSR)
#endif  // S_IRWXU
#ifndef S_IRWXG
    #define S_IRWXG (S_IRGRP |                                                 \
                     S_IWGRP |                                                 \
                     S_IXGRP)
#endif  // S_IRWXG
#ifndef S_IRWXO
    #define S_IRWXO (S_IROTH |                                                 \
                     S_IWOTH |                                                 \
                     S_IXOTH)
#endif  // S_IRWXO

// 4.2.5
// S_ISREG .. S_ISSOCK
//   macro: file type tests against a d_stat_t.st_mode value.
#ifndef S_ISREG
    #define S_ISREG(m)  (((m) & 0170000) == 0100000)
#endif  // S_ISREG
#ifndef S_ISDIR
    #define S_ISDIR(m)  (((m) & 0170000) == 0040000)
#endif  // S_ISDIR
#ifndef S_ISLNK
    #define S_ISLNK(m)  (((m) & 0170000) == 0120000)
#endif  // S_ISLNK
#ifndef S_ISCHR
    #define S_ISCHR(m)  (((m) & 0170000) == 0020000)
#endif  // S_ISCHR
#ifndef S_ISBLK
    #define S_ISBLK(m)  (((m) & 0170000) == 0060000)
#endif  // S_ISBLK
#ifndef S_ISFIFO
    #define S_ISFIFO(m) (((m) & 0170000) == 0010000)
#endif  // S_ISFIFO
#ifndef S_ISSOCK
    #define S_ISSOCK(m) (((m) & 0170000) == 0140000)
#endif  // S_ISSOCK

// 4.3    Lock operations
//------------------------------------------------------------------------------
// 4.3.1
// D_LOCK_SH / D_LOCK_EX / D_LOCK_NB / D_LOCK_UN
//   constant: d_file_lock_fd operations. Deliberately djinterp's own numbering
// rather than LOCK_*, which is absent on Windows and differs on Solaris.
#define D_LOCK_SH   1   // shared lock
#define D_LOCK_EX   2   // exclusive lock
#define D_LOCK_NB   4   // non-blocking
#define D_LOCK_UN   8   // unlock

// 4.4    Seek origins
//------------------------------------------------------------------------------
// 4.4.1
// SEEK_SET / SEEK_CUR / SEEK_END
//   constant: seek origins, for the rare target lacking them.
#ifndef SEEK_SET
    #define SEEK_SET 0
#endif  // SEEK_SET
#ifndef SEEK_CUR
    #define SEEK_CUR 1
#endif  // SEEK_CUR
#ifndef SEEK_END
    #define SEEK_END 2
#endif  // SEEK_END


//==============================================================================
// 5.  NOTIFICATIONS
//==============================================================================
// An optional, process-wide hook through which every fs module reports
// failures and notable decisions. The severities compiled in are fixed by
// D_CFG_FILE_NOTIFY_LEVEL; a notice filtered out at build time costs nothing.


// 5.1    Notification types
//------------------------------------------------------------------------------
// 5.1.1
// d_file_notify_level
//   enum: severity of a notification. The values match
// D_CFG_FILE_NOTIFY_LEVEL, so the compile-time ceiling and the runtime
// record speak the same language.
enum d_file_notify_level
{
    D_FILE_NOTIFY_NONE  = 0,
    D_FILE_NOTIFY_ERROR = 1,    // the call failed
    D_FILE_NOTIFY_WARN  = 2,    // the call succeeded, but degraded
    D_FILE_NOTIFY_INFO  = 3,    // a notable decision (fast path declined)
    D_FILE_NOTIFY_TRACE = 4     // per-call entry/exit
};

// 5.1.2
// d_file_notice
//   struct: one notification record. Every string it carries is a literal
// with static storage duration -- the handler may read them but must copy
// anything it intends to keep past the callback.
struct d_file_notice
{
    int         level;      // one of enum d_file_notify_level
    int         error;      // errno-style code; 0 when not applicable
    const char* function;   // originating djinterp function
    const char* path;       // path involved; NULL unless NOTIFY_PATHS is on
    const char* message;    // short static description
};

// 5.1.3
// fn_file_notify
//   type: notification handler. Called on the thread that raised the notice,
// possibly with errno live, so a handler must not clobber errno and must not
// call back into the fs module that raised it.
typedef void (*fn_file_notify)(const struct d_file_notice* _notice,
                               void*                       _context);

// 5.2    Handler management
//------------------------------------------------------------------------------
/**
 * @brief Installs the notification handler for the whole fs subframework.
 *
 * @note The handler is a plain pointer, not an atomic: install it once,
 *       during startup, before any thread uses an fs module. When this build
 *       compiled notifications out, the call is accepted and does nothing.
 *
 * @param[in] _handler  the handler to install, or `NULL` to remove the
 *                      current one and silence notifications.
 * @param[in] _context  an opaque cookie passed back to `_handler` with every
 *                      notice; stored, not copied, so it must outlive the
 *                      handler.
 */
void           d_file_notify_set_handler(fn_file_notify _handler,
                                         void*          _context);
/**
 * @brief Retrieves the active notification handler, so a caller can chain
 *        onto it rather than displace it.
 *
 * @param[out] _context  receives the cookie installed with the handler; may
 *                       be `NULL`.
 * @return the active handler, or `NULL` when none is installed or this build
 *         compiled notifications out.
 */
fn_file_notify d_file_notify_get_handler(void** _context);
/**
 * @brief djinterp's built-in handler: writes one line per notice to the
 *        stream named by D_CFG_FILE_NOTIFY_STREAM, preserving errno.
 *
 * @note It is installed only when D_CFG_FILE_NOTIFY_DEFAULT_HANDLER is 1, but
 *       is always available to install by hand.
 *
 * @param[in] _notice   the record to report; `NULL` is ignored.
 * @param[in] _context  unused.
 */
void           d_file_notify_default_handler(
                   const struct d_file_notice* _notice,
                   void*                       _context);

// 5.3    Diagnostic names
//------------------------------------------------------------------------------
// display names -- static, NUL-terminated and never NULL: "unknown" for a
// level outside enum d_file_notify_level, and "native", "posix" or "stdc" for
// the backend this build resolved to
const char* d_file_notify_level_name(int _level);
const char* d_file_backend_name(void);


//==============================================================================
// 6.  INTERNAL SUPPORT
//==============================================================================
// Shared by the fs modules and not API. They live in the header because every
// module needs them, and because the macros must expand at the call site to
// disappear when the corresponding knob is off.


// 6.1    Allocation and emission
//------------------------------------------------------------------------------
/**
 * @brief Allocates through the configured allocator, refusing a request past
 *        D_CFG_FILE_MAX_ALLOC. Every fs allocation funnels through here.
 *
 * @param[in] _size  bytes to allocate.
 * @return the block, or `NULL` on failure or refusal, with errno set to
 *         ENOMEM when this build reports through errno.
 * @post A non-`NULL` result is released with d_internal_file_free.
 */
void* d_internal_file_alloc(size_t _size);
/**
 * @brief Resizes a block through the configured allocator, under the same
 *        ceiling as d_internal_file_alloc.
 *
 * @warning On failure the original block is still valid and still the
 *          caller's to free, exactly as with realloc: never assign the result
 *          over the only pointer to the block.
 *
 * @param[in] _ptr   the block to resize; `NULL` behaves as an allocation.
 * @param[in] _size  the new size, in bytes.
 * @return the resized block, or `NULL` on failure or refusal with the
 *         original block untouched.
 */
void* d_internal_file_realloc(void*  _ptr,
                              size_t _size);
/**
 * @brief Releases a block obtained from d_internal_file_alloc or
 *        d_internal_file_realloc.
 *
 * @param[in] _ptr  the block to release; may be `NULL`.
 * @post `_ptr` is invalid.
 */
void  d_internal_file_free(void* _ptr);
/**
 * @brief Dispatches one notice to the active handler: the out-of-line half of
 *        D_INTERNAL_FILE_NOTIFY, declared here so the macro has something to
 *        call.
 *
 * @param[in] _level     one of enum d_file_notify_level.
 * @param[in] _error     errno-style code, or 0 when not applicable.
 * @param[in] _function  originating function name; a static literal.
 * @param[in] _path      path involved, or `NULL`.
 * @param[in] _message   short description; a static literal.
 */
void  d_internal_file_notify_emit(int         _level,
                                  int         _error,
                                  const char* _function,
                                  const char* _path,
                                  const char* _message);

// 6.2    Failure reporting
//------------------------------------------------------------------------------
// 6.2.1
// D_INTERNAL_FILE_SET_ERR
//   macro: record a djinterp-level failure code, or nothing when this build
// does not report through errno.
#if (D_INTERNAL_FILE_SET_ERRNO == 1)
    #define D_INTERNAL_FILE_SET_ERR(_code) (errno = (_code))
#else
    #define D_INTERNAL_FILE_SET_ERR(_code) ((void)0)
#endif

// 6.2.2
// D_INTERNAL_FILE_NOTIFY
//   macro: raise a notice if this build compiles that severity in. The level
// test is a preprocessor test, so a filtered-out notice costs nothing at all
// -- not a branch, and not the string it would have carried.
#if (D_INTERNAL_FILE_NOTIFY_LEVEL > 0)
    #define D_INTERNAL_FILE_NOTIFY(_level, _error, _fn, _path, _msg)           \
        do                                                                     \
        {                                                                      \
            if ((_level) <= D_INTERNAL_FILE_NOTIFY_LEVEL)                      \
            {                                                                  \
                d_internal_file_notify_emit((_level),                          \
                                            (_error),                          \
                                            (_fn),                             \
                                            (_path),                           \
                                            (_msg));                           \
            }                                                                  \
        }                                                                      \
        while (0)
#else
    #define D_INTERNAL_FILE_NOTIFY(_level, _error, _fn, _path, _msg)           \
        ((void)0)
#endif

// 6.2.3
// D_INTERNAL_FILE_NOTIFY_PATH
//   macro: the path to attach to a notice -- the real one, or NULL when this
// build must not leak paths to the handler.
#if D_CFG_IS_ON(D_CFG_FILE_NOTIFY_PATHS)
    #define D_INTERNAL_FILE_NOTIFY_PATH(_path) (_path)
#else
    #define D_INTERNAL_FILE_NOTIFY_PATH(_path) (NULL)
#endif

// 6.2.4
// D_INTERNAL_FILE_FAIL
//   macro: the single failure exit used by every parameter check -- set
// errno, raise an error notice, and hand back the caller's error value. It
// is one macro so that a build with validation off, errno off and
// notifications off leaves literally nothing behind.
#define D_INTERNAL_FILE_FAIL(_code, _fn, _path, _msg, _ret)                    \
    do                                                                         \
    {                                                                          \
        D_INTERNAL_FILE_SET_ERR(_code);                                        \
        D_INTERNAL_FILE_NOTIFY(D_FILE_NOTIFY_ERROR,                            \
                               (_code),                                        \
                               (_fn),                                          \
                               D_INTERNAL_FILE_NOTIFY_PATH(_path),             \
                               (_msg));                                        \
        return (_ret);                                                         \
    }                                                                          \
    while (0)

// 6.2.5
// D_INTERNAL_FILE_REQUIRE
//   macro: the parameter guard. Expands to nothing when validation is off,
// which is the whole point of the knob -- the check is not merely predicted
// away, it is never emitted.
#if (D_INTERNAL_FILE_VALIDATE == 1)
    #define D_INTERNAL_FILE_REQUIRE(_cond, _code, _fn, _path, _msg, _ret)      \
        do                                                                     \
        {                                                                      \
            if (!(_cond))                                                      \
            {                                                                  \
                D_INTERNAL_FILE_FAIL((_code),                                  \
                                     (_fn),                                    \
                                     (_path),                                  \
                                     (_msg),                                   \
                                     (_ret));                                  \
            }                                                                  \
        }                                                                      \
        while (0)
#else
    #define D_INTERNAL_FILE_REQUIRE(_cond, _code, _fn, _path, _msg, _ret)      \
        ((void)0)
#endif

// 6.3    Interrupted calls
//------------------------------------------------------------------------------
// 6.3.1
// D_INTERNAL_FILE_RETRY_EINTR
//   macro: run an expression, repeating it for as long as the kernel keeps
// interrupting it. A signal arriving mid-read is not a read error, and a
// caller that has to know the difference is a caller writing this loop by
// hand at every call site.
#if D_CFG_IS_ON(D_CFG_FILE_EINTR_RETRY)
    #define D_INTERNAL_FILE_RETRY_EINTR(_result, _expr)                        \
        do                                                                     \
        {                                                                      \
            do                                                                 \
            {                                                                  \
                (_result) = (_expr);                                           \
            }                                                                  \
            while ( ((_result) < 0) &&                                         \
                    (errno == EINTR) );                                        \
        }                                                                      \
        while (0)
#else
    #define D_INTERNAL_FILE_RETRY_EINTR(_result, _expr)                        \
        do                                                                     \
        {                                                                      \
            (_result) = (_expr);                                               \
        }                                                                      \
        while (0)
#endif


D_EXTERN_C_END


#endif  // DJINTERP_C_FS_FILE_COMMON_H
