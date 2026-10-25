/*******************************************************************************
* djinterp [config]                                           cfg_env_postgres.h
*
* Per-module configuration for env_postgresql.h. Owns all
* D_CFG_ENV_POSTGRESQL_* defaults plus D_CFG_ENV_PG_CUSTOM and the
* pre-defined-detection auto-activation logic for it.
*
*   Optionally pulls in dconfig.h for user-level central overrides when
* D_CFG_CUSTOM is defined.
*
*
* path:      /inc/djinterp/config/core/env/db/postgres/cfg_env_postgres.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.22
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION CONTROL
    -------------------------------
    1.  Client header
         1.  D_CFG_ENV_USING_POSTGRESQL
         2.  D_CFG_ENV_POSTGRESQL_C_PATH
         3.  D_CFG_ENV_POSTGRESQL_CPP_PATH
2.  DETECTION MODE
    --------------
    1.  Client detection
         1.  D_CFG_ENV_PG_CUSTOM
*/

#ifndef DJINTERP_CONFIG_CORE_ENV_DB_POSTGRES_CFG_ENV_POSTGRES_H
#define DJINTERP_CONFIG_CORE_ENV_DB_POSTGRES_CFG_ENV_POSTGRES_H 1

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
// D_CFG_ENV_USING_POSTGRESQL
//   configuration: 1 to enable PostgreSQL header inclusion and detection.
#ifndef D_CFG_ENV_USING_POSTGRESQL
    #define D_CFG_ENV_USING_POSTGRESQL 0
#endif  // D_CFG_ENV_USING_POSTGRESQL

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_USING_POSTGRESQL)
    #error "D_CFG_ENV_USING_POSTGRESQL must be 0 or 1"
#endif

// 1.1.2
// D_CFG_ENV_POSTGRESQL_C_PATH
//   configuration: include path for the libpq C client header.
#ifndef D_CFG_ENV_POSTGRESQL_C_PATH
    #define D_CFG_ENV_POSTGRESQL_C_PATH <libpq-fe.h>
#endif  // D_CFG_ENV_POSTGRESQL_C_PATH

// 1.1.3
// D_CFG_ENV_POSTGRESQL_CPP_PATH
//   configuration: include path for the libpqxx C++ header
// (only consulted in C++ builds).
#ifndef D_CFG_ENV_POSTGRESQL_CPP_PATH
    #define D_CFG_ENV_POSTGRESQL_CPP_PATH <pqxx/pqxx>
#endif  // D_CFG_ENV_POSTGRESQL_CPP_PATH


//==============================================================================
// 2.  DETECTION MODE
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_PG_CUSTOM
//   configuration: master PostgreSQL environment detection control flag.
// values:
//   0 (default): perform full automatic detection
//   1: skip all detection (requires pre-defined D_ENV_PG_DETECTED_*
//      variables)
// A pre-defined D_ENV_*_DETECTED_* result makes 1 the default, and an
// explicit setting still wins (decision 118 of the register).
#ifndef D_CFG_ENV_PG_CUSTOM
    #if ( (defined(D_ENV_PG_DETECTED_VERSION)) ||                              \
          (defined(D_ENV_PG_DETECTED_9_4))     ||                              \
          (defined(D_ENV_PG_DETECTED_9_5))     ||                              \
          (defined(D_ENV_PG_DETECTED_9_6))     ||                              \
          (defined(D_ENV_PG_DETECTED_10))      ||                              \
          (defined(D_ENV_PG_DETECTED_11))      ||                              \
          (defined(D_ENV_PG_DETECTED_12))      ||                              \
          (defined(D_ENV_PG_DETECTED_13))      ||                              \
          (defined(D_ENV_PG_DETECTED_14))      ||                              \
          (defined(D_ENV_PG_DETECTED_15))      ||                              \
          (defined(D_ENV_PG_DETECTED_16))      ||                              \
          (defined(D_ENV_PG_DETECTED_17))      ||                              \
          (defined(D_ENV_PG_DETECTED_18))      ||                              \
          (defined(D_ENV_PG_DETECTED_19)) )
        #define D_CFG_ENV_PG_CUSTOM 1
    #else
        #define D_CFG_ENV_PG_CUSTOM 0
    #endif
#endif  // D_CFG_ENV_PG_CUSTOM

// validation: #if reads the switch as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_ENV_PG_CUSTOM)
    #error "D_CFG_ENV_PG_CUSTOM must be 0 or 1"
#endif


#endif  // DJINTERP_CONFIG_CORE_ENV_DB_POSTGRES_CFG_ENV_POSTGRES_H
