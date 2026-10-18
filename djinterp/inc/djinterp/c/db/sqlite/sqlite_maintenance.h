/*******************************************************************************
* djinterp [c]                                              sqlite_maintenance.h
*
* Looking after an SQLite database: compacting it, keeping the planner's
* statistics current, checking it, moving its write-ahead log into it, and
* copying it while it is in use.
*   Each takes the database's schema, NULL meaning "main". VACUUM of a schema
* other than "main" needs 3.15.0 -- before it, SQLite accepted the name and
* vacuumed "main" instead -- and is refused with D_DB_UNSUPPORTED there, as
* are VACUUM INTO before 3.27.0, PRAGMA optimize before 3.18.0, and the
* TRUNCATE checkpoint before 3.8.8.
*   The backups use SQLite's online backup API, which copies a database page
* by page while other connections go on using it, and produces a file that is
* a database in its own right.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_maintenance.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  UPKEEP
    ------
    1.  Compacting
    2.  Statistics
    3.  Checking
2.  WRITE-AHEAD LOG
    ---------------
    1.  Checkpoint modes
         1.  d_sqlite_checkpoint_mode
    2.  Checkpoints
3.  BACKUPS
    -------
    1.  Copying databases
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_MAINTENANCE_H
#define DJINTERP_C_DB_SQLITE_SQLITE_MAINTENANCE_H 1

// djinterp
#include "../../djinterp.h"   // framework root
#include "./sqlite_common.h"  // sqlite3, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  UPKEEP
//==============================================================================
// Each returns SQLITE_OK, D_DB_UNSUPPORTED as the banner says, the SQLite
// result code of a failure -- SQLITE_ERROR for a schema that is not attached,
// and, for the two VACUUMs, inside a transaction -- or SQLITE_MISUSE for a
// NULL argument or an empty name.


// 1.1    Compacting
//------------------------------------------------------------------------------
// compacting -- VACUUM rebuilds the database to reclaim free pages and undo
// fragmentation; VACUUM INTO writes the rebuilt copy to a new file instead,
// leaving the database as it was, which makes it a backup as well. The file
// must not exist, or be empty.
int  d_sqlite_vacuum(struct sqlite3* _db,
                     const char*     _schema) D_NOTHROW;
int  d_sqlite_vacuum_into(struct sqlite3* _db,
                          const char*     _schema,
                          const char*     _path) D_NOTHROW;

// 1.2    Statistics
//------------------------------------------------------------------------------
// statistics -- ANALYZE gathers the statistics the query planner chooses
// indexes by; PRAGMA optimize does so only where they look stale, and is
// cheap enough to run before closing every long-lived connection
int  d_sqlite_analyze(struct sqlite3* _db,
                      const char*     _schema) D_NOTHROW;
int  d_sqlite_optimize(struct sqlite3* _db,
                       const char*     _schema) D_NOTHROW;

// 1.3    Checking
//------------------------------------------------------------------------------
// checking -- `_ok` receives whether the database passed, written only on
// SQLITE_OK. integrity_check reads everything, indexes against their tables
// included; quick_check skips that comparison and is much faster.
int  d_sqlite_integrity_check(struct sqlite3* _db,
                              const char*     _schema,
                              bool*           _ok) D_NOTHROW;
int  d_sqlite_quick_check(struct sqlite3* _db,
                          const char*     _schema,
                          bool*           _ok) D_NOTHROW;


//==============================================================================
// 2.  WRITE-AHEAD LOG
//==============================================================================


// 2.1    Checkpoint modes
//------------------------------------------------------------------------------
// 2.1.1
// d_sqlite_checkpoint_mode
//   enum: how hard a checkpoint tries. The values are SQLite's own.
enum d_sqlite_checkpoint_mode
{
    D_SQLITE_CHECKPOINT_PASSIVE  = 0,  // what can be done without waiting
    D_SQLITE_CHECKPOINT_FULL     = 1,  // all of it, waiting for writers
    D_SQLITE_CHECKPOINT_RESTART  = 2,  // ... and readers, restarting the log
    D_SQLITE_CHECKPOINT_TRUNCATE = 3   // ... and truncate it to zero; 3.8.8
};

// 2.2    Checkpoints
//------------------------------------------------------------------------------
/**
 * @brief Copies the write-ahead log's committed pages into the database.
 *
 * @note A database not in WAL mode has no log, and succeeds at once.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the database; `NULL` for "main".
 * @param[in] _mode    how hard to try.
 * @return SQLITE_OK; SQLITE_BUSY when FULL, RESTART or TRUNCATE could not
 *         finish for other connections, with as much done as could be;
 *         D_DB_UNSUPPORTED for TRUNCATE before 3.8.8; the SQLite result code
 *         of a failure; or SQLITE_MISUSE.
 */
int  d_sqlite_checkpoint(struct sqlite3*               _db,
                         const char*                   _schema,
                         enum d_sqlite_checkpoint_mode _mode) D_NOTHROW;


//==============================================================================
// 3.  BACKUPS
//==============================================================================
// Each copies one database over another with the online backup API, and
// returns SQLITE_OK; SQLITE_BUSY or SQLITE_LOCKED when the source is being
// written or the destination is in use, either worth a retry; another SQLite
// result code; or SQLITE_MISUSE for a NULL argument or an empty name.
// Whatever the destination held before is replaced.


// 3.1    Copying databases
//------------------------------------------------------------------------------
/**
 * @brief Copies one connection's database over another's.
 *
 * @param[in] _source              the connection to copy from.
 * @param[in] _source_schema       its database; `NULL` for "main".
 * @param[in] _destination         the connection to copy to; another one.
 * @param[in] _destination_schema  its database; `NULL` for "main".
 * @return as section 3 describes.
 * @post On success the destination database is a copy of the source.
 */
int  d_sqlite_copy_database(struct sqlite3* _source,
                            const char*     _source_schema,
                            struct sqlite3* _destination,
                            const char*     _destination_schema) D_NOTHROW;
/**
 * @brief Backs a database up to a file, which is created or overwritten.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the database; `NULL` for "main".
 * @param[in] _path    the file, in UTF-8.
 * @return as section 3 describes, SQLITE_CANTOPEN among them.
 * @post On success `_path` holds a database equal to the one backed up.
 */
int  d_sqlite_backup_to_file(struct sqlite3* _db,
                             const char*     _schema,
                             const char*     _path) D_NOTHROW;
/**
 * @brief Replaces a database with the contents of a database file.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the database to replace; `NULL` for "main".
 * @param[in] _path    the file to restore from, in UTF-8; opened read-only.
 * @return as section 3 describes, SQLITE_CANTOPEN and SQLITE_NOTADB among
 *         them.
 * @post On success the database equals the file.
 */
int  d_sqlite_restore_from_file(struct sqlite3* _db,
                                const char*     _schema,
                                const char*     _path) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_SQLITE_SQLITE_MAINTENANCE_H
