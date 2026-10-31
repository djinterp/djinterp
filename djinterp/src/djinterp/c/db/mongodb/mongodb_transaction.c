/*******************************************************************************
* djinterp [c]                                             mongodb_transaction.c
*
* Implementation of mongodb_transaction.h.
*   The handle's session is started at its first transaction and kept,
* each transaction numbered as it begins, so that a cursor knows whether
* the transaction it was opened in is still the open one. The driver keeps
* the transaction's state, and reads it as the drivers' Transactions
* specification has it: a commit may be called again after one whose result
* is unknown, and an abort ignores the server's own failure, so that a
* transaction the server ended rolls back without one. Before the first
* transaction there is no session to ask; the layer answers as the driver
* answers a session with none, "No transaction started", with the driver's
* code.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_transaction.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_transaction.h"  // corresponding header
// std
#include <stddef.h>  // NULL
#include <string.h>  // memset, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Records the driver's failure for a handle that has no session:
 *        no transaction was started.
 *
 * @param[in,out] _db  the handle.
 * @return the driver's code for it, packed.
 */
static
int d_internal_mongodb_no_transaction(
    struct dj_mongodb* _db
)
{
    bson_error_t error;

    memset(&error, 0, sizeof(error));
    error.domain = MONGOC_ERROR_TRANSACTION;
    error.code   = MONGOC_ERROR_TRANSACTION_INVALID_STATE;
    bson_strncpy(error.message,
                 "No transaction started",
                 sizeof(error.message));

    return d_internal_mongodb_fail(_db, &error, NULL);
}

/**
 * @brief Applies a transaction's maxCommitTimeMS.
 *
 * @param[in,out] _db     the handle, on which a refusal is recorded.
 * @param[in]     _field  the option.
 * @param[in,out] _opts   the driver's transaction options.
 * @return 0, or D_DB_INVALID_ARGUMENT for a value that is not a count of
 *         milliseconds.
 */
static
int d_internal_mongodb_commit_time(
    struct dj_mongodb*        _db,
    const bson_iter_t*        _field,
    mongoc_transaction_opt_t* _opts
)
{
    const int64_t value = (BSON_ITER_HOLDS_NUMBER(_field))
                              ? bson_iter_as_int64(_field)
                              : -1;

    // a count, not a negative
    if (value < 0)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "maxCommitTimeMS is a count of "
                                         "milliseconds");
    }

    mongoc_transaction_opts_set_max_commit_time_ms(_opts, value);

    return 0;
}

/**
 * @brief Reads one option of a transaction's into the driver's.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _field  the option.
 * @param[in,out] _opts   the driver's options.
 * @return 0, or D_DB_INVALID_ARGUMENT for an option of no such name or a
 *         value it cannot take, recorded.
 *
 * The concerns and the preference are read as mongodb_internal.c reads
 * them, and copied into the options, which own their copies.
 */
static
int d_internal_mongodb_transaction_option(
    struct dj_mongodb*        _db,
    const bson_iter_t*        _field,
    mongoc_transaction_opt_t* _opts
)
{
    const char* const       key       = bson_iter_key(_field);
    bson_t                  document;
    mongoc_read_concern_t*  read      = NULL;
    mongoc_write_concern_t* write     = NULL;
    mongoc_read_prefs_t*    prefs     = NULL;
    int                     result    = 0;
    const bool              is_document =
        d_internal_mongodb_field_document(_field, &document);

    // maxCommitTimeMS: a count of milliseconds
    if (strcmp(key, "maxCommitTimeMS") == 0)
    {
        return d_internal_mongodb_commit_time(_db, _field, _opts);
    }

    // the concerns and the preference: documents
    if ( (!is_document) ||
         ( (strcmp(key, "readConcern") != 0)  &&
           (strcmp(key, "writeConcern") != 0) &&
           (strcmp(key, "readPreference") != 0) ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "a transaction cannot take its "
                                         "option \"%s\" as given",
                                         key);
    }

    if (key[0] == 'w')
    {
        result = d_internal_mongodb_write_concern(_db, &document, &write);
        mongoc_transaction_opts_set_write_concern(_opts, write);
    }
    else if (strcmp(key, "readConcern") == 0)
    {
        result = d_internal_mongodb_read_concern(_db, &document, &read);
        mongoc_transaction_opts_set_read_concern(_opts, read);
    }
    else
    {
        result = d_internal_mongodb_read_prefs(_db, &document, &prefs);
        mongoc_transaction_opts_set_read_prefs(_opts, prefs);
    }

    mongoc_read_prefs_destroy(prefs);
    mongoc_write_concern_destroy(write);
    mongoc_read_concern_destroy(read);

    return result;
}


//==============================================================================
// 1.  TRANSACTIONS
//==============================================================================

// 1.1    Beginning
//------------------------------------------------------------------------------
/*
d_mongodb_begin_transaction
  The handle's own concerns and preference, which the driver falls back to.
*/
int d_mongodb_begin_transaction(
    struct dj_mongodb* _db
)
{
    return d_mongodb_begin_transaction_with(_db, NULL);
}

/**
 * @brief Starts a transaction in the handle's session, which the first
 *        starts, and numbers it.
 *
 * @param[in,out] _db    the handle.
 * @param[in]     _opts  the transaction's options.
 * @return 0, or the driver's failure, recorded.
 */
static
int d_internal_mongodb_open_transaction(
    struct dj_mongodb*              _db,
    const mongoc_transaction_opt_t* _opts
)
{
    bson_error_t error;

    // the session, at the first transaction
    if (!_db->session)
    {
        _db->session = mongoc_client_start_session(_db->client, NULL, &error);

        if (!_db->session)
        {
            return d_internal_mongodb_fail(_db, &error, NULL);
        }
    }

    // the transaction, numbered
    if (!mongoc_client_session_start_transaction(_db->session, _opts, &error))
    {
        return d_internal_mongodb_fail(_db, &error, NULL);
    }

    ++_db->transaction;
    _db->failed = false;

    return 0;
}

/*
d_mongodb_begin_transaction_with
  What the options do not say falls back, in the driver, to the session's
defaults, and then to the client's: the handle's.
*/
int d_mongodb_begin_transaction_with(
    struct dj_mongodb* _db,
    const char*        _options
)
{
    // a handle, and a deployment that has transactions
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (!d_mongodb_supports(_db, D_MONGODB_FEATURE_TRANSACTIONS))
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_UNSUPPORTED,
                                         "a transaction needs a replica set "
                                         "of MongoDB 4.0, a sharded cluster "
                                         "of 4.2, or a load balancer");
    }

    mongoc_transaction_opt_t* const opts = mongoc_transaction_opts_new();
    bson_t                          options;
    bson_iter_t                     field;
    int                             result =
        d_internal_mongodb_read_document(_db, _options, false, &options);

    // each option, to the driver's
    if ( (result == 0) &&
         (bson_iter_init(&field, &options)) )
    {
        while ( (result == 0) &&
                (bson_iter_next(&field)) )
        {
            result = d_internal_mongodb_transaction_option(_db, &field, opts);
        }
    }

    result = (result == 0) ? d_internal_mongodb_open_transaction(_db, opts)
                           : result;

    bson_destroy(&options);
    mongoc_transaction_opts_destroy(opts);

    return result;
}

// 1.2    Ending
//------------------------------------------------------------------------------
/*
d_mongodb_commit
  The driver's commit, which may be called again after one whose result is
unknown; its reply carries the labels that say so. The transaction's
cursors end after it, outside any transaction.
*/
int d_mongodb_commit(
    struct dj_mongodb* _db
)
{
    // a handle, and a session to commit
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (!_db->session)
    {
        return d_internal_mongodb_no_transaction(_db);
    }

    bson_t       reply;
    bson_error_t error;

    bson_init(&reply);

    const int result = (mongoc_client_session_commit_transaction(_db->session,
                                                                 &reply,
                                                                 &error))
                           ? 0
                           : d_internal_mongodb_fail(_db, &error, &reply);

    d_internal_mongodb_end_cursors(_db);
    _db->failed = false;
    bson_destroy(&reply);

    return result;
}

/*
d_mongodb_rollback
  The driver's abort, which ignores the server's failure to abort a
transaction the server already ended. The transaction's cursors end after
it, outside any transaction.
*/
int d_mongodb_rollback(
    struct dj_mongodb* _db
)
{
    // a handle, and a session to roll back
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (!_db->session)
    {
        return d_internal_mongodb_no_transaction(_db);
    }

    bson_error_t error;

    const int result = (mongoc_client_session_abort_transaction(_db->session,
                                                                &error))
                           ? 0
                           : d_internal_mongodb_fail(_db, &error, NULL);

    d_internal_mongodb_end_cursors(_db);
    _db->failed = false;

    return result;
}

// 1.3    State
//------------------------------------------------------------------------------
/*
d_mongodb_in_transaction
  The driver's state: starting, or in progress.
*/
bool d_mongodb_in_transaction(
    const struct dj_mongodb* _db
)
{
    return ( (_db)          &&
             (_db->session) &&
             (mongoc_client_session_in_transaction(_db->session)) );
}

/*
d_mongodb_in_failed_transaction
  Open, and marked failed by a failure the server's transaction did not
survive.
*/
bool d_mongodb_in_failed_transaction(
    const struct dj_mongodb* _db
)
{
    return ( (d_mongodb_in_transaction(_db)) &&
             (_db->failed) );
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports what a build
// without the driver can: D_DB_UNSUPPORTED, or false where it cannot fail.

/*
d_mongodb_begin_transaction
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_begin_transaction(
    struct dj_mongodb* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_begin_transaction_with
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_begin_transaction_with(
    struct dj_mongodb* _db,
    const char*        _options
)
{
    (void)_db;
    (void)_options;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_commit
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_commit(
    struct dj_mongodb* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_rollback
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_rollback(
    struct dj_mongodb* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_in_transaction
  Without the driver, false.
*/
bool d_mongodb_in_transaction(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return false;
}

/*
d_mongodb_in_failed_transaction
  Without the driver, false.
*/
bool d_mongodb_in_failed_transaction(
    const struct dj_mongodb* _db
)
{
    (void)_db;

    return false;
}

#endif  // D_INTERNAL_MONGODB == 1
