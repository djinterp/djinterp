/******************************************************************************
* djinterp [db]                                                    env_mysql.h
*
* djinterp Oracle MySQL environmental detection header:
* This header provides comprehensive compile-time detection of Oracle MySQL
* environments, capabilities, and version-gated features, including:
*   - version decomposition (major, minor, patch) and comparison macros
*   - client library variant detection (libmysqlclient, Connector/C)
*   - storage engine features (InnoDB internals, NDB, system tables)
*   - authentication plugin detection (sha256, caching_sha2, FIDO, etc.)
*   - character set and collation framework (utf8mb4_0900, defaults)
*   - SSL/TLS protocol-level features (SSL_MODE, TLS ciphersuites, FIPS)
*   - replication, Group Replication, and InnoDB Cluster/ClusterSet
*   - asynchronous C API, session tracking, query attributes
*   - data type support gated by version (JSON, check constraints, etc.)
*   - performance and optimizer features (CTEs, window functions, hash join)
*   - X Protocol and X DevAPI detection
*   - security features (roles, partial revokes, dual passwords)
*   - deprecation and removal tracking
*
*   This header is for Oracle MySQL ONLY. For MariaDB, use env_mariadb.h.
* Both headers share common infrastructure from env_mysql_common.h.
*
*   CONFIGURATION SYSTEM:
*   This header supports custom MySQL environment simulation via
* D_CFG_ENV_MYSQL_CUSTOM:
*   - 0 (default): full automatic detection via MySQL-provided macros
*   - 1: skip all detection (requires pre-defined D_ENV_MYSQL_DETECTED_*
*     variables)
*   Pre-defining D_ENV_MYSQL_DETECTED_* variables automatically enables
* custom mode.
*
*   NAMING CONVENTION:
*   D_ENV_MYSQL_[CATEGORY]_[FEATURE]  - 1 if available, 0 otherwise
*   D_ENV_MYSQL_VERSION_[COMPONENT]   - version number components
*   D_ENV_MYSQL_HAS_[CAPABILITY]      - capability flag (1/0)
*
* 
* path:      /inc/djinterp/core/env/db/mysql/env_mysql.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_MYSQL_
#define DJINTERP_ENVIRONMENT_MYSQL_ 1

#include "./env_mysql_common.h"


// =============================================================================
// I.   CONFIGURATION SYSTEM
// =============================================================================
//   All D_CFG_ENV_MYSQL_* macros (USING flag, header paths, CUSTOM flag)
// live in env_mysql_config.h, which is pulled in by env_mysql_common.h
// before this file's detection code runs.


// =============================================================================
// II.  VENDOR GUARD
// =============================================================================

// D_ENV_MYSQL_IS_MARIADB
//   detection: alias from common header for backward compatibility.
#define D_ENV_MYSQL_IS_MARIADB D_ENV_MYSQL_COMMON_IS_MARIADB


// =============================================================================
// III. VERSION DETECTION
// =============================================================================

// version ID constants for well-known releases
#define D_ENV_MYSQL_VERSION_5_0_0      50000
#define D_ENV_MYSQL_VERSION_5_1_0      50100
#define D_ENV_MYSQL_VERSION_5_5_0      50500
#define D_ENV_MYSQL_VERSION_5_6_0      50600
#define D_ENV_MYSQL_VERSION_5_7_0      50700
#define D_ENV_MYSQL_VERSION_5_7_8      50708
#define D_ENV_MYSQL_VERSION_5_7_12     50712
#define D_ENV_MYSQL_VERSION_5_7_17     50717
#define D_ENV_MYSQL_VERSION_5_7_22     50722
#define D_ENV_MYSQL_VERSION_8_0_0      80000
#define D_ENV_MYSQL_VERSION_8_0_3      80003
#define D_ENV_MYSQL_VERSION_8_0_11     80011
#define D_ENV_MYSQL_VERSION_8_0_13     80013
#define D_ENV_MYSQL_VERSION_8_0_14     80014
#define D_ENV_MYSQL_VERSION_8_0_16     80016
#define D_ENV_MYSQL_VERSION_8_0_17     80017
#define D_ENV_MYSQL_VERSION_8_0_19     80019
#define D_ENV_MYSQL_VERSION_8_0_22     80022
#define D_ENV_MYSQL_VERSION_8_0_23     80023
#define D_ENV_MYSQL_VERSION_8_0_25     80025
#define D_ENV_MYSQL_VERSION_8_0_27     80027
#define D_ENV_MYSQL_VERSION_8_0_28     80028
#define D_ENV_MYSQL_VERSION_8_0_29     80029
#define D_ENV_MYSQL_VERSION_8_0_30     80030
#define D_ENV_MYSQL_VERSION_8_0_32     80032
#define D_ENV_MYSQL_VERSION_8_0_34     80034
#define D_ENV_MYSQL_VERSION_8_1_0      80100
#define D_ENV_MYSQL_VERSION_8_2_0      80200
#define D_ENV_MYSQL_VERSION_8_3_0      80300
#define D_ENV_MYSQL_VERSION_8_4_0      80400
#define D_ENV_MYSQL_VERSION_9_0_0      90000
#define D_ENV_MYSQL_VERSION_9_1_0      90100

#if (D_CFG_ENV_MYSQL_CUSTOM == 0)

    // automatic detection requires the MySQL header to be in scope; if
    // D_CFG_ENV_USING_MYSQL was not enabled the sentinel is 0 and we skip
    // cleanly (no reference to MYSQL_VERSION_ID etc.)
    #if ( D_ENV_MYSQL_HEADER_INCLUDED           &&  \
          D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL )
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         MYSQL_VERSION_ID
        #define D_ENV_MYSQL_VERSION_MAJOR      \
            D_ENV_MYSQL_COMMON_DECODE_MAJOR(MYSQL_VERSION_ID)
        #define D_ENV_MYSQL_VERSION_MINOR      \
            D_ENV_MYSQL_COMMON_DECODE_MINOR(MYSQL_VERSION_ID)
        #define D_ENV_MYSQL_VERSION_PATCH      \
            D_ENV_MYSQL_COMMON_DECODE_PATCH(MYSQL_VERSION_ID)

        #ifdef MYSQL_SERVER_VERSION
            #define D_ENV_MYSQL_VERSION_STRING  MYSQL_SERVER_VERSION
        #else
            #define D_ENV_MYSQL_VERSION_STRING  "unknown"
        #endif
    #else
        #define D_ENV_MYSQL_DETECTED           0
    #endif

#else
    // manual mode: use pre-defined detection variables
    #ifdef D_ENV_MYSQL_DETECTED_VERSION
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_DETECTED_VERSION
        #define D_ENV_MYSQL_VERSION_MAJOR      \
            D_ENV_MYSQL_COMMON_DECODE_MAJOR(D_ENV_MYSQL_DETECTED_VERSION)
        #define D_ENV_MYSQL_VERSION_MINOR      \
            D_ENV_MYSQL_COMMON_DECODE_MINOR(D_ENV_MYSQL_DETECTED_VERSION)
        #define D_ENV_MYSQL_VERSION_PATCH      \
            D_ENV_MYSQL_COMMON_DECODE_PATCH(D_ENV_MYSQL_DETECTED_VERSION)
        #define D_ENV_MYSQL_VERSION_STRING     "manual"

    #elif defined(D_ENV_MYSQL_DETECTED_9_1)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_1_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      1
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.1.0"

    #elif defined(D_ENV_MYSQL_DETECTED_9_0)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_9_0_0
        #define D_ENV_MYSQL_VERSION_MAJOR      9
        #define D_ENV_MYSQL_VERSION_MINOR      0
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "9.0.0"

    #elif defined(D_ENV_MYSQL_DETECTED_8_4)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_8_4_0
        #define D_ENV_MYSQL_VERSION_MAJOR      8
        #define D_ENV_MYSQL_VERSION_MINOR      4
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "8.4.0"

    #elif defined(D_ENV_MYSQL_DETECTED_8_0)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_8_0_0
        #define D_ENV_MYSQL_VERSION_MAJOR      8
        #define D_ENV_MYSQL_VERSION_MINOR      0
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "8.0.0"

    #elif defined(D_ENV_MYSQL_DETECTED_5_7)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_5_7_0
        #define D_ENV_MYSQL_VERSION_MAJOR      5
        #define D_ENV_MYSQL_VERSION_MINOR      7
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "5.7.0"

    #elif defined(D_ENV_MYSQL_DETECTED_5_6)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_5_6_0
        #define D_ENV_MYSQL_VERSION_MAJOR      5
        #define D_ENV_MYSQL_VERSION_MINOR      6
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "5.6.0"

    #elif defined(D_ENV_MYSQL_DETECTED_5_5)
        #define D_ENV_MYSQL_DETECTED           1
        #define D_ENV_MYSQL_VERSION_ID         D_ENV_MYSQL_VERSION_5_5_0
        #define D_ENV_MYSQL_VERSION_MAJOR      5
        #define D_ENV_MYSQL_VERSION_MINOR      5
        #define D_ENV_MYSQL_VERSION_PATCH      0
        #define D_ENV_MYSQL_VERSION_STRING     "5.5.0"

    #else
        #define D_ENV_MYSQL_DETECTED           0
    #endif

#endif  // D_CFG_ENV_MYSQL_CUSTOM


// =============================================================================
// IV.  VERSION COMPARISON MACROS
// =============================================================================
//   NOTE: vendor header inclusion is performed by env_mysql_common.h under
// control of D_CFG_ENV_USING_MYSQL. Do NOT re-include <mysql/mysql.h> here
// (and absolutely not <mariadb/conncpp.hpp>, which belongs to MariaDB, not
// Oracle MySQL). The old unconditional includes at this location were the
// source of cross-vendor symbol collisions and spurious linker errors.

#if D_ENV_MYSQL_DETECTED

    #define D_ENV_MYSQL_VERSION_AT_LEAST(major, minor, patch) \
        (D_ENV_MYSQL_VERSION_ID >= \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_MYSQL_VERSION_BELOW(major, minor, patch) \
        (D_ENV_MYSQL_VERSION_ID < \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_MYSQL_VERSION_EXACT(major, minor, patch) \
        (D_ENV_MYSQL_VERSION_ID == \
            D_ENV_MYSQL_COMMON_ENCODE_VERSION(major, minor, patch))

    #define D_ENV_MYSQL_VERSION_IN_RANGE(min_maj, min_min, min_pat,  \
                                         max_maj, max_min, max_pat)  \
        ( D_ENV_MYSQL_VERSION_AT_LEAST(min_maj, min_min, min_pat) && \
          D_ENV_MYSQL_VERSION_BELOW(max_maj, max_min, max_pat) )

    // series macros
    #define D_ENV_MYSQL_IS_5_5 \
        D_ENV_MYSQL_VERSION_IN_RANGE(5, 5, 0, 5, 6, 0)
    #define D_ENV_MYSQL_IS_5_6 \
        D_ENV_MYSQL_VERSION_IN_RANGE(5, 6, 0, 5, 7, 0)
    #define D_ENV_MYSQL_IS_5_7 \
        D_ENV_MYSQL_VERSION_IN_RANGE(5, 7, 0, 8, 0, 0)
    #define D_ENV_MYSQL_IS_8_0 \
        D_ENV_MYSQL_VERSION_IN_RANGE(8, 0, 0, 8, 1, 0)
    #define D_ENV_MYSQL_IS_8_4 \
        D_ENV_MYSQL_VERSION_IN_RANGE(8, 4, 0, 8, 5, 0)
    #define D_ENV_MYSQL_IS_9_PLUS \
        D_ENV_MYSQL_VERSION_AT_LEAST(9, 0, 0)

    // release model
    #define D_ENV_MYSQL_IS_LTS       ( D_ENV_MYSQL_IS_8_4 )
    #define D_ENV_MYSQL_IS_INNOVATION \
        ( D_ENV_MYSQL_VERSION_IN_RANGE(8, 1, 0, 8, 4, 0) || \
          D_ENV_MYSQL_IS_9_PLUS )


// =============================================================================
// V.   CLIENT LIBRARY (MYSQL-SPECIFIC)
// =============================================================================

    #define D_ENV_MYSQL_HAS_CLIENT_LIB \
        D_ENV_MYSQL_COMMON_HAS_CLIENT_LIB

    #ifndef D_ENV_MYSQL_HAS_CONNECTOR_C
        #if ( defined(MYSQL_CONNECTOR_VERSION)      ||  \
              defined(D_ENV_MYSQL_DETECTED_CONNECTOR_C) )
            #define D_ENV_MYSQL_HAS_CONNECTOR_C 1
        #else
            #define D_ENV_MYSQL_HAS_CONNECTOR_C 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_LIBMYSQLCLIENT
        #if ( defined(LIBMYSQL_VERSION)             ||  \
              defined(LIBMYSQL_VERSION_ID)          ||  \
              D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL    ||  \
              defined(D_ENV_MYSQL_DETECTED_LIBMYSQLCLIENT) )
            #define D_ENV_MYSQL_HAS_LIBMYSQLCLIENT 1
        #else
            #define D_ENV_MYSQL_HAS_LIBMYSQLCLIENT 0
        #endif
    #endif

    #define D_ENV_MYSQL_HAS_EMBEDDED \
        D_ENV_MYSQL_COMMON_HAS_EMBEDDED


// =============================================================================
// VI.  C API (MYSQL VERSION-GATED)
// =============================================================================

    #ifndef D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
            #define D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING 1
        #else
            #define D_ENV_MYSQL_HAS_MYSQL_REAL_CONNECT_NONBLOCKING 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_ASYNC_API
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
            #define D_ENV_MYSQL_HAS_ASYNC_API 1
        #else
            #define D_ENV_MYSQL_HAS_ASYNC_API 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_RESET_CONNECTION
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 3)
            #define D_ENV_MYSQL_HAS_RESET_CONNECTION 1
        #else
            #define D_ENV_MYSQL_HAS_RESET_CONNECTION 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_SESSION_TRACK
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 4)
            #define D_ENV_MYSQL_HAS_SESSION_TRACK 1
        #else
            #define D_ENV_MYSQL_HAS_SESSION_TRACK 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_GET_OPTION
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 3)
            #define D_ENV_MYSQL_HAS_GET_OPTION 1
        #else
            #define D_ENV_MYSQL_HAS_GET_OPTION 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_STMT_NEXT_RESULT
        #if D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 3)
            #define D_ENV_MYSQL_HAS_STMT_NEXT_RESULT 1
        #else
            #define D_ENV_MYSQL_HAS_STMT_NEXT_RESULT 0
        #endif
    #endif

    // protocol
    #ifndef D_ENV_MYSQL_HAS_ZSTD_COMPRESSION
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
            #define D_ENV_MYSQL_HAS_ZSTD_COMPRESSION 1
        #else
            #define D_ENV_MYSQL_HAS_ZSTD_COMPRESSION 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
            #define D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA 1
        #else
            #define D_ENV_MYSQL_HAS_OPTIONAL_RESULT_METADATA 0
        #endif
    #endif

    #ifndef D_ENV_MYSQL_HAS_QUERY_ATTRS
        #if D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 25)
            #define D_ENV_MYSQL_HAS_QUERY_ATTRS 1
        #else
            #define D_ENV_MYSQL_HAS_QUERY_ATTRS 0
        #endif
    #endif


// =============================================================================
// VII. SSL/TLS PROTOCOL FEATURES
// =============================================================================

    #ifndef D_ENV_MYSQL_HAS_SSL
        #if D_ENV_MYSQL_COMMON_HAS_ANY_SSL
            #define D_ENV_MYSQL_HAS_SSL 1
        #elif D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 0)
            #define D_ENV_MYSQL_HAS_SSL 1
        #else
            #define D_ENV_MYSQL_HAS_SSL 0
        #endif
    #endif

    #define D_ENV_MYSQL_HAS_SSL_MODE \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 11)
    #define D_ENV_MYSQL_HAS_TLS_VERSION_OPTION \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 10)
    #define D_ENV_MYSQL_HAS_TLS_CIPHERSUITES \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)

    #ifndef D_ENV_MYSQL_HAS_SSL_FIPS_MODE
        #if D_ENV_MYSQL_VERSION_IN_RANGE(8, 0, 11, 8, 0, 34)
            #define D_ENV_MYSQL_HAS_SSL_FIPS_MODE 1
        #else
            #define D_ENV_MYSQL_HAS_SSL_FIPS_MODE 0
        #endif
    #endif

    #define D_ENV_MYSQL_HAS_GET_SSL_SESSION_REUSED \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 29)


// =============================================================================
// VIII. AUTHENTICATION
// =============================================================================

    #define D_ENV_MYSQL_HAS_AUTH_NATIVE \
        D_ENV_MYSQL_COMMON_HAS_AUTH_NATIVE
    #define D_ENV_MYSQL_AUTH_NATIVE_DEPRECATED \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 34)
    #define D_ENV_MYSQL_HAS_PLUGGABLE_AUTH \
        D_ENV_MYSQL_COMMON_HAS_PLUGGABLE_AUTH

    #define D_ENV_MYSQL_HAS_AUTH_SHA256 \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 6)
    #define D_ENV_MYSQL_HAS_AUTH_CACHING_SHA2 \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_DEFAULT_AUTH_IS_CACHING_SHA2 \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 4)
    #define D_ENV_MYSQL_HAS_AUTH_LDAP_SIMPLE \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 19)
    #define D_ENV_MYSQL_HAS_AUTH_LDAP_SASL \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 19)
    #define D_ENV_MYSQL_HAS_AUTH_KERBEROS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 26)
    #define D_ENV_MYSQL_HAS_AUTH_OCI \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)
    #define D_ENV_MYSQL_HAS_AUTH_FIDO \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)
    #define D_ENV_MYSQL_HAS_MULTI_FACTOR_AUTH \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)


// =============================================================================
// IX.  DATA TYPES
// =============================================================================

    #define D_ENV_MYSQL_HAS_JSON_TYPE \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 8)
    #define D_ENV_MYSQL_HAS_JSON_TABLE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 4)
    #define D_ENV_MYSQL_HAS_JSON_SCHEMA_VALIDATION \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 17)
    #define D_ENV_MYSQL_HAS_JSON_VALUE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 21)
    #define D_ENV_MYSQL_HAS_JSON_ARRAYAGG \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 22)
    #define D_ENV_MYSQL_HAS_MULTI_VALUE_INDEX \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 17)
    #define D_ENV_MYSQL_HAS_GEOMETRY_TYPES \
        D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES
    #define D_ENV_MYSQL_HAS_SRID_SUPPORT \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_INVISIBLE_COLUMNS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 23)
    #define D_ENV_MYSQL_HAS_FUNCTIONAL_INDEX \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 13)
    #define D_ENV_MYSQL_HAS_DESCENDING_INDEX \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_HAS_GENERATED_COLUMNS \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 6)
    #define D_ENV_MYSQL_HAS_DEFAULT_EXPRESSION \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 13)
    #define D_ENV_MYSQL_HAS_CHECK_CONSTRAINTS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)


// =============================================================================
// X.   STORAGE ENGINES
// =============================================================================

    #define D_ENV_MYSQL_HAS_INNODB         D_ENV_MYSQL_COMMON_HAS_INNODB
    #define D_ENV_MYSQL_HAS_MYISAM         D_ENV_MYSQL_COMMON_HAS_MYISAM
    #define D_ENV_MYSQL_HAS_MEMORY_ENGINE  D_ENV_MYSQL_COMMON_HAS_MEMORY_ENGINE
    #define D_ENV_MYSQL_HAS_NDB_CLUSTER    D_ENV_MYSQL_COMMON_HAS_NDB_CLUSTER
    #define D_ENV_MYSQL_HAS_ARCHIVE_ENGINE D_ENV_MYSQL_COMMON_HAS_ARCHIVE_ENGINE
    #define D_ENV_MYSQL_HAS_CSV_ENGINE     D_ENV_MYSQL_COMMON_HAS_CSV_ENGINE
    #define D_ENV_MYSQL_HAS_BLACKHOLE_ENGINE \
        D_ENV_MYSQL_COMMON_HAS_BLACKHOLE_ENGINE
    #define D_ENV_MYSQL_HAS_FEDERATED_ENGINE \
        D_ENV_MYSQL_COMMON_HAS_FEDERATED_ENGINE
    #define D_ENV_MYSQL_SYSTEM_TABLES_USE_INNODB \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)


// =============================================================================
// XI.  INNODB FEATURES
// =============================================================================

    #define D_ENV_MYSQL_HAS_INNODB_FULLTEXT \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 4)
    #define D_ENV_MYSQL_HAS_INNODB_SPATIAL_INDEX \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 5)
    #define D_ENV_MYSQL_HAS_INNODB_ONLINE_DDL \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 7)
    #define D_ENV_MYSQL_HAS_INNODB_INSTANT_DDL \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 12)
    #define D_ENV_MYSQL_HAS_INNODB_TABLESPACE_ENCRYPTION \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 11)
    #define D_ENV_MYSQL_HAS_INNODB_REDO_LOG \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 30)
    #define D_ENV_MYSQL_HAS_INNODB_UNDO_TRUNCATION \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 5)
    #define D_ENV_MYSQL_HAS_INNODB_DEDICATED_SERVER \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)


// =============================================================================
// XII. REPLICATION AND HIGH AVAILABILITY
// =============================================================================

    #define D_ENV_MYSQL_HAS_GTID \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 5)
    #define D_ENV_MYSQL_HAS_SEMI_SYNC_REPL \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 1)
    #define D_ENV_MYSQL_HAS_MULTI_SOURCE_REPL \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 6)
    #define D_ENV_MYSQL_HAS_GROUP_REPLICATION \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 17)
    #define D_ENV_MYSQL_HAS_INNODB_CLUSTER \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 17)
    #define D_ENV_MYSQL_HAS_INNODB_CLUSTERSET \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 27)
    #define D_ENV_MYSQL_HAS_INNODB_REPLICASET \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 19)
    #define D_ENV_MYSQL_HAS_CLONE_PLUGIN \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 17)
    #define D_ENV_MYSQL_HAS_REPL_CHANNELS \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 6)
    #define D_ENV_MYSQL_HAS_REPL_PRIVILEGE_CHECKS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)


// =============================================================================
// XIII. X PROTOCOL AND X DEVAPI
// =============================================================================

    #define D_ENV_MYSQL_HAS_X_PROTOCOL \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 12)
    #define D_ENV_MYSQL_HAS_X_DEVAPI \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 12)

    #ifndef D_ENV_MYSQL_HAS_MYSQLX_PLUGIN
        #if ( defined(MYSQLX_VERSION)       ||  \
              defined(MYSQLXCLIENT_VERSION) )
            #define D_ENV_MYSQL_HAS_MYSQLX_PLUGIN 1
        #elif D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 11)
            #define D_ENV_MYSQL_HAS_MYSQLX_PLUGIN 1
        #else
            #define D_ENV_MYSQL_HAS_MYSQLX_PLUGIN 0
        #endif
    #endif


// =============================================================================
// XIV. CHARACTER SET
// =============================================================================

    #define D_ENV_MYSQL_HAS_UTF8MB4 \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 3)
    #define D_ENV_MYSQL_DEFAULT_CHARSET_IS_UTF8MB4 \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_HAS_UTF8MB4_0900 \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_UTF8_IS_UTF8MB3 \
        D_ENV_MYSQL_COMMON_UTF8_IS_UTF8MB3


// =============================================================================
// XV.  OPTIMIZER AND PERFORMANCE
// =============================================================================

    #define D_ENV_MYSQL_HAS_DATA_DICTIONARY \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_HAS_RESOURCE_GROUPS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_WINDOW_FUNCTIONS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 2)
    #define D_ENV_MYSQL_HAS_CTE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 1)
    #define D_ENV_MYSQL_HAS_LATERAL_DERIVED \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 14)
    #define D_ENV_MYSQL_HAS_HASH_JOIN \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
    #define D_ENV_MYSQL_HAS_HISTOGRAMS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_EXPLAIN_ANALYZE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
    #define D_ENV_MYSQL_HAS_EXPLAIN_FORMAT_TREE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
    #define D_ENV_MYSQL_HAS_OPTIMIZER_HINTS \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 7)
    #define D_ENV_MYSQL_HAS_PERFORMANCE_SCHEMA \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 0)
    #define D_ENV_MYSQL_HAS_SYS_SCHEMA \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 7, 7)


// =============================================================================
// XVI. SECURITY AND ADMINISTRATION
// =============================================================================

    #define D_ENV_MYSQL_HAS_ROLES \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_HAS_PARTIAL_REVOKE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 16)
    #define D_ENV_MYSQL_HAS_AUDIT_LOG \
        D_ENV_MYSQL_VERSION_AT_LEAST(5, 6, 20)
    #define D_ENV_MYSQL_HAS_DATA_MASKING \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 13)
    #define D_ENV_MYSQL_HAS_PASSWORD_HISTORY \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 3)
    #define D_ENV_MYSQL_HAS_DUAL_PASSWORDS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 14)
    #define D_ENV_MYSQL_HAS_RANDOM_PASSWORD \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 18)
    #define D_ENV_MYSQL_HAS_ATOMIC_DDL \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)


// =============================================================================
// XVII. PLATFORM
// =============================================================================

    #define D_ENV_MYSQL_HAS_UNIX_SOCKET_CONN \
        D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET
    #define D_ENV_MYSQL_HAS_NAMED_PIPE \
        D_ENV_MYSQL_COMMON_HAS_NAMED_PIPE
    #define D_ENV_MYSQL_HAS_SHARED_MEMORY \
        D_ENV_MYSQL_COMMON_HAS_SHARED_MEMORY

    #ifndef D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH
        #if ( D_ENV_MYSQL_VERSION_AT_LEAST(5, 5, 10) && \
              D_ENV_MYSQL_COMMON_HAS_UNIX_SOCKET )
            #define D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH 1
        #else
            #define D_ENV_MYSQL_HAS_UNIX_SOCKET_AUTH 0
        #endif
    #endif


// =============================================================================
// XVIII. COMPOSITE MACROS
// =============================================================================

    #define D_ENV_MYSQL_HAS_MODERN_AUTH \
        ( D_ENV_MYSQL_HAS_AUTH_CACHING_SHA2 && \
          D_ENV_MYSQL_HAS_PLUGGABLE_AUTH )

    #define D_ENV_MYSQL_HAS_MODERN_JSON \
        ( D_ENV_MYSQL_HAS_JSON_TYPE          && \
          D_ENV_MYSQL_HAS_JSON_TABLE         && \
          D_ENV_MYSQL_HAS_JSON_VALUE         && \
          D_ENV_MYSQL_HAS_MULTI_VALUE_INDEX )

    #define D_ENV_MYSQL_HAS_MODERN_DDL \
        ( D_ENV_MYSQL_HAS_ATOMIC_DDL         && \
          D_ENV_MYSQL_HAS_INNODB_INSTANT_DDL && \
          D_ENV_MYSQL_HAS_DATA_DICTIONARY )

    #define D_ENV_MYSQL_HAS_MODERN_SQL \
        ( D_ENV_MYSQL_HAS_WINDOW_FUNCTIONS && \
          D_ENV_MYSQL_HAS_CTE             && \
          D_ENV_MYSQL_HAS_LATERAL_DERIVED )

    #define D_ENV_MYSQL_HAS_HA_SUITE \
        ( D_ENV_MYSQL_HAS_GROUP_REPLICATION && \
          D_ENV_MYSQL_HAS_INNODB_CLUSTER    && \
          D_ENV_MYSQL_HAS_CLONE_PLUGIN )

    #define D_ENV_MYSQL_HAS_MODERN_SECURITY \
        ( D_ENV_MYSQL_HAS_ROLES            && \
          D_ENV_MYSQL_HAS_PARTIAL_REVOKE   && \
          D_ENV_MYSQL_HAS_PASSWORD_HISTORY && \
          D_ENV_MYSQL_HAS_DUAL_PASSWORDS )

    #define D_ENV_MYSQL_HAS_MODERN_OPTIMIZER \
        ( D_ENV_MYSQL_HAS_HASH_JOIN        && \
          D_ENV_MYSQL_HAS_HISTOGRAMS       && \
          D_ENV_MYSQL_HAS_EXPLAIN_ANALYZE )

    #define D_ENV_MYSQL_IS_FULLY_MODERN \
        ( D_ENV_MYSQL_HAS_MODERN_AUTH      && \
          D_ENV_MYSQL_HAS_MODERN_DDL       && \
          D_ENV_MYSQL_HAS_MODERN_SQL       && \
          D_ENV_MYSQL_HAS_MODERN_SECURITY  && \
          D_ENV_MYSQL_HAS_MODERN_OPTIMIZER )


// =============================================================================
// XIX.  DEPRECATION AND REMOVAL
// =============================================================================

    #define D_ENV_MYSQL_REMOVED_QUERY_CACHE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_REMOVED_PARTITION_ENGINE \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_REMOVED_PASSWORD_FUNCTION \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 11)
    #define D_ENV_MYSQL_REMOVED_EMBEDDED_SERVER \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_REMOVED_FRM_FILES \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 0)
    #define D_ENV_MYSQL_DEPRECATED_UTF8MB3_ALIAS \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 28)
    #define D_ENV_MYSQL_DEPRECATED_MYSQL_NATIVE_PASSWORD \
        D_ENV_MYSQL_VERSION_AT_LEAST(8, 0, 34)


#endif  // D_ENV_MYSQL_DETECTED


#endif  // DJINTERP_ENVIRONMENT_MYSQL_
