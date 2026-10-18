/*******************************************************************************
* djinterp [c]                                                  mariadb_common.h
*
* Shared foundation of djinterp's MariaDB/MySQL convenience layer.
*   One layer serves both servers, MariaDB and Oracle's MySQL, through either
* client library, MariaDB Connector/C or libmysqlclient, which share the C
* API. It works through a handle of its own, struct d_mariadb, around the
* client's MYSQL: the two clients disagree on that struct's tag, and mysql.h
* spells `long long`, which ISO C++98 refuses, so a public header can neither
* name the client's type nor include its header. d_mariadb_native hands back
* the MYSQL* for the client's own calls, and d_mariadb_adopt wraps one a
* program set up itself -- for TLS, say. Every module includes this header:
*     mariadb_connection   connect, adopt, close, ping, ids, changes
*     mariadb_query        exec, scripts, scalar queries, rows, quoting
*     mariadb_statement    prepared statements: bind, execute, finalize
*     mariadb_database     databases: has, create, drop, list, use
*     mariadb_schema       tables, columns, indexes, views, triggers
*     mariadb_transaction  begin, commit, rollback, savepoints
*     mariadb_settings     autocommit, isolation, character set, time zone,
*                          SQL mode, lock waits, any session variable
*     mariadb_maintenance  analyze, optimize, check, checksum
*   RESULTS. A function that can fail returns 0 or the client's error number,
* as mysql_errno reports it -- a server error such as 1146 (no such table) or
* a client one such as 2006 (server gone away) -- which d_mariadb_error_message
* then describes. The client has no number for an argument that cannot be
* right, so that is db_common.h's D_DB_INVALID_ARGUMENT; D_DB_UNSUPPORTED is a
* server too old for the request, or a build without a client; D_DB_EXISTS
* and D_DB_NOT_FOUND are as db_common.h describes. Predicates come in pairs,
* as there: `has_x` folds a failure into `false`, `has_x_s` reports it.
*   NAMES. Every name -- database, table, column -- is quoted by the layer, so
* any name the server accepts is safe to pass. A NULL schema means the
* connection's current database. Text that is SQL -- a definition, a SELECT,
* a DEFAULT -- is the caller's SQL and is used as written. Where one server
* has an IF [NOT] EXISTS the other lacks, the layer asks first, so the flags
* mean the same on both.
*   VERSIONS. The client floor is libmysqlclient 5.7 or a MariaDB client; the
* servers written and tested for are MySQL 5.7 and MariaDB 10.3, and later.
* What a server can do is worked out once, when the handle is made, from its
* own version string, and checked before each request that needs it.
*   BUILDS. Without a client library the layer builds as stubs that report
* D_DB_UNSUPPORTED (cfg_mariadb_common.h, D_CFG_MARIADB).
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_common.h
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
         1.  d_mariadb
         2.  d_mariadb_statement
    2.  Servers
         1.  d_mariadb_flavor
    3.  Features
         1.  d_mariadb_feature
2.  LIBRARY
    -------
    1.  Client
    2.  Result messages
3.  SERVERS
    -------
    1.  Versions
    2.  Features
4.  HANDLES
    -------
    1.  The client's handle
    2.  Errors
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_COMMON_H
#define DJINTERP_C_DB_MARIADB_MARIADB_COMMON_H 1

// djinterp
#include "../../djinterp.h"                                  // framework root
#include "../db_common.h"                                    // the vocabulary
#include "../../../config/c/db/mariadb/cfg_mariadb_common.h"  // layer config


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Handles
//------------------------------------------------------------------------------
// 1.1.1
// d_mariadb
//   struct: a connection to a MariaDB or MySQL server, as the layer holds it:
// the client's MYSQL, and what the layer learned of the server when it was
// made. Declared and never defined here; every function takes a pointer.
struct d_mariadb;

// 1.1.2
// d_mariadb_statement
//   struct: a prepared statement, as the layer holds it: the client's
// MYSQL_STMT, and the values bound to its parameters until it next runs.
// Declared and never defined here; mariadb_statement.h's functions take a
// pointer.
struct d_mariadb_statement;

// 1.2    Servers
//------------------------------------------------------------------------------
// 1.2.1
// d_mariadb_flavor
//   enum: which server a connection reached, by its own version string.
enum d_mariadb_flavor
{
    D_MARIADB_FLAVOR_UNKNOWN = 0,  // no connection, or one that did not say
    D_MARIADB_FLAVOR_MARIADB,      // MariaDB
    D_MARIADB_FLAVOR_MYSQL         // Oracle's MySQL, or a fork that does not
                                   // call itself MariaDB
};

// 1.3    Features
//------------------------------------------------------------------------------
// 1.3.1
// d_mariadb_feature
//   enum: capabilities that depend on the server, by its flavor and version.
// Each names, in brackets, the releases that introduced it; "--" for a server
// that has never had it.
enum d_mariadb_feature
{
    D_MARIADB_FEATURE_CTE = 0,              // WITH       [MySQL 8.0.1,
                                            //             MariaDB 10.2.2]
    D_MARIADB_FEATURE_WINDOW_FUNCTIONS,     // OVER (...) [8.0.2, 10.2.0]
    D_MARIADB_FEATURE_CHECK_CONSTRAINTS,    // CHECK, enforced
                                            //            [8.0.16, 10.2.1]
    D_MARIADB_FEATURE_JSON,                 // JSON type  [5.7.8, 10.2.7]
    D_MARIADB_FEATURE_RENAME_COLUMN,        // RENAME COLUMN
                                            //            [8.0.3, 10.5.2]
    D_MARIADB_FEATURE_INVISIBLE_COLUMNS,    // INVISIBLE  [8.0.23, 10.3.3]
    D_MARIADB_FEATURE_EXPRESSION_DEFAULTS,  // DEFAULT (expression)
                                            //            [8.0.13, 10.2.1]
    D_MARIADB_FEATURE_DESCENDING_INDEXES,   // DESC keys  [8.0.1, 10.8.1]
    D_MARIADB_FEATURE_RETURNING,            // INSERT ... RETURNING
                                            //            [--, 10.5.0]
    D_MARIADB_FEATURE_SEQUENCES,            // CREATE SEQUENCE [--, 10.3.0]
    D_MARIADB_FEATURE_SYSTEM_VERSIONING     // WITH SYSTEM VERSIONING
                                            //            [--, 10.3.4]
};


//==============================================================================
// 2.  LIBRARY
//==============================================================================
// These describe the client library, not a connection, and none of them can
// fail.


// 2.1    Client
//------------------------------------------------------------------------------
// client -- the client library's version string and number (MAJOR*10000 +
// MINOR*100 + PATCH, as mysql_get_client_version gives it), and whether it is
// MariaDB's rather than Oracle's; "", 0 and false in a build without one
const char*    d_mariadb_client_version(void) D_NOTHROW;
unsigned long  d_mariadb_client_version_number(void) D_NOTHROW;
bool           d_mariadb_client_is_mariadb(void) D_NOTHROW;

// 2.2    Result messages
//------------------------------------------------------------------------------
// result messages -- an English description of a result the layer returns:
// db_common.h's own words for its values; the layer's own for the three
// client numbers it returns itself, 2000 (a failure the server reported
// without a number), 2008 (out of memory) and 2034 (a parameter the statement
// does not have); and for any other client or server number, a note that
// d_mariadb_error_message describes it, since only the connection keeps that
// text. Never NULL, and never to be freed.
const char*    d_mariadb_result_message(int _result) D_NOTHROW;


//==============================================================================
// 3.  SERVERS
//==============================================================================
// What the server said of itself when the handle was made. A NULL handle has
// no server: these answer D_MARIADB_FLAVOR_UNKNOWN, NULL, 0 or false.


// 3.1    Versions
//------------------------------------------------------------------------------
// versions -- the server's flavor; its version string, as the client keeps
// it (Connector/C drops the "5.5.5-" older MariaDB servers put before their
// version for old clients, and libmysqlclient keeps it); its version number
// (MAJOR*10000 + MINOR*100 + PATCH), MariaDB's true one either way; and
// whether it is of the flavor named, at that version or later
enum d_mariadb_flavor
               d_mariadb_server_flavor(const struct d_mariadb* _db) D_NOTHROW;
const char*    d_mariadb_server_version(const struct d_mariadb* _db) D_NOTHROW;
unsigned long  d_mariadb_server_version_number(const struct d_mariadb* _db)
                                               D_NOTHROW;
bool           d_mariadb_server_at_least(const struct d_mariadb* _db,
                                         enum d_mariadb_flavor   _flavor,
                                         unsigned                _major,
                                         unsigned                _minor,
                                         unsigned                _patch)
                                         D_NOTHROW;

// 3.2    Features
//------------------------------------------------------------------------------
// features -- whether the server can do it; `false` for a value outside enum
// d_mariadb_feature
bool           d_mariadb_supports(const struct d_mariadb* _db,
                                  enum d_mariadb_feature  _feature) D_NOTHROW;


//==============================================================================
// 4.  HANDLES
//==============================================================================


// 4.1    The client's handle
//------------------------------------------------------------------------------
// the client's handle -- the MYSQL* inside, for the client's own calls: cast
// it to MYSQL* where mysql.h is included. It stays the layer's: close the
// handle with d_mariadb_close, never with mysql_close. NULL for NULL.
void*          d_mariadb_native(struct d_mariadb* _db) D_NOTHROW;

// 4.2    Errors
//------------------------------------------------------------------------------
// errors -- the client's last error on the handle: its number (mysql_errno),
// its message (mysql_error, "" for none) and its SQLSTATE (mysql_sqlstate,
// "00000" for none). A result of the layer's own, negative, leaves them as
// they were. A NULL handle answers 0 and "".
unsigned       d_mariadb_error_code(const struct d_mariadb* _db) D_NOTHROW;
const char*    d_mariadb_error_message(const struct d_mariadb* _db) D_NOTHROW;
const char*    d_mariadb_sqlstate(const struct d_mariadb* _db) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_COMMON_H
