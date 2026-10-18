/*******************************************************************************
* djinterp [c]                                             mariadb_transaction.c
*
* Implementation of mariadb_transaction.h.
*   Every verb is the servers' own statement. Whether a transaction is open is
* the status flag the server sends with each reply, which the client keeps:
* no statement is needed to ask.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_transaction.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_transaction.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/*
d_internal_mariadb_savepoint
  The three savepoint statements differ only in their words, the name always
quoted.
*/
static int
d_internal_mariadb_savepoint(
    struct d_mariadb* _db,
    const char*       _verb,
    const char*       _name
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_mariadb_name_ok(_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db, NULL, "%s %I", _verb, _name);
}

//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================

// 1.2    Beginning and ending
//------------------------------------------------------------------------------
/*
d_mariadb_begin_transaction
  START TRANSACTION, in the mode's words: READ ONLY needs MySQL 5.6.5 or
MariaDB 10.0, both below the layer's floors.
*/
int
d_mariadb_begin_transaction(
    struct d_mariadb*               _db,
    enum d_mariadb_transaction_mode _mode
)
{
    static const char* const statements[] =
    {
        "START TRANSACTION",
        "START TRANSACTION READ ONLY",
        "START TRANSACTION WITH CONSISTENT SNAPSHOT"
    };

    // parameter validation
    if ( (!_db) ||
         ((unsigned)_mode >= (sizeof(statements) / sizeof(statements[0]))) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db, NULL, "%s", statements[_mode]);
}

/*
d_mariadb_commit
  COMMIT, which with no transaction open does nothing and succeeds.
*/
int
d_mariadb_commit(
    struct d_mariadb* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db, NULL, "COMMIT");
}

/*
d_mariadb_rollback
  ROLLBACK, which with no transaction open does nothing and succeeds.
*/
int
d_mariadb_rollback(
    struct d_mariadb* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db, NULL, "ROLLBACK");
}

// 1.3    State
//------------------------------------------------------------------------------
/*
d_mariadb_in_transaction
  SERVER_STATUS_IN_TRANS, as the server's last reply set it.
*/
bool
d_mariadb_in_transaction(
    const struct d_mariadb* _db
)
{
    return ( (_db) &&
             ((_db->native->server_status & SERVER_STATUS_IN_TRANS) != 0u) );
}

//==============================================================================
// 2.  SAVEPOINTS
//==============================================================================

// 2.1    Savepoints
//------------------------------------------------------------------------------
/*
d_mariadb_create_savepoint
  SAVEPOINT; a name already set moves to here, as the servers have it.
*/
int
d_mariadb_create_savepoint(
    struct d_mariadb* _db,
    const char*       _name
)
{
    return d_internal_mariadb_savepoint(_db, "SAVEPOINT", _name);
}

/*
d_mariadb_release_savepoint
  RELEASE SAVEPOINT.
*/
int
d_mariadb_release_savepoint(
    struct d_mariadb* _db,
    const char*       _name
)
{
    return d_internal_mariadb_savepoint(_db, "RELEASE SAVEPOINT", _name);
}

/*
d_mariadb_rollback_to_savepoint
  ROLLBACK TO SAVEPOINT, which keeps the savepoint.
*/
int
d_mariadb_rollback_to_savepoint(
    struct d_mariadb* _db,
    const char*       _name
)
{
    return d_internal_mariadb_savepoint(_db, "ROLLBACK TO SAVEPOINT", _name);
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_begin_transaction
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_begin_transaction(
    struct d_mariadb*               _db,
    enum d_mariadb_transaction_mode _mode
)
{
    (void)_db;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_commit
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_commit(
    struct d_mariadb* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_rollback
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_rollback(
    struct d_mariadb* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_in_transaction
  Without a client, false.
*/
bool
d_mariadb_in_transaction(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return false;
}

/*
d_mariadb_create_savepoint
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_create_savepoint(
    struct d_mariadb* _db,
    const char*       _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_release_savepoint
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_release_savepoint(
    struct d_mariadb* _db,
    const char*       _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_rollback_to_savepoint
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_rollback_to_savepoint(
    struct d_mariadb* _db,
    const char*       _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MARIADB
