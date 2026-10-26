/*******************************************************************************
* djinterp [c]                                               mariadb_statement.h
*
* Prepared statements on a MariaDB or MySQL connection: SQL the server
* compiles once, values bound to its parameters, and run as often as needed.
*   d_mariadb_prepare has the server compile one statement, whose parameters
* are written `?` and numbered from 1 by their place. The bind functions give
* them values, which the statement keeps until it runs, and a parameter never
* bound is NULL. d_mariadb_execute_update runs the statement and counts the
* rows it changed; d_mariadb_execute_query runs it and returns its rows,
* typed as mariadb_query.h describes, so that a value reads the same whether
* it came through a statement or a query. Each run reads every result the
* statement produced, so the connection is ready for the next statement
* whatever the run did; the values stay bound. d_mariadb_open_cursor runs a
* statement whose rows d_mariadb_fetch_rows then reads a few at a time, as
* the server sends them, until a fetch comes back short or
* d_mariadb_close_cursor ends the run. d_mariadb_finalize releases the
* statement.
*   RESULTS. As every module's (mariadb_common.h): 0, or the client's error
* number for a failure -- a server error, such as 1062 (a duplicate key),
* which d_mariadb_error_message on the statement's connection then
* describes; CR_INVALID_PARAMETER_NO (2034) for a parameter the statement
* does not have; CR_OUT_OF_MEMORY (2008); CR_COMMANDS_OUT_OF_SYNC (2014)
* while another statement's run holds the connection; D_DB_INVALID_ARGUMENT
* for an argument that cannot be right.
*   The binary protocol sends what the client asks for; the layer asks for
* each value as text, the protocol's text, and reads it as a query's.
*   A RUN A FEW ROWS AT A TIME holds its connection until it ends, the server
* still sending its rows: every function on the connection that would send it
* anything -- another statement's prepare or run, a query, a ping -- is
* CR_COMMANDS_OUT_OF_SYNC (2014) meanwhile, having sent nothing. Running the
* statement again, or finalizing it, ends its run first, reading the rest of
* its rows away. Another statement finalized meanwhile is released at once;
* the server is told when the connection is next free.
*   A statement belongs to its connection. Closing the connection leaves its
* statements unable to run -- each run is then D_DB_INVALID_ARGUMENT -- and
* only to be finalized; a run still open ends with it. A prepare that did not
* block and failed leaves its statement so too (1.4).
*   WITHOUT BLOCKING (1.4, 3.4), with MariaDB's client alone -- libmysqlclient
* has no statement calls that do not block (D_DB_UNSUPPORTED), nor has
* MariaDB's over TLS it cannot carry so (mariadb_connection.h 1.8): each starts,
* answers D_DB_WAIT with what to wait for, and is continued with what came,
* as mariadb_connection.h 1.8 describes, until it answers as the blocking
* form would. Meanwhile the connection is held -- every other command on it
* is 2014, the statement's own blocking calls and binds among them -- and a
* statement finalized meanwhile abandons its own call: the client's statement
* stays with the connection, which nothing can use again but d_mariadb_close.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_statement.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STATEMENTS
    ----------
    1.  Preparing
    2.  Finalizing
    3.  Describing
    4.  Preparing without blocking
2.  VALUES
    ------
    1.  Binding
    2.  Clearing
3.  RUNNING
    -------
    1.  Rows changed
    2.  Rows
    3.  Rows a few at a time
    4.  Running without blocking
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_STATEMENT_H
#define DJINTERP_C_DB_MARIADB_MARIADB_STATEMENT_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"    // framework root
#include "./mariadb_common.h"  // d_mariadb, d_mariadb_statement, the results
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; int64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  STATEMENTS
//==============================================================================


// 1.1    Preparing
//------------------------------------------------------------------------------
/**
 * @brief Has the server compile one SQL statement.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _sql        one statement; a second is the server's syntax
 *                         error, as is a statement the server cannot
 *                         prepare.
 * @return 0; the client's error number for the failure, which
 *         d_mariadb_error_message describes; CR_OUT_OF_MEMORY (2008);
 *         CR_COMMANDS_OUT_OF_SYNC (2014) while a statement's run holds the
 *         connection; D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success the caller releases `*_statement` with
 *       d_mariadb_finalize.
 */
int    d_mariadb_prepare(struct d_mariadb*            _db,
                         struct d_mariadb_statement** _statement,
                         const char*                  _sql) D_NOTHROW;
/**
 * @brief Formats one SQL statement and has the server compile it, as
 *        d_mariadb_prepare.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _format     one statement, as a format (mariadb_query.h);
 *                         values are better bound than formatted, but a
 *                         name cannot be bound.
 * @param[in]  ...         the format's arguments.
 * @return as d_mariadb_prepare; D_DB_INVALID_ARGUMENT also for a format the
 *         layer cannot read.
 * @post As for d_mariadb_prepare.
 */
int    d_mariadb_preparef(struct d_mariadb*            _db,
                          struct d_mariadb_statement** _statement,
                          const char*                  _format,
                          ...) D_NOTHROW;

// 1.2    Finalizing
//------------------------------------------------------------------------------
/**
 * @brief Releases a statement, and the server's copy of it.
 *
 * @note A run of its own ends first, read to its end. While another
 *       statement's run, or a call that does not block, holds the
 *       connection, the server is told once the connection is free --
 *       before the layer's next command on it, or at its close. A call of
 *       the statement's own that does not block is abandoned, never waited
 *       for: the connection is then good only for d_mariadb_close.
 *
 * @param[in] _statement  the statement; may be `NULL`.
 * @return 0, or the client's error number when the server could not be told,
 *         the statement released all the same.
 * @post `_statement` is invalid.
 */
int    d_mariadb_finalize(struct d_mariadb_statement* _statement) D_NOTHROW;

// 1.3    Describing
//------------------------------------------------------------------------------
// describing -- _parameter_count is the number of `?` the statement has, 0
// for NULL; _statement_connection is the connection it belongs to, NULL for
// NULL or once that connection is closed
size_t d_mariadb_parameter_count(const struct d_mariadb_statement* _statement)
       D_NOTHROW;
struct d_mariadb*
       d_mariadb_statement_connection(
           const struct d_mariadb_statement* _statement) D_NOTHROW;

// 1.4    Preparing without blocking
//------------------------------------------------------------------------------
// d_mariadb_prepare_start begins what d_mariadb_prepare does -- the
// multi-statement mode a failed script left on turned off, then the prepare
// -- as steps that never wait, each continued by d_mariadb_prepare_continue
// as the socket allows. The statement exists from the start, and is the
// caller's to finalize whatever the continuation answers: a prepare that
// fails leaves it unprepared, good only for finalizing -- each run of it is
// D_DB_INVALID_ARGUMENT, and it has no parameters.
/**
 * @brief Begins having the server compile one SQL statement, without
 *        waiting.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement being prepared; `NULL` on
 *                         failure.
 * @param[in]  _sql        as for d_mariadb_prepare.
 * @param[out] _pending    receives what to wait for before continuing.
 * @return D_DB_WAIT; 0 should every step finish at once; as
 *         d_mariadb_prepare for a failure; D_DB_UNSUPPORTED with
 *         libmysqlclient, and over TLS the client cannot carry so;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, or SQL longer than
 *         the client's lengths carry.
 * @post Unless it failed, the caller releases `*_statement` with
 *       d_mariadb_finalize, and continues it with d_mariadb_prepare_continue
 *       while the answer is D_DB_WAIT.
 */
int    d_mariadb_prepare_start(struct d_mariadb*            _db,
                               struct d_mariadb_statement** _statement,
                               const char*                  _sql,
                               struct d_db_pending*         _pending)
                               D_NOTHROW;
/**
 * @brief Continues the prepare d_mariadb_prepare_start began.
 *
 * @param[in]     _statement  the statement being prepared.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @return 0, the statement compiled and its parameters counted; D_DB_WAIT;
 *         as d_mariadb_prepare for a failure, which leaves the statement
 *         unprepared; D_DB_INVALID_ARGUMENT for a `NULL` argument, or a
 *         statement with no prepare under way.
 */
int    d_mariadb_prepare_continue(struct d_mariadb_statement* _statement,
                                  unsigned                    _events,
                                  struct d_db_pending*        _pending)
                                  D_NOTHROW;


//==============================================================================
// 2.  VALUES
//==============================================================================


// 2.1    Binding
//------------------------------------------------------------------------------
// binding -- each gives the parameter numbered `_index`, from 1, a value
// until another is bound or the values are cleared: NULL; a boolean, as 1 or
// 0; an integer; a real number; text, copied, NULL binding NULL; bytes,
// `_size` of them copied, NULL binding NULL and a pointer to none the empty
// string of bytes. A run already begun keeps the values it began with; those
// bound meanwhile serve the next. Each returns 0; CR_INVALID_PARAMETER_NO
// (2034) for a parameter the statement does not have; CR_OUT_OF_MEMORY
// (2008); or D_DB_INVALID_ARGUMENT for a NULL statement, or text or bytes
// longer than the client can send.
int    d_mariadb_bind_null(struct d_mariadb_statement* _statement,
                           size_t                      _index) D_NOTHROW;
int    d_mariadb_bind_bool(struct d_mariadb_statement* _statement,
                           size_t                      _index,
                           bool                        _value) D_NOTHROW;
int    d_mariadb_bind_int64(struct d_mariadb_statement* _statement,
                            size_t                      _index,
                            int64_t                     _value) D_NOTHROW;
int    d_mariadb_bind_double(struct d_mariadb_statement* _statement,
                             size_t                      _index,
                             double                      _value) D_NOTHROW;
int    d_mariadb_bind_text(struct d_mariadb_statement* _statement,
                           size_t                      _index,
                           const char*                 _text) D_NOTHROW;
int    d_mariadb_bind_blob(struct d_mariadb_statement* _statement,
                           size_t                      _index,
                           const void*                 _bytes,
                           size_t                      _size) D_NOTHROW;

// 2.2    Clearing
//------------------------------------------------------------------------------
// clearing -- every parameter NULL again: 0, or D_DB_INVALID_ARGUMENT for a
// NULL statement
int    d_mariadb_clear_bindings(struct d_mariadb_statement* _statement)
       D_NOTHROW;


//==============================================================================
// 3.  RUNNING
//==============================================================================


// 3.1    Rows changed
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement, discarding any rows it returns.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _changes    receives, on success, the rows it affected (an
 *                         UPDATE counts those it changed, not those it
 *                         matched), or the rows it returned; 0 for a
 *                         statement that counts none; may be `NULL`.
 * @return 0; the client's error number for the failure; CR_OUT_OF_MEMORY
 *         (2008); CR_COMMANDS_OUT_OF_SYNC (2014) while another statement's
 *         run holds the connection; D_DB_INVALID_ARGUMENT for a `NULL`
 *         statement, or one whose connection is closed.
 * @post A run of its own that was open has ended, read to its end. Every
 *       result the statement produced has been read; its values stay bound.
 */
int    d_mariadb_execute_update(struct d_mariadb_statement* _statement,
                                int64_t*                    _changes)
       D_NOTHROW;

// 3.2    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement and returns every row it produced.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _rows       receives the rows, typed as mariadb_query.h
 *                         describes; written only on success.
 * @return 0; the client's error number for the failure; CR_OUT_OF_MEMORY
 *         (2008) when the rows could not be built; CR_COMMANDS_OUT_OF_SYNC
 *         (2014) while another statement's run holds the connection;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, or a statement whose
 *         connection is closed.
 * @post A run of its own that was open has ended, read to its end. Every
 *       result the statement produced has been read, the rows of the first
 *       kept; its values stay bound. On success the caller releases `_rows`
 *       with d_db_rows_free.
 */
int    d_mariadb_execute_query(struct d_mariadb_statement* _statement,
                               struct d_db_rows*           _rows) D_NOTHROW;

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement whose rows d_mariadb_fetch_rows then reads a few
 *        at a time, as the server sends them, rather than all at once.
 *
 * @note The run holds the connection until it ends, as the banner describes;
 *       a run of the statement's own that was open ends first, read to its
 *       end.
 *
 * @param[in] _statement  the statement, its values bound.
 * @return 0; the client's error number for the failure, the run not begun;
 *         CR_OUT_OF_MEMORY (2008); CR_COMMANDS_OUT_OF_SYNC (2014) while
 *         another statement's run holds the connection;
 *         D_DB_INVALID_ARGUMENT for a `NULL` statement, or one whose
 *         connection is closed.
 */
int    d_mariadb_open_cursor(struct d_mariadb_statement* _statement)
       D_NOTHROW;
/**
 * @brief Reads the next rows of the run d_mariadb_open_cursor began.
 *
 * @param[in]  _statement  the statement.
 * @param[in]  _max        the most rows to read; 1 or more.
 * @param[out] _rows       receives the columns and up to `_max` rows, typed
 *                         as mariadb_query.h describes; fewer -- none among
 *                         them -- once the run has none left, which ends it,
 *                         every result after the first read and discarded,
 *                         as d_mariadb_execute_query reads them. A statement
 *                         that returns no rows has no columns either, and
 *                         its first fetch ends its run. Written only on
 *                         success.
 * @return 0; the client's error number for a failure, which ends the run;
 *         CR_OUT_OF_MEMORY (2008) when the rows could not be built, which
 *         ends it too, read to its end; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument, no rows asked for, or no run open.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_mariadb_fetch_rows(struct d_mariadb_statement* _statement,
                            size_t                      _max,
                            struct d_db_rows*           _rows) D_NOTHROW;
// ending a run -- _close_cursor ends the run d_mariadb_open_cursor began,
// reading away whatever rows and results it had left, so that the
// connection is free again: 0, or the client's error number for a result
// after the first that failed, the run ended all the same; 0 for a statement
// with no run open; D_DB_INVALID_ARGUMENT for NULL
int    d_mariadb_close_cursor(struct d_mariadb_statement* _statement)
       D_NOTHROW;

// 3.4    Running without blocking
//------------------------------------------------------------------------------
// d_mariadb_execute_start sends the statement with its values; its results
// are read by d_mariadb_execute_update_continue, which gives them as
// d_mariadb_execute_update does, or by d_mariadb_execute_query_continue, as
// d_mariadb_execute_query does -- whichever is called first, the other then
// refused -- each answering D_DB_WAIT until every result has arrived. The
// start answers D_DB_WAIT even where the statement ran at once, its answer
// still to be read: what it says to wait for, the socket writable, is
// ready at once. d_mariadb_open_cursor_start opens a run as
// d_mariadb_open_cursor does, continued by d_mariadb_open_cursor_continue;
// d_mariadb_fetch_rows_start reads the next page as d_mariadb_fetch_rows
// does, a row at a time as each arrives, giving it once it is whole -- the
// connection keeping it between continuations -- continued by
// d_mariadb_fetch_rows_continue. Each start refuses, 2014, while a run, a
// page's start excepted, or a call that does not block holds the
// connection, rather than end or wait for it.
/**
 * @brief Sends a statement to run, with its values, without waiting for its
 *        results.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _pending    receives what to wait for.
 * @return D_DB_WAIT, the results to read with a continuation; the client's
 *         error number for a failure before anything was sent, the
 *         connection lost among them; 2014 while the connection is held;
 *         D_DB_UNSUPPORTED with libmysqlclient, and over TLS the client
 *         cannot carry so; D_DB_INVALID_ARGUMENT for a `NULL` argument, or a
 *         statement whose connection is closed.
 * @post While the answer is D_DB_WAIT, the caller reads the results with
 *       d_mariadb_execute_update_continue or
 *       d_mariadb_execute_query_continue.
 */
int    d_mariadb_execute_start(struct d_mariadb_statement* _statement,
                               struct d_db_pending*        _pending)
                               D_NOTHROW;
/**
 * @brief Reads the results of the run d_mariadb_execute_start sent, once
 *        they have arrived, counting the rows it changed.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @param[out]    _changes    as for d_mariadb_execute_update; may be `NULL`.
 * @return as d_mariadb_execute_update, and D_DB_WAIT; D_DB_INVALID_ARGUMENT
 *         for a `NULL` statement or pending call, or a statement with no run
 *         sent, or whose results d_mariadb_execute_query_continue reads.
 */
int    d_mariadb_execute_update_continue(
           struct d_mariadb_statement* _statement,
           unsigned                    _events,
           struct d_db_pending*        _pending,
           int64_t*                    _changes) D_NOTHROW;
/**
 * @brief Reads the results of the run d_mariadb_execute_start sent, once
 *        they have arrived, returning the rows of the first.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @param[out]    _rows       as for d_mariadb_execute_query.
 * @return as d_mariadb_execute_query, and D_DB_WAIT; D_DB_INVALID_ARGUMENT
 *         also for a statement with no run sent, or whose results
 *         d_mariadb_execute_update_continue reads.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_mariadb_execute_query_continue(
           struct d_mariadb_statement* _statement,
           unsigned                    _events,
           struct d_db_pending*        _pending,
           struct d_db_rows*           _rows) D_NOTHROW;
/**
 * @brief Opens a run as d_mariadb_open_cursor does, without waiting.
 *
 * @param[in]  _statement  the statement, its values bound.
 * @param[out] _pending    receives what to wait for before continuing.
 * @return 0, the run open, its pages to fetch; D_DB_WAIT; as
 *         d_mariadb_open_cursor for a failure; 2014 while the connection is
 *         held, a run of the statement's own among them; D_DB_UNSUPPORTED
 *         with libmysqlclient, and over TLS the client cannot carry so;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, or a statement whose
 *         connection is closed.
 */
int    d_mariadb_open_cursor_start(struct d_mariadb_statement* _statement,
                                   struct d_db_pending*        _pending)
                                   D_NOTHROW;
/**
 * @brief Continues the run d_mariadb_open_cursor_start began opening.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @return as d_mariadb_open_cursor_start; D_DB_INVALID_ARGUMENT also for a
 *         statement with no run being opened.
 */
int    d_mariadb_open_cursor_continue(
           struct d_mariadb_statement* _statement,
           unsigned                    _events,
           struct d_db_pending*        _pending) D_NOTHROW;
/**
 * @brief Begins reading the next page of the open run, without waiting.
 *
 * @param[in]  _statement  the statement, its run open.
 * @param[in]  _max        the most rows the page holds; 1 or more.
 * @param[out] _pending    receives what to wait for while the page is not
 *                         whole.
 * @param[out] _rows       as for d_mariadb_fetch_rows, once the page is
 *                         given.
 * @return as d_mariadb_fetch_rows, and D_DB_WAIT; 2014 while a call that
 *         does not block holds the connection; D_DB_UNSUPPORTED with
 *         libmysqlclient, and over TLS the client cannot carry so;
 *         D_DB_INVALID_ARGUMENT as d_mariadb_fetch_rows gives it.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_mariadb_fetch_rows_start(struct d_mariadb_statement* _statement,
                                  size_t                      _max,
                                  struct d_db_pending*        _pending,
                                  struct d_db_rows*           _rows)
                                  D_NOTHROW;
/**
 * @brief Reads more of the page d_mariadb_fetch_rows_start began, giving it
 *        once it is whole.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @param[out]    _rows       as for d_mariadb_fetch_rows_start.
 * @return as d_mariadb_fetch_rows_start; D_DB_INVALID_ARGUMENT also for a
 *         statement with no page under way.
 * @post As for d_mariadb_fetch_rows_start.
 */
int    d_mariadb_fetch_rows_continue(
           struct d_mariadb_statement* _statement,
           unsigned                    _events,
           struct d_db_pending*        _pending,
           struct d_db_rows*           _rows) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_STATEMENT_H
