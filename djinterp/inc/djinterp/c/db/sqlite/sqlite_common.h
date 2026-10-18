/*******************************************************************************
* djinterp [c]                                                   sqlite_common.h
*
* Shared foundation of djinterp's SQLite convenience layer.
*   The layer is a set of plain functions over SQLite's own connection handle,
* sqlite3*, so that it mixes freely with SQLite's API: open with
* d_sqlite_open or sqlite3_open_v2, prepare statements natively, and reach
* for d_sqlite_has_table or d_sqlite_create_table where they save the
* boilerplate. Every module includes this header:
*     sqlite_connection   open, close, path, read-only, row ids, changes
*     sqlite_query        exec, scalar queries, rows, quoting
*     sqlite_statement    prepared statements: bind, execute, finalize
*     sqlite_database     attached databases, database files
*     sqlite_schema       tables, columns, indexes, views, triggers
*     sqlite_transaction  begin, commit, rollback, savepoints
*     sqlite_settings     foreign keys, journal mode, synchronous, versions
*     sqlite_maintenance  vacuum, analyze, checks, checkpoints, backups
*   RESULTS. A function that can fail returns SQLITE_OK (0) or the SQLite
* result code of the failure, which sqlite3_errmsg on the same connection then
* describes; an argument that cannot be right -- a NULL handle, an empty name
* -- is SQLITE_MISUSE. The few conditions SQLite has no code for are
* db_common.h's: D_DB_UNSUPPORTED when the SQLite library in use is too old
* for the request, D_DB_EXISTS and D_DB_NOT_FOUND. Predicates come in pairs,
* as db_common.h describes: `has_x` folds a failure into `false`, `has_x_s`
* reports it.
*   NAMES. Every name -- schema, table, column -- is quoted by the layer, so
* any string SQLite accepts as a name is safe to pass. A NULL schema means
* "main". Text that is SQL -- a column definition, a SELECT, a DEFAULT -- is
* the caller's SQL and is used as written.
*   VERSIONS. The floor is SQLite 3.7.17. C API calls newer than that are
* chosen when the layer is compiled, from the version env_sqlite.h detects
* (which a build may set by hand, to target an older shared library); SQL
* newer than that is checked when it runs, against the library actually
* loaded. Public headers never include sqlite3.h, which ISO C++98 rejects;
* include it yourself to call SQLite directly.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Handles
         1.  sqlite3
         2.  sqlite3_stmt
    2.  Features
         1.  d_sqlite_feature
2.  LIBRARY
    -------
    1.  Versions
    2.  Features
    3.  Result messages
3.  CONNECTIONS
    -----------
    1.  Errors
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_COMMON_H
#define DJINTERP_C_DB_SQLITE_SQLITE_COMMON_H 1

// djinterp
#include "../../djinterp.h"                                // framework root
#include "../db_common.h"                                  // the vocabulary
#include "../../../config/c/db/sqlite/cfg_sqlite_common.h"  // layer config


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Handles
//------------------------------------------------------------------------------
// 1.1.1
// sqlite3
//   struct: SQLite's connection object, declared and never defined here, as
// sqlite3.h declares it. A sqlite3* from SQLite passes to every function in
// this layer unchanged.
struct sqlite3;

// 1.1.2
// sqlite3_stmt
//   struct: SQLite's prepared statement, declared as sqlite3.h declares it. A
// statement from d_sqlite_prepare is SQLite's own, and one prepared natively
// passes to sqlite_statement.h's functions unchanged.
struct sqlite3_stmt;

// 1.2    Features
//------------------------------------------------------------------------------
// 1.2.1
// d_sqlite_feature
//   enum: capabilities that depend on the SQLite library in use, by its
// version or by how it was built. Each names, in brackets, the release that
// introduced it or the build option that provides it.
enum d_sqlite_feature
{
    D_SQLITE_FEATURE_PARTIAL_INDEX = 0,  // CREATE INDEX ... WHERE  [3.8.0]
    D_SQLITE_FEATURE_WITHOUT_ROWID,      // WITHOUT ROWID tables    [3.8.2]
    D_SQLITE_FEATURE_CTE,                // WITH and WITH RECURSIVE [3.8.3]
    D_SQLITE_FEATURE_EXPRESSION_INDEX,   // CREATE INDEX ... (f(x)) [3.9.0]
    D_SQLITE_FEATURE_ROW_VALUES,         // (a, b) = (1, 2)         [3.15.0]
    D_SQLITE_FEATURE_VACUUM_SCHEMA,      // VACUUM <schema>         [3.15.0]
    D_SQLITE_FEATURE_OPTIMIZE,           // PRAGMA optimize         [3.18.0]
    D_SQLITE_FEATURE_UPSERT,             // ON CONFLICT DO UPDATE   [3.24.0]
    D_SQLITE_FEATURE_WINDOW_FUNCTIONS,   // OVER (...)              [3.25.0]
    D_SQLITE_FEATURE_RENAME_COLUMN,      // ALTER ... RENAME COLUMN [3.25.0]
    D_SQLITE_FEATURE_VACUUM_INTO,        // VACUUM INTO <file>      [3.27.0]
    D_SQLITE_FEATURE_GENERATED_COLUMNS,  // GENERATED ALWAYS AS     [3.31.0]
    D_SQLITE_FEATURE_DROP_COLUMN,        // ALTER ... DROP COLUMN   [3.35.0]
    D_SQLITE_FEATURE_RETURNING,          // ... RETURNING           [3.35.0]
    D_SQLITE_FEATURE_STRICT_TABLES,      // STRICT tables           [3.37.0]
    D_SQLITE_FEATURE_JSON,               // json() and kin  [3.38.0, or JSON1]
    D_SQLITE_FEATURE_MATH_FUNCTIONS,     // sin(), ln(), ...  [3.35.0 + MATH]
    D_SQLITE_FEATURE_FTS5,               // full-text search 5        [FTS5]
    D_SQLITE_FEATURE_RTREE               // R*Tree index             [RTREE]
};


//==============================================================================
// 2.  LIBRARY
//==============================================================================
// These describe the library, not a connection, and none of them can fail.


// 2.1    Versions
//------------------------------------------------------------------------------
// versions -- the "library" is the one loaded at run time, the "header" the
// one the layer was compiled against; numbers are MAJOR*1000000 + MINOR*1000
// + PATCH, as SQLITE_VERSION_NUMBER encodes them
const char* d_sqlite_library_version(void) D_NOTHROW;
int         d_sqlite_library_version_number(void) D_NOTHROW;
int         d_sqlite_header_version_number(void) D_NOTHROW;
bool        d_sqlite_library_at_least(int _major,
                                      int _minor,
                                      int _patch) D_NOTHROW;

// 2.2    Features
//------------------------------------------------------------------------------
// features -- whether the library loaded now can do it; `false` for a value
// outside enum d_sqlite_feature
bool        d_sqlite_supports(enum d_sqlite_feature _feature) D_NOTHROW;

// 2.3    Result messages
//------------------------------------------------------------------------------
// result messages -- an English description of any result this layer
// returns, SQLite's own included; never NULL, and never to be freed
const char* d_sqlite_result_message(int _result) D_NOTHROW;


//==============================================================================
// 3.  CONNECTIONS
//==============================================================================


// 3.1    Errors
//------------------------------------------------------------------------------
// errors -- the message SQLite keeps for the connection's most recent
// failure, as sqlite3_errmsg words it ("no such table: users"), where
// d_sqlite_result_message words the result code alone; "not an error" when
// the latest call succeeded. NULL for a NULL connection, or in a build
// without SQLite. Valid until the next call on the connection; never to be
// freed.
const char* d_sqlite_error_message(struct sqlite3* _db) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_SQLITE_SQLITE_COMMON_H
