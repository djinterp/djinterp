/*******************************************************************************
* djinterp [c]                                              mariadb_connection.h
*
* Making, adopting and closing connections, and what a connection reports.
*   d_mariadb_connect makes a connection from a struct of the usual fields and
* sets it up with the layer's defaults: the character set D_CFG_MARIADB_CHARSET
* names, utf8mb4 unless the build says otherwise; a bounded connect timeout
* (cfg_mariadb_connection.h); and LOAD DATA LOCAL off, since it lets a server
* read any file the client can. d_mariadb_connect_with takes, beside the
* struct, the client settings a program commonly needs -- timeouts, TLS,
* compression, an init command, connect attributes, client flags -- and any
* other of mysql_options's, as given. A program that needs more still -- a
* plugin directory, its own authentication -- sets up a MYSQL* with the
* client's own calls and hands it to d_mariadb_adopt, which wraps it without
* changing it.
*   Either way the handle is the layer's until d_mariadb_close, which closes
* the client's connection too when the layer made it or was given it to own.
* A handle is for one thread at a time. A program that connects from several
* threads calls mysql_library_init once, before the first connection, as the
* client libraries ask. d_mariadb_reset_connection gives a connection a fresh
* session, set up as the connect set it up, without signing in again.
*   Connecting, changing user, resetting and pinging have forms that do not
* block (db_common.h 1.3), as far as the client can make them so, and
* d_mariadb_cancel asks the server, from a second connection, to stop what a
* connection runs.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONNECTIONS
    -----------
    1.  Options
         1.  d_mariadb_options
         2.  d_mariadb_setting_name
         3.  d_mariadb_ssl_mode
         4.  d_mariadb_setting
    2.  Connecting and closing
    3.  Changing user
    4.  Resetting the session
    5.  Liveness
    6.  Rows changed
    7.  Options read back
    8.  Without blocking
    9.  Cancelling
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_CONNECTION_H
#define DJINTERP_C_DB_MARIADB_MARIADB_CONNECTION_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"    // framework root
#include "./mariadb_common.h"  // d_mariadb, the layer's results
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; int64_t, uint64_t

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
// d_mariadb_options
//   struct: where and as whom to connect. Every field may be left 0 or NULL
// for the client's default; the order suits a positional initializer, which
// is all C++ before C++20 has:
//     struct d_mariadb_options options = { "db.example", "app", secret };
// GCC's and Clang's -Wextra warn of the members such a list leaves out;
// listing them all, as NULL and 0, keeps it quiet.
struct d_mariadb_options
{
    const char* host;         // name or address; NULL: localhost
    const char* user;         // NULL: the client's default user
    const char* password;     // NULL: none
    const char* database;     // the current database; NULL: none
    unsigned    port;         // 0: the default, 3306
    const char* unix_socket;  // a Unix socket or Windows pipe; NULL: the
                              // default, where host is localhost
};

// 1.1.2
// d_mariadb_setting_name
//   enum: a client setting d_mariadb_connect_with applies before connecting,
// beyond what d_mariadb_options says, and which of a setting's fields it
// reads (d_mariadb_setting). The last five are options not every client
// has, and a setting the client has no option for is D_DB_UNSUPPORTED:
// MAX_PACKET is every libmysqlclient's and Connector/C's from 3.0;
// DEFAULT_AUTH every client's; TLS_VERSION libmysqlclient's from 5.7.10,
// which refuses a list it cannot read, and Connector/C's from 3.0, which
// passes over a version it does not know; TLS_CIPHERSUITES libmysqlclient's
// from 8.0.16 alone; and CLEARTEXT_PLUGIN libmysqlclient's switch, without
// which it refuses to send a password in clear text -- MariaDB's clients send
// one unasked, whatever the setting says. CHARSET, or a NATIVE setting of
// MYSQL_SET_CHARSET_NAME, replaces the layer's character set, and the session
// is given it where the handshake loses it, as it is given the layer's.
enum d_mariadb_setting_name
{
    D_MARIADB_SETTING_CONNECT_TIMEOUT = 0,  // number: seconds to wait for the
                                            // server to answer; replaces the
                                            // layer's own
    D_MARIADB_SETTING_READ_TIMEOUT,         // number: seconds to wait for a
                                            // reply; 0, the client's, waits
                                            // as long as it must
    D_MARIADB_SETTING_WRITE_TIMEOUT,        // number: seconds to wait to send
    D_MARIADB_SETTING_COMPRESS,             // number: nonzero compresses what
                                            // passes between them
    D_MARIADB_SETTING_INIT_COMMAND,         // text: SQL the server runs each
                                            // time the client connects
    D_MARIADB_SETTING_LOCAL_INFILE,         // number: nonzero lets LOAD DATA
                                            // LOCAL have the server read the
                                            // client's files, which the layer
                                            // forbids otherwise
    D_MARIADB_SETTING_SSL_KEY,              // text: the client's private key,
                                            // a PEM file
    D_MARIADB_SETTING_SSL_CERT,             // text: the client's certificate
    D_MARIADB_SETTING_SSL_CA,               // text: the certificates that
                                            // vouch for the server's
    D_MARIADB_SETTING_SSL_MODE,             // number: a d_mariadb_ssl_mode
    D_MARIADB_SETTING_CONNECT_ATTRIBUTE,    // text and value: an attribute's
                                            // name and its value, a string,
                                            // which performance_schema shows
    D_MARIADB_SETTING_CLIENT_FLAGS,         // number: CLIENT_* flags, added to
                                            // the layer's CLIENT_MULTI_RESULTS
    D_MARIADB_SETTING_NATIVE,               // number and value: mysql_options's
                                            // option and argument, as given
    D_MARIADB_SETTING_MAX_PACKET,           // number: the largest packet, in
                                            // bytes, the client takes from
                                            // the server -- a row; a larger
                                            // one loses the connection (2020
                                            // from libmysqlclient, 2013 from
                                            // Connector/C) -- and, in
                                            // libmysqlclient, the largest a
                                            // prepared statement's values
                                            // make, refused past it (2020),
                                            // the connection kept; 0 leaves
                                            // the client's own, 1 GiB
    D_MARIADB_SETTING_DEFAULT_AUTH,         // text: the authentication plugin
                                            // the client answers with first,
                                            // before the server names its own
    D_MARIADB_SETTING_TLS_VERSION,          // text: the TLS versions allowed,
                                            // as "TLSv1.2,TLSv1.3"
    D_MARIADB_SETTING_TLS_CIPHERSUITES,     // text: TLS 1.3's cipher suites
                                            // allowed, as
                                            // "TLS_AES_128_GCM_SHA256"
    D_MARIADB_SETTING_CLEARTEXT_PLUGIN,     // number: nonzero lets the client
                                            // send the password in clear
                                            // text when the server's
                                            // authentication asks for it
    D_MARIADB_SETTING_SERVER_PUBLIC_KEY,    // text: a file holding the
                                            // server's RSA public key, in
                                            // PEM, with which MySQL's
                                            // caching_sha2_password and
                                            // sha256_password encrypt the
                                            // password where the connection
                                            // has no TLS
    D_MARIADB_SETTING_GET_SERVER_PUBLIC_KEY, // number: nonzero lets the
                                             // client ask the server for
                                             // that key where no file gives
                                             // it -- open to a man in the
                                             // middle, as a file is not.
                                             // libmysqlclient asks only when
                                             // told to, from 8.0.3
                                             // (D_DB_UNSUPPORTED before it);
                                             // Connector/C always asks, so 0
                                             // is D_DB_UNSUPPORTED there
    D_MARIADB_SETTING_CHARSET                // text: the character set, in
                                             // place of the layer's
                                             // D_CFG_MARIADB_CHARSET; the
                                             // session is given it where
                                             // the handshake loses it, as it
                                             // is given the layer's; not
                                             // empty
};

// 1.1.3
// d_mariadb_ssl_mode
//   enum: what the connection asks of TLS; PREFERRED where no setting names
// a mode, with every client. MySQL's client takes each as its --ssl-mode
// does. MariaDB's client is given each by its two switches -- PREFERRED
// too, which it would otherwise take, before 3.4, as no TLS unless a key, a
// certificate or an authority is given, and from 3.4 as TLS insisted on and
// its certificate checked -- and cannot verify the authority without the
// server's name as well, so VERIFY_CA is D_DB_UNSUPPORTED with it. With
// either, a mode that insists on TLS is checked once connected, and a
// connection without it is refused with 2026.
enum d_mariadb_ssl_mode
{
    D_MARIADB_SSL_PREFERRED = 0,     // TLS where the server offers it, the
                                     // certificate unchecked
    D_MARIADB_SSL_DISABLED,          // never TLS
    D_MARIADB_SSL_REQUIRED,          // TLS or no connection, the server's
                                     // certificate unchecked
    D_MARIADB_SSL_VERIFY_CA,         // ... and signed by an authority given
    D_MARIADB_SSL_VERIFY_IDENTITY    // ... and naming the host connected to
};

// 1.1.4
// d_mariadb_setting
//   struct: one setting; the fields its name does not read are ignored.
// Positional initializers fill it in order:
//     struct d_mariadb_setting settings[] =
//     {
//         { D_MARIADB_SETTING_READ_TIMEOUT, 30ul, NULL, NULL },
//         { D_MARIADB_SETTING_SSL_MODE, D_MARIADB_SSL_REQUIRED, NULL, NULL }
//     };
struct d_mariadb_setting
{
    enum d_mariadb_setting_name name;    // which setting
    unsigned long               number;  // seconds, a switch, flags, a mode;
                                         // NATIVE's option
    const char*                 text;    // a file, SQL, an attribute's name
    const void*                 value;   // an attribute's value, as a
                                         // const char*; NATIVE's argument
};

// 1.2    Connecting and closing
//------------------------------------------------------------------------------
/**
 * @brief Connects to a server and sets the connection up with the layer's
 *        defaults.
 *
 * @param[in]  _options  where and as whom; `NULL` connects with every field
 *                       defaulted.
 * @param[out] _db       receives the handle; `NULL` on failure.
 * @return 0, the client's error number for a failure (1045, access denied;
 *         2002 or 2003, no server there; 2005, unknown host), CR_OUT_OF_MEMORY
 *         (2008) when the handle cannot be allocated, or
 *         D_DB_INVALID_ARGUMENT for a `NULL` `_db`.
 * @post On success the caller closes `*_db` with d_mariadb_close.
 */
int            d_mariadb_connect(const struct d_mariadb_options* _options,
                                 struct d_mariadb**              _db) D_NOTHROW;
/**
 * @brief Connects as d_mariadb_connect does, with the client settings given
 *        applied first.
 *
 * @note The settings are applied in their order, after the layer's own
 *       defaults, which one may replace: a CONNECT_TIMEOUT its timeout, a
 *       LOCAL_INFILE its refusal. The character set is the layer's;
 *       d_mariadb_set_charset changes it once connected.
 *
 * @param[in]  _options   where and as whom; `NULL` connects with every field
 *                        defaulted.
 * @param[in]  _settings  the settings; may be `NULL` when `_count` is 0.
 * @param[in]  _count     how many settings there are.
 * @param[out] _db        receives the handle; `NULL` on failure.
 * @param[out] _message   receives the reason when no connection is made --
 *                        the client's own words ("Access denied for user
 *                        'app'@'localhost' (using password: YES)"), which
 *                        the error number alone cannot give once the failed
 *                        connection is closed, or the layer's for a refusal
 *                        of its own -- released with d_db_free; `NULL` on
 *                        success, and when memory runs out for the copy. May
 *                        be `NULL` when the reason is not wanted.
 * @return as d_mariadb_connect; the client's error number for a setting it
 *         refuses (2026 for TLS that could not be had), or 2000 when it
 *         refuses one without a number (a TLS version list libmysqlclient
 *         cannot read); D_DB_UNSUPPORTED for an SSL_MODE the client cannot
 *         honour, or a setting it has no option for; D_DB_INVALID_ARGUMENT
 *         for a setting of no such name, one without the text or value it
 *         needs, a timeout past what the client takes, or `NULL` settings
 *         with a nonzero count.
 * @post On success the caller closes `*_db` with d_mariadb_close.
 */
int            d_mariadb_connect_with(const struct d_mariadb_options* _options,
                                      const struct d_mariadb_setting* _settings,
                                      size_t                          _count,
                                      struct d_mariadb**              _db,
                                      char**                          _message)
                                      D_NOTHROW;
/**
 * @brief Wraps a connection a program made with the client's own calls.
 *
 * @param[in]  _native  a connected MYSQL*, which is left as it is.
 * @param[in]  _owned   whether d_mariadb_close is to close it as well.
 * @param[out] _db      receives the handle; `NULL` on failure.
 * @return 0, CR_OUT_OF_MEMORY (2008) when the handle cannot be allocated, or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success the caller closes `*_db` with d_mariadb_close; on failure
 *       `_native` is the caller's, whatever `_owned` said.
 */
int            d_mariadb_adopt(void*              _native,
                               bool               _owned,
                               struct d_mariadb** _db) D_NOTHROW;
// closes a handle, and the client's connection with it when the layer owns
// that; NULL is accepted. Returns 0.
int            d_mariadb_close(struct d_mariadb* _db) D_NOTHROW;

// 1.3    Changing user
//------------------------------------------------------------------------------
/**
 * @brief Signs the connection in as another user, as mysql_change_user does.
 *
 * @note The server ends the session and begins another: an open transaction
 *       is rolled back, and temporary tables, user variables and prepared
 *       statements go, so a statement prepared on the connection can only be
 *       finalized after. The character set is kept.
 *
 * @param[in] _db        the handle.
 * @param[in] _user      the user; `NULL` for the client's default.
 * @param[in] _password  the password; `NULL` for none.
 * @param[in] _database  the current database after; `NULL` for none.
 * @return 0; the server's error number for a failure (1045, access
 *         denied), after which a MariaDB server keeps the connection signed
 *         in as before, and a MySQL server closes it, so that d_mariadb_ping
 *         then answers 2013; or D_DB_INVALID_ARGUMENT for a `NULL` handle.
 */
int            d_mariadb_change_user(struct d_mariadb* _db,
                                     const char*       _user,
                                     const char*       _password,
                                     const char*       _database) D_NOTHROW;

// 1.4    Resetting the session
//------------------------------------------------------------------------------
/**
 * @brief Gives the connection a fresh session, as mysql_reset_connection
 *        does, without signing in again, and sets it up as the layer's
 *        connect did.
 *
 * @note The server ends the session's state: an open transaction is rolled
 *       back and its locks released; temporary tables, user variables and
 *       prepared statements go -- a statement prepared on the connection
 *       fails from then on (2056 through libmysqlclient, 2013 through
 *       Connector/C) and can only be finalized; session variables take the
 *       server's global values again -- MySQL's character set among them,
 *       MariaDB's returning to the one the session began with. The current
 *       database stays. Then the layer runs again the init commands
 *       d_mariadb_connect_with's settings gave (not one given through a
 *       NATIVE setting, nor any of an adopted connection's), in their order,
 *       and sets the client's character set again where the session's can
 *       differ from it: on a MySQL server always, where a program changed it
 *       since the session began, or where the handshake could not carry it.
 *       Query attributes not yet sent are dropped.
 *
 * @param[in] _db  the handle.
 * @return 0; the client's error number for the reset, or for the first init
 *         command or character set that failed after it, the reset having
 *         happened; D_DB_UNSUPPORTED where the server is older than MySQL
 *         5.7.3 or MariaDB 10.2.4, or the client has no reset (a MariaDB
 *         client before Connector/C 3.0); D_DB_INVALID_ARGUMENT for a `NULL`
 *         handle.
 */
int            d_mariadb_reset_connection(struct d_mariadb* _db) D_NOTHROW;

// 1.5    Liveness
//------------------------------------------------------------------------------
// liveness -- whether the server still answers: 0, or the client's error
// number (2006, gone away; 2013, lost); D_DB_INVALID_ARGUMENT for NULL
int            d_mariadb_ping(struct d_mariadb* _db) D_NOTHROW;
// the server's id for the connection, as CONNECTION_ID() and KILL see it; 0
// for NULL
unsigned long  d_mariadb_connection_id(const struct d_mariadb* _db) D_NOTHROW;
// the server's summary of itself, as mysql_stat words it ("Uptime: 3600
// Threads: 2  Questions: 15 ..."), in `_status`, which the caller releases
// with d_db_free; written only on success. Returns 0, the client's error
// number, CR_OUT_OF_MEMORY (2008), or D_DB_INVALID_ARGUMENT for NULL
int            d_mariadb_server_status(struct d_mariadb* _db,
                                       char**            _status) D_NOTHROW;

// 1.6    Rows changed
//------------------------------------------------------------------------------
// rows changed -- the AUTO_INCREMENT value the last statement produced, 0 if
// it produced none; the rows it affected, as mysql_affected_rows counts them
// (an UPDATE counts the rows it changed, not those it matched, and a
// statement that returned rows counts those), -1 when it failed; and the
// warnings it raised. Any statement is the last, the layer's own among them
// -- d_mariadb_has_table asks one -- so read these straight after the
// statement they are about. A NULL handle answers 0.
uint64_t       d_mariadb_last_insert_id(const struct d_mariadb* _db) D_NOTHROW;
int64_t        d_mariadb_changes(const struct d_mariadb* _db) D_NOTHROW;
unsigned       d_mariadb_warning_count(const struct d_mariadb* _db) D_NOTHROW;

// 1.7    Options read back
//------------------------------------------------------------------------------
/**
 * @brief Reads one of the client's options for the connection back, as
 *        mysql_get_option reads it, without the server.
 *
 * @note `_value` points at the option's own type, as mysql.h documents it
 *       for mysql_options: an unsigned int for MYSQL_OPT_CONNECT_TIMEOUT, a
 *       const char* for MYSQL_SET_CHARSET_NAME, and so on. A text read is
 *       the client's, valid until the option changes or the connection
 *       closes. Options the layer sets from d_mariadb_options and the
 *       settings read back as it set them.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _option  the option, as mysql.h numbers it.
 * @param[out] _value   receives the option's value.
 * @return 0; D_DB_UNSUPPORTED for an option the client cannot read back, and
 *         for a client without mysql_get_option (Connector/C before 3.0);
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 */
int            d_mariadb_get_option(const struct d_mariadb* _db,
                                    int                     _option,
                                    void*                   _value) D_NOTHROW;

// 1.8    Without blocking
//------------------------------------------------------------------------------
// The forms of 1.2 to 1.5 that never wait: each starts, answers D_DB_WAIT with
// what to wait for in `*_pending`, and is continued with what came until it
// answers anything else, as the blocking form would; meanwhile the connection
// is held, every other command on it 2014. MariaDB's client (Connector/C) makes
// each without blocking, and says what it waits for, its own timeouts among
// them -- the connect timeout too -- but for a connect's TLS handshake, which
// before 3.4.10 it makes by waiting itself, up to the connect timeout. Before
// 3.4.10 it also reads TLS, on POSIX, through a socket it leaves blocking, so
// the layer makes the socket non-blocking for each of its calls, and blocking
// again after. libmysqlclient, from 8.0.16, makes only the connect so, and
// nothing else of this subsection (D_DB_UNSUPPORTED) -- its queries' are in
// mariadb_query.h -- and says nothing of what it waits for: its calls wait for
// the socket to be readable, and are made again at least every 10 milliseconds,
// so that a write the socket could not take at once goes on. It keeps no
// connect timeout in this mode, so the layer keeps it, in whole seconds: once
// it has passed, the connect fails with 2013, as libmysqlclient's blocking
// connect reports it. An older libmysqlclient makes none of them. A start may
// finish at once, every answer having come. A connection is made, and set up,
// as the blocking forms make it, and is the handle's from the start: a connect
// abandoned is closed with d_mariadb_close. Each first tidies as the blocking
// forms do -- closes a statement whose prepare failed, and those finalized
// during a run -- and turns off the multi-statement mode a failed script left
// on, which libmysqlclient can do only by waiting. Where the client cannot
// carry TLS so (d_mariadb_client_nonblocking_tls: Connector/C before 3.4.10
// with Schannel, or before 3.0.4), each start refuses a connection under TLS
// with D_DB_UNSUPPORTED, and a connect's start a connection that would ask
// the server for TLS -- the layer's default, PREFERRED, does -- the reason
// given where a message is wanted: such a connection is made with
// D_MARIADB_SSL_DISABLED and no key, certificate, authority, cipher,
// fingerprint or defaults file, or by blocking.
/**
 * @brief Starts a connection as d_mariadb_connect makes one, without
 *        waiting.
 *
 * @param[in]  _options  as for d_mariadb_connect.
 * @param[out] _db       receives the handle, its connection being made;
 *                       `NULL` on failure.
 * @param[out] _pending  receives what to wait for before continuing.
 * @return D_DB_WAIT; 0 should it be made at once; as d_mariadb_connect for a
 *         failure, `*_db` then `NULL`; D_DB_UNSUPPORTED for a client that
 *         cannot connect without blocking, or not over the TLS the
 *         connection would ask for; D_DB_INVALID_ARGUMENT for a `NULL` `_db`
 *         or `_pending`.
 * @post While the answer is D_DB_WAIT, the caller continues with
 *       d_mariadb_connect_continue, or abandons the connection with
 *       d_mariadb_close; on 0 the caller closes `*_db` with d_mariadb_close.
 */
int            d_mariadb_connect_start(const struct d_mariadb_options* _options,
                                       struct d_mariadb**              _db,
                                       struct d_db_pending*            _pending)
                                       D_NOTHROW;
/**
 * @brief Starts a connection as d_mariadb_connect_with makes one, without
 *        waiting.
 *
 * @param[in]  _options   as for d_mariadb_connect_with.
 * @param[in]  _settings  as for d_mariadb_connect_with.
 * @param[in]  _count     as for d_mariadb_connect_with.
 * @param[out] _db        receives the handle, its connection being made;
 *                        `NULL` on failure.
 * @param[out] _pending   receives what to wait for before continuing.
 * @param[out] _message   as for d_mariadb_connect_with.
 * @return as d_mariadb_connect_start, and as d_mariadb_connect_with for the
 *         settings.
 * @post As for d_mariadb_connect_start.
 */
int            d_mariadb_connect_with_start(
                   const struct d_mariadb_options* _options,
                   const struct d_mariadb_setting* _settings,
                   size_t                          _count,
                   struct d_mariadb**              _db,
                   struct d_db_pending*            _pending,
                   char**                          _message) D_NOTHROW;
/**
 * @brief Continues a connection a start of 1.8 began.
 *
 * @param[in,out] _db       the handle, its connection being made; `NULL`ed
 *                          when it fails, which closes it.
 * @param[in]     _events   what came, as d_db_wait.
 * @param[in,out] _pending  what the start or the last continuation wrote;
 *                          receives what to wait for next.
 * @param[out]    _message  as for d_mariadb_connect_with.
 * @return 0, the connection made and set up; D_DB_WAIT; as d_mariadb_connect
 *         for a failure; D_DB_INVALID_ARGUMENT for a `NULL` argument, or a
 *         handle with no connect under way.
 * @post On 0 the caller closes `*_db` with d_mariadb_close; on a failure
 *       `*_db` is `NULL`.
 */
int            d_mariadb_connect_continue(struct d_mariadb**   _db,
                                          unsigned             _events,
                                          struct d_db_pending* _pending,
                                          char**               _message)
                                          D_NOTHROW;
/**
 * @brief Starts signing the connection in as another user, as
 *        d_mariadb_change_user does, without waiting.
 *
 * @param[in]  _db        the handle.
 * @param[in]  _user      as for d_mariadb_change_user.
 * @param[in]  _password  as for d_mariadb_change_user.
 * @param[in]  _database  as for d_mariadb_change_user.
 * @param[out] _pending   receives what to wait for before continuing.
 * @return D_DB_WAIT; 0 should it finish at once; as d_mariadb_change_user;
 *         2014 while the connection is held; D_DB_UNSUPPORTED with
 *         libmysqlclient, and over TLS the client cannot carry so;
 *         D_DB_INVALID_ARGUMENT for a `NULL` handle or pending call.
 */
int            d_mariadb_change_user_start(struct d_mariadb*    _db,
                                           const char*          _user,
                                           const char*          _password,
                                           const char*          _database,
                                           struct d_db_pending* _pending)
                                           D_NOTHROW;
/**
 * @brief Gives the connection a fresh session, as
 *        d_mariadb_reset_connection does, without waiting: the reset, then
 *        the init commands and the character set, each in its turn.
 *
 * @param[in]  _db       the handle.
 * @param[out] _pending  receives what to wait for before continuing.
 * @return D_DB_WAIT; 0 should it finish at once; as
 *         d_mariadb_reset_connection; 2014 while the connection is held;
 *         D_DB_UNSUPPORTED with libmysqlclient, and over TLS the client
 *         cannot carry so, besides where the blocking form is;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 */
int            d_mariadb_reset_connection_start(struct d_mariadb*    _db,
                                                struct d_db_pending* _pending)
                                                D_NOTHROW;
/**
 * @brief Asks whether the server still answers, as d_mariadb_ping does,
 *        without waiting.
 *
 * @param[in]  _db       the handle.
 * @param[out] _pending  receives what to wait for before continuing.
 * @return D_DB_WAIT; as d_mariadb_ping; 2014 while the connection is held;
 *         D_DB_UNSUPPORTED with libmysqlclient, and over TLS the client
 *         cannot carry so; D_DB_INVALID_ARGUMENT for a `NULL` argument.
 */
int            d_mariadb_ping_start(struct d_mariadb*    _db,
                                    struct d_db_pending* _pending) D_NOTHROW;
// continuing -- each continues what its start of this subsection began,
// told what came, as d_db_wait, and given what the start or the last
// continuation wrote, which receives what to wait for next: D_DB_WAIT, or the
// answer the blocking form gives; D_DB_INVALID_ARGUMENT for a NULL argument,
// or a handle with no such call under way
int            d_mariadb_change_user_continue(struct d_mariadb*    _db,
                                              unsigned             _events,
                                              struct d_db_pending* _pending)
                                              D_NOTHROW;
int            d_mariadb_reset_connection_continue(
                   struct d_mariadb*    _db,
                   unsigned             _events,
                   struct d_db_pending* _pending) D_NOTHROW;
int            d_mariadb_ping_continue(struct d_mariadb*    _db,
                                       unsigned             _events,
                                       struct d_db_pending* _pending)
                                       D_NOTHROW;

// 1.9    Cancelling
//------------------------------------------------------------------------------
// The protocol has no cancel of its own: a statement is stopped by KILL QUERY
// with its connection's id, sent on another connection. d_mariadb_cancel makes
// one -- to the connection's host and port, or socket, as its user, with the
// layer's defaults (TLS PREFERRED, the connect timeout) and none of the
// connection's own settings -- runs KILL QUERY on it, and closes it. The
// statement, if it still runs, stops with 1317 (query execution was
// interrupted), which the connection's reply reports; one that has finished is
// not touched. Nor is one the server has not yet begun -- sent but not yet
// read, or waiting for a thread: MariaDB discards a KILL QUERY that reaches a
// connection between statements, so a request made the moment a statement is
// sent can miss it. A caller that must stop it waits until the server shows it
// running (information_schema.PROCESSLIST, its ID d_mariadb_connection_id) and
// asks then. _cancel_start and _cancel_continue do it without blocking, as 1.8
// connects -- with libmysqlclient too -- the request's connection a handle of
// its own until it is done, abandoned with d_mariadb_close. A user who cannot
// sign in again -- with a one-time password, say -- cannot cancel this way, and
// a connection whose settings the second one needs -- a certificate of its own
// -- is not reached the same way. Where the client cannot carry TLS without
// blocking (1.8), _cancel_start refuses a connection under TLS, and makes its
// own without TLS, as the connection was made.
/**
 * @brief Asks the server to stop what the connection runs, and waits until
 *        the request is done.
 *
 * @param[in]  _db       the connection whose statement to stop.
 * @param[out] _message  receives the reason when the request could not be
 *                       made, released with d_db_free; `NULL` otherwise;
 *                       may be `NULL`.
 * @return 0, the request made -- which says nothing of whether a statement
 *         stopped; the client's error number when the second connection
 *         could not be made, or KILL QUERY failed;
 *         D_DB_INVALID_ARGUMENT for a `NULL` connection.
 */
int            d_mariadb_cancel(struct d_mariadb* _db,
                                char**            _message) D_NOTHROW;
/**
 * @brief Starts asking the server to stop what the connection runs,
 *        without waiting.
 *
 * @param[in]  _db         the connection whose statement to stop.
 * @param[out] _canceller  receives the request's own connection; `NULL` once
 *                         the request is done, or on failure.
 * @param[out] _pending    receives what to wait for before continuing.
 * @param[out] _message    as for d_mariadb_cancel.
 * @return D_DB_WAIT; as d_mariadb_cancel; D_DB_UNSUPPORTED for a client that
 *         cannot connect without blocking, and for a connection under TLS
 *         the client cannot carry so; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 * @post While the answer is D_DB_WAIT, the caller continues with
 *       d_mariadb_cancel_continue, or abandons the request with
 *       d_mariadb_close.
 */
int            d_mariadb_cancel_start(struct d_mariadb*    _db,
                                      struct d_mariadb**   _canceller,
                                      struct d_db_pending* _pending,
                                      char**               _message)
                                      D_NOTHROW;
/**
 * @brief Continues a request d_mariadb_cancel_start began.
 *
 * @param[in,out] _canceller  the request's connection; closed, and
 *                            `NULL`ed, once the request is done or fails.
 * @param[in]     _events     what came, as d_db_wait.
 * @param[in,out] _pending    what the start or the last continuation
 *                            wrote; receives what to wait for next.
 * @param[out]    _message    as for d_mariadb_cancel.
 * @return as d_mariadb_cancel, and D_DB_WAIT; D_DB_INVALID_ARGUMENT for a
 *         `NULL` argument, or a handle with no request under way.
 */
int            d_mariadb_cancel_continue(struct d_mariadb**   _canceller,
                                         unsigned             _events,
                                         struct d_db_pending* _pending,
                                         char**               _message)
                                         D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_CONNECTION_H
