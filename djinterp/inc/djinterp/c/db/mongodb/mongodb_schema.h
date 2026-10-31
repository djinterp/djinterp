/*******************************************************************************
* djinterp [c]                                                  mongodb_schema.h
*
* A database's collections, views and indexes.
*   COLLECTIONS. A collection is created by its first write, or explicitly,
* with options -- a validator, a capped size, time series, clustering, a
* collation, change stream pre- and post-images -- as the create command
* takes them, given as JSON. A view is a pipeline over a collection or
* another view, read as a collection is and written to never. Each list
* names what listCollections lists, in its order, the system collections
* among them; time series collections are collections.
*   INDEXES. An index is named and defined by its keys, a JSON document of
* fields and their kinds -- {"n": 1} ascending, {"s": -1} descending,
* {"t": "text"}, {"g": "2dsphere"}, {"h": "hashed"}, {"$**": 1} a wildcard
* -- and options as createIndexes takes them: unique (or D_DB_UNIQUE),
* sparse, partialFilterExpression, expireAfterSeconds, collation, hidden,
* wildcardProjection. Every collection has the index "_id_", which cannot be
* dropped.
*   EXISTENCE. The servers disagree: before 7.0 a create of a collection
* that is there is the server's 48, NamespaceExists, and a drop of one that is
* not is 26, NamespaceNotFound; from 7.0 both succeed, as a drop of an index
* that is not there does from 8.3, where 27, IndexNotFound, was. The layer
* asks first, so the codes and the flags mean the same on every server: a
* create of a collection or a view that is there is 48, a drop of one that
* is not 26, and of an index 27, unless the flags say otherwise; a create of
* an index of a name already there is D_DB_EXISTS, which no server reports
* -- each succeeds silently for the same keys and options -- unless
* D_DB_IF_NOT_EXISTS. A failure is the server's code, the driver's, or
* D_DB_INVALID_ARGUMENT for a NULL argument, a document of the wrong shape,
* or no current database where the database is NULL.
*   TRANSACTIONS. A create of a collection, a view or an index joins an
* open transaction from MongoDB 4.4; a drop or a rename never can. Either
* where the server would refuse it is D_DB_UNSUPPORTED, refused before it is
* sent, so that the transaction stays open, which the server's refusal would
* end. Outside a transaction each runs with the handle's write concern.
* Existence and the lists are asked outside any transaction, which holds
* none of them, and so do not see what the open one has made.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_schema.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FLAGS
    -----
    1.  Renaming
         1.  D_MONGODB_DROP_TARGET
2.  COLLECTIONS
    -----------
    1.  Existence
    2.  Creating and dropping
    3.  Renaming
    4.  Listing
    5.  Description
3.  VIEWS
    -----
    1.  Existence
    2.  Creating and dropping
    3.  Listing
4.  INDEXES
    -------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_SCHEMA_H
#define DJINTERP_C_DB_MONGODB_MONGODB_SCHEMA_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // D_DB_IF_*, D_DB_UNIQUE, D_DB_VENDOR_FLAGS,
                               // dj_db_name_list, dj_db_rows
#include "./mongodb_common.h"  // dj_mongodb, the layer's results
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // INT64_MAX: section 4.3's
                                                 // floor, dj_db_rows's


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FLAGS
//==============================================================================


// 1.1    Renaming
//------------------------------------------------------------------------------
// 1.1.1
// D_MONGODB_DROP_TARGET
//   constant: a rename onto a collection that is there drops it first,
// rather than failing with 48.
#define D_MONGODB_DROP_TARGET  (D_DB_VENDOR_FLAGS << 1)


//==============================================================================
// 2.  COLLECTIONS
//==============================================================================


// 2.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the database has a collection of the name, a view
// not counting; has_x_s reports a failure and writes false on one
bool d_mongodb_has_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
) D_NOTHROW;
int d_mongodb_has_collection_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    bool*              _exists
) D_NOTHROW;

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
// creating and dropping -- create_collection makes a collection with the
// create command's options as JSON (NULL for none): {"capped": true,
// "size": 4096}, {"timeseries": {"timeField": "t"}}, {"validator":
// {"$jsonSchema": ...}}, {"clusteredIndex": {"key": {"_id": 1}, "unique":
// true}}; D_DB_IF_NOT_EXISTS leaves one that is there as it is.
// drop_collection drops a collection and its indexes, or a view;
// D_DB_IF_EXISTS succeeds where there is neither
int d_mongodb_create_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    const char*        _options,
    unsigned           _flags
) D_NOTHROW;
int d_mongodb_drop_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    unsigned           _flags
) D_NOTHROW;

// 2.3    Renaming
//------------------------------------------------------------------------------
// renaming -- renames a collection, within its database or into another
// (_to_database NULL keeps it in its own), its indexes with it; one onto a
// collection that is there is 48 unless _flags has D_MONGODB_DROP_TARGET; a
// collection that is not there is 26
int d_mongodb_rename_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _from,
    const char*        _to_database,
    const char*        _to,
    unsigned           _flags
) D_NOTHROW;

// 2.4    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a database's collections, views left out.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database; `NULL` for the current one.
 * @param[out] _names     receives the names; written only on success. A
 *                        database that is not there has none.
 * @return 0, the failure, or D_MONGODB_LAYER_NO_MEMORY.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int d_mongodb_list_collections(
    struct dj_mongodb*      _db,
    const char*             _database,
    struct dj_db_name_list* _names
) D_NOTHROW;

// 2.5    Description
//------------------------------------------------------------------------------
/**
 * @brief Hands back what listCollections says of a collection or a view:
 *        its type, its options as it was created with them, its info (its
 *        UUID, whether it is read-only), and its _id index.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database; `NULL` for the current one.
 * @param[in]  _name      the collection's or the view's name.
 * @param[out] _info      receives the description, as canonical Extended
 *                        JSON, released with d_db_free; written only on
 *                        success.
 * @return 0; 26, the server's NamespaceNotFound, for neither of the name;
 *         the failure; D_MONGODB_LAYER_NO_MEMORY.
 * @post On success the caller releases `*_info` with d_db_free.
 */
int d_mongodb_collection_info(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    char**             _info
) D_NOTHROW;


//==============================================================================
// 3.  VIEWS
//==============================================================================


// 3.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the database has a view of the name; has_x_s reports
// a failure and writes false on one
bool d_mongodb_has_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
) D_NOTHROW;
int d_mongodb_has_view_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    bool*              _exists
) D_NOTHROW;

// 3.2    Creating and dropping
//------------------------------------------------------------------------------
// creating and dropping -- create_view makes a view of a collection or of
// another view of the same database, the pipeline a JSON array of stages,
// and options as create takes them, a collation, or NULL;
// D_DB_IF_NOT_EXISTS leaves a view, or a collection, of the name as it is.
// drop_view drops a view, and never a collection: a name with no view is 26,
// as for a drop_collection of nothing, a collection of the name or not, and
// D_DB_IF_EXISTS succeeds there, leaving any collection as it is
int d_mongodb_create_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    const char*        _source,
    const char*        _pipeline,
    const char*        _options,
    unsigned           _flags
) D_NOTHROW;
int d_mongodb_drop_view(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name,
    unsigned           _flags
) D_NOTHROW;

// 3.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a database's views.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database; `NULL` for the current one.
 * @param[out] _names     receives the names; written only on success.
 * @return 0, the failure, or D_MONGODB_LAYER_NO_MEMORY.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int d_mongodb_list_views(
    struct dj_mongodb*      _db,
    const char*             _database,
    struct dj_db_name_list* _names
) D_NOTHROW;


//==============================================================================
// 4.  INDEXES
//==============================================================================


// 4.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the collection has an index of the name; has_x_s
// reports a failure, and writes false on one: a collection that is not
// there has no indexes, and answers false without failing
bool d_mongodb_has_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name
) D_NOTHROW;
int d_mongodb_has_index_s(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    bool*              _exists
) D_NOTHROW;

// 4.2    Creating and dropping
//------------------------------------------------------------------------------
// creating and dropping -- create_index builds an index of the name over
// the keys, a JSON document, with createIndexes' options as JSON or NULL;
// D_DB_UNIQUE adds "unique": true; the collection is created if it is not
// there; D_DB_IF_NOT_EXISTS leaves an index of the name as it is, whatever
// its keys. The server refuses keys an index of another name already has
// (85, IndexOptionsConflict); a name already there the layer answers, with
// D_DB_EXISTS, before the server's 86, IndexKeySpecsConflict, can.
// drop_index drops the index of the name, "_id_"
// refused by the server; D_DB_IF_EXISTS succeeds where there is none, or no
// collection
int d_mongodb_create_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    const char*        _keys,
    const char*        _options,
    unsigned           _flags
) D_NOTHROW;
int d_mongodb_drop_index(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _name,
    unsigned           _flags
) D_NOTHROW;

// 4.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists a collection's indexes by name, "_id_" first.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection; one that is not there has none.
 * @param[out] _names       receives the names; written only on success.
 * @return 0, the failure, or D_MONGODB_LAYER_NO_MEMORY.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int d_mongodb_list_indexes(
    struct dj_mongodb*      _db,
    const char*             _database,
    const char*             _collection,
    struct dj_db_name_list* _names
) D_NOTHROW;

#if defined(INT64_MAX)
/**
 * @brief Returns each of a collection's indexes as listIndexes describes
 *        it: its version, keys, name, and options.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection; one that is not there has none.
 * @param[out] _rows        receives the descriptions, as documents
 *                          (mongodb_query.h); written only on success.
 * @return 0, the failure, or D_MONGODB_LAYER_NO_MEMORY.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_describe_indexes(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    struct dj_db_rows* _rows
) D_NOTHROW;
#endif  // defined(INT64_MAX)


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_SCHEMA_H
