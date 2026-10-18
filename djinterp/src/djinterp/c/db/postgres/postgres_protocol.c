/*******************************************************************************
* djinterp [c]                                               postgres_protocol.c
*
* Implementation of postgres_protocol.h.
*   libpq's own calls, each result read by the layer's rules: a COPY with the
* client that a statement starts unbidden is ended, as everywhere else in the
* layer, and a failure libpq reports without an SQLSTATE is 08006 for a lost
* connection and 08000 for any other. The rows of a result are typed as
* postgres_query.h describes, by the same builder.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_protocol.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_protocol.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // NULL, size_t
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
refuses while SQL sent before is unfinished.
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

    return (PQsendQuery(_db, _sql) == 1) ? 0 : d_internal_postgres_trouble(_db);
}

// 3.2    Results
//------------------------------------------------------------------------------
/*
d_postgres_next_result
  One result a call; none left is D_DB_NOT_FOUND. A result's count is read
before its rows are built, which frees it.
*/
int
d_postgres_next_result(
    struct pg_conn*   _db,
    struct d_db_rows* _rows,
    int64_t*          _changes
)
{
    PGresult* taken = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_rows) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    PGresult* const next = PQgetResult(_db);

    // every result read
    if (!next)
    {
        return D_DB_NOT_FOUND;
    }

    const int result = d_internal_postgres_take(_db, next, &taken);

    // a failure, or a COPY ended
    if (result != 0)
    {
        return result;
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
d_postgres_is_busy
  PQisBusy reads nothing; d_postgres_consume_input does.
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

#endif  // D_INTERNAL_POSTGRES
