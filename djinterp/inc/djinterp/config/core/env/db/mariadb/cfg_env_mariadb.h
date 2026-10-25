/*******************************************************************************
* djinterp [config]                                            cfg_env_mariadb.h
*
* Per-module configuration for env_mariadb.h. Owns all D_CFG_ENV_MARIADB_*
* defaults and the pre-defined-detection auto-activation logic for
* D_CFG_ENV_MARIADB_CUSTOM.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
*
* path:      /inc/djinterp/config/core/env/db/mariadb/cfg_env_mariadb.h
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
         1.  D_CFG_ENV_USING_MARIADB
         2.  D_CFG_ENV_MARIADB_C_PATH
         3.  D_CFG_ENV_MARIADB_CPP_PATH
2.  DETECTION MODE
    --------------
    1.  Client detection
         1.  D_CFG_ENV_MARIADB_CUSTOM
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_MARIADB_CFG_ENV_MARIADB_H
#define DJINTERP_CONFIG_CORE_ENV_DB_MARIADB_CFG_ENV_MARIADB_H 1

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
// D_CFG_ENV_USING_MARIADB
//   configuration: 1 to enable MariaDB header inclusion and detection.
// When 0 (default), env_mariadb.h produces no vendor includes, no detection,
// and no vendor-symbol references.
#ifndef D_CFG_ENV_USING_MARIADB
    #define D_CFG_ENV_USING_MARIADB 0
#endif  // D_CFG_ENV_USING_MARIADB

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_MARIADB)
    #error "D_CFG_ENV_USING_MARIADB must be 0 or 1"
#endif

// 1.1.2
// D_CFG_ENV_MARIADB_C_PATH
//   configuration: include path for the MariaDB/MySQL C client header.
// <mysql.h> by default: the header in the directory the client's own flags
// put on the include path (mariadb_config --cflags, mysql_config --cflags),
// so the header of the library the build links, where both clients are
// installed and lay struct MYSQL out differently. Where it is not found,
// env_mysql_common.h tries <mariadb/mysql.h>, then <mysql/mysql.h>, under
// __has_include. Override if the headers live elsewhere.
#ifndef D_CFG_ENV_MARIADB_C_PATH
    #define D_CFG_ENV_MARIADB_C_PATH <mysql.h>
#endif  // D_CFG_ENV_MARIADB_C_PATH

// 1.1.3
// D_CFG_ENV_MARIADB_CPP_PATH
//   configuration: include path for the MariaDB C++ connector header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_MARIADB_CPP_PATH
    #define D_CFG_ENV_MARIADB_CPP_PATH <mariadb/conncpp.hpp>
#endif  // D_CFG_ENV_MARIADB_CPP_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_MARIADB_CUSTOM
//   configuration: master MariaDB environment detection control flag.
// values:
//   0 (default): perform full automatic detection via vendor macros
//   1: skip all detection (requires pre-defined D_ENV_MARIADB_DETECTED_*
//      variables)
// Pre-defining any D_ENV_MARIADB_DETECTED_* variable automatically enables
// custom mode below.
// A pre-defined D_ENV_*_DETECTED_* result makes 1 the default, and an
// explicit setting still wins (decision 118 of the register).
#ifndef D_CFG_ENV_MARIADB_CUSTOM
    #if ( (defined(D_ENV_MARIADB_DETECTED_VERSION)) ||                         \
          (defined(D_ENV_MARIADB_DETECTED_5_5))     ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_0))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_1))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_2))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_3))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_4))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_5))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_6))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_10_11))   ||                         \
          (defined(D_ENV_MARIADB_DETECTED_11_0))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_11_4))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_11_8))    ||                         \
          (defined(D_ENV_MARIADB_DETECTED_12_3)) )
        #define D_CFG_ENV_MARIADB_CUSTOM 1
    #else
        #define D_CFG_ENV_MARIADB_CUSTOM 0
    #endif
#endif  // D_CFG_ENV_MARIADB_CUSTOM

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_MARIADB_CUSTOM)
    #error "D_CFG_ENV_MARIADB_CUSTOM must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_MARIADB_CFG_ENV_MARIADB_H
