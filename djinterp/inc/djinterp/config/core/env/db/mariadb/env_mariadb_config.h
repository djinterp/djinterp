/******************************************************************************
* djinterp [db][config]                                   env_mariadb_config.h
*
* Per-module configuration for env_mariadb.h. Owns all D_CFG_ENV_MARIADB_*
* defaults and the pre-defined-detection auto-activation logic for
* D_CFG_ENV_MARIADB_CUSTOM.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
* 
* path:      /inc/djinterp/config/core/env/db/mariadb/env_mariadb_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.22
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENV_MARIADB_
#define DJINTERP_CONFIG_ENV_MARIADB_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../../dconfig.h"
#endif


// =============================================================================
// I.   ENABLE / PATH CONFIGURATION
// =============================================================================

// D_CFG_ENV_USING_MARIADB
//   configuration: 1 to enable MariaDB header inclusion and detection.
// When 0 (default), env_mariadb.h produces no vendor includes, no detection,
// and no vendor-symbol references.
#ifndef D_CFG_ENV_USING_MARIADB
    #define D_CFG_ENV_USING_MARIADB 0
#endif

// D_CFG_ENV_MARIADB_C_PATH
//   configuration: include path for the MariaDB/MySQL C client header.
// Override if your MariaDB Connector/C headers live in a non-standard
// location. Vendor header will probe well-known alternatives if this path
// fails under __has_include.
#ifndef D_CFG_ENV_MARIADB_C_PATH
    #define D_CFG_ENV_MARIADB_C_PATH <mysql/mysql.h>
#endif

// D_CFG_ENV_MARIADB_CPP_PATH
//   configuration: include path for the MariaDB C++ connector header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_MARIADB_CPP_PATH
    #define D_CFG_ENV_MARIADB_CPP_PATH <mariadb/conncpp.hpp>
#endif


// =============================================================================
// II.  DETECTION-MODE CONFIGURATION
// =============================================================================

// D_CFG_ENV_MARIADB_CUSTOM
//   configuration: master MariaDB environment detection control flag.
// values:
//   0 (default): perform full automatic detection via vendor macros
//   1: skip all detection (requires pre-defined D_ENV_MARIADB_DETECTED_*
//      variables)
// Pre-defining any D_ENV_MARIADB_DETECTED_* variable automatically enables
// custom mode below.
#ifndef D_CFG_ENV_MARIADB_CUSTOM
    #define D_CFG_ENV_MARIADB_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_MARIADB_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_MARIADB_DETECTED_VERSION)  ||  \
      defined(D_ENV_MARIADB_DETECTED_5_5)      ||  \
      defined(D_ENV_MARIADB_DETECTED_10_0)     ||  \
      defined(D_ENV_MARIADB_DETECTED_10_1)     ||  \
      defined(D_ENV_MARIADB_DETECTED_10_2)     ||  \
      defined(D_ENV_MARIADB_DETECTED_10_3)     ||  \
      defined(D_ENV_MARIADB_DETECTED_10_4)     ||  \
      defined(D_ENV_MARIADB_DETECTED_10_5)     ||  \
      defined(D_ENV_MARIADB_DETECTED_10_6)     ||  \
      defined(D_ENV_MARIADB_DETECTED_10_11)    ||  \
      defined(D_ENV_MARIADB_DETECTED_11_0)     ||  \
      defined(D_ENV_MARIADB_DETECTED_11_4) )
    #undef  D_CFG_ENV_MARIADB_CUSTOM
    #define D_CFG_ENV_MARIADB_CUSTOM 1
#endif


#endif  // DJINTERP_CONFIG_ENV_MARIADB_
