/*******************************************************************************
* djinterp [c]                                                  mariadb_common.c
*
* Implementation of mariadb_common.h.
*   What a server can do is decided by its flavor and version, which the
* handle read from the server's version string when it was made: no request
* is sent to answer d_mariadb_supports. The client library's own answers come
* from the library loaded at run time.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.25
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_common.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strcmp, strncmp
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // d_db_status_message
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_mariadb_feature_need
//   struct: the releases that brought one feature to each server.
struct d_internal_mariadb_feature_need
{
    enum d_mariadb_feature feature;        // the feature
    unsigned long          mysql_since;    // MySQL's first release with it,
                                           // or 0 for none yet
    unsigned long          mariadb_since;  // MariaDB's, likewise
};

// d_internal_mariadb_feature_needs
//   table: every feature, and the releases, numbered MAJOR*10000 +
// MINOR*100 + PATCH, that introduced it, as mariadb_common.h lists them.
static const struct d_internal_mariadb_feature_need
d_internal_mariadb_feature_needs[] =
{
    { D_MARIADB_FEATURE_CTE,                 80001ul, 100202ul },
    { D_MARIADB_FEATURE_WINDOW_FUNCTIONS,    80002ul, 100200ul },
    { D_MARIADB_FEATURE_CHECK_CONSTRAINTS,   80016ul, 100201ul },
    { D_MARIADB_FEATURE_JSON,                50708ul, 100207ul },
    { D_MARIADB_FEATURE_RENAME_COLUMN,       80003ul, 100502ul },
    { D_MARIADB_FEATURE_INVISIBLE_COLUMNS,   80023ul, 100303ul },
    { D_MARIADB_FEATURE_EXPRESSION_DEFAULTS, 80013ul, 100201ul },
    { D_MARIADB_FEATURE_DESCENDING_INDEXES,  80001ul, 100801ul },
    { D_MARIADB_FEATURE_RETURNING,           0ul,     100500ul },
    { D_MARIADB_FEATURE_SEQUENCES,           0ul,     100300ul },
    { D_MARIADB_FEATURE_SYSTEM_VERSIONING,   0ul,     100304ul },
    { D_MARIADB_FEATURE_INET6,               0ul,     100500ul },
    { D_MARIADB_FEATURE_UUID,                0ul,     100700ul },
    { D_MARIADB_FEATURE_BINARY_JSON,         50708ul, 0ul      },
    { D_MARIADB_FEATURE_LATERAL_DERIVED,     80014ul, 0ul      },
    { D_MARIADB_FEATURE_ROLES,               80000ul, 100005ul },
    { D_MARIADB_FEATURE_ATOMIC_DDL,          80000ul, 100601ul },
    { D_MARIADB_FEATURE_GALERA,              0ul,     100100ul },
    { D_MARIADB_FEATURE_X_PROTOCOL,          50712ul, 0ul      },
    { D_MARIADB_FEATURE_GROUP_REPLICATION,   50717ul, 0ul      },
    { D_MARIADB_FEATURE_RESET_CONNECTION,    50703ul, 100204ul },
    { D_MARIADB_FEATURE_SESSION_TRACKING,    50704ul, 100202ul },
    { D_MARIADB_FEATURE_QUERY_ATTRIBUTES,    80023ul, 0ul      }
};

/*
d_internal_mariadb_client_has
  Whether the client the layer was built with has the call a feature of the
client's needs (mariadb_internal.h, section 1.2); every other feature is the
server's alone.
*/
static bool
d_internal_mariadb_client_has(
    enum d_mariadb_feature _feature
)
{
    switch (_feature)
    {
        case D_MARIADB_FEATURE_RESET_CONNECTION:
            return (D_INTERNAL_MARIADB_RESET == 1);

        case D_MARIADB_FEATURE_SESSION_TRACKING:
            return (D_INTERNAL_MARIADB_SESSION_TRACK == 1);

        case D_MARIADB_FEATURE_QUERY_ATTRIBUTES:
            return (D_INTERNAL_MARIADB_QUERY_ATTRIBUTES == 1);

        default:
            break;
    }

    return true;
}

//==============================================================================
// 2.  LIBRARY
//==============================================================================

// 2.1    Client
//------------------------------------------------------------------------------
/*
d_mariadb_client_version
  The loaded library's own string: Connector/C's version (3.3.17, say),
or libmysqlclient's, which is its server release (8.0.46).
*/
const char*
d_mariadb_client_version(
    void
)
{
    return mysql_get_client_info();
}

/*
d_mariadb_client_version_number
  The loaded library's own number, as its string reads.
*/
unsigned long
d_mariadb_client_version_number(
    void
)
{
    return mysql_get_client_version();
}

/*
d_mariadb_client_is_mariadb
  Decided when the layer was compiled, by the header env found: the two
clients' libraries are not interchangeable at run time.
*/
bool
d_mariadb_client_is_mariadb(
    void
)
{
    return (D_ENV_MYSQL_COMMON_IS_MARIADB != 0);
}

#if (D_INTERNAL_MARIADB_TLS_LIBRARY == 1)

/*
d_mariadb_client_nonblocking_tls
  Both answers are the library's at run time, which need not be the one the
layer was compiled against: its number, then its TLS library's name, which
Schannel's build gives as "Schannel" (schannel.c's tls_library_version) and
a build without TLS as "Off", which has no TLS to pass by.
*/
bool
d_mariadb_client_nonblocking_tls(
    void
)
{
    const unsigned long version = mysql_get_client_version();
    const char*         library = NULL;

    // one path for both, from 3.4.10
    if (version >= D_INTERNAL_MARIADB_ASYNC_TLS_SINCE)
    {
        return true;
    }

    // a library the client cannot name: as one without Schannel
    if ( (mariadb_get_infov(NULL, MARIADB_TLS_LIBRARY, &library) != 0) ||
         (!library) )
    {
        return (version >= D_INTERNAL_MARIADB_ASYNC_ANY_TLS_SINCE);
    }

    // Schannel before 3.4.10, any TLS before 3.0.4
    return ( (strcmp(library, "Off") == 0) ||
             ( (strncmp(library, "Schannel", 8u) != 0) &&
               (version >= D_INTERNAL_MARIADB_ASYNC_ANY_TLS_SINCE) ) );
}

#else

/*
d_mariadb_client_nonblocking_tls
  A client without Schannel carries TLS as it carries anything else, where
it has calls that do not block at all.
*/
bool
d_mariadb_client_nonblocking_tls(
    void
)
{
    return (D_INTERNAL_MARIADB_ASYNC != 0);
}

#endif  // D_INTERNAL_MARIADB_TLS_LIBRARY

// 2.2    Result messages
//------------------------------------------------------------------------------
/*
d_mariadb_result_message
  A client or server error's text lives with the connection that met it, so
only d_mariadb_error_message can give it; this says so, save for the four
numbers the layer returns itself, which have no text there.
*/
const char*
d_mariadb_result_message(
    int _result
)
{
    // the layer's own results, in db_common.h's words
    if (_result <= 0)
    {
        const char* const message = d_db_status_message(_result);

        return (message) ? message : "unknown result";
    }

    switch (_result)
    {
        case D_INTERNAL_MARIADB_UNKNOWN_ERROR:
            return "the server reported a failure without an error number";

        case D_INTERNAL_MARIADB_OUT_OF_MEMORY:
            return "out of memory";

        case D_INTERNAL_MARIADB_INVALID_PARAMETER:
            return "the statement has no parameter of that number";

        case D_INTERNAL_MARIADB_OUT_OF_SYNC:
            return "a statement's run holds the connection, its rows still "
                   "arriving";

        default:
            break;
    }

    return "a client or server error, which d_mariadb_error_message "
           "describes";
}

//==============================================================================
// 3.  SERVERS
//==============================================================================

// 3.1    Versions
//------------------------------------------------------------------------------
/*
d_mariadb_server_flavor
  Read when the handle was made.
*/
enum d_mariadb_flavor
d_mariadb_server_flavor(
    const struct d_mariadb* _db
)
{
    return (_db) ? _db->flavor : D_MARIADB_FLAVOR_UNKNOWN;
}

/*
d_mariadb_server_version
  The client's copy of what the server sent; Connector/C strips MariaDB's
"5.5.5-" from it, and libmysqlclient does not.
*/
const char*
d_mariadb_server_version(
    const struct d_mariadb* _db
)
{
    return (_db) ? mysql_get_server_info(_db->native) : NULL;
}

/*
d_mariadb_server_version_number
  Read when the handle was made, past MariaDB's disguise, where
mysql_get_server_version under libmysqlclient answers 50505.
*/
unsigned long
d_mariadb_server_version_number(
    const struct d_mariadb* _db
)
{
    return (_db) ? _db->version : 0ul;
}

/*
d_mariadb_server_at_least
  The version is taken apart and compared a part at a time, so no argument,
however large, can overflow an encoded number.
*/
bool
d_mariadb_server_at_least(
    const struct d_mariadb* _db,
    enum d_mariadb_flavor   _flavor,
    unsigned                _major,
    unsigned                _minor,
    unsigned                _patch
)
{
    // a server of that flavor, known
    if ( (!_db)                                   ||
         (_flavor == D_MARIADB_FLAVOR_UNKNOWN)    ||
         (_db->flavor != _flavor) )
    {
        return false;
    }

    const unsigned long major = _db->version / 10000ul;
    const unsigned long minor = (_db->version / 100ul) % 100ul;

    // the first part that differs decides
    if (major != _major)
    {
        return (major > _major);
    }

    if (minor != _minor)
    {
        return (minor > _minor);
    }

    return ((_db->version % 100ul) >= _patch);
}

// 3.2    Features
//------------------------------------------------------------------------------
/*
d_mariadb_supports
  The feature's row names a release for each flavor, 0 where the flavor has
none; a server of unknown flavor supports nothing, and a value outside the
table is no feature. A call the client lacks is no feature, whatever the
server.
*/
bool
d_mariadb_supports(
    const struct d_mariadb* _db,
    enum d_mariadb_feature  _feature
)
{
    const size_t count = sizeof(d_internal_mariadb_feature_needs) /
                         sizeof(d_internal_mariadb_feature_needs[0]);

    // no server, or no call, no feature
    if ( (!_db) ||
         (!d_internal_mariadb_client_has(_feature)) )
    {
        return false;
    }

    for (size_t i = 0u; i < count; ++i)
    {
        const struct d_internal_mariadb_feature_need* const need =
            &d_internal_mariadb_feature_needs[i];

        // the feature's row decides
        if (need->feature == _feature)
        {
            const unsigned long since =
                (_db->flavor == D_MARIADB_FLAVOR_MARIADB) ? need->mariadb_since
                : (_db->flavor == D_MARIADB_FLAVOR_MYSQL) ? need->mysql_since
                                                          : 0ul;

            return ( (since != 0ul) &&
                     (_db->version >= since) );
        }
    }

    return false;
}

//==============================================================================
// 4.  HANDLES
//==============================================================================

// 4.1    The client's handle
//------------------------------------------------------------------------------
/*
d_mariadb_native
  The MYSQL* inside, as void* because no public header can name it.
*/
void*
d_mariadb_native(
    struct d_mariadb* _db
)
{
    return (_db) ? (void*)_db->native : NULL;
}

// 4.2    Errors
//------------------------------------------------------------------------------
/*
d_mariadb_error_code
  The client's record of the last call on the connection.
*/
unsigned
d_mariadb_error_code(
    const struct d_mariadb* _db
)
{
    return (_db) ? mysql_errno(_db->native) : 0u;
}

/*
d_mariadb_error_message
  As d_mariadb_error_code, in words; the client's buffer, valid until the
next call on the connection.
*/
const char*
d_mariadb_error_message(
    const struct d_mariadb* _db
)
{
    return (_db) ? mysql_error(_db->native) : "";
}

/*
d_mariadb_sqlstate
  As d_mariadb_error_code, as an SQLSTATE.
*/
const char*
d_mariadb_sqlstate(
    const struct d_mariadb* _db
)
{
    return (_db) ? mysql_sqlstate(_db->native) : "";
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_client_version
  Without a client there is no version: the empty string, never NULL.
*/
const char*
d_mariadb_client_version(
    void
)
{
    return "";
}

/*
d_mariadb_client_version_number
  Without a client the version is 0, below every release.
*/
unsigned long
d_mariadb_client_version_number(
    void
)
{
    return 0ul;
}

/*
d_mariadb_client_is_mariadb
  Without a client, false.
*/
bool
d_mariadb_client_is_mariadb(
    void
)
{
    return false;
}

/*
d_mariadb_client_nonblocking_tls
  Without a client, false.
*/
bool
d_mariadb_client_nonblocking_tls(
    void
)
{
    return false;
}

/*
d_mariadb_result_message
  Without a client only the layer's own results occur; this build's
D_DB_UNSUPPORTED says why, and any other value is unknown.
*/
const char*
d_mariadb_result_message(
    int _result
)
{
    const char* const message = d_db_status_message(_result);

    // the build, not the server, is what cannot
    if (_result == D_DB_UNSUPPORTED)
    {
        return "this build of the MariaDB layer has no client library";
    }

    return (message) ? message : "unknown result";
}

/*
d_mariadb_server_flavor
  Without a client no server is reached.
*/
enum d_mariadb_flavor
d_mariadb_server_flavor(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return D_MARIADB_FLAVOR_UNKNOWN;
}

/*
d_mariadb_server_version
  Without a client, NULL.
*/
const char*
d_mariadb_server_version(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return NULL;
}

/*
d_mariadb_server_version_number
  Without a client the version is 0, below every release.
*/
unsigned long
d_mariadb_server_version_number(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return 0ul;
}

/*
d_mariadb_server_at_least
  Without a client, false.
*/
bool
d_mariadb_server_at_least(
    const struct d_mariadb* _db,
    enum d_mariadb_flavor   _flavor,
    unsigned                _major,
    unsigned                _minor,
    unsigned                _patch
)
{
    (void)_db;
    (void)_flavor;
    (void)_major;
    (void)_minor;
    (void)_patch;

    return false;
}

/*
d_mariadb_supports
  Without a client, false.
*/
bool
d_mariadb_supports(
    const struct d_mariadb* _db,
    enum d_mariadb_feature  _feature
)
{
    (void)_db;
    (void)_feature;

    return false;
}

/*
d_mariadb_native
  Without a client, NULL.
*/
void*
d_mariadb_native(
    struct d_mariadb* _db
)
{
    (void)_db;

    return NULL;
}

/*
d_mariadb_error_code
  Without a client there is nothing to count: 0.
*/
unsigned
d_mariadb_error_code(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return 0u;
}

/*
d_mariadb_error_message
  Without a client there is no error to describe: the empty string.
*/
const char*
d_mariadb_error_message(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return "";
}

/*
d_mariadb_sqlstate
  Without a client there is no error to describe: the empty string.
*/
const char*
d_mariadb_sqlstate(
    const struct d_mariadb* _db
)
{
    (void)_db;

    return "";
}

#endif  // D_INTERNAL_MARIADB
