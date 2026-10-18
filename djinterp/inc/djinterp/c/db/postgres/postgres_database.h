/*******************************************************************************
* djinterp [c]                                               postgres_database.h
*
* The databases on a PostgreSQL server, the schemas a database is divided
* into, and the extensions installed in it.
*   A server holds many databases, but a connection works in one, chosen
* when it is made, and SQL cannot name an object in another. Inside it,
* schemas are the namespaces SQL names objects by -- schema.table -- which
* makes a schema what the other layers call a database or a schema: the scope
* every function of postgres_schema.h takes, NULL meaning the current one.
* So d_postgres_has_database asks after databases on the server, and
* d_postgres_has_schema after namespaces in the connection's database.
*   Extensions -- pgcrypto, citext, PostGIS -- are installed in one database
* at a time, into a schema.
*   A database cannot be created or dropped inside a transaction; the server
* refuses with D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION (25001). Schemas and
* extensions can, and roll back with it.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_database.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DATABASES
    ---------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
    4.  The current database
2.  SCHEMAS
    -------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
    4.  The current schema
3.  EXTENSIONS
    ----------
    1.  Existence
    2.  Creating and dropping
    3.  Listing
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_DATABASE_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_DATABASE_H 1

// djinterp
#include "../../djinterp.h"     // framework root
#include "../db_common.h"       // d_db_name_list, D_DB_IF_*
#include "./postgres_common.h"  // pg_conn, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DATABASES
//==============================================================================


// 1.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the server has a database of that name. The plain form
// answers false for a failure or a bad argument; the _s form returns 0 and
// writes the answer, or the failure, the answer then false.
bool         d_postgres_has_database(struct pg_conn* _db,
                                     const char*     _name) D_NOTHROW;
int          d_postgres_has_database_s(struct pg_conn* _db,
                                       const char*     _name,
                                       bool*           _exists) D_NOTHROW;

// 1.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Creates a database, as a copy of template1, as CREATE DATABASE does
 *        without options.
 *
 * @param[in] _db     the connection.
 * @param[in] _name   the database.
 * @param[in] _flags  0, or D_DB_IF_NOT_EXISTS to succeed when it is there,
 *                    which the layer settles itself, since the server has
 *                    no clause for it.
 * @return 0; the SQLSTATE of a failure (42P04, it exists; 42501, no
 *         privilege; 25001, inside a transaction); or D_DB_INVALID_ARGUMENT
 *         for a `NULL` argument, an empty name or another flag.
 */
int          d_postgres_create_database(struct pg_conn* _db,
                                        const char*     _name,
                                        unsigned        _flags) D_NOTHROW;
/**
 * @brief Drops a database and everything in it.
 *
 * @param[in] _db     the connection, which must be to another database.
 * @param[in] _name   the database.
 * @param[in] _flags  D_DB_IF_EXISTS, to succeed when it is not there, and
 *                    D_POSTGRES_FORCE, in any combination.
 * @return 0; the SQLSTATE of a failure (3D000, no such database; 55006,
 *         sessions are connected to it, the current one among them; 25001,
 *         inside a transaction); D_DB_UNSUPPORTED for D_POSTGRES_FORCE
 *         before PostgreSQL 13; or D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument, an empty name or another flag.
 */
int          d_postgres_drop_database(struct pg_conn* _db,
                                      const char*     _name,
                                      unsigned        _flags) D_NOTHROW;

// 1.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists every database on the server, the templates template0 and
 *        template1 and the postgres database among them, sorted by name,
 *        byte by byte.
 *
 * @param[in]  _db     the connection.
 * @param[out] _names  receives the names; written only on success.
 * @return 0, the SQLSTATE of a failure, 53200 when the list could not be
 *         built, or D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int          d_postgres_list_databases(struct pg_conn*        _db,
                                       struct d_db_name_list* _names) D_NOTHROW;

// 1.4    The current database
//------------------------------------------------------------------------------
// the current database -- the one the connection was made to, libpq's copy
// of its name, valid while the connection is; NULL for NULL. It cannot
// change: connect again to work in another.
const char*  d_postgres_current_database(const struct pg_conn* _db) D_NOTHROW;


//==============================================================================
// 2.  SCHEMAS
//==============================================================================


// 2.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the database has a schema of that name, the system's
// own included; "pg_temp" asks whether the session has a temporary schema
// yet. As for databases, the _s form reports a failure.
bool         d_postgres_has_schema(struct pg_conn* _db,
                                   const char*     _name) D_NOTHROW;
int          d_postgres_has_schema_s(struct pg_conn* _db,
                                     const char*     _name,
                                     bool*           _exists) D_NOTHROW;

// 2.2    Creating and dropping
//------------------------------------------------------------------------------
// creating -- CREATE SCHEMA; D_DB_IF_NOT_EXISTS makes an existing one a
// success. 0, the SQLSTATE of a failure (42P06, it exists; 42939, a name
// beginning pg_, which is the system's), or D_DB_INVALID_ARGUMENT for a NULL
// argument, an empty name or another flag.
int          d_postgres_create_schema(struct pg_conn* _db,
                                      const char*     _name,
                                      unsigned        _flags) D_NOTHROW;
// dropping -- DROP SCHEMA; D_DB_IF_EXISTS makes a missing one a success, and
// D_POSTGRES_CASCADE drops what it holds with it, where without it a schema
// that holds anything is refused (2BP01). 0, the SQLSTATE of a failure
// (3F000, no such schema), or D_DB_INVALID_ARGUMENT.
int          d_postgres_drop_schema(struct pg_conn* _db,
                                    const char*     _name,
                                    unsigned        _flags) D_NOTHROW;

// 2.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists the database's schemas, the system's own left out -- those
 *        whose names begin pg_ (pg_catalog, pg_toast, the temporary ones) and
 *        information_schema -- sorted by name, byte by byte.
 *
 * @param[in]  _db     the connection.
 * @param[out] _names  receives the names; written only on success.
 * @return 0, the SQLSTATE of a failure, 53200, or D_DB_INVALID_ARGUMENT for a
 *         `NULL` argument.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int          d_postgres_list_schemas(struct pg_conn*        _db,
                                     struct d_db_name_list* _names) D_NOTHROW;

// 2.4    The current schema
//------------------------------------------------------------------------------
// the current schema -- the first schema of the search path that exists, as
// current_schema() names it: where an unqualified CREATE puts its object,
// and what every NULL schema the layer is given means. Written into _name, a
// string the caller releases with d_db_free: 0, D_DB_NOT_FOUND when no schema
// of the search path exists, or a failure as for the queries.
int          d_postgres_current_schema(struct pg_conn* _db,
                                       char**          _name) D_NOTHROW;


//==============================================================================
// 3.  EXTENSIONS
//==============================================================================


// 3.1    Existence
//------------------------------------------------------------------------------
// existence -- whether the database has the extension installed (not merely
// available to install). As for databases, the _s form reports a failure.
bool         d_postgres_has_extension(struct pg_conn* _db,
                                      const char*     _name) D_NOTHROW;
int          d_postgres_has_extension_s(struct pg_conn* _db,
                                        const char*     _name,
                                        bool*           _exists) D_NOTHROW;

// 3.2    Creating and dropping
//------------------------------------------------------------------------------
/**
 * @brief Installs an extension in the database.
 *
 * @param[in] _db      the connection.
 * @param[in] _name    the extension ("pgcrypto", "citext").
 * @param[in] _schema  the schema its objects go into; `NULL` for the one the
 *                     extension names, or else the current schema.
 * @param[in] _flags   D_DB_IF_NOT_EXISTS and D_POSTGRES_CASCADE, which
 *                     installs the extensions it needs, in any combination.
 * @return 0; the SQLSTATE of a failure (42710, it is installed; 58P01, or
 *         0A000 from PostgreSQL 19, the server has no such extension to
 *         install; 42501, no privilege -- before PostgreSQL 13 every
 *         extension needs a superuser, and from 13 only those not marked
 *         trusted do); or D_DB_INVALID_ARGUMENT for a `NULL` argument, an
 *         empty name or another flag.
 */
int          d_postgres_create_extension(struct pg_conn* _db,
                                         const char*     _name,
                                         const char*     _schema,
                                         unsigned        _flags) D_NOTHROW;
// dropping -- DROP EXTENSION; D_DB_IF_EXISTS makes a missing one a success,
// and D_POSTGRES_CASCADE drops what depends on it, such as a column of its
// type. 0, the SQLSTATE of a failure (42704, not installed), or
// D_DB_INVALID_ARGUMENT.
int          d_postgres_drop_extension(struct pg_conn* _db,
                                       const char*     _name,
                                       unsigned        _flags) D_NOTHROW;

// 3.3    Listing
//------------------------------------------------------------------------------
// lists the extensions installed in the database, plpgsql among them, sorted
// by name; as d_postgres_list_schemas.
int          d_postgres_list_extensions(struct pg_conn*        _db,
                                        struct d_db_name_list* _names)
                                        D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_DATABASE_H
