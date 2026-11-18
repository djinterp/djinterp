/*******************************************************************************
* djinterp [config]                                             cfg_option_env.h
*
* Configuration for option_env.h and option_env_registry.h: the room the
* program's djin registry has, and what an import does with a value it could
* not bind.
*   The registry is the program's root frame (option_env_registry.h): it
* lists every djin, and holds the program-wide settings -- from files, the
* environment and the command line -- of the variables it declares itself.
* Its storage is static, sized here, so a build that never uses the registry
* pays for it only if it links option_env_registry.c. A djin's own frame is
* its module's, sized by the module's configuration.
*   targets:  c/option/option_env_registry.h ->
*                 D_CFG_OPTION_ENV_REGISTRY_BINDINGS,
*                 D_CFG_OPTION_ENV_REGISTRY_VARIABLES,
*                 D_CFG_OPTION_ENV_REGISTRY_TEXT,
*                 D_CFG_OPTION_ENV_REGISTRY_DJINS
*             c/option/option_env.h -> D_CFG_OPTION_ENV_IMPORT_REPORT
*   requires: cfg_common.h
*
*
* path:      /inc/djinterp/config/c/option/cfg_option_env.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  The registry's room
         1.  D_CFG_OPTION_ENV_REGISTRY_BINDINGS
         2.  D_CFG_OPTION_ENV_REGISTRY_VARIABLES
         3.  D_CFG_OPTION_ENV_REGISTRY_TEXT
         4.  D_CFG_OPTION_ENV_REGISTRY_DJINS
    2.  Importing
         1.  D_CFG_OPTION_ENV_IMPORT_REPORT
2.  VALIDATION
    ----------
    1.  Ranges
*/

#ifndef DJINTERP_CONFIG_C_OPTION_CFG_OPTION_ENV_H
#define DJINTERP_CONFIG_C_OPTION_CFG_OPTION_ENV_H 1

// djinterp
#include "../../cfg_common.h"  // D_CFG_NORM, user overrides


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    The registry's room
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_OPTION_ENV_REGISTRY_BINDINGS
//   configuration: the bindings the registry's root frame holds at once, one
// per source per variable bound there. 256 by default.
#ifndef D_CFG_OPTION_ENV_REGISTRY_BINDINGS
    #define D_CFG_OPTION_ENV_REGISTRY_BINDINGS 256
#endif  // D_CFG_OPTION_ENV_REGISTRY_BINDINGS

// 1.1.2
// D_CFG_OPTION_ENV_REGISTRY_VARIABLES
//   configuration: the program-wide variables the root frame itself can
// declare -- what every djin beneath it sees. 64 by default.
#ifndef D_CFG_OPTION_ENV_REGISTRY_VARIABLES
    #define D_CFG_OPTION_ENV_REGISTRY_VARIABLES 64
#endif  // D_CFG_OPTION_ENV_REGISTRY_VARIABLES

// 1.1.3
// D_CFG_OPTION_ENV_REGISTRY_TEXT
//   configuration: the bytes of the root frame's text arena, which it
// compacts when it fills. 4096 by default.
#ifndef D_CFG_OPTION_ENV_REGISTRY_TEXT
    #define D_CFG_OPTION_ENV_REGISTRY_TEXT 4096
#endif  // D_CFG_OPTION_ENV_REGISTRY_TEXT

// 1.1.4
// D_CFG_OPTION_ENV_REGISTRY_DJINS
//   configuration: the djins the registry lists. 32 by default.
#ifndef D_CFG_OPTION_ENV_REGISTRY_DJINS
    #define D_CFG_OPTION_ENV_REGISTRY_DJINS 32
#endif  // D_CFG_OPTION_ENV_REGISTRY_DJINS


// 1.2    Importing
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_OPTION_ENV_IMPORT_REPORT
//   configuration: what d_option_env_import and d_option_env_registry_import
// do with a value they could not bind, where no report is passed: 0 nothing,
// the first failure returned; 1 the stock report, a line on stderr for each
// (d_option_env_report_print). A library never writes to a program's error
// stream unasked, so 0 by default; import_each takes any report.
#ifndef D_CFG_OPTION_ENV_IMPORT_REPORT
    #define D_CFG_OPTION_ENV_IMPORT_REPORT 0
#endif  // D_CFG_OPTION_ENV_IMPORT_REPORT


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Ranges
//------------------------------------------------------------------------------
#if ( (D_CFG_NORM(D_CFG_OPTION_ENV_REGISTRY_BINDINGS) < 1)  ||                 \
      (D_CFG_NORM(D_CFG_OPTION_ENV_REGISTRY_VARIABLES) < 1) ||                 \
      (D_CFG_NORM(D_CFG_OPTION_ENV_REGISTRY_TEXT) < 1)      ||                 \
      (D_CFG_NORM(D_CFG_OPTION_ENV_REGISTRY_DJINS) < 1) )
    #error "D_CFG_OPTION_ENV_REGISTRY_* sizes must be positive"
#endif

#if ( (D_CFG_OPTION_ENV_IMPORT_REPORT < 0) ||                                 \
      (D_CFG_OPTION_ENV_IMPORT_REPORT > 1) )
    #error "D_CFG_OPTION_ENV_IMPORT_REPORT must be 0 (none) or 1 (stderr)"
#endif


#endif  // DJINTERP_CONFIG_C_OPTION_CFG_OPTION_ENV_H
