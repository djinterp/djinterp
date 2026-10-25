/*******************************************************************************
* djinterp [c]                                               sqlite_connection.c
*
* Implementation of sqlite_connection.h.
*   Every open goes through sqlite3_open_v2, which allocates a handle even
* when it fails; the handle is closed here on failure, so a caller is left
* with NULL and nothing to release. The defaults are applied with run-time
* branches on the resolved knobs, so both sides always compile.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_connection.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.21
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_connection.h"  // corresponding header
// std
#include <stddef.h>  // NULL, size_t
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy, strlen
// djinterp
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_connection.h"  // defaults
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Opens a connection with the given flags and applies the defaults,
 *        or leaves nothing behind.
 *
 * @param[in]  _path   the file, in UTF-8.
 * @param[in]  _flags  SQLITE_OPEN_* flags.
 * @param[in]  _vfs    the VFS's name, or `NULL` for the default.
 * @param[out] _db     receives the connection, or `NULL`.
 * @return SQLITE_OK, or the result code of the open or of the default that
 *         failed.
 *
 * With extended result codes configured, the code returned for a failed open
 * is the extended one, read from the handle before it is closed: the
 * connection the caller would have used is the one that reports in that mode.
 */
static int
d_internal_sqlite_open(
    const char* _path,
    int         _flags,
    const char* _vfs,
    sqlite3**   _db
)
{
    sqlite3* handle = NULL;
    int      result = sqlite3_open_v2(_path, &handle, _flags, _vfs);

    // set the new connection up the way the layer promises
    if (result == SQLITE_OK)
    {
        result = d_sqlite_apply_defaults(handle);
    }

    // a failure leaves no connection behind
    if (result != SQLITE_OK)
    {
        // the extended code, when the build asked for those
        if ( (D_INTERNAL_SQLITE_OPEN_EXTENDED_CODES) &&
             (handle) )
        {
            result = sqlite3_extended_errcode(handle);
        }

        sqlite3_close(handle);
        *_db = NULL;

        return result;
    }

    *_db = handle;

    return SQLITE_OK;
}

//==============================================================================
// 1.  CONNECTIONS
//==============================================================================

// 1.1    Opening and closing
//------------------------------------------------------------------------------
/*
d_sqlite_open
  READWRITE | CREATE is sqlite3_open's own behaviour, made explicit.
*/
int
d_sqlite_open(
    const char* _path,
    sqlite3**   _db
)
{
    // parameter validation
    if ( (!_path) ||
         (!_db) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_open(_path,
                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                  NULL,
                                  _db);
}

/*
d_sqlite_open_readonly
  READONLY without CREATE: a missing file is SQLITE_CANTOPEN, not a new one.
*/
int
d_sqlite_open_readonly(
    const char* _path,
    sqlite3**   _db
)
{
    // parameter validation
    if ( (!_path) ||
         (!_db) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_open(_path, SQLITE_OPEN_READONLY, NULL, _db);
}

/*
d_sqlite_open_memory
  ":memory:" opens a database private to this connection on every version;
SQLITE_OPEN_MEMORY, the flag form, arrived only in 3.7.13.
*/
int
d_sqlite_open_memory(
    sqlite3** _db
)
{
    // parameter validation
    if (!_db)
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_open(":memory:",
                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                  NULL,
                                  _db);
}

/*
d_sqlite_open_with
  The flags are checked here, since sqlite3_open_v2 leaves a combination
outside its list undefined: one way of opening, no unknown bit, and not both
of a pair. A flag newer than the library loaded is refused rather than
passed, which an older library would take silently and ignore.
*/
int
d_sqlite_open_with(
    const char* _path,
    unsigned    _flags,
    const char* _vfs,
    sqlite3**   _db
)
{
    const unsigned known  = D_SQLITE_OPEN_READONLY     |
                            D_SQLITE_OPEN_READWRITE    |
                            D_SQLITE_OPEN_CREATE       |
                            D_SQLITE_OPEN_URI          |
                            D_SQLITE_OPEN_MEMORY       |
                            D_SQLITE_OPEN_NOMUTEX      |
                            D_SQLITE_OPEN_FULLMUTEX    |
                            D_SQLITE_OPEN_SHAREDCACHE  |
                            D_SQLITE_OPEN_PRIVATECACHE |
                            D_SQLITE_OPEN_NOFOLLOW;
    const unsigned access = _flags & (D_SQLITE_OPEN_READONLY  |
                                      D_SQLITE_OPEN_READWRITE |
                                      D_SQLITE_OPEN_CREATE);
    const unsigned mutex  = D_SQLITE_OPEN_NOMUTEX | D_SQLITE_OPEN_FULLMUTEX;
    const unsigned cache  = D_SQLITE_OPEN_SHAREDCACHE |
                            D_SQLITE_OPEN_PRIVATECACHE;

    // a failed open leaves NULL behind, the arguments' failures included
    if (_db)
    {
        *_db = NULL;
    }

    // parameter validation: the arguments, then the flags
    if ( (!_path)                                          ||
         (!_db)                                            ||
         ((_flags & ~known) != 0u)                         ||
         ( (access != D_SQLITE_OPEN_READONLY)  &&
           (access != D_SQLITE_OPEN_READWRITE) &&
           (access != (D_SQLITE_OPEN_READWRITE |
                       D_SQLITE_OPEN_CREATE)) )            ||
         ((_flags & mutex) == mutex)                       ||
         ((_flags & cache) == cache) )
    {
        return SQLITE_MISUSE;
    }

    // NOFOLLOW only where the library knows it
    if ( ((_flags & D_SQLITE_OPEN_NOFOLLOW) != 0u) &&
         (!d_internal_sqlite_runtime_at_least(3031000)) )
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_sqlite_open(_path, (int)_flags, _vfs, _db);
}

/*
d_sqlite_apply_defaults
  The knobs are resolved constants, read as ordinary conditions so both
branches compile in every configuration. Foreign keys go last: their PRAGMA
is the only step that can fail, and its failure is then the one reported.
*/
int
d_sqlite_apply_defaults(
    sqlite3* _db
)
{
    // parameter validation
    if (!_db)
    {
        return SQLITE_MISUSE;
    }

    // result codes first, so every later failure reports in the chosen mode
    if (D_INTERNAL_SQLITE_OPEN_EXTENDED_CODES)
    {
        sqlite3_extended_result_codes(_db, 1);
    }

    // wait for other connections' locks rather than failing at once
    if (D_INTERNAL_SQLITE_OPEN_BUSY_TIMEOUT_MS > 0)
    {
        sqlite3_busy_timeout(_db, (int)D_INTERNAL_SQLITE_OPEN_BUSY_TIMEOUT_MS);
    }

    // enforce the REFERENCES clauses the schema declares
    if (D_INTERNAL_SQLITE_OPEN_FOREIGN_KEYS)
    {
        return sqlite3_exec(_db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
    }

    return SQLITE_OK;
}

/*
d_sqlite_close
  sqlite3_close_v2 (3.7.14) defers the close until the last statement is
finalized instead of failing with SQLITE_BUSY, which is what a convenience
close should do.
*/
int
d_sqlite_close(
    sqlite3* _db
)
{
    return sqlite3_close_v2(_db);
}

// 1.2    Location and access
//------------------------------------------------------------------------------
/*
d_sqlite_database_path
  sqlite3_db_filename answers NULL both for a database that is not attached
and, on some versions, for one held in memory. sqlite3_db_readonly tells them
apart: it is -1 only for a name that is not attached.
*/
const char*
d_sqlite_database_path(
    sqlite3*    _db,
    const char* _schema
)
{
    // parameter validation
    if (!_db)
    {
        return NULL;
    }

    const char* const schema = d_internal_sqlite_schema(_schema);
    const char* const path   = sqlite3_db_filename(_db, schema);

    // a file, or "" for a database in memory on newer versions
    if (path)
    {
        return path;
    }

    return (sqlite3_db_readonly(_db, schema) >= 0) ? "" : NULL;
}

/*
d_sqlite_is_readonly
  sqlite3_db_readonly (3.7.11) is 1, 0, or -1 for no such database.
*/
bool
d_sqlite_is_readonly(
    sqlite3*    _db,
    const char* _schema
)
{
    // parameter validation
    if (!_db)
    {
        return false;
    }

    return (sqlite3_db_readonly(_db, d_internal_sqlite_schema(_schema)) == 1);
}

// 1.3    Rows changed
//------------------------------------------------------------------------------
/*
d_sqlite_last_insert_id
  SQLite's rowid is 64-bit on every version.
*/
int64_t
d_sqlite_last_insert_id(
    sqlite3* _db
)
{
    // parameter validation
    if (!_db)
    {
        return 0;
    }

    return (int64_t)sqlite3_last_insert_rowid(_db);
}

/*
d_sqlite_changes
  sqlite3_changes is an int and wraps past 2^31 rows; sqlite3_changes64
(3.37.0) does not, and D_INTERNAL_SQLITE_CHANGES is the newer where the header
has it.
*/
int64_t
d_sqlite_changes(
    sqlite3* _db
)
{
    // parameter validation
    if (!_db)
    {
        return 0;
    }

    return (int64_t)D_INTERNAL_SQLITE_CHANGES(_db);
}

/*
d_sqlite_total_changes
  As d_sqlite_changes, over the connection's lifetime.
*/
int64_t
d_sqlite_total_changes(
    sqlite3* _db
)
{
    // parameter validation
    if (!_db)
    {
        return 0;
    }

    return (int64_t)D_INTERNAL_SQLITE_TOTAL_CHANGES(_db);
}

//==============================================================================
// 2.  EXTENSIONS
//==============================================================================

// 2.1    Loading
//------------------------------------------------------------------------------
/*
d_sqlite_load_extension
  From 3.13.0 the C API has a switch of its own, which is read, turned on for
the call, and put back as it was; SQL's load_extension() has the other switch,
which only d_sqlite_enable_load_extension touches. SQLite's message is in its
own memory, and is copied into the C runtime's, for d_db_free.
*/
int
d_sqlite_load_extension(
    sqlite3*    _db,
    const char* _path,
    const char* _entry_point,
    char**      _message
)
{
    // nothing said yet
    if (_message)
    {
        *_message = NULL;
    }

    // parameter validation
    if ( (!_db) ||
         (!_path) )
    {
        return SQLITE_MISUSE;
    }

    // a library, and a build, with it
    if (!d_sqlite_supports(D_SQLITE_FEATURE_LOAD_EXTENSION))
    {
        return D_DB_UNSUPPORTED;
    }

#if (D_INTERNAL_SQLITE_LOAD_EXTENSION == 1)
    char* error  = NULL;
    int   result = SQLITE_OK;

#if defined(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION)
    int was = 1;  // the C API's switch, as the call found it

    // the C API's own switch, on for this call
    if (d_internal_sqlite_runtime_at_least(3013000))
    {
        result = sqlite3_db_config(_db,
                                   SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION,
                                   -1,
                                   &was);

        if ( (result == SQLITE_OK) &&
             (was == 0) )
        {
            result = sqlite3_db_config(_db,
                                       SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION,
                                       1,
                                       (int*)NULL);
        }

        if (result != SQLITE_OK)
        {
            return result;
        }
    }
#endif  // SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION

    result = sqlite3_load_extension(_db, _path, _entry_point, &error);

#if defined(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION)
    // the switch as it was
    if (was == 0)
    {
        sqlite3_db_config(_db,
                          SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION,
                          0,
                          (int*)NULL);
    }
#endif  // SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION

    // SQLite's words, in memory d_db_free releases
    if ( (result != SQLITE_OK) &&
         (error)               &&
         (_message) )
    {
        const size_t length = strlen(error);

        *_message = (char*)malloc(length + 1u);

        if (*_message)
        {
            memcpy(*_message, error, length + 1u);
        }
    }

    sqlite3_free(error);

    return result;
#else
    (void)_entry_point;

    return D_DB_UNSUPPORTED;
#endif  // D_INTERNAL_SQLITE_LOAD_EXTENSION
}

// 2.2    Allowing SQL to load
//------------------------------------------------------------------------------
/*
d_sqlite_enable_load_extension
  SQLite's own switch, for SQL and the C API together.
*/
int
d_sqlite_enable_load_extension(
    sqlite3* _db,
    bool     _enabled
)
{
    // parameter validation
    if (!_db)
    {
        return SQLITE_MISUSE;
    }

    // a library, and a build, with it
    if (!d_sqlite_supports(D_SQLITE_FEATURE_LOAD_EXTENSION))
    {
        return D_DB_UNSUPPORTED;
    }

#if (D_INTERNAL_SQLITE_LOAD_EXTENSION == 1)
    return sqlite3_enable_load_extension(_db, (_enabled) ? 1 : 0);
#else
    (void)_enabled;

    return D_DB_UNSUPPORTED;
#endif  // D_INTERNAL_SQLITE_LOAD_EXTENSION
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_open
  Without SQLite there is no connection to open: D_DB_UNSUPPORTED, and the
handle NULL, as a failed open leaves it.
*/
int
d_sqlite_open(
    const char*      _path,
    struct sqlite3** _db
)
{
    (void)_path;

    // a failed open leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_open_readonly
  Without SQLite there is no connection to open: D_DB_UNSUPPORTED, and the
handle NULL, as a failed open leaves it.
*/
int
d_sqlite_open_readonly(
    const char*      _path,
    struct sqlite3** _db
)
{
    (void)_path;

    // a failed open leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_open_memory
  Without SQLite there is no connection to open: D_DB_UNSUPPORTED, and the
handle NULL, as a failed open leaves it.
*/
int
d_sqlite_open_memory(
    struct sqlite3** _db
)
{

    // a failed open leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_open_with
  Without SQLite there is no connection to open: D_DB_UNSUPPORTED, and the
handle NULL, as a failed open leaves it.
*/
int
d_sqlite_open_with(
    const char*      _path,
    unsigned         _flags,
    const char*      _vfs,
    struct sqlite3** _db
)
{
    (void)_path;
    (void)_flags;
    (void)_vfs;

    // a failed open leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_apply_defaults
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_apply_defaults(
    struct sqlite3* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_close
  Without SQLite no connection exists: NULL closes as it always does, and
anything else is no connection.
*/
int
d_sqlite_close(
    struct sqlite3* _db
)
{
    return (_db) ? D_DB_UNSUPPORTED : D_DB_OK;
}

/*
d_sqlite_database_path
  Without SQLite, NULL.
*/
const char*
d_sqlite_database_path(
    struct sqlite3* _db,
    const char*     _schema
)
{
    (void)_db;
    (void)_schema;

    return NULL;
}

/*
d_sqlite_is_readonly
  Without SQLite, false.
*/
bool
d_sqlite_is_readonly(
    struct sqlite3* _db,
    const char*     _schema
)
{
    (void)_db;
    (void)_schema;

    return false;
}

/*
d_sqlite_last_insert_id
  Without SQLite there are no rows: 0.
*/
int64_t
d_sqlite_last_insert_id(
    struct sqlite3* _db
)
{
    (void)_db;

    return 0;
}

/*
d_sqlite_changes
  Without SQLite there are no rows: 0.
*/
int64_t
d_sqlite_changes(
    struct sqlite3* _db
)
{
    (void)_db;

    return 0;
}

/*
d_sqlite_total_changes
  Without SQLite there are no rows: 0.
*/
int64_t
d_sqlite_total_changes(
    struct sqlite3* _db
)
{
    (void)_db;

    return 0;
}

/*
d_sqlite_load_extension
  Without SQLite, D_DB_UNSUPPORTED; nothing said.
*/
int
d_sqlite_load_extension(
    struct sqlite3* _db,
    const char*     _path,
    const char*     _entry_point,
    char**          _message
)
{
    (void)_db;
    (void)_path;
    (void)_entry_point;

    if (_message)
    {
        *_message = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_enable_load_extension
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_enable_load_extension(
    struct sqlite3* _db,
    bool            _enabled
)
{
    (void)_db;
    (void)_enabled;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_SQLITE
