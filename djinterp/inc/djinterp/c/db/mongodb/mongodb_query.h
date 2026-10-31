/*******************************************************************************
* djinterp [c]                                                   mongodb_query.h
*
* Running a MongoDB command, written as a document, without handling the
* driver's replies by hand.
*   MongoDB has no query language: everything a server does, it does as a
* command, a document whose first field names it -- {"find": "orders",
* "filter": {...}}, {"createIndexes": ...}, {"serverStatus": 1}. Here a
* command is Extended JSON, run on a database -- the handle's current one
* for a NULL database -- as written: the layer adds the session of an open
* transaction (mongodb_transaction.h) and nothing else, and sends it to the
* primary, as the driver sends a command it is given no read preference
* for. d_mongodb_exec runs a command and discards its reply;
* d_mongodb_exec_changes reports how many documents it wrote;
* d_mongodb_command hands the reply back; d_mongodb_query_rows reads the
* documents a command's cursor returns -- a find's, an aggregate's, a
* listCollections' -- every batch, as db_common.h's dj_db_rows, and the reply
* itself as the one row of a command that returns no cursor.
*   Each comes in a plain form, which takes the command as written, and an
* `f` form, which takes a printf-like format: %Q writes a string as a JSON
* string, quoted and escaped (and NULL as null), %I a name the same way, a
* collection's or a field's; %s writes text as it is (NULL as null), and %d,
* %u, %ld, %lld, %f, %g and their kin write numbers, as JSON writes them.
* A name with a qualifier, %T, is refused: a namespace is one string,
* "db.coll", which %Q writes. Use the `f` forms to put values into a
* command safely; the document functions (mongodb_document.h) take filters
* and documents apart, as JSON of their own.
*   ROWS. Every document is a row of one column, "document", typed
* D_DB_TYPE_JSON, its type_name "document", its value TEXT holding the
* document as the handle's document format writes it -- canonical Extended
* JSON unless the program asked for relaxed -- or, with D_MONGODB_BSON, a
* BLOB holding its BSON.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_query.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  COMMANDS
    --------
    1.  Running commands
    2.  Documents written
    3.  Replies
    4.  Rows
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_QUERY_H
#define DJINTERP_C_DB_MONGODB_MONGODB_QUERY_H 1

// djinterp
#include "../../djinterp.h"    // framework root
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
// 1.  COMMANDS
//==============================================================================


// 1.1    Running commands
//------------------------------------------------------------------------------
// running commands -- each returns 0; the server's code for a command that
// failed ("no such command" is 59), which d_mongodb_error_message then
// describes and d_mongodb_error_reply details; libbson's, packed, for text
// that is not a document, and the driver's for a failure of its own;
// D_MONGODB_LAYER_NO_MEMORY when the text could not be built; or
// D_DB_INVALID_ARGUMENT for a NULL handle or command, a format the layer
// cannot read, a value the format cannot write (a NaN, an infinity, a NUL,
// a %T), or no current database where the database is NULL. A reply with
// ok 0 is a failure; a write's errors are its failures too, the first
// reported.
int d_mongodb_exec(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _command
) D_NOTHROW;
int d_mongodb_execf(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _format,
    ...
) D_NOTHROW;

// 1.2    Documents written
//------------------------------------------------------------------------------
// documents written -- as d_mongodb_exec, and on success writes how many
// documents the command wrote, as its reply counts them: an update's
// documents modified with those it upserted, an insert's or a delete's n, a
// findAndModify's one document, and 0 for any other command, which writes
// none, count's n notwithstanding. Written only on success. Every form of
// section 1 counts a write so, for d_mongodb_changes (mongodb_connection.h)
int d_mongodb_exec_changes(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    const char*        _command
) D_NOTHROW;
int d_mongodb_execf_changes(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _changes,
    const char*        _format,
    ...
) D_NOTHROW;

// 1.3    Replies
//------------------------------------------------------------------------------
/**
 * @brief Runs a command and hands back the server's reply.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database to run it on; `NULL` for the
 *                        current one.
 * @param[out] _reply     receives the reply, as Extended JSON in the
 *                        handle's document format (canonical where it is
 *                        BSON), released with d_db_free; written only on
 *                        success.
 * @param[in]  _command   the command, as Extended JSON.
 * @return as d_mongodb_exec; D_MONGODB_LAYER_NO_MEMORY for a reply that
 *         could not be written.
 * @post On success the caller releases `*_reply` with d_db_free.
 */
int d_mongodb_command(
    struct dj_mongodb* _db,
    const char*        _database,
    char**             _reply,
    const char*        _command
) D_NOTHROW;
/**
 * @brief Formats a command, runs it and hands back the reply, as
 *        d_mongodb_command.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database to run it on; `NULL` for the
 *                        current one.
 * @param[out] _reply     receives the reply; written only on success.
 * @param[in]  _format    the command, as a format (above).
 * @param[in]  ...        the format's arguments.
 * @return as d_mongodb_command.
 * @post As for d_mongodb_command.
 */
int d_mongodb_commandf(
    struct dj_mongodb* _db,
    const char*        _database,
    char**             _reply,
    const char*        _format,
    ...
) D_NOTHROW;

// 1.4    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs a command and returns every document its cursor holds, each
 *        batch read; or its reply, as one row, for a command without one.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database to run it on; `NULL` for the
 *                        current one.
 * @param[out] _rows      receives the documents, as the header describes;
 *                        written only on success.
 * @param[in]  _command   the command, as Extended JSON.
 * @return as d_mongodb_exec; the server's code for a batch after the first
 *         that failed; D_MONGODB_LAYER_NO_MEMORY when the rows could not be
 *         built.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int d_mongodb_query_rows(
    struct dj_mongodb* _db,
    const char*        _database,
    struct dj_db_rows* _rows,
    const char*        _command
) D_NOTHROW;
/**
 * @brief Formats a command, runs it and returns its documents, as
 *        d_mongodb_query_rows.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _database  the database to run it on; `NULL` for the
 *                        current one.
 * @param[out] _rows      receives the documents; written only on success.
 * @param[in]  _format    the command, as a format (above).
 * @param[in]  ...        the format's arguments.
 * @return as d_mongodb_query_rows.
 * @post As for d_mongodb_query_rows.
 */
int d_mongodb_query_rowsf(
    struct dj_mongodb* _db,
    const char*        _database,
    struct dj_db_rows* _rows,
    const char*        _format,
    ...
) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_QUERY_H
