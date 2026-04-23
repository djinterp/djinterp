/******************************************************************************
* djinterp [db]                                             env_mysql_common.h
*
* djinterp MySQL-family common infrastructure header:
* This header provides compile-time detection of features, capabilities, and
* infrastructure shared across the MySQL-compatible database family,
* including both Oracle MySQL and MariaDB. Because MariaDB forked from
* MySQL 5.5, the two products share:
*   - wire protocol (MySQL client/server protocol)
*   - C API surface (libmysqlclient / MariaDB Connector/C)
*   - core storage engine interfaces (InnoDB, MyISAM, MEMORY, etc.)
*   - fundamental data types and SQL grammar
*   - version encoding scheme (MAJOR*10000 + MINOR*100 + PATCH)
*
*   This header detects:
*   - vendor disambiguation (Oracle MySQL vs MariaDB)
*   - client library presence (any MySQL-compatible C client)
*   - C API features common to all modern versions of both products
*   - embedded server library detection
*   - core storage engine detection (via compile-time defines)
*   - basic SSL/TLS library detection
*   - fundamental authentication and protocol features
*
*   DESIGN RATIONALE:
*   Version-gated features are NOT placed here because MySQL and MariaDB
* use entirely different version numbering after the fork (e.g., MySQL 5.7
* vs MariaDB 10.2, MySQL 8.0 vs MariaDB 10.5). Each vendor header
* (env_mysql.h, env_mariadb.h) handles its own version-specific gating
* and includes this common header for shared infrastructure.
*
*   NAMING CONVENTION:
*   D_ENV_MYSQL_COMMON_[FEATURE]  - shared capability flag (1/0)
*
*   USAGE:
*   Not intended for direct inclusion by application code. Include
* env_mysql.h or env_mariadb.h instead, which will pull in this header
* automatically.
*
* 
* path:      /inc/djinterp/core/env/db/mysql/env_mysql_common.h
* link(s)    TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_MYSQL_COMMON_
#define DJINTERP_ENVIRONMENT_MYSQL_COMMON_ 1

// env_mysql_common.h is pulled in by BOTH env_mariadb.h and env_mysql.h.
// To remain entry-point-agnostic, pull in both vendor config files so that
// D_CFG_ENV_USING_MARIADB, D_CFG_ENV_USING_MYSQL, and their path/custom
// companions are always defined by the time the include logic below runs.
#include "../../../config/core/env/db/mariadb/env_mariadb_config.h"
#include "../../../config/core/env/db/mysql/env_mysql_config.h"


// =============================================================================
// 0.   VENDOR HEADER INCLUSION
// =============================================================================
//   The MySQL family shares a single C client surface (libmysqlclient /
// MariaDB Connector/C / mysql.h). This section performs that inclusion once,
// driven by D_CFG_ENV_USING_MARIADB and D_CFG_ENV_USING_MYSQL from
// env_config.h. Because env_mysql.h and env_mariadb.h both include this
// header, neither of them needs to repeat the include logic themselves.
//
//   For each enabled vendor:
//     1. If __has_include is available, try D_CFG_ENV_<V>_C_PATH first, then
//        fall back to well-known alternate locations.
//     2. If __has_include is not available, trust the configured path and
//        #include it directly.
//     3. If nothing resolves, #error with actionable guidance.
//
//   The sentinels
//     D_ENV_MARIADB_HEADER_INCLUDED
//     D_ENV_MARIADB_CPP_HEADER_INCLUDED
//     D_ENV_MYSQL_HEADER_INCLUDED
//     D_ENV_MYSQL_CPP_HEADER_INCLUDED
// are set to 1 when their header is present, 0 otherwise. Downstream
// detection must gate on these so that no vendor symbol is referenced
// unless the header is actually in scope.


// --- MariaDB: C client header ---
#if (D_CFG_ENV_USING_MARIADB == 1)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MARIADB_C_PATH)
            #include D_CFG_ENV_MARIADB_C_PATH
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #elif __has_include(<mariadb/mysql.h>)
            #include <mariadb/mysql.h>
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #elif __has_include(<mysql/mysql.h>)
            #include <mysql/mysql.h>
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #elif __has_include(<mysql.h>)
            #include <mysql.h>
            #define D_ENV_MARIADB_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_MARIADB=1 but no MariaDB/MySQL C "       \
                   "client header was found. Install the MariaDB "           \
                   "Connector/C development package, or define "             \
                   "D_CFG_ENV_MARIADB_C_PATH to the correct location."
        #endif
    #else
        // pre-C++17 / pre-C23: no __has_include; trust configured path
        #include D_CFG_ENV_MARIADB_C_PATH
        #define D_ENV_MARIADB_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_C
        #define D_ENV_DB_HAS_MARIADB_CONNECTOR_C 1
    #endif

#else
    #define D_ENV_MARIADB_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_C
        #define D_ENV_DB_HAS_MARIADB_CONNECTOR_C 0
    #endif
#endif  // D_CFG_ENV_USING_MARIADB


// --- MariaDB: C++ connector header (C++ builds only) ---
#if ( (D_CFG_ENV_USING_MARIADB == 1) && defined(__cplusplus) )

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MARIADB_CPP_PATH)
            #include D_CFG_ENV_MARIADB_CPP_PATH
            #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 1
            #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 1
            #endif
        #else
            #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 0
            #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 0
            #endif
        #endif
    #else
        // no __has_include: C++ connector is opt-in only when the user has
        // explicitly overridden the path (otherwise we silently skip to
        // avoid pulling in libmariadbcpp symbols unexpectedly).
        #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 0
        #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
            #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 0
        #endif
    #endif

#else
    #define D_ENV_MARIADB_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP
        #define D_ENV_DB_HAS_MARIADB_CONNECTOR_CPP 0
    #endif
#endif  // D_CFG_ENV_USING_MARIADB && __cplusplus


// --- MySQL (Oracle): C client header ---
#if (D_CFG_ENV_USING_MYSQL == 1)

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MYSQL_C_PATH)
            #include D_CFG_ENV_MYSQL_C_PATH
            #define D_ENV_MYSQL_HEADER_INCLUDED 1
        #elif __has_include(<mysql/mysql.h>)
            #include <mysql/mysql.h>
            #define D_ENV_MYSQL_HEADER_INCLUDED 1
        #elif __has_include(<mysql.h>)
            #include <mysql.h>
            #define D_ENV_MYSQL_HEADER_INCLUDED 1
        #else
            #error "D_CFG_ENV_USING_MYSQL=1 but no MySQL C client header "   \
                   "was found. Install libmysqlclient-dev (or equivalent), " \
                   "or define D_CFG_ENV_MYSQL_C_PATH to the correct location."
        #endif
    #else
        #include D_CFG_ENV_MYSQL_C_PATH
        #define D_ENV_MYSQL_HEADER_INCLUDED 1
    #endif

    #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_C
        #define D_ENV_DB_HAS_MYSQL_CONNECTOR_C 1
    #endif

#else
    #define D_ENV_MYSQL_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_C
        #define D_ENV_DB_HAS_MYSQL_CONNECTOR_C 0
    #endif
#endif  // D_CFG_ENV_USING_MYSQL


// --- MySQL (Oracle): X DevAPI C++ header (C++ builds only) ---
#if ( (D_CFG_ENV_USING_MYSQL == 1) && defined(__cplusplus) )

    #if defined(__has_include)
        #if __has_include(D_CFG_ENV_MYSQL_CPP_PATH)
            #include D_CFG_ENV_MYSQL_CPP_PATH
            #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 1
            #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 1
            #endif
        #else
            #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 0
            #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
                #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 0
            #endif
        #endif
    #else
        #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 0
        #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
            #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 0
        #endif
    #endif

#else
    #define D_ENV_MYSQL_CPP_HEADER_INCLUDED 0
    #ifndef D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP
        #define D_ENV_DB_HAS_MYSQL_CONNECTOR_CPP 0
    #endif
#endif  // D_CFG_ENV_USING_MYSQL && __cplusplus

// =============================================================================
// I.   VENDOR DISAMBIGUATION
// =============================================================================
//   MariaDB defines MYSQL_VERSION_ID alongside MARIADB_VERSION_ID for
// compatibility. We must check the MariaDB sentinels first to avoid
// misidentifying a MariaDB build as Oracle MySQL.

// D_ENV_MYSQL_COMMON_IS_MARIADB
//   detection: 1 if the build environment is MariaDB, 0 if Oracle MySQL
// or undetected.
#ifndef D_ENV_MYSQL_COMMON_IS_MARIADB
    #if ( defined(MARIADB_VERSION_ID)           ||                            \
          defined(MARIADB_BASE_VERSION)         ||                            \
          defined(MARIADB_CLIENT_VERSION_STR)   ||                            \
          defined(MARIADB_PACKAGE_VERSION) )
        #define D_ENV_MYSQL_COMMON_IS_MARIADB 1
    #else
        #define D_ENV_MYSQL_COMMON_IS_MARIADB 0
    #endif
#endif

// D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL
//   detection: 1 if the build environment is Oracle MySQL (MYSQL_VERSION_ID
// defined AND not MariaDB).
#ifndef D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL
    #if ( defined(MYSQL_VERSION_ID) &&                                        \
          (!D_ENV_MYSQL_COMMON_IS_MARIADB) )
        #define D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL 1
    #else
        #define D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL 0
    #endif
#endif

// D_ENV_MYSQL_COMMON_FAMILY_DETECTED
//   detection: 1 if any MySQL-compatible environment is detected (either
// Oracle MySQL or MariaDB).
#ifndef D_ENV_MYSQL_COMMON_FAMILY_DETECTED
    #if ( D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL ||                               \
          D_ENV_MYSQL_COMMON_IS_MARIADB )
        #define D_ENV_MYSQL_COMMON_FAMILY_DETECTED 1
    #else
        #define D_ENV_MYSQL_COMMON_FAMILY_DETECTED 0
    #endif
#endif


// =============================================================================
// II.  VERSION ENCODING
// =============================================================================
//   Both Oracle MySQL and MariaDB use the same encoding scheme:
//     MAJOR * 10000 + MINOR * 100 + PATCH
//   e.g. MySQL 8.0.35 = 80035, MariaDB 11.4.2 = 110402.

// D_ENV_MYSQL_COMMON_ENCODE_VERSION
//   macro: encodes a (major, minor, patch) triple into the standard
// MySQL-family version ID format.
#define D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))

// D_ENV_MYSQL_COMMON_DECODE_MAJOR
//   macro: extracts the major version from an encoded version ID.
#define D_ENV_MYSQL_COMMON_DECODE_MAJOR(ver_id) \
    ((ver_id) / 10000)

// D_ENV_MYSQL_COMMON_DECODE_MINOR
//   macro: extracts the minor version from an encoded version ID.
#define D_ENV_MYSQL_COMMON_DECODE_MINOR(ver_id) \
    (((ver_id) / 100) % 100)

// D_ENV_MYSQL_COMMON_DECODE_PATCH
//   macro: extracts the patch version from an encoded version ID.
#define D_ENV_MYSQL_COMMON_DECODE_PATCH(ver_id) \
    ((ver_id) % 100)


// =============================================================================
// III. CLIENT LIBRARY DETECTION
// =============================================================================
//   These macros detect the presence of a MySQL-compatible C client library
// regardless of vendor. They do NOT depend on version numbering.

// D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
//   feature: detect if any MySQL-compatible client library is available.
#ifndef D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB
    #if ( defined(MYSQL_VERSION_ID)     ||  \
          defined(LIBMYSQL_VERSION_ID)  ||  \
          defined(MARIADB_VERSION_ID) )
        #define D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB 1
    #else
        #define D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB 0
    #endif
#endif

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
#endif


// =============================================================================
// IV.  C API FEATURES (VERSION-AGNOSTIC)
// =============================================================================
//   These C API features have existed since before the MySQL 5.5 fork point
// and are present in every modern build of both Oracle MySQL and MariaDB.
// They depend only on a client library being detected, not on version.

#if D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB

// -----------------------------------------------------------------------------
// A.  Connection fundamentals
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_REAL_CONNECT
    //   feature: mysql_real_connect() is available (present since MySQL 3.x).
    #define D_ENV_MYSQL_COMMON_HAS_REAL_CONNECT        1

    // D_ENV_MYSQL_COMMON_HAS_CHANGE_USER
    //   feature: mysql_change_user() is available (present since MySQL 3.x).
    #define D_ENV_MYSQL_COMMON_HAS_CHANGE_USER         1

    // D_ENV_MYSQL_COMMON_HAS_PING
    //   feature: mysql_ping() is available.
    #define D_ENV_MYSQL_COMMON_HAS_PING                1

    // D_ENV_MYSQL_COMMON_HAS_SELECT_DB
    //   feature: mysql_select_db() is available.
    #define D_ENV_MYSQL_COMMON_HAS_SELECT_DB           1

    // D_ENV_MYSQL_COMMON_HAS_SET_CHARACTER_SET
    //   feature: mysql_set_character_set() is available (since 5.0.7).
    #define D_ENV_MYSQL_COMMON_HAS_SET_CHARACTER_SET   1

    // D_ENV_MYSQL_COMMON_HAS_OPTIONS
    //   feature: mysql_options() / mysql_options4() are available.
    #define D_ENV_MYSQL_COMMON_HAS_OPTIONS             1

    // D_ENV_MYSQL_COMMON_HAS_AUTOCOMMIT
    //   feature: mysql_autocommit() is available.
    #define D_ENV_MYSQL_COMMON_HAS_AUTOCOMMIT          1

    // D_ENV_MYSQL_COMMON_HAS_COMMIT_ROLLBACK
    //   feature: mysql_commit() and mysql_rollback() are available.
    #define D_ENV_MYSQL_COMMON_HAS_COMMIT_ROLLBACK     1


// -----------------------------------------------------------------------------
// B.  Query execution
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_REAL_QUERY
    //   feature: mysql_real_query() is available.
    #define D_ENV_MYSQL_COMMON_HAS_REAL_QUERY          1

    // D_ENV_MYSQL_COMMON_HAS_REAL_ESCAPE_STRING
    //   feature: mysql_real_escape_string() is available.
    #define D_ENV_MYSQL_COMMON_HAS_REAL_ESCAPE_STRING  1

    // D_ENV_MYSQL_COMMON_HAS_MULTI_STATEMENTS
    //   feature: CLIENT_MULTI_STATEMENTS is available for executing
    // multiple SQL statements in a single call (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_MULTI_STATEMENTS    1

    // D_ENV_MYSQL_COMMON_HAS_MULTI_RESULTS
    //   feature: CLIENT_MULTI_RESULTS is available for processing result
    // sets from stored procedures and multi-statement queries.
    #define D_ENV_MYSQL_COMMON_HAS_MULTI_RESULTS       1

    // D_ENV_MYSQL_COMMON_HAS_NEXT_RESULT
    //   feature: mysql_next_result() is available for iterating
    // multi-result sets (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_NEXT_RESULT         1


// -----------------------------------------------------------------------------
// C.  Prepared statements
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_PREPARED_STATEMENTS
    //   feature: mysql_stmt_* API is available (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_PREPARED_STATEMENTS 1

    // D_ENV_MYSQL_COMMON_HAS_STMT_ATTR_CURSOR
    //   feature: server-side cursors via STMT_ATTR_CURSOR_TYPE are
    // available (since MySQL 5.0).
    #define D_ENV_MYSQL_COMMON_HAS_STMT_ATTR_CURSOR    1


// -----------------------------------------------------------------------------
// D.  Result set handling
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_STORE_RESULT
    //   feature: mysql_store_result() is available.
    #define D_ENV_MYSQL_COMMON_HAS_STORE_RESULT        1

    // D_ENV_MYSQL_COMMON_HAS_USE_RESULT
    //   feature: mysql_use_result() (streaming result set) is available.
    #define D_ENV_MYSQL_COMMON_HAS_USE_RESULT          1

    // D_ENV_MYSQL_COMMON_HAS_FETCH_ROW
    //   feature: mysql_fetch_row() is available.
    #define D_ENV_MYSQL_COMMON_HAS_FETCH_ROW           1

    // D_ENV_MYSQL_COMMON_HAS_FETCH_FIELDS
    //   feature: mysql_fetch_fields() / mysql_fetch_field() are available.
    #define D_ENV_MYSQL_COMMON_HAS_FETCH_FIELDS        1

    // D_ENV_MYSQL_COMMON_HAS_NUM_FIELDS
    //   feature: mysql_num_fields() is available.
    #define D_ENV_MYSQL_COMMON_HAS_NUM_FIELDS          1

    // D_ENV_MYSQL_COMMON_HAS_NUM_ROWS
    //   feature: mysql_num_rows() is available.
    #define D_ENV_MYSQL_COMMON_HAS_NUM_ROWS            1

    // D_ENV_MYSQL_COMMON_HAS_AFFECTED_ROWS
    //   feature: mysql_affected_rows() is available.
    #define D_ENV_MYSQL_COMMON_HAS_AFFECTED_ROWS       1

    // D_ENV_MYSQL_COMMON_HAS_INSERT_ID
    //   feature: mysql_insert_id() is available.
    #define D_ENV_MYSQL_COMMON_HAS_INSERT_ID           1

    // D_ENV_MYSQL_COMMON_HAS_DATA_SEEK
    //   feature: mysql_data_seek() is available.
    #define D_ENV_MYSQL_COMMON_HAS_DATA_SEEK           1

    // D_ENV_MYSQL_COMMON_HAS_ROW_SEEK
    //   feature: mysql_row_seek() / mysql_row_tell() are available.
    #define D_ENV_MYSQL_COMMON_HAS_ROW_SEEK            1


// -----------------------------------------------------------------------------
// E.  Protocol features
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_COMPRESSED_PROTOCOL
    //   feature: CLIENT_COMPRESS (zlib compression) is available
    // (since MySQL 3.22).
    #define D_ENV_MYSQL_COMMON_HAS_COMPRESSED_PROTOCOL 1


// -----------------------------------------------------------------------------
// F.  Error and status
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_ERRNO
    //   feature: mysql_errno() is available.
    #define D_ENV_MYSQL_COMMON_HAS_ERRNO               1

    // D_ENV_MYSQL_COMMON_HAS_ERROR
    //   feature: mysql_error() is available.
    #define D_ENV_MYSQL_COMMON_HAS_ERROR               1

    // D_ENV_MYSQL_COMMON_HAS_SQLSTATE
    //   feature: mysql_sqlstate() (SQLSTATE error codes) is available
    // (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_SQLSTATE            1

    // D_ENV_MYSQL_COMMON_HAS_WARNING_COUNT
    //   feature: mysql_warning_count() is available (since MySQL 4.1).
    #define D_ENV_MYSQL_COMMON_HAS_WARNING_COUNT       1

    // D_ENV_MYSQL_COMMON_HAS_INFO
    //   feature: mysql_info() is available.
    #define D_ENV_MYSQL_COMMON_HAS_INFO                1

    // D_ENV_MYSQL_COMMON_HAS_SERVER_INFO
    //   feature: mysql_get_server_info() is available.
    #define D_ENV_MYSQL_COMMON_HAS_SERVER_INFO         1

    // D_ENV_MYSQL_COMMON_HAS_SERVER_VERSION
    //   feature: mysql_get_server_version() is available.
    #define D_ENV_MYSQL_COMMON_HAS_SERVER_VERSION      1

    // D_ENV_MYSQL_COMMON_HAS_CLIENT_INFO
    //   feature: mysql_get_client_info() / mysql_get_client_version() are
    // available.
    #define D_ENV_MYSQL_COMMON_HAS_CLIENT_INFO         1

    // D_ENV_MYSQL_COMMON_HAS_STAT
    //   feature: mysql_stat() is available.
    #define D_ENV_MYSQL_COMMON_HAS_STAT                1

    // D_ENV_MYSQL_COMMON_HAS_THREAD_ID
    //   feature: mysql_thread_id() is available.
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_ID           1


// -----------------------------------------------------------------------------
// G.  Thread safety
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_THREAD_SAFE
    //   feature: mysql_thread_safe() is available (since MySQL 4.0).
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_SAFE         1

    // D_ENV_MYSQL_COMMON_HAS_THREAD_INIT
    //   feature: mysql_thread_init() / mysql_thread_end() are available.
    #define D_ENV_MYSQL_COMMON_HAS_THREAD_INIT         1

    // D_ENV_MYSQL_COMMON_HAS_LIBRARY_INIT
    //   feature: mysql_library_init() / mysql_library_end() are available
    // (since MySQL 5.0).
    #define D_ENV_MYSQL_COMMON_HAS_LIBRARY_INIT        1


// -----------------------------------------------------------------------------
// H.  Authentication
// -----------------------------------------------------------------------------

    // D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE
    //   feature: mysql_native_password authentication plugin is available.
    // Present since MySQL 4.1 in both MySQL and MariaDB.
    #define D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE         1

    // D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH
    //   feature: the pluggable authentication framework is available.
    // Present since MySQL 5.5.7, inherited by MariaDB at the fork.
    #define D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH      1


// =============================================================================
// V.   CORE STORAGE ENGINE DETECTION
// =============================================================================
//   Engines that are always compiled in, or detected via vendor-provided
// compile-time defines. These do not depend on version gating.

    // D_ENV_MYSQL_COMMON_HAS_INNODB
    //   feature: InnoDB is available (default engine since MySQL 5.5;
    // always present in both MySQL and MariaDB builds).
    #define D_ENV_MYSQL_COMMON_HAS_INNODB              1

    // D_ENV_MYSQL_COMMON_HAS_MYISAM
    //   feature: MyISAM is available (always compiled in).
    #define D_ENV_MYSQL_COMMON_HAS_MYISAM              1

    // D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE
    //   feature: MEMORY (HEAP) engine is available (always compiled in).
    #define D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE       1

    // D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE
    //   feature: ARCHIVE engine is available (compiled in by default in
    // both MySQL and MariaDB).
    #define D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE      1

    // D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE
    //   feature: CSV engine is available (compiled in by default).
    #define D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE          1

    // D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE
    //   feature: BLACKHOLE engine is available (compiled in by default).
    #define D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE    1

    // D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
    //   feature: NDB Cluster (MySQL Cluster) engine is available.
    // Detected via compile-time defines set by the NDB build system.
    #ifndef D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
        #if ( defined(HAVE_NDBCLUSTER)  ||  \
              defined(NDB_VERSION_MAJOR) )
            #define D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER 0
        #endif
    #endif

    // D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
    //   feature: FEDERATED engine is available.
    // note: disabled by default in most distributions of both products.
    #ifndef D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
        #if defined(HAVE_FEDERATED)
            #define D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE 0
        #endif
    #endif


// =============================================================================
// VI.  SSL/TLS LIBRARY DETECTION
// =============================================================================
//   Both products support SSL, but the underlying library varies (OpenSSL,
// wolfSSL, yaSSL). This section detects the SSL backend, not protocol
// features (which are version-gated and belong in vendor headers).

    // D_ENV_MYSQL_COMMON_HAS_OPENSSL
    //   feature: the client library was built with OpenSSL.
    #ifndef D_ENV_MYSQL_COMMON_HAS_OPENSSL
        #if defined(HAVE_OPENSSL)
            #define D_ENV_MYSQL_COMMON_HAS_OPENSSL 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_OPENSSL 0
        #endif
    #endif

    // D_ENV_MYSQL_COMMON_HAS_WOLFSSL
    //   feature: the client library was built with wolfSSL.
    #ifndef D_ENV_MYSQL_COMMON_HAS_WOLFSSL
        #if defined(HAVE_WOLFSSL)
            #define D_ENV_MYSQL_COMMON_HAS_WOLFSSL 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_WOLFSSL 0
        #endif
    #endif

    // D_ENV_MYSQL_COMMON_HAS_YASSL
    //   feature: the client library was built with yaSSL (legacy;
    // bundled in MySQL 5.x, removed in 8.0).
    #ifndef D_ENV_MYSQL_COMMON_HAS_YASSL
        #if defined(HAVE_YASSL)
            #define D_ENV_MYSQL_COMMON_HAS_YASSL 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_YASSL 0
        #endif
    #endif

    // D_ENV_MYSQL_COMMON_HAS_ANY_SSL
    //   feature: the client library has some SSL/TLS support.
    #define D_ENV_MYSQL_COMMON_HAS_ANY_SSL \
        ( D_ENV_MYSQL_COMMON_HAS_OPENSSL || \
          D_ENV_MYSQL_COMMON_HAS_WOLFSSL || \
          D_ENV_MYSQL_COMMON_HAS_YASSL )


// =============================================================================
// VII. COMMON DATA TYPES
// =============================================================================
//   Types present in every MySQL-compatible server and client since before
// the fork point.

    // D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES
    //   feature: spatial/geometry types (POINT, LINESTRING, POLYGON, etc.)
    // are available. Present since MySQL 4.1 in both products.
    #define D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES      1

    // D_ENV_MYSQL_COMMON_HAS_BLOB_TYPES
    //   feature: BLOB/TEXT family (TINYBLOB through LONGBLOB) is available.
    #define D_ENV_MYSQL_COMMON_HAS_BLOB_TYPES          1

    // D_ENV_MYSQL_COMMON_HAS_BIT_TYPE
    //   feature: BIT data type is available (since MySQL 5.0.3).
    #define D_ENV_MYSQL_COMMON_HAS_BIT_TYPE            1

    // D_ENV_MYSQL_COMMON_HAS_ENUM_TYPE
    //   feature: ENUM data type is available.
    #define D_ENV_MYSQL_COMMON_HAS_ENUM_TYPE           1

    // D_ENV_MYSQL_COMMON_HAS_SET_TYPE
    //   feature: SET data type is available.
    #define D_ENV_MYSQL_COMMON_HAS_SET_TYPE            1


// =============================================================================
// VIII. CHARACTER SET BASICS
// =============================================================================

    // D_ENV_MYSQL_COMMON_HAS_UTF8MB3
    //   feature: utf8 (3-byte, aliased utf8mb3) character set is available.
    // Present in both products since well before the fork.
    #define D_ENV_MYSQL_COMMON_HAS_UTF8MB3             1

    // D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3
    //   status: 1 if the 'utf8' charset alias refers to utf8mb3 (3-byte).
    // This is true in ALL versions of both products as of 2025. Neither
    // product has changed the alias to mean utf8mb4.
    #define D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3         1


// =============================================================================
// IX.  PLATFORM CONNECTION METHODS
// =============================================================================

    // D_ENV_MYSQL_COMMON_HAS_TCP_IP
    //   feature: TCP/IP connections are available (always).
    #define D_ENV_MYSQL_COMMON_HAS_TCP_IP              1

    // D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
    //   feature: UNIX domain socket connections are available (POSIX).
    #ifndef D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
        #if D_ENV_IS_OS_POSIX_LIKE(D_ENV_OS_ID)
            #define D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET 0
        #endif
    #endif

    // D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
    //   feature: named pipe connections are available (Windows).
    #ifndef D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
        #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
            #define D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE 0
        #endif
    #endif

    // D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY
    //   feature: shared memory connections are available (Windows).
    #ifndef D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY
        #if D_ENV_IS_OS_WINDOWS(D_ENV_OS_ID)
            #define D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY 1
        #else
            #define D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY 0
        #endif
    #endif


#else
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
    #endif
    #ifndef D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
        #define D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE 0
    #endif

    // SSL
    #ifndef D_ENV_MYSQL_COMMON_HAS_OPENSSL
        #define D_ENV_MYSQL_COMMON_HAS_OPENSSL         0
    #endif
    #ifndef D_ENV_MYSQL_COMMON_HAS_WOLFSSL
        #define D_ENV_MYSQL_COMMON_HAS_WOLFSSL         0
    #endif
    #ifndef D_ENV_MYSQL_COMMON_HAS_YASSL
        #define D_ENV_MYSQL_COMMON_HAS_YASSL           0
    #endif
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
    #endif
    #ifndef D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
        #define D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE      0
    #endif
    #ifndef D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY
        #define D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY   0
    #endif

#endif  // D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB


#endif  // DJINTERP_ENVIRONMENT_MYSQL_COMMON_
