/*******************************************************************************
* djinterp [config]                                        cfg_postgres_common.h
*
* Build-time configuration shared by every module of djinterp's PostgreSQL
* layer (c/db/postgres/postgres_*.h).
*   One knob decides what the layer is built over: libpq, PostgreSQL's own
* client library, or stubs that report D_DB_UNSUPPORTED, so that code using
* the layer compiles and links where libpq is not installed and decides at
* run time, as the SQLite and MariaDB layers do. It defaults to whether
* libpq-fe.h can be found.
*   Built over libpq, the layer's sources reach libpq-fe.h through
* env_postgres.h, which includes it only when D_CFG_ENV_USING_POSTGRESQL is
* 1. This file therefore sets that knob for the layer's own sources -- each
* defines D_INTERNAL_POSTGRES_SOURCE before its first include -- unless the
* build already has, and only then includes env's configuration, so that
* env's cascade honours it (the config README, 6.2). A build that asks for
* libpq but sets that knob to 0, or includes dconfig.h first, stops at the
* layer's sources with an #error; it sets D_CFG_ENV_USING_POSTGRESQL=1
* itself.
*   Nothing else that includes the layer, core/db's pg_connection among it,
* has libpq-fe.h in scope, unless the program asks for env's detection
* itself. The public headers never include it: on Windows it spells `long
* long`, which ISO C++98 refuses. They name libpq's connection by its struct
* tag, struct pg_conn, which every libpq has given PGconn.
*   libpq-fe.h declares no version, so the client floor below is checked at
* compile time only when env's manual detection names a version
* (D_ENV_PG_DETECTED_VERSION); d_postgres_client_version_number reports the
* library a program actually loads.
*
*   targets:  c/db/postgres/postgres_*.h and their sources ->
*             D_INTERNAL_POSTGRES, D_POSTGRES_FLOOR_*
*   requires: cfg_common.h, cfg_env_postgres.h
*
*
* path:      /inc/djinterp/config/c/db/postgres/cfg_postgres_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BINDING
    -------
    1.  Environment-defaulted enable
         1.  D_CFG_POSTGRES
2.  DEPENDENCIES
    ------------
    1.  Client detection
         1.  D_CFG_ENV_USING_POSTGRESQL
3.  VALIDATION
    ----------
    1.  Knob validation
4.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_POSTGRES
5.  QUERIES
    -------
    1.  Floors
         1.  D_POSTGRES_FLOOR_CLIENT_VERSION_NUM
         2.  D_POSTGRES_FLOOR_SERVER_VERSION_NUM
*/

#ifndef DJINTERP_CONFIG_C_DB_POSTGRES_CFG_POSTGRES_COMMON_H
#define DJINTERP_CONFIG_C_DB_POSTGRES_CFG_POSTGRES_COMMON_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_IS_ON, D_CFG_IS_BOOL, user overrides


//==============================================================================
// 1.  BINDING
//==============================================================================
// Whether the layer runs over libpq. Without it every function reports
// D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail.


// 1.1    Environment-defaulted enable
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_POSTGRES
//   knob: build the layer over libpq (1) or as stubs (0). Defaults to 1 where
// libpq-fe.h can be found -- at D_CFG_ENV_POSTGRESQL_C_PATH when the build
// names one, else as <libpq-fe.h> or <postgresql/libpq-fe.h>, where env
// looks -- and to 0 where it cannot; to 1 where the preprocessor has no
// __has_include to look with. Define it as 1 to require libpq, which stops a
// build that lacks it with an #error, or as 0 to build the stubs whatever is
// installed.
#ifndef D_CFG_POSTGRES
    #if defined(__has_include)
        #if defined(D_CFG_ENV_POSTGRESQL_C_PATH)
            #if __has_include(D_CFG_ENV_POSTGRESQL_C_PATH)
                #define D_CFG_POSTGRES 1
            #else
                #define D_CFG_POSTGRES 0
            #endif
        #elif __has_include(<libpq-fe.h>)
            #define D_CFG_POSTGRES 1
        #elif __has_include(<postgresql/libpq-fe.h>)
            #define D_CFG_POSTGRES 1
        #else
            #define D_CFG_POSTGRES 0
        #endif
    #else
        #define D_CFG_POSTGRES 1
    #endif
#endif  // D_CFG_POSTGRES


//==============================================================================
// 2.  DEPENDENCIES
//==============================================================================


// 2.1    Client detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_USING_POSTGRESQL
//   knob (env's): turned on here for the layer's own sources, when the layer
// is built over libpq, before env's own default can turn it off, so that
// they find libpq-fe.h. A source says it is one by defining
// D_INTERNAL_POSTGRES_SOURCE before its first include; everything else keeps
// env's default. A value the build gave first wins.
#if ( (D_CFG_IS_ON(D_CFG_POSTGRES)) &&                                         \
      (defined(D_INTERNAL_POSTGRES_SOURCE)) )
    #ifndef D_CFG_ENV_USING_POSTGRESQL
        #define D_CFG_ENV_USING_POSTGRESQL 1
    #endif  // D_CFG_ENV_USING_POSTGRESQL
#endif

// djinterp
#include "../../../core/env/db/postgres/cfg_env_postgres.h"  // env's knobs


//==============================================================================
// 3.  VALIDATION
//==============================================================================


// 3.1    Knob validation
//------------------------------------------------------------------------------
// validation: #if reads the knob as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_POSTGRES)
    #error "D_CFG_POSTGRES must be 0 or 1"
#endif


//==============================================================================
// 4.  RESOLVED VALUES
//==============================================================================


// 4.1    Effective values
//------------------------------------------------------------------------------
// 4.1.1
// D_INTERNAL_POSTGRES
//   brief: 1 when the layer is built over libpq, 0 when it is built as stubs;
// the sources choose their implementation by it.
#if D_CFG_IS_ON(D_CFG_POSTGRES)
    #define D_INTERNAL_POSTGRES 1
#else
    #define D_INTERNAL_POSTGRES 0
#endif


//==============================================================================
// 5.  QUERIES
//==============================================================================


// 5.1    Floors
//------------------------------------------------------------------------------
// Versions are PostgreSQL's own numbers, as PG_VERSION_NUM, PQlibVersion and
// PQserverVersion spell them: MAJOR*10000 + MINOR from PostgreSQL 10 on, so
// 16.4 is 160004. Safe in #if.

// 5.1.1
// D_POSTGRES_FLOOR_CLIENT_VERSION_NUM
//   query: the oldest libpq the layer is written and tested for, 10. It calls
// nothing libpq 9.1 lacked; the sources stop with an #error below this floor
// where env's manual detection names an older version.
#define D_POSTGRES_FLOOR_CLIENT_VERSION_NUM 100000

// 5.1.2
// D_POSTGRES_FLOOR_SERVER_VERSION_NUM
//   query: the oldest server the layer is written and tested for, 10, the
// series RHEL 8 ships by default. A request an older server, or one between
// this floor and the feature's release, cannot carry out is refused with
// D_DB_UNSUPPORTED.
#define D_POSTGRES_FLOOR_SERVER_VERSION_NUM 100000


#endif  // DJINTERP_CONFIG_C_DB_POSTGRES_CFG_POSTGRES_COMMON_H
