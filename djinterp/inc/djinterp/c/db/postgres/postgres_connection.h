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
*   Each connect has a form that does not block (db_common.h 1.3), as do
* d_postgres_reconnect, which makes a connection again, and the cancel
* request, which asks the server to stop what a connection runs: the call
* starts, says what to wait for, and is continued until it is done.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.24
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
    4.  Connecting without blocking
    5.  Reconnecting
    6.  Cancelling
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

// 1.4    Connecting without blocking
//------------------------------------------------------------------------------
// Each connect of 1.2 has a form that starts the connection and returns at
// once: D_DB_WAIT, with the connection being made in `*_db` and what to wait
// for in `*_pending`, after which the program waits so and calls
// d_postgres_connect_continue with what came, until it answers anything else.
// The connection is set up as the blocking forms set it up, and nothing after
// the handshake waits: the layer's settings travel in the startup packet, as
// they do for every connect. libpq keeps no connect_timeout in this mode, so
// the layer keeps it, as the libpq in use keeps it in its blocking connect --
// in whole seconds, and at least two, before libpq 17; to the microsecond
// from it -- but over the whole connect, where libpq gives each host and
// address of a list a timeout of its own. A connection being made is
// abandoned with d_postgres_close.
/**
 * @brief Starts a connection as d_postgres_connect makes one, without
 *        waiting.
 *
 * @param[in]  _options  as for d_postgres_connect.
 * @param[out] _db       receives the connection being made; `NULL` on
 *                       failure.
 * @param[out] _pending  receives what to wait for before continuing.
 * @param[out] _message  as for d_postgres_connect.
 * @return D_DB_WAIT; a failure as d_postgres_connect gives it, `*_db` then
 *         `NULL`; 08001 for a connect_timeout that is not an integer, as
 *         libpq 12 and later refuse it; or D_DB_INVALID_ARGUMENT for a
 *         `NULL` `_db` or `_pending`.
 * @post While the answer is D_DB_WAIT, the caller continues the connection
 *       with d_postgres_connect_continue, or abandons it with
 *       d_postgres_close.
 */
int    d_postgres_connect_start(const struct d_postgres_options* _options,
                                struct pg_conn**                 _db,
                                struct d_db_pending*             _pending,
                                char**                           _message)
                                D_NOTHROW;
/**
 * @brief Starts a connection as d_postgres_connect_string makes one, without
 *        waiting.
 *
 * @param[in]  _conninfo  as for d_postgres_connect_string.
 * @param[out] _db        receives the connection being made; `NULL` on
 *                        failure.
 * @param[out] _pending   receives what to wait for before continuing.
 * @param[out] _message   as for d_postgres_connect_string.
 * @return as d_postgres_connect_start, and D_DB_INVALID_ARGUMENT for a
 *         string libpq cannot read.
 * @post As for d_postgres_connect_start.
 */
int    d_postgres_connect_string_start(const char*          _conninfo,
                                       struct pg_conn**     _db,
                                       struct d_db_pending* _pending,
                                       char**               _message)
                                       D_NOTHROW;
/**
 * @brief Starts a connection as d_postgres_connect_with makes one, without
 *        waiting.
 *
 * @param[in]  _conninfo  as for d_postgres_connect_with.
 * @param[in]  _keywords  as for d_postgres_connect_with.
 * @param[in]  _values    as for d_postgres_connect_with.
 * @param[in]  _count     as for d_postgres_connect_with.
 * @param[out] _db        receives the connection being made; `NULL` on
 *                        failure.
 * @param[out] _pending   receives what to wait for before continuing.
 * @param[out] _message   as for d_postgres_connect_with.
 * @return as d_postgres_connect_start, and D_DB_INVALID_ARGUMENT as
 *         d_postgres_connect_with gives it.
 * @post As for d_postgres_connect_start.
 */
int    d_postgres_connect_with_start(const char*          _conninfo,
                                     const char* const*   _keywords,
                                     const char* const*   _values,
                                     size_t               _count,
                                     struct pg_conn**     _db,
                                     struct d_db_pending* _pending,
                                     char**               _message)
                                     D_NOTHROW;
/**
 * @brief Continues a connection a start of 1.4 began.
 *
 * @note libpq is asked to go on only once the socket is ready: a
 *       continuation with neither D_DB_WAIT_READ nor D_DB_WAIT_WRITE in
 *       `_events` -- the deadline passed, or nothing came -- fails the
 *       connection when the deadline has passed and otherwise waits again,
 *       for the same thing, for the time left.
 *
 * @param[in,out] _db       the connection being made; `NULL`ed when it
 *                          fails, which finishes it.
 * @param[in]     _events   what came, as d_db_wait: the socket ready to read
 *                          or to write, or D_DB_WAIT_TIMEOUT when the
 *                          deadline passed first.
 * @param[in,out] _pending  what the start or the last continuation wrote;
 *                          receives what to wait for next.
 * @param[out]    _message  as for d_postgres_connect; for a deadline that
 *                          passed, "timeout expired", libpq's words.
 * @return 0, the connection made and set up; D_DB_WAIT; 08001 when it
 *         cannot be made, its deadline passed among the reasons; 53200 when
 *         memory ran out; or D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On 0 the caller closes `*_db` with d_postgres_close; on a failure
 *       `*_db` is `NULL`.
 */
int    d_postgres_connect_continue(struct pg_conn**     _db,
                                   unsigned             _events,
                                   struct d_db_pending* _pending,
                                   char**               _message) D_NOTHROW;

// 1.5    Reconnecting
//------------------------------------------------------------------------------
// Reconnecting closes the connection and makes it again with the parameters it
// was made with, as libpq's PQreset does: for a connection that was lost, or
// one to start over on. Whatever the session held -- prepared statements,
// temporary tables, settings made with SET, a transaction, a run of rows --
// went with it; a statement the layer prepared must be prepared again.
// _reconnect_start and _reconnect_continue make it without blocking, as 1.4
// makes a connection, the deadline kept the same way; abandoning one under way
// is closing the connection.
/**
 * @brief Closes the connection and makes it again, waiting.
 *
 * @param[in] _db  the connection.
 * @return 0; 08001 when it cannot be made again, which leaves it lost --
 *         08006 to every call but another reconnect or a close --
 *         d_postgres_error_message then saying why; or
 *         D_DB_INVALID_ARGUMENT for `NULL`.
 */
int    d_postgres_reconnect(struct pg_conn* _db) D_NOTHROW;
/**
 * @brief Closes the connection and starts making it again, without waiting.
 *
 * @param[in]  _db       the connection.
 * @param[out] _pending  receives what to wait for before continuing.
 * @return D_DB_WAIT; 08001 when it cannot be started again, as
 *         d_postgres_reconnect gives it; or D_DB_INVALID_ARGUMENT for a
 *         `NULL` argument.
 * @post While the answer is D_DB_WAIT, the caller continues with
 *       d_postgres_reconnect_continue.
 */
int    d_postgres_reconnect_start(struct pg_conn*      _db,
                              struct d_db_pending* _pending) D_NOTHROW;
/**
 * @brief Continues a reconnect d_postgres_reconnect_start began.
 *
 * @note As for d_postgres_connect_continue, libpq goes on only once the
 *       socket is ready.
 *
 * @param[in]     _db       the connection being made again.
 * @param[in]     _events   what came, as for d_postgres_connect_continue.
 * @param[in,out] _pending  what the start or the last continuation wrote;
 *                          receives what to wait for next.
 * @return 0, the connection made again; D_DB_WAIT; 08001 when it cannot be,
 *         as d_postgres_reconnect gives it -- for a deadline that passed,
 *         d_postgres_error_message says nothing of it; or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 */
int    d_postgres_reconnect_continue(struct pg_conn*      _db,
                                 unsigned             _events,
                                 struct d_db_pending* _pending) D_NOTHROW;

// 1.6    Cancelling
//------------------------------------------------------------------------------
// A cancel request asks the server to stop what the connection is running.
// It travels on a connection of its own, so it is sent while the connection
// waits for results -- from another thread, or between continuations -- and
// the server stops the statement, if it is still running, with 57014
// (query_canceled), which the connection's next result reports; one that
// arrives after it finished does nothing. d_postgres_cancel sends a request
// and waits until it is delivered: from libpq 17 encrypted as the connection
// is, and before it in the clear, as libpq's PQcancel sends it.
// _cancel_start and _cancel_continue send one without blocking, which only
// libpq 17 can (D_POSTGRES_CLIENT_ASYNC_CANCEL), the deadline kept as 1.4
// keeps a connect's; d_postgres_cancel_finish abandons one under way.
/**
 * @brief Asks the server to stop what the connection is running, and waits
 *        until the request is delivered.
 *
 * @param[in]  _db       the connection whose statement to stop.
 * @param[out] _message  receives libpq's reason when the request could not
 *                       be delivered, which the caller releases with
 *                       d_db_free, and `NULL` otherwise; may be `NULL`.
 * @return 0, the request delivered -- which says nothing of whether a
 *         statement stopped; 08001 when it could not be; 08006 for a
 *         connection that is lost; 53200 when memory ran out; or
 *         D_DB_INVALID_ARGUMENT for a `NULL` connection.
 */
int    d_postgres_cancel(struct pg_conn* _db,
                         char**          _message) D_NOTHROW;
/**
 * @brief Starts sending a cancel request, without waiting.
 *
 * @param[in]  _db       the connection whose statement to stop.
 * @param[out] _cancel   receives the request's own connection; `NULL` on
 *                       failure.
 * @param[out] _pending  receives what to wait for before continuing.
 * @param[out] _message  as for d_postgres_cancel.
 * @return D_DB_WAIT; a failure as d_postgres_cancel gives it, `*_cancel`
 *         then `NULL`; D_DB_UNSUPPORTED with a libpq before 17; or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post While the answer is D_DB_WAIT, the caller continues with
 *       d_postgres_cancel_continue, or abandons the request with
 *       d_postgres_cancel_finish.
 */
int    d_postgres_cancel_start(struct pg_conn*         _db,
                               struct pg_cancel_conn** _cancel,
                               struct d_db_pending*    _pending,
                               char**                  _message) D_NOTHROW;
/**
 * @brief Continues a cancel request d_postgres_cancel_start began.
 *
 * @note As for d_postgres_connect_continue, libpq goes on only once the
 *       socket is ready.
 *
 * @param[in,out] _cancel   the request's connection; finished, and `NULL`ed,
 *                          once the request is delivered or fails.
 * @param[in]     _events   what came, as for d_postgres_connect_continue.
 * @param[in,out] _pending  what the start or the last continuation wrote;
 *                          receives what to wait for next.
 * @param[out]    _message  as for d_postgres_cancel; for a deadline that
 *                          passed, "timeout expired".
 * @return 0, the request delivered; D_DB_WAIT; 08001 when it could not be;
 *         D_DB_UNSUPPORTED with a libpq before 17; or D_DB_INVALID_ARGUMENT
 *         for a `NULL` argument.
 */
int    d_postgres_cancel_continue(struct pg_cancel_conn** _cancel,
                                  unsigned                _events,
                                  struct d_db_pending*    _pending,
                                  char**                  _message)
                                  D_NOTHROW;
// abandons a cancel request under way, finishing its connection; NULL is
// accepted. Returns 0
int    d_postgres_cancel_finish(struct pg_cancel_conn* _cancel) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_CONNECTION_H
