/*******************************************************************************
* djinterp [c]                                                    sqlite_query.c
*
* Implementation of sqlite_query.h.
*   The scalar queries prepare with sqlite3_prepare_v2 and hand the statement
* to the single-value steps in sqlite_internal.c, which finalize it whatever
* happens; the rows query prepares exactly one statement and collects it the
* way sqlite_statement.c does. The counted exec runs its statements one at a
* time, as sqlite3_exec does, so as to count the last. The quoting functions
* count first and allocate once.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_query.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_query.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, SIZE_MAX


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Prepares one statement, as written.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _sql        the statement.
 * @return SQLITE_OK, or the result of sqlite3_prepare_v2.
 *
 * Only the first statement in `_sql` is prepared; the rest is ignored, as a
 * single-value query has one statement to read from.
 */
static int
d_internal_sqlite_prepare_sql(
    sqlite3*       _db,
    sqlite3_stmt** _statement,
    const char*    _sql
)
{
    *_statement = NULL;

    return sqlite3_prepare_v2(_db, _sql, -1, _statement, NULL);
}

/**
 * @brief Quotes `_text` between `_quote` characters, doubling each one
 *        inside it.
 *
 * @param[in] _text   the text.
 * @param[in] _quote  the quote character.
 * @return the quoted copy from malloc, or `NULL` when memory ran out.
 *
 * The length is computed exactly before allocating, so one allocation
 * suffices and nothing is reallocated.
 */
static char*
d_internal_sqlite_quote(
    const char* _text,
    char        _quote
)
{
    const size_t length = strlen(_text);
    size_t       quotes = 0u;

    // count the quotes that need doubling
    for (size_t i = 0u; i < length; ++i)
    {
        if (_text[i] == _quote)
        {
            ++quotes;
        }
    }

    // refuse a size that cannot be represented
    if (length > ((SIZE_MAX - 3u) - quotes))
    {
        return NULL;
    }

    char* const result = (char*)malloc(length + quotes + 3u);

    // allocation failure
    if (!result)
    {
        return NULL;
    }

    size_t at = 0u;

    result[at++] = _quote;

    // copy, doubling each quote
    for (size_t i = 0u; i < length; ++i)
    {
        result[at++] = _text[i];

        if (_text[i] == _quote)
        {
            result[at++] = _quote;
        }
    }

    result[at++] = _quote;
    result[at]   = '\0';

    return result;
}

//==============================================================================
// 1.  QUERIES
//==============================================================================

// 1.1    Running statements
//------------------------------------------------------------------------------
/*
d_sqlite_exec
  sqlite3_exec with no callback: rows are stepped through and discarded.
*/
int
d_sqlite_exec(
    sqlite3*    _db,
    const char* _sql
)
{
    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return SQLITE_MISUSE;
    }

    return sqlite3_exec(_db, _sql, NULL, NULL, NULL);
}

/*
d_sqlite_execf
  The format is checked before va_start, so a misuse returns without touching
the arguments.
*/
int
d_sqlite_execf(
    sqlite3*    _db,
    const char* _format,
    ...
)
{
    // parameter validation
    if ( (!_db) ||
         (!_format) )
    {
        return SQLITE_MISUSE;
    }

    va_list args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vexecute(_db, _format, args);

    va_end(args);

    return result;
}

// 1.2    Rows changed
//------------------------------------------------------------------------------
/*
d_sqlite_exec_changes
  The statements run one at a time, counted as each finishes.
*/
int
d_sqlite_exec_changes(
    sqlite3*    _db,
    int64_t*    _changes,
    const char* _sql
)
{
    // parameter validation
    if ( (!_db)      ||
         (!_changes) ||
         (!_sql) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute_counted(_db, _changes, _sql);
}

/*
d_sqlite_execf_changes
  As d_sqlite_exec_changes, with the SQL formatted first.
*/
int
d_sqlite_execf_changes(
    sqlite3*    _db,
    int64_t*    _changes,
    const char* _format,
    ...
)
{
    // parameter validation
    if ( (!_db)      ||
         (!_changes) ||
         (!_format) )
    {
        return SQLITE_MISUSE;
    }

    va_list args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vexecute_counted(_db,
                                                          _changes,
                                                          _format,
                                                          args);

    va_end(args);

    return result;
}

// 1.3    Single values
//------------------------------------------------------------------------------
/*
d_sqlite_query_int64
  Prepare, then one step through sqlite_common.c's reader, which finalizes.
*/
int
d_sqlite_query_int64(
    sqlite3*    _db,
    int64_t*    _value,
    const char* _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    const int     result    = d_internal_sqlite_prepare_sql(_db,
                                                            &statement,
                                                            _sql);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_int64(statement, _value);
}

/*
d_sqlite_query_int64f
  As d_sqlite_query_int64, with the SQL formatted first.
*/
int
d_sqlite_query_int64f(
    sqlite3*    _db,
    int64_t*    _value,
    const char* _format,
    ...
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_format) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    va_list       args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vprepare(_db,
                                                  &statement,
                                                  _format,
                                                  args);

    va_end(args);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_int64(statement, _value);
}

/*
d_sqlite_query_double
  As d_sqlite_query_int64, reading a double.
*/
int
d_sqlite_query_double(
    sqlite3*    _db,
    double*     _value,
    const char* _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    const int     result    = d_internal_sqlite_prepare_sql(_db,
                                                            &statement,
                                                            _sql);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_double(statement, _value);
}

/*
d_sqlite_query_doublef
  As d_sqlite_query_double, with the SQL formatted first.
*/
int
d_sqlite_query_doublef(
    sqlite3*    _db,
    double*     _value,
    const char* _format,
    ...
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_format) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    va_list       args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vprepare(_db,
                                                  &statement,
                                                  _format,
                                                  args);

    va_end(args);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_double(statement, _value);
}

/*
d_sqlite_query_text
  As d_sqlite_query_int64, copying the text out before the statement goes.
*/
int
d_sqlite_query_text(
    sqlite3*    _db,
    char**      _value,
    const char* _sql
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_sql) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    const int     result    = d_internal_sqlite_prepare_sql(_db,
                                                            &statement,
                                                            _sql);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_text(statement, _value);
}

/*
d_sqlite_query_textf
  As d_sqlite_query_text, with the SQL formatted first.
*/
int
d_sqlite_query_textf(
    sqlite3*    _db,
    char**      _value,
    const char* _format,
    ...
)
{
    // parameter validation
    if ( (!_db)    ||
         (!_value) ||
         (!_format) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    va_list       args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vprepare(_db,
                                                  &statement,
                                                  _format,
                                                  args);

    va_end(args);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_text(statement, _value);
}

// 1.4    Rows
//------------------------------------------------------------------------------
/*
d_sqlite_query_rows
  Exactly one statement, collected as sqlite_statement.c collects one, and
finalized whatever happened.
*/
int
d_sqlite_query_rows(
    sqlite3*          _db,
    struct d_db_rows* _rows,
    const char*       _sql
)
{
    // parameter validation
    if ( (!_db)   ||
         (!_rows) ||
         (!_sql) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    int           result    = d_internal_sqlite_prepare_one(_db,
                                                            &statement,
                                                            _sql);

    // a statement that did not compile has nothing to run
    if (result != SQLITE_OK)
    {
        return result;
    }

    result = d_internal_sqlite_collect_rows(statement, _rows);

    (void)sqlite3_finalize(statement);

    return result;
}

/*
d_sqlite_query_rowsf
  As d_sqlite_query_rows, with the statement formatted first.
*/
int
d_sqlite_query_rowsf(
    sqlite3*          _db,
    struct d_db_rows* _rows,
    const char*       _format,
    ...
)
{
    // parameter validation
    if ( (!_db)   ||
         (!_rows) ||
         (!_format) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    va_list       args;

    va_start(args, _format);

    int result = d_internal_sqlite_vprepare_one(_db, &statement, _format,
                                                args);

    va_end(args);

    // a statement that did not compile has nothing to run
    if (result != SQLITE_OK)
    {
        return result;
    }

    result = d_internal_sqlite_collect_rows(statement, _rows);

    (void)sqlite3_finalize(statement);

    return result;
}

// 1.5    Quoting
//------------------------------------------------------------------------------
/*
d_sqlite_quote_identifier
  Double quotes are SQL's identifier quotes and SQLite's preferred ones; a
quoted name is never taken for a keyword or a string.
*/
char*
d_sqlite_quote_identifier(
    const char* _name
)
{
    // parameter validation
    if (!_name)
    {
        return NULL;
    }

    return d_internal_sqlite_quote(_name, '"');
}

/*
d_sqlite_quote_literal
  A NULL pointer becomes the keyword, as sqlite3_mprintf's %Q makes it, so the
result can stand anywhere a value can.
*/
char*
d_sqlite_quote_literal(
    const char* _text
)
{
    // the keyword, copied so that d_db_free can release it
    if (!_text)
    {
        char* const keyword = (char*)malloc(5u);

        // fill the copy, when there is one
        if (keyword)
        {
            memcpy(keyword, "NULL", 5u);
        }

        return keyword;
    }

    return d_internal_sqlite_quote(_text, '\'');
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_exec
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_exec(
    struct sqlite3* _db,
    const char*     _sql
)
{
    (void)_db;
    (void)_sql;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_execf
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_execf(
    struct sqlite3* _db,
    const char*     _format,
    ...
)
{
    (void)_db;
    (void)_format;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_exec_changes
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_exec_changes(
    struct sqlite3* _db,
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
d_sqlite_execf_changes
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_execf_changes(
    struct sqlite3* _db,
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
d_sqlite_query_int64
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_int64(
    struct sqlite3* _db,
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
d_sqlite_query_int64f
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_int64f(
    struct sqlite3* _db,
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
d_sqlite_query_double
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_double(
    struct sqlite3* _db,
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
d_sqlite_query_doublef
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_doublef(
    struct sqlite3* _db,
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
d_sqlite_query_text
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_text(
    struct sqlite3* _db,
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
d_sqlite_query_textf
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_textf(
    struct sqlite3* _db,
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
d_sqlite_query_rows
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_rows(
    struct sqlite3*   _db,
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
d_sqlite_query_rowsf
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_query_rowsf(
    struct sqlite3*   _db,
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
d_sqlite_quote_identifier
  Without SQLite, NULL.
*/
char*
d_sqlite_quote_identifier(
    const char* _name
)
{
    (void)_name;

    return NULL;
}

/*
d_sqlite_quote_literal
  Without SQLite, NULL.
*/
char*
d_sqlite_quote_literal(
    const char* _text
)
{
    (void)_text;

    return NULL;
}

#endif  // D_INTERNAL_SQLITE
