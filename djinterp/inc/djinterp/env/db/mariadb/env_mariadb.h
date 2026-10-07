/*******************************************************************************
* djinterp [env]                                                   env_mariadb.h
*
* djinterp MariaDB environment detection.
*   Compile-time detection of a MariaDB environment: the server version and
* its comparison macros, release series and LTS status, the client library
* (MariaDB Connector/C), and the version-gated capabilities built on them: C
* API features, SSL/TLS, authentication plugins, MariaDB's own storage engines,
* SQL extensions and data types, InnoDB, replication and Galera, character
* sets, the optimizer, and security and administration.
*   This header is for MariaDB only; Oracle MySQL has env_mysql.h. The two
* share env_mysql_common.h, which includes the client headers when
* D_CFG_ENV_USING_MARIADB is on, raises #error when they can't be found, and
* sets D_ENV_MARIADB_HEADER_INCLUDED accordingly. Versions are encoded as
* MAJOR*10000 + MINOR*100 + PATCH, as in MARIADB_VERSION_ID.
*   D_ENV_MARIADB_HAS_* are capability flags, D_ENV_MARIADB_VERSION_* the
* version and its parts, and D_ENV_MARIADB_IS_* the release series. Sections 2
* onward exist only when MariaDB is detected. Settings live in
* cfg_env_mariadb.h; D_CFG_ENV_MARIADB_CUSTOM, or any pre-defined
* D_ENV_MARIADB_DETECTED_*, switches to manual detection.
*   MariaDB forked from MySQL 5.5. Its series: 5.5, the initial fork; 10.0,
* the first major divergence; 10.1, integrated Galera and encryption at rest;
* 10.2, window functions and CTEs; 10.3, system-versioned tables, sequences
* and invisible columns; 10.4, instant ALTER and ed25519 authentication; 10.5,
* the S3 engine, integrated ColumnStore and INET6; 10.6, an LTS release with
* sys_schema; 10.7 to 10.11, incremental; 11.0, the first post-10.x series,
* with breaking changes; and 11.1 to 11.4, current development and LTS.
*
*
* path:      /inc/djinterp/env/db/mariadb/env_mariadb.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.06.15
*                                                            revised: 2026.10.04
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VERSION DETECTION
    -----------------
    1.  Release version IDs
         1.  D_ENV_MARIADB_VERSION_<MAJOR>_<MINOR>_<PATCH>
    2.  Detected version
         1.  D_ENV_MARIADB_DETECTED / D_ENV_MARIADB_VERSION_*
2.  VERSION COMPARISON MACROS
    -------------------------
    1.  Comparisons
         1.  D_ENV_MARIADB_VERSION_AT_LEAST
         2.  D_ENV_MARIADB_VERSION_BELOW
         3.  D_ENV_MARIADB_VERSION_EXACT
         4.  D_ENV_MARIADB_VERSION_IN_RANGE
    2.  Release series
         1.  D_ENV_MARIADB_IS_<SERIES>
    3.  Release model
         1.  D_ENV_MARIADB_IS_LTS
3.  CLIENT LIBRARY (MARIADB-SPECIFIC)
    ---------------------------------
    1.  Client library
         1.  D_ENV_MARIADB_HAS_CLIENT_LIB
         2.  D_ENV_MARIADB_HAS_CONNECTOR_C
         3.  D_ENV_MARIADB_HAS_EMBEDDED
         4.  D_ENV_MARIADB_HAS_ASYNC_API
4.  C API FEATURES (MARIADB VERSION-GATED)
    --------------------------------------
    1.  C API features
         1.  D_ENV_MARIADB_HAS_RESET_CONNECTION
         2.  D_ENV_MARIADB_HAS_SESSION_TRACK
         3.  D_ENV_MARIADB_HAS_GET_OPTION
         4.  D_ENV_MARIADB_HAS_STMT_NEXT_RESULT
5.  SSL/TLS FEATURES
    ----------------
    1.  SSL and TLS
         1.  D_ENV_MARIADB_HAS_SSL
         2.  D_ENV_MARIADB_HAS_TLS_VERSION_OPTION
         3.  D_ENV_MARIADB_HAS_REQUIRE_SECURE_TRANSPORT
6.  AUTHENTICATION
    --------------
    1.  Authentication plugins
         1.  D_ENV_MARIADB_HAS_AUTH_NATIVE / D_ENV_MARIADB_HAS_PLUGGABLE_AUTH
         2.  D_ENV_MARIADB_HAS_AUTH_ED25519
         3.  D_ENV_MARIADB_DEFAULT_AUTH_IS_ED25519
         4.  D_ENV_MARIADB_HAS_AUTH_PAM
         5.  D_ENV_MARIADB_HAS_AUTH_PAM_V2
         6.  D_ENV_MARIADB_HAS_AUTH_GSSAPI
         7.  D_ENV_MARIADB_HAS_UNIX_SOCKET_AUTH
         8.  D_ENV_MARIADB_UNIX_SOCKET_AUTH_IS_DEFAULT
         9.  D_ENV_MARIADB_HAS_AUTH_HASHICORP
7.  STORAGE ENGINES (MARIADB-SPECIFIC)
    ----------------------------------
    1.  Storage engines
         1.  Common engine flags
         2.  D_ENV_MARIADB_HAS_ARIA
         3.  D_ENV_MARIADB_HAS_COLUMNSTORE
         4.  D_ENV_MARIADB_HAS_SPIDER
         5.  D_ENV_MARIADB_HAS_S3_ENGINE
         6.  D_ENV_MARIADB_HAS_CONNECT_ENGINE
         7.  D_ENV_MARIADB_HAS_MROONGA
         8.  D_ENV_MARIADB_HAS_SEQUENCE_ENGINE
         9.  D_ENV_MARIADB_HAS_FEDERATEDX
         10. D_ENV_MARIADB_HAS_ROCKSDB
8.  SQL EXTENSIONS (MARIADB-SPECIFIC)
    ---------------------------------
    1.  SQL extensions
         1.  D_ENV_MARIADB_HAS_SEQUENCES
         2.  D_ENV_MARIADB_HAS_SYSTEM_VERSIONED_TABLES
         3.  D_ENV_MARIADB_HAS_APPLICATION_TIME_PERIODS
         4.  D_ENV_MARIADB_HAS_RETURNING
         5.  D_ENV_MARIADB_HAS_INTERSECT
         6.  D_ENV_MARIADB_HAS_WINDOW_FUNCTIONS
         7.  D_ENV_MARIADB_HAS_CTE
         8.  D_ENV_MARIADB_HAS_CHECK_CONSTRAINTS
         9.  D_ENV_MARIADB_HAS_DEFAULT_EXPRESSION
         10. D_ENV_MARIADB_HAS_INVISIBLE_COLUMNS
         11. D_ENV_MARIADB_HAS_GENERATED_COLUMNS
         12. D_ENV_MARIADB_HAS_ORACLE_MODE
         13. D_ENV_MARIADB_HAS_ADD_PERIOD_FOR
         14. D_ENV_MARIADB_HAS_SKIP_LOCKED
         15. D_ENV_MARIADB_HAS_NATURAL_SORT
         16. D_ENV_MARIADB_HAS_LATERAL_DERIVED
9.  DATA TYPES (MARIADB-SPECIFIC)
    -----------------------------
    1.  Data types
         1.  D_ENV_MARIADB_HAS_JSON_TYPE
         2.  D_ENV_MARIADB_JSON_IS_ALIAS
         3.  D_ENV_MARIADB_HAS_JSON_TABLE
         4.  D_ENV_MARIADB_HAS_JSON_ARRAYAGG
         5.  D_ENV_MARIADB_HAS_GEOMETRY_TYPES
         6.  D_ENV_MARIADB_HAS_INET6_TYPE
         7.  D_ENV_MARIADB_HAS_UUID_TYPE
         8.  D_ENV_MARIADB_HAS_DESCENDING_INDEX
10. INNODB FEATURES (MARIADB-SPECIFIC)
    ----------------------------------
    1.  InnoDB
         1.  D_ENV_MARIADB_HAS_INNODB_FULLTEXT
         2.  D_ENV_MARIADB_HAS_INNODB_ONLINE_DDL
         3.  D_ENV_MARIADB_HAS_INNODB_INSTANT_DDL
         4.  D_ENV_MARIADB_HAS_INNODB_TABLESPACE_ENCRYPTION
         5.  D_ENV_MARIADB_HAS_INNODB_PAGE_COMPRESSION
         6.  D_ENV_MARIADB_HAS_INNODB_SPATIAL_INDEX
         7.  D_ENV_MARIADB_HAS_INNODB_UNDO_TRUNCATION
11. REPLICATION AND HIGH AVAILABILITY
    ---------------------------------
    1.  Replication and high availability
         1.  D_ENV_MARIADB_HAS_GTID
         2.  D_ENV_MARIADB_HAS_GALERA
         3.  D_ENV_MARIADB_HAS_GALERA_4
         4.  D_ENV_MARIADB_HAS_SEMI_SYNC_REPL
         5.  D_ENV_MARIADB_HAS_MULTI_SOURCE_REPL
         6.  D_ENV_MARIADB_HAS_PARALLEL_REPL
         7.  D_ENV_MARIADB_HAS_DELAYED_REPL
         8.  D_ENV_MARIADB_HAS_MARIADB_BACKUP
         9.  D_ENV_MARIADB_HAS_FLASHBACK
12. CHARACTER SET
    -------------
    1.  Character sets
         1.  D_ENV_MARIADB_HAS_UTF8MB4
         2.  D_ENV_MARIADB_DEFAULT_CHARSET_IS_UTF8MB4
         3.  D_ENV_MARIADB_HAS_UTF8MB4_UNICODE_520
         4.  D_ENV_MARIADB_HAS_UCA_14
         5.  D_ENV_MARIADB_UTF8_IS_UTF8MB3
13. OPTIMIZER AND PERFORMANCE
    -------------------------
    1.  Optimizer and performance
         1.  D_ENV_MARIADB_HAS_OPTIMIZER_HINTS
         2.  D_ENV_MARIADB_HAS_HISTOGRAM_STATS
         3.  D_ENV_MARIADB_HAS_CONDITION_PUSHDOWN
         4.  D_ENV_MARIADB_HAS_EXPLAIN_ANALYZE
         5.  D_ENV_MARIADB_HAS_EXPLAIN_FORMAT_JSON
         6.  D_ENV_MARIADB_HAS_PERFORMANCE_SCHEMA
         7.  D_ENV_MARIADB_HAS_SYS_SCHEMA
         8.  D_ENV_MARIADB_HAS_QUERY_RESPONSE_TIME
         9.  D_ENV_MARIADB_HAS_THREADPOOL
14. SECURITY AND ADMINISTRATION
    ---------------------------
    1.  Security and administration
         1.  D_ENV_MARIADB_HAS_ROLES
         2.  D_ENV_MARIADB_HAS_ACCOUNT_LOCKING
         3.  D_ENV_MARIADB_HAS_PASSWORD_EXPIRY
         4.  D_ENV_MARIADB_HAS_DATA_AT_REST_ENCRYPTION
         5.  D_ENV_MARIADB_HAS_KEY_MANAGEMENT_PLUGINS
         6.  D_ENV_MARIADB_HAS_AUDIT_PLUGIN
         7.  D_ENV_MARIADB_HAS_ATOMIC_DDL
15. PLATFORM
    --------
    1.  Platform
16. COMPOSITE CHECKS
    ----------------
    1.  Composite checks
         1.  D_ENV_MARIADB_HAS_MODERN_SQL
         2.  D_ENV_MARIADB_HAS_MODERN_AUTH
         3.  D_ENV_MARIADB_HAS_MODERN_DDL
         4.  D_ENV_MARIADB_HAS_HA_SUITE
         5.  D_ENV_MARIADB_HAS_MODERN_SECURITY
         6.  D_ENV_MARIADB_IS_FULLY_MODERN
17. DEPRECATION AND REMOVAL
    -----------------------
    1.  Deprecations
         1.  D_ENV_MARIADB_REMOVED_EMBEDDED_SERVER
         2.  D_ENV_MARIADB_REMOVED_TOKUDB
         3.  D_ENV_MARIADB_REMOVED_CASSANDRA_ENGINE
         4.  D_ENV_MARIADB_DEPRECATED_QUERY_CACHE
         5.  D_ENV_MARIADB_REMOVED_QUERY_CACHE
*/

#ifndef DJINTERP_ENV_DB_MARIADB_ENV_MARIADB_H
#define DJINTERP_ENV_DB_MARIADB_ENV_MARIADB_H 1

// djinterp
#include "../mysql/env_mysql_common.h"  // D_ENV_MYSQL_COMMON_*, client headers


//==============================================================================
// 1.  VERSION DETECTION
//==============================================================================
// MariaDB encodes its version as MAJOR*10000 + MINOR*100 + PATCH, in
// MARIADB_VERSION_ID; MariaDB 11.4.2 is 110402.


// 1.1    Release version IDs
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_MARIADB_VERSION_<MAJOR>_<MINOR>_<PATCH>
//   constant: encoded IDs of the releases that gate features below.
#define D_ENV_MARIADB_VERSION_5_5_0      50500
#define D_ENV_MARIADB_VERSION_10_0_0    100000
#define D_ENV_MARIADB_VERSION_10_1_0    100100
#define D_ENV_MARIADB_VERSION_10_2_0    100200
#define D_ENV_MARIADB_VERSION_10_2_1    100201
#define D_ENV_MARIADB_VERSION_10_2_3    100203
#define D_ENV_MARIADB_VERSION_10_2_4    100204
#define D_ENV_MARIADB_VERSION_10_2_7    100207
#define D_ENV_MARIADB_VERSION_10_3_0    100300
#define D_ENV_MARIADB_VERSION_10_3_1    100301
#define D_ENV_MARIADB_VERSION_10_3_2    100302
#define D_ENV_MARIADB_VERSION_10_3_4    100304
#define D_ENV_MARIADB_VERSION_10_3_5    100305
#define D_ENV_MARIADB_VERSION_10_4_0    100400
#define D_ENV_MARIADB_VERSION_10_4_3    100403
#define D_ENV_MARIADB_VERSION_10_5_0    100500
#define D_ENV_MARIADB_VERSION_10_5_2    100502
#define D_ENV_MARIADB_VERSION_10_5_4    100504
#define D_ENV_MARIADB_VERSION_10_6_0    100600
#define D_ENV_MARIADB_VERSION_10_6_1    100601
#define D_ENV_MARIADB_VERSION_10_7_0    100700
#define D_ENV_MARIADB_VERSION_10_8_0    100800
#define D_ENV_MARIADB_VERSION_10_9_0    100900
#define D_ENV_MARIADB_VERSION_10_10_0   101000
#define D_ENV_MARIADB_VERSION_10_11_0   101100
#define D_ENV_MARIADB_VERSION_11_0_0    110000
#define D_ENV_MARIADB_VERSION_11_1_0    110100
#define D_ENV_MARIADB_VERSION_11_2_0    110200
#define D_ENV_MARIADB_VERSION_11_3_0    110300
#define D_ENV_MARIADB_VERSION_11_4_0    110400

// 1.2    Detected version
//------------------------------------------------------------------------------
// 1.2.1
// D_ENV_MARIADB_DETECTED / D_ENV_MARIADB_VERSION_*
//   detection: D_ENV_MARIADB_DETECTED is 1 when a MariaDB version is known,
// and D_ENV_MARIADB_VERSION_ID, _MAJOR, _MINOR, _PATCH and _STRING then
// describe it. Automatic mode needs the MariaDB header in scope
// (D_ENV_MARIADB_HEADER_INCLUDED, from env_mysql_common.h) and reads
// MARIADB_VERSION_ID. Manual mode (D_CFG_ENV_MARIADB_CUSTOM) reads
// D_ENV_MARIADB_DETECTED_VERSION, or one of D_ENV_MARIADB_DETECTED_5_5 to
// _11_4.
#if D_CFG_IS_OFF(D_CFG_ENV_MARIADB_CUSTOM)

    // automatic detection requires the MariaDB header to be in scope;
    // if D_CFG_ENV_USING_MARIADB was not enabled the sentinel is 0 and we
    // skip cleanly (no reference to MARIADB_VERSION_ID etc.)
    #if ( (D_ENV_MARIADB_HEADER_INCLUDED) &&                                   \
          (D_ENV_MYSQL_COMMON_IS_MARIADB) )
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       MARIADB_VERSION_ID
        #define D_ENV_MARIADB_VERSION_MAJOR                                    \
            D_ENV_MYSQL_COMMON_DECODE_MAJOR(MARIADB_VERSION_ID)
        #define D_ENV_MARIADB_VERSION_MINOR                                    \
            D_ENV_MYSQL_COMMON_DECODE_MINOR(MARIADB_VERSION_ID)
        #define D_ENV_MARIADB_VERSION_PATCH                                    \
            D_ENV_MYSQL_COMMON_DECODE_PATCH(MARIADB_VERSION_ID)

        #ifdef MARIADB_CLIENT_VERSION_STR
            #define D_ENV_MARIADB_VERSION_STRING                               \
                MARIADB_CLIENT_VERSION_STR
        #elif defined(MARIADB_BASE_VERSION)
            #define D_ENV_MARIADB_VERSION_STRING                               \
                MARIADB_BASE_VERSION
        #else
            #define D_ENV_MARIADB_VERSION_STRING "unknown"
        #endif  // MARIADB_CLIENT_VERSION_STR
    #else
        #define D_ENV_MARIADB_DETECTED         0
    #endif

#else
    // manual mode: use pre-defined detection variables
    #ifdef D_ENV_MARIADB_DETECTED_VERSION
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_DETECTED_VERSION
        #define D_ENV_MARIADB_VERSION_MAJOR                                    \
            D_ENV_MYSQL_COMMON_DECODE_MAJOR(D_ENV_MARIADB_DETECTED_VERSION)
        #define D_ENV_MARIADB_VERSION_MINOR                                    \
            D_ENV_MYSQL_COMMON_DECODE_MINOR(D_ENV_MARIADB_DETECTED_VERSION)
        #define D_ENV_MARIADB_VERSION_PATCH                                    \
            D_ENV_MYSQL_COMMON_DECODE_PATCH(D_ENV_MARIADB_DETECTED_VERSION)
        #define D_ENV_MARIADB_VERSION_STRING   "manual"

    #elif defined(D_ENV_MARIADB_DETECTED_11_4)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_11_4_0
        #define D_ENV_MARIADB_VERSION_MAJOR    11
        #define D_ENV_MARIADB_VERSION_MINOR    4
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "11.4.0"

    #elif defined(D_ENV_MARIADB_DETECTED_11_0)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_11_0_0
        #define D_ENV_MARIADB_VERSION_MAJOR    11
        #define D_ENV_MARIADB_VERSION_MINOR    0
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "11.0.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_11)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_11_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    11
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.11.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_6)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_6_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    6
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.6.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_5)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_5_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    5
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.5.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_4)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_4_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    4
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.4.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_3)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_3_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    3
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.3.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_2)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_2_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    2
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.2.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_1)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_1_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    1
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.1.0"

    #elif defined(D_ENV_MARIADB_DETECTED_10_0)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_10_0_0
        #define D_ENV_MARIADB_VERSION_MAJOR    10
        #define D_ENV_MARIADB_VERSION_MINOR    0
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "10.0.0"

    #elif defined(D_ENV_MARIADB_DETECTED_5_5)
        #define D_ENV_MARIADB_DETECTED         1
        #define D_ENV_MARIADB_VERSION_ID       D_ENV_MARIADB_VERSION_5_5_0
        #define D_ENV_MARIADB_VERSION_MAJOR    5
        #define D_ENV_MARIADB_VERSION_MINOR    5
        #define D_ENV_MARIADB_VERSION_PATCH    0
        #define D_ENV_MARIADB_VERSION_STRING   "5.5.0"

    #else
        #define D_ENV_MARIADB_DETECTED         0
    #endif  // D_ENV_MARIADB_DETECTED_VERSION

#endif  // D_CFG_ENV_MARIADB_CUSTOM


// sections 2 to 17 exist only when D_ENV_MARIADB_DETECTED is 1; see 1.2.1
#if D_ENV_MARIADB_DETECTED


//==============================================================================
// 2.  VERSION COMPARISON MACROS
//==============================================================================


// 2.1    Comparisons
//------------------------------------------------------------------------------
    // 2.1.1
    // D_ENV_MARIADB_VERSION_AT_LEAST
    //   macro: 1 if the detected version is at least major.minor.patch.
    #define D_ENV_MARIADB_VERSION_AT_LEAST(major, minor, patch)                \
        (D_ENV_MARIADB_VERSION_ID >=                                           \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    // 2.1.2
    // D_ENV_MARIADB_VERSION_BELOW
    //   macro: 1 if the detected version is below major.minor.patch.
    #define D_ENV_MARIADB_VERSION_BELOW(major, minor, patch)                   \
        (D_ENV_MARIADB_VERSION_ID <                                            \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    // 2.1.3
    // D_ENV_MARIADB_VERSION_EXACT
    //   macro: 1 if the detected version is exactly major.minor.patch.
    #define D_ENV_MARIADB_VERSION_EXACT(major, minor, patch)                   \
        (D_ENV_MARIADB_VERSION_ID ==                                           \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    // 2.1.4
    // D_ENV_MARIADB_VERSION_IN_RANGE
    //   macro: 1 if the detected version is at least the first triple and
    // below the second.
    #define D_ENV_MARIADB_VERSION_IN_RANGE(min_maj, min_min, min_pat,          \
                                           max_maj, max_min, max_pat)          \
        ( (D_ENV_MARIADB_VERSION_AT_LEAST(min_maj, min_min, min_pat)) &&       \
          (D_ENV_MARIADB_VERSION_BELOW(max_maj, max_min, max_pat)) )

// 2.2    Release series
//------------------------------------------------------------------------------
    // 2.2.1
    // D_ENV_MARIADB_IS_<SERIES>
    //   macro: 1 if the detected version is in that release series: 5.5, 10.0
    // to 10.6, 10.11 or 11.4. D_ENV_MARIADB_IS_11_PLUS covers 11.0 and later.
    #define D_ENV_MARIADB_IS_5_5                                               \
        D_ENV_MARIADB_VERSION_IN_RANGE(5, 5, 0, 10, 0, 0)
    #define D_ENV_MARIADB_IS_10_0                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 0, 0, 10, 1, 0)
    #define D_ENV_MARIADB_IS_10_1                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 1, 0, 10, 2, 0)
    #define D_ENV_MARIADB_IS_10_2                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 2, 0, 10, 3, 0)
    #define D_ENV_MARIADB_IS_10_3                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 3, 0, 10, 4, 0)
    #define D_ENV_MARIADB_IS_10_4                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 4, 0, 10, 5, 0)
    #define D_ENV_MARIADB_IS_10_5                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 5, 0, 10, 6, 0)
    #define D_ENV_MARIADB_IS_10_6                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 6, 0, 10, 7, 0)
    #define D_ENV_MARIADB_IS_10_11                                             \
        D_ENV_MARIADB_VERSION_IN_RANGE(10, 11, 0, 11, 0, 0)
    #define D_ENV_MARIADB_IS_11_PLUS                                           \
        D_ENV_MARIADB_VERSION_AT_LEAST(11, 0, 0)
    #define D_ENV_MARIADB_IS_11_4                                              \
        D_ENV_MARIADB_VERSION_IN_RANGE(11, 4, 0, 11, 5, 0)

// 2.3    Release model
//------------------------------------------------------------------------------
    // 2.3.1
    // D_ENV_MARIADB_IS_LTS
    //   macro: evaluates to 1 if this is a Long-Term Support release.
    // MariaDB LTS releases: 10.6 (until 2026), 10.11 (until 2028),
    // 11.4 (until 2029).
    #define D_ENV_MARIADB_IS_LTS                                               \
        ( (D_ENV_MARIADB_IS_10_6)  ||                                          \
          (D_ENV_MARIADB_IS_10_11) ||                                          \
          (D_ENV_MARIADB_IS_11_4) )


//==============================================================================
// 3.  CLIENT LIBRARY (MARIADB-SPECIFIC)
//==============================================================================


// 3.1    Client library
//------------------------------------------------------------------------------
    // 3.1.1
    // D_ENV_MARIADB_HAS_CLIENT_LIB
    //   feature: alias of D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB, which the MySQL
    // family shares.
    #define D_ENV_MARIADB_HAS_CLIENT_LIB                                       \
        D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB

    // 3.1.2
    // D_ENV_MARIADB_HAS_CONNECTOR_C
    //   feature: detect MariaDB Connector/C (standalone C client library,
    // distinct from libmysqlclient).
    #ifndef D_ENV_MARIADB_HAS_CONNECTOR_C
        #if ( (defined(MARIADB_PACKAGE_VERSION)) ||                            \
              (defined(MARIADB_CLIENT_VERSION_STR)) )
            #define D_ENV_MARIADB_HAS_CONNECTOR_C 1
        #else
            #define D_ENV_MARIADB_HAS_CONNECTOR_C 0
        #endif
    #endif  // D_ENV_MARIADB_HAS_CONNECTOR_C

    // 3.1.3
    // D_ENV_MARIADB_HAS_EMBEDDED
    //   feature: MariaDB embedded library. Removed in MariaDB 11.0.
    #ifndef D_ENV_MARIADB_HAS_EMBEDDED
        #if ( (D_ENV_MYSQL_COMMON_HAS_EMBEDDED) &&                             \
              (D_ENV_MARIADB_VERSION_BELOW(11, 0, 0)) )
            #define D_ENV_MARIADB_HAS_EMBEDDED 1
        #else
            #define D_ENV_MARIADB_HAS_EMBEDDED 0
        #endif
    #endif  // D_ENV_MARIADB_HAS_EMBEDDED

    // 3.1.4
    // D_ENV_MARIADB_HAS_ASYNC_API
    //   feature: MariaDB's non-blocking (asynchronous) C API.
    // MariaDB introduced non-blocking API in 5.5 (before MySQL did),
    // via mysql_real_connect_start/cont pattern.
    #ifndef D_ENV_MARIADB_HAS_ASYNC_API
        #if D_ENV_MARIADB_HAS_CLIENT_LIB
            #define D_ENV_MARIADB_HAS_ASYNC_API 1
        #else
            #define D_ENV_MARIADB_HAS_ASYNC_API 0
        #endif
    #endif  // D_ENV_MARIADB_HAS_ASYNC_API


//==============================================================================
// 4.  C API FEATURES (MARIADB VERSION-GATED)
//==============================================================================


// 4.1    C API features
//------------------------------------------------------------------------------
    // 4.1.1
    // D_ENV_MARIADB_HAS_RESET_CONNECTION
    //   feature: mysql_reset_connection() (MariaDB 10.2.4+).
    #define D_ENV_MARIADB_HAS_RESET_CONNECTION                                 \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 4)

    // 4.1.2
    // D_ENV_MARIADB_HAS_SESSION_TRACK
    //   feature: session state tracking (MariaDB 10.2.2+).
    #define D_ENV_MARIADB_HAS_SESSION_TRACK                                    \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 2)

    // 4.1.3
    // D_ENV_MARIADB_HAS_GET_OPTION
    //   feature: mysql_get_option() (MariaDB 10.0.0+).
    #define D_ENV_MARIADB_HAS_GET_OPTION                                       \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 0)

    // 4.1.4
    // D_ENV_MARIADB_HAS_STMT_NEXT_RESULT
    //   feature: mysql_stmt_next_result() (present since fork).
    #define D_ENV_MARIADB_HAS_STMT_NEXT_RESULT 1


//==============================================================================
// 5.  SSL/TLS FEATURES
//==============================================================================


// 5.1    SSL and TLS
//------------------------------------------------------------------------------
    // 5.1.1
    // D_ENV_MARIADB_HAS_SSL
    //   feature: SSL/TLS support.
    #ifndef D_ENV_MARIADB_HAS_SSL
        #if ( (D_ENV_MYSQL_COMMON_HAS_ANY_SSL) ||                              \
              (D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 0)) )
            #define D_ENV_MARIADB_HAS_SSL 1
        #else
            #define D_ENV_MARIADB_HAS_SSL 0
        #endif
    #endif  // D_ENV_MARIADB_HAS_SSL

    // 5.1.2
    // D_ENV_MARIADB_HAS_TLS_VERSION_OPTION
    //   feature: --tls-version option (MariaDB 10.4.6+).
    #define D_ENV_MARIADB_HAS_TLS_VERSION_OPTION                               \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 6)

    // 5.1.3
    // D_ENV_MARIADB_HAS_REQUIRE_SECURE_TRANSPORT
    //   feature: require_secure_transport variable (MariaDB 10.5.2+).
    #define D_ENV_MARIADB_HAS_REQUIRE_SECURE_TRANSPORT                         \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 5, 2)


//==============================================================================
// 6.  AUTHENTICATION
//==============================================================================


// 6.1    Authentication plugins
//------------------------------------------------------------------------------
    // 6.1.1
    // D_ENV_MARIADB_HAS_AUTH_NATIVE / D_ENV_MARIADB_HAS_PLUGGABLE_AUTH
    //   feature: aliases of the matching D_ENV_MYSQL_COMMON_HAS_* flags, which
    // the MySQL family shares.
    #define D_ENV_MARIADB_HAS_AUTH_NATIVE                                      \
        D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE
    #define D_ENV_MARIADB_HAS_PLUGGABLE_AUTH                                   \
        D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH

    // 6.1.2
    // D_ENV_MARIADB_HAS_AUTH_ED25519
    //   feature: ed25519 authentication plugin (MariaDB 10.1.22+).
    // default auth plugin since MariaDB 10.4.
    #define D_ENV_MARIADB_HAS_AUTH_ED25519                                     \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 22)

    // 6.1.3
    // D_ENV_MARIADB_DEFAULT_AUTH_IS_ED25519
    //   status: 1 if ed25519 or mysql_native_password is default.
    // note: MariaDB 10.4+ uses a ranked auth where unix_socket is tried
    // first, then mysql_native_password. ed25519 is recommended but not
    // the hard default.
    #define D_ENV_MARIADB_DEFAULT_AUTH_IS_ED25519 0

    // 6.1.4
    // D_ENV_MARIADB_HAS_AUTH_PAM
    //   feature: PAM authentication plugin (MariaDB 5.5+).
    #define D_ENV_MARIADB_HAS_AUTH_PAM                                         \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 6.1.5
    // D_ENV_MARIADB_HAS_AUTH_PAM_V2
    //   feature: PAM authentication plugin version 2 with dialog support
    // (MariaDB 10.4.0+).
    #define D_ENV_MARIADB_HAS_AUTH_PAM_V2                                      \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 0)

    // 6.1.6
    // D_ENV_MARIADB_HAS_AUTH_GSSAPI
    //   feature: GSSAPI/Kerberos authentication plugin (MariaDB 10.1.11+).
    #define D_ENV_MARIADB_HAS_AUTH_GSSAPI                                      \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 11)

    // 6.1.7
    // D_ENV_MARIADB_HAS_UNIX_SOCKET_AUTH
    //   feature: unix_socket authentication (MariaDB 5.5+, default
    // for root since 10.4).
    #ifndef D_ENV_MARIADB_HAS_UNIX_SOCKET_AUTH
        #if D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
            #define D_ENV_MARIADB_HAS_UNIX_SOCKET_AUTH 1
        #else
            #define D_ENV_MARIADB_HAS_UNIX_SOCKET_AUTH 0
        #endif
    #endif  // D_ENV_MARIADB_HAS_UNIX_SOCKET_AUTH

    // 6.1.8
    // D_ENV_MARIADB_UNIX_SOCKET_AUTH_IS_DEFAULT
    //   status: 1 if unix_socket is the default auth for root
    // (MariaDB 10.4+).
    #define D_ENV_MARIADB_UNIX_SOCKET_AUTH_IS_DEFAULT                          \
        ( (D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 0)) &&                        \
          (D_ENV_MARIADB_HAS_UNIX_SOCKET_AUTH) )

    // 6.1.9
    // D_ENV_MARIADB_HAS_AUTH_HASHICORP
    //   feature: hashicorp vault authentication (MariaDB 10.9.0+,
    // Enterprise only).
    #define D_ENV_MARIADB_HAS_AUTH_HASHICORP                                   \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 9, 0)


//==============================================================================
// 7.  STORAGE ENGINES (MARIADB-SPECIFIC)
//==============================================================================


// 7.1    Storage engines
//------------------------------------------------------------------------------
    // 7.1.1
    // Common engine flags
    //   feature: D_ENV_MARIADB_HAS_INNODB, D_ENV_MARIADB_HAS_MYISAM,
    // D_ENV_MARIADB_HAS_MEMORY_ENGINE, D_ENV_MARIADB_HAS_ARCHIVE_ENGINE,
    // D_ENV_MARIADB_HAS_CSV_ENGINE and D_ENV_MARIADB_HAS_BLACKHOLE_ENGINE,
    // aliases of the matching D_ENV_MYSQL_COMMON_HAS_* flags, which the MySQL
    // family shares.
    #define D_ENV_MARIADB_HAS_INNODB         D_ENV_MYSQL_COMMON_HAS_INNODB
    #define D_ENV_MARIADB_HAS_MYISAM         D_ENV_MYSQL_COMMON_HAS_MYISAM
    #define D_ENV_MARIADB_HAS_MEMORY_ENGINE                                    \
        D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE
    #define D_ENV_MARIADB_HAS_ARCHIVE_ENGINE                                   \
        D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE
    #define D_ENV_MARIADB_HAS_CSV_ENGINE     D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE
    #define D_ENV_MARIADB_HAS_BLACKHOLE_ENGINE                                 \
        D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE

    // 7.1.2
    // D_ENV_MARIADB_HAS_ARIA
    //   feature: Aria storage engine (crash-safe MyISAM replacement).
    // Present since MariaDB 5.1 (named Maria), renamed to Aria in 5.5.
    #define D_ENV_MARIADB_HAS_ARIA                                             \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 7.1.3
    // D_ENV_MARIADB_HAS_COLUMNSTORE
    //   feature: MariaDB ColumnStore engine (columnar analytics engine).
    // Integrated into MariaDB server packages since 10.5.4.
    #ifndef D_ENV_MARIADB_HAS_COLUMNSTORE
        #if ( (defined(MCS_VERSION_ID)) ||                                     \
              (D_ENV_MARIADB_VERSION_AT_LEAST(10, 5, 4)) )
            #define D_ENV_MARIADB_HAS_COLUMNSTORE 1
        #else
            #define D_ENV_MARIADB_HAS_COLUMNSTORE 0
        #endif
    #endif  // D_ENV_MARIADB_HAS_COLUMNSTORE

    // 7.1.4
    // D_ENV_MARIADB_HAS_SPIDER
    //   feature: Spider storage engine (sharding/federation).
    // Bundled since MariaDB 10.0.4.
    #define D_ENV_MARIADB_HAS_SPIDER                                           \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 4)

    // 7.1.5
    // D_ENV_MARIADB_HAS_S3_ENGINE
    //   feature: S3 storage engine (read-only tables on S3-compatible
    // object storage). Introduced in MariaDB 10.5.4.
    #define D_ENV_MARIADB_HAS_S3_ENGINE                                        \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 5, 4)

    // 7.1.6
    // D_ENV_MARIADB_HAS_CONNECT_ENGINE
    //   feature: CONNECT storage engine (access external data sources:
    // CSV, XML, JSON, ODBC, etc.). Bundled since MariaDB 10.0.
    #define D_ENV_MARIADB_HAS_CONNECT_ENGINE                                   \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 0)

    // 7.1.7
    // D_ENV_MARIADB_HAS_MROONGA
    //   feature: Mroonga full-text search engine (CJK-optimized).
    // Bundled since MariaDB 10.0.15.
    #define D_ENV_MARIADB_HAS_MROONGA                                          \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 15)

    // 7.1.8
    // D_ENV_MARIADB_HAS_SEQUENCE_ENGINE
    //   feature: SEQUENCE storage engine (virtual table generating
    // sequences on-the-fly). Bundled since MariaDB 10.0.
    #define D_ENV_MARIADB_HAS_SEQUENCE_ENGINE                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 0)

    // 7.1.9
    // D_ENV_MARIADB_HAS_FEDERATEDX
    //   feature: FederatedX engine (enhanced FEDERATED, MariaDB's fork).
    #define D_ENV_MARIADB_HAS_FEDERATEDX                                       \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 7.1.10
    // D_ENV_MARIADB_HAS_ROCKSDB
    //   feature: MyRocks (RocksDB) storage engine.
    // Bundled since MariaDB 10.2.5.
    #define D_ENV_MARIADB_HAS_ROCKSDB                                          \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 5)


//==============================================================================
// 8.  SQL EXTENSIONS (MARIADB-SPECIFIC)
//==============================================================================


// 8.1    SQL extensions
//------------------------------------------------------------------------------
    // 8.1.1
    // D_ENV_MARIADB_HAS_SEQUENCES
    //   feature: CREATE SEQUENCE / NEXT VALUE FOR / SETVAL() syntax.
    // Introduced in MariaDB 10.3.1.
    #define D_ENV_MARIADB_HAS_SEQUENCES                                        \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 3, 1)

    // 8.1.2
    // D_ENV_MARIADB_HAS_SYSTEM_VERSIONED_TABLES
    //   feature: system-versioned (temporal) tables with AS OF syntax.
    // Introduced in MariaDB 10.3.4.
    #define D_ENV_MARIADB_HAS_SYSTEM_VERSIONED_TABLES                          \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 3, 4)

    // 8.1.3
    // D_ENV_MARIADB_HAS_APPLICATION_TIME_PERIODS
    //   feature: application-time period tables (WITHOUT OVERLAPS,
    // FOR PORTION OF). Introduced in MariaDB 10.4.3.
    #define D_ENV_MARIADB_HAS_APPLICATION_TIME_PERIODS                         \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 3)

    // 8.1.4
    // D_ENV_MARIADB_HAS_RETURNING
    //   feature: DELETE/INSERT/REPLACE ... RETURNING clause.
    // Introduced in MariaDB 10.5.0.
    #define D_ENV_MARIADB_HAS_RETURNING                                        \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 5, 0)

    // 8.1.5
    // D_ENV_MARIADB_HAS_INTERSECT
    //   feature: INTERSECT and EXCEPT set operations.
    // Introduced in MariaDB 10.3.0.
    #define D_ENV_MARIADB_HAS_INTERSECT                                        \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 3, 0)

    // 8.1.6
    // D_ENV_MARIADB_HAS_WINDOW_FUNCTIONS
    //   feature: SQL window functions (ROW_NUMBER, RANK, etc.).
    // Introduced in MariaDB 10.2.0.
    #define D_ENV_MARIADB_HAS_WINDOW_FUNCTIONS                                 \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 0)

    // 8.1.7
    // D_ENV_MARIADB_HAS_CTE
    //   feature: Common Table Expressions (WITH / WITH RECURSIVE).
    // Introduced in MariaDB 10.2.1.
    #define D_ENV_MARIADB_HAS_CTE                                              \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 1)

    // 8.1.8
    // D_ENV_MARIADB_HAS_CHECK_CONSTRAINTS
    //   feature: enforced CHECK constraints.
    // MariaDB has enforced CHECK constraints since 10.2.1 (unlike
    // MySQL which silently ignored them until 8.0.16).
    #define D_ENV_MARIADB_HAS_CHECK_CONSTRAINTS                                \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 1)

    // 8.1.9
    // D_ENV_MARIADB_HAS_DEFAULT_EXPRESSION
    //   feature: DEFAULT (expression) for columns.
    // Introduced in MariaDB 10.2.1.
    #define D_ENV_MARIADB_HAS_DEFAULT_EXPRESSION                               \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 1)

    // 8.1.10
    // D_ENV_MARIADB_HAS_INVISIBLE_COLUMNS
    //   feature: invisible columns (not shown in SELECT *).
    // Introduced in MariaDB 10.3.3.
    #define D_ENV_MARIADB_HAS_INVISIBLE_COLUMNS                                \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 3, 3)

    // 8.1.11
    // D_ENV_MARIADB_HAS_GENERATED_COLUMNS
    //   feature: virtual and persistent generated columns.
    // Introduced in MariaDB 5.2 (virtual columns), present in 10.x+.
    #define D_ENV_MARIADB_HAS_GENERATED_COLUMNS                                \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 8.1.12
    // D_ENV_MARIADB_HAS_ORACLE_MODE
    //   feature: sql_mode=ORACLE compatibility mode.
    // Introduced in MariaDB 10.3.0.
    #define D_ENV_MARIADB_HAS_ORACLE_MODE                                      \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 3, 0)

    // 8.1.13
    // D_ENV_MARIADB_HAS_ADD_PERIOD_FOR
    //   feature: ADD PERIOD FOR syntax for temporal tables.
    // Introduced in MariaDB 10.5.0.
    #define D_ENV_MARIADB_HAS_ADD_PERIOD_FOR                                   \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 5, 0)

    // 8.1.14
    // D_ENV_MARIADB_HAS_SKIP_LOCKED
    //   feature: SELECT ... SKIP LOCKED syntax.
    // Introduced in MariaDB 10.6.0.
    #define D_ENV_MARIADB_HAS_SKIP_LOCKED                                      \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 6, 0)

    // 8.1.15
    // D_ENV_MARIADB_HAS_NATURAL_SORT
    //   feature: NATURAL_SORT_KEY() function for human-friendly sorting.
    // Introduced in MariaDB 10.7.0.
    #define D_ENV_MARIADB_HAS_NATURAL_SORT                                     \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 7, 0)

    // 8.1.16
    // D_ENV_MARIADB_HAS_LATERAL_DERIVED
    //   feature: LATERAL derived tables.
    // Introduced in MariaDB 10.11 (partial), 11.0+ (full).
    #define D_ENV_MARIADB_HAS_LATERAL_DERIVED                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(11, 0, 0)


//==============================================================================
// 9.  DATA TYPES (MARIADB-SPECIFIC)
//==============================================================================


// 9.1    Data types
//------------------------------------------------------------------------------
    // 9.1.1
    // D_ENV_MARIADB_HAS_JSON_TYPE
    //   feature: JSON data type support. MariaDB does NOT have a native
    // binary JSON type like MySQL; JSON is an alias for LONGTEXT with a
    // CHECK (JSON_VALID()) constraint. Available since 10.2.7.
    #define D_ENV_MARIADB_HAS_JSON_TYPE                                        \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 7)

    // 9.1.2
    // D_ENV_MARIADB_JSON_IS_ALIAS
    //   status: 1 because MariaDB's JSON is always a LONGTEXT alias,
    // not a native binary type like MySQL's.
    #define D_ENV_MARIADB_JSON_IS_ALIAS 1

    // 9.1.3
    // D_ENV_MARIADB_HAS_JSON_TABLE
    //   feature: JSON_TABLE() function (MariaDB 10.6.0+).
    #define D_ENV_MARIADB_HAS_JSON_TABLE                                       \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 6, 0)

    // 9.1.4
    // D_ENV_MARIADB_HAS_JSON_ARRAYAGG
    //   feature: JSON_ARRAYAGG() / JSON_OBJECTAGG() (MariaDB 10.5.0+).
    #define D_ENV_MARIADB_HAS_JSON_ARRAYAGG                                    \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 5, 0)

    // 9.1.5
    // D_ENV_MARIADB_HAS_GEOMETRY_TYPES
    //   feature: alias from common header.
    #define D_ENV_MARIADB_HAS_GEOMETRY_TYPES                                   \
        D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES

    // 9.1.6
    // D_ENV_MARIADB_HAS_INET6_TYPE
    //   feature: INET6 data type for IPv6 addresses.
    // Introduced in MariaDB 10.5.0.
    #define D_ENV_MARIADB_HAS_INET6_TYPE                                       \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 5, 0)

    // 9.1.7
    // D_ENV_MARIADB_HAS_UUID_TYPE
    //   feature: UUID data type (native binary UUID storage).
    // Introduced in MariaDB 10.7.0.
    #define D_ENV_MARIADB_HAS_UUID_TYPE                                        \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 7, 0)

    // 9.1.8
    // D_ENV_MARIADB_HAS_DESCENDING_INDEX
    //   feature: descending indexes (MariaDB 10.8.0+).
    #define D_ENV_MARIADB_HAS_DESCENDING_INDEX                                 \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 8, 0)


//==============================================================================
// 10.  INNODB FEATURES (MARIADB-SPECIFIC)
//==============================================================================


// 10.1   InnoDB
//------------------------------------------------------------------------------
    // 10.1.1
    // D_ENV_MARIADB_HAS_INNODB_FULLTEXT
    //   feature: InnoDB full-text indexes (inherited from MySQL 5.6).
    #define D_ENV_MARIADB_HAS_INNODB_FULLTEXT                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 0)

    // 10.1.2
    // D_ENV_MARIADB_HAS_INNODB_ONLINE_DDL
    //   feature: InnoDB online DDL (MariaDB 10.0+).
    #define D_ENV_MARIADB_HAS_INNODB_ONLINE_DDL                                \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 0)

    // 10.1.3
    // D_ENV_MARIADB_HAS_INNODB_INSTANT_DDL
    //   feature: InnoDB instant ALTER (add/reorder columns without
    // table rebuild). MariaDB 10.3.2+.
    #define D_ENV_MARIADB_HAS_INNODB_INSTANT_DDL                               \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 3, 2)

    // 10.1.4
    // D_ENV_MARIADB_HAS_INNODB_TABLESPACE_ENCRYPTION
    //   feature: InnoDB tablespace encryption (TDE).
    // MariaDB 10.1+ with file_key_management or encryption plugins.
    #define D_ENV_MARIADB_HAS_INNODB_TABLESPACE_ENCRYPTION                     \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 0)

    // 10.1.5
    // D_ENV_MARIADB_HAS_INNODB_PAGE_COMPRESSION
    //   feature: InnoDB page compression (transparent, per-table).
    // Introduced in MariaDB 10.1.0.
    #define D_ENV_MARIADB_HAS_INNODB_PAGE_COMPRESSION                          \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 0)

    // 10.1.6
    // D_ENV_MARIADB_HAS_INNODB_SPATIAL_INDEX
    //   feature: InnoDB spatial indexes (MariaDB 10.2.2+).
    #define D_ENV_MARIADB_HAS_INNODB_SPATIAL_INDEX                             \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 2)

    // 10.1.7
    // D_ENV_MARIADB_HAS_INNODB_UNDO_TRUNCATION
    //   feature: automatic undo tablespace truncation (MariaDB 10.3.0+).
    #define D_ENV_MARIADB_HAS_INNODB_UNDO_TRUNCATION                           \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 3, 0)


//==============================================================================
// 11.  REPLICATION AND HIGH AVAILABILITY
//==============================================================================


// 11.1   Replication and high availability
//------------------------------------------------------------------------------
    // 11.1.1
    // D_ENV_MARIADB_HAS_GTID
    //   feature: MariaDB Global Transaction IDs. MariaDB uses a
    // domain-based GTID format (domain-server_id-sequence), distinct
    // from MySQL's UUID-based format. Introduced in MariaDB 10.0.2.
    #define D_ENV_MARIADB_HAS_GTID                                             \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 2)

    // 11.1.2
    // D_ENV_MARIADB_HAS_GALERA
    //   feature: Galera Cluster integration (synchronous multi-master).
    // Bundled since MariaDB 10.1.0 (previously separate MariaDB Galera
    // Cluster packages for 5.5 and 10.0).
    #define D_ENV_MARIADB_HAS_GALERA                                           \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 0)

    // 11.1.3
    // D_ENV_MARIADB_HAS_GALERA_4
    //   feature: Galera 4 library (streaming replication).
    // Integrated in MariaDB 10.4.0.
    #define D_ENV_MARIADB_HAS_GALERA_4                                         \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 0)

    // 11.1.4
    // D_ENV_MARIADB_HAS_SEMI_SYNC_REPL
    //   feature: semi-synchronous replication (inherited from MySQL 5.5).
    #define D_ENV_MARIADB_HAS_SEMI_SYNC_REPL                                   \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 11.1.5
    // D_ENV_MARIADB_HAS_MULTI_SOURCE_REPL
    //   feature: multi-source replication (multiple masters).
    // Introduced in MariaDB 10.0.1.
    #define D_ENV_MARIADB_HAS_MULTI_SOURCE_REPL                                \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 1)

    // 11.1.6
    // D_ENV_MARIADB_HAS_PARALLEL_REPL
    //   feature: parallel replication (optimistic/conservative modes).
    // Introduced in MariaDB 10.0.5, enhanced in 10.1+ and 10.5+.
    #define D_ENV_MARIADB_HAS_PARALLEL_REPL                                    \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 5)

    // 11.1.7
    // D_ENV_MARIADB_HAS_DELAYED_REPL
    //   feature: delayed replication (CHANGE MASTER ... MASTER_DELAY).
    // Introduced in MariaDB 10.2.3.
    #define D_ENV_MARIADB_HAS_DELAYED_REPL                                     \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 3)

    // 11.1.8
    // D_ENV_MARIADB_HAS_MARIADB_BACKUP
    //   feature: mariabackup (Percona XtraBackup fork for MariaDB).
    // Bundled since MariaDB 10.1.23.
    #define D_ENV_MARIADB_HAS_MARIADB_BACKUP                                   \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 23)

    // 11.1.9
    // D_ENV_MARIADB_HAS_FLASHBACK
    //   feature: binary log flashback (DML rollback via binlog).
    // Introduced in MariaDB 10.2.4.
    #define D_ENV_MARIADB_HAS_FLASHBACK                                        \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 2, 4)


//==============================================================================
// 12.  CHARACTER SET
//==============================================================================


// 12.1   Character sets
//------------------------------------------------------------------------------
    // 12.1.1
    // D_ENV_MARIADB_HAS_UTF8MB4
    //   feature: utf8mb4 character set (present since fork).
    #define D_ENV_MARIADB_HAS_UTF8MB4                                          \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 12.1.2
    // D_ENV_MARIADB_DEFAULT_CHARSET_IS_UTF8MB4
    //   status: not yet the default in MariaDB. MariaDB still defaults
    // to latin1 as of 11.4 (server-level), though many distributions
    // override this.
    #define D_ENV_MARIADB_DEFAULT_CHARSET_IS_UTF8MB4 0

    // 12.1.3
    // D_ENV_MARIADB_HAS_UTF8MB4_UNICODE_520
    //   feature: utf8mb4_unicode_520_ci collation (Unicode 5.2 based).
    // Available since MariaDB 10.0.
    #define D_ENV_MARIADB_HAS_UTF8MB4_UNICODE_520                              \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 0)

    // 12.1.4
    // D_ENV_MARIADB_HAS_UCA_14
    //   feature: UCA 14.0 collations (uca1400_* family).
    // Introduced in MariaDB 10.10.0.
    #define D_ENV_MARIADB_HAS_UCA_14                                           \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 10, 0)

    // 12.1.5
    // D_ENV_MARIADB_UTF8_IS_UTF8MB3
    //   status: alias from common header.
    #define D_ENV_MARIADB_UTF8_IS_UTF8MB3                                      \
        D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3


//==============================================================================
// 13.  OPTIMIZER AND PERFORMANCE
//==============================================================================


// 13.1   Optimizer and performance
//------------------------------------------------------------------------------
    // 13.1.1
    // D_ENV_MARIADB_HAS_OPTIMIZER_HINTS
    //   feature: optimizer hints (MariaDB has its own hint syntax
    // partially compatible with MySQL's). Available since 10.1+ (partial).
    #define D_ENV_MARIADB_HAS_OPTIMIZER_HINTS                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 0)

    // 13.1.2
    // D_ENV_MARIADB_HAS_HISTOGRAM_STATS
    //   feature: engine-independent table statistics (histograms).
    // MariaDB's approach predates MySQL's; available since 10.0.1.
    #define D_ENV_MARIADB_HAS_HISTOGRAM_STATS                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 1)

    // 13.1.3
    // D_ENV_MARIADB_HAS_CONDITION_PUSHDOWN
    //   feature: engine condition pushdown.
    // Enhanced in MariaDB 10.4 for derived tables and views.
    #define D_ENV_MARIADB_HAS_CONDITION_PUSHDOWN                               \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 0)

    // 13.1.4
    // D_ENV_MARIADB_HAS_EXPLAIN_ANALYZE
    //   feature: EXPLAIN ANALYZE (actual execution statistics).
    // Introduced in MariaDB 10.1.0.
    #define D_ENV_MARIADB_HAS_EXPLAIN_ANALYZE                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 0)

    // 13.1.5
    // D_ENV_MARIADB_HAS_EXPLAIN_FORMAT_JSON
    //   feature: EXPLAIN FORMAT=JSON (MariaDB 10.1+).
    #define D_ENV_MARIADB_HAS_EXPLAIN_FORMAT_JSON                              \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 0)

    // 13.1.6
    // D_ENV_MARIADB_HAS_PERFORMANCE_SCHEMA
    //   feature: Performance Schema (inherited from MySQL 5.5).
    #define D_ENV_MARIADB_HAS_PERFORMANCE_SCHEMA                               \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 13.1.7
    // D_ENV_MARIADB_HAS_SYS_SCHEMA
    //   feature: sys schema (bundled since MariaDB 10.6.0).
    #define D_ENV_MARIADB_HAS_SYS_SCHEMA                                       \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 6, 0)

    // 13.1.8
    // D_ENV_MARIADB_HAS_QUERY_RESPONSE_TIME
    //   feature: QUERY_RESPONSE_TIME plugin.
    // Bundled since MariaDB 10.0.4.
    #define D_ENV_MARIADB_HAS_QUERY_RESPONSE_TIME                              \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 4)

    // 13.1.9
    // D_ENV_MARIADB_HAS_THREADPOOL
    //   feature: built-in thread pool (included in Community Edition).
    // Available since MariaDB 5.5 (unlike MySQL where it's Enterprise).
    #define D_ENV_MARIADB_HAS_THREADPOOL                                       \
        D_ENV_MARIADB_HAS_CLIENT_LIB


//==============================================================================
// 14.  SECURITY AND ADMINISTRATION
//==============================================================================


// 14.1   Security and administration
//------------------------------------------------------------------------------
    // 14.1.1
    // D_ENV_MARIADB_HAS_ROLES
    //   feature: SQL roles. Introduced in MariaDB 10.0.5.
    #define D_ENV_MARIADB_HAS_ROLES                                            \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 0, 5)

    // 14.1.2
    // D_ENV_MARIADB_HAS_ACCOUNT_LOCKING
    //   feature: account locking (ALTER USER ... ACCOUNT LOCK).
    // Introduced in MariaDB 10.4.2.
    #define D_ENV_MARIADB_HAS_ACCOUNT_LOCKING                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 2)

    // 14.1.3
    // D_ENV_MARIADB_HAS_PASSWORD_EXPIRY
    //   feature: password expiry (default_password_lifetime).
    // Introduced in MariaDB 10.4.3.
    #define D_ENV_MARIADB_HAS_PASSWORD_EXPIRY                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 4, 3)

    // 14.1.4
    // D_ENV_MARIADB_HAS_DATA_AT_REST_ENCRYPTION
    //   feature: encryption at rest (tablespace, redo/undo logs,
    // binary logs, temporary files). Introduced in MariaDB 10.1.3.
    #define D_ENV_MARIADB_HAS_DATA_AT_REST_ENCRYPTION                          \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 3)

    // 14.1.5
    // D_ENV_MARIADB_HAS_KEY_MANAGEMENT_PLUGINS
    //   feature: key management plugin API (file_key_management,
    // aws_key_management, etc.). Introduced in MariaDB 10.1.3.
    #define D_ENV_MARIADB_HAS_KEY_MANAGEMENT_PLUGINS                           \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 1, 3)

    // 14.1.6
    // D_ENV_MARIADB_HAS_AUDIT_PLUGIN
    //   feature: server_audit plugin (Community Edition included).
    // Bundled since MariaDB 5.5.
    #define D_ENV_MARIADB_HAS_AUDIT_PLUGIN                                     \
        D_ENV_MARIADB_HAS_CLIENT_LIB

    // 14.1.7
    // D_ENV_MARIADB_HAS_ATOMIC_DDL
    //   feature: atomic DDL (crash-safe CREATE/DROP/RENAME/TRUNCATE).
    // Introduced in MariaDB 10.6.1.
    #define D_ENV_MARIADB_HAS_ATOMIC_DDL                                       \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 6, 1)


//==============================================================================
// 15.  PLATFORM
//==============================================================================


// 15.1   Platform
//------------------------------------------------------------------------------
    #define D_ENV_MARIADB_HAS_UNIX_SOCKET_CONN                                 \
        D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
    #define D_ENV_MARIADB_HAS_NAMED_PIPE                                       \
        D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
    #define D_ENV_MARIADB_HAS_SHARED_MEMORY                                    \
        D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY


//==============================================================================
// 16.  COMPOSITE CHECKS
//==============================================================================


// 16.1   Composite checks
//------------------------------------------------------------------------------
    // 16.1.1
    // D_ENV_MARIADB_HAS_MODERN_SQL
    //   macro: evaluates to 1 if window functions, CTEs, sequences, and
    // system-versioned tables are all available.
    #define D_ENV_MARIADB_HAS_MODERN_SQL                                       \
        ( (D_ENV_MARIADB_HAS_WINDOW_FUNCTIONS) &&                              \
          (D_ENV_MARIADB_HAS_CTE)              &&                              \
          (D_ENV_MARIADB_HAS_SEQUENCES)        &&                              \
          (D_ENV_MARIADB_HAS_SYSTEM_VERSIONED_TABLES) )

    // 16.1.2
    // D_ENV_MARIADB_HAS_MODERN_AUTH
    //   macro: evaluates to 1 if ed25519, PAM v2, and unix_socket are
    // all available.
    #define D_ENV_MARIADB_HAS_MODERN_AUTH                                      \
        ( (D_ENV_MARIADB_HAS_AUTH_ED25519) &&                                  \
          (D_ENV_MARIADB_HAS_AUTH_PAM_V2)  &&                                  \
          (D_ENV_MARIADB_HAS_PLUGGABLE_AUTH) )

    // 16.1.3
    // D_ENV_MARIADB_HAS_MODERN_DDL
    //   macro: evaluates to 1 if instant ALTER and atomic DDL are
    // both available.
    #define D_ENV_MARIADB_HAS_MODERN_DDL                                       \
        ( (D_ENV_MARIADB_HAS_INNODB_INSTANT_DDL) &&                            \
          (D_ENV_MARIADB_HAS_ATOMIC_DDL) )

    // 16.1.4
    // D_ENV_MARIADB_HAS_HA_SUITE
    //   macro: evaluates to 1 if Galera 4 + mariabackup are available.
    #define D_ENV_MARIADB_HAS_HA_SUITE                                         \
        ( (D_ENV_MARIADB_HAS_GALERA_4)       &&                                \
          (D_ENV_MARIADB_HAS_MARIADB_BACKUP) &&                                \
          (D_ENV_MARIADB_HAS_GTID) )

    // 16.1.5
    // D_ENV_MARIADB_HAS_MODERN_SECURITY
    //   macro: evaluates to 1 if roles, encryption at rest, and account
    // locking are all available.
    #define D_ENV_MARIADB_HAS_MODERN_SECURITY                                  \
        ( (D_ENV_MARIADB_HAS_ROLES)                   &&                       \
          (D_ENV_MARIADB_HAS_DATA_AT_REST_ENCRYPTION) &&                       \
          (D_ENV_MARIADB_HAS_ACCOUNT_LOCKING)         &&                       \
          (D_ENV_MARIADB_HAS_PASSWORD_EXPIRY) )

    // 16.1.6
    // D_ENV_MARIADB_IS_FULLY_MODERN
    //   macro: evaluates to 1 if MariaDB has a comprehensive modern
    // feature set (roughly MariaDB 10.6+).
    #define D_ENV_MARIADB_IS_FULLY_MODERN                                      \
        ( (D_ENV_MARIADB_HAS_MODERN_SQL)  &&                                   \
          (D_ENV_MARIADB_HAS_MODERN_AUTH) &&                                   \
          (D_ENV_MARIADB_HAS_MODERN_DDL)  &&                                   \
          (D_ENV_MARIADB_HAS_HA_SUITE)    &&                                   \
          (D_ENV_MARIADB_HAS_MODERN_SECURITY) )


//==============================================================================
// 17.  DEPRECATION AND REMOVAL
//==============================================================================


// 17.1   Deprecations
//------------------------------------------------------------------------------
    // 17.1.1
    // D_ENV_MARIADB_REMOVED_EMBEDDED_SERVER
    //   status: 1 if embedded server was removed (MariaDB 11.0+).
    #define D_ENV_MARIADB_REMOVED_EMBEDDED_SERVER                              \
        D_ENV_MARIADB_VERSION_AT_LEAST(11, 0, 0)

    // 17.1.2
    // D_ENV_MARIADB_REMOVED_TOKUDB
    //   status: 1 if TokuDB engine was removed (MariaDB 10.6+).
    #define D_ENV_MARIADB_REMOVED_TOKUDB                                       \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 6, 0)

    // 17.1.3
    // D_ENV_MARIADB_REMOVED_CASSANDRA_ENGINE
    //   status: 1 if Cassandra SE was removed (MariaDB 10.6+).
    #define D_ENV_MARIADB_REMOVED_CASSANDRA_ENGINE                             \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 6, 0)

    // 17.1.4
    // D_ENV_MARIADB_DEPRECATED_QUERY_CACHE
    //   status: 1 if query cache is deprecated.
    // Deprecated since MariaDB 10.6, removed in 11.0.
    #define D_ENV_MARIADB_DEPRECATED_QUERY_CACHE                               \
        D_ENV_MARIADB_VERSION_AT_LEAST(10, 6, 0)

    // 17.1.5
    // D_ENV_MARIADB_REMOVED_QUERY_CACHE
    //   status: 1 if query cache is removed (MariaDB 11.0+).
    #define D_ENV_MARIADB_REMOVED_QUERY_CACHE                                  \
        D_ENV_MARIADB_VERSION_AT_LEAST(11, 0, 0)


#endif  // D_ENV_MARIADB_DETECTED


#endif  // DJINTERP_ENV_DB_MARIADB_ENV_MARIADB_H
