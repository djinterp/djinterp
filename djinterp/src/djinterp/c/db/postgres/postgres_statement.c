/*******************************************************************************
* djinterp [c]                                              postgres_statement.c
*
* Implementation of postgres_statement.h.
*   A statement is prepared on the server under a name the layer makes from
* the address of its own record, unique among the statements alive on a
* connection. The server may still hold a statement of that name -- one
* whose DEALLOCATE a failed transaction refused, before libpq 17 -- and it
* is then deallocated: inside a transaction, which a failed PREPARE would
* fail, after looking for the name first; outside one, after the prepare
* fails, which is then tried once more. PQdescribePrepared then says how
* many parameters there are, which sizes the arrays PQexecPrepared reads.
*   Values are kept as the server reads them -- text, formatted when bound,
* or bytes -- so a run only points libpq at them. Numbers are written in the
* C locale's absence: integers in decimal, real numbers by db_internal's
* formatter, which writes a `.` whatever the locale says.
*   A cursor's run is libpq's single-row mode over PQsendQueryPrepared: the
* server sends every row, and libpq hands them over one a result, so a page
* is built from as many results as it holds. Until the run's last result is
* read libpq reports the connection's transaction status as active, which is
* how every other call that waits on it knows to refuse
* (d_internal_postgres_ready); the statement only remembers that its own run
* is open. A statement released while another's run holds the connection
* cannot reach the server, so its copy there stays, under a name that a
* statement later prepared at the same address would take: from then on a
* prepare inside a transaction looks the name up first, as one before libpq
* 17 always does (d_internal_postgres_clear_stale).
*
*
* path:      /src/djinterp/c/db/postgres/postgres_statement.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.23
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_statement.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <math.h>    // isinf, isnan
#include <signal.h>  // sig_atomic_t
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, d_db_rows
#include "../db_internal.h"  // the rows builder, formatting, numbers
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, uintptr_t


// as its header: below the 64-bit floor there is nothing to define
#if defined(INT64_MAX)

#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_POSTGRES_NAME_ROOM
//   constant: room for a statement's name: "d_stmt_", the hexadecimal digits
// of an address, and a terminator.
#define D_INTERNAL_POSTGRES_NAME_ROOM (8u + (2u * sizeof(uintptr_t)) + 1u)

// d_internal_postgres_value
//   struct: what one parameter is bound to, as the server reads it.
struct d_internal_postgres_value
{
    enum d_db_value_type type;   // NULL until a bind says otherwise; TEXT
                                 // for every value sent as text, BLOB for
                                 // bytes
    char*                bytes;  // the text, terminated, or the bytes
    size_t               size;   // their length
};

// d_postgres_statement
//   struct: the definition postgres_common.h declares.
struct d_postgres_statement
{
    PGconn*                           db;       // the connection it is on
    char                              name[D_INTERNAL_POSTGRES_NAME_ROOM];
                                                // its name on the server
    size_t                            count;    // its parameters
    struct d_internal_postgres_value* values;   // count of them
    const char**                      texts;    // count, pointed at the
                                                // values to run
    int*                              lengths;  // count: the bytes' lengths
    int*                              formats;  // count: 1 for bytes, 0 for
                                                // text
    bool                              running;  // its cursor's run is open:
                                                // rows still to fetch
};

// d_internal_postgres_left_behind
//   flag: 1 once a statement's server copy has been left behind -- released
// while the connection could not take its Close, or in a failed transaction
// before libpq 17 -- anywhere in the process; never cleared. Each thread only
// ever writes 1, so a thread that reads 0 a moment late merely skips one
// lookup it did not yet need. sig_atomic_t, the one type C99 promises is
// written whole.
static volatile sig_atomic_t d_internal_postgres_left_behind = 0;

/**
 * @brief Names a statement after its record's address, in hexadecimal.
 *
 * @param[in,out] _statement  the statement, whose name is written.
 */
static void
d_internal_postgres_name(
    struct d_postgres_statement* _statement
)
{
    static const char digits[] = "0123456789abcdef";
    uintptr_t         address  = (uintptr_t)(void*)_statement;
    char*             at       = _statement->name;

    memcpy(at, "d_stmt_", 7u);
    at += 7u;

    // every digit, the most significant first
    for (size_t i = 2u * sizeof(uintptr_t); i > 0u; --i)
    {
        at[i - 1u] = digits[address & 0xFu];
        address  >>= 4;
    }

    at[2u * sizeof(uintptr_t)] = '\0';

    return;
}

/**
 * @brief Whether libpq closes a statement by the protocol's own message:
 *        PQclosePrepared, which the header compiled against must declare
 *        and the library loaded have, from 17.
 *
 * @return the answer.
 */
static bool
d_internal_postgres_closes(
    void
)
{
    return d_postgres_client_supports(D_POSTGRES_CLIENT_CLOSE_PREPARED);
}

/**
 * @brief Has the server drop a statement of the layer's.
 *
 * @param[in] _db    the connection.
 * @param[in] _name  the statement's name.
 * @return 0, or the failure's SQLSTATE, packed.
 *
 * libpq's PQclosePrepared, from 17, closes it by the protocol's own message,
 * which a failed transaction still takes; DEALLOCATE, before it, is a
 * statement, which a failed transaction refuses.
 */
static int
d_internal_postgres_drop(
    PGconn*     _db,
    const char* _name
)
{
    const int ready = d_internal_postgres_ready(_db);

    // a run holds the connection
    if (ready != 0)
    {
        return ready;
    }

#if defined(LIBPQ_HAS_CLOSE_PREPARED)
    if (d_internal_postgres_closes())
    {
        return d_internal_postgres_take(_db,
                                        PQclosePrepared(_db, _name),
                                        NULL);
    }
#endif  // LIBPQ_HAS_CLOSE_PREPARED

    return d_internal_postgres_run(_db, NULL, "DEALLOCATE %I", _name);
}

/**
 * @brief Whether the session holds a prepared statement of a name.
 *
 * @param[in]  _db    the connection.
 * @param[in]  _name  the name.
 * @param[out] _held  receives the answer; written only on success.
 * @return 0, or the failure's SQLSTATE, packed.
 */
static int
d_internal_postgres_held(
    PGconn*     _db,
    const char* _name,
    bool*       _held
)
{
    return d_internal_postgres_query_bool(
               _db,
               _held,
               "SELECT EXISTS (SELECT 1 FROM "
               "pg_catalog.pg_prepared_statements WHERE name = %Q)",
               _name);
}

/**
 * @brief Drops, inside a transaction, a statement of the name about to be
 *        prepared that the server still holds: one whose DEALLOCATE a failed
 *        transaction refused, at an address now reused.
 *
 * @param[in] _db    the connection.
 * @param[in] _name  the name.
 * @return 0, or the failure's SQLSTATE, packed.
 *
 * A PREPARE that fails fails the transaction it runs in, so there the name
 * is looked for first, at the cost of a round trip; outside one the prepare
 * is tried, and a name still held (42P05) dropped and tried again. libpq's
 * PQclosePrepared, from 17, leaves no name behind, and needs neither -- until
 * a statement is released while the connection cannot take its Close
 * (d_internal_postgres_left_behind).
 */
static int
d_internal_postgres_clear_stale(
    PGconn*     _db,
    const char* _name
)
{
    bool held   = false;
    int  result = 0;

    // a closing that has left no name behind; or no transaction in
    // progress, the only one a PREPARE that fails would fail
    if ( ( (d_internal_postgres_closes()) &&
           (d_internal_postgres_left_behind == 0) ) ||
         (PQtransactionStatus(_db) != PQTRANS_INTRANS) )
    {
        return 0;
    }

    result = d_internal_postgres_held(_db, _name, &held);

    return ( (result == 0) &&
             (held) ) ? d_internal_postgres_drop(_db, _name)
                      : result;
}

/**
 * @brief Empties a parameter's value, releasing its copy.
 *
 * @param[in,out] _value  the value; NULL after.
 */
static void
d_internal_postgres_value_clear(
    struct d_internal_postgres_value* _value
)
{
    free(_value->bytes);

    _value->type  = D_DB_VALUE_NULL;
    _value->bytes = NULL;
    _value->size  = 0u;

    return;
}

/**
 * @brief Checks that a statement has a parameter.
 *
 * @param[in] _statement  the statement.
 * @param[in] _index      the parameter, from 1.
 * @return 0; 42P02 for a parameter the statement does not have;
 *         D_DB_INVALID_ARGUMENT for a `NULL` statement.
 */
static int
d_internal_postgres_check(
    const struct d_postgres_statement* _statement,
    size_t                             _index
)
{
    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a parameter the statement has
    if ( (_index == 0u) ||
         (_index > _statement->count) )
    {
        return d_internal_postgres_answer(
                   _statement->db,
                   D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER);
    }

    return 0;
}

/**
 * @brief Gives a parameter a copy of bytes, as text or as bytes.
 *
 * @param[in] _statement  the statement.
 * @param[in] _index      the parameter, from 1.
 * @param[in] _type       D_DB_VALUE_TEXT, sent as text, or D_DB_VALUE_BLOB,
 *                        sent as bytes.
 * @param[in] _bytes      the bytes; not `NULL`.
 * @param[in] _size       how many.
 * @return 0; 42P02 for a parameter the statement does not have; 53200 when
 *         memory ran out; D_DB_INVALID_ARGUMENT for a `NULL` statement or a
 *         size libpq's int lengths cannot carry.
 *
 * The copy is made before the old value goes, so a failure leaves the
 * parameter as it was; it is terminated, so text reads as a C string, and
 * an empty value is still a pointer, which libpq sends as empty, not NULL.
 */
static int
d_internal_postgres_keep(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    enum d_db_value_type         _type,
    const void*                  _bytes,
    size_t                       _size
)
{
    const int checked = d_internal_postgres_check(_statement, _index);

    // a parameter the statement has
    if (checked != 0)
    {
        return checked;
    }

    // a length libpq's int cannot carry
    if (_size >= (size_t)INT_MAX)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    char* const copy = (char*)malloc(_size + 1u);

    // allocation failure
    if (!copy)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    memcpy(copy, _bytes, _size);
    copy[_size] = '\0';

    struct d_internal_postgres_value* const value =
        &_statement->values[_index - 1u];

    d_internal_postgres_value_clear(value);

    value->type  = _type;
    value->bytes = copy;
    value->size  = _size;

    return 0;
}

/**
 * @brief Gives a parameter a number, formatted as text.
 *
 * @param[in] _statement  the statement.
 * @param[in] _index      the parameter, from 1.
 * @param[in] _format     the number's conversion: `%lld` or `%g`, which
 *                        db_internal writes the same whatever the C library
 *                        or its locale -- MinGW's msvcrt has no `%lld`.
 * @param[in] ...         the number.
 * @return as d_internal_postgres_keep.
 */
static int
d_internal_postgres_keep_number(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    const char*                  _format,
    ...
)
{
    struct d_internal_db_buffer text = { NULL, 0u, 0u };
    va_list                     args;
    int                         result =
        d_internal_postgres_check(_statement, _index);

    // a parameter the statement has
    if (result != 0)
    {
        return result;
    }

    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_statement->db);

    va_start(args, _format);

    result = d_internal_postgres_format_result(
        d_internal_db_vformat(&text, &quoting, _format, args));

    va_end(args);

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_keep(_statement, _index, D_DB_VALUE_TEXT,
                                      text.data, text.length);

    d_internal_db_buffer_release(&text);

    return result;
}

/**
 * @brief Points libpq's arrays at a statement's values, for a run.
 *
 * @param[in,out] _statement  the statement.
 */
static void
d_internal_postgres_point(
    struct d_postgres_statement* _statement
)
{
    // libpq's arrays, pointed at the values
    for (size_t i = 0u; i < _statement->count; ++i)
    {
        const struct d_internal_postgres_value* const value =
            &_statement->values[i];

        _statement->texts[i]   = (value->type == D_DB_VALUE_NULL)
                                     ? NULL
                                     : value->bytes;
        _statement->lengths[i] = (int)value->size;
        _statement->formats[i] = (value->type == D_DB_VALUE_BLOB) ? 1 : 0;
    }

    return;
}

/**
 * @brief Ends a statement's open run, reading away every result left.
 *
 * @param[in,out] _statement  the statement, its run open.
 * @return 0, or the SQLSTATE, packed, of a failure among the results read
 *         away; D_DB_UNSUPPORTED for a COPY, which is ended.
 * @post The run is over and the connection free of it.
 *
 * The server sends every row whatever the client reads, so the rest of the
 * run is read and dropped; a failure the server reports among them -- a
 * division by zero at the thousandth row -- is still the run's, and the
 * transaction's, so it is reported.
 */
static int
d_internal_postgres_end_run(
    struct d_postgres_statement* _statement
)
{
    PGconn* const db     = _statement->db;
    int           result = 0;

    _statement->running = false;

    // every result left, to libpq's NULL
    for (PGresult* next = PQgetResult(db); next; next = PQgetResult(db))
    {
        const ExecStatusType status = PQresultStatus(next);

        // a COPY, ended with every result after it
        if (d_internal_postgres_copying(status))
        {
            PQclear(next);
            d_internal_postgres_end_copy(db, status);

            return (result != 0) ? result : D_DB_UNSUPPORTED;
        }

        const int code = d_internal_postgres_result(db, next);

        // the first failure is the run's
        if ( (code != 0) &&
             (result == 0) )
        {
            result = code;
        }

        PQclear(next);
    }

    return result;
}

/**
 * @brief Runs a statement with its values, ending its own open run first.
 *
 * @param[in,out] _statement  the statement.
 * @param[out]    _result     receives the statement's result on success.
 * @return 0; 08000 while another run holds the connection; or as
 *         d_internal_postgres_take.
 * @post On success the caller frees `*_result` with PQclear.
 */
static int
d_internal_postgres_execute_prepared(
    struct d_postgres_statement* _statement,
    PGresult**                   _result
)
{
    // its own run, read away
    if (_statement->running)
    {
        (void)d_internal_postgres_end_run(_statement);
    }

    const int ready = d_internal_postgres_ready(_statement->db);

    // another's run holds the connection
    if (ready != 0)
    {
        return ready;
    }

    d_internal_postgres_point(_statement);

    return d_internal_postgres_take(
               _statement->db,
               PQexecPrepared(_statement->db,
                              _statement->name,
                              (int)_statement->count,
                              _statement->texts,
                              _statement->lengths,
                              _statement->formats,
                              0),
               _result);
}

/**
 * @brief Releases a statement's memory, not touching the server.
 *
 * @param[in] _statement  the statement; may be `NULL`.
 */
static void
d_internal_postgres_free(
    struct d_postgres_statement* _statement
)
{
    // accept NULL, as free does
    if (!_statement)
    {
        return;
    }

    // the values' copies
    for (size_t i = 0u; ( (_statement->values) && (i < _statement->count) );
         ++i)
    {
        free(_statement->values[i].bytes);
    }

    free(_statement->values);
    free((void*)_statement->texts);
    free(_statement->lengths);
    free(_statement->formats);
    free(_statement);

    return;
}

/**
 * @brief Prepares a statement under the layer's name for it, and learns its
 *        parameters.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; left as it was on
 *                         failure.
 * @param[in]  _sql        the statement's text.
 * @return as d_postgres_prepare.
 *
 * A name the server still holds is a statement whose DEALLOCATE failed, in
 * a failed transaction, at an address now reused: inside a transaction it
 * is looked for and dropped first (d_internal_postgres_clear_stale); outside
 * one the prepare fails (42P05), and it is dropped and the prepare tried
 * once more.
 */
static int
d_internal_postgres_prepare_text(
    PGconn*                       _db,
    struct d_postgres_statement** _statement,
    const char*                   _sql
)
{
    const int ready = d_internal_postgres_ready(_db);

    // a run holds the connection
    if (ready != 0)
    {
        return ready;
    }

    struct d_postgres_statement* const made =
        (struct d_postgres_statement*)malloc(sizeof(*made));

    // allocation failure
    if (!made)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    made->db      = _db;
    made->count   = 0u;
    made->values  = NULL;
    made->texts   = NULL;
    made->lengths = NULL;
    made->formats = NULL;
    made->running = false;
    d_internal_postgres_name(made);

    int result = d_internal_postgres_clear_stale(_db, made->name);

    // a name the server held, gone; or a failure to tell
    if (result != 0)
    {
        d_internal_postgres_free(made);

        return result;
    }

    result = d_internal_postgres_take(
                 _db, PQprepare(_db, made->name, _sql, 0, NULL), NULL);

    // a stale statement of the same name, dropped, and once more
    if (result == D_POSTGRES_SQLSTATE_DUPLICATE_PREPARED_STATEMENT)
    {
        result = d_internal_postgres_drop(_db, made->name);
        result = (result == 0)
                     ? d_internal_postgres_take(
                           _db, PQprepare(_db, made->name, _sql, 0, NULL),
                           NULL)
                     : result;
    }

    // a statement the server would not prepare
    if (result != 0)
    {
        d_internal_postgres_free(made);

        return result;
    }

    PGresult* description = NULL;

    result = d_internal_postgres_take(_db,
                                      PQdescribePrepared(_db, made->name),
                                      &description);

    // the parameters, counted, and room for their values
    if (result == 0)
    {
        const int count = PQnparams(description);

        PQclear(description);

        made->count = (count > 0) ? (size_t)count : 0u;

        if (made->count > 0u)
        {
            made->values  = (struct d_internal_postgres_value*)malloc(
                                made->count *
                                sizeof(struct d_internal_postgres_value));
            made->texts   = (const char**)malloc(made->count *
                                                 sizeof(const char*));
            made->lengths = (int*)malloc(made->count * sizeof(int));
            made->formats = (int*)malloc(made->count * sizeof(int));

            if ( (!made->values)  ||
                 (!made->texts)   ||
                 (!made->lengths) ||
                 (!made->formats) )
            {
                free(made->values);

                made->values = NULL;
                result       = D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
            }
            else
            {
                for (size_t i = 0u; i < made->count; ++i)
                {
                    made->values[i].bytes = NULL;

                    d_internal_postgres_value_clear(&made->values[i]);
                }
            }
        }
    }

    // a statement the layer cannot hold is dropped again
    if (result != 0)
    {
        (void)d_internal_postgres_drop(_db, made->name);
        d_internal_postgres_free(made);

        return result;
    }

    *_statement = made;

    return 0;
}

//==============================================================================
// 1.  STATEMENTS
//==============================================================================

// 1.1    Preparing
//------------------------------------------------------------------------------
/*
d_postgres_prepare
  The server parses the text as it stands, and infers each parameter's type.
*/
int
d_postgres_prepare(
    PGconn*                       _db,
    struct d_postgres_statement** _statement,
    const char*                   _sql
)
{
    // no place to put a statement
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_statement = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_prepare_text(_db, _statement, _sql);
}

/*
d_postgres_preparef
  Formatted with the connection's quoting, then prepared.
*/
int
d_postgres_preparef(
    PGconn*                       _db,
    struct d_postgres_statement** _statement,
    const char*                   _format,
    ...
)
{
    struct d_internal_db_buffer sql = { NULL, 0u, 0u };
    va_list                     args;

    // no place to put a statement
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_statement = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);

    va_start(args, _format);

    int result = d_internal_postgres_format_result(
        d_internal_db_vformat(&sql, &quoting, _format, args));

    va_end(args);

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_prepare_text(_db, _statement, sql.data);

    d_internal_db_buffer_release(&sql);

    return result;
}

// 1.2    Finalizing and closing
//------------------------------------------------------------------------------
/*
d_postgres_finalize
  Its own run ended, then the server's copy, while the connection is known to
be the statement's; the memory goes whatever the server said. A copy the
server keeps is a name left behind, which prepares inside a transaction look
for from then on.
*/
int
d_postgres_finalize(
    struct d_postgres_statement* _statement
)
{
    // accept NULL, as free does
    if (!_statement)
    {
        return 0;
    }

    // its own run, read away
    if (_statement->running)
    {
        (void)d_internal_postgres_end_run(_statement);
    }

    const int result = d_internal_postgres_drop(_statement->db,
                                                _statement->name);

    // the server's copy kept: its name may come round again
    if (result != 0)
    {
        d_internal_postgres_left_behind = 1;
    }

    d_internal_postgres_free(_statement);

    return result;
}

/*
d_postgres_close_prepared
  The protocol's Close succeeds whether or not the session has the name;
DEALLOCATE fails where it has none (26000), so before libpq 17 the name is
looked for first.
*/
int
d_postgres_close_prepared(
    struct pg_conn* _db,
    const char*     _name
)
{
    bool held = false;

    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_name_ok(_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a statement the session has, where the server must be asked
    if (!d_internal_postgres_closes())
    {
        const int result = d_internal_postgres_held(_db, _name, &held);

        if ( (result != 0) ||
             (!held) )
        {
            return result;
        }
    }

    return d_internal_postgres_drop(_db, _name);
}

// 1.3    Describing
//------------------------------------------------------------------------------
/*
d_postgres_parameter_count
  As PQdescribePrepared counted them when the statement was prepared.
*/
size_t
d_postgres_parameter_count(
    const struct d_postgres_statement* _statement
)
{
    return (_statement) ? _statement->count : 0u;
}

/*
d_postgres_statement_connection
  The connection the statement was prepared on.
*/
PGconn*
d_postgres_statement_connection(
    const struct d_postgres_statement* _statement
)
{
    return (_statement) ? _statement->db : NULL;
}

//==============================================================================
// 2.  VALUES
//==============================================================================

// 2.1    Binding
//------------------------------------------------------------------------------
/*
d_postgres_bind_null
  The value emptied: a run passes libpq a NULL for it.
*/
int
d_postgres_bind_null(
    struct d_postgres_statement* _statement,
    size_t                       _index
)
{
    const int checked = d_internal_postgres_check(_statement, _index);

    // a parameter the statement has
    if (checked != 0)
    {
        return checked;
    }

    d_internal_postgres_value_clear(&_statement->values[_index - 1u]);

    return 0;
}

/*
d_postgres_bind_bool
  1 or 0, which boolean input reads as true and false, and integer input as
the numbers.
*/
int
d_postgres_bind_bool(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    bool                         _value
)
{
    return d_internal_postgres_keep(_statement, _index, D_DB_VALUE_TEXT,
                                    (_value) ? "1" : "0", 1u);
}

/*
d_postgres_bind_int64
  Decimal digits, which no locale changes.
*/
int
d_postgres_bind_int64(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    int64_t                      _value
)
{
    return d_internal_postgres_keep_number(_statement, _index, "%lld",
                                           (long long)_value);
}

/*
d_postgres_bind_double
  db_internal's %g writes the seventeen digits that read back as the same
double, its point a `.`; it refuses NaN and the infinities, which PostgreSQL
spells for itself.
*/
int
d_postgres_bind_double(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    double                       _value
)
{
    // PostgreSQL's spellings of NaN and the infinities
    if (isnan(_value))
    {
        return d_internal_postgres_keep(_statement, _index, D_DB_VALUE_TEXT,
                                        "NaN", 3u);
    }

    if (isinf(_value))
    {
        return (_value > 0.0)
                   ? d_internal_postgres_keep(_statement, _index,
                                              D_DB_VALUE_TEXT, "Infinity",
                                              8u)
                   : d_internal_postgres_keep(_statement, _index,
                                              D_DB_VALUE_TEXT, "-Infinity",
                                              9u);
    }

    return d_internal_postgres_keep_number(_statement, _index, "%g", _value);
}

/*
d_postgres_bind_text
  A copy, sent as text; NULL binds NULL.
*/
int
d_postgres_bind_text(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    const char*                  _text
)
{
    return (_text) ? d_internal_postgres_keep(_statement, _index,
                                              D_DB_VALUE_TEXT, _text,
                                              strlen(_text))
                   : d_postgres_bind_null(_statement, _index);
}

/*
d_postgres_bind_blob
  A copy, sent as bytes, the binary format, which a bytea takes as it is;
NULL binds NULL.
*/
int
d_postgres_bind_blob(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    const void*                  _bytes,
    size_t                       _size
)
{
    return (_bytes) ? d_internal_postgres_keep(_statement, _index,
                                               D_DB_VALUE_BLOB, _bytes,
                                               _size)
                    : d_postgres_bind_null(_statement, _index);
}

// 2.2    Clearing
//------------------------------------------------------------------------------
/*
d_postgres_clear_bindings
  Every value emptied; nothing is sent.
*/
int
d_postgres_clear_bindings(
    struct d_postgres_statement* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < _statement->count; ++i)
    {
        d_internal_postgres_value_clear(&_statement->values[i]);
    }

    return 0;
}

//==============================================================================
// 3.  RUNNING
//==============================================================================

// 3.1    Rows changed
//------------------------------------------------------------------------------
/*
d_postgres_execute_update
  The count is the command tag's, which a SELECT's carries too; a command
without one, such as CREATE TABLE, counts none.
*/
int
d_postgres_execute_update(
    struct d_postgres_statement* _statement,
    int64_t*                     _changes
)
{
    PGresult* result_rows = NULL;

    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_postgres_execute_prepared(_statement,
                                                            &result_rows);

    // a failure has nothing to count
    if (result != 0)
    {
        return result;
    }

    const char* const tuples = PQcmdTuples(result_rows);
    int64_t           count  = 0;

    // the tag's count; "" for a command that counts nothing
    const int counted = ( (tuples[0] == '\0') ||
                          (d_internal_db_parse_integer(tuples, &count)) )
                            ? 0
                            : D_DB_INVALID_ARGUMENT;

    PQclear(result_rows);

    // written only on success
    if ( (counted == 0) &&
         (_changes) )
    {
        *_changes = count;
    }

    return counted;
}

// 3.2    Rows
//------------------------------------------------------------------------------
/*
d_postgres_execute_query
  The rows of the result, read as a query's.
*/
int
d_postgres_execute_query(
    struct d_postgres_statement* _statement,
    struct d_db_rows*            _rows
)
{
    PGresult* result_rows = NULL;

    // parameter validation
    if ( (!_statement) ||
         (!_rows) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_postgres_execute_prepared(_statement,
                                                            &result_rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_postgres_collect_rows(result_rows, _rows);
}

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/*
d_postgres_open_cursor
  PQsendQueryPrepared, then single-row mode, which libpq takes only straight
after the send, before anything is read. A pipeline reads its results its
own way, statement by statement, and is refused.
*/
int
d_postgres_open_cursor(
    struct d_postgres_statement* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    PGconn* const db = _statement->db;

    // its own run, read away
    if (_statement->running)
    {
        (void)d_internal_postgres_end_run(_statement);
    }

    // pipeline mode
    if (d_internal_postgres_pipelined(db))
    {
        return d_internal_postgres_answer(
                   db,
                   D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    }

    const int ready = d_internal_postgres_ready(db);

    // another's run holds the connection
    if (ready != 0)
    {
        return ready;
    }

    d_internal_postgres_point(_statement);

    const int sent = PQsendQueryPrepared(db,
                                         _statement->name,
                                         (int)_statement->count,
                                         _statement->texts,
                                         _statement->lengths,
                                         _statement->formats,
                                         0);

    // libpq has answered, by its return value and its message
    d_internal_postgres_heard(db);

    // refused before anything was sent, or the connection lost
    if (!sent)
    {
        return (PQstatus(db) == CONNECTION_BAD)
                   ? D_POSTGRES_SQLSTATE_CONNECTION_FAILURE
                   : D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;
    }

    _statement->running = true;

    // libpq refuses single-row mode only at the wrong moment, which this is
    // not; were it to, the rows would come whole, and the run is ended
    if (PQsetSingleRowMode(db) != 1)
    {
        (void)d_internal_postgres_end_run(_statement);

        return d_internal_postgres_answer(
                   db,
                   D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION);
    }

    return 0;
}

/*
d_postgres_fetch_rows
  One result a row, until the page is full or a result that is not a row's
ends the run: the zero-row result of rows, which carries their columns; a
command's, which has none; or a failure. The run's end is read to libpq's
NULL, so that the connection is free when the call returns.
*/
int
d_postgres_fetch_rows(
    struct d_postgres_statement* _statement,
    size_t                       _max,
    struct d_db_rows*            _rows
)
{
    struct d_internal_db_rows builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u, 0u
    };

    // parameter validation
    if ( (!_statement)          ||
         (!_rows)               ||
         (_max == 0u)           ||
         (!_statement->running) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    PGconn* const db        = _statement->db;
    size_t        count     = 0u;
    bool          described = false;
    bool          ended     = false;
    bool          kept      = true;
    int           result    = 0;

    // a row a result, until the page is full or the run ends
    while ( (!ended) &&
            (count < _max) )
    {
        PGresult* const next = PQgetResult(db);

        // libpq has nothing more: the run is over, or the connection lost
        if (!next)
        {
            ended  = true;
            result = (PQstatus(db) == CONNECTION_BAD)
                         ? D_POSTGRES_SQLSTATE_CONNECTION_FAILURE
                         : 0;

            break;
        }

        const ExecStatusType status = PQresultStatus(next);

        // a COPY, which the layer ends everywhere
        if (d_internal_postgres_copying(status))
        {
            PQclear(next);
            d_internal_postgres_end_copy(db, status);
            _statement->running = false;
            d_internal_db_rows_release(&builder);

            return D_DB_UNSUPPORTED;
        }

        // the end of the run, a failure among the ways
        ended  = (status != PGRES_SINGLE_TUPLE);
        result = d_internal_postgres_result(db, next);

        // the columns once, from the first result; then any row
        if (result == 0)
        {
            kept = ( ( (described) ||
                       (d_internal_postgres_describe_columns(next,
                                                             &builder)) ) &&
                     (d_internal_postgres_keep_rows(next, &builder)) );
            described = true;
            count    += (size_t)PQntuples(next);
        }

        PQclear(next);

        // memory ran out, which ends the run too
        if (!kept)
        {
            ended  = true;
            result = D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
        }
    }

    // the run's end read to libpq's NULL; a failure keeps its own code
    if (ended)
    {
        const int rest = d_internal_postgres_end_run(_statement);

        result = (result != 0) ? result : rest;
    }

    // the page built, or memory ran out for its last step
    if ( (result == 0) &&
         (!d_internal_db_rows_finish(&builder, _rows)) )
    {
        result = D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;

        // the run ends with the page it could not give
        if (_statement->running)
        {
            (void)d_internal_postgres_end_run(_statement);
        }
    }

    // a failure gives no rows
    if (result != 0)
    {
        d_internal_db_rows_release(&builder);
    }

    return result;
}

/*
d_postgres_close_cursor
  The rest of the run read away (d_internal_postgres_end_run).
*/
int
d_postgres_close_cursor(
    struct d_postgres_statement* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // no run open
    if (!_statement->running)
    {
        return 0;
    }

    return d_internal_postgres_end_run(_statement);
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build
// without libpq can: D_DB_UNSUPPORTED, or 0 or NULL where it cannot fail. A
// statement a failed prepare promises is still written: NULL.

/*
d_postgres_prepare
  Without libpq there is nothing to prepare: D_DB_UNSUPPORTED, and the
statement NULL, as a failed prepare leaves it.
*/
int
d_postgres_prepare(
    struct pg_conn*               _db,
    struct d_postgres_statement** _statement,
    const char*                   _sql
)
{
    (void)_db;
    (void)_sql;

    // a failed prepare leaves NULL behind
    if (_statement)
    {
        *_statement = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_preparef
  As d_postgres_prepare.
*/
int
d_postgres_preparef(
    struct pg_conn*               _db,
    struct d_postgres_statement** _statement,
    const char*                   _format,
    ...
)
{
    (void)_db;
    (void)_format;

    // a failed prepare leaves NULL behind
    if (_statement)
    {
        *_statement = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_finalize
  Without libpq no statement exists, and releasing none succeeds.
*/
int
d_postgres_finalize(
    struct d_postgres_statement* _statement
)
{
    (void)_statement;

    return 0;
}

/*
d_postgres_close_prepared
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_close_prepared(
    struct pg_conn* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_parameter_count
  Without libpq, 0.
*/
size_t
d_postgres_parameter_count(
    const struct d_postgres_statement* _statement
)
{
    (void)_statement;

    return 0u;
}

/*
d_postgres_statement_connection
  Without libpq, NULL.
*/
struct pg_conn*
d_postgres_statement_connection(
    const struct d_postgres_statement* _statement
)
{
    (void)_statement;

    return NULL;
}

/*
d_postgres_bind_null
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_bind_null(
    struct d_postgres_statement* _statement,
    size_t                       _index
)
{
    (void)_statement;
    (void)_index;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_bind_bool
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_bind_bool(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    bool                         _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_bind_int64
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_bind_int64(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    int64_t                      _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_bind_double
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_bind_double(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    double                       _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_bind_text
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_bind_text(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    const char*                  _text
)
{
    (void)_statement;
    (void)_index;
    (void)_text;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_bind_blob
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_bind_blob(
    struct d_postgres_statement* _statement,
    size_t                       _index,
    const void*                  _bytes,
    size_t                       _size
)
{
    (void)_statement;
    (void)_index;
    (void)_bytes;
    (void)_size;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_clear_bindings
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_clear_bindings(
    struct d_postgres_statement* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_execute_update
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_execute_update(
    struct d_postgres_statement* _statement,
    int64_t*                     _changes
)
{
    (void)_statement;
    (void)_changes;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_execute_query
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_execute_query(
    struct d_postgres_statement* _statement,
    struct d_db_rows*            _rows
)
{
    (void)_statement;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_open_cursor
  Without libpq there is nothing to run: D_DB_UNSUPPORTED.
*/
int
d_postgres_open_cursor(
    struct d_postgres_statement* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_fetch_rows
  Without libpq there are no rows: D_DB_UNSUPPORTED, and the rows left as
they were.
*/
int
d_postgres_fetch_rows(
    struct d_postgres_statement* _statement,
    size_t                       _max,
    struct d_db_rows*            _rows
)
{
    (void)_statement;
    (void)_max;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_close_cursor
  Without libpq there is no run to end: D_DB_UNSUPPORTED.
*/
int
d_postgres_close_cursor(
    struct d_postgres_statement* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_POSTGRES

#endif  // defined(INT64_MAX)
