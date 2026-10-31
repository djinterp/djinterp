/*******************************************************************************
* djinterp [c]                                                mongodb_database.h
*
* The databases of a deployment, and the handle's current one.
*   A MongoDB database exists from its first collection and goes with its
* last: there is no empty database, nothing to create one with, and no
* d_mongodb_create_database. A write to a collection of a database that is
* not there makes both. Existence and the list are the server's
* listDatabases, nameOnly, which lists the databases the user may read --
* all of them for a user with the right to list them.
*   The current database is the handle's: the one a NULL database means in
* every function. A handle starts with the database its options or its
* connection string named, or none, which a function given a NULL database
* then refuses with D_DB_INVALID_ARGUMENT.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_database.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DATABASES
    ---------
    1.  Existence
    2.  Dropping
    3.  Listing
    4.  The current database
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_DATABASE_H
#define DJINTERP_C_DB_MONGODB_MONGODB_DATABASE_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // D_DB_IF_EXISTS, dj_db_name_list
#include "./mongodb_common.h"  // dj_mongodb, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DATABASES
//==============================================================================


// 1.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the deployment has the database, by its exact name
// (MongoDB refuses two that differ only in case); has_x_s reports a failure
// -- the server's, the driver's, or D_DB_INVALID_ARGUMENT for a NULL
// argument -- and writes false on one
bool d_mongodb_has_database(
    struct dj_mongodb* _db,
    const char*        _name
) D_NOTHROW;
int d_mongodb_has_database_s(
    struct dj_mongodb* _db,
    const char*        _name,
    bool*              _exists
) D_NOTHROW;

// 1.2    Dropping
//------------------------------------------------------------------------------
// dropping -- drops a database, every collection, view and index in it; one
// that is not there is D_DB_NOT_FOUND, which no server reports, unless
// _flags has D_DB_IF_EXISTS. Returns 0, or the failure: the server's code
// (13, unauthorized), the driver's, D_DB_INVALID_ARGUMENT for a NULL
// argument, D_DB_UNSUPPORTED inside a transaction, which holds no drop and
// which the layer leaves open by refusing before it sends
int d_mongodb_drop_database(
    struct dj_mongodb* _db,
    const char*        _name,
    unsigned           _flags
) D_NOTHROW;

// 1.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists the deployment's databases, in the order the server gives.
 *
 * @param[in]  _db     the handle.
 * @param[out] _names  receives the names; written only on success.
 * @return 0, the server's code or the driver's for a failure,
 *         D_MONGODB_LAYER_NO_MEMORY, or D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int d_mongodb_list_databases(
    struct dj_mongodb*      _db,
    struct dj_db_name_list* _names
) D_NOTHROW;

// 1.4    The current database
//------------------------------------------------------------------------------
// the current database -- use_database makes a database the handle's current
// one, which need not exist yet, or, given NULL, leaves the handle with none;
// it asks the server nothing, and refuses only an empty name, or one MongoDB
// forbids (a '/', '\', '.', ' ', '"', '$', or NUL, or more than 63 bytes),
// with D_MONGODB_ERROR_INVALID_NAMESPACE (73), the server's code for it, as
// the server would at the first operation; D_MONGODB_LAYER_NO_MEMORY;
// D_DB_INVALID_ARGUMENT for a NULL handle
int d_mongodb_use_database(
    struct dj_mongodb* _db,
    const char*        _name
) D_NOTHROW;
/**
 * @brief Hands back the name of the handle's current database.
 *
 * @param[in]  _db    the handle.
 * @param[out] _name  receives the name, released with d_db_free; written
 *                    only on success.
 * @return 0; D_DB_NOT_FOUND when the handle has none;
 *         D_MONGODB_LAYER_NO_MEMORY; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 * @post On success the caller releases `*_name` with d_db_free.
 */
int d_mongodb_current_database(
    const struct dj_mongodb* _db,
    char**                   _name
) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_DATABASE_H
