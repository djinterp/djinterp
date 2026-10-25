/*******************************************************************************
* djinterp [c]                                                mariadb_database.c
*
* Implementation of mariadb_database.h.
*   Databases are asked after in information_schema.SCHEMATA, as every other
* object is in its own table there, and match as the server's
* lower_case_table_names matches them. Both servers have CREATE DATABASE IF
* NOT EXISTS and DROP DATABASE IF EXISTS, so the flags are the servers' own.
* The current database is the client's to change, with mysql_select_db.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_database.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.22
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_database.h"  // corresponding header
// std
#include <stddef.h>  // NULL
#include <stdlib.h>  // free
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_IF_*, D_DB_*
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// 1.  DATABASES
//==============================================================================

// 1.1    Existence
//------------------------------------------------------------------------------
/*
d_mariadb_has_database
  The _s form, with every failure read as "no".
*/
bool
d_mariadb_has_database(
    struct d_mariadb* _db,
    const char*       _name
)
{
    bool exists = false;

    return ( (d_mariadb_has_database_s(_db, _name, &exists) == 0) &&
             (exists) );
}

/*
d_mariadb_has_database_s
  A row of SCHEMATA, which lists the databases the user can see.
*/
int
d_mariadb_has_database_s(
    struct d_mariadb* _db,
    const char*       _name,
    bool*             _exists
)
{
    const struct d_internal_mariadb_path path = { NULL, NULL, _name };

    return d_internal_mariadb_has(_db,
                                  D_INTERNAL_MARIADB_DATABASE,
                                  &path,
                                  _exists);
}

// 1.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_mariadb_create_database
  The layer's character set, unless the build left it to the server, is the
new database's default, which its tables then inherit.
*/
int
d_mariadb_create_database(
    struct d_mariadb* _db,
    const char*       _name,
    unsigned          _flags
)
{
    // parameter validation
    if ( (!_db)                                 ||
         (!d_internal_mariadb_name_ok(_name))   ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const char* const charset = D_INTERNAL_MARIADB_CHARSET;
    const char* const exists  = ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
                                    ? "IF NOT EXISTS "
                                    : "";

    return (charset[0] != '\0')
               ? d_internal_mariadb_run(_db,
                                        NULL,
                                        "CREATE DATABASE %s%I "
                                        "DEFAULT CHARACTER SET %I",
                                        exists,
                                        _name,
                                        charset)
               : d_internal_mariadb_run(_db,
                                        NULL,
                                        "CREATE DATABASE %s%I",
                                        exists,
                                        _name);
}

/*
d_mariadb_drop_database
  The servers' own DROP DATABASE [IF EXISTS].
*/
int
d_mariadb_drop_database(
    struct d_mariadb* _db,
    const char*       _name,
    unsigned          _flags
)
{
    // parameter validation
    if ( (!_db)                               ||
         (!d_internal_mariadb_name_ok(_name)) ||
         ((_flags & ~D_DB_IF_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_run(_db,
                                  NULL,
                                  "DROP DATABASE %s%I",
                                  ((_flags & D_DB_IF_EXISTS) != 0u)
                                      ? "IF EXISTS "
                                      : "",
                                  _name);
}

// 1.3    Listing
//------------------------------------------------------------------------------
/*
d_mariadb_list_databases
  Every row of SCHEMATA, in byte order.
*/
int
d_mariadb_list_databases(
    struct d_mariadb*      _db,
    struct d_db_name_list* _names
)
{
    const struct d_internal_mariadb_path path = { NULL, NULL, NULL };

    return d_internal_mariadb_list(_db,
                                   D_INTERNAL_MARIADB_DATABASE,
                                   &path,
                                   _names);
}

//==============================================================================
// 2.  THE CURRENT DATABASE
//==============================================================================

// 2.1    Choosing and asking
//------------------------------------------------------------------------------
/*
d_mariadb_use_database
  mysql_select_db, which also tells the client, for a reconnect, where the
connection was; the server's reply reports the change, where the client
reads it (D_INTERNAL_MARIADB_READS_SIMPLE_OK).
*/
int
d_mariadb_use_database(
    struct d_mariadb* _db,
    const char*       _name
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_mariadb_name_ok(_name)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int idle = d_internal_mariadb_idle(_db);

    // nothing is sent while a statement's rows are still arriving
    if (idle != 0)
    {
        return idle;
    }

    const int result = (mysql_select_db(_db->native, _name) == 0)
                           ? 0
                           : (int)mysql_errno(_db->native);

    d_internal_mariadb_replied(
        _db,
        ( (result == 0) &&
          (D_INTERNAL_MARIADB_READS_SIMPLE_OK == 1) ));

    return result;
}

/*
d_mariadb_current_database
  DATABASE() is NULL when no database is current, which is D_DB_NOT_FOUND
here.
*/
int
d_mariadb_current_database(
    struct d_mariadb* _db,
    char**            _name
)
{
    char* name = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_name) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_query(_db,
                                                &name,
                                                "SELECT DATABASE()");

    // a failure, or no database
    if (result != 0)
    {
        return result;
    }

    if (!name)
    {
        return D_DB_NOT_FOUND;
    }

    *_name = name;

    return 0;
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot
// fail. An answer a function promises even on failure is still written.

/*
d_mariadb_has_database
  Without a client, false.
*/
bool
d_mariadb_has_database(
    struct d_mariadb* _db,
    const char*       _name
)
{
    (void)_db;
    (void)_name;

    return false;
}

/*
d_mariadb_has_database_s
  Without a client there is nothing to look in: D_DB_UNSUPPORTED, and the
answer false, as a failure leaves it.
*/
int
d_mariadb_has_database_s(
    struct d_mariadb* _db,
    const char*       _name,
    bool*             _exists
)
{
    (void)_db;
    (void)_name;

    // a failure answers no, where there is somewhere to say it
    if (_exists)
    {
        *_exists = false;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_create_database
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_create_database(
    struct d_mariadb* _db,
    const char*       _name,
    unsigned          _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_drop_database
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_drop_database(
    struct d_mariadb* _db,
    const char*       _name,
    unsigned          _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_list_databases
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_list_databases(
    struct d_mariadb*      _db,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_use_database
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_use_database(
    struct d_mariadb* _db,
    const char*       _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_current_database
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_current_database(
    struct d_mariadb* _db,
    char**            _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MARIADB
