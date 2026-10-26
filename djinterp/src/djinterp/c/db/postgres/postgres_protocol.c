/*******************************************************************************
* djinterp [c]                                               postgres_protocol.c
*
* Implementation of postgres_protocol.h.
*   libpq's own calls, each result read by the layer's rules: a COPY with the
* client that a statement starts unbidden is ended, as everywhere else in the
* layer, and a failure libpq reports without an SQLSTATE is 08006 for a lost
* connection and 08000 for any other. The rows of a result are typed as
* postgres_query.h describes, by the same builder. In pipeline mode libpq
* ends each statement's results with no result, which the layer reads past,
* and reports a synchronization point and a statement skipped by results of
* their own, which the layer turns into D_DB_NOT_FOUND and 25P02; the server
* sends what it has only at a synchronization point or when asked, and the
* layer asks -- a flush request -- before any wait for a result. libpq's
* pipeline calls are 14's and its chunked rows 17's, made only where both the
* header compiled against and the library loaded have them
* (d_postgres_client_supports).
*
*
* path:      /src/djinterp/c/db/postgres/postgres_protocol.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.24
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_protocol.h"  // corresponding header
// std
#include <errno.h>   // errno, ENOENT
#include <limits.h>  // INT_MAX
#include <stddef.h>  // NULL, size_t
#include <stdio.h>   // FILE, fopen, fread, fwrite, fclose, snprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, d_db_rows
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_POSTGRES_TURNED_ON
//   constant: the bit of a pending send's state that says the send turned
// libpq's non-blocking mode on, and so turns it off once it is written.
#define D_INTERNAL_POSTGRES_TURNED_ON 1u

/*
d_internal_postgres_trouble
  A failure libpq reports by a return value alone: the connection's state
says which of the layer's two codes it is.
*/
static int
d_internal_postgres_trouble(
    const PGconn* _db
)
{
    return (PQstatus(_db) == CONNECTION_BAD)
               ? D_POSTGRES_SQLSTATE_CONNECTION_FAILURE
               : D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;
}

/*
d_internal_postgres_copy_text
  Text libpq holds, copied into memory of the layer's, so that d_db_free
releases it on every C runtime; NULL when memory ran out.
*/
static char*
d_internal_postgres_copy_text(
    const char* _text
)
{
    struct d_internal_db_buffer copy = { NULL, 0u, 0u };
    const char* const           text = (_text) ? _text : "";

    // the text, terminated
    if ( (!d_internal_db_buffer_append(&copy, text, strlen(text))) ||
         (!d_internal_db_buffer_append(&copy, "", 1u)) )
    {
        d_internal_db_buffer_release(&copy);

        return NULL;
    }

    return copy.data;
}

/*
d_internal_postgres_ask
  In pipeline mode the server sends what it has only at a synchronization
point or when asked: a result yet to arrive is asked for, by a flush request,
so that a statement queued after the last synchronization point has its
results sent all the same. Returns 0, or the trouble that kept the request
from being made.
*/
static int
d_internal_postgres_ask(
    PGconn* _db,
    bool    _pipelined
)
{
#if defined(LIBPQ_HAS_PIPELINING)
    // a result the server holds until asked, asked for
    if ( (_pipelined)    &&
         (PQisBusy(_db)) &&
         (PQsendFlushRequest(_db) != 1) )
    {
        return d_internal_postgres_trouble(_db);
    }
#else
    (void)_db;
    (void)_pipelined;
#endif  // LIBPQ_HAS_PIPELINING

    return 0;
}

/*
d_internal_postgres_await
  The next result, waiting for it, after asking for it where that is needed;
NULL when libpq has none.
*/
static int
d_internal_postgres_await(
    PGconn*    _db,
    bool       _pipelined,
    PGresult** _next
)
{
    const int asked = d_internal_postgres_ask(_db, _pipelined);

    *_next = (asked == 0) ? PQgetResult(_db) : NULL;

    return asked;
}

/*
d_internal_postgres_count
  How many rows a result's command tag counts -- INSERT 0 3, COPY 5 -- or 0
for one that counts none.
*/
static int64_t
d_internal_postgres_count(
    PGresult* _result
)
{
    int64_t           count  = 0;
    const char* const tuples = PQcmdTuples(_result);

    // a count, or nothing
    if ( (tuples[0] == '\0') ||
         (!d_internal_db_parse_integer(tuples, &count)) )
    {
        return 0;
    }

    return count;
}

//==============================================================================
// 1.  COPY
//==============================================================================

// 1.1    Copying rows in
//------------------------------------------------------------------------------
/*
d_postgres_copy_in_start
  PQexec stops at a COPY, which waits; any other success is a statement that
was no COPY FROM STDIN, and a COPY to the client is ended as the layer ends
one everywhere.
*/
int
d_postgres_copy_in_start(
    struct pg_conn* _db,
    const char*     _sql
)
{
    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int ready = d_internal_postgres_ready(_db);

    // a run holds the connection
    if (ready != 0)
    {
        return ready;
    }

    PGresult* const started = PQexec(_db, _sql);

    // the server waiting for rows
    if ( (started) &&
         (PQresultStatus(started) == PGRES_COPY_IN) )
    {
        PQclear(started);

        return 0;
    }

    const int result = d_internal_postgres_take(_db, started, NULL);

    return (result == 0) ? D_DB_INVALID_ARGUMENT : result;
}

/*
d_postgres_copy_in_data
  PQputCopyData counts its bytes in an int, so a larger buffer goes in
pieces; in blocking mode it answers 1 or -1, and -1 for no COPY at all.
*/
int
d_postgres_copy_in_data(
    struct pg_conn* _db,
    const void*     _data,
    size_t          _size
)
{
    const char* bytes = (const char*)_data;
    size_t      left  = _size;

    // parameter validation
    if ( (!_db) ||
         ( (!_data) &&
           (_size != 0u) ) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // the bytes, a piece an int can count at a time
    while (left > 0u)
    {
        const size_t piece = (left < (size_t)INT_MAX) ? left
                                                      : (size_t)INT_MAX;

        if (PQputCopyData(_db, bytes, (int)piece) != 1)
        {
            return d_internal_postgres_trouble(_db);
        }

        bytes += piece;
        left  -= piece;
    }

    return 0;
}

/*
d_postgres_copy_in_end
  The end, or the message that abandons the COPY; then the COPY's result,
which counts its rows, and every result after it, to the last: a second
COPY the SQL started is ended, and reported as D_DB_UNSUPPORTED, as the
first failure of any statement after the COPY is.
*/
int
d_postgres_copy_in_end(
    struct pg_conn* _db,
    const char*     _error,
    int64_t*        _changes
)
{
    int64_t count  = 0;
    int     result = 0;
    bool    first  = true;

    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // no COPY in progress, or the connection lost
    if (PQputCopyEnd(_db, _error) != 1)
    {
        return d_internal_postgres_trouble(_db);
    }

    // the COPY's result, then the rest
    for (PGresult* next = PQgetResult(_db); next; next = PQgetResult(_db))
    {
        const ExecStatusType status = PQresultStatus(next);

        if (d_internal_postgres_copying(status))
        {
            PQclear(next);
            d_internal_postgres_end_copy(_db, status);
            result = (result == 0) ? D_DB_UNSUPPORTED : result;

            break;
        }

        const int code = d_internal_postgres_result(_db, next);

        if ( (first) &&
             (code == 0) )
        {
            count = d_internal_postgres_count(next);
        }

        result = (result == 0) ? code : result;
        first  = false;
        PQclear(next);
    }

    // written only on success
    if ( (result == 0) &&
         (_changes) )
    {
        *_changes = count;
    }

    return result;
}

//==============================================================================
// 2.  NOTIFICATIONS
//==============================================================================

// 2.1    Channels
//------------------------------------------------------------------------------
/*
d_postgres_listen
  The channel quoted as a name, so that its case is kept.
*/
int
d_postgres_listen(
    struct pg_conn* _db,
    const char*     _channel
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_name_ok(_channel)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(_db, NULL, "LISTEN %I", _channel);
}

/*
d_postgres_unlisten
  UNLISTEN *, for every channel, when none is named.
*/
int
d_postgres_unlisten(
    struct pg_conn* _db,
    const char*     _channel
)
{
    // parameter validation
    if ( (!_db) ||
         ( (_channel) &&
           (_channel[0] == '\0') ) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (_channel)
               ? d_internal_postgres_run(_db, NULL, "UNLISTEN %I", _channel)
               : d_internal_postgres_send(_db, "UNLISTEN *", NULL);
}

/*
d_postgres_notify
  pg_notify, its channel and payload passed apart from the SQL; a NULL
payload is "", since pg_notify ignores a call with a NULL argument.
*/
int
d_postgres_notify(
    struct pg_conn* _db,
    const char*     _channel,
    const char*     _payload
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_name_ok(_channel)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int ready = d_internal_postgres_ready(_db);

    // a run holds the connection
    if (ready != 0)
    {
        return ready;
    }

    const char* const values[2] = { _channel, (_payload) ? _payload : "" };

    return d_internal_postgres_take(
               _db,
               PQexecParams(_db,
                            "SELECT pg_catalog.pg_notify($1, $2)",
                            2,
                            NULL,
                            values,
                            NULL,
                            NULL,
                            0),
               NULL);
}

// 2.2    Arrivals
//------------------------------------------------------------------------------
/*
d_postgres_next_notification
  What the server has sent is read first, without waiting, so that a
notification already on the socket is found.
*/
int
d_postgres_next_notification(
    struct pg_conn* _db,
    char**          _channel,
    char**          _payload,
    int*            _pid
)
{
    // parameter validation
    if ( (!_db) ||
         (!_channel) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // what has arrived
    if (!PQconsumeInput(_db))
    {
        return d_internal_postgres_trouble(_db);
    }

    PGnotify* const note = PQnotifies(_db);

    if (!note)
    {
        return D_DB_NOT_FOUND;
    }

    char* const channel = d_internal_postgres_copy_text(note->relname);
    char* const payload = (_payload)
                              ? d_internal_postgres_copy_text(note->extra)
                              : NULL;
    const int   pid     = note->be_pid;

    PQfreemem(note);

    // both copies, or neither
    if ( (!channel) ||
         ( (_payload) &&
           (!payload) ) )
    {
        d_db_free(channel);
        d_db_free(payload);

        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    *_channel = channel;

    if (_payload)
    {
        *_payload = payload;
    }

    if (_pid)
    {
        *_pid = pid;
    }

    return 0;
}

//==============================================================================
// 3.  STATEMENTS SENT
//==============================================================================

// 3.1    Sending
//------------------------------------------------------------------------------
/*
d_postgres_send_query
  PQsendQuery, libpq's simple query, which may hold several statements; it
refuses while SQL sent before is unfinished. A pipeline takes only the
extended protocol, which PQsendQueryParams speaks, with no parameters.
*/
int
d_postgres_send_query(
    struct pg_conn* _db,
    const char*     _sql
)
{
    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int sent = (d_internal_postgres_pipelined(_db))
                         ? PQsendQueryParams(_db,
                                             _sql,
                                             0,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             0)
                         : PQsendQuery(_db, _sql);

    return (sent == 1) ? 0 : d_internal_postgres_trouble(_db);
}

// 3.2    Results
//------------------------------------------------------------------------------
/*
d_internal_postgres_give
  A result libpq handed over, as d_postgres_next_result gives it: none left
is D_DB_NOT_FOUND. In pipeline mode a synchronization point is
D_DB_NOT_FOUND too, a statement a failure skipped is the layer's own 25P02,
and libpq ends each statement's results with no result, read at once after
the statement's last success -- libpq answers it without waiting -- so that
the next statement's rows can be asked for a few at a time. A result's count
is read before its rows are built, which frees it.
*/
static int
d_internal_postgres_give(
    PGconn*           _db,
    PGresult*         _next,
    bool              _pipelined,
    struct d_db_rows* _rows,
    int64_t*          _changes
)
{
    PGresult* taken = NULL;

    // every result read
    if (!_next)
    {
        return D_DB_NOT_FOUND;
    }

    const ExecStatusType status = PQresultStatus(_next);

#if defined(LIBPQ_HAS_PIPELINING)
    // a synchronization point, and a statement a failure skipped, whose
    // answer is the layer's own
    switch (status)
    {
        case PGRES_PIPELINE_SYNC:
            PQclear(_next);
            d_internal_postgres_heard(_db);

            return D_DB_NOT_FOUND;

        case PGRES_PIPELINE_ABORTED:
            PQclear(_next);
            PQclear(PQgetResult(_db));
            d_internal_postgres_heard(_db);

            return d_internal_postgres_answer(
                       _db,
                       D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);

        default:
            break;
    }
#endif  // LIBPQ_HAS_PIPELINING

    const int result = d_internal_postgres_take(_db, _next, &taken);

    // a failure, or a COPY ended
    if (result != 0)
    {
        return result;
    }

    // in a pipeline, the end of the statement's results, after its last
    if ( (_pipelined)                    &&
#if defined(LIBPQ_HAS_CHUNK_MODE)
         (status != PGRES_TUPLES_CHUNK)  &&
#endif  // LIBPQ_HAS_CHUNK_MODE
         (status != PGRES_SINGLE_TUPLE) )
    {
        PQclear(PQgetResult(_db));
    }

    const int64_t count = d_internal_postgres_count(taken);
    const int     built = d_internal_postgres_collect_rows(taken, _rows);

    // written only on success
    if ( (built == 0) &&
         (_changes) )
    {
        *_changes = count;
    }

    return built;
}

/*
d_postgres_next_result
  One result a call (d_internal_postgres_give). In pipeline mode a result
yet to arrive is asked for first (d_internal_postgres_ask); after the
server's failure, the end of the failed statement's results is read only
when the next result is asked for, since reading it clears the connection's
message, which must describe the failure until then.
*/
int
d_postgres_next_result(
    struct pg_conn*   _db,
    struct d_db_rows* _rows,
    int64_t*          _changes
)
{
    PGresult* next = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_rows) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const bool pipelined = d_internal_postgres_pipelined(_db);
    int        asked     = d_internal_postgres_await(_db, pipelined, &next);

    // in a pipeline, the end of a failed statement's results, and the next
    if ( (asked == 0) &&
         (!next)      &&
         (pipelined) )
    {
        asked = d_internal_postgres_await(_db, pipelined, &next);
    }

    // the server not asked
    if (asked != 0)
    {
        return asked;
    }

    return d_internal_postgres_give(_db, next, pipelined, _rows, _changes);
}

/*
d_postgres_is_busy
  PQisBusy reads nothing from the socket; d_postgres_consume_input does.
*/
bool
d_postgres_is_busy(
    struct pg_conn* _db
)
{
    return ( (_db) &&
             (PQisBusy(_db) != 0) );
}

/*
d_postgres_consume_input
  PQconsumeInput answers 0 only for trouble on the connection.
*/
int
d_postgres_consume_input(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (PQconsumeInput(_db) == 1) ? 0 : d_internal_postgres_trouble(_db);
}

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/*
d_postgres_set_chunked_rows
  One row is libpq's single-row mode, the same in every libpq; more is
PQsetChunkedRowsMode. Either refuses, answering 0, unless a statement's
results are still to come and libpq has read none of them: straight after a
send, or after a statement's last result -- in a pipeline, once the end of
its results is read too, which d_postgres_next_result reads at once after a
success.
*/
int
d_postgres_set_chunked_rows(
    struct pg_conn* _db,
    int             _rows
)
{
    int set = 0;

    // parameter validation
    if ( (!_db) ||
         (_rows < 1) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (_rows == 1)
    {
        set = PQsetSingleRowMode(_db);
    }
    else
    {
        // more than one row, from libpq 17
        if (!d_postgres_client_supports(D_POSTGRES_CLIENT_CHUNKED_ROWS))
        {
            return D_DB_UNSUPPORTED;
        }

#if defined(LIBPQ_HAS_CHUNK_MODE)
        set = PQsetChunkedRowsMode(_db, _rows);
#endif  // LIBPQ_HAS_CHUNK_MODE
    }

    return (set == 1) ? 0 : D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;
}

// 3.4    Without blocking
//------------------------------------------------------------------------------
/*
d_internal_postgres_sending
  What is left of a send, written as far as the socket takes it; a send
that finishes, or fails, leaves nothing pending.
*/
static int
d_internal_postgres_sending(
    PGconn*              _db,
    unsigned             _events,
    struct d_db_pending* _pending
)
{
    const int result = d_internal_postgres_flush(
                           _db,
                           _events,
                           ((_pending->state &
                             D_INTERNAL_POSTGRES_TURNED_ON) != 0u),
                           _pending);

    // finished, or failed
    if (result != D_DB_WAIT)
    {
        d_internal_postgres_settled(_pending);
    }

    return result;
}

/*
d_internal_postgres_next_arrived
  The next result once it has arrived (d_internal_postgres_give), outside
pipeline mode; until then, a wait for the socket to be readable.
*/
static int
d_internal_postgres_next_arrived(
    PGconn*              _db,
    struct d_db_pending* _pending,
    struct d_db_rows*    _rows,
    int64_t*             _changes
)
{
    const int arrived = d_internal_postgres_arrived(_db, _pending);

    // still to come, or trouble
    if (arrived != 0)
    {
        if (arrived != D_DB_WAIT)
        {
            d_internal_postgres_settled(_pending);
        }

        return arrived;
    }

    d_internal_postgres_settled(_pending);

    return d_internal_postgres_give(_db,
                                    PQgetResult(_db),
                                    false,
                                    _rows,
                                    _changes);
}

/*
d_postgres_send_query_start
  PQsendQuery in non-blocking mode queues what the socket cannot take at
once, and PQflush writes the rest as the socket takes it; the mode is turned
on only where it is off, so that a program's own choice of it stands.
*/
int
d_postgres_send_query_start(
    struct pg_conn*      _db,
    const char*          _sql,
    struct d_db_pending* _pending
)
{
    // nothing pending yet, whatever follows
    if (_pending)
    {
        d_internal_postgres_settled(_pending);
    }

    // parameter validation
    if ( (!_db)  ||
         (!_sql) ||
         (!_pending) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // pipeline mode queues without waiting already
    if (d_internal_postgres_pipelined(_db))
    {
        return d_internal_postgres_answer(
                   _db,
                   D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    }

    const bool turned = d_internal_postgres_nonblocking_on(_db);
    const int  sent   = PQsendQuery(_db, _sql);

    // libpq has answered, by its return value and its message
    d_internal_postgres_heard(_db);

    // refused: busy with SQL sent before, or the connection lost
    if (sent != 1)
    {
        if (turned)
        {
            (void)PQsetnonblocking(_db, 0);
        }

        return d_internal_postgres_trouble(_db);
    }

    _pending->state = (turned) ? D_INTERNAL_POSTGRES_TURNED_ON : 0u;

    return d_internal_postgres_sending(_db, 0u, _pending);
}

/*
d_postgres_send_query_continue
  The rest of the send (d_internal_postgres_sending).
*/
int
d_postgres_send_query_continue(
    struct pg_conn*      _db,
    unsigned             _events,
    struct d_db_pending* _pending
)
{
    // parameter validation
    if ( (!_db) ||
         (!_pending) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_sending(_db, _events, _pending);
}

/*
d_postgres_next_result_start
  The next result, once it has arrived (d_internal_postgres_next_arrived).
*/
int
d_postgres_next_result_start(
    struct pg_conn*      _db,
    struct d_db_pending* _pending,
    struct d_db_rows*    _rows,
    int64_t*             _changes
)
{
    // nothing pending yet, whatever follows
    if (_pending)
    {
        d_internal_postgres_settled(_pending);
    }

    // parameter validation
    if ( (!_db)      ||
         (!_pending) ||
         (!_rows) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // pipeline mode reads its results its own way
    if (d_internal_postgres_pipelined(_db))
    {
        return d_internal_postgres_answer(
                   _db,
                   D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    }

    return d_internal_postgres_next_arrived(_db, _pending, _rows, _changes);
}

/*
d_postgres_next_result_continue
  What came is read whatever the program says came: PQconsumeInput never
waits, and finds nothing when nothing came.
*/
int
d_postgres_next_result_continue(
    struct pg_conn*      _db,
    unsigned             _events,
    struct d_db_pending* _pending,
    struct d_db_rows*    _rows,
    int64_t*             _changes
)
{
    (void)_events;

    // parameter validation
    if ( (!_db)      ||
         (!_pending) ||
         (!_rows) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // pipeline mode reads its results its own way
    if (d_internal_postgres_pipelined(_db))
    {
        d_internal_postgres_settled(_pending);

        return d_internal_postgres_answer(
                   _db,
                   D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    }

    return d_internal_postgres_next_arrived(_db, _pending, _rows, _changes);
}

//==============================================================================
// 4.  PIPELINES
//==============================================================================

// 4.1    The mode
//------------------------------------------------------------------------------
/*
d_postgres_pipeline_status
  libpq's three states, one for one.
*/
enum d_postgres_pipeline
d_postgres_pipeline_status(
    const struct pg_conn* _db
)
{
    // a connection, in pipeline mode
    if ( (!_db) ||
         (!d_internal_postgres_pipelined(_db)) )
    {
        return D_POSTGRES_PIPELINE_OFF;
    }

#if defined(LIBPQ_HAS_PIPELINING)
    return (PQpipelineStatus(_db) == PQ_PIPELINE_ABORTED)
               ? D_POSTGRES_PIPELINE_ABORTED
               : D_POSTGRES_PIPELINE_ON;
#else
    return D_POSTGRES_PIPELINE_OFF;
#endif  // LIBPQ_HAS_PIPELINING
}

// 4.2    Entering and leaving
//------------------------------------------------------------------------------
/*
d_postgres_enter_pipeline
  PQenterPipelineMode answers 0 only for a connection that is not idle, its
message saying so; one in pipeline mode already enters it again, harmlessly.
  Rows a few at a time asked for outside the mode stay asked for, in libpq,
until the next send outside it resets them -- which no send inside it does
(libpq 14 to 19 alike), so the pipeline's first statement would have them
too. They are reset first, by a send libpq refuses before anything is sent:
a NULL statement, whose refusal is the connection's message until the
pipeline's first send clears it. A connection that is not idle is left to
PQenterPipelineMode's refusal alone.
*/
int
d_postgres_enter_pipeline(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a libpq with pipelines
    if (!d_postgres_client_supports(D_POSTGRES_CLIENT_PIPELINE))
    {
        return D_DB_UNSUPPORTED;
    }

#if defined(LIBPQ_HAS_PIPELINING)
    // rows a few at a time asked for outside the mode, reset
    if ( (!d_internal_postgres_pipelined(_db))         &&
         (PQstatus(_db) == CONNECTION_OK)              &&
         (PQtransactionStatus(_db) != PQTRANS_ACTIVE) )
    {
        (void)PQsendQueryParams(_db, NULL, 0, NULL, NULL, NULL, NULL, 0);
    }

    return (PQenterPipelineMode(_db) == 1)
               ? 0
               : D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;
#else
    return D_DB_UNSUPPORTED;
#endif  // LIBPQ_HAS_PIPELINING
}

/*
d_postgres_exit_pipeline
  PQexitPipelineMode answers 0 only while results are still to be read, its
message saying so; one not in pipeline mode leaves it again, harmlessly.
*/
int
d_postgres_exit_pipeline(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a libpq with pipelines
    if (!d_postgres_client_supports(D_POSTGRES_CLIENT_PIPELINE))
    {
        return D_DB_UNSUPPORTED;
    }

#if defined(LIBPQ_HAS_PIPELINING)
    return (PQexitPipelineMode(_db) == 1)
               ? 0
               : D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;
#else
    return D_DB_UNSUPPORTED;
#endif  // LIBPQ_HAS_PIPELINING
}

// 4.3    Synchronizing and flushing
//------------------------------------------------------------------------------
/*
d_postgres_pipeline_sync
  PQpipelineSync refuses outside pipeline mode, and fails when what is queued
cannot be sent, which the connection's state tells apart.
*/
int
d_postgres_pipeline_sync(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a libpq with pipelines
    if (!d_postgres_client_supports(D_POSTGRES_CLIENT_PIPELINE))
    {
        return D_DB_UNSUPPORTED;
    }

#if defined(LIBPQ_HAS_PIPELINING)
    return (PQpipelineSync(_db) == 1) ? 0 : d_internal_postgres_trouble(_db);
#else
    return D_DB_UNSUPPORTED;
#endif  // LIBPQ_HAS_PIPELINING
}

/*
d_postgres_pipeline_flush
  PQsendFlushRequest only queues its message, which PQflush then sends with
whatever else is queued; outside pipeline mode the server sends every result
unasked, and the request is refused, as PQpipelineSync is.
*/
int
d_postgres_pipeline_flush(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a libpq with pipelines
    if (!d_postgres_client_supports(D_POSTGRES_CLIENT_PIPELINE))
    {
        return D_DB_UNSUPPORTED;
    }

    // pipeline mode, where the server holds its results
    if (!d_internal_postgres_pipelined(_db))
    {
        return D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;
    }

#if defined(LIBPQ_HAS_PIPELINING)
    return ( (PQsendFlushRequest(_db) == 1) &&
             (PQflush(_db) == 0) )
               ? 0
               : d_internal_postgres_trouble(_db);
#else
    return D_DB_UNSUPPORTED;
#endif  // LIBPQ_HAS_PIPELINING
}

//==============================================================================
// 5.  LARGE OBJECTS
//==============================================================================

// D_INTERNAL_POSTGRES_LO_CHUNK
//   constant: the bytes one statement copies, 64 KiB.
#define D_INTERNAL_POSTGRES_LO_CHUNK 65536

/**
 * @brief The layer's answer for a file the C library could not open, read
 *        or write.
 *
 * @param[in] _db     the connection, for the answer's mark.
 * @param[in] _error  errno, as the failure left it.
 * @return D_DB_NOT_FOUND for ENOENT; 58030, as the layer's own, otherwise.
 */
static int
d_internal_postgres_file_failure(
    const PGconn* _db,
    int           _error
)
{
    return (_error == ENOENT)
               ? D_DB_NOT_FOUND
               : d_internal_postgres_answer(_db, D_POSTGRES_SQLSTATE_IO_ERROR);
}

/**
 * @brief Begins the transaction a copy needs, unless the caller's holds it.
 *
 * @param[in]  _db   the connection.
 * @param[out] _own  whether the layer began one, which it then ends.
 * @return 0; 08000 while a run holds the connection or in pipeline mode;
 *         25P02 in a failed transaction; or BEGIN's failure.
 */
static int
d_internal_postgres_lo_begin(
    PGconn* _db,
    bool*   _own
)
{
    *_own = false;

    // a pipeline's statements cannot wait for one another
    if (d_internal_postgres_pipelined(_db))
    {
        return d_internal_postgres_answer(
                   _db,
                   D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    }

    const int ready = d_internal_postgres_ready(_db);

    // a run holds the connection
    if (ready != 0)
    {
        return ready;
    }

    const PGTransactionStatusType status = PQtransactionStatus(_db);

    // a failed transaction runs nothing more
    if (status == PQTRANS_INERROR)
    {
        return d_internal_postgres_answer(
                   _db,
                   D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION);
    }

    // the caller's transaction holds the copy
    if (status != PQTRANS_IDLE)
    {
        return 0;
    }

    const int begun = d_internal_postgres_send(_db, "BEGIN", NULL);

    *_own = (begun == 0);

    return begun;
}

/**
 * @brief Ends the transaction the layer began for a copy: committed when the
 *        copy is whole, rolled back otherwise.
 *
 * @param[in] _db      the connection.
 * @param[in] _own     whether the layer began it.
 * @param[in] _result  the copy's result.
 * @return the copy's failure, or COMMIT's.
 */
static int
d_internal_postgres_lo_end(
    PGconn* _db,
    bool    _own,
    int     _result
)
{
    // the caller's transaction, or none begun
    if (!_own)
    {
        return _result;
    }

    // the copy whole
    if (_result == 0)
    {
        return d_internal_postgres_send(_db, "COMMIT", NULL);
    }

    // a lost connection takes the transaction with it
    if (PQstatus(_db) != CONNECTION_BAD)
    {
        (void)d_internal_postgres_send(_db, "ROLLBACK", NULL);
    }

    return _result;
}

/**
 * @brief Runs one statement of a copy, its values given as text or bytes.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _sql      the statement.
 * @param[in]  _count    its values.
 * @param[in]  _values   each value.
 * @param[in]  _lengths  each value's length, for bytes.
 * @param[in]  _formats  1 for bytes, 0 for text.
 * @param[out] _result   the statement's rows, in bytes.
 * @return 0, or as d_internal_postgres_take.
 */
static int
d_internal_postgres_lo_run(
    PGconn*            _db,
    const char*        _sql,
    int                _count,
    const char* const* _values,
    const int*         _lengths,
    const int*         _formats,
    PGresult**         _result
)
{
    return d_internal_postgres_take(_db,
                                    PQexecParams(_db, _sql, _count, NULL,
                                                 _values, _lengths, _formats,
                                                 1),
                                    _result);
}

/**
 * @brief Copies an open file into a large object it makes, a chunk a
 *        statement.
 *
 * @param[in]  _db    the connection, inside a transaction.
 * @param[in]  _file  the file, open for reading.
 * @param[out] _oid   the large object made.
 * @return 0, or the first failure.
 */
static int
d_internal_postgres_lo_put_file(
    PGconn*       _db,
    FILE*         _file,
    unsigned int* _oid
)
{
    PGresult* made = NULL;
    int       result = d_internal_postgres_lo_run(_db,
                                                  "SELECT lo_create(0)",
                                                  0, NULL, NULL, NULL,
                                                  &made);

    // the new object's OID, in network order
    if (result == 0)
    {
        const unsigned char* const bytes =
            (const unsigned char*)PQgetvalue(made, 0, 0);

        *_oid = ( ((unsigned int)bytes[0] << 24) |
                  ((unsigned int)bytes[1] << 16) |
                  ((unsigned int)bytes[2] << 8)  |
                   (unsigned int)bytes[3] );
        PQclear(made);
    }

    char* const chunk = (result == 0)
                            ? (char*)malloc(D_INTERNAL_POSTGRES_LO_CHUNK)
                            : NULL;

    // memory for the chunks
    if ( (result == 0) &&
         (!chunk) )
    {
        result = D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    char   oid_text[16]    = "";
    char   offset_text[24] = "";
    size_t offset          = 0u;

    (void)snprintf(oid_text, sizeof(oid_text), "%u", *_oid);

    // a chunk a statement, to the file's end
    while (result == 0)
    {
        const size_t read = fread(chunk, 1u, D_INTERNAL_POSTGRES_LO_CHUNK,
                                  _file);

        // the file could not be read
        if (ferror(_file))
        {
            result = d_internal_postgres_file_failure(_db, errno);

            break;
        }

        // the end of the file
        if (read == 0u)
        {
            break;
        }

        (void)snprintf(offset_text, sizeof(offset_text), "%zu", offset);

        const char* const values[3]  = { oid_text, offset_text, chunk };
        const int         lengths[3] = { 0, 0, (int)read };
        const int         formats[3] = { 0, 0, 1 };

        result  = d_internal_postgres_lo_run(
                      _db,
                      "SELECT lo_put($1::oid, $2::bigint, $3::bytea)",
                      3, values, lengths, formats, NULL);
        offset += read;
    }

    free(chunk);

    return result;
}

/*
d_postgres_lo_import
  The file is opened before anything reaches the server, so that a file that
cannot be had leaves the session as it was.
*/
int
d_postgres_lo_import(
    PGconn*       _db,
    const char*   _path,
    unsigned int* _oid
)
{
    // parameter validation
    if ( (!_db)   ||
         (!_path) ||
         (!_oid) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    FILE* const file = fopen(_path, "rb");

    // a file the client cannot open
    if (!file)
    {
        return d_internal_postgres_file_failure(_db, errno);
    }

    bool         own    = false;
    unsigned int oid    = 0u;
    int          result = d_internal_postgres_lo_begin(_db, &own);

    // the copy, in its transaction
    if (result == 0)
    {
        result = d_internal_postgres_lo_put_file(_db, file, &oid);
        result = d_internal_postgres_lo_end(_db, own, result);
    }

    (void)fclose(file);

    // the object, once its transaction has it
    if (result == 0)
    {
        *_oid = oid;
    }

    return result;
}

/**
 * @brief Copies a large object into an open file, a chunk a statement.
 *
 * @param[in] _db    the connection, inside a transaction.
 * @param[in] _oid   the large object.
 * @param[in] _file  the file, open for writing.
 * @return 0, or the first failure.
 */
static int
d_internal_postgres_lo_get_file(
    PGconn*      _db,
    unsigned int _oid,
    FILE*        _file
)
{
    char   oid_text[16]    = "";
    char   offset_text[24] = "";
    char   length_text[16] = "";
    size_t offset          = 0u;
    int    result          = 0;

    (void)snprintf(oid_text, sizeof(oid_text), "%u", _oid);
    (void)snprintf(length_text, sizeof(length_text), "%d",
                   D_INTERNAL_POSTGRES_LO_CHUNK);

    // a chunk a statement, until one comes short
    for (size_t got = D_INTERNAL_POSTGRES_LO_CHUNK;
         ( (result == 0) &&
           (got == D_INTERNAL_POSTGRES_LO_CHUNK) );
         offset += got)
    {
        (void)snprintf(offset_text, sizeof(offset_text), "%zu", offset);

        const char* const values[3] = { oid_text, offset_text, length_text };
        PGresult*         rows      = NULL;

        result = d_internal_postgres_lo_run(
                     _db,
                     "SELECT lo_get($1::oid, $2::bigint, $3::int)",
                     3, values, NULL, NULL, &rows);

        // the statement failed: nothing read
        if (result != 0)
        {
            break;
        }

        got = (size_t)PQgetlength(rows, 0, 0);

        // the chunk, into the file
        if ( (got > 0u) &&
             (fwrite(PQgetvalue(rows, 0, 0), 1u, got, _file) != got) )
        {
            result = d_internal_postgres_file_failure(_db, errno);
        }

        PQclear(rows);
    }

    return result;
}

/*
d_postgres_lo_export
  The file is made before anything reaches the server, as lo_import opens its
own first; the object's absence is the server's 42704.
*/
int
d_postgres_lo_export(
    PGconn*      _db,
    unsigned int _oid,
    const char*  _path
)
{
    // parameter validation
    if ( (!_db) ||
         (!_path) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    FILE* const file = fopen(_path, "wb");

    // a file the client cannot make
    if (!file)
    {
        return d_internal_postgres_file_failure(_db, errno);
    }

    bool own    = false;
    int  result = d_internal_postgres_lo_begin(_db, &own);

    // the copy, in its transaction
    if (result == 0)
    {
        result = d_internal_postgres_lo_get_file(_db, _oid, file);
        result = d_internal_postgres_lo_end(_db, own, result);
    }

    // the file, whole or as far as it came
    if ( (fclose(file) != 0) &&
         (result == 0) )
    {
        result = d_internal_postgres_file_failure(_db, errno);
    }

    return result;
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false where it cannot fail.

/*
d_postgres_copy_in_start
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_copy_in_start(
    struct pg_conn* _db,
    const char*     _sql
)
{
    (void)_db;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_copy_in_data
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_copy_in_data(
    struct pg_conn* _db,
    const void*     _data,
    size_t          _size
)
{
    (void)_db;
    (void)_data;
    (void)_size;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_copy_in_end
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_copy_in_end(
    struct pg_conn* _db,
    const char*     _error,
    int64_t*        _changes
)
{
    (void)_db;
    (void)_error;
    (void)_changes;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_listen
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_listen(
    struct pg_conn* _db,
    const char*     _channel
)
{
    (void)_db;
    (void)_channel;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_unlisten
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_unlisten(
    struct pg_conn* _db,
    const char*     _channel
)
{
    (void)_db;
    (void)_channel;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_notify
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_notify(
    struct pg_conn* _db,
    const char*     _channel,
    const char*     _payload
)
{
    (void)_db;
    (void)_channel;
    (void)_payload;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_next_notification
  Without libpq, D_DB_UNSUPPORTED, and nothing written.
*/
int
d_postgres_next_notification(
    struct pg_conn* _db,
    char**          _channel,
    char**          _payload,
    int*            _pid
)
{
    (void)_db;
    (void)_channel;
    (void)_payload;
    (void)_pid;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_send_query
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_send_query(
    struct pg_conn* _db,
    const char*     _sql
)
{
    (void)_db;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_next_result
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_next_result(
    struct pg_conn*   _db,
    struct d_db_rows* _rows,
    int64_t*          _changes
)
{
    (void)_db;
    (void)_rows;
    (void)_changes;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_is_busy
  Without libpq, false.
*/
bool
d_postgres_is_busy(
    struct pg_conn* _db
)
{
    (void)_db;

    return false;
}

/*
d_postgres_consume_input
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_consume_input(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_set_chunked_rows
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_chunked_rows(
    struct pg_conn* _db,
    int             _rows
)
{
    (void)_db;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_internal_postgres_nothing_pending
  What a start or continuation leaves without libpq: nothing pending.
D_DB_UNSUPPORTED.
*/
static int
d_internal_postgres_nothing_pending(
    struct d_db_pending* _pending
)
{
    if (_pending)
    {
        _pending->socket     = D_DB_SOCKET_NONE;
        _pending->events     = 0u;
        _pending->timeout_ms = 0u;
        _pending->deadline   = 0.0;
        _pending->state      = 0u;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_send_query_start
  Without libpq, D_DB_UNSUPPORTED, nothing pending.
*/
int
d_postgres_send_query_start(
    struct pg_conn*      _db,
    const char*          _sql,
    struct d_db_pending* _pending
)
{
    (void)_db;
    (void)_sql;

    return d_internal_postgres_nothing_pending(_pending);
}

/*
d_postgres_send_query_continue
  Without libpq, D_DB_UNSUPPORTED, nothing pending.
*/
int
d_postgres_send_query_continue(
    struct pg_conn*      _db,
    unsigned             _events,
    struct d_db_pending* _pending
)
{
    (void)_db;
    (void)_events;

    return d_internal_postgres_nothing_pending(_pending);
}

/*
d_postgres_next_result_start
  Without libpq, D_DB_UNSUPPORTED, nothing pending, and no rows.
*/
int
d_postgres_next_result_start(
    struct pg_conn*      _db,
    struct d_db_pending* _pending,
    struct d_db_rows*    _rows,
    int64_t*             _changes
)
{
    (void)_db;
    (void)_rows;
    (void)_changes;

    return d_internal_postgres_nothing_pending(_pending);
}

/*
d_postgres_next_result_continue
  Without libpq, D_DB_UNSUPPORTED, nothing pending, and no rows.
*/
int
d_postgres_next_result_continue(
    struct pg_conn*      _db,
    unsigned             _events,
    struct d_db_pending* _pending,
    struct d_db_rows*    _rows,
    int64_t*             _changes
)
{
    (void)_db;
    (void)_events;
    (void)_rows;
    (void)_changes;

    return d_internal_postgres_nothing_pending(_pending);
}

/*
d_postgres_pipeline_status
  Without libpq there is no pipeline.
*/
enum d_postgres_pipeline
d_postgres_pipeline_status(
    const struct pg_conn* _db
)
{
    (void)_db;

    return D_POSTGRES_PIPELINE_OFF;
}

/*
d_postgres_enter_pipeline
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_enter_pipeline(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_exit_pipeline
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_exit_pipeline(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_pipeline_sync
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_pipeline_sync(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_pipeline_flush
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_pipeline_flush(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_lo_import
  Without libpq there is no server: D_DB_UNSUPPORTED, and nothing written.
*/
int
d_postgres_lo_import(
    struct pg_conn* _db,
    const char*     _path,
    unsigned int*   _oid
)
{
    (void)_db;
    (void)_path;
    (void)_oid;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_lo_export
  Without libpq there is no server: D_DB_UNSUPPORTED.
*/
int
d_postgres_lo_export(
    struct pg_conn* _db,
    unsigned int    _oid,
    const char*     _path
)
{
    (void)_db;
    (void)_oid;
    (void)_path;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_POSTGRES
