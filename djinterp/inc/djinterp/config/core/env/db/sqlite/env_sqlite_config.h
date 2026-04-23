/******************************************************************************
* djinterp [core]                                          env_sqlite_config.h
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
* path:      /inc/djinterp/config/core/env/db/env_sqlite_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.22
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENV_SQLITE_
#define DJINTERP_CONFIG_ENV_SQLITE_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../../dconfig.h"
#endif


// =============================================================================
// I.   ENABLE / PATH CONFIGURATION
// =============================================================================

// D_CFG_ENV_USING_SQLITE
//   configuration: 1 to enable SQLite header inclusion and detection.
#ifndef D_CFG_ENV_USING_SQLITE
    #define D_CFG_ENV_USING_SQLITE 0
#endif

// D_CFG_ENV_SQLITE_C_PATH
//   configuration: include path for the SQLite C header.
#ifndef D_CFG_ENV_SQLITE_C_PATH
    #define D_CFG_ENV_SQLITE_C_PATH <sqlite3.h>
#endif


// =============================================================================
// II.  DETECTION-MODE CONFIGURATION
// =============================================================================

// D_CFG_ENV_SQLITE_CUSTOM
//   configuration: master SQLite environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_SQLITE_DETECTED_*
//      variables)
#ifndef D_CFG_ENV_SQLITE_CUSTOM
    #define D_CFG_ENV_SQLITE_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_SQLITE_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_SQLITE_DETECTED_VERSION)  ||  \
      defined(D_ENV_SQLITE_DETECTED_3_7)      ||  \
      defined(D_ENV_SQLITE_DETECTED_3_8)      ||  \
      defined(D_ENV_SQLITE_DETECTED_3_9)      ||  \
      defined(D_ENV_SQLITE_DETECTED_3_24)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_25)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_31)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_35)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_36)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_37)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_38)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_39)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_40)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_43)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_45)     ||  \
      defined(D_ENV_SQLITE_DETECTED_3_46) )
    #undef  D_CFG_ENV_SQLITE_CUSTOM
    #define D_CFG_ENV_SQLITE_CUSTOM 1
#endif


#endif  // DJINTERP_CONFIG_ENV_SQLITE_
