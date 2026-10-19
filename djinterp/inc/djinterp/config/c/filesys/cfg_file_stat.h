/*******************************************************************************
* djinterp [config]                                              cfg_file_stat.h
*
* Build-time configuration for c/filesys/file_stat.h -- file metadata.
*
*   This is the module that consumes D_CFG_FILE_HAS_STAT_NSEC and
* D_CFG_FILE_HAS_BIRTHTIME from cfg_file_common.h. Those gates exist because
* the timestamp members of the platform's struct stat are spelled differently
* everywhere (st_mtim on POSIX 2008, st_mtimespec on macOS/BSD, FILETIME on
* Win32) -- env cannot rename djinterp's OWN fields to dodge the st_mtime
* macro collision, but it can tell this module which member to READ. What the
* module actually reads, which is less than the platforms offer, is section
* 3's to say.
*
*   targets:  c/filesys/file_stat.h, c/filesys/file_stat.c ->
*             D_INTERNAL_FILE_STAT_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/filesys/cfg_file_stat.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
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
         5.  D_INTERNAL_FILE_STAT_STATX_MEMO
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_STAT_HAS_NSEC / D_FILE_STAT_HAS_BIRTHTIME
*/

#ifndef DJINTERP_CONFIG_C_FILESYS_CFG_FILE_STAT_H
#define DJINTERP_CONFIG_C_FILESYS_CFG_FILE_STAT_H 1

// djinterp
#include "cfg_file_common.h"  // shared filesys knobs, D_CFG_IS_BOOL


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
// statx also returns everything in one syscall that stat reports in pieces,
// and it is asked as stat asks -- for a descriptor as for a path, and without
// automounting the path's last component.
//   It degrades by itself: statx is Linux 4.11+ and glibc 2.28+, and a seccomp
// sandbox or an old kernel answers ENOSYS or EPERM -- in which case this
// falls back to stat() and st_created reads 0, which is the same honest
// answer as any other platform without btime. A refusal stat then answers
// for is remembered for the life of the process. So turning it off costs the
// creation timestamp and nothing else.
//   Available also takes the build: glibc declares statx only under
// _GNU_SOURCE, which a Linux build gives file_stat.c (its banner). Without
// it the stat() path is all there is, as though this were off.
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


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_STAT_NSEC
//   resolved: which sub-second spelling to read, or 0 for none.
//   0 = seconds only  1 = st_mtim.tv_nsec  2 = st_mtimespec
//   4 = st_mtimensec, Darwin's at a POSIX level without _DARWIN_C_SOURCE
//   Windows' FILETIME (3) is not among them: file_stat.c asks the CRT's
// _stat64, which reports whole seconds, so there the sub-second fields stay 0
// and D_FILE_STAT_HAS_NSEC says so.
#if ( (D_CFG_IS_ON(D_CFG_FILE_STAT_NSEC)) &&                                   \
      (D_CFG_NORM(D_CFG_FILE_HAS_STAT_NSEC) != 3) )
    #define D_INTERNAL_FILE_STAT_NSEC D_CFG_NORM(D_CFG_FILE_HAS_STAT_NSEC)
#else
    #define D_INTERNAL_FILE_STAT_NSEC 0
#endif

// 3.1.2
// D_INTERNAL_FILE_STAT_STATX
//   resolved: 1 when file_stat.c compiles the statx path. Needs the declaration
// (glibc 2.28+, under _GNU_SOURCE, which STATX_BTIME arrives with) as well as
// the knob; the KERNEL's support is a runtime question the module handles by
// falling back.
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
//   Windows' CRT ctime IS creation, Apple's systems expose st_birthtime, and
// Linux now qualifies through statx. One term per delivery mechanism, in one
// place -- and the BSDs have none yet: file_stat.c does not read their
// st_birthtim, so they do not get a term that would say it does.
#if ( (D_CFG_IS_ON(D_CFG_FILE_STAT_BIRTHTIME)) &&                              \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_BIRTHTIME)) &&                               \
      ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) ||                                 \
        (D_INTERNAL_FILE_OS_DARWIN == 1) ||                                    \
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
// D_INTERNAL_FILE_STAT_STATX_MEMO
//   resolved: 1 when a refused statx is remembered for the life of the
// process. Any thread may be the one to find out, so the memo is a flag every
// thread may write, which takes the compiler's __atomic builtins. Without
// them statx is asked every time, and a refusal costs one failed call per
// stat.
#if ( (D_INTERNAL_FILE_STAT_STATX == 1) &&                                     \
      (D_CFG_IS_ON(D_ENV_COMPILER_HAS_ATOMIC_BUILTINS)) )
    #define D_INTERNAL_FILE_STAT_STATX_MEMO 1
#else
    #define D_INTERNAL_FILE_STAT_STATX_MEMO 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_STAT_HAS_NSEC / D_FILE_STAT_HAS_BIRTHTIME
//   query: 1 when the corresponding d_stat_t fields carry real data, else 0.
// Safe in #if; the test suite uses them to decide what it may assert. On
// Linux the creation time answers for a translation unit built as
// file_stat.c is, with _GNU_SOURCE; one built without reads 0 even where the
// library reports one.
#define D_FILE_STAT_HAS_NSEC      (D_INTERNAL_FILE_STAT_NSEC != 0)
#define D_FILE_STAT_HAS_BIRTHTIME D_INTERNAL_FILE_STAT_BIRTHTIME


#endif  // DJINTERP_CONFIG_C_FILESYS_CFG_FILE_STAT_H
