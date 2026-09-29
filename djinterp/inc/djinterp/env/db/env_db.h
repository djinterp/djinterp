/*******************************************************************************
* djinterp [env]                                                        env_db.h
*
* djinterp database environment detection.
*   Compile-time identification of the database system a translation unit
* works with, from the vendors' own macros: its identity (D_ENV_DB_ID and
* D_ENV_DB_NAME), its version where the vendor headers expose one, its data
* model and characteristics as category bits, and its capabilities as feature
* bits, with tests and combinations over them.
*   The per-database headers, env_arangodb.h through env_sqlite.h, detect each
* system in depth, and each includes this one. This header reads only the
* vendors' macros, so their headers must come first; it includes nothing but
* its settings, cfg_env_db.h, where D_CFG_ENV_DB_CUSTOM, or any pre-defined
* D_ENV_DB_DETECTED_*, switches to manual detection.
*
*
* path:      /inc/djinterp/env/db/env_db.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.01.10
*                                                            revised: 2026.09.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DATABASE SYSTEM IDENTIFICATION
    ------------------------------
    1.  Database identifiers
         1.  D_ENV_DB_FLAG_<DATABASE>
2.  DATABASE CATEGORIZATION
    -----------------------
    1.  Primary models
         1.  D_ENV_DB_CAT_<MODEL>
    2.  Secondary characteristics
         1.  D_ENV_DB_CAT_<CHARACTERISTIC>
    3.  SQL support
         1.  D_ENV_DB_CAT_SQL / D_ENV_DB_CAT_NOSQL
    4.  Category tests
         1.  D_ENV_DB_IS_<CATEGORY>
3.  FEATURE DETECTION FLAGS
    -----------------------
    1.  Feature bits
         1.  Transaction support
         2.  ACID compliance
         3.  Query capabilities
         4.  Indexing and optimization
         5.  Replication and clustering
         6.  Advanced features
         7.  Security and access control
4.  DATABASE SYSTEM DETECTION
    -------------------------
    1.  Detected database
         1.  D_ENV_DB_ID / D_ENV_DB_NAME
5.  CONVENIENCE MACROS
    ------------------
    1.  Queries
         1.  D_ENV_DB_HAS_FEATURE
         2.  D_ENV_DB_IS_CATEGORY
         3.  D_ENV_DB_IS_DBMS
    2.  Version comparisons
         1.  D_ENV_DB_VERSION_AT_LEAST
         2.  D_ENV_DB_VERSION_BELOW
6.  FEATURE COMBINATION SHORTCUTS
    -----------------------------
    1.  Feature combinations
         1.  D_ENV_DB_IS_FULLY_ACID
         2.  D_ENV_DB_SUPPORTS_ADVANCED_TRANSACTIONS
         3.  D_ENV_DB_SUPPORTS_RELATIONAL_INTEGRITY
         4.  D_ENV_DB_SUPPORTS_HIGH_AVAILABILITY
         5.  D_ENV_DB_SUPPORTS_SCALE_OUT
*/

#ifndef DJINTERP_ENV_DB_ENV_DB_H
#define DJINTERP_ENV_DB_ENV_DB_H 1

// djinterp
#include "../../config/core/env/db/cfg_env_db.h"  // D_CFG_ENV_DB_CUSTOM


//==============================================================================
// 1.  DATABASE SYSTEM IDENTIFICATION
//==============================================================================
// The D_ENV_DB_FLAG_* values identify a database system in D_ENV_DB_ID.


// 1.1    Database identifiers
//------------------------------------------------------------------------------
// 1.1.1
// D_ENV_DB_FLAG_<DATABASE>
//   constant: the value D_ENV_DB_ID takes for each database system, one bit
// each; D_ENV_DB_FLAG_UNKNOWN is 0.
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
#define D_ENV_DB_FLAG_DYNAMODB     0x4000


//==============================================================================
// 2.  DATABASE CATEGORIZATION
//==============================================================================
// The D_ENV_DB_CAT_* bits describe a database's data model and
// characteristics; D_ENV_DB_CATEGORY combines those of the detected database.


// 2.1    Primary models
//------------------------------------------------------------------------------
// 2.1.1
// D_ENV_DB_CAT_<MODEL>
//   constant: category bits for the primary data models, which
// D_ENV_DB_CATEGORY combines; D_ENV_DB_CAT_UNKNOWN is 0.
#define D_ENV_DB_CAT_UNKNOWN       0x00000000
#define D_ENV_DB_CAT_RELATIONAL    0x00000001  // traditional RDBMS
#define D_ENV_DB_CAT_DOCUMENT      0x00000002  // document stores
#define D_ENV_DB_CAT_KEY_VALUE     0x00000004  // key-value stores
#define D_ENV_DB_CAT_GRAPH         0x00000008  // graph databases
#define D_ENV_DB_CAT_COLUMN_FAMILY 0x00000010  // wide-column stores
#define D_ENV_DB_CAT_TIME_SERIES   0x00000020  // time-series databases
#define D_ENV_DB_CAT_SEARCH_ENGINE 0x00000040  // full-text search engines

// 2.2    Secondary characteristics
//------------------------------------------------------------------------------
// 2.2.1
// D_ENV_DB_CAT_<CHARACTERISTIC>
//   constant: category bits for secondary characteristics: in-memory, embedded,
// distributed, cloud-native, and multi-model.
#define D_ENV_DB_CAT_IN_MEMORY     0x00000100  // in-memory database
#define D_ENV_DB_CAT_EMBEDDED      0x00000200  // embedded/serverless
#define D_ENV_DB_CAT_DISTRIBUTED   0x00000400  // distributed architecture
#define D_ENV_DB_CAT_CLOUD_NATIVE  0x00000800  // cloud-native service
#define D_ENV_DB_CAT_MULTI_MODEL   0x00001000  // supports multiple models

// 2.3    SQL support
//------------------------------------------------------------------------------
// 2.3.1
// D_ENV_DB_CAT_SQL / D_ENV_DB_CAT_NOSQL
//   constant: category bits for SQL (or SQL-like) and NoSQL databases.
#define D_ENV_DB_CAT_SQL           0x00010000  // SQL or SQL-like queries
#define D_ENV_DB_CAT_NOSQL         0x00020000  // NoSQL database

// 2.4    Category tests
//------------------------------------------------------------------------------
// 2.4.1
// D_ENV_DB_IS_<CATEGORY>
//   macro: nonzero when the category bits cat include that category:
// D_ENV_DB_IS_RDBMS, D_ENV_DB_IS_NOSQL, D_ENV_DB_IS_DOCUMENT,
// D_ENV_DB_IS_GRAPH, D_ENV_DB_IS_IN_MEMORY and D_ENV_DB_IS_EMBEDDED.
#define D_ENV_DB_IS_RDBMS(cat)     ((cat) & D_ENV_DB_CAT_RELATIONAL)
#define D_ENV_DB_IS_NOSQL(cat)     ((cat) & D_ENV_DB_CAT_NOSQL)
#define D_ENV_DB_IS_DOCUMENT(cat)  ((cat) & D_ENV_DB_CAT_DOCUMENT)
#define D_ENV_DB_IS_GRAPH(cat)     ((cat) & D_ENV_DB_CAT_GRAPH)
#define D_ENV_DB_IS_IN_MEMORY(cat) ((cat) & D_ENV_DB_CAT_IN_MEMORY)
#define D_ENV_DB_IS_EMBEDDED(cat)  ((cat) & D_ENV_DB_CAT_EMBEDDED)


//==============================================================================
// 3.  FEATURE DETECTION FLAGS
//==============================================================================
// D_ENV_DB_FEATURES combines the D_ENV_DB_SUPPORTS_* bits that the detected
// database supports; test one with D_ENV_DB_HAS_FEATURE (5.1.1).


// 3.1    Feature bits
//------------------------------------------------------------------------------
// 3.1.1
// Transaction support
//   constant: feature bits for transaction support.
#define D_ENV_DB_SUPPORTS_TRANSACTIONS          0x00000001
#define D_ENV_DB_SUPPORTS_SAVEPOINTS            0x00000002
#define D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS   0x00000004
#define D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT      0x00000008

// 3.1.2
// ACID compliance
//   constant: feature bits for ACID compliance.
#define D_ENV_DB_SUPPORTS_ACID                  0x00000010
#define D_ENV_DB_SUPPORTS_ATOMICITY             0x00000020
#define D_ENV_DB_SUPPORTS_CONSISTENCY           0x00000040
#define D_ENV_DB_SUPPORTS_ISOLATION             0x00000080
#define D_ENV_DB_SUPPORTS_DURABILITY            0x00000100

// 3.1.3
// Query capabilities
//   constant: feature bits for query capabilities.
#define D_ENV_DB_SUPPORTS_JOINS                 0x00000200
#define D_ENV_DB_SUPPORTS_SUBQUERIES            0x00000400
#define D_ENV_DB_SUPPORTS_VIEWS                 0x00000800
#define D_ENV_DB_SUPPORTS_STORED_PROCEDURES     0x00001000
#define D_ENV_DB_SUPPORTS_TRIGGERS              0x00002000
#define D_ENV_DB_SUPPORTS_USER_FUNCTIONS        0x00004000

// 3.1.4
// Indexing and optimization
//   constant: feature bits for indexing and optimization.
#define D_ENV_DB_SUPPORTS_INDEXES               0x00010000
#define D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS    0x00020000
#define D_ENV_DB_SUPPORTS_FOREIGN_KEYS          0x00040000
#define D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH      0x00080000

// 3.1.5
// Replication and clustering
//   constant: feature bits for replication and clustering.
#define D_ENV_DB_SUPPORTS_REPLICATION           0x00100000
#define D_ENV_DB_SUPPORTS_CLUSTERING            0x00200000
#define D_ENV_DB_SUPPORTS_SHARDING              0x00400000
#define D_ENV_DB_SUPPORTS_AUTO_FAILOVER         0x00800000

// 3.1.6
// Advanced features
//   constant: feature bits for advanced features.
#define D_ENV_DB_SUPPORTS_JSON                  0x01000000
#define D_ENV_DB_SUPPORTS_XML                   0x02000000
#define D_ENV_DB_SUPPORTS_SPATIAL_DATA          0x04000000
#define D_ENV_DB_SUPPORTS_PARTITIONING          0x08000000

// 3.1.7
// Security and access control
//   constant: feature bits for security and access control.
#define D_ENV_DB_SUPPORTS_ENCRYPTION            0x10000000
#define D_ENV_DB_SUPPORTS_SSL_TLS               0x20000000
#define D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY    0x40000000
#define D_ENV_DB_SUPPORTS_AUDIT_LOGGING         0x80000000


//==============================================================================
// 4.  DATABASE SYSTEM DETECTION
//==============================================================================
// One database per translation unit: automatic mode tests each vendor's own
// macros in turn, and the first whose macros are in scope wins; manual mode
// (D_CFG_ENV_DB_CUSTOM) takes a pre-defined D_ENV_DB_DETECTED_<DATABASE>.


// 4.1    Detected database
//------------------------------------------------------------------------------
// 4.1.1
// D_ENV_DB_ID / D_ENV_DB_NAME
//   detection: the detected database, as one of the D_ENV_DB_FLAG_* values and
// a name, with D_ENV_DB_CATEGORY and D_ENV_DB_FEATURES, its category and
// feature bits. Where the vendor's headers give a version, automatic mode also
// defines the D_ENV_DB_VERSION_* values: _ID, _MAJOR, _MINOR and _PATCH.
// MariaDB is tested before MySQL, since MariaDB defines MySQL's macros too.
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

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS       |                           \
              D_ENV_DB_SUPPORTS_SAVEPOINTS         |                           \
              D_ENV_DB_SUPPORTS_ACID               |                           \
              D_ENV_DB_SUPPORTS_JOINS              |                           \
              D_ENV_DB_SUPPORTS_SUBQUERIES         |                           \
              D_ENV_DB_SUPPORTS_VIEWS              |                           \
              D_ENV_DB_SUPPORTS_STORED_PROCEDURES  |                           \
              D_ENV_DB_SUPPORTS_TRIGGERS           |                           \
              D_ENV_DB_SUPPORTS_INDEXES            |                           \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS |                           \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS       |                           \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH   |                           \
              D_ENV_DB_SUPPORTS_REPLICATION        |                           \
              D_ENV_DB_SUPPORTS_CLUSTERING         |                           \
              D_ENV_DB_SUPPORTS_JSON               |                           \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA       |                           \
              D_ENV_DB_SUPPORTS_PARTITIONING       |                           \
              D_ENV_DB_SUPPORTS_ENCRYPTION         |                           \
              D_ENV_DB_SUPPORTS_SSL_TLS )

    // MySQL detection
    #elif defined(MYSQL_VERSION_ID)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MYSQL
        #define D_ENV_DB_NAME            "MySQL"
        #define D_ENV_DB_VERSION_MAJOR   (MYSQL_VERSION_ID / 10000)
        #define D_ENV_DB_VERSION_MINOR   ((MYSQL_VERSION_ID / 100) % 100)
        #define D_ENV_DB_VERSION_PATCH   (MYSQL_VERSION_ID % 100)
        #define D_ENV_DB_VERSION_ID      MYSQL_VERSION_ID

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS       |                           \
              D_ENV_DB_SUPPORTS_SAVEPOINTS         |                           \
              D_ENV_DB_SUPPORTS_ACID               |                           \
              D_ENV_DB_SUPPORTS_JOINS              |                           \
              D_ENV_DB_SUPPORTS_SUBQUERIES         |                           \
              D_ENV_DB_SUPPORTS_VIEWS              |                           \
              D_ENV_DB_SUPPORTS_STORED_PROCEDURES  |                           \
              D_ENV_DB_SUPPORTS_TRIGGERS           |                           \
              D_ENV_DB_SUPPORTS_INDEXES            |                           \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS |                           \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS       |                           \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH   |                           \
              D_ENV_DB_SUPPORTS_REPLICATION        |                           \
              D_ENV_DB_SUPPORTS_CLUSTERING         |                           \
              D_ENV_DB_SUPPORTS_JSON               |                           \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA       |                           \
              D_ENV_DB_SUPPORTS_PARTITIONING       |                           \
              D_ENV_DB_SUPPORTS_ENCRYPTION         |                           \
              D_ENV_DB_SUPPORTS_SSL_TLS )

    // PostgreSQL detection
    #elif defined(PG_VERSION_NUM)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_POSTGRESQL
        #define D_ENV_DB_NAME            "PostgreSQL"
        #define D_ENV_DB_VERSION_MAJOR   (PG_VERSION_NUM / 10000)
        #define D_ENV_DB_VERSION_MINOR   ((PG_VERSION_NUM / 100) % 100)
        #define D_ENV_DB_VERSION_PATCH   (PG_VERSION_NUM % 100)
        #define D_ENV_DB_VERSION_ID      PG_VERSION_NUM

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL           |        \
                                           D_ENV_DB_CAT_MULTI_MODEL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |                          \
              D_ENV_DB_SUPPORTS_SAVEPOINTS          |                          \
              D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS |                          \
              D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT    |                          \
              D_ENV_DB_SUPPORTS_ACID                |                          \
              D_ENV_DB_SUPPORTS_JOINS               |                          \
              D_ENV_DB_SUPPORTS_SUBQUERIES          |                          \
              D_ENV_DB_SUPPORTS_VIEWS               |                          \
              D_ENV_DB_SUPPORTS_STORED_PROCEDURES   |                          \
              D_ENV_DB_SUPPORTS_TRIGGERS            |                          \
              D_ENV_DB_SUPPORTS_USER_FUNCTIONS      |                          \
              D_ENV_DB_SUPPORTS_INDEXES             |                          \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS  |                          \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |                          \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH    |                          \
              D_ENV_DB_SUPPORTS_REPLICATION         |                          \
              D_ENV_DB_SUPPORTS_SHARDING            |                          \
              D_ENV_DB_SUPPORTS_JSON                |                          \
              D_ENV_DB_SUPPORTS_XML                 |                          \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA        |                          \
              D_ENV_DB_SUPPORTS_PARTITIONING        |                          \
              D_ENV_DB_SUPPORTS_ENCRYPTION          |                          \
              D_ENV_DB_SUPPORTS_SSL_TLS             |                          \
              D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY )

    // SQLite detection
    #elif defined(SQLITE_VERSION_NUMBER)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_SQLITE
        #define D_ENV_DB_NAME            "SQLite"
        #define D_ENV_DB_VERSION_MAJOR   (SQLITE_VERSION_NUMBER / 1000000)
        #define D_ENV_DB_VERSION_MINOR   ((SQLITE_VERSION_NUMBER / 1000) % 1000)
        #define D_ENV_DB_VERSION_PATCH   (SQLITE_VERSION_NUMBER % 1000)
        #define D_ENV_DB_VERSION_ID      SQLITE_VERSION_NUMBER

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL  |          \
                                           D_ENV_DB_CAT_SQL         |          \
                                           D_ENV_DB_CAT_EMBEDDED )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS       |                           \
              D_ENV_DB_SUPPORTS_SAVEPOINTS         |                           \
              D_ENV_DB_SUPPORTS_ACID               |                           \
              D_ENV_DB_SUPPORTS_JOINS              |                           \
              D_ENV_DB_SUPPORTS_SUBQUERIES         |                           \
              D_ENV_DB_SUPPORTS_VIEWS              |                           \
              D_ENV_DB_SUPPORTS_TRIGGERS           |                           \
              D_ENV_DB_SUPPORTS_INDEXES            |                           \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS |                           \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS       |                           \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH   |                           \
              D_ENV_DB_SUPPORTS_JSON               |                           \
              D_ENV_DB_SUPPORTS_PARTITIONING       |                           \
              D_ENV_DB_SUPPORTS_ENCRYPTION )

    // MongoDB detection
    #elif defined(MONGOC_VERSION_S)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MONGODB
        #define D_ENV_DB_NAME            "MongoDB"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS     |                             \
              D_ENV_DB_SUPPORTS_ACID             |                             \
              D_ENV_DB_SUPPORTS_INDEXES          |                             \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH |                             \
              D_ENV_DB_SUPPORTS_REPLICATION      |                             \
              D_ENV_DB_SUPPORTS_SHARDING         |                             \
              D_ENV_DB_SUPPORTS_AUTO_FAILOVER    |                             \
              D_ENV_DB_SUPPORTS_JSON             |                             \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA     |                             \
              D_ENV_DB_SUPPORTS_ENCRYPTION       |                             \
              D_ENV_DB_SUPPORTS_SSL_TLS          |                             \
              D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // Redis detection
    #elif defined(REDIS_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_REDIS
        #define D_ENV_DB_NAME            "Redis"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_KEY_VALUE     |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_IN_MEMORY     |        \
                                           D_ENV_DB_CAT_MULTI_MODEL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS |                                 \
              D_ENV_DB_SUPPORTS_REPLICATION  |                                 \
              D_ENV_DB_SUPPORTS_CLUSTERING   |                                 \
              D_ENV_DB_SUPPORTS_SHARDING     |                                 \
              D_ENV_DB_SUPPORTS_JSON         |                                 \
              D_ENV_DB_SUPPORTS_ENCRYPTION   |                                 \
              D_ENV_DB_SUPPORTS_SSL_TLS )

    // ArangoDB detection
    #elif defined(ARANGODB_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ARANGODB
        #define D_ENV_DB_NAME            "ArangoDB"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_GRAPH         |        \
                                           D_ENV_DB_CAT_KEY_VALUE     |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_MULTI_MODEL   |        \
                                           D_ENV_DB_CAT_DISTRIBUTED )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS       |                           \
              D_ENV_DB_SUPPORTS_ACID               |                           \
              D_ENV_DB_SUPPORTS_JOINS              |                           \
              D_ENV_DB_SUPPORTS_INDEXES            |                           \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS |                           \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH   |                           \
              D_ENV_DB_SUPPORTS_REPLICATION        |                           \
              D_ENV_DB_SUPPORTS_CLUSTERING         |                           \
              D_ENV_DB_SUPPORTS_SHARDING           |                           \
              D_ENV_DB_SUPPORTS_JSON               |                           \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA       |                           \
              D_ENV_DB_SUPPORTS_ENCRYPTION         |                           \
              D_ENV_DB_SUPPORTS_SSL_TLS )

    // Oracle Database detection
    #elif defined(ORACLE_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ORACLE
        #define D_ENV_DB_NAME            "Oracle Database"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL           |        \
                                           D_ENV_DB_CAT_MULTI_MODEL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS       |                           \
              D_ENV_DB_SUPPORTS_SAVEPOINTS         |                           \
              D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT   |                           \
              D_ENV_DB_SUPPORTS_ACID               |                           \
              D_ENV_DB_SUPPORTS_JOINS              |                           \
              D_ENV_DB_SUPPORTS_SUBQUERIES         |                           \
              D_ENV_DB_SUPPORTS_VIEWS              |                           \
              D_ENV_DB_SUPPORTS_STORED_PROCEDURES  |                           \
              D_ENV_DB_SUPPORTS_TRIGGERS           |                           \
              D_ENV_DB_SUPPORTS_USER_FUNCTIONS     |                           \
              D_ENV_DB_SUPPORTS_INDEXES            |                           \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS |                           \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS       |                           \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH   |                           \
              D_ENV_DB_SUPPORTS_REPLICATION        |                           \
              D_ENV_DB_SUPPORTS_CLUSTERING         |                           \
              D_ENV_DB_SUPPORTS_SHARDING           |                           \
              D_ENV_DB_SUPPORTS_JSON               |                           \
              D_ENV_DB_SUPPORTS_XML                |                           \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA       |                           \
              D_ENV_DB_SUPPORTS_PARTITIONING       |                           \
              D_ENV_DB_SUPPORTS_ENCRYPTION         |                           \
              D_ENV_DB_SUPPORTS_SSL_TLS            |                           \
              D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY |                           \
              D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // Microsoft SQL Server detection
    #elif defined(_MSSQL_VER)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MSSQL
        #define D_ENV_DB_NAME            "Microsoft SQL Server"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS        |                          \
              D_ENV_DB_SUPPORTS_SAVEPOINTS          |                          \
              D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS |                          \
              D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT    |                          \
              D_ENV_DB_SUPPORTS_ACID                |                          \
              D_ENV_DB_SUPPORTS_JOINS               |                          \
              D_ENV_DB_SUPPORTS_SUBQUERIES          |                          \
              D_ENV_DB_SUPPORTS_VIEWS               |                          \
              D_ENV_DB_SUPPORTS_STORED_PROCEDURES   |                          \
              D_ENV_DB_SUPPORTS_TRIGGERS            |                          \
              D_ENV_DB_SUPPORTS_USER_FUNCTIONS      |                          \
              D_ENV_DB_SUPPORTS_INDEXES             |                          \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS  |                          \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS        |                          \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH    |                          \
              D_ENV_DB_SUPPORTS_REPLICATION         |                          \
              D_ENV_DB_SUPPORTS_CLUSTERING          |                          \
              D_ENV_DB_SUPPORTS_JSON                |                          \
              D_ENV_DB_SUPPORTS_XML                 |                          \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA        |                          \
              D_ENV_DB_SUPPORTS_PARTITIONING        |                          \
              D_ENV_DB_SUPPORTS_ENCRYPTION          |                          \
              D_ENV_DB_SUPPORTS_SSL_TLS             |                          \
              D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY  |                          \
              D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // IBM DB2 detection
    #elif defined(DB2_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_DB2
        #define D_ENV_DB_NAME            "IBM DB2"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS       |                           \
              D_ENV_DB_SUPPORTS_SAVEPOINTS         |                           \
              D_ENV_DB_SUPPORTS_TWO_PHASE_COMMIT   |                           \
              D_ENV_DB_SUPPORTS_ACID               |                           \
              D_ENV_DB_SUPPORTS_JOINS              |                           \
              D_ENV_DB_SUPPORTS_SUBQUERIES         |                           \
              D_ENV_DB_SUPPORTS_VIEWS              |                           \
              D_ENV_DB_SUPPORTS_STORED_PROCEDURES  |                           \
              D_ENV_DB_SUPPORTS_TRIGGERS           |                           \
              D_ENV_DB_SUPPORTS_USER_FUNCTIONS     |                           \
              D_ENV_DB_SUPPORTS_INDEXES            |                           \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS |                           \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS       |                           \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH   |                           \
              D_ENV_DB_SUPPORTS_REPLICATION        |                           \
              D_ENV_DB_SUPPORTS_CLUSTERING         |                           \
              D_ENV_DB_SUPPORTS_JSON               |                           \
              D_ENV_DB_SUPPORTS_XML                |                           \
              D_ENV_DB_SUPPORTS_SPATIAL_DATA       |                           \
              D_ENV_DB_SUPPORTS_PARTITIONING       |                           \
              D_ENV_DB_SUPPORTS_ENCRYPTION         |                           \
              D_ENV_DB_SUPPORTS_SSL_TLS            |                           \
              D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // Firebase detection
    #elif defined(FIREBASE_VERSION_MAJOR)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_FIREBASE
        #define D_ENV_DB_NAME            "Firebase"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_CLOUD_NATIVE )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_INDEXES     |                                  \
              D_ENV_DB_SUPPORTS_REPLICATION |                                  \
              D_ENV_DB_SUPPORTS_JSON        |                                  \
              D_ENV_DB_SUPPORTS_ENCRYPTION  |                                  \
              D_ENV_DB_SUPPORTS_SSL_TLS     |                                  \
              D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY )

    // Apache Cassandra detection
    #elif defined(CASSANDRA_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_CASSANDRA
        #define D_ENV_DB_NAME            "Apache Cassandra"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_COLUMN_FAMILY |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_INDEXES       |                                \
              D_ENV_DB_SUPPORTS_REPLICATION   |                                \
              D_ENV_DB_SUPPORTS_CLUSTERING    |                                \
              D_ENV_DB_SUPPORTS_SHARDING      |                                \
              D_ENV_DB_SUPPORTS_AUTO_FAILOVER |                                \
              D_ENV_DB_SUPPORTS_JSON          |                                \
              D_ENV_DB_SUPPORTS_PARTITIONING  |                                \
              D_ENV_DB_SUPPORTS_ENCRYPTION    |                                \
              D_ENV_DB_SUPPORTS_SSL_TLS )

    // CouchDB detection
    #elif defined(COUCHDB_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_COUCHDB
        #define D_ENV_DB_NAME            "Apache CouchDB"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_ACID             |                             \
              D_ENV_DB_SUPPORTS_VIEWS            |                             \
              D_ENV_DB_SUPPORTS_INDEXES          |                             \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH |                             \
              D_ENV_DB_SUPPORTS_REPLICATION      |                             \
              D_ENV_DB_SUPPORTS_CLUSTERING       |                             \
              D_ENV_DB_SUPPORTS_JSON             |                             \
              D_ENV_DB_SUPPORTS_ENCRYPTION       |                             \
              D_ENV_DB_SUPPORTS_SSL_TLS )

    // Neo4j detection
    #elif defined(NEO4J_VERSION)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_NEO4J
        #define D_ENV_DB_NAME            "Neo4j"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_GRAPH         |        \
                                           D_ENV_DB_CAT_NOSQL )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS       |                           \
              D_ENV_DB_SUPPORTS_ACID               |                           \
              D_ENV_DB_SUPPORTS_INDEXES            |                           \
              D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS |                           \
              D_ENV_DB_SUPPORTS_FULL_TEXT_SEARCH   |                           \
              D_ENV_DB_SUPPORTS_REPLICATION        |                           \
              D_ENV_DB_SUPPORTS_CLUSTERING         |                           \
              D_ENV_DB_SUPPORTS_SHARDING           |                           \
              D_ENV_DB_SUPPORTS_ENCRYPTION         |                           \
              D_ENV_DB_SUPPORTS_SSL_TLS            |                           \
              D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY |                           \
              D_ENV_DB_SUPPORTS_AUDIT_LOGGING )

    // Amazon DynamoDB detection
    //   DynamoDB is a managed cloud service with no installable server and
    // no server version macro; detection keys off the AWS SDK for C++
    // DynamoDB client being in scope (AWS_SDK_VERSION_MAJOR).
    #elif ( (defined(AWS_SDK_VERSION_MAJOR)) &&                                \
            (defined(D_ENV_DYNAMODB_DETECTED)) )
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_DYNAMODB
        #define D_ENV_DB_NAME            "Amazon DynamoDB"

        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_KEY_VALUE     |        \
                                           D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED   |        \
                                           D_ENV_DB_CAT_CLOUD_NATIVE )

        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |                                \
              D_ENV_DB_SUPPORTS_INDEXES       |                                \
              D_ENV_DB_SUPPORTS_REPLICATION   |                                \
              D_ENV_DB_SUPPORTS_SHARDING      |                                \
              D_ENV_DB_SUPPORTS_PARTITIONING  |                                \
              D_ENV_DB_SUPPORTS_AUTO_FAILOVER |                                \
              D_ENV_DB_SUPPORTS_JSON          |                                \
              D_ENV_DB_SUPPORTS_ENCRYPTION    |                                \
              D_ENV_DB_SUPPORTS_SSL_TLS )

    // unknown/no database detected
    #else
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_UNKNOWN
        #define D_ENV_DB_NAME            "Unknown"
        #define D_ENV_DB_CATEGORY        D_ENV_DB_CAT_UNKNOWN
        #define D_ENV_DB_FEATURES        0

    #endif  // database detection

#else
    // manual detection using pre-defined variables
    #if defined(D_ENV_DB_DETECTED_MARIADB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MARIADB
        #define D_ENV_DB_NAME            "MariaDB"
        #define D_ENV_DB_CATEGORY                                              \
            ( D_ENV_DB_CAT_RELATIONAL |                                        \
              D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS |                                 \
              D_ENV_DB_SUPPORTS_ACID         |                                 \
              D_ENV_DB_SUPPORTS_JOINS        |                                 \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS |                                 \
              D_ENV_DB_SUPPORTS_REPLICATION )

    #elif defined(D_ENV_DB_DETECTED_MYSQL)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MYSQL
        #define D_ENV_DB_NAME            "MySQL"
        #define D_ENV_DB_CATEGORY                                              \
            ( D_ENV_DB_CAT_RELATIONAL |                                        \
              D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS |                                 \
              D_ENV_DB_SUPPORTS_ACID         |                                 \
              D_ENV_DB_SUPPORTS_JOINS        |                                 \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS |                                 \
              D_ENV_DB_SUPPORTS_REPLICATION )

    #elif defined(D_ENV_DB_DETECTED_POSTGRESQL)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_POSTGRESQL
        #define D_ENV_DB_NAME            "PostgreSQL"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL           |        \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS |                                 \
              D_ENV_DB_SUPPORTS_ACID         |                                 \
              D_ENV_DB_SUPPORTS_JOINS        |                                 \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS |                                 \
              D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_SQLITE)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_SQLITE
        #define D_ENV_DB_NAME            "SQLite"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL  |          \
                                           D_ENV_DB_CAT_SQL         |          \
                                           D_ENV_DB_CAT_EMBEDDED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |   \
                                           D_ENV_DB_SUPPORTS_ACID          |   \
                                           D_ENV_DB_SUPPORTS_JOINS         |   \
                                           D_ENV_DB_SUPPORTS_FOREIGN_KEYS )

    #elif defined(D_ENV_DB_DETECTED_MONGODB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MONGODB
        #define D_ENV_DB_NAME            "MongoDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |   \
                                           D_ENV_DB_SUPPORTS_REPLICATION   |   \
                                           D_ENV_DB_SUPPORTS_SHARDING      |   \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_REDIS)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_REDIS
        #define D_ENV_DB_NAME            "Redis"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_KEY_VALUE     |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_IN_MEMORY )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |   \
                                           D_ENV_DB_SUPPORTS_REPLICATION   |   \
                                           D_ENV_DB_SUPPORTS_CLUSTERING )

    #elif defined(D_ENV_DB_DETECTED_ARANGODB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ARANGODB
        #define D_ENV_DB_NAME            "ArangoDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_GRAPH         |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |   \
                                           D_ENV_DB_SUPPORTS_ACID          |   \
                                           D_ENV_DB_SUPPORTS_JOINS         |   \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_ORACLE)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_ORACLE
        #define D_ENV_DB_NAME            "Oracle Database"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_RELATIONAL    |        \
                                           D_ENV_DB_CAT_SQL           |        \
                                           D_ENV_DB_CAT_MULTI_MODEL )
        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS |                                 \
              D_ENV_DB_SUPPORTS_ACID         |                                 \
              D_ENV_DB_SUPPORTS_JOINS        |                                 \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS |                                 \
              D_ENV_DB_SUPPORTS_PARTITIONING )

    #elif defined(D_ENV_DB_DETECTED_MSSQL)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_MSSQL
        #define D_ENV_DB_NAME            "Microsoft SQL Server"
        #define D_ENV_DB_CATEGORY                                              \
            ( D_ENV_DB_CAT_RELATIONAL |                                        \
              D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS |                                 \
              D_ENV_DB_SUPPORTS_ACID         |                                 \
              D_ENV_DB_SUPPORTS_JOINS        |                                 \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS |                                 \
              D_ENV_DB_SUPPORTS_PARTITIONING )

    #elif defined(D_ENV_DB_DETECTED_DB2)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_DB2
        #define D_ENV_DB_NAME            "IBM DB2"
        #define D_ENV_DB_CATEGORY                                              \
            ( D_ENV_DB_CAT_RELATIONAL |                                        \
              D_ENV_DB_CAT_SQL )
        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_TRANSACTIONS |                                 \
              D_ENV_DB_SUPPORTS_ACID         |                                 \
              D_ENV_DB_SUPPORTS_JOINS        |                                 \
              D_ENV_DB_SUPPORTS_FOREIGN_KEYS |                                 \
              D_ENV_DB_SUPPORTS_PARTITIONING )

    #elif defined(D_ENV_DB_DETECTED_FIREBASE)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_FIREBASE
        #define D_ENV_DB_NAME            "Firebase"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_CLOUD_NATIVE )
        #define D_ENV_DB_FEATURES                                              \
            ( D_ENV_DB_SUPPORTS_JSON        |                                  \
              D_ENV_DB_SUPPORTS_REPLICATION |                                  \
              D_ENV_DB_SUPPORTS_ROW_LEVEL_SECURITY )

    #elif defined(D_ENV_DB_DETECTED_CASSANDRA)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_CASSANDRA
        #define D_ENV_DB_NAME            "Apache Cassandra"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_COLUMN_FAMILY |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_REPLICATION   |   \
                                           D_ENV_DB_SUPPORTS_SHARDING      |   \
                                           D_ENV_DB_SUPPORTS_AUTO_FAILOVER )

    #elif defined(D_ENV_DB_DETECTED_COUCHDB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_COUCHDB
        #define D_ENV_DB_NAME            "Apache CouchDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_ACID          |   \
                                           D_ENV_DB_SUPPORTS_REPLICATION   |   \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_NEO4J)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_NEO4J
        #define D_ENV_DB_NAME            "Neo4j"
        #define D_ENV_DB_CATEGORY                                              \
            ( D_ENV_DB_CAT_GRAPH |                                             \
              D_ENV_DB_CAT_NOSQL )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |   \
                                           D_ENV_DB_SUPPORTS_ACID          |   \
                                           D_ENV_DB_SUPPORTS_CLUSTERING )

    #elif defined(D_ENV_DB_DETECTED_DYNAMODB)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_DYNAMODB
        #define D_ENV_DB_NAME            "Amazon DynamoDB"
        #define D_ENV_DB_CATEGORY        ( D_ENV_DB_CAT_KEY_VALUE     |        \
                                           D_ENV_DB_CAT_DOCUMENT      |        \
                                           D_ENV_DB_CAT_NOSQL         |        \
                                           D_ENV_DB_CAT_DISTRIBUTED   |        \
                                           D_ENV_DB_CAT_CLOUD_NATIVE )
        #define D_ENV_DB_FEATURES        ( D_ENV_DB_SUPPORTS_TRANSACTIONS  |   \
                                           D_ENV_DB_SUPPORTS_REPLICATION   |   \
                                           D_ENV_DB_SUPPORTS_SHARDING      |   \
                                           D_ENV_DB_SUPPORTS_PARTITIONING  |   \
                                           D_ENV_DB_SUPPORTS_JSON )

    #elif defined(D_ENV_DB_DETECTED_UNKNOWN)
        #define D_ENV_DB_ID              D_ENV_DB_FLAG_UNKNOWN
        #define D_ENV_DB_NAME            "Unknown"
        #define D_ENV_DB_CATEGORY        D_ENV_DB_CAT_UNKNOWN
        #define D_ENV_DB_FEATURES        0

    #endif  // manual detection

#endif  // D_CFG_ENV_DB_CUSTOM


//==============================================================================
// 5.  CONVENIENCE MACROS
//==============================================================================


// 5.1    Queries
//------------------------------------------------------------------------------
// 5.1.1
// D_ENV_DB_HAS_FEATURE
//   macro: checks if the detected database supports a specific feature.
#define D_ENV_DB_HAS_FEATURE(feature)                                          \
    ((D_ENV_DB_FEATURES) & (feature))

// 5.1.2
// D_ENV_DB_IS_CATEGORY
//   macro: checks if the detected database belongs to a specific category.
#define D_ENV_DB_IS_CATEGORY(category)                                         \
    ((D_ENV_DB_CATEGORY) & (category))

// 5.1.3
// D_ENV_DB_IS_DBMS
//   macro: checks if the detected database is any type of database management
// system (RDBMS or NoSQL).
#define D_ENV_DB_IS_DBMS                                                       \
    (D_ENV_DB_ID != D_ENV_DB_FLAG_UNKNOWN)

// 5.2    Version comparisons
//------------------------------------------------------------------------------
#ifdef D_ENV_DB_VERSION_ID
    // 5.2.1
    // D_ENV_DB_VERSION_AT_LEAST
    //   macro: 1 if the version is at least major.minor.patch; defined only
    // when the detected database reports a version.
    #define D_ENV_DB_VERSION_AT_LEAST(major, minor, patch)                     \
        (D_ENV_DB_VERSION_ID >= ((major) * 10000 + (minor) * 100 + (patch)))

    // 5.2.2
    // D_ENV_DB_VERSION_BELOW
    //   macro: 1 if the version is below major.minor.patch; defined only
    // when the detected database reports a version.
    #define D_ENV_DB_VERSION_BELOW(major, minor, patch)                        \
        (D_ENV_DB_VERSION_ID < ((major) * 10000 + (minor) * 100 + (patch)))
#endif  // D_ENV_DB_VERSION_ID


//==============================================================================
// 6.  FEATURE COMBINATION SHORTCUTS
//==============================================================================


// 6.1    Feature combinations
//------------------------------------------------------------------------------
// 6.1.1
// D_ENV_DB_IS_FULLY_ACID
//   macro: checks if database supports all ACID properties.
#define D_ENV_DB_IS_FULLY_ACID                                                 \
    ( (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_ATOMICITY))   &&                 \
      (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_CONSISTENCY)) &&                 \
      (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_ISOLATION))   &&                 \
      (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_DURABILITY)) )

// 6.1.2
// D_ENV_DB_SUPPORTS_ADVANCED_TRANSACTIONS
//   macro: checks if database supports advanced transaction features.
#define D_ENV_DB_SUPPORTS_ADVANCED_TRANSACTIONS                                \
    ( (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_TRANSACTIONS)) &&                \
      ( (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_SAVEPOINTS)) ||                \
        (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_NESTED_TRANSACTIONS)) ) )

// 6.1.3
// D_ENV_DB_SUPPORTS_RELATIONAL_INTEGRITY
//   macro: checks if database supports relational integrity constraints.
#define D_ENV_DB_SUPPORTS_RELATIONAL_INTEGRITY                                 \
    ( (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_UNIQUE_CONSTRAINTS)) &&          \
      (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_FOREIGN_KEYS)) )

// 6.1.4
// D_ENV_DB_SUPPORTS_HIGH_AVAILABILITY
//   macro: checks if database supports high availability features.
#define D_ENV_DB_SUPPORTS_HIGH_AVAILABILITY                                    \
    ( (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_REPLICATION)) &&                 \
      ( (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_CLUSTERING)) ||                \
        (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_AUTO_FAILOVER)) ) )

// 6.1.5
// D_ENV_DB_SUPPORTS_SCALE_OUT
//   macro: checks if database supports horizontal scaling.
#define D_ENV_DB_SUPPORTS_SCALE_OUT                                            \
    ( (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_SHARDING)) ||                    \
      (D_ENV_DB_HAS_FEATURE(D_ENV_DB_SUPPORTS_PARTITIONING)) )


#endif  // DJINTERP_ENV_DB_ENV_DB_H
