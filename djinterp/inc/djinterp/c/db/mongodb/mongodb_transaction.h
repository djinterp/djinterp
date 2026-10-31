/*******************************************************************************
* djinterp [c]                                             mongodb_transaction.h
*
* Multi-document transactions.
*   A transaction runs in a session, which the handle starts at its first
* transaction and keeps until it closes; while one is open, every operation
* the layer sends on the handle -- commands, documents, cursors, schema --
* carries the session, so that each is part of it. A transaction needs a
* replica set (MongoDB 4.0 and later), a sharded cluster (4.2 and later) or a
* load balancer (D_MONGODB_FEATURE_TRANSACTIONS): a standalone server has
* none, and d_mongodb_begin_transaction says so at once, with
* D_DB_UNSUPPORTED, rather than at the first operation. GridFS's operations
* run outside it, as the driver runs them.
*   Any failure the server reports inside a transaction ends it there -- a
* duplicate key, a write conflict, an exceeded time, a bad operator, a
* command of no such name -- as does a lost connection, which the driver
* labels TransientTransactionError; the layer then reports the transaction
* failed (d_mongodb_in_failed_transaction) until it is rolled back, as the
* server refuses anything else in it. A refusal of the layer's own or the
* driver's, which never reached the server, leaves it open, as does an
* operation the server cannot hold in a transaction, which the layer
* refuses before sending (mongodb_schema.h, mongodb_database.h).
*   Whether to run it again is the failure's label:
* TransientTransactionError says the whole transaction may be retried, from
* its beginning; a commit that fails labelled UnknownTransactionCommitResult
* may itself be retried, and may have committed (d_mongodb_error_has_label,
* mongodb_common.h). The layer runs no code of its caller's, so the retry is
* the caller's loop, as core/db's is.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_transaction.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSACTIONS
    ------------
    1.  Beginning
    2.  Ending
    3.  State
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_TRANSACTION_H
#define DJINTERP_C_DB_MONGODB_MONGODB_TRANSACTION_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "./mongodb_common.h"  // dj_mongodb, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================


// 1.1    Beginning
//------------------------------------------------------------------------------
// beginning -- begins a transaction, reading and writing with the handle's
// read and write concerns and read preference (mongodb_settings.h), or with
// the options given as JSON: readConcern ({"level": "snapshot"}),
// writeConcern, readPreference (a transaction reads from the primary), and
// maxCommitTimeMS. Returns 0; D_DB_UNSUPPORTED where the deployment has no
// transactions; the driver's code for one already open, or for options it
// refuses; D_DB_INVALID_ARGUMENT for a NULL handle or options that are not
// a document
int d_mongodb_begin_transaction(
    struct dj_mongodb* _db
) D_NOTHROW;
int d_mongodb_begin_transaction_with(
    struct dj_mongodb* _db,
    const char*        _options
) D_NOTHROW;

// 1.2    Ending
//------------------------------------------------------------------------------
// ending -- commit makes the transaction's writes durable as its write
// concern asks; rollback aborts it, its writes undone, and rolls back a
// failed one without a failure of its own. Each returns 0; the server's
// code or the driver's (a commit's labels say whether to retry it); the
// driver's code for no open transaction, before the handle's first too;
// D_DB_INVALID_ARGUMENT for a NULL handle. Either way the transaction is
// over after, but for a commit whose result is unknown, which may be
// committed again
int d_mongodb_commit(
    struct dj_mongodb* _db
) D_NOTHROW;
int d_mongodb_rollback(
    struct dj_mongodb* _db
) D_NOTHROW;

// 1.3    State
//------------------------------------------------------------------------------
// state -- whether a transaction is open, and whether one is open that an
// operation in it failed, which only a rollback ends; false for a NULL
// handle
bool d_mongodb_in_transaction(
    const struct dj_mongodb* _db
) D_NOTHROW;
bool d_mongodb_in_failed_transaction(
    const struct dj_mongodb* _db
) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_TRANSACTION_H
