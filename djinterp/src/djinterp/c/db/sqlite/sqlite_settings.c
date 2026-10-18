/*******************************************************************************
* djinterp [c]                                                 sqlite_settings.c
*
* Implementation of sqlite_settings.h.
*   A typed set writes the PRAGMA and reads it back; a get reads it. Both go
* through one reader, so a PRAGMA that reports nothing -- a library built
* without the feature -- is told apart from one reporting 0.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_settings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_settings.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
// djinterp
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, int64_t


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Reads a PRAGMA's value as an integer.
 *
 * @param[in]  _db      the connection.
 * @param[out] _value   receives the value; written only on success.
 * @param[in]  _format  the PRAGMA, as an sqlite3_mprintf format.
 * @param[in]  ...      the format's arguments.
 * @return SQLITE_OK, D_DB_NOT_FOUND when it reports nothing, or the SQLite
 *         result code of the failure.
 */
static int
d_internal_sqlite_pragma_int64(
    sqlite3*    _db,
    int64_t*    _value,
    const char* _format,
    ...
)
{
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

/**
 * @brief The keyword PRAGMA journal_mode uses for a mode.
 *
 * @param[in] _mode  the mode.
 * @return the keyword, or `NULL` for a value outside the enumeration.
 */
static const char*
d_internal_sqlite_journal_keyword(
    enum d_sqlite_journal_mode _mode
)
{
    switch (_mode)
    {
        case D_SQLITE_JOURNAL_DELETE:   return "DELETE";
        case D_SQLITE_JOURNAL_TRUNCATE: return "TRUNCATE";
        case D_SQLITE_JOURNAL_PERSIST:  return "PERSIST";
        case D_SQLITE_JOURNAL_MEMORY:   return "MEMORY";
        case D_SQLITE_JOURNAL_WAL:      return "WAL";
        case D_SQLITE_JOURNAL_OFF:      return "OFF";
        default:                        break;
    }

    return NULL;
}

/**
 * @brief Reports whether text is a PRAGMA's name: letters, digits and
 *        underscores, not starting with a digit.
 *
 * @param[in] _name  the text.
 * @return `true` for a name that can be written into a statement unquoted.
 *
 * PRAGMA names cannot be bound or quoted, so this is what makes inserting
 * one safe. ASCII is compared directly, never through <ctype.h>, whose
 * answers follow the C locale.
 */
static bool
d_internal_sqlite_pragma_name_ok(
    const char* _name
)
{
    // a name has at least one character, and does not start with a digit
    if ( (!d_internal_sqlite_name_ok(_name)) ||
         ( (_name[0] >= '0') &&
           (_name[0] <= '9') ) )
    {
        return false;
    }

    // every character a letter, digit or underscore
    for (size_t i = 0u; _name[i] != '\0'; ++i)
    {
        const char c = _name[i];

        if ( (!( (c >= 'a') && (c <= 'z') )) &&
             (!( (c >= 'A') && (c <= 'Z') )) &&
             (!( (c >= '0') && (c <= '9') )) &&
             (c != '_') )
        {
            return false;
        }
    }

    return true;
}

//==============================================================================
// 2.  CONNECTION SETTINGS
//==============================================================================

// 2.1    Foreign keys
//------------------------------------------------------------------------------
/*
d_sqlite_set_foreign_keys
  SQLite ignores the PRAGMA inside a transaction, silently; the read-back is
how that is noticed.
*/
int
d_sqlite_set_foreign_keys(
    sqlite3* _db,
    bool     _enabled
)
{
    // parameter validation
    if (!_db)
    {
        return SQLITE_MISUSE;
    }

    int     result = sqlite3_exec(_db,
                                  (_enabled) ? "PRAGMA foreign_keys = ON"
                                             : "PRAGMA foreign_keys = OFF",
                                  NULL,
                                  NULL,
                                  NULL);
    int64_t now    = 0;

    // read back what took
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_pragma_int64(_db,
                                                &now,
                                                "PRAGMA foreign_keys");
    }

    // a library without foreign keys reports nothing
    if (result == D_DB_NOT_FOUND)
    {
        return D_DB_UNSUPPORTED;
    }

    // a setting that did not take
    if ( (result == SQLITE_OK) &&
         ((now != 0) != _enabled) )
    {
        return D_DB_UNSUPPORTED;
    }

    return result;
}

/*
d_sqlite_get_foreign_keys
  0 or 1, or nothing in a library built without foreign keys.
*/
int
d_sqlite_get_foreign_keys(
    sqlite3* _db,
    bool*    _enabled
)
{
    // parameter validation
    if ( (!_db) ||
         (!_enabled) )
    {
        return SQLITE_MISUSE;
    }

    int64_t   now    = 0;
    const int result = d_internal_sqlite_pragma_int64(_db,
                                                      &now,
                                                      "PRAGMA foreign_keys");

    // a library without foreign keys reports nothing
    if (result == D_DB_NOT_FOUND)
    {
        return D_DB_UNSUPPORTED;
    }

    // a value only on success
    if (result == SQLITE_OK)
    {
        *_enabled = (now != 0);
    }

    return result;
}

// 2.2    Busy timeout
//------------------------------------------------------------------------------
/*
d_sqlite_set_busy_timeout
  sqlite3_busy_timeout itself; a timeout of 0 removes the handler.
*/
int
d_sqlite_set_busy_timeout(
    sqlite3* _db,
    int      _milliseconds
)
{
    // parameter validation
    if ( (!_db) ||
         (_milliseconds < 0) )
    {
        return SQLITE_MISUSE;
    }

    return sqlite3_busy_timeout(_db, _milliseconds);
}

/*
d_sqlite_get_busy_timeout
  PRAGMA busy_timeout reads it back (3.7.15).
*/
int
d_sqlite_get_busy_timeout(
    sqlite3* _db,
    int*     _milliseconds
)
{
    // parameter validation
    if ( (!_db) ||
         (!_milliseconds) )
    {
        return SQLITE_MISUSE;
    }

    int64_t   now    = 0;
    const int result = d_internal_sqlite_pragma_int64(_db,
                                                      &now,
                                                      "PRAGMA busy_timeout");

    // a value only on success
    if (result == SQLITE_OK)
    {
        *_milliseconds = (int)now;
    }

    return result;
}

//==============================================================================
// 3.  DATABASE SETTINGS
//==============================================================================

// 3.1    Journal mode
//------------------------------------------------------------------------------
/*
d_sqlite_set_journal_mode
  PRAGMA journal_mode answers with the mode now in force, which is how a
refusal shows: the answer is the old mode, not an error.
*/
int
d_sqlite_set_journal_mode(
    sqlite3*                   _db,
    const char*                _schema,
    enum d_sqlite_journal_mode _mode
)
{
    // parameter validation
    if ( (!_db)                                             ||
         (!d_internal_sqlite_schema_ok(_schema))            ||
         (!d_internal_sqlite_journal_keyword(_mode)) )
    {
        return SQLITE_MISUSE;
    }

    const char* const keyword   = d_internal_sqlite_journal_keyword(_mode);
    sqlite3_stmt*     statement = NULL;
    char*             now       = NULL;
    int               result    = d_internal_sqlite_prepare(
        _db,
        &statement,
        "PRAGMA \"%w\".journal_mode = %s",
        d_internal_sqlite_schema(_schema),
        keyword);

    // the answer is the mode in force afterwards
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_step_text(statement, &now);
    }

    // the mode that took, against the one asked for
    if ( (result == SQLITE_OK) &&
         ( (!now) ||
           (sqlite3_stricmp(now, keyword) != 0) ) )
    {
        result = D_DB_UNSUPPORTED;
    }

    free(now);

    return result;
}

/*
d_sqlite_get_journal_mode
  The keyword back to the enumeration, ignoring case as SQLite does.
*/
int
d_sqlite_get_journal_mode(
    sqlite3*                    _db,
    const char*                 _schema,
    enum d_sqlite_journal_mode* _mode
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!_mode) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    char*         now       = NULL;
    int           result    = d_internal_sqlite_prepare(
        _db,
        &statement,
        "PRAGMA \"%w\".journal_mode",
        d_internal_sqlite_schema(_schema));

    // the answer is the mode in force
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_step_text(statement, &now);
    }

    // find the mode among the six this enumeration names
    if (result == SQLITE_OK)
    {
        result = D_DB_UNSUPPORTED;

        for (int i = D_SQLITE_JOURNAL_DELETE; i <= D_SQLITE_JOURNAL_OFF; ++i)
        {
            const enum d_sqlite_journal_mode mode =
                (enum d_sqlite_journal_mode)i;

            if ( (now) &&
                 (sqlite3_stricmp(now,
                                  d_internal_sqlite_journal_keyword(mode))
                  == 0) )
            {
                *_mode = mode;
                result = SQLITE_OK;

                break;
            }
        }
    }

    free(now);

    return result;
}

// 3.2    Synchronous
//------------------------------------------------------------------------------
/*
d_sqlite_set_synchronous
  Set, then read back. EXTRA is checked against the version first: before
3.11.0 SQLite stores the number 3 and reads it back, but acts on it as FULL,
so the read-back alone would report a success that is not one.
*/
int
d_sqlite_set_synchronous(
    sqlite3*                  _db,
    const char*               _schema,
    enum d_sqlite_synchronous _value
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         ((int)_value < (int)D_SQLITE_SYNCHRONOUS_OFF) ||
         ((int)_value > (int)D_SQLITE_SYNCHRONOUS_EXTRA) )
    {
        return SQLITE_MISUSE;
    }

    // EXTRA arrived in 3.11.0; an older library only pretends to take it
    if ( (_value == D_SQLITE_SYNCHRONOUS_EXTRA) &&
         (!d_internal_sqlite_runtime_at_least(
             D_ENV_SQLITE_ENCODE_VERSION(3, 11, 0))) )
    {
        return D_DB_UNSUPPORTED;
    }

    const char* const schema = d_internal_sqlite_schema(_schema);
    int64_t           now    = -1;
    int               result = d_internal_sqlite_execute(
        _db,
        "PRAGMA \"%w\".synchronous = %d",
        schema,
        (int)_value);

    // read back what took
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_pragma_int64(_db,
                                                &now,
                                                "PRAGMA \"%w\".synchronous",
                                                schema);
    }

    // a value that did not take, or a PRAGMA that reports nothing
    if ( (result == D_DB_NOT_FOUND) ||
         ( (result == SQLITE_OK) &&
           (now != (int64_t)_value) ) )
    {
        result = D_DB_UNSUPPORTED;
    }

    return result;
}

/*
d_sqlite_get_synchronous
  SQLite's numbers are the enumeration's, so only the range needs checking.
*/
int
d_sqlite_get_synchronous(
    sqlite3*                   _db,
    const char*                _schema,
    enum d_sqlite_synchronous* _value
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!_value) )
    {
        return SQLITE_MISUSE;
    }

    int64_t now    = -1;
    int     result = d_internal_sqlite_pragma_int64(
        _db,
        &now,
        "PRAGMA \"%w\".synchronous",
        d_internal_sqlite_schema(_schema));

    // a value the enumeration does not name, or none at all
    if ( (result == D_DB_NOT_FOUND) ||
         ( (result == SQLITE_OK) &&
           ( (now < (int64_t)D_SQLITE_SYNCHRONOUS_OFF) ||
             (now > (int64_t)D_SQLITE_SYNCHRONOUS_EXTRA) ) ) )
    {
        result = D_DB_UNSUPPORTED;
    }

    // a value only on success
    if (result == SQLITE_OK)
    {
        *_value = (enum d_sqlite_synchronous)now;
    }

    return result;
}

// 3.3    Version numbers
//------------------------------------------------------------------------------
/*
d_sqlite_set_user_version
  A signed 32-bit integer in the header, written with %d.
*/
int
d_sqlite_set_user_version(
    sqlite3*    _db,
    const char* _schema,
    int32_t     _version
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_sqlite_schema_ok(_schema)) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(_db,
                                     "PRAGMA \"%w\".user_version = %d",
                                     d_internal_sqlite_schema(_schema),
                                     (int)_version);
}

/*
d_sqlite_get_user_version
  Read as 64 bits, as every PRAGMA value is, and narrowed: it was written as
32.
*/
int
d_sqlite_get_user_version(
    sqlite3*    _db,
    const char* _schema,
    int32_t*    _version
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!_version) )
    {
        return SQLITE_MISUSE;
    }

    int64_t   now    = 0;
    const int result = d_internal_sqlite_pragma_int64(
        _db,
        &now,
        "PRAGMA \"%w\".user_version",
        d_internal_sqlite_schema(_schema));

    // a value only on success
    if (result == SQLITE_OK)
    {
        *_version = (int32_t)now;
    }

    return result;
}

/*
d_sqlite_set_application_id
  As d_sqlite_set_user_version, at its own offset in the header.
*/
int
d_sqlite_set_application_id(
    sqlite3*    _db,
    const char* _schema,
    int32_t     _id
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_sqlite_schema_ok(_schema)) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(_db,
                                     "PRAGMA \"%w\".application_id = %d",
                                     d_internal_sqlite_schema(_schema),
                                     (int)_id);
}

/*
d_sqlite_get_application_id
  As d_sqlite_get_user_version.
*/
int
d_sqlite_get_application_id(
    sqlite3*    _db,
    const char* _schema,
    int32_t*    _id
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!_id) )
    {
        return SQLITE_MISUSE;
    }

    int64_t   now    = 0;
    const int result = d_internal_sqlite_pragma_int64(
        _db,
        &now,
        "PRAGMA \"%w\".application_id",
        d_internal_sqlite_schema(_schema));

    // a value only on success
    if (result == SQLITE_OK)
    {
        *_id = (int32_t)now;
    }

    return result;
}

//==============================================================================
// 4.  ANY PRAGMA
//==============================================================================

// 4.1    Setting and getting
//------------------------------------------------------------------------------
/*
d_sqlite_set_pragma
  The name is checked to be one and written as is; the value goes in as a
string literal, which SQLite converts for every PRAGMA taking a value.
*/
int
d_sqlite_set_pragma(
    sqlite3*    _db,
    const char* _schema,
    const char* _pragma,
    const char* _value
)
{
    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_sqlite_schema_ok(_schema))   ||
         (!d_internal_sqlite_pragma_name_ok(_pragma)) ||
         (!_value) )
    {
        return SQLITE_MISUSE;
    }

    // unqualified, when no schema is given
    if (!_schema)
    {
        return d_internal_sqlite_execute(_db,
                                         "PRAGMA %s = %Q",
                                         _pragma,
                                         _value);
    }

    return d_internal_sqlite_execute(_db,
                                     "PRAGMA \"%w\".%s = %Q",
                                     _schema,
                                     _pragma,
                                     _value);
}

/*
d_sqlite_get_pragma
  The first column of the first row, as text; an unknown PRAGMA produces no
row at all, which reads as D_DB_NOT_FOUND.
*/
int
d_sqlite_get_pragma(
    sqlite3*    _db,
    const char* _schema,
    const char* _pragma,
    char**      _value
)
{
    // parameter validation
    if ( (!_db)                                       ||
         (!d_internal_sqlite_schema_ok(_schema))      ||
         (!d_internal_sqlite_pragma_name_ok(_pragma)) ||
         (!_value) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    const int     result    =
        (_schema) ? d_internal_sqlite_prepare(_db,
                                              &statement,
                                              "PRAGMA \"%w\".%s",
                                              _schema,
                                              _pragma)
                  : d_internal_sqlite_prepare(_db,
                                              &statement,
                                              "PRAGMA %s",
                                              _pragma);

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_step_text(statement, _value);
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_set_foreign_keys
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_set_foreign_keys(
    struct sqlite3* _db,
    bool            _enabled
)
{
    (void)_db;
    (void)_enabled;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_get_foreign_keys
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_get_foreign_keys(
    struct sqlite3* _db,
    bool*           _enabled
)
{
    (void)_db;
    (void)_enabled;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_set_busy_timeout
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_set_busy_timeout(
    struct sqlite3* _db,
    int             _milliseconds
)
{
    (void)_db;
    (void)_milliseconds;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_get_busy_timeout
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_get_busy_timeout(
    struct sqlite3* _db,
    int*            _milliseconds
)
{
    (void)_db;
    (void)_milliseconds;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_set_journal_mode
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_set_journal_mode(
    struct sqlite3*            _db,
    const char*                _schema,
    enum d_sqlite_journal_mode _mode
)
{
    (void)_db;
    (void)_schema;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_get_journal_mode
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_get_journal_mode(
    struct sqlite3*             _db,
    const char*                 _schema,
    enum d_sqlite_journal_mode* _mode
)
{
    (void)_db;
    (void)_schema;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_set_synchronous
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_set_synchronous(
    struct sqlite3*           _db,
    const char*               _schema,
    enum d_sqlite_synchronous _value
)
{
    (void)_db;
    (void)_schema;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_get_synchronous
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_get_synchronous(
    struct sqlite3*            _db,
    const char*                _schema,
    enum d_sqlite_synchronous* _value
)
{
    (void)_db;
    (void)_schema;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_set_user_version
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_set_user_version(
    struct sqlite3* _db,
    const char*     _schema,
    int32_t         _version
)
{
    (void)_db;
    (void)_schema;
    (void)_version;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_get_user_version
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_get_user_version(
    struct sqlite3* _db,
    const char*     _schema,
    int32_t*        _version
)
{
    (void)_db;
    (void)_schema;
    (void)_version;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_set_application_id
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_set_application_id(
    struct sqlite3* _db,
    const char*     _schema,
    int32_t         _id
)
{
    (void)_db;
    (void)_schema;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_get_application_id
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_get_application_id(
    struct sqlite3* _db,
    const char*     _schema,
    int32_t*        _id
)
{
    (void)_db;
    (void)_schema;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_set_pragma
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_set_pragma(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _pragma,
    const char*     _value
)
{
    (void)_db;
    (void)_schema;
    (void)_pragma;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_get_pragma
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_get_pragma(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _pragma,
    char**          _value
)
{
    (void)_db;
    (void)_schema;
    (void)_pragma;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_SQLITE
