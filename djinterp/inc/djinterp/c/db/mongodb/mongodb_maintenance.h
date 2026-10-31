/*******************************************************************************
* djinterp [c]                                             mongodb_maintenance.h
*
* Sizes, compaction and validation of a database's collections.
*   Sizes are what WiredTiger keeps on disk, as the server reckons them:
* a collection's whole footprint, its documents' storage and its indexes',
* from $collStats; a database's, from dbStats. Compaction rewrites a
* collection's storage to give its free space back to the operating system,
* as the server's compact command does: a replica set's primary compacts
* only by force, which the layer sends, as every server takes it, the
* collection usable meanwhile from 4.4 and blocked before. Validation checks a
* collection's data and indexes for corruption, and hands back the server's
* report. Each runs outside any transaction.
*   RESULTS. 0; D_DB_NOT_FOUND for a collection or a database that is not
* there; the server's code for a failure (13, unauthorized: each needs a role
* that allows it); the driver's; D_DB_INVALID_ARGUMENT for a NULL argument,
* or no current database where the database is NULL.
*
*
* path:      /inc/djinterp/c/db/mongodb/mongodb_maintenance.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FLAGS
    -----
    1.  Validation
         1.  D_MONGODB_VALIDATE_FULL
2.  MAINTENANCE
    -----------
    1.  Sizes
    2.  Compaction
    3.  Validation
*/

#ifndef DJINTERP_C_DB_MONGODB_MONGODB_MAINTENANCE_H
#define DJINTERP_C_DB_MONGODB_MONGODB_MAINTENANCE_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "../db_common.h"      // D_DB_VENDOR_FLAGS
#include "./mongodb_common.h"  // dj_mongodb, the layer's results
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // INT64_MAX: section 2.1's
                                                 // floor; int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FLAGS
//==============================================================================


// 1.1    Validation
//------------------------------------------------------------------------------
// 1.1.1
// D_MONGODB_VALIDATE_FULL
//   constant: a validation reads every document and index entry, as the
// server's full validation does, rather than its quicker checks.
#define D_MONGODB_VALIDATE_FULL  (D_DB_VENDOR_FLAGS << 2)


//==============================================================================
// 2.  MAINTENANCE
//==============================================================================


#if defined(INT64_MAX)
// 2.1    Sizes
//------------------------------------------------------------------------------
// sizes, in bytes, written only on success -- a collection's footprint, its
// documents' storage and every index's, as $collStats' storageStats reckon
// them; a database's, its collections' and its indexes' storage, as dbStats
// does, NULL meaning the current one. A view has no storage: its size is
// D_DB_NOT_FOUND, as for nothing of the name
int d_mongodb_collection_size(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    int64_t*           _bytes
) D_NOTHROW;
int d_mongodb_database_size(
    struct dj_mongodb* _db,
    const char*        _database,
    int64_t*           _bytes
) D_NOTHROW;
#endif  // defined(INT64_MAX)

// 2.2    Compaction
//------------------------------------------------------------------------------
// compaction -- compacts a collection's storage and its indexes'
int d_mongodb_compact(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection
) D_NOTHROW;

// 2.3    Validation
//------------------------------------------------------------------------------
/**
 * @brief Validates a collection's data and indexes.
 *
 * @param[in]  _db          the handle.
 * @param[in]  _database    the database; `NULL` for the current one.
 * @param[in]  _collection  the collection.
 * @param[in]  _flags       D_MONGODB_VALIDATE_FULL for the full checks.
 * @param[out] _valid       receives whether the server found it sound;
 *                          written only on success.
 * @param[out] _report      receives the server's report, as canonical
 *                          Extended JSON, released with d_db_free; written
 *                          only on success. May be `NULL` when the report is
 *                          not wanted.
 * @return as the header describes; D_MONGODB_LAYER_NO_MEMORY.
 * @post On success, when `_report` is not `NULL`, the caller releases
 *       `*_report` with d_db_free.
 */
int d_mongodb_validate(
    struct dj_mongodb* _db,
    const char*        _database,
    const char*        _collection,
    unsigned           _flags,
    bool*              _valid,
    char**             _report
) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_MONGODB_MONGODB_MAINTENANCE_H
