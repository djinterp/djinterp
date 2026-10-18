/*******************************************************************************
* djinterp [c]                                                 sqlite_database.h
*
* SQLite's databases: the ones a connection has attached, and the files they
* live in.
*   A connection sees several databases at once -- "main", "temp", and each one
* ATTACHed -- and SQL names an object in one of them as schema.object, which is
* what MariaDB calls database.object. So the connection-level functions here
* are the ones the other vendors' layers spell the same way: has_database asks
* whether a database is attached, as d_mariadb_has_database asks whether one
* exists on the server.
*   On disk a database is a file, plus the -journal, -wal and -shm files SQLite
* keeps beside it. The file-level functions create one, recognise one, and
* delete one with everything beside it. They go through SQLite's own VFS, so
* paths are UTF-8 on every platform, Windows included.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_database.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  ATTACHED DATABASES
    ------------------
    1.  Existence
    2.  Attaching and detaching
    3.  Listing
2.  DATABASE FILES
    --------------
    1.  Creating and deleting
    2.  Recognising
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_DATABASE_H
#define DJINTERP_C_DB_SQLITE_SQLITE_DATABASE_H 1

// djinterp
#include "../../djinterp.h"   // framework root
#include "../db_common.h"     // d_db_name_list, D_DB_IF_*
#include "./sqlite_common.h"  // sqlite3, the layer's results


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  ATTACHED DATABASES
//==============================================================================


// 1.1    Existence
//------------------------------------------------------------------------------
// existence -- whether a database of that name is attached to the connection.
// "main" and "temp" always are. Names match as SQLite matches them, ignoring
// ASCII case. The plain form answers false for a failure or a bad argument;
// the _s form returns SQLITE_OK and writes the answer, or SQLITE_MISUSE.
bool d_sqlite_has_database(struct sqlite3* _db,
                           const char*     _name) D_NOTHROW;
int  d_sqlite_has_database_s(struct sqlite3* _db,
                             const char*     _name,
                             bool*           _exists) D_NOTHROW;

// 1.2    Attaching and detaching
//------------------------------------------------------------------------------
/**
 * @brief Attaches a database file to the connection under a name, creating
 *        the file if it does not exist.
 *
 * @note Not inside a transaction: SQLite refuses to attach there.
 *
 * @param[in] _db    the connection.
 * @param[in] _path  the file, in UTF-8; ":memory:" or "" attach a private
 *                   in-memory or temporary database.
 * @param[in] _name  the name to attach it as; SQL then reaches its objects as
 *                   name.object.
 * @return SQLITE_OK, or the SQLite result code of the failure -- among them
 *         SQLITE_ERROR for a name already in use, or too many attached --
 *         or SQLITE_MISUSE for a `NULL` argument or an empty name.
 */
int  d_sqlite_attach_database(struct sqlite3* _db,
                              const char*     _path,
                              const char*     _name) D_NOTHROW;
/**
 * @brief Detaches a database the connection attached.
 *
 * @param[in] _db    the connection.
 * @param[in] _name  the name it was attached as; "main" and "temp" cannot be
 *                   detached.
 * @return SQLITE_OK, or the SQLite result code of the failure -- SQLITE_ERROR
 *         when no such database is attached, or it is in use -- or
 *         SQLITE_MISUSE for a `NULL` argument or an empty name.
 */
int  d_sqlite_detach_database(struct sqlite3* _db,
                              const char*     _name) D_NOTHROW;

// 1.3    Listing
//------------------------------------------------------------------------------
/**
 * @brief Lists the databases attached to the connection: "main", "temp" once
 *        SQLite has created it, then the rest in the order attached.
 *
 * @param[in]  _db     the connection.
 * @param[out] _names  receives the names; written only on success.
 * @return SQLITE_OK, or the SQLite result code of the failure, SQLITE_NOMEM
 *         when the list could not be built, or SQLITE_MISUSE for a `NULL`
 *         argument.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_sqlite_list_databases(struct sqlite3*        _db,
                             struct d_db_name_list* _names) D_NOTHROW;


//==============================================================================
// 2.  DATABASE FILES
//==============================================================================
// No connection may have the file open while it is created or deleted: SQLite
// cannot see a file change underneath a connection to it, and on Windows a
// file that is open cannot be deleted at all.


// 2.1    Creating and deleting
//------------------------------------------------------------------------------
/**
 * @brief Creates an empty SQLite database file.
 *
 * @param[in] _path   the file, in UTF-8. Its directory must exist.
 * @param[in] _flags  D_DB_IF_NOT_EXISTS to accept a file already there,
 *                    provided it is a database; 0 to require a new one.
 * @return SQLITE_OK; D_DB_EXISTS when the file exists and the flag is not
 *         given; SQLITE_NOTADB when it exists and is not a database; the
 *         SQLite result code of any other failure; or SQLITE_MISUSE for a
 *         `NULL` or empty path or an unknown flag.
 * @post On success `_path` holds a database a later open finds valid.
 */
int  d_sqlite_create_database_file(const char* _path,
                                   unsigned    _flags) D_NOTHROW;
/**
 * @brief Deletes a database file, and the -journal, -wal and -shm files
 *        SQLite may have left beside it.
 *
 * @warning Deleting a database a connection still has open loses data; and a
 *          -journal or -wal file left by a crash holds changes the database
 *          needs. Delete only what no one needs.
 *
 * @param[in] _path   the file, in UTF-8.
 * @param[in] _flags  D_DB_IF_EXISTS to accept a file that is not there; 0 to
 *                    require one.
 * @return SQLITE_OK; D_DB_NOT_FOUND when there is no file and the flag is not
 *         given; the SQLite result code of a failure to delete, such as
 *         SQLITE_IOERR_DELETE; or SQLITE_MISUSE for a `NULL` or empty path or
 *         an unknown flag.
 * @post On success neither the file nor any of the three beside it exists.
 */
int  d_sqlite_delete_database_file(const char* _path,
                                   unsigned    _flags) D_NOTHROW;

// 2.2    Recognising
//------------------------------------------------------------------------------
// recognising -- true when the path names a file SQLite can read as a
// database, an empty file included (SQLite treats one as an empty database);
// false for a missing file, anything else, or an encrypted database whose key
// is not given
bool d_sqlite_is_database_file(const char* _path) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_SQLITE_SQLITE_DATABASE_H
