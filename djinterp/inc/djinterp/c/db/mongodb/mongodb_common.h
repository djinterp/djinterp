/*******************************************************************************
* djinterp [c]                                                  mongodb_common.h
*
* Shared foundation of djinterp's MongoDB convenience layer.
*   The layer works over MongoDB's C driver, libmongoc with libbson, through
* a handle of its own, struct dj_mongodb, around the driver's mongoc_client_t:
* it keeps what the driver does not -- the current database, the session a
* transaction runs in, the server's version and deployment, the last
* failure's whole description, and how the layer writes documents. The
* public headers name only the layer's types, so they never include the
* driver's; d_mongodb_native hands back the mongoc_client_t* for the
* driver's own calls, and d_mongodb_adopt wraps one a program set up itself.
* Every module includes this header:
*     mongodb_connection   connect, adopt, close, ping, rows changed, ids
*     mongodb_query        commands: run one, read its documents
*     mongodb_document     insert, find, update, replace, delete, count,
*                          distinct, aggregate, find-and-modify, bulk writes
*     mongodb_cursor       cursors read a page at a time; change streams
*     mongodb_database     databases: has, drop, list, use
*     mongodb_schema       collections, views, indexes
*     mongodb_transaction  begin, commit, rollback, a failed transaction
*     mongodb_settings     read and write concerns, read preference, the
*                          document format, a time limit for each operation
*     mongodb_gridfs       files in a GridFS bucket
*     mongodb_maintenance  sizes, compact, validate
*   RESULTS. A function that can fail returns 0, or a code that keeps every
* failure's own number (decision B9). A failure the server reports -- a
* command's or a write's, or a write concern's -- is the server's own code,
* unchanged and positive: 11000 a duplicate key, 26 a namespace not found,
* 13 unauthorized, and any other the server sends, which d_mongodb_error_name
* then names as the server does ("DuplicateKey"). A failure of the driver's
* own -- no server it could select, a stream that broke, a sign-in that
* failed -- or of libbson's -- JSON that is not JSON -- is packed above every
* code a server uses (its codes reach seven digits), as
* D_MONGODB_CLIENT_ERRORS + domain * 65536 + code, so that its domain and
* its code are both recoverable (d_mongodb_result_domain, _code): a
* libmongoc domain as the driver numbers it, a libbson one from
* D_MONGODB_DOMAIN_BSON, and the layer's own from D_MONGODB_DOMAIN_LAYER.
* db_common.h's negatives are what neither reports: D_DB_INVALID_ARGUMENT,
* D_DB_UNSUPPORTED (a server, a deployment or a driver that cannot, or a
* build without the driver), D_DB_EXISTS and D_DB_NOT_FOUND where no server
* has a code for the condition, and D_DB_MISMATCH. d_mongodb_error_message
* describes the last failure in the words of whoever reported it.
*   DOCUMENTS. Documents, filters, updates, pipelines and options travel as
* MongoDB Extended JSON, which the layer reads in any of its forms --
* canonical, relaxed, or the legacy one -- and writes as the handle's
* document format says (mongodb_settings.h): canonical by default, every
* value's BSON type written out, so that what is read and written back keeps
* its types; relaxed, readable, at the cost of int64 values that fit 32 bits
* reading back as int32; or the BSON itself, as bytes. Text is UTF-8.
*   NAMES. A database, a collection, an index is passed as its name; a NULL
* database means the handle's current one (mongodb_database.h). MongoDB has
* no empty database: one exists from its first collection, and lists and
* `has` answer so.
*   VERSIONS. The driver floor is 1.21, and both of the driver's lines are
* supported through the calls they share; the servers written and tested
* for are 4.2 and later. What a server can do is worked out once, when the
* handle is made, from its version and its deployment, and checked before
* each request that needs it: a transaction needs a replica set, a sharded
* cluster or a load balancer.
*   BUILDS. Without the driver the layer builds as stubs that report
* D_DB_UNSUPPORTED (cfg_mongodb_common.h, D_CFG_MONGODB). The driver has no
* calls that do not block, so the layer has none (ruling 11.26.6).
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  Handles
         1.  dj_mongodb
         2.  dj_mongodb_cursor
         3.  dj_mongodb_change_stream
    2.  Deployments
         1.  dj_mongodb_topology
    3.  Documents
         1.  dj_mongodb_document_format
    4.  Features
         1.  dj_mongodb_feature
2.  RESULTS
    -------
    1.  Packed codes
         1.  D_MONGODB_CLIENT_ERRORS
         2.  D_MONGODB_DOMAIN_BSON
         3.  D_MONGODB_DOMAIN_LAYER
         4.  D_MONGODB_LAYER_NO_MEMORY
         5.  D_MONGODB_LAYER_TOO_LARGE
         6.  D_MONGODB_LAYER_UNDESCRIBED
         7.  D_MONGODB_DOMAIN_STREAM
         8.  D_MONGODB_DOMAIN_SERVER_SELECTION
    2.  The server's codes
         1.  D_MONGODB_ERROR_<NAME>
    3.  Reading a code
    4.  Result messages
3.  LIBRARY
    -------
    1.  Driver
    2.  The driver's log
4.  SERVERS
    -------
    1.  Versions
    2.  Deployment
    3.  Features
5.  HANDLES
    -------
    1.  The driver's client
    2.  Errors
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_COMMON_H
#define DJINTERP_C_DB_MONGODB_MONGODB_COMMON_H 1

// djinterp
#include "../../djinterp.h"                                  // framework root
#include "../db_common.h"                                    // the vocabulary
#include "../../../config/c/db/mongodb/cfg_mongodb_common.h"  // layer config


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    Handles
//------------------------------------------------------------------------------
// 1.1.1
// dj_mongodb
//   struct: a connection to a MongoDB deployment, as the layer holds it: the
// driver's client, the current database, the session a transaction runs in,
// what the layer learned of the server when it was made, and the last
// failure. Declared and never defined here; every function takes a pointer.
// A handle is for one thread at a time, as the driver's client is.
struct dj_mongodb;

// 1.1.2
// dj_mongodb_cursor
//   struct: a query's results read a page at a time (mongodb_cursor.h): the
// driver's cursor and the handle it reads through. Declared and never
// defined here.
struct dj_mongodb_cursor;

// 1.1.3
// dj_mongodb_change_stream
//   struct: the changes made to a collection, a database or a deployment, as
// they happen (mongodb_cursor.h). Declared and never defined here.
struct dj_mongodb_change_stream;

// 1.2    Deployments
//------------------------------------------------------------------------------
// 1.2.1
// dj_mongodb_topology
//   enum: what a handle reached, as the server it selected first described
// itself.
enum dj_mongodb_topology
{
    D_MONGODB_TOPOLOGY_UNKNOWN = 0,   // no handle, or a server that did not
                                      // say
    D_MONGODB_TOPOLOGY_STANDALONE,    // one mongod, no replica set
    D_MONGODB_TOPOLOGY_REPLICA_SET,   // a member of a replica set
    D_MONGODB_TOPOLOGY_SHARDED,       // a mongos, in front of shards
    D_MONGODB_TOPOLOGY_LOAD_BALANCED  // a load balancer, in front of mongos
};

// 1.3    Documents
//------------------------------------------------------------------------------
// 1.3.1
// dj_mongodb_document_format
//   enum: how the layer writes a document it returns (mongodb_settings.h).
enum dj_mongodb_document_format
{
    D_MONGODB_JSON_CANONICAL = 0,  // canonical Extended JSON: every value's
                                   // type written out; reads back the same
    D_MONGODB_JSON_RELAXED,        // relaxed Extended JSON: numbers as JSON
                                   // numbers; an int64 that fits 32 bits
                                   // reads back as an int32
    D_MONGODB_BSON                 // the document's BSON, as bytes
};

// 1.4    Features
//------------------------------------------------------------------------------
// 1.4.1
// dj_mongodb_feature
//   enum: capabilities that depend on the server, its deployment or the
// driver. Each names, in brackets, what has it.
enum dj_mongodb_feature
{
    D_MONGODB_FEATURE_TRANSACTIONS = 0,  // multi-document transactions
                                         //   [a replica set from 4.0, a
                                         //   sharded cluster from 4.2, a
                                         //   load balancer]
    D_MONGODB_FEATURE_CHANGE_STREAMS,    // change streams [not standalone]
    D_MONGODB_FEATURE_TIME_SERIES,       // time series collections [5.0]
    D_MONGODB_FEATURE_CLUSTERED,         // clustered collections [5.3]
    D_MONGODB_FEATURE_STABLE_API,        // a declared API version [5.0;
                                         //   driver 1.18]
    D_MONGODB_FEATURE_SNAPSHOT_READS,    // reads at a point in time outside
                                         //   a transaction [5.0, not
                                         //   standalone]
    D_MONGODB_FEATURE_PRE_POST_IMAGES,   // change events carrying a document
                                         //   as it was [6.0, not standalone]
    D_MONGODB_FEATURE_BULK_WRITE,        // one bulk write across
                                         //   collections [8.0; driver 1.28]
    D_MONGODB_FEATURE_HIDDEN_INDEXES,    // indexes the planner ignores [4.4]
    D_MONGODB_FEATURE_WILDCARD_INDEXES   // indexes over every field [4.2]
};


//==============================================================================
// 2.  RESULTS
//==============================================================================


// 2.1    Packed codes
//------------------------------------------------------------------------------
// 2.1.1
// D_MONGODB_CLIENT_ERRORS
//   constant: where the packed codes of failures the server did not report
// begin: 2^30, above every code a server uses, below INT_MAX.
#define D_MONGODB_CLIENT_ERRORS     0x40000000

// 2.1.2
// D_MONGODB_DOMAIN_BSON
//   constant: libbson's error domains are packed from here, BSON_ERROR_JSON
// (1) as D_MONGODB_DOMAIN_BSON + 1, apart from libmongoc's, which they
// share numbers with.
#define D_MONGODB_DOMAIN_BSON       0x0100

// 2.1.3
// D_MONGODB_DOMAIN_LAYER
//   constant: the domain of the layer's own failures, below.
#define D_MONGODB_DOMAIN_LAYER      0x0200

// 2.1.4
// D_MONGODB_LAYER_NO_MEMORY
//   constant: the layer could not allocate what it returns.
#define D_MONGODB_LAYER_NO_MEMORY                                              \
    (D_MONGODB_CLIENT_ERRORS + D_MONGODB_DOMAIN_LAYER * 65536 + 1)

// 2.1.5
// D_MONGODB_LAYER_TOO_LARGE
//   constant: a document, or bytes for GridFS, larger than the layer can
// hand over.
#define D_MONGODB_LAYER_TOO_LARGE                                              \
    (D_MONGODB_CLIENT_ERRORS + D_MONGODB_DOMAIN_LAYER * 65536 + 2)

// 2.1.6
// D_MONGODB_LAYER_UNDESCRIBED
//   constant: the driver reported a failure and described none, its domain
// and its code both 0.
#define D_MONGODB_LAYER_UNDESCRIBED                                            \
    (D_MONGODB_CLIENT_ERRORS + D_MONGODB_DOMAIN_LAYER * 65536 + 3)

// 2.1.7
// D_MONGODB_DOMAIN_STREAM
//   constant: the driver's domain of a connection to a server that broke,
// or could not be made (mongoc_error_domain_t's MONGOC_ERROR_STREAM).
#define D_MONGODB_DOMAIN_STREAM            2u

// 2.1.8
// D_MONGODB_DOMAIN_SERVER_SELECTION
//   constant: the driver's domain of a deployment with no server an
// operation could use, in the time it waits for one
// (MONGOC_ERROR_SERVER_SELECTION).
#define D_MONGODB_DOMAIN_SERVER_SELECTION  15u

// 2.2    The server's codes
//------------------------------------------------------------------------------
// 2.2.1
// D_MONGODB_ERROR_<NAME>
//   constant: the server's codes the layer's own contracts name. Every other
// code the server sends comes back as it is, named by d_mongodb_error_name.
// TRANSACTION_CURSOR is the server's refusal to read a transaction's cursor
// outside it, which it names Location50740, and which the layer answers
// itself for a cursor whose transaction has ended (mongodb_cursor.h).
#define D_MONGODB_ERROR_BAD_VALUE                       2
#define D_MONGODB_ERROR_FAILED_TO_PARSE                 9
#define D_MONGODB_ERROR_UNAUTHORIZED                    13
#define D_MONGODB_ERROR_TYPE_MISMATCH                   14
#define D_MONGODB_ERROR_ILLEGAL_OPERATION               20
#define D_MONGODB_ERROR_NAMESPACE_NOT_FOUND             26
#define D_MONGODB_ERROR_INDEX_NOT_FOUND                 27
#define D_MONGODB_ERROR_NAMESPACE_EXISTS                48
#define D_MONGODB_ERROR_MAX_TIME_MS_EXPIRED             50
#define D_MONGODB_ERROR_COMMAND_NOT_FOUND               59
#define D_MONGODB_ERROR_INVALID_NAMESPACE               73
#define D_MONGODB_ERROR_INDEX_OPTIONS_CONFLICT          85
#define D_MONGODB_ERROR_INDEX_KEY_SPECS_CONFLICT        86
#define D_MONGODB_ERROR_WRITE_CONFLICT                  112
#define D_MONGODB_ERROR_COMMAND_NOT_SUPPORTED_ON_VIEW   166
#define D_MONGODB_ERROR_NO_SUCH_TRANSACTION             251
#define D_MONGODB_ERROR_DUPLICATE_KEY                   11000
#define D_MONGODB_ERROR_TRANSACTION_CURSOR              50740

// 2.3    Reading a code
//------------------------------------------------------------------------------
// reading a code -- whether a result is a packed failure of the driver's,
// of libbson's or of the layer's rather than the server's; its domain (0
// for a server's code, 0 too for 0 and db_common.h's negatives); its code
// within the domain (the server's code as it is); and whether it is the
// driver's failure to reach the deployment, in either domain above -- a
// connection that broke or could not be made, or no server to use -- after
// which the driver connects again by itself for the next request. None can
// fail
bool d_mongodb_result_is_client(
    int _result
) D_NOTHROW;
unsigned d_mongodb_result_domain(
    int _result
) D_NOTHROW;
unsigned d_mongodb_result_code(
    int _result
) D_NOTHROW;
bool d_mongodb_result_is_unreachable(
    int _result
) D_NOTHROW;

// 2.4    Result messages
//------------------------------------------------------------------------------
// result messages -- an English description of a result the layer returns:
// db_common.h's own words for its values, the layer's for its own failures,
// and for a server's code or the driver's a note that d_mongodb_error_message
// describes it, since only the handle keeps that text. Never NULL, and never
// to be freed.
const char* d_mongodb_result_message(
    int _result
) D_NOTHROW;


//==============================================================================
// 3.  LIBRARY
//==============================================================================


// 3.1    Driver
//------------------------------------------------------------------------------
// driver -- the driver's version string and number, MAJOR*10000 + MINOR*100
// + PATCH, as the program loaded it; libbson's string; "" and 0 in a build
// without the driver. None can fail
const char* d_mongodb_client_version(void) D_NOTHROW;
unsigned long d_mongodb_client_version_number(void) D_NOTHROW;
const char* d_mongodb_bson_version(void) D_NOTHROW;

// 3.2    The driver's log
//------------------------------------------------------------------------------
// the driver's log -- the most verbose level the layer's handler passes on
// to the driver's default handler, from 0 to 6 (cfg_mongodb_common.h,
// D_CFG_MONGODB_LOG_LEVEL), or -1 for nothing at all: the handler is the
// process's, so the level is too. Setting it installs the layer's handler at
// that level, replacing whatever handler the process had, and is safe from
// any thread; a level outside -1 to 6 is D_DB_INVALID_ARGUMENT, the level
// kept, and D_MONGODB_LAYER_NO_MEMORY means the process could not make the
// lock the layer takes, nothing installed. Read back, the level set, or the
// knob's until then -- -1 for a knob of -1 too, which installs no handler of
// the layer's at all. In a build without the driver, setting is
// D_DB_UNSUPPORTED and the level -1
int d_mongodb_set_log_level(
    int _level
) D_NOTHROW;
int d_mongodb_get_log_level(void) D_NOTHROW;


//==============================================================================
// 4.  SERVERS
//==============================================================================
// What the server said of itself when the handle was made. A NULL handle
// has no server: these answer NULL, 0, D_MONGODB_TOPOLOGY_UNKNOWN or false.


// 4.1    Versions
//------------------------------------------------------------------------------
// versions -- the server's version string, as buildInfo gives it; its number,
// MAJOR*10000 + MINOR*100 + PATCH; whether it is at that version or later;
// and the newest wire protocol it speaks (its maxWireVersion). A server that
// refuses buildInfo -- to a strict declared API, which it is outside of, or
// to a connection that has not signed in -- has its number reckoned from its
// wire version, the oldest release that speaks it, MAJOR.MINOR.0, and its
// string empty
const char* d_mongodb_server_version(
    const struct dj_mongodb* _db
) D_NOTHROW;
unsigned long d_mongodb_server_version_number(
    const struct dj_mongodb* _db
) D_NOTHROW;
bool d_mongodb_server_at_least(
    const struct dj_mongodb* _db,
    unsigned                 _major,
    unsigned                 _minor,
    unsigned                 _patch
) D_NOTHROW;
int d_mongodb_server_wire_version(
    const struct dj_mongodb* _db
) D_NOTHROW;

// 4.2    Deployment
//------------------------------------------------------------------------------
// deployment -- what the handle reached, and the replica set's name, NULL
// where there is none
enum dj_mongodb_topology d_mongodb_server_topology(
    const struct dj_mongodb* _db
) D_NOTHROW;
const char* d_mongodb_server_replica_set(
    const struct dj_mongodb* _db
) D_NOTHROW;

// 4.3    Features
//------------------------------------------------------------------------------
// features -- whether the server, its deployment and the driver can do it;
// `false` for a value outside enum dj_mongodb_feature
bool d_mongodb_supports(
    const struct dj_mongodb* _db,
    enum dj_mongodb_feature  _feature
) D_NOTHROW;


//==============================================================================
// 5.  HANDLES
//==============================================================================


// 5.1    The driver's client
//------------------------------------------------------------------------------
// the driver's client -- the mongoc_client_t* inside, for the driver's own
// calls: cast it where mongoc.h is included. It stays the layer's: close the
// handle with d_mongodb_close, never with mongoc_client_destroy. NULL for
// NULL.
void* d_mongodb_native(
    struct dj_mongodb* _db
) D_NOTHROW;

// 5.2    Errors
//------------------------------------------------------------------------------
// errors -- the handle's last failure: its result, as the function that
// failed returned it; its message, in the words of the server, the driver or
// the layer ("" for none); the server's name for its code ("DuplicateKey"),
// as the server's reply gives it or, where the reply has none -- a write
// error's -- or the layer answered the code itself, as the server names it
// among the codes section 2.2 lists, "" for any other; and whether the
// server or the driver labelled it so ("TransientTransactionError",
// "RetryableWriteError"), which says whether a transaction may be retried. A
// function that succeeds leaves them as they were. A NULL handle answers 0,
// "" and false.
int d_mongodb_error_code(
    const struct dj_mongodb* _db
) D_NOTHROW;
const char* d_mongodb_error_message(
    const struct dj_mongodb* _db
) D_NOTHROW;
const char* d_mongodb_error_name(
    const struct dj_mongodb* _db
) D_NOTHROW;
bool d_mongodb_error_has_label(
    const struct dj_mongodb* _db,
    const char*              _label
) D_NOTHROW;
/**
 * @brief Hands back the server's whole reply to the request that failed
 *        last, which holds what a code cannot: each write error of a bulk
 *        write, the key a duplicate repeated, a write concern's details.
 *
 * @param[in]  _db     the handle.
 * @param[out] _reply  receives the reply as the handle's document format
 *                     writes JSON (canonical where the format is BSON),
 *                     released with d_db_free; written only on success.
 * @return 0; D_DB_NOT_FOUND when the last failure had no reply, or there
 *         was none; D_MONGODB_LAYER_NO_MEMORY; D_DB_INVALID_ARGUMENT for a
 *         `NULL` argument; D_DB_UNSUPPORTED in a build without the driver.
 * @post On success the caller releases `*_reply` with d_db_free.
 */
int d_mongodb_error_reply(
    const struct dj_mongodb* _db,
    char**                   _reply
) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_COMMON_H
