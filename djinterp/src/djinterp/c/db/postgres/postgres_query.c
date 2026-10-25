/*******************************************************************************
* djinterp [c]                                                  postgres_query.c
*
* Implementation of postgres_query.h.
*   Each function is a face of the plumbing in postgres_internal.c: the plain
* forms pass the caller's SQL through a "%s" format, so that a `%` in it is
* never read as a conversion, and the `f` forms pass the caller's format
* itself. libpq's simple query runs whatever statements the text holds and
* answers with the last one's result, which is the one the layer reads. The
* rows query runs its one statement through the extended protocol instead,
* as a prepared statement runs, which refuses a second statement.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.23
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_query.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../db_internal.h"  // buffers, formatting
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // INT64_MAX, int64_t


// as its header: below the 64-bit floor there is nothing to define
#if defined(INT64_MAX)

#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/*
d_internal_postgres_vexec
  Every running function's body: the arguments checked, the statement
formatted and sent, its rows discarded.
*/
static int
d_internal_postgres_vexec(
    PGconn*     _db,
    const char* _format,
    va_list     _args
)
{
    // parameter validation
    if ( (!_db) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_vrun(_db, NULL, _format, _args);
}

/*
d_internal_postgres_exec_as
  The plain forms' way through the plumbing: the caller's SQL as the one
argument of "%s", so that a `%` in it stays text.
*/
static int
d_internal_postgres_exec_as(
    PGconn*     _db,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_postgres_vexec(_db, _format, args);

    va_end(args);

    return result;
}

/*
d_internal_postgres_changes_as
  As d_internal_postgres_exec_as, for the count.
*/
static int
d_internal_postgres_changes_as(
    PGconn*     _db,
    int64_t*    _changes,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_postgres_vchanges(_db,
                                                    _changes,
                                                    _format,
                                                    args);

    va_end(args);

    return result;
}

/*
d_internal_postgres_text_as
  As d_internal_postgres_exec_as, for a single value as text.
*/
static int
d_internal_postgres_text_as(
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
d_internal_postgres_double_as
  As d_internal_postgres_exec_as, for a single value as a double.
*/
static int
d_internal_postgres_double_as(
    PGconn*     _db,
    double*     _value,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result =
        d_internal_postgres_vquery_double(_db, _value, _format, args);

    va_end(args);

    return result;
}

//==============================================================================
// 1.  QUERIES
//==============================================================================

// 1.1    Running statements
//------------------------------------------------------------------------------
/*
d_postgres_exec
  The SQL as written.
*/
int
d_postgres_exec(
    struct pg_conn* _db,
    const char*     _sql
)
{
    // parameter validation
    if (!_sql)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_exec_as(_db, "%s", _sql);
}

/*
d_postgres_execf
  The caller's format.
*/
int
d_postgres_execf(
    struct pg_conn* _db,
    const char*     _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_postgres_vexec(_db, _format, args);

    va_end(args);

    return result;
}

// 1.2    Rows changed
//------------------------------------------------------------------------------
/*
d_postgres_exec_changes
  The SQL as written, its count read from the command tag.
*/
int
d_postgres_exec_changes(
    struct pg_conn* _db,
    int64_t*        _changes,
    const char*     _sql
)
{
    // parameter validation
    if ( (!_db)      ||
         (!_changes) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_changes_as(_db, _changes, "%s", _sql);
}

/*
d_postgres_execf_changes
  The caller's format, its count read from the command tag.
*/
int
d_postgres_execf_changes(
    struct pg_conn* _db,
    int64_t*        _changes,
    const char*     _format,
    ...
)
{
    va_list args;

    // parameter validation
    if ( (!_db)      ||
         (!_changes) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    va_start(args, _format);

    const int result = d_internal_postgres_vchanges(_db,
                                                    _changes,
                                                    _format,
                                                    args);

    va_end(args);

    return result;
}

// 1.3    Single values
//------------------------------------------------------------------------------
/*
d_postgres_query_int64
  The SQL as written, its value read as an integer.
*/
int
d_postgres_query_int64(
    struct pg_conn* _db,
    int64_t*        _value,
    const char*     _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_query_int64(_db, _value, "%s", _sql);
}

/*
d_postgres_query_int64f
  The caller's format, its value read as an integer.
*/
int
d_postgres_query_int64f(
    struct pg_conn* _db,
    int64_t*        _value,
    const char*     _format,
    ...
)
{
    va_list args;

    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    va_start(args, _format);

    const int result =
        d_internal_postgres_vquery_int64(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_postgres_query_double
  The SQL as written, its value read as a double.
*/
int
d_postgres_query_double(
    struct pg_conn* _db,
    double*         _value,
    const char*     _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_double_as(_db, _value, "%s", _sql);
}

/*
d_postgres_query_doublef
  The caller's format, its value read as a double.
*/
int
d_postgres_query_doublef(
    struct pg_conn* _db,
    double*         _value,
    const char*     _format,
    ...
)
{
    va_list args;

    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    va_start(args, _format);

    const int result =
        d_internal_postgres_vquery_double(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_postgres_query_text
  The SQL as written, its value copied out.
*/
int
d_postgres_query_text(
    struct pg_conn* _db,
    char**          _value,
    const char*     _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_text_as(_db, _value, "%s", _sql);
}

/*
d_postgres_query_textf
  The caller's format, its value copied out.
*/
int
d_postgres_query_textf(
    struct pg_conn* _db,
    char**          _value,
    const char*     _format,
    ...
)
{
    va_list args;

    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    va_start(args, _format);

    const int result = d_internal_postgres_vquery(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_postgres_query_bool
  The SQL as written, its value read as t or f.
*/
int
d_postgres_query_bool(
    struct pg_conn* _db,
    bool*           _value,
    const char*     _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_query_bool(_db, _value, "%s", _sql);
}

/*
d_postgres_query_boolf
  The caller's format, its value read as t or f.
*/
int
d_postgres_query_boolf(
    struct pg_conn* _db,
    bool*           _value,
    const char*     _format,
    ...
)
{
    va_list args;

    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    va_start(args, _format);

    const int result =
        d_internal_postgres_vquery_bool(_db, _value, _format, args);

    va_end(args);

    return result;
}

// 1.4    Rows
//------------------------------------------------------------------------------
/*
d_postgres_query_rows
  PQexecParams with no parameters: one statement, its rows as text.
*/
int
d_postgres_query_rows(
    PGconn*           _db,
    struct d_db_rows* _rows,
    const char*       _sql
)
{
    PGresult* result_rows = NULL;

    // parameter validation
    if ( (!_db)   ||
         (!_rows) ||
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

    const int result = d_internal_postgres_take(
                           _db,
                           PQexecParams(_db, _sql, 0, NULL, NULL, NULL,
                                        NULL, 0),
                           &result_rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_postgres_collect_rows(result_rows, _rows);
}

/*
d_postgres_query_rows_params
  PQexecParams with the values as text, the server reading each as its
parameter's type, as an unspecified type lets it; 65535 is the protocol's
limit, which libpq would refuse without an SQLSTATE.
*/
int
d_postgres_query_rows_params(
    PGconn*            _db,
    struct d_db_rows*  _rows,
    const char*        _sql,
    const char* const* _values,
    size_t             _count
)
{
    PGresult* result_rows = NULL;

    // parameter validation
    if ( (!_db)                     ||
         (!_rows)                   ||
         (!_sql)                    ||
         ( (!_values) &&
           (_count != 0u) )         ||
         (_count > 65535u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int ready = d_internal_postgres_ready(_db);

    // a run holds the connection
    if (ready != 0)
    {
        return ready;
    }

    const int result = d_internal_postgres_take(
                           _db,
                           PQexecParams(_db, _sql, (int)_count, NULL,
                                        _values, NULL, NULL, 0),
                           &result_rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_postgres_collect_rows(result_rows, _rows);
}

/*
d_postgres_query_rowsf
  Formatted with the connection's quoting, then as d_postgres_query_rows.
*/
int
d_postgres_query_rowsf(
    PGconn*           _db,
    struct d_db_rows* _rows,
    const char*       _format,
    ...
)
{
    struct d_internal_db_buffer sql = { NULL, 0u, 0u };
    va_list                     args;

    // parameter validation
    if ( (!_db)   ||
         (!_rows) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int ready = d_internal_postgres_ready(_db);

    // a run holds the connection
    if (ready != 0)
    {
        return ready;
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

    PGresult* result_rows = NULL;

    result = d_internal_postgres_take(
                 _db,
                 PQexecParams(_db, sql.data, 0, NULL, NULL, NULL, NULL, 0),
                 &result_rows);
    d_internal_db_buffer_release(&sql);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_postgres_collect_rows(result_rows, _rows);
}

// 1.5    Quoting
//------------------------------------------------------------------------------
/*
d_postgres_quote_identifier
  The layer's own quoting, into a string of its own.
*/
char*
d_postgres_quote_identifier(
    const char* _name
)
{
    struct d_internal_db_buffer quoted = { NULL, 0u, 0u };

    // a name, quoted and terminated, or nothing
    if ( (!_name)                                                     ||
         (!d_internal_postgres_quote_name(NULL, &quoted, _name))      ||
         (!d_internal_db_buffer_append(&quoted, "", 1u)) )
    {
        d_internal_db_buffer_release(&quoted);

        return NULL;
    }

    return quoted.data;
}

/*
d_postgres_quote_literal
  libpq's quoting, copied into memory of the layer's, so that d_db_free
releases it on every C runtime; NULL is the keyword.
*/
char*
d_postgres_quote_literal(
    struct pg_conn* _db,
    const char*     _text
)
{
    struct d_internal_db_buffer quoted = { NULL, 0u, 0u };

    // parameter validation
    if (!_db)
    {
        return NULL;
    }

    const bool ok = (_text)
                        ? d_internal_postgres_quote_value(_db, &quoted, _text)
                        : d_internal_db_buffer_append(&quoted, "NULL", 4u);

    // the value, terminated, or nothing
    if ( (!ok) ||
         (!d_internal_db_buffer_append(&quoted, "", 1u)) )
    {
        d_internal_db_buffer_release(&quoted);

        return NULL;
    }

    return quoted.data;
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_exec
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_exec(
    struct pg_conn* _db,
    const char*     _sql
)
{
    (void)_db;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_execf
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_execf(
    struct pg_conn* _db,
    const char*     _format,
    ...
)
{
    (void)_db;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_exec_changes
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_exec_changes(
    struct pg_conn* _db,
    int64_t*        _changes,
    const char*     _sql
)
{
    (void)_db;
    (void)_changes;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_execf_changes
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_execf_changes(
    struct pg_conn* _db,
    int64_t*        _changes,
    const char*     _format,
    ...
)
{
    (void)_db;
    (void)_changes;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_int64
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_int64(
    struct pg_conn* _db,
    int64_t*        _value,
    const char*     _sql
)
{
    (void)_db;
    (void)_value;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_int64f
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_int64f(
    struct pg_conn* _db,
    int64_t*        _value,
    const char*     _format,
    ...
)
{
    (void)_db;
    (void)_value;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_double
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_double(
    struct pg_conn* _db,
    double*         _value,
    const char*     _sql
)
{
    (void)_db;
    (void)_value;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_doublef
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_doublef(
    struct pg_conn* _db,
    double*         _value,
    const char*     _format,
    ...
)
{
    (void)_db;
    (void)_value;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_text
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_text(
    struct pg_conn* _db,
    char**          _value,
    const char*     _sql
)
{
    (void)_db;
    (void)_value;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_textf
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_textf(
    struct pg_conn* _db,
    char**          _value,
    const char*     _format,
    ...
)
{
    (void)_db;
    (void)_value;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_bool
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_bool(
    struct pg_conn* _db,
    bool*           _value,
    const char*     _sql
)
{
    (void)_db;
    (void)_value;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_boolf
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_boolf(
    struct pg_conn* _db,
    bool*           _value,
    const char*     _format,
    ...
)
{
    (void)_db;
    (void)_value;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_rows
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_rows(
    struct pg_conn*   _db,
    struct d_db_rows* _rows,
    const char*       _sql
)
{
    (void)_db;
    (void)_rows;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_rows_params
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_rows_params(
    struct pg_conn*    _db,
    struct d_db_rows*  _rows,
    const char*        _sql,
    const char* const* _values,
    size_t             _count
)
{
    (void)_db;
    (void)_rows;
    (void)_sql;
    (void)_values;
    (void)_count;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_query_rowsf
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_query_rowsf(
    struct pg_conn*   _db,
    struct d_db_rows* _rows,
    const char*       _format,
    ...
)
{
    (void)_db;
    (void)_rows;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_quote_identifier
  Without libpq, NULL.
*/
char*
d_postgres_quote_identifier(
    const char* _name
)
{
    (void)_name;

    return NULL;
}

/*
d_postgres_quote_literal
  Without libpq, NULL.
*/
char*
d_postgres_quote_literal(
    struct pg_conn* _db,
    const char*     _text
)
{
    (void)_db;
    (void)_text;

    return NULL;
}

#endif  // D_INTERNAL_POSTGRES

#endif  // defined(INT64_MAX)
