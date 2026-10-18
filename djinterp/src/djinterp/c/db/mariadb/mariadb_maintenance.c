/*******************************************************************************
* djinterp [c]                                             mariadb_maintenance.c
*
* Implementation of mariadb_maintenance.h.
*   ANALYZE, OPTIMIZE and CHECK TABLE answer with rows of Table, Op, Msg_type
* and Msg_text. Success is a `status` row; a table that is not there is an
* `Error` row and a `status` of "Operation failed"; an engine that cannot do
* the operation sends a `note` alone. The note's words follow the server's
* language and the status words do not, so the layer reads the rows' types
* and the status words only. Each function first asks whether the table is
* there, so a missing one reads as D_DB_NOT_FOUND on both servers, where
* MySQL would also say so of a view.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_maintenance.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_maintenance.h"  // corresponding header
// std
#include <stddef.h>  // NULL
#include <string.h>  // strcmp
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../db_internal.h"  // d_internal_db_parse_integer
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, uint64_t


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/*
d_internal_mariadb_is_type
  A Msg_type, compared without regard to ASCII case: the servers write
"status" and "note" in lower case and "Error" capitalized.
*/
static bool
d_internal_mariadb_is_type(
    const char* _text,
    const char* _type
)
{
    size_t i = 0u;

    // character by character, folding ASCII upper case
    while ( (_text[i] != '\0') &&
            (_type[i] != '\0') )
    {
        const char letter = ( (_text[i] >= 'A') && (_text[i] <= 'Z') )
                                ? (char)(_text[i] - 'A' + 'a')
                                : _text[i];

        if (letter != _type[i])
        {
            return false;
        }

        ++i;
    }

    return (_text[i] == _type[i]);
}

/**
 * @brief Checks the arguments every function here takes, and that the table
 *        is there.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the database; `NULL` for the current.
 * @param[in] _table   the table.
 * @return 0 when it is, D_DB_NOT_FOUND when it is not, the client's error
 *         number for a failure to ask, or D_DB_INVALID_ARGUMENT.
 */
static int
d_internal_mariadb_find_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table
)
{
    const struct d_internal_mariadb_path path   = { _schema, NULL, _table };
    bool                                 exists = false;

    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_schema_ok(_schema)) ||
         (!d_internal_mariadb_name_ok(_table)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_has(_db,
                                              D_INTERNAL_MARIADB_TABLE,
                                              &path,
                                              &exists);

    return (result != 0) ? result : (exists) ? 0 : D_DB_NOT_FOUND;
}

/**
 * @brief Reads the rows a maintenance statement answered with.
 *
 * @param[in]  _rows   Table, Op, Msg_type and Msg_text; may be `NULL`.
 * @param[out] _sound  receives whether the last status was "OK" or "Table
 *                     is already up to date"; may be `NULL`.
 * @return 0 for a last status other than "Operation failed",
 *         D_DB_UNSUPPORTED for notes without a status, or the unknown error
 *         for a failure the rows report.
 *
 * A stored result's rows stay valid until it is freed, so the last status
 * is read where it lies; the caller frees the result after.
 */
static int
d_internal_mariadb_verdict(
    MYSQL_RES* _rows,
    bool*      _sound
)
{
    const char* status = NULL;
    bool        noted  = false;

    // the last status, and whether a note came
    for (MYSQL_ROW row = ( (_rows) &&
                           (mysql_num_fields(_rows) >= 4u) )
                             ? mysql_fetch_row(_rows)
                             : NULL;
         row;
         row = mysql_fetch_row(_rows))
    {
        if ( (row[2]) &&
             (row[3]) &&
             (d_internal_mariadb_is_type(row[2], "status")) )
        {
            status = row[3];
        }
        else if ( (row[2]) &&
                  (d_internal_mariadb_is_type(row[2], "note")) )
        {
            noted = true;
        }
    }

    // soundness, by the status's words
    if ( (_sound) &&
         (status) )
    {
        *_sound = ( (strcmp(status, "OK") == 0) ||
                    (strcmp(status, "Table is already up to date") == 0) );
    }

    return (status) ? ( (strcmp(status, "Operation failed") == 0)
                            ? D_INTERNAL_MARIADB_UNKNOWN_ERROR
                            : 0 )
                    : (noted) ? D_DB_UNSUPPORTED
                              : D_INTERNAL_MARIADB_UNKNOWN_ERROR;
}

/*
d_internal_mariadb_maintain
  ANALYZE, OPTIMIZE or CHECK TABLE on a table that is there, its rows read
by d_internal_mariadb_verdict.
*/
static int
d_internal_mariadb_maintain(
    struct d_mariadb* _db,
    const char*       _keyword,
    const char*       _schema,
    const char*       _table,
    bool*             _sound
)
{
    MYSQL_RES* rows   = NULL;
    const int  result = d_internal_mariadb_run(_db,
                                               &rows,
                                               "%s TABLE %T",
                                               _keyword,
                                               _schema,
                                               _table);

    // a failure to run it at all
    if (result != 0)
    {
        return result;
    }

    const int verdict = d_internal_mariadb_verdict(rows, _sound);

    mysql_free_result(rows);

    return verdict;
}

/*
d_internal_mariadb_read_checksum
  CHECKSUM TABLE answers a row of Table and Checksum, the checksum a
non-negative integer, or NULL for a table it cannot find -- here, one gone
since it was found.
*/
static int
d_internal_mariadb_read_checksum(
    MYSQL_RES* _rows,
    uint64_t*  _checksum
)
{
    int64_t         value = 0;
    const MYSQL_ROW row   = ( (_rows) &&
                              (mysql_num_fields(_rows) >= 2u) )
                                ? mysql_fetch_row(_rows)
                                : NULL;

    // gone
    if ( (!row) ||
         (!row[1]) )
    {
        return D_DB_NOT_FOUND;
    }

    // a checksum, or something no server writes
    if ( (!d_internal_db_parse_integer(row[1], &value)) ||
         (value < 0) )
    {
        return D_INTERNAL_MARIADB_UNKNOWN_ERROR;
    }

    *_checksum = (uint64_t)value;

    return 0;
}

/*
d_internal_mariadb_maintenance
  The table is found first, then the statement run.
*/
static int
d_internal_mariadb_maintenance(
    struct d_mariadb* _db,
    const char*       _keyword,
    const char*       _schema,
    const char*       _table,
    bool*             _sound
)
{
    const int result = d_internal_mariadb_find_table(_db, _schema, _table);

    // missing, or not to be asked after
    if (result != 0)
    {
        return result;
    }

    return d_internal_mariadb_maintain(_db,
                                       _keyword,
                                       _schema,
                                       _table,
                                       _sound);
}

//==============================================================================
// 1.  TABLES
//==============================================================================

// 1.1    Statistics and storage
//------------------------------------------------------------------------------
/*
d_mariadb_analyze_table
  ANALYZE TABLE, read by its rows.
*/
int
d_mariadb_analyze_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table
)
{
    return d_internal_mariadb_maintenance(_db,
                                          "ANALYZE",
                                          _schema,
                                          _table,
                                          NULL);
}

/*
d_mariadb_optimize_table
  OPTIMIZE TABLE, read by its rows; InnoDB adds a note that it recreates the
table instead, which the status after it outweighs.
*/
int
d_mariadb_optimize_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table
)
{
    return d_internal_mariadb_maintenance(_db,
                                          "OPTIMIZE",
                                          _schema,
                                          _table,
                                          NULL);
}

// 1.2    Checks
//------------------------------------------------------------------------------
/*
d_mariadb_check_table
  CHECK TABLE, whose last status says whether the table is sound: "OK", or
"Corrupt".
*/
int
d_mariadb_check_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    bool*             _ok
)
{
    bool sound = false;

    // parameter validation
    if (!_ok)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_maintenance(_db,
                                                      "CHECK",
                                                      _schema,
                                                      _table,
                                                      &sound);

    // written only on success
    if (result == 0)
    {
        *_ok = sound;
    }

    return result;
}

/*
d_mariadb_checksum_table
  The table is found first, then CHECKSUM TABLE run and its row read.
*/
int
d_mariadb_checksum_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    uint64_t*         _checksum
)
{
    MYSQL_RES* rows = NULL;

    // parameter validation
    if (!_checksum)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_find_table(_db, _schema, _table);

    // the table, then its checksum
    if (result == 0)
    {
        result = d_internal_mariadb_run(_db,
                                        &rows,
                                        "CHECKSUM TABLE %T",
                                        _schema,
                                        _table);
    }

    if (result == 0)
    {
        result = d_internal_mariadb_read_checksum(rows, _checksum);
        mysql_free_result(rows);
    }

    return result;
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_analyze_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_analyze_table(
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
d_mariadb_optimize_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_optimize_table(
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
d_mariadb_check_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_check_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    bool*             _ok
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_ok;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_checksum_table
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_checksum_table(
    struct d_mariadb* _db,
    const char*       _schema,
    const char*       _table,
    uint64_t*         _checksum
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_checksum;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MARIADB
