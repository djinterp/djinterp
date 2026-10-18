/*******************************************************************************
* djinterp [c]                                              mariadb_connection.c
*
* Implementation of mariadb_connection.h.
*   A handle is made around a connected MYSQL*, and reads the server's flavor
* and version from its version string then, once. The connections the layer
* makes are set up before they connect: a bounded connect timeout, the
* layer's character set, and LOAD DATA LOCAL off -- it lets a server ask for
* any file the client can read, and libmysqlclient 8.0 already has it off
* where Connector/C does not. Multiple results are asked for, so that a CALL
* works through d_mariadb_exec. Where the handshake cannot carry the
* character set -- libmysqlclient from 8.0 against MariaDB, or MySQL before
* 8.0 -- it is set again once connected, and again after a change of user.
* d_mariadb_connect_with applies a program's settings after the layer's
* defaults, so that one can replace a default.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_connection.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_connection.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX, UINT_MAX
#include <stddef.h>  // NULL, size_t
#include <stdlib.h>  // malloc, free
#include <string.h>  // strlen, strncmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_connection.h"  // D_INTERNAL_MARIADB, the timeout
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, uint64_t


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/*
d_internal_mariadb_parse_version
  MAJOR.MINOR.PATCH at the start of a text, as MAJOR*10000 + MINOR*100 +
PATCH; 0 when the text does not start with a digit, and missing parts read as
0. Each part stops growing at 100000, and minor and patch at 99 in the sum, so
a garbled string can neither overflow nor carry into the next part.
*/
static unsigned long
d_internal_mariadb_parse_version(
    const char* _text
)
{
    unsigned long parts[3] = { 0ul, 0ul, 0ul };
    const char*   at       = _text;
    size_t        count    = 0u;

    // up to three parts, each of digits, separated by dots
    while ( (count < 3u)  &&
            (*at >= '0') &&
            (*at <= '9') )
    {
        while ( (*at >= '0') &&
                (*at <= '9') )
        {
            parts[count] = (parts[count] < 100000ul)
                               ? ((parts[count] * 10ul) +
                                  (unsigned long)(*at - '0'))
                               : parts[count];
            ++at;
        }

        ++count;
        at += (*at == '.') ? 1 : 0;
    }

    return (parts[0] * 10000ul)                                  +
           (((parts[1] < 99ul) ? parts[1] : 99ul) * 100ul)       +
           ((parts[2] < 99ul) ? parts[2] : 99ul);
}

/*
d_internal_mariadb_describe
  The flavor is MariaDB when the version string names it, and MySQL's
otherwise. MariaDB long sent "5.5.5-" before its true version, for clients
that would not believe a major version past 5; libmysqlclient passes it on,
so the string is read past it here.
*/
static void
d_internal_mariadb_describe(
    struct d_mariadb* _db
)
{
    const char* const info = mysql_get_server_info(_db->native);

    _db->flavor  = D_MARIADB_FLAVOR_UNKNOWN;
    _db->version = 0ul;

    // a connection that has not said what it reached
    if ( (!info) ||
         (info[0] == '\0') )
    {
        return;
    }

    const bool        mariadb = (strstr(info, "MariaDB") != NULL);
    const char* const text    = ( (mariadb) &&
                                  (strncmp(info, "5.5.5-", 6u) == 0) )
                                    ? (info + 6)
                                    : info;

    _db->flavor  = (mariadb) ? D_MARIADB_FLAVOR_MARIADB
                             : D_MARIADB_FLAVOR_MYSQL;
    _db->version = d_internal_mariadb_parse_version(text);

    return;
}

/*
d_internal_mariadb_wrap
  A new handle around a connection, which learns the server's flavor and
version at once.
*/
static struct d_mariadb*
d_internal_mariadb_wrap(
    MYSQL* _native,
    bool   _owned
)
{
    struct d_mariadb* const db = (struct d_mariadb*)malloc(sizeof(*db));

    // the handle's memory
    if (!db)
    {
        return NULL;
    }

    db->native           = _native;
    db->owned            = _owned;
    db->multi_statements = false;
    db->case_rule        = -1;
    db->failed           = NULL;
    d_internal_mariadb_describe(db);

    return db;
}

/*
d_internal_mariadb_refused
  mysql_options refuses only an option the client does not know, without an
error number; that is read as the unknown error.
*/
static int
d_internal_mariadb_refused(
    MYSQL* _native
)
{
    return (mysql_errno(_native) != 0u) ? (int)mysql_errno(_native)
                                        : D_INTERNAL_MARIADB_UNKNOWN_ERROR;
}

/*
d_internal_mariadb_configure
  An unknown character set is reported by the connect itself.
*/
static int
d_internal_mariadb_configure(
    MYSQL* _native
)
{
    unsigned int      timeout      = D_INTERNAL_MARIADB_CONNECT_TIMEOUT_S;
    unsigned int      local_infile = 0u;
    const char* const charset      = D_INTERNAL_MARIADB_CHARSET;

    // the timeout, LOAD DATA LOCAL off, and the character set if named
    if ( (mysql_options(_native, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) != 0)   ||
         (mysql_options(_native, MYSQL_OPT_LOCAL_INFILE, &local_infile) != 0) ||
         ( (charset[0] != '\0') &&
           (mysql_options(_native, MYSQL_SET_CHARSET_NAME, charset) != 0) ) )
    {
        return d_internal_mariadb_refused(_native);
    }

    return 0;
}

#if (D_INTERNAL_MARIADB_SSL_SWITCHES == 1)

/*
d_internal_mariadb_ssl_mode
  libmysqlclient's own modes, one for one.
*/
static int
d_internal_mariadb_ssl_mode(
    MYSQL*        _native,
    unsigned long _mode
)
{
    static const unsigned int modes[] =
    {
        SSL_MODE_PREFERRED,
        SSL_MODE_DISABLED,
        SSL_MODE_REQUIRED,
        SSL_MODE_VERIFY_CA,
        SSL_MODE_VERIFY_IDENTITY
    };

    // parameter validation
    if (_mode >= (sizeof(modes) / sizeof(modes[0])))
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (mysql_options(_native, MYSQL_OPT_SSL_MODE, &modes[_mode]) == 0)
               ? 0
               : d_internal_mariadb_refused(_native);
}

#elif (D_INTERNAL_MARIADB_SSL_SWITCHES == 2)

/*
d_internal_mariadb_ssl_mode
  Connector/C's two switches: one insists on TLS, the other on a certificate
an authority signed for the host's name. It has none to refuse TLS, which it
uses only when a key, a certificate or an authority is given, nor one to
check the authority alone.
*/
static int
d_internal_mariadb_ssl_mode(
    MYSQL*        _native,
    unsigned long _mode
)
{
    const d_internal_mariadb_flag on = 1;

    switch (_mode)
    {
        case D_MARIADB_SSL_PREFERRED:
        case D_MARIADB_SSL_DISABLED:
            return 0;

        case D_MARIADB_SSL_REQUIRED:
            return (mysql_options(_native, MYSQL_OPT_SSL_ENFORCE, &on) == 0)
                       ? 0
                       : d_internal_mariadb_refused(_native);

        case D_MARIADB_SSL_VERIFY_CA:
            return D_DB_UNSUPPORTED;

        case D_MARIADB_SSL_VERIFY_IDENTITY:
            return ( (mysql_options(_native,
                                    MYSQL_OPT_SSL_ENFORCE,
                                    &on) == 0) &&
                     (mysql_options(_native,
                                    MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                                    &on) == 0) )
                       ? 0
                       : d_internal_mariadb_refused(_native);

        default:
            break;
    }

    return D_DB_INVALID_ARGUMENT;
}

#else

/*
d_internal_mariadb_ssl_mode
  A libmysqlclient older than 5.7.11 has neither the modes nor the switches:
only its default can be had.
*/
static int
d_internal_mariadb_ssl_mode(
    MYSQL*        _native,
    unsigned long _mode
)
{
    (void)_native;

    // the default, which needs no switch; a mode, which cannot be had
    if (_mode == (unsigned long)D_MARIADB_SSL_PREFERRED)
    {
        return 0;
    }

    return (_mode <= (unsigned long)D_MARIADB_SSL_VERIFY_IDENTITY)
               ? D_DB_UNSUPPORTED
               : D_DB_INVALID_ARGUMENT;
}

#endif  // D_INTERNAL_MARIADB_SSL_SWITCHES

/*
d_internal_mariadb_setting_option
  The client's option for a setting that takes a number of seconds or a text,
by the setting's name; d_internal_mariadb_set asks only for those.
*/
static enum mysql_option
d_internal_mariadb_setting_option(
    enum d_mariadb_setting_name _name
)
{
    switch (_name)
    {
        case D_MARIADB_SETTING_READ_TIMEOUT:  return MYSQL_OPT_READ_TIMEOUT;
        case D_MARIADB_SETTING_WRITE_TIMEOUT: return MYSQL_OPT_WRITE_TIMEOUT;
        case D_MARIADB_SETTING_INIT_COMMAND:  return MYSQL_INIT_COMMAND;
        case D_MARIADB_SETTING_SSL_KEY:       return MYSQL_OPT_SSL_KEY;
        case D_MARIADB_SETTING_SSL_CERT:      return MYSQL_OPT_SSL_CERT;
        case D_MARIADB_SETTING_SSL_CA:        return MYSQL_OPT_SSL_CA;
        default:                              break;
    }

    return MYSQL_OPT_CONNECT_TIMEOUT;
}

/*
d_internal_mariadb_set
  One setting, onto a connection not yet made, each option given its argument
as the client documents it; or the CLIENT_* flags it adds, into `_flags`.
CLIENT_MULTI_STATEMENTS is refused: the layer turns that mode on for
d_mariadb_exec_script alone, and runs one statement at a time otherwise. A
mode that insists on TLS sets `_tls`, for the check once connected.
*/
static int
d_internal_mariadb_set(
    MYSQL*                          _native,
    const struct d_mariadb_setting* _setting,
    unsigned long*                  _flags,
    bool*                           _tls
)
{
    const enum mysql_option option  =
        d_internal_mariadb_setting_option(_setting->name);
    unsigned int            number  = 0u;
    int                     refused = 0;

    switch (_setting->name)
    {
        case D_MARIADB_SETTING_CONNECT_TIMEOUT:
        case D_MARIADB_SETTING_READ_TIMEOUT:
        case D_MARIADB_SETTING_WRITE_TIMEOUT:
            // seconds, as many as an unsigned int holds
            if (_setting->number > (unsigned long)UINT_MAX)
            {
                return D_DB_INVALID_ARGUMENT;
            }

            number  = (unsigned int)_setting->number;
            refused = mysql_options(_native, option, &number);
            break;

        case D_MARIADB_SETTING_INIT_COMMAND:
        case D_MARIADB_SETTING_SSL_KEY:
        case D_MARIADB_SETTING_SSL_CERT:
        case D_MARIADB_SETTING_SSL_CA:
            // a text, which these cannot do without
            if (!_setting->text)
            {
                return D_DB_INVALID_ARGUMENT;
            }

            refused = mysql_options(_native, option, _setting->text);
            break;

        case D_MARIADB_SETTING_COMPRESS:
            refused = (_setting->number != 0ul)
                          ? mysql_options(_native, MYSQL_OPT_COMPRESS, NULL)
                          : 0;
            break;

        case D_MARIADB_SETTING_LOCAL_INFILE:
            number  = (_setting->number != 0ul) ? 1u : 0u;
            refused = mysql_options(_native, MYSQL_OPT_LOCAL_INFILE, &number);
            break;

        case D_MARIADB_SETTING_SSL_MODE:
            *_tls = (_setting->number >= (unsigned long)D_MARIADB_SSL_REQUIRED);

            return d_internal_mariadb_ssl_mode(_native, _setting->number);

        case D_MARIADB_SETTING_CONNECT_ATTRIBUTE:
            // a name and a value
            if ( (!_setting->text) ||
                 (!_setting->value) )
            {
                return D_DB_INVALID_ARGUMENT;
            }

            refused = mysql_options4(_native,
                                     MYSQL_OPT_CONNECT_ATTR_ADD,
                                     _setting->text,
                                     _setting->value);
            break;

        case D_MARIADB_SETTING_CLIENT_FLAGS:
            // the one mode the layer keeps for itself
            if ((_setting->number & (unsigned long)CLIENT_MULTI_STATEMENTS) !=
                0ul)
            {
                return D_DB_INVALID_ARGUMENT;
            }

            *_flags |= _setting->number;

            return 0;

        case D_MARIADB_SETTING_NATIVE:
            // an option the client numbers, as given
            if (_setting->number > (unsigned long)INT_MAX)
            {
                return D_DB_INVALID_ARGUMENT;
            }

            refused = mysql_options(_native,
                                    (enum mysql_option)(int)_setting->number,
                                    _setting->value);
            break;

        default:
            return D_DB_INVALID_ARGUMENT;
    }

    return (refused == 0) ? 0 : d_internal_mariadb_refused(_native);
}

/*
d_internal_mariadb_settle_charset
  libmysqlclient from 8.0 names utf8mb4_0900_ai_ci in its handshake, which
MariaDB and MySQL before 8.0 do not have: such a server keeps its own
default character set for the session, silently, while the client believes
it has the one asked for, and text in any other character set is mangled.
With that client, against those servers, the character set named is set
again once connected, by mysql_set_character_set's SET NAMES, which the
client records; with every other pairing the handshake carried it. A change
of user is a handshake too, and is settled the same way.
*/
static int
d_internal_mariadb_settle_charset(
    struct d_mariadb* _db,
    const char*       _charset
)
{
    const char* const charset = (_charset) ? _charset : "";

    // a character set named, and a pairing whose handshake lost it
    if ( (charset[0] == '\0')                     ||
         (D_ENV_MYSQL_COMMON_IS_MARIADB != 0)       ||
         (mysql_get_client_version() < 80000ul)     ||
         ( (_db->flavor == D_MARIADB_FLAVOR_MYSQL) &&
           (_db->version >= 80000ul) ) )
    {
        return 0;
    }

    // SET NAMES, which the client then records
    if (mysql_set_character_set(_db->native, charset) != 0)
    {
        return (mysql_errno(_db->native) != 0u)
                   ? (int)mysql_errno(_db->native)
                   : D_INTERNAL_MARIADB_UNKNOWN_ERROR;
    }

    return 0;
}

/*
d_internal_mariadb_reason
  Why a connection was not made, in memory the caller releases with
d_db_free: the client's own words where it has some, which closing the
connection loses, and the layer's for a refusal of its own -- TLS that a
mode insisted on and did not get, the numbers the layer returns itself, a
setting it refused; NULL when memory runs out.
*/
static char*
d_internal_mariadb_reason(
    MYSQL* _native,
    int    _result
)
{
    const char* reason = mysql_error(_native);

    // the layer's words where the client has none
    if ( (!reason) ||
         (reason[0] == '\0') )
    {
        reason = (_result == D_INTERNAL_MARIADB_SSL_ERROR)
                     ? "the connection is not encrypted, which its TLS "
                       "mode requires"
                     : d_mariadb_result_message(_result);
    }

    const size_t size = strlen(reason) + 1u;
    char* const  copy = (char*)malloc(size);

    // the copy, where there was memory for it
    if (copy)
    {
        memcpy(copy, reason, size);
    }

    return copy;
}

/*
d_internal_mariadb_hold
  The layer's handle around a connection d_mariadb_connect made, its
character set settled; on a failure no handle, and the connection still the
caller's to close.
*/
static int
d_internal_mariadb_hold(
    MYSQL*             _native,
    struct d_mariadb** _db
)
{
    *_db = d_internal_mariadb_wrap(_native, true);

    // the handle's memory
    if (!*_db)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    const int result = d_internal_mariadb_settle_charset(
                           *_db, D_INTERNAL_MARIADB_CHARSET);

    // a character set the server would not take: no handle
    if (result != 0)
    {
        free(*_db);
        *_db = NULL;
    }

    return result;
}

//==============================================================================
// 1.  CONNECTIONS
//==============================================================================

// 1.2    Connecting and closing
//------------------------------------------------------------------------------
/*
d_mariadb_connect
  d_mariadb_connect_with, with no settings.
*/
int
d_mariadb_connect(
    const struct d_mariadb_options* _options,
    struct d_mariadb**              _db
)
{
    return d_mariadb_connect_with(_options, NULL, 0u, _db, NULL);
}

/*
d_mariadb_connect_with
  The client's handle is made, set up -- the layer's defaults, then the
settings in order -- connected, wrapped, and its character set settled, in
that order; a failure at any step closes what was made, after its error
number and its reason are read, since closing would lose them.
*/
int
d_mariadb_connect_with(
    const struct d_mariadb_options* _options,
    const struct d_mariadb_setting* _settings,
    size_t                          _count,
    struct d_mariadb**              _db,
    char**                          _message
)
{
    static const struct d_mariadb_options defaults =
    {
        NULL, NULL, NULL, NULL, 0u, NULL
    };
    unsigned long flags = CLIENT_MULTI_RESULTS;
    bool          tls   = false;

    // nothing yet, whatever follows
    if (_message)
    {
        *_message = NULL;
    }

    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_db = NULL;

    if ( (!_settings) &&
         (_count != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const struct d_mariadb_options* const options = (_options) ? _options
                                                               : &defaults;
    MYSQL* const                          native  = mysql_init(NULL);

    // mysql_init fails only for want of memory
    if (!native)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    int result = d_internal_mariadb_configure(native);

    // the settings, in their order, over the defaults
    for (size_t i = 0u; (result == 0) && (i < _count); ++i)
    {
        result = d_internal_mariadb_set(native, &_settings[i], &flags, &tls);
    }

    // the connection, then the layer's handle around it
    if ( (result == 0) &&
         (!mysql_real_connect(native,
                              options->host,
                              options->user,
                              options->password,
                              options->database,
                              options->port,
                              options->unix_socket,
                              flags)) )
    {
        result = (int)mysql_errno(native);
    }

    // TLS, where a mode insisted on it: Connector/C 3.3 connects without it
    // when the server has none, its switch notwithstanding
    if ( (result == 0) &&
         (tls)         &&
         (!mysql_get_ssl_cipher(native)) )
    {
        result = D_INTERNAL_MARIADB_SSL_ERROR;
    }

    if (result == 0)
    {
        result = d_internal_mariadb_hold(native, _db);
    }

    // a failure leaves nothing behind but its reason, where it is wanted
    if (result != 0)
    {
        if (_message)
        {
            *_message = d_internal_mariadb_reason(native, result);
        }

        mysql_close(native);
    }

    return result;
}

/*
d_mariadb_adopt
  The connection is wrapped as it is: none of d_mariadb_connect's set-up is
applied, since the program chose its own.
*/
int
d_mariadb_adopt(
    void*              _native,
    bool               _owned,
    struct d_mariadb** _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_db = NULL;

    if (!_native)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_db = d_internal_mariadb_wrap((MYSQL*)_native, _owned);

    return (*_db) ? 0 : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
}

/*
d_mariadb_close
  The connection goes only when the handle owns it; the handle always, and a
statement whose prepare failed, which the handle kept, with it.
*/
int
d_mariadb_close(
    struct d_mariadb* _db
)
{
    // accept NULL, as free does
    if (_db)
    {
        if (_db->failed)
        {
            (void)mysql_stmt_close(_db->failed);
        }

        if (_db->owned)
        {
            mysql_close(_db->native);
        }

        free(_db);
    }

    return 0;
}

// 1.3    Changing user
//------------------------------------------------------------------------------
/*
d_mariadb_change_user
  The client's own call, after the layer's housekeeping. The new session
starts from the character set the client names in the request, which the
servers d_internal_mariadb_settle_charset knows of may not have, so it is
settled as at connecting -- the client's current one, which a program may
have changed since.
*/
int
d_mariadb_change_user(
    struct d_mariadb* _db,
    const char*       _user,
    const char*       _password,
    const char*       _database
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_ready(_db);

    // the request itself
    if ( (result == 0) &&
         (mysql_change_user(_db->native, _user, _password, _database) != 0) )
    {
        result = (mysql_errno(_db->native) != 0u)
                     ? (int)mysql_errno(_db->native)
                     : D_INTERNAL_MARIADB_UNKNOWN_ERROR;
    }

    if (result == 0)
    {
        result = d_internal_mariadb_settle_charset(
                     _db, mysql_character_set_name(_db->native));
    }

    return result;
}

// 1.4    Liveness
//------------------------------------------------------------------------------
/*
d_mariadb_ping
  The client's ping, which does not reconnect: both clients leave automatic
reconnection off unless a program turns it on.
*/
int
d_mariadb_ping(
    struct d_mariadb* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (mysql_ping(_db->native) == 0) ? 0 : (int)mysql_errno(_db->native);
}

/*
d_mariadb_connection_id
  The id the server gave the connection when it connected.
*/
unsigned long
d_mariadb_connection_id(
    const struct d_mariadb* _db
)
{
    return (_db) ? mysql_thread_id(_db->native) : 0ul;
}

/*
d_mariadb_server_status
  mysql_stat's text is the client's, valid until the next call; the layer's
copy is the caller's. For a failure both clients answer their error message
rather than the NULL they document, so the error number decides: a command
clears it when it starts.
*/
int
d_mariadb_server_status(
    struct d_mariadb* _db,
    char**            _status
)
{
    struct d_internal_db_buffer copy = { NULL, 0u, 0u };

    // parameter validation
    if ( (!_db) ||
         (!_status) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_ready(_db);

    // the client's ready state, then its text
    if (result != 0)
    {
        return result;
    }

    const char* const text = mysql_stat(_db->native);

    if ( (!text) ||
         (mysql_errno(_db->native) != 0u) )
    {
        return d_internal_mariadb_refused(_db->native);
    }

    // the copy, terminated
    if ( (!d_internal_db_buffer_append(&copy, text, strlen(text))) ||
         (!d_internal_db_buffer_append(&copy, "", 1u)) )
    {
        d_internal_db_buffer_release(&copy);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    *_status = copy.data;

    return 0;
}

// 1.5    Rows changed
//------------------------------------------------------------------------------
/*
d_mariadb_last_insert_id
  As the server reported it with the last statement.
*/
uint64_t
d_mariadb_last_insert_id(
    const struct d_mariadb* _db
)
{
    return (_db) ? (uint64_t)mysql_insert_id(_db->native) : 0u;
}

/*
d_mariadb_changes
  mysql_affected_rows answers its all-ones value for a failure; no real count
comes near INT64_MAX.
*/
int64_t
d_mariadb_changes(
    const struct d_mariadb* _db
)
{
    // no handle, no statement
    if (!_db)
    {
        return 0;
    }

    const unsigned long long rows = mysql_affected_rows(_db->native);

    return (rows == (unsigned long long)-1) ? -1 : (int64_t)rows;
}

/*
d_mariadb_warning_count
  As the server reported it with the last statement.
*/
unsigned
d_mariadb_warning_count(
    const struct d_mariadb* _db
)
{
    return (_db) ? mysql_warning_count(_db->native) : 0u;
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_connect
  Without a client there is no connection to make: D_DB_UNSUPPORTED, and the
handle NULL, as a failure leaves it.
*/
int
d_mariadb_connect(
    const struct d_mariadb_options* _options,
    struct d_mariadb**              _db
)
{
    (void)_options;

    // a failure leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_connect_with
  Without a client there is no connection to make: D_DB_UNSUPPORTED, and the
handle and the reason NULL, as a failure leaves them -- this build's reason
is the result itself.
*/
int
d_mariadb_connect_with(
    const struct d_mariadb_options* _options,
    const struct d_mariadb_setting* _settings,
    size_t                          _count,
    struct d_mariadb**              _db,
    char**                          _message
)
{
    (void)_options;
    (void)_settings;
    (void)_count;

    // a failure leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    if (_message)
    {
        *_message = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_adopt
  Without a client there is no connection to make: D_DB_UNSUPPORTED, and the
handle NULL, as a failure leaves it.
*/
int
d_mariadb_adopt(
    void*              _native,
    bool               _owned,
    struct d_mariadb** _db
)
{
    (void)_native;
    (void)_owned;

    // a failure leaves NULL behind
    if (_db)
    {
        *_db = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_close
  Without a client no handle exists: NULL closes as it always does, and
anything else is no handle.
*/
int
d_mariadb_close(
    struct d_mariadb* _db
)
{
    return (_db) ? D_DB_UNSUPPORTED : D_DB_OK;
}

/*
d_mariadb_change_user
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_change_user(
    struct d_mariadb* _db,
    const char*       _user,
    const char*       _password,
    const char*       _database
)
{
    (void)_db;
    (void)_user;
    (void)_password;
    (void)_database;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_ping
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_ping(
    struct d_mariadb* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_connection_id
  Without a client there is nothing to count: 0.
*/
unsigned long
d_mariadb_connection_id(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return 0ul;
}

/*
d_mariadb_server_status
  Without a client, D_DB_UNSUPPORTED, and nothing written.
*/
int
d_mariadb_server_status(
    struct d_mariadb* _db,
    char**            _status
)
{
    (void)_db;
    (void)_status;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_last_insert_id
  Without a client there are no rows: 0.
*/
uint64_t
d_mariadb_last_insert_id(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return 0u;
}

/*
d_mariadb_changes
  Without a client there are no rows: 0.
*/
int64_t
d_mariadb_changes(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return 0;
}

/*
d_mariadb_warning_count
  Without a client there is nothing to count: 0.
*/
unsigned
d_mariadb_warning_count(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return 0u;
}

#endif  // D_INTERNAL_MARIADB
