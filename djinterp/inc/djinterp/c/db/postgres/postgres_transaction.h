/*******************************************************************************
* djinterp [c]                                            postgres_transaction.h
*
* Transactions and savepoints on a PostgreSQL connection.
*   Outside a transaction every statement commits on its own. Inside one,
* PostgreSQL makes everything transactional, schema changes too -- and a
* statement that fails leaves the transaction failed: everything after it is
* refused (D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION) until the caller
* rolls back, to a savepoint set before the failure or the whole way.
* d_postgres_in_failed_transaction tells that state apart,
* d_postgres_transaction_status tells every state libpq knows, a statement
* still running among them, and d_postgres_commit reports a commit the server
* turned into a rollback for it.
*   The state comes from libpq, which the server updates with every reply, so
* asking it costs no round trip.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_transaction.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSACTIONS
    ------------
    1.  Modes
         1.  d_postgres_transaction_mode
    2.  Beginning and ending
    3.  State
         1.  d_postgres_transaction_state
2.  SAVEPOINTS
    ----------
    1.  Savepoints
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_TRANSACTION_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_TRANSACTION_H 1

// djinterp
#include "../../djinterp.h"     // framework root
#include "./postgres_common.h"  // pg_conn, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================


// 1.1    Modes
//------------------------------------------------------------------------------
// 1.1.1
// d_postgres_transaction_mode
//   enum: how BEGIN begins one.
enum d_postgres_transaction_mode
{
    D_POSTGRES_TRANSACTION_DEFAULT = 0,      // BEGIN: the session's isolation
                                             // level and access mode -- READ
                                             // COMMITTED, read write, unless
                                             // the session says otherwise
    D_POSTGRES_TRANSACTION_READ_ONLY,        // BEGIN READ ONLY: the server
                                             // refuses writes
    D_POSTGRES_TRANSACTION_REPEATABLE_READ,  // ISOLATION LEVEL REPEATABLE
                                             // READ: one snapshot for the
                                             // whole transaction
    D_POSTGRES_TRANSACTION_SERIALIZABLE,     // ISOLATION LEVEL SERIALIZABLE:
                                             // as if transactions ran one at
                                             // a time; a commit may fail
                                             // (40001), to be retried
    D_POSTGRES_TRANSACTION_DEFERRABLE,       // ISOLATION LEVEL SERIALIZABLE
                                             // READ ONLY DEFERRABLE: waits
                                             // for a snapshot that cannot
                                             // fail to serialize, then never
                                             // does -- for long reports
    D_POSTGRES_TRANSACTION_READ_COMMITTED    // ISOLATION LEVEL READ COMMITTED:
                                             // a snapshot for each statement,
                                             // whatever the session's level
};

// 1.2    Beginning and ending
//------------------------------------------------------------------------------
// beginning and ending -- each returns 0, the SQLSTATE of a failure, or
// D_DB_INVALID_ARGUMENT for a NULL connection or a mode outside the enum.
// Beginning inside a transaction is refused with
// D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION (25001), which the server only
// warns of. Committing a failed transaction rolls it back, as the server
// does, and returns D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION (25P02), so
// that the caller knows nothing was committed. A commit or rollback with no
// transaction open succeeds, sending nothing.
int    d_postgres_begin_transaction(struct pg_conn*                  _db,
                                    enum d_postgres_transaction_mode _mode)
                                    D_NOTHROW;
int    d_postgres_commit(struct pg_conn* _db) D_NOTHROW;
int    d_postgres_rollback(struct pg_conn* _db) D_NOTHROW;

// 1.3    State
//------------------------------------------------------------------------------
// 1.3.1
// d_postgres_transaction_state
//   enum: where a connection stands, as libpq's PQtransactionStatus says.
enum d_postgres_transaction_state
{
    D_POSTGRES_STATE_IDLE = 0,                // no transaction open, and
                                              // nothing running
    D_POSTGRES_STATE_ACTIVE,                  // SQL sent is running, or its
                                              // results are still to be read
                                              // (d_postgres_send_query), or a
                                              // COPY is in progress
    D_POSTGRES_STATE_IN_TRANSACTION,          // a transaction is open
    D_POSTGRES_STATE_IN_FAILED_TRANSACTION,   // a transaction is open and has
                                              // failed: only a rollback ends
                                              // it
    D_POSTGRES_STATE_UNKNOWN                  // no connection, or a lost one
};

// state -- whether a transaction is open, failed or not; and whether it is
// open and failed, so that only a rollback can end it. False for NULL.
bool   d_postgres_in_transaction(const struct pg_conn* _db) D_NOTHROW;
bool   d_postgres_in_failed_transaction(const struct pg_conn* _db) D_NOTHROW;
// the state -- every state libpq knows, as above; D_POSTGRES_STATE_UNKNOWN
// for NULL
enum d_postgres_transaction_state
       d_postgres_transaction_status(const struct pg_conn* _db) D_NOTHROW;


//==============================================================================
// 2.  SAVEPOINTS
//==============================================================================


// 2.1    Savepoints
//------------------------------------------------------------------------------
// savepoints -- inside a transaction only: outside one, the server refuses
// with D_POSTGRES_SQLSTATE_NO_ACTIVE_SQL_TRANSACTION (25P01). Rolling back to
// one undoes what followed it, mends a failed transaction, and keeps it;
// releasing one forgets it and those after it, keeping what they did. Each
// returns 0, the SQLSTATE of a failure (3B001, no such savepoint), or
// D_DB_INVALID_ARGUMENT for a NULL argument or an empty name.
int    d_postgres_create_savepoint(struct pg_conn* _db,
                                   const char*     _name) D_NOTHROW;
int    d_postgres_release_savepoint(struct pg_conn* _db,
                                    const char*     _name) D_NOTHROW;
int    d_postgres_rollback_to_savepoint(struct pg_conn* _db,
                                        const char*     _name) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_TRANSACTION_H
