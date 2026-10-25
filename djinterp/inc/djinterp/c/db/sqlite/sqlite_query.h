/*******************************************************************************
* djinterp [c]                                                    sqlite_query.h
*
* Running SQL on an SQLite connection without preparing statements by hand.
*   d_sqlite_exec runs any number of statements and discards their rows, and
* d_sqlite_exec_changes reports how many rows the last of them changed. The
* d_sqlite_query_* functions run one statement and return the first column of
* its first row, which covers the common single-value case -- a count, a
* version, a setting -- in one call; d_sqlite_query_rows returns all of its
* rows, as db_common.h's d_db_rows.
*   Each comes in a plain form, which takes SQL as written, and an `f` form,
* which takes a printf-like format in one of two dialects. A format is
* sqlite3_mprintf's -- %q doubles single quotes, %Q also surrounds the value
* with them (or writes NULL for a NULL pointer), %w doubles double quotes,
* for a name written between them, with printf's flags and widths -- unless
* it names %I or %T, which sqlite3_mprintf stops at. Then it is the dialect
* MariaDB's and PostgreSQL's layers read: %Q a value, %I a name, %T a
* qualifier and a name written "qualifier"."name" (the name alone when the
* qualifier is NULL), %s and %c as written (%s of NULL writes NULL), %d,
* %i, %u and their l, ll and (%zu) z forms, %f, and %g to the 17 digits that
* read back as the same double; no flags, widths or precisions, and NaN,
* infinity, a NUL character or a NULL name are refused with SQLITE_MISUSE.
* Use the `f` forms to put values into SQL safely, and the plain forms for
* SQL that holds a `%`, such as a LIKE pattern. Values that are not text are
* better bound to a prepared statement (sqlite_statement.h).
*   ROWS. Each value arrives as SQLite stored it: an INTEGER, a REAL, TEXT,
* a BLOB or NULL, value by value, whatever its column's declared type says.
* A column's type is read from the type its CREATE TABLE declared, which
* type_name holds ("" for an expression), by the names below, or failing
* them by SQLite's affinity rules:
*     BOOLEAN, BOOL                                    D_DB_TYPE_BOOLEAN
*     TINYINT, SMALLINT, MEDIUMINT, INT2               D_DB_TYPE_INTEGER
*     INT, INTEGER, BIGINT, INT8, UNSIGNED BIG INT,
*     and any other naming INT                         D_DB_TYPE_BIG_INTEGER
*     REAL, DOUBLE, DOUBLE PRECISION, FLOAT, and any
*     other naming REAL, FLOA or DOUB                  D_DB_TYPE_FLOATING_POINT
*     NUMERIC, DECIMAL, and any other not named here   D_DB_TYPE_DECIMAL
*     TEXT, CHARACTER, VARCHAR, NCHAR, NVARCHAR, CLOB,
*     and any other naming CHAR, CLOB or TEXT          D_DB_TYPE_STRING
*     BLOB, and any other naming BLOB                  D_DB_TYPE_BINARY
*     DATE, TIME, DATETIME, TIMESTAMP                  the type of that name
*     JSON, JSONB; XML; UUID, GUID                     D_DB_TYPE_JSON, _XML,
*                                                      _UUID
*     none, or ANY                                     D_DB_TYPE_NONE
* Every integer SQLite holds is 64-bit, so INT and INTEGER read as
* BIG_INTEGER, and the smaller names as INTEGER only because they say so.
* ANY reads as NONE in every table, as STRICT means it, though SQLite gives it
* numeric affinity elsewhere; a name is matched whole, a size in parentheses
* aside, so TIMESTAMP(3) is a TIMESTAMP and TIMESTAMP WITH TIME ZONE falls to
* the affinity rules.
*
*
* path:      /inc/djinterp/c/db/sqlite/sqlite_query.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  QUERIES
    -------
    1.  Running statements
    2.  Rows changed
    3.  Single values
    4.  Rows
    5.  Quoting
*/

#ifndef DJINTERP_C_DB_SQLITE_SQLITE_QUERY_H
#define DJINTERP_C_DB_SQLITE_SQLITE_QUERY_H 1

// djinterp
#include "../../djinterp.h"   // framework root
#include "./sqlite_common.h"  // sqlite3, the layer's results
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
// 1.  QUERIES
//==============================================================================


// 1.1    Running statements
//------------------------------------------------------------------------------
/**
 * @brief Runs one or more SQL statements, discarding any rows they produce.
 *
 * @param[in] _db   the connection.
 * @param[in] _sql  the statements, separated by semicolons.
 * @return SQLITE_OK, or the SQLite result code of the first statement that
 *         failed, after which none runs; SQLITE_MISUSE for a `NULL` argument.
 */
int   d_sqlite_exec(struct sqlite3* _db,
                    const char*     _sql) D_NOTHROW;
/**
 * @brief Formats SQL and runs it, as d_sqlite_exec.
 *
 * @param[in] _db      the connection.
 * @param[in] _format  the statements, as a format of either dialect (see
 *                     above).
 * @param[in] ...      the format's arguments.
 * @return SQLITE_OK; SQLITE_NOMEM when formatting ran out of memory;
 *         SQLITE_MISUSE for a shared-dialect format it refuses; or as
 *         d_sqlite_exec.
 */
int   d_sqlite_execf(struct sqlite3* _db,
                     const char*     _format,
                     ...) D_NOTHROW;

// 1.2    Rows changed
//------------------------------------------------------------------------------
// rows changed -- as d_sqlite_exec, and on success writes how many rows the
// last statement inserted, updated or deleted, not counting a trigger's; the
// rows it returned, for one that changes none and returns rows, a SELECT
// say; and 0 for any other, CREATE TABLE among them. Written only on success.
// The `f` form fails as d_sqlite_execf does.
int   d_sqlite_exec_changes(struct sqlite3* _db,
                            int64_t*        _changes,
                            const char*     _sql) D_NOTHROW;
int   d_sqlite_execf_changes(struct sqlite3* _db,
                             int64_t*        _changes,
                             const char*     _format,
                             ...) D_NOTHROW;

// 1.3    Single values
//------------------------------------------------------------------------------
// single values -- each runs one statement and reads the first column of its
// first row. The value is written only on SQLITE_OK. A statement producing no
// row is D_DB_NOT_FOUND; a NULL reads as 0, and as a NULL string. A failure is
// its SQLite result code; a NULL argument, SQLITE_MISUSE; and in the `f`
// forms, formatting that ran out of memory, SQLITE_NOMEM, and a format or an
// argument the shared dialect refuses, SQLITE_MISUSE. The text forms
// allocate: release the string with d_db_free.
int   d_sqlite_query_int64(struct sqlite3* _db,
                           int64_t*        _value,
                           const char*     _sql) D_NOTHROW;
int   d_sqlite_query_int64f(struct sqlite3* _db,
                            int64_t*        _value,
                            const char*     _format,
                            ...) D_NOTHROW;
int   d_sqlite_query_double(struct sqlite3* _db,
                            double*         _value,
                            const char*     _sql) D_NOTHROW;
int   d_sqlite_query_doublef(struct sqlite3* _db,
                             double*         _value,
                             const char*     _format,
                             ...) D_NOTHROW;
int   d_sqlite_query_text(struct sqlite3* _db,
                          char**          _value,
                          const char*     _sql) D_NOTHROW;
int   d_sqlite_query_textf(struct sqlite3* _db,
                           char**          _value,
                           const char*     _format,
                           ...) D_NOTHROW;

// 1.4    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs one statement and returns every row it produced.
 *
 * @param[in]  _db    the connection.
 * @param[out] _rows  receives the rows, typed as the header describes;
 *                    written only on success.
 * @param[in]  _sql   one statement; text holding another after it is
 *                    refused, before anything runs.
 * @return SQLITE_OK; SQLITE_NOMEM when the rows could not be built; the
 *         SQLite result code of the failure; SQLITE_MISUSE for a `NULL`
 *         argument, for text holding no statement, or more than one.
 * @post On success the caller releases `_rows` with d_db_rows_free. A
 *       statement that returns no rows -- an INSERT, say -- gives rows with
 *       its columns, if any, and none of them.
 */
int   d_sqlite_query_rows(struct sqlite3*   _db,
                          struct d_db_rows* _rows,
                          const char*       _sql) D_NOTHROW;
/**
 * @brief Formats a statement, runs it, and returns every row it produced, as
 *        d_sqlite_query_rows.
 *
 * @param[in]  _db      the connection.
 * @param[out] _rows    receives the rows; written only on success.
 * @param[in]  _format  one statement, as a format of either dialect.
 * @param[in]  ...      the format's arguments.
 * @return as d_sqlite_query_rows; SQLITE_NOMEM when formatting ran out of
 *         memory; SQLITE_MISUSE for a shared-dialect format it refuses.
 * @post As for d_sqlite_query_rows.
 */
int   d_sqlite_query_rowsf(struct sqlite3*   _db,
                           struct d_db_rows* _rows,
                           const char*       _format,
                           ...) D_NOTHROW;

// 1.5    Quoting
//------------------------------------------------------------------------------
/**
 * @brief Quotes a name for use in SQL: "name", with any double quote in it
 *        doubled, so it can never end the name early.
 *
 * @param[in] _name  the name.
 * @return the quoted name, released with d_db_free; `NULL` for a `NULL` name
 *         or when memory ran out.
 */
char* d_sqlite_quote_identifier(const char* _name) D_NOTHROW;
/**
 * @brief Quotes text as an SQL string literal: 'text', with any single quote
 *        in it doubled.
 *
 * @param[in] _text  the text; `NULL` quotes as the keyword NULL.
 * @return the literal, released with d_db_free; `NULL` when memory ran out.
 */
char* d_sqlite_quote_literal(const char* _text) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_SQLITE_SQLITE_QUERY_H
