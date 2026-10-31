/*******************************************************************************
* djinterp [c]                                                  mongodb_schema.c
*
* Implementation of mongodb_schema.h.
*   What a name is, a collection or a view or neither, is asked of
* listCollections, and an index's of listIndexes, both outside any
* transaction, which may hold neither; the layer asks before it creates or
* drops, so that the codes and the flags mean the same on servers that
* disagree (mongodb_schema.h). A rename needs no question: every server
* answers 26 for a source that is not there and 48 for a target that is.
*   DDL runs as the drivers run it: outside a transaction with the client's
* write concern, which the layer appends, and inside one with its session,
* where the server takes it -- a create of a collection or an index, from
* 4.4. A drop or a rename the server never takes in a transaction, and the
* layer refuses before sending, which the server would do by ending the
* transaction.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_schema.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_schema.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strcmp
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

/**
 * @brief Checks the names an operation is given: a database, named or
 *        current, and a name in it, neither NULL nor empty.
 *
 * @param[in,out] _db        the handle, not `NULL`.
 * @param[in]     _database  the database given, or `NULL`.
 * @param[in]     _name      the name.
 * @param[out]    _out       receives the database's name.
 * @return 0; D_DB_INVALID_ARGUMENT for no database or a NULL name; 73 for
 *         an empty one.
 */
static
int d_internal_mongodb_names(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    const char**       _out
)
{
    *_out = d_internal_mongodb_database(_db, _database);

    // a database, and a name in it
    if (!*_out)
    {
        return _db->error;
    }

    if ( (!_name) ||
         (!*_name) )
    {
        return d_internal_mongodb_answer(
                   _db,
                   (_name) ? D_MONGODB_ERROR_INVALID_NAMESPACE
                           : D_DB_INVALID_ARGUMENT,
                   (_name) ? "a name in a database cannot be empty"
                           : "no name was given");
    }

    return 0;
}

/**
 * @brief Refuses flags an operation gives no meaning.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _flags    the flags.
 * @param[in]     _allowed  the flags it reads.
 * @return 0, or D_DB_INVALID_ARGUMENT, recorded.
 */
static
int d_internal_mongodb_flags(
    struct dj_mongodb* _db,
    unsigned           _flags,
    unsigned           _allowed
)
{
    return ((_flags & ~_allowed) != 0u)
               ? d_internal_mongodb_answer(_db,
                                           D_DB_INVALID_ARGUMENT,
                                           "flags of no meaning here, 0x%x",
                                           _flags & ~_allowed)
               : 0;
}

/**
 * @brief Refuses DDL inside a transaction where the server does not take
 *        it there: a drop or a rename ever, a create before 4.4.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _what    the command, for the message.
 * @param[in]     _create  whether it creates, which 4.4 takes.
 * @return 0, or D_DB_UNSUPPORTED, recorded.
 */
static
int d_internal_mongodb_ddl_allowed(
    struct dj_mongodb* _db,
    const char*        _what,
    bool               _create
)
{
    const bool in_transaction = ( (_db->session) &&
                                  (mongoc_client_session_in_transaction(
                                       _db->session)) );

    // outside a transaction, anything; inside, a create from 4.4
    if ( (!in_transaction) ||
         ( (_create) &&
           (_db->version >= 40400ul) ) )
    {
        return 0;
    }

    return d_internal_mongodb_answer(_db,
                                     D_DB_UNSUPPORTED,
                                     (_create) ? "%s runs in a transaction "
                                                 "from MongoDB 4.4"
                                               : "%s cannot run in a "
                                                 "transaction",
                                     _what);
}

/**
 * @brief Runs a DDL command: inside a transaction with its session,
 *        outside one with the client's write concern appended.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _name     the database.
 * @param[in]     _command  the command.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_run_ddl(
    struct dj_mongodb* _db,
    const char*        _name,
    const bson_t*      _command
)
{
    bson_t reply;
    int    result = 0;

    // in a transaction, as written, with the session
    if ( (_db->session) &&
         (mongoc_client_session_in_transaction(_db->session)) )
    {
        result = d_internal_mongodb_run(_db, _name, _command, false, &reply);
        bson_destroy(&reply);

        return result;
    }

    const mongoc_write_concern_t* const concern =
        mongoc_client_get_write_concern(_db->client);
    bson_t                              command;

    bson_init(&command);
    bson_concat(&command, _command);

    // the client's write concern, where it has one
    if (!mongoc_write_concern_is_default(concern))
    {
        mongoc_write_concern_t* const copy =
            mongoc_write_concern_copy(concern);

        (void)mongoc_write_concern_append(copy, &command);
        mongoc_write_concern_destroy(copy);
    }

    result = d_internal_mongodb_run(_db, _name, &command, true, &reply);
    bson_destroy(&reply);
    bson_destroy(&command);

    return result;
}

/**
 * @brief Asks what a name is, for a has_x_s.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _database  the database, or `NULL`.
 * @param[in]     _name      the name.
 * @param[in]     _kind      the kind asked after.
 * @param[out]    _exists    receives whether the name is of it; false on
 *                           a failure.
 * @return 0, or the failure.
 */
static int
d_internal_mongodb_has_kind(
    struct dj_mongodb*            _db,
    const char*                  _database,
    const char*                  _name,
    enum dj_internal_mongodb_kind _kind,
    bool*                        _exists
)
{
    enum dj_internal_mongodb_kind kind = D_INTERNAL_MONGODB_NOTHING;
    const char*                  name = NULL;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    // a handle, the names, and somewhere for the answer
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = (_exists) ? d_internal_mongodb_names(_db,
                                                      _database,
                                                      _name,
                                                      &name)
                           : d_internal_mongodb_answer(
                                 _db,
                                 D_DB_INVALID_ARGUMENT,
                                 "no place was given for the answer");

    result = (result == 0)
                 ? d_internal_mongodb_kind_of(_db, name, _name, &kind)
                 : result;

    // the answer, on success
    if (result == 0)
    {
        *_exists = (kind == _kind);
    }

    return result;
}

/**
 * @brief Lists the names of a kind, for a list_x.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _database  the database, or `NULL`.
 * @param[in]     _kind      the kind.
 * @param[out]    _names     receives the names.
 * @return 0, or the failure.
 */
static int
d_internal_mongodb_list_of(
    struct dj_mongodb*            _db,
    const char*                  _database,
    enum dj_internal_mongodb_kind _kind,
    struct dj_db_name_list*       _names
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

    const char* const name = d_internal_mongodb_database(_db, _database);

    return (name) ? d_internal_mongodb_list_kind(_db, name, _kind, _names)
                  : _db->error;
}

/**
 * @brief Creates a collection or a view, unless something of the name is
 *        there.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _name      the database.
 * @param[in]     _command   the create command.
 * @param[in]     _what      the name created.
 * @param[in]     _flags     D_DB_IF_NOT_EXISTS, or 0.
 * @return 0; 48 for a name that is there; the failure.
 */
static
int d_internal_mongodb_create_kind(
    struct dj_mongodb* _db,
    const char*        _name,
    const bson_t*      _command,
    const char*        _what,
    unsigned           _flags
)
{
    enum dj_internal_mongodb_kind kind   = D_INTERNAL_MONGODB_NOTHING;
    int                          result = d_internal_mongodb_ddl_allowed(
                                              _db,
                                              "create",
                                              true);

    result = (result == 0)
                 ? d_internal_mongodb_kind_of(_db, _name, _what, &kind)
                 : result;

    // something of the name already
    if ( (result == 0) &&
         (kind != D_INTERNAL_MONGODB_NOTHING) )
    {
        return ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
                   ? 0
                   : d_internal_mongodb_answer(
                         _db,
                         D_MONGODB_ERROR_NAMESPACE_EXISTS,
                         "a %s named \"%s\" is already there",
                         (kind == D_INTERNAL_MONGODB_VIEW) ? "view"
                                                           : "collection",
                         _what);
    }

    return (result == 0) ? d_internal_mongodb_run_ddl(_db, _name, _command)
                         : result;
}

/**
 * @brief Drops a collection or a view by the drop command, where its name
 *        is of the kinds it may be.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database, or `NULL`.
 * @param[in]     _name        the name.
 * @param[in]     _flags       D_DB_IF_EXISTS, or 0.
 * @param[in]     _views_only  whether only a view is dropped.
 * @return 0; 26 for nothing of the kinds; the failure.
 */
static
int d_internal_mongodb_drop_kind(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    unsigned           _flags,
    bool               _views_only
)
{
    enum dj_internal_mongodb_kind kind = D_INTERNAL_MONGODB_NOTHING;
    const char*                  name = NULL;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_flags(_db, _flags, D_DB_IF_EXISTS);

    result = (result == 0)
                 ? d_internal_mongodb_names(_db, _database, _name, &name)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_ddl_allowed(_db, "drop", false)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_kind_of(_db, name, _name, &kind)
                 : result;

    // a failure, which sends nothing
    if (result != 0)
    {
        return result;
    }

    // nothing of the kinds
    if ( (kind == D_INTERNAL_MONGODB_NOTHING) ||
         ( (_views_only) &&
           (kind != D_INTERNAL_MONGODB_VIEW) ) )
    {
        return ((_flags & D_DB_IF_EXISTS) != 0u)
                   ? 0
                   : d_internal_mongodb_answer(
                         _db,
                         D_MONGODB_ERROR_NAMESPACE_NOT_FOUND,
                         "no %s named \"%s\"",
                         (_views_only) ? "view" : "collection or view",
                         _name);
    }

    bson_t command;

    bson_init(&command);
    BSON_APPEND_UTF8(&command, "drop", _name);
    result = d_internal_mongodb_run_ddl(_db, name, &command);
    bson_destroy(&command);

    return result;
}


//==============================================================================
// 2.  COLLECTIONS
//==============================================================================

// 2.1    Existence
//------------------------------------------------------------------------------
/*
d_mongodb_has_collection
  The _s form, with every failure read as "no".
*/
bool d_mongodb_has_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
)
{
    bool exists = false;

    return ( (d_mongodb_has_collection_s(_db, _database, _name, &exists) ==
              0) &&
             (exists) );
}

/*
d_mongodb_has_collection_s
  listCollections by name; a view is not a collection.
*/
int d_mongodb_has_collection_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    bool*              _exists
)
{
    return d_internal_mongodb_has_kind(_db,
                                       _database,
                                       _name,
                                       D_INTERNAL_MONGODB_COLLECTION,
                                       _exists);
}

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_mongodb_create_collection
  {create: name, ...options}, after asking whether the name is taken.
*/
int d_mongodb_create_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    const char*        _options,
    unsigned           _flags
)
{
    const char* name = NULL;
    bson_t      options;
    bson_t      command;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&options);
    bson_init(&command);

    int result = d_internal_mongodb_flags(_db, _flags, D_DB_IF_NOT_EXISTS);

    result = (result == 0)
                 ? d_internal_mongodb_names(_db, _database, _name, &name)
                 : result;
    result = (result == 0) ? d_internal_mongodb_read_document(_db,
                                                              _options,
                                                              false,
                                                              &options)
                           : result;

    // the command, then the create
    if (result == 0)
    {
        BSON_APPEND_UTF8(&command, "create", _name);
        bson_concat(&command, &options);
        result = d_internal_mongodb_create_kind(_db,
                                                name,
                                                &command,
                                                _name,
                                                _flags);
    }

    bson_destroy(&command);
    bson_destroy(&options);

    return result;
}

/*
d_mongodb_drop_collection
  A collection, or a view: drop takes either.
*/
int d_mongodb_drop_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    unsigned           _flags
)
{
    return d_internal_mongodb_drop_kind(_db, _database, _name, _flags, false);
}

// 2.3    Renaming
//------------------------------------------------------------------------------
/*
d_mongodb_rename_collection
  renameCollection on admin, the namespaces whole: "db.collection".
*/
int d_mongodb_rename_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _from,
    const char*        _to_database,
    const char*        _to,
    unsigned           _flags
)
{
    const char* name = NULL;
    const char* to   = NULL;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_flags(_db, _flags, D_MONGODB_DROP_TARGET);

    result = (result == 0)
                 ? d_internal_mongodb_names(_db, _database, _from, &name)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_names(_db,
                                            (_to_database) ? _to_database
                                                           : name,
                                            _to,
                                            &to)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_ddl_allowed(_db,
                                                  "renameCollection",
                                                  false)
                 : result;

    // the namespaces, whole
    if (result == 0)
    {
        char* const from_ns = bson_strdup_printf("%s.%s", name, _from);
        char* const to_ns   = bson_strdup_printf("%s.%s", to, _to);
        bson_t      command;

        bson_init(&command);
        BSON_APPEND_UTF8(&command, "renameCollection", from_ns);
        BSON_APPEND_UTF8(&command, "to", to_ns);
        BSON_APPEND_BOOL(&command,
                         "dropTarget",
                         ((_flags & D_MONGODB_DROP_TARGET) != 0u));
        result = d_internal_mongodb_run_ddl(_db, "admin", &command);
        bson_destroy(&command);
        bson_free(to_ns);
        bson_free(from_ns);
    }

    return result;
}

// 2.4    Listing
//------------------------------------------------------------------------------
/*
d_mongodb_list_collections
  listCollections of every type but "view".
*/
int d_mongodb_list_collections(
    struct dj_mongodb*      _db,
    const char*             _database,
    struct dj_db_name_list* _names
)
{
    return d_internal_mongodb_list_of(_db,
                                      _database,
                                      D_INTERNAL_MONGODB_COLLECTION,
                                      _names);
}

// 2.5    Description
//------------------------------------------------------------------------------
/**
 * @brief Writes out the first entry listCollections' cursor finds.
 *
 * @param[in,out] _db      the handle.
 * @param[in,out] _cursor  the cursor.
 * @param[in]     _name    the name it looked for, for a message.
 * @param[out]    _info    receives the entry, as canonical Extended JSON,
 *                         on success.
 * @return 0; the cursor's failure; 26 for no entry of the name;
 *         D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_first_entry(
    struct dj_mongodb* _db,
    mongoc_cursor_t*   _cursor,
    const char*        _name,
    char**             _info
)
{
    const bson_t* entry = NULL;
    bson_error_t  error;
    const bson_t* reply = NULL;

    // no entry, or the cursor's failure
    if (!mongoc_cursor_next(_cursor, &entry))
    {
        return (mongoc_cursor_error_document(_cursor, &error, &reply))
                   ? d_internal_mongodb_fail_outside(_db, &error, reply)
                   : d_internal_mongodb_answer(
                         _db,
                         D_MONGODB_ERROR_NAMESPACE_NOT_FOUND,
                         "no collection or view named \"%s\"",
                         _name);
    }

    char* const text = d_internal_mongodb_json(entry,
                                               D_MONGODB_JSON_CANONICAL);

    // the entry, written out
    if (!text)
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the description");
    }

    *_info = text;

    return 0;
}

/*
d_mongodb_collection_info
  listCollections by name, not nameOnly, so that the server reads the
options and the info; outside any transaction.
*/
int d_mongodb_collection_info(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    char**             _info
)
{
    const char* name = NULL;

    // a handle, the names, and somewhere for the description
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = (_info) ? d_internal_mongodb_names(_db,
                                                    _database,
                                                    _name,
                                                    &name)
                         : d_internal_mongodb_answer(
                               _db,
                               D_DB_INVALID_ARGUMENT,
                               "no place was given for the description");

    // names to look for
    if (result != 0)
    {
        return result;
    }

    mongoc_database_t* const database =
        mongoc_client_get_database(_db->client, name);
    bson_t                   opts;
    bson_t                   filter;

    bson_init(&opts);
    BSON_APPEND_DOCUMENT_BEGIN(&opts, "filter", &filter);
    BSON_APPEND_UTF8(&filter, "name", _name);
    bson_append_document_end(&opts, &filter);

    mongoc_cursor_t* const cursor =
        mongoc_database_find_collections_with_opts(database, &opts);

    result = d_internal_mongodb_first_entry(_db, cursor, _name, _info);
    mongoc_cursor_destroy(cursor);
    bson_destroy(&opts);
    mongoc_database_destroy(database);

    return result;
}


//==============================================================================
// 3.  VIEWS
//==============================================================================

// 3.1    Existence
//------------------------------------------------------------------------------
/*
d_mongodb_has_view
  The _s form, with every failure read as "no".
*/
bool d_mongodb_has_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
)
{
    bool exists = false;

    return ( (d_mongodb_has_view_s(_db, _database, _name, &exists) == 0) &&
             (exists) );
}

/*
d_mongodb_has_view_s
  listCollections by name, of type "view".
*/
int d_mongodb_has_view_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    bool*              _exists
)
{
    return d_internal_mongodb_has_kind(_db,
                                       _database,
                                       _name,
                                       D_INTERNAL_MONGODB_VIEW,
                                       _exists);
}

// 3.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_mongodb_create_view
  {create: name, viewOn: source, pipeline: [...], ...options}.
*/
int d_mongodb_create_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    const char*        _source,
    const char*        _pipeline,
    const char*        _options,
    unsigned           _flags
)
{
    const char* name = NULL;
    bson_t      pipeline;
    bson_t      options;
    bson_t      command;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&pipeline);
    bson_init(&options);
    bson_init(&command);

    int result = d_internal_mongodb_flags(_db, _flags, D_DB_IF_NOT_EXISTS);

    result = (result == 0)
                 ? d_internal_mongodb_names(_db, _database, _name, &name)
                 : result;
    result = ( (result == 0) &&
               ( (!_source) ||
                 (!*_source) ) )
                 ? d_internal_mongodb_answer(_db,
                                             D_DB_INVALID_ARGUMENT,
                                             "a view needs its source")
                 : result;
    result = (result == 0) ? d_internal_mongodb_read_array(_db,
                                                           _pipeline,
                                                           false,
                                                           &pipeline)
                           : result;
    result = (result == 0) ? d_internal_mongodb_read_document(_db,
                                                              _options,
                                                              false,
                                                              &options)
                           : result;

    // the command, then the create
    if (result == 0)
    {
        BSON_APPEND_UTF8(&command, "create", _name);
        BSON_APPEND_UTF8(&command, "viewOn", _source);
        BSON_APPEND_ARRAY(&command, "pipeline", &pipeline);
        bson_concat(&command, &options);
        result = d_internal_mongodb_create_kind(_db,
                                                name,
                                                &command,
                                                _name,
                                                _flags);
    }

    bson_destroy(&command);
    bson_destroy(&options);
    bson_destroy(&pipeline);

    return result;
}

/*
d_mongodb_drop_view
  A view, and never a collection of the name.
*/
int d_mongodb_drop_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    unsigned           _flags
)
{
    return d_internal_mongodb_drop_kind(_db, _database, _name, _flags, true);
}

// 3.3    Listing
//------------------------------------------------------------------------------
/*
d_mongodb_list_views
  listCollections of type "view".
*/
int d_mongodb_list_views(
    struct dj_mongodb*      _db,
    const char*             _database,
    struct dj_db_name_list* _names
)
{
    return d_internal_mongodb_list_of(_db,
                                      _database,
                                      D_INTERNAL_MONGODB_VIEW,
                                      _names);
}


//==============================================================================
// 4.  INDEXES
//==============================================================================

/**
 * @brief Opens a listIndexes cursor on a collection, outside any
 *        transaction.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database, or `NULL`.
 * @param[in]     _collection  the collection.
 * @param[out]    _cursor      receives the cursor, or `NULL`.
 * @return 0, or the failure.
 *
 * The driver reads a collection that is not there as one of no indexes,
 * as the Index Enumeration specification has it.
 */
static
int d_internal_mongodb_index_cursor(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    mongoc_cursor_t**  _cursor
)
{
    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    *_cursor = NULL;

    // the collection
    if (!collection)
    {
        return _db->error;
    }

    *_cursor = mongoc_collection_find_indexes_with_opts(collection, NULL);
    mongoc_collection_destroy(collection);

    return 0;
}

/**
 * @brief Whether a collection has an index of a name.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database, or `NULL`.
 * @param[in]     _collection  the collection.
 * @param[in]     _name        the index's name.
 * @param[out]    _exists      receives the answer.
 * @return 0, or the failure.
 */
static
int d_internal_mongodb_index_exists(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    bool*              _exists
)
{
    mongoc_cursor_t* cursor = NULL;
    const bson_t*    entry  = NULL;
    bson_error_t     error;
    const bson_t*    reply  = NULL;
    int              result = d_internal_mongodb_index_cursor(_db,
                                                              _database,
                                                              _collection,
                                                              &cursor);

    *_exists = false;

    // each index, by its name
    while ( (result == 0)                      &&
            (!*_exists)                        &&
            (mongoc_cursor_next(cursor, &entry)) )
    {
        bson_iter_t name;

        *_exists = ( (bson_iter_init_find(&name, entry, "name")) &&
                     (BSON_ITER_HOLDS_UTF8(&name))              &&
                     (strcmp(bson_iter_utf8(&name, NULL), _name) == 0) );
    }

    // the cursor's failure
    if ( (result == 0) &&
         (mongoc_cursor_error_document(cursor, &error, &reply)) )
    {
        *_exists = false;
        result   = d_internal_mongodb_fail_outside(_db, &error, reply);
    }

    mongoc_cursor_destroy(cursor);

    return result;
}

// 4.1    Existence
//------------------------------------------------------------------------------
/*
d_mongodb_has_index
  The _s form, with every failure read as "no".
*/
bool d_mongodb_has_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name
)
{
    bool exists = false;

    return ( (d_mongodb_has_index_s(_db,
                                    _database,
                                    _collection,
                                    _name,
                                    &exists) == 0) &&
             (exists) );
}

/*
d_mongodb_has_index_s
  listIndexes, by name.
*/
int d_mongodb_has_index_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
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
                                         "an index's name and a place for "
                                         "the answer are needed");
    }

    return d_internal_mongodb_index_exists(_db,
                                           _database,
                                           _collection,
                                           _name,
                                           _exists);
}

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Builds createIndexes for one index.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the collection.
 * @param[in]     _name        the index's name.
 * @param[in]     _keys        its keys, read.
 * @param[in]     _options     its options, read.
 * @param[in]     _flags       D_DB_UNIQUE, or 0.
 * @param[out]    _command     initialized, and receives the command.
 * @return 0, or D_DB_INVALID_ARGUMENT for options that name the index's
 *         keys or name, which the arguments give, or that unique it
 *         against D_DB_UNIQUE.
 */
static
int d_internal_mongodb_index_command(
    struct dj_mongodb* _db,
    const char*        _collection,
    const char*        _name,
    const bson_t*      _keys,
    const bson_t*      _options,
    unsigned           _flags,
    bson_t*            _command
)
{
    const bool unique = ((_flags & D_DB_UNIQUE) != 0u);
    bson_t     indexes;
    bson_t     index;

    bson_init(_command);

    // the arguments say the keys and the name, and the flag uniqueness
    if ( (bson_has_field(_options, "key"))  ||
         (bson_has_field(_options, "name")) ||
         ( (unique) &&
           (bson_has_field(_options, "unique")) ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "an index's options name neither "
                                         "its keys nor its name, nor unique "
                                         "beside D_DB_UNIQUE");
    }

    BSON_APPEND_UTF8(_command, "createIndexes", _collection);
    bson_init(&indexes);
    BSON_APPEND_DOCUMENT_BEGIN(&indexes, "0", &index);
    BSON_APPEND_DOCUMENT(&index, "key", _keys);
    BSON_APPEND_UTF8(&index, "name", _name);

    // uniqueness, where the flag asks
    if (unique)
    {
        BSON_APPEND_BOOL(&index, "unique", true);
    }

    bson_concat(&index, _options);
    bson_append_document_end(&indexes, &index);
    BSON_APPEND_ARRAY(_command, "indexes", &indexes);
    bson_destroy(&indexes);

    return 0;
}

/**
 * @brief Reads an index's name, keys and options, and makes its
 *        createIndexes.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the collection.
 * @param[in]     _name        the index's name, which must be given.
 * @param[in]     _keys        its keys, JSON.
 * @param[in]     _options     its options, JSON, or `NULL`.
 * @param[in]     _flags       the flags.
 * @param[in,out] _command     an initialized document, which receives the
 *                             command.
 * @return 0, or the refusal: D_DB_INVALID_ARGUMENT for no name; the JSON's;
 *         d_internal_mongodb_index_command's.
 */
static
int d_internal_mongodb_index_read(
    struct dj_mongodb* _db,
    const char*        _collection,
    const char*        _name,
    const char*        _keys,
    const char*        _options,
    unsigned           _flags,
    bson_t*            _command
)
{
    bson_t keys;
    bson_t options;

    // a name
    if ( (!_name) ||
         (!*_name) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "an index needs its name");
    }

    int result = d_internal_mongodb_read_document(_db, _keys, true, &keys);

    bson_init(&options);
    result = (result == 0) ? d_internal_mongodb_read_document(_db,
                                                              _options,
                                                              false,
                                                              &options)
                           : result;
    result = (result == 0) ? d_internal_mongodb_index_command(_db,
                                                              _collection,
                                                              _name,
                                                              &keys,
                                                              &options,
                                                              _flags,
                                                              _command)
                           : result;

    bson_destroy(&options);
    bson_destroy(&keys);

    return result;
}

/*
d_mongodb_create_index
  An index of the name that is there is D_DB_EXISTS, which no server
reports: each takes the same index again in silence, and refuses another
of the name with 86.
*/
int d_mongodb_create_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    const char*        _keys,
    const char*        _options,
    unsigned           _flags
)
{
    const char* name   = NULL;
    bool        exists = false;
    bson_t      command;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&command);

    int result = d_internal_mongodb_flags(_db,
                                          _flags,
                                          D_DB_IF_NOT_EXISTS | D_DB_UNIQUE);

    result = (result == 0)
                 ? d_internal_mongodb_names(_db, _database, _collection, &name)
                 : result;
    result = (result == 0) ? d_internal_mongodb_index_read(_db,
                                                           _collection,
                                                           _name,
                                                           _keys,
                                                           _options,
                                                           _flags,
                                                           &command)
                           : result;
    result = (result == 0)
                 ? d_internal_mongodb_ddl_allowed(_db, "createIndexes", true)
                 : result;
    result = (result == 0) ? d_internal_mongodb_index_exists(_db,
                                                             name,
                                                             _collection,
                                                             _name,
                                                             &exists)
                           : result;

    // an index of the name, there already
    if ( (result == 0) &&
         (exists) )
    {
        result = ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
                     ? 0
                     : d_internal_mongodb_answer(_db,
                                                 D_DB_EXISTS,
                                                 "an index named \"%s\" is "
                                                 "already there",
                                                 _name);
    }
    else if (result == 0)
    {
        result = d_internal_mongodb_run_ddl(_db, name, &command);
    }

    bson_destroy(&command);

    return result;
}

/*
d_mongodb_drop_index
  dropIndexes of the one name, after asking, so that one that is not there
is 27 on every server, 8.3's included, which drops nothing in silence.
*/
int d_mongodb_drop_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    unsigned           _flags
)
{
    const char* name   = NULL;
    bool        exists = false;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_flags(_db, _flags, D_DB_IF_EXISTS);

    result = (result == 0)
                 ? d_internal_mongodb_names(_db, _database, _collection, &name)
                 : result;
    result = ( (result == 0) &&
               (!_name) )
                 ? d_internal_mongodb_answer(_db,
                                             D_DB_INVALID_ARGUMENT,
                                             "no index was named")
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_ddl_allowed(_db, "dropIndexes", false)
                 : result;
    result = (result == 0) ? d_internal_mongodb_index_exists(_db,
                                                             name,
                                                             _collection,
                                                             _name,
                                                             &exists)
                           : result;

    // a failure, which sends nothing
    if (result != 0)
    {
        return result;
    }

    // none of the name
    if (!exists)
    {
        return ((_flags & D_DB_IF_EXISTS) != 0u)
                   ? 0
                   : d_internal_mongodb_answer(
                         _db,
                         D_MONGODB_ERROR_INDEX_NOT_FOUND,
                         "no index named \"%s\"",
                         _name);
    }

    bson_t command;

    bson_init(&command);
    BSON_APPEND_UTF8(&command, "dropIndexes", _collection);
    BSON_APPEND_UTF8(&command, "index", _name);
    result = d_internal_mongodb_run_ddl(_db, name, &command);
    bson_destroy(&command);

    return result;
}

// 4.3    Listing
//------------------------------------------------------------------------------
/*
d_mongodb_list_indexes
  listIndexes' names, in its order, which puts "_id_" first.
*/
int d_mongodb_list_indexes(
    struct dj_mongodb*      _db,
    const char*             _database,
    const char*             _collection,
    struct dj_db_name_list* _names
)
{
    mongoc_cursor_t* cursor = NULL;

    // a handle, and somewhere for the names
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = (_names) ? d_internal_mongodb_index_cursor(_db,
                                                            _database,
                                                            _collection,
                                                            &cursor)
                          : d_internal_mongodb_answer(
                                _db,
                                D_DB_INVALID_ARGUMENT,
                                "no place was given for the names");

    struct dj_internal_db_names builder = { { NULL, 0u, 0u },
                                           { NULL, 0u, 0u },
                                           0u };
    const bson_t*              entry   = NULL;
    bool                       ok      = true;
    bson_error_t               error;
    const bson_t*              reply   = NULL;

    // each name
    while ( (result == 0) &&
            (ok)          &&
            (mongoc_cursor_next(cursor, &entry)) )
    {
        bson_iter_t name;

        ok = ( (!bson_iter_init_find(&name, entry, "name")) ||
               (!BSON_ITER_HOLDS_UTF8(&name))              ||
               (d_internal_db_names_add(&builder,
                                        bson_iter_utf8(&name, NULL))) );
    }

    // the cursor's failure, or the names, packed
    if ( (result == 0) &&
         (mongoc_cursor_error_document(cursor, &error, &reply)) )
    {
        result = d_internal_mongodb_fail_outside(_db, &error, reply);
    }
    else if ( (result == 0) &&
              ( (!ok) ||
                (!d_internal_db_names_finish(&builder, _names)) ) )
    {
        result = d_internal_mongodb_answer(_db,
                                           D_MONGODB_LAYER_NO_MEMORY,
                                           "no memory for the names listed");
    }

    d_internal_db_names_release(&builder);
    mongoc_cursor_destroy(cursor);

    return result;
}

/*
d_mongodb_describe_indexes
  listIndexes' documents, as rows.
*/
int d_mongodb_describe_indexes(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    struct dj_db_rows* _rows
)
{
    mongoc_cursor_t* cursor = NULL;

    // a handle, and somewhere for the rows
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = (_rows) ? d_internal_mongodb_index_cursor(_db,
                                                           _database,
                                                           _collection,
                                                           &cursor)
                         : d_internal_mongodb_answer(
                               _db,
                               D_DB_INVALID_ARGUMENT,
                               "no place was given for the rows");

    result = (result == 0) ? d_internal_mongodb_cursor_rows(_db,
                                                            cursor,
                                                            (size_t)-1,
                                                            _rows,
                                                            NULL,
                                                            false)
                           : result;
    mongoc_cursor_destroy(cursor);

    return result;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports what a build
// without the driver can: D_DB_UNSUPPORTED, or false where it cannot fail.
// An answer a function promises even on failure is still written.

/*
d_mongodb_has_collection
  Without the driver, false.
*/
bool d_mongodb_has_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
)
{
    (void)_db;
    (void)_database;
    (void)_name;

    return false;
}

/*
d_mongodb_has_collection_s
  Without the driver, D_DB_UNSUPPORTED, and the answer false.
*/
int d_mongodb_has_collection_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    bool*              _exists
)
{
    (void)_db;
    (void)_database;
    (void)_name;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_create_collection
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_create_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    const char*        _options,
    unsigned           _flags
)
{
    (void)_db;
    (void)_database;
    (void)_name;
    (void)_options;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_drop_collection
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_drop_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    unsigned           _flags
)
{
    (void)_db;
    (void)_database;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_rename_collection
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_rename_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _from,
    const char*        _to_database,
    const char*        _to,
    unsigned           _flags
)
{
    (void)_db;
    (void)_database;
    (void)_from;
    (void)_to_database;
    (void)_to;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_list_collections
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_list_collections(
    struct dj_mongodb*      _db,
    const char*             _database,
    struct dj_db_name_list* _names
)
{
    (void)_db;
    (void)_database;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_collection_info
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_collection_info(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    char**             _info
)
{
    (void)_db;
    (void)_database;
    (void)_name;
    (void)_info;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_has_view
  Without the driver, false.
*/
bool d_mongodb_has_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
)
{
    (void)_db;
    (void)_database;
    (void)_name;

    return false;
}

/*
d_mongodb_has_view_s
  Without the driver, D_DB_UNSUPPORTED, and the answer false.
*/
int d_mongodb_has_view_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    bool*              _exists
)
{
    (void)_db;
    (void)_database;
    (void)_name;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_create_view
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_create_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    const char*        _source,
    const char*        _pipeline,
    const char*        _options,
    unsigned           _flags
)
{
    (void)_db;
    (void)_database;
    (void)_name;
    (void)_source;
    (void)_pipeline;
    (void)_options;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_drop_view
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_drop_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    unsigned           _flags
)
{
    (void)_db;
    (void)_database;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_list_views
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_list_views(
    struct dj_mongodb*      _db,
    const char*             _database,
    struct dj_db_name_list* _names
)
{
    (void)_db;
    (void)_database;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_has_index
  Without the driver, false.
*/
bool d_mongodb_has_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_name;

    return false;
}

/*
d_mongodb_has_index_s
  Without the driver, D_DB_UNSUPPORTED, and the answer false.
*/
int d_mongodb_has_index_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    bool*              _exists
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_name;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_create_index
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_create_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    const char*        _keys,
    const char*        _options,
    unsigned           _flags
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_name;
    (void)_keys;
    (void)_options;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_drop_index
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_drop_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    unsigned           _flags
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_list_indexes
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_list_indexes(
    struct dj_mongodb*      _db,
    const char*             _database,
    const char*             _collection,
    struct dj_db_name_list* _names
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_describe_indexes
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_describe_indexes(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
