/*******************************************************************************
* djinterp [c]                                                 postgres_schema.c
*
* Implementation of postgres_schema.h.
*   Existence and listing are pg_catalog lookups (postgres_internal.c);
* creating, changing and dropping are the server's own statements, names
* quoted and the caller's SQL used as written. A NULL schema is asked of the
* server, current_schema(), and named in the statement, so that an
* unqualified name never reaches a temporary table, or one further along the
* search path, that the layer's lookups would not have seen. Where the server
* lacks an IF NOT EXISTS -- views and triggers -- the layer runs the CREATE
* and reads "already exists" as success (d_internal_postgres_tolerate).
*   Columns are described from pg_attribute, their defaults from pg_attrdef
* as pg_get_expr writes them, and their key places from the primary key's
* pg_index row; a generated column's expression, kept in pg_attrdef too from
* PostgreSQL 12, is not a default and is left out.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_schema.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_schema.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // d_db_column, flags
#include "../db_internal.h"  // buffers, builders, formatting, numbers
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

// D_INTERNAL_POSTGRES_COLUMN_FLAGS
//   constant: every flag a struct d_db_column may carry.
#define D_INTERNAL_POSTGRES_COLUMN_FLAGS                                       \
    (D_DB_COLUMN_PRIMARY_KEY    |                                              \
     D_DB_COLUMN_AUTO_INCREMENT |                                              \
     D_DB_COLUMN_NOT_NULL       |                                              \
     D_DB_COLUMN_UNIQUE)

// D_INTERNAL_POSTGRES_DESCRIBED
//   constant: the relation kinds whose columns the layer describes: tables,
// ordinary and partitioned, views, materialized views and foreign tables.
#define D_INTERNAL_POSTGRES_DESCRIBED "('r', 'p', 'v', 'm', 'f')"

// d_internal_postgres_spelling
//   struct: how one d_db_type is declared.
struct d_internal_postgres_spelling
{
    enum d_db_type type;   // the type
    const char*    sized;  // with a size, as a format taking it; NULL where
                           // the size is not read
    const char*    plain;  // without one
};

// d_internal_postgres_spellings
//   table: postgres_schema.h's mapping, but for numeric's two numbers.
static const struct d_internal_postgres_spelling
d_internal_postgres_spellings[] =
{
    { D_DB_TYPE_BOOLEAN,        NULL,              " boolean"          },
    { D_DB_TYPE_INTEGER,        NULL,              " integer"          },
    { D_DB_TYPE_BIG_INTEGER,    NULL,              " bigint"           },
    { D_DB_TYPE_FLOATING_POINT, NULL,              " double precision" },
    { D_DB_TYPE_STRING,         " varchar(%u)",    " text"             },
    { D_DB_TYPE_BINARY,         NULL,              " bytea"            },
    { D_DB_TYPE_DATE,           NULL,              " date"             },
    { D_DB_TYPE_TIME,           " time(%u)",       " time"             },
    { D_DB_TYPE_DATETIME,       " timestamp(%u)",  " timestamp"        },
    { D_DB_TYPE_TIMESTAMP,      " timestamptz(%u)", " timestamptz"     },
    { D_DB_TYPE_JSON,           NULL,              " jsonb"            },
    { D_DB_TYPE_XML,            NULL,              " xml"              },
    { D_DB_TYPE_UUID,           NULL,              " uuid"             }
};

/**
 * @brief Appends a column's type, as postgres_schema.h tabulates it.
 *
 * @param[in]     _db      the connection, for its quoting.
 * @param[in,out] _sql     the definition so far.
 * @param[in]     _column  the column.
 * @return as d_internal_db_format; BAD for NONE without type_sql, a numeric
 *         scale without a size, or a type outside enum d_db_type, which
 *         leaves the buffer to the caller.
 */
static enum d_internal_db_format_status
d_internal_postgres_append_type(
    PGconn*                      _db,
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _column
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);
    const size_t count = sizeof(d_internal_postgres_spellings) /
                         sizeof(d_internal_postgres_spellings[0]);

    // the caller's own spelling wins
    if (_column->type_sql)
    {
        return d_internal_db_format(_sql, &quoting, " %s", _column->type_sql);
    }

    // numeric takes two numbers, or neither
    if (_column->type == D_DB_TYPE_DECIMAL)
    {
        return (_column->size)
                   ? d_internal_db_format(_sql,
                                          &quoting,
                                          " numeric(%u, %u)",
                                          _column->size,
                                          _column->scale)
               : (_column->scale)
                   ? D_INTERNAL_DB_FORMAT_BAD
                   : d_internal_db_format(_sql, &quoting, " numeric");
    }

    for (size_t i = 0u; i < count; ++i)
    {
        const struct d_internal_postgres_spelling* const spelling =
            &d_internal_postgres_spellings[i];

        // the type's row: sized or plain
        if (spelling->type == _column->type)
        {
            return ( (_column->size) &&
                     (spelling->sized) )
                       ? d_internal_db_format(_sql,
                                              &quoting,
                                              spelling->sized,
                                              _column->size)
                       : d_internal_db_format(_sql, &quoting, spelling->plain);
        }
    }

    return D_INTERNAL_DB_FORMAT_BAD;
}

/**
 * @brief Appends one column's definition.
 *
 * @param[in]     _db      the connection, for its quoting.
 * @param[in,out] _sql     the definition so far.
 * @param[in]     _column  the column.
 * @param[in]     _key     whether a PRIMARY KEY flag is written on the
 *                         column, as it is for a sole key column.
 * @return as d_internal_db_format; BAD for an empty name, an unknown flag,
 *         or a type d_internal_postgres_append_type refuses, which leaves
 *         what was written for the caller to release.
 *
 * AUTO_INCREMENT is an identity column, BY DEFAULT so that a value can still
 * be given; the server makes it NOT NULL itself.
 */
static enum d_internal_db_format_status
d_internal_postgres_append_column(
    PGconn*                      _db,
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _column,
    bool                         _key
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);
    const unsigned                     flags   = _column->flags;

    // a column needs a name and known flags
    if ( (!d_internal_postgres_name_ok(_column->name)) ||
         ((flags & ~D_INTERNAL_POSTGRES_COLUMN_FLAGS) != 0u) )
    {
        return D_INTERNAL_DB_FORMAT_BAD;
    }

    const char* const dflt   = _column->default_sql;
    enum d_internal_db_format_status status =
        d_internal_db_format(_sql, &quoting, "%I", _column->name);

    // the type, then the constraints
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_postgres_append_type(_db, _sql, _column);
    }

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_db_format(
            _sql,
            &quoting,
            "%s%s%s%s%s%s",
            d_internal_postgres_clause(flags,
                                       D_DB_COLUMN_NOT_NULL,
                                       " NOT NULL"),
            (dflt) ? " DEFAULT " : "",
            (dflt) ? dflt : "",
            d_internal_postgres_clause(flags,
                                       D_DB_COLUMN_AUTO_INCREMENT,
                                       " GENERATED BY DEFAULT AS IDENTITY"),
            d_internal_postgres_clause(flags, D_DB_COLUMN_UNIQUE, " UNIQUE"),
            ( (_key) &&
              ((flags & D_DB_COLUMN_PRIMARY_KEY) != 0u) ) ? " PRIMARY KEY"
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
d_internal_postgres_count_keys(
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
 * A sole key is written on its column; a composite one as a table
 * constraint after the columns.
 */
static enum d_internal_db_format_status
d_internal_postgres_build_definition(
    PGconn*                      _db,
    struct d_internal_db_buffer* _sql,
    const struct d_db_column*    _columns,
    size_t                       _count
)
{
    const struct d_internal_db_quoting quoting   =
        d_internal_postgres_quoting(_db);
    size_t                             keys      = 0u;
    const char*                        separator = ", PRIMARY KEY (";
    enum d_internal_db_format_status   status    =
        (d_internal_postgres_count_keys(_columns, _count, &keys))
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
            status = d_internal_postgres_append_column(_db,
                                                       _sql,
                                                       &_columns[i],
                                                       (keys == 1u));
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
d_internal_postgres_create_table
  CREATE TABLE once the definition is settled, in the schema named or the
current one, asked for.
*/
static int
d_internal_postgres_create_table(
    PGconn*     _db,
    const char* _schema,
    const char* _table,
    const char* _definition,
    unsigned    _flags
)
{
    const char* schema = NULL;
    char*       owned  = NULL;
    int         result = d_internal_postgres_target(_db,
                                                    _schema,
                                                    &schema,
                                                    &owned);

    // the schema, then the statement
    if (result == 0)
    {
        result = d_internal_postgres_run(
                     _db,
                     NULL,
                     "CREATE TABLE %s%T (%s)",
                     d_internal_postgres_clause(_flags,
                                                D_DB_IF_NOT_EXISTS,
                                                "IF NOT EXISTS "),
                     schema,
                     _table,
                     _definition);
    }

    free(owned);

    return result;
}

/*
d_internal_postgres_on_table
  A statement that names one object in a schema with %T, after a clause,
and before another: DROP TABLE, ALTER TABLE ... and their kin. The schema
is settled first.
*/
static int
d_internal_postgres_on_table(
    PGconn*     _db,
    const char* _schema,
    const char* _object,
    const char* _before,
    const char* _after
)
{
    const char* schema = NULL;
    char*       owned  = NULL;
    int         result = d_internal_postgres_target(_db,
                                                    _schema,
                                                    &schema,
                                                    &owned);

    // the schema, then the statement
    if (result == 0)
    {
        result = d_internal_postgres_run(_db,
                                         NULL,
                                         "%s%T%s",
                                         _before,
                                         schema,
                                         _object,
                                         _after);
    }

    free(owned);

    return result;
}

/*
d_internal_postgres_verbatim
  Whether an index's key text is the rest of the statement rather than what
goes between its parentheses: it begins, past any space, with "(" or the
keyword USING.
*/
static bool
d_internal_postgres_verbatim(
    const char* _columns
)
{
    static const char keyword[] = "using";
    const char*       at        = _columns;

    // past the spaces
    while ( (*at == ' ')  ||
            (*at == '\t') ||
            (*at == '\n') ||
            (*at == '\r') )
    {
        ++at;
    }

    if (*at == '(')
    {
        return true;
    }

    // USING in any case, as a whole word
    for (size_t i = 0u; i < sizeof(keyword) - 1u; ++i)
    {
        const char letter = ( (at[i] >= 'A') && (at[i] <= 'Z') )
                                ? (char)(at[i] - 'A' + 'a')
                                : at[i];

        if (letter != keyword[i])
        {
            return false;
        }
    }

    return ( (at[5] == ' ')  ||
             (at[5] == '\t') ||
             (at[5] == '\n') ||
             (at[5] == '\r') );
}

/**
 * @brief Reads one row of the column description into the builder.
 *
 * @param[in]     _rows     the description.
 * @param[in]     _row      which row.
 * @param[in,out] _columns  the builder.
 * @return `true`, or `false` when memory ran out or the key place is not a
 *         number.
 *
 * The row is name, type, default, NOT NULL as t or f, the 1-based place in
 * the primary key, 0 for none, and whether the column is generated, t or f.
 */
static bool
d_internal_postgres_add_column_info(
    const PGresult*               _rows,
    int                           _row,
    struct d_internal_db_columns* _columns
)
{
    int64_t                       place    = 0;
    const bool                    numbered =
        d_internal_db_parse_integer(PQgetvalue(_rows, _row, 4), &place);
    const struct d_db_column_info info     =
    {
        PQgetvalue(_rows, _row, 0),
        PQgetvalue(_rows, _row, 1),
        (PQgetisnull(_rows, _row, 2)) ? NULL : PQgetvalue(_rows, _row, 2),
        ( (place > 0) && (place < 1000) ) ? (int)place : 0,
        (PQgetvalue(_rows, _row, 3)[0] == 't'),
        (PQgetvalue(_rows, _row, 5)[0] == 't')
    };

    // the key place is a number the server wrote
    return ( (numbered) &&
             (d_internal_db_columns_add(_columns, &info)) );
}

/**
 * @brief Writes the statement that describes a table's columns.
 *
 * @param[in]     _db      the connection.
 * @param[in,out] _sql     receives the statement.
 * @param[in]     _schema  the schema; `NULL` for the current one.
 * @param[in]     _table   the table.
 * @return as d_internal_db_format.
 *
 * pg_attrdef holds a generated column's expression too, from 12; it is not a
 * default, and attgenerated, which arrived with it, tells it apart, and says
 * the column is generated, which before 12 none is. A default
 * of NULL, which the server keeps as NULL::type, reads as none, as on the
 * other layers. A key column's place is its ordinal in the primary key's
 * indkey, an int2vector, whose subscripts start at 0, so array_position
 * would read one short.
 */
static enum d_internal_db_format_status
d_internal_postgres_columns_sql(
    PGconn*                      _db,
    struct d_internal_db_buffer* _sql,
    const char*                  _schema,
    const char*                  _table
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);
    const bool                         has_generated =
        d_postgres_server_at_least(_db, 12u, 0u);
    const char* const                  generated =
        (has_generated) ? "a.attgenerated <> '' OR "
                        : "";
    const char* const                  computed  =
        (has_generated) ? "a.attgenerated <> ''"
                        : "false";
    enum d_internal_db_format_status   status  = d_internal_db_format(
        _sql,
        &quoting,
        "SELECT a.attname, pg_catalog.format_type(a.atttypid, a.atttypmod), "
        "CASE WHEN %spg_catalog.pg_get_expr(d.adbin, d.adrelid) "
        "~ '^NULL(::[^:]+)*$' THEN NULL "
        "ELSE pg_catalog.pg_get_expr(d.adbin, d.adrelid) END, "
        "a.attnotnull, COALESCE((SELECT k.n FROM pg_catalog.pg_index x, "
        "pg_catalog.unnest(x.indkey) WITH ORDINALITY AS k(attnum, n) "
        "WHERE x.indrelid = t.oid AND x.indisprimary "
        "AND k.attnum = a.attnum), 0), %s "
        "FROM pg_catalog.pg_attribute a "
        "JOIN pg_catalog.pg_class t ON t.oid = a.attrelid "
        "JOIN pg_catalog.pg_namespace n ON n.oid = t.relnamespace "
        "LEFT JOIN pg_catalog.pg_attrdef d "
        "ON d.adrelid = a.attrelid AND d.adnum = a.attnum "
        "WHERE a.attnum > 0 AND NOT a.attisdropped "
        "AND t.relkind IN " D_INTERNAL_POSTGRES_DESCRIBED " "
        "AND t.relname = %Q::pg_catalog.name",
        generated,
        computed,
        _table);

    // the schema, then the order
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_postgres_match_schema(_db, _sql, _schema);
    }

    return (status == D_INTERNAL_DB_FORMAT_OK)
               ? d_internal_db_format(_sql, &quoting, " ORDER BY a.attnum")
               : status;
}

/*
d_internal_postgres_described
  Whether a table or view with no columns exists at all, which a description
of no rows leaves open.
*/
static int
d_internal_postgres_described(
    PGconn*     _db,
    const char* _schema,
    const char* _table,
    bool*       _exists
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);
    struct d_internal_db_buffer        sql     = { NULL, 0u, 0u };
    enum d_internal_db_format_status   status  = d_internal_db_format(
        &sql,
        &quoting,
        "SELECT 1 FROM pg_catalog.pg_class t "
        "JOIN pg_catalog.pg_namespace n ON n.oid = t.relnamespace "
        "WHERE t.relkind IN " D_INTERNAL_POSTGRES_DESCRIBED " "
        "AND t.relname = %Q::pg_catalog.name",
        _table);

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_postgres_match_schema(_db, &sql, _schema);
    }

    PGresult* rows   = NULL;
    int       result = d_internal_postgres_format_result(status);

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_send(_db, sql.data, &rows);
    d_internal_db_buffer_release(&sql);
    *_exists = ( (result == 0) &&
                 (rows)        &&
                 (PQntuples(rows) > 0) );
    PQclear(rows);

    return result;
}

/*
d_internal_postgres_read_columns
  Each row into the builder, then the list; no rows is an empty list for a
table that exists, and D_DB_NOT_FOUND for one that does not.
*/
static int
d_internal_postgres_read_columns(
    PGconn*                  _db,
    PGresult*                _rows,
    const char*              _schema,
    const char*              _table,
    struct d_db_column_list* _columns
)
{
    struct d_internal_db_columns builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u
    };
    const int count  = (_rows) ? PQntuples(_rows) : 0;
    bool      ok     = true;
    bool      exists = true;

    // each column, until one cannot be added
    for (int i = 0; (ok) && (i < count); ++i)
    {
        ok = d_internal_postgres_add_column_info(_rows, i, &builder);
    }

    PQclear(_rows);

    if (!ok)
    {
        d_internal_db_columns_release(&builder);

        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    // no columns: a table without any, or none at all
    if (count == 0)
    {
        const int result =
            d_internal_postgres_described(_db, _schema, _table, &exists);

        if ( (result != 0) ||
             (!exists) )
        {
            d_internal_db_columns_release(&builder);

            return (result != 0) ? result : D_DB_NOT_FOUND;
        }
    }

    return (d_internal_db_columns_finish(&builder, _columns))
               ? 0
               : D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
}

/*
d_internal_postgres_relation_sql
  A SELECT of one relation of a kind in a schema, as a lookup writes it, with
what to select written by the caller: nextval(c.oid), say.
*/
static enum d_internal_db_format_status
d_internal_postgres_relation_sql(
    PGconn*                      _db,
    struct d_internal_db_buffer* _sql,
    const char*                  _select,
    const char*                  _schema,
    const char*                  _name
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);
    enum d_internal_db_format_status   status  = d_internal_db_format(
        _sql,
        &quoting,
        "SELECT %s FROM pg_catalog.pg_class c "
        "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
        "WHERE c.relname = %Q::pg_catalog.name",
        _select,
        _name);

    return (status == D_INTERNAL_DB_FORMAT_OK)
               ? d_internal_postgres_match_schema(_db, _sql, _schema)
               : status;
}

//==============================================================================
// 2.  TABLES
//==============================================================================

// 2.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_table
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_table(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table
)
{
    bool exists = false;

    return ( (d_postgres_has_table_s(_db, _schema, _table, &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_table_s
  A row of pg_class, an ordinary or partitioned table, in the schema.
*/
int
d_postgres_has_table_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { _schema, NULL, _table };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_TABLE,
                                   &path,
                                   _exists);
}

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_table
  The column set is checked and written first, wholly the layer's work, so a
column it refuses never reaches the server. Six parameters, one past the
guide's tripwire: they are the statement's own parts, and the array's
length rides with it, as C passes every array.
*/
int
d_postgres_create_table(
    struct pg_conn*           _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _columns,
    size_t                    _count,
    unsigned                  _flags
)
{
    struct d_internal_db_buffer definition = { NULL, 0u, 0u };

    // parameter validation
    if ( (!_db)                                        ||
         (!d_internal_postgres_schema_ok(_schema))     ||
         (!d_internal_postgres_name_ok(_table))        ||
         (!_columns)                                   ||
         (_count == 0u)                                ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_format_result(
        d_internal_postgres_build_definition(_db,
                                             &definition,
                                             _columns,
                                             _count));

    // the definition is released whether it was built or not
    if (result == 0)
    {
        result = d_internal_postgres_create_table(_db,
                                                  _schema,
                                                  _table,
                                                  definition.data,
                                                  _flags);
    }

    d_internal_db_buffer_release(&definition);

    return result;
}

/*
d_postgres_create_table_sql
  The caller's definition as written.
*/
int
d_postgres_create_table_sql(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _definition,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!_definition)                            ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_create_table(_db,
                                            _schema,
                                            _table,
                                            _definition,
                                            _flags);
}

/*
d_postgres_drop_table
  The server's own DROP TABLE [IF EXISTS] ... [CASCADE].
*/
int
d_postgres_drop_table(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                                  ||
         (!d_internal_postgres_schema_ok(_schema))               ||
         (!d_internal_postgres_name_ok(_table))                  ||
         ((_flags & ~(D_DB_IF_EXISTS | D_POSTGRES_CASCADE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_on_table(
               _db,
               _schema,
               _table,
               ((_flags & D_DB_IF_EXISTS) != 0u) ? "DROP TABLE IF EXISTS "
                                                 : "DROP TABLE ",
               d_internal_postgres_clause(_flags, D_POSTGRES_CASCADE,
                                          " CASCADE"));
}

// 2.3    Changing
//------------------------------------------------------------------------------
/*
d_postgres_rename_table
  RENAME TO takes an unqualified name: a table cannot move between schemas
by renaming.
*/
int
d_postgres_rename_table(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _new_name
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!d_internal_postgres_name_ok(_new_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the statement
    if (result == 0)
    {
        result = d_internal_postgres_run(_db,
                                         NULL,
                                         "ALTER TABLE %T RENAME TO %I",
                                         schema,
                                         _table,
                                         _new_name);
    }

    free(owned);

    return result;
}

// 2.4    Rows
//------------------------------------------------------------------------------
/*
d_postgres_truncate_table
  RESTART IDENTITY restarts the sequences the table's columns own, as the
other layers' truncation restarts their counters.
*/
int
d_postgres_truncate_table(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table
)
{
    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_on_table(_db,
                                        _schema,
                                        _table,
                                        "TRUNCATE TABLE ",
                                        " RESTART IDENTITY");
}

/*
d_postgres_count_rows
  count(*), a bigint, read exactly.
*/
int
d_postgres_count_rows(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    int64_t*        _count
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!_count) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the count
    if (result == 0)
    {
        result = d_internal_postgres_query_int64(
                     _db,
                     _count,
                     "SELECT pg_catalog.count(*) FROM %T",
                     schema,
                     _table);
    }

    free(owned);

    return result;
}

// 2.5    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_tables
  pg_class's ordinary and partitioned tables in the schema, in byte order.
*/
int
d_postgres_list_tables(
    struct pg_conn*        _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { _schema, NULL, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_TABLE,
                                    &path,
                                    _names);
}

//==============================================================================
// 3.  COLUMNS
//==============================================================================

// 3.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_column
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_column(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column
)
{
    bool exists = false;

    return ( (d_postgres_has_column_s(_db,
                                      _schema,
                                      _table,
                                      _column,
                                      &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_column_s
  A row of pg_attribute, the system's columns and dropped ones aside.
*/
int
d_postgres_has_column_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { _schema, _table, _column };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_COLUMN,
                                   &path,
                                   _exists);
}

// 3.2    Adding, renaming and dropping
//------------------------------------------------------------------------------
/*
d_postgres_add_column
  The column is written as create_table writes one, its own PRIMARY KEY
flag on it, then appended as SQL.
*/
int
d_postgres_add_column(
    struct pg_conn*           _db,
    const char*               _schema,
    const char*               _table,
    const struct d_db_column* _column
)
{
    struct d_internal_db_buffer definition = { NULL, 0u, 0u };
    size_t                      keys       = 0u;

    // parameter validation
    if ( (!_db)                                          ||
         (!d_internal_postgres_schema_ok(_schema))       ||
         (!d_internal_postgres_name_ok(_table))          ||
         (!_column)                                      ||
         (!d_internal_postgres_count_keys(_column, 1u, &keys)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_format_result(
        d_internal_postgres_append_column(_db, &definition, _column, true));

    // the definition is released whether it was built or not
    if (result == 0)
    {
        result = d_postgres_add_column_sql(_db,
                                           _schema,
                                           _table,
                                           definition.data);
    }

    d_internal_db_buffer_release(&definition);

    return result;
}

/*
d_postgres_add_column_sql
  ALTER TABLE ... ADD COLUMN, the caller's definition as written.
*/
int
d_postgres_add_column_sql(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _definition
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!_definition) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the statement
    if (result == 0)
    {
        result = d_internal_postgres_run(_db,
                                         NULL,
                                         "ALTER TABLE %T ADD COLUMN %s",
                                         schema,
                                         _table,
                                         _definition);
    }

    free(owned);

    return result;
}

/*
d_postgres_rename_column
  RENAME COLUMN, which every supported server has.
*/
int
d_postgres_rename_column(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column,
    const char*     _new_name
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!d_internal_postgres_name_ok(_column))   ||
         (!d_internal_postgres_name_ok(_new_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the statement
    if (result == 0)
    {
        result = d_internal_postgres_run(_db,
                                         NULL,
                                         "ALTER TABLE %T RENAME COLUMN %I "
                                         "TO %I",
                                         schema,
                                         _table,
                                         _column,
                                         _new_name);
    }

    free(owned);

    return result;
}

/*
d_postgres_drop_column
  DROP COLUMN, which the server allows on a table's last column too.
*/
int
d_postgres_drop_column(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    const char*     _column
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!d_internal_postgres_name_ok(_column)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the statement
    if (result == 0)
    {
        result = d_internal_postgres_run(_db,
                                         NULL,
                                         "ALTER TABLE %T DROP COLUMN %I",
                                         schema,
                                         _table,
                                         _column);
    }

    free(owned);

    return result;
}

// 3.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_columns
  One statement describes every column; d_internal_postgres_read_columns
tells an empty table from a missing one.
*/
int
d_postgres_list_columns(
    struct pg_conn*          _db,
    const char*              _schema,
    const char*              _table,
    struct d_db_column_list* _columns
)
{
    struct d_internal_db_buffer sql  = { NULL, 0u, 0u };
    PGresult*                   rows = NULL;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!_columns) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_format_result(
        d_internal_postgres_columns_sql(_db, &sql, _schema, _table));

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_send(_db, sql.data, &rows);
    d_internal_db_buffer_release(&sql);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_postgres_read_columns(_db,
                                            rows,
                                            _schema,
                                            _table,
                                            _columns);
}

//==============================================================================
// 4.  INDEXES
//==============================================================================

// 4.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_index
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_index(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _index
)
{
    bool exists = false;

    return ( (d_postgres_has_index_s(_db, _schema, _index, &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_index_s
  A row of pg_class, an index, ordinary or partitioned, in the schema.
*/
int
d_postgres_has_index_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _index,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { _schema, NULL, _index };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_INDEX,
                                   &path,
                                   _exists);
}

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_index
  The server's own CREATE [UNIQUE] INDEX [CONCURRENTLY] [IF NOT EXISTS]. The
key goes in parentheses, unless it is the rest of the statement. Six
parameters, one past the guide's tripwire: each is a part of the statement.
*/
int
d_postgres_create_index(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _index,
    const char*     _table,
    const char*     _columns,
    unsigned        _flags
)
{
    const unsigned allowed = D_DB_UNIQUE | D_DB_IF_NOT_EXISTS |
                             D_POSTGRES_CONCURRENTLY;
    const char*    schema  = NULL;
    char*          owned   = NULL;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_index))    ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!_columns)                               ||
         ((_flags & ~allowed) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the statement
    if (result == 0)
    {
        const bool verbatim = d_internal_postgres_verbatim(_columns);

        result = d_internal_postgres_run(
            _db,
            NULL,
            "CREATE %sINDEX %s%s%I ON %T %s%s%s",
            d_internal_postgres_clause(_flags, D_DB_UNIQUE, "UNIQUE "),
            d_internal_postgres_clause(_flags,
                                       D_POSTGRES_CONCURRENTLY,
                                       "CONCURRENTLY "),
            d_internal_postgres_clause(_flags,
                                       D_DB_IF_NOT_EXISTS,
                                       "IF NOT EXISTS "),
            _index,
            schema,
            _table,
            (verbatim) ? "" : "(",
            _columns,
            (verbatim) ? "" : ")");
    }

    free(owned);

    return result;
}

/*
d_postgres_drop_index
  The server's own DROP INDEX [CONCURRENTLY] [IF EXISTS] ... [CASCADE].
*/
int
d_postgres_drop_index(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _index,
    unsigned        _flags
)
{
    const unsigned allowed = D_DB_IF_EXISTS | D_POSTGRES_CONCURRENTLY |
                             D_POSTGRES_CASCADE;

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_index))    ||
         ((_flags & ~allowed) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const char* const before =
        ((_flags & D_POSTGRES_CONCURRENTLY) != 0u)
            ? ( ((_flags & D_DB_IF_EXISTS) != 0u)
                    ? "DROP INDEX CONCURRENTLY IF EXISTS "
                    : "DROP INDEX CONCURRENTLY " )
            : ( ((_flags & D_DB_IF_EXISTS) != 0u) ? "DROP INDEX IF EXISTS "
                                                  : "DROP INDEX " );

    return d_internal_postgres_on_table(
               _db,
               _schema,
               _index,
               before,
               d_internal_postgres_clause(_flags, D_POSTGRES_CASCADE,
                                          " CASCADE"));
}

// 4.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_indexes
  pg_class's indexes in the schema, or on one table, those a constraint owns
left out, in byte order.
*/
int
d_postgres_list_indexes(
    struct pg_conn*        _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { _schema, _table, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_INDEX,
                                    &path,
                                    _names);
}

//==============================================================================
// 5.  VIEWS
//==============================================================================

// 5.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_view
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_view(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _view
)
{
    bool exists = false;

    return ( (d_postgres_has_view_s(_db, _schema, _view, &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_view_s
  A row of pg_class, a view, in the schema.
*/
int
d_postgres_has_view_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _view,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { _schema, NULL, _view };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_VIEW,
                                   &path,
                                   _exists);
}

// 5.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_view
  CREATE VIEW has no IF NOT EXISTS; the layer reads 42P07, a relation of the
name, as success. OR REPLACE is the server's own.
*/
int
d_postgres_create_view(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _view,
    const char*     _select,
    unsigned        _flags
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // parameter validation
    if ( (!_db)                                                       ||
         (!d_internal_postgres_schema_ok(_schema))                    ||
         (!d_internal_postgres_name_ok(_view))                        ||
         (!_select)                                                   ||
         ((_flags & ~(D_DB_IF_NOT_EXISTS | D_POSTGRES_REPLACE)) != 0u) ||
         (_flags == (D_DB_IF_NOT_EXISTS | D_POSTGRES_REPLACE)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the statement, settled or the server's own
    if (result == 0)
    {
        result = ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
                     ? d_internal_postgres_tolerate(
                           _db,
                           D_POSTGRES_SQLSTATE_DUPLICATE_TABLE,
                           "CREATE VIEW %T AS %s",
                           schema,
                           _view,
                           _select)
                     : d_internal_postgres_run(
                           _db,
                           NULL,
                           "CREATE %sVIEW %T AS %s",
                           d_internal_postgres_clause(_flags,
                                                      D_POSTGRES_REPLACE,
                                                      "OR REPLACE "),
                           schema,
                           _view,
                           _select);
    }

    free(owned);

    return result;
}

/*
d_postgres_drop_view
  The server's own DROP VIEW [IF EXISTS] ... [CASCADE].
*/
int
d_postgres_drop_view(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _view,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                                  ||
         (!d_internal_postgres_schema_ok(_schema))               ||
         (!d_internal_postgres_name_ok(_view))                   ||
         ((_flags & ~(D_DB_IF_EXISTS | D_POSTGRES_CASCADE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_on_table(
               _db,
               _schema,
               _view,
               ((_flags & D_DB_IF_EXISTS) != 0u) ? "DROP VIEW IF EXISTS "
                                                 : "DROP VIEW ",
               d_internal_postgres_clause(_flags, D_POSTGRES_CASCADE,
                                          " CASCADE"));
}

// 5.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_views
  pg_class's views in the schema, in byte order.
*/
int
d_postgres_list_views(
    struct pg_conn*        _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { _schema, NULL, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_VIEW,
                                    &path,
                                    _names);
}

//==============================================================================
// 6.  TRIGGERS
//==============================================================================

// 6.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_trigger
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_trigger(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _trigger,
    const char*     _table
)
{
    bool exists = false;

    return ( (d_postgres_has_trigger_s(_db,
                                       _schema,
                                       _trigger,
                                       _table,
                                       &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_trigger_s
  A row of pg_trigger on the table, PostgreSQL's own triggers aside.
*/
int
d_postgres_has_trigger_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _trigger,
    const char*     _table,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { _schema, _table, _trigger };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_TRIGGER,
                                   &path,
                                   _exists);
}

// 6.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_trigger
  CREATE TRIGGER has no IF NOT EXISTS; the layer reads 42710, a trigger of
the name on that table, as success. OR REPLACE is the server's own, from 14.
*/
int
d_postgres_create_trigger(
    struct pg_conn* _db,
    const char*     _trigger,
    const char*     _definition,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                                       ||
         (!d_internal_postgres_name_ok(_trigger))                     ||
         (!_definition)                                               ||
         ((_flags & ~(D_DB_IF_NOT_EXISTS | D_POSTGRES_REPLACE)) != 0u) ||
         (_flags == (D_DB_IF_NOT_EXISTS | D_POSTGRES_REPLACE)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // OR REPLACE arrived with 14
    if ( ((_flags & D_POSTGRES_REPLACE) != 0u) &&
         (!d_postgres_supports(_db, D_POSTGRES_FEATURE_REPLACE_TRIGGER)) )
    {
        return D_DB_UNSUPPORTED;
    }

    return ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
               ? d_internal_postgres_tolerate(
                     _db,
                     D_POSTGRES_SQLSTATE_DUPLICATE_OBJECT,
                     "CREATE TRIGGER %I %s",
                     _trigger,
                     _definition)
               : d_internal_postgres_run(
                     _db,
                     NULL,
                     "CREATE %sTRIGGER %I %s",
                     d_internal_postgres_clause(_flags,
                                                D_POSTGRES_REPLACE,
                                                "OR REPLACE "),
                     _trigger,
                     _definition);
}

/*
d_postgres_drop_trigger
  The server's own DROP TRIGGER [IF EXISTS] ... ON ... [CASCADE].
*/
int
d_postgres_drop_trigger(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _trigger,
    const char*     _table,
    unsigned        _flags
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // parameter validation
    if ( (!_db)                                                  ||
         (!d_internal_postgres_schema_ok(_schema))               ||
         (!d_internal_postgres_name_ok(_trigger))                ||
         (!d_internal_postgres_name_ok(_table))                  ||
         ((_flags & ~(D_DB_IF_EXISTS | D_POSTGRES_CASCADE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the statement
    if (result == 0)
    {
        result = d_internal_postgres_run(
                     _db,
                     NULL,
                     "DROP TRIGGER %s%I ON %T%s",
                     d_internal_postgres_clause(_flags,
                                                D_DB_IF_EXISTS,
                                                "IF EXISTS "),
                     _trigger,
                     schema,
                     _table,
                     d_internal_postgres_clause(_flags,
                                                D_POSTGRES_CASCADE,
                                                " CASCADE"));
    }

    free(owned);

    return result;
}

// 6.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_triggers
  pg_trigger's rows on the schema's tables, or one table's, PostgreSQL's own
aside, in byte order.
*/
int
d_postgres_list_triggers(
    struct pg_conn*        _db,
    const char*            _schema,
    const char*            _table,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { _schema, _table, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_TRIGGER,
                                    &path,
                                    _names);
}

//==============================================================================
// 7.  SEQUENCES
//==============================================================================

// 7.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_sequence
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_sequence(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence
)
{
    bool exists = false;

    return ( (d_postgres_has_sequence_s(_db,
                                        _schema,
                                        _sequence,
                                        &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_sequence_s
  A row of pg_class, a sequence, in the schema.
*/
int
d_postgres_has_sequence_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { _schema, NULL, _sequence };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_SEQUENCE,
                                   &path,
                                   _exists);
}

// 7.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_sequence
  The server's own CREATE SEQUENCE [IF NOT EXISTS].
*/
int
d_postgres_create_sequence(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_sequence)) ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_on_table(
               _db,
               _schema,
               _sequence,
               ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
                   ? "CREATE SEQUENCE IF NOT EXISTS "
                   : "CREATE SEQUENCE ",
               "");
}

/*
d_postgres_drop_sequence
  The server's own DROP SEQUENCE [IF EXISTS] ... [CASCADE].
*/
int
d_postgres_drop_sequence(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                                  ||
         (!d_internal_postgres_schema_ok(_schema))               ||
         (!d_internal_postgres_name_ok(_sequence))               ||
         ((_flags & ~(D_DB_IF_EXISTS | D_POSTGRES_CASCADE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_on_table(
               _db,
               _schema,
               _sequence,
               ((_flags & D_DB_IF_EXISTS) != 0u) ? "DROP SEQUENCE IF EXISTS "
                                                 : "DROP SEQUENCE ",
               d_internal_postgres_clause(_flags, D_POSTGRES_CASCADE,
                                          " CASCADE"));
}

// 7.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_sequences
  pg_class's sequences in the schema, in byte order.
*/
int
d_postgres_list_sequences(
    struct pg_conn*        _db,
    const char*            _schema,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { _schema, NULL, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_SEQUENCE,
                                    &path,
                                    _names);
}

// 7.4    Values
//------------------------------------------------------------------------------
/*
d_postgres_next_value
  nextval() on the sequence's oid, found as a lookup finds it, so that a
missing sequence is no row -- D_DB_NOT_FOUND -- rather than a failure that
would leave a transaction failed.
*/
int
d_postgres_next_value(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    int64_t*        _value
)
{
    struct d_internal_db_buffer sql = { NULL, 0u, 0u };

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_sequence)) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_format_result(
        d_internal_postgres_relation_sql(_db,
                                         &sql,
                                         "pg_catalog.nextval(c.oid)",
                                         _schema,
                                         _sequence));

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_query_int64(_db,
                                             _value,
                                             "%s AND c.relkind = 'S'",
                                             sql.data);
    d_internal_db_buffer_release(&sql);

    return result;
}

/*
d_postgres_last_value
  lastval() fails, 55000, before the session's first nextval(); inside a
transaction that failure would leave it failed, so the question is asked
under a savepoint there, rolled back to for that failure. A failed
transaction refuses everything, the savepoint included, so it is answered
at once with the code the server would send.
*/
int
d_postgres_last_value(
    struct pg_conn* _db,
    int64_t*        _value
)
{
    static const int undefined = D_POSTGRES_SQLSTATE('5', '5', '0', '0', '0');
    int64_t          value     = 0;

    // parameter validation
    if ( (!_db) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const PGTransactionStatusType state   = PQtransactionStatus(_db);
    const bool                    guarded = (state == PQTRANS_INTRANS);

    // nothing runs in a failed transaction
    if (state == PQTRANS_INERROR)
    {
        return D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION;
    }

    int result = (guarded)
                     ? d_internal_postgres_send(
                           _db,
                           "SAVEPOINT d_postgres_last_value",
                           NULL)
                     : 0;

    if (result == 0)
    {
        result = d_internal_postgres_query_int64(_db,
                                                 &value,
                                                 "SELECT pg_catalog.lastval()");
    }

    // the savepoint settled: undone for the one failure, released otherwise
    if (guarded)
    {
        if (result == undefined)
        {
            (void)d_internal_postgres_send(
                _db,
                "ROLLBACK TO SAVEPOINT d_postgres_last_value; "
                "RELEASE SAVEPOINT d_postgres_last_value",
                NULL);
        }
        else if (result == 0)
        {
            result = d_internal_postgres_send(
                         _db,
                         "RELEASE SAVEPOINT d_postgres_last_value",
                         NULL);
        }
    }

    // none yet in the session
    if (result == undefined)
    {
        return D_DB_NOT_FOUND;
    }

    // written only on success
    if (result == 0)
    {
        *_value = value;
    }

    return result;
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_has_table
  Without libpq, false.
*/
bool
d_postgres_has_table(
    struct pg_conn* _db,
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
d_postgres_has_table_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_table_s(
    struct pg_conn* _db,
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
d_postgres_create_table
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_table(
    struct pg_conn*           _db,
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
d_postgres_create_table_sql
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_table_sql(
    struct pg_conn* _db,
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
d_postgres_drop_table
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_table(
    struct pg_conn* _db,
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
d_postgres_rename_table
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_rename_table(
    struct pg_conn* _db,
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
d_postgres_truncate_table
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_truncate_table(
    struct pg_conn* _db,
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
d_postgres_count_rows
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_count_rows(
    struct pg_conn* _db,
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
d_postgres_list_tables
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_tables(
    struct pg_conn*        _db,
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
d_postgres_has_column
  Without libpq, false.
*/
bool
d_postgres_has_column(
    struct pg_conn* _db,
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
d_postgres_has_column_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_column_s(
    struct pg_conn* _db,
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
d_postgres_add_column
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_add_column(
    struct pg_conn*           _db,
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
d_postgres_add_column_sql
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_add_column_sql(
    struct pg_conn* _db,
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
d_postgres_rename_column
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_rename_column(
    struct pg_conn* _db,
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
d_postgres_drop_column
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_column(
    struct pg_conn* _db,
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
d_postgres_list_columns
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_columns(
    struct pg_conn*          _db,
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
d_postgres_has_index
  Without libpq, false.
*/
bool
d_postgres_has_index(
    struct pg_conn* _db,
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
d_postgres_has_index_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_index_s(
    struct pg_conn* _db,
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
d_postgres_create_index
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_index(
    struct pg_conn* _db,
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
d_postgres_drop_index
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_index(
    struct pg_conn* _db,
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
d_postgres_list_indexes
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_indexes(
    struct pg_conn*        _db,
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
d_postgres_has_view
  Without libpq, false.
*/
bool
d_postgres_has_view(
    struct pg_conn* _db,
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
d_postgres_has_view_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_view_s(
    struct pg_conn* _db,
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
d_postgres_create_view
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_view(
    struct pg_conn* _db,
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
d_postgres_drop_view
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_view(
    struct pg_conn* _db,
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
d_postgres_list_views
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_views(
    struct pg_conn*        _db,
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
d_postgres_has_trigger
  Without libpq, false.
*/
bool
d_postgres_has_trigger(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _trigger,
    const char*     _table
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;
    (void)_table;

    return false;
}

/*
d_postgres_has_trigger_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_trigger_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _trigger,
    const char*     _table,
    bool*           _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;
    (void)_table;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_create_trigger
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_trigger(
    struct pg_conn* _db,
    const char*     _trigger,
    const char*     _definition,
    unsigned        _flags
)
{
    (void)_db;
    (void)_trigger;
    (void)_definition;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_drop_trigger
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_trigger(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _trigger,
    const char*     _table,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_trigger;
    (void)_table;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_list_triggers
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_triggers(
    struct pg_conn*        _db,
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
d_postgres_has_sequence
  Without libpq, false.
*/
bool
d_postgres_has_sequence(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence
)
{
    (void)_db;
    (void)_schema;
    (void)_sequence;

    return false;
}

/*
d_postgres_has_sequence_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_sequence_s(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    bool*           _exists
)
{
    (void)_db;
    (void)_schema;
    (void)_sequence;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_create_sequence
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_sequence(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_sequence;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_drop_sequence
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_sequence(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_sequence;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_list_sequences
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_sequences(
    struct pg_conn*        _db,
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
d_postgres_next_value
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_next_value(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _sequence,
    int64_t*        _value
)
{
    (void)_db;
    (void)_schema;
    (void)_sequence;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_last_value
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_last_value(
    struct pg_conn* _db,
    int64_t*        _value
)
{
    (void)_db;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_POSTGRES

#endif  // defined(INT64_MAX)
