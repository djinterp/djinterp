/*******************************************************************************
* djinterp [c]                                                   sqlite_schema.c
*
* Implementation of sqlite_schema.h.
*   Tables, indexes, views and triggers are all rows of a schema's catalogue
* (sqlite_master, or sqlite_temp_master for "temp"), told apart by `type`, so
* one query each answers "has", "list" and their variants; DROP differs only
* in its keyword. Values -- a type, a name to find -- are bound; names that
* become part of a statement are quoted with %w; SQL the caller wrote is
* inserted as written, which is the contract.
*   Columns come from PRAGMA table_xinfo, or table_info before 3.26.0.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_schema.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.21
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_schema.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // d_db_column, flags
#include "../db_internal.h"     // d_internal_db_buffer, the builders
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_SQLITE_TABLE_FLAGS
//   constant: every flag a CREATE TABLE takes.
#define D_INTERNAL_SQLITE_TABLE_FLAGS                                          \
    (D_DB_IF_NOT_EXISTS | D_SQLITE_STRICT | D_SQLITE_WITHOUT_ROWID)

// D_INTERNAL_SQLITE_COLUMN_FLAGS
//   constant: every flag a struct d_db_column may carry.
#define D_INTERNAL_SQLITE_COLUMN_FLAGS                                         \
    (D_DB_COLUMN_PRIMARY_KEY    |                                              \
     D_DB_COLUMN_AUTO_INCREMENT |                                              \
     D_DB_COLUMN_NOT_NULL       |                                              \
     D_DB_COLUMN_UNIQUE)

/**
 * @brief Asks a schema's catalogue whether it has an object of a type and
 *        name.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for "main".
 * @param[in]  _type    "table", "index", "view" or "trigger".
 * @param[in]  _name    the object's name, matched ignoring ASCII case.
 * @param[out] _exists  receives the answer; `false` on any failure.
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE.
 *
 * The comparison is COLLATE NOCASE because SQLite resolves names that way:
 * a table created as "Users" is the one "users" names.
 */
static int
d_internal_sqlite_has_object(
    sqlite3*    _db,
    const char* _schema,
    const char* _type,
    const char* _name,
    bool*       _exists
)
{
    // parameter validation
    if ( (!_exists)                            ||
         (!_db)                                ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_name)) )
    {
        if (_exists)
        {
            *_exists = false;
        }

        return SQLITE_MISUSE;
    }

    *_exists = false;

    sqlite3_stmt* statement = NULL;
    int           result    = d_internal_sqlite_prepare(
        _db,
        &statement,
        "SELECT 1 FROM \"%w\".%s WHERE type = ?1 AND name = ?2 COLLATE NOCASE",
        d_internal_sqlite_schema(_schema),
        d_internal_sqlite_schema_table(_schema));

    // a schema that is not attached fails here
    if (result != SQLITE_OK)
    {
        return result;
    }

    result = sqlite3_bind_text(statement, 1, _type, -1, SQLITE_STATIC);

    // the name, then the one step that answers
    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(statement, 2, _name, -1, SQLITE_STATIC);
    }

    if (result == SQLITE_OK)
    {
        result = sqlite3_step(statement);
    }

    *_exists = (result == SQLITE_ROW);

    return d_internal_sqlite_finish(statement, result);
}

/**
 * @brief Lists the names of a schema's objects of one type, optionally only
 *        those belonging to one table.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for "main".
 * @param[in]  _type    "table", "index", "view" or "trigger".
 * @param[in]  _table   the table the objects must belong to; `NULL` for any.
 * @param[out] _names   receives the names, in name order.
 * @return SQLITE_OK, the SQLite result code of the failure, SQLITE_NOMEM, or
 *         SQLITE_MISUSE.
 *
 * SQLite reserves names beginning "sqlite_", in any case, for its own
 * objects; lower(substr(...)) filters them where LIKE could not be trusted,
 * since PRAGMA case_sensitive_like changes what LIKE matches.
 */
static int
d_internal_sqlite_list_objects(
    sqlite3*               _db,
    const char*            _schema,
    const char*            _type,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         ( (_table) &&
           (!d_internal_sqlite_name_ok(_table)) ) ||
         (!_names) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    int           result    = d_internal_sqlite_prepare(
        _db,
        &statement,
        "SELECT name FROM \"%w\".%s WHERE type = ?1 "
        "AND lower(substr(name, 1, 7)) <> 'sqlite_'%s ORDER BY name",
        d_internal_sqlite_schema(_schema),
        d_internal_sqlite_schema_table(_schema),
        (_table) ? " AND tbl_name = ?2 COLLATE NOCASE" : "");

    // a schema that is not attached fails here
    if (result != SQLITE_OK)
    {
        return result;
    }

    result = sqlite3_bind_text(statement, 1, _type, -1, SQLITE_STATIC);

    // the table, when the list is one table's
    if ( (result == SQLITE_OK) &&
         (_table) )
    {
        result = sqlite3_bind_text(statement, 2, _table, -1, SQLITE_STATIC);
    }

    // a value that would not bind ends the statement
    if (result != SQLITE_OK)
    {
        return d_internal_sqlite_finish(statement, result);
    }

    return d_internal_sqlite_collect_names(statement, 0, _names);
}

/**
 * @brief Drops a schema object of the kind its keyword names.
 *
 * @param[in] _db       the connection.
 * @param[in] _schema   the schema; `NULL` for "main".
 * @param[in] _keyword  "TABLE", "INDEX", "VIEW" or "TRIGGER".
 * @param[in] _name     the object's name.
 * @param[in] _flags    D_DB_IF_EXISTS, or 0.
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE.
 */
static int
d_internal_sqlite_drop_object(
    sqlite3*    _db,
    const char* _schema,
    const char* _keyword,
    const char* _name,
    unsigned    _flags
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_name))     ||
         ((_flags & ~D_DB_IF_EXISTS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(_db,
                                     "DROP %s %s\"%w\".\"%w\"",
                                     _keyword,
                                     (_flags & D_DB_IF_EXISTS)
                                         ? "IF EXISTS "
                                         : "",
                                     d_internal_sqlite_schema(_schema),
                                     _name);
}

/**
 * @brief Appends formatted text to a statement under construction.
 *
 * @param[in,out] _sql     the statement so far.
 * @param[in]     _format  an sqlite3_mprintf format.
 * @param[in]     ...      the format's arguments.
 * @return SQLITE_OK, or SQLITE_NOMEM.
 */
static int
d_internal_sqlite_append(
    struct d_internal_db_buffer* _sql,
    const char*                  _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    char* const piece = sqlite3_vmprintf(_format, args);

    va_end(args);

    // formatting fails only for want of memory
    if (!piece)
    {
        return SQLITE_NOMEM;
    }

    const bool appended = d_internal_db_buffer_append(_sql,
                                                      piece,
                                                      strlen(piece));

    sqlite3_free(piece);

    return (appended) ? SQLITE_OK : SQLITE_NOMEM;
}

/**
 * @brief Ends a statement under construction with its terminator.
 *
 * @param[in,out] _sql  the statement.
 * @return SQLITE_OK, or SQLITE_NOMEM.
 */
static int
d_internal_sqlite_terminate(
    struct d_internal_db_buffer* _sql
)
{
    return (d_internal_db_buffer_append(_sql, "", 1u))
               ? SQLITE_OK
               : SQLITE_NOMEM;
}

/**
 * @brief The type a column is declared with, as sqlite_schema.h tabulates.
 *
 * @param[in] _column  the column.
 * @param[in] _strict  whether its table is STRICT.
 * @return the declared type, "" for none, or `NULL` for a type that is not a
 *         d_db_type.
 */
static const char*
d_internal_sqlite_type_sql(
    const struct d_db_column* _column,
    bool                      _strict
)
{
    // the caller's own spelling wins
    if (_column->type_sql)
    {
        return _column->type_sql;
    }

    switch (_column->type)
    {
        case D_DB_TYPE_NONE:
            return (_strict) ? "ANY" : "";

        case D_DB_TYPE_BOOLEAN:
        case D_DB_TYPE_INTEGER:
        case D_DB_TYPE_BIG_INTEGER:
            return "INTEGER";

        case D_DB_TYPE_FLOATING_POINT:
            return "REAL";

        case D_DB_TYPE_DECIMAL:
            return (_strict) ? "TEXT" : "NUMERIC";

        case D_DB_TYPE_STRING:
        case D_DB_TYPE_DATE:
        case D_DB_TYPE_TIME:
        case D_DB_TYPE_DATETIME:
        case D_DB_TYPE_TIMESTAMP:
        case D_DB_TYPE_JSON:
        case D_DB_TYPE_XML:
        case D_DB_TYPE_UUID:
            return "TEXT";

        case D_DB_TYPE_BINARY:
            return "BLOB";

        default:
            break;
    }

    return NULL;
}

/**
 * @brief Appends one column's definition to a statement under construction.
 *
 * @param[in,out] _sql     the statement so far.
 * @param[in]     _column  the column.
 * @param[in]     _strict  whether its table is STRICT.
 * @param[in]     _key     whether a PRIMARY KEY flag is written inline, as
 *                         it is for a sole key column.
 * @return SQLITE_OK, SQLITE_NOMEM, or SQLITE_MISUSE for an empty name, an
 *         unknown type or an unknown flag.
 *
 * A DEFAULT is parenthesized, which SQLite requires of any expression that is
 * not a literal and accepts around one that is.
 */
static int
d_internal_sqlite_append_column(
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _column,
    bool                         _strict,
    bool                         _key
)
{
    const char* const type = d_internal_sqlite_type_sql(_column, _strict);

    // a column needs a name, a known type and known flags
    if ( (!type)                                                       ||
         (!d_internal_sqlite_name_ok(_column->name))                   ||
         ((_column->flags & ~D_INTERNAL_SQLITE_COLUMN_FLAGS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    const unsigned flags   = _column->flags;
    const bool     key     = ( (_key) &&
                               ((flags & D_DB_COLUMN_PRIMARY_KEY) != 0u) );
    const char*    dflt    = _column->default_sql;

    return d_internal_sqlite_append(
        _sql,
        "\"%w\"%s%s%s%s%s%s%s%s%s",
        _column->name,
        (type[0] != '\0') ? " " : "",
        type,
        (key) ? " PRIMARY KEY" : "",
        ( (key) &&
          ((flags & D_DB_COLUMN_AUTO_INCREMENT) != 0u) ) ? " AUTOINCREMENT"
                                                        : "",
        ((flags & D_DB_COLUMN_NOT_NULL) != 0u) ? " NOT NULL" : "",
        ((flags & D_DB_COLUMN_UNIQUE) != 0u) ? " UNIQUE" : "",
        (dflt) ? " DEFAULT (" : "",
        (dflt) ? dflt : "",
        (dflt) ? ")" : "");
}

/**
 * @brief Counts a column set's primary-key columns, and checks that
 *        AUTO_INCREMENT appears only where SQLite allows it.
 *
 * @param[in]  _columns  the columns.
 * @param[in]  _count    how many.
 * @param[out] _keys     receives the number of primary-key columns.
 * @return SQLITE_OK, or SQLITE_MISUSE for AUTO_INCREMENT on a column that is
 *         not the sole primary key, or key places that are not sound
 *         (d_internal_db_key_places_ok).
 */
static int
d_internal_sqlite_count_keys(
    const struct d_db_column* _columns,
    size_t                    _count,
    size_t*                   _keys
)
{
    size_t keys           = 0u;
    bool   auto_increment = false;

    // tally the key columns, and whether any asks to auto-increment
    for (size_t i = 0u; i < _count; ++i)
    {
        const unsigned flags = _columns[i].flags;

        // AUTO_INCREMENT is meaningless off the key
        if ( ((flags & D_DB_COLUMN_AUTO_INCREMENT) != 0u) &&
             ((flags & D_DB_COLUMN_PRIMARY_KEY) == 0u) )
        {
            return SQLITE_MISUSE;
        }

        keys           += ((flags & D_DB_COLUMN_PRIMARY_KEY) != 0u) ? 1u : 0u;
        auto_increment  = ( (auto_increment) ||
                            ((flags & D_DB_COLUMN_AUTO_INCREMENT) != 0u) );
    }

    // and impossible on a composite key; the key's places sound
    if ( ( (auto_increment) &&
           (keys != 1u) ) ||
         (!d_internal_db_key_places_ok(_columns, _count)) )
    {
        return SQLITE_MISUSE;
    }

    *_keys = keys;

    return SQLITE_OK;
}

/**
 * @brief Writes a column set as the definition between CREATE TABLE's
 *        parentheses.
 *
 * @param[in,out] _sql      the definition; terminated on success.
 * @param[in]     _columns  the columns.
 * @param[in]     _count    how many.
 * @param[in]     _strict   whether the table is STRICT.
 * @return SQLITE_OK, SQLITE_NOMEM, or SQLITE_MISUSE.
 *
 * A sole key is written on its column, which is the only place AUTOINCREMENT
 * can go; a composite one as a table constraint after the columns.
 */
static int
d_internal_sqlite_build_definition(
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _columns,
    size_t                       _count,
    bool                         _strict
)
{
    size_t keys   = 0u;
    int    result = d_internal_sqlite_count_keys(_columns, _count, &keys);

    // each column, comma-separated
    for (size_t i = 0u; (result == SQLITE_OK) && (i < _count); ++i)
    {
        result = (i) ? d_internal_sqlite_append(_sql, ", ") : SQLITE_OK;

        if (result == SQLITE_OK)
        {
            result = d_internal_sqlite_append_column(_sql,
                                                     &_columns[i],
                                                     _strict,
                                                     (keys == 1u));
        }
    }

    // a composite key, as a table constraint naming its columns in the
    // key's order
    if ( (result == SQLITE_OK) &&
         (keys > 1u) )
    {
        const char* separator = ", PRIMARY KEY (";

        for (size_t place = 1u; (result == SQLITE_OK) && (place <= keys);
             ++place)
        {
            const size_t i = d_internal_db_key_column(_columns, _count, place);

            result    = d_internal_sqlite_append(_sql,
                                                 "%s\"%w\"",
                                                 separator,
                                                 _columns[i].name);
            separator = ", ";
        }

        if (result == SQLITE_OK)
        {
            result = d_internal_sqlite_append(_sql, ")");
        }
    }

    return (result == SQLITE_OK) ? d_internal_sqlite_terminate(_sql) : result;
}

/**
 * @brief Runs CREATE TABLE once the definition is settled.
 *
 * @param[in] _db          the connection.
 * @param[in] _schema      the schema; `NULL` for "main".
 * @param[in] _table       the table's name.
 * @param[in] _definition  the text between the parentheses.
 * @param[in] _flags       D_INTERNAL_SQLITE_TABLE_FLAGS, already checked.
 * @return SQLITE_OK, D_DB_UNSUPPORTED, or the SQLite result code of the
 *         failure.
 *
 * Table options follow the parenthesis, comma-separated when there are two.
 */
static int
d_internal_sqlite_create_table(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _definition,
    unsigned    _flags
)
{
    const bool strict        = ((_flags & D_SQLITE_STRICT) != 0u);
    const bool without_rowid = ((_flags & D_SQLITE_WITHOUT_ROWID) != 0u);

    // refuse an option the library in use cannot parse
    if ( ( (strict) &&
           (!d_sqlite_supports(D_SQLITE_FEATURE_STRICT_TABLES)) ) ||
         ( (without_rowid) &&
           (!d_sqlite_supports(D_SQLITE_FEATURE_WITHOUT_ROWID)) ) )
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_sqlite_execute(
        _db,
        "CREATE TABLE %s\"%w\".\"%w\" (%s)%s%s%s",
        ((_flags & D_DB_IF_NOT_EXISTS) != 0u) ? "IF NOT EXISTS " : "",
        d_internal_sqlite_schema(_schema),
        _table,
        _definition,
        (without_rowid) ? " WITHOUT ROWID" : "",
        ( (without_rowid) &&
          (strict) ) ? "," : "",
        (strict) ? " STRICT" : "");
}

/**
 * @brief Reports whether a table is STRICT, which decides how a column added
 *        to it is declared.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema; `NULL` for "main".
 * @param[in]  _table   the table's name.
 * @param[out] _strict  receives the answer; `false` for a missing table, and
 *                      before 3.37.0, which has no STRICT tables.
 * @return SQLITE_OK, or the SQLite result code of the failure.
 */
static int
d_internal_sqlite_table_is_strict(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    bool*       _strict
)
{
    *_strict = false;

    // no STRICT tables before 3.37.0, and no table_list to ask either
    if (!d_sqlite_supports(D_SQLITE_FEATURE_STRICT_TABLES))
    {
        return SQLITE_OK;
    }

    sqlite3_stmt* statement = NULL;
    int           result    = d_internal_sqlite_prepare(
        _db,
        &statement,
        "SELECT strict FROM pragma_table_list "
        "WHERE schema = ?1 COLLATE NOCASE AND name = ?2 COLLATE NOCASE");

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    result = sqlite3_bind_text(statement,
                               1,
                               d_internal_sqlite_schema(_schema),
                               -1,
                               SQLITE_STATIC);

    // the table, after the schema
    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_text(statement, 2, _table, -1, SQLITE_STATIC);
    }

    // a value that would not bind ends the statement
    if (result != SQLITE_OK)
    {
        return d_internal_sqlite_finish(statement, result);
    }

    int64_t strict = 0;

    result   = d_internal_sqlite_step_int64(statement, &strict);
    *_strict = ( (result == SQLITE_OK) &&
                 (strict != 0) );

    // a missing table is not STRICT, and not this function's error to report
    return (result == D_DB_NOT_FOUND) ? SQLITE_OK : result;
}

/**
 * @brief Prepares the PRAGMA that describes a table's columns.
 *
 * @param[in]  _db         the connection.
 * @param[in]  _schema     the schema; `NULL` for "main".
 * @param[in]  _table      the table's name.
 * @param[out] _statement  receives the statement.
 * @param[out] _hidden     receives whether rows carry a `hidden` column (6).
 * @return SQLITE_OK, or the SQLite result code of the failure.
 *
 * table_xinfo (3.26.0) lists generated columns, which table_info leaves out;
 * an older library has no generated columns to miss. Before 3.26.0 an
 * unknown PRAGMA is silently empty, so the version is checked, not guessed.
 */
static int
d_internal_sqlite_prepare_columns(
    sqlite3*       _db,
    const char*    _schema,
    const char*    _table,
    sqlite3_stmt** _statement,
    bool*          _hidden
)
{
    *_hidden = d_internal_sqlite_runtime_at_least(
        D_ENV_SQLITE_ENCODE_VERSION(3, 26, 0));

    return d_internal_sqlite_prepare(_db,
                                     _statement,
                                     (*_hidden)
                                         ? "PRAGMA \"%w\".table_xinfo(%Q)"
                                         : "PRAGMA \"%w\".table_info(%Q)",
                                     d_internal_sqlite_schema(_schema),
                                     _table);
}

/**
 * @brief Steps a column-describing PRAGMA to its end, adding each visible
 *        column to a builder.
 *
 * @param[in]     _statement  the PRAGMA, from
 *                            d_internal_sqlite_prepare_columns; not
 *                            finalized here.
 * @param[in]     _hidden     whether rows carry a `hidden` column (6).
 * @param[in,out] _builder    receives the columns.
 * @return the last step's result -- SQLITE_DONE when every row was read --
 *         or SQLITE_NOMEM when a name or a copy could not be had.
 *
 * Rows are name (1), type (2), NOT NULL (3), the default's SQL (4) and the
 * place in the primary key (5). Hidden columns (1) are a virtual table's
 * machinery and are skipped; 2 and 3 are generated columns, virtual and
 * stored, which table_info leaves out and table_xinfo lists. The strings are
 * copied before the next step, which would free them.
 */
static int
d_internal_sqlite_collect_columns(
    sqlite3_stmt*                 _statement,
    bool                          _hidden,
    struct d_internal_db_columns* _builder
)
{
    int result = sqlite3_step(_statement);

    // collect each visible column
    for (; result == SQLITE_ROW; result = sqlite3_step(_statement))
    {
        const int               hidden =
            (_hidden) ? sqlite3_column_int(_statement, 6) : 0;
        struct d_db_column_info column =
        {
            (const char*)sqlite3_column_text(_statement, 1),
            (const char*)sqlite3_column_text(_statement, 2),
            (const char*)sqlite3_column_text(_statement, 4),
            sqlite3_column_int(_statement, 5),
            (sqlite3_column_int(_statement, 3) != 0),
            ( (hidden == 2) ||
              (hidden == 3) )
        };

        // a virtual table's machinery, not one of its columns
        if (hidden == 1)
        {
            continue;
        }

        // a missing name is SQLite out of memory; so is a failed copy
        if ( (!column.name) ||
             (!d_internal_db_columns_add(_builder, &column)) )
        {
            return SQLITE_NOMEM;
        }
    }

    return result;
}

/**
 * @brief Deletes a table's rows and its sqlite_sequence entry, together.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema, resolved: never `NULL`.
 * @param[in] _table   the table's name.
 * @return SQLITE_OK, or the SQLite result code of the failure.
 *
 * Both statements run inside a savepoint, which nests in a transaction the
 * caller may have open. A failure rolls the savepoint back and releases it,
 * so nothing is left half done or held open; the code returned is the
 * failure's, though sqlite3_errmsg then describes the rollback.
 */
static int
d_internal_sqlite_truncate_sequenced(
    sqlite3*    _db,
    const char* _schema,
    const char* _table
)
{
    int result = d_internal_sqlite_execute(_db, "SAVEPOINT d_sqlite_truncate");

    // a savepoint that would not open leaves nothing to undo
    if (result != SQLITE_OK)
    {
        return result;
    }

    result = d_internal_sqlite_execute(
        _db,
        "DELETE FROM \"%w\".\"%w\"; "
        "DELETE FROM \"%w\".sqlite_sequence WHERE name = %Q COLLATE NOCASE",
        _schema,
        _table,
        _schema,
        _table);

    // undo whatever ran, keeping the failure's code
    if (result != SQLITE_OK)
    {
        (void)sqlite3_exec(_db,
                           "ROLLBACK TO d_sqlite_truncate; "
                           "RELEASE d_sqlite_truncate",
                           NULL,
                           NULL,
                           NULL);

        return result;
    }

    return d_internal_sqlite_execute(_db, "RELEASE d_sqlite_truncate");
}

//==============================================================================
// 2.  TABLES
//==============================================================================

// 2.1    Existence
//------------------------------------------------------------------------------
/*
d_sqlite_has_table
  The _s form, with every failure read as "no".
*/
bool
d_sqlite_has_table(
    sqlite3*    _db,
    const char* _schema,
    const char* _table
)
{
    bool exists = false;

    return ( (d_sqlite_has_table_s(_db, _schema, _table, &exists) ==
              SQLITE_OK) &&
             (exists) );
}

/*
d_sqlite_has_table_s
  A catalogue row of type 'table', which virtual tables are too.
*/
int
d_sqlite_has_table_s(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    bool*       _exists
)
{
    return d_internal_sqlite_has_object(_db,
                                        _schema,
                                        "table",
                                        _table,
                                        _exists);
}

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_sqlite_create_table
  The column set is checked and written into one buffer, then run through the
same path as a definition written by hand. Six parameters, one past the
guide's tripwire: the columns and their count are one argument, split in two
as C passes an array.
*/
int
d_sqlite_create_table(
    sqlite3*                  _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _columns,
    size_t                    _count,
    unsigned                  _flags
)
{
    // parameter validation
    if ( (!_db)                                                 ||
         (!d_internal_sqlite_schema_ok(_schema))                ||
         (!d_internal_sqlite_name_ok(_table))                   ||
         (!_columns)                                            ||
         (_count == 0u)                                         ||
         ((_flags & ~D_INTERNAL_SQLITE_TABLE_FLAGS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    struct d_internal_db_buffer definition = { NULL, 0u, 0u };
    int                         result     =
        d_internal_sqlite_build_definition(
            &definition,
            _columns,
            _count,
            ((_flags & D_SQLITE_STRICT) != 0u));

    // the definition is settled; run it
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_create_table(_db,
                                                _schema,
                                                _table,
                                                definition.data,
                                                _flags);
    }

    d_internal_db_buffer_release(&definition);

    return result;
}

/*
d_sqlite_create_table_sql
  The definition is the caller's SQL, placed between the parentheses as is.
*/
int
d_sqlite_create_table_sql(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _definition,
    unsigned    _flags
)
{
    // parameter validation
    if ( (!_db)                                                 ||
         (!d_internal_sqlite_schema_ok(_schema))                ||
         (!d_internal_sqlite_name_ok(_table))                   ||
         (!d_internal_sqlite_name_ok(_definition))              ||
         ((_flags & ~D_INTERNAL_SQLITE_TABLE_FLAGS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_create_table(_db,
                                          _schema,
                                          _table,
                                          _definition,
                                          _flags);
}

/*
d_sqlite_drop_table
  DROP TABLE takes the table's indexes and triggers with it.
*/
int
d_sqlite_drop_table(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    unsigned    _flags
)
{
    return d_internal_sqlite_drop_object(_db, _schema, "TABLE", _table, _flags);
}

// 2.3    Changing
//------------------------------------------------------------------------------
/*
d_sqlite_rename_table
  RENAME TO takes an unqualified name: a table cannot move between schemas.
*/
int
d_sqlite_rename_table(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _new_name
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table))    ||
         (!d_internal_sqlite_name_ok(_new_name)) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(
        _db,
        "ALTER TABLE \"%w\".\"%w\" RENAME TO \"%w\"",
        d_internal_sqlite_schema(_schema),
        _table,
        _new_name);
}

/*
d_sqlite_truncate_table
  A DELETE with no WHERE, which SQLite performs as a truncation, and the
table's row in sqlite_sequence, which exists only once some table in the
schema uses AUTOINCREMENT; without it, the DELETE alone.
*/
int
d_sqlite_truncate_table(
    sqlite3*    _db,
    const char* _schema,
    const char* _table
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table)) )
    {
        return SQLITE_MISUSE;
    }

    const char* const schema   = d_internal_sqlite_schema(_schema);
    bool              sequence = false;
    const int         result   = d_internal_sqlite_has_object(
        _db,
        _schema,
        "table",
        "sqlite_sequence",
        &sequence);

    // a schema that could not be asked
    if (result != SQLITE_OK)
    {
        return result;
    }

    // without AUTOINCREMENT there is only the one statement
    if (!sequence)
    {
        return d_internal_sqlite_execute(_db,
                                         "DELETE FROM \"%w\".\"%w\"",
                                         schema,
                                         _table);
    }

    return d_internal_sqlite_truncate_sequenced(_db, schema, _table);
}

// 2.4    Rows
//------------------------------------------------------------------------------
/*
d_sqlite_count_rows
  count(*) over the whole table, which SQLite answers from the table's b-tree
without reading the rows.
*/
int
d_sqlite_count_rows(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    int64_t*    _count
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table))    ||
         (!_count) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    const int     result    = d_internal_sqlite_prepare(
        _db,
        &statement,
        "SELECT count(*) FROM \"%w\".\"%w\"",
        d_internal_sqlite_schema(_schema),
        _table);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_int64(statement, _count);
}

// 2.5    Listing
//------------------------------------------------------------------------------
/*
d_sqlite_list_tables
  The catalogue's 'table' rows, SQLite's own left out.
*/
int
d_sqlite_list_tables(
    sqlite3*               _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    return d_internal_sqlite_list_objects(_db, _schema, "table", NULL, _names);
}

//==============================================================================
// 3.  COLUMNS
//==============================================================================

// 3.1    Existence
//------------------------------------------------------------------------------
/*
d_sqlite_has_column
  The _s form, with every failure read as "no".
*/
bool
d_sqlite_has_column(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _column
)
{
    bool exists = false;

    return ( (d_sqlite_has_column_s(_db,
                                    _schema,
                                    _table,
                                    _column,
                                    &exists) == SQLITE_OK) &&
             (exists) );
}

/*
d_sqlite_has_column_s
  The PRAGMA's rows are read until the name turns up; a missing table simply
has none. Hidden columns (1) belong to virtual tables' machinery and are
skipped, as list_columns skips them, so the two always agree.
*/
int
d_sqlite_has_column_s(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _column,
    bool*       _exists
)
{
    // parameter validation
    if ( (!_exists)                              ||
         (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table))    ||
         (!d_internal_sqlite_name_ok(_column)) )
    {
        if (_exists)
        {
            *_exists = false;
        }

        return SQLITE_MISUSE;
    }

    *_exists = false;

    sqlite3_stmt* statement = NULL;
    bool          hidden    = false;
    int           result    = d_internal_sqlite_prepare_columns(_db,
                                                                _schema,
                                                                _table,
                                                                &statement,
                                                                &hidden);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    // read until the name is found or the rows run out
    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        const char* const name =
            (const char*)sqlite3_column_text(statement, 1);

        // stop at a visible column of that name
        if ( ( (!hidden) ||
               (sqlite3_column_int(statement, 6) != 1) ) &&
             (name)                                       &&
             (sqlite3_stricmp(name, _column) == 0) )
        {
            *_exists = true;

            break;
        }
    }

    return d_internal_sqlite_finish(statement, result);
}

// 3.2    Adding, renaming and dropping
//------------------------------------------------------------------------------
/*
d_sqlite_add_column
  The table's STRICT-ness is asked first, since it decides the declared type.
*/
int
d_sqlite_add_column(
    sqlite3*                  _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _column
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table))    ||
         (!_column) )
    {
        return SQLITE_MISUSE;
    }

    struct d_internal_db_buffer definition = { NULL, 0u, 0u };
    bool                        strict     = false;
    int                         result     =
        d_internal_sqlite_table_is_strict(_db, _schema, _table, &strict);

    // the column's definition, its key flag written as given
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_append_column(&definition,
                                                 _column,
                                                 strict,
                                                 true);
    }

    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_terminate(&definition);
    }

    if (result == SQLITE_OK)
    {
        result = d_sqlite_add_column_sql(_db,
                                         _schema,
                                         _table,
                                         definition.data);
    }

    d_internal_db_buffer_release(&definition);

    return result;
}

/*
d_sqlite_add_column_sql
  ADD COLUMN, with the caller's definition as written.
*/
int
d_sqlite_add_column_sql(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _definition
)
{
    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_sqlite_schema_ok(_schema))   ||
         (!d_internal_sqlite_name_ok(_table))      ||
         (!d_internal_sqlite_name_ok(_definition)) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(_db,
                                     "ALTER TABLE \"%w\".\"%w\" ADD COLUMN %s",
                                     d_internal_sqlite_schema(_schema),
                                     _table,
                                     _definition);
}

/*
d_sqlite_rename_column
  Checked against the library in use: before 3.25.0 the statement is a syntax
error whose message names neither the feature nor the version.
*/
int
d_sqlite_rename_column(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _column,
    const char* _new_name
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table))    ||
         (!d_internal_sqlite_name_ok(_column))   ||
         (!d_internal_sqlite_name_ok(_new_name)) )
    {
        return SQLITE_MISUSE;
    }

    // RENAME COLUMN arrived in 3.25.0
    if (!d_sqlite_supports(D_SQLITE_FEATURE_RENAME_COLUMN))
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_sqlite_execute(
        _db,
        "ALTER TABLE \"%w\".\"%w\" RENAME COLUMN \"%w\" TO \"%w\"",
        d_internal_sqlite_schema(_schema),
        _table,
        _column,
        _new_name);
}

/*
d_sqlite_drop_column
  Checked against the library in use, as d_sqlite_rename_column; DROP COLUMN
arrived in 3.35.0.
*/
int
d_sqlite_drop_column(
    sqlite3*    _db,
    const char* _schema,
    const char* _table,
    const char* _column
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table))    ||
         (!d_internal_sqlite_name_ok(_column)) )
    {
        return SQLITE_MISUSE;
    }

    // DROP COLUMN arrived in 3.35.0
    if (!d_sqlite_supports(D_SQLITE_FEATURE_DROP_COLUMN))
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_sqlite_execute(
        _db,
        "ALTER TABLE \"%w\".\"%w\" DROP COLUMN \"%w\"",
        d_internal_sqlite_schema(_schema),
        _table,
        _column);
}

// 3.3    Listing
//------------------------------------------------------------------------------
/*
d_sqlite_list_columns
  One builder entry per visible row: name (1), type (2), NOT NULL (3), the
default's SQL text (4), and the place in the primary key (5). Every table has
a column, so no rows means no table.
*/
int
d_sqlite_list_columns(
    sqlite3*                 _db,
    const char*              _schema,
    const char*              _table,
    struct d_db_column_list* _columns
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_table))    ||
         (!_columns) )
    {
        return SQLITE_MISUSE;
    }

    struct d_internal_db_columns builder   =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u
    };
    sqlite3_stmt*                statement = NULL;
    bool                         hidden    = false;
    int                          result    =
        d_internal_sqlite_prepare_columns(_db,
                                          _schema,
                                          _table,
                                          &statement,
                                          &hidden);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    result = d_internal_sqlite_finish(
        statement,
        d_internal_sqlite_collect_columns(statement, hidden, &builder));

    // a failure drops what was collected, and so does an empty answer
    if ( (result != SQLITE_OK) ||
         (builder.count == 0u) )
    {
        d_internal_db_columns_release(&builder);

        return (result != SQLITE_OK) ? result : D_DB_NOT_FOUND;
    }

    return (d_internal_db_columns_finish(&builder, _columns))
               ? SQLITE_OK
               : SQLITE_NOMEM;
}

//==============================================================================
// 4.  INDEXES
//==============================================================================

// 4.1    Existence
//------------------------------------------------------------------------------
/*
d_sqlite_has_index
  The _s form, with every failure read as "no".
*/
bool
d_sqlite_has_index(
    sqlite3*    _db,
    const char* _schema,
    const char* _index
)
{
    bool exists = false;

    return ( (d_sqlite_has_index_s(_db, _schema, _index, &exists) ==
              SQLITE_OK) &&
             (exists) );
}

/*
d_sqlite_has_index_s
  A catalogue row of type 'index'; SQLite's automatic indexes have rows too.
*/
int
d_sqlite_has_index_s(
    sqlite3*    _db,
    const char* _schema,
    const char* _index,
    bool*       _exists
)
{
    return d_internal_sqlite_has_object(_db,
                                        _schema,
                                        "index",
                                        _index,
                                        _exists);
}

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_sqlite_create_index
  The index is qualified and the table is not: SQLite builds an index in its
table's schema, and CREATE INDEX says which with the index's name. Six
parameters, one past the guide's tripwire: each is a separate part of the
statement, and a struct would only rename them.
*/
int
d_sqlite_create_index(
    sqlite3*    _db,
    const char* _schema,
    const char* _index,
    const char* _table,
    const char* _columns,
    unsigned    _flags
)
{
    // parameter validation
    if ( (!_db)                                                ||
         (!d_internal_sqlite_schema_ok(_schema))               ||
         (!d_internal_sqlite_name_ok(_index))                  ||
         (!d_internal_sqlite_name_ok(_table))                  ||
         (!d_internal_sqlite_name_ok(_columns))                ||
         ((_flags & ~(D_DB_IF_NOT_EXISTS | D_DB_UNIQUE)) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(
        _db,
        "CREATE %sINDEX %s\"%w\".\"%w\" ON \"%w\" (%s)",
        ((_flags & D_DB_UNIQUE) != 0u) ? "UNIQUE " : "",
        ((_flags & D_DB_IF_NOT_EXISTS) != 0u) ? "IF NOT EXISTS " : "",
        d_internal_sqlite_schema(_schema),
        _index,
        _table,
        _columns);
}

/*
d_sqlite_drop_index
  SQLite refuses to drop the index behind a PRIMARY KEY or UNIQUE constraint.
*/
int
d_sqlite_drop_index(
    sqlite3*    _db,
    const char* _schema,
    const char* _index,
    unsigned    _flags
)
{
    return d_internal_sqlite_drop_object(_db, _schema, "INDEX", _index, _flags);
}

// 4.3    Listing
//------------------------------------------------------------------------------
/*
d_sqlite_list_indexes
  The catalogue's 'index' rows, by tbl_name when a table is given; the
automatic indexes' "sqlite_autoindex_" names keep them out.
*/
int
d_sqlite_list_indexes(
    sqlite3*               _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    return d_internal_sqlite_list_objects(_db,
                                          _schema,
                                          "index",
                                          _table,
                                          _names);
}

//==============================================================================
// 5.  VIEWS
//==============================================================================

// 5.1    Existence
//------------------------------------------------------------------------------
/*
d_sqlite_has_view
  The _s form, with every failure read as "no".
*/
bool
d_sqlite_has_view(
    sqlite3*    _db,
    const char* _schema,
    const char* _view
)
{
    bool exists = false;

    return ( (d_sqlite_has_view_s(_db, _schema, _view, &exists) ==
              SQLITE_OK) &&
             (exists) );
}

/*
d_sqlite_has_view_s
  A catalogue row of type 'view'.
*/
int
d_sqlite_has_view_s(
    sqlite3*    _db,
    const char* _schema,
    const char* _view,
    bool*       _exists
)
{
    return d_internal_sqlite_has_object(_db, _schema, "view", _view, _exists);
}

// 5.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_sqlite_create_view
  CREATE VIEW ... AS, with the caller's SELECT as written.
*/
int
d_sqlite_create_view(
    sqlite3*    _db,
    const char* _schema,
    const char* _view,
    const char* _select,
    unsigned    _flags
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_view))     ||
         (!d_internal_sqlite_name_ok(_select))   ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(
        _db,
        "CREATE VIEW %s\"%w\".\"%w\" AS %s",
        ((_flags & D_DB_IF_NOT_EXISTS) != 0u) ? "IF NOT EXISTS " : "",
        d_internal_sqlite_schema(_schema),
        _view,
        _select);
}

/*
d_sqlite_drop_view
  DROP VIEW; the tables it reads are untouched.
*/
int
d_sqlite_drop_view(
    sqlite3*    _db,
    const char* _schema,
    const char* _view,
    unsigned    _flags
)
{
    return d_internal_sqlite_drop_object(_db, _schema, "VIEW", _view, _flags);
}

// 5.3    Listing
//------------------------------------------------------------------------------
/*
d_sqlite_list_views
  The catalogue's 'view' rows.
*/
int
d_sqlite_list_views(
    sqlite3*               _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    return d_internal_sqlite_list_objects(_db, _schema, "view", NULL, _names);
}

//==============================================================================
// 6.  TRIGGERS
//==============================================================================

// 6.1    Existence
//------------------------------------------------------------------------------
/*
d_sqlite_has_trigger
  The _s form, with every failure read as "no".
*/
bool
d_sqlite_has_trigger(
    sqlite3*    _db,
    const char* _schema,
    const char* _trigger
)
{
    bool exists = false;

    return ( (d_sqlite_has_trigger_s(_db, _schema, _trigger, &exists) ==
              SQLITE_OK) &&
             (exists) );
}

/*
d_sqlite_has_trigger_s
  A catalogue row of type 'trigger'.
*/
int
d_sqlite_has_trigger_s(
    sqlite3*    _db,
    const char* _schema,
    const char* _trigger,
    bool*       _exists
)
{
    return d_internal_sqlite_has_object(_db,
                                        _schema,
                                        "trigger",
                                        _trigger,
                                        _exists);
}

// 6.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_sqlite_create_trigger
  CREATE TRIGGER and the name; everything after is the caller's SQL.
*/
int
d_sqlite_create_trigger(
    sqlite3*    _db,
    const char* _schema,
    const char* _trigger,
    const char* _definition,
    unsigned    _flags
)
{
    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_sqlite_schema_ok(_schema))   ||
         (!d_internal_sqlite_name_ok(_trigger))    ||
         (!d_internal_sqlite_name_ok(_definition)) ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(
        _db,
        "CREATE TRIGGER %s\"%w\".\"%w\" %s",
        ((_flags & D_DB_IF_NOT_EXISTS) != 0u) ? "IF NOT EXISTS " : "",
        d_internal_sqlite_schema(_schema),
        _trigger,
        _definition);
}

/*
d_sqlite_drop_trigger
  DROP TRIGGER; its table is untouched.
*/
int
d_sqlite_drop_trigger(
    sqlite3*    _db,
    const char* _schema,
    const char* _trigger,
    unsigned    _flags
)
{
    return d_internal_sqlite_drop_object(_db,
                                         _schema,
                                         "TRIGGER",
                                         _trigger,
                                         _flags);
}

// 6.3    Listing
//------------------------------------------------------------------------------
/*
d_sqlite_list_triggers
  The catalogue's 'trigger' rows, by tbl_name when a table is given.
*/
int
d_sqlite_list_triggers(
    sqlite3*               _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    return d_internal_sqlite_list_objects(_db,
                                          _schema,
                                          "trigger",
                                          _table,
                                          _names);
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_has_table
  Without SQLite, false.
*/
bool
d_sqlite_has_table(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table
)
{
    (void)_db;
    (void)_schema;
    (void)_table;

    return false;
}

/*
d_sqlite_has_table_s
  Without SQLite there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_sqlite_has_table_s(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    bool*           _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_table;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_create_table
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_create_table(
    struct sqlite3*           _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _columns,
    size_t                    _count,
    unsigned                  _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_columns;
    (void)_count;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_create_table_sql
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_create_table_sql(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _definition,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_definition;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_drop_table
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_drop_table(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_rename_table
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_rename_table(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _new_name
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_new_name;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_truncate_table
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_truncate_table(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table
)
{
    (void)_db;
    (void)_schema;
    (void)_table;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_count_rows
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_count_rows(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    int64_t*        _count
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_count;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_list_tables
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_list_tables(
    struct sqlite3*        _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_schema;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_has_column
  Without SQLite, false.
*/
bool
d_sqlite_has_column(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_column;

    return false;
}

/*
d_sqlite_has_column_s
  Without SQLite there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_sqlite_has_column_s(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column,
    bool*           _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_column;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_add_column
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_add_column(
    struct sqlite3*           _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _column
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_column;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_add_column_sql
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_add_column_sql(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _definition
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_definition;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_rename_column
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_rename_column(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column,
    const char*     _new_name
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_column;
    (void)_new_name;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_drop_column
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_drop_column(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_column;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_list_columns
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_list_columns(
    struct sqlite3*          _db,
    const char*              _schema,
    const char*              _table,
    struct d_db_column_list* _columns
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_columns;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_has_index
  Without SQLite, false.
*/
bool
d_sqlite_has_index(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _index
)
{
    (void)_db;
    (void)_schema;
    (void)_index;

    return false;
}

/*
d_sqlite_has_index_s
  Without SQLite there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_sqlite_has_index_s(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _index,
    bool*           _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_index;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_create_index
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_create_index(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _index,
    const char*     _table,
    const char*     _columns,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_index;
    (void)_table;
    (void)_columns;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_drop_index
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_drop_index(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _index,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_index;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_list_indexes
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_list_indexes(
    struct sqlite3*        _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_has_view
  Without SQLite, false.
*/
bool
d_sqlite_has_view(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _view
)
{
    (void)_db;
    (void)_schema;
    (void)_view;

    return false;
}

/*
d_sqlite_has_view_s
  Without SQLite there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_sqlite_has_view_s(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _view,
    bool*           _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_view;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_create_view
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_create_view(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _view,
    const char*     _select,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_view;
    (void)_select;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_drop_view
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_drop_view(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _view,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_view;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_list_views
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_list_views(
    struct sqlite3*        _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_schema;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_has_trigger
  Without SQLite, false.
*/
bool
d_sqlite_has_trigger(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _trigger
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;

    return false;
}

/*
d_sqlite_has_trigger_s
  Without SQLite there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_sqlite_has_trigger_s(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _trigger,
    bool*           _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_create_trigger
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_create_trigger(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _trigger,
    const char*     _definition,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;
    (void)_definition;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_drop_trigger
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_drop_trigger(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _trigger,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_list_triggers
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_list_triggers(
    struct sqlite3*        _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_SQLITE
