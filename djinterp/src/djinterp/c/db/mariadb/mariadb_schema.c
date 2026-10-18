/*******************************************************************************
* djinterp [c]                                                  mariadb_schema.c
*
* Implementation of mariadb_schema.h.
*   Existence and listing are information_schema lookups (mariadb_internal.c);
* creating, changing and dropping are the servers' own statements, names
* quoted and the caller's SQL used as written. Where MySQL lacks an IF [NOT]
* EXISTS that MariaDB has -- for indexes, views and, before 8.0.29, triggers
* -- the layer asks first; MariaDB keeps its own, which is atomic. Asking
* treats a name as taken the way MariaDB's own clause does: a view's name by
* any table or view.
*   Columns are described from information_schema.COLUMNS, their key places
* from STATISTICS. MariaDB reports a default as SQL; MySQL reports a string's
* value bare, which the layer quotes so that both read alike.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_schema.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_schema.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strcmp, strncmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // d_db_column, flags
#include "../db_internal.h"  // buffers, builders, formatting, numbers
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_MARIADB_COLUMN_FLAGS
//   constant: every flag a struct d_db_column may carry.
#define D_INTERNAL_MARIADB_COLUMN_FLAGS                                        \
    (D_DB_COLUMN_PRIMARY_KEY    |                                              \
     D_DB_COLUMN_AUTO_INCREMENT |                                              \
     D_DB_COLUMN_NOT_NULL       |                                              \
     D_DB_COLUMN_UNIQUE)

// d_internal_mariadb_spelling
//   struct: how one d_db_type is declared.
struct d_internal_mariadb_spelling
{
    enum d_db_type type;   // the type
    const char*    sized;  // with a size, as a format taking it; NULL where
                           // the size is not read
    const char*    plain;  // without one
    const char*    key;    // without one, on a key column; NULL: as plain
};

// d_internal_mariadb_spellings
//   table: mariadb_schema.h's mapping, but for DECIMAL's two numbers. A key
// column cannot be LONGTEXT or LONGBLOB without a prefix length, so it takes
// a bounded type instead.
static const struct d_internal_mariadb_spelling
d_internal_mariadb_spellings[] =
{
    { D_DB_TYPE_BOOLEAN,        NULL, " BOOLEAN", NULL },
    { D_DB_TYPE_INTEGER,        NULL, " INT",     NULL },
    { D_DB_TYPE_BIG_INTEGER,    NULL, " BIGINT",  NULL },
    { D_DB_TYPE_FLOATING_POINT, NULL, " DOUBLE",  NULL },
    { D_DB_TYPE_STRING,    " VARCHAR(%u)",   " LONGTEXT",  " VARCHAR(255)"   },
    { D_DB_TYPE_BINARY,    " VARBINARY(%u)", " LONGBLOB",  " VARBINARY(255)" },
    { D_DB_TYPE_DATE,      NULL,             " DATE",      NULL },
    { D_DB_TYPE_TIME,      " TIME(%u)",      " TIME",      NULL },
    { D_DB_TYPE_DATETIME,  " DATETIME(%u)",  " DATETIME",  NULL },
    { D_DB_TYPE_TIMESTAMP, " TIMESTAMP(%u)", " TIMESTAMP", NULL },
    { D_DB_TYPE_JSON,      NULL,             " JSON",      NULL },
    { D_DB_TYPE_XML,       NULL,             " LONGTEXT",  NULL },
    { D_DB_TYPE_UUID,      NULL, " CHAR(36) CHARACTER SET ascii", NULL }
};

// d_internal_mariadb_sql_types
//   table: the DATA_TYPE of each type whose default MySQL's information_schema
// writes as SQL already: numbers, and bit values (b'101').
static const char* const d_internal_mariadb_sql_types[] =
{
    "tinyint", "smallint", "mediumint", "int",    "bigint",
    "decimal", "float",    "double",    "bit",    "year"
};

// d_internal_mariadb_binary_types
//   table: the DATA_TYPE of each binary string type, whose default MySQL 8.0
// writes in hexadecimal (0x7879), which is SQL.
static const char* const d_internal_mariadb_binary_types[] =
{
    "binary", "varbinary", "tinyblob", "blob", "mediumblob", "longblob"
};

/*
d_internal_mariadb_listed
  Whether a string is one of a list's.
*/
static bool
d_internal_mariadb_listed(
    const char*        _text,
    const char* const* _list,
    size_t             _count
)
{
    for (size_t i = 0u; i < _count; ++i)
    {
        if (strcmp(_text, _list[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Appends a column's type, as mariadb_schema.h tabulates it.
 *
 * @param[in]     _db      the connection, for its quoting.
 * @param[in,out] _sql     the definition so far.
 * @param[in]     _column  the column.
 * @param[in]     _keyed   whether the column is a key -- primary or unique --
 *                         which an unbounded type cannot be.
 * @return as d_internal_db_format; BAD for NONE without type_sql, or a type
 *         outside enum d_db_type, which leaves the buffer to the caller.
 */
static enum d_internal_db_format_status
d_internal_mariadb_append_type(
    struct d_mariadb*            _db,
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _column,
    bool                         _keyed
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);
    const size_t count = sizeof(d_internal_mariadb_spellings) /
                         sizeof(d_internal_mariadb_spellings[0]);

    // the caller's own spelling wins
    if (_column->type_sql)
    {
        return d_internal_db_format(_sql, &quoting, " %s", _column->type_sql);
    }

    // DECIMAL takes two numbers, the first defaulting to the servers' 10
    if (_column->type == D_DB_TYPE_DECIMAL)
    {
        return d_internal_db_format(_sql,
                                    &quoting,
                                    " DECIMAL(%u, %u)",
                                    (_column->size) ? _column->size : 10u,
                                    _column->scale);
    }

    for (size_t i = 0u; i < count; ++i)
    {
        const struct d_internal_mariadb_spelling* const spelling =
            &d_internal_mariadb_spellings[i];

        // the type's row: sized, plain, or as a key
        if (spelling->type == _column->type)
        {
            return ( (_column->size) &&
                     (spelling->sized) )
                       ? d_internal_db_format(_sql,
                                              &quoting,
                                              spelling->sized,
                                              _column->size)
                       : d_internal_db_format(_sql,
                                              &quoting,
                                              ( (_keyed) &&
                                                (spelling->key) )
                                                  ? spelling->key
                                                  : spelling->plain);
        }
    }

    return D_INTERNAL_DB_FORMAT_BAD;
}

/**
 * @brief Appends one column's definition.
 *
 * @param[in]     _db       the connection, for its quoting.
 * @param[in,out] _sql      the definition so far.
 * @param[in]     _column   the column.
 * @param[in]     _written  the key constraints written on the column, of
 *                          those its flags ask for: D_DB_COLUMN_PRIMARY_KEY,
 *                          as for a sole key column, and D_DB_COLUMN_UNIQUE.
 *                          A key flag left out still chooses the type.
 * @return as d_internal_db_format; BAD for an empty name, an unknown flag,
 *         or a type d_internal_mariadb_append_type refuses, which leaves
 *         what was written for the caller to release.
 *
 * A TIMESTAMP that may be NULL is declared NULL, so that no server gives it
 * a default of its own. The DEFAULT is the caller's SQL, unparenthesized,
 * which MySQL before 8.0.13 requires of a literal.
 */
static enum d_internal_db_format_status
d_internal_mariadb_append_column(
    struct d_mariadb*            _db,
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _column,
    unsigned                     _written
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);
    const unsigned                     flags   = _column->flags;
    const unsigned                     written = (flags & _written);

    // a column needs a name and known flags
    if ( (!d_internal_mariadb_name_ok(_column->name)) ||
         ((flags & ~D_INTERNAL_MARIADB_COLUMN_FLAGS) != 0u) )
    {
        return D_INTERNAL_DB_FORMAT_BAD;
    }

    const bool  keyed     = ((flags & (D_DB_COLUMN_PRIMARY_KEY |
                                       D_DB_COLUMN_UNIQUE)) != 0u);
    const bool  timestamp = ( (!_column->type_sql) &&
                              (_column->type == D_DB_TYPE_TIMESTAMP) &&
                              (!keyed) );
    const char* dflt      = _column->default_sql;
    enum d_internal_db_format_status status =
        d_internal_db_format(_sql, &quoting, "%I", _column->name);

    // the type, then the attributes, in the order the grammar lists them
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_mariadb_append_type(_db, _sql, _column, keyed);
    }

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_db_format(
            _sql,
            &quoting,
            "%s%s%s%s%s%s",
            ((flags & D_DB_COLUMN_NOT_NULL) != 0u) ? " NOT NULL"
            : (timestamp)                          ? " NULL"
                                                   : "",
            (dflt) ? " DEFAULT " : "",
            (dflt) ? dflt : "",
            ((flags & D_DB_COLUMN_AUTO_INCREMENT) != 0u) ? " AUTO_INCREMENT"
                                                         : "",
            ((written & D_DB_COLUMN_UNIQUE) != 0u) ? " UNIQUE" : "",
            ((written & D_DB_COLUMN_PRIMARY_KEY) != 0u) ? " PRIMARY KEY"
                                                        : "");
    }

    return status;
}

/**
 * @brief Counts a column set's primary-key columns, and checks that
 *        AUTO_INCREMENT appears only where the layer allows it.
 *
 * @param[in]  _columns  the columns.
 * @param[in]  _count    how many.
 * @param[out] _keys     receives the number of primary-key columns.
 * @return `true`, or `false` for AUTO_INCREMENT on a column that is not the
 *         sole primary key.
 */
static bool
d_internal_mariadb_count_keys(
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

        // AUTO_INCREMENT off the key is refused
        if ( ((flags & D_DB_COLUMN_AUTO_INCREMENT) != 0u) &&
             ((flags & D_DB_COLUMN_PRIMARY_KEY) == 0u) )
        {
            return false;
        }

        keys           += ((flags & D_DB_COLUMN_PRIMARY_KEY) != 0u) ? 1u : 0u;
        auto_increment  = ( (auto_increment) ||
                            ((flags & D_DB_COLUMN_AUTO_INCREMENT) != 0u) );
    }

    *_keys = keys;

    // and on a composite key
    return ( (!auto_increment) ||
             (keys == 1u) );
}

/**
 * @brief Writes a column set as the definition between CREATE TABLE's
 *        parentheses.
 *
 * @param[in]     _db       the connection, for its quoting.
 * @param[in,out] _sql      the definition.
 * @param[in]     _columns  the columns.
 * @param[in]     _count    how many.
 * @return as d_internal_db_format; BAD for a column the layer refuses,
 *         which leaves what was written for the caller to release.
 *
 * A sole key is written on its column, beside its AUTO_INCREMENT; a
 * composite one as a table constraint after the columns.
 */
static enum d_internal_db_format_status
d_internal_mariadb_build_definition(
    struct d_mariadb*            _db,
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _columns,
    size_t                       _count
)
{
    const struct d_internal_db_quoting quoting   =
        d_internal_mariadb_quoting(_db);
    size_t                             keys      = 0u;
    const char*                        separator = ", PRIMARY KEY (";
    enum d_internal_db_format_status   status    =
        (d_internal_mariadb_count_keys(_columns, _count, &keys))
            ? D_INTERNAL_DB_FORMAT_OK
            : D_INTERNAL_DB_FORMAT_BAD;

    // each column, comma-separated
    for (size_t i = 0u; (status == D_INTERNAL_DB_FORMAT_OK) && (i < _count);
         ++i)
    {
        status = (i) ? d_internal_db_format(_sql, &quoting, ", ")
                     : D_INTERNAL_DB_FORMAT_OK;

        if (status == D_INTERNAL_DB_FORMAT_OK)
        {
            status = d_internal_mariadb_append_column(
                _db,
                _sql,
                &_columns[i],
                (keys == 1u) ? (D_DB_COLUMN_PRIMARY_KEY | D_DB_COLUMN_UNIQUE)
                             : D_DB_COLUMN_UNIQUE);
        }
    }

    // a composite key, as a constraint naming its columns in order
    for (size_t i = 0u; (status == D_INTERNAL_DB_FORMAT_OK) &&
                        (keys > 1u) &&
                        (i < _count);
         ++i)
    {
        if ((_columns[i].flags & D_DB_COLUMN_PRIMARY_KEY) != 0u)
        {
            status    = d_internal_db_format(_sql,
                                             &quoting,
                                             "%s%I",
                                             separator,
                                             _columns[i].name);
            separator = ", ";
        }
    }

    return ( (status == D_INTERNAL_DB_FORMAT_OK) &&
             (keys > 1u) )
               ? d_internal_db_format(_sql, &quoting, ")")
               : status;
}

/*
d_internal_mariadb_create_table
  CREATE TABLE once the definition is settled, in the layer's character set
unless the build left that to the server.
*/
static int
d_internal_mariadb_create_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _definition,
    unsigned          _flags
)
{
    const char* const charset = D_INTERNAL_MARIADB_CHARSET;
    const char* const exists  = ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
                                    ? "IF NOT EXISTS "
                                    : "";

    return (charset[0] != '\0')
               ? d_internal_mariadb_run(_db,
                                        NULL,
                                        "CREATE TABLE %s%T (%s) "
                                        "DEFAULT CHARACTER SET %I",
                                        exists,
                                        _schema,
                                        _table,
                                        _definition,
                                        charset)
               : d_internal_mariadb_run(_db,
                                        NULL,
                                        "CREATE TABLE %s%T (%s)",
                                        exists,
                                        _schema,
                                        _table,
                                        _definition);
}

/**
 * @brief Settles an IF [NOT] EXISTS the server cannot, by asking whether the
 *        object is there.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to ask after.
 * @param[in]  _path    where.
 * @param[in]  _wanted  whether the statement needs it there (a drop) or not
 *                      (a create).
 * @param[out] _done    receives whether the answer makes the statement
 *                      unnecessary.
 * @return 0, or the failure to ask, `_done` then false.
 */
static int
d_internal_mariadb_settled(
    struct d_mariadb*                     _db,
    enum d_internal_mariadb_object        _object,
    const struct d_internal_mariadb_path* _path,
    bool                                  _wanted,
    bool*                                 _done
)
{
    bool      exists = false;
    const int result = d_internal_mariadb_has(_db, _object, _path, &exists);

    *_done = ( (result == 0) &&
               (exists != _wanted) );

    return result;
}

/*
d_internal_mariadb_needs_quotes
  Whether MySQL wrote a default as a string's bare value, which needs quoting
to be SQL. An expression is SQL, marked DEFAULT_GENERATED from 8.0.13, as
CURRENT_TIMESTAMP is, which before that is recognised by its words; numbers
and bit values are SQL, and from 8.0 a binary value too, in hexadecimal.
*/
static bool
d_internal_mariadb_needs_quotes(
    const struct d_mariadb* _db,
    const char*             _extra,
    const char*             _type,
    const char*             _value
)
{
    const size_t sql_types    = sizeof(d_internal_mariadb_sql_types) /
                                sizeof(d_internal_mariadb_sql_types[0]);
    const size_t binary_types = sizeof(d_internal_mariadb_binary_types) /
                                sizeof(d_internal_mariadb_binary_types[0]);

    // an expression, marked
    if ( (_extra) &&
         (strstr(_extra, "DEFAULT_GENERATED")) )
    {
        return false;
    }

    // CURRENT_TIMESTAMP, unmarked before 8.0.13
    if ( (_type)                                              &&
         ( (strcmp(_type, "timestamp") == 0) ||
           (strcmp(_type, "datetime") == 0) )                 &&
         (strncmp(_value, "CURRENT_TIMESTAMP", 17u) == 0) )
    {
        return false;
    }

    // numbers and bit values; binary values in hexadecimal from 8.0
    return !( (_type) &&
              ( (d_internal_mariadb_listed(_type,
                                           d_internal_mariadb_sql_types,
                                           sql_types)) ||
                ( (_db->version >= 80000ul)                         &&
                  (strncmp(_value, "0x", 2u) == 0)                  &&
                  (d_internal_mariadb_listed(_type,
                                             d_internal_mariadb_binary_types,
                                             binary_types)) ) ) );
}

/**
 * @brief Renders a column's default, as information_schema reports it, as
 *        SQL.
 *
 * @param[in]     _db      the connection.
 * @param[in]     _row     the column's row: COLUMN_DEFAULT at 2, EXTRA at 4,
 *                         DATA_TYPE at 5.
 * @param[in,out] _quoted  an empty buffer, for a value that needs quoting;
 *                         the caller releases it.
 * @param[out]    _sql     receives the default as SQL, or `NULL` for none.
 * @return `true`, or `false` when memory ran out.
 *
 * MariaDB writes NULL as the word, and every other default as SQL; MySQL
 * writes NULL as NULL, and needs d_internal_mariadb_needs_quotes.
 */
static bool
d_internal_mariadb_default_sql(
    struct d_mariadb*            _db,
    MYSQL_ROW                    _row,
    struct d_internal_db_buffer* _quoted,
    const char**                 _sql
)
{
    const char* const value   = _row[2];
    const bool        mariadb = (_db->flavor == D_MARIADB_FLAVOR_MARIADB);

    // none; MariaDB's NULL is none too
    if ( (!value) ||
         ( (mariadb) &&
           (strcmp(value, "NULL") == 0) ) )
    {
        *_sql = NULL;

        return true;
    }

    // SQL already
    if ( (mariadb) ||
         (!d_internal_mariadb_needs_quotes(_db, _row[4], _row[5], value)) )
    {
        *_sql = value;

        return true;
    }

    // a string's value, quoted as one
    if ( (!d_internal_mariadb_quote_value(_db, _quoted, value)) ||
         (!d_internal_db_buffer_append(_quoted, "", 1u)) )
    {
        return false;
    }

    *_sql = _quoted->data;

    return true;
}

/*
d_internal_mariadb_add_column_info
  One row of the column query -- COLUMN_NAME, COLUMN_TYPE, COLUMN_DEFAULT,
IS_NULLABLE, EXTRA, DATA_TYPE, SEQ_IN_INDEX -- into the list, a column
generated where EXTRA says so (VIRTUAL, STORED or, on older MariaDB,
PERSISTENT GENERATED, never MySQL's DEFAULT_GENERATED); the builder
copies every string, so the quoted default's buffer goes at once.
*/
static int
d_internal_mariadb_add_column_info(
    struct d_mariadb*             _db,
    struct d_internal_db_columns* _columns,
    MYSQL_ROW                     _row
)
{
    struct d_internal_db_buffer quoted   = { NULL, 0u, 0u };
    const char*                 dflt     = NULL;
    int64_t                     position = 0;

    // the default, as SQL
    if (!d_internal_mariadb_default_sql(_db, _row, &quoted, &dflt))
    {
        d_internal_db_buffer_release(&quoted);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    const struct d_db_column_info info =
    {
        _row[0],
        _row[1],
        dflt,
        ( (_row[6]) &&
          (d_internal_db_parse_integer(_row[6], &position)) )
            ? (int)position
            : 0,
        ( (_row[3]) &&
          (strcmp(_row[3], "NO") == 0) ),
        ( (_row[4]) &&
          ( (strstr(_row[4], "VIRTUAL GENERATED"))    ||
            (strstr(_row[4], "STORED GENERATED"))     ||
            (strstr(_row[4], "PERSISTENT GENERATED")) ) )
    };
    const bool added = ( (_row[0]) &&
                         (d_internal_db_columns_add(_columns, &info)) );

    d_internal_db_buffer_release(&quoted);

    return (added) ? 0 : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
}

/**
 * @brief Appends the derived table of a table's primary-key columns and
 *        their places, joined to the columns.
 *
 * @param[in]     _db      the connection.
 * @param[in,out] _sql     the query so far.
 * @param[in]     _schema  the database; `NULL` for the current.
 * @param[in]     _table   the table.
 * @param[in]     _exact   whether the server matches table names by case.
 * @return as d_internal_db_format.
 *
 * The derived table's own conditions name the table, which keeps MariaDB
 * from reading every table's indexes to join them.
 */
static enum d_internal_db_format_status
d_internal_mariadb_key_sql(
    struct d_mariadb*            _db,
    struct d_internal_db_buffer* _sql,
    const char*                  _schema,
    const char*                  _table,
    bool                         _exact
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);
    enum d_internal_db_format_status   status  = d_internal_db_format(
        _sql,
        &quoting,
        " LEFT JOIN (SELECT COLUMN_NAME, SEQ_IN_INDEX "
        "FROM information_schema.STATISTICS WHERE INDEX_NAME = 'PRIMARY'");

    // the table's, and no other's
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_mariadb_match(_db,
                                          _sql,
                                          "TABLE_SCHEMA",
                                          _schema,
                                          _exact);
    }

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_mariadb_match(_db,
                                          _sql,
                                          "TABLE_NAME",
                                          _table,
                                          _exact);
    }

    return (status == D_INTERNAL_DB_FORMAT_OK)
               ? d_internal_db_format(_sql,
                                      &quoting,
                                      ") AS k ON k.COLUMN_NAME = c.COLUMN_NAME")
               : status;
}

/**
 * @brief Writes the query that describes a table's columns.
 *
 * @param[in]     _db      the connection.
 * @param[in,out] _sql     receives the query.
 * @param[in]     _schema  the database; `NULL` for the current.
 * @param[in]     _table   the table.
 * @param[in]     _exact   whether the server matches table names by case.
 * @return as d_internal_db_format.
 *
 * Each row is COLUMN_NAME, COLUMN_TYPE, COLUMN_DEFAULT, IS_NULLABLE, EXTRA,
 * DATA_TYPE and the column's place in the primary key, in the table's
 * order.
 */
static enum d_internal_db_format_status
d_internal_mariadb_columns_sql(
    struct d_mariadb*            _db,
    struct d_internal_db_buffer* _sql,
    const char*                  _schema,
    const char*                  _table,
    bool                         _exact
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);
    enum d_internal_db_format_status   status  = d_internal_db_format(
        _sql,
        &quoting,
        "SELECT c.COLUMN_NAME, c.COLUMN_TYPE, c.COLUMN_DEFAULT, "
        "c.IS_NULLABLE, c.EXTRA, c.DATA_TYPE, k.SEQ_IN_INDEX "
        "FROM information_schema.COLUMNS AS c");

    // the key's places, then the table's columns
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_mariadb_key_sql(_db, _sql, _schema, _table, _exact);
    }

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_db_format(_sql, &quoting, " WHERE TRUE");
    }

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_mariadb_match(_db,
                                          _sql,
                                          "c.TABLE_SCHEMA",
                                          _schema,
                                          _exact);
    }

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_mariadb_match(_db,
                                          _sql,
                                          "c.TABLE_NAME",
                                          _table,
                                          _exact);
    }

    return (status == D_INTERNAL_DB_FORMAT_OK)
               ? d_internal_db_format(_sql,
                                      &quoting,
                                      " ORDER BY c.ORDINAL_POSITION")
               : status;
}

/*
d_internal_mariadb_read_columns
  Each row into the builder, then the list; a table with no rows is no
table. The rows are freed whatever happens.
*/
static int
d_internal_mariadb_read_columns(
    struct d_mariadb*        _db,
    MYSQL_RES*               _rows,
    struct d_db_column_list* _columns
)
{
    struct d_internal_db_columns builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u
    };
    int result = 0;

    // each column, in the table's order
    for (MYSQL_ROW row = ( (_rows) &&
                           (mysql_num_fields(_rows) >= 7u) )
                             ? mysql_fetch_row(_rows)
                             : NULL;
         (result == 0) && (row);
         row = mysql_fetch_row(_rows))
    {
        result = d_internal_mariadb_add_column_info(_db, &builder, row);
    }

    mysql_free_result(_rows);

    // no columns: no table
    if ( (result == 0) &&
         (builder.count == 0u) )
    {
        result = D_DB_NOT_FOUND;
    }

    if (result != 0)
    {
        d_internal_db_columns_release(&builder);

        return result;
    }

    return (d_internal_db_columns_finish(&builder, _columns))
               ? 0
               : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
}

//==============================================================================
// 2.  TABLES
//==============================================================================

// 2.1    Existence
//------------------------------------------------------------------------------
/*
d_mariadb_has_table
  The _s form, with every failure read as "no".
*/
bool
d_mariadb_has_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table
)
{
    bool exists = false;

    return ( (d_mariadb_has_table_s(_db, _schema, _table, &exists) == 0) &&
             (exists) );
}

/*
d_mariadb_has_table_s
  A base table's row in information_schema.TABLES; a temporary table has
none on MySQL, and MariaDB's, which have one from 11.2, are left out to
match.
*/
int
d_mariadb_has_table_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    bool*             _exists
)
{
    const struct d_internal_mariadb_path path = { _schema, NULL, _table };

    return d_internal_mariadb_has(_db,
                                  D_INTERNAL_MARIADB_TABLE,
                                  &path,
                                  _exists);
}

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_mariadb_create_table
  The columns become a definition, which then goes the way of
d_mariadb_create_table_sql's. Six parameters, one past the guide's tripwire:
the columns and their count are one argument, split in two as C passes an
array.
*/
int
d_mariadb_create_table(
    struct d_mariadb*         _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _columns,
    size_t                    _count,
    unsigned                  _flags
)
{
    struct d_internal_db_buffer definition = { NULL, 0u, 0u };

    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!_columns)                              ||
         (_count == 0u)                           ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_format_result(
        d_internal_mariadb_build_definition(_db,
                                            &definition,
                                            _columns,
                                            _count));

    // the table, once its definition is whole; a column refused part way
    // leaves text behind, which goes with the rest
    if (result == 0)
    {
        result = d_internal_mariadb_create_table(_db,
                                                 _schema,
                                                 _table,
                                                 definition.data,
                                                 _flags);
    }

    d_internal_db_buffer_release(&definition);

    return result;
}

/*
d_mariadb_create_table_sql
  The caller's definition, between the parentheses as written.
*/
int
d_mariadb_create_table_sql(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _definition,
    unsigned          _flags
)
{
    // parameter validation
    if ( (!_db)                                     ||
         (!d_internal_mariadb_schema_ok(_schema))   ||
         (!d_internal_mariadb_name_ok(_table))      ||
         (!d_internal_mariadb_name_ok(_definition)) ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_create_table(_db,
                                           _schema,
                                           _table,
                                           _definition,
                                           _flags);
}

/*
d_mariadb_drop_table
  The servers' own DROP TABLE [IF EXISTS].
*/
int
d_mariadb_drop_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    unsigned          _flags
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         ((_flags & ~D_DB_IF_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "DROP TABLE %s%T",
                                  ((_flags & D_DB_IF_EXISTS) != 0u)
                                      ? "IF EXISTS "
                                      : "",
                                  _schema,
                                  _table);
}

// 2.3    Changing
//------------------------------------------------------------------------------
/*
d_mariadb_rename_table
  RENAME TABLE, the new name in the same database.
*/
int
d_mariadb_rename_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _new_name
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!d_internal_mariadb_name_ok(_new_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "RENAME TABLE %T TO %T",
                                  _schema,
                                  _table,
                                  _schema,
                                  _new_name);
}

// 2.4    Rows
//------------------------------------------------------------------------------
/*
d_mariadb_truncate_table
  TRUNCATE TABLE, which the servers carry out as a drop and re-create.
*/
int
d_mariadb_truncate_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "TRUNCATE TABLE %T",
                                  _schema,
                                  _table);
}

/*
d_mariadb_count_rows
  SELECT COUNT(*), which InnoDB answers exactly, by reading an index.
*/
int
d_mariadb_count_rows(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    int64_t*          _count
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!_count) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_query_int64(_db,
                                          _count,
                                          "SELECT COUNT(*) FROM %T",
                                          _schema,
                                          _table);
}

// 2.5    Listing
//------------------------------------------------------------------------------
/*
d_mariadb_list_tables
  The base tables' rows of information_schema.TABLES.
*/
int
d_mariadb_list_tables(
    struct d_mariadb*      _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    const struct d_internal_mariadb_path path = { _schema, NULL, NULL };

    return d_internal_mariadb_list(_db,
                                   D_INTERNAL_MARIADB_TABLE,
                                   &path,
                                   _names);
}

//==============================================================================
// 3.  COLUMNS
//==============================================================================

// 3.1    Existence
//------------------------------------------------------------------------------
/*
d_mariadb_has_column
  The _s form, with every failure read as "no".
*/
bool
d_mariadb_has_column(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column
)
{
    bool exists = false;

    return ( (d_mariadb_has_column_s(_db,
                                     _schema,
                                     _table,
                                     _column,
                                     &exists) == 0) &&
             (exists) );
}

/*
d_mariadb_has_column_s
  A row of information_schema.COLUMNS, which a view's columns have too.
*/
int
d_mariadb_has_column_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column,
    bool*             _exists
)
{
    const struct d_internal_mariadb_path path = { _schema, _table, _column };

    return d_internal_mariadb_has(_db,
                                  D_INTERNAL_MARIADB_COLUMN,
                                  &path,
                                  _exists);
}

// 3.2    Adding, redefining, renaming and dropping
//------------------------------------------------------------------------------
/*
d_mariadb_add_column
  The column's definition, judged as a table's sole column would be, after
ADD COLUMN.
*/
int
d_mariadb_add_column(
    struct d_mariadb*         _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _column
)
{
    struct d_internal_db_buffer definition = { NULL, 0u, 0u };
    size_t                      keys       = 0u;

    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!_column)                               ||
         (!d_internal_mariadb_count_keys(_column, 1u, &keys)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_format_result(
        d_internal_mariadb_append_column(_db,
                                         &definition,
                                         _column,
                                         (D_DB_COLUMN_PRIMARY_KEY |
                                          D_DB_COLUMN_UNIQUE)));

    // the column, once its definition is whole; one refused part way leaves
    // text behind, which goes with the rest
    if (result == 0)
    {
        result = d_internal_mariadb_run(_db,
                                        NULL,
                                        "ALTER TABLE %T ADD COLUMN %s",
                                        _schema,
                                        _table,
                                        definition.data);
    }

    d_internal_db_buffer_release(&definition);

    return result;
}

/*
d_mariadb_add_column_sql
  The caller's definition, after ADD COLUMN as written.
*/
int
d_mariadb_add_column_sql(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _definition
)
{
    // parameter validation
    if ( (!_db)                                     ||
         (!d_internal_mariadb_schema_ok(_schema))   ||
         (!d_internal_mariadb_name_ok(_table))      ||
         (!d_internal_mariadb_name_ok(_definition)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "ALTER TABLE %T ADD COLUMN %s",
                                  _schema,
                                  _table,
                                  _definition);
}

/*
d_mariadb_modify_column
  The column's definition, judged as add_column judges one, after MODIFY
COLUMN, with no key constraint written: the column keeps the keys it has.
*/
int
d_mariadb_modify_column(
    struct d_mariadb*         _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _column
)
{
    struct d_internal_db_buffer definition = { NULL, 0u, 0u };
    size_t                      keys       = 0u;

    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!_column)                               ||
         (!d_internal_mariadb_count_keys(_column, 1u, &keys)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_format_result(
        d_internal_mariadb_append_column(_db, &definition, _column, 0u));

    // the column, once its definition is whole; one refused part way leaves
    // text behind, which goes with the rest
    if (result == 0)
    {
        result = d_internal_mariadb_run(_db,
                                        NULL,
                                        "ALTER TABLE %T MODIFY COLUMN %s",
                                        _schema,
                                        _table,
                                        definition.data);
    }

    d_internal_db_buffer_release(&definition);

    return result;
}

/*
d_mariadb_rename_column
  RENAME COLUMN, where the server has it; CHANGE COLUMN, which older ones
offer, would need the column's whole definition restated, which the layer
does not attempt.
*/
int
d_mariadb_rename_column(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column,
    const char*       _new_name
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!d_internal_mariadb_name_ok(_column))   ||
         (!d_internal_mariadb_name_ok(_new_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a server old enough to lack it
    if (!d_mariadb_supports(_db, D_MARIADB_FEATURE_RENAME_COLUMN))
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "ALTER TABLE %T RENAME COLUMN %I TO %I",
                                  _schema,
                                  _table,
                                  _column,
                                  _new_name);
}

/*
d_mariadb_drop_column
  DROP COLUMN, which the servers refuse for a table's last column.
*/
int
d_mariadb_drop_column(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!d_internal_mariadb_name_ok(_column)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "ALTER TABLE %T DROP COLUMN %I",
                                  _schema,
                                  _table,
                                  _column);
}

// 3.3    Listing
//------------------------------------------------------------------------------
/*
d_mariadb_list_columns
  The server's case rule decides whether the table's name is matched by its
bytes; then the query, and its rows read into the list.
*/
int
d_mariadb_list_columns(
    struct d_mariadb*        _db,
    const char*              _schema,
    const char*              _table,
    struct d_db_column_list* _columns
)
{
    struct d_internal_db_buffer sql  = { NULL, 0u, 0u };
    MYSQL_RES*                  rows = NULL;
    int                         rule = 0;

    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table))    ||
         (!_columns) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_case_rule(_db, &rule);

    // the rule, the query, the rows
    if (result == 0)
    {
        result = d_internal_mariadb_format_result(
            d_internal_mariadb_columns_sql(_db,
                                           &sql,
                                           _schema,
                                           _table,
                                           (rule == 0)));
    }

    if (result == 0)
    {
        result = d_internal_mariadb_send(_db, sql.data, sql.length, &rows);
        d_internal_db_buffer_release(&sql);
    }

    return (result == 0)
               ? d_internal_mariadb_read_columns(_db, rows, _columns)
               : result;
}

//==============================================================================
// 4.  INDEXES
//==============================================================================

// 4.1    Existence
//------------------------------------------------------------------------------
/*
d_mariadb_has_index
  The _s form, with every failure read as "no".
*/
bool
d_mariadb_has_index(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table
)
{
    bool exists = false;

    return ( (d_mariadb_has_index_s(_db,
                                    _schema,
                                    _index,
                                    _table,
                                    &exists) == 0) &&
             (exists) );
}

/*
d_mariadb_has_index_s
  A row of information_schema.STATISTICS.
*/
int
d_mariadb_has_index_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table,
    bool*             _exists
)
{
    const struct d_internal_mariadb_path path = { _schema, _table, _index };

    return d_internal_mariadb_has(_db,
                                  D_INTERNAL_MARIADB_INDEX,
                                  &path,
                                  _exists);
}

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_mariadb_create_index
  MariaDB has CREATE INDEX IF NOT EXISTS (10.1.4); MySQL has not, and is
asked first. Six parameters, one past the guide's tripwire: each is a
separate part of the statement, and a struct would only rename them.
*/
int
d_mariadb_create_index(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table,
    const char*       _columns,
    unsigned          _flags
)
{
    const struct d_internal_mariadb_path path = { _schema, _table, _index };
    bool                                 done = false;

    // parameter validation
    if ( (!_db)                                                ||
         (!d_internal_mariadb_schema_ok(_schema))              ||
         (!d_internal_mariadb_name_ok(_index))                 ||
         (!d_internal_mariadb_name_ok(_table))                 ||
         (!d_internal_mariadb_name_ok(_columns))               ||
         ((_flags & ~(D_DB_UNIQUE | D_DB_IF_NOT_EXISTS)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const bool unless = ((_flags & D_DB_IF_NOT_EXISTS) != 0u);
    const bool native = d_internal_mariadb_at_least(_db,
                                                    D_MARIADB_FLAVOR_MARIADB,
                                                    100104ul);

    // asked first, where the server has no clause for it
    if ( (unless) &&
         (!native) )
    {
        const int result = d_internal_mariadb_settled(_db,
                                                      D_INTERNAL_MARIADB_INDEX,
                                                      &path,
                                                      false,
                                                      &done);

        if ( (result != 0) ||
             (done) )
        {
            return result;
        }
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "CREATE %sINDEX %s%I ON %T (%s)",
                                  ((_flags & D_DB_UNIQUE) != 0u) ? "UNIQUE "
                                                                 : "",
                                  ( (unless) &&
                                    (native) ) ? "IF NOT EXISTS " : "",
                                  _index,
                                  _schema,
                                  _table,
                                  _columns);
}

/*
d_mariadb_drop_index
  MariaDB has DROP INDEX IF EXISTS (10.1.4); MySQL has not, and is asked
first.
*/
int
d_mariadb_drop_index(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table,
    unsigned          _flags
)
{
    const struct d_internal_mariadb_path path = { _schema, _table, _index };
    bool                                 done = false;

    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_index))    ||
         (!d_internal_mariadb_name_ok(_table))    ||
         ((_flags & ~D_DB_IF_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const bool unless = ((_flags & D_DB_IF_EXISTS) != 0u);
    const bool native = d_internal_mariadb_at_least(_db,
                                                    D_MARIADB_FLAVOR_MARIADB,
                                                    100104ul);

    // asked first, where the server has no clause for it
    if ( (unless) &&
         (!native) )
    {
        const int result = d_internal_mariadb_settled(_db,
                                                      D_INTERNAL_MARIADB_INDEX,
                                                      &path,
                                                      true,
                                                      &done);

        if ( (result != 0) ||
             (done) )
        {
            return result;
        }
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "DROP INDEX %s%I ON %T",
                                  ( (unless) &&
                                    (native) ) ? "IF EXISTS " : "",
                                  _index,
                                  _schema,
                                  _table);
}

// 4.3    Listing
//------------------------------------------------------------------------------
/*
d_mariadb_list_indexes
  STATISTICS' first key part of each index but PRIMARY.
*/
int
d_mariadb_list_indexes(
    struct d_mariadb*      _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    const struct d_internal_mariadb_path path = { _schema, _table, NULL };

    return d_internal_mariadb_list(_db,
                                   D_INTERNAL_MARIADB_INDEX,
                                   &path,
                                   _names);
}

//==============================================================================
// 5.  VIEWS
//==============================================================================

// 5.1    Existence
//------------------------------------------------------------------------------
/*
d_mariadb_has_view
  The _s form, with every failure read as "no".
*/
bool
d_mariadb_has_view(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view
)
{
    bool exists = false;

    return ( (d_mariadb_has_view_s(_db, _schema, _view, &exists) == 0) &&
             (exists) );
}

/*
d_mariadb_has_view_s
  A view's row in information_schema.TABLES.
*/
int
d_mariadb_has_view_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view,
    bool*             _exists
)
{
    const struct d_internal_mariadb_path path = { _schema, NULL, _view };

    return d_internal_mariadb_has(_db,
                                  D_INTERNAL_MARIADB_VIEW,
                                  &path,
                                  _exists);
}

// 5.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_mariadb_create_view
  MariaDB has CREATE VIEW IF NOT EXISTS (10.1.3), which a table of the name
satisfies too; MySQL has not, and is asked after any table or view of the
name, to match.
*/
int
d_mariadb_create_view(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view,
    const char*       _select,
    unsigned          _flags
)
{
    const struct d_internal_mariadb_path path = { _schema, NULL, _view };
    bool                                 done = false;

    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_view))     ||
         (!d_internal_mariadb_name_ok(_select))   ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const bool unless = ((_flags & D_DB_IF_NOT_EXISTS) != 0u);
    const bool native = d_internal_mariadb_at_least(_db,
                                                    D_MARIADB_FLAVOR_MARIADB,
                                                    100103ul);

    // asked first, where the server has no clause for it
    if ( (unless) &&
         (!native) )
    {
        const int result = d_internal_mariadb_settled(
            _db,
            D_INTERNAL_MARIADB_RELATION,
            &path,
            false,
            &done);

        if ( (result != 0) ||
             (done) )
        {
            return result;
        }
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "CREATE VIEW %s%T AS %s",
                                  ( (unless) &&
                                    (native) ) ? "IF NOT EXISTS " : "",
                                  _schema,
                                  _view,
                                  _select);
}

/*
d_mariadb_drop_view
  The servers' own DROP VIEW [IF EXISTS].
*/
int
d_mariadb_drop_view(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view,
    unsigned          _flags
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_view))     ||
         ((_flags & ~D_DB_IF_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "DROP VIEW %s%T",
                                  ((_flags & D_DB_IF_EXISTS) != 0u)
                                      ? "IF EXISTS "
                                      : "",
                                  _schema,
                                  _view);
}

// 5.3    Listing
//------------------------------------------------------------------------------
/*
d_mariadb_list_views
  The views' rows of information_schema.TABLES.
*/
int
d_mariadb_list_views(
    struct d_mariadb*      _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    const struct d_internal_mariadb_path path = { _schema, NULL, NULL };

    return d_internal_mariadb_list(_db,
                                   D_INTERNAL_MARIADB_VIEW,
                                   &path,
                                   _names);
}

//==============================================================================
// 6.  TRIGGERS
//==============================================================================

// 6.1    Existence
//------------------------------------------------------------------------------
/*
d_mariadb_has_trigger
  The _s form, with every failure read as "no".
*/
bool
d_mariadb_has_trigger(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger
)
{
    bool exists = false;

    return ( (d_mariadb_has_trigger_s(_db, _schema, _trigger, &exists) == 0) &&
             (exists) );
}

/*
d_mariadb_has_trigger_s
  A row of information_schema.TRIGGERS.
*/
int
d_mariadb_has_trigger_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger,
    bool*             _exists
)
{
    const struct d_internal_mariadb_path path = { _schema, NULL, _trigger };

    return d_internal_mariadb_has(_db,
                                  D_INTERNAL_MARIADB_TRIGGER,
                                  &path,
                                  _exists);
}

// 6.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_mariadb_create_trigger
  CREATE TRIGGER IF NOT EXISTS is MariaDB's from 10.1.4 and MySQL's from
8.0.29; an older MySQL is asked first.
*/
int
d_mariadb_create_trigger(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger,
    const char*       _definition,
    unsigned          _flags
)
{
    const struct d_internal_mariadb_path path = { _schema, NULL, _trigger };
    bool                                 done = false;

    // parameter validation
    if ( (!_db)                                     ||
         (!d_internal_mariadb_schema_ok(_schema))   ||
         (!d_internal_mariadb_name_ok(_trigger))    ||
         (!d_internal_mariadb_name_ok(_definition)) ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const bool unless = ((_flags & D_DB_IF_NOT_EXISTS) != 0u);
    const bool native =
        ( (d_internal_mariadb_at_least(_db,
                                       D_MARIADB_FLAVOR_MARIADB,
                                       100104ul)) ||
          (d_internal_mariadb_at_least(_db, D_MARIADB_FLAVOR_MYSQL, 80029ul)) );

    // asked first, where the server has no clause for it
    if ( (unless) &&
         (!native) )
    {
        const int result = d_internal_mariadb_settled(
            _db,
            D_INTERNAL_MARIADB_TRIGGER,
            &path,
            false,
            &done);

        if ( (result != 0) ||
             (done) )
        {
            return result;
        }
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "CREATE TRIGGER %s%T %s",
                                  ( (unless) &&
                                    (native) ) ? "IF NOT EXISTS " : "",
                                  _schema,
                                  _trigger,
                                  _definition);
}

/*
d_mariadb_drop_trigger
  The servers' own DROP TRIGGER [IF EXISTS].
*/
int
d_mariadb_drop_trigger(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger,
    unsigned          _flags
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_trigger))  ||
         ((_flags & ~D_DB_IF_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "DROP TRIGGER %s%T",
                                  ((_flags & D_DB_IF_EXISTS) != 0u)
                                      ? "IF EXISTS "
                                      : "",
                                  _schema,
                                  _trigger);
}

// 6.3    Listing
//------------------------------------------------------------------------------
/*
d_mariadb_list_triggers
  information_schema.TRIGGERS, narrowed to one table's by
EVENT_OBJECT_TABLE when a table is named.
*/
int
d_mariadb_list_triggers(
    struct d_mariadb*      _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    const struct d_internal_mariadb_path path = { _schema, _table, NULL };

    return d_internal_mariadb_list(_db,
                                   D_INTERNAL_MARIADB_TRIGGER,
                                   &path,
                                   _names);
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_has_table
  Without a client, false.
*/
bool
d_mariadb_has_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table
)
{
    (void)_db;
    (void)_schema;
    (void)_table;

    return false;
}

/*
d_mariadb_has_table_s
  Without a client there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_mariadb_has_table_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    bool*             _exists
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
d_mariadb_create_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_create_table(
    struct d_mariadb*         _db,
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
d_mariadb_create_table_sql
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_create_table_sql(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _definition,
    unsigned          _flags
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
d_mariadb_drop_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_drop_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    unsigned          _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_rename_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_rename_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _new_name
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_new_name;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_truncate_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_truncate_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table
)
{
    (void)_db;
    (void)_schema;
    (void)_table;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_count_rows
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_count_rows(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    int64_t*          _count
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_count;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_list_tables
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_list_tables(
    struct d_mariadb*      _db,
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
d_mariadb_has_column
  Without a client, false.
*/
bool
d_mariadb_has_column(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_column;

    return false;
}

/*
d_mariadb_has_column_s
  Without a client there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_mariadb_has_column_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column,
    bool*             _exists
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
d_mariadb_add_column
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_add_column(
    struct d_mariadb*         _db,
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
d_mariadb_add_column_sql
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_add_column_sql(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _definition
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_definition;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_modify_column
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_modify_column(
    struct d_mariadb*         _db,
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
d_mariadb_rename_column
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_rename_column(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column,
    const char*       _new_name
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
d_mariadb_drop_column
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_drop_column(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    const char*       _column
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_column;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_list_columns
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_list_columns(
    struct d_mariadb*        _db,
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
d_mariadb_has_index
  Without a client, false.
*/
bool
d_mariadb_has_index(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table
)
{
    (void)_db;
    (void)_schema;
    (void)_index;
    (void)_table;

    return false;
}

/*
d_mariadb_has_index_s
  Without a client there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_mariadb_has_index_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table,
    bool*             _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_index;
    (void)_table;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_create_index
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_create_index(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table,
    const char*       _columns,
    unsigned          _flags
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
d_mariadb_drop_index
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_drop_index(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _index,
    const char*       _table,
    unsigned          _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_index;
    (void)_table;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_list_indexes
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_list_indexes(
    struct d_mariadb*      _db,
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
d_mariadb_has_view
  Without a client, false.
*/
bool
d_mariadb_has_view(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view
)
{
    (void)_db;
    (void)_schema;
    (void)_view;

    return false;
}

/*
d_mariadb_has_view_s
  Without a client there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_mariadb_has_view_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view,
    bool*             _exists
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
d_mariadb_create_view
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_create_view(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view,
    const char*       _select,
    unsigned          _flags
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
d_mariadb_drop_view
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_drop_view(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _view,
    unsigned          _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_view;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_list_views
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_list_views(
    struct d_mariadb*      _db,
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
d_mariadb_has_trigger
  Without a client, false.
*/
bool
d_mariadb_has_trigger(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;

    return false;
}

/*
d_mariadb_has_trigger_s
  Without a client there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_mariadb_has_trigger_s(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger,
    bool*             _exists
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
d_mariadb_create_trigger
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_create_trigger(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger,
    const char*       _definition,
    unsigned          _flags
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
d_mariadb_drop_trigger
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_drop_trigger(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _trigger,
    unsigned          _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_list_triggers
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_list_triggers(
    struct d_mariadb*      _db,
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

#endif  // D_INTERNAL_MARIADB
