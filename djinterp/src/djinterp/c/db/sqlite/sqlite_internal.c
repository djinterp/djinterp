/*******************************************************************************
* djinterp [c]                                                 sqlite_internal.c
*
* Implementation of the plumbing sqlite_internal.h declares for the SQLite
* layer's modules: name checks, and statements formatted, prepared, run and
* read.
*   A helper that runs a statement leaves the connection as the statement left
* it, so that after a failure sqlite3_errmsg still describes the failure.
*   A format is sqlite3_mprintf's, unless it names %I or %T, which
* sqlite3_mprintf stops at: then it is the shared formatter's
* (db_internal.h), with SQLite's quoting, the dialect every layer reads.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_internal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "./sqlite_internal.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strchr, strlen
// djinterp
#include "../db_internal.h"  // d_internal_db_names, d_internal_db_vformat
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_sqlite_formatted
//   struct: a statement's formatted text, and whose memory holds it:
// sqlite3_mprintf's, or the shared formatter's buffer.
struct d_internal_sqlite_formatted
{
    char*                       text;    // the statement; NULL before
    struct d_internal_db_buffer shared;  // the buffer, when the shared
                                         // formatter wrote it
};

/**
 * @brief Writes text between two `_quote` characters, each one inside it
 *        doubled: SQLite's quoting of a value ('...') and of a name ("..."),
 *        as the shared formatter asks for it (db_internal.h, 3.1).
 *
 * @param[in,out] _out    the buffer the text is appended to.
 * @param[in]     _text   the text.
 * @param[in]     _quote  the quote character.
 * @return `true`, or `false` when memory ran out, the buffer as it was.
 *
 * The text is copied a run at a time, each run ending at a quote, which is
 * then written a second time.
 */
static bool
d_internal_sqlite_append_quoted(
    struct d_internal_db_buffer* _out,
    const char*                  _text,
    char                         _quote
)
{
    const size_t length_before = _out->length;
    const char*  run           = _text;
    bool         ok            = d_internal_db_buffer_append(_out,
                                                             &_quote,
                                                             1u);

    // each run, through the next quote, which is written twice
    while ( (ok) &&
            (*run != '\0') )
    {
        const char* const quote  = strchr(run, _quote);
        const size_t      length = (quote) ? ((size_t)(quote - run) + 1u)
                                           : strlen(run);

        ok   = ( (d_internal_db_buffer_append(_out, run, length)) &&
                 ( (!quote) ||
                   (d_internal_db_buffer_append(_out, &_quote, 1u)) ) );
        run += length;
    }

    // the closing quote; a failure takes back everything written
    if ( (!ok) ||
         (!d_internal_db_buffer_append(_out, &_quote, 1u)) )
    {
        _out->length = length_before;

        return false;
    }

    return true;
}

/**
 * @brief The shared formatter's quoting of a value, for SQLite.
 *
 * @param[in]     _context  unused: SQLite's quoting needs no connection.
 * @param[in,out] _out      the buffer.
 * @param[in]     _text     the value.
 * @return `true`, or `false` when memory ran out, the buffer as it was.
 */
static bool
d_internal_sqlite_quote_value(
    void*                        _context,
    struct d_internal_db_buffer* _out,
    const char*                  _text
)
{
    (void)_context;

    return d_internal_sqlite_append_quoted(_out, _text, '\'');
}

/**
 * @brief The shared formatter's quoting of a name, for SQLite.
 *
 * @param[in]     _context  unused: SQLite's quoting needs no connection.
 * @param[in,out] _out      the buffer.
 * @param[in]     _name     the name.
 * @return `true`, or `false` when memory ran out, the buffer as it was.
 */
static bool
d_internal_sqlite_quote_name(
    void*                        _context,
    struct d_internal_db_buffer* _out,
    const char*                  _name
)
{
    (void)_context;

    return d_internal_sqlite_append_quoted(_out, _name, '"');
}

/**
 * @brief Whether a format names %I or %T, read as the shared formatter
 *        reads one: %% is a percent sign, and a conversion is a length
 *        modifier (ll, l or z), if any, and a letter.
 *
 * @param[in] _format  the format.
 * @return `true` when a conversion's letter is I or T.
 *
 * sqlite3_mprintf stops at either letter, so no format it writes in full
 * names them, and reading such a format in the shared dialect changes
 * nothing that worked.
 */
static bool
d_internal_sqlite_names_shared(
    const char* _format
)
{
    const char* cursor = strchr(_format, '%');

    // each conversion; the text between them does not matter
    while (cursor)
    {
        const char* letter = cursor + 1;

        // the length modifier, ll before l, which it begins with
        if ( (letter[0] == 'l') &&
             (letter[1] == 'l') )
        {
            letter += 2;
        }
        else if ( (letter[0] == 'l') ||
                  (letter[0] == 'z') )
        {
            ++letter;
        }

        // the letter that names the dialect
        if ( (*letter == 'I') ||
             (*letter == 'T') )
        {
            return true;
        }

        // past the letter, which may be a second percent sign; a format
        // that ends at its conversion has no letter to pass
        cursor = (*letter != '\0') ? strchr(letter + 1, '%')
                                    : NULL;
    }

    return false;
}

/**
 * @brief Formats a statement in the dialect its format is written in.
 *
 * @param[out] _formatted  receives the text, and the memory holding it;
 *                         released by d_internal_sqlite_formatted_release.
 * @param[in]  _format     the format.
 * @param[in]  _args       the format's arguments.
 * @return SQLITE_OK; SQLITE_NOMEM when memory ran out; SQLITE_MISUSE when
 *         a format in the shared dialect has a conversion it lacks, or an
 *         argument it refuses: a NULL name, a NUL character, a NaN or an
 *         infinity.
 */
static int
d_internal_sqlite_vformat(
    struct d_internal_sqlite_formatted* _formatted,
    const char*                         _format,
    va_list                             _args
)
{
    _formatted->text            = NULL;
    _formatted->shared.data     = NULL;
    _formatted->shared.length   = 0u;
    _formatted->shared.capacity = 0u;

    // sqlite3_mprintf's dialect, as every format without %I or %T is read
    if (!d_internal_sqlite_names_shared(_format))
    {
        _formatted->text = sqlite3_vmprintf(_format, _args);

        return (_formatted->text) ? SQLITE_OK
                                  : SQLITE_NOMEM;
    }

    const struct d_internal_db_quoting quoting =
    {
        d_internal_sqlite_quote_value,
        d_internal_sqlite_quote_name,
        NULL
    };
    const enum d_internal_db_format_status status =
        d_internal_db_vformat(&_formatted->shared, &quoting, _format, _args);

    // memory, or a format or an argument the dialect refuses
    if (status != D_INTERNAL_DB_FORMAT_OK)
    {
        return (status == D_INTERNAL_DB_FORMAT_NO_MEMORY) ? SQLITE_NOMEM
                                                          : SQLITE_MISUSE;
    }

    _formatted->text = _formatted->shared.data;

    return SQLITE_OK;
}

/**
 * @brief Releases a statement's formatted text, to whichever allocator
 *        holds it.
 *
 * @param[in,out] _formatted  the text; empty after.
 */
static void
d_internal_sqlite_formatted_release(
    struct d_internal_sqlite_formatted* _formatted
)
{
    // the shared formatter's buffer, or sqlite3_mprintf's text
    if (_formatted->shared.data)
    {
        d_internal_db_buffer_release(&_formatted->shared);
    }
    else
    {
        sqlite3_free(_formatted->text);
    }

    _formatted->text = NULL;

    return;
}

/**
 * @brief Whether text after a statement holds nothing SQLite would run:
 *        whitespace, semicolons and comments alone.
 *
 * @param[in] _tail  the text; `NULL` reads as none.
 * @return `true` when no other statement follows.
 *
 * Read as SQLite's tokenizer reads it: `--` comments to the end of the line,
 * and a block comment to its `*` `/`, or to the end of the text when it has
 * none.
 */
static bool
d_internal_sqlite_only_space(
    const char* _tail
)
{
    const char* at = (_tail) ? _tail : "";

    // each character, comment or separator, until something else
    while (*at != '\0')
    {
        if ( (*at == ' ')  ||
             (*at == '\t') ||
             (*at == '\n') ||
             (*at == '\f') ||
             (*at == '\r') ||
             (*at == ';') )
        {
            ++at;
        }
        else if ( (at[0] == '-') &&
                  (at[1] == '-') )
        {
            const char* const end = strchr(at, '\n');

            at = (end) ? (end + 1) : (at + strlen(at));
        }
        else if ( (at[0] == '/') &&
                  (at[1] == '*') )
        {
            const char* end = at + 2;

            // to the comment's end, or the text's
            while ( (*end != '\0') &&
                    ( (end[0] != '*') ||
                      (end[1] != '/') ) )
            {
                ++end;
            }

            at = (*end != '\0') ? (end + 2) : end;
        }
        else
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether a declared type names a word anywhere in it, ignoring ASCII
 *        case, as SQLite's affinity rules read one.
 *
 * @param[in] _declared  the declared type.
 * @param[in] _word      the word, in capitals.
 * @return `true` when the word appears.
 */
static bool
d_internal_sqlite_names_word(
    const char* _declared,
    const char* _word
)
{
    const size_t length = strlen(_word);

    // the word at each place it could start
    for (const char* at = _declared; *at != '\0'; ++at)
    {
        if (sqlite3_strnicmp(at, _word, (int)length) == 0)
        {
            return true;
        }
    }

    return false;
}

//==============================================================================
// 2.  NAMES
//==============================================================================

// 2.1    Schemas and names
//------------------------------------------------------------------------------
/*
d_internal_sqlite_schema
  "main" is the database an unqualified CREATE writes to, which makes it the
least surprising default for every function that takes a schema.
*/
const char*
d_internal_sqlite_schema(
    const char* _schema
)
{
    return (_schema) ? _schema : "main";
}

/*
d_internal_sqlite_schema_table
  temp's catalogue is sqlite_temp_master. The alias temp.sqlite_master
arrived only with 3.33.0's sqlite_schema renaming, so this names the one
spelling every version accepts. SQLite matches schema names ASCII-blind, as
sqlite3_stricmp does.
*/
const char*
d_internal_sqlite_schema_table(
    const char* _schema
)
{
    return (sqlite3_stricmp(d_internal_sqlite_schema(_schema), "temp") == 0)
               ? "sqlite_temp_master"
               : "sqlite_master";
}

/*
d_internal_sqlite_is_main
  As SQLite matches schema names: ASCII-blind.
*/
bool
d_internal_sqlite_is_main(
    const char* _schema
)
{
    return (sqlite3_stricmp(d_internal_sqlite_schema(_schema), "main") == 0);
}

/*
d_internal_sqlite_name_ok
  SQLite would accept an empty quoted name, but a caller passing one has
almost certainly made a mistake, and the layer says so with SQLITE_MISUSE.
*/
bool
d_internal_sqlite_name_ok(
    const char* _name
)
{
    return ( (_name) &&
             (_name[0] != '\0') );
}

/*
d_internal_sqlite_schema_ok
  NULL is the documented way to say "main"; an empty name is never one.
*/
bool
d_internal_sqlite_schema_ok(
    const char* _schema
)
{
    return ( (!_schema) ||
             (_schema[0] != '\0') );
}

/*
d_internal_sqlite_runtime_at_least
  The library actually loaded decides, not the header compiled against.
*/
bool
d_internal_sqlite_runtime_at_least(
    int _version
)
{
    return (sqlite3_libversion_number() >= _version);
}

//==============================================================================
// 3.  STATEMENTS
//==============================================================================

// 3.1    Preparing and running
//------------------------------------------------------------------------------
/*
d_internal_sqlite_vprepare
  The format's dialect does the quoting it asks for, and the formatted text
is freed as soon as SQLite has compiled it.
*/
int
d_internal_sqlite_vprepare(
    sqlite3*       _db,
    sqlite3_stmt** _statement,
    const char*    _format,
    va_list        _args
)
{
    struct d_internal_sqlite_formatted sql = { NULL, { NULL, 0u, 0u } };

    *_statement = NULL;

    const int formatted = d_internal_sqlite_vformat(&sql, _format, _args);

    // nothing to compile
    if (formatted != SQLITE_OK)
    {
        return formatted;
    }

    const int result = sqlite3_prepare_v2(_db, sql.text, -1, _statement, NULL);

    d_internal_sqlite_formatted_release(&sql);

    return result;
}

/*
d_internal_sqlite_prepare
  The variadic face of d_internal_sqlite_vprepare.
*/
int
d_internal_sqlite_prepare(
    sqlite3*       _db,
    sqlite3_stmt** _statement,
    const char*    _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vprepare(_db,
                                                  _statement,
                                                  _format,
                                                  args);

    va_end(args);

    return result;
}

/*
d_internal_sqlite_prepare_one
  sqlite3_prepare_v2 compiles the first statement and points past it; the
rest is read by hand rather than compiled, so a refusal does not replace the
connection's message with one about the text that follows.
*/
int
d_internal_sqlite_prepare_one(
    sqlite3*       _db,
    sqlite3_stmt** _statement,
    const char*    _sql
)
{
    const char* tail = NULL;

    *_statement = NULL;

    const int result = sqlite3_prepare_v2(_db, _sql, -1, _statement, &tail);

    // a statement that did not compile
    if (result != SQLITE_OK)
    {
        return result;
    }

    // text that held none, or more than one
    if ( (!*_statement) ||
         (!d_internal_sqlite_only_space(tail)) )
    {
        (void)sqlite3_finalize(*_statement);
        *_statement = NULL;

        return SQLITE_MISUSE;
    }

    return SQLITE_OK;
}

/*
d_internal_sqlite_vprepare_one
  The text after the statement is read before the formatted text is freed,
since the tail points into it.
*/
int
d_internal_sqlite_vprepare_one(
    sqlite3*       _db,
    sqlite3_stmt** _statement,
    const char*    _format,
    va_list        _args
)
{
    struct d_internal_sqlite_formatted sql = { NULL, { NULL, 0u, 0u } };

    *_statement = NULL;

    const int formatted = d_internal_sqlite_vformat(&sql, _format, _args);

    // nothing to compile
    if (formatted != SQLITE_OK)
    {
        return formatted;
    }

    const int result = d_internal_sqlite_prepare_one(_db, _statement,
                                                     sql.text);

    d_internal_sqlite_formatted_release(&sql);

    return result;
}

/*
d_internal_sqlite_vexecute
  sqlite3_exec runs every statement in the text and stops at the first
failure, leaving its message on the connection.
*/
int
d_internal_sqlite_vexecute(
    sqlite3*    _db,
    const char* _format,
    va_list     _args
)
{
    struct d_internal_sqlite_formatted sql = { NULL, { NULL, 0u, 0u } };
    const int formatted = d_internal_sqlite_vformat(&sql, _format, _args);

    // nothing to run
    if (formatted != SQLITE_OK)
    {
        return formatted;
    }

    const int result = sqlite3_exec(_db, sql.text, NULL, NULL, NULL);

    d_internal_sqlite_formatted_release(&sql);

    return result;
}

/*
d_internal_sqlite_execute
  The variadic face of d_internal_sqlite_vexecute.
*/
int
d_internal_sqlite_execute(
    sqlite3*    _db,
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_sqlite_vexecute(_db, _format, args);

    va_end(args);

    return result;
}

/*
d_internal_sqlite_execute_counted
  sqlite3_exec cannot say what any statement changed, so the statements are
prepared and run here one at a time, as it would run them, each counted by
d_internal_sqlite_run; the last count stands.
*/
int
d_internal_sqlite_execute_counted(
    sqlite3*    _db,
    int64_t*    _changes,
    const char* _sql
)
{
    const char* next    = _sql;
    int64_t     changes = 0;

    // each statement, until the text runs out
    while ( (next) &&
            (*next != '\0') )
    {
        sqlite3_stmt* statement = NULL;
        const char*   tail      = NULL;
        int           result    = sqlite3_prepare_v2(_db, next, -1,
                                                     &statement, &tail);

        // a statement that did not compile stops the rest
        if (result != SQLITE_OK)
        {
            return result;
        }

        // whitespace and comments compile to no statement
        if (statement)
        {
            result = d_internal_sqlite_run(statement, &changes);

            (void)sqlite3_finalize(statement);

            // a statement that failed stops the rest
            if (result != SQLITE_OK)
            {
                return result;
            }
        }

        next = tail;
    }

    *_changes = changes;

    return SQLITE_OK;
}

/*
d_internal_sqlite_vexecute_counted
  The formatted text outlives every statement prepared from it, since each
tail points into it.
*/
int
d_internal_sqlite_vexecute_counted(
    sqlite3*    _db,
    int64_t*    _changes,
    const char* _format,
    va_list     _args
)
{
    struct d_internal_sqlite_formatted sql = { NULL, { NULL, 0u, 0u } };
    const int formatted = d_internal_sqlite_vformat(&sql, _format, _args);

    // nothing to run
    if (formatted != SQLITE_OK)
    {
        return formatted;
    }

    const int result = d_internal_sqlite_execute_counted(_db, _changes,
                                                         sql.text);

    d_internal_sqlite_formatted_release(&sql);

    return result;
}

/*
d_internal_sqlite_finish
  With sqlite3_prepare_v2, sqlite3_finalize repeats the failure of the last
step; the step's own code is kept because it is the one the caller saw
happen. A successful step's result is whatever finalizing reports.
*/
int
d_internal_sqlite_finish(
    sqlite3_stmt* _statement,
    int           _result
)
{
    const int finalized = sqlite3_finalize(_statement);

    // SQLITE_ROW and SQLITE_DONE are a step's successes
    if ( (_result == SQLITE_OK)  ||
         (_result == SQLITE_ROW) ||
         (_result == SQLITE_DONE) )
    {
        return finalized;
    }

    return _result;
}

// 3.2    Single-value results
//------------------------------------------------------------------------------
/*
d_internal_sqlite_step_int64
  One step decides: a row carries the value, SQLITE_DONE means there was none.
*/
int
d_internal_sqlite_step_int64(
    sqlite3_stmt* _statement,
    int64_t*      _value
)
{
    const int step = sqlite3_step(_statement);

    // read the value while the row is current
    if (step == SQLITE_ROW)
    {
        *_value = (int64_t)sqlite3_column_int64(_statement, 0);
    }

    const int result = d_internal_sqlite_finish(_statement, step);

    // a failure stands; a success without a row is not one
    if (result != SQLITE_OK)
    {
        return result;
    }

    return (step == SQLITE_ROW) ? SQLITE_OK : D_DB_NOT_FOUND;
}

/*
d_internal_sqlite_step_double
  As d_internal_sqlite_step_int64, reading a double.
*/
int
d_internal_sqlite_step_double(
    sqlite3_stmt* _statement,
    double*       _value
)
{
    const int step = sqlite3_step(_statement);

    // read the value while the row is current
    if (step == SQLITE_ROW)
    {
        *_value = sqlite3_column_double(_statement, 0);
    }

    const int result = d_internal_sqlite_finish(_statement, step);

    // a failure stands; a success without a row is not one
    if (result != SQLITE_OK)
    {
        return result;
    }

    return (step == SQLITE_ROW) ? SQLITE_OK : D_DB_NOT_FOUND;
}

/*
d_internal_sqlite_step_text
  The text is copied before finalizing, since SQLite's copy dies with the
statement, and copied by length rather than to the first NUL, so text holding
NULs survives whole. sqlite3_column_text returning NULL for a value that is
not NULL means SQLite could not allocate the conversion.
*/
int
d_internal_sqlite_step_text(
    sqlite3_stmt* _statement,
    char**        _value
)
{
    int   step = sqlite3_step(_statement);
    char* copy = NULL;

    // copy the value while the row is current
    if ( (step == SQLITE_ROW) &&
         (sqlite3_column_type(_statement, 0) != SQLITE_NULL) )
    {
        const unsigned char* const text  = sqlite3_column_text(_statement, 0);
        const int                  bytes = sqlite3_column_bytes(_statement,
                                                                0);

        copy = (text) ? (char*)malloc((size_t)bytes + 1u) : NULL;

        // either SQLite or the copy ran out of memory
        if (!copy)
        {
            step = SQLITE_NOMEM;
        }
        else
        {
            memcpy(copy, text, (size_t)bytes);
            copy[bytes] = '\0';
        }
    }

    const int result = d_internal_sqlite_finish(_statement, step);

    // a failure stands, and the copy goes with it
    if (result != SQLITE_OK)
    {
        free(copy);

        return result;
    }

    // a success without a row is not one
    if (step != SQLITE_ROW)
    {
        return D_DB_NOT_FOUND;
    }

    *_value = copy;

    return SQLITE_OK;
}

// 3.3    Name lists
//------------------------------------------------------------------------------
/*
d_internal_sqlite_collect_names
  Names are copied into the builder as each row arrives, since SQLite's text
dies with the next step. A failure part-way keeps the step's code and drops
what was collected, so the caller never receives half a list.
*/
int
d_internal_sqlite_collect_names(
    sqlite3_stmt*          _statement,
    int                    _column,
    struct d_db_name_list* _names
)
{
    struct d_internal_db_names builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u
    };
    int step = sqlite3_step(_statement);

    // collect each row's name until the rows or the memory run out
    while (step == SQLITE_ROW)
    {
        const char* const name =
            (const char*)sqlite3_column_text(_statement, _column);

        // no text for a value that is not NULL: SQLite ran out of memory
        if ( (!name) &&
             (sqlite3_column_type(_statement, _column) != SQLITE_NULL) )
        {
            step = SQLITE_NOMEM;

            break;
        }

        // keep the name; a NULL value has none to keep
        if ( (name) &&
             (!d_internal_db_names_add(&builder, name)) )
        {
            step = SQLITE_NOMEM;

            break;
        }

        step = sqlite3_step(_statement);
    }

    const int result = d_internal_sqlite_finish(_statement, step);

    // a failure drops whatever was collected
    if (result != SQLITE_OK)
    {
        d_internal_db_names_release(&builder);

        return result;
    }

    return (d_internal_db_names_finish(&builder, _names))
               ? SQLITE_OK
               : SQLITE_NOMEM;
}

// 3.4    Rows
//------------------------------------------------------------------------------
/*
d_internal_sqlite_collect_rows
  The columns are described before the first step, which SQLite allows, and
each value is copied as its row arrives, since SQLite's copy dies with the
next step. sqlite3_column_text is read before sqlite3_column_bytes, the order
SQLite asks for, so the count is of the text read. A zero-length BLOB reads
as NULL from sqlite3_column_blob and is still a BLOB. The reset before the
first step may report the previous run's failure, which is not this run's;
the one after the last repeats this run's, which the caller already has.
*/
int
d_internal_sqlite_collect_rows(
    sqlite3_stmt*     _statement,
    struct d_db_rows* _rows
)
{
    struct d_internal_db_rows builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u, 0u
    };
    const int columns = sqlite3_column_count(_statement);
    int       step    = SQLITE_OK;

    (void)sqlite3_reset(_statement);

    // each column, described once
    for (int i = 0; ( (step == SQLITE_OK) && (i < columns) ); ++i)
    {
        const char* const name     = sqlite3_column_name(_statement, i);
        const char* const declared = sqlite3_column_decltype(_statement, i);

        // no name means SQLite ran out of memory
        if ( (!name) ||
             (!d_internal_db_rows_column(
                  &builder,
                  name,
                  d_internal_sqlite_declared_type(declared),
                  declared)) )
        {
            step = SQLITE_NOMEM;
        }
    }

    step = (step == SQLITE_OK) ? sqlite3_step(_statement) : step;

    // each row, value by value, until the rows or the memory run out
    while (step == SQLITE_ROW)
    {
        for (int i = 0; ( (step == SQLITE_ROW) && (i < columns) ); ++i)
        {
            bool kept = false;

            switch (sqlite3_column_type(_statement, i))
            {
                case SQLITE_INTEGER:
                    kept = d_internal_db_rows_integer(
                               &builder,
                               (int64_t)sqlite3_column_int64(_statement, i));

                    break;

                case SQLITE_FLOAT:
                    kept = d_internal_db_rows_real(
                               &builder,
                               sqlite3_column_double(_statement, i));

                    break;

                case SQLITE_TEXT:
                {
                    const unsigned char* const text =
                        sqlite3_column_text(_statement, i);
                    const int bytes = sqlite3_column_bytes(_statement, i);

                    kept = ( (text) &&
                             (d_internal_db_rows_text(&builder,
                                                      (const char*)text,
                                                      (size_t)bytes)) );

                    break;
                }

                case SQLITE_BLOB:
                {
                    const void* const blob  = sqlite3_column_blob(_statement,
                                                                  i);
                    const int         bytes = sqlite3_column_bytes(_statement,
                                                                   i);

                    kept = ( ( (blob) ||
                               (bytes == 0) ) &&
                             (d_internal_db_rows_blob(&builder,
                                                      blob,
                                                      (size_t)bytes)) );

                    break;
                }

                default:
                    kept = d_internal_db_rows_null(&builder);

                    break;
            }

            // the builder, or SQLite's conversion, ran out of memory
            if (!kept)
            {
                step = SQLITE_NOMEM;
            }
        }

        step = (step == SQLITE_ROW) ? sqlite3_step(_statement) : step;
    }

    (void)sqlite3_reset(_statement);

    // a failure drops whatever was collected
    if (step != SQLITE_DONE)
    {
        d_internal_db_rows_release(&builder);

        return step;
    }

    return (d_internal_db_rows_finish(&builder, _rows))
               ? SQLITE_OK
               : SQLITE_NOMEM;
}

/*
d_internal_sqlite_run
  sqlite3_changes keeps the count of the last INSERT, UPDATE or DELETE to
finish, so after a statement of any other kind it still holds an older
statement's. The running total moves only when rows change, and so tells the
two apart; a statement that changed none counts the rows it returned, if any.
*/
int
d_internal_sqlite_run(
    sqlite3_stmt* _statement,
    int64_t*      _changes
)
{
    sqlite3* const db       = sqlite3_db_handle(_statement);
    int64_t        returned = 0;

    (void)sqlite3_reset(_statement);

    const int64_t before = (int64_t)D_INTERNAL_SQLITE_TOTAL_CHANGES(db);
    int           step   = sqlite3_step(_statement);

    // every row, counted and discarded
    while (step == SQLITE_ROW)
    {
        ++returned;
        step = sqlite3_step(_statement);
    }

    // the count, read before the reset
    if (step == SQLITE_DONE)
    {
        const int64_t after = (int64_t)D_INTERNAL_SQLITE_TOTAL_CHANGES(db);

        *_changes = (after != before) ? (int64_t)D_INTERNAL_SQLITE_CHANGES(db)
                                      : returned;
    }

    (void)sqlite3_reset(_statement);

    return (step == SQLITE_DONE) ? SQLITE_OK : step;
}

/*
d_internal_sqlite_declared_type
  The common names are matched whole, what precedes any parenthesis, ignoring
ASCII case; anything else falls to SQLite's own affinity rules, in SQLite's
order -- INT first, so that SQLite's well-known reading of "FLOATING POINT"
as an integer type is kept, since it is how SQLite stores the values.
*/
enum d_db_type
d_internal_sqlite_declared_type(
    const char* _declared
)
{
    static const struct
    {
        const char*    name;  // as written, in capitals
        enum d_db_type type;  // what it names
    } names[] =
    {
        { "BOOLEAN",          D_DB_TYPE_BOOLEAN },
        { "BOOL",             D_DB_TYPE_BOOLEAN },
        { "TINYINT",          D_DB_TYPE_INTEGER },
        { "SMALLINT",         D_DB_TYPE_INTEGER },
        { "MEDIUMINT",        D_DB_TYPE_INTEGER },
        { "INT2",             D_DB_TYPE_INTEGER },
        { "DOUBLE PRECISION", D_DB_TYPE_FLOATING_POINT },
        { "DATETIME",         D_DB_TYPE_DATETIME },
        { "TIMESTAMP",        D_DB_TYPE_TIMESTAMP },
        { "DATE",             D_DB_TYPE_DATE },
        { "TIME",             D_DB_TYPE_TIME },
        { "JSON",             D_DB_TYPE_JSON },
        { "JSONB",            D_DB_TYPE_JSON },
        { "XML",              D_DB_TYPE_XML },
        { "UUID",             D_DB_TYPE_UUID },
        { "GUID",             D_DB_TYPE_UUID },
        { "ANY",              D_DB_TYPE_NONE }
    };

    // an expression's column has no declared type
    if (!_declared)
    {
        return D_DB_TYPE_NONE;
    }

    // the name, without spaces around it or any parenthesis after it
    const char* start = _declared;

    while (*start == ' ')
    {
        ++start;
    }

    const char* end = start;

    while ( (*end != '\0') &&
            (*end != '(') )
    {
        ++end;
    }

    while ( (end > start) &&
            (end[-1] == ' ') )
    {
        --end;
    }

    const size_t length = (size_t)(end - start);

    // no declared type at all
    if (length == 0u)
    {
        return D_DB_TYPE_NONE;
    }

    // a name the table knows, whole
    for (size_t i = 0u; i < (sizeof(names) / sizeof(names[0])); ++i)
    {
        if ( (strlen(names[i].name) == length) &&
             (sqlite3_strnicmp(start, names[i].name, (int)length) == 0) )
        {
            return names[i].type;
        }
    }

    // SQLite's affinity rules, in its order
    if (d_internal_sqlite_names_word(_declared, "INT"))
    {
        return D_DB_TYPE_BIG_INTEGER;
    }

    if ( (d_internal_sqlite_names_word(_declared, "CHAR")) ||
         (d_internal_sqlite_names_word(_declared, "CLOB")) ||
         (d_internal_sqlite_names_word(_declared, "TEXT")) )
    {
        return D_DB_TYPE_STRING;
    }

    if (d_internal_sqlite_names_word(_declared, "BLOB"))
    {
        return D_DB_TYPE_BINARY;
    }

    if ( (d_internal_sqlite_names_word(_declared, "REAL")) ||
         (d_internal_sqlite_names_word(_declared, "FLOA")) ||
         (d_internal_sqlite_names_word(_declared, "DOUB")) )
    {
        return D_DB_TYPE_FLOATING_POINT;
    }

    return D_DB_TYPE_DECIMAL;
}

#else

// d_internal_sqlite_no_plumbing
//   type: declared so that the translation unit is not empty, which ISO C
// forbids; built as stubs, the layer has no plumbing to define.
typedef int d_internal_sqlite_no_plumbing;

#endif  // D_INTERNAL_SQLITE
