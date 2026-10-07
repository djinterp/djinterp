/*******************************************************************************
* djinterp [env]                                              env_mysql_common.h
*
* djinterp MySQL-family common detection.
*   Compile-time detection shared by Oracle MySQL and MariaDB, which forked
* from MySQL 5.5 and still share its wire protocol, C API, core storage
* engines, basic types and SQL, and version encoding (MAJOR*10000 + MINOR*100
* + PATCH). It includes the vendors' client headers, tells the two products
* apart, and detects what doesn't depend on the version: the client library
* and its C API, the embedded server, core storage engines, the SSL/TLS
* library, common data types, character sets, and connection methods.
*   Version-gated features live in env_mysql.h and env_mariadb.h, since the two
* numberings diverged after the fork (MySQL 8.0 against MariaDB 10.5, for
* instance). Both include this header, which is not meant to be included
* directly. It pulls in both vendors' configs, so that their settings exist
* whichever of the two is included, and env.h, for the platform macros it
* reads. D_ENV_MYSQL_COMMON_* are the shared capability flags; with no client
* library, all of them read 0.
*
*
* path:      /inc/djinterp/env/db/mysql/env_mysql_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR HEADER INCLUSION
    -----------------------
    1.  MariaDB headers
         1.  D_ENV_MARIADB_HEADER_INCLUDED
         2.  D_ENV_MARIADB_CPP_HEADER_INCLUDED
    2.  MySQL headers
         1.  D_ENV_MYSQL_HEADER_INCLUDED
         2.  D_ENV_MYSQL_CPP_HEADER_INCLUDED
2.  VENDOR DISAMBIGUATION
    ---------------------
    1.  Vendor
         1.  D_ENV_MYSQL_COMMON_IS_MARIADB
         2.  D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL
         3.  D_ENV_MYSQL_COMMON_FAMILY_DETECTED
3.  VERSION ENCODING
    ----------------
    1.  Encoding and decoding
         1.  D_ENV_MYSQL_COMMON_ENCODE_VERSION
         2.  D_ENV_MYSQL_COMMON_DECODE_MAJOR
         3.  D_ENV_MYSQL_COMMON_DECODE_MINOR
         4.  D_ENV_MYSQL_COMMON_DECODE_PATCH
4.  CLIENT LIBRARY DETECTION
    ------------------------
    1.  Client library
         1.  D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
         2.  D_ENV_MYSQL_COMMON_HAS_EMBEDDED
5.  C API FEATURES (VERSION-AGNOSTIC)
    ---------------------------------
    1.  Connection fundamentals
         1.  D_ENV_MYSQL_COMMON_HAS_REAL_CONNECT
         2.  D_ENV_MYSQL_COMMON_HAS_CHANGE_USER
         3.  D_ENV_MYSQL_COMMON_HAS_PING
         4.  D_ENV_MYSQL_COMMON_HAS_SELECT_DB
         5.  D_ENV_MYSQL_COMMON_HAS_SET_CHARACTER_SET
         6.  D_ENV_MYSQL_COMMON_HAS_OPTIONS
         7.  D_ENV_MYSQL_COMMON_HAS_AUTOCOMMIT
         8.  D_ENV_MYSQL_COMMON_HAS_COMMIT_ROLLBACK
    2.  Query execution
         1.  D_ENV_MYSQL_COMMON_HAS_REAL_QUERY
         2.  D_ENV_MYSQL_COMMON_HAS_REAL_ESCAPE_STRING
         3.  D_ENV_MYSQL_COMMON_HAS_MULTI_STATEMENTS
         4.  D_ENV_MYSQL_COMMON_HAS_MULTI_RESULTS
         5.  D_ENV_MYSQL_COMMON_HAS_NEXT_RESULT
    3.  Prepared statements
         1.  D_ENV_MYSQL_COMMON_HAS_PREPARED_STATEMENTS
         2.  D_ENV_MYSQL_COMMON_HAS_STMT_ATTR_CURSOR
    4.  Result set handling
         1.  D_ENV_MYSQL_COMMON_HAS_STORE_RESULT
         2.  D_ENV_MYSQL_COMMON_HAS_USE_RESULT
         3.  D_ENV_MYSQL_COMMON_HAS_FETCH_ROW
         4.  D_ENV_MYSQL_COMMON_HAS_FETCH_FIELDS
         5.  D_ENV_MYSQL_COMMON_HAS_NUM_FIELDS
         6.  D_ENV_MYSQL_COMMON_HAS_NUM_ROWS
         7.  D_ENV_MYSQL_COMMON_HAS_AFFECTED_ROWS
         8.  D_ENV_MYSQL_COMMON_HAS_INSERT_ID
         9.  D_ENV_MYSQL_COMMON_HAS_DATA_SEEK
         10. D_ENV_MYSQL_COMMON_HAS_ROW_SEEK
    5.  Protocol features
         1.  D_ENV_MYSQL_COMMON_HAS_COMPRESSED_PROTOCOL
    6.  Error and status
         1.  D_ENV_MYSQL_COMMON_HAS_ERRNO
         2.  D_ENV_MYSQL_COMMON_HAS_ERROR
         3.  D_ENV_MYSQL_COMMON_HAS_SQLSTATE
         4.  D_ENV_MYSQL_COMMON_HAS_WARNING_COUNT
         5.  D_ENV_MYSQL_COMMON_HAS_INFO
         6.  D_ENV_MYSQL_COMMON_HAS_SERVER_INFO
         7.  D_ENV_MYSQL_COMMON_HAS_SERVER_VERSION
         8.  D_ENV_MYSQL_COMMON_HAS_CLIENT_INFO
         9.  D_ENV_MYSQL_COMMON_HAS_STAT
         10. D_ENV_MYSQL_COMMON_HAS_THREAD_ID
    7.  Thread safety
         1.  D_ENV_MYSQL_COMMON_HAS_THREAD_SAFE
         2.  D_ENV_MYSQL_COMMON_HAS_THREAD_INIT
         3.  D_ENV_MYSQL_COMMON_HAS_LIBRARY_INIT
    8.  Authentication
         1.  D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE
         2.  D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH
6.  CORE STORAGE ENGINE DETECTION
    -----------------------------
    1.  Storage engines
         1.  D_ENV_MYSQL_COMMON_HAS_INNODB
         2.  D_ENV_MYSQL_COMMON_HAS_MYISAM
         3.  D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE
         4.  D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE
         5.  D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE
         6.  D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE
         7.  D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
         8.  D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
7.  SSL/TLS LIBRARY DETECTION
    -------------------------
    1.  SSL and TLS
         1.  D_ENV_MYSQL_COMMON_HAS_OPENSSL
         2.  D_ENV_MYSQL_COMMON_HAS_WOLFSSL
         3.  D_ENV_MYSQL_COMMON_HAS_YASSL
         4.  D_ENV_MYSQL_COMMON_HAS_ANY_SSL
8.  COMMON DATA TYPES
    -----------------
    1.  Data types
         1.  D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES
         2.  D_ENV_MYSQL_COMMON_HAS_BLOB_TYPES
         3.  D_ENV_MYSQL_COMMON_HAS_BIT_TYPE
         4.  D_ENV_MYSQL_COMMON_HAS_ENUM_TYPE
         5.  D_ENV_MYSQL_COMMON_HAS_SET_TYPE
9.  CHARACTER SET BASICS
    --------------------
    1.  Character sets
         1.  D_ENV_MYSQL_COMMON_HAS_UTF8MB3
         2.  D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3
10. PLATFORM CONNECTION METHODS
    ---------------------------
    1.  Connection methods
         1.  D_ENV_MYSQL_COMMON_HAS_TCP_IP
         2.  D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
*/

#ifndef DJINTERP_ENV_DB_MYSQL_ENV_MYSQL_COMMON_H
#define DJINTERP_ENV_DB_MYSQL_ENV_MYSQL_COMMON_H 1

// djinterp
#include "../../env.h"                                            // D_ENV_OS_ID
#include "../../../config/core/env/db/mariadb/cfg_env_mariadb.h"  // settings
#include "../../../config/core/env/db/mysql/cfg_env_mysql.h"      // settings


//==============================================================================
// 1.  VENDOR HEADER INCLUSION
//==============================================================================
// The MySQL family shares one C client surface (libmysqlclient, MariaDB
// Connector/C, mysql.h), so this section includes it once, for both
// env_mysql.h and env_mariadb.h, driven by D_CFG_ENV_USING_MARIADB and
// D_CFG_ENV_USING_MYSQL. Downstream detection gates on the four
// D_ENV_*_HEADER_INCLUDED sentinels, so that no vendor symbol is referenced
// unless its header is in scope.


// 1.1    MariaDB headers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_MARIADB_HEADER_INCLUDED
//   detection: 1 once the MariaDB C client header is included, and 0 when
// D_CFG_ENV_USING_MARIADB is off. D_CFG_ENV_MARIADB_C_PATH is tried first, then
// <mariadb/mysql.h>, <mysql/mysql.h> and <mysql.h>; without __has_include, the
// configured path is included as is, and with none found, #error.
// D_ENV_DB_HAS_MARIADB_CONNECTOR_C follows it unless pre-defined.
#if D_CFG_IS_ON(D_CFG_ENV_USING_MARIADB)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MARIADB_C_PATH)
            #include D_CFG_ENV_MARIADB_C_PATH  // C client
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #elif __has_include(<mariadb/mysql.h>)
            #include <mariadb/mysql.h>  // C client
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #elif __has_include(<mysql/mysql.h>)
            #include <mysql/mysql.h>  // C client
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #elif __has_include(<mysql.h>)
            #include <mysql.h>  // C client
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_MARIADB=1 but no MariaDB/MySQL C "         \
                   "client header was found. Install the MariaDB "             \
                   "Connector/C development package, or define "               \
                   "D_CFG_ENV_MARIADB_C_PATH to the correct location."
        #endif
    #else
        // pre-C++17 / pre-C23: no __has_include; trust configured path
        #include D_CFG_ENV_MARIADB_C_PATH  // C client
        #define D_ENV_MARIADB_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_C
        #define D_ENV_DB_HAS_MARIADB_CONNECTOR_C 1
    #endif  // D_ENV_DB_HAS_MARIADB_CONNECTOR_C

#else
    #define D_ENV_MARIADB_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_C
        #define D_ENV_DB_HAS_MARIADB_CONNECTOR_C 0
    #endif  // D_ENV_DB_HAS_MARIADB_CONNECTOR_C
#endif  // D_CFG_ENV_USING_MARIADB

// 1.1.2
// D_ENV_MARIADB_CPP_HEADER_INCLUDED
//   detection: 1 once the MariaDB C++ connector header,
// D_CFG_ENV_MARIADB_CPP_PATH, is found and included; C++ only, and only where
// __has_include can find it, so that libmariadbcpp is never pulled in
// unexpectedly. D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP follows it unless
// pre-defined.
#if ( (D_CFG_IS_ON(D_CFG_ENV_USING_MARIADB)) &&                                \
      (defined(__cplusplus)) )

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MARIADB_CPP_PATH)
            #include D_CFG_ENV_MARIADB_CPP_PATH  // C++ connector
            #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 1
            #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 1
            #endif  // D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
        #else
            #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 0
            #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 0
            #endif  // D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
        #endif
    #else
        // no __has_include: C++ connector is opt-in only when the user has
        // explicitly overridden the path (otherwise we silently skip to
        // avoid pulling in libmariadbcpp symbols unexpectedly).
        #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 0
        #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
            #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 0
        #endif  // D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
    #endif

#else
    #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
        #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 0
    #endif  // D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
#endif  // D_CFG_ENV_USING_MARIADB && __cplusplus

// 1.2    MySQL headers
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_MYSQL_HEADER_INCLUDED
//   detection: 1 once the MySQL C client header is included, and 0 when
// D_CFG_ENV_USING_MYSQL is off. D_CFG_ENV_MYSQL_C_PATH is tried first, then
// <mysql/mysql.h> and <mysql.h>; without __has_include, the configured path
// is included as is, and with none found, #error.
// D_ENV_DB_HAS_MYSQL_CONNECTOR_C follows it unless pre-defined.
#if D_CFG_IS_ON(D_CFG_ENV_USING_MYSQL)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MYSQL_C_PATH)
            #include D_CFG_ENV_MYSQL_C_PATH  // C client
            #define D_ENV_MYSQL_HEADER_INCLUDED 1
        #elif __has_include(<mysql/mysql.h>)
            #include <mysql/mysql.h>  // C client
            #define D_ENV_MYSQL_HEADER_INCLUDED 1
        #elif __has_include(<mysql.h>)
            #include <mysql.h>  // C client
            #define D_ENV_MYSQL_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_MYSQL=1 but no MySQL C client header "     \
                   "was found. Install libmysqlclient-dev (or equivalent), "   \
                   "or define D_CFG_ENV_MYSQL_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_MYSQL_C_PATH  // C client
        #define D_ENV_MYSQL_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_C
        #define D_ENV_DB_HAS_MYSQL_CONNECTOR_C 1
    #endif  // D_ENV_DB_HAS_MYSQL_CONNECTOR_C

#else
    #define D_ENV_MYSQL_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_C
        #define D_ENV_DB_HAS_MYSQL_CONNECTOR_C 0
    #endif  // D_ENV_DB_HAS_MYSQL_CONNECTOR_C
#endif  // D_CFG_ENV_USING_MYSQL

// 1.2.2
// D_ENV_MYSQL_CPP_HEADER_INCLUDED
//   detection: 1 once the MySQL X DevAPI C++ header, D_CFG_ENV_MYSQL_CPP_PATH,
// is found and included; C++ only, and only where __has_include can find it.
// D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP follows it unless pre-defined.
#if ( (D_CFG_IS_ON(D_CFG_ENV_USING_MYSQL)) &&                                  \
      (defined(__cplusplus)) )

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MYSQL_CPP_PATH)
            #include D_CFG_ENV_MYSQL_CPP_PATH  // C++ connector
            #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 1
            #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 1
            #endif  // D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
        #else
            #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 0
            #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 0
            #endif  // D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
        #endif
    #else
        #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 0
        #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
            #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 0
        #endif  // D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
    #endif

#else
    #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
        #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 0
    #endif  // D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
#endif  // D_CFG_ENV_USING_MYSQL && __cplusplus


//==============================================================================
// 2.  VENDOR DISAMBIGUATION
//==============================================================================
// MariaDB defines MYSQL_VERSION_ID alongside MARIADB_VERSION_ID for
// compatibility. We must check the MariaDB sentinels first to avoid
// misidentifying a MariaDB build as Oracle MySQL.


// 2.1    Vendor
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_MYSQL_COMMON_IS_MARIADB
//   detection: 1 if the build environment is MariaDB, 0 if Oracle MySQL
// or undetected.
#ifndef D_ENV_MYSQL_COMMON_IS_MARIADB
    #if ( (defined(MARIADB_VERSION_ID))         ||                             \
          (defined(MARIADB_BASE_VERSION))       ||                             \
          (defined(MARIADB_CLIENT_VERSION_STR)) ||                             \
          (defined(MARIADB_PACKAGE_VERSION)) )
        #define D_ENV_MYSQL_COMMON_IS_MARIADB 1
    #else
        #define D_ENV_MYSQL_COMMON_IS_MARIADB 0
    #endif
#endif  // D_ENV_MYSQL_COMMON_IS_MARIADB

// 2.1.2
// D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL
//   detection: 1 if the build environment is Oracle MySQL (MYSQL_VERSION_ID
// defined AND not MariaDB).
#ifndef D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL
    #if ( (defined(MYSQL_VERSION_ID)) &&                                       \
          (!D_ENV_MYSQL_COMMON_IS_MARIADB) )
        #define D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL 1
    #else
        #define D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL 0
    #endif
#endif  // D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL

// 2.1.3
// D_ENV_MYSQL_COMMON_FAMILY_DETECTED
//   detection: 1 if any MySQL-compatible environment is detected (either
// Oracle MySQL or MariaDB).
#ifndef D_ENV_MYSQL_COMMON_FAMILY_DETECTED
    #if ( (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) ||                              \
          (D_ENV_MYSQL_COMMON_IS_MARIADB) )
        #define D_ENV_MYSQL_COMMON_FAMILY_DETECTED 1
    #else
        #define D_ENV_MYSQL_COMMON_FAMILY_DETECTED 0
    #endif
#endif  // D_ENV_MYSQL_COMMON_FAMILY_DETECTED


//==============================================================================
// 3.  VERSION ENCODING
//==============================================================================
// Both Oracle MySQL and MariaDB use the same encoding scheme:
// MAJOR * 10000 + MINOR * 100 + PATCH
// e.g. MySQL 8.0.35 = 80035, MariaDB 11.4.2 = 110402.


// 3.1    Encoding and decoding
//------------------------------------------------------------------------------
// 3.1.1
// D_ENV_MYSQL_COMMON_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple into the standard
// MySQL-family version ID format.
#define D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch)                 \
    ((major) * 10000 + (minor) * 100 + (patch))

// 3.1.2
// D_ENV_MYSQL_COMMON_DECODE_MAJOR
//   macro: extracts the major version from an encoded version ID.
#define D_ENV_MYSQL_COMMON_DECODE_MAJOR(ver_id)                                \
    ((ver_id) / 10000)

// 3.1.3
// D_ENV_MYSQL_COMMON_DECODE_MINOR
//   macro: extracts the minor version from an encoded version ID.
#define D_ENV_MYSQL_COMMON_DECODE_MINOR(ver_id)                                \
    (((ver_id) / 100) % 100)

// 3.1.4
// D_ENV_MYSQL_COMMON_DECODE_PATCH
//   macro: extracts the patch version from an encoded version ID.
#define D_ENV_MYSQL_COMMON_DECODE_PATCH(ver_id)                                \
    ((ver_id) % 100)


//==============================================================================
// 4.  CLIENT LIBRARY DETECTION
//==============================================================================
// These macros detect the presence of a MySQL-compatible C client library
// regardless of vendor. They do NOT depend on version numbering.


// 4.1    Client library
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
//   feature: detect if any MySQL-compatible client library is available.
#ifndef D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
    #if ( (defined(MYSQL_VERSION_ID))    ||                                    \
          (defined(LIBMYSQL_VERSION_ID)) ||                                    \
          (defined(MARIADB_VERSION_ID)) )
        #define D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB 1
    #else
        #define D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB 0
    #endif
#endif  // D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB

// 4.1.2
// D_ENV_MYSQL_COMMON_HAS_EMBEDDED
//   feature: detect if the MySQL embedded server library (libmysqld or
// MariaDB embedded) is present.
// note: Oracle MySQL removed embedded server in 8.0. MariaDB retained
// the embedded library through 10.x; removed in 11.0.
#ifndef D_ENV_MYSQL_COMMON_HAS_EMBEDDED
    #if defined(EMBEDDED_LIBRARY)
        #define D_ENV_MYSQL_COMMON_HAS_EMBEDDED 1
    #else
        #define D_ENV_MYSQL_COMMON_HAS_EMBEDDED 0
    #endif
#endif  // D_ENV_MYSQL_COMMON_HAS_EMBEDDED


// sections 5 to 10 describe a detected client library; without one, the
// #else after section 10 defines all their flags as 0
#if D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB


//==============================================================================
// 5.  C API FEATURES (VERSION-AGNOSTIC)
//==============================================================================
// These C API features have existed since before the MySQL 5.5 fork point
// and are present in every modern build of both Oracle MySQL and MariaDB.
// They depend only on a client library being detected, not on version.


// 5.1    Connection fundamentals
//------------------------------------------------------------------------------
    // 5.1.1
    // D_ENV_MYSQL_COMMON_HAS_REAL_CONNECT
    //   feature: mysql_real_connect() is available (present since MySQL 3.x).
    #define D_ENV_MYSQL_COMMON_HAS_REAL_CONNECT        1

    // 5.1.2
    // D_ENV_MYSQL_COMMON_HAS_CHANGE_USER
    //   feature: mysql_change_user() is available (present since MySQL 3.x).
    #define D_ENV_MYSQL_COMMON_HAS_CHANGE_USER         1

    // 5.1.3
    // D_ENV_MYSQL_COMMON_HAS_PING
    //   feature: mysql_ping() is available.
    #define D_ENV_MYSQL_COMMON_HAS_PING                1

    // 5.1.4
    // D_ENV_MYSQL_COMMON_HAS_SELECT_DB
    //   feature: mysql_select_db() is available.
    #define D_ENV_MYSQL_COMMON_HAS_SELECT_DB           1

    // 5.1.5
    // D_ENV_MYSQL_COMMON_HAS_SET_CHARACTER_SET
    //   feature: mysql_set_character_set() is available (since 5.0.7).
    #define D_ENV_MYSQL_COMMON_HAS_SET_CHARACTER_SET   1

    // 5.1.6
    // D_ENV_MYSQL_COMMON_HAS_OPTIONS
    //   feature: mysql_options() / mysql_options4() are available.
    #define D_ENV_MYSQL_COMMON_HAS_OPTIONS             1

    // 5.1.7
    // D_ENV_MYSQL_COMMON_HAS_AUTOCOMMIT
    //   feature: mysql_autocommit() is available.
    #define D_ENV_MYSQL_COMMON_HAS_AUTOCOMMIT          1

    // 5.1.8
    // D_ENV_MYSQL_COMMON_HAS_COMMIT_ROLLBACK
    //   feature: mysql_commit() and mysql_rollback() are available.
    #define D_ENV_MYSQL_COMMON_HAS_COMMIT_ROLLBACK     1

// 5.2    Query execution
//------------------------------------------------------------------------------
    // 5.2.1
    // D_ENV_MYSQL_COMMON_HAS_REAL_QUERY
    //   feature: mysql_real_query() is available.
    #define D_ENV_MYSQL_COMMON_HAS_REAL_QUERY          1

    // 5.2.2
    // D_ENV_MYSQL_COMMON_HAS_REAL_ESCAPE_STRING
    //   feature: mysql_real_escape_string() is available.
    #define D_ENV_MYSQL_COMMON_HAS_REAL_ESCAPE_STRING  1

    // 5.2.3
    // D_ENV_MYSQL_COMMON_HAS_MULTI_STATEMENTS
    //   feature: CLIENT_MULTI_STATEMENTS is available for executing
    // multiple SQL statements in a single call (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_MULTI_STATEMENTS    1

    // 5.2.4
    // D_ENV_MYSQL_COMMON_HAS_MULTI_RESULTS
    //   feature: CLIENT_MULTI_RESULTS is available for processing result
    // sets from stored procedures and multi-statement queries.
    #define D_ENV_MYSQL_COMMON_HAS_MULTI_RESULTS       1

    // 5.2.5
    // D_ENV_MYSQL_COMMON_HAS_NEXT_RESULT
    //   feature: mysql_next_result() is available for iterating
    // multi-result sets (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_NEXT_RESULT         1

// 5.3    Prepared statements
//------------------------------------------------------------------------------
    // 5.3.1
    // D_ENV_MYSQL_COMMON_HAS_PREPARED_STATEMENTS
    //   feature: mysql_stmt_* API is available (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_PREPARED_STATEMENTS 1

    // 5.3.2
    // D_ENV_MYSQL_COMMON_HAS_STMT_ATTR_CURSOR
    //   feature: server-side cursors via STMT_ATTR_CURSOR_TYPE are
    // available (since MySQL 5.0).
    #define D_ENV_MYSQL_COMMON_HAS_STMT_ATTR_CURSOR    1

// 5.4    Result set handling
//------------------------------------------------------------------------------
    // 5.4.1
    // D_ENV_MYSQL_COMMON_HAS_STORE_RESULT
    //   feature: mysql_store_result() is available.
    #define D_ENV_MYSQL_COMMON_HAS_STORE_RESULT        1

    // 5.4.2
    // D_ENV_MYSQL_COMMON_HAS_USE_RESULT
    //   feature: mysql_use_result() (streaming result set) is available.
    #define D_ENV_MYSQL_COMMON_HAS_USE_RESULT          1

    // 5.4.3
    // D_ENV_MYSQL_COMMON_HAS_FETCH_ROW
    //   feature: mysql_fetch_row() is available.
    #define D_ENV_MYSQL_COMMON_HAS_FETCH_ROW           1

    // 5.4.4
    // D_ENV_MYSQL_COMMON_HAS_FETCH_FIELDS
    //   feature: mysql_fetch_fields() / mysql_fetch_field() are available.
    #define D_ENV_MYSQL_COMMON_HAS_FETCH_FIELDS        1

    // 5.4.5
    // D_ENV_MYSQL_COMMON_HAS_NUM_FIELDS
    //   feature: mysql_num_fields() is available.
    #define D_ENV_MYSQL_COMMON_HAS_NUM_FIELDS          1

    // 5.4.6
    // D_ENV_MYSQL_COMMON_HAS_NUM_ROWS
    //   feature: mysql_num_rows() is available.
    #define D_ENV_MYSQL_COMMON_HAS_NUM_ROWS            1

    // 5.4.7
    // D_ENV_MYSQL_COMMON_HAS_AFFECTED_ROWS
    //   feature: mysql_affected_rows() is available.
    #define D_ENV_MYSQL_COMMON_HAS_AFFECTED_ROWS       1

    // 5.4.8
    // D_ENV_MYSQL_COMMON_HAS_INSERT_ID
    //   feature: mysql_insert_id() is available.
    #define D_ENV_MYSQL_COMMON_HAS_INSERT_ID           1

    // 5.4.9
    // D_ENV_MYSQL_COMMON_HAS_DATA_SEEK
    //   feature: mysql_data_seek() is available.
    #define D_ENV_MYSQL_COMMON_HAS_DATA_SEEK           1

    // 5.4.10
    // D_ENV_MYSQL_COMMON_HAS_ROW_SEEK
    //   feature: mysql_row_seek() / mysql_row_tell() are available.
    #define D_ENV_MYSQL_COMMON_HAS_ROW_SEEK            1

// 5.5    Protocol features
//------------------------------------------------------------------------------
    // 5.5.1
    // D_ENV_MYSQL_COMMON_HAS_COMPRESSED_PROTOCOL
    //   feature: CLIENT_COMPRESS (zlib compression) is available
    // (since MySQL 3.22).
    #define D_ENV_MYSQL_COMMON_HAS_COMPRESSED_PROTOCOL 1

// 5.6    Error and status
//------------------------------------------------------------------------------
    // 5.6.1
    // D_ENV_MYSQL_COMMON_HAS_ERRNO
    //   feature: mysql_errno() is available.
    #define D_ENV_MYSQL_COMMON_HAS_ERRNO               1

    // 5.6.2
    // D_ENV_MYSQL_COMMON_HAS_ERROR
    //   feature: mysql_error() is available.
    #define D_ENV_MYSQL_COMMON_HAS_ERROR               1

    // 5.6.3
    // D_ENV_MYSQL_COMMON_HAS_SQLSTATE
    //   feature: mysql_sqlstate() (SQLSTATE error codes) is available
    // (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_SQLSTATE            1

    // 5.6.4
    // D_ENV_MYSQL_COMMON_HAS_WARNING_COUNT
    //   feature: mysql_warning_count() is available (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_WARNING_COUNT       1

    // 5.6.5
    // D_ENV_MYSQL_COMMON_HAS_INFO
    //   feature: mysql_info() is available.
    #define D_ENV_MYSQL_COMMON_HAS_INFO                1

    // 5.6.6
    // D_ENV_MYSQL_COMMON_HAS_SERVER_INFO
    //   feature: mysql_get_server_info() is available.
    #define D_ENV_MYSQL_COMMON_HAS_SERVER_INFO         1

    // 5.6.7
    // D_ENV_MYSQL_COMMON_HAS_SERVER_VERSION
    //   feature: mysql_get_server_version() is available.
    #define D_ENV_MYSQL_COMMON_HAS_SERVER_VERSION      1

    // 5.6.8
    // D_ENV_MYSQL_COMMON_HAS_CLIENT_INFO
    //   feature: mysql_get_client_info() / mysql_get_client_version() are
    // available.
    #define D_ENV_MYSQL_COMMON_HAS_CLIENT_INFO         1

    // 5.6.9
    // D_ENV_MYSQL_COMMON_HAS_STAT
    //   feature: mysql_stat() is available.
    #define D_ENV_MYSQL_COMMON_HAS_STAT                1

    // 5.6.10
    // D_ENV_MYSQL_COMMON_HAS_THREAD_ID
    //   feature: mysql_thread_id() is available.
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_ID           1

// 5.7    Thread safety
//------------------------------------------------------------------------------
    // 5.7.1
    // D_ENV_MYSQL_COMMON_HAS_THREAD_SAFE
    //   feature: mysql_thread_safe() is available (since MySQL 4.0).
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_SAFE         1

    // 5.7.2
    // D_ENV_MYSQL_COMMON_HAS_THREAD_INIT
    //   feature: mysql_thread_init() / mysql_thread_end() are available.
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_INIT         1

    // 5.7.3
    // D_ENV_MYSQL_COMMON_HAS_LIBRARY_INIT
    //   feature: mysql_library_init() / mysql_library_end() are available
    // (since MySQL 5.0).
    #define D_ENV_MYSQL_COMMON_HAS_LIBRARY_INIT        1

// 5.8    Authentication
//------------------------------------------------------------------------------
    // 5.8.1
    // D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE
    //   feature: mysql_native_password authentication plugin is available.
    // Present since MySQL 4.1 in both MySQL and MariaDB.
    #define D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE         1

    // 5.8.2
    // D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH
    //   feature: the pluggable authentication framework is available.
    // Present since MySQL 5.5.7, inherited by MariaDB at the fork.
    #define D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH      1


//==============================================================================
// 6.  CORE STORAGE ENGINE DETECTION
//==============================================================================
// Engines that are always compiled in, or detected via vendor-provided
// compile-time defines. These do not depend on version gating.


// 6.1    Storage engines
//------------------------------------------------------------------------------
    // 6.1.1
    // D_ENV_MYSQL_COMMON_HAS_INNODB
    //   feature: InnoDB is available (default engine since MySQL 5.5;
    // always present in both MySQL and MariaDB builds).
    #define D_ENV_MYSQL_COMMON_HAS_INNODB              1

    // 6.1.2
    // D_ENV_MYSQL_COMMON_HAS_MYISAM
    //   feature: MyISAM is available (always compiled in).
    #define D_ENV_MYSQL_COMMON_HAS_MYISAM              1

    // 6.1.3
    // D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE
    //   feature: MEMORY (HEAP) engine is available (always compiled in).
    #define D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE       1

    // 6.1.4
    // D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE
    //   feature: ARCHIVE engine is available (compiled in by default in
    // both MySQL and MariaDB).
    #define D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE      1

    // 6.1.5
    // D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE
    //   feature: CSV engine is available (compiled in by default).
    #define D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE          1

    // 6.1.6
    // D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE
    //   feature: BLACKHOLE engine is available (compiled in by default).
    #define D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE    1

    // 6.1.7
    // D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
    //   feature: NDB Cluster (MySQL Cluster) engine is available.
    // Detected via compile-time defines set by the NDB build system.
    #ifndef D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
        #if ( (defined(HAVE_NDBCLUSTER)) ||                                    \
              (defined(NDB_VERSION_MAJOR)) )
            #define D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER

    // 6.1.8
    // D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
    //   feature: FEDERATED engine is available.
    // note: disabled by default in most distributions of both products.
    #ifndef D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
        #if defined(HAVE_FEDERATED)
            #define D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE


//==============================================================================
// 7.  SSL/TLS LIBRARY DETECTION
//==============================================================================
// Both products support SSL, but the underlying library varies (OpenSSL,
// wolfSSL, yaSSL). This section detects the SSL backend, not protocol
// features (which are version-gated and belong in vendor headers).


// 7.1    SSL and TLS
//------------------------------------------------------------------------------
    // 7.1.1
    // D_ENV_MYSQL_COMMON_HAS_OPENSSL
    //   feature: the client library was built with OpenSSL.
    #ifndef D_ENV_MYSQL_COMMON_HAS_OPENSSL
        #if defined(HAVE_OPENSSL)
            #define D_ENV_MYSQL_COMMON_HAS_OPENSSL 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_OPENSSL 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_OPENSSL

    // 7.1.2
    // D_ENV_MYSQL_COMMON_HAS_WOLFSSL
    //   feature: the client library was built with wolfSSL.
    #ifndef D_ENV_MYSQL_COMMON_HAS_WOLFSSL
        #if defined(HAVE_WOLFSSL)
            #define D_ENV_MYSQL_COMMON_HAS_WOLFSSL 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_WOLFSSL 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_WOLFSSL

    // 7.1.3
    // D_ENV_MYSQL_COMMON_HAS_YASSL
    //   feature: the client library was built with yaSSL (legacy;
    // bundled in MySQL 5.x, removed in 8.0).
    #ifndef D_ENV_MYSQL_COMMON_HAS_YASSL
        #if defined(HAVE_YASSL)
            #define D_ENV_MYSQL_COMMON_HAS_YASSL 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_YASSL 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_YASSL

    // 7.1.4
    // D_ENV_MYSQL_COMMON_HAS_ANY_SSL
    //   feature: the client library has some SSL/TLS support.
    #define D_ENV_MYSQL_COMMON_HAS_ANY_SSL                                     \
        ( (D_ENV_MYSQL_COMMON_HAS_OPENSSL) ||                                  \
          (D_ENV_MYSQL_COMMON_HAS_WOLFSSL) ||                                  \
          (D_ENV_MYSQL_COMMON_HAS_YASSL) )


//==============================================================================
// 8.  COMMON DATA TYPES
//==============================================================================
// Types present in every MySQL-compatible server and client since before
// the fork point.


// 8.1    Data types
//------------------------------------------------------------------------------
    // 8.1.1
    // D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES
    //   feature: spatial/geometry types (POINT, LINESTRING, POLYGON, etc.)
    // are available. Present since MySQL 4.1 in both products.
    #define D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES      1

    // 8.1.2
    // D_ENV_MYSQL_COMMON_HAS_BLOB_TYPES
    //   feature: BLOB/TEXT family (TINYBLOB through LONGBLOB) is available.
    #define D_ENV_MYSQL_COMMON_HAS_BLOB_TYPES          1

    // 8.1.3
    // D_ENV_MYSQL_COMMON_HAS_BIT_TYPE
    //   feature: BIT data type is available (since MySQL 5.0.3).
    #define D_ENV_MYSQL_COMMON_HAS_BIT_TYPE            1

    // 8.1.4
    // D_ENV_MYSQL_COMMON_HAS_ENUM_TYPE
    //   feature: ENUM data type is available.
    #define D_ENV_MYSQL_COMMON_HAS_ENUM_TYPE           1

    // 8.1.5
    // D_ENV_MYSQL_COMMON_HAS_SET_TYPE
    //   feature: SET data type is available.
    #define D_ENV_MYSQL_COMMON_HAS_SET_TYPE            1


//==============================================================================
// 9.  CHARACTER SET BASICS
//==============================================================================


// 9.1    Character sets
//------------------------------------------------------------------------------
    // 9.1.1
    // D_ENV_MYSQL_COMMON_HAS_UTF8MB3
    //   feature: utf8 (3-byte, aliased utf8mb3) character set is available.
    // Present in both products since well before the fork.
    #define D_ENV_MYSQL_COMMON_HAS_UTF8MB3             1

    // 9.1.2
    // D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3
    //   status: 1 if the 'utf8' charset alias refers to utf8mb3 (3-byte).
    // This is true in ALL versions of both products as of 2025. Neither
    // product has changed the alias to mean utf8mb4.
    #define D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3         1


//==============================================================================
// 10.  PLATFORM CONNECTION METHODS
//==============================================================================


// 10.1   Connection methods
//------------------------------------------------------------------------------
    // 10.1.1
    // D_ENV_MYSQL_COMMON_HAS_TCP_IP
    //   feature: TCP/IP connections are available (always).
    #define D_ENV_MYSQL_COMMON_HAS_TCP_IP              1

    // 10.1.2
    // D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
    //   feature: UNIX domain socket connections are available (POSIX).
    #ifndef D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
        #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
            #define D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET 1


#else  // !D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
            #define D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET

    // D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
    //   feature: named pipe connections are available (Windows).
    #ifndef D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
        #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
            #define D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE 1
#else  // !D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
            #define D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE

    // D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY
    //   feature: shared memory connections are available (Windows).
    #ifndef D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY
        #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
            #define D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY 1
#else  // !D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
            #define D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY 0
        #endif
    #endif  // D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY


#else  // !D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
    // no MySQL-compatible client library detected -- zero everything

    // C API
    #define D_ENV_MYSQL_COMMON_HAS_REAL_CONNECT        0
    #define D_ENV_MYSQL_COMMON_HAS_CHANGE_USER         0
    #define D_ENV_MYSQL_COMMON_HAS_PING                0
    #define D_ENV_MYSQL_COMMON_HAS_SELECT_DB           0
    #define D_ENV_MYSQL_COMMON_HAS_SET_CHARACTER_SET   0
    #define D_ENV_MYSQL_COMMON_HAS_OPTIONS             0
    #define D_ENV_MYSQL_COMMON_HAS_AUTOCOMMIT          0
    #define D_ENV_MYSQL_COMMON_HAS_COMMIT_ROLLBACK     0
    #define D_ENV_MYSQL_COMMON_HAS_REAL_QUERY          0
    #define D_ENV_MYSQL_COMMON_HAS_REAL_ESCAPE_STRING  0
    #define D_ENV_MYSQL_COMMON_HAS_MULTI_STATEMENTS    0
    #define D_ENV_MYSQL_COMMON_HAS_MULTI_RESULTS       0
    #define D_ENV_MYSQL_COMMON_HAS_NEXT_RESULT         0
    #define D_ENV_MYSQL_COMMON_HAS_PREPARED_STATEMENTS 0
    #define D_ENV_MYSQL_COMMON_HAS_STMT_ATTR_CURSOR    0
    #define D_ENV_MYSQL_COMMON_HAS_STORE_RESULT        0
    #define D_ENV_MYSQL_COMMON_HAS_USE_RESULT          0
    #define D_ENV_MYSQL_COMMON_HAS_FETCH_ROW           0
    #define D_ENV_MYSQL_COMMON_HAS_FETCH_FIELDS        0
    #define D_ENV_MYSQL_COMMON_HAS_NUM_FIELDS          0
    #define D_ENV_MYSQL_COMMON_HAS_NUM_ROWS            0
    #define D_ENV_MYSQL_COMMON_HAS_AFFECTED_ROWS       0
    #define D_ENV_MYSQL_COMMON_HAS_INSERT_ID           0
    #define D_ENV_MYSQL_COMMON_HAS_DATA_SEEK           0
    #define D_ENV_MYSQL_COMMON_HAS_ROW_SEEK            0
    #define D_ENV_MYSQL_COMMON_HAS_COMPRESSED_PROTOCOL 0
    #define D_ENV_MYSQL_COMMON_HAS_ERRNO               0
    #define D_ENV_MYSQL_COMMON_HAS_ERROR               0
    #define D_ENV_MYSQL_COMMON_HAS_SQLSTATE            0
    #define D_ENV_MYSQL_COMMON_HAS_WARNING_COUNT       0
    #define D_ENV_MYSQL_COMMON_HAS_INFO                0
    #define D_ENV_MYSQL_COMMON_HAS_SERVER_INFO         0
    #define D_ENV_MYSQL_COMMON_HAS_SERVER_VERSION      0
    #define D_ENV_MYSQL_COMMON_HAS_CLIENT_INFO         0
    #define D_ENV_MYSQL_COMMON_HAS_STAT                0
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_ID           0
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_SAFE         0
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_INIT         0
    #define D_ENV_MYSQL_COMMON_HAS_LIBRARY_INIT        0
    #define D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE         0
    #define D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH      0

    // storage engines
    #define D_ENV_MYSQL_COMMON_HAS_INNODB              0
    #define D_ENV_MYSQL_COMMON_HAS_MYISAM              0
    #define D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE       0
    #define D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE      0
    #define D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE          0
    #define D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE    0
    #ifndef D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
        #define D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER     0
    #endif  // D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
    #ifndef D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
        #define D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE 0
    #endif  // D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE

    // SSL
    #ifndef D_ENV_MYSQL_COMMON_HAS_OPENSSL
        #define D_ENV_MYSQL_COMMON_HAS_OPENSSL         0
    #endif  // D_ENV_MYSQL_COMMON_HAS_OPENSSL
    #ifndef D_ENV_MYSQL_COMMON_HAS_WOLFSSL
        #define D_ENV_MYSQL_COMMON_HAS_WOLFSSL         0
    #endif  // D_ENV_MYSQL_COMMON_HAS_WOLFSSL
    #ifndef D_ENV_MYSQL_COMMON_HAS_YASSL
        #define D_ENV_MYSQL_COMMON_HAS_YASSL           0
    #endif  // D_ENV_MYSQL_COMMON_HAS_YASSL
    #define D_ENV_MYSQL_COMMON_HAS_ANY_SSL             0

    // data types
    #define D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES      0
    #define D_ENV_MYSQL_COMMON_HAS_BLOB_TYPES          0
    #define D_ENV_MYSQL_COMMON_HAS_BIT_TYPE            0
    #define D_ENV_MYSQL_COMMON_HAS_ENUM_TYPE           0
    #define D_ENV_MYSQL_COMMON_HAS_SET_TYPE            0

    // character sets
    #define D_ENV_MYSQL_COMMON_HAS_UTF8MB3             0
    #define D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3         0

    // platform
    #define D_ENV_MYSQL_COMMON_HAS_TCP_IP              0
    #ifndef D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
        #define D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET     0
    #endif  // D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
    #ifndef D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
        #define D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE      0
    #endif  // D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
    #ifndef D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY
        #define D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY   0
    #endif  // D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY

#endif  // D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB


#endif  // DJINTERP_ENV_DB_MYSQL_ENV_MYSQL_COMMON_H
