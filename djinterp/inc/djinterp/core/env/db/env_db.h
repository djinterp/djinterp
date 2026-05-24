/******************************************************************************
* djinterp [config][db]                                               env_db.h
* 
* djinterp database environmental detection header:
* This header provides comprehensive compile-time detection of database
* environments and their capabilities, including:
*   - database systems (MariaDB, MySQL, PostgreSQL, SQLite, MongoDB, etc.)
*   - version information and feature detection
*   - database categorization (RDBMS, NoSQL, in-memory, graph, document, etc.)
*   - supported features (transactions, ACID, replication, etc.)
*   - vendor-specific capabilities and extensions
*
*   The header creates a unified D_ENV_DB_* macro interface enabling portable
* database code that adapts to different database systems and versions. All
* detection is performed at compile-time with zero runtime overhead.
*
*   CONFIGURATION SYSTEM:
*   This header supports custom database environment simulation via
* D_CFG_ENV_DB_CUSTOM:
*   - 0 (default): full automatic detection
*   - 1: skip all detection (requires pre-defined D_ENV_DB_DETECTED_* variables)
*   Pre-defining D_ENV_DB_DETECTED_* variables automatically enables custom mode.
*
*   USAGE:
*   This header includes env.h for base environment detection capabilities.
*   Database-specific detection is performed via preprocessor checks of
*   vendor-provided macros and header files.
*
* 
* path:      /inc/djinterp/core/db/env_db.h                                           
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.01.10
******************************************************************************/

#ifndef DJINTERP_ENVIRONMENT_DATABASE_
#define DJINTERP_ENVIRONMENT_DATABASE_ 1

#include "../../../config/core/env/db/env_db_config.h"


// ===========================================================================
// II.  DATABASE SYSTEM IDENTIFICATION
// ===========================================================================

// database system ID flags (unique bit per vendor)
#define D_ENV_DB_FLAG_UNKNOWN      0x0000
#define D_ENV_DB_FLAG_MARIADB      0x0001
#define D_ENV_DB_FLAG_MYSQL        0x0002
#define D_ENV_DB_FLAG_POSTGRESQL   0x0004
#define D_ENV_DB_FLAG_SQLITE       0x0008
#define D_ENV_DB_FLAG_MONGODB      0x0010
#define D_ENV_DB_FLAG_REDIS        0x0020
#define D_ENV_DB_FLAG_ARANGODB     0x0040
#define D_ENV_DB_FLAG_ORACLE       0x0080
#define D_ENV_DB_FLAG_MSSQL        0x0100
#define D_ENV_DB_FLAG_DB2          0x0200
#define D_ENV_DB_FLAG_FIREBASE     0x0400
#define D_ENV_DB_FLAG_CASSANDRA    0x0800
#define D_ENV_DB_FLAG_COUCHDB      0x1000
#define D_ENV_DB_FLAG_NEO4J        0x2000


// ===========================================================================
// III. DATABASE CATEGORIZATION
// ===========================================================================

// database type/category flags (can have multiple categories)
#define D_ENV_DB_CAT_UNKNOWN       0x00000000

// primary database models
#define D_ENV_DB_CAT_RELATIONAL    0x00000001  // traditional RDBMS
#define D_ENV_DB_CAT_DOCUMENT      0x00000002  // document stores
#define D_ENV_DB_CAT_KEY_VALUE     0x00000004  // key-value stores
#define D_ENV_DB_CAT_GRAPH         0x00000008  // graph databases
#define D_ENV_DB_CAT_COLUMN_FAMILY 0x00000010  // wide-column stores
#define D_ENV_DB_CAT_TIME_SERIES   0x00000020  // time-series databases
#define D_ENV_DB_CAT_SEARCH_ENGINE 0x00000040  // full-text search engines

// secondary characteristics
#define D_ENV_DB_CAT_IN_MEMORY     0x00000100  // in-memory database
#define D_ENV_DB_CAT_EMBEDDED      0x00000200  // embedded/serverless
#define D_ENV_DB_CAT_DISTRIBUTED   0x00000400  // distributed architecture
#define D_ENV_DB_CAT_CLOUD_NATIVE  0x00000800  // cloud-native service
#define D_ENV_DB_CAT_MULTI_MODEL   0x00001000  // supports multiple models

// SQL support
#define D_ENV_DB_CAT_SQL           0x00010000  // SQL or SQL-like queries
#define D_ENV_DB_CAT_NOSQL         0x00020000  // NoSQL database

// convenience macros for common categorizations
#define D_ENV_DB_IS_RDBMS(cat)     ((cat) & D_ENV_DB_CAT_RELATIONAL)
#define D_ENV_DB_IS_NOSQL(cat)     ((cat) & D_ENV_DB_CAT_NOSQL)
#define D_ENV_DB_IS_DOCUMENT(cat)  ((cat) & D_ENV_DB_CAT_DOCUMENT)
#define D_ENV_DB_IS_GRAPH(cat)     ((cat) & D_ENV_DB_CAT_GRAPH)
#define D_ENV_DB_IS_IN_MEMORY(cat) ((cat) & D_ENV_DB_CAT_IN_MEMORY)
#define D_ENV_DB_IS_EMBEDDED(cat)  ((cat) & D_ENV_DB_CAT_EMBEDDED)


// ===========================================================================
// IV.  FEATURE DETECTION FLAGS
// ===========================================================================

// D_ENV_DB_SUPPORTS_* - feature capability flags

// transaction support
#define D_ENV_DB_SUPPORTS_TRANSACTIONS          0x00000001
#define D_ENV_DB_SUPPORTS_SAVEPOINTS            0x00000002
#define D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS   0x00000004
#define D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT      0x00000008

// ACID compliance
#define D_ENV_DB_SUPPORTS_ACID                  0x00000010
#define D_ENV_DB_SUPPORTS_ATOMICITY             0x00000020
#define D_ENV_DB_SUPPORTS_CONSISTENCY           0x00000040
#define D_ENV_DB_SUPPORTS_ISOLATION             0x00000080
#define D_ENV_DB_SUPPORTS_DURABILITY            0x00000100

// query capabilities
#define D_ENV_DB_SUPPORTS_JOINS                 0x00000200
#define D_ENV_DB_SUPPORTS_SUBQUERIES            0x00000400
#define D_ENV_DB_SUPPORTS_VIEWS                 0x00000800
#define D_ENV_DB_SUPPORTS_STORED_PROCEDURES     0x00001000
#define D_ENV_DB_SUPPORTS_TRIGGERS              0x00002000
#define D_ENV_DB_SUPPORTS_USER_FUNCTIONS        0x00004000

// indexing and optimization
#define D_ENV_DB_SUPPORTS_INDEXES               0x00010000
#define D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS    0x00020000
#define D_ENV_DB_SUPPORTS_FOREIGN_KEYS          0x00040000
#define D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH      0x00080000

// replication and clustering
#define D_ENV_DB_SUPPORTS_REPLICATION           0x00100000
#define D_ENV_DB_SUPPORTS_CLUSTERING            0x00200000
#define D_ENV_DB_SUPPORTS_SHARDING              0x00400000
#define D_ENV_DB_SUPPORTS_AUTO_FAILOVER         0x00800000

// advanced features
#define D_ENV_DB_SUPPORTS_JSON                  0x01000000
#define D_ENV_DB_SUPPORTS_XML                   0x02000000
#define D_ENV_DB_SUPPORTS_SPATIAL_DATA          0x04000000
#define D_ENV_DB_SUPPORTS_PARTITIONING          0x08000000

// security and access control
#define D_ENV_DB_SUPPORTS_ENCRYPTION            0x10000000
#define D_ENV_DB_SUPPORTS_SSL_TLS               0x20000000
#define D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY    0x40000000
#define D_ENV_DB_SUPPORTS_AUDIT_LOGGING         0x80000000


// ===========================================================================
// V.   DATABASE SYSTEM DETECTION
// ===========================================================================

#if (D_CFG_ENV_DB_CUSTOM == 0)
    // automatic detection based on vendor-specific preprocessor macros

    // MariaDB detection (check before MySQL as MariaDB defines MySQL macros)
    #if defined(MARIADB_VERSION_ID)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MARIADB
        #define D_ENV_DB_NAME            "MariaDB"
        #define D_ENV_DB_VERSION_MAJOR   (MARIADB_VERSION_ID / 10000)
        #define D_ENV_DB_VERSION_MINOR   ((MARIADB_VERSION_ID / 100) % 100)
        #define D_ENV_DB_VERSION_PATCH   (MARIADB_VERSION_ID % 100)
        #define D_ENV_DB_VERSION_ID      MARIADB_VERSION_ID
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_SAVEPOINTS          |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_SUBQUERIES          |  \
                                           D_ENV_DB_SUPPORTS_VIEWS               |  \
                                           D_ENV_DB_SUPPORTS_STORED_PROCEDURES   |  \
                                           D_ENV_DB_SUPPORTS_TRIGGERS            |  \
                                           D_ENV_DB_SUPPORTS_INDEXES             |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS  |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH    |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION         |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING          |  \
                                           D_ENV_DB_SUPPORTS_JSON                |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA        |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING        |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION          |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS )

    // MySQL detection
    #elif defined(MYSQL_VERSION_ID)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MYSQL
        #define D_ENV_DB_NAME            "MySQL"
        #define D_ENV_DB_VERSION_MAJOR   (MYSQL_VERSION_ID / 10000)
        #define D_ENV_DB_VERSION_MINOR   ((MYSQL_VERSION_ID / 100) % 100)
        #define D_ENV_DB_VERSION_PATCH   (MYSQL_VERSION_ID % 100)
        #define D_ENV_DB_VERSION_ID      MYSQL_VERSION_ID
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_SAVEPOINTS          |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_SUBQUERIES          |  \
                                           D_ENV_DB_SUPPORTS_VIEWS               |  \
                                           D_ENV_DB_SUPPORTS_STORED_PROCEDURES   |  \
                                           D_ENV_DB_SUPPORTS_TRIGGERS            |  \
                                           D_ENV_DB_SUPPORTS_INDEXES             |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS  |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH    |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION         |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING          |  \
                                           D_ENV_DB_SUPPORTS_JSON                |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA        |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING        |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION          |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS )

    // PostgreSQL detection
    #elif defined(PG_VERSION_NUM)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_POSTGRESQL
        #define D_ENV_DB_NAME            "PostgreSQL"
        #define D_ENV_DB_VERSION_MAJOR   (PG_VERSION_NUM / 10000)
        #define D_ENV_DB_VERSION_MINOR   ((PG_VERSION_NUM / 100) % 100)
        #define D_ENV_DB_VERSION_PATCH   (PG_VERSION_NUM % 100)
        #define D_ENV_DB_VERSION_ID      PG_VERSION_NUM
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL           |  \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS          |  \
                                           D_ENV_DB_SUPPORTS_SAVEPOINTS            |  \
                                           D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS   |  \
                                           D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT      |  \
                                           D_ENV_DB_SUPPORTS_ACID                  |  \
                                           D_ENV_DB_SUPPORTS_JOINS                 |  \
                                           D_ENV_DB_SUPPORTS_SUBQUERIES            |  \
                                           D_ENV_DB_SUPPORTS_VIEWS                 |  \
                                           D_ENV_DB_SUPPORTS_STORED_PROCEDURES     |  \
                                           D_ENV_DB_SUPPORTS_TRIGGERS              |  \
                                           D_ENV_DB_SUPPORTS_USER_FUNCTIONS        |  \
                                           D_ENV_DB_SUPPORTS_INDEXES               |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS    |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS          |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH      |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION           |  \
                                           D_ENV_DB_SUPPORTS_SHARDING              |  \
                                           D_ENV_DB_SUPPORTS_JSON                  |  \
                                           D_ENV_DB_SUPPORTS_XML                   |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA          |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING          |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION            |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS               |  \
                                           D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY )

    // SQLite detection
    #elif defined(SQLITE_VERSION_NUMBER)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_SQLITE
        #define D_ENV_DB_NAME            "SQLite"
        #define D_ENV_DB_VERSION_MAJOR   (SQLITE_VERSION_NUMBER / 1000000)
        #define D_ENV_DB_VERSION_MINOR   ((SQLITE_VERSION_NUMBER / 1000) % 1000)
        #define D_ENV_DB_VERSION_PATCH   (SQLITE_VERSION_NUMBER % 1000)
        #define D_ENV_DB_VERSION_ID      SQLITE_VERSION_NUMBER
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL  |  \
                                           D_ENV_DB_CAT_SQL         |  \
                                           D_ENV_DB_CAT_EMBEDDED )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_SAVEPOINTS          |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_SUBQUERIES          |  \
                                           D_ENV_DB_SUPPORTS_VIEWS               |  \
                                           D_ENV_DB_SUPPORTS_TRIGGERS            |  \
                                           D_ENV_DB_SUPPORTS_INDEXES             |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS  |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH    |  \
                                           D_ENV_DB_SUPPORTS_JSON                |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING        |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION )

    // MongoDB detection
    #elif defined(MONGOC_VERSION_S)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MONGODB
        #define D_ENV_DB_NAME            "MongoDB"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS      |  \
                                           D_ENV_DB_SUPPORTS_ACID              |  \
                                           D_ENV_DB_SUPPORTS_INDEXES           |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH  |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION       |  \
                                           D_ENV_DB_SUPPORTS_SHARDING          |  \
                                           D_ENV_DB_SUPPORTS_AUTO_FAILOVER     |  \
                                           D_ENV_DB_SUPPORTS_JSON              |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA      |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION        |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS           |  \
                                           D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // Redis detection
    #elif defined(REDIS_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_REDIS
        #define D_ENV_DB_NAME            "Redis"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_KEY_VALUE     |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_IN_MEMORY     |  \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS      |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION       |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING        |  \
                                           D_ENV_DB_SUPPORTS_SHARDING          |  \
                                           D_ENV_DB_SUPPORTS_JSON              |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION        |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS )

    // ArangoDB detection
    #elif defined(ARANGODB_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ARANGODB
        #define D_ENV_DB_NAME            "ArangoDB"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_GRAPH         |  \
                                           D_ENV_DB_CAT_KEY_VALUE     |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_MULTI_MODEL   |  \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_INDEXES             |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS  |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH    |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION         |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING          |  \
                                           D_ENV_DB_SUPPORTS_SHARDING            |  \
                                           D_ENV_DB_SUPPORTS_JSON                |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA        |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION          |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS )

    // Oracle Database detection
    #elif defined(ORACLE_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ORACLE
        #define D_ENV_DB_NAME            "Oracle Database"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL           |  \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS          |  \
                                           D_ENV_DB_SUPPORTS_SAVEPOINTS            |  \
                                           D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT      |  \
                                           D_ENV_DB_SUPPORTS_ACID                  |  \
                                           D_ENV_DB_SUPPORTS_JOINS                 |  \
                                           D_ENV_DB_SUPPORTS_SUBQUERIES            |  \
                                           D_ENV_DB_SUPPORTS_VIEWS                 |  \
                                           D_ENV_DB_SUPPORTS_STORED_PROCEDURES     |  \
                                           D_ENV_DB_SUPPORTS_TRIGGERS              |  \
                                           D_ENV_DB_SUPPORTS_USER_FUNCTIONS        |  \
                                           D_ENV_DB_SUPPORTS_INDEXES               |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS    |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS          |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH      |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION           |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING            |  \
                                           D_ENV_DB_SUPPORTS_SHARDING              |  \
                                           D_ENV_DB_SUPPORTS_JSON                  |  \
                                           D_ENV_DB_SUPPORTS_XML                   |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA          |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING          |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION            |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS               |  \
                                           D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY    |  \
                                           D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // Microsoft SQL Server detection
    #elif defined(_MSSQL_VER)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MSSQL
        #define D_ENV_DB_NAME            "Microsoft SQL Server"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS          |  \
                                           D_ENV_DB_SUPPORTS_SAVEPOINTS            |  \
                                           D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS   |  \
                                           D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT      |  \
                                           D_ENV_DB_SUPPORTS_ACID                  |  \
                                           D_ENV_DB_SUPPORTS_JOINS                 |  \
                                           D_ENV_DB_SUPPORTS_SUBQUERIES            |  \
                                           D_ENV_DB_SUPPORTS_VIEWS                 |  \
                                           D_ENV_DB_SUPPORTS_STORED_PROCEDURES     |  \
                                           D_ENV_DB_SUPPORTS_TRIGGERS              |  \
                                           D_ENV_DB_SUPPORTS_USER_FUNCTIONS        |  \
                                           D_ENV_DB_SUPPORTS_INDEXES               |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS    |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS          |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH      |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION           |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING            |  \
                                           D_ENV_DB_SUPPORTS_JSON                  |  \
                                           D_ENV_DB_SUPPORTS_XML                   |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA          |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING          |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION            |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS               |  \
                                           D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY    |  \
                                           D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // IBM DB2 detection
    #elif defined(DB2_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_DB2
        #define D_ENV_DB_NAME            "IBM DB2"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS          |  \
                                           D_ENV_DB_SUPPORTS_SAVEPOINTS            |  \
                                           D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT      |  \
                                           D_ENV_DB_SUPPORTS_ACID                  |  \
                                           D_ENV_DB_SUPPORTS_JOINS                 |  \
                                           D_ENV_DB_SUPPORTS_SUBQUERIES            |  \
                                           D_ENV_DB_SUPPORTS_VIEWS                 |  \
                                           D_ENV_DB_SUPPORTS_STORED_PROCEDURES     |  \
                                           D_ENV_DB_SUPPORTS_TRIGGERS              |  \
                                           D_ENV_DB_SUPPORTS_USER_FUNCTIONS        |  \
                                           D_ENV_DB_SUPPORTS_INDEXES               |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS    |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS          |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH      |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION           |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING            |  \
                                           D_ENV_DB_SUPPORTS_JSON                  |  \
                                           D_ENV_DB_SUPPORTS_XML                   |  \
                                           D_ENV_DB_SUPPORTS_SPATIAL_DATA          |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING          |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION            |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS               |  \
                                           D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // Firebase detection
    #elif defined(FIREBASE_VERSION_MAJOR)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_FIREBASE
        #define D_ENV_DB_NAME            "Firebase"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_CLOUD_NATIVE )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_INDEXES           |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION       |  \
                                           D_ENV_DB_SUPPORTS_JSON              |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION        |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS           |  \
                                           D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY )

    // Apache Cassandra detection
    #elif defined(CASSANDRA_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_CASSANDRA
        #define D_ENV_DB_NAME            "Apache Cassandra"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_COLUMN_FAMILY |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_INDEXES           |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION       |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING        |  \
                                           D_ENV_DB_SUPPORTS_SHARDING          |  \
                                           D_ENV_DB_SUPPORTS_AUTO_FAILOVER     |  \
                                           D_ENV_DB_SUPPORTS_JSON              |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING      |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION        |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS )

    // CouchDB detection
    #elif defined(COUCHDB_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_COUCHDB
        #define D_ENV_DB_NAME            "Apache CouchDB"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_ACID              |  \
                                           D_ENV_DB_SUPPORTS_VIEWS             |  \
                                           D_ENV_DB_SUPPORTS_INDEXES           |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH  |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION       |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING        |  \
                                           D_ENV_DB_SUPPORTS_JSON              |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION        |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS )

    // Neo4j detection
    #elif defined(NEO4J_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_NEO4J
        #define D_ENV_DB_NAME            "Neo4j"
        
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_GRAPH         |  \
                                           D_ENV_DB_CAT_NOSQL )
        
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_INDEXES             |  \
                                           D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS  |  \
                                           D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH    |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION         |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING          |  \
                                           D_ENV_DB_SUPPORTS_SHARDING            |  \
                                           D_ENV_DB_SUPPORTS_ENCRYPTION          |  \
                                           D_ENV_DB_SUPPORTS_SSL_TLS             |  \
                                           D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY  |  \
                                           D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // unknown/no database detected
    #else
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_UNKNOWN
        #define D_ENV_DB_NAME            "Unknown"
        #define D_ENV_DB_CATEGORY        D_ENV_DB_CAT_UNKNOWN
        #define D_ENV_DB_FEATURES        0

    #endif  // database detection

#else
    // manual detection using pre-defined variables
    #ifdef D_ENV_DB_DETECTED_MARIADB
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MARIADB
        #define D_ENV_DB_NAME            "MariaDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL | D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION )

    #elif defined(D_ENV_DB_DETECTED_MYSQL)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MYSQL
        #define D_ENV_DB_NAME            "MySQL"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL | D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION )

    #elif defined(D_ENV_DB_DETECTED_POSTGRESQL)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_POSTGRESQL
        #define D_ENV_DB_NAME            "PostgreSQL"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL           |  \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_SQLITE)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_SQLITE
        #define D_ENV_DB_NAME            "SQLite"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL  |  \
                                           D_ENV_DB_CAT_SQL         |  \
                                           D_ENV_DB_CAT_EMBEDDED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |  \
                                           D_ENV_DB_SUPPORTS_ACID          |  \
                                           D_ENV_DB_SUPPORTS_JOINS         |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS )

    #elif defined(D_ENV_DB_DETECTED_MONGODB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MONGODB
        #define D_ENV_DB_NAME            "MongoDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION   |  \
                                           D_ENV_DB_SUPPORTS_SHARDING      |  \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_REDIS)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_REDIS
        #define D_ENV_DB_NAME            "Redis"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_KEY_VALUE     |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_IN_MEMORY )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION   |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING )

    #elif defined(D_ENV_DB_DETECTED_ARANGODB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ARANGODB
        #define D_ENV_DB_NAME            "ArangoDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_GRAPH         |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |  \
                                           D_ENV_DB_SUPPORTS_ACID          |  \
                                           D_ENV_DB_SUPPORTS_JOINS         |  \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_ORACLE)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ORACLE
        #define D_ENV_DB_NAME            "Oracle Database"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |  \
                                           D_ENV_DB_CAT_SQL           |  \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING )

    #elif defined(D_ENV_DB_DETECTED_MSSQL)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MSSQL
        #define D_ENV_DB_NAME            "Microsoft SQL Server"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL | D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING )

    #elif defined(D_ENV_DB_DETECTED_DB2)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_DB2
        #define D_ENV_DB_NAME            "IBM DB2"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL | D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |  \
                                           D_ENV_DB_SUPPORTS_ACID                |  \
                                           D_ENV_DB_SUPPORTS_JOINS               |  \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |  \
                                           D_ENV_DB_SUPPORTS_PARTITIONING )

    #elif defined(D_ENV_DB_DETECTED_FIREBASE)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_FIREBASE
        #define D_ENV_DB_NAME            "Firebase"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_CLOUD_NATIVE )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_JSON              |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION       |  \
                                           D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY )

    #elif defined(D_ENV_DB_DETECTED_CASSANDRA)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_CASSANDRA
        #define D_ENV_DB_NAME            "Apache Cassandra"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_COLUMN_FAMILY |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_REPLICATION   |  \
                                           D_ENV_DB_SUPPORTS_SHARDING      |  \
                                           D_ENV_DB_SUPPORTS_AUTO_FAILOVER )

    #elif defined(D_ENV_DB_DETECTED_COUCHDB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_COUCHDB
        #define D_ENV_DB_NAME            "Apache CouchDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |  \
                                           D_ENV_DB_CAT_NOSQL         |  \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_ACID          |  \
                                           D_ENV_DB_SUPPORTS_REPLICATION   |  \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_NEO4J)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_NEO4J
        #define D_ENV_DB_NAME            "Neo4j"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_GRAPH | D_ENV_DB_CAT_NOSQL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |  \
                                           D_ENV_DB_SUPPORTS_ACID          |  \
                                           D_ENV_DB_SUPPORTS_CLUSTERING )

    #elif defined(D_ENV_DB_DETECTED_UNKNOWN)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_UNKNOWN
        #define D_ENV_DB_NAME            "Unknown"
        #define D_ENV_DB_CATEGORY        D_ENV_DB_CAT_UNKNOWN
        #define D_ENV_DB_FEATURES        0

    #endif  // manual detection

#endif  // D_CFG_ENV_DB_CUSTOM


// ===========================================================================
// VI.  CONVENIENCE MACROS
// ===========================================================================

// D_ENV_DB_HAS_FEATURE
//   macro: checks if the detected database supports a specific feature.
#define D_ENV_DB_HAS_FEATURE(feature) \
    ((D_ENV_DB_FEATURES) & (feature))

// D_ENV_DB_IS_CATEGORY
//   macro: checks if the detected database belongs to a specific category.
#define D_ENV_DB_IS_CATEGORY(category) \
    ((D_ENV_DB_CATEGORY) & (category))

// D_ENV_DB_IS_DBMS
//   macro: checks if the detected database is any type of database management
// system (RDBMS or NoSQL).
#define D_ENV_DB_IS_DBMS \
    (D_ENV_DB_ID != D_ENV_DB_FLAG_UNKNOWN)

// version comparison macros (only available for databases with version info)
#ifdef D_ENV_DB_VERSION_ID
    // D_ENV_DB_VERSION_AT_LEAST
    //   macro: checks if database version is at least the specified version.
    #define D_ENV_DB_VERSION_AT_LEAST(major, minor, patch) \
        (D_ENV_DB_VERSION_ID >= ((major) * 10000 + (minor) * 100 + (patch)))

    // D_ENV_DB_VERSION_BELOW
    //   macro: checks if database version is below the specified version.
    #define D_ENV_DB_VERSION_BELOW(major, minor, patch) \
        (D_ENV_DB_VERSION_ID < ((major) * 10000 + (minor) * 100 + (patch)))
#endif


// ===========================================================================
// VII. FEATURE COMBINATION SHORTCUTS
// ===========================================================================

// D_ENV_DB_IS_FULLY_ACID
//   macro: checks if database supports all ACID properties.
#define D_ENV_DB_IS_FULLY_ACID \
    ( D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_ATOMICITY)    &&  \
      D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_CONSISTENCY)  &&  \
      D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_ISOLATION)    &&  \
      D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_DURABILITY) )

// D_ENV_DB_SUPPORTS_ADVANCED_TRANSACTIONS
//   macro: checks if database supports advanced transaction features.
#define D_ENV_DB_SUPPORTS_ADVANCED_TRANSACTIONS \
    ( D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_TRANSACTIONS)  &&  \
      ( D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_SAVEPOINTS) ||  \
        D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS) ) )

// D_ENV_DB_SUPPORTS_RELATIONAL_INTEGRITY
//   macro: checks if database supports relational integrity constraints.
#define D_ENV_DB_SUPPORTS_RELATIONAL_INTEGRITY \
    ( D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS) &&  \
      D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_FOREIGN_KEYS) )

// D_ENV_DB_SUPPORTS_HIGH_AVAILABILITY
//   macro: checks if database supports high availability features.
#define D_ENV_DB_SUPPORTS_HIGH_AVAILABILITY \
    ( D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_REPLICATION)   &&  \
      ( D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_CLUSTERING) ||  \
        D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_AUTO_FAILOVER) ) )

// D_ENV_DB_SUPPORTS_SCALE_OUT
//   macro: checks if database supports horizontal scaling.
#define D_ENV_DB_SUPPORTS_SCALE_OUT \
    ( D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_SHARDING) ||  \
      D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_PARTITIONING) )


#endif  // DJINTERP_ENVIRONMENT_DATABASE_