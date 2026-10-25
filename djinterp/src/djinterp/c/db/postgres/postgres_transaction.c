/*******************************************************************************
* djinterp [c]                                            postgres_transaction.c
*
* Implementation of postgres_transaction.h.
*   libpq's PQtransactionStatus follows the status byte the server sends
* with every reply, so the layer decides from it, without a round trip, what
* the server would only warn of: BEGIN inside a transaction, and COMMIT or
* ROLLBACK outside one, each of which the server answers with success and a
* WARNING, which libpq's default notice processor prints on stderr. A failed
* transaction's COMMIT, which the server answers with ROLLBACK and success,
* is sent as the ROLLBACK it is and reported as the failure it reports.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_transaction.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.21
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_transaction.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing


#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_postgres_begins
//   table: the BEGIN each d_postgres_transaction_mode sends, in the enum's
// order.
static const char* const d_internal_postgres_begins[] =
{
    "BEGIN",
    "BEGIN READ ONLY",
    "BEGIN ISOLATION LEVEL REPEATABLE READ",
    "BEGIN ISOLATION LEVEL SERIALIZABLE",
    "BEGIN ISOLATION LEVEL SERIALIZABLE READ ONLY DEFERRABLE",
    "BEGIN ISOLATION LEVEL READ COMMITTED"
};

D_STATIC_ASSERT( (sizeof(d_internal_postgres_begins) /
                  sizeof(d_internal_postgres_begins[0])) ==
                 ((size_t)D_POSTGRES_TRANSACTION_READ_COMMITTED + 1u),
                 "every d_postgres_transaction_mode has a BEGIN" );

/*
d_internal_postgres_savepoint
  Every savepoint function's body: the arguments checked, and the statement,
whose words come before the quoted name.
*/
static int
d_internal_postgres_savepoint(
    PGconn*     _db,
    const char* _words,
    const char* _name
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_name_ok(_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(_db, NULL, "%s%I", _words, _name);
}

//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================

// 1.2    Beginning and ending
//------------------------------------------------------------------------------
/*
d_postgres_begin_transaction
  Inside a transaction the server would warn and carry on; the layer refuses
with the code that warning carries, 25001.
*/
int
d_postgres_begin_transaction(
    struct pg_conn*                  _db,
    enum d_postgres_transaction_mode _mode
)
{
    // parameter validation
    if ( (!_db) ||
         ((unsigned)_mode >
          (unsigned)D_POSTGRES_TRANSACTION_READ_COMMITTED) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // one transaction at a time
    if (d_postgres_in_transaction(_db))
    {
        return d_internal_postgres_answer(
                   _db,
                   D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION);
    }

    return d_internal_postgres_send(_db,
                                    d_internal_postgres_begins[_mode],
                                    NULL);
}

/*
d_postgres_commit
  Nothing is sent outside a transaction. A failed one is rolled back, which
is what the server would make of the COMMIT, and reported as 25P02.
*/
int
d_postgres_commit(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    switch (PQtransactionStatus(_db))
    {
        case PQTRANS_IDLE:
            return 0;

        case PQTRANS_INERROR:
        {
            const int result = d_internal_postgres_send(_db, "ROLLBACK", NULL);

            return (result == 0)
                       ? d_internal_postgres_answer(
                             _db,
                             D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION)
                       : result;
        }

        default:
            break;
    }

    return d_internal_postgres_send(_db, "COMMIT", NULL);
}

/*
d_postgres_rollback
  Nothing is sent outside a transaction.
*/
int
d_postgres_rollback(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (PQtransactionStatus(_db) == PQTRANS_IDLE)
               ? 0
               : d_internal_postgres_send(_db, "ROLLBACK", NULL);
}

// 1.3    State
//------------------------------------------------------------------------------
/*
d_postgres_in_transaction
  Open, whether it has failed or not.
*/
bool
d_postgres_in_transaction(
    const struct pg_conn* _db
)
{
    const PGTransactionStatusType status = (_db) ? PQtransactionStatus(_db)
                                                 : PQTRANS_UNKNOWN;

    return ( (status == PQTRANS_INTRANS) ||
             (status == PQTRANS_INERROR) );
}

/*
d_postgres_in_failed_transaction
  Open and failed: only a rollback ends it.
*/
bool
d_postgres_in_failed_transaction(
    const struct pg_conn* _db
)
{
    return ( (_db) &&
             (PQtransactionStatus(_db) == PQTRANS_INERROR) );
}

/*
d_postgres_transaction_status
  libpq's five states, one for one.
*/
enum d_postgres_transaction_state
d_postgres_transaction_status(
    const struct pg_conn* _db
)
{
    // no connection is in no state libpq can name
    if (!_db)
    {
        return D_POSTGRES_STATE_UNKNOWN;
    }

    switch (PQtransactionStatus(_db))
    {
        case PQTRANS_IDLE:
            return D_POSTGRES_STATE_IDLE;

        case PQTRANS_ACTIVE:
            return D_POSTGRES_STATE_ACTIVE;

        case PQTRANS_INTRANS:
            return D_POSTGRES_STATE_IN_TRANSACTION;

        case PQTRANS_INERROR:
            return D_POSTGRES_STATE_IN_FAILED_TRANSACTION;

        case PQTRANS_UNKNOWN:
        default:
            return D_POSTGRES_STATE_UNKNOWN;
    }
}

//==============================================================================
// 2.  SAVEPOINTS
//==============================================================================

// 2.1    Savepoints
//------------------------------------------------------------------------------
/*
d_postgres_create_savepoint
  SAVEPOINT, which the server refuses outside a transaction.
*/
int
d_postgres_create_savepoint(
    struct pg_conn* _db,
    const char*     _name
)
{
    return d_internal_postgres_savepoint(_db, "SAVEPOINT ", _name);
}

/*
d_postgres_release_savepoint
  RELEASE SAVEPOINT.
*/
int
d_postgres_release_savepoint(
    struct pg_conn* _db,
    const char*     _name
)
{
    return d_internal_postgres_savepoint(_db, "RELEASE SAVEPOINT ", _name);
}

/*
d_postgres_rollback_to_savepoint
  ROLLBACK TO SAVEPOINT, which also mends a failed transaction.
*/
int
d_postgres_rollback_to_savepoint(
    struct pg_conn* _db,
    const char*     _name
)
{
    return d_internal_postgres_savepoint(_db, "ROLLBACK TO SAVEPOINT ", _name);
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_begin_transaction
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_begin_transaction(
    struct pg_conn*                  _db,
    enum d_postgres_transaction_mode _mode
)
{
    (void)_db;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_commit
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_commit(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_rollback
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_rollback(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_in_transaction
  Without libpq, false.
*/
bool
d_postgres_in_transaction(
    const struct pg_conn* _db
)
{
    (void)_db;

    return false;
}

/*
d_postgres_in_failed_transaction
  Without libpq, false.
*/
bool
d_postgres_in_failed_transaction(
    const struct pg_conn* _db
)
{
    (void)_db;

    return false;
}

/*
d_postgres_transaction_status
  Without libpq, no state.
*/
enum d_postgres_transaction_state
d_postgres_transaction_status(
    const struct pg_conn* _db
)
{
    (void)_db;

    return D_POSTGRES_STATE_UNKNOWN;
}

/*
d_postgres_create_savepoint
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_savepoint(
    struct pg_conn* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_release_savepoint
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_release_savepoint(
    struct pg_conn* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_rollback_to_savepoint
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_rollback_to_savepoint(
    struct pg_conn* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_POSTGRES
