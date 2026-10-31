/*******************************************************************************
* djinterp [c]                                             mongodb_maintenance.c
*
* Implementation of mongodb_maintenance.h.
*   Each asks first whether its collection, or its database, is there, so
* that one that is not is D_DB_NOT_FOUND on every server, which otherwise
* answer with zeros or with a code that differs by release; and each runs
* outside any transaction, which holds none of them.
*   A collection's size is its documents' storage and its indexes', summed
* over what $collStats returns -- one document for each shard of a sharded
* collection -- which is 4.4's totalSize, on servers before it too. Every
* release's compact refuses a replica set's primary without force (17
* before 7.0, 20 from it), which the layer sends to every server, as the
* call asks for the compaction and a standalone or a secondary takes it
* too.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_maintenance.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_maintenance.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_database.h"  // has_database_s
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Checks that a collection of the name is there, a view not
 *        counting.
 *
 * @param[in,out] _db          the handle, not `NULL`.
 * @param[in]     _database    the database, or `NULL`.
 * @param[in]     _collection  the collection's name.
 * @param[out]    _name        receives the database's name.
 * @return 0; D_DB_NOT_FOUND for none; D_DB_INVALID_ARGUMENT for no
 *         database or no name; the failure.
 */
static
int d_internal_mongodb_collection_there(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char**       _name
)
{
    enum dj_internal_mongodb_kind kind = D_INTERNAL_MONGODB_NOTHING;

    *_name = d_internal_mongodb_database(_db, _database);

    // a database, and a collection's name
    if (!*_name)
    {
        return _db->error;
    }

    if ( (!_collection) ||
         (!*_collection) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "no collection was named");
    }

    const int result = d_internal_mongodb_kind_of(_db,
                                                  *_name,
                                                  _collection,
                                                  &kind);

    return ( (result == 0) &&
             (kind != D_INTERNAL_MONGODB_COLLECTION) )
               ? d_internal_mongodb_answer(_db,
                                           D_DB_NOT_FOUND,
                                           "no collection named \"%s\"",
                                           _collection)
               : result;
}

/**
 * @brief Reads a number from a document by its dotted path.
 *
 * @param[in] _document  the document.
 * @param[in] _path      the path.
 * @return the number; 0 where there is none.
 */
static
int64_t d_internal_mongodb_size_at(
    const bson_t* _document,
    const char*   _path
)
{
    bson_iter_t top;
    bson_iter_t found;

    return ( (bson_iter_init(&top, _document))                &&
             (bson_iter_find_descendant(&top, _path, &found)) &&
             (BSON_ITER_HOLDS_NUMBER(&found)) )
               ? bson_iter_as_int64(&found)
               : 0;
}

/**
 * @brief Reads validate's reply: its answer, and the reply itself as the
 *        report, where one is wanted.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _reply   the reply.
 * @param[out]    _valid   receives whether the server found it sound.
 * @param[out]    _report  receives the report; `NULL` where it is not
 *                         wanted.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY, neither answer written.
 */
static
int d_internal_mongodb_validated(
    struct dj_mongodb* _db,
    const bson_t*      _reply,
    bool*              _valid,
    char**             _report
)
{
    bson_iter_t field;
    char* const report = (_report)
                             ? d_internal_mongodb_json(_reply,
                                                       D_MONGODB_JSON_CANONICAL)
                             : NULL;

    // the report, where it is wanted
    if ( (_report) &&
         (!report) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the report");
    }

    *_valid = ( (bson_iter_init_find(&field, _reply, "valid")) &&
                (bson_iter_as_bool(&field)) );

    if (_report)
    {
        *_report = report;
    }

    return 0;
}

/**
 * @brief Sums a collection's storage and its indexes' over what $collStats
 *        returns: one document, or one for each shard.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database's name.
 * @param[in]     _collection  the collection.
 * @param[out]    _bytes       receives the sum, on success.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_storage_total(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    int64_t*           _bytes
)
{
    mongoc_collection_t* const collection =
        mongoc_client_get_collection(_db->client, _database, _collection);
    bson_t                     pipeline;
    bson_t                     stage;
    bson_t                     stats;
    bson_t                     storage;

    bson_init(&pipeline);
    BSON_APPEND_DOCUMENT_BEGIN(&pipeline, "0", &stage);
    BSON_APPEND_DOCUMENT_BEGIN(&stage, "$collStats", &stats);
    BSON_APPEND_DOCUMENT_BEGIN(&stats, "storageStats", &storage);
    bson_append_document_end(&stats, &storage);
    bson_append_document_end(&stage, &stats);
    bson_append_document_end(&pipeline, &stage);

    mongoc_cursor_t* const cursor = mongoc_collection_aggregate(
                                        collection,
                                        MONGOC_QUERY_NONE,
                                        &pipeline,
                                        NULL,
                                        NULL);
    const bson_t*          entry  = NULL;
    int64_t                total  = 0;
    bson_error_t           error;
    const bson_t*          reply  = NULL;

    // each shard's storage and indexes
    while (mongoc_cursor_next(cursor, &entry))
    {
        total += d_internal_mongodb_size_at(entry,
                                            "storageStats.storageSize") +
                 d_internal_mongodb_size_at(entry,
                                            "storageStats.totalIndexSize");
    }

    const int result = (mongoc_cursor_error_document(cursor, &error, &reply))
                           ? d_internal_mongodb_fail_outside(_db,
                                                             &error,
                                                             reply)
                           : 0;

    // the sum, on success
    if (result == 0)
    {
        *_bytes = total;
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(&pipeline);
    mongoc_collection_destroy(collection);

    return result;
}

//==============================================================================
// 2.  MAINTENANCE
//==============================================================================

// 2.1    Sizes
//------------------------------------------------------------------------------
/*
d_mongodb_collection_size
  $collStats with storageStats, its storageSize and totalIndexSize summed
over every document it returns.
*/
int d_mongodb_collection_size(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    int64_t*           _bytes
)
{
    const char* name = NULL;

    // a handle, and somewhere for the size
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = (_bytes)
                           ? d_internal_mongodb_collection_there(_db,
                                                                 _database,
                                                                 _collection,
                                                                 &name)
                           : d_internal_mongodb_answer(
                                 _db,
                                 D_DB_INVALID_ARGUMENT,
                                 "no place was given for the size");

    return (result == 0) ? d_internal_mongodb_storage_total(_db,
                                                            name,
                                                            _collection,
                                                            _bytes)
                         : result;
}

/*
d_mongodb_database_size
  dbStats, its storageSize and indexSize summed, for a database that is
there: one that is not, dbStats reckons at zero.
*/
int d_mongodb_database_size(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _bytes
)
{
    bool exists = false;

    // a handle, and somewhere for the size
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const char* const name = d_internal_mongodb_database(_db, _database);

    int result = (!name)
                     ? _db->error
                     : (!_bytes)
                           ? d_internal_mongodb_answer(
                                 _db,
                                 D_DB_INVALID_ARGUMENT,
                                 "no place was given for the size")
                           : d_mongodb_has_database_s(_db, name, &exists);

    // a database that is there
    if ( (result == 0) &&
         (!exists) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_NOT_FOUND,
                                         "no database \"%s\"",
                                         name);
    }

    bson_t command;
    bson_t reply;

    bson_init(&command);
    bson_init(&reply);
    BSON_APPEND_INT32(&command, "dbStats", 1);
    result = (result == 0)
                 ? d_internal_mongodb_run(_db, name, &command, true, &reply)
                 : result;

    // the size, on success
    if (result == 0)
    {
        *_bytes = d_internal_mongodb_size_at(&reply, "storageSize") +
                  d_internal_mongodb_size_at(&reply, "indexSize");
    }

    bson_destroy(&reply);
    bson_destroy(&command);

    return result;
}

// 2.2    Compaction
//------------------------------------------------------------------------------
/*
d_mongodb_compact
  compact on the collection's database, outside any transaction.
*/
int d_mongodb_compact(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection
)
{
    const char* name = NULL;

    // a handle, and a collection that is there
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_collection_there(_db,
                                                     _database,
                                                     _collection,
                                                     &name);

    // no command for a collection that is not there
    if (result != 0)
    {
        return result;
    }

    bson_t command;
    bson_t reply;

    bson_init(&command);
    bson_init(&reply);
    BSON_APPEND_UTF8(&command, "compact", _collection);
    BSON_APPEND_BOOL(&command, "force", true);

    result = d_internal_mongodb_run(_db, name, &command, true, &reply);

    bson_destroy(&reply);
    bson_destroy(&command);

    return result;
}

// 2.3    Validation
//------------------------------------------------------------------------------
/*
d_mongodb_validate
  validate, its reply's valid the answer and the reply the report.
*/
int d_mongodb_validate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    unsigned           _flags,
    bool*              _valid,
    char**             _report
)
{
    const char* name = NULL;

    // a handle, flags it knows, somewhere for the answer, a collection
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = ((_flags & ~(unsigned)D_MONGODB_VALIDATE_FULL) != 0u)
                     ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "flags of no meaning here, "
                                                 "0x%x",
                                                 _flags)
                     : (!_valid)
                           ? d_internal_mongodb_answer(
                                 _db,
                                 D_DB_INVALID_ARGUMENT,
                                 "no place was given for the answer")
                           : d_internal_mongodb_collection_there(_db,
                                                                 _database,
                                                                 _collection,
                                                                 &name);

    // no command for a collection that is not there
    if (result != 0)
    {
        return result;
    }

    bson_t command;
    bson_t reply;

    bson_init(&command);
    bson_init(&reply);
    BSON_APPEND_UTF8(&command, "validate", _collection);
    BSON_APPEND_BOOL(&command,
                     "full",
                     ((_flags & D_MONGODB_VALIDATE_FULL) != 0u));
    result = d_internal_mongodb_run(_db, name, &command, true, &reply);
    result = (result == 0) ? d_internal_mongodb_validated(_db,
                                                          &reply,
                                                          _valid,
                                                          _report)
                           : result;

    bson_destroy(&reply);
    bson_destroy(&command);

    return result;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports D_DB_UNSUPPORTED,
// its answers untouched.

/*
d_mongodb_collection_size
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_collection_size(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    int64_t*           _bytes
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_bytes;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_database_size
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_database_size(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _bytes
)
{
    (void)_db;
    (void)_database;
    (void)_bytes;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_compact
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_compact(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection
)
{
    (void)_db;
    (void)_database;
    (void)_collection;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_validate
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_validate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    unsigned           _flags,
    bool*              _valid,
    char**             _report
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_flags;
    (void)_valid;
    (void)_report;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
