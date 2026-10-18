/*******************************************************************************
* djinterp [c]                                             postgres_connection.h
*
* Making and closing connections, and what a connection reports.
*   d_postgres_connect makes a connection from a struct of the usual fields,
* d_postgres_connect_string from a connection string -- keywords
* ("host=db.example dbname=app") or a URI ("postgresql://app@db.example/app")
* -- and d_postgres_connect_with from libpq's own keywords, any it knows, and
* a connection string beside them, whose parts override them. All three set
* the connection up with the layer's defaults (cfg_postgres_connection.h)
* where neither the caller nor libpq's environment says otherwise: a bounded
* connect timeout, UTF-8 as the client encoding, and NOTICE messages left
* unsent. Whatever the caller does not name, libpq takes from its environment
* variables (PGHOST, PGUSER, PGPASSWORD, PGSSLMODE, ...), its password file
* and its service file, as for any libpq program.
*   What is made is libpq's own connection: close it with d_postgres_close or
* PQfinish, and use it with libpq's calls as with the layer's. A connection is
* for one thread at a time, as libpq's always are.
*   PostgreSQL binds a connection to one database for its whole life; there
* is no USE. To work in another database, connect again.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONNECTIONS
    -----------
    1.  Options
         1.  d_postgres_options
    2.  Connecting and closing
    3.  Liveness
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_CONNECTION_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_CONNECTION_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"     // framework root
#include "./postgres_common.h"  // pg_conn, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONNECTIONS
//==============================================================================


// 1.1    Options
//------------------------------------------------------------------------------
// 1.1.1
// d_postgres_options
//   struct: where and as whom to connect. Every field may be left 0 or NULL
// for libpq's default, which its environment variables can set; the order
// suits a positional initializer, which is all C++ before C++20 has:
//     struct d_postgres_options options = { "db.example", "app", secret };
// GCC's and Clang's -Wextra warn of the members such a list leaves out;
// listing them all, as NULL and 0, keeps it quiet.
struct d_postgres_options
{
    const char* host;      // a name, an address, or the directory of a Unix
                           // socket ("/var/run/postgresql"); NULL: PGHOST, or
                           // the default socket
    const char* user;      // NULL: PGUSER, or the operating-system user
    const char* password;  // NULL: PGPASSWORD, the password file, or none
    const char* database;  // NULL: PGDATABASE, or the user's name
    unsigned    port;      // 0: PGPORT, or 5432
    const char* sslmode;   // "disable", "prefer", "require", "verify-ca" or
                           // "verify-full"; NULL: PGSSLMODE, or "prefer"
};

// 1.2    Connecting and closing
//------------------------------------------------------------------------------
/**
 * @brief Connects to a server and sets the connection up with the layer's
 *        defaults.
 *
 * @param[in]  _options  where and as whom; `NULL` leaves every field to
 *                       libpq.
 * @param[out] _db       receives the connection; `NULL` on failure.
 * @param[out] _message  receives libpq's reason when the connection cannot
 *                       be made ("password authentication failed for user
 *                       ...", "connection refused"), which the caller
 *                       releases with d_db_free, and `NULL` otherwise; may
 *                       be `NULL` when the reason is not wanted.
 * @return 0; D_POSTGRES_SQLSTATE_CANNOT_CONNECT (08001) when no connection
 *         could be made, for whatever reason -- libpq gives none of its own
 *         numbers for it, so `_message` says which; 53200 when memory ran
 *         out; or D_DB_INVALID_ARGUMENT for a `NULL` `_db`.
 * @post On success the caller closes `*_db` with d_postgres_close.
 */
int    d_postgres_connect(const struct d_postgres_options* _options,
                          struct pg_conn**                 _db,
                          char**                           _message) D_NOTHROW;
/**
 * @brief Connects as a connection string says, and sets the connection up
 *        with the layer's defaults, where the string does not name them.
 *
 * @param[in]  _conninfo  keywords and values ("host=db.example port=5433
 *                        dbname=app") or a URI
 *                        ("postgresql://app:secret@db.example:5433/app");
 *                        `NULL` or "" leaves everything to libpq.
 * @param[out] _db        receives the connection; `NULL` on failure.
 * @param[out] _message   as for d_postgres_connect; for a string libpq
 *                        cannot read, its reason.
 * @return as d_postgres_connect, and D_DB_INVALID_ARGUMENT for a string
 *         libpq cannot read.
 * @post On success the caller closes `*_db` with d_postgres_close.
 */
int    d_postgres_connect_string(const char*      _conninfo,
                                 struct pg_conn** _db,
                                 char**           _message) D_NOTHROW;
/**
 * @brief Connects with libpq's keywords and values and a connection string
 *        beside them, and sets the connection up with the layer's defaults,
 *        where neither names them.
 *
 * @note Each name settles in one place: the connection string's parts
 *       override the keywords, which override the layer's defaults, and a
 *       keyword given twice takes its later value. A dbname among the
 *       keywords is a database's name, never a connection string: that is
 *       `_conninfo`'s to give, so that a program can name a database the
 *       string may leave out.
 *
 * @param[in]  _conninfo  keywords and values or a URI, as for
 *                        d_postgres_connect_string, or a database's name;
 *                        `NULL` or "" for none.
 * @param[in]  _keywords  libpq's connection keywords ("application_name",
 *                        "sslrootcert", "target_session_attrs", ...); may be
 *                        `NULL` when `_count` is 0.
 * @param[in]  _values    their values, in the same order; a `NULL` or empty
 *                        value is left out, as libpq leaves it.
 * @param[in]  _count     how many keywords there are.
 * @param[out] _db        receives the connection; `NULL` on failure.
 * @param[out] _message   as for d_postgres_connect_string; for a keyword
 *                        libpq does not know, libpq's words for it.
 * @return as d_postgres_connect_string, and D_DB_INVALID_ARGUMENT for
 *         `NULL` keywords or values where `_count` is not 0, a `NULL`
 *         keyword, or a keyword libpq does not know.
 * @post On success the caller closes `*_db` with d_postgres_close.
 */
int    d_postgres_connect_with(const char*        _conninfo,
                               const char* const* _keywords,
                               const char* const* _values,
                               size_t             _count,
                               struct pg_conn**   _db,
                               char**             _message) D_NOTHROW;
// closes a connection, as PQfinish does; NULL is accepted. Returns 0.
int    d_postgres_close(struct pg_conn* _db) D_NOTHROW;

// 1.3    Liveness
//------------------------------------------------------------------------------
// liveness -- whether the server still answers, by a round trip that runs
// nothing, which works inside a failed transaction too: 0, or
// D_POSTGRES_SQLSTATE_CONNECTION_FAILURE (08006) for a connection that is
// lost, which stays lost (PQreset makes it again); D_DB_INVALID_ARGUMENT for
// NULL
int    d_postgres_ping(struct pg_conn* _db) D_NOTHROW;
// the process id of the server's backend for the connection, as
// pg_backend_pid() and pg_terminate_backend() see it; 0 for NULL or a lost
// connection
int    d_postgres_backend_pid(const struct pg_conn* _db) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_CONNECTION_H
