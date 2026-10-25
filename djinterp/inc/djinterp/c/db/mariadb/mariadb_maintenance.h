/*******************************************************************************
* djinterp [c]                                             mariadb_maintenance.h
*
* Upkeep of the tables on a MariaDB or MySQL server.
*   Each statement here reports through rows rather than an error number:
* ANALYZE, OPTIMIZE and CHECK TABLE answer with a status row, and CHECKSUM
* TABLE with a NULL for a table it cannot find. The layer reads those rows
* and returns what they mean: 0 for a table the operation succeeded on,
* D_DB_NOT_FOUND for a table that is not there -- a view is not one -- and
* D_DB_UNSUPPORTED for an engine that cannot do it. A failure the rows report
* has no number, and is CR_UNKNOWN_ERROR (2000); the client's error number
* still reports a failure to run the statement at all.
*   Backups are mariadb-dump's and mysqldump's work, which the client
* library does not offer, and are not here.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_maintenance.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TABLES
    ------
    1.  Statistics and storage
    2.  Checks
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_MAINTENANCE_H
#define DJINTERP_C_DB_MARIADB_MARIADB_MAINTENANCE_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "./mariadb_common.h"  // d_mariadb, the layer's results
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TABLES
//==============================================================================
// Each takes the database, NULL meaning the current one, and the table, and
// returns as the banner says, or D_DB_INVALID_ARGUMENT for a NULL handle or
// table, or an empty name.


// 1.1    Statistics and storage
//------------------------------------------------------------------------------
// statistics and storage -- ANALYZE TABLE refreshes the key distribution the
// optimizer plans by; OPTIMIZE TABLE rebuilds the table to reclaim space,
// which InnoDB does as a recreate and analyze
int    d_mariadb_analyze_table(struct d_mariadb* _db,
                               const char*       _schema,
                               const char*       _table) D_NOTHROW;
int    d_mariadb_optimize_table(struct d_mariadb* _db,
                                const char*       _schema,
                                const char*       _table) D_NOTHROW;

// 1.2    Checks
//------------------------------------------------------------------------------
// checks -- CHECK TABLE writes to _ok whether the table is sound; CHECKSUM
// TABLE writes a checksum of its contents, which two tables share only when
// they hold the same rows. Each answer is written only on success.
int    d_mariadb_check_table(struct d_mariadb* _db,
                             const char*       _schema,
                             const char*       _table,
                             bool*             _ok) D_NOTHROW;
int    d_mariadb_checksum_table(struct d_mariadb* _db,
                                const char*       _schema,
                                const char*       _table,
                                uint64_t*         _checksum) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_MAINTENANCE_H
