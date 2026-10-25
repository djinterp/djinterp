/*******************************************************************************
* djinterp [c]                                                     file_common.h
*
* Shared foundation for the djinterp filesys subframework.
*   Every c/filesys/file_*.h includes this one and nothing else of djinterp's
* filesys tree, so a program that wants only one capability -- reading a file,
* say -- compiles and links exactly this plus that one module.
*   It owns what genuinely crosses module boundaries: the platform includes,
* the portable types (offsets, file status, directory entries, d_dir_t), the
* POSIX constants that a non-POSIX target lacks, and the notification hook. It
* contains no configuration logic -- every D_CFG_FILE_* knob is resolved in
* cfg_file_common.h and only read here.
*   TWO WIDTHS. Whatever holds an offset, a size, a timestamp, an inode number
* or a capacity is declared twice, each with a fixed layout: a 32-bit twin
* (d_stat32_t, d_file_stat32) and a 64-bit one (d_stat64_t, d_file_stat64).
* The unsuffixed name (d_stat_t, d_file_stat) is an alias of the twin at
* D_FILE_WIDTH: 64 on a 64-bit target or with _FILE_OFFSET_BITS at 64, 32
* otherwise (D_CFG_FILE_WIDTH). Every module follows the same rules:
*     - The 64-bit twins need a 64-bit integer type, and are absent below the
*       64-bit floor (D_FILE_64_IS_AVAILABLE). The 32-bit twins exist on every
*       build, so the unsuffixed names do too.
*     - A 32-bit twin takes its arguments as they come and hands back what
*       its 64-bit twin would, or fails with EOVERFLOW when a value does not
*       fit its 32-bit type -- a large size or offset, a time past 2038, a
*       wide inode number. It never truncates. The one change of unit is
*       capacity, which the 32-bit twin counts in KiB (file_space.h).
*     - The library calls the 64-bit twins internally, so a 32-bit twin's only
*       limit is what it can hand back: a file past 2 GiB can still be read,
*       written and seeked through it.
*     - On a 32-bit POSIX host the library builds with the large-file
*       interface and, where the C library offers it, 64-bit time
*       (-D_FILE_OFFSET_BITS=64 -D_TIME_BITS=64, which its sources assert),
*       so both twins reach past 2 GiB and 2038 there too -- on a handle this
*       subframework opened. A FILE* or descriptor opened elsewhere without
*       that interface stays capped at 2 GiB by the kernel, whichever twin it
*       is handed to.
*
*
* path:      /inc/djinterp/c/filesys/file_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.21
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
3.  TYPES
    -----
    1.  Portable types
         1.  d_off32_t / d_off64_t
         2.  d_stat64_t / d_stat32_t
         3.  d_dirent64_t / d_dirent32_t
         4.  d_dir_t
    2.  Default width
         1.  d_off_t
         2.  d_stat_t
         3.  d_dirent_t
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
         6.  S_IFLNK
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
    4.  Large files
         1.  D_INTERNAL_FILE_ASSERT_LARGE_FILES
    5.  Windows support
    6.  Path encoding
         1.  d_internal_file_nchar_t
         2.  D_INTERNAL_FILE_NATIVE_MAX
*/

#ifndef DJINTERP_C_FILESYS_FILE_COMMON_H
#define DJINTERP_C_FILESYS_FILE_COMMON_H 1

// std
#include <errno.h>   // errno, EINTR
#include <stdarg.h>  // va_list, for file_desc's variadic open
#include <stddef.h>  // size_t, NULL
#include <stdio.h>   // FILE, SEEK_SET / SEEK_CUR / SEEK_END
#include <stdlib.h>  // malloc, realloc, free: the allocator defaults
#include <string.h>  // memcpy, memset, strlen, for the filesys sources
// djinterp
#include "../djinterp.h"                             // framework root
#include "../memory/dmemory.h"                       // unused by filesys
#include "../../config/c/filesys/cfg_file_common.h"  // knobs, D_INTERNAL_FILE_*
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, int64_t, uint8_t,
                                              // uint32_t, uint64_t


//==============================================================================
// 1.  PLATFORM SUPPORT
//==============================================================================
// The operating-system headers the filesys modules call into, included once
// here behind the detected backend so that no module repeats the platform gate.


// 1.1    MSVC deprecation warnings
//------------------------------------------------------------------------------
// 1.1.1
// _CRT_SECURE_NO_WARNINGS
//   macro: silences MSVC's deprecation of the ISO calls this library wraps;
// the warning fires on the very calls the wrappers exist to make safe.
#if ( (defined(D_ENV_CRT_MSVC))           &&                                   \
      (D_ENV_CRT_MSVC)                    &&                                   \
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
// family, config/cfg_qualifiers.h gates it. A filesys-local `extern "C"` block
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


D_EXTERN_C_BEGIN


//==============================================================================
// 3.  TYPES
//==============================================================================


// 3.1    Portable types
//------------------------------------------------------------------------------
// 3.1.1
// d_off32_t / d_off64_t
//   type: a file offset -- signed, since a seek may count back from the end --
// at a fixed width whatever the platform's off_t is.
typedef int32_t d_off32_t;
#if D_FILE_64_IS_AVAILABLE
    typedef int64_t d_off64_t;
#endif  // D_FILE_64_IS_AVAILABLE

// 3.1.2
// d_stat64_t / d_stat32_t
//   struct: portable file status, in two fixed layouts. d_stat64_t holds every
// field at full width; d_stat32_t is the same structure with each 64-bit
// field narrowed to 32 bits of the same signedness -- a size to 4 GiB, a time
// to 2038 -- and is filled only when every value fits.
//   The timestamps are deliberately NOT called st_mtime / st_atime /
// st_ctime. Those are not member names on a modern POSIX host, they are
// MACROS -- glibc has `#define st_mtime st_mtim.tv_sec`, macOS has
// `#define st_mtime st_mtimespec.tv_sec` -- so a struct declaring them
// expands to `uint64_t st_mtim.tv_sec;` and does not compile, and any code
// reading the field hits the same expansion. The macros only appear once
// _POSIX_C_SOURCE >= 200809L or _GNU_SOURCE is set, which is why the old
// dfile.h got away with it: the moment a build asks for a Linux fast path it
// defines _GNU_SOURCE, and the collision goes live.
//   st_changed is the honest name for what POSIX calls ctime. It is the
// metadata change time, NOT the creation time; only Windows (and Linux via
// statx) reports true creation, and calling the field st_ctime invited
// everyone to read it as the wrong one. So creation gets its own field and
// the two never substitute for each other -- a fabricated timestamp is a lie
// the caller cannot detect. Where a platform lacks one, it reports 0.
#if D_FILE_64_IS_AVAILABLE
struct d_stat64_t
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
#endif  // D_FILE_64_IS_AVAILABLE

struct d_stat32_t
{
    uint32_t st_size;           // file size in bytes, to 4 GiB
    int32_t  st_modified;       // last content modification, to 2038
    int32_t  st_accessed;       // last access, to 2038
    int32_t  st_changed;        // last metadata change; 0 on Windows
    int32_t  st_created;        // true creation; 0 where unavailable
    uint32_t st_modified_nsec;  // the sub-second parts, as in d_stat64_t
    uint32_t st_accessed_nsec;
    uint32_t st_changed_nsec;
    uint32_t st_mode;           // file mode and permissions
    uint32_t st_nlink;          // number of hard links
    uint32_t st_uid;            // owner user ID
    uint32_t st_gid;            // owner group ID
    uint32_t st_dev;            // device ID
    uint32_t st_ino;            // inode number
};

// 3.1.3
// d_dirent64_t / d_dirent32_t
//   struct: portable directory entry, in two fixed layouts that differ only in
// the inode number's width. d_name is sized from the resolved
// D_CFG_FILE_NAME_MAX, so a target with longer names is a rebuild rather
// than a patch -- but note it is an ABI break for anything that embeds one.
#if D_FILE_64_IS_AVAILABLE
struct d_dirent64_t
{
    char     d_name[D_INTERNAL_FILE_NAME_MAX + 1];  // filename, NUL-terminated
    uint64_t d_ino;                                 // inode (0 on Windows)
    uint8_t  d_type;                                // file type (DT_*)
};
#endif  // D_FILE_64_IS_AVAILABLE

struct d_dirent32_t
{
    char     d_name[D_INTERNAL_FILE_NAME_MAX + 1];  // filename, NUL-terminated
    uint32_t d_ino;                                 // inode (0 on Windows)
    uint8_t  d_type;                                // file type (DT_*)
};

// 3.1.4
// d_dir_t
//   struct: opaque directory handle, read at either width; defined by
// file_dir.c.
struct d_dir_t;

// 3.2    Default width
//------------------------------------------------------------------------------
//   The unsuffixed names, each the twin at D_FILE_WIDTH. A struct tag can be
// renamed only by a macro, and a typedef would hide that the type is a struct,
// so d_stat_t and d_dirent_t are macros; each module's calls follow suit, in
// its own section 2, because a forwarding function would be a second symbol
// with an address of its own where the alias IS the twin. These are the one
// place the subframework spells a macro in lower case: an alias must read
// exactly like the name it stands for (`struct d_stat_t st;`), as zlib's
// gzseek does for gzseek64 and Win32's CreateFile for CreateFileW. The offset
// is a scalar and gets a typedef.

// 3.2.1
// d_off_t
//   type: d_off64_t where D_FILE_WIDTH is 64, else d_off32_t.
#if (D_FILE_WIDTH == 64)
    typedef d_off64_t d_off_t;
#else
    typedef d_off32_t d_off_t;
#endif  // D_FILE_WIDTH == 64

// 3.2.2
// d_stat_t
//   alias: the struct tag of the status twin at D_FILE_WIDTH, so that
// `struct d_stat_t` is `struct d_stat64_t` or `struct d_stat32_t`.
#if (D_FILE_WIDTH == 64)
    #define d_stat_t d_stat64_t
#else
    #define d_stat_t d_stat32_t
#endif  // D_FILE_WIDTH == 64

// 3.2.3
// d_dirent_t
//   alias: the struct tag of the directory-entry twin at D_FILE_WIDTH, so
// that `struct d_dirent_t` is `struct d_dirent64_t` or `struct d_dirent32_t`.
#if (D_FILE_WIDTH == 64)
    #define d_dirent_t d_dirent64_t
#else
    #define d_dirent_t d_dirent32_t
#endif  // D_FILE_WIDTH == 64


//==============================================================================
// 4.  CONSTANTS
//==============================================================================


// 4.1    Path limits and separators
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_PATH_MAX
//   constant: the longest path this build constructs, in bytes. A build that
// hands Windows UTF-16 allows three for each unit Windows takes (780, or
// 98301 with long paths), so that every path Windows can hold fits in UTF-8.
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

// 4.2.6
// S_IFLNK
//   constant: the type bits of a symbolic link, which the Windows CRT does not
// define -- file_stat describes a link there as POSIX describes one.
#ifndef S_IFLNK
    #define S_IFLNK 0120000
#endif  // S_IFLNK

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
// An optional, process-wide hook through which every filesys module reports
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
// call back into the filesys module that raised it.
typedef void (*fn_file_notify)(const struct d_file_notice* _notice,
                               void*                       _context);

// 5.2    Handler management
//------------------------------------------------------------------------------
/**
 * @brief Installs the notification handler for the whole filesys subframework.
 *
 * @note The handler is a plain pointer, not an atomic: install it once,
 *       during startup, before any thread uses a filesys module.
 *       When this build compiled notifications out, the call is accepted
 *       and does nothing.
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
 * @note One line per notice,
 *       `[djinterp/filesys] level: function: message: 'path' (errno=N)`,
 *       where the message and the path appear only when the notice carries
 *       them.
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
// Shared by the filesys modules and not API. They live in the header because
// every module needs them, and because the macros must expand at the call site
// to disappear when the corresponding knob is off.


// 6.1    Allocation and emission
//------------------------------------------------------------------------------
/**
 * @brief Allocates through the configured allocator, refusing a request past
 *        D_CFG_FILE_MAX_ALLOC. Every filesys allocation funnels through here.
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

// 6.4    Large files
//------------------------------------------------------------------------------
// 6.4.1
// D_INTERNAL_FILE_ASSERT_LARGE_FILES
//   macro: a file-scope assertion, in each source whose calls must reach past
// 2 GiB or 2038, that the build asked the C library for its large-file
// interface, and for 64-bit time where the library offers it
// (D_INTERNAL_FILE_HAS_TIME_BITS). A 32-bit POSIX host has a 32-bit off_t
// without the first, and there a 64-bit twin would stop at 2 GiB and a handle
// opened by this subframework could not grow past it; without the second its
// stat fails on a time past 2038. The build asks with -D_FILE_OFFSET_BITS=64
// and -D_TIME_BITS=64, as it asks for POSIX with -D_XOPEN_SOURCE=700: a
// feature-test macro comes from the build, never from a source (the C guide's
// rule). Off POSIX it asserts nothing: Windows' calls are 64-bit by name, and
// a build with neither API reaches a file only through ISO C's long offsets,
// which no build flag widens.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) &&                                   \
      (D_INTERNAL_FILE_HAS_TIME_BITS == 1) )
    #define D_INTERNAL_FILE_ASSERT_LARGE_FILES                                 \
        D_STATIC_ASSERT( (sizeof(off_t) >= 8) &&                               \
                         (sizeof(time_t) >= 8),                                \
                         "c/filesys builds with -D_FILE_OFFSET_BITS=64 "       \
                         "-D_TIME_BITS=64 here")
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
    #define D_INTERNAL_FILE_ASSERT_LARGE_FILES                                 \
        D_STATIC_ASSERT(sizeof(off_t) >= 8,                                    \
                        "c/filesys builds with -D_FILE_OFFSET_BITS=64 here")
#else
    #define D_INTERNAL_FILE_ASSERT_LARGE_FILES                                 \
        D_STATIC_ASSERT(1, "off POSIX there is no off_t to widen")
#endif

// 6.5    Windows support
//------------------------------------------------------------------------------
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
/**
 * @brief Translates a Win32 error into the errno a POSIX call failing for the
 *        same reason sets, for the calls that report through GetLastError.
 *
 * @param[in] _error  a GetLastError() value.
 * @return the matching errno code; EIO for a reason POSIX has no name for.
 */
int   d_internal_file_errno_win32(DWORD _error);
/**
 * @brief Reports whether a reparse tag makes a link: a symbolic link or a
 *        junction, the two kinds that name another path.
 *
 * @note Every other reparse point -- a cloud placeholder, a deduplicated file,
 *       an app alias -- is the file or directory it appears to be, and
 *       c/filesys describes it as one.
 *
 * @param[in] _tag  a reparse tag, as FILE_ATTRIBUTE_TAG_INFO or a find
 *                  record's dwReserved0 reports it.
 * @return non-zero for a link's tag.
 */
int   d_internal_file_tag_is_link_win32(DWORD _tag);
/**
 * @brief Reports whether a path is itself a link (a symbolic link or a
 *        junction), without following it.
 *
 * @note A reparse point whose tag cannot be read counts as a link: mistaking a
 *       link for the directory it names is the error that costs data.
 *
 * @param[in]  _path  the path to examine.
 * @param[out] _info  receives the link's own information when it is one, or
 *                    zeroes where that cannot be read; may be `NULL`.
 * @return non-zero for a link; 0 for anything else, and when the path cannot
 *         be opened.
 */
int   d_internal_file_is_link_win32(const char*                 _path,
                                    BY_HANDLE_FILE_INFORMATION* _info);
#endif  // D_CFG_FILE_HAS_WIN32

// 6.6    Path encoding
//------------------------------------------------------------------------------
//   A path is UTF-8 on every target when D_CFG_FILE_WIDE_PATHS is set: POSIX
// hands its bytes to the kernel as they are, and Windows, whose wide API
// takes UTF-16, gets them converted here (D_INTERNAL_FILE_WIDE). Without it a
// path goes to every platform as it is, which on Windows means the ANSI API
// and the active code page. A module converts a path on its way in with
// d_internal_file_to_native and a name on its way out with
// d_internal_file_from_native, and calls the platform's A or W function by
// D_INTERNAL_FILE_WIDE.

// 6.6.1
// d_internal_file_nchar_t
//   type: the character the platform's path calls take -- wchar_t where this
// build hands Windows UTF-16, char everywhere else.
#if (D_INTERNAL_FILE_WIDE == 1)
    typedef wchar_t d_internal_file_nchar_t;
#else
    typedef char    d_internal_file_nchar_t;
#endif

// 6.6.2
// D_INTERNAL_FILE_NATIVE_MAX
//   constant: the characters a buffer for d_internal_file_to_native needs for
// a path. A UTF-8 path never takes more UTF-16 units than bytes, so a path
// that fits D_FILE_PATH_MAX bytes fits D_FILE_PATH_MAX units; where a path is
// handed on as it is, the buffer goes unused and is one character.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_FILE_NATIVE_MAX D_INTERNAL_FILE_NATIVE_PATH_MAX
#else
    #define D_INTERNAL_FILE_NATIVE_MAX 1
#endif

/**
 * @brief Gives text that is no path -- a mode -- the form the platform's
 *        calls take: converted from UTF-8 to UTF-16 into `_buf` where this
 *        build hands Windows UTF-16, and `_text` itself everywhere else.
 *
 * @param[in]  _text   NUL-terminated text; UTF-8 where it is converted.
 * @param[out] _buf    receives the conversion. Unused, and untouched, where
 *                     the text is handed on as it is.
 * @param[in]  _count  capacity of `_buf`, in characters.
 * @return the text to hand the platform, or `NULL` with errno set: EILSEQ
 *         for text that is not UTF-8, ENAMETOOLONG for text that does not
 *         fit `_count` characters with its terminator.
 */
const d_internal_file_nchar_t*
      d_internal_file_to_native_text(const char*              _text,
                                     d_internal_file_nchar_t* _buf,
                                     size_t                   _count);
/**
 * @brief Gives a path the form the platform's calls take: the text as
 *        d_internal_file_to_native_text gives it, and where the build takes
 *        long paths (D_CFG_FILE_LONG_PATHS), one that MAX_PATH would refuse
 *        made absolute and given the long-path prefix.
 *
 * @param[in]  _text   NUL-terminated path; UTF-8 where it is converted.
 * @param[out] _buf    receives the conversion; D_INTERNAL_FILE_NATIVE_MAX
 *                     characters hold any path. Unused, and untouched, where
 *                     the text is handed on as it is.
 * @param[in]  _count  capacity of `_buf`, in characters.
 * @return the path to hand the platform, or `NULL` with errno set: EILSEQ
 *         for text that is not UTF-8, ENAMETOOLONG for a path that does not
 *         fit `_count` characters with its terminator and any prefix, ENOMEM,
 *         or the reason Windows gives for a path it cannot make absolute.
 */
const d_internal_file_nchar_t*
      d_internal_file_to_native(const char*              _text,
                                d_internal_file_nchar_t* _buf,
                                size_t                   _count);
// D_INTERNAL_FILE_NATIVE_ON_HEAP
//   constant: 1 where a path's room in the platform's text is on the heap --
// a build that hands Windows UTF-16 with long paths, where the room is 32767
// characters and some -- and 0 where it is an array on the stack.
#if ( (D_INTERNAL_FILE_WIDE == 1) &&                                           \
      (D_CFG_IS_ON(D_CFG_FILE_LONG_PATHS)) )
    #define D_INTERNAL_FILE_NATIVE_ON_HEAP 1
#else
    #define D_INTERNAL_FILE_NATIVE_ON_HEAP 0
#endif  // D_INTERNAL_FILE_WIDE == 1 && D_CFG_FILE_LONG_PATHS

// D_INTERNAL_FILE_NATIVE_ROOM
//   constant: the characters a path's room holds: D_INTERNAL_FILE_NATIVE_MAX,
// and a few more for an answer the platform writes there that runs past its
// own limit, as GetTempPathW's MAX_PATH + 1 does.
#if (D_INTERNAL_FILE_WIDE == 1)
    #define D_INTERNAL_FILE_NATIVE_ROOM (D_INTERNAL_FILE_NATIVE_MAX + 8)
#else
    #define D_INTERNAL_FILE_NATIVE_ROOM 1
#endif  // D_INTERNAL_FILE_WIDE == 1

// d_internal_file_native
//   struct: room for a path in the platform's text. It is an array in the
// struct where that is small; in a long-path build it is a block on the
// heap, allocated when first asked for and returned by
// d_internal_file_native_release -- 64 KiB a path is no stack's to give.
// Zero-initialize it.
struct d_internal_file_native
{
#if (D_INTERNAL_FILE_NATIVE_ON_HEAP == 1)
    d_internal_file_nchar_t* room;
#else
    d_internal_file_nchar_t  room[D_INTERNAL_FILE_NATIVE_ROOM];
#endif  // D_INTERNAL_FILE_NATIVE_ON_HEAP == 1
};

/**
 * @brief The room a holder keeps for a path in the platform's text.
 *
 * @param[in,out] _native  the holder.
 * @return D_INTERNAL_FILE_NATIVE_ROOM characters of room, or `NULL` where
 *         its block could not be allocated, the failure reported.
 * @post In a long-path build the room is the holder's to release.
 */
d_internal_file_nchar_t*
      d_internal_file_native_room(struct d_internal_file_native* _native);
/**
 * @brief A path in the platform's text (d_internal_file_to_native), written
 *        into a holder's room.
 *
 * @param[in,out] _native  the holder.
 * @param[in]     _path    the path.
 * @return the path to hand the platform, or `NULL` with errno set, as
 *         d_internal_file_to_native sets it, or ENOMEM.
 * @post On success the holder has room to release; on failure it has none.
 */
const d_internal_file_nchar_t*
      d_internal_file_native_of(struct d_internal_file_native* _native,
                                const char*                    _path);
/**
 * @brief Returns a holder's room, where it was allocated. errno, and on
 *        Windows the last error, are what they were: a call made with the
 *        path is read after its room is gone.
 *
 * @param[in,out] _native  the holder; releasing it twice is releasing it once.
 */
void  d_internal_file_native_release(struct d_internal_file_native* _native);
// D_INTERNAL_FILE_PATH_ON_HEAP
//   constant: 1 where a buffer that builds a path in UTF-8 is kept on the
// heap -- a long-path Windows build, where D_FILE_PATH_MAX is 32767 -- and 0
// where it is an array on the stack: 260 bytes on Windows, PATH_MAX on POSIX.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) &&                                   \
      (D_CFG_IS_ON(D_CFG_FILE_LONG_PATHS)) )
    #define D_INTERNAL_FILE_PATH_ON_HEAP 1
#else
    #define D_INTERNAL_FILE_PATH_ON_HEAP 0
#endif  // D_CFG_FILE_HAS_WIN32 && D_CFG_FILE_LONG_PATHS

// D_INTERNAL_FILE_PATH_ROOM
//   constant: the bytes such a buffer holds: a path and its terminator.
#define D_INTERNAL_FILE_PATH_ROOM (D_FILE_PATH_MAX + 1)

// d_internal_file_path
//   struct: room to build a path in UTF-8: an array in the struct, or in a
// long-path build a block on the heap, allocated when first asked for and
// returned by d_internal_file_path_release. Either way it starts empty,
// every byte zero. Zero-initialize it.
struct d_internal_file_path
{
#if (D_INTERNAL_FILE_PATH_ON_HEAP == 1)
    char* room;
#else
    char  room[D_INTERNAL_FILE_PATH_ROOM];
#endif  // D_INTERNAL_FILE_PATH_ON_HEAP == 1
};

/**
 * @brief The room a holder keeps to build a path in.
 *
 * @param[in,out] _path  the holder.
 * @return D_INTERNAL_FILE_PATH_ROOM bytes, zero, or `NULL` with errno ENOMEM
 *         where the block could not be allocated, the failure reported.
 * @post In a long-path build the room is the holder's to release.
 */
char* d_internal_file_path_room(struct d_internal_file_path* _path);
/**
 * @brief Returns a holder's room, where it was allocated, leaving errno -- and
 *        on Windows the last error -- as they were.
 *
 * @param[in,out] _path  the holder; releasing it twice is releasing it once.
 */
void  d_internal_file_path_release(struct d_internal_file_path* _path);
// d_internal_file_u16_t
//   type (internal): a UTF-16 code unit: wchar_t on Windows, where it is one
// and the wide API takes it, uint16_t elsewhere, where the converters below
// are compiled for their tests alone.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    typedef wchar_t d_internal_file_u16_t;
#else
    typedef uint16_t d_internal_file_u16_t;
#endif

// D_INTERNAL_FILE_TERMINATED
//   constant (internal): the length that tells the converters below to read
// their input as far as its terminator.
#define D_INTERNAL_FILE_TERMINATED ((size_t)-1)

/**
 * @brief Converts WTF-8 to UTF-16. WTF-8 is UTF-8 in which an unpaired
 *        surrogate, which Windows allows in a name, is written as the three
 *        bytes its code point would take (ED A0 80 to ED BF BF).
 *
 * @note A pair written as two such forms is refused: WTF-8 spells a pair as
 *       one four-byte form, so that a name has exactly one spelling.
 *
 * @param[in]  _in       the text.
 * @param[in]  _in_size  its length in bytes, or D_INTERNAL_FILE_TERMINATED.
 * @param[out] _out      receives the units and a terminator; `NULL` only to
 *                       measure.
 * @param[in]  _count    the room in `_out`, in units, terminator included.
 * @param[out] _made     receives the units the text takes, terminator aside;
 *                       may be `NULL`.
 * @return 0; 1 when `_out` has no room for the text and its terminator; -1
 *         for text that is not WTF-8: a stray or missing continuation byte, a
 *         form longer than needed, a code point past U+10FFFF, or a pair
 *         written as two three-byte forms.
 */
int d_internal_file_wtf8_to_utf16(const char*            _in,
                                  size_t                 _in_size,
                                  d_internal_file_u16_t* _out,
                                  size_t                 _count,
                                  size_t*                _made);

/**
 * @brief Converts UTF-16 to WTF-8: every sequence of units has a spelling,
 *        an unpaired surrogate the three bytes its code point would take.
 *
 * @param[in]  _in        the units.
 * @param[in]  _in_count  how many, or D_INTERNAL_FILE_TERMINATED.
 * @param[out] _out       receives the text and a terminator; `NULL` only to
 *                        measure.
 * @param[in]  _size      the room in `_out`, in bytes, terminator included.
 * @param[out] _made      receives the bytes the text takes, terminator
 *                        aside; may be `NULL`.
 * @return 0, or 1 when `_out` has no room for the text and its terminator.
 */
int d_internal_file_utf16_to_wtf8(const d_internal_file_u16_t* _in,
                                  size_t                       _in_count,
                                  char*                        _out,
                                  size_t                       _size,
                                  size_t*                      _made);

/**
 * @brief The reverse of d_internal_file_to_native: text the platform handed
 *        back -- a name, a directory -- as UTF-8 where this build converts,
 *        and as it is everywhere else. A lone surrogate, which Windows allows
 *        in a name and UTF-8 cannot carry, comes back in WTF-8's three bytes,
 *        which d_internal_file_to_native takes back unchanged.
 *
 * @param[in]  _native  NUL-terminated text in the platform's form.
 * @param[out] _out     receives the text and its terminator; on failure it
 *                      holds an empty string, when it has room for one.
 * @param[in]  _size    capacity of `_out`, in bytes.
 * @return 0, or -1 with errno set: ERANGE when the text does not fit `_size`
 *         bytes.
 */
int   d_internal_file_from_native(const d_internal_file_nchar_t* _native,
                                  char*                          _out,
                                  size_t                         _size);

D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_COMMON_H
