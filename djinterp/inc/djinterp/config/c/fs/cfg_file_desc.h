/*******************************************************************************
* djinterp [config]                                              cfg_file_desc.h
*
* Build-time configuration for c/fs/file_desc.h -- descriptor lifecycle
* (open, close, dup, dup2, fileno).
*
*   This module owns descriptors, not the bytes that move through them; that
* is file_io. The split is deliberate: a program that is handed a descriptor
* by its parent and only ever reads it links file_io and not this.
*
*   targets:  c/fs/file_desc.h, c/fs/file_desc.c -> D_INTERNAL_FILE_DESC_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Open behaviour
         1.  D_CFG_FILE_DESC_CLOEXEC
         2.  D_CFG_FILE_DESC_BINARY
         3.  D_CFG_FILE_DESC_CREATE_MODE
    2.  Duplication
         1.  D_CFG_FILE_DESC_DUP_CLOEXEC
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_DESC_CREATE_MODE
         2.  D_INTERNAL_FILE_DESC_CLOEXEC
         3.  D_INTERNAL_FILE_DESC_DUP_CLOEXEC
         4.  D_INTERNAL_FILE_DESC_BINARY
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_DESC_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_DESC_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Open behaviour
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_DESC_CLOEXEC
//   knob: add O_CLOEXEC to every d_file_open unless the caller asked otherwise.
// Follows the subframework's D_CFG_FILE_CLOEXEC_DEFAULT.
//   Worth doing here rather than leaving to callers: without O_CLOEXEC there
// is a window between open() and a manual fcntl() during which another thread
// can fork+exec and leak the descriptor into the child. O_CLOEXEC closes the
// window because it is atomic with the open.
#ifndef D_CFG_FILE_DESC_CLOEXEC
    #define D_CFG_FILE_DESC_CLOEXEC D_CFG_FILE_CLOEXEC_DEFAULT
#endif  // D_CFG_FILE_DESC_CLOEXEC

// 1.1.2
// D_CFG_FILE_DESC_BINARY
//   knob: add O_BINARY on Windows unless the caller named O_TEXT.
//   On by default, and this one is close to non-negotiable at the descriptor
// level: a text-mode descriptor translates "\n" to "\r\n" on write and back on
// read, so d_file_read_fd of N bytes from a file of N bytes returns fewer than
// N and the caller cannot tell why. Streams may reasonably want translation;
// raw descriptors essentially never do. No effect on POSIX, which has no such
// notion.
#ifndef D_CFG_FILE_DESC_BINARY
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_DESC_BINARY D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_DESC_BINARY 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_DESC_BINARY

// 1.1.3
// D_CFG_FILE_DESC_CREATE_MODE
//   knob: the permission bits used when a caller passes O_CREAT and no mode
// (value model). Default 0644.
//   POSIX says the mode argument is required with O_CREAT and that omitting
// it is undefined -- in practice it reads whatever garbage is on the stack
// and creates a file with those permissions, which is a security bug that
// looks like a typo. This module substitutes a defined value instead.
#ifndef D_CFG_FILE_DESC_CREATE_MODE
    #define D_CFG_FILE_DESC_CREATE_MODE 0644
#endif  // D_CFG_FILE_DESC_CREATE_MODE

// 1.2    Duplication
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_DESC_DUP_CLOEXEC
//   knob: make d_file_dup_fd produce a close-on-exec descriptor
// (F_DUPFD_CLOEXEC).
//   Follows D_CFG_FILE_DESC_CLOEXEC, and note it must be asked for
// separately: POSIX specifies that dup() and dup2() explicitly CLEAR the
// close-on-exec flag on the new descriptor. A careful O_CLOEXEC open followed
// by a dup silently produces an inheritable descriptor otherwise.
//   d_file_dup2_fd is deliberately NOT covered: the whole purpose of dup2 is
// usually to install a descriptor onto 0/1/2 for a child to inherit, so forcing
// close-on-exec there would defeat the call.
#ifndef D_CFG_FILE_DESC_DUP_CLOEXEC
    #define D_CFG_FILE_DESC_DUP_CLOEXEC D_CFG_FILE_DESC_CLOEXEC
#endif  // D_CFG_FILE_DESC_DUP_CLOEXEC


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_DESC_CLOEXEC)
    #error "D_CFG_FILE_DESC_CLOEXEC must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_DESC_BINARY)
    #error "D_CFG_FILE_DESC_BINARY must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_DESC_DUP_CLOEXEC)
    #error "D_CFG_FILE_DESC_DUP_CLOEXEC must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_DESC_CREATE_MODE
//   resolved: the substituted creation mode, as a bare constant (#if-safe).
#define D_INTERNAL_FILE_DESC_CREATE_MODE D_CFG_FILE_DESC_CREATE_MODE

// 3.1.2
// D_INTERNAL_FILE_DESC_CLOEXEC
//   resolved: 1 when d_file_open adds a close-on-exec flag. Requires
// descriptors, so the ISO C backend cannot honour it.
#if ( (D_CFG_IS_ON(D_CFG_FILE_DESC_CLOEXEC)) &&                                \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_DESC_CLOEXEC 1
#else
    #define D_INTERNAL_FILE_DESC_CLOEXEC 0
#endif

// 3.1.3
// D_INTERNAL_FILE_DESC_DUP_CLOEXEC
//   resolved: 1 when d_file_dup_fd restores close-on-exec that dup() would have
// cleared.
#if ( (D_CFG_IS_ON(D_CFG_FILE_DESC_DUP_CLOEXEC)) &&                            \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)) )
    #define D_INTERNAL_FILE_DESC_DUP_CLOEXEC 1
#else
    #define D_INTERNAL_FILE_DESC_DUP_CLOEXEC 0
#endif

// 3.1.4
// D_INTERNAL_FILE_DESC_BINARY
//   resolved: 1 when d_file_open forces binary mode. Windows-only by
// construction.
#if ( (D_CFG_IS_ON(D_CFG_FILE_DESC_BINARY)) &&                                 \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #define D_INTERNAL_FILE_DESC_BINARY 1
#else
    #define D_INTERNAL_FILE_DESC_BINARY 0
#endif


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_DESC_H
