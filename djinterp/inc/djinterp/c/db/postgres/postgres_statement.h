/*******************************************************************************
* djinterp [c]                                              postgres_statement.h
*
* Prepared statements on a PostgreSQL connection: SQL the server parses and
* plans once, values bound to its parameters, and run as often as needed.
*   d_postgres_prepare has the server prepare one statement under a name of
* the layer's -- d_stmt_ and hexadecimal digits, which other code should not
* give its own statements -- and asks it what type each parameter is.
* Parameters are written $1, $2 and so on, as PostgreSQL writes them, and a
* type the statement leaves open is the one the server infers. The bind
* functions give them values, which the statement keeps until it runs, and a
* parameter never bound is NULL. A value goes to the server as text, as its
* own input functions read text -- an integer in decimal, a real number to
* the seventeen digits that read back as the same double, NaN and the
* infinities as PostgreSQL spells them, a boolean as 1 or 0, which boolean
* and integer parameters alike accept -- and bytes go as bytes.
* d_postgres_execute_update runs the statement and counts the rows it
* changed; d_postgres_execute_query runs it and returns its rows, typed as
* postgres_query.h describes. d_postgres_open_cursor runs it with its rows
* read a few at a time: d_postgres_fetch_rows reads the next of them, up to
* a number, until a short page ends the run, and d_postgres_close_cursor
* ends it sooner. The server sends every row; libpq hands them over one at a
* time, so a run holds the connection until it ends: every other call that
* waits on the connection meanwhile -- another statement's run, prepare or
* release among them -- is 08000, as the layer's own answer
* (d_postgres_result_is_own). A statement released then cannot reach the
* server, whose copy stays until the session ends. d_postgres_finalize
* releases the statement, and the server's copy; d_postgres_close_prepared
* closes, by its name, one the session prepared another way -- with PREPARE,
* say.
*   RESULTS. As every module's (postgres_common.h): 0, or a failure's
* SQLSTATE, packed, which d_postgres_error_message on the statement's
* connection then describes; 42P02 (undefined parameter) for a parameter the
* statement does not have; 53200 when memory runs out; D_DB_INVALID_ARGUMENT
* for an argument that cannot be right.
*   A statement belongs to its connection and holds it: finalize the
* statement before closing the connection. Its run ends before anything
* else it does: running it again, opening another run, or finalizing it
* reads away whatever rows the run had left.
*   Preparing, running, opening a run and reading its pages each have a form
* that never waits (db_common.h 1.3), which starts, answers D_DB_WAIT while it
* waits on the socket, and is continued until it is done; meanwhile the
* connection is held, as by a run. A blocking call of the statement's own
* ends such a call first, waiting for what is left of it, as it ends a run.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_statement.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STATEMENTS
    ----------
    1.  Preparing
    2.  Finalizing and closing
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

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_STATEMENT_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_STATEMENT_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"     // framework root
#include "./postgres_common.h"  // pg_conn, d_postgres_statement, the results
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
 * @brief Has the server prepare one SQL statement.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _sql        one statement; a second is the server's syntax
 *                         error (42601).
 * @return 0; the SQLSTATE of the failure, packed; 53200 when memory ran out;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success the caller releases `*_statement` with
 *       d_postgres_finalize.
 */
int    d_postgres_prepare(struct pg_conn*               _db,
                          struct d_postgres_statement** _statement,
                          const char*                   _sql) D_NOTHROW;
/**
 * @brief Formats one SQL statement and has the server prepare it, as
 *        d_postgres_prepare.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _format     one statement, as a format (postgres_query.h);
 *                         values are better bound than formatted, but a
 *                         name cannot be bound.
 * @param[in]  ...         the format's arguments.
 * @return as d_postgres_prepare; D_DB_INVALID_ARGUMENT also for a format the
 *         layer cannot read.
 * @post As for d_postgres_prepare.
 */
int    d_postgres_preparef(struct pg_conn*               _db,
                           struct d_postgres_statement** _statement,
                           const char*                   _format,
                           ...) D_NOTHROW;

// 1.2    Finalizing and closing
//------------------------------------------------------------------------------
/**
 * @brief Releases a statement, and the server's copy of it.
 *
 * @note The server's copy is closed with libpq's PQclosePrepared where libpq
 *       has it (17), which works inside a failed transaction too, and with
 *       DEALLOCATE elsewhere, which does not: there the server keeps the
 *       copy until the session ends, or until a statement prepared later
 *       takes its name, which drops it first; the statement is released all
 *       the same.
 *
 * @param[in] _statement  the statement; may be `NULL`.
 * @return 0, or the SQLSTATE, packed, of a failure to close the server's
 *         copy.
 * @post `_statement` is invalid.
 */
int    d_postgres_finalize(struct d_postgres_statement* _statement)
       D_NOTHROW;
/**
 * @brief Closes a statement the session has prepared, by its name: one
 *        PREPARE made, or a program's own libpq call.
 *
 * @note libpq from 17 closes it by the protocol's own message
 *       (PQclosePrepared), which a failed transaction takes too; before it
 *       the layer looks for the statement, and runs DEALLOCATE, which a
 *       failed transaction refuses (25P02). The layer's own statements are
 *       its own to close (d_postgres_finalize); a name of theirs, d_stmt_
 *       and hexadecimal digits, is no name to give here.
 *
 * @param[in] _db    the connection.
 * @param[in] _name  the statement's name.
 * @return 0, whether or not the session had a statement of the name; the
 *         SQLSTATE of the failure, packed; D_DB_INVALID_ARGUMENT for a
 *         `NULL` connection, or a `NULL` or empty name.
 */
int    d_postgres_close_prepared(struct pg_conn* _db,
                                 const char*     _name) D_NOTHROW;

// 1.3    Describing
//------------------------------------------------------------------------------
// describing -- _parameter_count is the number of the highest parameter the
// statement uses, 0 for NULL; _statement_connection is the connection it
// belongs to, NULL for NULL
size_t d_postgres_parameter_count(const struct d_postgres_statement* _statement)
       D_NOTHROW;
struct pg_conn*
       d_postgres_statement_connection(
           const struct d_postgres_statement* _statement) D_NOTHROW;


// 1.4    Preparing without blocking
//------------------------------------------------------------------------------
// d_postgres_prepare_start begins what d_postgres_prepare does -- a stale name
// looked for where it must be and dropped, the prepare, tried once more after
// a stale name the server still held, the parameters described -- as
// exchanges that never wait, each continued by d_postgres_prepare_continue as
// the socket allows, until one answers anything but D_DB_WAIT. The statement
// exists from the start, and is the caller's to finalize whatever the
// outcome: a prepare that fails leaves it unprepared, good for nothing else,
// and finalizing one under way waits for what is left of it. Not in pipeline
// mode (08000).
/**
 * @brief Begins having the server prepare one SQL statement, without
 *        waiting.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement being prepared; `NULL` on
 *                         failure.
 * @param[in]  _sql        as for d_postgres_prepare.
 * @param[out] _pending    receives what to wait for before continuing.
 * @return D_DB_WAIT; 0 should every exchange finish at once; 08000 while a
 *         run holds the connection, or in pipeline mode; 08006 when the
 *         connection is lost; 53200 when memory ran out;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post Unless it failed, the caller releases `*_statement` with
 *       d_postgres_finalize, and continues it with
 *       d_postgres_prepare_continue while the answer is D_DB_WAIT.
 */
int    d_postgres_prepare_start(struct pg_conn*               _db,
                                struct d_postgres_statement** _statement,
                                const char*                   _sql,
                                struct d_db_pending*          _pending)
                                D_NOTHROW;
/**
 * @brief Continues the prepare d_postgres_prepare_start began.
 *
 * @param[in]     _statement  the statement being prepared.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @return 0, the statement prepared and its parameters described;
 *         D_DB_WAIT; as d_postgres_prepare for a failure, which leaves the
 *         statement unprepared; D_DB_INVALID_ARGUMENT for a `NULL` argument,
 *         or a statement with no prepare under way.
 */
int    d_postgres_prepare_continue(struct d_postgres_statement* _statement,
                                   unsigned                     _events,
                                   struct d_db_pending*         _pending)
                                   D_NOTHROW;


//==============================================================================
// 2.  VALUES
//==============================================================================


// 2.1    Binding
//------------------------------------------------------------------------------
// binding -- each gives the parameter numbered `_index`, from 1, a value
// until another is bound or the values are cleared: NULL; a boolean, as 1 or
// 0; an integer; a real number; text, copied, NULL binding NULL; bytes,
// `_size` of them copied and sent as bytes, NULL binding NULL and a pointer
// to none an empty bytea. Each returns 0; 42P02 for a parameter the
// statement does not have; 53200 when memory ran out; or
// D_DB_INVALID_ARGUMENT for a NULL statement, or text or bytes longer than
// libpq can send.
int    d_postgres_bind_null(struct d_postgres_statement* _statement,
                            size_t                       _index) D_NOTHROW;
int    d_postgres_bind_bool(struct d_postgres_statement* _statement,
                            size_t                       _index,
                            bool                         _value) D_NOTHROW;
int    d_postgres_bind_int64(struct d_postgres_statement* _statement,
                             size_t                       _index,
                             int64_t                      _value) D_NOTHROW;
int    d_postgres_bind_double(struct d_postgres_statement* _statement,
                              size_t                       _index,
                              double                       _value) D_NOTHROW;
int    d_postgres_bind_text(struct d_postgres_statement* _statement,
                            size_t                       _index,
                            const char*                  _text) D_NOTHROW;
int    d_postgres_bind_blob(struct d_postgres_statement* _statement,
                            size_t                       _index,
                            const void*                  _bytes,
                            size_t                       _size) D_NOTHROW;

// 2.2    Clearing
//------------------------------------------------------------------------------
// clearing -- every parameter NULL again: 0, or D_DB_INVALID_ARGUMENT for a
// NULL statement
int    d_postgres_clear_bindings(struct d_postgres_statement* _statement)
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
 * @param[out] _changes    receives, on success, the rows it inserted,
 *                         updated, deleted or merged, or returned; 0 for a
 *                         statement that counts none; may be `NULL`.
 * @return 0; the SQLSTATE of the failure, packed; D_DB_UNSUPPORTED for COPY
 *         to or from the client, which the layer ends at once;
 *         D_DB_INVALID_ARGUMENT for a `NULL` statement.
 * @post Its values stay bound.
 */
int    d_postgres_execute_update(struct d_postgres_statement* _statement,
                                 int64_t*                     _changes)
       D_NOTHROW;

// 3.2    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement and returns every row it produced.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _rows       receives the rows, typed as postgres_query.h
 *                         describes; written only on success.
 * @return as d_postgres_execute_update; 53200 also when the rows could not
 *         be built; D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post Its values stay bound. On success the caller releases `_rows` with
 *       d_db_rows_free.
 */
int    d_postgres_execute_query(struct d_postgres_statement* _statement,
                                struct d_db_rows*            _rows) D_NOTHROW;

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement whose rows d_postgres_fetch_rows then reads a few
 *        at a time, as the server sends them, rather than all at once.
 *
 * @note The run holds the connection until it ends, as the banner
 *       describes; a run of the statement's own that was open ends first,
 *       read to its end. Values bound during the run serve the next one.
 *
 * @param[in] _statement  the statement, its values bound.
 * @return 0; 08000 while another run holds the connection, or in pipeline
 *         mode, which reads its results statement by statement
 *         (postgres_protocol.h); 08006 when the connection is lost;
 *         D_DB_INVALID_ARGUMENT for a `NULL` statement.
 */
int    d_postgres_open_cursor(struct d_postgres_statement* _statement)
       D_NOTHROW;
/**
 * @brief Reads the next rows of the run d_postgres_open_cursor began.
 *
 * @param[in]  _statement  the statement.
 * @param[in]  _max        the most rows to read; 1 or more.
 * @param[out] _rows       receives the columns and up to `_max` rows, typed
 *                         as postgres_query.h describes; fewer -- none
 *                         among them -- once the run has none left, which
 *                         ends it. A statement that returns no rows has no
 *                         columns either, and its first fetch ends its
 *                         run. Written only on success.
 * @return 0; the SQLSTATE of a failure, packed, which ends the run -- the
 *         server's failure part-way through the rows among them;
 *         D_DB_UNSUPPORTED for COPY, ended at once; 53200 when the rows
 *         could not be built, which ends the run too, read to its end;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, no rows asked for,
 *         or no run open.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_postgres_fetch_rows(struct d_postgres_statement* _statement,
                             size_t                       _max,
                             struct d_db_rows*            _rows) D_NOTHROW;
// ending a run -- _close_cursor ends the run d_postgres_open_cursor began,
// reading away whatever rows it had left, so that the connection is free
// again: 0, or the SQLSTATE, packed, of a failure the server reported among
// them, the run ended all the same; 0 for a statement with no run open;
// D_DB_INVALID_ARGUMENT for NULL
int    d_postgres_close_cursor(struct d_postgres_statement* _statement)
       D_NOTHROW;

// 3.4    Running without blocking
//------------------------------------------------------------------------------
// d_postgres_execute_start sends the statement with its values and returns,
// never reading: its result is read by d_postgres_execute_update_continue,
// which gives it as d_postgres_execute_update does, or by
// d_postgres_execute_query_continue, as d_postgres_execute_query does, each
// answering D_DB_WAIT until it has arrived, the statement keeping it until
// one of them reads it. d_postgres_open_cursor_start opens a run as
// d_postgres_open_cursor does, d_postgres_open_cursor_continue writing what
// the socket could not take; d_postgres_fetch_rows_start reads a page as
// d_postgres_fetch_rows does, from whatever has arrived, giving it once it is
// whole -- the statement keeping it between continuations -- and
// d_postgres_fetch_rows_continue reads more. Each start refuses, 08000, while
// a run or a call of the statement's own is under way, rather than wait for
// it -- a page's start excepted, which is the run's -- and in pipeline mode.
/**
 * @brief Sends a statement to run, with its values, without waiting for its
 *        result.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _pending    receives what to wait for: room to write the
 *                         rest, or the result.
 * @return D_DB_WAIT, the result to read with a continuation; 08000 while a
 *         run or a call of its own, or another's run, is under way, or in
 *         pipeline mode; 08006 when the connection is lost;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post While the answer is D_DB_WAIT, the caller reads the result with
 *       d_postgres_execute_update_continue or
 *       d_postgres_execute_query_continue.
 */
int    d_postgres_execute_start(struct d_postgres_statement* _statement,
                                struct d_db_pending*         _pending)
                                D_NOTHROW;
/**
 * @brief Reads the result of the run d_postgres_execute_start sent, once it
 *        has arrived, counting the rows it changed.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @param[out]    _changes    as for d_postgres_execute_update; may be
 *                            `NULL`.
 * @return as d_postgres_execute_update, and D_DB_WAIT;
 *         D_DB_INVALID_ARGUMENT for a `NULL` statement or pending call, or a
 *         statement with no run sent.
 */
int    d_postgres_execute_update_continue(
           struct d_postgres_statement* _statement,
           unsigned                     _events,
           struct d_db_pending*         _pending,
           int64_t*                     _changes) D_NOTHROW;
/**
 * @brief Reads the result of the run d_postgres_execute_start sent, once it
 *        has arrived, returning its rows.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @param[out]    _rows       as for d_postgres_execute_query.
 * @return as d_postgres_execute_query, and D_DB_WAIT; D_DB_INVALID_ARGUMENT
 *         also for a statement with no run sent.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_postgres_execute_query_continue(
           struct d_postgres_statement* _statement,
           unsigned                     _events,
           struct d_db_pending*         _pending,
           struct d_db_rows*            _rows) D_NOTHROW;
/**
 * @brief Opens a run as d_postgres_open_cursor does, without waiting for
 *        the socket.
 *
 * @param[in]  _statement  the statement, its values bound.
 * @param[out] _pending    receives what to wait for while some of the run's
 *                         message is left to write.
 * @return 0, the run open, its pages to fetch; D_DB_WAIT; 08000 as
 *         d_postgres_execute_start gives it; 08006 when the connection is
 *         lost; D_DB_INVALID_ARGUMENT for a `NULL` argument.
 */
int    d_postgres_open_cursor_start(struct d_postgres_statement* _statement,
                                    struct d_db_pending*         _pending)
                                    D_NOTHROW;
/**
 * @brief Writes more of the run d_postgres_open_cursor_start opened.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @return as d_postgres_open_cursor_start; D_DB_INVALID_ARGUMENT also for a
 *         statement with no run being opened.
 */
int    d_postgres_open_cursor_continue(
           struct d_postgres_statement* _statement,
           unsigned                     _events,
           struct d_db_pending*         _pending) D_NOTHROW;
/**
 * @brief Begins reading the next page of the open run, without waiting.
 *
 * @param[in]  _statement  the statement, its run open.
 * @param[in]  _max        the most rows the page holds; 1 or more.
 * @param[out] _pending    receives what to wait for while the page is not
 *                         whole.
 * @param[out] _rows       as for d_postgres_fetch_rows, once the page is
 *                         given.
 * @return as d_postgres_fetch_rows, and D_DB_WAIT; 08000 while a call of
 *         its own is under way; D_DB_INVALID_ARGUMENT as
 *         d_postgres_fetch_rows gives it.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_postgres_fetch_rows_start(struct d_postgres_statement* _statement,
                                   size_t                       _max,
                                   struct d_db_pending*         _pending,
                                   struct d_db_rows*            _rows)
                                   D_NOTHROW;
/**
 * @brief Reads more of the page d_postgres_fetch_rows_start began, giving
 *        it once it is whole.
 *
 * @param[in]     _statement  the statement.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation wrote;
 *                            receives what to wait for next.
 * @param[out]    _rows       as for d_postgres_fetch_rows_start.
 * @return as d_postgres_fetch_rows_start; D_DB_INVALID_ARGUMENT also for a
 *         statement with no page under way.
 * @post As for d_postgres_fetch_rows_start.
 */
int    d_postgres_fetch_rows_continue(
           struct d_postgres_statement* _statement,
           unsigned                     _events,
           struct d_db_pending*         _pending,
           struct d_db_rows*            _rows) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_STATEMENT_H
