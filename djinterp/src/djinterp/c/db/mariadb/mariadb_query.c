/*******************************************************************************
* djinterp [c]                                                   mariadb_query.c
*
* Implementation of mariadb_query.h.
*   The plain forms pass SQL through untouched, as the `%s` of an otherwise
* empty format, so a `%` in it means nothing; the `f` forms format with the
* connection's quoting. A script turns the client's multi-statement mode on
* for its own call only. Values arrive as text, as the protocol sends them,
* and are read as numbers by db_internal's readers, which no C locale can
* mislead; rows are read by mariadb_internal.c, which prepared statements
* read theirs through too, so a value reads the same either way.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_query.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // NULL
#include <string.h>  // strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../db_internal.h"  // d_internal_db_buffer
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// 1.  QUERIES
//==============================================================================

// 1.1    Running statements
//------------------------------------------------------------------------------
/*
d_mariadb_exec
  One statement: the client's multi-statement mode is off, as the layer
connects, so a second one is the server's syntax error.
*/
int
d_mariadb_exec(
    struct d_mariadb* _db,
    const char*       _sql
)
{
    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_send(_db, _sql, strlen(_sql), NULL);
}

/*
d_mariadb_execf
  Formatted with the connection's quoting, then as d_mariadb_exec.
*/
int
d_mariadb_execf(
    struct d_mariadb* _db,
    const char*       _format,
    ...
)
{
    va_list args;

    // parameter validation
    if ( (!_db) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    va_start(args, _format);

    const int result = d_internal_mariadb_vrun(_db, NULL, _format, args);

    va_end(args);

    return result;
}

/*
d_mariadb_exec_script
  Multi-statement mode is turned on for the call and off after it. After a
failure it is left on until the layer next runs a statement, since turning
it off is itself a command, which would clear the error the caller is about
to read.
*/
int
d_mariadb_exec_script(
    struct d_mariadb* _db,
    const char*       _sql
)
{
    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    MYSQL* const native = _db->native;

    // the mode on, unless a failed script left it so
    if ( (!_db->multi_statements) &&
         (mysql_set_server_option(native,
                                  MYSQL_OPTION_MULTI_STATEMENTS_ON) != 0) )
    {
        return (int)mysql_errno(native);
    }

    _db->multi_statements = false;

    const int result = d_internal_mariadb_send(_db, _sql, strlen(_sql), NULL);

    // a failure keeps the mode, and with it the error, for now
    if (result != 0)
    {
        _db->multi_statements = true;

        return result;
    }

    return (mysql_set_server_option(native,
                                    MYSQL_OPTION_MULTI_STATEMENTS_OFF) == 0)
               ? 0
               : (int)mysql_errno(native);
}

// 1.2    Rows changed
//------------------------------------------------------------------------------
/*
d_mariadb_exec_changes
  The count is the client's, read straight after the statement and its
results, as d_mariadb_changes reads it: mysql_store_result leaves a SELECT's
row count there.
*/
int
d_mariadb_exec_changes(
    struct d_mariadb* _db,
    int64_t*          _changes,
    const char*       _sql
)
{
    // parameter validation
    if ( (!_db)      ||
         (!_changes) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_send(_db, _sql, strlen(_sql), NULL);

    // the count, read before anything else runs
    if (result == 0)
    {
        *_changes = (int64_t)mysql_affected_rows(_db->native);
    }

    return result;
}

/*
d_mariadb_execf_changes
  Formatted with the connection's quoting, then as d_mariadb_exec_changes.
*/
int
d_mariadb_execf_changes(
    struct d_mariadb* _db,
    int64_t*          _changes,
    const char*       _format,
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

    const int result = d_internal_mariadb_vrun(_db, NULL, _format, args);

    va_end(args);

    // the count, read before anything else runs
    if (result == 0)
    {
        *_changes = (int64_t)mysql_affected_rows(_db->native);
    }

    return result;
}

// 1.3    Single values
//------------------------------------------------------------------------------
/*
d_mariadb_query_int64
  The SQL as the `%s` of a format, so it passes through untouched.
*/
int
d_mariadb_query_int64(
    struct d_mariadb* _db,
    int64_t*          _value,
    const char*       _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_query_int64(_db, _value, "%s", _sql);
}

/*
d_mariadb_query_int64f
  Formatted with the connection's quoting.
*/
int
d_mariadb_query_int64f(
    struct d_mariadb* _db,
    int64_t*          _value,
    const char*       _format,
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
        d_internal_mariadb_vquery_int64(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_mariadb_query_double
  As d_mariadb_query_int64, reading a double.
*/
int
d_mariadb_query_double(
    struct d_mariadb* _db,
    double*           _value,
    const char*       _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_mariadb_query_doublef(_db, _value, "%s", _sql);
}

/*
d_mariadb_query_doublef
  As d_mariadb_query_int64f, reading a double.
*/
int
d_mariadb_query_doublef(
    struct d_mariadb* _db,
    double*           _value,
    const char*       _format,
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
        d_internal_mariadb_vquery_double(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_mariadb_query_text
  As d_mariadb_query_int64, copying the text.
*/
int
d_mariadb_query_text(
    struct d_mariadb* _db,
    char**            _value,
    const char*       _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_query(_db, _value, "%s", _sql);
}

/*
d_mariadb_query_textf
  As d_mariadb_query_int64f, copying the text.
*/
int
d_mariadb_query_textf(
    struct d_mariadb* _db,
    char**            _value,
    const char*       _format,
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

    const int result = d_internal_mariadb_vquery(_db, _value, _format, args);

    va_end(args);

    return result;
}

// 1.4    Rows
//------------------------------------------------------------------------------
/*
d_mariadb_query_rows
  The statement's first result, stored, then read into rows and freed.
*/
int
d_mariadb_query_rows(
    struct d_mariadb* _db,
    struct d_db_rows* _rows,
    const char*       _sql
)
{
    MYSQL_RES* result_rows = NULL;

    // parameter validation
    if ( (!_db)   ||
         (!_rows) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_send(_db, _sql, strlen(_sql),
                                               &result_rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_mariadb_collect_rows(result_rows, _rows);
}

/*
d_mariadb_query_rowsf
  Formatted with the connection's quoting, then as d_mariadb_query_rows.
*/
int
d_mariadb_query_rowsf(
    struct d_mariadb* _db,
    struct d_db_rows* _rows,
    const char*       _format,
    ...
)
{
    MYSQL_RES* result_rows = NULL;
    va_list    args;

    // parameter validation
    if ( (!_db)   ||
         (!_rows) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    va_start(args, _format);

    const int result = d_internal_mariadb_vrun(_db, &result_rows, _format,
                                               args);

    va_end(args);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_mariadb_collect_rows(result_rows, _rows);
}

// 1.5    Quoting
//------------------------------------------------------------------------------
/*
d_mariadb_quote_identifier
  The layer's own quoting, which needs no connection, then a terminator.
*/
char*
d_mariadb_quote_identifier(
    const char* _name
)
{
    struct d_internal_db_buffer quoted = { NULL, 0u, 0u };

    // a name to quote, quoted and terminated
    if ( (!_name)                                                    ||
         (!d_internal_mariadb_quote_name(NULL, &quoted, _name))      ||
         (!d_internal_db_buffer_append(&quoted, "", 1u)) )
    {
        d_internal_db_buffer_release(&quoted);

        return NULL;
    }

    return quoted.data;
}

/*
d_mariadb_quote_literal
  The client's escaping for the connection, quotes added; NULL is the
keyword.
*/
char*
d_mariadb_quote_literal(
    struct d_mariadb* _db,
    const char*       _text
)
{
    struct d_internal_db_buffer quoted = { NULL, 0u, 0u };

    // a connection whose rules apply
    if (!_db)
    {
        return NULL;
    }

    const bool written = (_text)
                             ? d_internal_mariadb_quote_value(_db,
                                                              &quoted,
                                                              _text)
                             : d_internal_db_buffer_append(&quoted,
                                                           "NULL",
                                                           4u);

    // the text and its terminator
    if ( (!written) ||
         (!d_internal_db_buffer_append(&quoted, "", 1u)) )
    {
        d_internal_db_buffer_release(&quoted);

        return NULL;
    }

    return quoted.data;
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_exec
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_exec(
    struct d_mariadb* _db,
    const char*       _sql
)
{
    (void)_db;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_execf
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_execf(
    struct d_mariadb* _db,
    const char*       _format,
    ...
)
{
    (void)_db;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_exec_script
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_exec_script(
    struct d_mariadb* _db,
    const char*       _sql
)
{
    (void)_db;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_query_int64
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_int64(
    struct d_mariadb* _db,
    int64_t*          _value,
    const char*       _sql
)
{
    (void)_db;
    (void)_value;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_query_int64f
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_int64f(
    struct d_mariadb* _db,
    int64_t*          _value,
    const char*       _format,
    ...
)
{
    (void)_db;
    (void)_value;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_query_double
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_double(
    struct d_mariadb* _db,
    double*           _value,
    const char*       _sql
)
{
    (void)_db;
    (void)_value;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_query_doublef
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_doublef(
    struct d_mariadb* _db,
    double*           _value,
    const char*       _format,
    ...
)
{
    (void)_db;
    (void)_value;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_query_text
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_text(
    struct d_mariadb* _db,
    char**            _value,
    const char*       _sql
)
{
    (void)_db;
    (void)_value;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_query_textf
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_textf(
    struct d_mariadb* _db,
    char**            _value,
    const char*       _format,
    ...
)
{
    (void)_db;
    (void)_value;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_exec_changes
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_exec_changes(
    struct d_mariadb* _db,
    int64_t*          _changes,
    const char*       _sql
)
{
    (void)_db;
    (void)_changes;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_execf_changes
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_execf_changes(
    struct d_mariadb* _db,
    int64_t*          _changes,
    const char*       _format,
    ...
)
{
    (void)_db;
    (void)_changes;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_query_rows
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_rows(
    struct d_mariadb* _db,
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
d_mariadb_query_rowsf
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_query_rowsf(
    struct d_mariadb* _db,
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
d_mariadb_quote_identifier
  Without a client, NULL.
*/
char*
d_mariadb_quote_identifier(
    const char* _name
)
{
    (void)_name;

    return NULL;
}

/*
d_mariadb_quote_literal
  Without a client, NULL.
*/
char*
d_mariadb_quote_literal(
    struct d_mariadb* _db,
    const char*       _text
)
{
    (void)_db;
    (void)_text;

    return NULL;
}

#endif  // D_INTERNAL_MARIADB
