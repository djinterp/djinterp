/*******************************************************************************
* djinterp [config]                                              cfg_file_pipe.h
*
* Build-time configuration for c/fs/file_pipe.h -- process pipes.
*
*   d_pipe_open runs a command through a SHELL. Every use of it with a string
* that contains anything a user influenced is a command-injection bug. There is
* no knob here that fixes that, because the fix is not to call it -- it is to
* use posix_spawn / CreateProcess with an argument vector, which this module
* does not offer and which is a different module's job.
*
*   targets:  c/fs/file_pipe.h, c/fs/file_pipe.c -> D_INTERNAL_FILE_PIPE_BINARY
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_pipe.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Mode
         1.  D_CFG_FILE_PIPE_BINARY
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_PIPE_BINARY
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_PIPE_IS_AVAILABLE
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_PIPE_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_PIPE_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Mode
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_PIPE_BINARY
//   knob: open pipes in binary mode on Windows.
//   OFF by default, which is the opposite of file_desc's choice and is
// deliberate. A pipe usually carries a child process's TEXT output, and on
// Windows that output has "\r\n" line endings; text mode turns them back into
// "\n" so fgets and friends behave as they do on POSIX. Turn it on when the
// child emits binary -- a compressed stream, an image -- where translating
// 0x0D 0x0A into 0x0A is data corruption.
//   No effect on POSIX.
#ifndef D_CFG_FILE_PIPE_BINARY
    #define D_CFG_FILE_PIPE_BINARY 0
#endif  // D_CFG_FILE_PIPE_BINARY


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_PIPE_BINARY)
    #error "D_CFG_FILE_PIPE_BINARY must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_PIPE_BINARY
//   resolved: 1 when d_pipe_open appends 'b' to the mode on Windows.
#if ( (D_CFG_IS_ON(D_CFG_FILE_PIPE_BINARY)) &&                                 \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #define D_INTERNAL_FILE_PIPE_BINARY 1
#else
    #define D_INTERNAL_FILE_PIPE_BINARY 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_PIPE_IS_AVAILABLE
//   query: 1 when this build compiles the pipe API. Safe in #if.
#define D_FILE_PIPE_IS_AVAILABLE D_INTERNAL_FILE_HAS_PIPES


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_PIPE_H
