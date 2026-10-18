/*******************************************************************************
* djinterp [config]                                         cfg_mariadb_common.h
*
* Build-time configuration shared by every module of djinterp's MariaDB/MySQL
* layer (c/db/mariadb/mariadb_*.h).
*   One knob decides what the layer is built over: a MySQL-protocol client
* library -- MariaDB Connector/C or Oracle's libmysqlclient, which share the
* C API -- or stubs that report D_DB_UNSUPPORTED, so that code using the
* layer compiles and links where neither is installed and decides at run
* time, as the SQLite layer and the libcurl binding do. It defaults to
* whether the client's mysql.h can be found.
*   Built over a client, the layer's sources reach mysql.h through
* env_mysql_common.h, which includes it only when D_CFG_ENV_USING_MARIADB is
* 1. This file therefore sets that knob for the layer's own sources -- each
* defines D_INTERNAL_MARIADB_SOURCE before its first include -- unless the
* build already has, and only then includes env's configuration, so that
* env's cascade honours it (the config README, 6.2). A build that asks for
* the client but sets that knob to 0, or includes dconfig.h first, stops at
* the layer's sources with an #error; it sets D_CFG_ENV_USING_MARIADB=1
* itself.
*   Nothing else that includes the layer has mysql.h in scope, unless the
* program asks for env's detection itself. Not the public headers: it spells
* `long long`, which ISO C++98 refuses, and the two clients disagree on its
* struct's tag. Not core/db: its mariadb_connection would meet Connector/C's
* function of that name under `using namespace djinterp`, its mysql_ssl_mode
* libmysqlclient's enum, and on Windows libmysqlclient's mysql.h brings
* <windows.h>, and its min and max macros, with it.
*   Where both clients are installed, name the header of the library the
* program links: env looks for <mysql/mysql.h> first, which may be the other
* client's, and the two lay struct MYSQL out differently. With the client's
* own flags (mariadb_config --cflags, mysql_config --cflags), define
* D_CFG_ENV_MARIADB_C_PATH as <mysql.h>, which they put first.
*
*   targets:  c/db/mariadb/mariadb_*.h and their sources ->
*             D_INTERNAL_MARIADB, D_INTERNAL_MARIADB_CHARSET,
*             D_MARIADB_FLOOR_*
*   requires: cfg_common.h, cfg_env_mariadb.h
*
*
* path:      /inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h
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
         1.  D_CFG_MARIADB
2.  KNOBS
    -----
    1.  Character set
         1.  D_CFG_MARIADB_CHARSET
3.  DEPENDENCIES
    ------------
    1.  Client detection
         1.  D_CFG_ENV_USING_MARIADB
4.  VALIDATION
    ----------
    1.  Knob validation
5.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_MARIADB
         2.  D_INTERNAL_MARIADB_CHARSET
6.  QUERIES
    -------
    1.  Floors
         1.  D_MARIADB_FLOOR_CLIENT_VERSION_ID
         2.  D_MARIADB_FLOOR_MYSQL_SERVER_ID
         3.  D_MARIADB_FLOOR_MARIADB_SERVER_ID
*/

#ifndef DJINTERP_CONFIG_C_DB_MARIADB_CFG_MARIADB_COMMON_H
#define DJINTERP_CONFIG_C_DB_MARIADB_CFG_MARIADB_COMMON_H 1

// djinterp
#include "../../../cfg_common.h"  // D_CFG_IS_ON, D_CFG_IS_BOOL, user overrides


//==============================================================================
// 1.  BINDING
//==============================================================================
// Whether the layer runs over a client library. Without one every function
// reports D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail.


// 1.1    Environment-defaulted enable
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_MARIADB
//   knob: build the layer over a client library (1) or as stubs (0).
// Defaults to 1 where the client's mysql.h can be found -- at
// D_CFG_ENV_MARIADB_C_PATH when the build names one, else as
// <mariadb/mysql.h>, <mysql/mysql.h> or <mysql.h>, where env looks -- and to
// 0 where it cannot; to 1 where the preprocessor has no __has_include to
// look with. Define it as 1 to require a client, which stops a build that
// lacks one with an #error, or as 0 to build the stubs whatever is installed.
#ifndef D_CFG_MARIADB
    #if defined(__has_include)
        #if defined(D_CFG_ENV_MARIADB_C_PATH)
            #if __has_include(D_CFG_ENV_MARIADB_C_PATH)
                #define D_CFG_MARIADB 1
            #else
                #define D_CFG_MARIADB 0
            #endif
        #elif __has_include(<mariadb/mysql.h>)
            #define D_CFG_MARIADB 1
        #elif __has_include(<mysql/mysql.h>)
            #define D_CFG_MARIADB 1
        #elif __has_include(<mysql.h>)
            #define D_CFG_MARIADB 1
        #else
            #define D_CFG_MARIADB 0
        #endif
    #else
        #define D_CFG_MARIADB 1
    #endif
#endif  // D_CFG_MARIADB


//==============================================================================
// 2.  KNOBS
//==============================================================================


// 2.1    Character set
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_MARIADB_CHARSET
//   knob: the character set the layer asks for -- for every connection it
// makes, every database it creates, and every table it creates -- as a string
// the server knows. The default, "utf8mb4", is the whole of Unicode, where
// both servers long defaulted to latin1 (MySQL until 8.0); "" leaves all
// three to the server's defaults.
#ifndef D_CFG_MARIADB_CHARSET
    #define D_CFG_MARIADB_CHARSET "utf8mb4"
#endif  // D_CFG_MARIADB_CHARSET


//==============================================================================
// 3.  DEPENDENCIES
//==============================================================================


// 3.1    Client detection
//------------------------------------------------------------------------------
// 3.1.1
// D_CFG_ENV_USING_MARIADB
//   knob (env's): turned on here for the layer's own sources, when the layer
// is built over a client, before env's own default can turn it off, so that
// they find mysql.h. A source says it is one by defining
// D_INTERNAL_MARIADB_SOURCE before its first include; everything else keeps
// env's default. A value the build gave first wins.
#if ( (D_CFG_IS_ON(D_CFG_MARIADB)) &&                                          \
      (defined(D_INTERNAL_MARIADB_SOURCE)) )
    #ifndef D_CFG_ENV_USING_MARIADB
        #define D_CFG_ENV_USING_MARIADB 1
    #endif  // D_CFG_ENV_USING_MARIADB
#endif

// djinterp
#include "../../../core/env/db/mariadb/cfg_env_mariadb.h"  // env's knobs


//==============================================================================
// 4.  VALIDATION
//==============================================================================


// 4.1    Knob validation
//------------------------------------------------------------------------------
// validation: #if reads the knob as a number, so it must be 0 or 1
#if !D_CFG_IS_BOOL(D_CFG_MARIADB)
    #error "D_CFG_MARIADB must be 0 or 1"
#endif


//==============================================================================
// 5.  RESOLVED VALUES
//==============================================================================


// 5.1    Effective values
//------------------------------------------------------------------------------
// 5.1.1
// D_INTERNAL_MARIADB
//   brief: 1 when the layer is built over a client, 0 when it is built as
// stubs; the sources choose their implementation by it.
#if D_CFG_IS_ON(D_CFG_MARIADB)
    #define D_INTERNAL_MARIADB 1
#else
    #define D_INTERNAL_MARIADB 0
#endif

// 5.1.2
// D_INTERNAL_MARIADB_CHARSET
//   brief: D_CFG_MARIADB_CHARSET, which the sources read through this name.
#define D_INTERNAL_MARIADB_CHARSET D_CFG_MARIADB_CHARSET


//==============================================================================
// 6.  QUERIES
//==============================================================================


// 6.1    Floors
//------------------------------------------------------------------------------
// Versions are the MySQL family's MAJOR*10000 + MINOR*100 + PATCH, as
// MYSQL_VERSION_ID and mysql_get_server_version spell them. Safe in #if.

// 6.1.1
// D_MARIADB_FLOOR_CLIENT_VERSION_ID
//   query: the oldest client the layer builds against, by the MYSQL_VERSION_ID
// its mysql.h declares: libmysqlclient 5.7; MariaDB's clients, Connector/C
// among them, declare their 10.x series and pass. The sources stop with an
// #error below it.
#define D_MARIADB_FLOOR_CLIENT_VERSION_ID  50700

// 6.1.2
// D_MARIADB_FLOOR_MYSQL_SERVER_ID
//   query: the oldest MySQL server the layer is written and tested for, 5.7.
#define D_MARIADB_FLOOR_MYSQL_SERVER_ID    50700

// 6.1.3
// D_MARIADB_FLOOR_MARIADB_SERVER_ID
//   query: the oldest MariaDB server the layer is written and tested for,
// 10.3, the series RHEL 8 and Ubuntu 20.04 ship.
#define D_MARIADB_FLOOR_MARIADB_SERVER_ID 100300


#endif  // DJINTERP_CONFIG_C_DB_MARIADB_CFG_MARIADB_COMMON_H
