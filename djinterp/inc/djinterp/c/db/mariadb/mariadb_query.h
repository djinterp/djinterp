/*******************************************************************************
* djinterp [c]                                                   mariadb_query.h
*
* Running SQL on a MariaDB or MySQL connection without handling result sets
* by hand.
*   d_mariadb_exec runs one statement and discards any rows it returns, and
* d_mariadb_exec_changes also counts the rows it changed; d_mariadb_exec_script
* runs several, separated by semicolons, by turning the client's
* multi-statement mode on for just that call. The d_mariadb_query_* functions
* run one statement and return the first column of its first row, which
* covers the common single-value case -- a count, a version, a setting -- in
* one call; d_mariadb_query_rows returns all of its rows, as db_common.h's
* d_db_rows.
*   Each comes in a plain form, which takes SQL as written, and an `f` form,
* which takes a printf-like format: %Q writes a string as a quoted value (and
* NULL as NULL), %I as a quoted name, and %T a qualifier and a name as
* `qualifier`.`name`; %s writes text as it is, and %d, %u, %ld, %lld, %f, %g
* and their kin write numbers. Use the `f` forms to put values into SQL
* safely, and the plain forms for SQL that holds a `%`, such as a LIKE
* pattern. Quoting goes through the client, so it suits the connection's
* character set and the server's SQL mode.
*   ROWS. The protocol sends values as text, which the layer reads as the
* column's type says: an integer column's values as INTEGER, except an
* unsigned BIGINT's past INT64_MAX, which stay TEXT; FLOAT and DOUBLE as
* REAL, read back exactly from the shortest text the servers write; a binary
* string, BLOB, BIT or spatial value as a BLOB; and every other value -- DECIMAL
* among them, which a double cannot hold exactly -- as TEXT. A column's type
* is its field type, and type_name its name as DDL writes it, " UNSIGNED"
* added where the column has it:
*     TINYINT(1)                                    D_DB_TYPE_BOOLEAN
*     TINYINT, SMALLINT, MEDIUMINT, INT, YEAR       D_DB_TYPE_INTEGER
*     INT UNSIGNED, BIGINT                          D_DB_TYPE_BIG_INTEGER
*     FLOAT, DOUBLE                                 D_DB_TYPE_FLOATING_POINT
*     DECIMAL                                       D_DB_TYPE_DECIMAL
*     CHAR, VARCHAR, the TEXTs, ENUM, SET           D_DB_TYPE_STRING
*     BINARY, VARBINARY, the BLOBs, BIT             D_DB_TYPE_BINARY
*     DATE, TIME, DATETIME, TIMESTAMP               the type of that name
*     JSON                                          D_DB_TYPE_JSON
*     GEOMETRY, NULL                                D_DB_TYPE_NONE
* A string column counts as binary by its character set, `binary`. TINYTEXT,
* MEDIUMTEXT and LONGTEXT, and their BLOBs, are named so. MySQL's JSON is a
* type of its own; MariaDB's JSON is a LONGTEXT, and arrives as TEXT.
*   ATTRIBUTES. MySQL from 8.0.23 takes attributes with a query -- names and
* values beside its text, which the server's query_attributes component
* reads with mysql_query_attribute_string() -- set here for the next query
* the connection sends.
*   STATEMENTS SENT. d_mariadb_send_query_start sends SQL -- one statement,
* or several where the multi-statement mode allows -- and
* d_mariadb_next_result_start reads its results one at a time, each without
* blocking (db_common.h 1.3), as PostgreSQL's layer's do.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_query.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.25
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
2.  QUERY ATTRIBUTES
    ----------------
    1.  Setting and clearing
3.  STATEMENTS SENT
    ---------------
    1.  Without blocking
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_QUERY_H
#define DJINTERP_C_DB_MARIADB_MARIADB_QUERY_H 1

// djinterp
#include "../../djinterp.h"    // framework root
#include "./mariadb_common.h"  // d_mariadb, the layer's results
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
// running statements -- each returns 0, the client's error number for the
// statement that failed, which d_mariadb_error_message then describes,
// CR_OUT_OF_MEMORY (2008) when the text could not be built, or
// D_DB_INVALID_ARGUMENT for a NULL handle or SQL, or a format the layer
// cannot read. Rows a statement returns are read and discarded.
//   _exec runs one statement; _execf formats it first; _exec_script runs
// several, separated by semicolons, stopping at the first that fails, the
// ones before it having run. A script that fails leaves the client's
// multi-statement mode on until the layer next runs a statement, so that
// d_mariadb_error_message can still describe the failure.
int    d_mariadb_exec(struct d_mariadb* _db,
                      const char*       _sql) D_NOTHROW;
int    d_mariadb_execf(struct d_mariadb* _db,
                       const char*       _format,
                       ...) D_NOTHROW;
int    d_mariadb_exec_script(struct d_mariadb* _db,
                             const char*       _sql) D_NOTHROW;

// 1.2    Rows changed
//------------------------------------------------------------------------------
// rows changed -- as d_mariadb_exec and d_mariadb_execf, and on success write
// what d_mariadb_changes would read straight after: the rows the statement
// affected (an UPDATE counts those it changed, not those it matched), or the
// rows it returned; 0 for a statement that counts none, such as CREATE TABLE.
// Written only on success.
int    d_mariadb_exec_changes(struct d_mariadb* _db,
                              int64_t*          _changes,
                              const char*       _sql) D_NOTHROW;
int    d_mariadb_execf_changes(struct d_mariadb* _db,
                               int64_t*          _changes,
                               const char*       _format,
                               ...) D_NOTHROW;

// 1.3    Single values
//------------------------------------------------------------------------------
// single values -- each runs one statement and reads the first column of its
// first row. A row yields 0 and the value, written only then; no row, or no
// rows at all, is D_DB_NOT_FOUND; a failure is as for the statements above.
// A NULL value reads as 0, or as a NULL string. The text is the caller's,
// released with d_db_free. The numbers come as text, which the layer reads
// whatever the C locale: _int64 reads an integer exactly, and a number with a
// fraction or an exponent truncated toward zero; _double rounds to the
// nearest double. A value that is not a number, or not one the type can
// hold, is D_DB_INVALID_ARGUMENT, the statement having run.
int    d_mariadb_query_int64(struct d_mariadb* _db,
                             int64_t*          _value,
                             const char*       _sql) D_NOTHROW;
int    d_mariadb_query_int64f(struct d_mariadb* _db,
                              int64_t*          _value,
                              const char*       _format,
                              ...) D_NOTHROW;
int    d_mariadb_query_double(struct d_mariadb* _db,
                              double*           _value,
                              const char*       _sql) D_NOTHROW;
int    d_mariadb_query_doublef(struct d_mariadb* _db,
                               double*           _value,
                               const char*       _format,
                               ...) D_NOTHROW;
int    d_mariadb_query_text(struct d_mariadb* _db,
                            char**            _value,
                            const char*       _sql) D_NOTHROW;
int    d_mariadb_query_textf(struct d_mariadb* _db,
                             char**            _value,
                             const char*       _format,
                             ...) D_NOTHROW;

// 1.4    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs one statement and returns every row it produced.
 *
 * @param[in]  _db    the connection.
 * @param[out] _rows  receives the rows, typed as the header describes;
 *                    written only on success.
 * @param[in]  _sql   one statement; a second is the server's syntax error.
 * @return 0; the client's error number for the failure; CR_OUT_OF_MEMORY
 *         (2008) when the rows could not be built; D_DB_INVALID_ARGUMENT for
 *         a `NULL` argument.
 * @post On success the caller releases `_rows` with d_db_rows_free. A
 *       statement that returns no rows -- an INSERT, say -- gives rows with
 *       no columns. Every result after the first, which only a CALL
 *       produces, is read and discarded.
 */
int    d_mariadb_query_rows(struct d_mariadb* _db,
                            struct d_db_rows* _rows,
                            const char*       _sql) D_NOTHROW;
/**
 * @brief Formats a statement, runs it, and returns every row it produced, as
 *        d_mariadb_query_rows.
 *
 * @param[in]  _db      the connection.
 * @param[out] _rows    receives the rows; written only on success.
 * @param[in]  _format  one statement, as a format (above).
 * @param[in]  ...      the format's arguments.
 * @return as d_mariadb_query_rows; D_DB_INVALID_ARGUMENT also for a format
 *         the layer cannot read.
 * @post As for d_mariadb_query_rows.
 */
int    d_mariadb_query_rowsf(struct d_mariadb* _db,
                             struct d_db_rows* _rows,
                             const char*       _format,
                             ...) D_NOTHROW;

// 1.5    Quoting
//------------------------------------------------------------------------------
/**
 * @brief Quotes a name for SQL: in backquotes, any backquote in it doubled.
 *
 * @param[in] _name  the name.
 * @return the quoted name, which the caller releases with d_db_free; `NULL`
 *         for a `NULL` name, or when memory ran out.
 */
char*  d_mariadb_quote_identifier(const char* _name) D_NOTHROW;
/**
 * @brief Quotes a value for SQL, as the connection's character set and SQL
 *        mode require, quotes included; `NULL` becomes the keyword NULL.
 *
 * @param[in] _db    the connection whose rules apply.
 * @param[in] _text  the value.
 * @return the quoted value, which the caller releases with d_db_free; `NULL`
 *         for a `NULL` handle, or when memory ran out or the client refused.
 */
char*  d_mariadb_quote_literal(struct d_mariadb* _db,
                               const char*       _text) D_NOTHROW;


//==============================================================================
// 2.  QUERY ATTRIBUTES
//==============================================================================


// 2.1    Setting and clearing
//------------------------------------------------------------------------------
/**
 * @brief Sets an attribute the next query the connection sends carries, as
 *        mysql_bind_param does; setting a name again replaces its value.
 *
 * @note The next query is whatever the layer sends next, its own among them
 *       -- d_mariadb_has_table asks one, d_mariadb_set_charset sends SET
 *       NAMES -- and it drops every attribute set, whether it succeeds or
 *       not. A prepared statement's execution neither carries nor drops
 *       them; a reset, a change of user and a close drop them. A query a
 *       program sends with the client's own calls carries them too, the
 *       layer keeping its copies until its own next query.
 *
 * @param[in] _db     the connection.
 * @param[in] _name   the attribute's name; not empty.
 * @param[in] _value  its value, sent as a string.
 * @return 0; the client's error number, or CR_OUT_OF_MEMORY (2008), for a
 *         failure, after which the attributes are as they were;
 *         D_DB_UNSUPPORTED where the server is not MySQL 8.0.23 or later, or
 *         the client has no query attributes (Connector/C, or libmysqlclient
 *         before 8.0.23); D_DB_INVALID_ARGUMENT for a `NULL` argument, an
 *         empty name, or a value longer than the client's lengths carry.
 */
int    d_mariadb_set_query_attribute(struct d_mariadb* _db,
                                     const char*       _name,
                                     const char*       _value) D_NOTHROW;
// drops the attributes set and not yet sent: 0, whether any were, and
// wherever there can be none; D_DB_INVALID_ARGUMENT for a NULL handle
int    d_mariadb_clear_query_attributes(struct d_mariadb* _db) D_NOTHROW;


//==============================================================================
// 3.  STATEMENTS SENT
//==============================================================================


// 3.1    Without blocking
//------------------------------------------------------------------------------
// d_mariadb_send_query_start sends SQL as d_mariadb_exec would, never
// waiting: it answers D_DB_WAIT until the client has sent it,
// d_mariadb_send_query_continue going on each time -- and, with
// libmysqlclient, whose calls read a reply only with its query, until the
// first statement has run and its answer has come. MariaDB's client sends
// alone, as PostgreSQL's layer does. d_mariadb_next_result_start then gives
// the results one a call, each once it has arrived -- a statement's rows,
// typed as 1.4 describes, or empty rows for one that returns none, and its
// count -- and D_DB_NOT_FOUND once every result is read, continued by
// d_mariadb_next_result_continue meanwhile. The first statement's failure
// comes as the first result, as every failure does, and ends the results; a
// failure of the client's own while sending -- the connection lost -- is the
// send's. Until the last result is read the connection is held: every other
// command on it is 2014, a send among them. MariaDB's client and
// libmysqlclient from 8.0.16 can do both (D_DB_UNSUPPORTED otherwise, and
// over TLS a client cannot carry so), as mariadb_connection.h 1.8 describes.
// The SQL is copied: the caller's text need not outlive the start.
/**
 * @brief Sends SQL without blocking.
 *
 * @param[in]  _db       the connection.
 * @param[in]  _sql      one statement, or several where the multi-statement
 *                       mode is on.
 * @param[out] _pending  receives what to wait for while it is being sent.
 * @return 0, sent, its results to read; D_DB_WAIT; the client's error
 *         number for a failure of its own; 2014 while the connection is
 *         held; D_DB_UNSUPPORTED for a client that cannot send without
 *         blocking, or not over the connection's TLS;
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, or SQL longer than
 *         the client's lengths carry.
 * @post On 0 the results are read with d_mariadb_next_result_start until it
 *       answers D_DB_NOT_FOUND, or a failure.
 */
int    d_mariadb_send_query_start(struct d_mariadb*    _db,
                                  const char*          _sql,
                                  struct d_db_pending* _pending) D_NOTHROW;
/**
 * @brief Gives the next result of the SQL sent once it has arrived, without
 *        waiting for it.
 *
 * @param[in]  _db       the connection.
 * @param[out] _pending  receives what to wait for while it has yet to
 *                       arrive.
 * @param[out] _rows     receives the statement's rows, or empty rows for
 *                       one that returns none; written only on success.
 * @param[out] _changes  receives, on success, the rows it changed or
 *                       returned, as d_mariadb_changes counts them; may be
 *                       `NULL`.
 * @return 0; D_DB_WAIT; D_DB_NOT_FOUND when every result has been read, or
 *         nothing was sent; the client's error number for the statement that
 *         failed, after which only D_DB_NOT_FOUND follows; CR_OUT_OF_MEMORY
 *         (2008) when the rows could not be built; 2014 while another call
 *         holds the connection; D_DB_UNSUPPORTED as for
 *         d_mariadb_send_query_start; D_DB_INVALID_ARGUMENT for a `NULL`
 *         connection, pending call or rows.
 * @post On success the caller releases `_rows` with d_db_rows_free.
 */
int    d_mariadb_next_result_start(struct d_mariadb*    _db,
                                   struct d_db_pending* _pending,
                                   struct d_db_rows*    _rows,
                                   int64_t*             _changes) D_NOTHROW;
// continuing -- _send_query_continue goes on with a send, and
// _next_result_continue with a result's read, given what came, as d_db_wait,
// and what the start or the last continuation wrote, which receives what to
// wait for next; each answers as its start does, and D_DB_INVALID_ARGUMENT
// for a NULL argument or no such call under way
int    d_mariadb_send_query_continue(struct d_mariadb*    _db,
                                     unsigned             _events,
                                     struct d_db_pending* _pending) D_NOTHROW;
int    d_mariadb_next_result_continue(struct d_mariadb*    _db,
                                      unsigned             _events,
                                      struct d_db_pending* _pending,
                                      struct d_db_rows*    _rows,
                                      int64_t*             _changes)
                                      D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_QUERY_H
