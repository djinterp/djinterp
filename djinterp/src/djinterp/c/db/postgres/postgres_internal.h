/*******************************************************************************
* djinterp [c]                                               postgres_internal.h
*
* Private to the PostgreSQL layer's sources: libpq-fe.h, the client floor,
* and the plumbing every module repeats -- when the layer is built over
* libpq. Built as stubs (D_INTERNAL_POSTGRES 0), it declares nothing, and env
* includes no libpq-fe.h.
*   libpq-fe.h arrives through env_postgres.h, which honours
* D_CFG_ENV_POSTGRESQL_C_PATH; cfg_postgres_common.h has already turned
* D_CFG_ENV_USING_POSTGRESQL on, for a source that defined
* D_INTERNAL_POSTGRES_SOURCE before its first include. The layer calls
* nothing libpq 9.1 lacked.
*   The plumbing keeps one rule the public contract depends on: after a
* statement fails, nothing else runs on the connection before control returns,
* so PQerrorMessage still describes the failure. A helper that runs more than
* one statement stops at the first that fails.
*   Names reach SQL in double quotes, any double quote doubled; values are
* quoted by libpq, for the connection's encoding; SQL the caller wrote is used
* as written. What a database holds is read from pg_catalog, which, unlike
* information_schema, shows every object whatever the user may do with it,
* and every name in it is matched as the server's own name type reads it --
* truncated, as the server truncates an identifier, to 63 bytes.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  REQUIREMENTS
    ------------
    1.  Client floor
2.  RESULTS
    -------
    1.  Reading results
    2.  Answers of the layer's own
    3.  The connection held
    4.  Waiting
3.  NAMES
    -----
    1.  Checks
    2.  Quoting
    3.  The current schema
    4.  Clauses
4.  STATEMENTS
    ----------
    1.  Running
    2.  Single values
    3.  Rows
         1.  Whole results
         2.  Rows a few at a time
    4.  Settling IF NOT EXISTS
5.  CATALOGUE
    ---------
    1.  Objects
         1.  d_internal_postgres_object
         2.  d_internal_postgres_path
    2.  Lookups
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_INTERNAL_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_INTERNAL_H 1

// std
#include <stdarg.h>  // va_list
#include <stddef.h>  // size_t
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/db/postgres/postgres_common.h"  // layer
#include "../../../../../inc/djinterp/env/db/postgres/env_postgres.h"  // libpq
#include "../db_internal.h"  // d_internal_db_buffer, the builders
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_POSTGRES == 1)


//==============================================================================
// 1.  REQUIREMENTS
//==============================================================================


// 1.1    Client floor
//------------------------------------------------------------------------------
// libpq-fe.h declares no version, so the floor can be checked only where env
// knows one from the build: by its manual detection
// (D_ENV_PG_DETECTED_VERSION), or by pg_config.h, if the build included it.
#if !D_CFG_IS_ON(D_CFG_ENV_USING_POSTGRESQL)
    #error "D_CFG_ENV_USING_POSTGRESQL is off: a layer source defines "        \
           "D_INTERNAL_POSTGRES_SOURCE before its first include"
#elif !D_ENV_POSTGRESQL_HEADER_INCLUDED
    #error "the PostgreSQL layer needs libpq-fe.h: D_CFG_ENV_POSTGRESQL_C_PATH"
#elif D_ENV_PG_DETECTED
    #if (D_ENV_PG_VERSION_ID < D_POSTGRES_FLOOR_CLIENT_VERSION_NUM)
        #error "the PostgreSQL layer needs libpq 10 or later"
    #endif
#endif


//==============================================================================
// 2.  RESULTS
//==============================================================================


// 2.1    Reading results
//------------------------------------------------------------------------------
// reading results -- the layer's result for what libpq handed back: 0 for a
// command, rows -- whole, one row (single-row mode) or a chunk of them
// (libpq 17) -- or an empty query; for a failure, its SQLSTATE, packed; for
// one libpq reports without an SQLSTATE, 08006 when the connection is lost
// and 08000 when it is not; for no result at all, 08006 when the connection
// is lost, 08000 in pipeline mode, where libpq refuses every call that waits
// for its result, and 53200, memory, otherwise
int  d_internal_postgres_result(const PGconn*   _db,
                                const PGresult* _result);
/**
 * @brief Settles a result libpq handed back: ends a COPY it started, and
 *        reads what it comes to.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _result  the result; may be `NULL`; freed, or handed over.
 * @param[out] _out     receives the result on success; `NULL` to free it.
 * @return 0, a failure as d_internal_postgres_result reads it, or
 *         D_DB_UNSUPPORTED for a COPY with the client, which is ended, and
 *         every result after it read, so that the connection is ready for
 *         the next statement.
 * @post On success with `_out`, the caller frees `*_out` with PQclear.
 */
int  d_internal_postgres_take(PGconn*    _db,
                              PGresult*  _result,
                              PGresult** _out);
// the layer's result for how formatting ended: 0, or 53200, or
// D_DB_INVALID_ARGUMENT for a format or value the formatter would not write
int  d_internal_postgres_format_result(enum d_internal_db_format_status _state);
// COPY -- _copying says whether a result's status starts a COPY with the
// client; _end_copy abandons that COPY, and every one the SQL starts after
// it, reading every result to the last, so that the connection takes a
// statement again
bool d_internal_postgres_copying(ExecStatusType _status);
void d_internal_postgres_end_copy(PGconn*        _db,
                                  ExecStatusType _status);
// pipelined -- whether the connection is in pipeline mode, a failure having
// aborted it or not; false wherever the libpq compiled against or loaded has
// no pipelines (d_postgres_client_supports)
bool d_internal_postgres_pipelined(const PGconn* _db);

// 2.2    Answers of the layer's own
//------------------------------------------------------------------------------
// answers of the layer's own -- _answer marks `_result` as given by the
// layer on `_db`, on this thread, before anything was sent, and returns it;
// _heard forgets the mark for `_db` once libpq has answered on it, as
// d_internal_postgres_result does for every result; _answered says whether
// `_result` is the mark for `_db` (d_postgres_result_is_own). Without
// thread-local storage nothing is marked, and _answered says true for the
// four codes the layer may give: 25001, 25P02, 3F000 and 42P02
int  d_internal_postgres_answer(const PGconn* _db,
                                int           _result);
void d_internal_postgres_heard(const PGconn* _db);
bool d_internal_postgres_answered(const PGconn* _db,
                                  int           _result);

// 2.3    The connection held
//------------------------------------------------------------------------------
/**
 * @brief Says whether the connection may run a statement now, before a call
 *        that waits for its result.
 *
 * @param[in] _db  the connection.
 * @return 0; or 08000, as the layer's own answer
 *         (d_internal_postgres_answer), while a run holds the connection:
 *         rows a statement's cursor has not read to their end
 *         (postgres_statement.h 3.3), or the results of a query sent
 *         asynchronously that have not been read (postgres_protocol.h).
 *
 * libpq would otherwise read every pending result away, unseen, before the
 * call -- the cursor's rows among them. Pipeline mode is libpq's to refuse,
 * which it does for every call that waits (08000 as well), so a connection
 * in it is reported ready.
 */
int  d_internal_postgres_ready(const PGconn* _db);

// 2.4    Waiting
//------------------------------------------------------------------------------
// waiting -- the plumbing of the calls that do not block (db_common.h 1.3).
// _now is the clock libpq's blocking connect keeps connect_timeout by:
// PQgetCurrentTimeUSec's microseconds from libpq 17, and time()'s whole
// seconds before it. _pend fills a pending call -- the socket, what to wait
// for, and, where it has a deadline, D_DB_WAIT_TIMEOUT and the milliseconds
// left, rounded up, so that a program that wakes then finds it passed -- and
// returns D_DB_WAIT; _settled leaves one as a finished call leaves it. A send
// runs in libpq's non-blocking mode, which _nonblocking_on turns on where it
// is off, answering whether it did, so that the send turns it off again once
// everything is written: the layer's other calls -- COPY's among them -- need
// it off, and turning it off then clears no failure's message, as libpq 15
// and later would once every result is read.
double d_internal_postgres_now(void);
int    d_internal_postgres_pend(int                  _socket,
                                unsigned             _events,
                                struct d_db_pending* _pending);
void   d_internal_postgres_settled(struct d_db_pending* _pending);
bool   d_internal_postgres_nonblocking_on(PGconn* _db);
/**
 * @brief Writes what libpq holds of a send, as far as the socket takes it.
 *
 * @param[in]     _db         the connection, in non-blocking mode.
 * @param[in]     _events     what came: with the socket readable, what the
 *                            server sent is read first, as libpq asks, since
 *                            a server can stop reading while its own
 *                            writing waits.
 * @param[in]     _turned_on  whether the send turned non-blocking mode on,
 *                            and so turns it off once everything is written,
 *                            or the send fails.
 * @param[in,out] _pending    receives what to wait for while some is left:
 *                            the socket, readable or writable.
 * @return 0, everything written; D_DB_WAIT; or, for trouble libpq reports,
 *         08006 when the connection is lost and 08000 when it is not.
 */
int    d_internal_postgres_flush(PGconn*              _db,
                                 unsigned             _events,
                                 bool                 _turned_on,
                                 struct d_db_pending* _pending);
/**
 * @brief Reads what the server has sent, and says whether the next result
 *        has arrived.
 *
 * @param[in]     _db       the connection.
 * @param[in,out] _pending  receives, while the next result has yet to
 *                          arrive, what to wait for: the socket readable.
 * @return 0, PQgetResult then answering without waiting; D_DB_WAIT; or
 *         08006 when the connection is lost, 08000 for other trouble.
 */
int    d_internal_postgres_arrived(PGconn*              _db,
                                   struct d_db_pending* _pending);


//==============================================================================
// 3.  NAMES
//==============================================================================


// 3.1    Checks
//------------------------------------------------------------------------------
// checks -- neither allocates nor fails. _name_ok is false for a NULL or empty
// name; _schema_ok for an empty one, NULL meaning the current schema
bool d_internal_postgres_name_ok(const char* _name);
bool d_internal_postgres_schema_ok(const char* _schema);

// 3.2    Quoting
//------------------------------------------------------------------------------
// quoting -- d_internal_db_quote_fn's for the server: _quote_name writes a
// name in double quotes, any double quote doubled, and needs no connection;
// _quote_value writes a value as libpq's PQescapeLiteral does, for the
// connection `_context` points to, and fails for want of memory or for text
// that is not valid in the connection's encoding, leaving the buffer's length
// as it was. _quoting gathers both, for d_internal_db_format.
bool d_internal_postgres_quote_name(void*                        _context,
                                    struct d_internal_db_buffer* _out,
                                    const char*                  _name);
bool d_internal_postgres_quote_value(void*                        _context,
                                     struct d_internal_db_buffer* _out,
                                     const char*                  _text);
struct d_internal_db_quoting
     d_internal_postgres_quoting(PGconn* _db);

// 3.3    The current schema
//------------------------------------------------------------------------------
/**
 * @brief Settles the schema a statement names: the caller's, or for `NULL`
 *        the current one, asked of the server.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the caller's schema, or `NULL`.
 * @param[out] _use     receives the schema to name.
 * @param[out] _owned   receives what the caller frees with free() after,
 *                      `NULL` when nothing was allocated.
 * @return 0; D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME (3F000), as the server
 *         gives it for an unqualified CREATE, when no schema of the search
 *         path exists; or the failure to ask.
 */
int  d_internal_postgres_target(PGconn*      _db,
                                const char*  _schema,
                                const char** _use,
                                char**       _owned);

// 3.4    Clauses
//------------------------------------------------------------------------------
// clauses -- the words a flag adds to a statement when it is set in _flags
// ("IF EXISTS ", " CASCADE"), or "" when it is not
const char*
     d_internal_postgres_clause(unsigned    _flags,
                                unsigned    _flag,
                                const char* _words);


//==============================================================================
// 4.  STATEMENTS
//==============================================================================
// Formats are d_internal_db_format's: %Q a quoted value, %I a quoted name,
// %T a qualified one, %s text as written. Each returns 0, the SQLSTATE of the
// statement that failed, packed, or as section 2 says for failures libpq
// reports itself; 53200 when the text could not be built; or
// D_DB_INVALID_ARGUMENT for a format it could not read, or a value it could
// not write (NaN, say).


// 4.1    Running
//------------------------------------------------------------------------------
/**
 * @brief Sends SQL as written and reads what it produces.
 *
 * @param[in]  _db    the connection.
 * @param[in]  _sql   the SQL: one statement or several.
 * @param[out] _rows  receives the last statement's rows, or NULL for one that
 *                    returned none; `NULL` to discard them.
 * @return 0, a failure as section 4 says, or D_DB_UNSUPPORTED for COPY to or
 *         from the client, which is ended at once, so that the connection is
 *         ready for the next statement.
 * @post On success with `_rows`, the caller frees `*_rows` with PQclear.
 */
int  d_internal_postgres_send(PGconn*     _db,
                              const char* _sql,
                              PGresult**  _rows);
/**
 * @brief Formats a statement and sends it, as d_internal_postgres_send.
 *
 * @param[in]  _db      the connection.
 * @param[out] _rows    as for d_internal_postgres_send.
 * @param[in]  _format  the statement, as a d_internal_db_format format.
 * @param[in]  _args    the format's arguments.
 * @return as section 4 says.
 */
int  d_internal_postgres_vrun(PGconn*     _db,
                              PGresult**  _rows,
                              const char* _format,
                              va_list     _args);
// as d_internal_postgres_vrun, taking the arguments directly
int  d_internal_postgres_run(PGconn*     _db,
                             PGresult**  _rows,
                             const char* _format,
                             ...);

// 4.2    Single values
//------------------------------------------------------------------------------
// single values -- each runs SQL and reads the first column of the first row
// of its last statement. A row yields 0 and the value, written only then; no
// row, or no rows at all, is D_DB_NOT_FOUND; a failure is as section 4 says.
// A NULL reads as a NULL string, 0 or false. _vquery's string is the
// caller's, released with d_db_free. _int64 reads an integer exactly, and a
// number with a fraction truncated toward zero; _double rounds to the nearest
// double; _bool reads t or f; each reads text of another shape, or a number
// the type cannot hold, as D_DB_INVALID_ARGUMENT. _changes reads how many
// rows the statement counted, 0 for one that counts none.
int  d_internal_postgres_vquery(PGconn*     _db,
                                char**      _value,
                                const char* _format,
                                va_list     _args);
int  d_internal_postgres_query(PGconn*     _db,
                               char**      _value,
                               const char* _format,
                               ...);
int  d_internal_postgres_vquery_int64(PGconn*     _db,
                                      int64_t*    _value,
                                      const char* _format,
                                      va_list     _args);
int  d_internal_postgres_query_int64(PGconn*     _db,
                                     int64_t*    _value,
                                     const char* _format,
                                     ...);
int  d_internal_postgres_vquery_double(PGconn*     _db,
                                       double*     _value,
                                       const char* _format,
                                       va_list     _args);
int  d_internal_postgres_vquery_bool(PGconn*     _db,
                                     bool*       _value,
                                     const char* _format,
                                     va_list     _args);
int  d_internal_postgres_query_bool(PGconn*     _db,
                                    bool*       _value,
                                    const char* _format,
                                    ...);
int  d_internal_postgres_vchanges(PGconn*     _db,
                                  int64_t*    _changes,
                                  const char* _format,
                                  va_list     _args);

// 4.3    Rows
//------------------------------------------------------------------------------
// 4.3.1
// Whole results
// rows -- a column's type, and its type's name, from the type's OID, as
// postgres_query.h tabulates them; "" names a type not tabulated
enum d_db_type
     d_internal_postgres_oid_type(Oid _oid);
const char*
     d_internal_postgres_oid_name(Oid _oid);
/**
 * @brief Reads every row of a result into rows, and frees the result.
 *
 * @param[in]  _result  the result: rows -- all of them, one, or a chunk --
 *                      or a command's, which gives empty rows.
 * @param[out] _rows    receives the rows; written only on success.
 * @return 0, or 53200 when memory ran out.
 * @post `_result` is freed whatever happens. On success the caller releases
 *       `_rows` with d_db_rows_free.
 */
int  d_internal_postgres_collect_rows(PGresult*         _result,
                                      struct d_db_rows* _rows);

// 4.3.2
// Rows a few at a time
// rows a few at a time -- the halves of _collect_rows, for a cursor that
// builds its pages from one result at a time: _describe_columns describes a
// result's columns into a builder that has none yet; _keep_rows adds every
// row of a result. Each returns false when memory ran out, the builder then
// the caller's to release; neither frees the result.
bool d_internal_postgres_describe_columns(
         const PGresult*            _result,
         struct d_internal_db_rows* _builder);
bool d_internal_postgres_keep_rows(const PGresult*            _result,
                                   struct d_internal_db_rows* _builder);

// 4.4    Settling IF NOT EXISTS
//------------------------------------------------------------------------------
/**
 * @brief Runs a CREATE the server has no IF NOT EXISTS for, reading the
 *        failure that says the object is there as success.
 *
 * @param[in] _db         the connection.
 * @param[in] _tolerated  the SQLSTATE, packed, that means "already exists".
 * @param[in] _format     the statement, as a d_internal_db_format format.
 * @param[in] ...         the format's arguments.
 * @return 0 for success or the tolerated failure; any other failure as
 *         section 4 says.
 *
 * Inside a transaction the statement runs under a savepoint, rolled back to
 * when the tolerated failure comes, so that the transaction survives it;
 * any other failure leaves the transaction failed, as a statement's failure
 * always does. Atomic either way: the server alone decides whether the
 * object is there.
 */
int  d_internal_postgres_tolerate(PGconn*     _db,
                                  int         _tolerated,
                                  const char* _format,
                                  ...);


//==============================================================================
// 5.  CATALOGUE
//==============================================================================
// What a database holds, read from pg_catalog. Names are compared as the
// server's name type reads them; a schema is matched as section 3 of
// postgres_common.h describes: NULL as current_schema(), "pg_temp" as the
// session's temporary schema, any other name as written.


// 5.1    Objects
//------------------------------------------------------------------------------
// 5.1.1
// d_internal_postgres_object
//   enum: what a lookup looks for.
enum d_internal_postgres_object
{
    D_INTERNAL_POSTGRES_DATABASE = 0,  // a database on the server
    D_INTERNAL_POSTGRES_SCHEMA,        // a schema; listing leaves the
                                       // system's out
    D_INTERNAL_POSTGRES_EXTENSION,     // an installed extension
    D_INTERNAL_POSTGRES_TABLE,         // a table, ordinary or partitioned
    D_INTERNAL_POSTGRES_VIEW,          // a view
    D_INTERNAL_POSTGRES_SEQUENCE,      // a sequence
    D_INTERNAL_POSTGRES_INDEX,         // an index; listing leaves out those
                                       // a constraint owns
    D_INTERNAL_POSTGRES_COLUMN,        // a column of a table or view
    D_INTERNAL_POSTGRES_TRIGGER        // a trigger of the user's
};

// 5.1.2
// d_internal_postgres_path
//   struct: where an object is.
struct d_internal_postgres_path
{
    const char* schema;  // its schema; NULL: the current one. Not read for a
                         // database, a schema or an extension
    const char* table;   // the table it belongs to: required for a column, and
                         // for a trigger by name; optional for an index or a
                         // trigger listing; NULL otherwise
    const char* name;    // its name; NULL when listing
};

// 5.2    Lookups
//------------------------------------------------------------------------------
/**
 * @brief Answers whether an object exists.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to look for.
 * @param[in]  _path    where, and its name.
 * @param[out] _exists  receives the answer; `false` on any failure.
 * @return 0, a failure as section 4 says, or D_DB_INVALID_ARGUMENT for a
 *         `NULL` argument, a name that is `NULL` or empty, or a table missing
 *         where the object needs one.
 */
int  d_internal_postgres_has(PGconn*                                _db,
                             enum d_internal_postgres_object        _object,
                             const struct d_internal_postgres_path* _path,
                             bool*                                  _exists);
/**
 * @brief Lists the objects of a kind, sorted by their names' bytes.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to list.
 * @param[in]  _path    where; its name is not read.
 * @param[out] _names   receives the names; written only on success.
 * @return 0, a failure as section 4 says, 53200, or D_DB_INVALID_ARGUMENT as
 *         for d_internal_postgres_has.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_internal_postgres_list(PGconn*                                _db,
                              enum d_internal_postgres_object        _object,
                              const struct d_internal_postgres_path* _path,
                              struct d_db_name_list*                 _names);
/**
 * @brief Appends a condition placing the catalogue's namespace, aliased n,
 *        at a schema.
 *
 * @param[in]     _db      the connection, whose quoting applies.
 * @param[in,out] _sql     the statement so far.
 * @param[in]     _schema  the schema; `NULL` for the current one.
 * @return as d_internal_db_format.
 */
enum d_internal_db_format_status
     d_internal_postgres_match_schema(PGconn*                      _db,
                                      struct d_internal_db_buffer* _sql,
                                      const char*                  _schema);


#endif  // D_INTERNAL_POSTGRES


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_INTERNAL_H
