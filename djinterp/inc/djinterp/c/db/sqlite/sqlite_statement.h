/*******************************************************************************
* djinterp [c]                                                sqlite_statement.h
*
* Prepared statements on an SQLite connection: SQL compiled once, values bound
* to its parameters, and run as often as needed.
*   A statement is SQLite's own sqlite3_stmt, so these functions and SQLite's
* mix freely. d_sqlite_prepare compiles one statement. The bind functions give
* its parameters values -- written `?`, `?NNN`, `:name`, `@name` or `$name`,
* as SQLite reads them, and numbered from 1 as SQLite numbers them -- and a
* parameter never bound is NULL. d_sqlite_execute_update runs the statement
* and counts the rows it changed; d_sqlite_execute_query runs it and returns
* its rows, typed as sqlite_query.h describes. Each run starts from the
* beginning and ends with the statement reset and its values kept, so the next
* run needs nothing first, and no lock outlives a run. d_sqlite_open_cursor
* begins a run whose rows d_sqlite_fetch_rows reads a few at a time, as SQLite
* steps through them, until a fetch comes back short or d_sqlite_close_cursor
* ends it. d_sqlite_finalize releases the statement.
*   RESULTS. As every module's (sqlite_common.h): SQLITE_OK, or SQLite's
* result code for a failure, which d_sqlite_error_message on the statement's
* connection then describes; SQLITE_RANGE for a parameter the statement does
* not have, and SQLITE_MISUSE for an argument that cannot be right.
*   A statement belongs to its connection: d_sqlite_close waits for the last
* statement to be finalized before it closes the connection.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_statement.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STATEMENTS
    ----------
    1.  Preparing
    2.  Finalizing
    3.  Describing
2.  VALUES
    ------
    1.  Binding
    2.  Clearing
3.  RUNNING
    -------
    1.  Rows changed
    2.  Rows
    3.  Rows a few at a time
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_STATEMENT_H
#define DJINTERP_C_DB_SQLITE_SQLITE_STATEMENT_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"   // framework root
#include "./sqlite_common.h"  // sqlite3, sqlite3_stmt, the layer's results
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
// 1.  STATEMENTS
//==============================================================================


// 1.1    Preparing
//------------------------------------------------------------------------------
/**
 * @brief Compiles one SQL statement.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _sql        one statement; text holding another after it is
 *                         refused.
 * @return SQLITE_OK; the SQLite result code of the failure; SQLITE_MISUSE
 *         for a `NULL` argument, for text holding no statement, or more than
 *         one.
 * @post On success the caller releases `*_statement` with d_sqlite_finalize.
 */
int    d_sqlite_prepare(struct sqlite3*       _db,
                        struct sqlite3_stmt** _statement,
                        const char*           _sql) D_NOTHROW;
/**
 * @brief Formats one SQL statement and compiles it, as d_sqlite_prepare.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _format     one statement, as a format of either dialect
 *                         (sqlite_query.h); values are better bound than
 *                         formatted, but a name cannot be bound.
 * @param[in]  ...         the format's arguments.
 * @return as d_sqlite_prepare; SQLITE_NOMEM when formatting ran out of
 *         memory; SQLITE_MISUSE for a shared-dialect format it refuses.
 * @post As for d_sqlite_prepare.
 */
int    d_sqlite_preparef(struct sqlite3*       _db,
                         struct sqlite3_stmt** _statement,
                         const char*           _format,
                         ...) D_NOTHROW;

// 1.2    Finalizing
//------------------------------------------------------------------------------
/**
 * @brief Releases a statement.
 *
 * @param[in] _statement  the statement; may be `NULL`.
 * @return SQLITE_OK. (SQLite's own finalize repeats the failure of the
 *         statement's last run, which the run itself has already reported.)
 * @post `_statement` is invalid.
 */
int    d_sqlite_finalize(struct sqlite3_stmt* _statement) D_NOTHROW;

// 1.3    Describing
//------------------------------------------------------------------------------
// describing -- _parameter_count is the number of the highest parameter, 0
// for none or NULL; _parameter_index finds a named parameter by its name as
// written, its `:`, `@` or `$` included: SQLITE_OK and its number, written
// only then, D_DB_NOT_FOUND, or SQLITE_MISUSE for a NULL argument;
// _statement_connection is the connection the statement belongs to, NULL for
// NULL
size_t d_sqlite_parameter_count(struct sqlite3_stmt* _statement) D_NOTHROW;
int    d_sqlite_parameter_index(struct sqlite3_stmt* _statement,
                                const char*          _name,
                                size_t*              _index) D_NOTHROW;
struct sqlite3*
       d_sqlite_statement_connection(struct sqlite3_stmt* _statement)
       D_NOTHROW;


//==============================================================================
// 2.  VALUES
//==============================================================================


// 2.1    Binding
//------------------------------------------------------------------------------
// binding -- each gives the parameter numbered `_index`, from 1, a value
// until another is bound or the values are cleared: NULL; a boolean, as 1 or
// 0; an integer; a real number, SQLite storing a NaN as NULL; text, copied,
// NULL binding NULL; bytes, `_size` of them copied, NULL binding NULL and a
// pointer to none an empty BLOB. Each returns SQLITE_OK; SQLITE_RANGE for a
// parameter the statement does not have; SQLITE_TOOBIG for text or bytes
// past SQLite's limit; SQLITE_NOMEM; or SQLITE_MISUSE for a NULL statement,
// or for one part-way through a run (section 3.3), whose values SQLite reads
// as it goes.
int    d_sqlite_bind_null(struct sqlite3_stmt* _statement,
                          size_t               _index) D_NOTHROW;
int    d_sqlite_bind_bool(struct sqlite3_stmt* _statement,
                          size_t               _index,
                          bool                 _value) D_NOTHROW;
int    d_sqlite_bind_int64(struct sqlite3_stmt* _statement,
                           size_t               _index,
                           int64_t              _value) D_NOTHROW;
int    d_sqlite_bind_double(struct sqlite3_stmt* _statement,
                            size_t               _index,
                            double               _value) D_NOTHROW;
int    d_sqlite_bind_text(struct sqlite3_stmt* _statement,
                          size_t               _index,
                          const char*          _text) D_NOTHROW;
int    d_sqlite_bind_blob(struct sqlite3_stmt* _statement,
                          size_t               _index,
                          const void*          _bytes,
                          size_t               _size) D_NOTHROW;

// 2.2    Clearing
//------------------------------------------------------------------------------
// clearing -- every parameter NULL again: SQLITE_OK, or SQLITE_MISUSE for a
// NULL statement, or for one part-way through a run, as binding
int    d_sqlite_clear_bindings(struct sqlite3_stmt* _statement) D_NOTHROW;


//==============================================================================
// 3.  RUNNING
//==============================================================================


// 3.1    Rows changed
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement to its end, discarding any rows it returns.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _changes    receives, on success, the rows it inserted,
 *                         updated or deleted, not counting a trigger's; the
 *                         rows it returned, for one that changes none and
 *                         returns rows; 0 for any other; may be `NULL`.
 * @return SQLITE_OK; the SQLite result code of the failure; SQLITE_MISUSE
 *         for a `NULL` statement.
 * @post The statement is reset, its values kept, whatever the result.
 */
int    d_sqlite_execute_update(struct sqlite3_stmt* _statement,
                               int64_t*             _changes) D_NOTHROW;

// 3.2    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement to its end and returns every row it produced.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _rows       receives the rows, typed as sqlite_query.h
 *                         describes; written only on success.
 * @return SQLITE_OK; SQLITE_NOMEM when the rows could not be built; the
 *         SQLite result code of the failure; SQLITE_MISUSE for a `NULL`
 *         argument.
 * @post The statement is reset, its values kept, whatever the result. On
 *       success the caller releases `_rows` with d_db_rows_free.
 */
int    d_sqlite_execute_query(struct sqlite3_stmt* _statement,
                              struct d_db_rows*    _rows) D_NOTHROW;

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/**
 * @brief Begins a run of a statement whose rows d_sqlite_fetch_rows then
 *        reads a few at a time, rather than all at once.
 *
 * @note SQLite runs the statement step by step, as its rows are fetched, so
 *       the run begins with the first fetch, and from then until it ends the
 *       statement's values cannot change: binding one, or clearing them, is
 *       SQLITE_MISUSE. Meanwhile the connection runs other statements;
 *       running this one again, by any of the functions above, ends the run
 *       and begins another.
 *
 * @param[in] _statement  the statement, its values bound.
 * @return SQLITE_OK; SQLITE_MISUSE for a `NULL` statement.
 * @post The statement is reset, its values kept.
 */
int    d_sqlite_open_cursor(struct sqlite3_stmt* _statement) D_NOTHROW;
/**
 * @brief Reads the next rows of the run d_sqlite_open_cursor began.
 *
 * @param[in]  _statement  the statement.
 * @param[in]  _max        the most rows to read; 1 or more.
 * @param[out] _rows       receives the columns and up to `_max` rows, typed
 *                         as sqlite_query.h describes; fewer -- none among
 *                         them -- once the run has none left, which ends it:
 *                         the statement is then reset, and a further fetch
 *                         begins another run. Written only on success.
 * @return SQLITE_OK; SQLITE_NOMEM when the rows could not be built; the
 *         SQLite result code of the step that failed, which ends the run;
 *         SQLITE_MISUSE for a `NULL` argument or no rows asked for.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_sqlite_fetch_rows(struct sqlite3_stmt* _statement,
                           size_t               _max,
                           struct d_db_rows*    _rows) D_NOTHROW;
// ending a run -- _close_cursor ends the run d_sqlite_open_cursor began,
// whatever rows it had left, and resets the statement, its values kept:
// SQLITE_OK, or SQLITE_MISUSE for NULL
int    d_sqlite_close_cursor(struct sqlite3_stmt* _statement) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_SQLITE_SQLITE_STATEMENT_H
