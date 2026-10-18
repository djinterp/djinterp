/*******************************************************************************
* djinterp [c]                                              sqlite_transaction.h
*
* Transactions and savepoints on an SQLite connection.
*   SQLite runs every statement outside an explicit transaction in one of its
* own, committed as the statement ends. A transaction begun here groups
* statements until d_sqlite_commit or d_sqlite_rollback. Savepoints nest,
* inside a transaction or on their own, which then starts one.
*   The mode says when the write lock is taken: DEFERRED at the first write,
* IMMEDIATE at once, so a write cannot later fail for want of it, EXCLUSIVE at
* once and keeping readers out too (outside WAL mode).
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_transaction.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSACTIONS
    ------------
    1.  Modes
         1.  d_sqlite_transaction_mode
    2.  Beginning and ending
    3.  State
2.  SAVEPOINTS
    ----------
    1.  Savepoints
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_TRANSACTION_H
#define DJINTERP_C_DB_SQLITE_SQLITE_TRANSACTION_H 1

// djinterp
#include "../../djinterp.h"   // framework root
#include "./sqlite_common.h"  // sqlite3, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================


// 1.1    Modes
//------------------------------------------------------------------------------
// 1.1.1
// d_sqlite_transaction_mode
//   enum: when a transaction takes the database's write lock.
enum d_sqlite_transaction_mode
{
    D_SQLITE_TRANSACTION_DEFERRED  = 0,  // at its first write; the default
    D_SQLITE_TRANSACTION_IMMEDIATE = 1,  // at once
    D_SQLITE_TRANSACTION_EXCLUSIVE = 2   // at once, keeping readers out too
};

// 1.2    Beginning and ending
//------------------------------------------------------------------------------
/**
 * @brief Begins a transaction.
 *
 * @param[in] _db    the connection.
 * @param[in] _mode  when to take the write lock.
 * @return SQLITE_OK; SQLITE_BUSY when another connection holds the lock an
 *         IMMEDIATE or EXCLUSIVE begin needs, beyond the busy timeout;
 *         SQLITE_ERROR inside a transaction already, as SQLite does not nest
 *         them (use a savepoint); or SQLITE_MISUSE for a `NULL` connection or
 *         an unknown mode.
 */
int  d_sqlite_begin_transaction(struct sqlite3*                _db,
                                enum d_sqlite_transaction_mode _mode) D_NOTHROW;
/**
 * @brief Commits the transaction.
 *
 * @param[in] _db  the connection.
 * @return SQLITE_OK; SQLITE_BUSY when readers keep the commit from finishing
 *         -- the transaction is still open, and the commit can be retried;
 *         SQLITE_ERROR outside a transaction; or SQLITE_MISUSE.
 */
int  d_sqlite_commit(struct sqlite3* _db) D_NOTHROW;
/**
 * @brief Rolls the transaction back.
 *
 * @param[in] _db  the connection.
 * @return SQLITE_OK; SQLITE_ERROR outside a transaction; or SQLITE_MISUSE.
 * @post No transaction is open.
 */
int  d_sqlite_rollback(struct sqlite3* _db) D_NOTHROW;

// 1.3    State
//------------------------------------------------------------------------------
// state -- true while a transaction is open: one begun here, or natively, or
// by a savepoint; false for a NULL connection
bool d_sqlite_in_transaction(struct sqlite3* _db) D_NOTHROW;


//==============================================================================
// 2.  SAVEPOINTS
//==============================================================================


// 2.1    Savepoints
//------------------------------------------------------------------------------
// savepoints -- each takes the savepoint's name, which is quoted, and returns
// SQLITE_OK or the SQLite result code of the failure (SQLITE_ERROR for a
// release or rollback naming no open savepoint), or SQLITE_MISUSE for a NULL
// argument or an empty name. Releasing the outermost savepoint commits, when
// it began the transaction. Rolling back to a savepoint undoes the work since
// it and leaves it open; release it to close it.
int  d_sqlite_create_savepoint(struct sqlite3* _db,
                               const char*     _name) D_NOTHROW;
int  d_sqlite_release_savepoint(struct sqlite3* _db,
                                const char*     _name) D_NOTHROW;
int  d_sqlite_rollback_to_savepoint(struct sqlite3* _db,
                                    const char*     _name) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_SQLITE_SQLITE_TRANSACTION_H
