/*******************************************************************************
* djinterp [c]                                                mongodb_internal.h
*
* Private to the MongoDB layer's sources: the driver's headers, the handle's
* and the cursors' layouts, and the plumbing every module shares.
*   Only a source that defines D_INTERNAL_MONGODB_SOURCE before its first
* include reaches this header's driver half: cfg_mongodb_common.h then has
* env include mongoc.h. A build without the driver (D_CFG_MONGODB 0) sees
* nothing here but the stubs' needs, which are none.
*   RESULTS. A failure is recorded on the handle as it is returned -- its
* result, its message, the server's name for its code, its reply -- and an
* operation in an open transaction that fails marks the transaction failed.
* A success records nothing.
*   JSON. A document or an array the caller gives is read whole: libbson's
* reader is asked twice, so that text after the first document, which
* bson_init_from_json passes over, is refused, and the first character says
* which of the two it is, which libbson does not, reading an array as a
* document of "0", "1" and on.
*   MEMORY. What the layer hands back is malloc's, released by d_db_free;
* what libbson writes -- JSON text -- is copied there and bson_free'd.
*
*
* path:      /src/djinterp/c/db/mongodb/mongodb_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE DRIVER
    ----------
    1.  Floors
2.  TYPES
    -----
    1.  Sizes
         1.  D_INTERNAL_MONGODB_MESSAGE_SIZE
         2.  D_INTERNAL_MONGODB_NAME_SIZE
         3.  D_INTERNAL_MONGODB_VERSION_SIZE
    2.  Handles
         1.  dj_mongodb
         2.  dj_mongodb_cursor
         3.  dj_mongodb_change_stream
    3.  Options
         1.  D_INTERNAL_MONGODB_READ
         2.  D_INTERNAL_MONGODB_SESSION
    4.  Collection kinds
         1.  dj_internal_mongodb_kind
3.  RESULTS
    -------
    1.  Packing
    2.  Recording
4.  JSON
    ----
    1.  Reading
    2.  Writing
    3.  Formats
5.  OPERATIONS
    ----------
    1.  Names
    2.  Options
    3.  Commands
    4.  Rows
    5.  The catalog
    6.  Writes
    7.  The log
    8.  Collections
    9.  Concerns
    10. Cursors
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_INTERNAL_H
#define DJINTERP_C_DB_MONGODB_MONGODB_INTERNAL_H 1

// std
#include <stdarg.h>  // va_list
#include <stddef.h>  // size_t
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/db/db_common.h"  // the vocabulary
#include "../../../../../inc/djinterp/config/c/db/mongodb/cfg_mongodb_common.h"  // D_INTERNAL_MONGODB
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_common.h"  // results
#include "../../../../../inc/djinterp/c/db/mongodb/mongodb_document.h"  // counts
#include "../db_internal.h"  // builders, the formatter
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, int32_t,
                                                         // uint32_t

#if (D_INTERNAL_MONGODB == 1)

// djinterp
#include "../../../../../inc/djinterp/env/db/mongodb/env_mongodb.h"  // mongoc.h


//==============================================================================
// 1.  THE DRIVER
//==============================================================================


// 1.1    Floors
//------------------------------------------------------------------------------
// floors -- the driver's header must be in scope, which cfg_mongodb_common.h
// arranges for a source that says it is one of the layer's, and must be the
// floor's or later; and the build must spell a 64-bit integer, as every C99
// one does, since the layer's sources are C
#if !defined(D_INTERNAL_MONGODB_SOURCE)
    #error "mongodb_internal.h is the layer's own: a layer source defines "    \
           "D_INTERNAL_MONGODB_SOURCE before its first include"
#endif

#if !defined(INT64_MAX)
    #error "the MongoDB layer's sources need int64_t, which its handle and "  \
           "its counts hold"
#endif

#if !D_ENV_MONGODB_HEADER_INCLUDED
    #error "the MongoDB layer is built over the driver, but mongoc.h is not "  \
           "in scope: D_CFG_ENV_USING_MONGODB was turned off, or dconfig.h "  \
           "was included before the layer's first header"
#endif

#if ( (!defined(MONGOC_MAJOR_VERSION)) ||                                      \
      (D_ENV_MONGO_ENCODE_VERSION(MONGOC_MAJOR_VERSION,                        \
                                  MONGOC_MINOR_VERSION,                        \
                                  MONGOC_MICRO_VERSION) <                      \
       D_MONGODB_FLOOR_DRIVER_VERSION) )
    #error "the MongoDB layer needs MongoDB's C driver 1.21 or later"
#endif


//==============================================================================
// 2.  TYPES
//==============================================================================


// 2.1    Sizes
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_MONGODB_MESSAGE_SIZE
//   constant: the bytes a handle keeps of its last failure's message, its
// terminator among them; the driver's own are 504 at most (503 from 2.0).
#define D_INTERNAL_MONGODB_MESSAGE_SIZE 512

// 2.1.2
// D_INTERNAL_MONGODB_NAME_SIZE
//   constant: the bytes a handle keeps of the server's name for a code; the
// longest the server has is under 48.
#define D_INTERNAL_MONGODB_NAME_SIZE 64

// 2.1.3
// D_INTERNAL_MONGODB_VERSION_SIZE
//   constant: the bytes a handle keeps of the server's version string.
#define D_INTERNAL_MONGODB_VERSION_SIZE 48

// 2.2    Handles
//------------------------------------------------------------------------------
// 2.2.1
// dj_mongodb
//   struct: the handle, which mongodb_common.h declares.
struct dj_mongodb
{
    mongoc_client_t*               client;       // the driver's client
    bool                           owned;        // destroyed with the handle
    char*                          database;     // the current database,
                                                 // malloc's; NULL: none
    mongoc_client_session_t*       session;      // the transactions' session,
                                                 // NULL until the first
    unsigned long                  transaction;  // the latest transaction's
                                                 // number, from 1; 0: none
    bool                           failed;       // an operation of the open
                                                 // transaction failed
    enum dj_mongodb_topology        topology;     // what the handle reached
    char*                          replica_set;  // the set's name, malloc's;
                                                 // NULL: none
    unsigned long                  version;      // the server's, MAJOR*10000
                                                 // + MINOR*100 + PATCH
    char                           release[D_INTERNAL_MONGODB_VERSION_SIZE];
                                                 // the server's version, as
                                                 // written
    int                            wire;         // its maxWireVersion
    enum dj_mongodb_document_format format;       // how documents are written
    int64_t                        max_time_ms;  // a read's maxTimeMS; 0:
                                                 // none
    int64_t                        changes;      // the latest write's count
    bool                           has_id;       // `id` holds a value
    bson_value_t                   id;           // the latest insert's or
                                                 // upsert's _id, a copy
    int                            error;        // the last failure's result
    char                           message[D_INTERNAL_MONGODB_MESSAGE_SIZE];
                                                 // its message; "" for none
    char                           error_name[D_INTERNAL_MONGODB_NAME_SIZE];
                                                 // the server's name for its
                                                 // code; "" for none
    bson_t*                        reply;        // its reply; NULL for none
    struct dj_mongodb_cursor*       cursors;      // the open transaction's
                                                 // cursors, which end with
                                                 // it; NULL: none
};

// 2.2.2
// dj_mongodb_cursor
//   struct: a cursor, which mongodb_common.h declares.
struct dj_mongodb_cursor
{
    struct dj_mongodb*        db;           // the handle it reads through
    mongoc_cursor_t*         cursor;       // the driver's; NULL once its
                                           // transaction ended it
    unsigned long            transaction;  // the transaction it was opened
                                           // in, by its number; 0: none
    bool                     done;         // its documents are done, or it
                                           // failed
    bool                     ended;        // its transaction ended before
                                           // its documents did
    struct dj_mongodb_cursor* previous;     // the transaction's cursors,
    struct dj_mongodb_cursor* next;         // linked from the handle's
};

// 2.2.3
// dj_mongodb_change_stream
//   struct: a change stream, which mongodb_common.h declares.
struct dj_mongodb_change_stream
{
    struct dj_mongodb*       db;      // the handle it reads through
    mongoc_change_stream_t* stream;  // the driver's
};

// 2.3    Options
//------------------------------------------------------------------------------
// 2.3.1
// D_INTERNAL_MONGODB_READ
//   constant: options for a read, to which the handle's time limit is added
// where they name none.
#define D_INTERNAL_MONGODB_READ     0x0001u

// 2.3.2
// D_INTERNAL_MONGODB_SESSION
//   constant: options to which the open transaction's session is added.
#define D_INTERNAL_MONGODB_SESSION  0x0002u

// 2.4    Collection kinds
//------------------------------------------------------------------------------
// 2.4.1
// dj_internal_mongodb_kind
//   enum: what listCollections says a name is.
enum dj_internal_mongodb_kind
{
    D_INTERNAL_MONGODB_NOTHING = 0,  // nothing of the name
    D_INTERNAL_MONGODB_COLLECTION,   // a collection, time series among them
    D_INTERNAL_MONGODB_VIEW          // a view
};


//==============================================================================
// 3.  RESULTS
//==============================================================================


// 3.1    Packing
//------------------------------------------------------------------------------
// packing -- the result a driver's or libbson's failure is returned as: the
// server's code where its domain is the server's or a write concern's, and
// otherwise packed above every server code, as mongodb_common.h describes,
// libbson's domains from D_MONGODB_DOMAIN_BSON. A code past 16 bits in a
// client domain keeps its low 16, its whole value in the message.
int d_internal_mongodb_pack(
    const bson_error_t* _error,
    bool                _from_bson
);

// 3.2    Recording
//------------------------------------------------------------------------------
// recording -- each records a failure on the handle and returns its result:
// _fail a driver's failure of an operation that carried the open
// transaction's session, its reply (may be NULL) kept for its labels and
// details; _fail_outside one that ran outside it; _fail_bson libbson's;
// _answer a result of the layer's own, or a server code it answers itself,
// with its message, printf-formatted. A failure the server's transaction does
// not survive -- any the server reports, and any labelled
// TransientTransactionError -- marks an open transaction failed when _fail
// records it, and only then: the server never saw the others. A NULL handle
// records nothing; each still returns the result.
int d_internal_mongodb_fail(
    struct dj_mongodb*  _db,
    const bson_error_t* _error,
    const bson_t*       _reply
);
int d_internal_mongodb_fail_outside(
    struct dj_mongodb*  _db,
    const bson_error_t* _error,
    const bson_t*       _reply
);
int d_internal_mongodb_fail_bson(
    struct dj_mongodb*  _db,
    const bson_error_t* _error
);
int d_internal_mongodb_answer(
    struct dj_mongodb* _db,
    int                _result,
    const char*        _format,
    ...
);
// the reply's first write error or write concern error, where a command or
// a write that carried the open transaction's session returned true; 0
// where there is none, or the result recorded
int d_internal_mongodb_reply_failure(
    struct dj_mongodb* _db,
    const bson_t*      _reply
);


//==============================================================================
// 4.  JSON
//==============================================================================


// 4.1    Reading
//------------------------------------------------------------------------------
// reading -- each initializes _out, which the caller destroys whatever the
// result, and reads into it the JSON given: one document, or one array, and
// nothing after it. NULL is an empty document or array unless _required, when
// it is D_DB_INVALID_ARGUMENT, as text that is the other of the two is.
// libbson's failure is recorded and returned, packed
int d_internal_mongodb_read_document(
    struct dj_mongodb* _db,
    const char*        _json,
    bool               _required,
    bson_t*            _out
);
int d_internal_mongodb_read_array(
    struct dj_mongodb* _db,
    const char*        _json,
    bool               _required,
    bson_t*            _out
);
// a document or an array, whichever the text is -- an update, which may be
// either -- `_array` saying which; NULL is D_DB_INVALID_ARGUMENT
int d_internal_mongodb_read_either(
    struct dj_mongodb* _db,
    const char*        _json,
    bson_t*            _out,
    bool*              _array
);
// a single value as Extended JSON -- an id, say -- read into _holder, which
// the caller destroys whatever the result, and found in it; _value is valid
// while _holder is
int d_internal_mongodb_read_value(
    struct dj_mongodb* _db,
    const char*        _json,
    bson_t*            _holder,
    bson_value_t*      _value
);

// 4.2    Writing
//------------------------------------------------------------------------------
// writing -- a document or a value as Extended JSON in the format given,
// canonical for D_MONGODB_BSON, copied into malloc's memory for d_db_free;
// NULL when memory runs out
char* d_internal_mongodb_json(
    const bson_t*                   _document,
    enum dj_mongodb_document_format _format
);
char* d_internal_mongodb_value_json(
    const bson_value_t*             _value,
    enum dj_mongodb_document_format _format
);
// a copy of a string, malloc's, for d_db_free; NULL for NULL, or no memory
char* d_internal_mongodb_strdup(
    const char* _text
);

// 4.3    Formats
//------------------------------------------------------------------------------
/**
 * @brief Formats a command in JSON's dialect (mongodb_query.h), its strings
 *        quoted and escaped as JSON.
 *
 * @param[in]  _db      the handle, on which a failure is recorded.
 * @param[out] _text    receives the command, malloc's; written only on
 *                      success.
 * @param[in]  _format  the format.
 * @param[in]  _args    its arguments.
 * @return 0; D_MONGODB_LAYER_NO_MEMORY; D_DB_INVALID_ARGUMENT for a format
 *         the layer cannot read, or a value it cannot write: a NaN, an
 *         infinity, a NUL, text that is not UTF-8, a %T.
 */
int d_internal_mongodb_vformat(
    struct dj_mongodb* _db,
    char**             _text,
    const char*        _format,
    va_list            _args
);


//==============================================================================
// 5.  OPERATIONS
//==============================================================================


// 5.1    Names
//------------------------------------------------------------------------------
// names -- the database a function acts in: the one given, or the handle's
// current one for NULL; NULL, D_DB_INVALID_ARGUMENT recorded, where there
// is neither
const char* d_internal_mongodb_database(
    struct dj_mongodb* _db,
    const char*        _database
);

// 5.2    Options
//------------------------------------------------------------------------------
// options -- initializes _opts, which the caller destroys whatever the
// result, from the options given as JSON, adding by _what (D_INTERNAL_
// MONGODB_*) the handle's time limit, where they name no maxTimeMS, and the
// open transaction's session
int d_internal_mongodb_options(
    struct dj_mongodb* _db,
    const char*        _json,
    unsigned           _what,
    bson_t*            _opts
);
// the open transaction's session added to _opts, which is initialized; 0, or
// the driver's failure, recorded
int d_internal_mongodb_session(
    struct dj_mongodb* _db,
    bson_t*            _opts
);

// 5.3    Commands
//------------------------------------------------------------------------------
// commands -- runs a command on a database as written, with the open
// transaction's session unless _outside, on the primary; _reply, which the
// caller destroys whatever the result, receives the reply. A reply that
// reports a write's failure is a failure. 0, or the result recorded
int d_internal_mongodb_run(
    struct dj_mongodb* _db,
    const char*        _database,
    const bson_t*      _command,
    bool               _outside,
    bson_t*            _reply
);

// 5.4    Rows
//------------------------------------------------------------------------------
// rows -- a builder's one column for documents, "document", or for values,
// "value"; a document added as a row in the format given; a value as one
bool d_internal_mongodb_rows_column(
    struct dj_internal_db_rows* _rows,
    bool                        _values
);
bool d_internal_mongodb_rows_document(
    struct dj_internal_db_rows*     _rows,
    const bson_t*                   _document,
    enum dj_mongodb_document_format _format
);
bool d_internal_mongodb_rows_value(
    struct dj_internal_db_rows*     _rows,
    const bson_value_t*             _value,
    enum dj_mongodb_document_format _format
);
/**
 * @brief Reads a cursor's next documents into rows, up to `_max`.
 *
 * @param[in]  _db      the handle, on which a failure is recorded.
 * @param[in]  _cursor  the driver's cursor.
 * @param[in]  _max     the most to read; (size_t)-1 for all of them.
 * @param[out] _rows    receives the documents; written only on success.
 * @param[out] _done    receives whether the cursor is done: it ran out, or
 *                      failed. May be `NULL`.
 * @param[in]  _session whether the cursor's getMores carry the open
 *                      transaction's session, so that a failure ends it.
 * @return 0; the cursor's failure, recorded; D_MONGODB_LAYER_NO_MEMORY.
 */
int d_internal_mongodb_cursor_rows(
    struct dj_mongodb* _db,
    mongoc_cursor_t*   _cursor,
    size_t             _max,
    struct dj_db_rows* _rows,
    bool*              _done,
    bool               _session
);

// 5.5    The catalog
//------------------------------------------------------------------------------
// the catalog -- what a name of a database is, asked of listCollections
// outside any transaction, which the server keeps it from; 0, or the
// failure, recorded
int d_internal_mongodb_kind_of(
    struct dj_mongodb*             _db,
    const char*                    _database,
    const char*                    _name,
    enum dj_internal_mongodb_kind* _kind
);
// the names a listCollections of a kind lists, into _names; 0, or the
// failure, recorded
int d_internal_mongodb_list_kind(
    struct dj_mongodb*            _db,
    const char*                   _database,
    enum dj_internal_mongodb_kind _kind,
    struct dj_db_name_list*       _names
);

// 5.6    Writes
//------------------------------------------------------------------------------
// writes -- the counts a write's reply holds, by the names insert, update,
// delete, a bulk write's and findAndModify's replies give them; the latest
// write's count recorded on the handle; and its new _id, recorded, or none
void d_internal_mongodb_counts(
    const bson_t*                   _reply,
    struct dj_mongodb_write_counts* _counts
);
void d_internal_mongodb_wrote(
    struct dj_mongodb*                    _db,
    const struct dj_mongodb_write_counts* _counts
);
void d_internal_mongodb_new_id(
    struct dj_mongodb*  _db,
    const bson_value_t* _id
);

// 5.7    The log
//------------------------------------------------------------------------------
// the log -- _start initializes the driver, once for the process, and the
// first time any handle is made or wrapped installs the layer's handler for
// the driver's log at the level D_CFG_MONGODB_LOG_LEVEL names, unless the
// knob is -1 or _log_set ran first; _log_set installs it at the level given,
// -1 passing nothing on, and returns `false`, installing nothing, where the
// lock it takes could not be made; _log_get answers the level installed, or
// the knob's while none is. Each is safe from any thread
void d_internal_mongodb_start(void);
bool d_internal_mongodb_log_set(
    int _level
);
int d_internal_mongodb_log_get(void);

// 5.8    Collections
//------------------------------------------------------------------------------
// collections -- the driver's handle on a collection of a database, the
// database NULL for the handle's current one; NULL, the refusal recorded,
// for no database, no collection's name (D_DB_INVALID_ARGUMENT), or an
// empty one (the server's 73, InvalidNamespace). The caller destroys it
// with mongoc_collection_destroy
mongoc_collection_t* d_internal_mongodb_collection(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _name
);

// 5.9    Concerns
//------------------------------------------------------------------------------
// concerns -- a write concern, a read concern and a read preference read
// from the document a command writes each as; each refuses, as
// D_DB_INVALID_ARGUMENT recorded, a field of no such name, a value of the
// wrong type, a level, a mode or a w of no such name, a w of 0 -- the layer
// reports what every write did, which needs it acknowledged -- a mode of
// primary with tags or a staleness, and a staleness under 90 seconds. An
// empty write or read concern is the deployment's default, *_out NULL; a read
// preference must name its mode. The caller destroys *_out
int d_internal_mongodb_write_concern(
    struct dj_mongodb*       _db,
    const bson_t*            _document,
    mongoc_write_concern_t** _out
);
int d_internal_mongodb_read_concern(
    struct dj_mongodb*      _db,
    const bson_t*           _document,
    mongoc_read_concern_t** _out
);
int d_internal_mongodb_read_prefs(
    struct dj_mongodb*    _db,
    const bson_t*         _document,
    mongoc_read_prefs_t** _out
);
// a document's field that holds a document, as a document that reads its
// bytes in place, valid while the first is; `false` where it has none
bool d_internal_mongodb_subdocument(
    const bson_t* _document,
    const char*   _field,
    bson_t*       _out
);
// a field that holds a document, likewise; `false` for one that does not
bool d_internal_mongodb_field_document(
    const bson_iter_t* _field,
    bson_t*            _out
);

// 5.10   Cursors
//------------------------------------------------------------------------------
// cursors -- each opens the driver's cursor over a query, the caller's to
// destroy: a find, its filter and options as d_mongodb_find reads them, the
// open transaction's session and the handle's time limit added, `_one`
// limiting it to its first document in one batch; an aggregation, on a
// collection or, for a NULL one, on the database. Each returns 0 and the
// cursor, or the failure, recorded, and NULL
int d_internal_mongodb_open_find(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _filter,
    const char*        _options,
    bool               _one,
    mongoc_cursor_t**  _cursor
);
int d_internal_mongodb_open_aggregate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    const char*        _pipeline,
    const char*        _options,
    mongoc_cursor_t**  _cursor
);
/**
 * @brief Runs a command where its cursor's getMores can follow it, and
 *        opens the cursor its reply holds.
 *
 * @param[in,out] _db       the handle.
 * @param[in]     _name     the database.
 * @param[in]     _command  the command.
 * @param[out]    _cursor   receives the cursor, the caller's to destroy, or
 *                          `NULL` for a reply that holds none.
 * @param[out]    _reply    initialized, the caller's to destroy; receives
 *                          the reply where it holds no cursor, and is left
 *                          empty where the cursor took it.
 * @return 0, or the failure, recorded.
 *
 * Outside a transaction the server is selected first, for writes, as the
 * command would be, and the command and the cursor go to it by its id;
 * inside one, the session pins them.
 */
int d_internal_mongodb_run_cursor(
    struct dj_mongodb* _db,
    const char*        _name,
    const bson_t*      _command,
    mongoc_cursor_t**  _cursor,
    bson_t*            _reply
);
// ending -- ends the cursors of a transaction that has just ended, as a
// commit or a rollback leaves it: each driver's cursor is destroyed, which
// asks the server to kill it outside any transaction, before a later one
// could carry the request and fail by it; the layer's cursors stay, ended,
// for their callers to close
void d_internal_mongodb_end_cursors(
    struct dj_mongodb* _db
);


#endif  // D_INTERNAL_MONGODB == 1


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_INTERNAL_H
