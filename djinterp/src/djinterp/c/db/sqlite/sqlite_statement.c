/*******************************************************************************
* djinterp [c]                                                sqlite_statement.c
*
* Implementation of sqlite_statement.h.
*   A statement is SQLite's own, so most functions are a check and a call.
* Running one goes through sqlite_internal.c, which resets the statement
* before its first step and after its last: the next run then starts clean,
* and no read lock outlives a run. A cursor's run steps only as far as its
* rows are fetched, and is reset at its end. Text and bytes are bound as
* copies (SQLITE_TRANSIENT), so a caller's buffer need not outlive the call.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_statement.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.22
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_statement.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strlen
// djinterp
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Converts a parameter's number to the int SQLite takes.
 *
 * @param[in]  _index  the number, from 1.
 * @param[out] _out    receives it as an int.
 * @return SQLITE_OK, or SQLITE_RANGE for 0 or a number no int holds, which
 *         no statement has.
 */
static int
d_internal_sqlite_parameter(
    size_t _index,
    int*   _out
)
{
    // no statement numbers a parameter 0, or past INT_MAX
    if ( (_index == 0u) ||
         (_index > (size_t)INT_MAX) )
    {
        return SQLITE_RANGE;
    }

    *_out = (int)_index;

    return SQLITE_OK;
}

/**
 * @brief Binds text, copied, by its length.
 *
 * @param[in] _statement  the statement.
 * @param[in] _index      the parameter.
 * @param[in] _text       the text; not `NULL`.
 * @return what SQLite's bind returns; SQLITE_TOOBIG for text whose length
 *         SQLite's int-sized bind cannot take.
 */
static int
d_internal_sqlite_bind_text(
    sqlite3_stmt* _statement,
    int           _index,
    const char*   _text
)
{
    const size_t length = strlen(_text);

#if D_INTERNAL_SQLITE_BIND64
    return sqlite3_bind_text64(_statement, _index, _text,
                               (sqlite3_uint64)length, SQLITE_TRANSIENT,
                               SQLITE_UTF8);
#else
    // a length the int-sized bind would truncate
    if (length > (size_t)INT_MAX)
    {
        return SQLITE_TOOBIG;
    }

    return sqlite3_bind_text(_statement, _index, _text, (int)length,
                             SQLITE_TRANSIENT);
#endif
}

/**
 * @brief Binds bytes, copied.
 *
 * @param[in] _statement  the statement.
 * @param[in] _index      the parameter.
 * @param[in] _bytes      the bytes; not `NULL`.
 * @param[in] _size       how many.
 * @return what SQLite's bind returns; SQLITE_TOOBIG for a size SQLite's
 *         int-sized bind cannot take.
 */
static int
d_internal_sqlite_bind_blob(
    sqlite3_stmt* _statement,
    int           _index,
    const void*   _bytes,
    size_t        _size
)
{
#if D_INTERNAL_SQLITE_BIND64
    return sqlite3_bind_blob64(_statement, _index, _bytes,
                               (sqlite3_uint64)_size, SQLITE_TRANSIENT);
#else
    // a size the int-sized bind would truncate
    if (_size > (size_t)INT_MAX)
    {
        return SQLITE_TOOBIG;
    }

    return sqlite3_bind_blob(_statement, _index, _bytes, (int)_size,
                             SQLITE_TRANSIENT);
#endif
}

//==============================================================================
// 1.  STATEMENTS
//==============================================================================

// 1.1    Preparing
//------------------------------------------------------------------------------
/*
d_sqlite_prepare
  sqlite_internal.c prepares the first statement and reads, without compiling
it, whatever follows.
*/
int
d_sqlite_prepare(
    sqlite3*       _db,
    sqlite3_stmt** _statement,
    const char*    _sql
)
{
    // no place to put a statement
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    *_statement = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_prepare_one(_db, _statement, _sql);
}

/*
d_sqlite_preparef
  The format is checked before va_start, so a misuse returns without touching
the arguments.
*/
int
d_sqlite_preparef(
    sqlite3*       _db,
    sqlite3_stmt** _statement,
    const char*    _format,
    ...
)
{
    // no place to put a statement
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    *_statement = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_format) )
    {
        return SQLITE_MISUSE;
    }

    va_list args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vprepare_one(_db, _statement,
                                                      _format, args);

    va_end(args);

    return result;
}

// 1.2    Finalizing
//------------------------------------------------------------------------------
/*
d_sqlite_finalize
  sqlite3_finalize answers with the failure of the statement's last step,
which that step's own caller has already seen; finalizing itself cannot fail.
*/
int
d_sqlite_finalize(
    sqlite3_stmt* _statement
)
{
    (void)sqlite3_finalize(_statement);

    return SQLITE_OK;
}

// 1.3    Describing
//------------------------------------------------------------------------------
/*
d_sqlite_parameter_count
  sqlite3_bind_parameter_count is the highest number, which ?NNN can make
larger than the count of parameters written.
*/
size_t
d_sqlite_parameter_count(
    sqlite3_stmt* _statement
)
{
    // no statement has no parameters
    if (!_statement)
    {
        return 0u;
    }

    return (size_t)sqlite3_bind_parameter_count(_statement);
}

/*
d_sqlite_parameter_index
  SQLite matches the name with its prefix, exactly as written.
*/
int
d_sqlite_parameter_index(
    sqlite3_stmt* _statement,
    const char*   _name,
    size_t*       _index
)
{
    // parameter validation
    if ( (!_statement) ||
         (!_name)      ||
         (!_index) )
    {
        return SQLITE_MISUSE;
    }

    const int index = sqlite3_bind_parameter_index(_statement, _name);

    // 0 is SQLite's answer for no such name
    if (index == 0)
    {
        return D_DB_NOT_FOUND;
    }

    *_index = (size_t)index;

    return SQLITE_OK;
}

/*
d_sqlite_statement_connection
  sqlite3_db_handle, NULL-safe.
*/
sqlite3*
d_sqlite_statement_connection(
    sqlite3_stmt* _statement
)
{
    return (_statement) ? sqlite3_db_handle(_statement) : NULL;
}

//==============================================================================
// 2.  VALUES
//==============================================================================

// 2.1    Binding
//------------------------------------------------------------------------------
/*
d_sqlite_bind_null
  sqlite3_bind_null, the number checked first.
*/
int
d_sqlite_bind_null(
    sqlite3_stmt* _statement,
    size_t        _index
)
{
    int index = 0;

    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    const int range = d_internal_sqlite_parameter(_index, &index);

    return (range == SQLITE_OK) ? sqlite3_bind_null(_statement, index)
                                : range;
}

/*
d_sqlite_bind_bool
  SQLite has no boolean: 1 and 0, as its own TRUE and FALSE are.
*/
int
d_sqlite_bind_bool(
    sqlite3_stmt* _statement,
    size_t        _index,
    bool          _value
)
{
    int index = 0;

    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    const int range = d_internal_sqlite_parameter(_index, &index);

    return (range == SQLITE_OK)
               ? sqlite3_bind_int(_statement, index, (_value) ? 1 : 0)
               : range;
}

/*
d_sqlite_bind_int64
  sqlite3_bind_int64, the number checked first.
*/
int
d_sqlite_bind_int64(
    sqlite3_stmt* _statement,
    size_t        _index,
    int64_t       _value
)
{
    int index = 0;

    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    const int range = d_internal_sqlite_parameter(_index, &index);

    return (range == SQLITE_OK)
               ? sqlite3_bind_int64(_statement, index,
                                    (sqlite3_int64)_value)
               : range;
}

/*
d_sqlite_bind_double
  sqlite3_bind_double, which stores a NaN as NULL.
*/
int
d_sqlite_bind_double(
    sqlite3_stmt* _statement,
    size_t        _index,
    double        _value
)
{
    int index = 0;

    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    const int range = d_internal_sqlite_parameter(_index, &index);

    return (range == SQLITE_OK)
               ? sqlite3_bind_double(_statement, index, _value)
               : range;
}

/*
d_sqlite_bind_text
  A NULL string binds NULL, as SQLite's own bind would.
*/
int
d_sqlite_bind_text(
    sqlite3_stmt* _statement,
    size_t        _index,
    const char*   _text
)
{
    int index = 0;

    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    const int range = d_internal_sqlite_parameter(_index, &index);

    // a parameter the statement cannot have
    if (range != SQLITE_OK)
    {
        return range;
    }

    return (_text) ? d_internal_sqlite_bind_text(_statement, index, _text)
                   : sqlite3_bind_null(_statement, index);
}

/*
d_sqlite_bind_blob
  A NULL pointer binds NULL, as SQLite's own bind would; a pointer to no
bytes binds the empty BLOB.
*/
int
d_sqlite_bind_blob(
    sqlite3_stmt* _statement,
    size_t        _index,
    const void*   _bytes,
    size_t        _size
)
{
    int index = 0;

    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    const int range = d_internal_sqlite_parameter(_index, &index);

    // a parameter the statement cannot have
    if (range != SQLITE_OK)
    {
        return range;
    }

    return (_bytes) ? d_internal_sqlite_bind_blob(_statement, index, _bytes,
                                                  _size)
                    : sqlite3_bind_null(_statement, index);
}

// 2.2    Clearing
//------------------------------------------------------------------------------
/*
d_sqlite_clear_bindings
  sqlite3_clear_bindings, which cannot fail -- and which, unlike SQLite's
binds, does not refuse a statement part-way through a run: the run may still
be reading the values it would release, so the layer refuses it, as the binds
do.
*/
int
d_sqlite_clear_bindings(
    sqlite3_stmt* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    // a run part-way through still reads the values
    if (sqlite3_stmt_busy(_statement))
    {
        return SQLITE_MISUSE;
    }

    (void)sqlite3_clear_bindings(_statement);

    return SQLITE_OK;
}

//==============================================================================
// 3.  RUNNING
//==============================================================================

// 3.1    Rows changed
//------------------------------------------------------------------------------
/*
d_sqlite_execute_update
  The count lands in a local first, so a failure writes nothing.
*/
int
d_sqlite_execute_update(
    sqlite3_stmt* _statement,
    int64_t*      _changes
)
{
    int64_t changes = 0;

    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    const int result = d_internal_sqlite_run(_statement, &changes);

    // the count, when there is one and somewhere to put it
    if ( (result == SQLITE_OK) &&
         (_changes) )
    {
        *_changes = changes;
    }

    return result;
}

// 3.2    Rows
//------------------------------------------------------------------------------
/*
d_sqlite_execute_query
  sqlite_internal.c's collector, which the rows query uses too.
*/
int
d_sqlite_execute_query(
    sqlite3_stmt*     _statement,
    struct d_db_rows* _rows
)
{
    // parameter validation
    if ( (!_statement) ||
         (!_rows) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_collect_rows(_statement, _rows);
}

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/*
d_sqlite_open_cursor
  A run is SQLite's own stepping: nothing is run before the first fetch, and
a reset starts afresh.
*/
int
d_sqlite_open_cursor(
    sqlite3_stmt* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    (void)sqlite3_reset(_statement);

    return SQLITE_OK;
}

/*
d_sqlite_fetch_rows
  A step past the last row ends the run, the statement reset at once, so that
no read lock outlives it; a fetch never steps further after the rows it was
asked for, so the run goes on where it stopped. A failure drops what this
fetch kept, and resets the statement too.
*/
int
d_sqlite_fetch_rows(
    sqlite3_stmt*     _statement,
    size_t            _max,
    struct d_db_rows* _rows
)
{
    struct d_internal_db_rows builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u, 0u
    };
    size_t                    fetched = 0u;

    // parameter validation
    if ( (!_statement) ||
         (!_rows)      ||
         (_max == 0u) )
    {
        return SQLITE_MISUSE;
    }

    int step = d_internal_sqlite_describe_columns(_statement, &builder);

    // up to the rows asked for, each kept as it arrives
    while ( (step == SQLITE_OK) &&
            (fetched < _max) )
    {
        step = sqlite3_step(_statement);

        if (step == SQLITE_ROW)
        {
            step = d_internal_sqlite_keep_row(_statement, &builder);
            ++fetched;
        }
    }

    // the run's end, or its failure, ends it
    if (step != SQLITE_OK)
    {
        (void)sqlite3_reset(_statement);
    }

    if ( (step != SQLITE_OK) &&
         (step != SQLITE_DONE) )
    {
        d_internal_db_rows_release(&builder);

        return step;
    }

    // rows that cannot be handed over end the run too, rather than leave a
    // gap in it
    if (!d_internal_db_rows_finish(&builder, _rows))
    {
        (void)sqlite3_reset(_statement);

        return SQLITE_NOMEM;
    }

    return SQLITE_OK;
}

/*
d_sqlite_close_cursor
  The reset ends the run; what it reports is the run's, already seen.
*/
int
d_sqlite_close_cursor(
    sqlite3_stmt* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return SQLITE_MISUSE;
    }

    (void)sqlite3_reset(_statement);

    return SQLITE_OK;
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or 0 or NULL where it cannot fail. A statement
// a failed prepare promises is still written: NULL.

/*
d_sqlite_prepare
  Without SQLite there is nothing to compile: D_DB_UNSUPPORTED, and the
statement NULL, as a failed prepare leaves it.
*/
int
d_sqlite_prepare(
    struct sqlite3*       _db,
    struct sqlite3_stmt** _statement,
    const char*           _sql
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
d_sqlite_preparef
  As d_sqlite_prepare.
*/
int
d_sqlite_preparef(
    struct sqlite3*       _db,
    struct sqlite3_stmt** _statement,
    const char*           _format,
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
d_sqlite_finalize
  Without SQLite no statement exists, and releasing none succeeds.
*/
int
d_sqlite_finalize(
    struct sqlite3_stmt* _statement
)
{
    (void)_statement;

    return D_DB_OK;
}

/*
d_sqlite_parameter_count
  Without SQLite, 0.
*/
size_t
d_sqlite_parameter_count(
    struct sqlite3_stmt* _statement
)
{
    (void)_statement;

    return 0u;
}

/*
d_sqlite_parameter_index
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_parameter_index(
    struct sqlite3_stmt* _statement,
    const char*          _name,
    size_t*              _index
)
{
    (void)_statement;
    (void)_name;
    (void)_index;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_statement_connection
  Without SQLite, NULL.
*/
struct sqlite3*
d_sqlite_statement_connection(
    struct sqlite3_stmt* _statement
)
{
    (void)_statement;

    return NULL;
}

/*
d_sqlite_bind_null
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_bind_null(
    struct sqlite3_stmt* _statement,
    size_t               _index
)
{
    (void)_statement;
    (void)_index;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_bind_bool
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_bind_bool(
    struct sqlite3_stmt* _statement,
    size_t               _index,
    bool                 _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_bind_int64
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_bind_int64(
    struct sqlite3_stmt* _statement,
    size_t               _index,
    int64_t              _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_bind_double
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_bind_double(
    struct sqlite3_stmt* _statement,
    size_t               _index,
    double               _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_bind_text
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_bind_text(
    struct sqlite3_stmt* _statement,
    size_t               _index,
    const char*          _text
)
{
    (void)_statement;
    (void)_index;
    (void)_text;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_bind_blob
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_bind_blob(
    struct sqlite3_stmt* _statement,
    size_t               _index,
    const void*          _bytes,
    size_t               _size
)
{
    (void)_statement;
    (void)_index;
    (void)_bytes;
    (void)_size;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_clear_bindings
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_clear_bindings(
    struct sqlite3_stmt* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_execute_update
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_execute_update(
    struct sqlite3_stmt* _statement,
    int64_t*             _changes
)
{
    (void)_statement;
    (void)_changes;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_execute_query
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_execute_query(
    struct sqlite3_stmt* _statement,
    struct d_db_rows*    _rows
)
{
    (void)_statement;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_open_cursor
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_open_cursor(
    struct sqlite3_stmt* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_fetch_rows
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_fetch_rows(
    struct sqlite3_stmt* _statement,
    size_t               _max,
    struct d_db_rows*    _rows
)
{
    (void)_statement;
    (void)_max;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_close_cursor
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_close_cursor(
    struct sqlite3_stmt* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_SQLITE
