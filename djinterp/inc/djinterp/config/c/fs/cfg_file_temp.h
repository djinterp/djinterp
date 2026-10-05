/*******************************************************************************
* djinterp [config]                                              cfg_file_temp.h
*
* Build-time configuration for c/fs/file_temp.h -- temporary files.
*
*   Temporary-file APIs are where the classic security bugs live, and the
* reason is always the same: any interface that hands you a NAME and lets you
* open it later has a window in which somebody else creates that name first.
* The knobs here are mostly about narrowing or refusing that window.
*
*   targets:  c/fs/file_temp.h, c/fs/file_temp.c -> D_INTERNAL_FILE_TEMP_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_temp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Security
         1.  D_CFG_FILE_TEMP_MODE
         2.  D_CFG_FILE_TEMP_ALLOW_TMPNAM
    2.  Location
         1.  D_CFG_FILE_TEMP_HONOUR_ENV
         2.  D_CFG_FILE_TEMP_DIR_FALLBACK
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_TEMP_MODE
         2.  D_INTERNAL_FILE_TEMP_TMPNAM
         3.  D_INTERNAL_FILE_TEMP_SUFFIX_LEN
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_TEMP_HAS_TMPNAM
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_TEMP_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_TEMP_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Security
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_TEMP_MODE
//   knob: permission bits for a file d_file_temp_create creates (value model).
//   0600, and do not raise it without a reason. A temporary in a
// world-writable directory is the single most attacked object a program
// creates; anything readable there is readable by everyone on the machine.
// mkstemp is specified to use 0600 for exactly this reason.
#ifndef D_CFG_FILE_TEMP_MODE
    #define D_CFG_FILE_TEMP_MODE 0600
#endif  // D_CFG_FILE_TEMP_MODE

// 1.1.2
// D_CFG_FILE_TEMP_ALLOW_TMPNAM
//   knob: compile d_file_temp_name at all.
//   ON for compatibility, but understand what it is. Any function that
// returns a NAME for you to open later is a TOCTOU race by construction:
// between the name being chosen and your open, an attacker in the same
// directory can create it -- as a symlink to /etc/passwd, classically -- and
// your program writes there instead, with your privileges.
//   d_file_temp_create has no such window: it chooses and opens atomically.
// Every use of d_file_temp_name should be a d_file_temp_create instead. Set
// this to 0 to make the codebase prove it has none left.
#ifndef D_CFG_FILE_TEMP_ALLOW_TMPNAM
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_TEMP_ALLOW_TMPNAM D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_TEMP_ALLOW_TMPNAM 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_TEMP_ALLOW_TMPNAM

// 1.2    Location
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_TEMP_HONOUR_ENV
//   knob: let TMPDIR / TMP / TEMP redirect d_dir_temp.
//   ON, matching every POSIX tool and the C library itself. Note the security
// consequence, which is real but is not fixed by turning this off: an
// attacker who controls the environment controls where your temporaries go.
// Turning it off is right for a set-uid program -- which should not trust the
// environment for anything -- and wrong for everything else, where a user who
// points TMPDIR at a big disk expects to be obeyed.
#ifndef D_CFG_FILE_TEMP_HONOUR_ENV
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_TEMP_HONOUR_ENV D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_TEMP_HONOUR_ENV 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_TEMP_HONOUR_ENV

// 1.2.2
// D_CFG_FILE_TEMP_DIR_FALLBACK
//   knob: the directory d_dir_temp reports when the environment says nothing
// (value model). "/tmp" on POSIX; ignored on Windows, where the API is asked
// instead.
#ifndef D_CFG_FILE_TEMP_DIR_FALLBACK
    #define D_CFG_FILE_TEMP_DIR_FALLBACK "/tmp"
#endif  // D_CFG_FILE_TEMP_DIR_FALLBACK


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_TEMP_ALLOW_TMPNAM)
    #error "D_CFG_FILE_TEMP_ALLOW_TMPNAM must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_TEMP_HONOUR_ENV)
    #error "D_CFG_FILE_TEMP_HONOUR_ENV must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_TEMP_MODE
//   resolved: creation mode for d_file_temp_create (bare constant, #if-safe).
#define D_INTERNAL_FILE_TEMP_MODE D_CFG_FILE_TEMP_MODE

// 3.1.2
// D_INTERNAL_FILE_TEMP_TMPNAM
//   resolved: 1 when d_file_temp_name is compiled.
#if D_CFG_IS_ON(D_CFG_FILE_TEMP_ALLOW_TMPNAM)
    #define D_INTERNAL_FILE_TEMP_TMPNAM 1
#else
    #define D_INTERNAL_FILE_TEMP_TMPNAM 0
#endif

// 3.1.3
// D_INTERNAL_FILE_TEMP_SUFFIX_LEN
//   resolved: the number of trailing 'X' characters a d_file_temp_create
// template must end with. Six, as POSIX specifies.
#define D_INTERNAL_FILE_TEMP_SUFFIX_LEN 6


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_TEMP_HAS_TMPNAM
//   query: 1 when d_file_temp_name exists in this build. Safe in #if.
#define D_FILE_TEMP_HAS_TMPNAM D_INTERNAL_FILE_TEMP_TMPNAM


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_TEMP_H
