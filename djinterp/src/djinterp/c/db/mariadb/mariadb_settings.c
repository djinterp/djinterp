/*******************************************************************************
* djinterp [c]                                                mariadb_settings.c
*
* Implementation of mariadb_settings.h.
*   Each setter is a SET SESSION, or the client's own call where the client
* must know the setting too: the character set, which decides how values are
* escaped. Getters read the session variable, except autocommit, which the
* server reports with every reply, and the character set, which the client
* knows. The isolation level's variable was renamed: MySQL 5.7.20 added
* transaction_isolation and 8.0 removed tx_isolation, and MariaDB has had the
* new name only since 11.1.1, so the getter asks for whichever the server
* has.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_settings.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_settings.h"  // corresponding header
// std
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, free
#include <string.h>  // memcpy, strcmp, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_MARIADB_LOCK_WAIT_MAX
//   constant: the longest lock wait, in seconds, MySQL accepts; MariaDB's
// own ceiling is lower, and it clamps to it.
#define D_INTERNAL_MARIADB_LOCK_WAIT_MAX 1073741824u

// d_internal_mariadb_isolation_names
//   table: each d_mariadb_isolation as the server spells its value, in the
// enum's order.
static const char* const d_internal_mariadb_isolation_names[] =
{
    "READ-UNCOMMITTED",
    "READ-COMMITTED",
    "REPEATABLE-READ",
    "SERIALIZABLE"
};

/*
d_internal_mariadb_set_text
  SET SESSION for a variable whose value is a string, quoted as a value.
*/
static int
d_internal_mariadb_set_text(
    struct d_mariadb* _db,
    const char*       _variable,
    const char*       _value
)
{
    // parameter validation
    if ( (!_db) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "SET SESSION %s = %Q",
                                  _variable,
                                  _value);
}

/*
d_internal_mariadb_get_text
  A session variable's value, as text the caller frees.
*/
static int
d_internal_mariadb_get_text(
    struct d_mariadb* _db,
    const char*       _variable,
    char**            _value
)
{
    // parameter validation
    if ( (!_db) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_query(_db,
                                    _value,
                                    "SELECT @@SESSION.%s",
                                    _variable);
}

//==============================================================================
// 2.  TRANSACTION SETTINGS
//==============================================================================

// 2.1    Autocommit
//------------------------------------------------------------------------------
/*
d_mariadb_set_autocommit
  SET autocommit; turning it on also commits an open transaction, as the
servers do.
*/
int
d_mariadb_set_autocommit(
    struct d_mariadb* _db,
    bool              _enabled
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "SET SESSION autocommit = %d",
                                  (_enabled) ? 1 : 0);
}

/*
d_mariadb_get_autocommit
  SERVER_STATUS_AUTOCOMMIT, as the server's last reply set it: no statement
is needed.
*/
int
d_mariadb_get_autocommit(
    struct d_mariadb* _db,
    bool*             _enabled
)
{
    // parameter validation
    if ( (!_db) ||
         (!_enabled) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_enabled =
        ((_db->native->server_status & SERVER_STATUS_AUTOCOMMIT) != 0u);

    return 0;
}

// 2.2    Isolation level
//------------------------------------------------------------------------------
/*
d_mariadb_set_isolation_level
  SET SESSION TRANSACTION, whose words are the level's name with spaces.
*/
int
d_mariadb_set_isolation_level(
    struct d_mariadb*        _db,
    enum d_mariadb_isolation _level
)
{
    static const char* const levels[] =
    {
        "READ UNCOMMITTED",
        "READ COMMITTED",
        "REPEATABLE READ",
        "SERIALIZABLE"
    };

    // parameter validation
    if ( (!_db) ||
         ((unsigned)_level >= (sizeof(levels) / sizeof(levels[0]))) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "SET SESSION TRANSACTION ISOLATION LEVEL %s",
                                  levels[_level]);
}

/*
d_mariadb_get_isolation_level
  The variable's value names the level with hyphens. A value the layer does
not know -- none exists -- is the unknown error.
*/
int
d_mariadb_get_isolation_level(
    struct d_mariadb*         _db,
    enum d_mariadb_isolation* _level
)
{
    const size_t count = sizeof(d_internal_mariadb_isolation_names) /
                         sizeof(d_internal_mariadb_isolation_names[0]);
    char*        text  = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_level) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const bool renamed =
        ( (d_internal_mariadb_at_least(_db, D_MARIADB_FLAVOR_MYSQL, 50720ul)) ||
          (d_internal_mariadb_at_least(_db,
                                       D_MARIADB_FLAVOR_MARIADB,
                                       110101ul)) );
    int result = d_internal_mariadb_get_text(_db,
                                             (renamed) ? "transaction_isolation"
                                                       : "tx_isolation",
                                             &text);

    // the text, then the level it names
    if (result != 0)
    {
        return result;
    }

    result = D_INTERNAL_MARIADB_UNKNOWN_ERROR;

    for (size_t i = 0u; (text) && (i < count); ++i)
    {
        if (strcmp(text, d_internal_mariadb_isolation_names[i]) == 0)
        {
            *_level = (enum d_mariadb_isolation)i;
            result  = 0;
        }
    }

    free(text);

    return result;
}

// 2.3    Lock waits
//------------------------------------------------------------------------------
/*
d_mariadb_set_next_isolation_level
  SET TRANSACTION without SESSION or GLOBAL, which the server applies to the
next transaction and then forgets.
*/
int
d_mariadb_set_next_isolation_level(
    struct d_mariadb*        _db,
    enum d_mariadb_isolation _level
)
{
    static const char* const levels[] =
    {
        "READ UNCOMMITTED",
        "READ COMMITTED",
        "REPEATABLE READ",
        "SERIALIZABLE"
    };

    // parameter validation
    if ( (!_db) ||
         ((unsigned)_level >= (sizeof(levels) / sizeof(levels[0]))) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "SET TRANSACTION ISOLATION LEVEL %s",
                                  levels[_level]);
}

/*
d_mariadb_set_lock_wait_timeout
  The servers would take 0 and clamp it, with only a warning to say so; the
layer refuses it, and anything past MySQL's ceiling, itself.
*/
int
d_mariadb_set_lock_wait_timeout(
    struct d_mariadb* _db,
    unsigned          _seconds
)
{
    // parameter validation
    if ( (!_db)          ||
         (_seconds == 0u) ||
         (_seconds > D_INTERNAL_MARIADB_LOCK_WAIT_MAX) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "SET SESSION innodb_lock_wait_timeout = %u",
                                  _seconds);
}

/*
d_mariadb_get_lock_wait_timeout
  The session's value, which both servers keep within an unsigned's range.
*/
int
d_mariadb_get_lock_wait_timeout(
    struct d_mariadb* _db,
    unsigned*         _seconds
)
{
    int64_t value = 0;

    // parameter validation
    if ( (!_db) ||
         (!_seconds) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_query_int64(
        _db,
        &value,
        "SELECT @@SESSION.innodb_lock_wait_timeout");

    // a value an unsigned holds, or none
    if (result != 0)
    {
        return result;
    }

    if ( (value < 0) ||
         (value > (int64_t)D_INTERNAL_MARIADB_LOCK_WAIT_MAX) )
    {
        return D_INTERNAL_MARIADB_UNKNOWN_ERROR;
    }

    *_seconds = (unsigned)value;

    return 0;
}

//==============================================================================
// 3.  SESSION SETTINGS
//==============================================================================

// 3.1    Character set
//------------------------------------------------------------------------------
/*
d_mariadb_set_charset
  mysql_set_character_set, which sets the session's character set and the
client's together, so values are escaped for the new one; SET NAMES would
change only the session's.
*/
int
d_mariadb_set_charset(
    struct d_mariadb* _db,
    const char*       _charset
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_mariadb_name_ok(_charset)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (mysql_set_character_set(_db->native, _charset) == 0)
               ? 0
               : (int)mysql_errno(_db->native);
}

/*
d_mariadb_get_charset
  The client's record of it, copied, which needs no statement.
*/
int
d_mariadb_get_charset(
    struct d_mariadb* _db,
    char**            _charset
)
{
    // parameter validation
    if ( (!_db) ||
         (!_charset) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const char* const name   = mysql_character_set_name(_db->native);
    const size_t      length = (name) ? strlen(name) : 0u;
    char* const       copy   = (char*)malloc(length + 1u);

    // the copy is the caller's
    if (!copy)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    memcpy(copy, (name) ? name : "", length + 1u);
    *_charset = copy;

    return 0;
}

// 3.2    Time zone
//------------------------------------------------------------------------------
/*
d_mariadb_set_time_zone
  SET SESSION time_zone, the zone quoted as a value.
*/
int
d_mariadb_set_time_zone(
    struct d_mariadb* _db,
    const char*       _zone
)
{
    return d_internal_mariadb_set_text(_db, "time_zone", _zone);
}

/*
d_mariadb_get_time_zone
  @@SESSION.time_zone.
*/
int
d_mariadb_get_time_zone(
    struct d_mariadb* _db,
    char**            _zone
)
{
    return d_internal_mariadb_get_text(_db, "time_zone", _zone);
}

// 3.3    SQL mode
//------------------------------------------------------------------------------
/*
d_mariadb_set_sql_mode
  SET SESSION sql_mode. The server's reply tells the client whether
NO_BACKSLASH_ESCAPES is now on, and its escaping follows.
*/
int
d_mariadb_set_sql_mode(
    struct d_mariadb* _db,
    const char*       _mode
)
{
    return d_internal_mariadb_set_text(_db, "sql_mode", _mode);
}

/*
d_mariadb_get_sql_mode
  @@SESSION.sql_mode.
*/
int
d_mariadb_get_sql_mode(
    struct d_mariadb* _db,
    char**            _mode
)
{
    return d_internal_mariadb_get_text(_db, "sql_mode", _mode);
}

//==============================================================================
// 4.  ANY VARIABLE
//==============================================================================

// 4.1    Setting and getting
//------------------------------------------------------------------------------
/*
d_mariadb_set_variable
  The name is checked for a variable's shape, and written as it is; the
value is the caller's SQL.
*/
int
d_mariadb_set_variable(
    struct d_mariadb* _db,
    const char*       _name,
    const char*       _value
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_mariadb_variable_ok(_name)) ||
         (!_value) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "SET SESSION %s = %s",
                                  _name,
                                  _value);
}

/*
d_mariadb_get_variable
  @@SESSION.name, the name checked as for d_mariadb_set_variable.
*/
int
d_mariadb_get_variable(
    struct d_mariadb* _db,
    const char*       _name,
    char**            _value
)
{
    // parameter validation
    if (!d_internal_mariadb_variable_ok(_name))
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_get_text(_db, _name, _value);
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_set_autocommit
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_autocommit(
    struct d_mariadb* _db,
    bool              _enabled
)
{
    (void)_db;
    (void)_enabled;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_get_autocommit
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_get_autocommit(
    struct d_mariadb* _db,
    bool*             _enabled
)
{
    (void)_db;
    (void)_enabled;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_set_isolation_level
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_isolation_level(
    struct d_mariadb*        _db,
    enum d_mariadb_isolation _level
)
{
    (void)_db;
    (void)_level;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_get_isolation_level
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_get_isolation_level(
    struct d_mariadb*         _db,
    enum d_mariadb_isolation* _level
)
{
    (void)_db;
    (void)_level;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_set_next_isolation_level
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_next_isolation_level(
    struct d_mariadb*        _db,
    enum d_mariadb_isolation _level
)
{
    (void)_db;
    (void)_level;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_set_lock_wait_timeout
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_lock_wait_timeout(
    struct d_mariadb* _db,
    unsigned          _seconds
)
{
    (void)_db;
    (void)_seconds;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_get_lock_wait_timeout
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_get_lock_wait_timeout(
    struct d_mariadb* _db,
    unsigned*         _seconds
)
{
    (void)_db;
    (void)_seconds;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_set_charset
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_charset(
    struct d_mariadb* _db,
    const char*       _charset
)
{
    (void)_db;
    (void)_charset;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_get_charset
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_get_charset(
    struct d_mariadb* _db,
    char**            _charset
)
{
    (void)_db;
    (void)_charset;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_set_time_zone
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_time_zone(
    struct d_mariadb* _db,
    const char*       _zone
)
{
    (void)_db;
    (void)_zone;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_get_time_zone
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_get_time_zone(
    struct d_mariadb* _db,
    char**            _zone
)
{
    (void)_db;
    (void)_zone;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_set_sql_mode
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_sql_mode(
    struct d_mariadb* _db,
    const char*       _mode
)
{
    (void)_db;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_get_sql_mode
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_get_sql_mode(
    struct d_mariadb* _db,
    char**            _mode
)
{
    (void)_db;
    (void)_mode;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_set_variable
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_set_variable(
    struct d_mariadb* _db,
    const char*       _name,
    const char*       _value
)
{
    (void)_db;
    (void)_name;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_get_variable
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_get_variable(
    struct d_mariadb* _db,
    const char*       _name,
    char**            _value
)
{
    (void)_db;
    (void)_name;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MARIADB
