/*******************************************************************************
* djinterp [c]                                                  postgres_query.h
*
* Running SQL on a PostgreSQL connection without handling results by hand.
*   d_postgres_exec runs SQL and discards any rows it returns; the SQL may
* hold several statements, separated by semicolons, which run as one
* transaction unless they hold their own BEGIN and COMMIT, as libpq's simple
* query runs them. d_postgres_exec_changes does the same and reports how
* many rows the last statement changed. The d_postgres_query_* functions run
* SQL and return the first column of its first row, which covers the common
* single-value case -- a count, an EXISTS, a setting, an INSERT's RETURNING
* id -- in one call; d_postgres_query_rows runs one statement and returns all
* of its rows, as db_common.h's d_db_rows, and d_postgres_query_rows_params
* does so with values for the statement's parameters, apart from its SQL.
*   Each comes in a plain form, which takes SQL as written, and an `f` form,
* which takes a printf-like format: %Q writes a string as a quoted value (and
* NULL as NULL), %I as a quoted name, and %T a qualifier and a name as
* "qualifier"."name"; %s writes text as it is, and %d, %u, %ld, %lld, %f, %g
* and their kin write numbers. Use the `f` forms to put values into SQL
* safely, and the plain forms for SQL that holds a `%`, such as a LIKE
* pattern. Values are quoted by libpq, so they suit the connection's encoding
* and its standard_conforming_strings.
*   ROWS. The server sends values as text, which the layer reads by the
* column's type: boolean as an INTEGER, 1 or 0; smallint, integer, bigint
* and oid as INTEGER; real and double precision as REAL, NaN and the
* infinities included, read exactly from the shortest text servers from 12
* write (before 12, extra_float_digits decides how many digits come); bytea
* as a BLOB, decoded from the text, hex or escaped; and every other value --
* numeric among them, which a double cannot hold exactly -- as TEXT. A
* column's type is read from its type's OID, and type_name is the type's
* name as format_type writes it, "" for one not listed here:
*     boolean                                       D_DB_TYPE_BOOLEAN
*     smallint, integer                             D_DB_TYPE_INTEGER
*     bigint, oid                                   D_DB_TYPE_BIG_INTEGER
*     real, double precision                        D_DB_TYPE_FLOATING_POINT
*     numeric                                       D_DB_TYPE_DECIMAL
*     text, character varying, character, name,
*     "char"                                        D_DB_TYPE_STRING
*     bytea                                         D_DB_TYPE_BINARY
*     date                                          D_DB_TYPE_DATE
*     time without time zone, time with time zone   D_DB_TYPE_TIME
*     timestamp without time zone                   D_DB_TYPE_DATETIME
*     timestamp with time zone                      D_DB_TYPE_TIMESTAMP
*     json, jsonb                                   D_DB_TYPE_JSON
*     xml                                           D_DB_TYPE_XML
*     uuid                                          D_DB_TYPE_UUID
*     interval, inet, cidr, macaddr, bit, bit
*     varying, money, unknown, void, and any other  D_DB_TYPE_NONE
*
*
* path:      /inc/djinterp/c/db/postgres/postgres_query.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.15
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

#ifndef DJINTERP_C_DB_POSTGRES_POSTGRES_QUERY_H
#define DJINTERP_C_DB_POSTGRES_POSTGRES_QUERY_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"     // framework root
#include "./postgres_common.h"  // pg_conn, the layer's results
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
// 1.  QUERIES
//==============================================================================


// 1.1    Running statements
//------------------------------------------------------------------------------
// running statements -- each returns 0; the SQLSTATE of the statement that
// failed, which d_postgres_error_message then describes, the ones before it
// rolled back with it unless the SQL committed them itself; 53200 when the
// text could not be built; D_DB_UNSUPPORTED for COPY to or from the client,
// which the layer ends at once, leaving the connection ready; or
// D_DB_INVALID_ARGUMENT for a NULL connection or SQL, or a format the layer
// cannot read. Rows a statement returns are discarded.
int    d_postgres_exec(struct pg_conn* _db,
                       const char*     _sql) D_NOTHROW;
int    d_postgres_execf(struct pg_conn* _db,
                        const char*     _format,
                        ...) D_NOTHROW;

// 1.2    Rows changed
//------------------------------------------------------------------------------
// rows changed -- as d_postgres_exec, and on success writes how many rows the
// last statement inserted, updated, deleted or merged, or returned, for a
// SELECT; 0 for a statement that counts none, such as CREATE TABLE. Written
// only on success.
int    d_postgres_exec_changes(struct pg_conn* _db,
                               int64_t*        _changes,
                               const char*     _sql) D_NOTHROW;
int    d_postgres_execf_changes(struct pg_conn* _db,
                                int64_t*        _changes,
                                const char*     _format,
                                ...) D_NOTHROW;

// 1.3    Single values
//------------------------------------------------------------------------------
// single values -- each runs SQL and reads the first column of the first row
// of its last statement. A row yields 0 and the value, written only then; no
// row is D_DB_NOT_FOUND; a failure is as for the statements above. A NULL
// reads as 0, false, or a NULL string. The text is the caller's, released
// with d_db_free. Values come as text, which the layer reads whatever the C
// locale: _int64 reads an integer exactly, and a number with a fraction or
// an exponent truncated toward zero; _double rounds to the nearest double;
// _bool reads a boolean as the server writes one, t or f. A value of another
// shape, or one the type cannot hold, is D_DB_INVALID_ARGUMENT, the
// statement having run.
int    d_postgres_query_int64(struct pg_conn* _db,
                              int64_t*        _value,
                              const char*     _sql) D_NOTHROW;
int    d_postgres_query_int64f(struct pg_conn* _db,
                               int64_t*        _value,
                               const char*     _format,
                               ...) D_NOTHROW;
int    d_postgres_query_double(struct pg_conn* _db,
                               double*         _value,
                               const char*     _sql) D_NOTHROW;
int    d_postgres_query_doublef(struct pg_conn* _db,
                                double*         _value,
                                const char*     _format,
                                ...) D_NOTHROW;
int    d_postgres_query_text(struct pg_conn* _db,
                             char**          _value,
                             const char*     _sql) D_NOTHROW;
int    d_postgres_query_textf(struct pg_conn* _db,
                              char**          _value,
                              const char*     _format,
                              ...) D_NOTHROW;
int    d_postgres_query_bool(struct pg_conn* _db,
                             bool*           _value,
                             const char*     _sql) D_NOTHROW;
int    d_postgres_query_boolf(struct pg_conn* _db,
                              bool*           _value,
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
 * @param[in]  _sql   one statement; a second is the server's syntax error
 *                    (42601), as one prepared statement holds one.
 * @return 0; the SQLSTATE of the failure, packed; 53200 when the rows could
 *         not be built; D_DB_UNSUPPORTED for COPY to or from the client,
 *         which the layer ends at once; D_DB_INVALID_ARGUMENT for a `NULL`
 *         argument.
 * @post On success the caller releases `_rows` with d_db_rows_free. A
 *       statement that returns no rows -- an INSERT without RETURNING, say
 *       -- gives rows with no columns.
 */
int    d_postgres_query_rows(struct pg_conn*   _db,
                             struct d_db_rows* _rows,
                             const char*       _sql) D_NOTHROW;
/**
 * @brief Formats a statement, runs it, and returns every row it produced, as
 *        d_postgres_query_rows.
 *
 * @param[in]  _db      the connection.
 * @param[out] _rows    receives the rows; written only on success.
 * @param[in]  _format  one statement, as a format (above).
 * @param[in]  ...      the format's arguments.
 * @return as d_postgres_query_rows; D_DB_INVALID_ARGUMENT also for a format
 *         the layer cannot read.
 * @post As for d_postgres_query_rows.
 */
int    d_postgres_query_rowsf(struct pg_conn*   _db,
                              struct d_db_rows* _rows,
                              const char*       _format,
                              ...) D_NOTHROW;

/**
 * @brief Runs one statement with values for its parameters, written $1, $2
 *        and on, and returns every row it produced, in one round trip.
 *
 * @note The values travel apart from the SQL, so no value needs quoting and
 *       none can change what the statement does. Each is text, which the
 *       server reads as its parameter's type does: bytes for a bytea go as
 *       its text, "\x" and hex digits. A statement to run often is better
 *       prepared, once (postgres_statement.h).
 *
 * @param[in]  _db      the connection.
 * @param[out] _rows    receives the rows, typed as the header describes;
 *                      written only on success.
 * @param[in]  _sql     one statement.
 * @param[in]  _values  `_count` values, $1's first; a `NULL` one is SQL
 *                      NULL. May be `NULL` when `_count` is 0.
 * @param[in]  _count   how many values: as many as the statement has
 *                      parameters, up to 65535.
 * @return as d_postgres_query_rows; D_POSTGRES_SQLSTATE_PROTOCOL_VIOLATION
 *         (08P01) for a count the statement's parameters do not match; the
 *         server's SQLSTATE for a value its parameter's type cannot read
 *         (22P02); D_DB_INVALID_ARGUMENT also for `NULL` values with a
 *         nonzero count, or a count past 65535.
 * @post As for d_postgres_query_rows.
 */
int    d_postgres_query_rows_params(struct pg_conn*    _db,
                                    struct d_db_rows*  _rows,
                                    const char*        _sql,
                                    const char* const* _values,
                                    size_t             _count) D_NOTHROW;

// 1.5    Quoting
//------------------------------------------------------------------------------
/**
 * @brief Quotes a name for SQL: in double quotes, any double quote in it
 *        doubled, as every encoding PostgreSQL supports allows.
 *
 * @param[in] _name  the name.
 * @return the quoted name, which the caller releases with d_db_free; `NULL`
 *         for a `NULL` name, or when memory ran out.
 */
char*  d_postgres_quote_identifier(const char* _name) D_NOTHROW;
/**
 * @brief Quotes a value for SQL, as the connection's encoding and
 *        standard_conforming_strings require, quotes included; `NULL` becomes
 *        the keyword NULL.
 *
 * A value holding a backslash comes back in the escape-string form, E'...',
 * with a space before it, as libpq writes it so that it cannot run into the
 * token before it.
 *
 * @param[in] _db    the connection whose rules apply.
 * @param[in] _text  the value.
 * @return the quoted value, which the caller releases with d_db_free; `NULL`
 *         for a `NULL` connection, when memory ran out, or for text that is
 *         not valid in the connection's encoding, which libpq refuses from
 *         its releases of February 2025 (17.3, 16.7, 15.11, 14.16, 13.19);
 *         an older libpq quotes it, and the server then refuses the
 *         statement (22021).
 */
char*  d_postgres_quote_literal(struct pg_conn* _db,
                                const char*     _text) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_POSTGRES_POSTGRES_QUERY_H
