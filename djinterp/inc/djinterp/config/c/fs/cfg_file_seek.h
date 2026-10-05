/*******************************************************************************
* djinterp [config]                                              cfg_file_seek.h
*
* Build-time configuration for c/fs/file_seek.h -- 64-bit positioning and
* truncation.
*
*   targets:  c/fs/file_seek.h, c/fs/file_seek.c -> D_INTERNAL_FILE_SEEK_FLUSH
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_seek.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Truncation
         1.  D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_SEEK_FLUSH
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_SEEK_IS_64BIT
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_SEEK_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_SEEK_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Truncation
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE
//   knob: flush a stream's stdio buffer before truncating the file beneath
// it.
//   ON, and you almost certainly want it on. d_file_truncate_stream works
// through the descriptor inside the stream, and the descriptor knows nothing
// about bytes still sitting in stdio's buffer. Truncate to 10 with 200
// unflushed bytes pending, and stdio writes them out afterwards -- the file
// ends up 200 bytes long, not 10, and the truncation appears to have silently
// done nothing.
//   The only reason this is a knob at all is that the flush is a write, and a
// caller who has already flushed pays for a second syscall that does nothing.
// Set 0 only if you flush yourself and have measured that it matters.
#ifndef D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE)
    #error "D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_SEEK_FLUSH
//   resolved: 1 when d_file_truncate_stream flushes first.
#if D_CFG_IS_ON(D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE)
    #define D_INTERNAL_FILE_SEEK_FLUSH 1
#else
    #define D_INTERNAL_FILE_SEEK_FLUSH 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_SEEK_IS_64BIT
//   query: 1 when positioning is genuinely 64-bit on this build. When 0,
// offsets beyond 2 GiB are rejected rather than silently wrapped.
#define D_FILE_SEEK_IS_64BIT D_INTERNAL_FILE_HAS_FSEEKO


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_SEEK_H
