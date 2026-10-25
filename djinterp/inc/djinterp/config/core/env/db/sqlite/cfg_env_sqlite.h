/*******************************************************************************
* djinterp [config]                                             cfg_env_sqlite.h
*
* Per-module configuration for env_sqlite.h. Owns all D_CFG_ENV_SQLITE_*
* defaults and the pre-defined-detection auto-activation logic for
* D_CFG_ENV_SQLITE_CUSTOM.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
*   NOTE: SQLite has no official C++ header in the standard distribution,
* so only C_PATH is provided.
*
*
* path:      /inc/djinterp/config/core/env/db/sqlite/cfg_env_sqlite.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.22
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION CONTROL
    -------------------------------
    1.  Client header
         1.  D_CFG_ENV_USING_SQLITE
         2.  D_CFG_ENV_SQLITE_C_PATH
2.  DETECTION MODE
    --------------
    1.  Client detection
         1.  D_CFG_ENV_SQLITE_CUSTOM
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_SQLITE_CFG_ENV_SQLITE_H
#define DJINTERP_CONFIG_CORE_ENV_DB_SQLITE_CFG_ENV_SQLITE_H 1

// cfg_common.h, never the dconfig.h umbrella: it picks up user overrides
// itself, and the umbrella, reached from here, re-entered env detection
// before it had finished
#include "../../../../cfg_common.h"  // D_CFG_* helpers, user overrides


//==============================================================================
// 1.  VENDOR HEADER INCLUSION CONTROL
//==============================================================================


// 1.1    Client header
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_USING_SQLITE
//   configuration: 1 to enable SQLite header inclusion and detection.
#ifndef D_CFG_ENV_USING_SQLITE
    #define D_CFG_ENV_USING_SQLITE 0
#endif  // D_CFG_ENV_USING_SQLITE

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_SQLITE)
    #error "D_CFG_ENV_USING_SQLITE must be 0 or 1"
#endif

// 1.1.2
// D_CFG_ENV_SQLITE_C_PATH
//   configuration: include path for the SQLite C header.
#ifndef D_CFG_ENV_SQLITE_C_PATH
    #define D_CFG_ENV_SQLITE_C_PATH <sqlite3.h>
#endif  // D_CFG_ENV_SQLITE_C_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_SQLITE_CUSTOM
//   configuration: master SQLite environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_SQLITE_DETECTED_*
//      variables)
// A pre-defined D_ENV_*_DETECTED_* result makes 1 the default, and an
// explicit setting still wins (decision 118 of the register).
#ifndef D_CFG_ENV_SQLITE_CUSTOM
    #if ( (defined(D_ENV_SQLITE_DETECTED_VERSION)) ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_7))     ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_8))     ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_9))     ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_24))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_25))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_31))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_35))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_36))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_37))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_38))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_39))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_40))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_43))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_45))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_46))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_47))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_48))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_49))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_50))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_51))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_53))    ||                          \
          (defined(D_ENV_SQLITE_DETECTED_3_54)) )
        #define D_CFG_ENV_SQLITE_CUSTOM 1
    #else
        #define D_CFG_ENV_SQLITE_CUSTOM 0
    #endif
#endif  // D_CFG_ENV_SQLITE_CUSTOM

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_SQLITE_CUSTOM)
    #error "D_CFG_ENV_SQLITE_CUSTOM must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_SQLITE_CFG_ENV_SQLITE_H
