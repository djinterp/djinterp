/*******************************************************************************
* djinterp [config]                                               cfg_file_ops.h
*
* Build-time configuration for c/fs/file_ops.h -- whole-file operations
* (remove, unlink, rename, copy).
*
*   This is where D_INTERNAL_FILE_USE_NATIVE_COPY from cfg_file_common.h is
* finally consumed: copy_file_range(2) on Linux, fcopyfile(3) on macOS,
* CopyFileEx on Win32.
*
*   targets:  c/fs/file_ops.h, c/fs/file_ops.c -> D_INTERNAL_FILE_OPS_COPY_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_ops.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Copy
         1.  D_CFG_FILE_OPS_COPY_BUF_SIZE
         2.  D_CFG_FILE_OPS_COPY_NATIVE
         3.  D_CFG_FILE_OPS_COPY_PRESERVE_MODE
         4.  D_CFG_FILE_OPS_COPY_OVERWRITE
    2.  Rename
         1.  D_CFG_FILE_OPS_RENAME_CROSS_DEVICE
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_OPS_COPY_BUF
         2.  D_INTERNAL_FILE_OPS_COPY_NATIVE
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_OPS_COPY_IS_NATIVE
         2.  D_FILE_OPS_RENAME_IS_ATOMIC
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_OPS_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_OPS_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Copy
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_OPS_COPY_BUF_SIZE
//   knob: bounce-buffer size for the portable copy path, in bytes (value
// model). Only used when no native engine is available or NATIVE is off.
// Default 64 KiB: large enough to amortise the syscalls, small enough to stay
// off the stack and inside L2.
#ifndef D_CFG_FILE_OPS_COPY_BUF_SIZE
    #define D_CFG_FILE_OPS_COPY_BUF_SIZE (64u * 1024u)
#endif  // D_CFG_FILE_OPS_COPY_BUF_SIZE

// 1.1.2
// D_CFG_FILE_OPS_COPY_NATIVE
//   knob: hand a whole-file copy to the platform's own engine --
// copy_file_range(2) on Linux, fcopyfile(3) on macOS, CopyFileEx on Win32 --
// instead of shuttling bytes through user space.
//   On by default and worth it: copy_file_range never leaves the kernel and
// can be serviced by the filesystem itself, so on btrfs or XFS a "copy"
// becomes a reflink -- instant, and consuming no additional space.
//   Set 0 when you need the copy to be observable, byte-by-byte, in the same
// way on every platform: a reflink changes the storage semantics (the copy
// shares extents until written), and fcopyfile brings metadata along that the
// portable path does not.
#ifndef D_CFG_FILE_OPS_COPY_NATIVE
    #define D_CFG_FILE_OPS_COPY_NATIVE D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_OPS_COPY_NATIVE

// 1.1.3
// D_CFG_FILE_OPS_COPY_PRESERVE_MODE
//   knob: give the destination the source's permission bits.
//   On by default: copying a 0600 private key to a 0644 file is a security
// bug, and it is the default behaviour if nobody does this. The process umask
// still applies to the initial create, so this is a chmod after the fact.
#ifndef D_CFG_FILE_OPS_COPY_PRESERVE_MODE
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_OPS_COPY_PRESERVE_MODE D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_OPS_COPY_PRESERVE_MODE 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_OPS_COPY_PRESERVE_MODE

// 1.1.4
// D_CFG_FILE_OPS_COPY_OVERWRITE
//   knob: let d_file_copy replace an existing destination.
//   ON, matching cp(1) and the old dfile behaviour. Set 0 to make an existing
// destination an EEXIST failure -- the safer default for a program that never
// intends to clobber, and one that turns a data-loss bug into an error.
#ifndef D_CFG_FILE_OPS_COPY_OVERWRITE
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_OPS_COPY_OVERWRITE D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_OPS_COPY_OVERWRITE 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_OPS_COPY_OVERWRITE

// 1.2    Rename
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_OPS_RENAME_CROSS_DEVICE
//   knob: when rename fails with EXDEV, fall back to copy-then-delete.
//   OFF by default, and think before turning it on. rename(2) is ATOMIC:
// either the new name exists or it does not, and no observer sees a partial
// file. Copy-then-delete is not atomic, is not instant, and can leave BOTH
// files behind if it is interrupted. A caller who wrote rename() and got
// EXDEV asked for an atomic operation and cannot have one; silently
// substituting a slow non-atomic one hides that.
//   Turn it on only where you want mv(1) semantics and have accepted them.
#ifndef D_CFG_FILE_OPS_RENAME_CROSS_DEVICE
    #define D_CFG_FILE_OPS_RENAME_CROSS_DEVICE 0
#endif  // D_CFG_FILE_OPS_RENAME_CROSS_DEVICE


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_OPS_COPY_NATIVE)
    #error "D_CFG_FILE_OPS_COPY_NATIVE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_OPS_COPY_PRESERVE_MODE)
    #error "D_CFG_FILE_OPS_COPY_PRESERVE_MODE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_OPS_COPY_OVERWRITE)
    #error "D_CFG_FILE_OPS_COPY_OVERWRITE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_OPS_RENAME_CROSS_DEVICE)
    #error "D_CFG_FILE_OPS_RENAME_CROSS_DEVICE must be literally 0 or 1"
#endif
#if (D_CFG_NORM(D_CFG_FILE_OPS_COPY_BUF_SIZE) <= 0)
    #error "D_CFG_FILE_OPS_COPY_BUF_SIZE must be greater than 0"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_OPS_COPY_BUF
//   resolved: the portable copy path's buffer size (bare constant, #if-safe).
#define D_INTERNAL_FILE_OPS_COPY_BUF D_CFG_FILE_OPS_COPY_BUF_SIZE

// 3.1.2
// D_INTERNAL_FILE_OPS_COPY_NATIVE
//   resolved: 1 when a native copy engine is both present and wanted.
#if ( (D_CFG_IS_ON(D_CFG_FILE_OPS_COPY_NATIVE)) &&                             \
      (D_INTERNAL_FILE_USE_NATIVE_COPY == 1) )
    #define D_INTERNAL_FILE_OPS_COPY_NATIVE 1
#else
    #define D_INTERNAL_FILE_OPS_COPY_NATIVE 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_OPS_COPY_IS_NATIVE
//   query: 1 when d_file_copy may offload to the platform. Safe in #if.
#define D_FILE_OPS_COPY_IS_NATIVE  D_INTERNAL_FILE_OPS_COPY_NATIVE

// 4.1.2
// D_FILE_OPS_RENAME_IS_ATOMIC
//   query: 1 when d_file_rename is always atomic; 0 when a cross-device
// fallback may silently substitute a non-atomic copy-then-delete.
#define D_FILE_OPS_RENAME_IS_ATOMIC                                            \
    (!D_CFG_IS_ON(D_CFG_FILE_OPS_RENAME_CROSS_DEVICE))


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_OPS_H
