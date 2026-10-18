/*******************************************************************************
* djinterp [config]                                          cfg_sqlite_common.h
*
* Build-time configuration shared by every module of djinterp's SQLite layer
* (c/db/sqlite/sqlite_*.h).
*   One knob decides what the layer is built over: SQLite, or stubs that
* report D_DB_UNSUPPORTED, so that code using the layer compiles and links
* where SQLite is absent and decides at run time, as the libcurl binding does.
* It defaults to whether sqlite3.h can be found.
*   Built over SQLite, the layer's sources need sqlite3.h, which reaches them
* through env_sqlite.h, and env_sqlite.h includes it only when
* D_CFG_ENV_USING_SQLITE is 1. This file therefore sets that knob for the
* layer's own sources -- each defines D_INTERNAL_SQLITE_SOURCE before its
* first include -- unless the build already has, and only then includes
* cfg_env_sqlite.h, so that env's cascade honours the value (the config
* README, 6.2). A build that asks for SQLite but sets that knob to 0, or
* includes dconfig.h first (which resolves env's config before this one),
* stops at the layer's sources with an #error; it sets
* D_CFG_ENV_USING_SQLITE=1 itself.
*   Nothing else that includes the layer, core/db's sqlite_connection among
* it, has sqlite3.h in scope, unless the program asks for env's detection
* itself. The public headers never include it: it spells `long long`, which
* ISO C++98 refuses, and the headers are meant to be consumable from C++98
* on.
*
*   targets:  c/db/sqlite/sqlite_*.h and their sources ->
*             D_INTERNAL_SQLITE, D_SQLITE_FLOOR_VERSION_NUMBER
*   requires: cfg_common.h, cfg_env_sqlite.h
*
*
* path:      /inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BINDING
    -------
    1.  Environment-defaulted enable
         1.  D_CFG_SQLITE
2.  DEPENDENCIES
    ------------
    1.  SQLite detection
         1.  D_CFG_ENV_USING_SQLITE
3.  VALIDATION
    ----------
    1.  Knob validation
4.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_SQLITE
5.  QUERIES
    -------
    1.  Version floor
         1.  D_SQLITE_FLOOR_VERSION_NUMBER
*/

#ifndef DJINTERP_CONFIG_C_DB_SQLITE_CFG_SQLITE_COMMON_H
#define DJINTERP_CONFIG_C_DB_SQLITE_CFG_SQLITE_COMMON_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_IS_ON, D_CFG_IS_BOOL, user overrides


//==============================================================================
// 1.  BINDING
//==============================================================================
// Whether the layer runs over SQLite. Without it every function reports
// D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail.


// 1.1    Environment-defaulted enable
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_SQLITE
//   knob: build the layer over SQLite (1) or as stubs (0). Defaults to 1
// where sqlite3.h can be found -- at D_CFG_ENV_SQLITE_C_PATH when the build
// names one, else as <sqlite3.h> -- and to 0 where it cannot; to 1 where the
// preprocessor has no __has_include to look with. Define it as 1 to require
// SQLite, which stops a build that lacks it with an #error, or as 0 to build
// the stubs whatever is installed.
#ifndef D_CFG_SQLITE
    #if defined(__has_include)
        #if defined(D_CFG_ENV_SQLITE_C_PATH)
            #if __has_include(D_CFG_ENV_SQLITE_C_PATH)
                #define D_CFG_SQLITE 1
            #else
                #define D_CFG_SQLITE 0
            #endif
        #elif __has_include(<sqlite3.h>)
            #define D_CFG_SQLITE 1
        #else
            #define D_CFG_SQLITE 0
        #endif
    #else
        #define D_CFG_SQLITE 1
    #endif
#endif  // D_CFG_SQLITE


//==============================================================================
// 2.  DEPENDENCIES
//==============================================================================


// 2.1    SQLite detection
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_ENV_USING_SQLITE
//   knob (env's): turned on here for the layer's own sources, when the layer
// is built over SQLite, before env's own default can turn it off, so that
// they find sqlite3.h. A source says it is one by defining
// D_INTERNAL_SQLITE_SOURCE before its first include; everything else keeps
// env's default. A value the build gave first wins.
#if ( (D_CFG_IS_ON(D_CFG_SQLITE)) &&                                           \
      (defined(D_INTERNAL_SQLITE_SOURCE)) )
    #ifndef D_CFG_ENV_USING_SQLITE
        #define D_CFG_ENV_USING_SQLITE 1
    #endif  // D_CFG_ENV_USING_SQLITE
#endif

// djinterp
#include "../../../core/env/db/sqlite/cfg_env_sqlite.h"  // env's SQLite knobs


//==============================================================================
// 3.  VALIDATION
//==============================================================================


// 3.1    Knob validation
//------------------------------------------------------------------------------
// validation: #if reads the knob as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_SQLITE)
    #error "D_CFG_SQLITE must be 0 or 1"
#endif


//==============================================================================
// 4.  RESOLVED VALUES
//==============================================================================


// 4.1    Effective values
//------------------------------------------------------------------------------
// 4.1.1
// D_INTERNAL_SQLITE
//   brief: 1 when the layer is built over SQLite, 0 when it is built as
// stubs; the sources choose their implementation by it.
#if D_CFG_IS_ON(D_CFG_SQLITE)
    #define D_INTERNAL_SQLITE 1
#else
    #define D_INTERNAL_SQLITE 0
#endif


//==============================================================================
// 5.  QUERIES
//==============================================================================


// 5.1    Version floor
//------------------------------------------------------------------------------
// 5.1.1
// D_SQLITE_FLOOR_VERSION_NUMBER
//   query: the oldest SQLite the layer supports, 3.7.17 (2013), the release
// Red Hat Enterprise Linux 7 ships, encoded as SQLite encodes a version
// (MAJOR*1000000 + MINOR*1000 + PATCH) and so comparable with
// sqlite3_libversion_number(). Every C API the layer calls without a version
// test exists there; its sources stop with an #error below it. Safe in #if.
#define D_SQLITE_FLOOR_VERSION_NUMBER 3007017


#endif  // DJINTERP_CONFIG_C_DB_SQLITE_CFG_SQLITE_COMMON_H
