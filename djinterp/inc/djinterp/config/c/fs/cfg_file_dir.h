/*******************************************************************************
* djinterp [config]                                               cfg_file_dir.h
*
* Build-time configuration for c/fs/file_dir.h -- directory creation,
* removal, traversal, and the path operations that must ask the filesystem
* (getcwd, chdir, realpath).
*
*   The lexical counterpart is file_path, which answers the same shape of
* question without a syscall. If a query does not need the disk, it belongs
* there; d_path_resolve is here precisely because it does.
*
*   targets:  c/fs/file_dir.h, c/fs/file_dir.c -> D_INTERNAL_FILE_DIR_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_dir.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Creation
         1.  D_CFG_FILE_DIR_CREATE_MODE
         2.  D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK
    2.  Traversal
         1.  D_CFG_FILE_HAS_DIRENT_TYPE
         2.  D_CFG_FILE_DIR_FILL_TYPE
         3.  D_CFG_FILE_DIR_SKIP_DOTS
    3.  Resolution
         1.  D_CFG_FILE_DIR_REALPATH_ALLOC
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_DIR_CREATE_MODE
         2.  D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL
         3.  D_INTERNAL_FILE_DIR_TYPE_BY_STAT
         4.  D_INTERNAL_FILE_DIR_REALPATH_ALLOC
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_DIR_TYPE_IS_FREE
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_DIR_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_DIR_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Creation
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_DIR_CREATE_MODE
//   knob: permission bits for the INTERMEDIATE directories d_dir_create_parents
// creates, when the caller's mode is not obviously right for them (value
// model). Default 0755.
//   It is separate from the caller's mode on purpose. Passing 0700 to
// d_dir_create_parents("/a/b/c") means "c should be private" far more often
// than it means "and make /a and /a/b private too" -- and a caller who wanted
// the latter can pass this. The process umask still applies.
#ifndef D_CFG_FILE_DIR_CREATE_MODE
    #define D_CFG_FILE_DIR_CREATE_MODE 0755
#endif  // D_CFG_FILE_DIR_CREATE_MODE

// 1.1.2
// D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK
//   knob: d_dir_create_parents succeeds when the directory already exists.
//   ON, and it is the entire difference between mkdir and mkdir -p: the
// request is "ensure this path exists", and a path that already exists has
// satisfied it. Set 0 to make an existing target EEXIST, i.e. to get mkdir
// semantics with intermediate creation, which is a strange thing to want but
// is at least expressible.
#ifndef D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK

// 1.2    Traversal
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_HAS_DIRENT_TYPE
//   knob: the platform's struct dirent carries a d_type field.
//   Linux, macOS and the BSDs do; Solaris and some embedded libcs do not.
// Even where the field exists it may read DT_UNKNOWN -- XFS without ftype,
// and most network filesystems, genuinely do not know without a stat.
#ifndef D_CFG_FILE_HAS_DIRENT_TYPE
    #if ( (defined(D_ENV_PLATFORM_LINUX)) ||                                   \
          (defined(D_ENV_PLATFORM_MACOS)) ||                                   \
          (defined(D_ENV_PLATFORM_BSD)) )
        #define D_CFG_FILE_HAS_DIRENT_TYPE 1
    #else
        #define D_CFG_FILE_HAS_DIRENT_TYPE 0
    #endif
#endif  // D_CFG_FILE_HAS_DIRENT_TYPE

// 1.2.2
// D_CFG_FILE_DIR_FILL_TYPE
//   knob: populate d_dirent_t.d_type, falling back to a stat per entry where
// the kernel will not say.
//   ON. Read the cost honestly: where the kernel supplies d_type this is
// free, and where it does not (or says DT_UNKNOWN) it is ONE STAT PER ENTRY.
// Walking a 100k-entry directory on a network filesystem, that is 100k round
// trips to answer a question the caller may not have asked.
//   Set 0 to leave d_type as DT_UNKNOWN always and never pay. A caller who
// needs the type can d_file_stat the entries it actually cares about, which is
// usually far fewer than all of them.
#ifndef D_CFG_FILE_DIR_FILL_TYPE
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_DIR_FILL_TYPE D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_DIR_FILL_TYPE 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_DIR_FILL_TYPE

// 1.2.3
// D_CFG_FILE_DIR_SKIP_DOTS
//   knob: have d_dir_read hide "." and "..".
//   OFF, matching readdir(3), because hiding them is a policy and readdir is
// a mechanism. Nearly every caller does skip them -- and a recursive walk that
// forgets to is an infinite loop -- but a caller that wants an exact directory
// listing is entitled to one. Turn it on to make the common case harder to
// get wrong.
#ifndef D_CFG_FILE_DIR_SKIP_DOTS
    #define D_CFG_FILE_DIR_SKIP_DOTS 0
#endif  // D_CFG_FILE_DIR_SKIP_DOTS

// 1.3    Resolution
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_FILE_DIR_REALPATH_ALLOC
//   knob: let d_path_resolve(_path, NULL) allocate its result, as POSIX
// realpath does.
//   ON. The alternative -- requiring a D_FILE_PATH_MAX buffer -- is a real
// constraint on Linux, where PATH_MAX is 4096 and a caller may not want that
// on the stack. Set 0 for a build with no allocator, where the NULL form then
// fails with EINVAL rather than silently doing something else.
#ifndef D_CFG_FILE_DIR_REALPATH_ALLOC
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_DIR_REALPATH_ALLOC D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_DIR_REALPATH_ALLOC 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_DIR_REALPATH_ALLOC


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK)
    #error "D_CFG_FILE_DIR_MKDIR_P_EXISTING_OK must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_HAS_DIRENT_TYPE)
    #error "D_CFG_FILE_HAS_DIRENT_TYPE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_DIR_FILL_TYPE)
    #error "D_CFG_FILE_DIR_FILL_TYPE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_DIR_SKIP_DOTS)
    #error "D_CFG_FILE_DIR_SKIP_DOTS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_DIR_REALPATH_ALLOC)
    #error "D_CFG_FILE_DIR_REALPATH_ALLOC must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_DIR_CREATE_MODE
//   resolved: intermediate-directory mode (bare constant, #if-safe).
#define D_INTERNAL_FILE_DIR_CREATE_MODE D_CFG_FILE_DIR_CREATE_MODE

// 3.1.2
// D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL
//   resolved: 1 when d_type can be read straight from the platform's dirent --
// the free path.
#if ( (D_CFG_IS_ON(D_CFG_FILE_DIR_FILL_TYPE)) &&                               \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_DIRENT_TYPE)) &&                             \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) )
    #define D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL 1
#else
    #define D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL 0
#endif

// 3.1.3
// D_INTERNAL_FILE_DIR_TYPE_BY_STAT
//   resolved: 1 when d_dir_read may fall back to a stat per entry to answer
// d_type. This is the expensive path; see the knob's note.
#if ( (D_CFG_IS_ON(D_CFG_FILE_DIR_FILL_TYPE)) &&                               \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_WIN32)) )
    #define D_INTERNAL_FILE_DIR_TYPE_BY_STAT 1
#else
    #define D_INTERNAL_FILE_DIR_TYPE_BY_STAT 0
#endif

// 3.1.4
// D_INTERNAL_FILE_DIR_REALPATH_ALLOC
//   resolved: 1 when d_path_resolve(p, NULL) allocates.
#if D_CFG_IS_ON(D_CFG_FILE_DIR_REALPATH_ALLOC)
    #define D_INTERNAL_FILE_DIR_REALPATH_ALLOC 1
#else
    #define D_INTERNAL_FILE_DIR_REALPATH_ALLOC 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_DIR_TYPE_IS_FREE
//   query: 1 when d_dirent_t.d_type costs nothing; 0 when it may cost a stat
// per entry, or is never populated. Safe in #if.
#define D_FILE_DIR_TYPE_IS_FREE D_INTERNAL_FILE_DIR_TYPE_FROM_KERNEL


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_DIR_H
