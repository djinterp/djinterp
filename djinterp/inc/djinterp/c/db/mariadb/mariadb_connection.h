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
* client libraries ask.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
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
    4.  Liveness
    5.  Rows changed
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
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
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
// reads (d_mariadb_setting).
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
    D_MARIADB_SETTING_NATIVE                // number and value: mysql_options's
                                            // option and argument, as given
};

// 1.1.3
// d_mariadb_ssl_mode
//   enum: what the connection asks of TLS. MySQL's client takes each as its
// --ssl-mode does. MariaDB's client has no switch for DISABLED, which then
// sets nothing -- that client uses TLS only when a key, a certificate or an
// authority is given -- and cannot verify the authority without the server's
// name as well, so VERIFY_CA is D_DB_UNSUPPORTED with it. With either, a mode
// that insists on TLS is checked once connected, and a connection without it
// is refused with 2026.
enum d_mariadb_ssl_mode
{
    D_MARIADB_SSL_PREFERRED = 0,     // the client's default: TLS when it can
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
 *         refuses one without a number; D_DB_UNSUPPORTED for an SSL_MODE the
 *         client cannot honour; D_DB_INVALID_ARGUMENT for a setting of no
 *         such name, one without the text or value it needs, a timeout past
 *         what the client takes, or `NULL` settings with a nonzero count.
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

// 1.4    Liveness
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

// 1.5    Rows changed
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


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_CONNECTION_H
