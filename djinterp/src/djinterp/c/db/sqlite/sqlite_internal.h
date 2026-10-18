/*******************************************************************************
* djinterp [c]                                                 sqlite_internal.h
*
* Private to the SQLite layer's sources: sqlite3.h, the version floor, and the
* statement plumbing every module repeats -- when the layer is built over
* SQLite. Built as stubs (D_INTERNAL_SQLITE 0), it declares nothing, and env
* includes no sqlite3.h.
*   sqlite3.h arrives through env_sqlite.h, which honours
* D_CFG_ENV_SQLITE_C_PATH (an SQLCipher build, say) and works out the version;
* cfg_sqlite_common.h has already turned D_CFG_ENV_USING_SQLITE on, for a
* source that defined D_INTERNAL_SQLITE_SOURCE before its first include. The
* floor
* is checked here once, so a source compiled against an older SQLite stops
* with one clear message instead of a dozen missing declarations.
*   The plumbing keeps one rule the public contract depends on: after a
* statement fails, nothing else runs on the connection before control returns,
* so sqlite3_errmsg still describes the failure. A helper that must clean up
* after a failure says so.
*
*
* path:      /src/djinterp/c/db/sqlite/sqlite_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  REQUIREMENTS
    ------------
    1.  Version floor
    2.  Compile-time API choices
         1.  D_INTERNAL_SQLITE_CHANGES
         2.  D_INTERNAL_SQLITE_TOTAL_CHANGES
         3.  D_INTERNAL_SQLITE_BIND64
2.  NAMES
    -----
    1.  Schemas and names
3.  STATEMENTS
    ----------
    1.  Preparing and running
    2.  Single-value results
    3.  Name lists
    4.  Rows
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_INTERNAL_H
#define DJINTERP_C_DB_SQLITE_SQLITE_INTERNAL_H 1

// std
#include <stdarg.h>  // va_list
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/db/sqlite/sqlite_common.h"  // layer
#include "../../../../../inc/djinterp/env/db/sqlite/env_sqlite.h"  // sqlite3.h
#include "../db_internal.h"  // d_internal_db_names
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_SQLITE == 1)


//==============================================================================
// 1.  REQUIREMENTS
//==============================================================================


// 1.1    Version floor
//------------------------------------------------------------------------------
#if !D_CFG_IS_ON(D_CFG_ENV_USING_SQLITE)
    #error "D_CFG_ENV_USING_SQLITE is off: a layer source defines "            \
           "D_INTERNAL_SQLITE_SOURCE before its first include"
#elif !D_ENV_SQLITE_DETECTED
    #error "the SQLite layer needs sqlite3.h; see D_CFG_ENV_SQLITE_C_PATH"
#elif (D_ENV_SQLITE_VERSION_ID < D_SQLITE_FLOOR_VERSION_NUMBER)
    #error "the SQLite layer needs SQLite 3.7.17 or later"
#endif

// 1.2    Compile-time API choices
//------------------------------------------------------------------------------
// 1.2.1
// D_INTERNAL_SQLITE_CHANGES
//   macro: the function that counts the rows the last statement changed:
// sqlite3_changes64 (3.37.0), whose count survives past 2^31, and
// sqlite3_changes before it. Chosen here, from the version env detected (which
// a build may lower by hand to target an older shared library), so that no
// function body needs a conditional.
// 1.2.2
// D_INTERNAL_SQLITE_TOTAL_CHANGES
//   macro: as D_INTERNAL_SQLITE_CHANGES, over the connection's lifetime.
#if D_ENV_SQLITE_VERSION_AT_LEAST(3, 37, 0)
    #define D_INTERNAL_SQLITE_CHANGES       sqlite3_changes64
    #define D_INTERNAL_SQLITE_TOTAL_CHANGES sqlite3_total_changes64
#else
    #define D_INTERNAL_SQLITE_CHANGES       sqlite3_changes
    #define D_INTERNAL_SQLITE_TOTAL_CHANGES sqlite3_total_changes
#endif

// 1.2.3
// D_INTERNAL_SQLITE_BIND64
//   macro: 1 where text and bytes are bound by a 64-bit length --
// sqlite3_bind_text64 and sqlite3_bind_blob64, from 3.8.7 -- and 0 where
// sqlite3_bind_text and sqlite3_bind_blob take an int, past which the layer
// answers SQLITE_TOOBIG itself.
#if D_ENV_SQLITE_VERSION_AT_LEAST(3, 8, 7)
    #define D_INTERNAL_SQLITE_BIND64 1
#else
    #define D_INTERNAL_SQLITE_BIND64 0
#endif


//==============================================================================
// 2.  NAMES
//==============================================================================


// 2.1    Schemas and names
//------------------------------------------------------------------------------
// names -- none of these allocate or fail
//   _schema resolves NULL to "main"; _schema_table names the table a schema
// lists its objects in, which for "temp" is sqlite_temp_master on every
// version (temp.sqlite_master resolves only from 3.33.0); _is_main matches
// "main" as SQLite does, ignoring ASCII case; _name_ok is false for a NULL or
// empty name, and _schema_ok for an empty one, NULL meaning "main";
// _runtime_at_least compares the library loaded now
const char* d_internal_sqlite_schema(const char* _schema);
const char* d_internal_sqlite_schema_table(const char* _schema);
bool        d_internal_sqlite_is_main(const char* _schema);
bool        d_internal_sqlite_name_ok(const char* _name);
bool        d_internal_sqlite_schema_ok(const char* _schema);
bool        d_internal_sqlite_runtime_at_least(int _version);


//==============================================================================
// 3.  STATEMENTS
//==============================================================================
// Formats are sqlite3_mprintf's: %w doubles double quotes, for a name written
// between them; %q doubles single quotes, and %Q also adds them, or writes
// NULL for a NULL pointer. A format that names %I or %T, which
// sqlite3_mprintf stops at, is the shared formatter's instead (db_internal.h,
// 3.2), quoted as SQLite quotes: the dialect the other layers read.


// 3.1    Preparing and running
//------------------------------------------------------------------------------
/**
 * @brief Formats a statement and prepares it.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _format     the statement, as a format of either dialect.
 * @param[in]  _args       the format's arguments.
 * @return SQLITE_OK; SQLITE_NOMEM when formatting ran out of memory;
 *         SQLITE_MISUSE for a shared-dialect format it refuses; or the
 *         result of sqlite3_prepare_v2.
 * @post On success the caller finalizes `*_statement`.
 */
int  d_internal_sqlite_vprepare(sqlite3*       _db,
                                sqlite3_stmt** _statement,
                                const char*    _format,
                                va_list        _args);
// as d_internal_sqlite_vprepare, taking the arguments directly
int  d_internal_sqlite_prepare(sqlite3*       _db,
                               sqlite3_stmt** _statement,
                               const char*    _format,
                               ...);
/**
 * @brief Prepares exactly one statement, as written.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _sql        the statement.
 * @return SQLITE_OK; the result of sqlite3_prepare_v2; SQLITE_MISUSE for
 *         text that holds no statement, or another after the first.
 * @post On success the caller finalizes `*_statement`.
 *
 * The text after the statement is read, not compiled, so refusing it leaves
 * the connection's error state as the first statement's preparation left it.
 */
int  d_internal_sqlite_prepare_one(sqlite3*       _db,
                                   sqlite3_stmt** _statement,
                                   const char*    _sql);
// as d_internal_sqlite_prepare_one, the statement formatted first: SQLITE_NOMEM
// when formatting ran out of memory, SQLITE_MISUSE for a shared-dialect format
// it refuses
int  d_internal_sqlite_vprepare_one(sqlite3*       _db,
                                    sqlite3_stmt** _statement,
                                    const char*    _format,
                                    va_list        _args);
/**
 * @brief Formats one or more statements and runs them to completion.
 *
 * @param[in] _db      the connection.
 * @param[in] _format  the statements, as a format of either dialect.
 * @param[in] _args    the format's arguments.
 * @return SQLITE_OK; SQLITE_NOMEM when formatting ran out of memory;
 *         SQLITE_MISUSE for a shared-dialect format it refuses; or the
 *         result of the statement that failed.
 */
int  d_internal_sqlite_vexecute(sqlite3*    _db,
                                const char* _format,
                                va_list     _args);
// as d_internal_sqlite_vexecute, taking the arguments directly
int  d_internal_sqlite_execute(sqlite3*    _db,
                               const char* _format,
                               ...);
/**
 * @brief Runs every statement in text, one at a time, and counts the rows
 *        the last one changed.
 *
 * @param[in]  _db       the connection.
 * @param[out] _changes  receives the count, as d_sqlite_exec_changes
 *                       describes it; written only on success.
 * @param[in]  _sql      the statements.
 * @return SQLITE_OK, or the result of the statement that failed, after which
 *         none runs.
 *
 * Each statement is prepared where the last ended, as sqlite3_exec prepares
 * them; text that holds none counts 0.
 */
int  d_internal_sqlite_execute_counted(sqlite3*    _db,
                                       int64_t*    _changes,
                                       const char* _sql);
// as d_internal_sqlite_execute_counted, the statements formatted first:
// SQLITE_NOMEM when formatting ran out of memory, SQLITE_MISUSE for a
// shared-dialect format it refuses
int  d_internal_sqlite_vexecute_counted(sqlite3*    _db,
                                        int64_t*    _changes,
                                        const char* _format,
                                        va_list     _args);
/**
 * @brief Finalizes a statement and settles the result of using it.
 *
 * @param[in] _statement  the statement; may be `NULL`.
 * @param[in] _result     the last sqlite3_step (or bind) result.
 * @return `_result` when it was a failure, so the failure is what the caller
 *         sees; otherwise what sqlite3_finalize returned.
 * @post `_statement` is invalid.
 */
int  d_internal_sqlite_finish(sqlite3_stmt* _statement,
                              int           _result);

// 3.2    Single-value results
//------------------------------------------------------------------------------
// single values -- each steps a prepared statement once, reads its first
// column, and finalizes it, so the statement is gone whatever happens. A row
// yields SQLITE_OK and the value, written only then; no row is D_DB_NOT_FOUND;
// a failure is its SQLite result code. A NULL reads as 0, or as a NULL string.
// _text's string is the caller's, released with d_db_free.
int  d_internal_sqlite_step_int64(sqlite3_stmt* _statement,
                                  int64_t*      _value);
int  d_internal_sqlite_step_double(sqlite3_stmt* _statement,
                                   double*       _value);
int  d_internal_sqlite_step_text(sqlite3_stmt* _statement,
                                 char**        _value);

// 3.3    Name lists
//------------------------------------------------------------------------------
/**
 * @brief Steps a prepared statement to its end, collecting one column of each
 *        row into a name list, and finalizes it.
 *
 * @param[in]  _statement  the statement; finalized whatever happens.
 * @param[in]  _column     the column holding the names; a NULL is skipped.
 * @param[out] _names      receives the list; written only on success.
 * @return SQLITE_OK, SQLITE_NOMEM when the list could not be built, or the
 *         SQLite result code of the step that failed.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_internal_sqlite_collect_names(sqlite3_stmt*          _statement,
                                     int                    _column,
                                     struct d_db_name_list* _names);

// 3.4    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs a prepared statement from its beginning to its end, collecting
 *        its rows, and resets it.
 *
 * @param[in]  _statement  the statement; reset before its first step and
 *                         after its last, its values kept.
 * @param[out] _rows       receives the rows; written only on success.
 * @return SQLITE_OK, SQLITE_NOMEM when the rows could not be built, or the
 *         SQLite result code of the step that failed.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int  d_internal_sqlite_collect_rows(sqlite3_stmt*     _statement,
                                    struct d_db_rows* _rows);
/**
 * @brief Runs a prepared statement from its beginning to its end, discarding
 *        its rows, and resets it.
 *
 * @param[in]  _statement  the statement; reset before its first step and
 *                         after its last, its values kept.
 * @param[out] _changes    receives, on success, the rows it changed, as
 *                         d_sqlite_execute_update counts them.
 * @return SQLITE_OK, or the SQLite result code of the step that failed.
 *
 * The connection's running total of changes, read before and after, tells a
 * statement that changed rows from one that did not, for which
 * sqlite3_changes still holds an earlier statement's count.
 */
int  d_internal_sqlite_run(sqlite3_stmt* _statement,
                           int64_t*      _changes);
// the type a declared type names, as sqlite_query.h tabulates; NULL, for an
// expression's column, is D_DB_TYPE_NONE
enum d_db_type
     d_internal_sqlite_declared_type(const char* _declared);


#endif  // D_INTERNAL_SQLITE


#endif  // DJINTERP_C_DB_SQLITE_SQLITE_INTERNAL_H
