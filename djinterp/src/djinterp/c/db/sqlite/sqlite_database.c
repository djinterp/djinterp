/*******************************************************************************
* djinterp [c]                                                 sqlite_database.c
*
* Implementation of sqlite_database.h.
*   Whether a database is attached is sqlite3_db_readonly's question: it is -1
* exactly when the name is not attached, which needs no statement at all.
* Files are created by opening them, recognised by reading their catalogue,
* and deleted through the default VFS -- the same calls SQLite itself makes,
* so a path means what it means to SQLite on every platform.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_database.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_database.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_IF_*, D_DB_*
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_SQLITE_CATALOGUE_READ
//   constant: a statement that reads a database's catalogue, which is what
// makes SQLite look at the file: an open alone reads nothing.
#define D_INTERNAL_SQLITE_CATALOGUE_READ "SELECT count(*) FROM sqlite_master"

// D_INTERNAL_SQLITE_SUFFIX_ROOM
//   constant: the length of the longest suffix SQLite gives a database's
// companion files, "-journal".
#define D_INTERNAL_SQLITE_SUFFIX_ROOM 8u

/**
 * @brief Deletes one file through a VFS, treating "not there" as success.
 *
 * @param[in]  _vfs      the VFS.
 * @param[in]  _path     the file's full path, as xFullPathname made it.
 * @param[out] _missing  set to whether the file was not there; may be `NULL`.
 * @return SQLITE_OK, or the VFS's result code for a failure to delete.
 *
 * Both of SQLite's own VFSs, unix and win32, report a missing file as
 * SQLITE_IOERR_DELETE_NOENT, and have since before the layer's floor.
 */
static int
d_internal_sqlite_delete_file(
    sqlite3_vfs* _vfs,
    const char*  _path,
    bool*        _missing
)
{
    const int result = _vfs->xDelete(_vfs, _path, 0);

    // report whether there was anything to delete
    if (_missing)
    {
        *_missing = (result == SQLITE_IOERR_DELETE_NOENT);
    }

    return (result == SQLITE_IOERR_DELETE_NOENT) ? SQLITE_OK : result;
}

/**
 * @brief Deletes the files SQLite keeps beside a database -- its rollback
 *        journal, write-ahead log and shared-memory index -- where they exist.
 *
 * @param[in]     _vfs   the VFS.
 * @param[in,out] _path  the database's full path, in a buffer with room for
 *                       D_INTERNAL_SQLITE_SUFFIX_ROOM more characters; each
 *                       suffix is written after it in turn, and the path is
 *                       restored before returning.
 * @return SQLITE_OK, or the VFS's result code for the first file it failed to
 *         delete, the rest then left alone.
 */
static int
d_internal_sqlite_delete_companions(
    sqlite3_vfs* _vfs,
    char*        _path
)
{
    const char* const suffixes[] = { "-journal", "-wal", "-shm" };
    const size_t      count      = sizeof(suffixes) / sizeof(suffixes[0]);
    const size_t      length     = strlen(_path);
    int               result     = SQLITE_OK;

    // each in turn, until one fails
    for (size_t i = 0u; (i < count) && (result == SQLITE_OK); ++i)
    {
        memcpy(_path + length, suffixes[i], strlen(suffixes[i]) + 1u);
        result = d_internal_sqlite_delete_file(_vfs, _path, NULL);
    }

    _path[length] = '\0';

    return result;
}

//==============================================================================
// 1.  ATTACHED DATABASES
//==============================================================================

// 1.1    Existence
//------------------------------------------------------------------------------
/*
d_sqlite_has_database
  The _s form, with every failure read as "no".
*/
bool
d_sqlite_has_database(
    sqlite3*    _db,
    const char* _name
)
{
    bool exists = false;

    return ( (d_sqlite_has_database_s(_db, _name, &exists) == SQLITE_OK) &&
             (exists) );
}

/*
d_sqlite_has_database_s
  "main" always exists, and "temp" can always be written to even before
SQLite has created its file, so both answer yes without asking. For any other
name, sqlite3_db_readonly (3.7.11) is -1 exactly when it is not attached.
*/
int
d_sqlite_has_database_s(
    sqlite3*    _db,
    const char* _name,
    bool*       _exists
)
{
    // parameter validation
    if ( (!_exists)                          ||
         (!_db)                              ||
         (!d_internal_sqlite_name_ok(_name)) )
    {
        // a failure answers no, where there is somewhere to say it
        if (_exists)
        {
            *_exists = false;
        }

        return SQLITE_MISUSE;
    }

    *_exists = ( (sqlite3_stricmp(_name, "main") == 0) ||
                 (sqlite3_stricmp(_name, "temp") == 0) ||
                 (sqlite3_db_readonly(_db, _name) >= 0) );

    return SQLITE_OK;
}

// 1.2    Attaching and detaching
//------------------------------------------------------------------------------
/*
d_sqlite_attach_database
  The path is a string literal and the name an identifier, each quoted for
what it is.
*/
int
d_sqlite_attach_database(
    sqlite3*    _db,
    const char* _path,
    const char* _name
)
{
    // parameter validation
    if ( (!_db)                              ||
         (!_path)                            ||
         (!d_internal_sqlite_name_ok(_name)) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(_db,
                                     "ATTACH DATABASE %Q AS \"%w\"",
                                     _path,
                                     _name);
}

/*
d_sqlite_detach_database
  SQLite itself refuses "main" and "temp", with its own message.
*/
int
d_sqlite_detach_database(
    sqlite3*    _db,
    const char* _name
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_sqlite_name_ok(_name)) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(_db, "DETACH DATABASE \"%w\"", _name);
}

// 1.3    Listing
//------------------------------------------------------------------------------
/*
d_sqlite_list_databases
  PRAGMA database_list has one row per database, the name in column 1, in
the order SQLite numbers them.
*/
int
d_sqlite_list_databases(
    sqlite3*               _db,
    struct d_db_name_list* _names
)
{
    // parameter validation
    if ( (!_db) ||
         (!_names) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    const int     result    = d_internal_sqlite_prepare(_db,
                                                        &statement,
                                                        "PRAGMA database_list");

    // a statement that did not compile has nothing to step
    if (result != SQLITE_OK)
    {
        return result;
    }

    return d_internal_sqlite_collect_names(statement, 1, _names);
}

//==============================================================================
// 2.  DATABASE FILES
//==============================================================================

// 2.1    Creating and deleting
//------------------------------------------------------------------------------
/*
d_sqlite_create_database_file
  A read-write open without CREATE succeeds only for a file that is there, so
it answers "does it exist" with SQLite's notion of the path. A new file is
created by opening with CREATE and then committing an empty transaction,
which makes SQLite write the header: an open alone leaves no file, or an empty
one, behind.
*/
int
d_sqlite_create_database_file(
    const char* _path,
    unsigned    _flags
)
{
    // parameter validation
    if ( (!d_internal_sqlite_name_ok(_path)) ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3* db     = NULL;
    int      result = sqlite3_open_v2(_path, &db, SQLITE_OPEN_READWRITE, NULL);

    // the file is there: refuse it, or accept it if it is a database
    if (result == SQLITE_OK)
    {
        result = (_flags & D_DB_IF_NOT_EXISTS)
                     ? sqlite3_exec(db,
                                    D_INTERNAL_SQLITE_CATALOGUE_READ,
                                    NULL,
                                    NULL,
                                    NULL)
                     : D_DB_EXISTS;
        sqlite3_close(db);

        return result;
    }

    // a failed open still allocated a handle
    sqlite3_close(db);
    db     = NULL;
    result = sqlite3_open_v2(_path,
                             &db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                             NULL);

    // write the header, so the file exists and is recognisably a database
    if (result == SQLITE_OK)
    {
        result = sqlite3_exec(db, "BEGIN IMMEDIATE; COMMIT", NULL, NULL, NULL);
    }

    sqlite3_close(db);

    return result;
}

/*
d_sqlite_delete_database_file
  xFullPathname turns the path into the absolute form SQLite would use, in a
buffer with room for the companions' suffixes after it. The main file goes
first, so an error deleting it leaves its companions -- which may hold its
only copy of committed changes -- untouched.
*/
int
d_sqlite_delete_database_file(
    const char* _path,
    unsigned    _flags
)
{
    // parameter validation
    if ( (!d_internal_sqlite_name_ok(_path)) ||
         ((_flags & ~D_DB_IF_EXISTS) != 0u) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_vfs* const vfs = sqlite3_vfs_find(NULL);

    // no default VFS means no files at all
    if (!vfs)
    {
        return SQLITE_ERROR;
    }

    const size_t capacity = (size_t)vfs->mxPathname + 1u;
    char* const  full     =
        (char*)malloc(capacity + D_INTERNAL_SQLITE_SUFFIX_ROOM);

    // allocation failure
    if (!full)
    {
        return SQLITE_NOMEM;
    }

    bool missing = false;
    int  result  = vfs->xFullPathname(vfs, _path, (int)capacity, full);

    // the file itself
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_delete_file(vfs, full, &missing);
    }

    // a file that was not there is an error unless the caller allowed it
    if ( (result == SQLITE_OK) &&
         (missing)             &&
         (!(_flags & D_DB_IF_EXISTS)) )
    {
        result = D_DB_NOT_FOUND;
    }

    // then its companions
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_delete_companions(vfs, full);
    }

    free(full);

    return result;
}

// 2.2    Recognising
//------------------------------------------------------------------------------
/*
d_sqlite_is_database_file
  Read-only, so a check never creates or changes a file. ":memory:" names no
file, and is answered before SQLite would happily open one.
*/
bool
d_sqlite_is_database_file(
    const char* _path
)
{
    // parameter validation
    if ( (!d_internal_sqlite_name_ok(_path)) ||
         (strcmp(_path, ":memory:") == 0) )
    {
        return false;
    }

    sqlite3* db     = NULL;
    int      result = sqlite3_open_v2(_path, &db, SQLITE_OPEN_READONLY, NULL);

    // reading the catalogue is what makes SQLite judge the file
    if (result == SQLITE_OK)
    {
        result = sqlite3_exec(db,
                              D_INTERNAL_SQLITE_CATALOGUE_READ,
                              NULL,
                              NULL,
                              NULL);
    }

    sqlite3_close(db);

    return (result == SQLITE_OK);
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_has_database
  Without SQLite, false.
*/
bool
d_sqlite_has_database(
    struct sqlite3* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return false;
}

/*
d_sqlite_has_database_s
  Without SQLite there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_sqlite_has_database_s(
    struct sqlite3* _db,
    const char*     _name,
    bool*           _exists
)
{
    (void)_db;
    (void)_name;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_attach_database
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_attach_database(
    struct sqlite3* _db,
    const char*     _path,
    const char*     _name
)
{
    (void)_db;
    (void)_path;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_detach_database
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_detach_database(
    struct sqlite3* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_list_databases
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_list_databases(
    struct sqlite3*        _db,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_create_database_file
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_create_database_file(
    const char* _path,
    unsigned    _flags
)
{
    (void)_path;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_delete_database_file
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_delete_database_file(
    const char* _path,
    unsigned    _flags
)
{
    (void)_path;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_is_database_file
  Without SQLite, false.
*/
bool
d_sqlite_is_database_file(
    const char* _path
)
{
    (void)_path;

    return false;
}

#endif  // D_INTERNAL_SQLITE
