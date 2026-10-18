/*******************************************************************************
* djinterp [c]                                              sqlite_maintenance.c
*
* Implementation of sqlite_maintenance.h.
*   Version-dependent statements are checked against the library loaded, so an
* old one refuses clearly instead of misreading the statement -- VACUUM of a
* schema before 3.15.0 parsed the name and then vacuumed "main". A backup is
* one sqlite3_backup_step(-1) between init and finish: the whole database in
* one pass, under the source's read lock.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_maintenance.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_maintenance.h"  // corresponding header
// std
#include <stddef.h>  // NULL
#include <stdlib.h>  // free
#include <string.h>  // strcmp
// djinterp
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Runs PRAGMA integrity_check or quick_check and reads its verdict.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the database; `NULL` for "main".
 * @param[in]  _pragma  "integrity_check" or "quick_check".
 * @param[out] _ok      receives the verdict; written only on success.
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE.
 *
 * The argument 1 stops the check at its first problem, which is all a verdict
 * needs. A sound database answers with the single row "ok"; anything else is
 * a description of what is wrong.
 */
static int
d_internal_sqlite_check(
    sqlite3*    _db,
    const char* _schema,
    const char* _pragma,
    bool*       _ok
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!_ok) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_stmt* statement = NULL;
    char*         verdict   = NULL;
    int           result    = d_internal_sqlite_prepare(
        _db,
        &statement,
        "PRAGMA \"%w\".%s(1)",
        d_internal_sqlite_schema(_schema),
        _pragma);

    // the verdict is the first row
    if (result == SQLITE_OK)
    {
        result = d_internal_sqlite_step_text(statement, &verdict);
    }

    // a value only on success
    if (result == SQLITE_OK)
    {
        *_ok = ( (verdict) &&
                 (strcmp(verdict, "ok") == 0) );
    }

    free(verdict);

    return result;
}

//==============================================================================
// 1.  UPKEEP
//==============================================================================

// 1.1    Compacting
//------------------------------------------------------------------------------
/*
d_sqlite_vacuum
  From 3.15.0 the schema is named; before it, only "main" can be vacuumed, and
plain VACUUM is the statement that does it.
*/
int
d_sqlite_vacuum(
    sqlite3*    _db,
    const char* _schema
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_sqlite_schema_ok(_schema)) )
    {
        return SQLITE_MISUSE;
    }

    // a named schema, where the library understands one
    if (d_sqlite_supports(D_SQLITE_FEATURE_VACUUM_SCHEMA))
    {
        return d_internal_sqlite_execute(_db,
                                         "VACUUM \"%w\"",
                                         d_internal_sqlite_schema(_schema));
    }

    return (d_internal_sqlite_is_main(_schema))
               ? sqlite3_exec(_db, "VACUUM", NULL, NULL, NULL)
               : D_DB_UNSUPPORTED;
}

/*
d_sqlite_vacuum_into
  VACUUM INTO (3.27.0) writes the copy through the same machinery as VACUUM,
so it is consistent without a backup's page-by-page copy.
*/
int
d_sqlite_vacuum_into(
    sqlite3*    _db,
    const char* _schema,
    const char* _path
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_path)) )
    {
        return SQLITE_MISUSE;
    }

    // VACUUM INTO arrived in 3.27.0
    if (!d_sqlite_supports(D_SQLITE_FEATURE_VACUUM_INTO))
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_sqlite_execute(_db,
                                     "VACUUM \"%w\" INTO %Q",
                                     d_internal_sqlite_schema(_schema),
                                     _path);
}

// 1.2    Statistics
//------------------------------------------------------------------------------
/*
d_sqlite_analyze
  ANALYZE with a schema name analyzes every table in it, on every version.
*/
int
d_sqlite_analyze(
    sqlite3*    _db,
    const char* _schema
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_sqlite_schema_ok(_schema)) )
    {
        return SQLITE_MISUSE;
    }

    return d_internal_sqlite_execute(_db,
                                     "ANALYZE \"%w\"",
                                     d_internal_sqlite_schema(_schema));
}

/*
d_sqlite_optimize
  Before 3.18.0 SQLite ignores the unknown PRAGMA without a word, so the
version is checked rather than the call trusted.
*/
int
d_sqlite_optimize(
    sqlite3*    _db,
    const char* _schema
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_sqlite_schema_ok(_schema)) )
    {
        return SQLITE_MISUSE;
    }

    // PRAGMA optimize arrived in 3.18.0
    if (!d_sqlite_supports(D_SQLITE_FEATURE_OPTIMIZE))
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_sqlite_execute(_db,
                                     "PRAGMA \"%w\".optimize",
                                     d_internal_sqlite_schema(_schema));
}

// 1.3    Checking
//------------------------------------------------------------------------------
/*
d_sqlite_integrity_check
  The thorough check.
*/
int
d_sqlite_integrity_check(
    sqlite3*    _db,
    const char* _schema,
    bool*       _ok
)
{
    return d_internal_sqlite_check(_db, _schema, "integrity_check", _ok);
}

/*
d_sqlite_quick_check
  The fast check.
*/
int
d_sqlite_quick_check(
    sqlite3*    _db,
    const char* _schema,
    bool*       _ok
)
{
    return d_internal_sqlite_check(_db, _schema, "quick_check", _ok);
}

//==============================================================================
// 2.  WRITE-AHEAD LOG
//==============================================================================

// 2.2    Checkpoints
//------------------------------------------------------------------------------
/*
d_sqlite_checkpoint
  The modes are SQLite's numbers, passed through; the TRUNCATE constant is
absent from headers older than 3.8.8, which is why none is named here.
*/
int
d_sqlite_checkpoint(
    sqlite3*                      _db,
    const char*                   _schema,
    enum d_sqlite_checkpoint_mode _mode
)
{
    // parameter validation
    if ( (!_db)                                          ||
         (!d_internal_sqlite_schema_ok(_schema))         ||
         ((int)_mode < (int)D_SQLITE_CHECKPOINT_PASSIVE) ||
         ((int)_mode > (int)D_SQLITE_CHECKPOINT_TRUNCATE) )
    {
        return SQLITE_MISUSE;
    }

    // TRUNCATE arrived in 3.8.8
    if ( (_mode == D_SQLITE_CHECKPOINT_TRUNCATE) &&
         (!d_internal_sqlite_runtime_at_least(
             D_ENV_SQLITE_ENCODE_VERSION(3, 8, 8))) )
    {
        return D_DB_UNSUPPORTED;
    }

    return sqlite3_wal_checkpoint_v2(_db,
                                     d_internal_sqlite_schema(_schema),
                                     (int)_mode,
                                     NULL,
                                     NULL);
}

//==============================================================================
// 3.  BACKUPS
//==============================================================================

// 3.1    Copying databases
//------------------------------------------------------------------------------
/*
d_sqlite_copy_database
  sqlite3_backup_init reports its failure on the destination connection, not
in a return code. A step of -1 copies every page at once, ending in
SQLITE_DONE; finish then reports any error the step met, and releases the
backup either way.
*/
int
d_sqlite_copy_database(
    sqlite3*    _source,
    const char* _source_schema,
    sqlite3*    _destination,
    const char* _destination_schema
)
{
    // parameter validation
    if ( (!_source)                                          ||
         (!d_internal_sqlite_schema_ok(_source_schema))      ||
         (!_destination)                                     ||
         (!d_internal_sqlite_schema_ok(_destination_schema)) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3_backup* const backup =
        sqlite3_backup_init(_destination,
                            d_internal_sqlite_schema(_destination_schema),
                            _source,
                            d_internal_sqlite_schema(_source_schema));

    // the reason is on the destination connection
    if (!backup)
    {
        return sqlite3_errcode(_destination);
    }

    const int step     = sqlite3_backup_step(backup, -1);
    const int finished = sqlite3_backup_finish(backup);

    return (step == SQLITE_DONE) ? finished : step;
}

/*
d_sqlite_backup_to_file
  The file is opened as a database in its own right and the copy goes into its
"main"; it is closed whatever happens.
*/
int
d_sqlite_backup_to_file(
    sqlite3*    _db,
    const char* _schema,
    const char* _path
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_path)) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3* file   = NULL;
    int      result = sqlite3_open_v2(_path,
                                      &file,
                                      SQLITE_OPEN_READWRITE |
                                      SQLITE_OPEN_CREATE,
                                      NULL);

    // copy into the file once it is open
    if (result == SQLITE_OK)
    {
        result = d_sqlite_copy_database(_db, _schema, file, "main");
    }

    sqlite3_close(file);

    return result;
}

/*
d_sqlite_restore_from_file
  The file is opened read-only, so a restore can never change it.
*/
int
d_sqlite_restore_from_file(
    sqlite3*    _db,
    const char* _schema,
    const char* _path
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_sqlite_schema_ok(_schema)) ||
         (!d_internal_sqlite_name_ok(_path)) )
    {
        return SQLITE_MISUSE;
    }

    sqlite3* file   = NULL;
    int      result = sqlite3_open_v2(_path,
                                      &file,
                                      SQLITE_OPEN_READONLY,
                                      NULL);

    // copy out of the file once it is open
    if (result == SQLITE_OK)
    {
        result = d_sqlite_copy_database(file, "main", _db, _schema);
    }

    sqlite3_close(file);

    return result;
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_vacuum
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_vacuum(
    struct sqlite3* _db,
    const char*     _schema
)
{
    (void)_db;
    (void)_schema;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_vacuum_into
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_vacuum_into(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _path
)
{
    (void)_db;
    (void)_schema;
    (void)_path;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_analyze
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_analyze(
    struct sqlite3* _db,
    const char*     _schema
)
{
    (void)_db;
    (void)_schema;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_optimize
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_optimize(
    struct sqlite3* _db,
    const char*     _schema
)
{
    (void)_db;
    (void)_schema;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_integrity_check
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_integrity_check(
    struct sqlite3* _db,
    const char*     _schema,
    bool*           _ok
)
{
    (void)_db;
    (void)_schema;
    (void)_ok;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_quick_check
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_quick_check(
    struct sqlite3* _db,
    const char*     _schema,
    bool*           _ok
)
{
    (void)_db;
    (void)_schema;
    (void)_ok;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_checkpoint
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_checkpoint(
    struct sqlite3*               _db,
    const char*                   _schema,
    enum d_sqlite_checkpoint_mode _mode
)
{
    (void)_db;
    (void)_schema;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_copy_database
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_copy_database(
    struct sqlite3* _source,
    const char*     _source_schema,
    struct sqlite3* _destination,
    const char*     _destination_schema
)
{
    (void)_source;
    (void)_source_schema;
    (void)_destination;
    (void)_destination_schema;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_backup_to_file
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_backup_to_file(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _path
)
{
    (void)_db;
    (void)_schema;
    (void)_path;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_restore_from_file
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_restore_from_file(
    struct sqlite3* _db,
    const char*     _schema,
    const char*     _path
)
{
    (void)_db;
    (void)_schema;
    (void)_path;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_SQLITE
