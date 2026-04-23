/******************************************************************************
* djinterp [db][config]                                     env_mysql_config.h
*
* Per-module configuration for env_mysql.h (Oracle MySQL). Owns all
* D_CFG_ENV_MYSQL_* defaults and the pre-defined-detection auto-activation
* logic for D_CFG_ENV_MYSQL_CUSTOM.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
* 
* path:      /inc/djinterp/config/core/env/db/mysql/env_mysql_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.22
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENV_MYSQL_
#define DJINTERP_CONFIG_ENV_MYSQL_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../../dconfig.h"
#endif


// =============================================================================
// I.   ENABLE / PATH CONFIGURATION
// =============================================================================

// D_CFG_ENV_USING_MYSQL
//   configuration: 1 to enable Oracle MySQL header inclusion and detection.
// When 0 (default), env_mysql.h produces no vendor includes, no detection,
// and no vendor-symbol references.
#ifndef D_CFG_ENV_USING_MYSQL
    #define D_CFG_ENV_USING_MYSQL 0
#endif

// D_CFG_ENV_MYSQL_C_PATH
//   configuration: include path for the MySQL C client header.
#ifndef D_CFG_ENV_MYSQL_C_PATH
    #define D_CFG_ENV_MYSQL_C_PATH <mysql/mysql.h>
#endif

// D_CFG_ENV_MYSQL_CPP_PATH
//   configuration: include path for the MySQL X DevAPI C++ header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_MYSQL_CPP_PATH
    #define D_CFG_ENV_MYSQL_CPP_PATH <mysqlx/xdevapi.h>
#endif


// =============================================================================
// II.  DETECTION-MODE CONFIGURATION
// =============================================================================

// D_CFG_ENV_MYSQL_CUSTOM
//   configuration: master MySQL environment detection control flag.
// values:
//   0 (default): perform full automatic detection via vendor macros
//   1: skip all detection (requires pre-defined D_ENV_MYSQL_DETECTED_*
//      variables)
#ifndef D_CFG_ENV_MYSQL_CUSTOM
    #define D_CFG_ENV_MYSQL_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_MYSQL_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_MYSQL_DETECTED_VERSION)         ||  \
      defined(D_ENV_MYSQL_DETECTED_CONNECTOR_C)     ||  \
      defined(D_ENV_MYSQL_DETECTED_LIBMYSQLCLIENT)  ||  \
      defined(D_ENV_MYSQL_DETECTED_5_5)             ||  \
      defined(D_ENV_MYSQL_DETECTED_5_6)             ||  \
      defined(D_ENV_MYSQL_DETECTED_5_7)             ||  \
      defined(D_ENV_MYSQL_DETECTED_8_0)             ||  \
      defined(D_ENV_MYSQL_DETECTED_8_4)             ||  \
      defined(D_ENV_MYSQL_DETECTED_9_0)             ||  \
      defined(D_ENV_MYSQL_DETECTED_9_1) )
    #undef  D_CFG_ENV_MYSQL_CUSTOM
    #define D_CFG_ENV_MYSQL_CUSTOM 1
#endif


#endif  // DJINTERP_CONFIG_ENV_MYSQL_
