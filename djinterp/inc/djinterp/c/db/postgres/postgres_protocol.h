/*******************************************************************************
* djinterp [c]                                               postgres_protocol.h
*
* What PostgreSQL's protocol offers beyond a statement and its reply: rows
* copied in, notifications, statements sent without waiting, their rows read
* a few at a time, and pipelines.
*   COPY. d_postgres_copy_in_start runs a COPY ... FROM STDIN, after which the
* server takes the rows the caller sends with d_postgres_copy_in_data, in the
* COPY's format -- text by default, a line a row, its columns separated by
* tabs -- in chunks of any size, until d_postgres_copy_in_end ends it, or
* abandons it with a message. Until then the connection runs nothing else.
*   NOTIFICATIONS. d_postgres_listen subscribes the connection to a channel,
* d_postgres_notify sends on one, and d_postgres_next_notification reads
* what has arrived, without waiting: a program that waits does so on the
* connection's socket (libpq's PQsocket) or calls again later. A
* notification sent inside a transaction arrives when it commits; one that
* rolls back sends none. Channels are names, case kept as given.
*   STATEMENTS SENT. d_postgres_send_query sends SQL -- one statement or
* several -- and returns at once; d_postgres_next_result reads its results,
* one a call, waiting for each, until it answers D_DB_NOT_FOUND. A program
* that must not block calls d_postgres_consume_input when the socket is
* readable and d_postgres_next_result only when d_postgres_is_busy says a
* result is ready. Until the last result is read nothing more can be sent,
* and a call that runs SQL meanwhile is 08000, the results left to read --
* libpq would wait for them and discard them unseen, as it would a
* statement's rows read a few at a time (postgres_statement.h 3.3), which
* hold the connection the same way. d_postgres_set_chunked_rows has the rows
* of the statements still to come arrive a few at a time -- one a result, as
* every libpq can, or more, from libpq 17 -- rather than all at once.
* d_postgres_send_query_start and d_postgres_next_result_start are the forms
* that never wait (db_common.h 1.3): the send writes what the socket takes,
* and the read gives a result once it has arrived, each answering D_DB_WAIT
* until then and continued when the socket is ready. Every statement sent
* this way -- one or several, rows whole or a few at a time -- is read this
* way too; d_postgres_exec and the layer's other calls are its blocking
* compositions.
*   PIPELINES. In pipeline mode (libpq 14) d_postgres_send_query queues
* statements, one a call, without waiting for any of them, and
* d_postgres_pipeline_sync marks a synchronization point, which sends what is
* queued. The server runs the statements in order, and those between two
* synchronization points as one transaction, unless they begin their own; a
* statement that fails has the server skip every statement after it, to the
* next synchronization point. d_postgres_next_result reads the results in
* the order sent -- each statement's rows, or its failure's SQLSTATE, or
* 25P02 for each statement skipped -- and answers D_DB_NOT_FOUND for each
* synchronization point, after which the next batch's results follow, and
* once nothing is left to read. The server holds a result until a
* synchronization point or until asked for it -- d_postgres_next_result asks
* before it waits, and d_postgres_pipeline_flush asks -- so a statement
* queued after the last synchronization point has its results read all the
* same, though the transaction it is part of ends only at the next one. Rows
* a few at a time are asked for statement by statement. In pipeline mode
* every other call that runs SQL is 08000: libpq refuses a call that waits
* for its result.
*   RESULTS. As every module's (postgres_common.h): 0; the SQLSTATE of the
* failure, packed; 08006 for a connection that is lost, 08000 for one that
* cannot do what is asked; 53200 when memory runs out; D_DB_INVALID_ARGUMENT
* for an argument that cannot be right; D_DB_UNSUPPORTED where libpq lacks
* what is asked.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_protocol.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.24
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  COPY
    ----
    1.  Copying rows in
2.  NOTIFICATIONS
    -------------
    1.  Channels
    2.  Arrivals
3.  STATEMENTS SENT
    ---------------
    1.  Sending
    2.  Results
    3.  Rows a few at a time
    4.  Without blocking
4.  PIPELINES
    ---------
    1.  The mode
         1.  d_postgres_pipeline
    2.  Entering and leaving
    3.  Synchronizing and flushing
5.  LARGE OBJECTS
    -------------
    1.  Files in and out
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_PROTOCOL_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_PROTOCOL_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"     // framework root
#include "./postgres_common.h"  // pg_conn, the layer's results
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
// 1.  COPY
//==============================================================================


// 1.1    Copying rows in
//------------------------------------------------------------------------------
/**
 * @brief Runs a COPY ... FROM STDIN, which then waits for rows.
 *
 * @param[in] _db   the connection.
 * @param[in] _sql  the COPY, as written ("COPY t (a, b) FROM STDIN",
 *                  "COPY t FROM STDIN (FORMAT csv)"). Any other statement
 *                  runs as it would anywhere, and is then
 *                  D_DB_INVALID_ARGUMENT, having done what it does.
 * @return 0, the server waiting for rows; the SQLSTATE of the failure;
 *         D_DB_UNSUPPORTED for a COPY to the client, which the layer ends;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, or SQL that is no
 *         COPY FROM STDIN.
 * @post On success the connection runs nothing but d_postgres_copy_in_data
 *       and d_postgres_copy_in_end until the second ends the COPY.
 */
int    d_postgres_copy_in_start(struct pg_conn* _db,
                                const char*     _sql) D_NOTHROW;
/**
 * @brief Sends rows to the COPY in progress.
 *
 * @param[in] _db    the connection.
 * @param[in] _data  bytes of rows in the COPY's format; a chunk may end
 *                   anywhere, a row's middle included.
 * @param[in] _size  how many bytes; 0 sends nothing.
 * @return 0; 08000 when no COPY is in progress, 08006 when the connection
 *         is lost; D_DB_INVALID_ARGUMENT for a `NULL` connection, or `NULL`
 *         data with a nonzero size. A row the server cannot read is reported
 *         by d_postgres_copy_in_end, which the server's refusal reaches.
 */
int    d_postgres_copy_in_data(struct pg_conn* _db,
                               const void*     _data,
                               size_t          _size) D_NOTHROW;
/**
 * @brief Ends the COPY in progress, or abandons it.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _error    `NULL` to end the COPY, keeping every row sent; or a
 *                       message, which abandons it, keeping none.
 * @param[out] _changes  receives, on success, how many rows the COPY took;
 *                       may be `NULL`.
 * @return 0; the server's SQLSTATE for rows it refused (22P02 for text a
 *         column cannot read, 23505 for a duplicate key), or for the COPY
 *         abandoned (57014); 08000 when no COPY is in progress; or
 *         D_DB_INVALID_ARGUMENT for a `NULL` connection.
 * @post No COPY is in progress, and the connection is ready.
 */
int    d_postgres_copy_in_end(struct pg_conn* _db,
                              const char*     _error,
                              int64_t*        _changes) D_NOTHROW;


//==============================================================================
// 2.  NOTIFICATIONS
//==============================================================================


// 2.1    Channels
//------------------------------------------------------------------------------
// channels -- _listen subscribes the connection to a channel, as LISTEN
// does, and _unlisten ends that, or every subscription for `NULL`, as
// UNLISTEN does; inside a transaction either takes effect when it commits.
// _notify sends a notification on a channel, with a payload of up to 7999
// bytes or none for `NULL`, as pg_notify does; every connection listening
// gets it, the sender's own included. Each returns 0, the SQLSTATE of the
// failure (22023 for a payload too long), or D_DB_INVALID_ARGUMENT for a
// `NULL` connection or an empty or `NULL` channel.
int    d_postgres_listen(struct pg_conn* _db,
                         const char*     _channel) D_NOTHROW;
int    d_postgres_unlisten(struct pg_conn* _db,
                           const char*     _channel) D_NOTHROW;
int    d_postgres_notify(struct pg_conn* _db,
                         const char*     _channel,
                         const char*     _payload) D_NOTHROW;

// 2.2    Arrivals
//------------------------------------------------------------------------------
/**
 * @brief Reads the next notification to have arrived, without waiting for
 *        one.
 *
 * @param[in]  _db       the connection.
 * @param[out] _channel  receives the channel, which the caller releases with
 *                       d_db_free; written only on success.
 * @param[out] _payload  receives the payload, "" for none, which the caller
 *                       releases with d_db_free; written only on success;
 *                       may be `NULL`.
 * @param[out] _pid      receives the process id of the sender's backend;
 *                       written only on success; may be `NULL`.
 * @return 0; D_DB_NOT_FOUND when none has arrived; 08006 when the
 *         connection is lost; 53200 when memory ran out, the notification
 *         lost with it; D_DB_INVALID_ARGUMENT for a `NULL` connection or
 *         channel.
 */
int    d_postgres_next_notification(struct pg_conn* _db,
                                    char**          _channel,
                                    char**          _payload,
                                    int*            _pid) D_NOTHROW;


//==============================================================================
// 3.  STATEMENTS SENT
//==============================================================================


// 3.1    Sending
//------------------------------------------------------------------------------
/**
 * @brief Sends SQL without waiting for it to run.
 *
 * @param[in] _db   the connection.
 * @param[in] _sql  one statement or several, separated by semicolons, which
 *                  run as d_postgres_exec runs them; in pipeline mode one
 *                  statement, sent by the extended protocol, which takes no
 *                  more (a second is the server's syntax error, 42601).
 * @return 0, the SQL sent, or queued; 08000 when the connection is still
 *         busy with SQL sent before, or a COPY; 08006 when it is lost; or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success, outside pipeline mode, nothing more can be sent until
 *       d_postgres_next_result answers D_DB_NOT_FOUND; a call that runs SQL
 *       first waits for the results still to come, and discards them.
 */
int    d_postgres_send_query(struct pg_conn* _db,
                             const char*     _sql) D_NOTHROW;

// 3.2    Results
//------------------------------------------------------------------------------
/**
 * @brief Reads the next result of the SQL sent, waiting for it.
 *
 * @note Where d_postgres_set_chunked_rows asked for them, a statement's rows
 *       come a few at a time, each result the next of them, and then its
 *       end: rows with its columns and none of its rows. Its count comes
 *       with the last result that has a count, as libpq gives it: the end's
 *       in single-row mode, the last chunk's in libpq's chunked mode.
 *
 * @param[in]  _db       the connection.
 * @param[out] _rows     receives the statement's rows, typed as
 *                       postgres_query.h describes, or rows with no columns
 *                       for a statement that returns none; written only on
 *                       success.
 * @param[out] _changes  receives, on success, how many rows the statement
 *                       inserted, updated, deleted or returned, 0 for one
 *                       that counts none; may be `NULL`.
 * @return 0; D_DB_NOT_FOUND when every result has been read, the
 *         connection then ready -- in pipeline mode, at each synchronization
 *         point too; the SQLSTATE of the statement that failed, after which
 *         only D_DB_NOT_FOUND follows -- in pipeline mode, 25P02 for each
 *         statement skipped after it, up to the synchronization point;
 *         D_DB_UNSUPPORTED for a COPY, which the layer ends, with every
 *         result after it; 53200 when the rows could not be built;
 *         D_DB_INVALID_ARGUMENT for a `NULL` connection or rows.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_postgres_next_result(struct pg_conn*   _db,
                              struct d_db_rows* _rows,
                              int64_t*          _changes) D_NOTHROW;
// waiting -- _consume_input reads what the server has sent, without waiting:
// 0, 08006 when the connection is lost, or D_DB_INVALID_ARGUMENT for NULL.
// _is_busy says whether the next result has yet to arrive, d_postgres_
// next_result then waiting for it; false for NULL, and when nothing was
// sent. In pipeline mode the result of a statement queued after the last
// synchronization point arrives only once asked for
// (d_postgres_pipeline_flush); and straight after a statement there fails,
// _is_busy is false even while the next synchronization point's own result
// -- the next result, when no statement was skipped -- is still on its way,
// which d_postgres_next_result then waits for
bool   d_postgres_is_busy(struct pg_conn* _db) D_NOTHROW;
int    d_postgres_consume_input(struct pg_conn* _db) D_NOTHROW;

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/**
 * @brief Has the rows of the statements still to come arrive a few at a
 *        time, as d_postgres_next_result reads them, rather than all at
 *        once.
 *
 * @note Called before the rows it is for are looked at: straight after
 *       d_postgres_send_query, before any other call on the connection, or
 *       straight after a statement's last result is read, for the
 *       statements after it. Outside pipeline mode it holds for the rest of
 *       the SQL sent; in pipeline mode, for the next statement alone, as
 *       libpq has it asked for statement by statement. One row a result is
 *       libpq's single-row mode, which every libpq has; more is libpq 17's
 *       chunked mode.
 *
 * @param[in] _db    the connection.
 * @param[in] _rows  the most rows a result holds; 1 or more.
 * @return 0; 08000 when it comes too late: no statement's results are still
 *         to come, or the next one's have begun; D_DB_UNSUPPORTED for more
 *         than one row with a libpq before 17; D_DB_INVALID_ARGUMENT for a
 *         `NULL` connection, or fewer than one row.
 */
int    d_postgres_set_chunked_rows(struct pg_conn* _db,
                                   int             _rows) D_NOTHROW;

// 3.4    Without blocking
//------------------------------------------------------------------------------
// The forms of 3.1 and 3.2 that never wait. d_postgres_send_query_start sends
// as d_postgres_send_query does, in libpq's non-blocking mode, which is off
// again once everything is written: it answers D_DB_WAIT while some of the
// SQL is left, the socket full, and d_postgres_send_query_continue writes more
// each time the socket takes it, reading what the server sends meanwhile.
// d_postgres_next_result_start reads what the server has sent and gives the
// next result as d_postgres_next_result does -- D_DB_NOT_FOUND once every
// result is read -- or D_DB_WAIT while it has yet to arrive, after which
// d_postgres_next_result_continue reads more each time the socket is
// readable. Rows a few at a time arrive the same way, a result a page; and
// between continuations the connection is held, every call that waits on it
// 08000, as for a query sent by 3.1. Pipeline mode, whose calls queue without
// waiting already, is not for these (08000). A COPY among the results is
// ended as d_postgres_next_result ends one, which waits.
/**
 * @brief Sends SQL as d_postgres_send_query does, without waiting for the
 *        socket.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _sql      as for d_postgres_send_query, outside pipeline
 *                       mode.
 * @param[out] _pending  receives what to wait for while some is left to
 *                       write; left settled once everything is.
 * @return 0, the SQL sent; D_DB_WAIT, some left to write; 08000 in pipeline
 *         mode, or as d_postgres_send_query gives it; 08006 when the
 *         connection is lost; or D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 * @post On 0 and while D_DB_WAIT the results are read as 3.2 or this
 *       subsection reads them -- once everything is written, for the
 *       latter.
 */
int    d_postgres_send_query_start(struct pg_conn*      _db,
                                   const char*          _sql,
                                   struct d_db_pending* _pending) D_NOTHROW;
/**
 * @brief Writes more of the SQL d_postgres_send_query_start began to send.
 *
 * @param[in]     _db       the connection.
 * @param[in]     _events   what came, as d_db_wait.
 * @param[in,out] _pending  what the start or the last continuation wrote;
 *                          receives what to wait for next.
 * @return as d_postgres_send_query_start; D_DB_INVALID_ARGUMENT for a
 *         `NULL` argument.
 */
int    d_postgres_send_query_continue(struct pg_conn*      _db,
                                      unsigned             _events,
                                      struct d_db_pending* _pending)
                                      D_NOTHROW;
/**
 * @brief Gives the next result of the SQL sent once it has arrived,
 *        without waiting for it.
 *
 * @param[in]  _db       the connection.
 * @param[out] _pending  receives what to wait for while the result has yet
 *                       to arrive; left settled once it has.
 * @param[out] _rows     as for d_postgres_next_result.
 * @param[out] _changes  as for d_postgres_next_result; may be `NULL`.
 * @return as d_postgres_next_result, and D_DB_WAIT while the result has yet
 *         to arrive; 08000 in pipeline mode; D_DB_INVALID_ARGUMENT for a
 *         `NULL` connection, pending call or rows.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_postgres_next_result_start(struct pg_conn*      _db,
                                    struct d_db_pending* _pending,
                                    struct d_db_rows*    _rows,
                                    int64_t*             _changes) D_NOTHROW;
/**
 * @brief Reads more of what d_postgres_next_result_start waited for, and
 *        gives the result once it has arrived.
 *
 * @param[in]     _db       the connection.
 * @param[in]     _events   what came, as d_db_wait.
 * @param[in,out] _pending  what the start or the last continuation wrote;
 *                          receives what to wait for next.
 * @param[out]    _rows     as for d_postgres_next_result.
 * @param[out]    _changes  as for d_postgres_next_result; may be `NULL`.
 * @return as d_postgres_next_result_start.
 * @post As for d_postgres_next_result_start.
 */
int    d_postgres_next_result_continue(struct pg_conn*      _db,
                                       unsigned             _events,
                                       struct d_db_pending* _pending,
                                       struct d_db_rows*    _rows,
                                       int64_t*             _changes)
                                       D_NOTHROW;


//==============================================================================
// 4.  PIPELINES
//==============================================================================
// Pipeline mode is libpq 14's: with an older libpq, each call here is
// D_DB_UNSUPPORTED, and the status D_POSTGRES_PIPELINE_OFF. Every server the
// layer reaches takes a pipeline, which is the extended protocol's.


// 4.1    The mode
//------------------------------------------------------------------------------
// 4.1.1
// d_postgres_pipeline
//   enum: whether a connection is in pipeline mode, as libpq's
// PQpipelineStatus says.
enum d_postgres_pipeline
{
    D_POSTGRES_PIPELINE_OFF = 0,  // statements run one at a time, each
                                  // call waiting for its result
    D_POSTGRES_PIPELINE_ON,       // statements are queued, and their
                                  // results read in order
    D_POSTGRES_PIPELINE_ABORTED   // a statement failed: the server skips
                                  // the rest, to the next synchronization
                                  // point
};

// the connection's mode; D_POSTGRES_PIPELINE_OFF for NULL, or a libpq without
// pipelines
enum d_postgres_pipeline
       d_postgres_pipeline_status(const struct pg_conn* _db) D_NOTHROW;

// 4.2    Entering and leaving
//------------------------------------------------------------------------------
// entering and leaving -- _enter_pipeline puts the connection in pipeline
// mode, which needs it idle -- no results to read, no COPY -- and succeeds
// at once where it is in the mode already; rows a few at a time asked for
// before hold no further. _exit_pipeline takes it out, which needs every
// result read, up to the last synchronization point, and succeeds at once
// where it is out already. Each returns 0; 08000 when the connection is not
// as that needs; D_DB_UNSUPPORTED with a libpq before 14;
// D_DB_INVALID_ARGUMENT for NULL
int    d_postgres_enter_pipeline(struct pg_conn* _db) D_NOTHROW;
int    d_postgres_exit_pipeline(struct pg_conn* _db) D_NOTHROW;

// 4.3    Synchronizing and flushing
//------------------------------------------------------------------------------
/**
 * @brief Marks a synchronization point in the pipeline, and sends every
 *        statement queued.
 *
 * @note The statements between two synchronization points are one
 *       transaction, unless one of them begins a transaction of its own,
 *       which then outlasts the point; the point's own result, read by
 *       d_postgres_next_result, is D_DB_NOT_FOUND.
 *
 * @param[in] _db  the connection.
 * @return 0; 08000 outside pipeline mode; 08006 when the connection is lost;
 *         D_DB_UNSUPPORTED with a libpq before 14; D_DB_INVALID_ARGUMENT for
 *         a `NULL` connection.
 */
int    d_postgres_pipeline_sync(struct pg_conn* _db) D_NOTHROW;
/**
 * @brief Asks the server to send the results it has, without a
 *        synchronization point, and sends every statement queued.
 *
 * @note The server holds its results until a synchronization point, or
 *       until asked: d_postgres_next_result asks before it waits, but a
 *       program that waits on the socket, as d_postgres_is_busy has it,
 *       asks here first. The transaction goes on, to the next
 *       synchronization point.
 *
 * @param[in] _db  the connection.
 * @return 0; 08000 outside pipeline mode; 08006 when the connection is lost;
 *         D_DB_UNSUPPORTED with a libpq before 14; D_DB_INVALID_ARGUMENT for
 *         a `NULL` connection.
 */
int    d_postgres_pipeline_flush(struct pg_conn* _db) D_NOTHROW;


//==============================================================================
// 5.  LARGE OBJECTS
//==============================================================================
// A large object is a file the server keeps, by OID, outside any table.
// These copy one between it and a file of the client's, as libpq's lo_import
// and lo_export do, but over SQL -- lo_create, lo_put and lo_get, from
// PostgreSQL 9.4 -- 64 KiB a statement, so that every failure the server
// reports keeps its SQLSTATE, which libpq's own calls drop. A copy is one
// transaction's: inside the caller's it is part of it, and a failure fails
// it, as any statement's does; outside one the layer begins one of its own,
// committed when the copy is whole and rolled back otherwise -- after which
// the connection no longer describes the server's failure, though the
// SQLSTATE is still its own. A file the client cannot open, read or write is
// the layer's own answer: D_DB_NOT_FOUND for one that is not there, 58030
// otherwise, the C library's errno saying more.


// 5.1    Files in and out
//------------------------------------------------------------------------------
/**
 * @brief Copies a file of the client's into a new large object.
 *
 * @param[in]  _db    the connection.
 * @param[in]  _path  the file, as the client's C library names it.
 * @param[out] _oid   receives the large object's OID; written only on
 *                    success.
 * @return 0; D_DB_NOT_FOUND for a file that is not there; 58030 for one that
 *         cannot be opened or read; 25P02 in a failed transaction; 08000
 *         while a run holds the connection, or in pipeline mode; the
 *         server's SQLSTATE, packed; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 */
int    d_postgres_lo_import(struct pg_conn* _db,
                            const char*     _path,
                            unsigned int*   _oid) D_NOTHROW;
/**
 * @brief Copies a large object into a file of the client's, made or
 *        emptied first.
 *
 * @note A copy that fails part-way leaves the file with the bytes copied so
 *       far.
 *
 * @param[in] _db    the connection.
 * @param[in] _oid   the large object.
 * @param[in] _path  the file, as the client's C library names it.
 * @return 0; 42704 for an OID no large object has; 58030 for a file that
 *         cannot be made or written; D_DB_NOT_FOUND for a directory that is
 *         not there; otherwise as d_postgres_lo_import.
 */
int    d_postgres_lo_export(struct pg_conn* _db,
                            unsigned int    _oid,
                            const char*     _path) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_PROTOCOL_H
