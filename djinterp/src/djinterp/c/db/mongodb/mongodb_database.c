/*******************************************************************************
* djinterp [c]                                                mongodb_database.c
*
* Implementation of mongodb_database.h.
*   Existence and the list are listDatabases with nameOnly, the one by its
* filter; neither may run in a transaction, so both run outside the open
* one. A drop asks first, so that a database that is not there is
* D_DB_NOT_FOUND on every server, and is refused inside a transaction before
* it is sent, which the server would refuse by ending the transaction.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_database.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_database.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free
#include <string.h>  // strcspn, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, the lists
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "../db_internal.h"       // the names builder
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// MAX_DATABASE_NAME
//   constant: the most bytes MongoDB takes in a database's name.
static const size_t MAX_DATABASE_NAME = 63u;

// FORBIDDEN_IN_DATABASE_NAMES
//   constant: the characters MongoDB forbids in a database's name.
static const char FORBIDDEN_IN_DATABASE_NAMES[] = "/\\. \"$";

/**
 * @brief Refuses an operation that cannot run in a transaction, while one
 *        is open, before it is sent: the server would end the transaction
 *        to refuse it.
 *
 * @param[in,out] _db    the handle.
 * @param[in]     _what  the operation, for the message.
 * @return 0 outside a transaction; else D_DB_UNSUPPORTED, recorded.
 */
static
int d_internal_mongodb_not_in_transaction(
    struct dj_mongodb* _db,
    const char*        _what
)
{
    return ( (_db->session) &&
             (mongoc_client_session_in_transaction(_db->session)) )
               ? d_internal_mongodb_answer(_db,
                                           D_DB_UNSUPPORTED,
                                           "%s cannot run in a transaction",
                                           _what)
               : 0;
}


//==============================================================================
// 1.  DATABASES
//==============================================================================

// 1.1    Existence
//------------------------------------------------------------------------------
/*
d_mongodb_has_database
  The _s form, with every failure read as "no".
*/
bool d_mongodb_has_database(
    struct dj_mongodb* _db,
    const char*        _name
)
{
    bool exists = false;

    return ( (d_mongodb_has_database_s(_db, _name, &exists) == 0) &&
             (exists) );
}

/*
d_mongodb_has_database_s
  listDatabases, its filter on the exact name, outside any transaction.
*/
int d_mongodb_has_database_s(
    struct dj_mongodb* _db,
    const char*        _name,
    bool*              _exists
)
{
    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    // a handle, a name, and somewhere for the answer
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if ( (!_name) ||
         (!_exists) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "a database's name and a place for "
                                         "the answer are needed");
    }

    bson_t      command;
    bson_t      filter;
    bson_t      reply;
    bson_iter_t top;
    bson_iter_t found;

    bson_init(&command);
    BSON_APPEND_INT32(&command, "listDatabases", 1);
    BSON_APPEND_BOOL(&command, "nameOnly", true);
    BSON_APPEND_DOCUMENT_BEGIN(&command, "filter", &filter);
    BSON_APPEND_UTF8(&filter, "name", _name);
    bson_append_document_end(&command, &filter);

    const int result = d_internal_mongodb_run(_db,
                                              "admin",
                                              &command,
                                              true,
                                              &reply);

    *_exists = ( (result == 0)                       &&
                 (bson_iter_init(&top, &reply))      &&
                 (bson_iter_find_descendant(&top,
                                            "databases.0.name",
                                            &found)) );

    bson_destroy(&reply);
    bson_destroy(&command);

    return result;
}

// 1.2    Dropping
//------------------------------------------------------------------------------
/*
d_mongodb_drop_database
  The driver's drop, with the database's write concern, which is the
client's.
*/
int d_mongodb_drop_database(
    struct dj_mongodb* _db,
    const char*        _name,
    unsigned           _flags
)
{
    bool exists = false;

    // a handle, outside a transaction
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_not_in_transaction(_db,
                                                       "dropDatabase");

    // a database that is there, or flags that let it not be
    result = (result == 0) ? d_mongodb_has_database_s(_db, _name, &exists)
                           : result;

    if ( (result == 0) &&
         (!exists) )
    {
        return ((_flags & D_DB_IF_EXISTS) != 0u)
                   ? 0
                   : d_internal_mongodb_answer(_db,
                                               D_DB_NOT_FOUND,
                                               "no database \"%s\"",
                                               _name);
    }

    // the drop
    if (result == 0)
    {
        mongoc_database_t* const database =
            mongoc_client_get_database(_db->client, _name);
        bson_error_t             error;

        result = (mongoc_database_drop_with_opts(database, NULL, &error))
                     ? 0
                     : d_internal_mongodb_fail_outside(_db, &error, NULL);
        mongoc_database_destroy(database);
    }

    return result;
}

// 1.3    Listing
//------------------------------------------------------------------------------
/*
d_mongodb_list_databases
  The driver's listDatabases of names, outside any transaction.
*/
int d_mongodb_list_databases(
    struct dj_mongodb*      _db,
    struct dj_db_name_list* _names
)
{
    // a handle, and somewhere for the names
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (!_names)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no place was given for the names");
    }

    bson_error_t error;
    char** const names =
        mongoc_client_get_database_names_with_opts(_db->client, NULL, &error);

    // the server's list, or its failure
    if (!names)
    {
        return d_internal_mongodb_fail_outside(_db, &error, NULL);
    }

    struct dj_internal_db_names builder = { { NULL, 0u, 0u },
                                           { NULL, 0u, 0u },
                                           0u };
    bool                       ok      = true;
    size_t                     index   = 0u;

    // each name, in the server's order
    for (index = 0u; (ok) && (names[index]); ++index)
    {
        ok = d_internal_db_names_add(&builder, names[index]);
    }

    bson_strfreev(names);

    // the list, packed
    if ( (!ok) ||
         (!d_internal_db_names_finish(&builder, _names)) )
    {
        d_internal_db_names_release(&builder);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the names listed");
    }

    return 0;
}

// 1.4    The current database
//------------------------------------------------------------------------------
/*
d_mongodb_use_database
  MongoDB's rules for a name are checked here, as the server would check
them at the first operation: no empty name, none of its forbidden
characters, 63 bytes at most.
*/
int d_mongodb_use_database(
    struct dj_mongodb* _db,
    const char*        _name
)
{
    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a name MongoDB takes
    if ( (_name) &&
         ( (!*_name)                                               ||
           (strlen(_name) > MAX_DATABASE_NAME)                     ||
           (_name[strcspn(_name, FORBIDDEN_IN_DATABASE_NAMES)] != '\0') ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_ERROR_INVALID_NAMESPACE,
                                         "\"%s\" is not a database's name: "
                                         "MongoDB takes 1 to 63 bytes, none "
                                         "of them / \\ . space \" or $",
                                         _name);
    }

    char* const copy = d_internal_mongodb_strdup(_name);

    // the copy, where one is wanted
    if ( (_name) &&
         (!copy) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the database's name");
    }

    free(_db->database);
    _db->database = copy;

    return 0;
}

/*
d_mongodb_current_database
  A copy, for d_db_free.
*/
int d_mongodb_current_database(
    const struct dj_mongodb* _db,
    char**                   _name
)
{
    // a handle, and somewhere for the name
    if ( (!_db) ||
         (!_name) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // none
    if (!_db->database)
    {
        return D_DB_NOT_FOUND;
    }

    char* const copy = d_internal_mongodb_strdup(_db->database);

    // room for it
    if (!copy)
    {
        return D_MONGODB_LAYER_NO_MEMORY;
    }

    *_name = copy;

    return 0;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports what a build
// without the driver can: D_DB_UNSUPPORTED, or false where it cannot fail.
// An answer a function promises even on failure is still written.

/*
d_mongodb_has_database
  Without the driver, false.
*/
bool d_mongodb_has_database(
    struct dj_mongodb* _db,
    const char*        _name
)
{
    (void)_db;
    (void)_name;

    return false;
}

/*
d_mongodb_has_database_s
  Without the driver there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int d_mongodb_has_database_s(
    struct dj_mongodb* _db,
    const char*        _name,
    bool*              _exists
)
{
    (void)_db;
    (void)_name;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_drop_database
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_drop_database(
    struct dj_mongodb* _db,
    const char*        _name,
    unsigned           _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_list_databases
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_list_databases(
    struct dj_mongodb*      _db,
    struct dj_db_name_list* _names
)
{
    (void)_db;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_use_database
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_use_database(
    struct dj_mongodb* _db,
    const char*        _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_current_database
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_current_database(
    const struct dj_mongodb* _db,
    char**                   _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
