/*******************************************************************************
* djinterp [c]                                                  mongodb_cursor.c
*
* Implementation of mongodb_cursor.h.
*   A cursor is the driver's, opened as d_mongodb_find, d_mongodb_aggregate
* and d_mongodb_query_rows open theirs (mongodb_internal.c), and kept with
* the number of the transaction it was opened in: a failure of its getMore
* ends that transaction, if it is still the open one, and no other. The
* handle links the open transaction's cursors, and its commit or rollback
* destroys their drivers' cursors (d_internal_mongodb_end_cursors): the
* driver sends a cursor's getMore and its killCursors in the session's
* transaction of the moment, and a later transaction's server refuses both
* and fails by them.
*   A change stream is the driver's too, opened outside any transaction,
* which cannot hold one. The driver reports a stream that could not be
* opened only when asked, so the layer asks at once, and returns the
* failure rather than a stream that has none to give. Each read asks the
* driver for events until the page is full or one wait goes by without one:
* the wait is the server's, maxAwaitTimeMS, and a page that filled its
* driver batch and asks for more waits once more at most.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_cursor.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/
// D_INTERNAL_MONGODB_SOURCE
//   macro: this file is one of the MongoDB layer's own sources, for which
// cfg_mongodb_common.h has env include mongoc.h; it precedes every include.
#define D_INTERNAL_MONGODB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_cursor.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // calloc, free
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, dj_db_rows
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "../db_internal.h"       // the rows builder
#include "./mongodb_internal.h"  // mongoc.h, the floor, the plumbing


#if (D_INTERNAL_MONGODB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Wraps the driver's cursor in the layer's, numbered by the
 *        transaction open when it was made, and linked to the handle's
 *        cursors of that transaction.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _native  the driver's cursor, which the layer's owns
 *                         after, or destroys where memory runs out.
 * @param[out]    _cursor  receives the layer's.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_wrap_cursor(
    struct dj_mongodb*         _db,
    mongoc_cursor_t*           _native,
    struct dj_mongodb_cursor** _cursor
)
{
    struct dj_mongodb_cursor* const cursor =
        (struct dj_mongodb_cursor*)calloc(1u, sizeof(*cursor));

    // the layer's cursor
    if (!cursor)
    {
        mongoc_cursor_destroy(_native);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the cursor");
    }

    cursor->db          = _db;
    cursor->cursor      = _native;
    cursor->transaction = ( (_db->session) &&
                            (mongoc_client_session_in_transaction(
                                 _db->session)) )
                              ? _db->transaction
                              : 0ul;
    cursor->done        = false;
    cursor->ended       = false;
    cursor->previous    = NULL;
    cursor->next        = NULL;

    // one of the open transaction's, which ends with it
    if (cursor->transaction != 0ul)
    {
        cursor->next = _db->cursors;

        if (_db->cursors)
        {
            _db->cursors->previous = cursor;
        }

        _db->cursors = cursor;
    }

    *_cursor = cursor;

    return 0;
}

/**
 * @brief Takes a cursor out of its handle's list of the open transaction's
 *        cursors.
 *
 * @param[in,out] _cursor  the cursor, linked.
 */
static
void d_internal_mongodb_unlink_cursor(
    struct dj_mongodb_cursor* _cursor
)
{
    // its neighbours, or the handle, past it
    if (_cursor->previous)
    {
        _cursor->previous->next = _cursor->next;
    }
    else
    {
        _cursor->db->cursors = _cursor->next;
    }

    if (_cursor->next)
    {
        _cursor->next->previous = _cursor->previous;
    }

    _cursor->previous = NULL;
    _cursor->next     = NULL;

    return;
}

/**
 * @brief Checks what an opening is given: a handle, and somewhere for what
 *        it opens.
 *
 * @param[in,out] _db    the handle.
 * @param[in]     _out   where it is written.
 * @param[in]     _what  what it is, for a message.
 * @return 0, or D_DB_INVALID_ARGUMENT.
 */
static
int d_internal_mongodb_check_open(
    struct dj_mongodb* _db,
    const void*        _out,
    const char*        _what
)
{
    // a handle
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (_out) ? 0
                  : d_internal_mongodb_answer(_db,
                                              D_DB_INVALID_ARGUMENT,
                                              "no place was given for the %s",
                                              _what);
}

/**
 * @brief Makes rows of no documents, as a cursor already done reads.
 *
 * @param[out] _rows  receives them.
 * @return `true`, or `false` when memory ran out.
 */
static
bool d_internal_mongodb_no_rows(
    struct dj_db_rows* _rows
)
{
    struct dj_internal_db_rows builder = { { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          0u,
                                          0u };

    // the column, and no row
    if ( (!d_internal_mongodb_rows_column(&builder, false)) ||
         (!d_internal_db_rows_finish(&builder, _rows)) )
    {
        d_internal_db_rows_release(&builder);

        return false;
    }

    return true;
}


//==============================================================================
// 1.  CURSORS
//==============================================================================

// 1.1    Opening
//------------------------------------------------------------------------------
/*
d_mongodb_find_cursor
  d_mongodb_find's query, kept open.
*/
int d_mongodb_find_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _collection,
    const char*                _filter,
    const char*                _options,
    struct dj_mongodb_cursor** _cursor
)
{
    mongoc_cursor_t* native = NULL;
    int              result = d_internal_mongodb_check_open(_db,
                                                            _cursor,
                                                            "cursor");

    // the query, then the layer's cursor over it
    if (result == 0)
    {
        *_cursor = NULL;
        result   = d_internal_mongodb_open_find(_db,
                                                _database,
                                                _collection,
                                                _filter,
                                                _options,
                                                false,
                                                &native);
    }

    return (result == 0)
               ? d_internal_mongodb_wrap_cursor(_db, native, _cursor)
               : result;
}

/*
d_mongodb_aggregate_cursor
  d_mongodb_aggregate's pipeline, kept open.
*/
int d_mongodb_aggregate_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _collection,
    const char*                _pipeline,
    const char*                _options,
    struct dj_mongodb_cursor** _cursor
)
{
    mongoc_cursor_t* native = NULL;
    int              result = d_internal_mongodb_check_open(_db,
                                                            _cursor,
                                                            "cursor");

    // the pipeline, then the layer's cursor over it
    if (result == 0)
    {
        *_cursor = NULL;
        result   = d_internal_mongodb_open_aggregate(_db,
                                                     _database,
                                                     _collection,
                                                     _pipeline,
                                                     _options,
                                                     &native);
    }

    return (result == 0)
               ? d_internal_mongodb_wrap_cursor(_db, native, _cursor)
               : result;
}

/*
d_mongodb_command_cursor
  A command's reply that holds no cursor is refused, its reply recorded as
the failure's, so that it can still be read.
*/
int d_mongodb_command_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _command,
    struct dj_mongodb_cursor** _cursor
)
{
    int result = d_internal_mongodb_check_open(_db, _cursor, "cursor");

    // a handle, and somewhere for the cursor
    if (result != 0)
    {
        return result;
    }

    const char* const name   = d_internal_mongodb_database(_db, _database);
    mongoc_cursor_t*  native = NULL;
    bson_t            command;
    bson_t            reply;

    *_cursor = NULL;
    bson_init(&command);
    bson_init(&reply);
    result = (name) ? d_internal_mongodb_read_document(_db,
                                                       _command,
                                                       true,
                                                       &command)
                    : _db->error;
    result = (result == 0) ? d_internal_mongodb_run_cursor(_db,
                                                           name,
                                                           &command,
                                                           &native,
                                                           &reply)
                           : result;

    // a reply with no cursor
    if ( (result == 0) &&
         (!native) )
    {
        result = d_internal_mongodb_answer(_db,
                                           D_DB_INVALID_ARGUMENT,
                                           "the command's reply holds no "
                                           "cursor");
    }

    bson_destroy(&reply);
    bson_destroy(&command);

    return (result == 0)
               ? d_internal_mongodb_wrap_cursor(_db, native, _cursor)
               : result;
}

// 1.2    Reading
//------------------------------------------------------------------------------
/*
d_mongodb_cursor_fetch
  A failure ends the cursor; one opened in a transaction ends the
transaction too, where it is still the open one.
*/
int d_mongodb_cursor_fetch(
    struct dj_mongodb_cursor* _cursor,
    size_t                    _max,
    struct dj_db_rows*        _rows
)
{
    // a cursor, a page, and somewhere for it
    if ( (!_cursor) ||
         (!_rows)   ||
         (_max == 0u) )
    {
        return (_cursor) ? d_internal_mongodb_answer(_cursor->db,
                                                     D_DB_INVALID_ARGUMENT,
                                                     "a page needs room for "
                                                     "a document, and a "
                                                     "place to go")
                         : D_DB_INVALID_ARGUMENT;
    }

    // ended by its transaction: the server's refusal, as it would answer
    if (_cursor->ended)
    {
        return d_internal_mongodb_answer(_cursor->db,
                                         D_MONGODB_ERROR_TRANSACTION_CURSOR,
                                         "the cursor was opened in "
                                         "transaction %lu, which has ended",
                                         _cursor->transaction);
    }

    // done: no documents, as often as it is read
    if (_cursor->done)
    {
        return (d_internal_mongodb_no_rows(_rows))
                   ? 0
                   : d_internal_mongodb_answer(_cursor->db,
                                               D_MONGODB_LAYER_NO_MEMORY,
                                               "no memory for the rows");
    }

    struct dj_mongodb* const db      = _cursor->db;
    const bool              session = ( (_cursor->transaction != 0ul) &&
                                        (_cursor->transaction ==
                                         db->transaction) );
    bool                    done    = false;
    const int               result  = d_internal_mongodb_cursor_rows(
                                          db,
                                          _cursor->cursor,
                                          _max,
                                          _rows,
                                          &done,
                                          session);

    _cursor->done = ( (done) ||
                      (result != 0) );

    return result;
}

/*
d_mongodb_cursor_is_done
  As the last read left it.
*/
bool d_mongodb_cursor_is_done(
    const struct dj_mongodb_cursor* _cursor
)
{
    return ( (!_cursor) ||
             (_cursor->done) );
}

// 1.3    Closing
//------------------------------------------------------------------------------
/*
d_mongodb_cursor_close
  The driver kills the server's cursor if it is still open; one its
transaction ended has none left, and is unlinked already.
*/
int d_mongodb_cursor_close(
    struct dj_mongodb_cursor* _cursor
)
{
    // nothing to close
    if (!_cursor)
    {
        return 0;
    }

    // still one of the open transaction's, which only such a cursor with
    // its driver's is: out of the handle's list
    if ( (_cursor->transaction != 0ul) &&
         (_cursor->cursor) )
    {
        d_internal_mongodb_unlink_cursor(_cursor);
    }

    mongoc_cursor_destroy(_cursor->cursor);
    free(_cursor);

    return 0;
}


//==============================================================================
// 2.  CHANGE STREAMS
//==============================================================================

// 2.1    Opening
//------------------------------------------------------------------------------
/**
 * @brief Opens the driver's stream of a collection, of a database, or of
 *        the deployment.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database; `NULL` for the deployment.
 * @param[in]     _collection  the collection; `NULL` for the database.
 * @param[in]     _pipeline    the pipeline.
 * @param[in]     _opts        the options.
 * @return the stream, which reports a failure only when asked.
 */
static
mongoc_change_stream_t* d_internal_mongodb_open_stream(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const bson_t*      _pipeline,
    const bson_t*      _opts
)
{
    mongoc_change_stream_t* native = NULL;

    // the stream of the collection, the database or the deployment
    if (_collection)
    {
        mongoc_collection_t* const collection =
            mongoc_client_get_collection(_db->client, _database, _collection);

        native = mongoc_collection_watch(collection, _pipeline, _opts);
        mongoc_collection_destroy(collection);
    }
    else if (_database)
    {
        mongoc_database_t* const database =
            mongoc_client_get_database(_db->client, _database);

        native = mongoc_database_watch(database, _pipeline, _opts);
        mongoc_database_destroy(database);
    }
    else
    {
        native = mongoc_client_watch(_db->client, _pipeline, _opts);
    }

    return native;
}

/**
 * @brief Wraps the driver's stream in the layer's.
 *
 * @param[in,out] _db      the handle.
 * @param[in]     _native  the driver's stream, which the layer's owns
 *                         after, or destroys where memory runs out.
 * @param[out]    _stream  receives the layer's.
 * @return 0, or D_MONGODB_LAYER_NO_MEMORY.
 */
static
int d_internal_mongodb_wrap_stream(
    struct dj_mongodb*                _db,
    mongoc_change_stream_t*           _native,
    struct dj_mongodb_change_stream** _stream
)
{
    struct dj_mongodb_change_stream* const stream =
        (struct dj_mongodb_change_stream*)calloc(1u, sizeof(*stream));

    // the layer's stream around the driver's
    if (!stream)
    {
        mongoc_change_stream_destroy(_native);

        return d_internal_mongodb_answer(_db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the stream");
    }

    stream->db     = _db;
    stream->stream = _native;
    *_stream       = stream;

    return 0;
}

/**
 * @brief Opens a change stream on a collection, a database, or the
 *        deployment.
 *
 * @param[in,out] _db          the handle.
 * @param[in]     _database    the database, or `NULL` for the deployment's
 *                             stream.
 * @param[in]     _collection  the collection, or `NULL` for the database's.
 * @param[in]     _pipeline    the stages, or `NULL`.
 * @param[in]     _options     the options, or `NULL`.
 * @param[out]    _stream      receives the stream.
 * @return as mongodb_cursor.h's section 2.1 describes.
 *
 * The driver's options are its own: those it reads go into the
 * $changeStream stage, the rest into the aggregate command, where the
 * server refuses what does not belong.
 */
static
int d_internal_mongodb_watch(
    struct dj_mongodb*                _db,
    const char*                       _database,
    const char*                       _collection,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
)
{
    bson_t pipeline;
    bson_t opts;
    int    result = d_internal_mongodb_read_array(_db,
                                                  _pipeline,
                                                  false,
                                                  &pipeline);

    bson_init(&opts);
    result = (result == 0) ? d_internal_mongodb_read_document(_db,
                                                              _options,
                                                              false,
                                                              &opts)
                           : result;

    mongoc_change_stream_t* const native =
        (result == 0) ? d_internal_mongodb_open_stream(_db,
                                                       _database,
                                                       _collection,
                                                       &pipeline,
                                                       &opts)
                      : NULL;
    bson_error_t                  error;
    const bson_t*                 reply  = NULL;

    bson_destroy(&opts);
    bson_destroy(&pipeline);

    // a stream that could not be opened says so only when asked
    if ( (native) &&
         (mongoc_change_stream_error_document(native, &error, &reply)) )
    {
        result = d_internal_mongodb_fail_outside(_db, &error, reply);
        mongoc_change_stream_destroy(native);

        return result;
    }

    return (native) ? d_internal_mongodb_wrap_stream(_db, native, _stream)
                    : result;
}

/**
 * @brief Checks a watch's handle, its place for the stream and its
 *        deployment, which a standalone server is not.
 *
 * @param[in,out] _db      the handle.
 * @param[out]    _stream  where the stream goes, written NULL.
 * @return 0, D_DB_UNSUPPORTED, or D_DB_INVALID_ARGUMENT.
 */
static
int d_internal_mongodb_check_watch(
    struct dj_mongodb*                _db,
    struct dj_mongodb_change_stream** _stream
)
{
    const int result = d_internal_mongodb_check_open(_db, _stream, "stream");

    // a handle, and somewhere for the stream
    if (result != 0)
    {
        return result;
    }

    *_stream = NULL;

    return (d_mongodb_supports(_db, D_MONGODB_FEATURE_CHANGE_STREAMS))
               ? 0
               : d_internal_mongodb_answer(_db,
                                           D_DB_UNSUPPORTED,
                                           "a change stream needs a replica "
                                           "set or a sharded cluster");
}

/*
d_mongodb_watch_collection
  One collection's changes.
*/
int d_mongodb_watch_collection(
    struct dj_mongodb*                _db,
    const char*                       _database,
    const char*                       _collection,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
)
{
    int result = d_internal_mongodb_check_watch(_db, _stream);

    // the database, and the collection named
    if (result != 0)
    {
        return result;
    }

    mongoc_collection_t* const collection =
        d_internal_mongodb_collection(_db, _database, _collection);

    if (!collection)
    {
        return _db->error;
    }

    mongoc_collection_destroy(collection);
    result = d_internal_mongodb_watch(
                 _db,
                 d_internal_mongodb_database(_db, _database),
                 _collection,
                 _pipeline,
                 _options,
                 _stream);

    return result;
}

/*
d_mongodb_watch_database
  Every collection's changes in one database.
*/
int d_mongodb_watch_database(
    struct dj_mongodb*                _db,
    const char*                       _database,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
)
{
    const int result = d_internal_mongodb_check_watch(_db, _stream);

    // a database, named or current
    if (result != 0)
    {
        return result;
    }

    const char* const name = d_internal_mongodb_database(_db, _database);

    return (name) ? d_internal_mongodb_watch(_db,
                                             name,
                                             NULL,
                                             _pipeline,
                                             _options,
                                             _stream)
                  : _db->error;
}

/*
d_mongodb_watch_deployment
  Every database's changes.
*/
int d_mongodb_watch_deployment(
    struct dj_mongodb*                _db,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
)
{
    const int result = d_internal_mongodb_check_watch(_db, _stream);

    return (result == 0) ? d_internal_mongodb_watch(_db,
                                                    NULL,
                                                    NULL,
                                                    _pipeline,
                                                    _options,
                                                    _stream)
                         : result;
}

// 2.2    Reading
//------------------------------------------------------------------------------
/*
d_mongodb_change_stream_fetch
  mongoc_change_stream_next returns false both for a wait that went by and
for a failure; the error document tells them apart. A failure the driver
could resume from it has resumed from already.
*/
int d_mongodb_change_stream_fetch(
    struct dj_mongodb_change_stream* _stream,
    size_t                           _max,
    struct dj_db_rows*               _rows
)
{
    // a stream, a page, and somewhere for it
    if ( (!_stream) ||
         (!_rows)   ||
         (_max == 0u) )
    {
        return (_stream) ? d_internal_mongodb_answer(_stream->db,
                                                     D_DB_INVALID_ARGUMENT,
                                                     "a page needs room for "
                                                     "an event, and a place "
                                                     "to go")
                         : D_DB_INVALID_ARGUMENT;
    }

    struct dj_mongodb* const   db      = _stream->db;
    struct dj_internal_db_rows builder = { { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          { NULL, 0u, 0u },
                                          0u,
                                          0u };
    const bson_t*             event   = NULL;
    size_t                    count   = 0u;
    bool                      ok      = d_internal_mongodb_rows_column(
                                            &builder,
                                            false);

    // the events, until the page is full or a wait goes by
    while ( (ok)           &&
            (count < _max) &&
            (mongoc_change_stream_next(_stream->stream, &event)) )
    {
        ok = d_internal_mongodb_rows_document(&builder, event, db->format);
        ++count;
    }

    bson_error_t  error;
    const bson_t* reply = NULL;

    // a failure it could not resume from
    if ( (ok) &&
         (mongoc_change_stream_error_document(_stream->stream,
                                              &error,
                                              &reply)) )
    {
        d_internal_db_rows_release(&builder);

        return d_internal_mongodb_fail_outside(db, &error, reply);
    }

    // the events, packed
    if ( (!ok) ||
         (!d_internal_db_rows_finish(&builder, _rows)) )
    {
        d_internal_db_rows_release(&builder);

        return d_internal_mongodb_answer(db,
                                         D_MONGODB_LAYER_NO_MEMORY,
                                         "no memory for the events");
    }

    return 0;
}

/*
d_mongodb_change_stream_resume_token
  The driver's own: the last event's _id, or the batch's
postBatchResumeToken once its events are read.
*/
int d_mongodb_change_stream_resume_token(
    const struct dj_mongodb_change_stream* _stream,
    char**                                 _token
)
{
    // a stream, and somewhere for the token
    if ( (!_stream) ||
         (!_token) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const bson_t* const token =
        mongoc_change_stream_get_resume_token(_stream->stream);

    // none yet
    if (!token)
    {
        return D_DB_NOT_FOUND;
    }

    char* const text = d_internal_mongodb_json(token,
                                               D_MONGODB_JSON_CANONICAL);

    // room for it
    if (!text)
    {
        return D_MONGODB_LAYER_NO_MEMORY;
    }

    *_token = text;

    return 0;
}

// 2.3    Closing
//------------------------------------------------------------------------------
/*
d_mongodb_change_stream_close
  The driver kills the server's cursor.
*/
int d_mongodb_change_stream_close(
    struct dj_mongodb_change_stream* _stream
)
{
    // nothing to close
    if (!_stream)
    {
        return 0;
    }

    mongoc_change_stream_destroy(_stream->stream);
    free(_stream);

    return 0;
}

#else

//==============================================================================
// WITHOUT THE DRIVER
//==============================================================================
// Built as stubs (D_CFG_MONGODB 0), each function reports D_DB_UNSUPPORTED,
// writing NULL where it promises a cursor or a stream even on failure.

/*
d_mongodb_find_cursor
  Without the driver, D_DB_UNSUPPORTED and no cursor.
*/
int d_mongodb_find_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _collection,
    const char*                _filter,
    const char*                _options,
    struct dj_mongodb_cursor** _cursor
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_filter;
    (void)_options;

    // no cursor, where there is somewhere to say so
    if (_cursor)
    {
        *_cursor = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_aggregate_cursor
  Without the driver, D_DB_UNSUPPORTED and no cursor.
*/
int d_mongodb_aggregate_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _collection,
    const char*                _pipeline,
    const char*                _options,
    struct dj_mongodb_cursor** _cursor
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_pipeline;
    (void)_options;

    // no cursor, where there is somewhere to say so
    if (_cursor)
    {
        *_cursor = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_command_cursor
  Without the driver, D_DB_UNSUPPORTED and no cursor.
*/
int d_mongodb_command_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _command,
    struct dj_mongodb_cursor** _cursor
)
{
    (void)_db;
    (void)_database;
    (void)_command;

    // no cursor, where there is somewhere to say so
    if (_cursor)
    {
        *_cursor = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_cursor_fetch
  Without the driver no cursor exists: D_DB_UNSUPPORTED.
*/
int d_mongodb_cursor_fetch(
    struct dj_mongodb_cursor* _cursor,
    size_t                    _max,
    struct dj_db_rows*        _rows
)
{
    (void)_cursor;
    (void)_max;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_cursor_is_done
  Without the driver, true.
*/
bool d_mongodb_cursor_is_done(
    const struct dj_mongodb_cursor* _cursor
)
{
    (void)_cursor;

    return true;
}

/*
d_mongodb_cursor_close
  Without the driver no cursor exists: 0.
*/
int d_mongodb_cursor_close(
    struct dj_mongodb_cursor* _cursor
)
{
    (void)_cursor;

    return 0;
}

/*
d_mongodb_watch_collection
  Without the driver, D_DB_UNSUPPORTED and no stream.
*/
int d_mongodb_watch_collection(
    struct dj_mongodb*                _db,
    const char*                       _database,
    const char*                       _collection,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
)
{
    (void)_db;
    (void)_database;
    (void)_collection;
    (void)_pipeline;
    (void)_options;

    // no stream, where there is somewhere to say so
    if (_stream)
    {
        *_stream = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_watch_database
  Without the driver, D_DB_UNSUPPORTED and no stream.
*/
int d_mongodb_watch_database(
    struct dj_mongodb*                _db,
    const char*                       _database,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
)
{
    (void)_db;
    (void)_database;
    (void)_pipeline;
    (void)_options;

    // no stream, where there is somewhere to say so
    if (_stream)
    {
        *_stream = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_watch_deployment
  Without the driver, D_DB_UNSUPPORTED and no stream.
*/
int d_mongodb_watch_deployment(
    struct dj_mongodb*                _db,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
)
{
    (void)_db;
    (void)_pipeline;
    (void)_options;

    // no stream, where there is somewhere to say so
    if (_stream)
    {
        *_stream = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_change_stream_fetch
  Without the driver no stream exists: D_DB_UNSUPPORTED.
*/
int d_mongodb_change_stream_fetch(
    struct dj_mongodb_change_stream* _stream,
    size_t                           _max,
    struct dj_db_rows*               _rows
)
{
    (void)_stream;
    (void)_max;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_change_stream_resume_token
  Without the driver, D_DB_UNSUPPORTED.
*/
int d_mongodb_change_stream_resume_token(
    const struct dj_mongodb_change_stream* _stream,
    char**                                 _token
)
{
    (void)_stream;
    (void)_token;

    return D_DB_UNSUPPORTED;
}

/*
d_mongodb_change_stream_close
  Without the driver no stream exists: 0.
*/
int d_mongodb_change_stream_close(
    struct dj_mongodb_change_stream* _stream
)
{
    (void)_stream;

    return 0;
}

#endif  // D_INTERNAL_MONGODB == 1
