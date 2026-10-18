/*******************************************************************************
* djinterp [c]                                               mariadb_statement.h
*
* Prepared statements on a MariaDB or MySQL connection: SQL the server
* compiles once, values bound to its parameters, and run as often as needed.
*   d_mariadb_prepare has the server compile one statement, whose parameters
* are written `?` and numbered from 1 by their place. The bind functions give
* them values, which the statement keeps until it runs, and a parameter never
* bound is NULL. d_mariadb_execute_update runs the statement and counts the
* rows it changed; d_mariadb_execute_query runs it and returns its rows,
* typed as mariadb_query.h describes, so that a value reads the same whether
* it came through a statement or a query. Each run reads every result the
* statement produced, so the connection is ready for the next statement
* whatever the run did; the values stay bound. d_mariadb_finalize releases
* the statement.
*   RESULTS. As every module's (mariadb_common.h): 0, or the client's error
* number for a failure -- a server error, such as 1062 (a duplicate key),
* which d_mariadb_error_message on the statement's connection then
* describes; CR_INVALID_PARAMETER_NO (2034) for a parameter the statement
* does not have; CR_OUT_OF_MEMORY (2008); D_DB_INVALID_ARGUMENT for an
* argument that cannot be right.
*   The binary protocol sends what the client asks for; the layer asks for
* each value as text, the protocol's text, and reads it as a query's.
*   A statement belongs to its connection. Finalize it before closing the
* connection, which the client otherwise leaves the statement unable to do
* anything but be finalized.
*
*
* path:      /inc/djinterp/c/db/mariadb/mariadb_statement.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.15
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
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_STATEMENT_H
#define DJINTERP_C_DB_MARIADB_MARIADB_STATEMENT_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../../djinterp.h"    // framework root
#include "./mariadb_common.h"  // d_mariadb, d_mariadb_statement, the results
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
// 1.  STATEMENTS
//==============================================================================


// 1.1    Preparing
//------------------------------------------------------------------------------
/**
 * @brief Has the server compile one SQL statement.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _sql        one statement; a second is the server's syntax
 *                         error, as is a statement the server cannot
 *                         prepare.
 * @return 0; the client's error number for the failure, which
 *         d_mariadb_error_message describes; CR_OUT_OF_MEMORY (2008);
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument.
 * @post On success the caller releases `*_statement` with
 *       d_mariadb_finalize.
 */
int    d_mariadb_prepare(struct d_mariadb*            _db,
                         struct d_mariadb_statement** _statement,
                         const char*                  _sql) D_NOTHROW;
/**
 * @brief Formats one SQL statement and has the server compile it, as
 *        d_mariadb_prepare.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; `NULL` on failure.
 * @param[in]  _format     one statement, as a format (mariadb_query.h);
 *                         values are better bound than formatted, but a
 *                         name cannot be bound.
 * @param[in]  ...         the format's arguments.
 * @return as d_mariadb_prepare; D_DB_INVALID_ARGUMENT also for a format the
 *         layer cannot read.
 * @post As for d_mariadb_prepare.
 */
int    d_mariadb_preparef(struct d_mariadb*            _db,
                          struct d_mariadb_statement** _statement,
                          const char*                  _format,
                          ...) D_NOTHROW;

// 1.2    Finalizing
//------------------------------------------------------------------------------
/**
 * @brief Releases a statement, and the server's copy of it.
 *
 * @param[in] _statement  the statement; may be `NULL`.
 * @return 0, or the client's error number when the server could not be told,
 *         the statement released all the same.
 * @post `_statement` is invalid.
 */
int    d_mariadb_finalize(struct d_mariadb_statement* _statement) D_NOTHROW;

// 1.3    Describing
//------------------------------------------------------------------------------
// describing -- _parameter_count is the number of `?` the statement has, 0
// for NULL; _statement_connection is the connection it belongs to, NULL for
// NULL
size_t d_mariadb_parameter_count(const struct d_mariadb_statement* _statement)
       D_NOTHROW;
struct d_mariadb*
       d_mariadb_statement_connection(
           const struct d_mariadb_statement* _statement) D_NOTHROW;


//==============================================================================
// 2.  VALUES
//==============================================================================


// 2.1    Binding
//------------------------------------------------------------------------------
// binding -- each gives the parameter numbered `_index`, from 1, a value
// until another is bound or the values are cleared: NULL; a boolean, as 1 or
// 0; an integer; a real number; text, copied, NULL binding NULL; bytes,
// `_size` of them copied, NULL binding NULL and a pointer to none the empty
// string of bytes. Each returns 0; CR_INVALID_PARAMETER_NO (2034) for a
// parameter the statement does not have; CR_OUT_OF_MEMORY (2008); or
// D_DB_INVALID_ARGUMENT for a NULL statement, or text or bytes longer than
// the client can send.
int    d_mariadb_bind_null(struct d_mariadb_statement* _statement,
                           size_t                      _index) D_NOTHROW;
int    d_mariadb_bind_bool(struct d_mariadb_statement* _statement,
                           size_t                      _index,
                           bool                        _value) D_NOTHROW;
int    d_mariadb_bind_int64(struct d_mariadb_statement* _statement,
                            size_t                      _index,
                            int64_t                     _value) D_NOTHROW;
int    d_mariadb_bind_double(struct d_mariadb_statement* _statement,
                             size_t                      _index,
                             double                      _value) D_NOTHROW;
int    d_mariadb_bind_text(struct d_mariadb_statement* _statement,
                           size_t                      _index,
                           const char*                 _text) D_NOTHROW;
int    d_mariadb_bind_blob(struct d_mariadb_statement* _statement,
                           size_t                      _index,
                           const void*                 _bytes,
                           size_t                      _size) D_NOTHROW;

// 2.2    Clearing
//------------------------------------------------------------------------------
// clearing -- every parameter NULL again: 0, or D_DB_INVALID_ARGUMENT for a
// NULL statement
int    d_mariadb_clear_bindings(struct d_mariadb_statement* _statement)
       D_NOTHROW;


//==============================================================================
// 3.  RUNNING
//==============================================================================


// 3.1    Rows changed
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement, discarding any rows it returns.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _changes    receives, on success, the rows it affected (an
 *                         UPDATE counts those it changed, not those it
 *                         matched), or the rows it returned; 0 for a
 *                         statement that counts none; may be `NULL`.
 * @return 0; the client's error number for the failure; CR_OUT_OF_MEMORY
 *         (2008); D_DB_INVALID_ARGUMENT for a `NULL` statement.
 * @post Every result the statement produced has been read; its values stay
 *       bound.
 */
int    d_mariadb_execute_update(struct d_mariadb_statement* _statement,
                                int64_t*                    _changes)
       D_NOTHROW;

// 3.2    Rows
//------------------------------------------------------------------------------
/**
 * @brief Runs a statement and returns every row it produced.
 *
 * @param[in]  _statement  the statement.
 * @param[out] _rows       receives the rows, typed as mariadb_query.h
 *                         describes; written only on success.
 * @return 0; the client's error number for the failure; CR_OUT_OF_MEMORY
 *         (2008) when the rows could not be built; D_DB_INVALID_ARGUMENT for
 *         a `NULL` argument.
 * @post Every result the statement produced has been read, the rows of the
 *       first kept; its values stay bound. On success the caller releases
 *       `_rows` with d_db_rows_free.
 */
int    d_mariadb_execute_query(struct d_mariadb_statement* _statement,
                               struct d_db_rows*           _rows) D_NOTHROW;


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_DB_MARIADB_MARIADB_STATEMENT_H
