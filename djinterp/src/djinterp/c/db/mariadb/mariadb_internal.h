/*******************************************************************************
* djinterp [c]                                                mariadb_internal.h
*
* Private to the MariaDB layer's sources: mysql.h, the client floor, the
* handle's definition, and the plumbing every module repeats -- when the layer
* is built over a client. Built as stubs (D_INTERNAL_MARIADB 0), it declares
* nothing, and env includes no mysql.h.
*   mysql.h arrives through env_mariadb.h, which honours
* D_CFG_ENV_MARIADB_C_PATH and finds Connector/C's header or
* libmysqlclient's; cfg_mariadb_common.h has already turned
* D_CFG_ENV_USING_MARIADB on, for a source that defined
* D_INTERNAL_MARIADB_SOURCE before its first include. The layer reads only
* what the two clients
* share, and the floor is checked here once, so a source compiled against an
* older client stops with one clear message.
*   The plumbing keeps one rule the public contract depends on: after a
* statement fails, nothing else runs on the connection before control returns,
* so mysql_error still describes the failure. A helper that runs more than one
* statement stops at the first that fails.
*   Names reach SQL in backquotes, any backquote doubled; values are escaped
* by the client, for the connection's character set and SQL mode; SQL the
* caller wrote is used as written. What a database holds is read from
* information_schema, whose tables both servers have kept the same since
* MySQL 5.7 and MariaDB 10.3.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  REQUIREMENTS
    ------------
    1.  Client floor
    2.  Compile-time API choices
         1.  D_INTERNAL_MARIADB_ESCAPE_QUOTE
         2.  D_INTERNAL_MARIADB_LENGTH_MAX
         3.  d_internal_mariadb_flag
         4.  D_INTERNAL_MARIADB_SSL_SWITCHES
         5.  D_INTERNAL_MARIADB_RESET
         6.  D_INTERNAL_MARIADB_SESSION_TRACK
         7.  D_INTERNAL_MARIADB_QUERY_ATTRIBUTES
         8.  D_INTERNAL_MARIADB_READS_SIMPLE_OK
         9.  D_INTERNAL_MARIADB_MAX_PACKET
         10. D_INTERNAL_MARIADB_TLS_VERSION
         11. D_INTERNAL_MARIADB_TLS_CIPHERSUITES
         12. D_INTERNAL_MARIADB_CLEARTEXT_SWITCH
         13. D_INTERNAL_MARIADB_GET_OPTION
         14. D_INTERNAL_MARIADB_PUBLIC_KEY_ASK
         15. D_INTERNAL_MARIADB_ASYNC
         16. D_INTERNAL_MARIADB_ASYNC_POLL_MS
         17. D_INTERNAL_MARIADB_TLS_LIBRARY
         18. D_INTERNAL_MARIADB_ASYNC_TLS_SINCE
         19. D_INTERNAL_MARIADB_ASYNC_ANY_TLS_SINCE
2.  HANDLES
    -------
    1.  The handle
         1.  D_INTERNAL_MARIADB_CHARSET_SIZE
         2.  d_internal_mariadb_attribute
         3.  d_internal_mariadb_step
         4.  d_internal_mariadb_async
         5.  d_mariadb
    2.  Result codes
         1.  D_INTERNAL_MARIADB_UNKNOWN_ERROR
         2.  D_INTERNAL_MARIADB_OUT_OF_MEMORY
         3.  D_INTERNAL_MARIADB_INVALID_PARAMETER
         4.  D_INTERNAL_MARIADB_SSL_ERROR
         5.  D_INTERNAL_MARIADB_OUT_OF_SYNC
         6.  D_INTERNAL_MARIADB_SERVER_LOST
    3.  Server facts
    4.  Replies
    5.  Query attributes
    6.  Statements
    7.  Calls that do not block
3.  NAMES
    -----
    1.  Checks
    2.  Quoting
4.  STATEMENTS
    ----------
    1.  Running
    2.  Single values
    3.  Rows
5.  CATALOGUE
    ---------
    1.  Objects
         1.  d_internal_mariadb_object
         2.  d_internal_mariadb_path
    2.  Lookups
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_INTERNAL_H
#define DJINTERP_C_DB_MARIADB_MARIADB_INTERNAL_H 1

// std
#include <limits.h>  // ULONG_MAX
#include <stdarg.h>  // va_list
#include <stddef.h>  // size_t
#include <time.h>    // time_t
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_common.h"  // layer
#include "../../../../../inc/djinterp/env/db/mariadb/env_mariadb.h"  // mysql.h
#include "../db_internal.h"  // d_internal_db_buffer, the builders
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MARIADB == 1)


//==============================================================================
// 1.  REQUIREMENTS
//==============================================================================


// 1.1    Client floor
//------------------------------------------------------------------------------
#if !D_CFG_IS_ON(D_CFG_ENV_USING_MARIADB)
    #error "D_CFG_ENV_USING_MARIADB is off: a layer source defines "           \
           "D_INTERNAL_MARIADB_SOURCE before its first include"
#elif !D_ENV_MYSQL_COMMON_FAMILY_DETECTED
    #error "the MariaDB layer needs mysql.h; see D_CFG_ENV_MARIADB_C_PATH"
#elif (MYSQL_VERSION_ID < D_MARIADB_FLOOR_CLIENT_VERSION_ID)
    #error "the MariaDB layer needs libmysqlclient 5.7, or a MariaDB client"
#endif

// 1.2    Compile-time API choices
//------------------------------------------------------------------------------
// 1.2.1
// D_INTERNAL_MARIADB_ESCAPE_QUOTE
//   brief: 1 where values are escaped with mysql_real_escape_string_quote --
// libmysqlclient from 5.7.6, whose plain mysql_real_escape_string fails
// outright under the NO_BACKSLASH_ESCAPES SQL mode -- and 0 where they are
// escaped with the plain call, which MariaDB's clients adapt to that mode
// themselves. Chosen here, so that no function body needs a conditional.
#if ( (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) &&                                  \
      (MYSQL_VERSION_ID >= 50706) )
    #define D_INTERNAL_MARIADB_ESCAPE_QUOTE 1
#else
    #define D_INTERNAL_MARIADB_ESCAPE_QUOTE 0
#endif

// 1.2.2
// D_INTERNAL_MARIADB_LENGTH_MAX
//   constant: the longest text the client's unsigned long lengths can carry,
// as a size_t: 4 GiB less a byte where unsigned long is 32 bits, as on 64-bit
// Windows, and any length elsewhere.
#if (ULONG_MAX < SIZE_MAX)
    #define D_INTERNAL_MARIADB_LENGTH_MAX ((size_t)ULONG_MAX)
#else
    #define D_INTERNAL_MARIADB_LENGTH_MAX SIZE_MAX
#endif

// 1.2.3
// d_internal_mariadb_flag
//   type: the client's one-byte truth value, as MYSQL_BIND points at it and
// mysql_stmt_attr_set reads it: bool in libmysqlclient from 8.0, which
// dropped the name my_bool, and my_bool, a char, in every other client.
#if ( (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) &&                                  \
      (MYSQL_VERSION_ID >= 80000) )
    typedef bool    d_internal_mariadb_flag;
#else
    typedef my_bool d_internal_mariadb_flag;
#endif

// 1.2.4
// D_INTERNAL_MARIADB_SSL_SWITCHES
//   brief: how the client is told what TLS to insist on: 1 for
// MYSQL_OPT_SSL_MODE, libmysqlclient's from 5.7.11; 2 for
// MYSQL_OPT_SSL_ENFORCE and MYSQL_OPT_SSL_VERIFY_SERVER_CERT, Connector/C's;
// 0 for neither, an older libmysqlclient, with which only the client's
// default can be had.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #define D_INTERNAL_MARIADB_SSL_SWITCHES 2
#elif (MYSQL_VERSION_ID >= 50711)
    #define D_INTERNAL_MARIADB_SSL_SWITCHES 1
#else
    #define D_INTERNAL_MARIADB_SSL_SWITCHES 0
#endif

// 1.2.5
// D_INTERNAL_MARIADB_RESET
//   brief: 1 where the client has mysql_reset_connection: libmysqlclient from
// 5.7.3, and Connector/C from 3.0 (MARIADB_PACKAGE_VERSION_ID, which older
// MariaDB clients do not define, and which reads as 0 there).
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30000)
        #define D_INTERNAL_MARIADB_RESET 1
    #else
        #define D_INTERNAL_MARIADB_RESET 0
    #endif
#elif (MYSQL_VERSION_ID >= 50703)
    #define D_INTERNAL_MARIADB_RESET 1
#else
    #define D_INTERNAL_MARIADB_RESET 0
#endif

// 1.2.6
// D_INTERNAL_MARIADB_SESSION_TRACK
//   brief: 1 where the client has mysql_session_track_get_first and _next:
// libmysqlclient from 5.7.4, and Connector/C from 3.0.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30000)
        #define D_INTERNAL_MARIADB_SESSION_TRACK 1
    #else
        #define D_INTERNAL_MARIADB_SESSION_TRACK 0
    #endif
#elif (MYSQL_VERSION_ID >= 50704)
    #define D_INTERNAL_MARIADB_SESSION_TRACK 1
#else
    #define D_INTERNAL_MARIADB_SESSION_TRACK 0
#endif

// 1.2.7
// D_INTERNAL_MARIADB_QUERY_ATTRIBUTES
//   brief: 1 where the client has mysql_bind_param, which sends attributes
// with the next query: libmysqlclient from 8.0.23. Connector/C has none.
#if ( (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) &&                                  \
      (MYSQL_VERSION_ID >= 80023) )
    #define D_INTERNAL_MARIADB_QUERY_ATTRIBUTES 1
#else
    #define D_INTERNAL_MARIADB_QUERY_ATTRIBUTES 0
#endif

// 1.2.8
// D_INTERNAL_MARIADB_READS_SIMPLE_OK
//   brief: 1 where the client reads the OK packet that answers a command
// without a statement -- a ping, a change of database, a reset -- for the
// server's status and the session's changes, as libmysqlclient does; 0 for
// Connector/C, which only checks it is no error, so that both stay as an
// earlier reply left them.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #define D_INTERNAL_MARIADB_READS_SIMPLE_OK 0
#else
    #define D_INTERNAL_MARIADB_READS_SIMPLE_OK 1
#endif

// 1.2.9
// D_INTERNAL_MARIADB_MAX_PACKET
//   brief: how the client is told the largest packet it takes, its
// MYSQL_OPT_MAX_ALLOWED_PACKET: 1 for libmysqlclient, which reads an unsigned
// long; 2 for Connector/C from 3.0, which reads a size_t -- the two differ
// where unsigned long is 32 bits, as on 64-bit Windows; 0 for an older
// MariaDB client, which has no such option.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30000)
        #define D_INTERNAL_MARIADB_MAX_PACKET 2
    #else
        #define D_INTERNAL_MARIADB_MAX_PACKET 0
    #endif
#else
    #define D_INTERNAL_MARIADB_MAX_PACKET 1
#endif

// 1.2.10
// D_INTERNAL_MARIADB_TLS_VERSION
//   brief: the client's option for the TLS versions a connection may use, as
// a list ("TLSv1.2,TLSv1.3"): 1 for libmysqlclient's MYSQL_OPT_TLS_VERSION,
// from 5.7.10, which refuses a list it cannot read; 2 for Connector/C's
// MARIADB_OPT_TLS_VERSION, from 3.0, which passes over a version it does not
// know; 0 for a client without either.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30000)
        #define D_INTERNAL_MARIADB_TLS_VERSION 2
    #else
        #define D_INTERNAL_MARIADB_TLS_VERSION 0
    #endif
#elif (MYSQL_VERSION_ID >= 50710)
    #define D_INTERNAL_MARIADB_TLS_VERSION 1
#else
    #define D_INTERNAL_MARIADB_TLS_VERSION 0
#endif

// 1.2.11
// D_INTERNAL_MARIADB_TLS_CIPHERSUITES
//   brief: 1 where the client has an option for TLS 1.3's cipher suites,
// MYSQL_OPT_TLS_CIPHERSUITES: libmysqlclient from 8.0.16. Connector/C has
// only one cipher option, whose text it reads by its TLS library's rules.
#if ( (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) &&                                  \
      (MYSQL_VERSION_ID >= 80016) )
    #define D_INTERNAL_MARIADB_TLS_CIPHERSUITES 1
#else
    #define D_INTERNAL_MARIADB_TLS_CIPHERSUITES 0
#endif

// 1.2.12
// D_INTERNAL_MARIADB_CLEARTEXT_SWITCH
//   brief: 1 where the client sends a password in clear text, when the
// server's authentication asks for it, only once told it may --
// libmysqlclient's MYSQL_ENABLE_CLEARTEXT_PLUGIN; 0 for MariaDB's clients,
// which send it unasked: Connector/C takes the option and does nothing with
// it.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #define D_INTERNAL_MARIADB_CLEARTEXT_SWITCH 0
#else
    #define D_INTERNAL_MARIADB_CLEARTEXT_SWITCH 1
#endif

// 1.2.13
// D_INTERNAL_MARIADB_GET_OPTION
//   brief: 1 where the client has mysql_get_option, which reads an option
// back: libmysqlclient from 5.7.3, and Connector/C from 3.0.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30000)
        #define D_INTERNAL_MARIADB_GET_OPTION 1
    #else
        #define D_INTERNAL_MARIADB_GET_OPTION 0
    #endif
#elif (MYSQL_VERSION_ID >= 50703)
    #define D_INTERNAL_MARIADB_GET_OPTION 1
#else
    #define D_INTERNAL_MARIADB_GET_OPTION 0
#endif

// 1.2.14
// D_INTERNAL_MARIADB_PUBLIC_KEY_ASK
//   brief: how the client comes by the server's RSA public key where no file
// gives it: 1, asking when told to (MYSQL_OPT_GET_SERVER_PUBLIC_KEY),
// libmysqlclient from 8.0.3; 2, always asking, Connector/C; 0, never,
// libmysqlclient before 8.0.3.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #define D_INTERNAL_MARIADB_PUBLIC_KEY_ASK 2
#elif (MYSQL_VERSION_ID >= 80003)
    #define D_INTERNAL_MARIADB_PUBLIC_KEY_ASK 1
#else
    #define D_INTERNAL_MARIADB_PUBLIC_KEY_ASK 0
#endif

// 1.2.15
// D_INTERNAL_MARIADB_ASYNC
//   brief: the client's calls that do not block (db_common.h 1.3): 2 for
// MariaDB's, a _start and a _cont for each call, which say what they wait
// for; 1 for libmysqlclient's _nonblocking calls, from 8.0.16, called again
// with the same arguments until done, which say nothing of what they wait
// for and cover connecting, queries and their results alone; 0 for none.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #define D_INTERNAL_MARIADB_ASYNC 2
#elif (MYSQL_VERSION_ID >= 80016)
    #define D_INTERNAL_MARIADB_ASYNC 1
#else
    #define D_INTERNAL_MARIADB_ASYNC 0
#endif

// 1.2.16
// D_INTERNAL_MARIADB_ASYNC_POLL_MS
//   constant: how long a libmysqlclient call that does not block waits on its
// socket for reading before it is called again regardless, in milliseconds.
// The client never says it waits to write, and waiting for the socket to be
// writable -- which it nearly always is -- would spin; the server answers
// what it is sent, so a call waits to read, and a write the socket could not
// take at once goes on at least this often.
#define D_INTERNAL_MARIADB_ASYNC_POLL_MS 10u

// 1.2.17
// D_INTERNAL_MARIADB_TLS_LIBRARY
//   brief: 1 where the client names its TLS library at run time
// (mariadb_get_infov's MARIADB_TLS_LIBRARY) and reads its options back with
// mysql_get_option: Connector/C from 3.0, the first with Schannel, Windows'
// own TLS; 0 for every other client, none of which has Schannel.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #if (MARIADB_PACKAGE_VERSION_ID >= 30000)
        #define D_INTERNAL_MARIADB_TLS_LIBRARY 1
    #else
        #define D_INTERNAL_MARIADB_TLS_LIBRARY 0
    #endif
#else
    #define D_INTERNAL_MARIADB_TLS_LIBRARY 0
#endif

// 1.2.18
// D_INTERNAL_MARIADB_ASYNC_TLS_SINCE
//   constant: the first Connector/C, as mysql_get_client_version numbers it,
// whose calls that do not block go through Schannel: 3.4.10. Before it,
// ma_pvio_read and ma_pvio_write pass a connection's TLS by when a call does
// not block, where the library is Schannel (HAVE_SCHANNEL) -- 3.3 throughout,
// to 3.3.21 -- and the first packet read under TLS, the server's answer to
// the login, fails with 2013.
#define D_INTERNAL_MARIADB_ASYNC_TLS_SINCE 30410ul

// 1.2.19
// D_INTERNAL_MARIADB_ASYNC_ANY_TLS_SINCE
//   constant: the first Connector/C whose calls that do not block go through
// any TLS library at all: 3.0.4, which added ma_tls_read_async and
// ma_tls_write_async for OpenSSL and GnuTLS. From 3.0.0 to 3.0.3 every
// such call passes TLS by.
#define D_INTERNAL_MARIADB_ASYNC_ANY_TLS_SINCE 30004ul


//==============================================================================
// 2.  HANDLES
//==============================================================================


// 2.1    The handle
//------------------------------------------------------------------------------
// 2.1.1
// D_INTERNAL_MARIADB_CHARSET_SIZE
//   constant: room for a character set's name and its terminator; the
// clients' longest is 32 bytes (MY_CS_NAME_SIZE).
#define D_INTERNAL_MARIADB_CHARSET_SIZE 64u

// 2.1.2
// d_internal_mariadb_attribute
//   struct: a query attribute the next query sends (mariadb_query.h): the
// layer's copies, which the client's binds point into until it is sent.
struct d_internal_mariadb_attribute
{
    char*         name;    // the attribute's name, terminated
    char*         value;   // its value, terminated
    unsigned long length;  // the value's bytes
};

// 2.1.3
// d_internal_mariadb_step
//   enum: the client calls a call that does not block is made of, each
// begun once and continued until done (2.7).
enum d_internal_mariadb_step
{
    D_INTERNAL_MARIADB_STEP_NONE = 0,   // no client call under way
    D_INTERNAL_MARIADB_STEP_CONNECT,    // mysql_real_connect
    D_INTERNAL_MARIADB_STEP_QUERY,      // mysql_real_query
    D_INTERNAL_MARIADB_STEP_SEND,       // mysql_send_query: MariaDB's
                                        // client alone
    D_INTERNAL_MARIADB_STEP_READ,       // mysql_read_query_result: MariaDB's
                                        // client alone
    D_INTERNAL_MARIADB_STEP_STORE,      // mysql_store_result
    D_INTERNAL_MARIADB_STEP_NEXT,       // mysql_next_result
    D_INTERNAL_MARIADB_STEP_OPTION,     // mysql_set_server_option, the
                                        // multi-statement mode off
    D_INTERNAL_MARIADB_STEP_PING,       // mysql_ping
    D_INTERNAL_MARIADB_STEP_RESET,      // mysql_reset_connection
    D_INTERNAL_MARIADB_STEP_USER,       // mysql_change_user
    D_INTERNAL_MARIADB_STEP_CHARSET,    // mysql_set_character_set
    D_INTERNAL_MARIADB_STEP_PREPARE,    // mysql_stmt_prepare
    D_INTERNAL_MARIADB_STEP_EXECUTE,    // mysql_stmt_execute
    D_INTERNAL_MARIADB_STEP_STMT_STORE, // mysql_stmt_store_result
    D_INTERNAL_MARIADB_STEP_STMT_NEXT,  // mysql_stmt_next_result
    D_INTERNAL_MARIADB_STEP_STMT_FREE,  // mysql_stmt_free_result
    D_INTERNAL_MARIADB_STEP_FETCH       // mysql_stmt_fetch
};

// 2.1.4
// d_internal_mariadb_async
//   struct: a call that does not block, under way on a connection: the
// operation the program began, where it is, the client call it waits on,
// what that call answered, and the copies of whatever the client reads again
// while it waits. Made the first time a connection needs one.
struct d_internal_mariadb_async
{
    int                        (*advance)(struct d_mariadb*    _db,
                                          struct d_db_pending* _pending);
                                            // the operation's next stage,
                                            // or NULL when none is under
                                            // way
    int                          stage;     // where the operation is
    int                          sub;       // where a statement it runs to
                                            // its end is (2.7)
    int                          failure;   // its first failure, or 0
    enum d_internal_mariadb_step step;      // the client call under way
    int                          status;    // MariaDB's client: what that
                                            // call waits for, MYSQL_WAIT_*
    int                          ret;       // its answer, an int
    d_internal_mariadb_flag      flag;      // its answer, a truth value
    MYSQL*                       made;      // mysql_real_connect's answer
    MYSQL_RES*                   result;    // mysql_store_result's answer
    MYSQL_STMT*                  stmt;      // the client statement a
                                            // statement's call is on
    struct d_mariadb_statement*  statement; // the layer's statement whose
                                            // call it is; NULL for none
    char*                        text;      // SQL being sent, the layer's
                                            // copy
    unsigned long                length;    // its length
    char*                        host;      // a connect's, or a change of
    char*                        user;      // user's, arguments: copies,
    char*                        password;  // which libmysqlclient reads
    char*                        database;  // again each time it is called
    char*                        socket;
    unsigned                     port;
    unsigned long                flags;     // a connect's client flags
    bool                         tls;       // a connect insists on TLS
    time_t                       deadline;  // libmysqlclient: when a
                                            // connect's timeout expires,
                                            // which the layer keeps; 0 for
                                            // none
    bool                         results;   // SQL sent whose results have
                                            // still to be read to the end
    bool                         given;     // the current result given
    bool                         unread;    // the first statement's answer
                                            // still to read: MariaDB's
                                            // client sends alone
    int                          deferred;  // the first statement's
                                            // failure, read with the send
                                            // by libmysqlclient, for the
                                            // first result to give
    struct d_db_rows*            rows;      // where the call under way
                                            // gives rows
    int64_t*                     changes;   // and the count
    int                          mode;      // what a statement's run gives:
                                            // mariadb_statement.c's
    int64_t                      count;     // the count it read
    size_t                       max;       // the most rows a page takes
    size_t                       fetched;   // the rows it has
    struct d_internal_db_rows    page;      // the rows a statement's call
                                            // has read, kept between waits
    size_t                       command;   // the init command a reset runs
                                            // next
    char                         charset[D_INTERNAL_MARIADB_CHARSET_SIZE];
                                            // SET NAMES's argument
};

// 2.1.5
// d_mariadb
//   struct: the definition mariadb_common.h declares: the client's
// connection, and what the layer has learned of the server it reached.
struct d_mariadb
{
    MYSQL*                native;            // the client's connection
    bool                  owned;             // closed along with the handle
    bool                  multi_statements;  // left on by a script that
                                             // failed, so that its error
                                             // still describes it
    bool                  tracked;           // the client's record of the
                                             // session's changes is the
                                             // reply to the layer's last
                                             // command (2.4)
    enum d_mariadb_flavor flavor;            // the server's, by its version
                                             // string
    unsigned long         version;           // the server's true version,
                                             // MAJOR*10000 + MINOR*100 +
                                             // PATCH
    int                   case_rule;         // lower_case_table_names; -1
                                             // until it is first needed
    MYSQL_STMT*           failed;            // a statement whose prepare
                                             // failed, closed when the
                                             // layer next runs a statement:
                                             // closing it clears the error
                                             // the caller is about to read
    char                  charset[D_INTERNAL_MARIADB_CHARSET_SIZE];
                                             // the character set the
                                             // session began with, at
                                             // connecting or changing user,
                                             // which a reset returns to; ""
                                             // when the client named none,
                                             // or one too long to keep
    bool                  charset_named;     // a character set named for the
                                             // connect -- the layer's, or one
                                             // a native setting of
                                             // MYSQL_SET_CHARSET_NAME gave in
                                             // its place -- which the session
                                             // is given where the handshake
                                             // loses it
    char*                 init_commands;     // the init commands the layer's
                                             // connect gave the client, each
                                             // terminated, one after another,
                                             // for a reset to run again; NULL
                                             // for none
    size_t                init_count;        // how many
    struct d_internal_mariadb_attribute*
                          attributes;        // the next query's attributes;
                                             // NULL for none
    size_t                attribute_count;   // how many
    struct d_mariadb_statement*
                          statements;        // the statements prepared on
                                             // it, which closing it leaves
                                             // only to be finalized (2.6)
    struct d_mariadb_statement*
                          cursor;            // the statement whose run holds
                                             // the connection, its rows still
                                             // arriving; NULL for none
    struct d_mariadb_statement*
                          retired;           // statements finalized while a
                                             // run held the connection, the
                                             // server told once it is free
    struct d_internal_mariadb_async*
                          async;             // the call that does not
                                             // block, under way or last
                                             // made; NULL until one is
};

// 2.2    Result codes
//------------------------------------------------------------------------------
// The client's own numbers for conditions the layer can meet itself
// (errmsg.h, which mysql.h does not include, names them CR_UNKNOWN_ERROR and
// CR_OUT_OF_MEMORY). Returned by the layer, they leave the client's error
// state as it was.

// 2.2.1
// D_INTERNAL_MARIADB_UNKNOWN_ERROR
//   constant: a failure the server reported in a result row rather than with
// a number of its own.
#define D_INTERNAL_MARIADB_UNKNOWN_ERROR 2000

// 2.2.2
// D_INTERNAL_MARIADB_OUT_OF_MEMORY
//   constant: memory ran out in the layer.
#define D_INTERNAL_MARIADB_OUT_OF_MEMORY 2008

// 2.2.3
// D_INTERNAL_MARIADB_INVALID_PARAMETER
//   constant: a parameter number the statement does not have
// (CR_INVALID_PARAMETER_NO).
#define D_INTERNAL_MARIADB_INVALID_PARAMETER 2034

// 2.2.4
// D_INTERNAL_MARIADB_SSL_ERROR
//   constant: a TLS connection that could not be made, though one was
// required (CR_SSL_CONNECTION_ERROR).
#define D_INTERNAL_MARIADB_SSL_ERROR 2026

// 2.2.5
// D_INTERNAL_MARIADB_OUT_OF_SYNC
//   constant: a command refused because a statement's run holds the
// connection, its rows still arriving (CR_COMMANDS_OUT_OF_SYNC, the client's
// own number for a command sent while a result is unread).
#define D_INTERNAL_MARIADB_OUT_OF_SYNC 2014

// 2.2.6
// D_INTERNAL_MARIADB_SERVER_LOST
//   constant: a connect whose server did not answer within its timeout, as
// the client's blocking connect reports it (CR_SERVER_LOST): the layer's to
// report for libmysqlclient's calls that do not block, which keep no timeout.
#define D_INTERNAL_MARIADB_SERVER_LOST 2013

// 2.3    Server facts
//------------------------------------------------------------------------------
// server facts -- _at_least answers whether the server is of the flavor
// named, at the version given (MAJOR*10000 + MINOR*100 + PATCH) or later;
// _case_rule reads the server's lower_case_table_names once, and the copy the
// handle keeps after that: 0 and the rule, or the client's error number for
// the query that failed
bool d_internal_mariadb_at_least(const struct d_mariadb* _db,
                                 enum d_mariadb_flavor   _flavor,
                                 unsigned long           _version);
int  d_internal_mariadb_case_rule(struct d_mariadb* _db,
                                  int*              _rule);

// 2.4    Replies
//------------------------------------------------------------------------------
// replies -- the server reports what a command changed in the session with
// the OK packet that ends its reply (SERVER_SESSION_STATE_CHANGED), and the
// client keeps the latest it read, until it reads another: Connector/C even
// past replies that carry none -- an error, rows ended by EOF, a ping -- so
// that its record would describe an earlier command. Every command the layer
// sends records whether its reply carried the changes: _replied with `_ok`
// for a reply that ended in an OK packet, which carried them where the
// server says so; _rows_end_ok answers whether a result's rows end in one,
// as they do where the client and the server agreed on CLIENT_DEPRECATE_EOF
// (libmysqlclient from 5.7.5), and never through Connector/C
void d_internal_mariadb_replied(struct d_mariadb* _db,
                                bool              _ok);
bool d_internal_mariadb_rows_end_ok(const struct d_mariadb* _db);

// 2.5    Query attributes
//------------------------------------------------------------------------------
// query attributes -- _sent follows a query the layer sent, whatever became
// of it: the client dropped the attributes it bound (and is told to, so that
// a query that never left drops them too), and the layer frees its copies;
// _release frees the copies alone, for a reset, a change of user and a
// close, which drop the client's binds themselves
void d_internal_mariadb_attributes_sent(struct d_mariadb* _db);
void d_internal_mariadb_attributes_release(struct d_mariadb* _db);

// 2.6    Statements
//------------------------------------------------------------------------------
// statements -- a statement's run that d_mariadb_open_cursor began holds the
// connection until it ends, the server still sending its rows, and nothing
// else may be sent meanwhile: a client sending a command then either refuses
// it or reads the run's rows away as it goes, so the layer refuses it first.
// _idle answers 0, or D_INTERNAL_MARIADB_OUT_OF_SYNC while a run holds the
// connection, for every function that sends a command; _close_retired closes
// the statements finalized while one did, on a connection free again;
// _detach, for a close, ends a run still open -- reading the rest of it only
// where `_read_rest` says, since a connection about to close needs nothing
// read -- and leaves every statement prepared on the connection without it,
// only to be finalized (mariadb_statement.c)
int  d_internal_mariadb_idle(const struct d_mariadb* _db);
void d_internal_mariadb_close_retired(struct d_mariadb* _db);
void d_internal_mariadb_detach(struct d_mariadb* _db,
                               bool              _read_rest);

// 2.7    Calls that do not block
//------------------------------------------------------------------------------
// A call that does not block is an operation -- a connect, SQL sent, its
// results read, a statement prepared or run -- made of client calls that do
// not block, steps, one after another. The operation keeps its place in the
// handle's record (2.1.4), and its advance function, given the last step's
// answers, begins the next step or ends the operation; d_internal_mariadb_
// drive continues the step under way, as the socket allows, and advances,
// until a step must wait or the operation is done. While one is under way
// the connection is held: _idle and _ready answer D_INTERNAL_MARIADB_OUT_OF_
// SYNC for every other command, as for a statement's run.
//   _async_get gives the record, made on first use -- with MariaDB's client
// its non-blocking context too, which a connection the layer connected
// without one gains then -- or NULL when memory ran out; _async_free
// releases it, for a close. _busy answers whether an operation is under
// way, or SQL sent has results still to read. _begin starts an operation,
// given its advance function, and drives it;
// _drive continues one; both answer D_DB_WAIT, or the operation's result,
// which leaves the record idle and `_pending` settled. _step begins a step:
// D_DB_WAIT while it waits, `_pending` then saying for what; 0 when it
// finished at once, its answers in the record; D_DB_UNSUPPORTED for a step
// the client cannot make without blocking. _unpend leaves a pending call
// as a finished one leaves it. _keep_text copies text into the record, NULL
// kept as NULL; false when memory ran out.
struct d_internal_mariadb_async*
     d_internal_mariadb_async_get(struct d_mariadb* _db);
void d_internal_mariadb_async_free(struct d_mariadb* _db);
bool d_internal_mariadb_busy(const struct d_mariadb* _db);
int  d_internal_mariadb_begin(struct d_mariadb*    _db,
                              int                (*_advance)(
                                                     struct d_mariadb*,
                                                     struct d_db_pending*),
                              struct d_db_pending* _pending);
int  d_internal_mariadb_drive(struct d_mariadb*    _db,
                              unsigned             _events,
                              struct d_db_pending* _pending);
int  d_internal_mariadb_step(struct d_mariadb*            _db,
                             enum d_internal_mariadb_step _step,
                             struct d_db_pending*         _pending);
void d_internal_mariadb_unpend(struct d_db_pending* _pending);
bool d_internal_mariadb_keep_text(char**      _to,
                                  const char* _text);
// TLS a client cannot carry without blocking (d_mariadb_client_
// nonblocking_tls false) -- _tls_blocks answers whether the connection uses
// TLS so, which every start but a connect's refuses with D_DB_UNSUPPORTED;
// _tls_blocks_connect whether a connection set up, not yet made, would ask
// the server for TLS so, which a connect's start refuses: Connector/C asks
// with its switch on, the server's certificate to be checked, or a key, a
// certificate, an authority, a cipher or a fingerprint given, and an option
// it cannot report, or a defaults file it reads while connecting, counts as
// asking. Neither fails; both are false with a client that can.
bool d_internal_mariadb_tls_blocks(const struct d_mariadb* _db);
bool d_internal_mariadb_tls_blocks_connect(MYSQL* _native);
// before an operation (mariadb_connection.c) -- _tidy does what
// d_internal_mariadb_ready does before a command, without blocking: 2014
// while a run or an operation holds the connection, else 0, the statement
// whose prepare failed and those retired closed, and with libmysqlclient
// the multi-statement mode turned off, waiting, since it has no step for
// it. _mode_off turns that mode off as a step where a failed script left it
// on: stage `_stage` begins it and `_stage` + 1 reads its answer; it
// answers D_DB_WAIT, 0 once off or never on, or the client's error number.
int  d_internal_mariadb_tidy(struct d_mariadb* _db);
int  d_internal_mariadb_mode_off(struct d_mariadb*    _db,
                                 int                  _stage,
                                 struct d_db_pending* _pending);
/**
 * @brief Runs SQL as d_internal_mariadb_send does, without blocking: the
 *        record's text sent, then every result it produces read and freed.
 *
 * @param[in,out] _db       the connection, an operation under way, its
 *                          record's text the SQL and `sub` 0 to begin.
 * @param[in,out] _pending  receives what to wait for.
 * @return D_DB_WAIT; 0, every result read; or the client's error number
 *         for the statement that failed.
 *
 * An operation's stage calls it again until it answers anything but
 * D_DB_WAIT; the query attributes set for the next query go with it, and
 * its last result's end records whether the reply carried the session's
 * changes, as for any statement the layer runs.
 */
int  d_internal_mariadb_run_async(struct d_mariadb*    _db,
                                  struct d_db_pending* _pending);


//==============================================================================
// 3.  NAMES
//==============================================================================


// 3.1    Checks
//------------------------------------------------------------------------------
// checks -- none of these allocate or fail. _name_ok is false for a NULL or
// empty name; _schema_ok for an empty one, NULL meaning the current database;
// _variable_ok for anything but a letter or underscore followed by letters,
// digits and underscores, the shape of every system variable's name
bool d_internal_mariadb_name_ok(const char* _name);
bool d_internal_mariadb_schema_ok(const char* _schema);
bool d_internal_mariadb_variable_ok(const char* _name);

// 3.2    Quoting
//------------------------------------------------------------------------------
// quoting -- d_internal_db_quote_fn's for the servers: _quote_name writes a
// name in backquotes, any backquote doubled, and needs no connection;
// _quote_value writes a value in single quotes, escaped by the client for the
// connection `_context` points to. Each fails only for want of memory,
// leaving the buffer's length as it was. _quoting gathers both, for
// d_internal_db_format.
bool d_internal_mariadb_quote_name(void*                        _context,
                                   struct d_internal_db_buffer* _out,
                                   const char*                  _name);
bool d_internal_mariadb_quote_value(void*                        _context,
                                    struct d_internal_db_buffer* _out,
                                    const char*                  _text);
struct d_internal_db_quoting
     d_internal_mariadb_quoting(struct d_mariadb* _db);


//==============================================================================
// 4.  STATEMENTS
//==============================================================================
// Formats are d_internal_db_format's: %Q a quoted value, %I a quoted name,
// %T a qualified one, %s text as written. Each returns 0, the client's error
// number for the statement that failed, D_INTERNAL_MARIADB_OUT_OF_MEMORY when
// the text could not be built, or D_DB_INVALID_ARGUMENT for a format it could
// not read, or a value it could not write (NaN, say).


// 4.1    Running
//------------------------------------------------------------------------------
// the layer's result for how formatting ended: 0, or
// D_INTERNAL_MARIADB_OUT_OF_MEMORY, or D_DB_INVALID_ARGUMENT for a format or
// value the formatter would not write
int  d_internal_mariadb_format_result(enum d_internal_db_format_status _status);
/**
 * @brief Readies the connection for the layer's next statement, undoing what
 *        a failure left so that its error would still read: closes a
 *        statement whose prepare failed, and those finalized while a run
 *        held the connection, and turns off the multi-statement mode a
 *        failed script left on.
 *
 * @param[in] _db  the connection.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_SYNC while a statement's run holds
 *         the connection, having done nothing; or the client's error number
 *         when the mode could not be turned off.
 */
int  d_internal_mariadb_ready(struct d_mariadb* _db);
/**
 * @brief Sends SQL as written and reads every result it produces.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _sql     the SQL; several statements only while the client's
 *                      multi-statement mode is on.
 * @param[in]  _length  its length in bytes.
 * @param[out] _rows    receives the first result's rows, or NULL for a
 *                      statement that returned none; `NULL` to discard them.
 * @return 0, or the client's error number for the statement that failed.
 * @post On success with `_rows`, the caller frees `*_rows` with
 *       mysql_free_result.
 *
 * Every result after the first, which only a CALL or a script produces, is
 * read and discarded, so the connection is ready for the next statement.
 */
int  d_internal_mariadb_send(struct d_mariadb* _db,
                             const char*       _sql,
                             size_t            _length,
                             MYSQL_RES**       _rows);
/**
 * @brief Formats a statement and sends it, as d_internal_mariadb_send.
 *
 * @param[in]  _db      the connection.
 * @param[out] _rows    as for d_internal_mariadb_send.
 * @param[in]  _format  the statement, as a d_internal_db_format format.
 * @param[in]  _args    the format's arguments.
 * @return as section 4 says.
 */
int  d_internal_mariadb_vrun(struct d_mariadb* _db,
                             MYSQL_RES**       _rows,
                             const char*       _format,
                             va_list           _args);
// as d_internal_mariadb_vrun, taking the arguments directly
int  d_internal_mariadb_run(struct d_mariadb* _db,
                            MYSQL_RES**       _rows,
                            const char*       _format,
                            ...);

// 4.2    Single values
//------------------------------------------------------------------------------
// single values -- each runs one statement and reads the first column of its
// first row. A row yields 0 and the value, written only then; no row, or no
// result at all, is D_DB_NOT_FOUND; a failure is as section 4 says. A NULL
// reads as a NULL string, or as 0. _vquery's string is the caller's, released
// with d_db_free. _int64 reads an integer exactly, and a number with a
// fraction truncated toward zero; _double rounds to the nearest double;
// either reads text that is not a number, or not one the type can hold, as
// D_DB_INVALID_ARGUMENT.
int  d_internal_mariadb_vquery(struct d_mariadb* _db,
                               char**            _value,
                               const char*       _format,
                               va_list           _args);
int  d_internal_mariadb_query(struct d_mariadb* _db,
                              char**            _value,
                              const char*       _format,
                              ...);
int  d_internal_mariadb_vquery_int64(struct d_mariadb* _db,
                                     int64_t*          _value,
                                     const char*       _format,
                                     va_list           _args);
int  d_internal_mariadb_query_int64(struct d_mariadb* _db,
                                    int64_t*          _value,
                                    const char*       _format,
                                    ...);
int  d_internal_mariadb_vquery_double(struct d_mariadb* _db,
                                      double*           _value,
                                      const char*       _format,
                                      va_list           _args);

// 4.3    Rows
//------------------------------------------------------------------------------
// rows -- a column's type and its type's name, as mariadb_query.h tabulates
// them; _rows_columns describes each field of a result to rows under
// construction; _rows_value adds one value, as the protocol sent it in text,
// read as its field's type says: `_text` NULL for SQL NULL, and `_length` its
// bytes. Each of the last two returns `true`, or `false` when memory ran out.
enum d_db_type
     d_internal_mariadb_field_type(const MYSQL_FIELD* _field);
const char*
     d_internal_mariadb_field_type_name(const MYSQL_FIELD* _field);
bool d_internal_mariadb_rows_columns(struct d_internal_db_rows* _rows,
                                     const MYSQL_FIELD*         _fields,
                                     unsigned                   _count);
bool d_internal_mariadb_rows_value(struct d_internal_db_rows* _rows,
                                   const MYSQL_FIELD*         _field,
                                   const char*                _text,
                                   unsigned long              _length);
/**
 * @brief Reads every row of a result into rows, and frees the result.
 *
 * @param[in]  _result  the result, from mysql_store_result; `NULL` for a
 *                      statement that returned none, which gives empty rows.
 * @param[out] _rows    receives the rows; written only on success.
 * @return 0, or D_INTERNAL_MARIADB_OUT_OF_MEMORY when the rows could not be
 *         built.
 * @post `_result` is freed whatever happens. On success the caller releases
 *       `_rows` with d_db_rows_free.
 */
int  d_internal_mariadb_collect_rows(MYSQL_RES*        _result,
                                     struct d_db_rows* _rows);


//==============================================================================
// 5.  CATALOGUE
//==============================================================================
// What a database holds, read from information_schema. A server matches some
// names by case and others not -- databases, tables and views as its
// lower_case_table_names says, triggers so too on MariaDB and MySQL 5.7 but
// never on MySQL 8, columns and indexes never -- while information_schema
// compares them all without case on MariaDB. Where the server matches by
// case, the lookup compares the names' bytes too, beside the comparison an
// information_schema lookup is optimized for.


// 5.1    Objects
//------------------------------------------------------------------------------
// 5.1.1
// d_internal_mariadb_object
//   enum: what a lookup looks for.
enum d_internal_mariadb_object
{
    D_INTERNAL_MARIADB_DATABASE = 0,  // a database
    D_INTERNAL_MARIADB_TABLE,         // a base table, system-versioned or not
    D_INTERNAL_MARIADB_VIEW,          // a view
    D_INTERNAL_MARIADB_RELATION,      // a table or a view: whatever holds a
                                      // name CREATE VIEW would collide with
    D_INTERNAL_MARIADB_COLUMN,        // a table's column
    D_INTERNAL_MARIADB_INDEX,         // a table's index, its primary key the
                                      // one named PRIMARY
    D_INTERNAL_MARIADB_TRIGGER        // a trigger
};

// 5.1.2
// d_internal_mariadb_path
//   struct: where an object is.
struct d_internal_mariadb_path
{
    const char* schema;  // its database; NULL: the current one
    const char* table;   // the table it belongs to: required for a column or
                         // an index, optional for a trigger, NULL otherwise
    const char* name;    // its name; NULL when listing
};

// 5.2    Lookups
//------------------------------------------------------------------------------
/**
 * @brief Answers whether an object exists.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to look for.
 * @param[in]  _path    where, and its name.
 * @param[out] _exists  receives the answer; `false` on any failure.
 * @return 0, the client's error number for a failure, or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, a name that is `NULL`
 *         or empty, or a table missing where the object needs one.
 */
int  d_internal_mariadb_has(struct d_mariadb*                     _db,
                            enum d_internal_mariadb_object        _object,
                            const struct d_internal_mariadb_path* _path,
                            bool*                                 _exists);
/**
 * @brief Lists the objects of a kind, sorted by their names' bytes.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to list; an index list leaves PRIMARY out.
 * @param[in]  _path    where; its name is not read.
 * @param[out] _names   receives the names; written only on success.
 * @return 0, the client's error number for a failure,
 *         D_INTERNAL_MARIADB_OUT_OF_MEMORY, or D_DB_INVALID_ARGUMENT as for
 *         d_internal_mariadb_has.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_internal_mariadb_list(struct d_mariadb*                     _db,
                             enum d_internal_mariadb_object        _object,
                             const struct d_internal_mariadb_path* _path,
                             struct d_db_name_list*                _names);
/**
 * @brief Appends a condition matching a column to a database's or table's
 *        name, as the server matches it.
 *
 * @param[in]     _db      the connection, whose quoting applies.
 * @param[in,out] _sql     the statement so far.
 * @param[in]     _column  the column, as SQL.
 * @param[in]     _name    the name; `NULL` for the current database.
 * @param[in]     _exact   whether the server matches it by case.
 * @return as d_internal_db_format.
 *
 * Writes ` AND column = name`, and with `_exact` also
 * ` AND HEX(column) = HEX(name)`.
 */
enum d_internal_db_format_status
     d_internal_mariadb_match(struct d_mariadb*            _db,
                              struct d_internal_db_buffer* _sql,
                              const char*                  _column,
                              const char*                  _name,
                              bool                         _exact);


#endif  // D_INTERNAL_MARIADB


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_INTERNAL_H
