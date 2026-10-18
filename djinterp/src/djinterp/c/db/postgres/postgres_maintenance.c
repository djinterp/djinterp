/*******************************************************************************
* djinterp [c]                                            postgres_maintenance.c
*
* Implementation of postgres_maintenance.h.
*   VACUUM and ANALYZE are written in the form every supported server takes,
* VACUUM FULL ANALYZE table, rather than the parenthesized options 9.0
* brought, so that one spelling serves all. Sizes are read with the server's
* own functions, on an object found as a lookup finds it, so that one that is
* not there is no row -- D_DB_NOT_FOUND -- rather than a failure.
*
*
* path:      /src/djinterp/c/db/postgres/postgres_maintenance.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.16
*******************************************************************************/
// D_INTERNAL_POSTGRES_SOURCE
//   macro: this file is one of the PostgreSQL layer's own sources, for which
// cfg_postgres_common.h has env include libpq-fe.h; it precedes every include.
#define D_INTERNAL_POSTGRES_SOURCE 1

#include "../../../../../inc/djinterp/c/db/postgres/postgres_maintenance.h"  // corresponding header
// std
#include <stddef.h>  // NULL
#include <stdlib.h>  // free
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*
#include "../db_internal.h"  // buffers, formatting
#include "../../../../../inc/djinterp/config/c/db/postgres/cfg_postgres_common.h"  // D_INTERNAL_POSTGRES
#include "./postgres_internal.h"  // libpq-fe.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // INT64_MAX, int64_t


// as its header: below the 64-bit floor there is nothing to define
#if defined(INT64_MAX)

#if (D_INTERNAL_POSTGRES == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/*
d_internal_postgres_upkeep
  VACUUM or ANALYZE, with its words, of one table or of the whole database:
the schema is settled only when there is a table to qualify.
*/
static int
d_internal_postgres_upkeep(
    PGconn*     _db,
    const char* _words,
    const char* _schema,
    const char* _table
)
{
    const char* schema = NULL;
    char*       owned  = NULL;

    // the whole database
    if (!_table)
    {
        return d_internal_postgres_run(_db, NULL, "%s", _words);
    }

    int result = d_internal_postgres_target(_db, _schema, &schema, &owned);

    // the schema, then the table's statement
    if (result == 0)
    {
        result = d_internal_postgres_run(_db,
                                         NULL,
                                         "%s %T",
                                         _words,
                                         schema,
                                         _table);
    }

    free(owned);

    return result;
}

/*
d_internal_postgres_upkeep_ok
  A schema only with a table, and neither empty.
*/
static bool
d_internal_postgres_upkeep_ok(
    const char* _schema,
    const char* _table
)
{
    return ( (_table) ? ( (d_internal_postgres_schema_ok(_schema)) &&
                          (d_internal_postgres_name_ok(_table)) )
                      : (!_schema) );
}

//==============================================================================
// 1.  MAINTENANCE
//==============================================================================

// 1.1    Vacuum and analyze
//------------------------------------------------------------------------------
/*
d_postgres_vacuum
  VACUUM [FULL] [ANALYZE] [table], which the server refuses inside a
transaction.
*/
int
d_postgres_vacuum(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    unsigned        _flags
)
{
    static const char* const forms[] =
    {
        "VACUUM", "VACUUM FULL", "VACUUM ANALYZE", "VACUUM FULL ANALYZE"
    };

    // parameter validation
    if ( (!_db)                                                     ||
         (!d_internal_postgres_upkeep_ok(_schema, _table))           ||
         ((_flags & ~(D_POSTGRES_FULL | D_POSTGRES_ANALYZE)) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const unsigned form = ( ((_flags & D_POSTGRES_FULL) != 0u) ? 1u : 0u ) |
                          ( ((_flags & D_POSTGRES_ANALYZE) != 0u) ? 2u : 0u );

    return d_internal_postgres_upkeep(_db, forms[form], _schema, _table);
}

/*
d_postgres_analyze
  ANALYZE [table].
*/
int
d_postgres_analyze(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table
)
{
    // parameter validation
    if ( (!_db) ||
         (!d_internal_postgres_upkeep_ok(_schema, _table)) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_upkeep(_db, "ANALYZE", _schema, _table);
}

// 1.2    Indexes and checkpoints
//------------------------------------------------------------------------------
/*
d_postgres_reindex
  REINDEX TABLE [CONCURRENTLY], which arrived with 12.
*/
int
d_postgres_reindex(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    unsigned        _flags
)
{
    // parameter validation
    if ( (!_db)                                       ||
         (!d_internal_postgres_schema_ok(_schema))    ||
         (!d_internal_postgres_name_ok(_table))       ||
         ((_flags & ~D_POSTGRES_CONCURRENTLY) != 0u) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // CONCURRENTLY arrived with 12
    if ( ((_flags & D_POSTGRES_CONCURRENTLY) != 0u) &&
         (!d_postgres_supports(_db, D_POSTGRES_FEATURE_REINDEX_CONCURRENTLY)) )
    {
        return D_DB_UNSUPPORTED;
    }

    return d_internal_postgres_upkeep(
               _db,
               ((_flags & D_POSTGRES_CONCURRENTLY) != 0u)
                   ? "REINDEX TABLE CONCURRENTLY"
                   : "REINDEX TABLE",
               _schema,
               _table);
}

/*
d_postgres_checkpoint
  CHECKPOINT, which the server keeps to its superusers, and from 15 to
pg_checkpoint's members too.
*/
int
d_postgres_checkpoint(
    struct pg_conn* _db
)
{
    // parameter validation
    if (!_db)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_postgres_send(_db, "CHECKPOINT", NULL);
}

// 1.3    Sizes
//------------------------------------------------------------------------------
/*
d_postgres_table_size
  pg_total_relation_size() of the table's oid, found as a lookup finds a
table.
*/
int
d_postgres_table_size(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    int64_t*        _bytes
)
{
    struct d_internal_db_buffer        sql     = { NULL, 0u, 0u };
    const struct d_internal_db_quoting quoting =
        d_internal_postgres_quoting(_db);

    // parameter validation
    if ( (!_db)                                    ||
         (!d_internal_postgres_schema_ok(_schema)) ||
         (!d_internal_postgres_name_ok(_table))    ||
         (!_bytes) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    enum d_internal_db_format_status status = d_internal_db_format(
        &sql,
        &quoting,
        "SELECT pg_catalog.pg_total_relation_size(c.oid) "
        "FROM pg_catalog.pg_class c "
        "JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
        "WHERE c.relkind IN ('r', 'p') AND c.relname = %Q::pg_catalog.name",
        _table);

    if (status == D_INTERNAL_DB_FORMAT_OK)
    {
        status = d_internal_postgres_match_schema(_db, &sql, _schema);
    }

    int result = d_internal_postgres_format_result(status);

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_postgres_query_int64(_db, _bytes, "%s", sql.data);
    d_internal_db_buffer_release(&sql);

    return result;
}

/*
d_postgres_database_size
  pg_database_size() of the database's oid, or of the current one's name.
*/
int
d_postgres_database_size(
    struct pg_conn* _db,
    const char*     _name,
    int64_t*        _bytes
)
{
    // parameter validation
    if ( (!_db)                                   ||
         (!d_internal_postgres_schema_ok(_name))  ||
         (!_bytes) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return (_name)
               ? d_internal_postgres_query_int64(
                     _db,
                     _bytes,
                     "SELECT pg_catalog.pg_database_size(d.oid) "
                     "FROM pg_catalog.pg_database d "
                     "WHERE d.datname = %Q::pg_catalog.name",
                     _name)
               : d_internal_postgres_query_int64(
                     _db,
                     _bytes,
                     "SELECT pg_catalog.pg_database_size("
                     "pg_catalog.current_database())");
}

#else

//==============================================================================
// WITHOUT LIBPQ
//==============================================================================
// Built as stubs (D_CFG_POSTGRES 0), each function reports what a build without
// libpq can: D_DB_UNSUPPORTED, or false, 0 or NULL where it cannot fail. An
// answer a function promises even on failure is still written.

/*
d_postgres_vacuum
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_vacuum(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_analyze
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_analyze(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table
)
{
    (void)_db;
    (void)_schema;
    (void)_table;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_reindex
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_reindex(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    unsigned        _flags
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_flags;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_checkpoint
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_checkpoint(
    struct pg_conn* _db
)
{
    (void)_db;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_table_size
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_table_size(
    struct pg_conn* _db,
    const char*     _schema,
    const char*     _table,
    int64_t*        _bytes
)
{
    (void)_db;
    (void)_schema;
    (void)_table;
    (void)_bytes;

    return D_DB_UNSUPPORTED;
}

/*
d_postgres_database_size
  Without libpq, D_DB_UNSUPPORTED.
*/
int
d_postgres_database_size(
    struct pg_conn* _db,
    const char*     _name,
    int64_t*        _bytes
)
{
    (void)_db;
    (void)_name;
    (void)_bytes;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_POSTGRES

#endif  // defined(INT64_MAX)
