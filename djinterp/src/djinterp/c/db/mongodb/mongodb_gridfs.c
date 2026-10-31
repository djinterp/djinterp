/*******************************************************************************
* djinterp [c]                                                  mongodb_gridfs.c
*
* Implementation of mongodb_gridfs.h.
*   Every call is the driver's GridFS bucket, made for the call on the
* database and the bucket's name, and following the client's concerns. The
* bytes in memory go through the bucket's upload and download streams, a
* piece at a time; a local file through the driver's file stream, opened
* for its bytes as they are -- O_BINARY where the C library has one, so that
* Windows translates no line ending -- and a file it makes with the mode the
* C library takes. A download into memory reads the file's length from its
* document first, so that it is allocated once.
*   The driver reports a file of no such id with its own code, which the
* layer returns as D_DB_NOT_FOUND, as mongodb_gridfs.h promises; its
* message stays the driver's.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_gridfs.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.29
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_gridfs.h"  // corresponding header
// std
#include <errno.h>   // errno
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memset
// posix
#include <fcntl.h>     // O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, O_BINARY
#include <sys/stat.h>  // _S_IREAD, _S_IWRITE (Windows')
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, dj_db_rows
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, int32_t


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// BINARY_FLAG
//   constant: the open flag that keeps a C library from translating line
// endings, Windows' O_BINARY; 0 where there is none to keep.
#if defined(O_BINARY)
static const int BINARY_FLAG = O_BINARY;
#else
static const int BINARY_FLAG = 0;
#endif

// FILE_MODE
//   constant: the permissions of a local file a download makes: 0644 where
// they are POSIX's; on Windows, read and write, the only bits its C library
// takes -- the driver opens the file with _sopen_s, which refuses any other
// with EINVAL.
#if defined(_WIN32)
static const int FILE_MODE = _S_IREAD | _S_IWRITE;
#else
static const int FILE_MODE = 0644;
#endif

// PIECE
//   constant: the most bytes one write or read of a stream moves.
static const size_t PIECE = (size_t)1 << 20;

/**
 * @brief Records a GridFS failure: one of no such file as D_DB_NOT_FOUND,
 *        with the driver's words; any other as the driver's.
 *
 * @param[in,out] _db     the handle.
 * @param[in]     _error  the failure.
 * @return the result.
 */
static
int d_internal_mongodb_gridfs_fail(
    struct dj_mongodb*  _db,
    const bson_error_t* _error
)
{
    // no such file
    if ( (_error->domain == (uint32_t)MONGOC_ERROR_GRIDFS) &&
         (_error->code ==
          (uint32_t)MONGOC_ERROR_GRIDFS_BUCKET_FILE_NOT_FOUND) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_DB_NOT_FOUND,
                                         "%s",
                                         _error->message);
    }

    return d_internal_mongodb_fail_outside(_db, _error, NULL);
}

/**
 * @brief Records a local file that could not be opened, with the driver's
 *        code for a GridFS stream that failed.
 *
 * @param[in,out] _db    the handle.
 * @param[in]     _path  the file.
 * @return the result.
 */
static
int d_internal_mongodb_file_fail(
    struct dj_mongodb* _db,
    const char*        _path
)
{
    bson_error_t error;
    const int    number = errno;

    memset(&error, 0, sizeof(error));
    error.domain = MONGOC_ERROR_GRIDFS;
    error.code   = MONGOC_ERROR_GRIDFS_BUCKET_STREAM;
    bson_snprintf(error.message,
                  sizeof(error.message),
                  "the local file %s could not be opened (errno %d)",
                  _path,
                  number);

    return d_internal_mongodb_fail_outside(_db, &error, NULL);
}

/**
 * @brief Makes the driver's bucket for a call.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _database  the database, or `NULL`.
 * @param[in]     _bucket    the bucket's name, or `NULL` for "fs".
 * @param[out]    _out       receives the bucket, the caller's to destroy.
 * @return 0, or the failure.
 */
static
int d_internal_mongodb_bucket(
    struct dj_mongodb*       _db,
    const char*              _database,
    const char*              _bucket,
    mongoc_gridfs_bucket_t** _out
)
{
    const char* const name = d_internal_mongodb_database(_db, _database);
    bson_t            opts;
    bson_error_t      error;

    *_out = NULL;

    // a database, and a bucket's name that is not empty
    if (!name)
    {
        return _db->error;
    }

    if ( (_bucket) &&
         (!*_bucket) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_ERROR_INVALID_NAMESPACE,
                                         "a bucket's name cannot be empty");
    }

    mongoc_database_t* const database =
        mongoc_client_get_database(_db->client, name);

    bson_init(&opts);
    BSON_APPEND_UTF8(&opts, "bucketName", (_bucket) ? _bucket : "fs");
    *_out = mongoc_gridfs_bucket_new(database, &opts, NULL, &error);
    bson_destroy(&opts);
    mongoc_database_destroy(database);

    return (*_out) ? 0 : d_internal_mongodb_fail_outside(_db, &error, NULL);
}

/**
 * @brief Reads an upload's options: the program's metadata.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _metadata  the metadata, JSON, or `NULL`.
 * @param[out]    _opts      initialized, and receives the options.
 * @return 0, or the JSON's failure.
 */
static
int d_internal_mongodb_upload_opts(
    struct dj_mongodb* _db,
    const char*        _metadata,
    bson_t*            _opts
)
{
    bson_t metadata;

    bson_init(_opts);

    // no metadata, no options
    if (!_metadata)
    {
        return 0;
    }

    const int result = d_internal_mongodb_read_document(_db,
                                                        _metadata,
                                                        true,
                                                        &metadata);

    if (result == 0)
    {
        BSON_APPEND_DOCUMENT(_opts, "metadata", &metadata);
    }

    bson_destroy(&metadata);

    return result;
}

/**
 * @brief Hands an uploaded file's id back where it is wanted.
 *
 * @param[in,out] _db  the handle.
 * @param[in]     _id  the id.
 * @param[out]    _out receives it in JSON, or `NULL`.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY, the file uploaded all the same.
 */
static
int d_internal_mongodb_hand_id(
    struct dj_mongodb*  _db,
    const bson_value_t* _id,
    char**              _out
)
{
    // nowhere to hand it
    if (!_out)
    {
        return 0;
    }

    char* const text = d_internal_mongodb_value_json(_id, _db->format);

    // room for it
    if (!text)
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "the file went in; no memory for "
                                         "its id");
    }

    *_out = text;

    return 0;
}

/**
 * @brief Writes bytes to an upload stream, a piece at a time.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _data    the bytes.
 * @param[in]     _size    how many.
 * @return `true`, or `false` when the stream failed.
 */
static
bool d_internal_mongodb_write_all(
    mongoc_stream_t* _stream,
    const void*      _data,
    size_t           _size
)
{
    const char* cursor = (const char*)_data;
    size_t      left   = _size;

    // each piece, as much as the stream takes
    while (left > 0u)
    {
        const size_t  count   = (left < PIECE) ? left : PIECE;
        const ssize_t written = mongoc_stream_write(_stream,
                                                    (void*)cursor,
                                                    count,
                                                    0);

        if (written <= 0)
        {
            return false;
        }

        cursor += written;
        left   -= (size_t)written;
    }

    return true;
}


//==============================================================================
// 1.  GRIDFS
//==============================================================================

// 1.1    Uploading
//------------------------------------------------------------------------------
/**
 * @brief Uploads bytes from memory into a bucket, through the driver's
 *        upload stream, and hands the file's id back.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _bucket    the bucket.
 * @param[in]     _filename  the file's name.
 * @param[in]     _data      the bytes.
 * @param[in]     _size      how many.
 * @param[in]     _opts      the upload's options.
 * @param[out]    _id        receives the id, or `NULL`.
 * @return 0, or the failure, recorded.
 *
 * The close is what finishes the file, its last piece and its document
 * written then.
 */
static
int d_internal_mongodb_upload_bytes(
    struct dj_mongodb*      _db,
    mongoc_gridfs_bucket_t* _bucket,
    const char*             _filename,
    const void*             _data,
    size_t                  _size,
    const bson_t*           _opts,
    char**                  _id
)
{
    bson_error_t           error;
    bson_value_t           id;
    mongoc_stream_t* const stream =
        mongoc_gridfs_bucket_open_upload_stream(_bucket,
                                                _filename,
                                                _opts,
                                                &id,
                                                &error);

    // a stream to write
    if (!stream)
    {
        return d_internal_mongodb_fail_outside(_db, &error, NULL);
    }

    const bool ok     = ( (d_internal_mongodb_write_all(stream,
                                                        _data,
                                                        _size)) &&
                          (mongoc_stream_close(stream) == 0) );
    int        result = (ok)
                            ? 0
                            : (mongoc_gridfs_bucket_stream_error(stream,
                                                                 &error))
                                  ? d_internal_mongodb_fail_outside(_db,
                                                                    &error,
                                                                    NULL)
                                  : d_internal_mongodb_answer(
                                        _db,
                                        D_MONGODB_LAYER_UNDESCRIBED,
                                        "the upload stream failed, and the "
                                        "driver described no failure");

    mongoc_stream_destroy(stream);
    result = (result == 0) ? d_internal_mongodb_hand_id(_db, &id, _id)
                           : result;
    bson_value_destroy(&id);

    return result;
}

/**
 * @brief Uploads a local file into a bucket, read whole by the driver, and
 *        hands the file's id back.
 *
 * @param[in,out] _db        the handle.
 * @param[in]     _bucket    the bucket.
 * @param[in]     _filename  the file's name in the bucket.
 * @param[in]     _path      the local file.
 * @param[in]     _opts      the upload's options.
 * @param[out]    _id        receives the id, or `NULL`.
 * @return 0, or the failure, recorded.
 */
static
int d_internal_mongodb_upload_from(
    struct dj_mongodb*      _db,
    mongoc_gridfs_bucket_t* _bucket,
    const char*             _filename,
    const char*             _path,
    const bson_t*           _opts,
    char**                  _id
)
{
    bson_error_t           error;
    bson_value_t           id;
    mongoc_stream_t* const source =
        mongoc_stream_file_new_for_path(_path, O_RDONLY | BINARY_FLAG, 0);

    // a file to read
    if (!source)
    {
        return d_internal_mongodb_file_fail(_db, _path);
    }

    const bool uploaded = mongoc_gridfs_bucket_upload_from_stream(_bucket,
                                                                  _filename,
                                                                  source,
                                                                  _opts,
                                                                  &id,
                                                                  &error);
    const int  result   = (uploaded)
                              ? d_internal_mongodb_hand_id(_db, &id, _id)
                              : d_internal_mongodb_fail_outside(_db,
                                                                &error,
                                                                NULL);

    // the id the driver made, on success
    if (uploaded)
    {
        bson_value_destroy(&id);
    }

    mongoc_stream_destroy(source);

    return result;
}

/**
 * @brief Reads the first file a bucket's cursor finds, its id in JSON.
 *
 * @param[in,out] _db        the handle.
 * @param[in,out] _cursor    the cursor.
 * @param[in]     _filename  the name it looked for, for a message.
 * @param[out]    _id        receives the id, on success.
 * @return 0; the cursor's failure; D_DB_NOT_FOUND for no file of the
 *         name; D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_first_id(
    struct dj_mongodb* _db,
    mongoc_cursor_t*   _cursor,
    const char*        _filename,
    char**             _id
)
{
    const bson_t* entry = NULL;
    bson_iter_t   field;
    bson_error_t  error;
    const bson_t* reply = NULL;

    // no file, or the cursor's failure
    if ( (!mongoc_cursor_next(_cursor, &entry)) ||
         (!bson_iter_init_find(&field, entry, "_id")) )
    {
        return (mongoc_cursor_error_document(_cursor, &error, &reply))
                   ? d_internal_mongodb_fail_outside(_db, &error, reply)
                   : d_internal_mongodb_answer(_db,
                                               D_DB_NOT_FOUND,
                                               "no file named \"%s\"",
                                               _filename);
    }

    char* const text = d_internal_mongodb_value_json(bson_iter_value(&field),
                                                     _db->format);

    // its _id, written out
    if (!text)
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the id");
    }

    *_id = text;

    return 0;
}

/*
d_mongodb_gridfs_upload
  The bucket's upload stream, which the driver gives an ObjectId; its close
writes the file's document, and is where a failure of the last piece
shows.
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
)
{
    mongoc_gridfs_bucket_t* bucket = NULL;
    bson_t                  opts;

    // a handle, a name, and bytes where there are any
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&opts);

    int result = ( (!_filename) ||
                   ( (!_data) &&
                     (_size != 0u) ) )
                     ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "a file needs its name, "
                                                 "and its bytes")
                     : d_internal_mongodb_upload_opts(_db, _metadata, &opts);

    result = (result == 0)
                 ? d_internal_mongodb_bucket(_db, _database, _bucket, &bucket)
                 : result;

    result = (result == 0) ? d_internal_mongodb_upload_bytes(_db,
                                                             bucket,
                                                             _filename,
                                                             _data,
                                                             _size,
                                                             &opts,
                                                             _id)
                           : result;

    mongoc_gridfs_bucket_destroy(bucket);
    bson_destroy(&opts);

    return result;
}

/*
d_mongodb_gridfs_upload_file
  The driver's file stream as the source of upload_from_stream, which
gives the file an ObjectId.
*/
int d_mongodb_gridfs_upload_file(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filename,
    const char*        _path,
    const char*        _metadata,
    char**             _id
)
{
    mongoc_gridfs_bucket_t* bucket = NULL;
    bson_t                  opts;

    // a handle, a name, and a path
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&opts);

    int result = ( (!_filename) ||
                   (!_path) )
                     ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "a file needs its name, "
                                                 "and the path it is read "
                                                 "from")
                     : d_internal_mongodb_upload_opts(_db, _metadata, &opts);

    result = (result == 0)
                 ? d_internal_mongodb_bucket(_db, _database, _bucket, &bucket)
                 : result;

    result = (result == 0) ? d_internal_mongodb_upload_from(_db,
                                                            bucket,
                                                            _filename,
                                                            _path,
                                                            &opts,
                                                            _id)
                           : result;

    mongoc_gridfs_bucket_destroy(bucket);
    bson_destroy(&opts);

    return result;
}

// 1.2    Downloading
//------------------------------------------------------------------------------
/**
 * @brief Finds a file's length from its document.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _bucket  the bucket.
 * @param[in]     _id      the file's id.
 * @param[out]    _length  receives the length.
 * @return 0; D_DB_NOT_FOUND for no such file; the failure.
 */
static
int d_internal_mongodb_file_length(
    struct dj_mongodb*      _db,
    mongoc_gridfs_bucket_t* _bucket,
    const bson_value_t*     _id,
    int64_t*                _length
)
{
    bson_t        filter;
    const bson_t* entry = NULL;
    bson_iter_t   field;
    bson_error_t  error;
    const bson_t* reply = NULL;

    bson_init(&filter);
    BSON_APPEND_VALUE(&filter, "_id", _id);

    mongoc_cursor_t* const cursor = mongoc_gridfs_bucket_find(_bucket,
                                                              &filter,
                                                              NULL);
    int                    result = 0;

    *_length = 0;

    // the document's length, or the cursor's failure, or no file
    if (mongoc_cursor_next(cursor, &entry))
    {
        *_length = ( (bson_iter_init_find(&field, entry, "length")) &&
                     (BSON_ITER_HOLDS_NUMBER(&field)) )
                       ? bson_iter_as_int64(&field)
                       : 0;
    }
    else
    {
        result = (mongoc_cursor_error_document(cursor, &error, &reply))
                     ? d_internal_mongodb_fail_outside(_db, &error, reply)
                     : d_internal_mongodb_answer(_db,
                                                 D_DB_NOT_FOUND,
                                                 "no file of the id given");
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(&filter);

    return result;
}

/**
 * @brief Reads a download stream into memory.
 *
 * @param[in,out] _db      the handle.
 * @param[in,out] _stream  the stream.
 * @param[in]     _length  the file's length.
 * @param[out]    _data    receives the bytes and a NUL.
 * @param[out]    _size    receives how many bytes.
 * @return 0; D_MONGODB_LAYER_TOO_LARGE; D_MONGODB_LAYER_NO_MEMORY; the
 *         stream's failure.
 */
static
int d_internal_mongodb_read_all(
    struct dj_mongodb* _db,
    mongoc_stream_t*   _stream,
    int64_t            _length,
    void**             _data,
    size_t*            _size
)
{
    bson_error_t error;

    // a length memory can hold, and the room for it
    if ( (_length < 0) ||
         ((uint64_t)_length >= (uint64_t)((size_t)-1)) )
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_TOO_LARGE,
                                         "the file is larger than memory "
                                         "can hold");
    }

    const size_t length = (size_t)_length;
    char* const  bytes  = (char*)malloc(length + 1u);
    size_t       done   = 0u;
    ssize_t      got    = 1;

    if (!bytes)
    {
        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the file");
    }

    // each piece, to the end
    while ( (done < length) &&
            (got > 0) )
    {
        const size_t count = ( (length - done) < PIECE ) ? (length - done)
                                                         : PIECE;

        got   = mongoc_stream_read(_stream, bytes + done, count, count, 0);
        done += (got > 0) ? (size_t)got : 0u;
    }

    // the stream's failure, or a file shorter than its length
    if ( (done < length) ||
         (mongoc_gridfs_bucket_stream_error(_stream, &error)) )
    {
        free(bytes);

        return (mongoc_gridfs_bucket_stream_error(_stream, &error))
                   ? d_internal_mongodb_fail_outside(_db, &error, NULL)
                   : d_internal_mongodb_answer(_db,
                                               D_MONGODB_LAYER_UNDESCRIBED,
                                               "the file ended before its "
                                               "length");
    }

    bytes[length] = '\0';
    *_data        = bytes;
    *_size        = length;

    return 0;
}

/*
d_mongodb_gridfs_download
  The file's length first, then its download stream.
*/
int d_mongodb_gridfs_download(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id,
    void**             _data,
    size_t*            _size
)
{
    mongoc_gridfs_bucket_t* bucket = NULL;
    bson_t                  holder;
    bson_value_t            id;
    bson_error_t            error;
    int64_t                 length = 0;

    // a handle, and somewhere for the bytes
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&holder);

    int result = ( (!_data) ||
                   (!_size) )
                     ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no place was given for the "
                                                 "bytes")
                     : d_internal_mongodb_read_value(_db, _id, &holder, &id);

    result = (result == 0)
                 ? d_internal_mongodb_bucket(_db, _database, _bucket, &bucket)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_file_length(_db, bucket, &id, &length)
                 : result;

    mongoc_stream_t* const stream =
        (result == 0) ? mongoc_gridfs_bucket_open_download_stream(bucket,
                                                                  &id,
                                                                  &error)
                      : NULL;

    // the bytes
    if ( (result == 0) &&
         (!stream) )
    {
        result = d_internal_mongodb_gridfs_fail(_db, &error);
    }
    else if (stream)
    {
        result = d_internal_mongodb_read_all(_db,
                                             stream,
                                             length,
                                             _data,
                                             _size);
        mongoc_stream_destroy(stream);
    }

    mongoc_gridfs_bucket_destroy(bucket);
    bson_destroy(&holder);

    return result;
}

/*
d_mongodb_gridfs_download_file
  The driver's file stream as the destination of download_to_stream.
*/
int d_mongodb_gridfs_download_file(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id,
    const char*        _path
)
{
    mongoc_gridfs_bucket_t* bucket = NULL;
    bson_t                  holder;
    bson_value_t            id;
    bson_error_t            error;

    // a handle, and a path
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&holder);

    int result = (!_path)
                     ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no path was given for the "
                                                 "file")
                     : d_internal_mongodb_read_value(_db, _id, &holder, &id);

    result = (result == 0)
                 ? d_internal_mongodb_bucket(_db, _database, _bucket, &bucket)
                 : result;

    mongoc_stream_t* const destination =
        (result == 0)
            ? mongoc_stream_file_new_for_path(_path,
                                              O_WRONLY | O_CREAT | O_TRUNC |
                                              BINARY_FLAG,
                                              FILE_MODE)
            : NULL;

    // the file, written whole
    if ( (result == 0) &&
         (!destination) )
    {
        result = d_internal_mongodb_file_fail(_db, _path);
    }
    else if (destination)
    {
        result = (mongoc_gridfs_bucket_download_to_stream(bucket,
                                                          &id,
                                                          destination,
                                                          &error))
                     ? 0
                     : d_internal_mongodb_gridfs_fail(_db, &error);
        (void)mongoc_stream_close(destination);
        mongoc_stream_destroy(destination);
    }

    mongoc_gridfs_bucket_destroy(bucket);
    bson_destroy(&holder);

    return result;
}

// 1.3    Deleting
//------------------------------------------------------------------------------
/*
d_mongodb_gridfs_delete
  The driver's delete_by_id: the file's document, then its chunks.
*/
int d_mongodb_gridfs_delete(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id
)
{
    mongoc_gridfs_bucket_t* bucket = NULL;
    bson_t                  holder;
    bson_value_t            id;
    bson_error_t            error;

    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mongodb_read_value(_db, _id, &holder, &id);

    result = (result == 0)
                 ? d_internal_mongodb_bucket(_db, _database, _bucket, &bucket)
                 : result;
    result = (result == 0)
                 ? ( (mongoc_gridfs_bucket_delete_by_id(bucket, &id, &error))
                         ? 0
                         : d_internal_mongodb_gridfs_fail(_db, &error) )
                 : result;

    mongoc_gridfs_bucket_destroy(bucket);
    bson_destroy(&holder);

    return result;
}

// 1.4    Finding
//------------------------------------------------------------------------------
/*
d_mongodb_gridfs_find_id
  The newest of the name: uploadDate descending, _id after it, which an
ObjectId orders by its time too.
*/
int d_mongodb_gridfs_find_id(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filename,
    char**             _id
)
{
    mongoc_gridfs_bucket_t* bucket = NULL;

    // a handle, a name, and somewhere for the id
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = ( (!_filename) ||
                   (!_id) )
                     ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "a file's name and a place "
                                                 "for its id are needed")
                     : d_internal_mongodb_bucket(_db,
                                                 _database,
                                                 _bucket,
                                                 &bucket);

    // a bucket to look in
    if (result != 0)
    {
        return result;
    }

    bson_t filter;
    bson_t opts;
    bson_t sort;

    bson_init(&filter);
    bson_init(&opts);
    BSON_APPEND_UTF8(&filter, "filename", _filename);
    BSON_APPEND_DOCUMENT_BEGIN(&opts, "sort", &sort);
    BSON_APPEND_INT32(&sort, "uploadDate", -1);
    BSON_APPEND_INT32(&sort, "_id", -1);
    bson_append_document_end(&opts, &sort);
    BSON_APPEND_INT64(&opts, "limit", 1);

    mongoc_cursor_t* const cursor = mongoc_gridfs_bucket_find(bucket,
                                                              &filter,
                                                              &opts);

    result = d_internal_mongodb_first_id(_db, cursor, _filename, _id);
    mongoc_cursor_destroy(cursor);
    bson_destroy(&opts);
    bson_destroy(&filter);
    mongoc_gridfs_bucket_destroy(bucket);

    return result;
}

/*
d_mongodb_gridfs_find
  The bucket's find over <bucket>.files, outside any transaction.
*/
int d_mongodb_gridfs_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    mongoc_gridfs_bucket_t* bucket = NULL;
    bson_t                  filter;
    bson_t                  opts;

    // a handle, and somewhere for the rows
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    bson_init(&filter);
    bson_init(&opts);

    int result = (!_rows)
                     ? d_internal_mongodb_answer(_db,
                                                 D_DB_INVALID_ARGUMENT,
                                                 "no place was given for the "
                                                 "rows")
                     : d_internal_mongodb_read_document(_db,
                                                        _filter,
                                                        false,
                                                        &filter);

    result = (result == 0)
                 ? d_internal_mongodb_options(_db,
                                              _options,
                                              D_INTERNAL_MONGODB_READ,
                                              &opts)
                 : result;
    result = (result == 0)
                 ? d_internal_mongodb_bucket(_db, _database, _bucket, &bucket)
                 : result;

    // the files' documents
    if (result == 0)
    {
        mongoc_cursor_t* const cursor = mongoc_gridfs_bucket_find(bucket,
                                                                  &filter,
                                                                  &opts);

        result = d_internal_mongodb_cursor_rows(_db,
                                                cursor,
                                                (size_t)-1,
                                                _rows,
                                                NULL,
                                                false);
        mongoc_cursor_destroy(cursor);
    }

    mongoc_gridfs_bucket_destroy(bucket);
    bson_destroy(&opts);
    bson_destroy(&filter);

    return result;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports D_DB_UNSUPPORTED,
// its answers untouched.

/*
d_mongodb_gridfs_upload
  Without the driver, D_DB_UNSUPPORTED.
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
)
{
    (void)_db;
    (void)_database;
    (void)_bucket;
    (void)_filename;
    (void)_data;
    (void)_size;
    (void)_metadata;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_gridfs_upload_file
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_gridfs_upload_file(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filename,
    const char*        _path,
    const char*        _metadata,
    char**             _id
)
{
    (void)_db;
    (void)_database;
    (void)_bucket;
    (void)_filename;
    (void)_path;
    (void)_metadata;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_gridfs_download
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_gridfs_download(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id,
    void**             _data,
    size_t*            _size
)
{
    (void)_db;
    (void)_database;
    (void)_bucket;
    (void)_id;
    (void)_data;
    (void)_size;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_gridfs_download_file
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_gridfs_download_file(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id,
    const char*        _path
)
{
    (void)_db;
    (void)_database;
    (void)_bucket;
    (void)_id;
    (void)_path;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_gridfs_delete
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_gridfs_delete(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _id
)
{
    (void)_db;
    (void)_database;
    (void)_bucket;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_gridfs_find_id
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_gridfs_find_id(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filename,
    char**             _id
)
{
    (void)_db;
    (void)_database;
    (void)_bucket;
    (void)_filename;
    (void)_id;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_gridfs_find
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_gridfs_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _bucket,
    const char*        _filter,
    const char*        _options,
    struct dj_db_rows* _rows
)
{
    (void)_db;
    (void)_database;
    (void)_bucket;
    (void)_filter;
    (void)_options;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MONGODB == 1
