/*******************************************************************************
* djinterp [c]                                                  mongodb_cursor.h
*
* Reading a query's documents a page at a time, and watching changes as they
* happen.
*   A cursor holds a query the server has begun -- a find, an aggregation, or
* any command whose reply holds a cursor -- and hands its documents back in
* pages, as rows in the handle's document format: d_mongodb_cursor_fetch
* reads up to the number asked for, each batch the server sends as it is
* reached, and a page shorter than that, none possibly, says the documents
* are done. A cursor does not hold the handle: the handle runs other
* operations while one is open, the cursor asking the server for its next
* batch when it needs one. One opened in a transaction belongs to it, and
* the server reads it in no other: the transaction's commit or rollback ends
* it, killing it on the server, and a read of it after answers
* D_MONGODB_ERROR_TRANSACTION_CURSOR, as the server would outside any
* transaction, without asking the server -- a request for it in a later
* transaction would fail that one. A cursor opened outside any transaction
* reads in or out of one alike.
*   A change stream follows the writes made to a collection, a database or a
* whole deployment -- inserts, updates, replacements, deletes, drops,
* renames -- each an event, a document, which the server sends as it
* happens; a stream needs a replica set or a sharded cluster
* (D_MONGODB_FEATURE_CHANGE_STREAMS). d_mongodb_change_stream_fetch waits
* for an event as long as the options' maxAwaitTimeMS -- the server's own
* wait, a second, without one -- and hands back the events that arrived, none
* when none did. A stream resumes after a failure the driver can resume
* from, and its resume token says where it is, for a stream opened later to
* start after: give it as "resumeAfter" or "startAfter" among the options.
*   Close every cursor and stream before the handle they read through.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_cursor.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CURSORS
    -------
    1.  Opening
    2.  Reading
    3.  Closing
2.  CHANGE STREAMS
    --------------
    1.  Opening
    2.  Reading
    3.  Closing
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_CURSOR_H
#define DJINTERP_C_DB_MONGODB_MONGODB_CURSOR_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // dj_db_rows
#include "./mongodb_common.h"  // dj_mongodb, dj_mongodb_cursor,
                               // dj_mongodb_change_stream
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CURSORS
//==============================================================================


// 1.1    Opening
//------------------------------------------------------------------------------
// opening -- each runs its query and opens a cursor over its documents: a
// find, its filter and options as d_mongodb_find's (mongodb_document.h); an
// aggregation, as d_mongodb_aggregate's; a command whose reply holds a
// cursor, as d_mongodb_query_rows takes one (mongodb_query.h). Each returns 0
// and the cursor, which the caller closes with d_mongodb_cursor_close, or
// what the query's failure is, as there, the cursor NULL;
// D_DB_INVALID_ARGUMENT also for a NULL _cursor, and for a command whose
// reply holds no cursor
int d_mongodb_find_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _collection,
    const char*                _filter,
    const char*                _options,
    struct dj_mongodb_cursor** _cursor
) D_NOTHROW;
int d_mongodb_aggregate_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _collection,
    const char*                _pipeline,
    const char*                _options,
    struct dj_mongodb_cursor** _cursor
) D_NOTHROW;
int d_mongodb_command_cursor(
    struct dj_mongodb*         _db,
    const char*                _database,
    const char*                _command,
    struct dj_mongodb_cursor** _cursor
) D_NOTHROW;

// 1.2    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads the cursor's next documents, up to a page.
 *
 * @param[in,out] _cursor  the cursor.
 * @param[in]     _max     the most to read, at least 1.
 * @param[out]    _rows    receives the documents, as d_mongodb_query_rows
 *                         returns them -- fewer than `_max`, none possibly,
 *                         when the documents are done; written only on
 *                         success.
 * @return 0; the server's code for a batch it could not send, which ends
 *         the cursor, its message on the cursor's handle;
 *         D_MONGODB_LAYER_NO_MEMORY; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument or a `_max` of 0; D_MONGODB_ERROR_TRANSACTION_CURSOR,
 *         as often as it is read, for one whose transaction ended before
 *         its documents did. A cursor already done returns no documents,
 *         as often as it is read.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_cursor_fetch(
    struct dj_mongodb_cursor* _cursor,
    size_t                    _max,
    struct dj_db_rows*        _rows
) D_NOTHROW;
// whether the cursor's documents are done -- the server has sent its last
// batch and every document of it has been read -- or it failed, or its
// transaction ended it; true for a NULL cursor
bool d_mongodb_cursor_is_done(
    const struct dj_mongodb_cursor* _cursor
) D_NOTHROW;

// 1.3    Closing
//------------------------------------------------------------------------------
// closes a cursor, telling the server it is done with it if it is not;
// NULL is accepted. Returns 0
int d_mongodb_cursor_close(
    struct dj_mongodb_cursor* _cursor
) D_NOTHROW;


//==============================================================================
// 2.  CHANGE STREAMS
//==============================================================================


// 2.1    Opening
//------------------------------------------------------------------------------
// opening -- each opens a stream of the changes to a collection, to every
// collection of a database, or to every database of the deployment, as the
// pipeline -- a JSON array of stages that filter or reshape the events, or
// NULL -- leaves them. The options are a stream's: fullDocument
// ("updateLookup", or from 6.0 "whenAvailable" or "required"),
// fullDocumentBeforeChange, resumeAfter, startAfter, startAtOperationTime,
// maxAwaitTimeMS, batchSize, collation, comment, showExpandedEvents -- the
// driver takes comment and fullDocumentBeforeChange from its 1.22, and
// showExpandedEvents later (1.26 does, 1.21 does not), and sends what it
// does not take to the aggregate command, where the server refuses it.
// Opened outside any transaction, which holds none. Each returns 0
// and the stream, which the caller closes with d_mongodb_change_stream_close,
// or the failure, the stream NULL: D_DB_UNSUPPORTED for a standalone server;
// the server's code, as d_mongodb_aggregate's; D_DB_INVALID_ARGUMENT also
// for a NULL _stream
int d_mongodb_watch_collection(
    struct dj_mongodb*                _db,
    const char*                       _database,
    const char*                       _collection,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
) D_NOTHROW;
int d_mongodb_watch_database(
    struct dj_mongodb*                _db,
    const char*                       _database,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
) D_NOTHROW;
int d_mongodb_watch_deployment(
    struct dj_mongodb*                _db,
    const char*                       _pipeline,
    const char*                       _options,
    struct dj_mongodb_change_stream** _stream
) D_NOTHROW;

// 2.2    Reading
//------------------------------------------------------------------------------
/**
 * @brief Reads the events that have arrived, up to a page, waiting for the
 *        first as the stream's maxAwaitTimeMS allows.
 *
 * @note A page the driver's batch does not fill asks the server for more,
 *       which waits once more at most, so a read waits at most twice the
 *       maxAwaitTimeMS.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _max     the most to read, at least 1.
 * @param[out]    _rows    receives the events, documents as
 *                         d_mongodb_query_rows returns them; none when none
 *                         arrived in the wait. Written only on success.
 * @return 0; the server's code, or the driver's, for a failure it could not
 *         resume from, which ends the stream; D_MONGODB_LAYER_NO_MEMORY;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument or a `_max` of 0.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_change_stream_fetch(
    struct dj_mongodb_change_stream* _stream,
    size_t                           _max,
    struct dj_db_rows*               _rows
) D_NOTHROW;
/**
 * @brief Hands back where the stream is: the resume token of the last
 *        event read, or of the latest batch the server sent.
 *
 * @param[in]  _stream  the stream.
 * @param[out] _token   receives the token, a document, as canonical
 *                      Extended JSON, released with d_db_free; written only
 *                      on success.
 * @return 0; D_DB_NOT_FOUND before the stream has one;
 *         D_MONGODB_LAYER_NO_MEMORY; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 * @post On success the caller releases `*_token` with d_db_free.
 */
int d_mongodb_change_stream_resume_token(
    const struct dj_mongodb_change_stream* _stream,
    char**                                 _token
) D_NOTHROW;

// 2.3    Closing
//------------------------------------------------------------------------------
// closes a stream; NULL is accepted. Returns 0
int d_mongodb_change_stream_close(
    struct dj_mongodb_change_stream* _stream
) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_CURSOR_H
