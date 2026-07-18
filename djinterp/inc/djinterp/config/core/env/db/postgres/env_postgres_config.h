/******************************************************************************
* djinterp [config][db]                                  env_postgres_config.h
*
* Per-module configuration for env_postgresql.h. Owns all
* D_CFG_ENV_POSTGRESQL_* defaults plus D_CFG_ENV_PG_CUSTOM and the
* pre-defined-detection auto-activation logic for it.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
* 
* path:      /inc/djinterp/config/core/env/db/postgres/env_postgres_config.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.22
******************************************************************************/

#ifndef DJINTERP_CONFIG_ENV_POSTGRESQL_
#define DJINTERP_CONFIG_ENV_POSTGRESQL_ 1

// pull in user-level overrides when requested at the compiler level
#ifdef D_CFG_CUSTOM
    #include "../../../dconfig.h"
#endif


// ===========================================================================
// I.   ENABLE / PATH CONFIGURATION
// ===========================================================================

// D_CFG_ENV_USING_POSTGRESQL
//   configuration: 1 to enable PostgreSQL header inclusion and detection.
#ifndef D_CFG_ENV_USING_POSTGRESQL
    #define D_CFG_ENV_USING_POSTGRESQL 0
#endif

// D_CFG_ENV_POSTGRESQL_C_PATH
//   configuration: include path for the libpq C client header.
#ifndef D_CFG_ENV_POSTGRESQL_C_PATH
    #define D_CFG_ENV_POSTGRESQL_C_PATH <libpq-fe.h>
#endif

// D_CFG_ENV_POSTGRESQL_CPP_PATH
//   configuration: include path for the libpqxx C++ header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_POSTGRESQL_CPP_PATH
    #define D_CFG_ENV_POSTGRESQL_CPP_PATH <pqxx/pqxx>
#endif


// ===========================================================================
// II.  DETECTION-MODE CONFIGURATION
// ===========================================================================

// D_CFG_ENV_PG_CUSTOM
//   configuration: master PostgreSQL environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_PG_DETECTED_*
//      variables)
#ifndef D_CFG_ENV_PG_CUSTOM
    #define D_CFG_ENV_PG_CUSTOM 0
#endif

// auto-detection: check for pre-defined D_ENV_PG_DETECTED_* variables
// and automatically enable custom mode
#if ( defined(D_ENV_PG_DETECTED_VERSION)  ||  \
      defined(D_ENV_PG_DETECTED_9_4)      ||  \
      defined(D_ENV_PG_DETECTED_9_5)      ||  \
      defined(D_ENV_PG_DETECTED_9_6)      ||  \
      defined(D_ENV_PG_DETECTED_10)       ||  \
      defined(D_ENV_PG_DETECTED_11)       ||  \
      defined(D_ENV_PG_DETECTED_12)       ||  \
      defined(D_ENV_PG_DETECTED_13)       ||  \
      defined(D_ENV_PG_DETECTED_14)       ||  \
      defined(D_ENV_PG_DETECTED_15)       ||  \
      defined(D_ENV_PG_DETECTED_16)       ||  \
      defined(D_ENV_PG_DETECTED_17) )
    #undef  D_CFG_ENV_PG_CUSTOM
    #define D_CFG_ENV_PG_CUSTOM 1
#endif


#endif  // DJINTERP_CONFIG_ENV_POSTGRESQL_
