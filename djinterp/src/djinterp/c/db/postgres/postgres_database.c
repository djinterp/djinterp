/*******************************************************************************
* djinterp [c]                                               postgres_database.c
*
* Implementation of postgres_database.h.
*   Databases, schemas and extensions are asked after in pg_database,
* pg_namespace and pg_extension (postgres_internal.c). The server has IF NOT
* EXISTS for schemas and extensions, and IF EXISTS for every drop; for a
* database it has no IF NOT EXISTS, so the layer runs CREATE DATABASE and
* reads "already exists" as success, which, unlike asking first, cannot race
* another connection creating it.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_database.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_database.h"  // corresponding header
// std
#include <stddef.h>  // NULL
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_IF_*, D_DB_*
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing


#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// 1.  DATABASES
//==============================================================================

// 1.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_database
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_database(
    struct pg_conn* _db,
    const char*     _name
)
{
    bool exists = false;

    return ( (d_postgres_has_database_s(_db, _name, &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_database_s
  A row of pg_database, which lists every database whatever the user may do
in it.
*/
int
d_postgres_has_database_s(
    struct pg_conn* _db,
    const char*     _name,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { NULL, NULL, _name };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_DATABASE,
                                   &path,
                                   _exists);
}

// 1.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_database
  CREATE DATABASE has no IF NOT EXISTS; the layer reads 42P04 as success. It
cannot run inside a transaction, so no savepoint is ever needed: the server
refuses it there first.
*/
int
d_postgres_create_database(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_postgres_name_ok(_name))   ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return ((_flags & D_DB_IF_NOT_EXISTS) != 0u)
               ? d_internal_postgres_tolerate(
                     _db,
                     D_POSTGRES_SQLSTATE_DUPLICATE_DATABASE,
                     "CREATE DATABASE %I",
                     _name)
               : d_internal_postgres_run(_db,
                                         NULL,
                                         "CREATE DATABASE %I",
                                         _name);
}

/*
d_postgres_drop_database
  The server's own DROP DATABASE [IF EXISTS], and WITH (FORCE) from 13.
*/
int
d_postgres_drop_database(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                                ||
         (!d_internal_postgres_name_ok(_name))                 ||
         ((_flags & ~(D_DB_IF_EXISTS | D_POSTGRES_FORCE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // FORCE arrived with 13
    if ( ((_flags & D_POSTGRES_FORCE) != 0u) &&
         (!d_postgres_supports(_db, D_POSTGRES_FEATURE_DROP_DATABASE_FORCE)) )
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_postgres_run(
               _db,
               NULL,
               "DROP DATABASE %s%I%s",
               d_internal_postgres_clause(_flags, D_DB_IF_EXISTS, "IF EXISTS "),
               _name,
               d_internal_postgres_clause(_flags,
                                        D_POSTGRES_FORCE,
                                        " WITH (FORCE)"));
}

// 1.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_databases
  Every row of pg_database, in byte order.
*/
int
d_postgres_list_databases(
    struct pg_conn*        _db,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { NULL, NULL, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_DATABASE,
                                    &path,
                                    _names);
}

// 1.4    The current database
//------------------------------------------------------------------------------
/*
d_postgres_current_database
  libpq's copy of the name the connection was made with, which the server
cannot change under it.
*/
const char*
d_postgres_current_database(
    const struct pg_conn* _db
)
{
    return (_db) ? PQdb(_db) : NULL;
}

//==============================================================================
// 2.  SCHEMAS
//==============================================================================

// 2.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_schema
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_schema(
    struct pg_conn* _db,
    const char*     _name
)
{
    bool exists = false;

    return ( (d_postgres_has_schema_s(_db, _name, &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_schema_s
  A row of pg_namespace, "pg_temp" matched as the session's temporary
schema.
*/
int
d_postgres_has_schema_s(
    struct pg_conn* _db,
    const char*     _name,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { NULL, NULL, _name };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_SCHEMA,
                                   &path,
                                   _exists);
}

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_schema
  The server's own CREATE SCHEMA [IF NOT EXISTS].
*/
int
d_postgres_create_schema(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                  ||
         (!d_internal_postgres_name_ok(_name))   ||
         ((_flags & ~D_DB_IF_NOT_EXISTS) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(
               _db,
               NULL,
               "CREATE SCHEMA %s%I",
               d_internal_postgres_clause(_flags,
                                        D_DB_IF_NOT_EXISTS,
                                        "IF NOT EXISTS "),
               _name);
}

/*
d_postgres_drop_schema
  The server's own DROP SCHEMA [IF EXISTS] ... [CASCADE].
*/
int
d_postgres_drop_schema(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                                  ||
         (!d_internal_postgres_name_ok(_name))                   ||
         ((_flags & ~(D_DB_IF_EXISTS | D_POSTGRES_CASCADE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(
               _db,
               NULL,
               "DROP SCHEMA %s%I%s",
               d_internal_postgres_clause(_flags, D_DB_IF_EXISTS, "IF EXISTS "),
               _name,
               d_internal_postgres_clause(_flags, D_POSTGRES_CASCADE,
                                          " CASCADE"));
}

// 2.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_schemas
  pg_namespace, the system's schemas left out, in byte order.
*/
int
d_postgres_list_schemas(
    struct pg_conn*        _db,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { NULL, NULL, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_SCHEMA,
                                    &path,
                                    _names);
}

// 2.4    The current schema
//------------------------------------------------------------------------------
/*
d_postgres_current_schema
  current_schema() is NULL when no schema of the search path exists, which
is D_DB_NOT_FOUND here.
*/
int
d_postgres_current_schema(
    struct pg_conn* _db,
    char**          _name
)
{
    char* name = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_name) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_postgres_query(
                           _db,
                           &name,
                           "SELECT pg_catalog.current_schema()");

    // a failure, or no schema
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

//==============================================================================
// 3.  EXTENSIONS
//==============================================================================

// 3.1    Existence
//------------------------------------------------------------------------------
/*
d_postgres_has_extension
  The _s form, with every failure read as "no".
*/
bool
d_postgres_has_extension(
    struct pg_conn* _db,
    const char*     _name
)
{
    bool exists = false;

    return ( (d_postgres_has_extension_s(_db, _name, &exists) == 0) &&
             (exists) );
}

/*
d_postgres_has_extension_s
  A row of pg_extension, which lists what is installed, not what could be.
*/
int
d_postgres_has_extension_s(
    struct pg_conn* _db,
    const char*     _name,
    bool*           _exists
)
{
    const struct d_internal_postgres_path path = { NULL, NULL, _name };

    return d_internal_postgres_has(_db,
                                   D_INTERNAL_POSTGRES_EXTENSION,
                                   &path,
                                   _exists);
}

// 3.2    Creating and dropping
//------------------------------------------------------------------------------
/*
d_postgres_create_extension
  The server's own CREATE EXTENSION [IF NOT EXISTS] ... [SCHEMA ...]
[CASCADE]; CASCADE arrived with 9.6, below the floor.
*/
int
d_postgres_create_extension(
    struct pg_conn* _db,
    const char*     _name,
    const char*     _schema,
    unsigned        _flags
)
{
    const unsigned allowed = D_DB_IF_NOT_EXISTS | D_POSTGRES_CASCADE;

    // parameter validation
    if ( (!_db)                                     ||
         (!d_internal_postgres_name_ok(_name))      ||
         (!d_internal_postgres_schema_ok(_schema))  ||
         ((_flags & ~allowed) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const char* const exists  = d_internal_postgres_clause(_flags,
                                                         D_DB_IF_NOT_EXISTS,
                                                         "IF NOT EXISTS ");
    const char* const cascade = d_internal_postgres_clause(_flags,
                                                         D_POSTGRES_CASCADE,
                                                         " CASCADE");

    return (_schema)
               ? d_internal_postgres_run(_db,
                                         NULL,
                                         "CREATE EXTENSION %s%I SCHEMA %I%s",
                                         exists,
                                         _name,
                                         _schema,
                                         cascade)
               : d_internal_postgres_run(_db,
                                         NULL,
                                         "CREATE EXTENSION %s%I%s",
                                         exists,
                                         _name,
                                         cascade);
}

/*
d_postgres_drop_extension
  The server's own DROP EXTENSION [IF EXISTS] ... [CASCADE].
*/
int
d_postgres_drop_extension(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                                  ||
         (!d_internal_postgres_name_ok(_name))                   ||
         ((_flags & ~(D_DB_IF_EXISTS | D_POSTGRES_CASCADE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_run(
               _db,
               NULL,
               "DROP EXTENSION %s%I%s",
               d_internal_postgres_clause(_flags, D_DB_IF_EXISTS, "IF EXISTS "),
               _name,
               d_internal_postgres_clause(_flags, D_POSTGRES_CASCADE,
                                          " CASCADE"));
}

// 3.3    Listing
//------------------------------------------------------------------------------
/*
d_postgres_list_extensions
  Every row of pg_extension, in byte order.
*/
int
d_postgres_list_extensions(
    struct pg_conn*        _db,
    struct d_db_name_list* _names
)
{
    const struct d_internal_postgres_path path = { NULL, NULL, NULL };

    return d_internal_postgres_list(_db,
                                    D_INTERNAL_POSTGRES_EXTENSION,
                                    &path,
                                    _names);
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_has_database
  Without libpq, false.
*/
bool
d_postgres_has_database(
    struct pg_conn* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return false;
}

/*
d_postgres_has_database_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_database_s(
    struct pg_conn* _db,
    const char*     _name,
    bool*           _exists
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
d_postgres_create_database
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_database(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_drop_database
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_database(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_list_databases
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_databases(
    struct pg_conn*        _db,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_current_database
  Without libpq, NULL.
*/
const char*
d_postgres_current_database(
    const struct pg_conn* _db
)
{
    (void)_db;

    return NULL;
}

/*
d_postgres_has_schema
  Without libpq, false.
*/
bool
d_postgres_has_schema(
    struct pg_conn* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return false;
}

/*
d_postgres_has_schema_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_schema_s(
    struct pg_conn* _db,
    const char*     _name,
    bool*           _exists
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
d_postgres_create_schema
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_schema(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_drop_schema
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_schema(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_list_schemas
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_schemas(
    struct pg_conn*        _db,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_current_schema
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_current_schema(
    struct pg_conn* _db,
    char**          _name
)
{
    (void)_db;
    (void)_name;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_has_extension
  Without libpq, false.
*/
bool
d_postgres_has_extension(
    struct pg_conn* _db,
    const char*     _name
)
{
    (void)_db;
    (void)_name;

    return false;
}

/*
d_postgres_has_extension_s
  Without libpq there is nothing to look in: D_DB_UNSUPPORTED,
and the answer false, as a failure leaves it.
*/
int
d_postgres_has_extension_s(
    struct pg_conn* _db,
    const char*     _name,
    bool*           _exists
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
d_postgres_create_extension
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_create_extension(
    struct pg_conn* _db,
    const char*     _name,
    const char*     _schema,
    unsigned        _flags
)
{
    (void)_db;
    (void)_name;
    (void)_schema;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_drop_extension
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_drop_extension(
    struct pg_conn* _db,
    const char*     _name,
    unsigned        _flags
)
{
    (void)_db;
    (void)_name;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_list_extensions
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_list_extensions(
    struct pg_conn*        _db,
    struct d_db_name_list* _names
)
{
    (void)_db;
    (void)_names;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_POSTGRES
