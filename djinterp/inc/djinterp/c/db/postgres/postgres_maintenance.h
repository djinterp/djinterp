/*******************************************************************************
* djinterp [c]                                            postgres_maintenance.h
*
* Upkeep of a PostgreSQL database: vacuum, analyze, reindex, checkpoint, and
* the sizes the server's storage reaches.
*   The server vacuums and analyzes on its own (autovacuum); these are for a
* moment that cannot wait -- after a bulk load, say. VACUUM cannot run inside
* a transaction, and the server refuses it there
* (D_POSTGRES_SQLSTATE_ACTIVE_SQL_TRANSACTION, 25001); ANALYZE and REINDEX
* can, save REINDEX CONCURRENTLY.
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_maintenance.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  MAINTENANCE
    -----------
    1.  Vacuum and analyze
    2.  Indexes and checkpoints
    3.  Sizes
*/

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_MAINTENANCE_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_MAINTENANCE_H 1

// djinterp
#include "../../djinterp.h"     // framework root
#include "./postgres_common.h"  // pg_conn, the layer's results and flags
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; int64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  MAINTENANCE
//==============================================================================


// 1.1    Vacuum and analyze
//------------------------------------------------------------------------------
/**
 * @brief Vacuums a table, or every table of the database the connection may
 *        vacuum: reclaims the space dead rows hold, for the table to reuse.
 *
 * @param[in] _db      the connection, outside a transaction.
 * @param[in] _schema  the table's schema; `NULL` for the current. Must be
 *                     `NULL` when `_table` is.
 * @param[in] _table   the table; `NULL` for the whole database.
 * @param[in] _flags   D_POSTGRES_FULL, to rewrite the table compactly under
 *                     an exclusive lock, and D_POSTGRES_ANALYZE, to update the
 *                     planner's statistics too, in any combination; or 0.
 * @return 0; the SQLSTATE of a failure (42P01, no such table; 25001, inside a
 *         transaction); or D_DB_INVALID_ARGUMENT for a `NULL` connection, an
 *         empty name, a schema without a table, or another flag.
 */
int    d_postgres_vacuum(struct pg_conn* _db,
                         const char*     _schema,
                         const char*     _table,
                         unsigned        _flags) D_NOTHROW;
// analyze -- updates the planner's statistics for a table, or, with _table
// NULL (and _schema then NULL too), for every table of the database. 0, the
// SQLSTATE of a failure (42P01, no such table), or D_DB_INVALID_ARGUMENT.
int    d_postgres_analyze(struct pg_conn* _db,
                          const char*     _schema,
                          const char*     _table) D_NOTHROW;

// 1.2    Indexes and checkpoints
//------------------------------------------------------------------------------
// reindex -- rebuilds every index of a table, as REINDEX TABLE does;
// D_POSTGRES_CONCURRENTLY, from PostgreSQL 12, without blocking writes to it,
// and outside a transaction. 0, the SQLSTATE of a failure (42P01, no such
// table), D_DB_UNSUPPORTED for D_POSTGRES_CONCURRENTLY before PostgreSQL 12,
// or D_DB_INVALID_ARGUMENT for a NULL argument, an empty name or another
// flag.
int    d_postgres_reindex(struct pg_conn* _db,
                          const char*     _schema,
                          const char*     _table,
                          unsigned        _flags) D_NOTHROW;
// checkpoint -- writes every changed page to disk at once, as CHECKPOINT
// does, which needs a superuser, or from PostgreSQL 15 the pg_checkpoint
// role. 0, the SQLSTATE of a failure (42501, no privilege), or
// D_DB_INVALID_ARGUMENT for NULL.
int    d_postgres_checkpoint(struct pg_conn* _db) D_NOTHROW;

// 1.3    Sizes
//------------------------------------------------------------------------------
// sizes, in bytes, written only on success -- _table_size, a table's whole
// footprint, its indexes and TOAST storage included, as
// pg_total_relation_size() reckons it; _database_size, a database's, as
// pg_database_size() does, NULL meaning the current one. 0, D_DB_NOT_FOUND
// for a table or database that does not exist, the SQLSTATE of a failure
// (42501, no privilege to see another database's), or D_DB_INVALID_ARGUMENT.
int    d_postgres_table_size(struct pg_conn* _db,
                             const char*     _schema,
                             const char*     _table,
                             int64_t*        _bytes) D_NOTHROW;
int    d_postgres_database_size(struct pg_conn* _db,
                                const char*     _name,
                                int64_t*        _bytes) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_MAINTENANCE_H
