/*******************************************************************************
* djinterp [c]                                              sqlite_transaction.c
*
* Implementation of sqlite_transaction.h.
*   Each function is one statement. Whether a transaction is open is
* sqlite3_get_autocommit's answer, which follows SQLite's own state and so
* sees transactions begun natively too.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_transaction.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_SQLITE_SOURCE
//   macro: this file is one of the SQLite layer's own sources, for which
// cfg_sqlite_common.h has env include sqlite3.h; it precedes every include.
#define D_INTERNAL_SQLITE_SOURCE 1

#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_transaction.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../inc/djinterp/config/c/db/sqlite/cfg_sqlite_common.h"  // D_INTERNAL_SQLITE
#include "./sqlite_internal.h"  // sqlite3.h, the floor, the plumbing


#if (D_INTERNAL_SQLITE == 1)

//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================

// 1.2    Beginning and ending
//------------------------------------------------------------------------------
/*
d_sqlite_begin_transaction
  The mode is spelled out even for DEFERRED, SQLite's default, so the
statement says what it does.
*/
int
d_sqlite_begin_transaction(
    sqlite3*                       _db,
    enum d_sqlite_transaction_mode _mode
)
{
    // parameter validation
    if (!_db)
    {
        return SQLITE_MISUSE;
    }

    switch (_mode)
    {
        case D_SQLITE_TRANSACTION_DEFERRED:
            return sqlite3_exec(_db, "BEGIN DEFERRED", NULL, NULL, NULL);

        case D_SQLITE_TRANSACTION_IMMEDIATE:
            return sqlite3_exec(_db, "BEGIN IMMEDIATE", NULL, NULL, NULL);

        case D_SQLITE_TRANSACTION_EXCLUSIVE:
            return sqlite3_exec(_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);

        default:
            break;
    }

    return SQLITE_MISUSE;
}

/*
d_sqlite_commit
  COMMIT; on SQLITE_BUSY SQLite leaves the transaction open for a retry.
*/
int
d_sqlite_commit(
    sqlite3* _db
)
{
    // parameter validation
    if (!_db)
    {
        return SQLITE_MISUSE;
    }

    return sqlite3_exec(_db, "COMMIT", NULL, NULL, NULL);
}

/*
d_sqlite_rollback
  ROLLBACK, which also closes every savepoint.
*/
int
d_sqlite_rollback(
    sqlite3* _db
)
{
    // parameter validation
    if (!_db)
    {
        return SQLITE_MISUSE;
    }

    return sqlite3_exec(_db, "ROLLBACK", NULL, NULL, NULL);
}

// 1.3    State
//------------------------------------------------------------------------------
/*
d_sqlite_in_transaction
  Autocommit is off exactly while a transaction is open.
*/
bool
d_sqlite_in_transaction(
    sqlite3* _db
)
{
    // parameter validation
    if (!_db)
    {
        return false;
    }

    return (sqlite3_get_autocommit(_db) == 0);
}

//==============================================================================
// 2.  SAVEPOINTS
//==============================================================================

// 2.1    Savepoints
//------------------------------------------------------------------------------
/*
d_sqlite_create_savepoint
  SAVEPOINT, its name quoted.
*/
int
d_sqlite_create_savepoint(
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

    return d_internal_sqlite_execute(_db, "SAVEPOINT \"%w\"", _name);
}

/*
d_sqlite_release_savepoint
  RELEASE, which also releases every savepoint opened after this one.
*/
int
d_sqlite_release_savepoint(
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

    return d_internal_sqlite_execute(_db, "RELEASE SAVEPOINT \"%w\"", _name);
}

/*
d_sqlite_rollback_to_savepoint
  ROLLBACK TO undoes back to the savepoint and leaves it open.
*/
int
d_sqlite_rollback_to_savepoint(
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

    return d_internal_sqlite_execute(_db,
                                     "ROLLBACK TO SAVEPOINT \"%w\"",
                                     _name);
}

#else

//==============================================================================
// WITHOUT SQLITE
//==============================================================================
// Built as stubs (D_CFG_SQLITE 0), each function reports what a build without
// SQLite can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_sqlite_begin_transaction
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_begin_transaction(
    struct sqlite3*                _db,
    enum d_sqlite_transaction_mode _mode
)
{
    (void)_db;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_commit
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_commit(
    struct sqlite3* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_rollback
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_rollback(
    struct sqlite3* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_in_transaction
  Without SQLite, false.
*/
bool
d_sqlite_in_transaction(
    struct sqlite3* _db
)
{
    (void)_db;

    return false;
}

/*
d_sqlite_create_savepoint
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_create_savepoint(
    struct sqlite3* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_release_savepoint
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_release_savepoint(
    struct sqlite3* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_sqlite_rollback_to_savepoint
  Without SQLite, D_DB_UNSUPPORTED.
*/
int
d_sqlite_rollback_to_savepoint(
    struct sqlite3* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_SQLITE
