/*******************************************************************************
* djinterp [config]                                              cfg_file_link.h
*
* Build-time configuration for c/fs/file_link.h -- symbolic links.
*
*   The gate that matters is D_INTERNAL_FILE_HAS_SYMLINKS from
* cfg_file_common.h, which is a COMPILE-time claim about the platform. It is
* not a runtime permission: Windows has had symlinks since Vista and still
* refuses to create one without SeCreateSymbolicLinkPrivilege, which an
* ordinary account does not have unless Developer Mode is on.
*
*   targets:  c/fs/file_link.h, c/fs/file_link.c -> D_INTERNAL_FILE_LINK_UNPRIV
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_link.h
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
         1.  D_CFG_FILE_LINK_WIN32_UNPRIVILEGED
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_LINK_UNPRIV
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_LINK_IS_AVAILABLE
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_LINK_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_LINK_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Creation
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_LINK_WIN32_UNPRIVILEGED
//   knob: pass SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE on Windows, so
// symlink creation works without administrator rights when Developer Mode is
// enabled.
//   ON. The flag is harmless where it is not honoured -- older Windows 10
// builds reject the whole call with ERROR_INVALID_PARAMETER, which this module
// detects and retries without it. Without the flag, d_file_symlink simply fails
// for every ordinary user, which is why the old dfile test suite had to probe
// at runtime and skip itself.
#ifndef D_CFG_FILE_LINK_WIN32_UNPRIVILEGED
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_LINK_WIN32_UNPRIVILEGED D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_LINK_WIN32_UNPRIVILEGED 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_LINK_WIN32_UNPRIVILEGED


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_LINK_WIN32_UNPRIVILEGED)
    #error "D_CFG_FILE_LINK_WIN32_UNPRIVILEGED must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_LINK_UNPRIV
//   resolved: 1 when d_file_symlink asks for unprivileged creation.
#if ( (D_CFG_IS_ON(D_CFG_FILE_LINK_WIN32_UNPRIVILEGED)) &&                     \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #define D_INTERNAL_FILE_LINK_UNPRIV 1
#else
    #define D_INTERNAL_FILE_LINK_UNPRIV 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_LINK_IS_AVAILABLE
//   query: 1 when this build compiles the symbolic-link API. A COMPILE-time
// claim: creating one may still fail at runtime for want of privilege. Safe
// in #if.
#define D_FILE_LINK_IS_AVAILABLE D_INTERNAL_FILE_HAS_SYMLINKS


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_LINK_H
