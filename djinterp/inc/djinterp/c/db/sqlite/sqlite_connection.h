/*******************************************************************************
* djinterp [c]                                               sqlite_connection.h
*
* Opening and closing SQLite connections, and what a connection can say about
* itself.
*   d_sqlite_open and its siblings open a connection and set it up with the
* defaults cfg_sqlite_connection.h chooses -- a busy timeout and foreign
* keys, unless the build says otherwise; d_sqlite_open_with takes SQLite's
* own open flags and a VFS, for what the others do not choose. A connection
* opened natively can be given the same defaults with
* d_sqlite_apply_defaults. On a failed open the handle SQLite allocated is
* closed, and the out-parameter is NULL: there is nothing for the caller to
* clean up.
*   Paths are UTF-8 on every platform, as SQLite takes them, including
* Windows, where SQLite converts them for the system.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_connection.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONNECTIONS
    -----------
    1.  Open flags
         1.  D_SQLITE_OPEN_READONLY
         2.  D_SQLITE_OPEN_READWRITE
         3.  D_SQLITE_OPEN_CREATE
         4.  D_SQLITE_OPEN_URI
         5.  D_SQLITE_OPEN_MEMORY
         6.  D_SQLITE_OPEN_NOMUTEX
         7.  D_SQLITE_OPEN_FULLMUTEX
         8.  D_SQLITE_OPEN_SHAREDCACHE
         9.  D_SQLITE_OPEN_PRIVATECACHE
        10.  D_SQLITE_OPEN_NOFOLLOW
    2.  Opening and closing
    3.  Location and access
    4.  Rows changed
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_CONNECTION_H
#define DJINTERP_C_DB_SQLITE_SQLITE_CONNECTION_H 1

// djinterp
#include "../../djinterp.h"   // framework root
#include "./sqlite_common.h"  // sqlite3, the layer's results
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's
                                                 // floor; int64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  CONNECTIONS
//==============================================================================


// 1.1    Open flags
//------------------------------------------------------------------------------
// What d_sqlite_open_with takes: SQLite's own SQLITE_OPEN_* values, named here
// so that a caller need not include sqlite3.h. Exactly one of READONLY,
// READWRITE and READWRITE | CREATE says how the file is opened; the others
// may be added to it, but not both of a pair (NOMUTEX and FULLMUTEX,
// SHAREDCACHE and PRIVATECACHE).

// 1.1.1
// D_SQLITE_OPEN_READONLY
//   constant: for reading only; a missing file is SQLITE_CANTOPEN.
#define D_SQLITE_OPEN_READONLY      0x00000001u

// 1.1.2
// D_SQLITE_OPEN_READWRITE
//   constant: for reading and writing, where the file allows it; a missing
// file is SQLITE_CANTOPEN without CREATE.
#define D_SQLITE_OPEN_READWRITE     0x00000002u

// 1.1.3
// D_SQLITE_OPEN_CREATE
//   constant: with READWRITE, creates a missing file.
#define D_SQLITE_OPEN_CREATE        0x00000004u

// 1.1.4
// D_SQLITE_OPEN_URI
//   constant: reads a path that begins "file:" as a URI, whose query can set
// the mode, the cache and the VFS ("file:data.db?mode=ro&cache=shared").
#define D_SQLITE_OPEN_URI           0x00000040u

// 1.1.5
// D_SQLITE_OPEN_MEMORY
//   constant: a database held in memory, the path naming it only for a shared
// cache.
#define D_SQLITE_OPEN_MEMORY        0x00000080u

// 1.1.6
// D_SQLITE_OPEN_NOMUTEX
//   constant: the connection takes no lock of its own, and is for one thread
// at a time (multi-thread mode), where the library was built thread-safe.
#define D_SQLITE_OPEN_NOMUTEX       0x00008000u

// 1.1.7
// D_SQLITE_OPEN_FULLMUTEX
//   constant: every call on the connection is serialized (serialized mode),
// where the library was built thread-safe.
#define D_SQLITE_OPEN_FULLMUTEX     0x00010000u

// 1.1.8
// D_SQLITE_OPEN_SHAREDCACHE
//   constant: the connection shares its page cache with the process's other
// connections to the same file, where the library allows it.
#define D_SQLITE_OPEN_SHAREDCACHE   0x00020000u

// 1.1.9
// D_SQLITE_OPEN_PRIVATECACHE
//   constant: the connection keeps its own page cache, whatever the process
// enabled.
#define D_SQLITE_OPEN_PRIVATECACHE  0x00040000u

// 1.1.10
// D_SQLITE_OPEN_NOFOLLOW
//   constant: refuses a path that is a symbolic link, with SQLITE_CANTOPEN;
// SQLite 3.31.0.
#define D_SQLITE_OPEN_NOFOLLOW      0x01000000u

// 1.2    Opening and closing
//------------------------------------------------------------------------------
/**
 * @brief Opens a database file for reading and writing, creating it if it
 *        does not exist, and applies the layer's connection defaults.
 *
 * @note SQLite reads the file only when it is first used, so a file that is
 *       not a database opens, and fails with SQLITE_NOTADB at its first
 *       query; d_sqlite_is_database_file asks beforehand. The laziness is
 *       what lets an encrypting build (SQLCipher) take its key after the
 *       open, so the layer does not read early.
 *
 * @param[in]  _path  the file, in UTF-8; ":memory:" opens a private in-memory
 *                    database, and "" a private temporary one on disk.
 * @param[out] _db    receives the connection, or `NULL` on failure.
 * @return SQLITE_OK, or the SQLite result code of the failure --
 *         SQLITE_CANTOPEN for a path that cannot be opened -- or
 *         SQLITE_MISUSE for a `NULL` argument.
 * @post On success the caller closes `*_db` with d_sqlite_close.
 */
int         d_sqlite_open(const char*     _path,
                          struct sqlite3** _db) D_NOTHROW;
/**
 * @brief Opens an existing database file for reading only, and applies the
 *        layer's connection defaults.
 *
 * @param[in]  _path  the file, in UTF-8.
 * @param[out] _db    receives the connection, or `NULL` on failure.
 * @return SQLITE_OK, or the SQLite result code of the failure, or
 *         SQLITE_MISUSE for a `NULL` argument.
 * @post On success the caller closes `*_db` with d_sqlite_close.
 */
int         d_sqlite_open_readonly(const char*     _path,
                                   struct sqlite3** _db) D_NOTHROW;
/**
 * @brief Opens a private in-memory database, which vanishes when it is
 *        closed, and applies the layer's connection defaults.
 *
 * @param[out] _db  receives the connection, or `NULL` on failure.
 * @return SQLITE_OK, or the SQLite result code of the failure, or
 *         SQLITE_MISUSE for a `NULL` argument.
 * @post On success the caller closes `*_db` with d_sqlite_close.
 */
int         d_sqlite_open_memory(struct sqlite3** _db) D_NOTHROW;

/**
 * @brief Opens a database file as the flags say, through the VFS named, and
 *        applies the layer's connection defaults.
 *
 * @param[in]  _path   the file, in UTF-8, or a URI with D_SQLITE_OPEN_URI;
 *                     with D_SQLITE_OPEN_MEMORY, the name of the in-memory
 *                     database, which matters only to a shared cache.
 * @param[in]  _flags  D_SQLITE_OPEN_* flags: one of READONLY, READWRITE and
 *                     READWRITE | CREATE, and any of the others.
 * @param[in]  _vfs    the name of the VFS to open the file through; `NULL`
 *                     for the default.
 * @param[out] _db     receives the connection, or `NULL` on failure.
 * @return SQLITE_OK, or the SQLite result code of the failure --
 *         SQLITE_CANTOPEN for a path that cannot be opened, SQLITE_ERROR for
 *         a VFS of no such name; D_DB_UNSUPPORTED for a flag the library
 *         loaded is too old for (NOFOLLOW before 3.31.0); or SQLITE_MISUSE
 *         for a `NULL` path or handle, or flags that are no D_SQLITE_OPEN_*,
 *         name no way of opening, or name both of a pair.
 * @post On success the caller closes `*_db` with d_sqlite_close.
 */
int         d_sqlite_open_with(const char*      _path,
                               unsigned         _flags,
                               const char*      _vfs,
                               struct sqlite3** _db) D_NOTHROW;
/**
 * @brief Gives a connection the defaults d_sqlite_open would have: the busy
 *        timeout, foreign keys and result codes cfg_sqlite_connection.h
 *        chooses.
 *
 * @param[in] _db  the connection, however it was opened.
 * @return SQLITE_OK, or the SQLite result code of the setting that failed,
 *         or SQLITE_MISUSE for a `NULL` connection.
 */
int         d_sqlite_apply_defaults(struct sqlite3* _db) D_NOTHROW;
/**
 * @brief Closes a connection.
 *
 * @note Statements still open on it do not keep the call from succeeding:
 *       the connection then closes when the last of them is finalized
 *       (sqlite3_close_v2).
 *
 * @param[in] _db  the connection; may be `NULL`.
 * @return SQLITE_OK, or the SQLite result code of the failure.
 * @post `_db` is invalid.
 */
int         d_sqlite_close(struct sqlite3* _db) D_NOTHROW;

// 1.3    Location and access
//------------------------------------------------------------------------------
// location -- _path is the file behind a schema (NULL meaning "main"): an
// absolute path, "" for an in-memory or temporary database, or NULL when no
// database of that name is attached; the string is SQLite's, valid while the
// database stays attached. _is_readonly is true only for a database attached
// for reading alone, and false when there is none.
const char* d_sqlite_database_path(struct sqlite3* _db,
                                   const char*     _schema) D_NOTHROW;
bool        d_sqlite_is_readonly(struct sqlite3* _db,
                                 const char*     _schema) D_NOTHROW;

// 1.4    Rows changed
//------------------------------------------------------------------------------
// rows changed -- the rowid of the latest successful INSERT on the connection;
// the rows the latest INSERT, UPDATE or DELETE changed; and every row changed
// since the connection opened. Counts are 64-bit where the library offers it
// (3.37.0), so they do not wrap at 2^31. All are 0 for a NULL connection.
int64_t     d_sqlite_last_insert_id(struct sqlite3* _db) D_NOTHROW;
int64_t     d_sqlite_changes(struct sqlite3* _db) D_NOTHROW;
int64_t     d_sqlite_total_changes(struct sqlite3* _db) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_SQLITE_SQLITE_CONNECTION_H
