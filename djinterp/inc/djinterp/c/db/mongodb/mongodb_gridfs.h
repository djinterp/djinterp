/*******************************************************************************
* djinterp [c]                                                  mongodb_gridfs.h
*
* Files kept in a database, larger than a document may be: GridFS.
*   A bucket -- "fs" unless named -- keeps each file as a document in
* <bucket>.files, holding its name, its length, its upload date and the
* program's metadata, and its bytes in pieces in <bucket>.chunks. A file is
* known by its id, an ObjectId the layer gives it, handed back as Extended
* JSON; a name may be shared by several files, its revisions, the newest
* last uploaded. The bytes come from memory or from a file of the machine
* the program runs on, and go back the same ways. The driver runs these
* outside any transaction.
*   RESULTS. 0; the server's code or the driver's for a failure; libbson's,
* packed, for JSON that is not; D_DB_NOT_FOUND for a file of no such id, or
* a name with no file; the driver's code, packed, for a local file that
* cannot be opened, read or written; D_MONGODB_LAYER_TOO_LARGE for a file
* larger than memory can hold; D_MONGODB_LAYER_NO_MEMORY;
* D_DB_INVALID_ARGUMENT for a NULL argument where one is required, or no
* current database where the database is NULL.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_gridfs.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  GRIDFS
    ------
    1.  Uploading
    2.  Downloading
    3.  Deleting
    4.  Finding
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_GRIDFS_H
#define DJINTERP_C_DB_MONGODB_MONGODB_GRIDFS_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // dj_db_rows
#include "./mongodb_common.h"  // dj_mongodb, the layer's results
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // INT64_MAX: section 1.4's
                                                 // floor, dj_db_rows's


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  GRIDFS
//==============================================================================


// 1.1    Uploading
//------------------------------------------------------------------------------
/**
 * @brief Uploads bytes from memory as a file of the bucket.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database; `NULL` for the current one.
 * @param[in]  _bucket    the bucket's name; `NULL` for "fs".
 * @param[in]  _filename  the file's name.
 * @param[in]  _data      the bytes; may be `NULL` when `_size` is 0.
 * @param[in]  _size      how many.
 * @param[in]  _metadata  the program's description of the file, a JSON
 *                        document kept with it, or `NULL`.
 * @param[out] _id        receives the file's id, as Extended JSON, released
 *                        with d_db_free; written only on success. May be
 *                        `NULL` when the id is not wanted.
 * @return as the header describes.
 * @post On success, when `_id` is not `NULL`, the caller releases `*_id`
 *       with d_db_free.
 */
int d_mongodb_gridfs_upload(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filename,
    const void*        _data,
    size_t             _size,
    const char*        _metadata,
    char**             _id
) D_NOTHROW;
// uploads a local file's bytes, as d_mongodb_gridfs_upload, the file at
// _path read whole
int d_mongodb_gridfs_upload_file(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filename,
    const char*        _path,
    const char*        _metadata,
    char**             _id
) D_NOTHROW;

// 1.2    Downloading
//------------------------------------------------------------------------------
/**
 * @brief Downloads a file's bytes into memory.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database; `NULL` for the current one.
 * @param[in]  _bucket    the bucket's name; `NULL` for "fs".
 * @param[in]  _id        the file's id, as Extended JSON.
 * @param[out] _data      receives the bytes, followed by a NUL that `_size`
 *                        does not count, released with d_db_free; never
 *                        `NULL` on success, even for an empty file. Written
 *                        only on success.
 * @param[out] _size      receives how many; written only on success.
 * @return as the header describes.
 * @post On success the caller releases `*_data` with d_db_free.
 */
int d_mongodb_gridfs_download(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id,
    void**             _data,
    size_t*            _size
) D_NOTHROW;
// downloads a file's bytes into a local file at _path, created or replaced;
// a failure part-way leaves what was written
int d_mongodb_gridfs_download_file(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id,
    const char*        _path
) D_NOTHROW;

// 1.3    Deleting
//------------------------------------------------------------------------------
// deleting -- deletes a file, its document and its chunks
int d_mongodb_gridfs_delete(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id
) D_NOTHROW;

// 1.4    Finding
//------------------------------------------------------------------------------
/**
 * @brief Finds the id of a name's newest revision: the file of that name
 *        uploaded last.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database; `NULL` for the current one.
 * @param[in]  _bucket    the bucket's name; `NULL` for "fs".
 * @param[in]  _filename  the name.
 * @param[out] _id        receives the id, as Extended JSON, released with
 *                        d_db_free; written only on success.
 * @return as the header describes.
 * @post On success the caller releases `*_id` with d_db_free.
 */
int d_mongodb_gridfs_find_id(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filename,
    char**             _id
) D_NOTHROW;

#if defined(INT64_MAX)
/**
 * @brief Finds the files a filter matches, as the documents of
 *        <bucket>.files.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database; `NULL` for the current one.
 * @param[in]  _bucket    the bucket's name; `NULL` for "fs".
 * @param[in]  _filter    a filter over the files' documents ({"filename":
 *                        "a.png"}, {"metadata.kind": "image"}); `NULL` for
 *                        every file.
 * @param[in]  _options   a find's options, or `NULL`.
 * @param[out] _rows      receives the documents (mongodb_query.h); written
 *                        only on success.
 * @return as the header describes.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_gridfs_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
) D_NOTHROW;
#endif  // defined(INT64_MAX)


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_GRIDFS_H
