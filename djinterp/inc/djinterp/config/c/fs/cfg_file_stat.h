/*******************************************************************************
* djinterp [config]                                              cfg_file_stat.h
*
* Build-time configuration for c/fs/file_stat.h -- file metadata.
*
*   This is the module that consumes D_CFG_FILE_HAS_STAT_NSEC and
* D_CFG_FILE_HAS_BIRTHTIME from cfg_file_common.h. Those gates exist because
* the timestamp members of the platform's struct stat are spelled differently
* everywhere (st_mtim on POSIX 2008, st_mtimespec on macOS/BSD, FILETIME on
* Win32) -- env cannot rename djinterp's OWN fields to dodge the st_mtime
* macro collision, but it can tell this module which member to READ.
*
*   targets:  c/fs/file_stat.h, c/fs/file_stat.c -> D_INTERNAL_FILE_STAT_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_stat.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Timestamps
         1.  D_CFG_FILE_STAT_NSEC
         2.  D_CFG_FILE_STAT_BIRTHTIME
    2.  Platform fast paths
         1.  D_CFG_FILE_STAT_USE_STATX
    3.  Symlink handling
         1.  D_CFG_FILE_STAT_FOLLOW_SYMLINKS
    4.  Predicates
         1.  D_CFG_FILE_STAT_PREDICATES_INLINE
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_STAT_NSEC
         2.  D_INTERNAL_FILE_STAT_STATX
         3.  D_INTERNAL_FILE_STAT_BIRTHTIME
         4.  D_INTERNAL_FILE_STAT_FOLLOW
         5.  D_FILE_STAT_PRED
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_STAT_HAS_NSEC / D_FILE_STAT_HAS_BIRTHTIME
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_STAT_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_STAT_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Timestamps
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_STAT_NSEC
//   knob: populate d_stat_t's sub-second timestamp fields.
//   On where the platform reports them. Turn it off if you compare
// timestamps for equality across a copy: most tools preserve whole seconds
// and drop nanoseconds, so a file and its copy compare EQUAL at second
// resolution and UNEQUAL at nanosecond resolution. Neither answer is wrong;
// they answer different questions.
#ifndef D_CFG_FILE_STAT_NSEC
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_STAT_NSEC D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_STAT_NSEC 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_STAT_NSEC

// 1.1.2
// D_CFG_FILE_STAT_BIRTHTIME
//   knob: populate d_stat_t.st_created with a true creation time where the
// platform has one.
//   On by default where available. Note what it is NOT: st_changed carries
// POSIX ctime, which is the METADATA CHANGE time and is not creation. The old
// dfile called that field st_ctime and described it as "creation/change
// time", which is two different things joined by a slash; they are separate
// fields here precisely so nobody has to guess which one they got.
#ifndef D_CFG_FILE_STAT_BIRTHTIME
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_STAT_BIRTHTIME D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_STAT_BIRTHTIME 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_STAT_BIRTHTIME

// 1.2    Platform fast paths
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_STAT_USE_STATX
//   knob: use Linux statx(2) instead of stat(2).
//   ON where available, and it is the only reason st_created can be real on
// Linux: btime exists in the inode but stat() has no field to return it in.
// statx also returns everything in one syscall that stat reports in pieces.
//   It degrades by itself: statx is Linux 4.11+ and glibc 2.28+, and a seccomp
// sandbox or an old kernel answers ENOSYS -- in which case this falls back to
// stat() and st_created reads 0, which is the same honest answer as any other
// platform without btime. So turning it off costs the creation timestamp and
// nothing else.
#ifndef D_CFG_FILE_STAT_USE_STATX
    #define D_CFG_FILE_STAT_USE_STATX D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_STAT_USE_STATX

// 1.3    Symlink handling
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_FILE_STAT_FOLLOW_SYMLINKS
//   knob: whether d_file_stat (and the predicates built on it) follow a
// symbolic link to its target.
//   ON, which is what stat() means and what callers expect: d_dir_exists on a
// link to a directory says yes. d_file_stat_nofollow is always available when
// you want the link itself, so this knob is for the rare program that wants
// EVERY query to stop at the link -- an archiver, a backup tool, anything that
// must not traverse.
#ifndef D_CFG_FILE_STAT_FOLLOW_SYMLINKS
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_STAT_FOLLOW_SYMLINKS D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_STAT_FOLLOW_SYMLINKS 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_STAT_FOLLOW_SYMLINKS

// 1.4    Predicates
//------------------------------------------------------------------------------
// 1.4.1
// D_CFG_FILE_STAT_PREDICATES_INLINE
//   knob: publish d_file_exists / d_file_is_regular / d_dir_exists as header
// inlines. Follows the subframework's D_CFG_FILE_INLINE_PREDICATES.
//   Note the cost the inlining does NOT remove: each predicate is still a
// full stat syscall. Three predicates on one path is three syscalls, and
// inlining makes that cheaper by a call frame, not by a syscall. If you are
// asking more than one question about a path, call d_file_stat once.
#ifndef D_CFG_FILE_STAT_PREDICATES_INLINE
    #define D_CFG_FILE_STAT_PREDICATES_INLINE D_CFG_FILE_INLINE_PREDICATES
#endif  // D_CFG_FILE_STAT_PREDICATES_INLINE


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_STAT_NSEC)
    #error "D_CFG_FILE_STAT_NSEC must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_STAT_BIRTHTIME)
    #error "D_CFG_FILE_STAT_BIRTHTIME must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_STAT_USE_STATX)
    #error "D_CFG_FILE_STAT_USE_STATX must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_STAT_FOLLOW_SYMLINKS)
    #error "D_CFG_FILE_STAT_FOLLOW_SYMLINKS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_STAT_PREDICATES_INLINE)
    #error "D_CFG_FILE_STAT_PREDICATES_INLINE must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_STAT_NSEC
//   resolved: which sub-second spelling to read, or 0 for none.
//   0 = seconds only  1 = st_mtim.tv_nsec  2 = st_mtimespec  3 = FILETIME
#if D_CFG_IS_ON(D_CFG_FILE_STAT_NSEC)
    #define D_INTERNAL_FILE_STAT_NSEC D_CFG_NORM(D_CFG_FILE_HAS_STAT_NSEC)
#else
    #define D_INTERNAL_FILE_STAT_NSEC 0
#endif

// 3.1.2
// D_INTERNAL_FILE_STAT_STATX
//   resolved: 1 when file_stat.c compiles the statx path. Needs the declaration
// (glibc 2.28+) as well as the knob; the KERNEL's support is a runtime
// question the module handles by falling back.
#if ( (D_CFG_IS_ON(D_CFG_FILE_STAT_USE_STATX)) &&                              \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_STATX)) &&                                   \
      (defined(STATX_BTIME)) )
    #define D_INTERNAL_FILE_STAT_STATX 1
#else
    #define D_INTERNAL_FILE_STAT_STATX 0
#endif

// 3.1.3
// D_INTERNAL_FILE_STAT_BIRTHTIME
//   resolved: 1 when d_stat_t.st_created carries a real creation time. When 0
// the field reads 0, never a substituted ctime -- a fabricated timestamp is a
// lie the caller cannot detect.
//
//   NOTE THE THIRD TERM, and why it is not redundant with
// D_CFG_FILE_HAS_BIRTHTIME. That knob answers "does the PLATFORM have a
// creation time"; this one answers "does THIS MODULE deliver one", and they
// are not the same question. Linux has btime -- but only through statx(2), so
// before the statx path existed this macro reported 1 while the field was
// always 0. A caller can branch on a macro that says no; it cannot defend
// against one that says yes and lies.
//   Windows' CRT ctime IS creation, macOS/BSD expose st_birthtimespec, and
// Linux now qualifies through statx. One term per delivery mechanism, in one
// place.
#if ( (D_CFG_IS_ON(D_CFG_FILE_STAT_BIRTHTIME)) &&                              \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_BIRTHTIME)) &&                               \
      ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) ||                                 \
        (defined(D_ENV_PLATFORM_MACOS)) ||                                     \
        (defined(D_ENV_PLATFORM_BSD)) ||                                       \
        (D_INTERNAL_FILE_STAT_STATX == 1) ) )
    #define D_INTERNAL_FILE_STAT_BIRTHTIME 1
#else
    #define D_INTERNAL_FILE_STAT_BIRTHTIME 0
#endif

// 3.1.4
// D_INTERNAL_FILE_STAT_FOLLOW
//   resolved: 1 when d_file_stat follows symbolic links.
#if D_CFG_IS_ON(D_CFG_FILE_STAT_FOLLOW_SYMLINKS)
    #define D_INTERNAL_FILE_STAT_FOLLOW 1
#else
    #define D_INTERNAL_FILE_STAT_FOLLOW 0
#endif

// 3.1.5
// D_FILE_STAT_PRED
//   query: storage class for the predicates.
#if D_CFG_IS_ON(D_CFG_FILE_STAT_PREDICATES_INLINE)
    #define D_FILE_STAT_PRED D_STATIC_INLINE
#else
    #define D_FILE_STAT_PRED
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_STAT_HAS_NSEC / D_FILE_STAT_HAS_BIRTHTIME
//   query: 1 when the corresponding d_stat_t fields carry real data, else 0.
// Safe in #if; the test suite uses them to decide what it may assert.
#define D_FILE_STAT_HAS_NSEC      (D_INTERNAL_FILE_STAT_NSEC != 0)
#define D_FILE_STAT_HAS_BIRTHTIME D_INTERNAL_FILE_STAT_BIRTHTIME


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_STAT_H
