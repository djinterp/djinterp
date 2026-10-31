/*******************************************************************************
* djinterp [c]                                                mongodb_document.c
*
* Implementation of mongodb_document.h.
*   Each function is the driver's own CRUD call -- insert_one, find_with_opts,
* update_one, count_documents, find_and_modify_with_opts, a bulk operation --
* on a collection the driver hands out from the handle's client, so that
* reads follow the client's read concern and read preference and writes its
* write concern, as the drivers' CRUD specification has them; the layer
* reads the JSON, adds the session of an open transaction and the handle's
* time limit, and turns the replies into counts and rows.
*   The driver checks an update for its operators and a replacement for
* none; findAndModify, where it does not, the layer checks, since the server
* would read an update without operators as a replacement. An insert_one's
* document without an _id is given an ObjectId here, first among its fields,
* so that the id is known; insert_many's, by the driver.
*   estimatedDocumentCount runs the count command, which no transaction may
* hold; inside one, the layer refuses it rather than send it, which would end
* the transaction.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_document.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.29
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_document.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // free, malloc, strtoul
#include <string.h>  // memset, strchr, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, dj_db_rows
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_transaction.h"  // in_transaction
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "../db_internal.h"       // the rows builder
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, uint32_t,
                                                         // UINT32_MAX


//==============================================================================
// FILE-LOCAL DEFINITIONS OF BOTH BUILDS
//==============================================================================

/**
 * @brief Writes the counts of no writes, where they are wanted: what a
 *        bulk write writes before it sends anything, so that a refusal,
 *        or a build without the driver, leaves them true.
 *
 * @param[out] _counts  the counts, or `NULL`.
 */
static
void d_internal_mongodb_no_counts(
    struct dj_mongodb_write_counts* _counts
)
{
    static const struct dj_mongodb_write_counts NONE = { 0, 0, 0, 0, 0 };

    // zeros, where there is somewhere for them
    if (_counts)
    {
        *_counts = NONE;
    }

    return;
}


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// dj_internal_mongodb_update_kind
//   enum: which of the three updates a call makes.
enum dj_internal_mongodb_update_kind
{
    D_INTERNAL_MONGODB_UPDATE_ONE = 0,  // the first document matched
    D_INTERNAL_MONGODB_UPDATE_MANY,     // every document matched
    D_INTERNAL_MONGODB_REPLACE_ONE      // the first, replaced
};

// dj_internal_mongodb_modify_kind
//   enum: what a find-and-modify does to the document it finds.
enum dj_internal_mongodb_modify_kind
{
    D_INTERNAL_MONGODB_MODIFY_UPDATE = 0,  // updates it
    D_INTERNAL_MONGODB_MODIFY_REPLACE,     // replaces it
    D_INTERNAL_MONGODB_MODIFY_DELETE       // deletes it
};

// D_INTERNAL_MONGODB_KNOWN_FLAGS
//   constant: the flags the functions that take flags read.
#define D_INTERNAL_MONGODB_KNOWN_FLAGS  (D_MONGODB_UNORDERED)

// d_internal_mongodb_empty_rows
//   constant: rows as nothing has written them.
static const struct dj_db_rows d_internal_mongodb_empty_rows = { 0u, 0u,
                                                                NULL, NULL };


//==============================================================================
// 1.  PLUMBING
//==============================================================================

/**
 * @brief Refuses flags this layer gives no meaning.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _flags  the flags.
 * @return 0, or D_DB_INVALID_ARGUMENT, recorded.
 */
static
int d_internal_mongodb_check_flags(
    struct dj_mongodb* _db,
    unsigned           _flags
)
{
    return ((_flags & ~(unsigned)D_INTERNAL_MONGODB_KNOWN_FLAGS) != 0u)
               ? d_internal_mongodb_answer(_db,
                                           D_DB_INVALID_ARGUMENT,
                                           "flags of no meaning here, 0x%x",
                                           _flags)
               : 0;
}

/**
 * @brief Reads a reply's upsertedId, as an update's reply names it.
 *
 * @param[in]  _reply  the reply.
 * @param[out] _id     receives the value, copied out of its iterator; its
 *                     own pointers are into the reply.
 * @return `true` where the reply has one.
 */
static
bool d_internal_mongodb_upserted_id(
    const bson_t* _reply,
    bson_value_t* _id
)
{
    bson_iter_t field;

    // the value, where there is one
    if (bson_iter_init_find(&field, _reply, "upsertedId"))
    {
        *_id = *bson_iter_value(&field);

        return true;
    }

    return false;
}

/**
 * @brief Ends a write the driver made: its failure recorded, or its counts
 *        read, recorded on the handle with an upsert's new _id, and handed
 *        back.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _ok      what the driver returned.
 * @param[in]     _error   its failure, where it failed.
 * @param[in]     _reply   its reply.
 * @param[out]    _counts  receives the counts on success; may be `NULL`.
 * @return 0, or the failure.
 */
static
int d_internal_mongodb_written(
    struct dj_mongodb*              _db,
    bool                            _ok,
    const bson_error_t*             _error,
    const bson_t*                   _reply,
    struct dj_mongodb_write_counts* _counts
)
{
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
    bson_value_t                  id;

    // the driver's failure, or the server's
    if (!_ok)
    {
        return d_internal_mongodb_fail(_db, _error, _reply);
    }

    d_internal_mongodb_counts(_reply, &counts);
    d_internal_mongodb_wrote(_db, &counts);

    // an upsert's new document
    if (d_internal_mongodb_upserted_id(_reply, &id))
    {
        d_internal_mongodb_new_id(_db, &id);
    }

    // the counts, where they are wanted
    if (_counts)
    {
        *_counts = counts;
    }

    return 0;
}

/**
 * @brief Reads a cursor's every document into rows, written only on
 *        success, and destroys the cursor.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _cursor  the cursor.
 * @param[out]    _rows    receives the rows.
 * @return as d_internal_mongodb_cursor_rows.
 */
static
int d_internal_mongodb_drain(
    struct dj_mongodb* _db,
    mongoc_cursor_t*   _cursor,
    struct dj_db_rows* _rows
)
{
    struct dj_db_rows rows   = d_internal_mongodb_empty_rows;
    const int        result = d_internal_mongodb_cursor_rows(_db,
                                                             _cursor,
                                                             (size_t)-1,
                                                             &rows,
                                                             NULL,
                                                             true);

    mongoc_cursor_destroy(_cursor);

    // the rows, on success
    if (result == 0)
    {
        *_rows = rows;
    }

    return result;
}

/**
 * @brief Makes rows of one document.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _document  the document.
 * @param[out]    _rows      receives the one row.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_one_row(
    struct dj_mongodb* _db,
    const bson_t*      _document,
    struct dj_db_rows* _rows
)
{
    struct dj_internal_db_rows builder = { { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          0u,
                                          0u };

    // the column, the document, the rows
    if ( (!d_internal_mongodb_rows_column(&builder, false))                ||
         (!d_internal_mongodb_rows_document(&builder,
                                            _document,
                                            _db->format))                 ||
         (!d_internal_db_rows_finish(&builder, _rows)) )
    {
        d_internal_db_rows_release(&builder);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the document");
    }

    return 0;
}

/**
 * @brief Whether an update document's first field is an operator.
 *
 * @param[in] _update  the update, a document.
 * @return `true` where it begins with `$`.
 */
static
bool d_internal_mongodb_has_operators(
    const bson_t* _update
)
{
    bson_iter_t first;

    return ( (bson_iter_init(&first, _update)) &&
             (bson_iter_next(&first))          &&
             (bson_iter_key(&first)[0] == '$') );
}


//==============================================================================
// 2.  DOCUMENTS
//==============================================================================

// 2.1    Inserting
//------------------------------------------------------------------------------
/**
 * @brief Gives a document an ObjectId, first among its fields, where it
 *        has no _id.
 *
 * @param[in]  _given  the document.
 * @param[out] _out    initialized, and receives the document with its id;
 *                     left empty where the document has one.
 * @return the document to insert: `_given` or `_out`.
 */
static
const bson_t* d_internal_mongodb_with_id(
    const bson_t* _given,
    bson_t*       _out
)
{
    bson_oid_t oid;

    bson_init(_out);

    // an _id of its own
    if (bson_has_field(_given, "_id"))
    {
        return _given;
    }

    bson_oid_init(&oid, NULL);
    BSON_APPEND_OID(_out, "_id", &oid);
    bson_concat(_out, _given);

    return _out;
}

/**
 * @brief Records an inserted document's _id, and writes it out where it is
 *        wanted.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _document  the document inserted.
 * @param[out]    _id        receives the id in JSON, or `NULL`.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY, the document in all the same.
 */
static
int d_internal_mongodb_inserted_id(
    struct dj_mongodb* _db,
    const bson_t*      _document,
    char**             _id
)
{
    bson_iter_t  field;
    bson_value_t id;

    // the _id, which the document has
    if (!bson_iter_init_find(&field, _document, "_id"))
    {
        return 0;
    }

    id = *bson_iter_value(&field);
    d_internal_mongodb_new_id(_db, &id);

    // the text, where it is wanted
    if (_id)
    {
        char* const text = d_internal_mongodb_value_json(&id, _db->format);

        if (!text)
        {
            return d_internal_mongodb_answer(_db,
                                             D_MONGODB_LAYER_NO_MEMORY,
                                             "the document went in; no "
                                             "memory for its id");
        }

        *_id = text;
    }

    return 0;
}

/*
d_mongodb_insert_one
  The id is the layer's where the document has none, so that it can be
handed back without a second read.
*/
int d_mongodb_insert_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _document,
    char**             _id
)
{
    // a handle to insert through
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    // a collection to insert into
    if (!collection)
    {
        return _db->error;
    }

    bson_t       given;
    bson_t       with_id;
    bson_t       opts;
    bson_t       reply;
    bson_error_t error;
    int          result = d_internal_mongodb_read_document(_db,
                                                           _document,
                                                           true,
                                                           &given);
    const bson_t* const document = d_internal_mongodb_with_id(&given,
                                                              &with_id);

    bson_init(&reply);
    bson_init(&opts);
    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              NULL,
                                              D_INTERNAL_MONGODB_SESSION,
                                              &opts)
                 : result;

    // the insert, then its id
    if (result == 0)
    {
        const bool ok = mongoc_collection_insert_one(collection,
                                                     document,
                                                     &opts,
                                                     &reply,
                                                     &error);

        result = d_internal_mongodb_written(_db, ok, &error, &reply, NULL);
        result = (result == 0)
                     ? d_internal_mongodb_inserted_id(_db, document, _id)
                     : result;
    }

    bson_destroy(&reply);
    bson_destroy(&opts);
    bson_destroy(&with_id);
    bson_destroy(&given);
    mongoc_collection_destroy(collection);

    return result;
}

/**
 * @brief Reads an array's elements as documents that read its bytes in
 *        place.
 *
 * @param[in,out] _db         the handle.
 * @param[in]     _array      the array.
 * @param[out]    _documents  receives the documents, malloc's, freed by the
 *                            caller with free.
 * @param[out]    _pointers   receives a pointer to each, malloc's likewise.
 * @param[out]    _count      receives how many.
 * @return 0; D_DB_INVALID_ARGUMENT for no element, or one that is not a
 *         document; D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_documents_of(
    struct dj_mongodb* _db,
    const bson_t*      _array,
    bson_t**           _documents,
    const bson_t***    _pointers,
    size_t*            _count
)
{
    const size_t count = (size_t)bson_count_keys(_array);
    bson_iter_t  element;
    size_t       index = 0u;

    *_documents = NULL;
    *_pointers  = NULL;
    *_count     = 0u;

    // a write needs something to write
    if (count == 0u)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "the array given has no documents");
    }

    *_documents = (bson_t*)malloc(count * sizeof(bson_t));
    *_pointers  = (const bson_t**)malloc(count * sizeof(bson_t*));

    // room for them
    if ( (!*_documents) ||
         (!*_pointers)  ||
         (!bson_iter_init(&element, _array)) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the documents");
    }

    // each element, a document read in place
    while ( (index < count) &&
            (bson_iter_next(&element)) )
    {
        uint32_t       length = 0u;
        const uint8_t* data   = NULL;

        if (!BSON_ITER_HOLDS_DOCUMENT(&element))
        {
            return d_internal_mongodb_answer(_db,
                                             D_DB_INVALID_ARGUMENT,
                                             "element %lu of the array is "
                                             "not a document",
                                             (unsigned long)index);
        }

        bson_iter_document(&element, &length, &data);
        (void)bson_init_static(&(*_documents)[index], data, length);
        (*_pointers)[index] = &(*_documents)[index];
        ++index;
    }

    *_count = index;

    return 0;
}

/**
 * @brief Inserts documents through the driver, ordered or not, in the open
 *        transaction's session.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the collection.
 * @param[in]     _documents   the documents.
 * @param[in]     _count       how many there are.
 * @param[in]     _flags       D_MONGODB_UNORDERED, or 0.
 * @param[out]    _inserted    receives the reply's count, on a failure too;
 *                             `NULL` where it is not wanted.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_insert_all(
    struct dj_mongodb*   _db,
    mongoc_collection_t* _collection,
    const bson_t**       _documents,
    size_t               _count,
    unsigned             _flags,
    int64_t*             _inserted
)
{
    bson_t       opts;
    bson_t       reply;
    bson_error_t error;

    bson_init(&opts);
    bson_init(&reply);
    BSON_APPEND_BOOL(&opts,
                     "ordered",
                     ((_flags & D_MONGODB_UNORDERED) == 0u));

    int result = d_internal_mongodb_session(_db, &opts);

    // the insert, its count on failure and success alike
    if (result == 0)
    {
        struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
        const bool                    ok     =
            mongoc_collection_insert_many(_collection,
                                          _documents,
                                          _count,
                                          &opts,
                                          &reply,
                                          &error);

        d_internal_mongodb_counts(&reply, &counts);
        result = d_internal_mongodb_written(_db, ok, &error, &reply, NULL);

        if (_inserted)
        {
            *_inserted = counts.inserted;
        }
    }

    bson_destroy(&reply);
    bson_destroy(&opts);

    return result;
}

/*
d_mongodb_insert_many
  The driver gives each document without an _id its ObjectId, and batches
the documents as the server's limits require. The count is the reply's,
which a failure carries too.
*/
int d_mongodb_insert_many(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _documents,
    unsigned           _flags,
    int64_t*           _inserted
)
{
    // none went in, until some do
    if (_inserted)
    {
        *_inserted = 0;
    }

    // a handle, and flags it knows
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_check_flags(_db, _flags);

    mongoc_collection_t* const collection =
        (result == 0)
            ? d_internal_mongodb_collection(_db, _database, _collection)
            : NULL;

    // a collection to insert into
    if (!collection)
    {
        return _db->error;
    }

    bson_t         array;
    bson_t*        documents = NULL;
    const bson_t** pointers  = NULL;
    size_t         count     = 0u;

    result = d_internal_mongodb_read_array(_db, _documents, true, &array);
    result = (result == 0) ? d_internal_mongodb_documents_of(_db,
                                                             &array,
                                                             &documents,
                                                             &pointers,
                                                             &count)
                           : result;
    result = (result == 0) ? d_internal_mongodb_insert_all(_db,
                                                           collection,
                                                           pointers,
                                                           count,
                                                           _flags,
                                                           _inserted)
                           : result;

    free(pointers);
    free(documents);
    bson_destroy(&array);
    mongoc_collection_destroy(collection);

    return result;
}

// 2.2    Finding
//------------------------------------------------------------------------------
/*
d_mongodb_find
  Every batch, read to the cursor's end.
*/
int d_mongodb_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    mongoc_cursor_t* cursor = NULL;

    // a handle, and somewhere for the rows
    if ( (!_db) ||
         (!_rows) )
    {
        return (_db) ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no place was given for the "
                                                 "rows")
                     : D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mongodb_open_find(_db,
                                                    _database,
                                                    _collection,
                                                    _filter,
                                                    _options,
                                                    false,
                                                    &cursor);

    return (result == 0) ? d_internal_mongodb_drain(_db, cursor, _rows)
                         : result;
}

/*
d_mongodb_find_one
  A limit of 1 in a single batch, which closes the server's cursor with it.
*/
int d_mongodb_find_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    mongoc_cursor_t* cursor = NULL;
    struct dj_db_rows rows   = d_internal_mongodb_empty_rows;

    // a handle, and somewhere for the row
    if ( (!_db) ||
         (!_rows) )
    {
        return (_db) ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no place was given for the "
                                                 "row")
                     : D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_open_find(_db,
                                              _database,
                                              _collection,
                                              _filter,
                                              _options,
                                              true,
                                              &cursor);

    result = (result == 0) ? d_internal_mongodb_drain(_db, cursor, &rows)
                           : result;

    // none matched
    if ( (result == 0) &&
         (rows.row_count == 0u) )
    {
        d_db_rows_free(&rows);

        return d_internal_mongodb_answer(_db,
                                         D_DB_NOT_FOUND,
                                         "no document matched the filter");
    }

    // the one row, on success
    if (result == 0)
    {
        *_rows = rows;
    }

    return result;
}

/**
 * @brief Adds a document to keyed rows: its _id as a filter that matches it
 *        alone, or NULL where it has none, and then the document, both
 *        canonical.
 *
 * @param[in,out] _builder   the rows, their two columns made.
 * @param[in]     _document  the document.
 * @return `true`, or `false` where memory ran out.
 */
static
bool d_internal_mongodb_rows_keyed(
    struct dj_internal_db_rows* _builder,
    const bson_t*               _document
)
{
    bson_iter_t iter;
    bool        added = false;

    // the _id alone, as a filter; NULL for a document without one
    if (bson_iter_init_find(&iter, _document, "_id"))
    {
        bson_t key;

        bson_init(&key);
        added = ( (bson_append_iter(&key, "_id", 3, &iter)) &&
                  (d_internal_mongodb_rows_document(
                       _builder,
                       &key,
                       D_MONGODB_JSON_CANONICAL)) );
        bson_destroy(&key);
    }
    else
    {
        added = d_internal_db_rows_null(_builder);
    }

    return ( (added) &&
             (d_internal_mongodb_rows_document(_builder,
                                               _document,
                                               D_MONGODB_JSON_CANONICAL)) );
}

/**
 * @brief Reads a cursor's every document into keyed rows, written only on
 *        success.
 *
 * @param[in,out] _db      the handle, on which a failure is recorded.
 * @param[in]     _cursor  the cursor, which the caller destroys.
 * @param[out]    _rows    receives the rows.
 * @return 0; the cursor's failure, recorded; D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_keyed_rows(
    struct dj_mongodb* _db,
    mongoc_cursor_t*   _cursor,
    struct dj_db_rows* _rows
)
{
    struct dj_internal_db_rows builder  = { { NULL, 0u, 0u },
                                           { NULL, 0u, 0u },
                                           { NULL, 0u, 0u },
                                           0u,
                                           0u };
    const bson_t*             document = NULL;
    const bson_t*             reply    = NULL;
    bson_error_t              error;
    bool                      ok       =
        ( (d_internal_db_rows_column(&builder,
                                     "_id",
                                     D_DB_TYPE_JSON,
                                     "document")) &&
          (d_internal_mongodb_rows_column(&builder, false)) );

    // every document, a row
    while ( (ok) &&
            (mongoc_cursor_next(_cursor, &document)) )
    {
        ok = d_internal_mongodb_rows_keyed(&builder, document);
    }

    // the cursor's failure, which shows once it stops
    if ( (ok) &&
         (mongoc_cursor_error_document(_cursor, &error, &reply)) )
    {
        d_internal_db_rows_release(&builder);

        return d_internal_mongodb_fail(_db, &error, reply);
    }

    // the rows, packed
    if ( (!ok) ||
         (!d_internal_db_rows_finish(&builder, _rows)) )
    {
        d_internal_db_rows_release(&builder);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the documents read");
    }

    return 0;
}

/*
d_mongodb_find_keyed
  A find as d_mongodb_find's, each document written twice: its _id, then
itself.
*/
int d_mongodb_find_keyed(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    mongoc_cursor_t* cursor = NULL;

    // a handle, and somewhere for the rows
    if ( (!_db) ||
         (!_rows) )
    {
        return (_db) ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no place was given for the "
                                                 "rows")
                     : D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_open_find(_db,
                                              _database,
                                              _collection,
                                              _filter,
                                              _options,
                                              false,
                                              &cursor);

    // the documents, the cursor then done with
    if (result == 0)
    {
        result = d_internal_mongodb_keyed_rows(_db, cursor, _rows);
        mongoc_cursor_destroy(cursor);
    }

    return result;
}

// 2.3    Updating and replacing
//------------------------------------------------------------------------------
/**
 * @brief Sends one of the three updates the driver makes.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the collection.
 * @param[in]     _filter      the filter.
 * @param[in]     _update      the update, a pipeline, or the replacement.
 * @param[in]     _opts        the driver's options.
 * @param[out]    _counts      receives the counts on success, or `NULL`.
 * @param[in]     _kind        which update.
 * @return 0, or the failure, recorded.
 */
static int
d_internal_mongodb_send_update(
    struct dj_mongodb*                   _db,
    mongoc_collection_t*                _collection,
    const bson_t*                       _filter,
    const bson_t*                       _update,
    const bson_t*                       _opts,
    struct dj_mongodb_write_counts*      _counts,
    enum dj_internal_mongodb_update_kind _kind
)
{
    bson_t       reply;
    bson_error_t error;

    bson_init(&reply);

    // the update the kind makes
    const bool ok =
        (_kind == D_INTERNAL_MONGODB_UPDATE_ONE)
            ? mongoc_collection_update_one(_collection, _filter, _update,
                                           _opts, &reply, &error)
            : (_kind == D_INTERNAL_MONGODB_UPDATE_MANY)
                  ? mongoc_collection_update_many(_collection, _filter,
                                                  _update, _opts, &reply,
                                                  &error)
                  : mongoc_collection_replace_one(_collection, _filter,
                                                  _update, _opts, &reply,
                                                  &error);
    const int result = d_internal_mongodb_written(_db,
                                                  ok,
                                                  &error,
                                                  &reply,
                                                  _counts);

    bson_destroy(&reply);

    return result;
}

/**
 * @brief Makes one of the three updates.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database, or `NULL`.
 * @param[in]     _collection  the collection.
 * @param[in]     _filter      the filter, or `NULL`.
 * @param[in]     _update      the update, a pipeline, or the replacement.
 * @param[in]     _options     the options, or `NULL`.
 * @param[out]    _counts      receives the counts on success, or `NULL`.
 * @param[in]     _kind        which update.
 * @return as the header describes.
 *
 * An update may be a document of operators or a pipeline, which the driver
 * tells apart by its keys; a replacement must be a document, which the
 * driver checks has no operators.
 */
static int
d_internal_mongodb_update(
    struct dj_mongodb*                   _db,
    const char*                         _database,
    const char*                         _collection,
    const char*                         _filter,
    const char*                         _update,
    const char*                         _options,
    struct dj_mongodb_write_counts*      _counts,
    enum dj_internal_mongodb_update_kind _kind
)
{
    // a handle to write through
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    // a collection to write to
    if (!collection)
    {
        return _db->error;
    }

    bson_t filter;
    bson_t update;
    bson_t opts;
    bool   pipeline = false;
    int    result   = d_internal_mongodb_read_document(_db,
                                                       _filter,
                                                       false,
                                                       &filter);

    bson_init(&update);
    bson_init(&opts);
    result = (result != 0)
                 ? result
                 : (_kind == D_INTERNAL_MONGODB_REPLACE_ONE)
                       ? d_internal_mongodb_read_document(_db,
                                                          _update,
                                                          true,
                                                          &update)
                       : d_internal_mongodb_read_either(_db,
                                                        _update,
                                                        &update,
                                                        &pipeline);
    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              _options,
                                              D_INTERNAL_MONGODB_SESSION,
                                              &opts)
                 : result;

    result = (result == 0) ? d_internal_mongodb_send_update(_db,
                                                            collection,
                                                            &filter,
                                                            &update,
                                                            &opts,
                                                            _counts,
                                                            _kind)
                           : result;

    bson_destroy(&opts);
    bson_destroy(&update);
    bson_destroy(&filter);
    mongoc_collection_destroy(collection);

    return result;
}

/*
d_mongodb_update_one
  The first document matched.
*/
int d_mongodb_update_one(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _update,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    return d_internal_mongodb_update(_db,
                                     _database,
                                     _collection,
                                     _filter,
                                     _update,
                                     _options,
                                     _counts,
                                     D_INTERNAL_MONGODB_UPDATE_ONE);
}

/*
d_mongodb_update_many
  Every document matched.
*/
int d_mongodb_update_many(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _update,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    return d_internal_mongodb_update(_db,
                                     _database,
                                     _collection,
                                     _filter,
                                     _update,
                                     _options,
                                     _counts,
                                     D_INTERNAL_MONGODB_UPDATE_MANY);
}

/*
d_mongodb_replace_one
  The first document matched, replaced whole.
*/
int d_mongodb_replace_one(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _replacement,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    return d_internal_mongodb_update(_db,
                                     _database,
                                     _collection,
                                     _filter,
                                     _replacement,
                                     _options,
                                     _counts,
                                     D_INTERNAL_MONGODB_REPLACE_ONE);
}

// 2.4    Deleting
//------------------------------------------------------------------------------
/**
 * @brief Sends the driver's delete of the first document matched, or of
 *        every one.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the collection.
 * @param[in]     _filter      the filter.
 * @param[in]     _opts        the driver's options.
 * @param[out]    _deleted     receives the count on success, or `NULL`.
 * @param[in]     _many        whether every one.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_send_delete(
    struct dj_mongodb*   _db,
    mongoc_collection_t* _collection,
    const bson_t*        _filter,
    const bson_t*        _opts,
    int64_t*             _deleted,
    bool                 _many
)
{
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
    bson_t                        reply;
    bson_error_t                  error;

    bson_init(&reply);

    // the delete, then its count
    const bool ok     = (_many) ? mongoc_collection_delete_many(_collection,
                                                                _filter,
                                                                _opts,
                                                                &reply,
                                                                &error)
                                : mongoc_collection_delete_one(_collection,
                                                               _filter,
                                                               _opts,
                                                               &reply,
                                                               &error);
    const int  result = d_internal_mongodb_written(_db,
                                                   ok,
                                                   &error,
                                                   &reply,
                                                   &counts);

    // the count, on success
    if ( (result == 0) &&
         (_deleted) )
    {
        *_deleted = counts.deleted;
    }

    bson_destroy(&reply);

    return result;
}

/**
 * @brief Deletes the first document matched, or every one.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database, or `NULL`.
 * @param[in]     _collection  the collection.
 * @param[in]     _filter      the filter, or `NULL`.
 * @param[in]     _options     the options, or `NULL`.
 * @param[out]    _deleted     receives the count on success, or `NULL`.
 * @param[in]     _many        whether every one.
 * @return as the header describes.
 */
static
int d_internal_mongodb_delete(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _deleted,
    bool               _many
)
{
    // a handle to delete through
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    // a collection to delete from
    if (!collection)
    {
        return _db->error;
    }

    bson_t filter;
    bson_t opts;
    int    result = d_internal_mongodb_read_document(_db,
                                                     _filter,
                                                     false,
                                                     &filter);

    bson_init(&opts);
    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              _options,
                                              D_INTERNAL_MONGODB_SESSION,
                                              &opts)
                 : result;
    result = (result == 0) ? d_internal_mongodb_send_delete(_db,
                                                            collection,
                                                            &filter,
                                                            &opts,
                                                            _deleted,
                                                            _many)
                           : result;

    bson_destroy(&opts);
    bson_destroy(&filter);
    mongoc_collection_destroy(collection);

    return result;
}

/*
d_mongodb_delete_one
  The first document matched.
*/
int d_mongodb_delete_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _deleted
)
{
    return d_internal_mongodb_delete(_db,
                                     _database,
                                     _collection,
                                     _filter,
                                     _options,
                                     _deleted,
                                     false);
}

/*
d_mongodb_delete_many
  Every document matched.
*/
int d_mongodb_delete_many(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _deleted
)
{
    return d_internal_mongodb_delete(_db,
                                     _database,
                                     _collection,
                                     _filter,
                                     _options,
                                     _deleted,
                                     true);
}

// 2.5    Counting
//------------------------------------------------------------------------------
/*
d_mongodb_count_documents
  The driver's count_documents, an aggregation of $match and $group, which
counts exactly and may run in a transaction.
*/
int d_mongodb_count_documents(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _count
)
{
    // a handle to count through
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    // a collection to count
    if (!collection)
    {
        return _db->error;
    }

    bson_t       filter;
    bson_t       opts;
    bson_t       reply;
    bson_error_t error;
    int          result = d_internal_mongodb_read_document(_db,
                                                           _filter,
                                                           false,
                                                           &filter);

    bson_init(&opts);
    bson_init(&reply);
    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              _options,
                                              D_INTERNAL_MONGODB_READ |
                                              D_INTERNAL_MONGODB_SESSION,
                                              &opts)
                 : result;

    // the count, or the failure
    if (result == 0)
    {
        const int64_t count = mongoc_collection_count_documents(collection,
                                                                &filter,
                                                                &opts,
                                                                NULL,
                                                                &reply,
                                                                &error);

        result = (count < 0) ? d_internal_mongodb_fail(_db, &error, &reply)
                             : 0;

        if ( (result == 0) &&
             (_count) )
        {
            *_count = count;
        }
    }

    bson_destroy(&reply);
    bson_destroy(&opts);
    bson_destroy(&filter);
    mongoc_collection_destroy(collection);

    return result;
}

/*
d_mongodb_estimated_document_count
  The count command, from the collection's metadata, which no transaction
may hold: refused inside one before it is sent, so that the transaction
lives.
*/
int d_mongodb_estimated_document_count(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _options,
    int64_t*           _count
)
{
    // a handle to count through, outside a transaction
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if (d_mongodb_in_transaction(_db))
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_UNSUPPORTED,
                                         "an estimated count cannot run in "
                                         "a transaction; count_documents "
                                         "can");
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    // a collection to count
    if (!collection)
    {
        return _db->error;
    }

    bson_t       opts;
    bson_t       reply;
    bson_error_t error;

    bson_init(&opts);
    bson_init(&reply);

    int           result = d_internal_mongodb_options(_db,
                                                      _options,
                                                      D_INTERNAL_MONGODB_READ,
                                                      &opts);
    const int64_t count  = (result == 0)
                               ? mongoc_collection_estimated_document_count(
                                     collection,
                                     &opts,
                                     NULL,
                                     &reply,
                                     &error)
                               : 0;

    result = ( (result == 0) &&
               (count < 0) ) ? d_internal_mongodb_fail(_db, &error, &reply)
                             : result;

    // the count, on success
    if ( (result == 0) &&
         (_count) )
    {
        *_count = count;
    }

    bson_destroy(&reply);
    bson_destroy(&opts);
    mongoc_collection_destroy(collection);

    return result;
}

/**
 * @brief Makes rows of a reply's array of values.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _reply  the reply.
 * @param[in]     _field  its array's field.
 * @param[out]    _rows   receives the rows.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_value_rows(
    struct dj_mongodb* _db,
    const bson_t*      _reply,
    const char*        _field,
    struct dj_db_rows* _rows
)
{
    struct dj_internal_db_rows builder = { { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          0u,
                                          0u };
    bson_iter_t               field;
    bson_iter_t               value;
    bool                      ok      = d_internal_mongodb_rows_column(
                                            &builder,
                                            true);

    // each value of the array, as a row
    if ( (ok)                                           &&
         (bson_iter_init_find(&field, _reply, _field)) &&
         (BSON_ITER_HOLDS_ARRAY(&field))                &&
         (bson_iter_recurse(&field, &value)) )
    {
        while ( (ok) &&
                (bson_iter_next(&value)) )
        {
            ok = d_internal_mongodb_rows_value(&builder,
                                               bson_iter_value(&value),
                                               _db->format);
        }
    }

    // the rows, packed
    if ( (!ok) ||
         (!d_internal_db_rows_finish(&builder, _rows)) )
    {
        d_internal_db_rows_release(&builder);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the values");
    }

    return 0;
}

/**
 * @brief Runs the distinct command, and makes rows of its values.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the driver's collection.
 * @param[in]     _name        the collection's name.
 * @param[in]     _field       the field.
 * @param[in]     _filter      the filter.
 * @param[in]     _opts        the driver's options.
 * @param[out]    _rows        receives the values, on success.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_run_distinct(
    struct dj_mongodb*   _db,
    mongoc_collection_t* _collection,
    const char*          _name,
    const char*          _field,
    const bson_t*        _filter,
    const bson_t*        _opts,
    struct dj_db_rows*   _rows
)
{
    bson_t       command;
    bson_t       reply;
    bson_error_t error;

    bson_init(&command);
    bson_init(&reply);
    BSON_APPEND_UTF8(&command, "distinct", _name);
    BSON_APPEND_UTF8(&command, "key", _field);
    BSON_APPEND_DOCUMENT(&command, "query", _filter);

    const int result = (mongoc_collection_read_command_with_opts(_collection,
                                                                 &command,
                                                                 NULL,
                                                                 _opts,
                                                                 &reply,
                                                                 &error))
                           ? d_internal_mongodb_value_rows(_db,
                                                           &reply,
                                                           "values",
                                                           _rows)
                           : d_internal_mongodb_fail(_db, &error, &reply);

    bson_destroy(&reply);
    bson_destroy(&command);

    return result;
}

/*
d_mongodb_distinct
  The distinct command, through read_command_with_opts, which applies the
collection's read concern and read preference, and appends the options.
*/
int d_mongodb_distinct(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _field,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    // a handle, a field, and somewhere for the rows
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    if ( (!_field) ||
         (!_rows) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         (_field) ? "no place was given for "
                                                    "the rows"
                                                  : "no field was named");
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    // a collection to read
    if (!collection)
    {
        return _db->error;
    }

    bson_t filter;
    bson_t opts;
    int    result = d_internal_mongodb_read_document(_db,
                                                     _filter,
                                                     false,
                                                     &filter);

    bson_init(&opts);
    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              _options,
                                              D_INTERNAL_MONGODB_READ |
                                              D_INTERNAL_MONGODB_SESSION,
                                              &opts)
                 : result;
    result = (result == 0) ? d_internal_mongodb_run_distinct(_db,
                                                             collection,
                                                             _collection,
                                                             _field,
                                                             &filter,
                                                             &opts,
                                                             _rows)
                           : result;

    bson_destroy(&opts);
    bson_destroy(&filter);
    mongoc_collection_destroy(collection);

    return result;
}

// 2.6    Aggregating
//------------------------------------------------------------------------------
/*
d_mongodb_aggregate
  A collection's aggregate, or the database's, {aggregate: 1}, where no
collection is named.
*/
int d_mongodb_aggregate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _pipeline,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    // a handle, and somewhere for the rows
    if ( (!_db) ||
         (!_rows) )
    {
        return (_db) ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no place was given for the "
                                                 "rows")
                     : D_DB_INVALID_ARGUMENT;
    }

    mongoc_cursor_t* cursor = NULL;
    const int        result = d_internal_mongodb_open_aggregate(_db,
                                                                _database,
                                                                _collection,
                                                                _pipeline,
                                                                _options,
                                                                &cursor);

    return (result == 0) ? d_internal_mongodb_drain(_db, cursor, _rows)
                         : result;
}

// 2.7    Finding and modifying
//------------------------------------------------------------------------------
/**
 * @brief Reads a find-and-modify's option that is one of the driver's
 *        flags: upsert, a boolean, or returnDocument, "before" or "after".
 *
 * @param[in]     _field  the option.
 * @param[in]     _kind   what the call does; a delete takes neither flag.
 * @param[in,out] _flags  the driver's flags.
 * @return `true`, or `false` for a value the option cannot take.
 */
static bool
d_internal_mongodb_modify_flag(
    const bson_iter_t*                  _field,
    enum dj_internal_mongodb_modify_kind _kind,
    unsigned*                           _flags
)
{
    const bool upsert = (strcmp(bson_iter_key(_field), "upsert") == 0);

    // upsert: a boolean, true setting the flag
    if (upsert)
    {
        if (!BSON_ITER_HOLDS_BOOL(_field))
        {
            return false;
        }

        if (bson_iter_bool(_field))
        {
            *_flags |= (unsigned)MONGOC_FIND_AND_MODIFY_UPSERT;

            return (_kind != D_INTERNAL_MONGODB_MODIFY_DELETE);
        }

        return true;
    }

    const char* const which = (BSON_ITER_HOLDS_UTF8(_field))
                                  ? bson_iter_utf8(_field, NULL)
                                  : "";

    // returnDocument: "before", or "after", setting the flag
    if (strcmp(which, "after") == 0)
    {
        *_flags |= (unsigned)MONGOC_FIND_AND_MODIFY_RETURN_NEW;

        return (_kind != D_INTERNAL_MONGODB_MODIFY_DELETE);
    }

    return (strcmp(which, "before") == 0);
}

/**
 * @brief Applies one of a find-and-modify's options to the driver's
 *        options, or keeps it to send as it is.
 *
 * @param[in]     _field   the option.
 * @param[in]     _kind    what the call does.
 * @param[in,out] _opts    the driver's options.
 * @param[in,out] _extra   what is sent as it is.
 * @param[in,out] _flags   the driver's flags.
 * @param[in,out] _timed   set when the option is maxTimeMS.
 * @return `true`, or `false` for a value the option cannot take.
 *
 * sort, projection, upsert, returnDocument, bypassDocumentValidation and
 * maxTimeMS have calls of the driver's; the rest -- collation, hint,
 * arrayFilters, let, comment, writeConcern -- go into the command as they
 * are, where the driver reads writeConcern for itself.
 */
static bool
d_internal_mongodb_modify_option(
    const bson_iter_t*                  _field,
    enum dj_internal_mongodb_modify_kind _kind,
    mongoc_find_and_modify_opts_t*      _opts,
    bson_t*                             _extra,
    unsigned*                           _flags,
    bool*                               _timed
)
{
    const char* const key = bson_iter_key(_field);
    bson_t            document;

    // sort and projection: documents, through the driver's calls
    if (strcmp(key, "sort") == 0)
    {
        return ( (d_internal_mongodb_field_document(_field, &document)) &&
                 (mongoc_find_and_modify_opts_set_sort(_opts, &document)) );
    }

    if (strcmp(key, "projection") == 0)
    {
        return ( (d_internal_mongodb_field_document(_field, &document)) &&
                 (mongoc_find_and_modify_opts_set_fields(_opts, &document)) );
    }

    // upsert and returnDocument: the driver's flags
    if ( (strcmp(key, "upsert") == 0) ||
         (strcmp(key, "returnDocument") == 0) )
    {
        return d_internal_mongodb_modify_flag(_field, _kind, _flags);
    }

    if (strcmp(key, "bypassDocumentValidation") == 0)
    {
        return ( (BSON_ITER_HOLDS_BOOL(_field)) &&
                 (mongoc_find_and_modify_opts_set_bypass_document_validation(
                      _opts,
                      bson_iter_bool(_field))) );
    }

    // maxTimeMS: the driver's 32 bits
    if (strcmp(key, "maxTimeMS") == 0)
    {
        const int64_t value = (BSON_ITER_HOLDS_NUMBER(_field))
                                  ? bson_iter_as_int64(_field)
                                  : -1;

        *_timed = true;

        return ( (value >= 0)                  &&
                 (value <= (int64_t)UINT32_MAX) &&
                 (mongoc_find_and_modify_opts_set_max_time_ms(
                      _opts,
                      (uint32_t)value)) );
    }

    return bson_append_iter(_extra, key, -1, _field);
}

/**
 * @brief Applies each of a find-and-modify's options, in their order.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _options  the options.
 * @param[in]     _kind     what the call does.
 * @param[in,out] _opts     the driver's options.
 * @param[in,out] _extra    what is sent as it is.
 * @param[in,out] _flags    the driver's flags.
 * @param[in,out] _timed    set when an option is maxTimeMS.
 * @return 0, or D_DB_INVALID_ARGUMENT for the first option given a value
 *         it cannot take.
 */
static int
d_internal_mongodb_modify_each(
    struct dj_mongodb*                   _db,
    const bson_t*                       _options,
    enum dj_internal_mongodb_modify_kind _kind,
    mongoc_find_and_modify_opts_t*      _opts,
    bson_t*                             _extra,
    unsigned*                           _flags,
    bool*                               _timed
)
{
    bson_iter_t field;

    // each option, to its place
    if (bson_iter_init(&field, _options))
    {
        while (bson_iter_next(&field))
        {
            if (!d_internal_mongodb_modify_option(&field,
                                                  _kind,
                                                  _opts,
                                                  _extra,
                                                  _flags,
                                                  _timed))
            {
                return d_internal_mongodb_answer(
                           _db,
                           D_DB_INVALID_ARGUMENT,
                           "a find-and-modify cannot take its option "
                           "\"%s\" as given",
                           bson_iter_key(&field));
            }
        }
    }

    return 0;
}

/**
 * @brief Applies the handle's time limit to a find-and-modify, capped at
 *        the driver's 32 bits, where it has one.
 *
 * @param[in]     _db    the handle.
 * @param[in,out] _opts  the driver's options.
 */
static
void d_internal_mongodb_modify_time(
    const struct dj_mongodb*       _db,
    mongoc_find_and_modify_opts_t* _opts
)
{
    // a limit to apply
    if (_db->max_time_ms > 0)
    {
        (void)mongoc_find_and_modify_opts_set_max_time_ms(
                  _opts,
                  (uint32_t)( (_db->max_time_ms > (int64_t)UINT32_MAX)
                                  ? (int64_t)UINT32_MAX
                                  : _db->max_time_ms ));
    }

    return;
}

/**
 * @brief Reads a find-and-modify's options into the driver's.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _json     the options, as JSON, or `NULL`.
 * @param[in]     _kind     what the call does.
 * @param[in,out] _opts     the driver's options.
 * @return 0; D_DB_INVALID_ARGUMENT for an option given a value it cannot
 *         take; the text's failure; the session's.
 *
 * The handle's time limit applies where the options name none.
 */
static int
d_internal_mongodb_modify_options(
    struct dj_mongodb*                   _db,
    const char*                         _json,
    enum dj_internal_mongodb_modify_kind _kind,
    mongoc_find_and_modify_opts_t*      _opts
)
{
    bson_t   options;
    bson_t   extra;
    unsigned flags  = (_kind == D_INTERNAL_MONGODB_MODIFY_DELETE)
                          ? (unsigned)MONGOC_FIND_AND_MODIFY_REMOVE
                          : 0u;
    bool     timed  = false;
    int      result = d_internal_mongodb_read_document(_db,
                                                       _json,
                                                       false,
                                                       &options);

    bson_init(&extra);
    result = (result == 0) ? d_internal_mongodb_modify_each(_db,
                                                            &options,
                                                            _kind,
                                                            _opts,
                                                            &extra,
                                                            &flags,
                                                            &timed)
                           : result;

    // the handle's time limit, where none was given
    if ( (result == 0) &&
         (!timed) )
    {
        d_internal_mongodb_modify_time(_db, _opts);
    }

    result = (result == 0) ? d_internal_mongodb_session(_db, &extra) : result;

    // the flags, and the rest as it is
    if ( (result == 0) &&
         ( (!mongoc_find_and_modify_opts_set_flags(
                _opts,
                (mongoc_find_and_modify_flags_t)flags)) ||
           (!mongoc_find_and_modify_opts_append(_opts, &extra)) ) )
    {
        result = d_internal_mongodb_answer(_db,
                                           D_MONGODB_LAYER_NO_MEMORY,
                                           "no memory for the options");
    }

    bson_destroy(&extra);
    bson_destroy(&options);

    return result;
}

/**
 * @brief Reads a find-and-modify's update or replacement, and checks it is
 *        the one it must be: an update's operators, a replacement's none.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _json    the text.
 * @param[in]     _kind    update or replace.
 * @param[out]    _update  initialized, and receives it.
 * @return 0; D_DB_INVALID_ARGUMENT for the wrong one; the text's failure.
 */
static int
d_internal_mongodb_modify_update(
    struct dj_mongodb*                   _db,
    const char*                         _json,
    enum dj_internal_mongodb_modify_kind _kind,
    bson_t*                             _update
)
{
    bool pipeline = false;
    int  result   = (_kind == D_INTERNAL_MONGODB_MODIFY_REPLACE)
                        ? d_internal_mongodb_read_document(_db,
                                                           _json,
                                                           true,
                                                           _update)
                        : d_internal_mongodb_read_either(_db,
                                                         _json,
                                                         _update,
                                                         &pipeline);

    // a document of operators updates; one of none replaces
    if ( (result == 0) &&
         (!pipeline)   &&
         (d_internal_mongodb_has_operators(_update) !=
          (_kind == D_INTERNAL_MONGODB_MODIFY_UPDATE)) )
    {
        result = d_internal_mongodb_answer(
                     _db,
                     D_DB_INVALID_ARGUMENT,
                     (_kind == D_INTERNAL_MONGODB_MODIFY_UPDATE)
                         ? "an update needs its operators, $set and its kin, "
                           "or a pipeline"
                         : "a replacement takes no update operators");
    }

    return result;
}

/**
 * @brief Records what a find-and-modify wrote, and makes its document the
 *        one row.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _kind   what it did.
 * @param[in]     _reply  its reply.
 * @param[out]    _rows   receives the row.
 * @return 0; D_DB_NOT_FOUND for no document, the write made all the same;
 *         D_MONGODB_LAYER_NO_MEMORY.
 *
 * The reply's lastErrorObject counts the one document: matched, or
 * upserted, or deleted. Whether a match was changed it does not say, so
 * the handle counts it written.
 */
static int
d_internal_mongodb_modified(
    struct dj_mongodb*                   _db,
    enum dj_internal_mongodb_modify_kind _kind,
    const bson_t*                       _reply,
    struct dj_db_rows*                   _rows
)
{
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
    bson_iter_t                   top;
    bson_iter_t                   field;
    bson_value_t                  id;
    bson_t                        document;

    const bool    upserted = ( (bson_iter_init(&top, _reply)) &&
                               (bson_iter_find_descendant(
                                    &top,
                                    "lastErrorObject.upserted",
                                    &field)) );
    const int64_t n        = ( (bson_iter_init(&top, _reply)) &&
                               (bson_iter_find_descendant(&top,
                                                          "lastErrorObject.n",
                                                          &field)) &&
                               (BSON_ITER_HOLDS_NUMBER(&field)) )
                                 ? bson_iter_as_int64(&field)
                                 : 0;

    counts.deleted  = (_kind == D_INTERNAL_MONGODB_MODIFY_DELETE) ? n : 0;
    counts.upserted = (upserted) ? n : 0;
    counts.matched  = ( (!upserted) &&
                        (_kind != D_INTERNAL_MONGODB_MODIFY_DELETE) ) ? n : 0;
    counts.modified = counts.matched;
    d_internal_mongodb_wrote(_db, &counts);

    // an upsert's new document
    if ( (upserted) &&
         (bson_iter_init(&top, _reply)) &&
         (bson_iter_find_descendant(&top, "lastErrorObject.upserted", &field)) )
    {
        id = *bson_iter_value(&field);
        d_internal_mongodb_new_id(_db, &id);
    }

    // the document, before or after
    if ( (!bson_iter_init_find(&field, _reply, "value")) ||
         (!d_internal_mongodb_field_document(&field, &document)) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_NOT_FOUND,
                                         "no document to hand back: none "
                                         "matched, or an upsert's was asked "
                                         "for as it was before");
    }

    return d_internal_mongodb_one_row(_db, &document, _rows);
}

/**
 * @brief Sends a find-and-modify its options hold, with its update or
 *        replacement, and reads what it wrote.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the collection.
 * @param[in]     _filter      the filter.
 * @param[in]     _update      the update or the replacement; unread for a
 *                             delete.
 * @param[in,out] _opts        the driver's options, read.
 * @param[in]     _kind        what it does.
 * @param[out]    _rows        receives the document.
 * @return 0, or the failure, recorded.
 */
static int
d_internal_mongodb_send_modify(
    struct dj_mongodb*                   _db,
    mongoc_collection_t*                _collection,
    const bson_t*                       _filter,
    const bson_t*                       _update,
    mongoc_find_and_modify_opts_t*      _opts,
    enum dj_internal_mongodb_modify_kind _kind,
    struct dj_db_rows*                   _rows
)
{
    bson_t       reply;
    bson_error_t error;

    bson_init(&reply);

    // the update, where there is one
    if (_kind != D_INTERNAL_MONGODB_MODIFY_DELETE)
    {
        (void)mongoc_find_and_modify_opts_set_update(_opts, _update);
    }

    const int result = (mongoc_collection_find_and_modify_with_opts(
                            _collection,
                            _filter,
                            _opts,
                            &reply,
                            &error))
                           ? d_internal_mongodb_modified(_db,
                                                         _kind,
                                                         &reply,
                                                         _rows)
                           : d_internal_mongodb_fail(_db, &error, &reply);

    bson_destroy(&reply);

    return result;
}

/**
 * @brief Makes one of the three find-and-modifies.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database, or `NULL`.
 * @param[in]     _collection  the collection.
 * @param[in]     _filter      the filter, or `NULL`.
 * @param[in]     _update      the update or replacement; `NULL` to delete.
 * @param[in]     _options     the options, or `NULL`.
 * @param[out]    _rows        receives the document.
 * @param[in]     _kind        what it does.
 * @return as the header describes.
 */
static int
d_internal_mongodb_find_and_modify(
    struct dj_mongodb*                   _db,
    const char*                         _database,
    const char*                         _collection,
    const char*                         _filter,
    const char*                         _update,
    const char*                         _options,
    struct dj_db_rows*                   _rows,
    enum dj_internal_mongodb_modify_kind _kind
)
{
    // a handle, and somewhere for the row
    if ( (!_db) ||
         (!_rows) )
    {
        return (_db) ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no place was given for the "
                                                 "row")
                     : D_DB_INVALID_ARGUMENT;
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    // a collection to write to
    if (!collection)
    {
        return _db->error;
    }

    mongoc_find_and_modify_opts_t* const opts =
        mongoc_find_and_modify_opts_new();
    bson_t filter;
    bson_t update;
    int    result = d_internal_mongodb_read_document(_db,
                                                     _filter,
                                                     false,
                                                     &filter);

    bson_init(&update);
    result = ( (result == 0) &&
               (_kind != D_INTERNAL_MONGODB_MODIFY_DELETE) )
                 ? d_internal_mongodb_modify_update(_db,
                                                    _update,
                                                    _kind,
                                                    &update)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_modify_options(_db,
                                                     _options,
                                                     _kind,
                                                     opts)
                 : result;
    result = (result == 0) ? d_internal_mongodb_send_modify(_db,
                                                            collection,
                                                            &filter,
                                                            &update,
                                                            opts,
                                                            _kind,
                                                            _rows)
                           : result;

    mongoc_find_and_modify_opts_destroy(opts);
    bson_destroy(&update);
    bson_destroy(&filter);
    mongoc_collection_destroy(collection);

    return result;
}

/*
d_mongodb_find_one_and_update
  An update of operators, or a pipeline.
*/
int d_mongodb_find_one_and_update(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _update,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    return d_internal_mongodb_find_and_modify(_db,
                                              _database,
                                              _collection,
                                              _filter,
                                              _update,
                                              _options,
                                              _rows,
                                              D_INTERNAL_MONGODB_MODIFY_UPDATE);
}

/*
d_mongodb_find_one_and_replace
  A replacement, of no operators.
*/
int d_mongodb_find_one_and_replace(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _replacement,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    return d_internal_mongodb_find_and_modify(
               _db,
               _database,
               _collection,
               _filter,
               _replacement,
               _options,
               _rows,
               D_INTERNAL_MONGODB_MODIFY_REPLACE);
}

/*
d_mongodb_find_one_and_delete
  findAndModify's remove.
*/
int d_mongodb_find_one_and_delete(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    return d_internal_mongodb_find_and_modify(_db,
                                              _database,
                                              _collection,
                                              _filter,
                                              NULL,
                                              _options,
                                              _rows,
                                              D_INTERNAL_MONGODB_MODIFY_DELETE);
}

// 2.8    Many writes at once
//------------------------------------------------------------------------------
/**
 * @brief Reads one write of many: its model's name, and its fields.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _write  the element, {model: {fields}}.
 * @param[in]     _index  its place, for a message.
 * @param[out]    _name   receives the model's name.
 * @param[out]    _spec   receives its fields, in place.
 * @return 0, or D_DB_INVALID_ARGUMENT for an element of another shape.
 */
static
int d_internal_mongodb_model_of(
    struct dj_mongodb* _db,
    const bson_iter_t* _write,
    size_t             _index,
    const char**       _name,
    bson_t*            _spec
)
{
    bson_t      model;
    bson_iter_t first;

    // {name: {fields}}, and nothing else
    if ( (!d_internal_mongodb_field_document(_write, &model)) ||
         (bson_count_keys(&model) != 1u)                    ||
         (!bson_iter_init(&first, &model))                  ||
         (!bson_iter_next(&first))                          ||
         (!d_internal_mongodb_field_document(&first, _spec)) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "write %lu is not one model and its "
                                         "fields, {\"insertOne\": {...}}",
                                         (unsigned long)_index);
    }

    *_name = bson_iter_key(&first);

    return 0;
}

/**
 * @brief Reads a field of a model that holds a document, or, for an
 *        update, a pipeline.
 *
 * @param[in]  _spec   the model's fields.
 * @param[in]  _field  the field.
 * @param[in]  _array  whether a pipeline will do.
 * @param[out] _out    receives it, in place.
 * @return `true`, or `false` where the field is not there or of another
 *         kind.
 */
static
bool d_internal_mongodb_model_field(
    const bson_t* _spec,
    const char*   _field,
    bool          _array,
    bson_t*       _out
)
{
    bson_iter_t    field;
    uint32_t       length = 0u;
    const uint8_t* data   = NULL;

    // the field, a document
    if (!bson_iter_init_find(&field, _spec, _field))
    {
        return false;
    }

    if (BSON_ITER_HOLDS_DOCUMENT(&field))
    {
        return d_internal_mongodb_field_document(&field, _out);
    }

    // or an array, where one will do
    if ( (!_array) ||
         (!BSON_ITER_HOLDS_ARRAY(&field)) )
    {
        return false;
    }

    bson_iter_array(&field, &length, &data);

    return bson_init_static(_out, data, length);
}

/**
 * @brief Adds an insert to a collection's bulk operation.
 *
 * @param[in,out] _db     the handle.
 * @param[in,out] _bulk   the operation.
 * @param[in]     _spec   the model's fields: its document, and no other.
 * @param[in]     _index  its place, for a message.
 * @return 0; D_DB_INVALID_ARGUMENT for no document, or more than it;
 *         the driver's refusal.
 */
static
int d_internal_mongodb_bulk_insert(
    struct dj_mongodb*       _db,
    mongoc_bulk_operation_t* _bulk,
    const bson_t*            _spec,
    size_t                   _index
)
{
    bson_t       document;
    bson_error_t error;

    // its document, and nothing else
    if ( (!d_internal_mongodb_model_field(_spec,
                                          "document",
                                          false,
                                          &document)) ||
         (bson_count_keys(_spec) != 1u) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "write %lu, insertOne, needs its "
                                         "document, and nothing else",
                                         (unsigned long)_index);
    }

    return (mongoc_bulk_operation_insert_with_opts(_bulk,
                                                   &document,
                                                   NULL,
                                                   &error))
               ? 0
               : d_internal_mongodb_fail(_db, &error, NULL);
}

/**
 * @brief Adds an update, a replacement or a delete to a collection's bulk
 *        operation, by the driver's call for its model.
 *
 * @param[in,out] _bulk    the operation.
 * @param[in]     _name    the model's name, one of the five.
 * @param[in]     _filter  its filter.
 * @param[in]     _body    its update or replacement; unread for a delete.
 * @param[in]     _opts    its options.
 * @param[out]    _error   receives the driver's refusal.
 * @return whether the driver took it.
 */
static
bool d_internal_mongodb_bulk_append(
    mongoc_bulk_operation_t* _bulk,
    const char*              _name,
    const bson_t*            _filter,
    const bson_t*            _body,
    const bson_t*            _opts,
    bson_error_t*            _error
)
{
    return (strcmp(_name, "updateOne") == 0)
               ? mongoc_bulk_operation_update_one_with_opts(_bulk, _filter,
                                                            _body, _opts,
                                                            _error)
         : (strcmp(_name, "updateMany") == 0)
               ? mongoc_bulk_operation_update_many_with_opts(_bulk, _filter,
                                                             _body, _opts,
                                                             _error)
         : (strcmp(_name, "replaceOne") == 0)
               ? mongoc_bulk_operation_replace_one_with_opts(_bulk, _filter,
                                                             _body, _opts,
                                                             _error)
         : (strcmp(_name, "deleteOne") == 0)
               ? mongoc_bulk_operation_remove_one_with_opts(_bulk, _filter,
                                                            _opts, _error)
               : mongoc_bulk_operation_remove_many_with_opts(_bulk, _filter,
                                                             _opts, _error);
}

/**
 * @brief Adds one write to a collection's bulk operation.
 *
 * @param[in,out] _db     the handle.
 * @param[in,out] _bulk   the operation.
 * @param[in]     _name   the model's name.
 * @param[in]     _spec   its fields.
 * @param[in]     _index  its place, for a message.
 * @return 0; D_DB_INVALID_ARGUMENT for a model of no such name, or one
 *         without what it needs; the driver's refusal of the rest.
 *
 * The fields besides a model's filter, update, replacement or document are
 * its options, which the driver reads: upsert, arrayFilters, collation,
 * hint, and sort where the driver has it; any other it refuses.
 */
static
int d_internal_mongodb_bulk_add(
    struct dj_mongodb*       _db,
    mongoc_bulk_operation_t* _bulk,
    const char*              _name,
    const bson_t*            _spec,
    size_t                   _index
)
{
    const bool   update  = ( (strcmp(_name, "updateOne") == 0) ||
                             (strcmp(_name, "updateMany") == 0) );
    const bool   replace = (strcmp(_name, "replaceOne") == 0);
    const bool   remove  = ( (strcmp(_name, "deleteOne") == 0) ||
                             (strcmp(_name, "deleteMany") == 0) );
    const char*  second  = (update) ? "update"
                         : (replace) ? "replacement"
                         : NULL;
    bson_t       filter;
    bson_t       body;
    bson_t       opts;
    bson_error_t error;

    // an insert has its document, and the rest their filter, and more
    if (strcmp(_name, "insertOne") == 0)
    {
        return d_internal_mongodb_bulk_insert(_db, _bulk, _spec, _index);
    }

    if ( ( (!update) &&
           (!replace) &&
           (!remove) )                                                    ||
         (!d_internal_mongodb_model_field(_spec, "filter", false, &filter)) ||
         ( (second) &&
           (!d_internal_mongodb_model_field(_spec, second, update, &body)) ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "write %lu, %s, is of no model the "
                                         "layer writes, or lacks its filter "
                                         "or its %s",
                                         (unsigned long)_index,
                                         _name,
                                         (second) ? second : "fields");
    }

    bson_init(&opts);
    bson_copy_to_excluding_noinit(_spec, &opts, "filter", "update",
                                  "replacement", NULL);

    const bool ok = d_internal_mongodb_bulk_append(_bulk,
                                                   _name,
                                                   &filter,
                                                   &body,
                                                   &opts,
                                                   &error);

    bson_destroy(&opts);

    return (ok) ? 0 : d_internal_mongodb_fail(_db, &error, NULL);
}

/**
 * @brief Makes a collection's bulk operation of the options given.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _collection  the collection.
 * @param[in]     _flags       the flags.
 * @param[in]     _options     the options, read.
 * @param[out]    _bulk        receives the operation, or `NULL`.
 * @return 0; D_DB_INVALID_ARGUMENT for ordered among the options, which the
 *         flags say, or a bypassDocumentValidation that is not a boolean;
 *         the session's failure.
 *
 * bypassDocumentValidation has a call of its own; the rest -- writeConcern,
 * comment, let -- are the bulk's options, which a driver that does not
 * take one refuses when it is made.
 */
static
int d_internal_mongodb_bulk_new(
    struct dj_mongodb*        _db,
    mongoc_collection_t*      _collection,
    unsigned                  _flags,
    const bson_t*             _options,
    mongoc_bulk_operation_t** _bulk
)
{
    bson_t      opts;
    bson_iter_t field;
    const bool  bypass = bson_iter_init_find(&field,
                                             _options,
                                             "bypassDocumentValidation");

    *_bulk = NULL;

    // the flags say whether it is ordered, and a bypass is a boolean
    if ( (bson_has_field(_options, "ordered")) ||
         ( (bypass) &&
           (!BSON_ITER_HOLDS_BOOL(&field)) ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "D_MONGODB_UNORDERED says whether "
                                         "writes are ordered, and "
                                         "bypassDocumentValidation is a "
                                         "boolean");
    }

    bson_init(&opts);
    bson_copy_to_excluding_noinit(_options,
                                  &opts,
                                  "bypassDocumentValidation",
                                  NULL);
    BSON_APPEND_BOOL(&opts, "ordered", ((_flags & D_MONGODB_UNORDERED) == 0u));

    int result = d_internal_mongodb_session(_db, &opts);

    // the operation, and its bypass
    if (result == 0)
    {
        *_bulk = mongoc_collection_create_bulk_operation_with_opts(_collection,
                                                                   &opts);

        if (bypass)
        {
            mongoc_bulk_operation_set_bypass_document_validation(
                *_bulk,
                bson_iter_bool(&field));
        }
    }

    bson_destroy(&opts);

    return result;
}

/**
 * @brief Adds every write of an array to a bulk operation.
 *
 * @param[in,out] _db          the handle.
 * @param[in,out] _bulk        the operation.
 * @param[in]     _operations  the writes.
 * @return 0, or the first refusal.
 */
static
int d_internal_mongodb_bulk_fill(
    struct dj_mongodb*       _db,
    mongoc_bulk_operation_t* _bulk,
    const bson_t*            _operations
)
{
    bson_iter_t write;
    size_t      index  = 0u;
    int         result = 0;

    // a bulk write needs something to write
    if (bson_count_keys(_operations) == 0u)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "the array given has no writes");
    }

    // each write, in its order
    if (bson_iter_init(&write, _operations))
    {
        while ( (result == 0) &&
                (bson_iter_next(&write)) )
        {
            const char* name = NULL;
            bson_t      spec;

            result = d_internal_mongodb_model_of(_db,
                                                 &write,
                                                 index,
                                                 &name,
                                                 &spec);
            result = (result == 0)
                         ? d_internal_mongodb_bulk_add(_db,
                                                       _bulk,
                                                       name,
                                                       &spec,
                                                       index)
                         : result;
            ++index;
        }
    }

    return result;
}

/**
 * @brief Runs a collection's bulk operation, and reads its counts.
 *
 * @param[in,out] _db      the handle.
 * @param[in,out] _bulk    the operation, filled.
 * @param[out]    _counts  receives the counts, on a failure too; `NULL`
 *                         where they are not wanted.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_bulk_execute(
    struct dj_mongodb*              _db,
    mongoc_bulk_operation_t*        _bulk,
    struct dj_mongodb_write_counts* _counts
)
{
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };
    bson_t                        reply;
    bson_error_t                  error;

    bson_init(&reply);

    // the writes, their counts on failure and success alike
    const bool ok     = (mongoc_bulk_operation_execute(_bulk,
                                                       &reply,
                                                       &error) != 0u);
    const int  result = (ok) ? 0
                             : d_internal_mongodb_fail(_db, &error, &reply);

    d_internal_mongodb_counts(&reply, &counts);

    // the handle's counts, on success
    if (ok)
    {
        d_internal_mongodb_wrote(_db, &counts);
    }

    if (_counts)
    {
        *_counts = counts;
    }

    bson_destroy(&reply);

    return result;
}

/*
d_mongodb_bulk_write
  The driver's bulk operation, which sends the writes in as few commands as
the server's limits allow and reports what they did in one reply, its
counts named nInserted and the like, on failure as on success.
*/
int d_mongodb_bulk_write(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _operations,
    unsigned                        _flags,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    d_internal_mongodb_no_counts(_counts);

    // a handle, and flags it knows
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_check_flags(_db, _flags);

    mongoc_collection_t* const collection =
        (result == 0)
            ? d_internal_mongodb_collection(_db, _database, _collection)
            : NULL;

    // a collection to write to
    if (!collection)
    {
        return _db->error;
    }

    mongoc_bulk_operation_t* bulk = NULL;
    bson_t                   operations;
    bson_t                   options;

    bson_init(&options);
    result = d_internal_mongodb_read_array(_db, _operations, true, &operations);
    result = (result == 0) ? d_internal_mongodb_read_document(_db,
                                                              _options,
                                                              false,
                                                              &options)
                           : result;
    result = (result == 0) ? d_internal_mongodb_bulk_new(_db,
                                                         collection,
                                                         _flags,
                                                         &options,
                                                         &bulk)
                           : result;
    result = (result == 0)
                 ? d_internal_mongodb_bulk_fill(_db, bulk, &operations)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_bulk_execute(_db, bulk, _counts)
                 : result;

    mongoc_bulk_operation_destroy(bulk);
    bson_destroy(&options);
    bson_destroy(&operations);
    mongoc_collection_destroy(collection);

    return result;
}

#if D_ENV_MONGO_HAS_CLIENT_BULK_WRITE

// d_internal_mongodb_model_options
//   struct: the options of one model of a client's bulk write, read before
// they are given to the driver's options of the model's own kind.
struct dj_internal_mongodb_model_options
{
    bool         upsert;         // upsert, where has_upsert
    bool         has_upsert;     // whether upsert was given
    bson_t       array_filters;  // arrayFilters, in place
    bool         has_filters;    // whether they were given
    bson_t       collation;      // collation, in place
    bool         has_collation;  // whether it was given
    bson_value_t hint;           // hint, copied out of its iterator
    bool         has_hint;       // whether it was given
    bson_t       sort;           // sort, in place
    bool         has_sort;       // whether it was given
};

/**
 * @brief Reads one of a model's options.
 *
 * @param[in,out] _field    the option, whose value libbson reads into it.
 * @param[in,out] _options  receives it.
 * @return `true`, or `false` for one of no such name, or of the wrong type.
 */
static
bool d_internal_mongodb_model_option(
    bson_iter_t*                              _field,
    struct dj_internal_mongodb_model_options* _options
)
{
    const char* const key = bson_iter_key(_field);

    // upsert: a boolean
    if (strcmp(key, "upsert") == 0)
    {
        _options->upsert     = bson_iter_as_bool(_field);
        _options->has_upsert = true;

        return BSON_ITER_HOLDS_BOOL(_field);
    }

    // arrayFilters: an array, read in place
    if (strcmp(key, "arrayFilters") == 0)
    {
        uint32_t       length = 0u;
        const uint8_t* data   = NULL;

        if (!BSON_ITER_HOLDS_ARRAY(_field))
        {
            return false;
        }

        bson_iter_array(_field, &length, &data);
        _options->has_filters = bson_init_static(&_options->array_filters,
                                                 data,
                                                 length);

        return _options->has_filters;
    }

    // collation and sort: documents, read in place
    if (strcmp(key, "collation") == 0)
    {
        _options->has_collation =
            d_internal_mongodb_field_document(_field, &_options->collation);

        return _options->has_collation;
    }

    if (strcmp(key, "sort") == 0)
    {
        _options->has_sort =
            d_internal_mongodb_field_document(_field, &_options->sort);

        return _options->has_sort;
    }

    // hint: a name or a document, copied out of its iterator
    if (strcmp(key, "hint") == 0)
    {
        _options->hint     = *bson_iter_value(_field);
        _options->has_hint = true;

        return true;
    }

    return false;
}

/**
 * @brief Reads a model's options: its fields besides its namespace, filter,
 *        update, replacement or document.
 *
 * @param[in]  _spec     the model's fields.
 * @param[in]  _allowed  the options its kind takes, as their first letters:
 *                       u upsert, a arrayFilters, c collation, h hint, s
 *                       sort.
 * @param[out] _options  receives them.
 * @return `true`, or `false` for a field of no such name, one its kind does
 *         not take, or one of the wrong type.
 */
static
bool d_internal_mongodb_model_options(
    const bson_t*                             _spec,
    const char*                               _allowed,
    struct dj_internal_mongodb_model_options* _options
)
{
    bson_iter_t field;
    bool        ok = bson_iter_init(&field, _spec);

    memset(_options, 0, sizeof(*_options));

    // each field, to its place
    while ( (ok) &&
            (bson_iter_next(&field)) )
    {
        const char* const key = bson_iter_key(&field);

        // the model's parts, which are not options
        if ( (strcmp(key, "namespace") == 0) ||
             (strcmp(key, "filter") == 0)    ||
             (strcmp(key, "update") == 0)    ||
             (strcmp(key, "replacement") == 0) )
        {
            continue;
        }

        // an option its kind takes, by its first letter, read
        ok = ( (key[0] != '\0')              &&
               (strchr(_allowed, key[0])) &&
               (d_internal_mongodb_model_option(&field, _options)) );
    }

    return ok;
}

/**
 * @brief Appends an updateOne model to a client's bulk write.
 *
 * @param[in,out] _write    the bulk write.
 * @param[in]     _ns       its namespace.
 * @param[in]     _filter   its filter.
 * @param[in]     _update   its update or pipeline.
 * @param[in]     _options  its options.
 * @param[out]    _error    receives the driver's refusal.
 * @return what the driver returned.
 */
static
bool d_internal_mongodb_append_update_one(
    mongoc_bulkwrite_t*                             _write,
    const char*                                     _ns,
    const bson_t*                                   _filter,
    const bson_t*                                   _update,
    const struct dj_internal_mongodb_model_options* _options,
    bson_error_t*                                   _error
)
{
    mongoc_bulkwrite_updateoneopts_t* const opts =
        mongoc_bulkwrite_updateoneopts_new();

    // the options it was given
    if (_options->has_upsert)
    {
        mongoc_bulkwrite_updateoneopts_set_upsert(opts, _options->upsert);
    }

    if (_options->has_filters)
    {
        mongoc_bulkwrite_updateoneopts_set_arrayfilters(
            opts,
            &_options->array_filters);
    }

    if (_options->has_collation)
    {
        mongoc_bulkwrite_updateoneopts_set_collation(opts,
                                                     &_options->collation);
    }

    if (_options->has_hint)
    {
        mongoc_bulkwrite_updateoneopts_set_hint(opts, &_options->hint);
    }

    if (_options->has_sort)
    {
        mongoc_bulkwrite_updateoneopts_set_sort(opts, &_options->sort);
    }

    const bool ok = mongoc_bulkwrite_append_updateone(_write,
                                                      _ns,
                                                      _filter,
                                                      _update,
                                                      opts,
                                                      _error);

    mongoc_bulkwrite_updateoneopts_destroy(opts);

    return ok;
}

/**
 * @brief Appends an updateMany model to a client's bulk write.
 *
 * @param[in,out] _write    the bulk write.
 * @param[in]     _ns       its namespace.
 * @param[in]     _filter   its filter.
 * @param[in]     _update   its update or pipeline.
 * @param[in]     _options  its options, which hold no sort.
 * @param[out]    _error    receives the driver's refusal.
 * @return what the driver returned.
 */
static
bool d_internal_mongodb_append_update_many(
    mongoc_bulkwrite_t*                             _write,
    const char*                                     _ns,
    const bson_t*                                   _filter,
    const bson_t*                                   _update,
    const struct dj_internal_mongodb_model_options* _options,
    bson_error_t*                                   _error
)
{
    mongoc_bulkwrite_updatemanyopts_t* const opts =
        mongoc_bulkwrite_updatemanyopts_new();

    // the options it was given
    if (_options->has_upsert)
    {
        mongoc_bulkwrite_updatemanyopts_set_upsert(opts, _options->upsert);
    }

    if (_options->has_filters)
    {
        mongoc_bulkwrite_updatemanyopts_set_arrayfilters(
            opts,
            &_options->array_filters);
    }

    if (_options->has_collation)
    {
        mongoc_bulkwrite_updatemanyopts_set_collation(opts,
                                                      &_options->collation);
    }

    if (_options->has_hint)
    {
        mongoc_bulkwrite_updatemanyopts_set_hint(opts, &_options->hint);
    }

    const bool ok = mongoc_bulkwrite_append_updatemany(_write,
                                                       _ns,
                                                       _filter,
                                                       _update,
                                                       opts,
                                                       _error);

    mongoc_bulkwrite_updatemanyopts_destroy(opts);

    return ok;
}

/**
 * @brief Appends a replaceOne model to a client's bulk write.
 *
 * @param[in,out] _write        the bulk write.
 * @param[in]     _ns           its namespace.
 * @param[in]     _filter       its filter.
 * @param[in]     _replacement  its replacement.
 * @param[in]     _options      its options.
 * @param[out]    _error        receives the driver's refusal.
 * @return what the driver returned.
 */
static
bool d_internal_mongodb_append_replace(
    mongoc_bulkwrite_t*                             _write,
    const char*                                     _ns,
    const bson_t*                                   _filter,
    const bson_t*                                   _replacement,
    const struct dj_internal_mongodb_model_options* _options,
    bson_error_t*                                   _error
)
{
    mongoc_bulkwrite_replaceoneopts_t* const opts =
        mongoc_bulkwrite_replaceoneopts_new();

    // the options it was given
    if (_options->has_upsert)
    {
        mongoc_bulkwrite_replaceoneopts_set_upsert(opts, _options->upsert);
    }

    if (_options->has_collation)
    {
        mongoc_bulkwrite_replaceoneopts_set_collation(opts,
                                                      &_options->collation);
    }

    if (_options->has_hint)
    {
        mongoc_bulkwrite_replaceoneopts_set_hint(opts, &_options->hint);
    }

    if (_options->has_sort)
    {
        mongoc_bulkwrite_replaceoneopts_set_sort(opts, &_options->sort);
    }

    const bool ok = mongoc_bulkwrite_append_replaceone(_write,
                                                       _ns,
                                                       _filter,
                                                       _replacement,
                                                       opts,
                                                       _error);

    mongoc_bulkwrite_replaceoneopts_destroy(opts);

    return ok;
}

/**
 * @brief Appends a delete model -- deleteOne, deleteMany -- to a client's
 *        bulk write.
 *
 * @param[in,out] _write    the bulk write.
 * @param[in]     _many     whether deleteMany.
 * @param[in]     _ns       its namespace.
 * @param[in]     _filter   its filter.
 * @param[in]     _options  its options.
 * @param[out]    _error    receives the driver's refusal.
 * @return what the driver returned.
 */
static
bool d_internal_mongodb_append_delete(
    mongoc_bulkwrite_t*                             _write,
    bool                                            _many,
    const char*                                     _ns,
    const bson_t*                                   _filter,
    const struct dj_internal_mongodb_model_options* _options,
    bson_error_t*                                   _error
)
{
    bool ok = false;

    // the one
    if (!_many)
    {
        mongoc_bulkwrite_deleteoneopts_t* const opts =
            mongoc_bulkwrite_deleteoneopts_new();

        if (_options->has_collation)
        {
            mongoc_bulkwrite_deleteoneopts_set_collation(opts,
                                                         &_options->collation);
        }

        if (_options->has_hint)
        {
            mongoc_bulkwrite_deleteoneopts_set_hint(opts, &_options->hint);
        }

        ok = mongoc_bulkwrite_append_deleteone(_write, _ns, _filter, opts,
                                               _error);
        mongoc_bulkwrite_deleteoneopts_destroy(opts);

        return ok;
    }

    mongoc_bulkwrite_deletemanyopts_t* const opts =
        mongoc_bulkwrite_deletemanyopts_new();

    // the many
    if (_options->has_collation)
    {
        mongoc_bulkwrite_deletemanyopts_set_collation(opts,
                                                      &_options->collation);
    }

    if (_options->has_hint)
    {
        mongoc_bulkwrite_deletemanyopts_set_hint(opts, &_options->hint);
    }

    ok = mongoc_bulkwrite_append_deletemany(_write, _ns, _filter, opts,
                                            _error);
    mongoc_bulkwrite_deletemanyopts_destroy(opts);

    return ok;
}

/**
 * @brief Appends a model to a client's bulk write, by the driver's call
 *        for its kind.
 *
 * @param[in,out] _write    the bulk write.
 * @param[in]     _name     the model's name, one of the six.
 * @param[in]     _ns       its namespace.
 * @param[in]     _filter   its filter; unread for an insert.
 * @param[in]     _body     its document, update or replacement; unread for
 *                          a delete.
 * @param[in]     _options  its options; unread for an insert.
 * @param[out]    _error    receives the driver's refusal.
 * @return what the driver returned.
 */
static
bool d_internal_mongodb_client_append(
    mongoc_bulkwrite_t*                             _write,
    const char*                                     _name,
    const char*                                     _ns,
    const bson_t*                                   _filter,
    const bson_t*                                   _body,
    const struct dj_internal_mongodb_model_options* _options,
    bson_error_t*                                   _error
)
{
    return (strcmp(_name, "insertOne") == 0)
               ? mongoc_bulkwrite_append_insertone(_write, _ns, _body, NULL,
                                                   _error)
         : (strcmp(_name, "updateOne") == 0)
               ? d_internal_mongodb_append_update_one(_write, _ns, _filter,
                                                      _body, _options,
                                                      _error)
         : (strcmp(_name, "updateMany") == 0)
               ? d_internal_mongodb_append_update_many(_write, _ns, _filter,
                                                       _body, _options,
                                                       _error)
         : (strcmp(_name, "replaceOne") == 0)
               ? d_internal_mongodb_append_replace(_write, _ns, _filter,
                                                   _body, _options, _error)
               : d_internal_mongodb_append_delete(
                     _write,
                     (strcmp(_name, "deleteMany") == 0),
                     _ns, _filter, _options, _error);
}

/**
 * @brief Adds one write to a client's bulk write.
 *
 * @param[in,out] _db     the handle.
 * @param[in,out] _write  the bulk write.
 * @param[in]     _name   the model's name.
 * @param[in]     _spec   its fields.
 * @param[in]     _index  its place, for a message.
 * @return 0; D_DB_INVALID_ARGUMENT for a model of no such name, or without
 *         its namespace or what else it needs, or with an option its kind
 *         does not take; the driver's refusal of the rest.
 */
static
int d_internal_mongodb_client_add(
    struct dj_mongodb*  _db,
    mongoc_bulkwrite_t* _write,
    const char*         _name,
    const bson_t*       _spec,
    size_t              _index
)
{
    const bool  insert  = (strcmp(_name, "insertOne") == 0);
    const bool  update  = ( (strcmp(_name, "updateOne") == 0) ||
                            (strcmp(_name, "updateMany") == 0) );
    const bool  replace = (strcmp(_name, "replaceOne") == 0);
    const bool  remove  = ( (strcmp(_name, "deleteOne") == 0) ||
                            (strcmp(_name, "deleteMany") == 0) );
    const char* body_of = (insert) ? "document"
                        : (update) ? "update"
                        : (replace) ? "replacement"
                        : NULL;
    const char* allowed = (strcmp(_name, "updateOne") == 0) ? "uachs"
                        : (update)                           ? "uach"
                        : (replace)                          ? "uchs"
                        : "ch";
    struct dj_internal_mongodb_model_options options;
    bson_iter_t                             ns;
    bson_t                                  filter;
    bson_t                                  body;
    bson_error_t                            error;

    // a model of a name, its namespace, its parts, its options
    if ( ( (!insert) && (!update) && (!replace) && (!remove) )       ||
         (!bson_iter_init_find(&ns, _spec, "namespace"))             ||
         (!BSON_ITER_HOLDS_UTF8(&ns))                                ||
         ( (!insert) &&
           (!d_internal_mongodb_model_field(_spec,
                                            "filter",
                                            false,
                                            &filter)) )              ||
         ( (body_of) &&
           (!d_internal_mongodb_model_field(_spec,
                                            body_of,
                                            update,
                                            &body)) )                ||
         ( (insert) &&
           (bson_count_keys(_spec) != 2u) )                          ||
         ( (!insert) &&
           (!d_internal_mongodb_model_options(_spec, allowed, &options)) ) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "write %lu, %s, is of no model the "
                                         "layer writes, or lacks its "
                                         "namespace or its parts, or has a "
                                         "field its model does not take",
                                         (unsigned long)_index,
                                         _name);
    }

    return (d_internal_mongodb_client_append(_write,
                                             _name,
                                             bson_iter_utf8(&ns, NULL),
                                             &filter,
                                             &body,
                                             &options,
                                             &error))
               ? 0
               : d_internal_mongodb_fail(_db, &error, NULL);
}

/**
 * @brief Reads a client's bulk write's options into the driver's.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _options  the options, read.
 * @param[in]     _flags    the flags.
 * @param[in,out] _opts     the driver's options.
 * @param[out]    _concern  receives a write concern given, which the caller
 *                          destroys after the write; `NULL` for none.
 * @return 0, or D_DB_INVALID_ARGUMENT for an option of no such name or of
 *         the wrong type, recorded.
 */
static
int d_internal_mongodb_client_options(
    struct dj_mongodb*       _db,
    const bson_t*            _options,
    unsigned                 _flags,
    mongoc_bulkwriteopts_t*  _opts,
    mongoc_write_concern_t** _concern
)
{
    bson_iter_t field;
    bson_t      document;
    int         result = 0;

    *_concern = NULL;
    mongoc_bulkwriteopts_set_ordered(_opts,
                                     ((_flags & D_MONGODB_UNORDERED) == 0u));

    // each option, to its call
    if (bson_iter_init(&field, _options))
    {
        while ( (result == 0) &&
                (bson_iter_next(&field)) )
        {
            const char* const key = bson_iter_key(&field);

            if ( (strcmp(key, "bypassDocumentValidation") == 0) &&
                 (BSON_ITER_HOLDS_BOOL(&field)) )
            {
                mongoc_bulkwriteopts_set_bypassdocumentvalidation(
                    _opts,
                    bson_iter_bool(&field));
            }
            else if (strcmp(key, "comment") == 0)
            {
                mongoc_bulkwriteopts_set_comment(_opts,
                                                 bson_iter_value(&field));
            }
            else if ( (strcmp(key, "let") == 0) &&
                      (d_internal_mongodb_field_document(&field, &document)) )
            {
                mongoc_bulkwriteopts_set_let(_opts, &document);
            }
            else if ( (strcmp(key, "writeConcern") == 0)                   &&
                      (!*_concern)                                         &&
                      (d_internal_mongodb_field_document(&field, &document)) )
            {
                result = d_internal_mongodb_write_concern(_db,
                                                          &document,
                                                          _concern);

                if ( (result == 0) &&
                     (*_concern) )
                {
                    mongoc_bulkwriteopts_set_writeconcern(_opts, *_concern);
                }
            }
            else
            {
                result = d_internal_mongodb_answer(
                             _db,
                             D_DB_INVALID_ARGUMENT,
                             "a bulk write cannot take its option \"%s\" as "
                             "given",
                             key);
            }
        }
    }

    return result;
}

/**
 * @brief Finds a client's bulk write's first write error by place.
 *
 * @param[in]  _errors  the errors, keyed by their writes' places.
 * @param[out] _first   receives the iterator at the first.
 * @return its place; `(unsigned long)-1` for no error.
 */
static
unsigned long d_internal_mongodb_first_place(
    const bson_t* _errors,
    bson_iter_t*  _first
)
{
    bson_iter_t   field;
    unsigned long least = (unsigned long)-1;

    memset(_first, 0, sizeof(*_first));

    // the least place among them
    if (bson_iter_init(&field, _errors))
    {
        while (bson_iter_next(&field))
        {
            const unsigned long place = strtoul(bson_iter_key(&field),
                                                NULL,
                                                10);

            if (place < least)
            {
                least  = place;
                *_first = field;
            }
        }
    }

    return least;
}

/**
 * @brief Writes a client's bulk write's error as a command writes one of
 *        its writeErrors: {index, code, errmsg, errInfo}.
 *
 * @param[in,out] _array  the array it is appended to, as its "0".
 * @param[in]     _error  the error, an iterator at its document.
 * @param[in]     _place  its write's place.
 */
static
void d_internal_mongodb_write_error_entry(
    bson_t*            _array,
    const bson_iter_t* _error,
    unsigned long      _place
)
{
    bson_t      spec;
    bson_t      entry;
    bson_iter_t part;

    (void)d_internal_mongodb_field_document(_error, &spec);
    BSON_APPEND_DOCUMENT_BEGIN(_array, "0", &entry);
    BSON_APPEND_INT64(&entry, "index", (int64_t)_place);

    // each part it has, by the command's name for it
    if (bson_iter_init_find(&part, &spec, "code"))
    {
        (void)bson_append_iter(&entry, "code", -1, &part);
    }

    if (bson_iter_init_find(&part, &spec, "message"))
    {
        (void)bson_append_iter(&entry, "errmsg", -1, &part);
    }

    if (bson_iter_init_find(&part, &spec, "details"))
    {
        (void)bson_append_iter(&entry, "errInfo", -1, &part);
    }

    bson_append_document_end(_array, &entry);

    return;
}

/**
 * @brief Records a client's bulk write's failure: its top-level error, or
 *        its first write error by place, or its first write concern error,
 *        with a reply that holds them all, as a command's would.
 *
 * @param[in,out] _db         the handle.
 * @param[in]     _exception  the driver's description.
 * @return the failure's code.
 */
static
int d_internal_mongodb_client_failure(
    struct dj_mongodb*                 _db,
    const mongoc_bulkwriteexception_t* _exception
)
{
    const bson_t* const errors   =
        mongoc_bulkwriteexception_writeerrors(_exception);
    const bson_t* const concerns =
        mongoc_bulkwriteexception_writeconcernerrors(_exception);
    bson_error_t        error;
    bson_iter_t         field;
    bson_iter_t         first;

    // a top-level error, the server's or the driver's
    if (mongoc_bulkwriteexception_error(_exception, &error))
    {
        return d_internal_mongodb_fail(
                   _db,
                   &error,
                   mongoc_bulkwriteexception_errorreply(_exception));
    }

    const unsigned long least = d_internal_mongodb_first_place(errors,
                                                               &first);
    bson_t              reply;
    bson_t              array;

    bson_init(&reply);
    bson_init(&array);

    // the first write error by place, as a command writes it
    if (least != (unsigned long)-1)
    {
        d_internal_mongodb_write_error_entry(&array, &first, least);
    }

    BSON_APPEND_ARRAY(&reply, "writeErrors", &array);
    bson_destroy(&array);
    BSON_APPEND_DOCUMENT(&reply, "writeErrorsByPlace", errors);
    BSON_APPEND_ARRAY(&reply, "writeConcernErrors", concerns);

    // the first write concern error, as a command's one is written
    if ( (bson_iter_init(&field, concerns)) &&
         (bson_iter_next(&field))          &&
         (BSON_ITER_HOLDS_DOCUMENT(&field)) )
    {
        (void)bson_append_iter(&reply, "writeConcernError", -1, &field);
    }

    const int result = d_internal_mongodb_reply_failure(_db, &reply);

    bson_destroy(&reply);

    return (result != 0) ? result
                         : d_internal_mongodb_answer(
                               _db,
                               D_MONGODB_LAYER_UNDESCRIBED,
                               "the bulk write failed, and the driver "
                               "described no failure");
}

/**
 * @brief Runs a client's bulk write, in the open transaction's session,
 *        and reads what it did.
 *
 * @param[in,out] _db      the handle.
 * @param[in,out] _write   the bulk write, filled.
 * @param[in]     _opts    its options.
 * @param[out]    _counts  receives the counts, on failure and success
 *                         alike; may be `NULL`.
 * @return 0, or the failure.
 */
static
int d_internal_mongodb_client_execute(
    struct dj_mongodb*              _db,
    mongoc_bulkwrite_t*             _write,
    const mongoc_bulkwriteopts_t*   _opts,
    struct dj_mongodb_write_counts* _counts
)
{
    struct dj_mongodb_write_counts counts = { 0, 0, 0, 0, 0 };

    // inside a transaction, its session
    if (d_mongodb_in_transaction(_db))
    {
        mongoc_bulkwrite_set_session(_write, _db->session);
    }

    const mongoc_bulkwritereturn_t returned = mongoc_bulkwrite_execute(_write,
                                                                       _opts);

    // what the writes known to have succeeded did
    if (returned.res)
    {
        counts.inserted = mongoc_bulkwriteresult_insertedcount(returned.res);
        counts.matched  = mongoc_bulkwriteresult_matchedcount(returned.res);
        counts.modified = mongoc_bulkwriteresult_modifiedcount(returned.res);
        counts.deleted  = mongoc_bulkwriteresult_deletedcount(returned.res);
        counts.upserted = mongoc_bulkwriteresult_upsertedcount(returned.res);
    }

    const int result = (returned.exc)
                           ? d_internal_mongodb_client_failure(_db,
                                                               returned.exc)
                           : 0;

    // a success is the handle's latest write
    if (result == 0)
    {
        d_internal_mongodb_wrote(_db, &counts);
    }

    if (_counts)
    {
        *_counts = counts;
    }

    mongoc_bulkwriteresult_destroy(returned.res);
    mongoc_bulkwriteexception_destroy(returned.exc);

    return result;
}

/**
 * @brief Adds every write of an array to a client's bulk write.
 *
 * @param[in,out] _db          the handle.
 * @param[in,out] _write       the bulk write.
 * @param[in]     _operations  the writes.
 * @return 0, or the first refusal.
 */
static
int d_internal_mongodb_client_fill(
    struct dj_mongodb*  _db,
    mongoc_bulkwrite_t* _write,
    const bson_t*       _operations
)
{
    bson_iter_t element;
    size_t      index  = 0u;
    int         result = 0;

    // a bulk write needs something to write
    if (bson_count_keys(_operations) == 0u)
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_INVALID_ARGUMENT,
                                         "the array given has no writes");
    }

    // each write, in its order
    if (bson_iter_init(&element, _operations))
    {
        while ( (result == 0) &&
                (bson_iter_next(&element)) )
        {
            const char* name = NULL;
            bson_t      spec;

            result = d_internal_mongodb_model_of(_db,
                                                 &element,
                                                 index,
                                                 &name,
                                                 &spec);
            result = (result == 0)
                         ? d_internal_mongodb_client_add(_db,
                                                         _write,
                                                         name,
                                                         &spec,
                                                         index)
                         : result;
            ++index;
        }
    }

    return result;
}

/*
d_mongodb_client_bulk_write
  The server's bulkWrite, through the driver's mongoc_bulkwrite_t, from
1.28: each model's namespace is its own, and its options are of its kind.
The failures come back by the place of the write that failed, which the
layer gives as a command's writeErrors are given, the first by place
first.
*/
int d_mongodb_client_bulk_write(
    struct dj_mongodb*              _db,
    const char*                     _operations,
    unsigned                        _flags,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    d_internal_mongodb_no_counts(_counts);

    // a handle, a server and a driver that can, and flags it knows
    if ( (!_db) ||
         (!d_mongodb_supports(_db, D_MONGODB_FEATURE_BULK_WRITE)) )
    {
        return (!_db) ? D_DB_INVALID_ARGUMENT
                      : d_internal_mongodb_answer(_db,
                                                  D_DB_UNSUPPORTED,
                                                  "a bulk write across "
                                                  "collections needs "
                                                  "MongoDB 8.0");
    }

    mongoc_bulkwrite_t* const     write   = mongoc_client_bulkwrite_new(
                                                _db->client);
    mongoc_bulkwriteopts_t* const opts    = mongoc_bulkwriteopts_new();
    mongoc_write_concern_t*       concern = NULL;
    bson_t                        operations;
    bson_t                        options;
    int                           result  = d_internal_mongodb_check_flags(
                                                _db,
                                                _flags);

    bson_init(&operations);
    bson_init(&options);
    result = (result == 0)
                 ? d_internal_mongodb_read_array(_db,
                                                 _operations,
                                                 true,
                                                 &operations)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_read_document(_db,
                                                    _options,
                                                    false,
                                                    &options)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_client_options(_db,
                                                     &options,
                                                     _flags,
                                                     opts,
                                                     &concern)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_client_fill(_db, write, &operations)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_client_execute(_db, write, opts, _counts)
                 : result;

    mongoc_write_concern_destroy(concern);
    mongoc_bulkwriteopts_destroy(opts);
    mongoc_bulkwrite_destroy(write);
    bson_destroy(&options);
    bson_destroy(&operations);

    return result;
}

#else

/*
d_mongodb_client_bulk_write
  The driver's headers declare no client bulk write before 1.28:
D_DB_UNSUPPORTED, whatever the server, nothing written.
*/
int d_mongodb_client_bulk_write(
    struct dj_mongodb*              _db,
    const char*                     _operations,
    unsigned                        _flags,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    (void)_operations;
    (void)_flags;
    (void)_options;

    d_internal_mongodb_no_counts(_counts);

    return (_db) ? d_internal_mongodb_answer(_db,
                                             D_DB_UNSUPPORTED,
                                             "a bulk write across collections "
                                             "needs the driver's 1.28")
                 : D_DB_INVALID_ARGUMENT;
}

#endif  // D_ENV_MONGO_HAS_CLIENT_BULK_WRITE

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports D_DB_UNSUPPORTED,
// writing the counts of no writes where it promises counts on any failure,
// and leaving its other answers untouched.

/*
d_mongodb_insert_one
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_insert_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _document,
    char**             _id
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_document;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_insert_many
  Without the driver, D_DB_UNSUPPORTED, none inserted.
*/
int d_mongodb_insert_many(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _documents,
    unsigned           _flags,
    int64_t*           _inserted
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_documents;
    (void)_flags;

    // none went in
    if (_inserted)
    {
        *_inserted = 0;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_find
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_find_one
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_find_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_find_keyed
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_find_keyed(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_update_one
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_update_one(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _update,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_update;
    (void)_options;
    (void)_counts;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_update_many
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_update_many(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _update,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_update;
    (void)_options;
    (void)_counts;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_replace_one
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_replace_one(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _replacement,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_replacement;
    (void)_options;
    (void)_counts;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_delete_one
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_delete_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _deleted
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;
    (void)_deleted;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_delete_many
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_delete_many(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _deleted
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;
    (void)_deleted;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_count_documents
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_count_documents(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _count
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;
    (void)_count;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_estimated_document_count
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_estimated_document_count(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _options,
    int64_t*           _count
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_options;
    (void)_count;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_distinct
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_distinct(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _field,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_field;
    (void)_filter;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_aggregate
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_aggregate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _pipeline,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_pipeline;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_find_one_and_update
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_find_one_and_update(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _update,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_update;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_find_one_and_replace
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_find_one_and_replace(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _replacement,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_replacement;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_find_one_and_delete
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_find_one_and_delete(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_bulk_write
  Without the driver, D_DB_UNSUPPORTED, nothing written.
*/
int d_mongodb_bulk_write(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _operations,
    unsigned                        _flags,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_operations;
    (void)_flags;
    (void)_options;

    d_internal_mongodb_no_counts(_counts);

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_client_bulk_write
  Without the driver, D_DB_UNSUPPORTED, nothing written.
*/
int d_mongodb_client_bulk_write(
    struct dj_mongodb*              _db,
    const char*                     _operations,
    unsigned                        _flags,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
)
{
    (void)_db;
    (void)_operations;
    (void)_flags;
    (void)_options;

    d_internal_mongodb_no_counts(_counts);

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
