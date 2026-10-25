/*******************************************************************************
* djinterp [c]                                                 postgres_common.h
*
* Shared foundation of djinterp's PostgreSQL convenience layer.
*   The layer works directly on libpq's connection, PGconn -- which libpq
* defines as struct pg_conn, the name the public headers use -- so a program
* passes the connection it already has to any function here, mixes them
* freely with libpq's own calls, and closes it with PQfinish or
* d_postgres_close alike. Nothing is kept beside the connection: every
* answer comes from libpq or the server. Every module includes this header:
*     postgres_connection   connect, close, ping, the backend's process id
*     postgres_query        exec, rows changed, scalar queries, rows,
*                           quoting
*     postgres_statement    prepared statements: bind, execute, finalize
*     postgres_database     databases, schemas, extensions
*     postgres_schema       tables, columns, indexes, views, triggers,
*                           sequences
*     postgres_transaction  begin, commit, rollback, savepoints
*     postgres_settings     search path, timeouts, isolation, encoding,
*                           time zone, any parameter
*     postgres_maintenance  vacuum, analyze, reindex, checkpoint, sizes
*   RESULTS. A function that can fail returns 0 or, when the server or libpq
* reports a failure, its SQLSTATE -- the five-character code PostgreSQL gives
* every error -- packed into an int the way the server itself packs it
* (D_POSTGRES_SQLSTATE below), so that switch statements can name it and
* d_postgres_result_sqlstate can spell it out again: 42P01 is a missing
* table, 23505 a duplicate key. libpq gives no SQLSTATE for failures of its
* own, so the layer gives them: 08001 when a connection cannot be made, 08006
* when one is lost, 08000 when one cannot run a statement, 53200 when memory
* runs out. d_postgres_error_message holds the words. An argument that cannot
* be right is db_common.h's D_DB_INVALID_ARGUMENT; D_DB_UNSUPPORTED is a
* server too old for the request, or a build without libpq; D_DB_NOT_FOUND
* is as db_common.h describes. Predicates come in pairs, as there: `has_x`
* folds a failure into `false`, `has_x_s` reports it.
*   NAMES. Every name -- schema, table, column -- is quoted by the layer, so
* any name the server accepts is safe to pass, and it is matched exactly, case
* included: a table created as CREATE TABLE Users, unquoted, is named users.
* A NULL schema means the current schema, the first schema of the search path
* that exists, as current_schema() names it; "pg_temp" means the session's
* own temporary schema, as it does in SQL. Text that is SQL -- a definition,
* a SELECT, a DEFAULT -- is the caller's SQL and is used as written.
*   TRANSACTIONS. A statement that fails inside a transaction leaves it
* failed, as in any PostgreSQL client, until the caller rolls it back. The
* layer's own lookups never fail on a missing object, and an IF [NOT] EXISTS
* the server has no clause for is settled under a savepoint, so neither puts
* the caller's transaction at risk.
*   VERSIONS. The layer is written and tested for PostgreSQL 10 and later,
* through libpq 10 and later. What a server can do is read from libpq's copy
* of the server's version, with no round trip, and checked before each
* request that needs it.
*   BUILDS. Without libpq the layer builds as stubs that report
* D_DB_UNSUPPORTED (cfg_postgres_common.h, D_CFG_POSTGRES).
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Connections
         1.  pg_conn
         2.  d_postgres_statement
    2.  Features
         1.  d_postgres_feature
         2.  d_postgres_client_feature
2.  FLAGS
    -----
    1.  Vendor flags
         1.  D_POSTGRES_CASCADE
         2.  D_POSTGRES_CONCURRENTLY
         3.  D_POSTGRES_FORCE
         4.  D_POSTGRES_REPLACE
         5.  D_POSTGRES_FULL
         6.  D_POSTGRES_ANALYZE
3.  RESULTS
    -------
    1.  SQLSTATE codes
         1.  D_POSTGRES_SQLSTATE
    2.  Conditions the layer documents
         1.  d_postgres_sqlstate
    3.  Reading results
4.  LIBRARY
    -------
    1.  Client
5.  SERVERS
    -------
    1.  Versions
    2.  Features
    3.  Reported parameters
6.  CONNECTIONS
    -----------
    1.  Errors
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_COMMON_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_COMMON_H 1

// djinterp
#include "../../djinterp.h"  // framework root
#include "../db_common.h"    // the vocabulary
#include "../../../config/c/db/postgres/cfg_postgres_common.h"  // layer config


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Connections
//------------------------------------------------------------------------------
// 1.1.1
// pg_conn
//   struct: libpq's connection, PGconn, by its struct tag. Declared here and
// never defined; libpq-fe.h declares PGconn as the same type, so a PGconn*
// passes as a struct pg_conn* and back without a cast.
struct pg_conn;

// 1.1.2
// d_postgres_statement
//   struct: a statement prepared on the server, as the layer holds it: its
// connection, its name there, its parameters' types, and the values bound to
// them until it next runs. Declared and never defined here;
// postgres_statement.h's functions take a pointer.
struct d_postgres_statement;

// 1.2    Features
//------------------------------------------------------------------------------
// 1.2.1
// d_postgres_feature
//   enum: capabilities that arrived after the layer's floor, PostgreSQL 10,
// each with the major release that brought it in brackets.
enum d_postgres_feature
{
    D_POSTGRES_FEATURE_PROCEDURES = 0,        // CREATE PROCEDURE, CALL  [11]
    D_POSTGRES_FEATURE_COVERING_INDEXES,      // CREATE INDEX ... INCLUDE
                                              //                         [11]
    D_POSTGRES_FEATURE_GENERATED_COLUMNS,     // GENERATED ALWAYS AS (...)
                                              // STORED                  [12]
    D_POSTGRES_FEATURE_REINDEX_CONCURRENTLY,  // REINDEX CONCURRENTLY    [12]
    D_POSTGRES_FEATURE_DROP_DATABASE_FORCE,   // DROP DATABASE ... WITH
                                              // (FORCE)                 [13]
    D_POSTGRES_FEATURE_TRUSTED_EXTENSIONS,    // extensions a database
                                              // owner may create        [13]
    D_POSTGRES_FEATURE_MULTIRANGES,           // multirange types        [14]
    D_POSTGRES_FEATURE_REPLACE_TRIGGER,       // CREATE OR REPLACE
                                              // TRIGGER                 [14]
    D_POSTGRES_FEATURE_MERGE,                 // MERGE                   [15]
    D_POSTGRES_FEATURE_NULLS_NOT_DISTINCT,    // UNIQUE NULLS NOT
                                              // DISTINCT                [15]
    D_POSTGRES_FEATURE_SQL_JSON,              // JSON_ARRAY, JSON_OBJECT,
                                              // IS JSON                 [16]
    D_POSTGRES_FEATURE_JSON_TABLE,            // JSON_TABLE              [17]
    D_POSTGRES_FEATURE_VIRTUAL_COLUMNS,       // GENERATED ALWAYS AS (...)
                                              // VIRTUAL                 [18]
    D_POSTGRES_FEATURE_UUIDV7,                // uuidv7()                [18]
    D_POSTGRES_FEATURE_RETURNING_OLD_NEW,     // RETURNING old.*, new.*  [18]
    D_POSTGRES_FEATURE_JSONPATH               // SQL/JSON path language,
                                              // jsonb_path_query, @?    [12]
};

// 1.2.2
// d_postgres_client_feature
//   enum: what libpq can do that arrived after the layer's floor, libpq 10,
// each with the release that brought it in brackets. The layer needs both the
// header it was compiled against and the library loaded to have it.
enum d_postgres_client_feature
{
    D_POSTGRES_CLIENT_PIPELINE = 0,   // pipeline mode                   [14]
    D_POSTGRES_CLIENT_CHUNKED_ROWS,   // results read in chunks of rows  [17]
    D_POSTGRES_CLIENT_CLOSE_PREPARED  // a statement closed by the
                                      // protocol's own message          [17]
};


//==============================================================================
// 2.  FLAGS
//==============================================================================
// The layer's own flags, from db_common.h's D_DB_VENDOR_FLAGS up, beside its
// shared ones; each function says which it takes.


// 2.1    Vendor flags
//------------------------------------------------------------------------------
// 2.1.1
// D_POSTGRES_CASCADE
//   constant: a drop takes what depends on the object with it -- a view on a
// table, a schema's contents -- where without it the server refuses
// (D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST); creating an extension
// installs the extensions it needs.
#define D_POSTGRES_CASCADE       (D_DB_VENDOR_FLAGS << 0)

// 2.1.2
// D_POSTGRES_CONCURRENTLY
//   constant: an index is built, dropped or rebuilt without blocking writes to
// its table, which takes longer and cannot run inside a transaction.
#define D_POSTGRES_CONCURRENTLY  (D_DB_VENDOR_FLAGS << 1)

// 2.1.3
// D_POSTGRES_FORCE
//   constant: dropping a database first ends the other sessions connected to
// it, where without it the server refuses (D_POSTGRES_SQLSTATE_OBJECT_IN_USE);
// PostgreSQL 13 and later.
#define D_POSTGRES_FORCE         (D_DB_VENDOR_FLAGS << 2)

// 2.1.4
// D_POSTGRES_REPLACE
//   constant: CREATE OR REPLACE -- a view or trigger of the name is replaced;
// triggers from PostgreSQL 14.
#define D_POSTGRES_REPLACE       (D_DB_VENDOR_FLAGS << 3)

// 2.1.5
// D_POSTGRES_FULL
//   constant: VACUUM FULL -- the table is rewritten under an exclusive lock,
// and its free space returned to the operating system.
#define D_POSTGRES_FULL          (D_DB_VENDOR_FLAGS << 4)

// 2.1.6
// D_POSTGRES_ANALYZE
//   constant: VACUUM ANALYZE -- the planner's statistics are updated as well.
#define D_POSTGRES_ANALYZE       (D_DB_VENDOR_FLAGS << 5)


//==============================================================================
// 3.  RESULTS
//==============================================================================


// 3.1    SQLSTATE codes
//------------------------------------------------------------------------------
// 3.1.1
// D_POSTGRES_SQLSTATE
//   macro: an SQLSTATE's five characters, digits and capital letters, packed
// into an int as the server packs them (its errcodes.h, MAKE_SQLSTATE): six
// bits a character, the first lowest. "00000", success, packs to 0, and
// every other code to a positive int below 2^30. A macro, where a function
// would do, because a result is compared in `case` labels, which need a
// constant expression.
#define D_POSTGRES_SQLSTATE(_c1, _c2, _c3, _c4, _c5)                           \
    ( ( ((_c1) - '0') & 0x3F )         +                                       \
      ( (((_c2) - '0') & 0x3F) << 6 )  +                                       \
      ( (((_c3) - '0') & 0x3F) << 12 ) +                                       \
      ( (((_c4) - '0') & 0x3F) << 18 ) +                                       \
      ( (((_c5) - '0') & 0x3F) << 24 ) )

// 3.2    Conditions the layer documents
//------------------------------------------------------------------------------
// 3.2.1
// d_postgres_sqlstate
//   enum: the SQLSTATEs the layer's headers name, packed, each named as the
// PostgreSQL manual's appendix A names its condition. Any other code the
// server sends arrives packed the same way, and D_POSTGRES_SQLSTATE builds
// its constant. The functions return int, so that every code fits.
enum d_postgres_sqlstate
{
    // class 08: connection exception -- three of the five codes the layer
    // gives itself (53200, out of memory, and 58030, a client's file, are
    // the others), and the server's 08P01
    D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION =
        D_POSTGRES_SQLSTATE('0', '8', '0', '0', '0'),  // a connection that
                                                       // cannot run a
                                                       // statement
    D_POSTGRES_SQLSTATE_CANNOT_CONNECT =
        D_POSTGRES_SQLSTATE('0', '8', '0', '0', '1'),  // no connection could
                                                       // be made
    D_POSTGRES_SQLSTATE_CONNECTION_FAILURE =
        D_POSTGRES_SQLSTATE('0', '8', '0', '0', '6'),  // the connection was
                                                       // lost
    D_POSTGRES_SQLSTATE_PROTOCOL_VIOLATION =
        D_POSTGRES_SQLSTATE('0', '8', 'P', '0', '1'),  // values for a
                                                       // statement's
                                                       // parameters that it
                                                       // does not have
    // class 0A, 22, 23: what a statement or its data cannot do
    D_POSTGRES_SQLSTATE_FEATURE_NOT_SUPPORTED =
        D_POSTGRES_SQLSTATE('0', 'A', '0', '0', '0'),
    D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE =
        D_POSTGRES_SQLSTATE('2', '2', '0', '2', '3'),
    D_POSTGRES_SQLSTATE_INVALID_TEXT_REPRESENTATION =
        D_POSTGRES_SQLSTATE('2', '2', 'P', '0', '2'),  // text a type cannot
                                                       // read
    D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION =
        D_POSTGRES_SQLSTATE('2', '3', '5', '0', '5'),
    // class 25: invalid transaction state
    D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION =
        D_POSTGRES_SQLSTATE('2', '5', '0', '0', '1'),  // not inside a
                                                       // transaction
    D_POSTGRES_SQLSTATE_NO_ACTIVE_SQL_TRANSACTION =
        D_POSTGRES_SQLSTATE('2', '5', 'P', '0', '1'),  // only inside one
    D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION =
        D_POSTGRES_SQLSTATE('2', '5', 'P', '0', '2'),  // the transaction
                                                       // failed
    // class 2B, 3B, 3D, 3F: dependents, savepoints, databases, schemas
    D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST =
        D_POSTGRES_SQLSTATE('2', 'B', 'P', '0', '1'),
    D_POSTGRES_SQLSTATE_INVALID_SAVEPOINT_SPECIFICATION =
        D_POSTGRES_SQLSTATE('3', 'B', '0', '0', '1'),
    D_POSTGRES_SQLSTATE_INVALID_CATALOG_NAME =
        D_POSTGRES_SQLSTATE('3', 'D', '0', '0', '0'),  // no such database
    D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME =
        D_POSTGRES_SQLSTATE('3', 'F', '0', '0', '0'),  // no such schema, or
                                                       // no current one
    // class 40: transaction rollback
    D_POSTGRES_SQLSTATE_SERIALIZATION_FAILURE =
        D_POSTGRES_SQLSTATE('4', '0', '0', '0', '1'),
    D_POSTGRES_SQLSTATE_DEADLOCK_DETECTED =
        D_POSTGRES_SQLSTATE('4', '0', 'P', '0', '1'),
    // class 42: syntax error or access rule violation
    D_POSTGRES_SQLSTATE_INSUFFICIENT_PRIVILEGE =
        D_POSTGRES_SQLSTATE('4', '2', '5', '0', '1'),
    D_POSTGRES_SQLSTATE_SYNTAX_ERROR =
        D_POSTGRES_SQLSTATE('4', '2', '6', '0', '1'),
    D_POSTGRES_SQLSTATE_DUPLICATE_COLUMN =
        D_POSTGRES_SQLSTATE('4', '2', '7', '0', '1'),
    D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN =
        D_POSTGRES_SQLSTATE('4', '2', '7', '0', '3'),
    D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT =
        D_POSTGRES_SQLSTATE('4', '2', '7', '0', '4'),  // no such index,
                                                       // trigger, extension
                                                       // or parameter
    D_POSTGRES_SQLSTATE_DUPLICATE_OBJECT =
        D_POSTGRES_SQLSTATE('4', '2', '7', '1', '0'),  // a trigger or
                                                       // extension of that
                                                       // name
    D_POSTGRES_SQLSTATE_UNDEFINED_FUNCTION =
        D_POSTGRES_SQLSTATE('4', '2', '8', '8', '3'),
    D_POSTGRES_SQLSTATE_UNDEFINED_TABLE =
        D_POSTGRES_SQLSTATE('4', '2', 'P', '0', '1'),  // no such table, view
                                                       // or sequence
    D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER =
        D_POSTGRES_SQLSTATE('4', '2', 'P', '0', '2'),  // no parameter of
                                                       // that number
    D_POSTGRES_SQLSTATE_DUPLICATE_DATABASE =
        D_POSTGRES_SQLSTATE('4', '2', 'P', '0', '4'),
    D_POSTGRES_SQLSTATE_DUPLICATE_PREPARED_STATEMENT =
        D_POSTGRES_SQLSTATE('4', '2', 'P', '0', '5'),
    D_POSTGRES_SQLSTATE_DUPLICATE_SCHEMA =
        D_POSTGRES_SQLSTATE('4', '2', 'P', '0', '6'),
    D_POSTGRES_SQLSTATE_DUPLICATE_TABLE =
        D_POSTGRES_SQLSTATE('4', '2', 'P', '0', '7'),  // a relation -- table,
                                                       // view, index,
                                                       // sequence -- of that
                                                       // name
    // class 53, 55, 57, 58, XX: resources, state, intervention, the system,
    // the server itself
    D_POSTGRES_SQLSTATE_OUT_OF_MEMORY =
        D_POSTGRES_SQLSTATE('5', '3', '2', '0', '0'),
    D_POSTGRES_SQLSTATE_OBJECT_IN_USE =
        D_POSTGRES_SQLSTATE('5', '5', '0', '0', '6'),
    D_POSTGRES_SQLSTATE_LOCK_NOT_AVAILABLE =
        D_POSTGRES_SQLSTATE('5', '5', 'P', '0', '3'),
    D_POSTGRES_SQLSTATE_QUERY_CANCELED =
        D_POSTGRES_SQLSTATE('5', '7', '0', '1', '4'),
    D_POSTGRES_SQLSTATE_IO_ERROR =
        D_POSTGRES_SQLSTATE('5', '8', '0', '3', '0'),  // a file that could
                                                       // not be read or
                                                       // written
    D_POSTGRES_SQLSTATE_INTERNAL_ERROR =
        D_POSTGRES_SQLSTATE('X', 'X', '0', '0', '0')
};

// 3.3    Reading results
//------------------------------------------------------------------------------
// reading results -- _result_sqlstate spells a result's SQLSTATE into
// `_sqlstate`, five characters and a terminator, and returns true; for 0 it
// writes "00000", and for a negative result or one no SQLSTATE packs to it
// writes "" and returns false. _result_from_sqlstate packs a five-character
// SQLSTATE, as libpq's PG_DIAG_SQLSTATE gives it; 0 for "00000", and
// D_DB_INVALID_ARGUMENT for NULL or text of any other shape.
bool         d_postgres_result_sqlstate(int  _result,
                                        char _sqlstate[6]) D_NOTHROW;
int          d_postgres_result_from_sqlstate(const char* _sqlstate) D_NOTHROW;
// result messages -- an English description of a result the layer returns:
// db_common.h's own words for its values; the layer's own for the five
// SQLSTATEs it gives itself; the PostgreSQL manual's for the conditions
// section 3.2 names ("undefined table" for 42P01); and for any other
// SQLSTATE, the manual's words for its class ("syntax error or access rule
// violation" for 42P10), since only the connection keeps the message itself
// (d_postgres_error_message). Never NULL, and never to be freed.
const char*  d_postgres_result_message(int _result) D_NOTHROW;


//==============================================================================
// 4.  LIBRARY
//==============================================================================


// 4.1    Client
//------------------------------------------------------------------------------
// client -- the loaded libpq's version, as PQlibVersion gives it (160004 for
// 16.4), and whether the layer can do what libpq added after 10: the header
// the layer was compiled against declares it, and the library loaded is of
// its release or later; 0 and false in a build without libpq, and false for a
// value outside enum d_postgres_client_feature. Neither can fail.
int          d_postgres_client_version_number(void) D_NOTHROW;
bool         d_postgres_client_supports(enum d_postgres_client_feature
                                            _feature) D_NOTHROW;


//==============================================================================
// 5.  SERVERS
//==============================================================================
// What the server said of itself when the connection was made, as libpq keeps
// it: no round trip. A NULL or broken connection has no server: these answer
// NULL, 0 or false.


// 5.1    Versions
//------------------------------------------------------------------------------
// versions -- the server's version string, as it reported it ("16.4",
// "17.2 (Debian 17.2-1.pgdg120+1)"), libpq's copy, valid while the connection
// is; its number, as PQserverVersion gives it (160004 for 16.4); and whether
// it is MAJOR.MINOR or later, PostgreSQL's numbering since 10 having only the
// two parts (16.4: major 16, minor 4)
const char*  d_postgres_server_version(const struct pg_conn* _db) D_NOTHROW;
int          d_postgres_server_version_number(const struct pg_conn* _db)
                                              D_NOTHROW;
bool         d_postgres_server_at_least(const struct pg_conn* _db,
                                        unsigned              _major,
                                        unsigned              _minor) D_NOTHROW;

// 5.2    Features
//------------------------------------------------------------------------------
// features -- whether the server can do it, by its major version; `false` for
// a value outside enum d_postgres_feature
bool         d_postgres_supports(const struct pg_conn*   _db,
                                 enum d_postgres_feature _feature) D_NOTHROW;

// 5.3    Reported parameters
//------------------------------------------------------------------------------
// reported parameters -- a setting the server reports to the client, as it
// did when the connection was made and does whenever the setting changes,
// as PQparameterStatus reads it: server_version, server_encoding,
// client_encoding, DateStyle, IntervalStyle, TimeZone, integer_datetimes,
// standard_conforming_strings, is_superuser, session_authorization,
// application_name, and from 14 default_transaction_read_only and
// in_hot_standby. libpq's copy, valid until the setting next changes; NULL
// for a setting the server does not report, or a NULL argument
const char*  d_postgres_parameter_status(const struct pg_conn* _db,
                                         const char*           _name)
                                         D_NOTHROW;


//==============================================================================
// 6.  CONNECTIONS
//==============================================================================


// 6.1    Errors
//------------------------------------------------------------------------------
// errors -- libpq's message for the last failure on the connection, as
// PQerrorMessage gives it ("ERROR:  relation \"t\" does not exist\n...");
// "" for none, or for NULL. A result of the layer's own, negative, leaves it
// as it was. libpq's buffer, valid until the next call on the connection.
const char*  d_postgres_error_message(const struct pg_conn* _db) D_NOTHROW;
/**
 * @brief Says whether a result is the layer's own: given before anything was
 *        sent on the connection, which then says nothing about it.
 *
 * @note The layer answers on its own for a connection a run holds
 *       (08000: postgres_statement.h 3.3), a transaction already open
 *       (25001), a failed one (25P02), no current schema to create in
 *       (3F000), a parameter a statement does not have (42P02), memory
 *       (53200), and a client's file it could not read or write (58030:
 *       postgres_protocol.h 5.1); the server and libpq give the same codes,
 *       with their own
 *       message. For the layer's own, d_postgres_error_message is about an
 *       older failure, and d_postgres_result_message describes the result;
 *       otherwise the connection's message is this failure's. The answer
 *       holds for the latest call on the connection made on this thread.
 *       Where the compiler has no thread-local storage, every result of the
 *       seven codes is reported as the layer's own.
 *
 * @param[in] _db      the connection the result came from; may be NULL.
 * @param[in] _result  the result.
 * @return `true` for every negative result, for 53200, and for a result of
 *         the other six codes the layer gave on its own; `false` otherwise.
 */
bool         d_postgres_result_is_own(const struct pg_conn* _db,
                                      int                   _result)
                                      D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_COMMON_H
