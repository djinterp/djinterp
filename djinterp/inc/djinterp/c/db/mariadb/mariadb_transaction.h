/*******************************************************************************
* djinterp [c]                                             mariadb_transaction.h
*
* Transactions and savepoints on a MariaDB or MySQL connection.
*   A transaction is begun explicitly, in one of the modes START TRANSACTION
* offers, and ended by commit or rollback. Savepoints mark points inside it
* to roll back to, and are named, quoted by the layer like every other name.
*   Both servers end a transaction silently before a statement that changes
* the schema, and neither nests transactions: beginning one inside another
* commits the first. Transactions apply to transactional engines -- InnoDB,
* which both servers use by default -- and not to MyISAM.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_transaction.h
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
         1.  d_mariadb_transaction_mode
    2.  Beginning and ending
    3.  State
2.  SAVEPOINTS
    ----------
    1.  Savepoints
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_TRANSACTION_H
#define DJINTERP_C_DB_MARIADB_MARIADB_TRANSACTION_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "./mariadb_common.h"  // d_mariadb, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================


// 1.1    Modes
//------------------------------------------------------------------------------
// 1.1.1
// d_mariadb_transaction_mode
//   enum: how START TRANSACTION begins one.
enum d_mariadb_transaction_mode
{
    D_MARIADB_TRANSACTION_READ_WRITE = 0,       // the default
    D_MARIADB_TRANSACTION_READ_ONLY,            // READ ONLY: the server
                                                // refuses writes to tables
                                                // others can see
    D_MARIADB_TRANSACTION_CONSISTENT_SNAPSHOT   // WITH CONSISTENT SNAPSHOT:
                                                // the snapshot is taken now,
                                                // not at the first read
};

// 1.2    Beginning and ending
//------------------------------------------------------------------------------
// beginning and ending -- each returns 0, the client's error number for a
// failure, or D_DB_INVALID_ARGUMENT for a NULL handle or a mode outside the
// enum. A commit or rollback with no transaction open succeeds, as on the
// servers themselves.
int    d_mariadb_begin_transaction(struct d_mariadb*               _db,
                                   enum d_mariadb_transaction_mode _mode)
                                   D_NOTHROW;
int    d_mariadb_commit(struct d_mariadb* _db) D_NOTHROW;
int    d_mariadb_rollback(struct d_mariadb* _db) D_NOTHROW;

// 1.3    State
//------------------------------------------------------------------------------
// state -- whether a transaction is open, by the status the server sent with
// its last reply; false for NULL
bool   d_mariadb_in_transaction(const struct d_mariadb* _db) D_NOTHROW;


//==============================================================================
// 2.  SAVEPOINTS
//==============================================================================


// 2.1    Savepoints
//------------------------------------------------------------------------------
// savepoints -- inside a transaction only: outside one, a savepoint is set and
// forgotten at once, as the servers do it. Rolling back to one undoes what
// followed it and keeps it; releasing one forgets it and those after it. Each
// returns 0, the client's error number (1305, no such savepoint), or
// D_DB_INVALID_ARGUMENT for a NULL argument or an empty name.
int    d_mariadb_create_savepoint(struct d_mariadb* _db,
                                  const char*       _name) D_NOTHROW;
int    d_mariadb_release_savepoint(struct d_mariadb* _db,
                                   const char*       _name) D_NOTHROW;
int    d_mariadb_rollback_to_savepoint(struct d_mariadb* _db,
                                       const char*       _name) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_TRANSACTION_H
