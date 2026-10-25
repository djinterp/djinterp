/*******************************************************************************
* djinterp [c]                                                 postgres_common.c
*
* Implementation of postgres_common.h.
*   Results are SQLSTATEs packed as the server packs them, so reading one
* needs no library: the three functions of section 3 work in both builds,
* with libpq or as stubs, and describe a result by the manual's words for its
* class. What a server can do is decided by its major version, which libpq
* read when the connection was made: no request is sent to answer
* d_postgres_supports.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.23
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_common.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
#include <string.h>  // strncmp
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // d_db_status_message
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// a packed SQLSTATE needs 30 bits, which an int has wherever libpq runs
D_STATIC_ASSERT( (INT_MAX >= 2147483647),
                 "a packed SQLSTATE needs an int of 32 bits" );

// d_internal_postgres_class
//   struct: an SQLSTATE class -- the code's first two characters -- and the
// words the PostgreSQL manual's appendix A gives it.
struct d_internal_postgres_class
{
    const char* code;   // the two characters
    const char* words;  // the manual's, lowercase
};

// d_internal_postgres_classes
//   table: every class the server's errcodes.txt defines.
static const struct d_internal_postgres_class d_internal_postgres_classes[] =
{
    { "00", "successful completion" },
    { "01", "warning" },
    { "02", "no data" },
    { "03", "SQL statement not yet complete" },
    { "08", "connection exception" },
    { "09", "triggered action exception" },
    { "0A", "feature not supported" },
    { "0B", "invalid transaction initiation" },
    { "0F", "locator exception" },
    { "0L", "invalid grantor" },
    { "0P", "invalid role specification" },
    { "0Z", "diagnostics exception" },
    { "20", "case not found" },
    { "21", "cardinality violation" },
    { "22", "data exception" },
    { "23", "integrity constraint violation" },
    { "24", "invalid cursor state" },
    { "25", "invalid transaction state" },
    { "26", "invalid SQL statement name" },
    { "27", "triggered data change violation" },
    { "28", "invalid authorization specification" },
    { "2B", "dependent privilege descriptors still exist" },
    { "2D", "invalid transaction termination" },
    { "2F", "SQL routine exception" },
    { "34", "invalid cursor name" },
    { "38", "external routine exception" },
    { "39", "external routine invocation exception" },
    { "3B", "savepoint exception" },
    { "3D", "invalid catalog name" },
    { "3F", "invalid schema name" },
    { "40", "transaction rollback" },
    { "42", "syntax error or access rule violation" },
    { "44", "WITH CHECK OPTION violation" },
    { "53", "insufficient resources" },
    { "54", "program limit exceeded" },
    { "55", "object not in prerequisite state" },
    { "57", "operator intervention" },
    { "58", "system error" },
    { "72", "snapshot failure" },
    { "F0", "configuration file error" },
    { "HV", "foreign data wrapper error" },
    { "P0", "PL/pgSQL error" },
    { "XX", "internal error" }
};

// d_internal_postgres_condition
//   struct: an SQLSTATE the layer's headers name, packed, and the words the
// PostgreSQL manual's appendix A gives its condition.
struct d_internal_postgres_condition
{
    int         code;   // the SQLSTATE, packed
    const char* words;  // the condition's name, in words
};

// d_internal_postgres_conditions
//   table: the conditions of postgres_common.h's enum d_postgres_sqlstate, the
// four the layer gives itself excepted, which it describes in its own words.
static const struct d_internal_postgres_condition
d_internal_postgres_conditions[] =
{
    { D_POSTGRES_SQLSTATE_PROTOCOL_VIOLATION,      "protocol violation" },
    { D_POSTGRES_SQLSTATE_FEATURE_NOT_SUPPORTED,   "feature not supported" },
    { D_POSTGRES_SQLSTATE_INVALID_PARAMETER_VALUE, "invalid parameter value" },
    { D_POSTGRES_SQLSTATE_INVALID_TEXT_REPRESENTATION,
      "invalid text representation" },
    { D_POSTGRES_SQLSTATE_UNIQUE_VIOLATION,        "unique violation" },
    { D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION,  "active SQL transaction" },
    { D_POSTGRES_SQLSTATE_NO_ACTIVE_SQL_TRANSACTION,
      "no active SQL transaction" },
    { D_POSTGRES_SQLSTATE_IN_FAILED_SQL_TRANSACTION,
      "in failed SQL transaction" },
    { D_POSTGRES_SQLSTATE_DEPENDENT_OBJECTS_STILL_EXIST,
      "dependent objects still exist" },
    { D_POSTGRES_SQLSTATE_INVALID_SAVEPOINT_SPECIFICATION,
      "invalid savepoint specification" },
    { D_POSTGRES_SQLSTATE_INVALID_CATALOG_NAME,    "invalid catalog name" },
    { D_POSTGRES_SQLSTATE_INVALID_SCHEMA_NAME,     "invalid schema name" },
    { D_POSTGRES_SQLSTATE_SERIALIZATION_FAILURE,   "serialization failure" },
    { D_POSTGRES_SQLSTATE_DEADLOCK_DETECTED,       "deadlock detected" },
    { D_POSTGRES_SQLSTATE_INSUFFICIENT_PRIVILEGE,  "insufficient privilege" },
    { D_POSTGRES_SQLSTATE_SYNTAX_ERROR,            "syntax error" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_COLUMN,        "duplicate column" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_COLUMN,        "undefined column" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_OBJECT,        "undefined object" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_OBJECT,        "duplicate object" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_FUNCTION,      "undefined function" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_TABLE,         "undefined table" },
    { D_POSTGRES_SQLSTATE_UNDEFINED_PARAMETER,     "undefined parameter" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_DATABASE,      "duplicate database" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_PREPARED_STATEMENT,
      "duplicate prepared statement" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_SCHEMA,        "duplicate schema" },
    { D_POSTGRES_SQLSTATE_DUPLICATE_TABLE,         "duplicate table" },
    { D_POSTGRES_SQLSTATE_OBJECT_IN_USE,           "object in use" },
    { D_POSTGRES_SQLSTATE_LOCK_NOT_AVAILABLE,      "lock not available" },
    { D_POSTGRES_SQLSTATE_QUERY_CANCELED,          "query canceled" },
    { D_POSTGRES_SQLSTATE_INTERNAL_ERROR,          "internal error" }
};

/*
d_internal_postgres_sqlstate_char
  A character an SQLSTATE may hold: a digit or a capital letter, tested by
value, as the server writes them in ASCII.
*/
static bool
d_internal_postgres_sqlstate_char(
    char _character
)
{
    return ( ( (_character >= '0') && (_character <= '9') ) ||
             ( (_character >= 'A') && (_character <= 'Z') ) );
}

/*
d_internal_postgres_own_message
  The layer's words for the five SQLSTATEs it gives itself, the manual's for
the other conditions its headers name, or NULL for any other result.
*/
static const char*
d_internal_postgres_own_message(
    int _result
)
{
    switch (_result)
    {
        case D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION:
            return "the connection could not run the statement";

        case D_POSTGRES_SQLSTATE_CANNOT_CONNECT:
            return "no connection to the server could be made";

        case D_POSTGRES_SQLSTATE_CONNECTION_FAILURE:
            return "the connection to the server was lost";

        case D_POSTGRES_SQLSTATE_OUT_OF_MEMORY:
            return "out of memory";

        case D_POSTGRES_SQLSTATE_IO_ERROR:
            return "the file could not be read or written";

        default:
            break;
    }

    const size_t count = sizeof(d_internal_postgres_conditions) /
                         sizeof(d_internal_postgres_conditions[0]);

    // a condition the headers name, in the manual's words
    for (size_t i = 0u; i < count; ++i)
    {
        if (d_internal_postgres_conditions[i].code == _result)
        {
            return d_internal_postgres_conditions[i].words;
        }
    }

    return NULL;
}

#if (D_INTERNAL_POSTGRES == 1)

// d_internal_postgres_feature_releases
//   table: the server version, as PQserverVersion numbers it, that brought
// each d_postgres_feature, in the enum's order, as postgres_common.h lists
// them.
static const int d_internal_postgres_feature_releases[] =
{
    110000,  // PROCEDURES
    110000,  // COVERING_INDEXES
    120000,  // GENERATED_COLUMNS
    120000,  // REINDEX_CONCURRENTLY
    130000,  // DROP_DATABASE_FORCE
    130000,  // TRUSTED_EXTENSIONS
    140000,  // MULTIRANGES
    140000,  // REPLACE_TRIGGER
    150000,  // MERGE
    150000,  // NULLS_NOT_DISTINCT
    160000,  // SQL_JSON
    170000,  // JSON_TABLE
    180000,  // VIRTUAL_COLUMNS
    180000,  // UUIDV7
    180000,  // RETURNING_OLD_NEW
    120000   // JSONPATH
};

D_STATIC_ASSERT( (sizeof(d_internal_postgres_feature_releases) /
                  sizeof(d_internal_postgres_feature_releases[0])) ==
                 ((size_t)D_POSTGRES_FEATURE_JSONPATH + 1u),
                 "every d_postgres_feature has a release" );

// d_internal_postgres_client_feature_releases
//   table: the libpq version, as PQlibVersion numbers it, that brought each
// d_postgres_client_feature, in the enum's order; 0 where the header the
// layer was compiled against does not declare it, which no library loaded
// can mend.
static const int d_internal_postgres_client_feature_releases[] =
{
#if defined(LIBPQ_HAS_PIPELINING)
    140000,  // PIPELINE
#else
    0,       // PIPELINE
#endif
#if defined(LIBPQ_HAS_CHUNK_MODE)
    170000,  // CHUNKED_ROWS
#else
    0,       // CHUNKED_ROWS
#endif
#if defined(LIBPQ_HAS_CLOSE_PREPARED)
    170000   // CLOSE_PREPARED
#else
    0        // CLOSE_PREPARED
#endif
};

D_STATIC_ASSERT( (sizeof(d_internal_postgres_client_feature_releases) /
                  sizeof(d_internal_postgres_client_feature_releases[0])) ==
                 ((size_t)D_POSTGRES_CLIENT_CLOSE_PREPARED + 1u),
                 "every d_postgres_client_feature has a release" );

#endif  // D_INTERNAL_POSTGRES

//==============================================================================
// 3.  RESULTS
//==============================================================================
// Built over libpq or not: reading a result needs nothing of it.

// 3.3    Reading results
//------------------------------------------------------------------------------
/*
d_postgres_result_sqlstate
  Each character is six bits, the first lowest, as D_POSTGRES_SQLSTATE packs
it; a value with bits above the thirtieth, or a character no SQLSTATE holds,
was never packed from one.
*/
bool
d_postgres_result_sqlstate(
    int  _result,
    char _sqlstate[6]
)
{
    // somewhere to write
    if (!_sqlstate)
    {
        return false;
    }

    _sqlstate[0] = '\0';

    // the layer's own results are not SQLSTATEs
    if ( (_result < 0) ||
         (_result >= (1 << 30)) )
    {
        return false;
    }

    for (int i = 0; i < 5; ++i)
    {
        const char character = (char)(((_result >> (6 * i)) & 0x3F) + '0');

        // a character no SQLSTATE holds
        if (!d_internal_postgres_sqlstate_char(character))
        {
            _sqlstate[0] = '\0';

            return false;
        }

        _sqlstate[i] = character;
    }

    _sqlstate[5] = '\0';

    return true;
}

/*
d_postgres_result_from_sqlstate
  Five digits or capital letters, and nothing after them.
*/
int
d_postgres_result_from_sqlstate(
    const char* _sqlstate
)
{
    int packed = 0;

    // parameter validation
    if (!_sqlstate)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    for (int i = 0; i < 5; ++i)
    {
        // a character no SQLSTATE holds, the terminator among them
        if (!d_internal_postgres_sqlstate_char(_sqlstate[i]))
        {
            return D_DB_INVALID_ARGUMENT;
        }

        packed += ((_sqlstate[i] - '0') & 0x3F) << (6 * i);
    }

    return (_sqlstate[5] == '\0') ? packed : D_DB_INVALID_ARGUMENT;
}

/*
d_postgres_result_message
  db_common.h's words for its own values -- save D_DB_UNSUPPORTED in a build
without libpq, which the build explains -- the layer's for the five codes it
gives itself, the manual's for the conditions the headers name and for any
other code's class; a value that is none of these is "unknown result".
*/
const char*
d_postgres_result_message(
    int _result
)
{
    const size_t count       = sizeof(d_internal_postgres_classes) /
                               sizeof(d_internal_postgres_classes[0]);
    char         sqlstate[6] = "";

    // without libpq, the build is what cannot
    if ( (_result == D_DB_UNSUPPORTED) &&
         (D_INTERNAL_POSTGRES == 0) )
    {
        return "this build of the PostgreSQL layer has no libpq";
    }

    // the layer's own results, in db_common.h's words
    if (_result <= 0)
    {
        const char* const message = d_db_status_message(_result);

        return (message) ? message : "unknown result";
    }

    const char* const own = d_internal_postgres_own_message(_result);

    // the codes the layer gives, then any other's class
    if (own)
    {
        return own;
    }

    if (d_postgres_result_sqlstate(_result, sqlstate))
    {
        for (size_t i = 0u; i < count; ++i)
        {
            if (strncmp(sqlstate, d_internal_postgres_classes[i].code, 2u) == 0)
            {
                return d_internal_postgres_classes[i].words;
            }
        }

        return "an error of a class this layer does not know";
    }

    return "unknown result";
}

#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// 4.  LIBRARY
//==============================================================================

// 4.1    Client
//------------------------------------------------------------------------------
/*
d_postgres_client_version_number
  The loaded library's own number.
*/
int
d_postgres_client_version_number(
    void
)
{
    return PQlibVersion();
}

/*
d_postgres_client_supports
  The header compiled against must declare the feature, and the library loaded
be of its release or later: a shared libpq older than the header lacks the
calls the layer makes.
*/
bool
d_postgres_client_supports(
    enum d_postgres_client_feature _feature
)
{
    const size_t count =
        sizeof(d_internal_postgres_client_feature_releases) /
        sizeof(d_internal_postgres_client_feature_releases[0]);

    // a feature there is, in this build
    if ( ((size_t)_feature >= count) ||
         (d_internal_postgres_client_feature_releases[_feature] == 0) )
    {
        return false;
    }

    return (PQlibVersion() >=
            d_internal_postgres_client_feature_releases[_feature]);
}

//==============================================================================
// 5.  SERVERS
//==============================================================================

// 5.1    Versions
//------------------------------------------------------------------------------
/*
d_postgres_server_version
  The server_version parameter the server reported when the connection was
made; libpq keeps it even after the connection is lost, so the connection's
state is asked first.
*/
const char*
d_postgres_server_version(
    const struct pg_conn* _db
)
{
    return ( (_db) &&
             (PQstatus(_db) == CONNECTION_OK) )
               ? PQparameterStatus(_db, "server_version")
               : NULL;
}

/*
d_postgres_server_version_number
  libpq's own number, 0 for a lost connection.
*/
int
d_postgres_server_version_number(
    const struct pg_conn* _db
)
{
    return (_db) ? PQserverVersion(_db) : 0;
}

/*
d_postgres_server_at_least
  The version is taken apart and compared a part at a time, so no argument,
however large, can overflow a number. Before 10 a version had three parts,
the minor the middle one (9.6.24 is 90624).
*/
bool
d_postgres_server_at_least(
    const struct pg_conn* _db,
    unsigned              _major,
    unsigned              _minor
)
{
    const int version = d_postgres_server_version_number(_db);

    // a server, known
    if (version <= 0)
    {
        return false;
    }

    const unsigned major = (unsigned)(version / 10000);
    const unsigned minor = (version >= 100000)
                               ? (unsigned)(version % 10000)
                               : (unsigned)((version / 100) % 100);

    // the first part that differs decides
    if (major != _major)
    {
        return (major > _major);
    }

    return (minor >= _minor);
}

// 5.2    Features
//------------------------------------------------------------------------------
/*
d_postgres_supports
  The feature's release decides; a value outside the table is no feature.
*/
bool
d_postgres_supports(
    const struct pg_conn*   _db,
    enum d_postgres_feature _feature
)
{
    const size_t count = sizeof(d_internal_postgres_feature_releases) /
                         sizeof(d_internal_postgres_feature_releases[0]);

    // a feature there is, on a server there is
    if ((size_t)_feature >= count)
    {
        return false;
    }

    const int version = d_postgres_server_version_number(_db);

    return ( (version > 0) &&
             (version >= d_internal_postgres_feature_releases[_feature]) );
}

// 5.3    Reported parameters
//------------------------------------------------------------------------------
/*
d_postgres_parameter_status
  libpq's copy of what the server last reported.
*/
const char*
d_postgres_parameter_status(
    const struct pg_conn* _db,
    const char*           _name
)
{
    return ( (_db) &&
             (_name) ) ? PQparameterStatus(_db, _name)
                       : NULL;
}

//==============================================================================
// 6.  CONNECTIONS
//==============================================================================

// 6.1    Errors
//------------------------------------------------------------------------------
/*
d_postgres_error_message
  libpq's record of the last failure on the connection.
*/
const char*
d_postgres_error_message(
    const struct pg_conn* _db
)
{
    return (_db) ? PQerrorMessage(_db) : "";
}

/*
d_postgres_result_is_own
  Memory the layer could not have is its own to report, as any negative
result is; the other six codes are its own when the layer marked them so
on this connection, on this thread, and libpq has not been heard since
(d_internal_postgres_answered).
*/
bool
d_postgres_result_is_own(
    const struct pg_conn* _db,
    int                   _result
)
{
    return ( (_result <= 0)                                 ||
             (_result == D_POSTGRES_SQLSTATE_OUT_OF_MEMORY) ||
             (d_internal_postgres_answered(_db, _result)) );
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_client_version_number
  Without libpq there is no library: 0.
*/
int
d_postgres_client_version_number(
    void
)
{
    return 0;
}

/*
d_postgres_client_supports
  Without libpq, false.
*/
bool
d_postgres_client_supports(
    enum d_postgres_client_feature _feature
)
{
    (void)_feature;

    return false;
}

/*
d_postgres_server_version
  Without libpq there is no server: NULL.
*/
const char*
d_postgres_server_version(
    const struct pg_conn* _db
)
{
    (void)_db;

    return NULL;
}

/*
d_postgres_server_version_number
  Without libpq there is no server: 0.
*/
int
d_postgres_server_version_number(
    const struct pg_conn* _db
)
{
    (void)_db;

    return 0;
}

/*
d_postgres_server_at_least
  Without libpq there is no server: false.
*/
bool
d_postgres_server_at_least(
    const struct pg_conn* _db,
    unsigned              _major,
    unsigned              _minor
)
{
    (void)_db;
    (void)_major;
    (void)_minor;

    return false;
}

/*
d_postgres_supports
  Without libpq there is no server: false.
*/
bool
d_postgres_supports(
    const struct pg_conn*   _db,
    enum d_postgres_feature _feature
)
{
    (void)_db;
    (void)_feature;

    return false;
}

/*
d_postgres_parameter_status
  Without libpq there is no server: NULL.
*/
const char*
d_postgres_parameter_status(
    const struct pg_conn* _db,
    const char*           _name
)
{
    (void)_db;
    (void)_name;

    return NULL;
}

/*
d_postgres_error_message
  Without libpq there is no connection to have failed: "".
*/
const char*
d_postgres_error_message(
    const struct pg_conn* _db
)
{
    (void)_db;

    return "";
}

/*
d_postgres_result_is_own
  Without libpq every result is the layer's own.
*/
bool
d_postgres_result_is_own(
    const struct pg_conn* _db,
    int                   _result
)
{
    (void)_db;
    (void)_result;

    return true;
}

#endif  // D_INTERNAL_POSTGRES
