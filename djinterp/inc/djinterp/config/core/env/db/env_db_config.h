/******************************************************************************
* djinterp [core]                                              env_db_config.h
*
* Per-module configuration for env_db.h. Owns D_CFG_ENV_DB_CUSTOM and the
* pre-defined-detection auto-activation logic for it.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
* 
* path:      /inc/djinterp/config/core/env/db/env_db_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.22
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENV_DB_
#define DJINTERP_CONFIG_ENV_DB_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../../dconfig.h"
#endif


// D_CFG_ENV_DB_CUSTOM
//   configuration: master database environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_DB_DETECTED_*
//      variables)
// Pre-defining any D_ENV_DB_DETECTED_* variable automatically enables
// custom mode below.
#ifndef D_CFG_ENV_DB_CUSTOM
    #define D_CFG_ENV_DB_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_DB_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_DB_DETECTED_MARIADB)     ||  \
      defined(D_ENV_DB_DETECTED_MYSQL)       ||  \
      defined(D_ENV_DB_DETECTED_POSTGRESQL)  ||  \
      defined(D_ENV_DB_DETECTED_SQLITE)      ||  \
      defined(D_ENV_DB_DETECTED_MONGODB)     ||  \
      defined(D_ENV_DB_DETECTED_REDIS)       ||  \
      defined(D_ENV_DB_DETECTED_ARANGODB)    ||  \
      defined(D_ENV_DB_DETECTED_ORACLE)      ||  \
      defined(D_ENV_DB_DETECTED_MSSQL)       ||  \
      defined(D_ENV_DB_DETECTED_DB2)         ||  \
      defined(D_ENV_DB_DETECTED_FIREBASE)    ||  \
      defined(D_ENV_DB_DETECTED_CASSANDRA)   ||  \
      defined(D_ENV_DB_DETECTED_COUCHDB)     ||  \
      defined(D_ENV_DB_DETECTED_NEO4J)       ||  \
      defined(D_ENV_DB_DETECTED_UNKNOWN) )
    #undef  D_CFG_ENV_DB_CUSTOM
    #define D_CFG_ENV_DB_CUSTOM 1
#endif


#endif  // DJINTERP_CONFIG_ENV_DB_
