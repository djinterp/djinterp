/*******************************************************************************
* djinterp [config]                                             cfg_file_space.h
*
* Build-time configuration for c/fs/file_space.h -- filesystem capacity.
*
*   targets:  c/fs/file_space.h, c/fs/file_space.c ->
*             D_INTERNAL_FILE_SPACE_AVAILABLE
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_space.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Availability
         1.  D_CFG_FILE_HAS_STATVFS
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_SPACE_AVAILABLE
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_SPACE_IS_AVAILABLE
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_SPACE_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_SPACE_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Availability
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_HAS_STATVFS
//   knob: the platform provides statvfs(3).
//   POSIX.1-2001, so essentially everywhere that is not Windows. Windows uses
// GetDiskFreeSpaceExA instead, which reports the same three numbers.
#ifndef D_CFG_FILE_HAS_STATVFS
    #if D_CFG_IS_ON(D_CFG_FILE_HAS_POSIX)
        #define D_CFG_FILE_HAS_STATVFS 1
    #else
        #define D_CFG_FILE_HAS_STATVFS 0
    #endif
#endif  // D_CFG_FILE_HAS_STATVFS


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_HAS_STATVFS)
    #error "D_CFG_FILE_HAS_STATVFS must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_SPACE_AVAILABLE
//   resolved: 1 when d_file_space can answer at all. There is no emulation: a
// filesystem's capacity is not derivable from anything else this subframework
// can see, so where the platform will not say, d_file_space reports ENOSYS
// rather than inventing a number.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_STATVFS)) ||                                 \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #define D_INTERNAL_FILE_SPACE_AVAILABLE 1
#else
    #define D_INTERNAL_FILE_SPACE_AVAILABLE 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_SPACE_IS_AVAILABLE
//   query: 1 when d_file_space works on this build. Safe in #if.
#define D_FILE_SPACE_IS_AVAILABLE D_INTERNAL_FILE_SPACE_AVAILABLE


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_SPACE_H
