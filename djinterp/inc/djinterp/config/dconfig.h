/******************************************************************************
* djinterp [core]                                                    dconfig.h
*
* Project-wide configuration override hub.
*
*   This file is included once per per-module *_config.h, and ONLY when
* D_CFG_CUSTOM is defined at the compiler level:
*     -DD_CFG_CUSTOM=1
*   Without that flag, this file is never parsed and each module falls back
* to the defaults baked into its own *_config.h.
*
*   Any #define placed here takes effect project-wide. Because it is
* included BEFORE per-module defaults, anything set here wins over the
* built-in defaults but loses to compiler -D flags (which is the intended
* precedence order: command-line > dconfig.h > module defaults).
*
*   Two flavors of configuration live here:
*     1. Enable / path flags:     D_CFG_ENV_USING_*   D_CFG_ENV_*_C_PATH
*                                 D_CFG_ENV_*_CPP_PATH
*     2. Detection-mode flags:    D_CFG_ENV_*_CUSTOM
*        Setting a _CUSTOM flag to 1 disables that module's auto-detection
*        and requires you to supply pre-defined D_ENV_*_DETECTED_* vars.
*        Leave these alone unless you are cross-compiling or stubbing.
*
*   Uncomment and edit the lines below to activate overrides. All lines are
* commented out by default; uncommenting is never required for normal use.
*
* path:      /inc/djinterp/config/dconfig.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.10.15
******************************************************************************/

#ifndef DJINTERP_CONFIG_
#define DJINTERP_CONFIG_ 1


// =============================================================================
// ENV - core environment (env_config.h)
// =============================================================================

// #define D_CFG_ENV_CUSTOM               0  // bitfield; see env_config.h


// =============================================================================
// ENV / DB - databases
// =============================================================================

// --- env_db.h (top-level DB dispatcher) ---
// #define D_CFG_ENV_DB_CUSTOM            0

// --- MariaDB ---
// #define D_CFG_ENV_USING_MARIADB        0
// #define D_CFG_ENV_MARIADB_C_PATH       <mysql/mysql.h>
// #define D_CFG_ENV_MARIADB_CPP_PATH     <mariadb/conncpp.hpp>
// #define D_CFG_ENV_MARIADB_CUSTOM       0

// --- MySQL (Oracle) ---
// #define D_CFG_ENV_USING_MYSQL          0
// #define D_CFG_ENV_MYSQL_C_PATH         <mysql/mysql.h>
// #define D_CFG_ENV_MYSQL_CPP_PATH       <mysqlx/xdevapi.h>
// #define D_CFG_ENV_MYSQL_CUSTOM         0

// --- PostgreSQL ---
// #define D_CFG_ENV_USING_POSTGRESQL     0
// #define D_CFG_ENV_POSTGRESQL_C_PATH    <libpq-fe.h>
// #define D_CFG_ENV_POSTGRESQL_CPP_PATH  <pqxx/pqxx>
// #define D_CFG_ENV_PG_CUSTOM            0

// --- SQLite ---
// #define D_CFG_ENV_USING_SQLITE         0
// #define D_CFG_ENV_SQLITE_C_PATH        <sqlite3.h>
// #define D_CFG_ENV_SQLITE_CUSTOM        0

// --- MongoDB ---
// #define D_CFG_ENV_USING_MONGODB        0
// #define D_CFG_ENV_MONGODB_C_PATH       <mongoc/mongoc.h>
// #define D_CFG_ENV_MONGODB_CPP_PATH     <mongocxx/client.hpp>
// #define D_CFG_ENV_MONGO_CUSTOM         0

// --- Oracle Database ---
// #define D_CFG_ENV_USING_ORACLE         0
// #define D_CFG_ENV_ORACLE_C_PATH        <oci.h>
// #define D_CFG_ENV_ORACLE_CPP_PATH      <occi.h>
// #define D_CFG_ENV_ORA_CUSTOM           0

// --- ArangoDB ---
// #define D_CFG_ENV_USING_ARANGODB       0
// #define D_CFG_ENV_ARANGODB_CPP_PATH    <velocypack/vpack.h>
// #define D_CFG_ENV_ARANGO_CUSTOM        0

// --- Redis ---
// #define D_CFG_ENV_USING_REDIS          0
// #define D_CFG_ENV_REDIS_C_PATH         <hiredis/hiredis.h>
// #define D_CFG_ENV_REDIS_CPP_PATH       <sw/redis++/redis++.h>

// --- Microsoft SQL Server ---
// #define D_CFG_ENV_USING_MSSQL          0
// #define D_CFG_ENV_MSSQL_C_PATH         <sql.h>

// --- IBM DB2 ---
// #define D_CFG_ENV_USING_DB2            0
// #define D_CFG_ENV_DB2_C_PATH           <sqlcli1.h>

// --- Cassandra ---
// #define D_CFG_ENV_USING_CASSANDRA      0
// #define D_CFG_ENV_CASSANDRA_C_PATH     <cassandra.h>

// --- CouchDB / Neo4j / Firebase ---
//   (HTTP-only - no canonical client header)
// #define D_CFG_ENV_USING_COUCHDB        0
// #define D_CFG_ENV_USING_NEO4J          0
// #define D_CFG_ENV_USING_FIREBASE       0


#endif  // DJINTERP_CONFIG_
