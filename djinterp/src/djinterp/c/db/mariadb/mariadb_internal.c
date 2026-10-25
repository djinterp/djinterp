/*******************************************************************************
* djinterp [c]                                                mariadb_internal.c
*
* Implementation of the plumbing mariadb_internal.h declares for the MariaDB
* layer's modules: server facts, name checks and quoting, statements run and
* read, and catalogue lookups.
*   Every statement goes through d_internal_mariadb_send, which reads every
* result the statement produces -- the protocol takes no new statement until
* the last one's results are read -- and returns the first failure's error
* number with nothing run after it.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_internal.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.22
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "./mariadb_internal.h"  // corresponding header
// std
#include <limits.h>  // ULONG_MAX
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strchr, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, the lists
#include "../db_internal.h"  // buffers, builders, formatting, numbers
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // SIZE_MAX, int64_t


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_mariadb_case
//   enum: whether a server matches a kind of name by case.
enum d_internal_mariadb_case
{
    D_INTERNAL_MARIADB_CASE_NEVER = 0,  // never: columns, indexes
    D_INTERNAL_MARIADB_CASE_TABLE,      // as lower_case_table_names says
    D_INTERNAL_MARIADB_CASE_TRIGGER     // so too, but never on MySQL 8, whose
                                        // data dictionary ignores case
};

// d_internal_mariadb_kind
//   struct: where information_schema keeps one kind of object.
struct d_internal_mariadb_kind
{
    const char*                  source;     // the information_schema table
    const char*                  schema;     // its database column, or NULL
                                             // where the object is one
    const char*                  table;      // its owning-table column, or
                                             // NULL
    const char*                  name;       // its name column
    const char*                  condition;  // a further condition, or NULL
    const char*                  listing;    // one more when listing, or NULL
    enum d_internal_mariadb_case name_case;  // how the server matches names
};

// d_internal_mariadb_kinds
//   table: each d_internal_mariadb_object's kind, in the enum's order. A
// table is a base table, which MariaDB's system-versioned tables are too;
// STATISTICS has a row per key part, and its first stands for the index.
static const struct d_internal_mariadb_kind d_internal_mariadb_kinds[] =
{
    { "SCHEMATA", NULL, NULL, "SCHEMA_NAME", NULL, NULL,
      D_INTERNAL_MARIADB_CASE_TABLE },
    { "TABLES", "TABLE_SCHEMA", NULL, "TABLE_NAME",
      "TABLE_TYPE IN ('BASE TABLE', 'SYSTEM VERSIONED')", NULL,
      D_INTERNAL_MARIADB_CASE_TABLE },
    { "TABLES", "TABLE_SCHEMA", NULL, "TABLE_NAME",
      "TABLE_TYPE = 'VIEW'", NULL,
      D_INTERNAL_MARIADB_CASE_TABLE },
    { "TABLES", "TABLE_SCHEMA", NULL, "TABLE_NAME",
      "TABLE_TYPE IN ('BASE TABLE', 'SYSTEM VERSIONED', 'VIEW')", NULL,
      D_INTERNAL_MARIADB_CASE_TABLE },
    { "COLUMNS", "TABLE_SCHEMA", "TABLE_NAME", "COLUMN_NAME", NULL, NULL,
      D_INTERNAL_MARIADB_CASE_NEVER },
    { "STATISTICS", "TABLE_SCHEMA", "TABLE_NAME", "INDEX_NAME",
      "SEQ_IN_INDEX = 1", "INDEX_NAME <> 'PRIMARY'",
      D_INTERNAL_MARIADB_CASE_NEVER },
    { "TRIGGERS", "TRIGGER_SCHEMA", "EVENT_OBJECT_TABLE", "TRIGGER_NAME",
      NULL, NULL,
      D_INTERNAL_MARIADB_CASE_TRIGGER }
};

D_STATIC_ASSERT( (sizeof(d_internal_mariadb_kinds) /
                  sizeof(d_internal_mariadb_kinds[0])) ==
                 ((size_t)D_INTERNAL_MARIADB_TRIGGER + 1u),
                 "every d_internal_mariadb_object has a kind" );

#if (D_INTERNAL_MARIADB_ESCAPE_QUOTE == 1)

/*
d_internal_mariadb_escape
  libmysqlclient's escaping for a value between single quotes, which under
NO_BACKSLASH_ESCAPES doubles them where the plain call would fail.
*/
static unsigned long
d_internal_mariadb_escape(
    MYSQL*        _native,
    char*         _to,
    const char*   _from,
    unsigned long _length
)
{
    return mysql_real_escape_string_quote(_native, _to, _from, _length, '\'');
}

#else

/*
d_internal_mariadb_escape
  The plain call, which MariaDB's clients adapt to NO_BACKSLASH_ESCAPES by
doubling quotes themselves.
*/
static unsigned long
d_internal_mariadb_escape(
    MYSQL*        _native,
    char*         _to,
    const char*   _from,
    unsigned long _length
)
{
    return mysql_real_escape_string(_native, _to, _from, _length);
}

#endif  // D_INTERNAL_MARIADB_ESCAPE_QUOTE

/*
d_internal_mariadb_ascii_letter
  A letter of the ASCII alphabet, tested by value, which isalpha is not
guaranteed to do in every locale.
*/
static bool
d_internal_mariadb_ascii_letter(
    char _character
)
{
    return ( ( (_character >= 'a') && (_character <= 'z') ) ||
             ( (_character >= 'A') && (_character <= 'Z') ) );
}

/**
 * @brief Reads every result a statement produced, keeping the first's rows
 *        when asked to.
 *
 * @param[in]  _native  the client's connection.
 * @param[out] _rows    receives the first result's rows, or NULL; `NULL` to
 *                      discard them. Released again if a later result fails.
 * @return 0, or the client's error number for the result that failed.
 *
 * mysql_store_result answers NULL both for a statement with no rows and for a
 * failure to read them; the field count tells which, and the error number
 * too, since a failure that closes the connection -- a row past the largest
 * packet the client takes -- leaves the field count 0. mysql_next_result
 * answers 0 for a further result and more for a failure; it is called only
 * while mysql_more_results says one follows, since both clients reset the
 * affected-row count on every call, even one past the last result.
 */
static int
d_internal_mariadb_read_results(
    MYSQL*      _native,
    MYSQL_RES** _rows
)
{
    MYSQL_RES* kept  = NULL;
    bool       first = true;
    int        more  = 0;

    // each result, until none remain or one fails
    do
    {
        MYSQL_RES* const rows = mysql_store_result(_native);

        // rows that should have come and did not
        if ( (!rows) &&
             ( (mysql_field_count(_native) != 0u) ||
               (mysql_errno(_native) != 0u) ) )
        {
            more = 1;

            break;
        }

        // the first result's rows are the caller's, when asked for
        if ( (first) &&
             (_rows) )
        {
            kept = rows;
        }
        else
        {
            mysql_free_result(rows);
        }

        first = false;
        more  = (mysql_more_results(_native)) ? mysql_next_result(_native)
                                              : -1;
    } while (more == 0);

    // a failure keeps nothing
    if (more > 0)
    {
        mysql_free_result(kept);

        return (int)mysql_errno(_native);
    }

    if (_rows)
    {
        *_rows = kept;
    }

    return 0;
}

/**
 * @brief Copies the first column of the first row of a result, and frees the
 *        result.
 *
 * @param[in]  _rows   the result; may be `NULL`, for a statement without one.
 * @param[out] _value  receives the copy, `NULL` for a NULL; written only on
 *                     success.
 * @return 0, D_DB_NOT_FOUND for no row, or D_INTERNAL_MARIADB_OUT_OF_MEMORY.
 *
 * The length comes from mysql_fetch_lengths, so a value holding NUL bytes is
 * copied whole, though C reads it only to the first.
 */
static int
d_internal_mariadb_first_value(
    MYSQL_RES* _rows,
    char**     _value
)
{
    const MYSQL_ROW row = ( (_rows) &&
                            (mysql_num_fields(_rows) > 0u) )
                              ? mysql_fetch_row(_rows)
                              : NULL;

    // no result, or no row in it
    if (!row)
    {
        mysql_free_result(_rows);

        return D_DB_NOT_FOUND;
    }

    const unsigned long* const lengths = mysql_fetch_lengths(_rows);
    char*                      copy    = NULL;

    // a value is copied with a terminator; a NULL stays NULL
    if (row[0])
    {
        copy = (char*)malloc((size_t)lengths[0] + 1u);

        if (!copy)
        {
            mysql_free_result(_rows);

            return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
        }

        memcpy(copy, row[0], (size_t)lengths[0]);
        copy[lengths[0]] = '\0';
    }

    mysql_free_result(_rows);
    *_value = copy;

    return 0;
}

/*
d_internal_mariadb_read_int64
  An integer is read exactly; a number with a fraction, or one written with
an exponent, is truncated toward zero, as C converts one, when it fits. Both
bounds are powers of two, which a double holds exactly.
*/
static int
d_internal_mariadb_read_int64(
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
d_internal_mariadb_collect
  The first column of each row, a NULL skipped, into one list; the result is
freed whatever happens. A stored result is in memory, so fetching cannot
fail part way.
*/
static int
d_internal_mariadb_collect(
    MYSQL_RES*             _rows,
    struct d_db_name_list* _names
)
{
    struct d_internal_db_names builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u
    };
    bool ok = true;

    // each name, until one cannot be added
    for (MYSQL_ROW row = (_rows) ? mysql_fetch_row(_rows) : NULL;
         (ok) && (row);
         row = mysql_fetch_row(_rows))
    {
        ok = ( (!row[0]) ||
               (d_internal_db_names_add(&builder, row[0])) );
    }

    mysql_free_result(_rows);

    // finishing releases the builder whether it succeeds or not
    if (!ok)
    {
        d_internal_db_names_release(&builder);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    return (d_internal_db_names_finish(&builder, _names))
               ? 0
               : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
}

/*
d_internal_mariadb_exact
  Whether a name of the given case kind must match by its bytes: only where
lower_case_table_names is 0, and for a trigger only off MySQL 8.
*/
static bool
d_internal_mariadb_exact(
    const struct d_mariadb*      _db,
    enum d_internal_mariadb_case _case,
    int                          _rule
)
{
    return ( (_rule == 0) &&
             ( (_case == D_INTERNAL_MARIADB_CASE_TABLE) ||
               ( (_case == D_INTERNAL_MARIADB_CASE_TRIGGER) &&
                 (!d_internal_mariadb_at_least(_db,
                                               D_MARIADB_FLAVOR_MYSQL,
                                               80000ul)) ) ) );
}

/**
 * @brief Checks a lookup's arguments.
 *
 * @param[in] _object  what is looked for.
 * @param[in] _path    where.
 * @param[in] _named   whether the path must name the object.
 * @return `true` when they can be right.
 */
static bool
d_internal_mariadb_path_ok(
    enum d_internal_mariadb_object        _object,
    const struct d_internal_mariadb_path* _path,
    bool                                  _named
)
{
    const bool needs_table = ( (_object == D_INTERNAL_MARIADB_COLUMN) ||
                               (_object == D_INTERNAL_MARIADB_INDEX) );
    const bool takes_table = ( (needs_table) ||
                               (_object == D_INTERNAL_MARIADB_TRIGGER) );

    // a kind there is, and a path to look along
    if ( ((unsigned)_object > (unsigned)D_INTERNAL_MARIADB_TRIGGER) ||
         (!_path) )
    {
        return false;
    }

    return ( (d_internal_mariadb_schema_ok(_path->schema))           &&
             ( (!_named) ||
               (d_internal_mariadb_name_ok(_path->name)) )           &&
             ( (_path->table)
                   ? ( (takes_table) &&
                       (d_internal_mariadb_name_ok(_path->table)) )
                   : (!needs_table) ) );
}

/**
 * @brief Appends the conditions that place an object: its database, the
 *        table it belongs to, and its name, each where the path gives it.
 *
 * @param[in]     _db    the connection.
 * @param[in,out] _sql   the statement so far.
 * @param[in]     _kind  where the object's kind is kept.
 * @param[in]     _path  where the object is.
 * @param[in]     _rule  the server's lower_case_table_names.
 * @return as d_internal_db_format.
 */
static enum d_internal_db_format_status
d_internal_mariadb_place(
    struct d_mariadb*                     _db,
    struct d_internal_db_buffer*          _sql,
    const struct d_internal_mariadb_kind* _kind,
    const struct d_internal_mariadb_path* _path,
    int                                   _rule
)
{
    enum d_internal_db_format_status status = D_INTERNAL_DB_FORMAT_OK;

    // the database, unless the object is one
    if (_kind->schema)
    {
        status = d_internal_mariadb_match(_db,
                                          _sql,
                                          _kind->schema,
                                          _path->schema,
                                          (_rule == 0));
    }

    // the table, where one is given
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (_path->table) )
    {
        status = d_internal_mariadb_match(_db,
                                          _sql,
                                          _kind->table,
                                          _path->table,
                                          (_rule == 0));
    }

    // the name, unless listing
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (_path->name) )
    {
        status = d_internal_mariadb_match(
            _db,
            _sql,
            _kind->name,
            _path->name,
            d_internal_mariadb_exact(_db, _kind->name_case, _rule));
    }

    return status;
}

/**
 * @brief Writes the statement a lookup runs.
 *
 * @param[in]     _db    the connection.
 * @param[in,out] _sql   receives the statement.
 * @param[in]     _kind  where the object's kind is kept.
 * @param[in]     _path  where the object is; its name, unless `NULL`, is
 *                       matched.
 * @param[in]     _rule  the server's lower_case_table_names.
 * @return as d_internal_db_format.
 *
 * SELECT name FROM information_schema.source WHERE TRUE, then a condition for
 * each part of the path given, then the kind's own. A lookup by name stops at
 * one row; a listing sorts by the names' bytes, the one order every server
 * and collation agree on.
 */
static enum d_internal_db_format_status
d_internal_mariadb_lookup_sql(
    struct d_mariadb*                     _db,
    struct d_internal_db_buffer*          _sql,
    const struct d_internal_mariadb_kind* _kind,
    const struct d_internal_mariadb_path* _path,
    int                                   _rule
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);
    enum d_internal_db_format_status   status  =
        d_internal_db_format(_sql,
                             &quoting,
                             "SELECT %s FROM information_schema.%s WHERE TRUE",
                             _kind->name,
                             _kind->source);

    // where the object is
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_mariadb_place(_db, _sql, _kind, _path, _rule);
    }

    // the kind's own condition, and a listing's
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (_kind->condition) )
    {
        status = d_internal_db_format(_sql,
                                      &quoting,
                                      " AND %s",
                                      _kind->condition);
    }

    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (!_path->name)                      &&
         (_kind->listing) )
    {
        status = d_internal_db_format(_sql,
                                      &quoting,
                                      " AND %s",
                                      _kind->listing);
    }

    // one row answers; a list comes in byte order
    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = (_path->name)
                     ? d_internal_db_format(_sql, &quoting, " LIMIT 1")
                     : d_internal_db_format(_sql,
                                            &quoting,
                                            " ORDER BY CAST(%s AS BINARY)",
                                            _kind->name);
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
 * @return 0, the client's error number, D_INTERNAL_MARIADB_OUT_OF_MEMORY, or
 *         D_DB_INVALID_ARGUMENT.
 *
 * The server's lower_case_table_names is read first, the first time, since
 * the statement depends on it.
 */
static int
d_internal_mariadb_lookup(
    struct d_mariadb*                     _db,
    enum d_internal_mariadb_object        _object,
    const struct d_internal_mariadb_path* _path,
    MYSQL_RES**                           _rows
)
{
    struct d_internal_db_buffer sql  = { NULL, 0u, 0u };
    int                         rule = 0;
    int                         result =
        d_internal_mariadb_case_rule(_db, &rule);

    // the rule, then the statement
    if (result != 0)
    {
        return result;
    }

    result = d_internal_mariadb_format_result(
        d_internal_mariadb_lookup_sql(_db,
                                      &sql,
                                      &d_internal_mariadb_kinds[_object],
                                      _path,
                                      rule));

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_mariadb_send(_db, sql.data, sql.length, _rows);
    d_internal_db_buffer_release(&sql);

    return result;
}

//==============================================================================
// 2.  HANDLES
//==============================================================================

// 2.3    Server facts
//------------------------------------------------------------------------------
/*
d_internal_mariadb_at_least
  The version was read once, when the handle was made.
*/
bool
d_internal_mariadb_at_least(
    const struct d_mariadb* _db,
    enum d_mariadb_flavor   _flavor,
    unsigned long           _version
)
{
    return ( (_db->flavor == _flavor) &&
             (_db->version >= _version) );
}

/*
d_internal_mariadb_case_rule
  lower_case_table_names is fixed when the server starts, so one reading
serves the handle's whole life. A value outside 0 to 2 is read as 0, which
matches names by case: the stricter reading.
*/
int
d_internal_mariadb_case_rule(
    struct d_mariadb* _db,
    int*              _rule
)
{
    int64_t value = 0;

    // read before: the handle remembers
    if (_db->case_rule >= 0)
    {
        *_rule = _db->case_rule;

        return 0;
    }

    const int result = d_internal_mariadb_query_int64(
        _db,
        &value,
        "SELECT @@lower_case_table_names");

    // the failure is the caller's to report
    if (result != 0)
    {
        return result;
    }

    _db->case_rule = ( (value >= 0) &&
                       (value <= 2) ) ? (int)value : 0;
    *_rule         = _db->case_rule;

    return 0;
}

// 2.4    Replies
//------------------------------------------------------------------------------
/*
d_internal_mariadb_replied
  The server sets SERVER_SESSION_STATE_CHANGED in the status of the OK packet
that carries changes, and the client copies that status into the MYSQL it
keeps, which both clients' mysql.h define in full.
*/
void
d_internal_mariadb_replied(
    struct d_mariadb* _db,
    bool              _ok
)
{
    _db->tracked = ( (_ok) &&
                     ((_db->native->server_status &
                       (unsigned int)SERVER_SESSION_STATE_CHANGED) != 0u) );

    return;
}

/*
d_internal_mariadb_rows_end_ok
  Both sides must have agreed: the client asks for CLIENT_DEPRECATE_EOF, and
keeps the flag where the server offered it too. Connector/C knows no such
flag.
*/
bool
d_internal_mariadb_rows_end_ok(
    const struct d_mariadb* _db
)
{
#if defined(CLIENT_DEPRECATE_EOF)
    const unsigned long flag = (unsigned long)CLIENT_DEPRECATE_EOF;

    return ( ((_db->native->client_flag & flag) != 0ul) &&
             ((_db->native->server_capabilities & flag) != 0ul) );
#else
    (void)_db;

    return false;
#endif  // CLIENT_DEPRECATE_EOF
}

// 2.5    Query attributes
//------------------------------------------------------------------------------
/*
d_internal_mariadb_attributes_sent
  The client drops what mysql_bind_param bound once a query is sent; it is
told to here as well, which costs nothing, so that a query the client never
sent drops them just the same. Binding none changes no error state, so a
failure the caller is about to read still reads.
*/
void
d_internal_mariadb_attributes_sent(
    struct d_mariadb* _db
)
{
    // nothing bound, nothing to drop
    if (_db->attribute_count == 0u)
    {
        return;
    }

#if (D_INTERNAL_MARIADB_QUERY_ATTRIBUTES == 1)
    (void)mysql_bind_param(_db->native, 0u, NULL, NULL);
#endif  // D_INTERNAL_MARIADB_QUERY_ATTRIBUTES

    d_internal_mariadb_attributes_release(_db);

    return;
}

/*
d_internal_mariadb_attributes_release
  Each attribute's two copies, then the array.
*/
void
d_internal_mariadb_attributes_release(
    struct d_mariadb* _db
)
{
    for (size_t i = 0u; i < _db->attribute_count; ++i)
    {
        free(_db->attributes[i].name);
        free(_db->attributes[i].value);
    }

    free(_db->attributes);

    _db->attributes      = NULL;
    _db->attribute_count = 0u;

    return;
}

// 2.6    Statements
//------------------------------------------------------------------------------
/*
d_internal_mariadb_idle
  The handle knows which statement's run, if any, holds the connection; the
client's own refusal is not relied on, since a client may instead read the
run's rows away.
*/
int
d_internal_mariadb_idle(
    const struct d_mariadb* _db
)
{
    return (_db->cursor) ? D_INTERNAL_MARIADB_OUT_OF_SYNC : 0;
}

//==============================================================================
// 3.  NAMES
//==============================================================================

// 3.1    Checks
//------------------------------------------------------------------------------
/*
d_internal_mariadb_name_ok
  The servers refuse an empty name themselves, but a caller passing one has
almost certainly made a mistake, and the layer says so first.
*/
bool
d_internal_mariadb_name_ok(
    const char* _name
)
{
    return ( (_name) &&
             (_name[0] != '\0') );
}

/*
d_internal_mariadb_schema_ok
  NULL is the documented way to say the current database; an empty name is
never one.
*/
bool
d_internal_mariadb_schema_ok(
    const char* _schema
)
{
    return ( (!_schema) ||
             (_schema[0] != '\0') );
}

/*
d_internal_mariadb_variable_ok
  A variable's name goes into SET and SELECT @@ unquoted, so it must have the
shape of one: nothing that could end the name and begin more SQL.
*/
bool
d_internal_mariadb_variable_ok(
    const char* _name
)
{
    // a letter or underscore first
    if ( (!_name) ||
         ( (!d_internal_mariadb_ascii_letter(_name[0])) &&
           (_name[0] != '_') ) )
    {
        return false;
    }

    // then letters, digits and underscores
    for (size_t i = 1u; _name[i] != '\0'; ++i)
    {
        if ( (!d_internal_mariadb_ascii_letter(_name[i])) &&
             ( (_name[i] < '0') || (_name[i] > '9') )     &&
             (_name[i] != '_') )
        {
            return false;
        }
    }

    return true;
}

// 3.2    Quoting
//------------------------------------------------------------------------------
/*
d_internal_mariadb_quote_name
  Backquotes work whatever the SQL mode, where double quotes need
ANSI_QUOTES. The name is copied a run at a time, each run ending at a
backquote, which is then doubled.
*/
bool
d_internal_mariadb_quote_name(
    void*                        _context,
    struct d_internal_db_buffer* _out,
    const char*                  _name
)
{
    const size_t length_before = _out->length;
    const char*  run           = _name;
    bool         ok            = d_internal_db_buffer_append(_out, "`", 1u);

    (void)_context;

    // each run, through the next backquote, which is written twice
    while ( (ok) &&
            (*run != '\0') )
    {
        const char* const tick   = strchr(run, '`');
        const size_t      length = (tick) ? ((size_t)(tick - run) + 1u)
                                          : strlen(run);

        ok   = ( (d_internal_db_buffer_append(_out, run, length)) &&
                 ( (!tick) ||
                   (d_internal_db_buffer_append(_out, "`", 1u)) ) );
        run += length;
    }

    // the closing quote; a failure takes back everything written
    if ( (!ok) ||
         (!d_internal_db_buffer_append(_out, "`", 1u)) )
    {
        _out->length = length_before;

        return false;
    }

    return true;
}

/*
d_internal_mariadb_quote_value
  The client escapes into the buffer directly, after room is made for the
worst case: every byte escaped to two, the quotes, and the terminator the
client writes. Escaping follows the connection's character set, so a
multibyte character whose bytes include a quote's is never split.
*/
bool
d_internal_mariadb_quote_value(
    void*                        _context,
    struct d_internal_db_buffer* _out,
    const char*                  _text
)
{
    struct d_mariadb* const db     = (struct d_mariadb*)_context;
    const size_t            length = strlen(_text);

    // a length whose worst case neither size_t nor the client can carry
    if ( (length > ((SIZE_MAX - 3u) / 2u))                       ||
         (length > (D_INTERNAL_MARIADB_LENGTH_MAX / 2u))          ||
         (!d_internal_db_buffer_reserve(_out, (2u * length) + 3u)) )
    {
        return false;
    }

    char* const at = _out->data + _out->length;

    at[0] = '\'';

    const unsigned long written = d_internal_mariadb_escape(
                                      db->native,
                                      at + 1,
                                      _text,
                                      (unsigned long)length);

    // a refusal leaves the length as it was
    if (written == (unsigned long)-1)
    {
        return false;
    }

    at[1u + written] = '\'';
    _out->length    += (size_t)written + 2u;

    return true;
}

/*
d_internal_mariadb_quoting
  Both quoting functions, with the connection as the context the value's
needs.
*/
struct d_internal_db_quoting
d_internal_mariadb_quoting(
    struct d_mariadb* _db
)
{
    const struct d_internal_db_quoting quoting =
    {
        d_internal_mariadb_quote_value,
        d_internal_mariadb_quote_name,
        _db
    };

    return quoting;
}

//==============================================================================
// 4.  STATEMENTS
//==============================================================================

// 4.1    Running
//------------------------------------------------------------------------------
/*
d_internal_mariadb_format_result
  Memory has the client's number; anything else the formatter would not
write -- a format it cannot read, a NaN, a NUL character -- is the caller's
argument.
*/
int
d_internal_mariadb_format_result(
    enum d_internal_db_format_status _status
)
{
    switch (_status)
    {
        case D_INTERNAL_DB_FORMAT_OK:
            return 0;

        case D_INTERNAL_DB_FORMAT_NO_MEMORY:
            return D_INTERNAL_MARIADB_OUT_OF_MEMORY;

        default:
            break;
    }

    return D_DB_INVALID_ARGUMENT;
}

/*
d_internal_mariadb_ready
  A run that holds the connection refuses everything first, before anything
is closed or sent. A statement whose prepare failed was never prepared on the
server, so closing it sends nothing, but it clears the connection's error,
which is why it was kept until now; so do the statements finalized while a
run held the connection, whose closing tells the server. The mode is a
command to the server, so it is turned off only when a script left it on,
which the handle remembers.
*/
int
d_internal_mariadb_ready(
    struct d_mariadb* _db
)
{
    // nothing is sent while a statement's rows are still arriving
    if (_db->cursor)
    {
        return D_INTERNAL_MARIADB_OUT_OF_SYNC;
    }

    // the statement whose failure has been read by now
    if (_db->failed)
    {
        (void)mysql_stmt_close(_db->failed);

        _db->failed = NULL;
    }

    // statements finalized while a run held the connection
    d_internal_mariadb_close_retired(_db);

    // the mode already off, as the layer connects
    if (!_db->multi_statements)
    {
        return 0;
    }

    const int result = (mysql_set_server_option(
                            _db->native,
                            MYSQL_OPTION_MULTI_STATEMENTS_OFF) == 0)
                           ? 0
                           : (int)mysql_errno(_db->native);

    // a reply that reports no changes
    d_internal_mariadb_replied(_db, false);

    if (result == 0)
    {
        _db->multi_statements = false;
    }

    return result;
}

/*
d_internal_mariadb_send
  A script that failed may have left the client's multi-statement mode on,
so that its error would still read; it goes off first, before anything else
runs. Then the statement, which takes the query attributes set for it, and
every result it produces; the last result's end says whether the reply
carried the session's changes.
*/
int
d_internal_mariadb_send(
    struct d_mariadb* _db,
    const char*       _sql,
    size_t            _length,
    MYSQL_RES**       _rows
)
{
    MYSQL* const native = _db->native;

    // nothing yet, whatever follows
    if (_rows)
    {
        *_rows = NULL;
    }

    // a length the client's unsigned long cannot carry
    if (_length > D_INTERNAL_MARIADB_LENGTH_MAX)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_ready(_db);

    // the statement, then its results, on the connection as the layer's
    // statements need it
    if (result == 0)
    {
        result = (mysql_real_query(native, _sql, (unsigned long)_length) == 0)
                     ? 0
                     : (int)mysql_errno(native);

        d_internal_mariadb_attributes_sent(_db);
    }

    if (result == 0)
    {
        result = d_internal_mariadb_read_results(native, _rows);
    }

    d_internal_mariadb_replied(_db,
                               ( (result == 0) &&
                                 ( (mysql_field_count(native) == 0u) ||
                                   (d_internal_mariadb_rows_end_ok(_db)) ) ));

    return result;
}

/*
d_internal_mariadb_vrun
  The statement is formatted with the connection's quoting, sent, and
released.
*/
int
d_internal_mariadb_vrun(
    struct d_mariadb* _db,
    MYSQL_RES**       _rows,
    const char*       _format,
    va_list           _args
)
{
    struct d_internal_db_buffer        sql     = { NULL, 0u, 0u };
    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);

    // nothing yet, whatever follows
    if (_rows)
    {
        *_rows = NULL;
    }

    const int result = d_internal_mariadb_format_result(
        d_internal_db_vformat(&sql, &quoting, _format, _args));

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    const int sent = d_internal_mariadb_send(_db, sql.data, sql.length, _rows);

    d_internal_db_buffer_release(&sql);

    return sent;
}

/*
d_internal_mariadb_run
  The variadic face of d_internal_mariadb_vrun.
*/
int
d_internal_mariadb_run(
    struct d_mariadb* _db,
    MYSQL_RES**       _rows,
    const char*       _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_mariadb_vrun(_db, _rows, _format, args);

    va_end(args);

    return result;
}

// 4.2    Single values
//------------------------------------------------------------------------------
/*
d_internal_mariadb_vquery
  The first value of the first result, copied out before the result is
freed.
*/
int
d_internal_mariadb_vquery(
    struct d_mariadb* _db,
    char**            _value,
    const char*       _format,
    va_list           _args
)
{
    MYSQL_RES* rows   = NULL;
    const int  result = d_internal_mariadb_vrun(_db, &rows, _format, _args);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_mariadb_first_value(rows, _value);
}

/*
d_internal_mariadb_query
  The variadic face of d_internal_mariadb_vquery.
*/
int
d_internal_mariadb_query(
    struct d_mariadb* _db,
    char**            _value,
    const char*       _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result = d_internal_mariadb_vquery(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_internal_mariadb_vquery_int64
  The value as text, as the protocol sends it, then read as an integer.
*/
int
d_internal_mariadb_vquery_int64(
    struct d_mariadb* _db,
    int64_t*          _value,
    const char*       _format,
    va_list           _args
)
{
    char*   text   = NULL;
    int64_t number = 0;
    int     result = d_internal_mariadb_vquery(_db, &text, _format, _args);

    // the text, then its number
    if (result != 0)
    {
        return result;
    }

    result = d_internal_mariadb_read_int64(text, &number);
    free(text);

    // written only on success
    if (result == 0)
    {
        *_value = number;
    }

    return result;
}

/*
d_internal_mariadb_query_int64
  The variadic face of d_internal_mariadb_vquery_int64.
*/
int
d_internal_mariadb_query_int64(
    struct d_mariadb* _db,
    int64_t*          _value,
    const char*       _format,
    ...
)
{
    va_list args;

    va_start(args, _format);

    const int result =
        d_internal_mariadb_vquery_int64(_db, _value, _format, args);

    va_end(args);

    return result;
}

/*
d_internal_mariadb_vquery_double
  As d_internal_mariadb_vquery_int64, reading a double; NULL reads as 0.
*/
int
d_internal_mariadb_vquery_double(
    struct d_mariadb* _db,
    double*           _value,
    const char*       _format,
    va_list           _args
)
{
    char*  text   = NULL;
    double number = 0.0;
    int    result = d_internal_mariadb_vquery(_db, &text, _format, _args);

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

// 4.3    Rows
//------------------------------------------------------------------------------
/*
d_internal_mariadb_field_type
  TINYINT(1) is the type BOOLEAN is an alias of, and the one every client
reads as a boolean; an INT UNSIGNED's values pass a 32-bit int's range. The
JSON type's number, 245, is spelled out: MYSQL_TYPE_JSON arrived in
libmysqlclient 5.7.8, after the floor. A string or BLOB is binary by its
character set, 63, whatever its collation says.
*/
enum d_db_type
d_internal_mariadb_field_type(
    const MYSQL_FIELD* _field
)
{
    const bool binary   = (_field->charsetnr == 63u);
    const bool unsigned_ = ((_field->flags & UNSIGNED_FLAG) != 0u);

    switch ((int)_field->type)
    {
        case MYSQL_TYPE_TINY:
            return (_field->length == 1u) ? D_DB_TYPE_BOOLEAN
                                          : D_DB_TYPE_INTEGER;

        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
            return D_DB_TYPE_INTEGER;

        case MYSQL_TYPE_LONG:
            return (unsigned_) ? D_DB_TYPE_BIG_INTEGER : D_DB_TYPE_INTEGER;

        case MYSQL_TYPE_LONGLONG:
            return D_DB_TYPE_BIG_INTEGER;

        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
            return D_DB_TYPE_FLOATING_POINT;

        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            return D_DB_TYPE_DECIMAL;

        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_NEWDATE:
            return D_DB_TYPE_DATE;

        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_TIME2:
            return D_DB_TYPE_TIME;

        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_DATETIME2:
            return D_DB_TYPE_DATETIME;

        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_TIMESTAMP2:
            return D_DB_TYPE_TIMESTAMP;

        case 245:  // MYSQL_TYPE_JSON
            return D_DB_TYPE_JSON;

        case MYSQL_TYPE_BIT:
            return D_DB_TYPE_BINARY;

        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_ENUM:
        case MYSQL_TYPE_SET:
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
            return (binary) ? D_DB_TYPE_BINARY : D_DB_TYPE_STRING;

        default:
            break;
    }

    return D_DB_TYPE_NONE;
}

/*
d_internal_mariadb_field_type_name
  Names as DDL writes them. A CHAR's ENUM and SET flags say which of the
three it is; a BLOB's character set whether it is a BLOB or a TEXT. The
protocol sends every BLOB and TEXT as MYSQL_TYPE_BLOB, its length the most
bytes the column holds -- 255, 65535, 16777215 or 4294967295, times the
character set's longest character, at most four bytes, for a TEXT, and
capped at 4294967295 -- so the bounds four times the smaller sizes tell the
four apart whatever the character set.
*/
const char*
d_internal_mariadb_field_type_name(
    const MYSQL_FIELD* _field
)
{
    const bool binary    = (_field->charsetnr == 63u);
    const bool unsigned_ = ((_field->flags & UNSIGNED_FLAG) != 0u);

    switch ((int)_field->type)
    {
        case MYSQL_TYPE_TINY:
            return (unsigned_) ? "TINYINT UNSIGNED" : "TINYINT";

        case MYSQL_TYPE_SHORT:
            return (unsigned_) ? "SMALLINT UNSIGNED" : "SMALLINT";

        case MYSQL_TYPE_INT24:
            return (unsigned_) ? "MEDIUMINT UNSIGNED" : "MEDIUMINT";

        case MYSQL_TYPE_LONG:
            return (unsigned_) ? "INT UNSIGNED" : "INT";

        case MYSQL_TYPE_LONGLONG:
            return (unsigned_) ? "BIGINT UNSIGNED" : "BIGINT";

        case MYSQL_TYPE_YEAR:
            return "YEAR";

        case MYSQL_TYPE_FLOAT:
            return "FLOAT";

        case MYSQL_TYPE_DOUBLE:
            return "DOUBLE";

        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            return "DECIMAL";

        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_NEWDATE:
            return "DATE";

        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_TIME2:
            return "TIME";

        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_DATETIME2:
            return "DATETIME";

        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_TIMESTAMP2:
            return "TIMESTAMP";

        case 245:  // MYSQL_TYPE_JSON
            return "JSON";

        case MYSQL_TYPE_BIT:
            return "BIT";

        case MYSQL_TYPE_ENUM:
            return "ENUM";

        case MYSQL_TYPE_SET:
            return "SET";

        case MYSQL_TYPE_STRING:
            if ((_field->flags & ENUM_FLAG) != 0u)
            {
                return "ENUM";
            }

            if ((_field->flags & SET_FLAG) != 0u)
            {
                return "SET";
            }

            return (binary) ? "BINARY" : "CHAR";

        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_VAR_STRING:
            return (binary) ? "VARBINARY" : "VARCHAR";

        case MYSQL_TYPE_TINY_BLOB:
            return (binary) ? "TINYBLOB" : "TINYTEXT";

        case MYSQL_TYPE_MEDIUM_BLOB:
            return (binary) ? "MEDIUMBLOB" : "MEDIUMTEXT";

        case MYSQL_TYPE_LONG_BLOB:
            return (binary) ? "LONGBLOB" : "LONGTEXT";

        case MYSQL_TYPE_BLOB:
            if (_field->length <= (255ul * 4ul))
            {
                return (binary) ? "TINYBLOB" : "TINYTEXT";
            }

            if (_field->length <= (65535ul * 4ul))
            {
                return (binary) ? "BLOB" : "TEXT";
            }

            if (_field->length <= (16777215ul * 4ul))
            {
                return (binary) ? "MEDIUMBLOB" : "MEDIUMTEXT";
            }

            return (binary) ? "LONGBLOB" : "LONGTEXT";

        case MYSQL_TYPE_GEOMETRY:
            return "GEOMETRY";

        case MYSQL_TYPE_NULL:
            return "NULL";

        default:
            break;
    }

    return "";
}

/*
d_internal_mariadb_rows_columns
  A field's name is never NULL in either client; the builder reads NULL as ""
all the same.
*/
bool
d_internal_mariadb_rows_columns(
    struct d_internal_db_rows* _rows,
    const MYSQL_FIELD*         _fields,
    unsigned                   _count
)
{
    // each field, described once
    for (unsigned i = 0u; i < _count; ++i)
    {
        if (!d_internal_db_rows_column(
                 _rows,
                 _fields[i].name,
                 d_internal_mariadb_field_type(&_fields[i]),
                 d_internal_mariadb_field_type_name(&_fields[i])))
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_mariadb_rows_value
  The protocol's text for a number is exact: an integer's digits, and for a
FLOAT or DOUBLE the shortest text that reads back as the stored value, which
both servers write through either protocol. An integer db_internal cannot
read is an unsigned BIGINT past INT64_MAX, kept as its text. MySQL's JSON
carries the binary character set's number and is still text; so are DECIMAL
and the temporal types, which carry it too.
*/
bool
d_internal_mariadb_rows_value(
    struct d_internal_db_rows* _rows,
    const MYSQL_FIELD*         _field,
    const char*                _text,
    unsigned long              _length
)
{
    // SQL NULL
    if (!_text)
    {
        return d_internal_db_rows_null(_rows);
    }

    switch ((int)_field->type)
    {
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_YEAR:
        {
            int64_t integer = 0;

            // an integer int64_t holds, or the digits as they came
            return (d_internal_db_parse_integer(_text, &integer))
                       ? d_internal_db_rows_integer(_rows, integer)
                       : d_internal_db_rows_text(_rows, _text,
                                                 (size_t)_length);
        }

        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        {
            double real = 0.0;

            // the number, or the text when it is none
            return (d_internal_db_parse_real(_text, &real))
                       ? d_internal_db_rows_real(_rows, real)
                       : d_internal_db_rows_text(_rows, _text,
                                                 (size_t)_length);
        }

        case MYSQL_TYPE_BIT:
        case MYSQL_TYPE_GEOMETRY:
            return d_internal_db_rows_blob(_rows, _text, (size_t)_length);

        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
            return (_field->charsetnr == 63u)
                       ? d_internal_db_rows_blob(_rows, _text,
                                                 (size_t)_length)
                       : d_internal_db_rows_text(_rows, _text,
                                                 (size_t)_length);

        default:
            break;
    }

    return d_internal_db_rows_text(_rows, _text, (size_t)_length);
}

/*
d_internal_mariadb_collect_rows
  A stored result is already in memory, so mysql_fetch_row ends only at its
last row, never with an error.
*/
int
d_internal_mariadb_collect_rows(
    MYSQL_RES*        _result,
    struct d_db_rows* _rows
)
{
    struct d_internal_db_rows builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u, 0u
    };
    bool kept = true;

    // a statement that returned no rows has no columns either
    if (_result)
    {
        const unsigned           count  = mysql_num_fields(_result);
        const MYSQL_FIELD* const fields = mysql_fetch_fields(_result);
        MYSQL_ROW                row    = NULL;

        kept = d_internal_mariadb_rows_columns(&builder, fields, count);

        // each row, value by value, until the rows or the memory run out
        while ( (kept) &&
                ((row = mysql_fetch_row(_result)) != NULL) )
        {
            const unsigned long* const lengths = mysql_fetch_lengths(_result);

            for (unsigned i = 0u; ( (kept) && (i < count) ); ++i)
            {
                kept = d_internal_mariadb_rows_value(&builder, &fields[i],
                                                     row[i], lengths[i]);
            }
        }

        mysql_free_result(_result);
    }

    // memory ran out part-way
    if (!kept)
    {
        d_internal_db_rows_release(&builder);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    return (d_internal_db_rows_finish(&builder, _rows))
               ? 0
               : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
}

//==============================================================================
// 5.  CATALOGUE
//==============================================================================

// 5.2    Lookups
//------------------------------------------------------------------------------
/*
d_internal_mariadb_has
  One row answers yes.
*/
int
d_internal_mariadb_has(
    struct d_mariadb*                     _db,
    enum d_internal_mariadb_object        _object,
    const struct d_internal_mariadb_path* _path,
    bool*                                 _exists
)
{
    MYSQL_RES* rows = NULL;

    // the answer is no until a row says otherwise
    if (!_exists)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_exists = false;

    // parameter validation
    if ( (!_db) ||
         (!d_internal_mariadb_path_ok(_object, _path, true)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_lookup(_db, _object, _path, &rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    *_exists = ( (rows) &&
                 (mysql_num_rows(rows) > 0u) );
    mysql_free_result(rows);

    return 0;
}

/*
d_internal_mariadb_list
  The lookup without a name, its rows collected.
*/
int
d_internal_mariadb_list(
    struct d_mariadb*                     _db,
    enum d_internal_mariadb_object        _object,
    const struct d_internal_mariadb_path* _path,
    struct d_db_name_list*                _names
)
{
    MYSQL_RES* rows = NULL;

    // parameter validation
    if ( (!_db)    ||
         (!_names) ||
         (!d_internal_mariadb_path_ok(_object, _path, false)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const struct d_internal_mariadb_path listing =
    {
        _path->schema, _path->table, NULL
    };
    const int result = d_internal_mariadb_lookup(_db,
                                                 _object,
                                                 &listing,
                                                 &rows);

    // a failure has no rows to read
    if (result != 0)
    {
        return result;
    }

    return d_internal_mariadb_collect(rows, _names);
}

/*
d_internal_mariadb_match
  The plain comparison is the one an information_schema lookup is optimized
for, so it is always written; HEX() makes the second compare bytes, which no
collation can fold. NULL names the current database, DATABASE().
*/
enum d_internal_db_format_status
d_internal_mariadb_match(
    struct d_mariadb*            _db,
    struct d_internal_db_buffer* _sql,
    const char*                  _column,
    const char*                  _name,
    bool                         _exact
)
{
    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);
    enum d_internal_db_format_status   status  =
        (_name) ? d_internal_db_format(_sql,
                                       &quoting,
                                       " AND %s = %Q",
                                       _column,
                                       _name)
                : d_internal_db_format(_sql,
                                       &quoting,
                                       " AND %s = DATABASE()",
                                       _column);

    // the bytes too, where the server matches by case
    if ( (status == D_INTERNAL_DB_FORMAT_OK) &&
         (_exact) )
    {
        status = (_name) ? d_internal_db_format(_sql,
                                                &quoting,
                                                " AND HEX(%s) = HEX(%Q)",
                                                _column,
                                                _name)
                         : d_internal_db_format(
                               _sql,
                               &quoting,
                               " AND HEX(%s) = HEX(DATABASE())",
                               _column);
    }

    return status;
}

#else

// an ISO C translation unit must declare something; built as stubs, the
// layer has no plumbing
typedef int d_internal_mariadb_no_plumbing;

#endif  // D_INTERNAL_MARIADB
