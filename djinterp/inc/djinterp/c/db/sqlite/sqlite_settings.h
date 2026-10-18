/*******************************************************************************
* djinterp [c]                                                 sqlite_settings.h
*
* SQLite's settings -- its PRAGMAs -- as get/set pairs.
*   The settings most programs touch have typed functions; d_sqlite_set_pragma
* and d_sqlite_get_pragma reach any other. Settings that belong to one
* database (journal mode, synchronous, the two version numbers) take its
* schema, NULL meaning "main"; settings of the connection take none.
*   SQLite ignores, without an error, a value it cannot apply: foreign keys
* cannot change inside a transaction, and an in-memory database has no WAL.
* Every set here therefore reads the setting back, and answers
* D_DB_UNSUPPORTED when it did not take, rather than letting the caller
* believe it had. Where reading back cannot tell -- a library older than
* 3.11.0 stores synchronous EXTRA without knowing it -- the version decides.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_settings.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VALUES
    ------
    1.  Enumerated settings
         1.  d_sqlite_journal_mode
         2.  d_sqlite_synchronous
2.  CONNECTION SETTINGS
    -------------------
    1.  Foreign keys
    2.  Busy timeout
3.  DATABASE SETTINGS
    -----------------
    1.  Journal mode
    2.  Synchronous
    3.  Version numbers
4.  ANY PRAGMA
    ----------
    1.  Setting and getting
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_SETTINGS_H
#define DJINTERP_C_DB_SQLITE_SQLITE_SETTINGS_H 1

// djinterp
#include "../../djinterp.h"   // framework root
#include "./sqlite_common.h"  // sqlite3, the layer's results
// re_std
#include "../../../../re_std/cstdint/dstdint.h"  // int32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  VALUES
//==============================================================================


// 1.1    Enumerated settings
//------------------------------------------------------------------------------
// 1.1.1
// d_sqlite_journal_mode
//   enum: how a database keeps what a transaction would undo (PRAGMA
// journal_mode).
enum d_sqlite_journal_mode
{
    D_SQLITE_JOURNAL_DELETE = 0,  // a rollback journal, deleted at commit
    D_SQLITE_JOURNAL_TRUNCATE,    // ... truncated at commit
    D_SQLITE_JOURNAL_PERSIST,     // ... its header zeroed at commit
    D_SQLITE_JOURNAL_MEMORY,      // ... held in memory
    D_SQLITE_JOURNAL_WAL,         // a write-ahead log
    D_SQLITE_JOURNAL_OFF          // none: a crash can corrupt the database
};

// 1.1.2
// d_sqlite_synchronous
//   enum: how often SQLite waits for the disk (PRAGMA synchronous). The values
// are SQLite's own.
enum d_sqlite_synchronous
{
    D_SQLITE_SYNCHRONOUS_OFF    = 0,  // never: fast, and unsafe on power loss
    D_SQLITE_SYNCHRONOUS_NORMAL = 1,  // at critical moments; safe with WAL
    D_SQLITE_SYNCHRONOUS_FULL   = 2,  // at every commit; the default
    D_SQLITE_SYNCHRONOUS_EXTRA  = 3   // also for the directory; 3.11.0
};


//==============================================================================
// 2.  CONNECTION SETTINGS
//==============================================================================


// 2.1    Foreign keys
//------------------------------------------------------------------------------
/**
 * @brief Turns foreign-key enforcement on or off for the connection.
 *
 * @param[in] _db       the connection.
 * @param[in] _enabled  whether to enforce.
 * @return SQLITE_OK; D_DB_UNSUPPORTED when the setting did not take -- inside
 *         a transaction, or in a library built without foreign keys; the
 *         SQLite result code of a failure; or SQLITE_MISUSE.
 */
int  d_sqlite_set_foreign_keys(struct sqlite3* _db,
                               bool            _enabled) D_NOTHROW;
/**
 * @brief Reports whether the connection enforces foreign keys.
 *
 * @param[in]  _db       the connection.
 * @param[out] _enabled  receives the answer; written only on success.
 * @return SQLITE_OK; D_DB_UNSUPPORTED in a library built without foreign
 *         keys; the SQLite result code of a failure; or SQLITE_MISUSE.
 */
int  d_sqlite_get_foreign_keys(struct sqlite3* _db,
                               bool*           _enabled) D_NOTHROW;

// 2.2    Busy timeout
//------------------------------------------------------------------------------
// busy timeout -- how long the connection waits, in milliseconds, for a lock
// another connection holds before reporting SQLITE_BUSY; 0 does not wait.
// Each returns SQLITE_OK, the SQLite result code of a failure, or
// SQLITE_MISUSE for a NULL argument or a negative timeout; get writes only on
// success.
int  d_sqlite_set_busy_timeout(struct sqlite3* _db,
                               int             _milliseconds) D_NOTHROW;
int  d_sqlite_get_busy_timeout(struct sqlite3* _db,
                               int*            _milliseconds) D_NOTHROW;


//==============================================================================
// 3.  DATABASE SETTINGS
//==============================================================================
// Each takes the database's schema, NULL meaning "main", and returns
// SQLITE_OK, the SQLite result code of a failure (SQLITE_ERROR for a schema
// that is not attached), or SQLITE_MISUSE for a NULL argument, an empty
// schema or a value outside its enumeration. A get writes only on success.


// 3.1    Journal mode
//------------------------------------------------------------------------------
// journal mode -- set answers D_DB_UNSUPPORTED when the database kept another
// mode (an in-memory database takes only MEMORY and OFF); SQLite refuses
// with SQLITE_ERROR to leave or enter WAL inside a transaction. Get answers
// D_DB_UNSUPPORTED for a mode this enumeration does not name.
int  d_sqlite_set_journal_mode(struct sqlite3*            _db,
                               const char*                _schema,
                               enum d_sqlite_journal_mode _mode) D_NOTHROW;
int  d_sqlite_get_journal_mode(struct sqlite3*             _db,
                               const char*                 _schema,
                               enum d_sqlite_journal_mode* _mode) D_NOTHROW;

// 3.2    Synchronous
//------------------------------------------------------------------------------
// synchronous -- set answers D_DB_UNSUPPORTED for EXTRA before 3.11.0, which
// an older library stores and reads back but treats as FULL, and when the
// value did not take
int  d_sqlite_set_synchronous(struct sqlite3*           _db,
                              const char*               _schema,
                              enum d_sqlite_synchronous _value) D_NOTHROW;
int  d_sqlite_get_synchronous(struct sqlite3*            _db,
                              const char*                _schema,
                              enum d_sqlite_synchronous* _value) D_NOTHROW;

// 3.3    Version numbers
//------------------------------------------------------------------------------
// version numbers -- two integers in the database header that SQLite stores
// and never interprets: user_version, for an application's schema version,
// and application_id, which identifies the file format to tools like file(1)
int  d_sqlite_set_user_version(struct sqlite3* _db,
                               const char*     _schema,
                               int32_t         _version) D_NOTHROW;
int  d_sqlite_get_user_version(struct sqlite3* _db,
                               const char*     _schema,
                               int32_t*        _version) D_NOTHROW;
int  d_sqlite_set_application_id(struct sqlite3* _db,
                                 const char*     _schema,
                                 int32_t         _id) D_NOTHROW;
int  d_sqlite_get_application_id(struct sqlite3* _db,
                                 const char*     _schema,
                                 int32_t*        _id) D_NOTHROW;


//==============================================================================
// 4.  ANY PRAGMA
//==============================================================================


// 4.1    Setting and getting
//------------------------------------------------------------------------------
/**
 * @brief Sets any PRAGMA, as PRAGMA [schema.]name = 'value'.
 *
 * @note Unlike the typed functions, nothing is read back: SQLite ignores an
 *       unknown PRAGMA, and what one does with a value is its own business.
 *
 * @param[in] _db      the connection.
 * @param[in] _schema  the schema to qualify it with; `NULL` for none, which
 *                     for a database's PRAGMA means every attached database.
 * @param[in] _pragma  the PRAGMA's name: letters, digits and underscores.
 * @param[in] _value   the value, passed as a string literal, which every
 *                     PRAGMA taking a number, a keyword or a name accepts.
 * @return SQLITE_OK, the SQLite result code of the failure, or SQLITE_MISUSE
 *         for a `NULL` argument or a name that is not a PRAGMA's.
 */
int  d_sqlite_set_pragma(struct sqlite3* _db,
                         const char*     _schema,
                         const char*     _pragma,
                         const char*     _value) D_NOTHROW;
/**
 * @brief Reads any PRAGMA: the first column of its first row, as text.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _schema  the schema to qualify it with; `NULL` for none.
 * @param[in]  _pragma  the PRAGMA's name: letters, digits and underscores.
 * @param[out] _value   receives the text, released with d_db_free, or `NULL`
 *                      for a NULL value; written only on success.
 * @return SQLITE_OK; D_DB_NOT_FOUND when the PRAGMA reports nothing, as an
 *         unknown one does; the SQLite result code of a failure; or
 *         SQLITE_MISUSE.
 */
int  d_sqlite_get_pragma(struct sqlite3* _db,
                         const char*     _schema,
                         const char*     _pragma,
                         char**          _value) D_NOTHROW;


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_SQLITE_SQLITE_SETTINGS_H
