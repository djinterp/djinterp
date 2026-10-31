/*******************************************************************************
* djinterp [c]                                              mongodb_connection.h
*
* Making, adopting and closing connections, and what a connection reports.
*   d_mongodb_connect makes a connection from a struct of the usual fields,
* d_mongodb_connect_uri from a connection string, and d_mongodb_connect_with
* from either with settings applied besides: timeouts, TLS, authentication,
* compression, retries, a declared API version, and any other option a
* connection string can carry. Each sets the driver's client up with the
* layer's defaults (cfg_mongodb_connection.h) where the connection string
* does not name them, asks for the driver's second error API, and does not
* hand the handle back until the server has answered: the driver connects
* on its first operation, so the layer pings, which signs in where
* credentials are given, then asks the server its version and what it is
* part of. A program that needs more -- its own TLS streams, a pool's client
* -- sets a mongoc_client_t up with the driver's own calls and hands it to
* d_mongodb_adopt, which wraps it as it is.
*   Either way the handle is the layer's until d_mongodb_close, which closes
* the driver's client too when the layer made it or was given it to own. A
* handle is for one thread at a time; the driver initializes itself on the
* layer's first call, once for the process.
*   Close every cursor and change stream made over a handle
* (mongodb_cursor.h) before the handle.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONNECTIONS
    -----------
    1.  Options
         1.  dj_mongodb_options
         2.  dj_mongodb_setting_name
         3.  dj_mongodb_setting
         4.  D_MONGODB_API_STRICT
         5.  D_MONGODB_API_DEPRECATION_ERRORS
    2.  Connecting and closing
    3.  Liveness
    4.  The latest write
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_CONNECTION_H
#define DJINTERP_C_DB_MONGODB_MONGODB_CONNECTION_H 1

// std
#include <stddef.h>  // size_t
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
// 1.  CONNECTIONS
//==============================================================================


// 1.1    Options
//------------------------------------------------------------------------------
// 1.1.1
// dj_mongodb_options
//   struct: where and as whom to connect. Every field may be left 0 or NULL
// for the driver's default; the order suits a positional initializer, which
// is all C++ before C++20 has:
//     struct dj_mongodb_options options = { "db.example", "app", secret };
// With `uri`, the connection string says where -- host and port must then
// be left NULL and 0 -- and each other field given replaces what it says.
// GCC's and Clang's -Wextra warn of the members such a list leaves out;
// listing them all, as NULL and 0, keeps it quiet.
struct dj_mongodb_options
{
    const char* host;         // name, address, or a Unix socket's path;
                              // NULL: localhost
    const char* user;         // NULL: none, or the connection string's
    const char* password;     // NULL: none
    const char* database;     // the current database, and the user's unless
                              // auth_source says otherwise; NULL: none, or
                              // the connection string's
    unsigned    port;         // 0: the default, 27017
    const char* auth_source;  // the database the user is defined in; NULL:
                              // the driver's rule, the database or admin
    const char* replica_set;  // the replica set's name, which the servers
                              // must have; NULL: none required
    const char* uri;          // a connection string, mongodb:// or
                              // mongodb+srv://; NULL: built from the rest
};

// 1.1.2
// dj_mongodb_setting_name
//   enum: a setting d_mongodb_connect_with applies before connecting, beyond
// what dj_mongodb_options says, and which of a setting's fields it reads
// (dj_mongodb_setting). Each is the connection-string option of its name,
// and replaces what the string says; the driver refuses a value it cannot
// read, with its own code.
enum dj_mongodb_setting_name
{
    D_MONGODB_SETTING_SERVER_SELECTION_TIMEOUT = 0,  // number: milliseconds
                                                     // to wait for a server
                                                     // an operation may use;
                                                     // replaces the layer's
    D_MONGODB_SETTING_CONNECT_TIMEOUT,               // number: milliseconds
                                                     // to wait for a socket
                                                     // to open
    D_MONGODB_SETTING_SOCKET_TIMEOUT,                // number: milliseconds
                                                     // to wait for a reply;
                                                     // 0 waits as long as it
                                                     // must
    D_MONGODB_SETTING_APP_NAME,                      // text: the name the
                                                     // server logs; replaces
                                                     // the layer's
    D_MONGODB_SETTING_TLS,                           // number: nonzero
                                                     // insists on TLS
    D_MONGODB_SETTING_TLS_CA_FILE,                   // text: the
                                                     // certificates that
                                                     // vouch for the
                                                     // server's, PEM
    D_MONGODB_SETTING_TLS_CERTIFICATE_KEY_FILE,      // text: the client's
                                                     // certificate and key,
                                                     // PEM
    D_MONGODB_SETTING_TLS_CERTIFICATE_KEY_PASSWORD,  // text: the key's
                                                     // passphrase
    D_MONGODB_SETTING_TLS_ALLOW_INVALID_CERTIFICATES, // number: nonzero
                                                      // accepts a server
                                                      // certificate no
                                                      // authority signed
    D_MONGODB_SETTING_TLS_ALLOW_INVALID_HOSTNAMES,   // number: nonzero
                                                     // accepts one naming
                                                     // another host
    D_MONGODB_SETTING_AUTH_MECHANISM,                // text: SCRAM-SHA-256,
                                                     // SCRAM-SHA-1,
                                                     // MONGODB-X509, PLAIN,
                                                     // GSSAPI, MONGODB-AWS
    D_MONGODB_SETTING_AUTH_MECHANISM_PROPERTIES,     // text: as a connection
                                                     // string writes them,
                                                     // "SERVICE_NAME:db"
    D_MONGODB_SETTING_COMPRESSORS,                   // text: "zstd,snappy,
                                                     // zlib", those the
                                                     // driver was built with
    D_MONGODB_SETTING_RETRY_WRITES,                  // number: zero turns
                                                     // the driver's retry
                                                     // of a failed write off
    D_MONGODB_SETTING_RETRY_READS,                   // number: zero turns
                                                     // its retry of a read
                                                     // off
    D_MONGODB_SETTING_DIRECT_CONNECTION,             // number: nonzero talks
                                                     // to the one host
                                                     // named, finding no
                                                     // others
    D_MONGODB_SETTING_LOAD_BALANCED,                 // number: nonzero
                                                     // expects a load
                                                     // balancer
    D_MONGODB_SETTING_SERVER_API,                    // text and number: the
                                                     // API version to
                                                     // declare, "1", and
                                                     // D_MONGODB_API_* flags;
                                                     // the server's 5.0
    D_MONGODB_SETTING_URI_OPTION                     // text and value: any
                                                     // option a connection
                                                     // string takes, by its
                                                     // name, and its value
                                                     // as the string would
                                                     // write it, a
                                                     // const char*
};

// 1.1.3
// dj_mongodb_setting
//   struct: one setting; the fields its name does not read are ignored.
// Positional initializers fill it in order:
//     struct dj_mongodb_setting settings[] =
//     {
//         { D_MONGODB_SETTING_TLS, 1ul, NULL, NULL },
//         { D_MONGODB_SETTING_URI_OPTION, 0ul, "w", "majority" }
//     };
struct dj_mongodb_setting
{
    enum dj_mongodb_setting_name name;    // which setting
    unsigned long               number;  // milliseconds, a switch, flags
    const char*                 text;    // a file, a name, an option's name
    const void*                 value;   // an option's value, as a const
                                         // char*
};

// 1.1.4
// D_MONGODB_API_STRICT
//   constant: with SERVER_API, the server refuses a command outside the
// declared API.
#define D_MONGODB_API_STRICT              0x0001u

// 1.1.5
// D_MONGODB_API_DEPRECATION_ERRORS
//   constant: with SERVER_API, the server refuses a command the declared API
// deprecates.
#define D_MONGODB_API_DEPRECATION_ERRORS  0x0002u

// 1.2    Connecting and closing
//------------------------------------------------------------------------------
/**
 * @brief Connects to a deployment and sets the connection up with the
 *        layer's defaults.
 *
 * @param[in]  _options  where and as whom; `NULL` connects to localhost
 *                       with every field defaulted.
 * @param[out] _db       receives the handle; `NULL` on failure.
 * @return 0; the server's code for a failure it reports; the driver's,
 *         packed, for one of its own (no server it could select, a sign-in
 *         that failed, a connection string it could not read);
 *         D_MONGODB_LAYER_NO_MEMORY when the handle cannot be allocated;
 *         D_DB_INVALID_ARGUMENT for a `NULL` `_db`, or a host or port given
 *         beside a `uri`.
 * @post On success the caller closes `*_db` with d_mongodb_close.
 */
int d_mongodb_connect(
    const struct dj_mongodb_options* _options,
    struct dj_mongodb**              _db
) D_NOTHROW;
/**
 * @brief Connects to the deployment a connection string names, as
 *        d_mongodb_connect does.
 *
 * @param[in]  _uri  the connection string, mongodb:// or mongodb+srv://.
 * @param[out] _db   receives the handle; `NULL` on failure.
 * @return as d_mongodb_connect; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 * @post On success the caller closes `*_db` with d_mongodb_close.
 */
int d_mongodb_connect_uri(
    const char*         _uri,
    struct dj_mongodb** _db
) D_NOTHROW;
/**
 * @brief Connects as d_mongodb_connect does, with the settings given
 *        applied first.
 *
 * @note The settings replace what the connection string and the options
 *       say: each URI_OPTION as if the string had carried it, read with
 *       it, and then the others, in their order.
 *
 * @param[in]  _options   where and as whom; `NULL` connects to localhost
 *                        with every field defaulted.
 * @param[in]  _settings  the settings; may be `NULL` when `_count` is 0.
 * @param[in]  _count     how many settings there are.
 * @param[out] _db        receives the handle; `NULL` on failure.
 * @param[out] _message   receives the reason when no connection is made, in
 *                        the words of whoever refused it -- the server, the
 *                        driver or the layer -- which the code alone cannot
 *                        give once the failed handle is closed; released
 *                        with d_db_free; `NULL` on success, and when memory
 *                        runs out for the copy. May be `NULL` when the
 *                        reason is not wanted.
 * @return as d_mongodb_connect; D_DB_INVALID_ARGUMENT also for a setting of
 *         no such name, one without the text or value it needs, a number
 *         the option's 32 bits cannot hold, an option of a value the driver
 *         refuses, or `NULL` settings with a nonzero count; the driver's
 *         code, packed, for a URI_OPTION of a value it cannot read, which
 *         its parser refuses -- one of no name it knows it passes over,
 *         with a warning in its log -- and for settings that cannot go
 *         together, loadBalanced with a replica set's name among them.
 * @post On success the caller closes `*_db` with d_mongodb_close.
 */
int d_mongodb_connect_with(
    const struct dj_mongodb_options* _options,
    const struct dj_mongodb_setting* _settings,
    size_t                           _count,
    struct dj_mongodb**              _db,
    char**                           _message
) D_NOTHROW;
/**
 * @brief Wraps a client a program made with the driver's own calls.
 *
 * @note The layer reads failures in the driver's second error API: it asks
 *       the server a command of no such name, to see which the client
 *       reads, and sets the second where the client was never set. A
 *       client its maker set to the first, or popped from a pool set so, is
 *       refused, since its server codes would come back as the driver's.
 *       The layer pings, as a connect does, to learn the server.
 *
 * @param[in]  _native  a mongoc_client_t*, which is left as it is.
 * @param[in]  _owned   whether d_mongodb_close is to destroy it as well.
 * @param[out] _db      receives the handle; `NULL` on failure.
 * @return 0; the server's code, or the driver's, when the server cannot be
 *         reached; D_MONGODB_LAYER_NO_MEMORY when the handle cannot be
 *         allocated; D_DB_INVALID_ARGUMENT for a `NULL` argument, or a
 *         client set to the driver's first error API.
 * @post On success the caller closes `*_db` with d_mongodb_close; on failure
 *       `_native` is the caller's, whatever `_owned` said.
 */
int d_mongodb_adopt(
    void*               _native,
    bool                _owned,
    struct dj_mongodb** _db
) D_NOTHROW;
// closes a handle -- ending its session, a transaction in it aborted -- and
// the driver's client with it when the layer owns that; NULL is accepted.
// Returns 0.
int d_mongodb_close(
    struct dj_mongodb* _db
) D_NOTHROW;

// 1.3    Liveness
//------------------------------------------------------------------------------
// liveness -- 0 when the server answers a ping; else the failure, the
// server's or the driver's; D_DB_INVALID_ARGUMENT for a NULL handle
int d_mongodb_ping(
    struct dj_mongodb* _db
) D_NOTHROW;

// 1.4    The latest write
//------------------------------------------------------------------------------
// the latest write -- how many documents the handle's latest successful
// write inserted, changed, deleted or upserted, as the server counted them
// (an update's documents modified, not those matched; a find-and-modify's
// document, which its reply does not say was changed); 0 before any, and for
// a NULL handle
int64_t d_mongodb_changes(
    const struct dj_mongodb* _db
) D_NOTHROW;
/**
 * @brief Hands back the `_id` of the document the latest insert_one gave
 *        the server, or the latest upsert inserted -- an update's, a
 *        replacement's, a find-and-modify's, or an update command's run
 *        through mongodb_query.h; a bulk write's upserts are not kept.
 *
 * @param[in]  _db  the handle.
 * @param[out] _id  receives the value as Extended JSON, as the handle's
 *                  document format writes it (canonical where the format is
 *                  BSON): `{"$oid": "..."}` for an ObjectId the layer made;
 *                  released with d_db_free; written only on success.
 * @return 0; D_DB_NOT_FOUND when no such write has happened on the handle,
 *         or the latest wrote no new document; D_MONGODB_LAYER_NO_MEMORY;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success the caller releases `*_id` with d_db_free.
 */
int d_mongodb_last_insert_id(
    const struct dj_mongodb* _db,
    char**                   _id
) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_CONNECTION_H
