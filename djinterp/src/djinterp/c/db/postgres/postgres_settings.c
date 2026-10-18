/*******************************************************************************
* djinterp [c]                                               postgres_settings.c
*
* Implementation of postgres_settings.h.
*   Settings are set with set_config(), whose name and value are values the
* layer quotes, rather than with SET, whose value would be SQL: nothing the
* caller passes can end the statement. Timeouts and the isolation level, which
* are numbers and keywords the layer writes itself, use SET. Timeouts are
* read from pg_settings, which gives them in their unit, milliseconds, where
* SHOW would write "5s".
*   The client encoding and the time zone are among the parameters the server
* reports to libpq whenever they change, so they are read from libpq's copy,
* and libpq quotes values for the encoding the server last reported.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_settings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_settings.h"  // corresponding header
// std
#include <limits.h>  // INT_MAX
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// d_internal_postgres_levels
//   table: each d_postgres_isolation as SQL writes it, in the enum's order,
// which is also how the server reports it, in lowercase.
static const char* const d_internal_postgres_levels[] =
{
    "READ UNCOMMITTED",
    "READ COMMITTED",
    "REPEATABLE READ",
    "SERIALIZABLE"
};

// d_internal_postgres_reported_levels
//   table: each level as default_transaction_isolation reads, in the enum's
// order.
static const char* const d_internal_postgres_reported_levels[] =
{
    "read uncommitted",
    "read committed",
    "repeatable read",
    "serializable"
};

D_STATIC_ASSERT( (sizeof(d_internal_postgres_levels) /
                  sizeof(d_internal_postgres_levels[0])) ==
                 ((size_t)D_POSTGRES_ISOLATION_SERIALIZABLE + 1u),
                 "every d_postgres_isolation has a spelling" );

D_STATIC_ASSERT( (sizeof(d_internal_postgres_reported_levels) /
                  sizeof(d_internal_postgres_reported_levels[0])) ==
                 ((size_t)D_POSTGRES_ISOLATION_SERIALIZABLE + 1u),
                 "every d_postgres_isolation has a reported spelling" );

/*
d_internal_postgres_set_timeout
  SET of a timeout, a number of milliseconds, which is the unit both take
without one; the server's own bound is INT_MAX.
*/
static int
d_internal_postgres_set_timeout(
    PGconn*     _db,
    const char* _name,
    unsigned    _milliseconds
)
{
    // parameter validation
    if ( (!_db) ||
         (_milliseconds > (unsigned)INT_MAX) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(_db,
                                   NULL,
                                   "SET %s = %u",
                                   _name,
                                   _milliseconds);
}

/*
d_internal_postgres_get_timeout
  pg_settings gives the setting in its unit, milliseconds, as text.
*/
static int
d_internal_postgres_get_timeout(
    PGconn*     _db,
    const char* _name,
    unsigned*   _milliseconds
)
{
    int64_t value = 0;

    // parameter validation
    if ( (!_db) ||
         (!_milliseconds) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_postgres_query_int64(
                           _db,
                           &value,
                           "SELECT setting FROM pg_catalog.pg_settings "
                           "WHERE name = %Q",
                           _name);

    // a failure, or a number no timeout can be
    if (result != 0)
    {
        return result;
    }

    if ( (value < 0) ||
         (value > INT_MAX) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_milliseconds = (unsigned)value;

    return 0;
}

/*
d_internal_postgres_reported
  A copy of a parameter the server reports to libpq, in memory the caller
releases with d_db_free; a lost connection reports nothing.
*/
static int
d_internal_postgres_reported(
    PGconn*     _db,
    const char* _name,
    char**      _value
)
{
    // parameter validation
    if ( (!_db) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const char* const value = PQparameterStatus(_db, _name);

    // a lost connection, or a server that never said
    if ( (PQstatus(_db) != CONNECTION_OK) ||
         (!value) )
    {
        return (PQstatus(_db) != CONNECTION_OK)
                   ? D_POSTGRES_SQLSTATE_CONNECTION_FAILURE
                   : D_DB_NOT_FOUND;
    }

    const size_t length = strlen(value);
    char* const  copy   = (char*)malloc(length + 1u);

    if (!copy)
    {
        return D_POSTGRES_SQLSTATE_OUT_OF_MEMORY;
    }

    memcpy(copy, value, length + 1u);
    *_value = copy;

    return 0;
}

/*
d_internal_postgres_set_config
  set_config(name, value, false): for the session, name and value quoted.
*/
static int
d_internal_postgres_set_config(
    PGconn*     _db,
    const char* _name,
    const char* _value
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_name_ok(_name)) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(_db,
                                   NULL,
                                   "SELECT pg_catalog.set_config(%Q, %Q, "
                                   "false)",
                                   _name,
                                   _value);
}

/*
d_internal_postgres_current_setting
  current_setting(name), copied out; an unknown name is the server's 42704.
*/
static int
d_internal_postgres_current_setting(
    PGconn*     _db,
    const char* _name,
    char**      _value
)
{
    char* value = NULL;

    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_name_ok(_name)) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_postgres_query(
                           _db,
                           &value,
                           "SELECT pg_catalog.current_setting(%Q)",
                           _name);

    // a failure, or a setting no server leaves NULL
    if (result != 0)
    {
        return result;
    }

    if (!value)
    {
        return D_DB_NOT_FOUND;
    }

    *_value = value;

    return 0;
}

//==============================================================================
// 1.  SETTINGS
//==============================================================================

// 1.2    The search path
//------------------------------------------------------------------------------
/*
d_postgres_set_search_path
  The list as one value, which the server parses as it parses SET's.
*/
int
d_postgres_set_search_path(
    struct pg_conn* _db,
    const char*     _path
)
{
    return d_internal_postgres_set_config(_db, "search_path", _path);
}

/*
d_postgres_get_search_path
  As the server holds it, "$user", public by default.
*/
int
d_postgres_get_search_path(
    struct pg_conn* _db,
    char**          _path
)
{
    return d_internal_postgres_current_setting(_db, "search_path", _path);
}

// 1.3    Timeouts
//------------------------------------------------------------------------------
/*
d_postgres_set_statement_timeout
  statement_timeout, in milliseconds.
*/
int
d_postgres_set_statement_timeout(
    struct pg_conn* _db,
    unsigned        _milliseconds
)
{
    return d_internal_postgres_set_timeout(_db,
                                           "statement_timeout",
                                           _milliseconds);
}

/*
d_postgres_get_statement_timeout
  statement_timeout, in milliseconds.
*/
int
d_postgres_get_statement_timeout(
    struct pg_conn* _db,
    unsigned*       _milliseconds
)
{
    return d_internal_postgres_get_timeout(_db,
                                           "statement_timeout",
                                           _milliseconds);
}

/*
d_postgres_set_lock_timeout
  lock_timeout, in milliseconds.
*/
int
d_postgres_set_lock_timeout(
    struct pg_conn* _db,
    unsigned        _milliseconds
)
{
    return d_internal_postgres_set_timeout(_db, "lock_timeout", _milliseconds);
}

/*
d_postgres_get_lock_timeout
  lock_timeout, in milliseconds.
*/
int
d_postgres_get_lock_timeout(
    struct pg_conn* _db,
    unsigned*       _milliseconds
)
{
    return d_internal_postgres_get_timeout(_db, "lock_timeout", _milliseconds);
}

// 1.4    Isolation
//------------------------------------------------------------------------------
/*
d_postgres_set_isolation_level
  The session's characteristics, which set default_transaction_isolation.
*/
int
d_postgres_set_isolation_level(
    struct pg_conn*           _db,
    enum d_postgres_isolation _level
)
{
    // parameter validation
    if ( (!_db) ||
         ((unsigned)_level > (unsigned)D_POSTGRES_ISOLATION_SERIALIZABLE) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(_db,
                                   NULL,
                                   "SET SESSION CHARACTERISTICS AS "
                                   "TRANSACTION ISOLATION LEVEL %s",
                                   d_internal_postgres_levels[_level]);
}

/*
d_postgres_get_isolation_level
  default_transaction_isolation, as the server spells it; a spelling the
layer does not know is not one of the enum's.
*/
int
d_postgres_get_isolation_level(
    struct pg_conn*            _db,
    enum d_postgres_isolation* _level
)
{
    char* value = NULL;

    // parameter validation
    if (!_level)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_postgres_current_setting(
                     _db,
                     "default_transaction_isolation",
                     &value);

    // the spelling, then its level
    if (result != 0)
    {
        return result;
    }

    result = D_DB_INVALID_ARGUMENT;

    for (size_t i = 0u; i <= (size_t)D_POSTGRES_ISOLATION_SERIALIZABLE; ++i)
    {
        if (strcmp(value, d_internal_postgres_reported_levels[i]) == 0)
        {
            *_level = (enum d_postgres_isolation)i;
            result  = 0;

            break;
        }
    }

    free(value);

    return result;
}

// 1.5    Encoding and time zone
//------------------------------------------------------------------------------
/*
d_postgres_set_client_encoding
  set_config, after which the server reports the new encoding to libpq,
which quotes values for it from then on.
*/
int
d_postgres_set_client_encoding(
    struct pg_conn* _db,
    const char*     _encoding
)
{
    // parameter validation
    if (!d_internal_postgres_name_ok(_encoding))
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_set_config(_db, "client_encoding", _encoding);
}

/*
d_postgres_get_client_encoding
  libpq's copy of what the server last reported.
*/
int
d_postgres_get_client_encoding(
    struct pg_conn* _db,
    char**          _encoding
)
{
    return d_internal_postgres_reported(_db, "client_encoding", _encoding);
}

/*
d_postgres_set_time_zone
  set_config of TimeZone, which takes a zone's name or an offset.
*/
int
d_postgres_set_time_zone(
    struct pg_conn* _db,
    const char*     _zone
)
{
    // parameter validation
    if (!d_internal_postgres_name_ok(_zone))
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_set_config(_db, "TimeZone", _zone);
}

/*
d_postgres_get_time_zone
  libpq's copy of what the server last reported.
*/
int
d_postgres_get_time_zone(
    struct pg_conn* _db,
    char**          _zone
)
{
    return d_internal_postgres_reported(_db, "TimeZone", _zone);
}

// 1.6    Any parameter
//------------------------------------------------------------------------------
/*
d_postgres_set_parameter
  set_config(name, value, false).
*/
int
d_postgres_set_parameter(
    struct pg_conn* _db,
    const char*     _name,
    const char*     _value
)
{
    return d_internal_postgres_set_config(_db, _name, _value);
}

/*
d_postgres_get_parameter
  current_setting(name).
*/
int
d_postgres_get_parameter(
    struct pg_conn* _db,
    const char*     _name,
    char**          _value
)
{
    return d_internal_postgres_current_setting(_db, _name, _value);
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_set_search_path
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_search_path(
    struct pg_conn* _db,
    const char*     _path
)
{
    (void)_db;
    (void)_path;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_get_search_path
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_get_search_path(
    struct pg_conn* _db,
    char**          _path
)
{
    (void)_db;
    (void)_path;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_set_statement_timeout
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_statement_timeout(
    struct pg_conn* _db,
    unsigned        _milliseconds
)
{
    (void)_db;
    (void)_milliseconds;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_get_statement_timeout
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_get_statement_timeout(
    struct pg_conn* _db,
    unsigned*       _milliseconds
)
{
    (void)_db;
    (void)_milliseconds;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_set_lock_timeout
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_lock_timeout(
    struct pg_conn* _db,
    unsigned        _milliseconds
)
{
    (void)_db;
    (void)_milliseconds;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_get_lock_timeout
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_get_lock_timeout(
    struct pg_conn* _db,
    unsigned*       _milliseconds
)
{
    (void)_db;
    (void)_milliseconds;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_set_isolation_level
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_isolation_level(
    struct pg_conn*           _db,
    enum d_postgres_isolation _level
)
{
    (void)_db;
    (void)_level;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_get_isolation_level
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_get_isolation_level(
    struct pg_conn*            _db,
    enum d_postgres_isolation* _level
)
{
    (void)_db;
    (void)_level;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_set_client_encoding
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_client_encoding(
    struct pg_conn* _db,
    const char*     _encoding
)
{
    (void)_db;
    (void)_encoding;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_get_client_encoding
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_get_client_encoding(
    struct pg_conn* _db,
    char**          _encoding
)
{
    (void)_db;
    (void)_encoding;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_set_time_zone
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_time_zone(
    struct pg_conn* _db,
    const char*     _zone
)
{
    (void)_db;
    (void)_zone;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_get_time_zone
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_get_time_zone(
    struct pg_conn* _db,
    char**          _zone
)
{
    (void)_db;
    (void)_zone;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_set_parameter
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_set_parameter(
    struct pg_conn* _db,
    const char*     _name,
    const char*     _value
)
{
    (void)_db;
    (void)_name;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_get_parameter
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_get_parameter(
    struct pg_conn* _db,
    const char*     _name,
    char**          _value
)
{
    (void)_db;
    (void)_name;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_POSTGRES
