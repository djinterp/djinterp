/*******************************************************************************
* djinterp [config]                                              cfg_env_mysql.h
*
* Per-module configuration for env_mysql.h (Oracle MySQL). Owns all
* D_CFG_ENV_MYSQL_* defaults and the pre-defined-detection auto-activation
* logic for D_CFG_ENV_MYSQL_CUSTOM.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
*
* path:      /inc/djinterp/config/core/env/db/mysql/cfg_env_mysql.h
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
         1.  D_CFG_ENV_USING_MYSQL
         2.  D_CFG_ENV_MYSQL_C_PATH
         3.  D_CFG_ENV_MYSQL_CPP_PATH
2.  DETECTION MODE
    --------------
    1.  Client detection
         1.  D_CFG_ENV_MYSQL_CUSTOM
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_MYSQL_CFG_ENV_MYSQL_H
#define DJINTERP_CONFIG_CORE_ENV_DB_MYSQL_CFG_ENV_MYSQL_H 1

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
// D_CFG_ENV_USING_MYSQL
//   configuration: 1 to enable Oracle MySQL header inclusion and detection.
// When 0 (default), env_mysql.h produces no vendor includes, no detection,
// and no vendor-symbol references.
#ifndef D_CFG_ENV_USING_MYSQL
    #define D_CFG_ENV_USING_MYSQL 0
#endif  // D_CFG_ENV_USING_MYSQL

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_MYSQL)
    #error "D_CFG_ENV_USING_MYSQL must be 0 or 1"
#endif

// 1.1.2
// D_CFG_ENV_MYSQL_C_PATH
//   configuration: include path for the MySQL C client header. <mysql.h> by
// default: the header in the directory the client's own flags put on the
// include path (mysql_config --cflags), so the linked client's; where it is
// not found, env_mysql_common.h tries <mysql/mysql.h>, under __has_include.
#ifndef D_CFG_ENV_MYSQL_C_PATH
    #define D_CFG_ENV_MYSQL_C_PATH <mysql.h>
#endif  // D_CFG_ENV_MYSQL_C_PATH

// 1.1.3
// D_CFG_ENV_MYSQL_CPP_PATH
//   configuration: include path for the MySQL X DevAPI C++ header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_MYSQL_CPP_PATH
    #define D_CFG_ENV_MYSQL_CPP_PATH <mysqlx/xdevapi.h>
#endif  // D_CFG_ENV_MYSQL_CPP_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_MYSQL_CUSTOM
//   configuration: master MySQL environment detection control flag.
// values:
//   0 (default): perform full automatic detection via vendor macros
//   1: skip all detection (requires pre-defined D_ENV_MYSQL_DETECTED_*
//      variables)
// A pre-defined D_ENV_*_DETECTED_* result makes 1 the default, and an
// explicit setting still wins (decision 118 of the register).
#ifndef D_CFG_ENV_MYSQL_CUSTOM
    #if ( (defined(D_ENV_MYSQL_DETECTED_VERSION))        ||                    \
          (defined(D_ENV_MYSQL_DETECTED_CONNECTOR_C))    ||                    \
          (defined(D_ENV_MYSQL_DETECTED_LIBMYSQLCLIENT)) ||                    \
          (defined(D_ENV_MYSQL_DETECTED_5_5))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_5_6))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_5_7))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_8_0))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_8_4))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_0))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_1))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_2))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_3))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_4))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_5))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_6))            ||                    \
          (defined(D_ENV_MYSQL_DETECTED_9_7)) )
        #define D_CFG_ENV_MYSQL_CUSTOM 1
    #else
        #define D_CFG_ENV_MYSQL_CUSTOM 0
    #endif
#endif  // D_CFG_ENV_MYSQL_CUSTOM

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_MYSQL_CUSTOM)
    #error "D_CFG_ENV_MYSQL_CUSTOM must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_MYSQL_CFG_ENV_MYSQL_H
