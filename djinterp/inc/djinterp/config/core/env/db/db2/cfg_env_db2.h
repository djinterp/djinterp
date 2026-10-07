/*******************************************************************************
* djinterp [config]                                                cfg_env_db2.h
*
* djinterp IBM Db2 detection configuration.
*   The settings env_db2.h reads: whether the Db2 CLI/ODBC client header is
* included and where to find it, whether automatic client detection is
* bypassed, which target server version feature gating assumes, and which
* platform family the target belongs to.
*   Every setting is #ifndef-guarded: define it before this header, for
* instance with -D, in cfg_custom.h or in a project-wide configuration header,
* to override it.
*   In manual mode (D_CFG_ENV_DB2_CUSTOM set to 1), env_db2.h takes the client
* version from D_ENV_DB2_DETECTED_CLIENT_VERSION. The target server version
* and platform are always manual: D_CFG_ENV_DB2_SERVER_VERSION or
* D_ENV_DB2_DETECTED_SERVER_*, and D_CFG_ENV_DB2_PLATFORM or
* D_ENV_DB2_DETECTED_ZOS and D_ENV_DB2_DETECTED_ISERIES.
*   targets:  env/db/db2/env_db2.h -> D_CFG_ENV_*
*   requires: cfg_common.h (D_CFG_IS_BOOL, user overrides, testing preset)
*
*
* path:      /inc/djinterp/config/core/env/db/db2/cfg_env_db2.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION CONTROL
    -------------------------------
    1.  Client header
         1.  D_CFG_ENV_USING_DB2
         2.  D_CFG_ENV_DB2_C_PATH
2.  DETECTION MODE
    --------------
    1.  Client detection
         1.  D_CFG_ENV_DB2_CUSTOM
3.  TARGET SERVER VERSION
    ---------------------
    1.  Server version
         1.  D_CFG_ENV_DB2_SERVER_VERSION
4.  TARGET PLATFORM FAMILY
    ----------------------
    1.  Platform family
         1.  D_CFG_ENV_DB2_PLATFORM_<FAMILY>
         2.  D_CFG_ENV_DB2_PLATFORM
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_DB2_CFG_ENV_DB2_H
#define DJINTERP_CONFIG_CORE_ENV_DB_DB2_CFG_ENV_DB2_H 1

// djinterp
#include "../../../../cfg_common.h"  // D_CFG_IS_BOOL, overrides, testing preset


//==============================================================================
// 1.  VENDOR HEADER INCLUSION CONTROL
//==============================================================================


// 1.1    Client header
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_ENV_USING_DB2
//   configuration: 1 to include the Db2 CLI/ODBC client header (sqlcli1.h) and
// detect the client at compile time. When 0, no Db2 symbol is referenced and
// the client reads as absent; server-version feature gating still works if
// configured.
#ifndef D_CFG_ENV_USING_DB2
    #define D_CFG_ENV_USING_DB2 0
#endif  // D_CFG_ENV_USING_DB2

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_DB2)
    #error "D_CFG_ENV_USING_DB2 must be 0 or 1"
#endif

// 1.1.2
// D_CFG_ENV_DB2_C_PATH
//   configuration: the include path of the CLI header. It is tried first, and
// included unconditionally where __has_include is unavailable. With a
// standard client or driver install, leave the default and rely on
// env_db2.h's <sqlcli1.h> fallback.
#ifndef D_CFG_ENV_DB2_C_PATH
    #define D_CFG_ENV_DB2_C_PATH <sqlcli1.h>
#endif  // D_CFG_ENV_DB2_C_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_DB2_CUSTOM
//   configuration: 0 (the default) for automatic client detection; 1 to skip
// it and use a pre-defined D_ENV_DB2_DETECTED_CLIENT_VERSION instead.
#ifndef D_CFG_ENV_DB2_CUSTOM
    #define D_CFG_ENV_DB2_CUSTOM 0
#endif  // D_CFG_ENV_DB2_CUSTOM

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_DB2_CUSTOM)
    #error "D_CFG_ENV_DB2_CUSTOM must be 0 or 1"
#endif


//==============================================================================
// 3.  TARGET SERVER VERSION
//==============================================================================
// Feature gating by server version.


// 3.1    Server version
//------------------------------------------------------------------------------
// 3.1.1
// D_CFG_ENV_DB2_SERVER_VERSION
//   configuration: the target server version, encoded as MAJOR*10000 +
// MINOR*100 + PATCH; for Db2 LUW 11.5, define it as 110500. The server
// version is a runtime property, so there is no default; without one,
// server-gated features read 0. The D_ENV_DB2_DETECTED_SERVER_* shortcuts in
// env_db2.h work as well.


//==============================================================================
// 4.  TARGET PLATFORM FAMILY
//==============================================================================
// Db2 is three related products with divergent version lines and feature
// sets: Db2 for Linux, UNIX and Windows (LUW), Db2 for z/OS, and Db2 for i
// (formerly DB2/400, on IBM i). The platform is a deployment property, so it
// must be declared for feature gating to branch correctly.


// 4.1    Platform family
//------------------------------------------------------------------------------
// 4.1.1
// D_CFG_ENV_DB2_PLATFORM_<FAMILY>
//   constant: the platform-family values: D_CFG_ENV_DB2_PLATFORM_LUW (0),
// D_CFG_ENV_DB2_PLATFORM_ZOS (1) and D_CFG_ENV_DB2_PLATFORM_ISERIES (2).
#define D_CFG_ENV_DB2_PLATFORM_LUW      0
#define D_CFG_ENV_DB2_PLATFORM_ZOS      1
#define D_CFG_ENV_DB2_PLATFORM_ISERIES  2

// 4.1.2
// D_CFG_ENV_DB2_PLATFORM
//   configuration: the target platform family, one of the values above: LUW
// by default, or z/OS or IBM i when D_ENV_DB2_DETECTED_ZOS or
// D_ENV_DB2_DETECTED_ISERIES is defined.
#ifndef D_CFG_ENV_DB2_PLATFORM
    #if defined(D_ENV_DB2_DETECTED_ZOS)
        #define D_CFG_ENV_DB2_PLATFORM D_CFG_ENV_DB2_PLATFORM_ZOS
    #elif defined(D_ENV_DB2_DETECTED_ISERIES)
        #define D_CFG_ENV_DB2_PLATFORM D_CFG_ENV_DB2_PLATFORM_ISERIES
    #else
        #define D_CFG_ENV_DB2_PLATFORM D_CFG_ENV_DB2_PLATFORM_LUW
    #endif
#endif  // D_CFG_ENV_DB2_PLATFORM

// validation: the switch must name one of its three values
#if ( (D_CFG_ENV_DB2_PLATFORM != D_CFG_ENV_DB2_PLATFORM_LUW) &&                \
      (D_CFG_ENV_DB2_PLATFORM != D_CFG_ENV_DB2_PLATFORM_ZOS) &&                \
      (D_CFG_ENV_DB2_PLATFORM != D_CFG_ENV_DB2_PLATFORM_ISERIES) )
    #error "D_CFG_ENV_DB2_PLATFORM must be one of its _PLATFORM_* values"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_DB2_CFG_ENV_DB2_H
