/*******************************************************************************
* djinterp [c]                                               postgres_protocol.h
*
* What PostgreSQL's protocol offers beyond a statement and its reply: rows
* copied in, notifications, and statements sent without waiting.
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
* result is ready. Until the last result is read nothing more can be sent;
* a call that runs SQL meanwhile first waits for the results still to come
* and discards them, as libpq does.
*   RESULTS. As every module's (postgres_common.h): 0; the SQLSTATE of the
* failure, packed; 08006 for a connection that is lost, 08000 for one that
* cannot do what is asked; 53200 when memory runs out; D_DB_INVALID_ARGUMENT
* for an argument that cannot be right.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_protocol.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
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
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
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
 *                  run as d_postgres_exec runs them.
 * @return 0, the SQL sent; 08000 when the connection is still busy with SQL
 *         sent before, or a COPY; 08006 when it is lost; or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success nothing more can be sent until d_postgres_next_result
 *       answers D_DB_NOT_FOUND; a call that runs SQL first waits for the
 *       results still to come, and discards them.
 */
int    d_postgres_send_query(struct pg_conn* _db,
                             const char*     _sql) D_NOTHROW;

// 3.2    Results
//------------------------------------------------------------------------------
/**
 * @brief Reads the next result of the SQL sent, waiting for it.
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
 *         connection then ready; the SQLSTATE of the statement that failed,
 *         after which only D_DB_NOT_FOUND follows; D_DB_UNSUPPORTED for a
 *         COPY, which the layer ends, with every result after it; 53200 when
 *         the rows could not be built; D_DB_INVALID_ARGUMENT for a `NULL`
 *         connection or rows.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_postgres_next_result(struct pg_conn*   _db,
                              struct d_db_rows* _rows,
                              int64_t*          _changes) D_NOTHROW;
// waiting -- _consume_input reads what the server has sent, without waiting:
// 0, 08006 when the connection is lost, or D_DB_INVALID_ARGUMENT for NULL.
// _is_busy says whether the next result has yet to arrive, d_postgres_
// next_result then waiting for it; false for NULL, and when nothing was sent
bool   d_postgres_is_busy(struct pg_conn* _db) D_NOTHROW;
int    d_postgres_consume_input(struct pg_conn* _db) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_PROTOCOL_H
