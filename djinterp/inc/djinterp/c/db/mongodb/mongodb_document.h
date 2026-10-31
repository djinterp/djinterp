/*******************************************************************************
* djinterp [c]                                                mongodb_document.h
*
* The documents of a collection: inserting, finding, updating, replacing,
* deleting, counting, aggregating, and many writes at once.
*   Each function takes the database -- the handle's current one when NULL
* -- and the collection, then what MongoDB's own drivers take, as Extended
* JSON: a filter ({"status": "open"}; NULL or "{}" for every document), an
* update ({"$set": {"status": "closed"}}, or a pipeline, an array), a
* replacement, a pipeline (an array of stages), and options, as the
* command's own fields -- {"sort": {"n": -1}, "limit": 10, "projection":
* {"_id": 0}} for a find, {"upsert": true} for an update, {"collation": ...},
* {"hint": ...}, {"comment": ...}; NULL for none. The layer adds the session
* of an open transaction, and the handle's time limit for each read
* (mongodb_settings.h) where the options give none. Reads follow the
* handle's read concern and read preference, writes its write concern.
*   Documents found come back as mongodb_query.h's rows -- one column,
* "document", in the handle's document format -- every batch of the cursor
* read; a query to read a page at a time is mongodb_cursor.h's. A write
* reports what the server counted, in struct dj_mongodb_write_counts.
*   RESULTS. 0; the server's code for a write or a command it refused --
* 11000 for a duplicate key, 121 for a document its validator refused --
* which d_mongodb_error_message describes and d_mongodb_error_reply details;
* libbson's, packed, for text that is not the JSON it must be; the driver's
* for a failure of its own; D_DB_NOT_FOUND where a function that hands back
* one document finds none; D_MONGODB_LAYER_NO_MEMORY;
* D_DB_INVALID_ARGUMENT for a NULL handle, collection, or document where one
* is required, JSON of the wrong shape (an array where a document is due,
* or the reverse), or no current database where the database is NULL; and
* D_DB_UNSUPPORTED where the server or the driver cannot.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_document.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Counts
         1.  dj_mongodb_write_counts
    2.  Flags
         1.  D_MONGODB_UNORDERED
2.  DOCUMENTS
    ---------
    1.  Inserting
    2.  Finding
    3.  Updating and replacing
    4.  Deleting
    5.  Counting
    6.  Aggregating
    7.  Finding and modifying
    8.  Many writes at once
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_DOCUMENT_H
#define DJINTERP_C_DB_MONGODB_MONGODB_DOCUMENT_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // D_DB_VENDOR_FLAGS, dj_db_rows
#include "./mongodb_common.h"  // dj_mongodb, the layer's results
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; int64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Counts
//------------------------------------------------------------------------------
// 1.1.1
// dj_mongodb_write_counts
//   struct: what a write did, as the server counted it; a count a write does
// not make is 0.
struct dj_mongodb_write_counts
{
    int64_t inserted;  // documents inserted
    int64_t matched;   // documents an update's filter matched
    int64_t modified;  // documents an update changed: one it matched but
                       // left as it was is not counted
    int64_t deleted;   // documents deleted
    int64_t upserted;  // documents an upsert inserted, none having matched
};

// 1.2    Flags
//------------------------------------------------------------------------------
// 1.2.1
// D_MONGODB_UNORDERED
//   constant: many writes go on past one that fails, and every failure is
// reported in the reply; without it they stop at the first, in order.
#define D_MONGODB_UNORDERED  (D_DB_VENDOR_FLAGS << 0)


//==============================================================================
// 2.  DOCUMENTS
//==============================================================================


// 2.1    Inserting
//------------------------------------------------------------------------------
/**
 * @brief Inserts a document.
 *
 * @note A document without an `_id` is given an ObjectId by the layer,
 *       first among its fields, so that the id can be handed back.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection, created by the insert if it is
 *                          not there.
 * @param[in]  _document    the document, as Extended JSON.
 * @param[out] _id          receives its `_id`, as Extended JSON, released
 *                          with d_db_free; written only on success. May be
 *                          `NULL` when the id is not wanted.
 * @return as the header describes; 11000 for an `_id`, or a unique index's
 *         key, already there.
 * @post On success, when `_id` is not `NULL`, the caller releases `*_id`
 *       with d_db_free; d_mongodb_last_insert_id answers the same id.
 */
int d_mongodb_insert_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _document,
    char**             _id
) D_NOTHROW;
// inserts the documents of a JSON array, in its order, as one command a
// batch at a time; each without an _id gets an ObjectId. Stops at the first
// that fails unless _flags has D_MONGODB_UNORDERED, and writes how many
// went in, on any failure as well as success -- 0 where none did, a
// refusal's too -- where _inserted is not NULL. A failure is the first write
// error's code
int d_mongodb_insert_many(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _documents,
    unsigned           _flags,
    int64_t*           _inserted
) D_NOTHROW;

// 2.2    Finding
//------------------------------------------------------------------------------
/**
 * @brief Finds the documents a filter matches.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection; one that is not there has no
 *                          documents.
 * @param[in]  _filter      the filter; `NULL` for every document.
 * @param[in]  _options     a find's options -- sort, projection, skip,
 *                          limit, batchSize, hint, collation, comment, let,
 *                          allowDiskUse, maxTimeMS -- or `NULL`.
 * @param[out] _rows        receives the documents; written only on success.
 * @return as the header describes.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;
/**
 * @brief Finds the first document a filter matches, in the options' sort.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection.
 * @param[in]  _filter      the filter; `NULL` for every document.
 * @param[in]  _options     a find's options, its limit replaced by 1, or
 *                          `NULL`.
 * @param[out] _rows        receives the document, the one row; written
 *                          only on success.
 * @return as the header describes; D_DB_NOT_FOUND when none matches.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_find_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;
/**
 * @brief Finds the documents a filter matches, as d_mongodb_find does, each
 *        beside its _id as a filter: for writing a document read back by
 *        its _id (core/db's mongo_table does).
 *
 * @note Both columns are canonical Extended JSON, whatever the handle's
 *       document format, so that a document read and written back keeps
 *       each value's type -- an int64 stays an int64, a date a date -- and
 *       the same _id is always the same text.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection; one that is not there has no
 *                          documents.
 * @param[in]  _filter      the filter; `NULL` for every document.
 * @param[in]  _options     a find's options, as d_mongodb_find's, or `NULL`.
 * @param[out] _rows        receives a row a document, of two columns: "_id",
 *                          `{"_id": ...}`, the filter that matches that
 *                          document alone, or NULL for one a projection
 *                          left without an _id; and "document", the
 *                          document. Written only on success.
 * @return as d_mongodb_find.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_find_keyed(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;

// 2.3    Updating and replacing
//------------------------------------------------------------------------------
// updating and replacing -- the first document the filter matches, or every
// one, given the update's operators or a pipeline; or the first, replaced by
// a document of no operators. The options are an update's: upsert,
// arrayFilters, collation, hint, let, comment, bypassDocumentValidation,
// writeConcern. Each writes what the server counted where _counts is not
// NULL, only on success; an upsert's new _id is d_mongodb_last_insert_id's
int d_mongodb_update_one(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _update,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
) D_NOTHROW;
int d_mongodb_update_many(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _update,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
) D_NOTHROW;
int d_mongodb_replace_one(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _filter,
    const char*                     _replacement,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
) D_NOTHROW;

// 2.4    Deleting
//------------------------------------------------------------------------------
// deleting -- the first document the filter matches, or every one; the
// options are a delete's: collation, hint, let, comment, writeConcern. Each
// writes how many went where _deleted is not NULL, only on success
int d_mongodb_delete_one(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _deleted
) D_NOTHROW;
int d_mongodb_delete_many(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _deleted
) D_NOTHROW;

// 2.5    Counting
//------------------------------------------------------------------------------
// counting -- the documents a filter matches, counted exactly by an
// aggregation, the options its skip, limit, hint, collation, comment and
// maxTimeMS; or the collection's count as its metadata keeps it, quickly
// and, after an unclean shutdown or in a sharded cluster, possibly off, the
// options its comment and maxTimeMS. Each writes the count only on success:
// 0 for a collection that is not there. The estimate is the count command,
// which no transaction may hold: inside one it is D_DB_UNSUPPORTED, refused
// before it is sent, which the server would do by ending the transaction
int d_mongodb_count_documents(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    int64_t*           _count
) D_NOTHROW;
int d_mongodb_estimated_document_count(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _options,
    int64_t*           _count
) D_NOTHROW;
/**
 * @brief Finds the distinct values of a field among the documents a filter
 *        matches.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection.
 * @param[in]  _field       the field, dotted for an embedded one ("a.b").
 * @param[in]  _filter      the filter; `NULL` for every document.
 * @param[in]  _options     distinct's options -- collation, comment,
 *                          maxTimeMS -- or `NULL`.
 * @param[out] _rows        receives the values: a row each, of one column,
 *                          "value", typed D_DB_TYPE_JSON, its type_name
 *                          "value", holding the value as Extended JSON in
 *                          the handle's document format -- a value is not a
 *                          document, so canonical where the format is BSON.
 *                          Written only on success.
 * @return as the header describes.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_distinct(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _field,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;

// 2.6    Aggregating
//------------------------------------------------------------------------------
/**
 * @brief Runs an aggregation pipeline and returns the documents it
 *        produces.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection the pipeline reads; `NULL` for a
 *                          pipeline that begins with a stage of the
 *                          database's own ($documents, $currentOp on
 *                          admin, $listLocalSessions).
 * @param[in]  _pipeline    the stages, as a JSON array.
 * @param[in]  _options     aggregate's options -- allowDiskUse, batchSize,
 *                          collation, hint, comment, let, maxTimeMS,
 *                          bypassDocumentValidation -- or `NULL`.
 * @param[out] _rows        receives the documents; written only on success.
 *                          A pipeline that ends in $out or $merge writes
 *                          and returns none.
 * @return as the header describes.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_aggregate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _pipeline,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;

// 2.7    Finding and modifying
//------------------------------------------------------------------------------
// finding and modifying -- in one atomic step, the first document the filter
// matches, in the options' sort, updated, replaced or deleted, and handed
// back as the one row of _rows: as it was before, or, with
// "returnDocument": "after" among the options, as it is after. The options
// are those the drivers' specification gives: sort, projection, upsert,
// returnDocument, collation, hint, arrayFilters (an update's), let, comment,
// maxTimeMS, bypassDocumentValidation, writeConcern. No document is
// D_DB_NOT_FOUND -- none matched, or an upsert inserted one and the options
// asked for it as it was; the write still made. Rows are written only on
// success, and released with d_db_rows_free
int d_mongodb_find_one_and_update(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _update,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;
int d_mongodb_find_one_and_replace(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _replacement,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;
int d_mongodb_find_one_and_delete(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;

// 2.8    Many writes at once
//------------------------------------------------------------------------------
/**
 * @brief Sends many writes to a collection, in as few commands as the
 *        server allows, and reports what they did.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection.
 * @param[in]  _operations  the writes, as a JSON array, each one model as
 *                          the drivers write it: {"insertOne": {"document":
 *                          ...}}, {"updateOne" or "updateMany": {"filter",
 *                          "update", and "upsert", "arrayFilters",
 *                          "collation", "hint"}}, {"replaceOne": {"filter",
 *                          "replacement", "upsert", "collation", "hint"}},
 *                          {"deleteOne" or "deleteMany": {"filter",
 *                          "collation", "hint"}}.
 * @param[in]  _flags       D_MONGODB_UNORDERED to go on past a failure.
 * @param[in]  _options     bypassDocumentValidation, comment, let,
 *                          writeConcern; or `NULL`. The driver takes comment
 *                          and let from its 1.22, and refuses them before,
 *                          with its own code.
 * @param[out] _counts      receives what the writes did, on any failure
 *                          as well as success -- zeros where none were
 *                          made, a refusal's too: what went in before a
 *                          failure is not undone. May be `NULL`.
 * @return as the header describes: the first write error's code, the
 *         others in d_mongodb_error_reply; D_DB_INVALID_ARGUMENT also for a
 *         model of no such name, or an empty array, or "ordered" among the
 *         options, which the flags say; the driver's code for a model's
 *         field it does not take.
 */
int d_mongodb_bulk_write(
    struct dj_mongodb*              _db,
    const char*                     _database,
    const char*                     _collection,
    const char*                     _operations,
    unsigned                        _flags,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
) D_NOTHROW;
/**
 * @brief Sends many writes, to any collections of any databases, as the
 *        server's one bulkWrite command.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _operations  the writes, as a JSON array of the models
 *                          d_mongodb_bulk_write takes, each with a
 *                          "namespace", "db.collection", beside its fields.
 * @param[in]  _flags       D_MONGODB_UNORDERED to go on past a failure.
 * @param[in]  _options     bypassDocumentValidation, comment, let,
 *                          writeConcern; or `NULL`.
 * @param[out] _counts      receives what the writes did, on any failure
 *                          as well as success, as d_mongodb_bulk_write's.
 *                          May be `NULL`.
 * @return as d_mongodb_bulk_write; D_DB_UNSUPPORTED for a server before
 *         8.0 or a driver before 1.28 (D_MONGODB_FEATURE_BULK_WRITE).
 */
int d_mongodb_client_bulk_write(
    struct dj_mongodb*              _db,
    const char*                     _operations,
    unsigned                        _flags,
    const char*                     _options,
    struct dj_mongodb_write_counts* _counts
) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_DOCUMENT_H
