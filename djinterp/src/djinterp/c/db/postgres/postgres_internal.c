/*******************************************************************************
* djinterp [c]                                               postgres_internal.c
*
* Implementation of the plumbing postgres_internal.h declares for the
* PostgreSQL layer's modules: results read, name checks and quoting,
* statements run and read, IF NOT EXISTS settled, and catalogue lookups.
*   Every statement goes through d_internal_postgres_execute, which runs it
* with PQexec -- the simple query protocol, which takes several statements in
* one string and needs no parameters, since every value is quoted into the
* text -- and turns what libpq hands back into the layer's result. A COPY to
* or from the client, which only the caller's own SQL can start, is ended
* there and then, so that a connection never leaves the layer mid-COPY.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_internal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "./postgres_internal.h"  // corresponding header
// std
#include <math.h>    // INFINITY, NAN
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strchr, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, the lists
#include "../db_internal.h"  // buffers, builders, formatting, numbers
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_POSTGRES_COPY_REFUSAL
//   constant: the reason the layer gives the server when it ends a COPY from
// the client that the caller's SQL started.
#define D_INTERNAL_POSTGRES_COPY_REFUSAL                                       \
    "COPY with the client is not supported through the djinterp layer"

// d_internal_postgres_kind
//   struct: where pg_catalog keeps one kind of object, as a SELECT of its
// names through the conditions every lookup of the kind needs. The aliases
// are fixed: n for the namespace, t for an owning table.
struct d_internal_postgres_kind
{
    const char* select;   // the SELECT, through its fixed conditions
    const char* name;     // its name column
    bool        scoped;   // whether it lives in a schema, n
    const char* table;    // its owning table's name column, or NULL
    const char* listing;  // one more condition when listing, or NULL
};

// d_internal_postgres_kinds
//   table: each d_internal_postgres_object's kind, in the enum's order.
// relkind 'p' and 'I', a partitioned table and index, arrived with 10 and
// 11, and are simply absent before. A listing leaves out the system's
// schemas, and the indexes a primary key, UNIQUE or EXCLUDE constraint owns.
static const struct d_internal_postgres_kind d_internal_postgres_kinds[] =
{
    { "SELECT d.datname FROM pg_catalog.pg_database d WHERE TRUE",
      "d.datname", false, NULL, NULL },
    { "SELECT n.nspname FROM pg_catalog.pg_namespace n WHERE TRUE",
      "n.nspname", false, NULL,
      "n.nspname !~ '^pg_' AND n.nspname <> 'information_schema'" },
    { "SELECT e.extname FROM pg_catalog.pg_extension e WHERE TRUE",
      "e.extname", false, NULL, NULL },
    { "SELECT c.relname FROM pg_catalog.pg_class c "
      "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
      "WHERE c.relkind IN ('r', 'p')",
      "c.relname", true, NULL, NULL },
    { "SELECT c.relname FROM pg_catalog.pg_class c "
      "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
      "WHERE c.relkind = 'v'",
      "c.relname", true, NULL, NULL },
    { "SELECT c.relname FROM pg_catalog.pg_class c "
      "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
      "WHERE c.relkind = 'S'",
      "c.relname", true, NULL, NULL },
    { "SELECT c.relname FROM pg_catalog.pg_class c "
      "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
      "JOIN pg_catalog.pg_index x ON x.indexrelid = c.oid "
      "JOIN pg_catalog.pg_class t ON t.oid = x.indrelid "
      "WHERE c.relkind IN ('i', 'I')",
      "c.relname", true, "t.relname",
      "NOT EXISTS (SELECT 1 FROM pg_catalog.pg_constraint k "
      "WHERE k.conindid = c.oid AND k.contype IN ('p', 'u', 'x'))" },
    { "SELECT a.attname FROM pg_catalog.pg_attribute a "
      "JOIN pg_catalog.pg_class t ON t.oid = a.attrelid "
      "JOIN pg_catalog.pg_namespace n ON n.oid = t.relnamespace "
      "WHERE a.attnum > 0 AND NOT a.attisdropped "
      "AND t.relkind IN ('r', 'p', 'v', 'm', 'f')",
      "a.attname", true, "t.relname", NULL },
    { "SELECT g.tgname FROM pg_catalog.pg_trigger g "
      "JOIN pg_catalog.pg_class t ON t.oid = g.tgrelid "
      "JOIN pg_catalog.pg_namespace n ON n.oid = t.relnamespace "
      "WHERE NOT g.tgisinternal",
      "g.tgname", true, "t.relname", NULL }
};

D_STATIC_ASSERT( (sizeof(d_internal_postgres_kinds) /
                  sizeof(d_internal_postgres_kinds[0])) ==
                 ((size_t)D_INTERNAL_POSTGRES_TRIGGER + 1u),
                 "every d_internal_postgres_object has a kind" );

/*
d_internal_postgres_copying
  Whether a result is the start of a COPY, which waits on the client.
*/
bool
d_internal_postgres_copying(
    ExecStatusType _status
)
{
    return ( (_status == PGRES_COPY_IN)  ||
             (_status == PGRES_COPY_OUT) ||
             (_status == PGRES_COPY_BOTH) );
}

/*
d_internal_postgres_abandon_copy
  One COPY ended from the client's side: what the server sends is read and
dropped, and what it waits for is refused, which fails the COPY.
*/
static void
d_internal_postgres_abandon_copy(
    PGconn*        _db,
    ExecStatusType _status
)
{
    char* data = NULL;

    // the server's rows, until it has sent them all or fails
    if ( (_status == PGRES_COPY_OUT) ||
         (_status == PGRES_COPY_BOTH) )
    {
        while (PQgetCopyData(_db, &data, 0) > 0)
        {
            PQfreemem(data);
            data = NULL;
        }
    }

    // the client's rows, refused
    if ( (_status == PGRES_COPY_IN) ||
         (_status == PGRES_COPY_BOTH) )
    {
        (void)PQputCopyEnd(_db, D_INTERNAL_POSTGRES_COPY_REFUSAL);
    }

    return;
}

/*
d_internal_postgres_end_copy
  Every COPY the SQL starts is abandoned, and every result after them read,
until libpq has none left and the connection takes a statement again.
*/
void
d_internal_postgres_end_copy(
    PGconn*        _db,
    ExecStatusType _status
)
{
    ExecStatusType status  = _status;
    bool           copying = true;

    // each COPY, then the results up to the next, or the end
    while (copying)
    {
        d_internal_postgres_abandon_copy(_db, status);
        copying = false;

        for (PGresult* result = PQgetResult(_db);
             result;
             result = PQgetResult(_db))
        {
            status = PQresultStatus(result);
            PQclear(result);

            // another COPY, to end in turn
            if (d_internal_postgres_copying(status))
            {
                copying = true;

                break;
            }
        }
    }

    return;
}

/**
 * @brief Runs SQL and hands back its last result, whatever kind of success
 *        it is.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _sql     the SQL.
 * @param[out] _result  receives the result on success; `NULL` to discard it.
 * @return 0, a failure as postgres_internal.h's section 4 says, or
 *         D_DB_UNSUPPORTED for a COPY with the client, ended.
 * @post On success with `_result`, the caller frees `*_result` with PQclear.
 */
static int
d_internal_postgres_execute(
    PGconn*     _db,
    const char* _sql,
    PGresult**  _result
)
{
    return d_internal_postgres_take(_db, PQexec(_db, _sql), _result);
}

/**
 * @brief Copies the first column of the first row of a result, and frees the
 *        result.
 *
 * @param[in]  _rows   the result; may be `NULL`, for a statement without
 *                     rows.
 * @param[out] _value  receives the copy, `NULL` for a NULL; written only on
 *                     success.
 * @return 0, D_DB_NOT_FOUND for no row, or 53200 when memory ran out.
 */
static int
d_internal_postgres_first_value(
    PGresult* _rows,
    char**    _value
)
{
    char* copy = NULL;

    // no result, or no row in it
    if ( (!_rows)                 ||
         (PQntuples(_rows) < 1)   ||
         (PQnfields(_rows) < 1) )
    {
        PQclear(_rows);

        return D_DB_NOT_FOUND;
    }

    // a value is copied with a terminator; a NULL stays NULL
    if (!PQgetisnull(_rows, 0, 0))
    {
        const size_t length = (size_t)PQgetlength(_rows, 0, 0);

        copy = (char*)malloc(length + 1u);

        if (!copy)
        {
            PQclear(_rows);

            return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
        }

        memcpy(copy, PQgetvalue(_rows, 0, 0), length);
        copy[length] = '\0';
    }

    PQclear(_rows);
    *_value = copy;

    return 0;
}

/*
d_internal_postgres_read_int64
  An integer is read exactly; a number with a fraction, or one written with
an exponent, is truncated toward zero, as C converts one, when it fits. Both
bounds are powers of two, which a double holds exactly.
*/
static int
d_internal_postgres_read_int64(
    const char* _text,
    int64_t*    _value
)
{
    double real = 0.0;

    // a NULL reads as 0
    if (!_text)
    {
        *_value = 0;

        return 0;
    }

    // an integer, exactly
    if (d_internal_db_parse_integer(_text, _value))
    {
        return 0;
    }

    // any other number, truncated, where it fits
    if ( (d_internal_db_parse_real(_text, &real)) &&
         (real >= -9223372036854775808.0)         &&
         (real < 9223372036854775808.0) )
    {
        *_value = (int64_t)real;

        return 0;
    }

    return D_DB_INVALID_ARGUMENT;
}

/*
d_internal_postgres_read_bool
  The server writes a boolean as t or f, whatever DateStyle or locale say; a
NULL reads as false.
*/
static int
d_internal_postgres_read_bool(
    const char* _text,
    bool*       _value
)
{
    // a NULL reads as false
    if (!_text)
    {
        *_value = false;

        return 0;
    }

    // t or f, alone
    if ( ( (_text[0] == 't') || (_text[0] == 'f') ) &&
         (_text[1] == '\0') )
    {
        *_value = (_text[0] == 't');

        return 0;
    }

    return D_DB_INVALID_ARGUMENT;
}

/*
d_internal_postgres_collect
  The first column of each row, a NULL skipped, into one list; the result is
freed whatever happens. A result is in memory whole, so reading it cannot
fail part way.
*/
static int
d_internal_postgres_collect(
    PGresult*              _rows,
    struct d_db_name_list* _names
)
{
    struct d_internal_db_names builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u
    };
    const int count = (_rows) ? PQntuples(_rows) : 0;
    bool      ok    = true;

    // each name, until one cannot be added
    for (int i = 0; (ok) && (i < count); ++i)
    {
        ok = ( (PQgetisnull(_rows, i, 0)) ||
               (d_internal_db_names_add(&builder, PQgetvalue(_rows, i, 0))) );
    }

    PQclear(_rows);

    // finishing releases the builder whether it succeeds or not
    if (!ok)
    {
        d_internal_db_names_release(&builder);

        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    return (d_internal_db_names_finish(&builder, _names))
               ? 0
               : D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
}

/**
 * @brief Checks a lookup's arguments.
 *
 * @param[in] _object  what is looked for.
 * @param[in] _path    where.
 * @param[in] _named   whether the path must name the object.
 * @return `true` when they can be right.
 *
 * A column always needs its table, and a trigger by name, its names being
 * the table's; an index or a trigger listing may narrow to one table; the
 * other kinds take none.
 */
static bool
d_internal_postgres_path_ok(
    enum d_internal_postgres_object        _object,
    const struct d_internal_postgres_path* _path,
    bool                                   _named
)
{
    // a kind there is, and a path to look along
    if ( ((unsigned)_object > (unsigned)D_INTERNAL_POSTGRES_TRIGGER) ||
         (!_path) )
    {
        return false;
    }

    const bool needs_table = ( (_object == D_INTERNAL_POSTGRES_COLUMN) ||
                               ( (_object == D_INTERNAL_POSTGRES_TRIGGER) &&
                                 (_named) ) );
    const bool takes_table = ( (needs_table)                          ||
                               (_object == D_INTERNAL_POSTGRES_INDEX) ||
                               (_object == D_INTERNAL_POSTGRES_TRIGGER) );

    return ( (d_internal_postgres_schema_ok(_path->schema))           &&
             ( (!_named) ||
               (d_internal_postgres_name_ok(_path->name)) )           &&
             ( (_path->table)
                   ? ( (takes_table) &&
                       (d_internal_postgres_name_ok(_path->table)) )
                   : (!needs_table) ) );
}

/**
 * @brief Appends the conditions that place an object: its schema, the table
 *        it belongs to, and its name, each where the kind and path have it.
 *
 * @param[in]     _db      the connection.
 * @param[in,out] _sql     the statement so far.
 * @param[in]     _object  what is looked for.
 * @param[in]     _path    where the object is.
 * @return as d_internal_db_format.
 *
 * A schema's own name is a schema, matched as one, "pg_temp" included.
 */
static enum d_internal_db_format_status
d_internal_postgres_place(
    PGconn*                                _db,
    struct d_internal_db_buffer*           _sql,
    enum d_internal_postgres_object        _object,
    const struct d_internal_postgres_path* _path
)
{
    const struct d_internal_postgres_kind* const kind =
        &d_internal_postgres_kinds[_object];
    const struct d_internal_db_quoting           quoting =
        d_internal_postgres_quoting(_db);
    enum d_internal_db_format_status             status  =
        (kind->scoped)
            ? d_internal_postgres_match_schema(_db, _sql, _path->schema)
            : D_INTERNAL_DB_FORMAT_OK;

    // the table, where one is given
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (_path->table) )
    {
        status = d_internal_db_format(_sql,
                                      &quoting,
                                      " AND %s = %Q::pg_catalog.name",
                                      kind->table,
                                      _path->table);
    }

    // the name, unless listing
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (_path->name) )
    {
        status = (_object == D_INTERNAL_POSTGRES_SCHEMA)
                     ? d_internal_postgres_match_schema(_db, _sql, _path->name)
                     : d_internal_db_format(_sql,
                                            &quoting,
                                            " AND %s = %Q::pg_catalog.name",
                                            kind->name,
                                            _path->name);
    }

    return status;
}

/**
 * @brief Runs a lookup and hands back its rows.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to look for, already checked.
 * @param[in]  _path    where, already checked.
 * @param[out] _rows    receives the rows.
 * @return 0, a failure as postgres_internal.h's section 4 says, 53200, or
 *         D_DB_INVALID_ARGUMENT.
 *
 * The kind's SELECT, the conditions that place the object, a listing's own
 * condition; then one row answers a lookup by name, and a listing comes in
 * byte order, as the name type sorts on every version.
 */
static int
d_internal_postgres_lookup(
    PGconn*                                _db,
    enum d_internal_postgres_object        _object,
    const struct d_internal_postgres_path* _path,
    PGresult**                             _rows
)
{
    const struct d_internal_postgres_kind* const kind =
        &d_internal_postgres_kinds[_object];
    const struct d_internal_db_quoting           quoting =
        d_internal_postgres_quoting(_db);
    struct d_internal_db_buffer                  sql     = { NULL, 0u, 0u };
    enum d_internal_db_format_status             status  =
        d_internal_db_format(&sql, &quoting, "%s", kind->select);

    // where the object is
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_postgres_place(_db, &sql, _object, _path);
    }

    // a listing's condition, then one row or the order
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (!_path->name)                      &&
         (kind->listing) )
    {
        status = d_internal_db_format(&sql, &quoting, " AND %s", kind->listing);
    }

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_db_format(&sql,
                                      &quoting,
                                      (_path->name) ? " LIMIT 1"
                                                    : " ORDER BY 1");
    }

    const int result = d_internal_postgres_format_result(status);

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    const int sent = d_internal_postgres_send(_db, sql.data, _rows);

    d_internal_db_buffer_release(&sql);

    return sent;
}

/**
 * @brief What a tolerated statement's result comes to, its savepoint
 *        settled.
 *
 * @param[in] _db         the connection.
 * @param[in] _result     the statement's result.
 * @param[in] _tolerated  the SQLSTATE that counts as success.
 * @param[in] _guarded    whether a savepoint went before the statement.
 * @return 0 for success or the tolerated SQLSTATE; any other result as it
 *         was; or a failure to settle the savepoint.
 *
 * Outside a transaction there is nothing to mend. Inside one, the tolerated
 * failure is rolled back to the savepoint, leaving the transaction as it
 * was, and the savepoint is released either way; any other failure leaves
 * the transaction failed, as a failing statement does.
 */
static int
d_internal_postgres_settle(
    PGconn* _db,
    int     _result,
    int     _tolerated,
    bool    _guarded
)
{
    // nothing to mend
    if (!_guarded)
    {
        return (_result == _tolerated) ? 0 : _result;
    }

    if (_result == _tolerated)
    {
        return d_internal_postgres_send(
                   _db,
                   "ROLLBACK TO SAVEPOINT d_postgres_tolerate; "
                   "RELEASE SAVEPOINT d_postgres_tolerate",
                   NULL);
    }

    return (_result == 0)
               ? d_internal_postgres_send(
                     _db,
                     "RELEASE SAVEPOINT d_postgres_tolerate",
                     NULL)
               : _result;
}

//==============================================================================
// 2.  RESULTS
//==============================================================================

// 2.1    Reading results
//------------------------------------------------------------------------------
/*
d_internal_postgres_result
  libpq answers no result only when memory runs out or the connection cannot
carry the statement; a failure it detects itself, a lost connection among
them, carries no SQLSTATE, so the connection's state says which.
*/
int
d_internal_postgres_result(
    const PGconn*   _db,
    const PGresult* _result
)
{
    const bool lost = (PQstatus(_db) == CONNECTION_BAD);

    // nothing came back
    if (!_result)
    {
        return (lost) ? D_POSTGRES_SQLSTATE_CONNECTION_FAILURE
                      : D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    switch (PQresultStatus(_result))
    {
        case PGRES_COMMAND_OK:
        case PGRES_TUPLES_OK:
        case PGRES_EMPTY_QUERY:
            return 0;

        default:
            break;
    }

    const int packed = d_postgres_result_from_sqlstate(
                           PQresultErrorField(_result, PG_DIAG_SQLSTATE));

    // the server's own code, or the layer's for libpq's
    if (packed > 0)
    {
        return packed;
    }

    return (lost) ? D_POSTGRES_SQLSTATE_CONNECTION_FAILURE
                  : D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;
}

/*
d_internal_postgres_take
  A COPY waits on the client, which the layer is not: it is ended, with every
result after it, so that the connection takes a statement again.
*/
int
d_internal_postgres_take(
    PGconn*    _db,
    PGresult*  _result,
    PGresult** _out
)
{
    // a COPY with the client, ended
    if ( (_result) &&
         (d_internal_postgres_copying(PQresultStatus(_result))) )
    {
        const ExecStatusType status = PQresultStatus(_result);

        PQclear(_result);
        d_internal_postgres_end_copy(_db, status);

        return D_DB_UNSUPPORTED;
    }

    const int code = d_internal_postgres_result(_db, _result);

    // a success the caller wants to read
    if ( (code == 0) &&
         (_out) )
    {
        *_out = _result;

        return 0;
    }

    PQclear(_result);

    return code;
}

/*
d_internal_postgres_format_result
  Memory has the server's code for it; anything else the formatter would not
write -- a format it cannot read, a NaN, a NUL character, text not valid in
the connection's encoding -- is the caller's argument.
*/
int
d_internal_postgres_format_result(
    enum d_internal_db_format_status _state
)
{
    switch (_state)
    {
        case D_INTERNAL_DB_FORMAT_OK:
            return 0;

        case D_INTERNAL_DB_FORMAT_NO_MEMORY:
            return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;

        default:
            break;
    }

    return D_DB_INVALID_ARGUMENT;
}

//==============================================================================
// 3.  NAMES
//==============================================================================

// 3.1    Checks
//------------------------------------------------------------------------------
/*
d_internal_postgres_name_ok
  The server refuses an empty name itself, but a caller passing one has
almost certainly made a mistake, and the layer says so first.
*/
bool
d_internal_postgres_name_ok(
    const char* _name
)
{
    return ( (_name) &&
             (_name[0] != '\0') );
}

/*
d_internal_postgres_schema_ok
  NULL is the documented way to say the current schema; an empty name is
never one.
*/
bool
d_internal_postgres_schema_ok(
    const char* _schema
)
{
    return ( (!_schema) ||
             (_schema[0] != '\0') );
}

// 3.2    Quoting
//------------------------------------------------------------------------------
/*
d_internal_postgres_quote_name
  A double quote never appears inside a multibyte character in any encoding
PostgreSQL takes from a client, so doubling every one is safe without
knowing the connection's. The name is copied a run at a time, each run
ending at a double quote, which is then doubled.
*/
bool
d_internal_postgres_quote_name(
    void*                        _context,
    struct d_internal_db_buffer* _out,
    const char*                  _name
)
{
    const size_t length_before = _out->length;
    const char*  run           = _name;
    bool         ok            = d_internal_db_buffer_append(_out, "\"", 1u);

    (void)_context;

    // each run, through the next double quote, which is written twice
    while ( (ok) &&
            (*run != '\0') )
    {
        const char* const quote  = strchr(run, '"');
        const size_t      length = (quote) ? ((size_t)(quote - run) + 1u)
                                           : strlen(run);

        ok   = ( (d_internal_db_buffer_append(_out, run, length)) &&
                 ( (!quote) ||
                   (d_internal_db_buffer_append(_out, "\"", 1u)) ) );
        run += length;
    }

    // the closing quote; a failure takes back everything written
    if ( (!ok) ||
         (!d_internal_db_buffer_append(_out, "\"", 1u)) )
    {
        _out->length = length_before;

        return false;
    }

    return true;
}

/*
d_internal_postgres_quote_value
  libpq quotes for the connection: the encoding, whose multibyte characters
it never splits, and standard_conforming_strings, writing E'...' with
backslashes doubled where they would otherwise be read as escapes. From its
releases of February 2025 it refuses text that is not valid in the
encoding; an older one quotes it, and the server, which checks the encoding
of every statement, refuses the statement instead.
*/
bool
d_internal_postgres_quote_value(
    void*                        _context,
    struct d_internal_db_buffer* _out,
    const char*                  _text
)
{
    PGconn* const db     = (PGconn*)_context;
    char* const   quoted = PQescapeLiteral(db, _text, strlen(_text));

    // refused, or out of memory
    if (!quoted)
    {
        return false;
    }

    const bool ok = d_internal_db_buffer_append(_out, quoted, strlen(quoted));

    PQfreemem(quoted);

    return ok;
}

/*
d_internal_postgres_quoting
  Both quoting functions, with the connection as the context the value's
needs.
*/
struct d_internal_db_quoting
d_internal_postgres_quoting(
    PGconn* _db
)
{
    const struct d_internal_db_quoting quoting =
    {
        d_internal_postgres_quote_value,
        d_internal_postgres_quote_name,
        _db
    };

    return quoting;
}

// 3.3    The current schema
//------------------------------------------------------------------------------
/*
d_internal_postgres_target
  current_schema() is NULL when no schema of the search path exists, which is
3F000, as the server says it for an unqualified CREATE.
*/
int
d_internal_postgres_target(
    PGconn*      _db,
    const char*  _schema,
    const char** _use,
    char**       _owned
)
{
    char* name = NULL;

    *_use   = _schema;
    *_owned = NULL;

    // the caller's own schema needs no asking
    if (_schema)
    {
        return 0;
    }

    const int result = d_internal_postgres_query(
                           _db,
                           &name,
                           "SELECT pg_catalog.current_schema()");

    // a failure, or no schema
    if (result != 0)
    {
        return result;
    }

    if (!name)
    {
        return D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME;
    }

    *_use   = name;
    *_owned = name;

    return 0;
}

// 3.4    Clauses
//------------------------------------------------------------------------------
/*
d_internal_postgres_clause
  A flag's words, or nothing, for a %s in a statement.
*/
const char*
d_internal_postgres_clause(
    unsigned    _flags,
    unsigned    _flag,
    const char* _words
)
{
    return ((_flags & _flag) != 0u) ? _words : "";
}

//==============================================================================
// 4.  STATEMENTS
//==============================================================================

// 4.1    Running
//------------------------------------------------------------------------------
/*
d_internal_postgres_send
  The last result is the caller's only when it holds rows.
*/
int
d_internal_postgres_send(
    PGconn*     _db,
    const char* _sql,
    PGresult**  _rows
)
{
    PGresult* result = NULL;

    // nothing yet, whatever follows
    if (_rows)
    {
        *_rows = NULL;
    }

    const int code = d_internal_postgres_execute(_db, _sql, &result);

    // a failure keeps nothing
    if (code != 0)
    {
        return code;
    }

    // rows, when they are wanted and came
    if ( (_rows) &&
         (PQresultStatus(result) == PGRES_TUPLES_OK) )
    {
        *_rows = result;

        return 0;
    }

    PQclear(result);

    return 0;
}

/*
d_internal_postgres_vrun
  The statement is formatted with the connection's quoting, sent, and
released.
*/
int
d_internal_postgres_vrun(
    PGconn*     _db,
    PGresult**  _rows,
    const char* _format,
    va_list     _args
)
{
    struct d_internal_db_buffer        sql     = { NULL, 0u, 0u };
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);

    // nothing yet, whatever follows
    if (_rows)
    {
        *_rows = NULL;
    }

    const int result = d_internal_postgres_format_result(
        d_internal_db_vformat(&sql, &quoting, _format, _args));

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    const int sent = d_internal_postgres_send(_db, sql.data, _rows);

    d_internal_db_buffer_release(&sql);

    return sent;
}

/*
d_internal_postgres_run
  The variadic face of d_internal_postgres_vrun.
*/
int
d_internal_postgres_run(
    PGconn*     _db,
    PGresult**  _rows,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_postgres_vrun(_db, _rows, _format, args);

    va_end(args);

    return result;
}

// 4.2    Single values
//------------------------------------------------------------------------------
/*
d_internal_postgres_vquery
  The first value of the last result, copied out before the result is freed.
*/
int
d_internal_postgres_vquery(
    PGconn*     _db,
    char**      _value,
    const char* _format,
    va_list     _args
)
{
    PGresult* rows   = NULL;
    const int result = d_internal_postgres_vrun(_db, &rows, _format, _args);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_postgres_first_value(rows, _value);
}

/*
d_internal_postgres_query
  The variadic face of d_internal_postgres_vquery.
*/
int
d_internal_postgres_query(
    PGconn*     _db,
    char**      _value,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_postgres_vquery(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_internal_postgres_vquery_int64
  The value as text, as the protocol sends it, then read as an integer.
*/
int
d_internal_postgres_vquery_int64(
    PGconn*     _db,
    int64_t*    _value,
    const char* _format,
    va_list     _args
)
{
    char*   text   = NULL;
    int64_t number = 0;
    int     result = d_internal_postgres_vquery(_db, &text, _format, _args);

    // the text, then its number
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_read_int64(text, &number);
    free(text);

    // written only on success
    if (result == 0)
    {
        *_value = number;
    }

    return result;
}

/*
d_internal_postgres_query_int64
  The variadic face of d_internal_postgres_vquery_int64.
*/
int
d_internal_postgres_query_int64(
    PGconn*     _db,
    int64_t*    _value,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result =
        d_internal_postgres_vquery_int64(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_internal_postgres_vquery_double
  As d_internal_postgres_vquery_int64, reading a double; NULL reads as 0. The
server writes a double's NaN and infinities as words, which are not numbers
here.
*/
int
d_internal_postgres_vquery_double(
    PGconn*     _db,
    double*     _value,
    const char* _format,
    va_list     _args
)
{
    char*  text   = NULL;
    double number = 0.0;
    int    result = d_internal_postgres_vquery(_db, &text, _format, _args);

    // the text, then its number
    if (result != 0)
    {
        return result;
    }

    result = ( (!text) ||
               (d_internal_db_parse_real(text, &number)) )
                 ? 0
                 : D_DB_INVALID_ARGUMENT;
    free(text);

    // written only on success
    if (result == 0)
    {
        *_value = number;
    }

    return result;
}

/*
d_internal_postgres_vquery_bool
  As d_internal_postgres_vquery_int64, reading t or f.
*/
int
d_internal_postgres_vquery_bool(
    PGconn*     _db,
    bool*       _value,
    const char* _format,
    va_list     _args
)
{
    char* text   = NULL;
    bool  answer = false;
    int   result = d_internal_postgres_vquery(_db, &text, _format, _args);

    // the text, then its truth
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_read_bool(text, &answer);
    free(text);

    // written only on success
    if (result == 0)
    {
        *_value = answer;
    }

    return result;
}

/*
d_internal_postgres_query_bool
  The variadic face of d_internal_postgres_vquery_bool.
*/
int
d_internal_postgres_query_bool(
    PGconn*     _db,
    bool*       _value,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result =
        d_internal_postgres_vquery_bool(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_internal_postgres_vchanges
  The statement's command tag carries its count -- INSERT 0 3, UPDATE 2 --
which PQcmdTuples isolates; a statement that counts nothing has "".
*/
int
d_internal_postgres_vchanges(
    PGconn*     _db,
    int64_t*    _changes,
    const char* _format,
    va_list     _args
)
{
    struct d_internal_db_buffer        sql     = { NULL, 0u, 0u };
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);
    PGresult*                          last    = NULL;
    int64_t                            count   = 0;
    int                                result  =
        d_internal_postgres_format_result(
            d_internal_db_vformat(&sql, &quoting, _format, _args));

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_execute(_db, sql.data, &last);
    d_internal_db_buffer_release(&sql);

    // a failure has no count to read
    if (result != 0)
    {
        return result;
    }

    const char* const tuples = PQcmdTuples(last);

    result = ( (tuples[0] == '\0') ||
               (d_internal_db_parse_integer(tuples, &count)) )
                 ? 0
                 : D_DB_INVALID_ARGUMENT;
    PQclear(last);

    // written only on success
    if (result == 0)
    {
        *_changes = count;
    }

    return result;
}

// 4.3    Rows
//------------------------------------------------------------------------------
/*
d_internal_postgres_oid_type
  The OIDs are fixed: the built-in types' have never changed, and every
release keeps them.
*/
enum d_db_type
d_internal_postgres_oid_type(
    Oid _oid
)
{
    switch (_oid)
    {
        case 16:    // boolean
            return D_DB_TYPE_BOOLEAN;

        case 21:    // smallint
        case 23:    // integer
            return D_DB_TYPE_INTEGER;

        case 20:    // bigint
        case 26:    // oid
            return D_DB_TYPE_BIG_INTEGER;

        case 700:   // real
        case 701:   // double precision
            return D_DB_TYPE_FLOATING_POINT;

        case 1700:  // numeric
            return D_DB_TYPE_DECIMAL;

        case 18:    // "char"
        case 19:    // name
        case 25:    // text
        case 1042:  // character
        case 1043:  // character varying
            return D_DB_TYPE_STRING;

        case 17:    // bytea
            return D_DB_TYPE_BINARY;

        case 1082:  // date
            return D_DB_TYPE_DATE;

        case 1083:  // time without time zone
        case 1266:  // time with time zone
            return D_DB_TYPE_TIME;

        case 1114:  // timestamp without time zone
            return D_DB_TYPE_DATETIME;

        case 1184:  // timestamp with time zone
            return D_DB_TYPE_TIMESTAMP;

        case 114:   // json
        case 3802:  // jsonb
            return D_DB_TYPE_JSON;

        case 142:   // xml
            return D_DB_TYPE_XML;

        case 2950:  // uuid
            return D_DB_TYPE_UUID;

        default:
            break;
    }

    return D_DB_TYPE_NONE;
}

/*
d_internal_postgres_oid_name
  As format_type writes each name; asking the server for the rest would cost
a round trip a row of metadata does not justify.
*/
const char*
d_internal_postgres_oid_name(
    Oid _oid
)
{
    static const struct
    {
        Oid         oid;   // the type's OID
        const char* name;  // as format_type writes it
    } names[] =
    {
        {   16, "boolean" },                     {   17, "bytea" },
        {   18, "\"char\"" },                    {   19, "name" },
        {   20, "bigint" },                      {   21, "smallint" },
        {   23, "integer" },                     {   25, "text" },
        {   26, "oid" },                         {  114, "json" },
        {  142, "xml" },                         {  650, "cidr" },
        {  700, "real" },                        {  701, "double precision" },
        {  705, "unknown" },                     {  790, "money" },
        {  829, "macaddr" },                     {  869, "inet" },
        { 1042, "character" },                   { 1043, "character varying" },
        { 1082, "date" },
        { 1083, "time without time zone" },
        { 1114, "timestamp without time zone" },
        { 1184, "timestamp with time zone" },
        { 1186, "interval" },
        { 1266, "time with time zone" },
        { 1560, "bit" },                         { 1562, "bit varying" },
        { 1700, "numeric" },                     { 2278, "void" },
        { 2950, "uuid" },                        { 3802, "jsonb" }
    };

    // the name of a tabulated type
    for (size_t i = 0u; i < (sizeof(names) / sizeof(names[0])); ++i)
    {
        if (names[i].oid == _oid)
        {
            return names[i].name;
        }
    }

    return "";
}

/**
 * @brief Adds one value, as the server sent it in text, to rows under
 *        construction, read by its column's type.
 *
 * @param[in,out] _rows    the builder.
 * @param[in]     _oid     the column's type.
 * @param[in]     _text    the value; `NULL` for SQL NULL.
 * @param[in]     _length  its bytes.
 * @return `true`, or `false` when memory ran out.
 *
 * A boolean is t or f; an integer's text is exact; a real number's is the
 * shortest that reads back as it, from 12, and NaN or an infinity as
 * PostgreSQL spells them, which db_internal's reader refuses. A bytea's text
 * is decoded by libpq, which reads both the hex and the escape format.
 */
static bool
d_internal_postgres_rows_value(
    struct d_internal_db_rows* _rows,
    Oid                        _oid,
    const char*                _text,
    size_t                     _length
)
{
    // SQL NULL
    if (!_text)
    {
        return d_internal_db_rows_null(_rows);
    }

    switch (_oid)
    {
        case 16:    // boolean
            if ( (strcmp(_text, "t") == 0) ||
                 (strcmp(_text, "f") == 0) )
            {
                return d_internal_db_rows_integer(_rows,
                                                  (_text[0] == 't') ? 1 : 0);
            }

            break;

        case 20:    // bigint
        case 21:    // smallint
        case 23:    // integer
        case 26:    // oid
        {
            int64_t integer = 0;

            // the integer, or the text below when it is none
            if (d_internal_db_parse_integer(_text, &integer))
            {
                return d_internal_db_rows_integer(_rows, integer);
            }

            break;
        }

        case 700:   // real
        case 701:   // double precision
        {
            double real = 0.0;

            // PostgreSQL's spellings of NaN and the infinities first
            if (strcmp(_text, "NaN") == 0)
            {
                return d_internal_db_rows_real(_rows, NAN);
            }

            if (strcmp(_text, "Infinity") == 0)
            {
                return d_internal_db_rows_real(_rows, INFINITY);
            }

            if (strcmp(_text, "-Infinity") == 0)
            {
                return d_internal_db_rows_real(_rows, -INFINITY);
            }

            if (d_internal_db_parse_real(_text, &real))
            {
                return d_internal_db_rows_real(_rows, real);
            }

            break;
        }

        case 17:    // bytea
        {
            size_t               size  = 0u;
            unsigned char* const bytes =
                PQunescapeBytea((const unsigned char*)_text, &size);

            // libpq ran out of memory
            if (!bytes)
            {
                return false;
            }

            const bool kept = d_internal_db_rows_blob(_rows, bytes, size);

            PQfreemem(bytes);

            return kept;
        }

        default:
            break;
    }

    return d_internal_db_rows_text(_rows, _text, _length);
}

/*
d_internal_postgres_collect_rows
  The whole result is in memory already, so each value is read straight from
it, and the result freed once every value is copied.
*/
int
d_internal_postgres_collect_rows(
    PGresult*         _result,
    struct d_db_rows* _rows
)
{
    struct d_internal_db_rows builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u, 0u
    };
    bool kept = true;

    // a command's result has no columns
    if (PQresultStatus(_result) == PGRES_TUPLES_OK)
    {
        const int columns = PQnfields(_result);
        const int count   = PQntuples(_result);

        // each column, described once
        for (int c = 0; ( (kept) && (c < columns) ); ++c)
        {
            const Oid oid = PQftype(_result, c);

            kept = d_internal_db_rows_column(&builder,
                                             PQfname(_result, c),
                                             d_internal_postgres_oid_type(oid),
                                             d_internal_postgres_oid_name(oid));
        }

        // each row, value by value
        for (int r = 0; ( (kept) && (r < count) ); ++r)
        {
            for (int c = 0; ( (kept) && (c < columns) ); ++c)
            {
                kept = d_internal_postgres_rows_value(
                           &builder,
                           PQftype(_result, c),
                           (PQgetisnull(_result, r, c))
                               ? NULL
                               : PQgetvalue(_result, r, c),
                           (size_t)PQgetlength(_result, r, c));
            }
        }
    }

    PQclear(_result);

    // memory ran out part-way
    if (!kept)
    {
        d_internal_db_rows_release(&builder);

        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    return (d_internal_db_rows_finish(&builder, _rows))
               ? 0
               : D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
}

// 4.4    Settling IF NOT EXISTS
//------------------------------------------------------------------------------
/*
d_internal_postgres_tolerate
  Inside a transaction the savepoint goes in the same string as the
statement, so a CREATE costs one round trip either way, and one more to
release the savepoint, or to roll back to it and release it.
*/
int
d_internal_postgres_tolerate(
    PGconn*     _db,
    int         _tolerated,
    const char* _format,
    ...
)
{
    struct d_internal_db_buffer        sql     = { NULL, 0u, 0u };
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);
    const bool                         guarded =
        (PQtransactionStatus(_db) == PQTRANS_INTRANS);
    enum d_internal_db_format_status   status  =
        (guarded) ? d_internal_db_format(&sql,
                                         &quoting,
                                         "SAVEPOINT d_postgres_tolerate; ")
                  : D_INTERNAL_DB_FORMAT_OK;
    va_list                            args;

    va_start(args, _format);

    // the statement, after the savepoint where there is one
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_db_vformat(&sql, &quoting, _format, args);
    }

    va_end(args);

    int result = d_internal_postgres_format_result(status);

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_send(_db, sql.data, NULL);
    d_internal_db_buffer_release(&sql);

    return d_internal_postgres_settle(_db, result, _tolerated, guarded);
}

//==============================================================================
// 5.  CATALOGUE
//==============================================================================

// 5.2    Lookups
//------------------------------------------------------------------------------
/*
d_internal_postgres_has
  One row answers yes.
*/
int
d_internal_postgres_has(
    PGconn*                                _db,
    enum d_internal_postgres_object        _object,
    const struct d_internal_postgres_path* _path,
    bool*                                  _exists
)
{
    PGresult* rows = NULL;

    // the answer is no until a row says otherwise
    if (!_exists)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_exists = false;

    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_path_ok(_object, _path, true)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_postgres_lookup(_db, _object, _path, &rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    *_exists = ( (rows) &&
                 (PQntuples(rows) > 0) );
    PQclear(rows);

    return 0;
}

/*
d_internal_postgres_list
  The lookup without a name, its rows collected.
*/
int
d_internal_postgres_list(
    PGconn*                                _db,
    enum d_internal_postgres_object        _object,
    const struct d_internal_postgres_path* _path,
    struct d_db_name_list*                 _names
)
{
    PGresult* rows = NULL;

    // parameter validation
    if ( (!_db)    ||
         (!_names) ||
         (!d_internal_postgres_path_ok(_object, _path, false)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const struct d_internal_postgres_path listing =
    {
        _path->schema, _path->table, NULL
    };
    const int result = d_internal_postgres_lookup(_db,
                                                  _object,
                                                  &listing,
                                                  &rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_postgres_collect(rows, _names);
}

/*
d_internal_postgres_match_schema
  NULL is current_schema(), which is itself NULL, matching nothing, when no
schema of the search path exists; "pg_temp" is the session's temporary
schema, whose real name is pg_temp_ and a number, and whose oid
pg_my_temp_schema() gives -- 0, matching nothing, before the session makes
one.
*/
enum d_internal_db_format_status
d_internal_postgres_match_schema(
    PGconn*                      _db,
    struct d_internal_db_buffer* _sql,
    const char*                  _schema
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);

    // the current schema, or the temporary one
    if (!_schema)
    {
        return d_internal_db_format(
                   _sql,
                   &quoting,
                   " AND n.nspname = pg_catalog.current_schema()");
    }

    if (strcmp(_schema, "pg_temp") == 0)
    {
        return d_internal_db_format(
                   _sql,
                   &quoting,
                   " AND n.oid = pg_catalog.pg_my_temp_schema()");
    }

    return d_internal_db_format(_sql,
                                &quoting,
                                " AND n.nspname = %Q::pg_catalog.name",
                                _schema);
}

#else

// an ISO C translation unit must declare something; built as stubs, the
// layer has no plumbing
typedef int d_internal_postgres_no_plumbing;

#endif  // D_INTERNAL_POSTGRES
