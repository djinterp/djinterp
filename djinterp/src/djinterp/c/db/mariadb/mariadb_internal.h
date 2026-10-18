/*******************************************************************************
* djinterp [c]                                                mariadb_internal.h
*
* Private to the MariaDB layer's sources: mysql.h, the client floor, the
* handle's definition, and the plumbing every module repeats -- when the layer
* is built over a client. Built as stubs (D_INTERNAL_MARIADB 0), it declares
* nothing, and env includes no mysql.h.
*   mysql.h arrives through env_mariadb.h, which honours
* D_CFG_ENV_MARIADB_C_PATH and finds Connector/C's header or
* libmysqlclient's; cfg_mariadb_common.h has already turned
* D_CFG_ENV_USING_MARIADB on, for a source that defined
* D_INTERNAL_MARIADB_SOURCE before its first include. The layer reads only
* what the two clients
* share, and the floor is checked here once, so a source compiled against an
* older client stops with one clear message.
*   The plumbing keeps one rule the public contract depends on: after a
* statement fails, nothing else runs on the connection before control returns,
* so mysql_error still describes the failure. A helper that runs more than one
* statement stops at the first that fails.
*   Names reach SQL in backquotes, any backquote doubled; values are escaped
* by the client, for the connection's character set and SQL mode; SQL the
* caller wrote is used as written. What a database holds is read from
* information_schema, whose tables both servers have kept the same since
* MySQL 5.7 and MariaDB 10.3.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  REQUIREMENTS
    ------------
    1.  Client floor
    2.  Compile-time API choices
         1.  D_INTERNAL_MARIADB_ESCAPE_QUOTE
         2.  D_INTERNAL_MARIADB_LENGTH_MAX
         3.  d_internal_mariadb_flag
         4.  D_INTERNAL_MARIADB_SSL_SWITCHES
2.  HANDLES
    -------
    1.  The handle
         1.  d_mariadb
    2.  Result codes
         1.  D_INTERNAL_MARIADB_UNKNOWN_ERROR
         2.  D_INTERNAL_MARIADB_OUT_OF_MEMORY
         3.  D_INTERNAL_MARIADB_INVALID_PARAMETER
         4.  D_INTERNAL_MARIADB_SSL_ERROR
    3.  Server facts
3.  NAMES
    -----
    1.  Checks
    2.  Quoting
4.  STATEMENTS
    ----------
    1.  Running
    2.  Single values
    3.  Rows
5.  CATALOGUE
    ---------
    1.  Objects
         1.  d_internal_mariadb_object
         2.  d_internal_mariadb_path
    2.  Lookups
*/

#ifndef DJINTERP_C_DB_MARIADB_MARIADB_INTERNAL_H
#define DJINTERP_C_DB_MARIADB_MARIADB_INTERNAL_H 1

// std
#include <limits.h>  // ULONG_MAX
#include <stdarg.h>  // va_list
#include <stddef.h>  // size_t
// djinterp
#include "../../../../../inc/djinterp/c/djinterp.h"  // framework root
#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_common.h"  // layer
#include "../../../../../inc/djinterp/env/db/mariadb/env_mariadb.h"  // mysql.h
#include "../db_internal.h"  // d_internal_db_buffer, the builders
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MARIADB == 1)


//==============================================================================
// 1.  REQUIREMENTS
//==============================================================================


// 1.1    Client floor
//------------------------------------------------------------------------------
#if !D_CFG_IS_ON(D_CFG_ENV_USING_MARIADB)
    #error "D_CFG_ENV_USING_MARIADB is off: a layer source defines "           \
           "D_INTERNAL_MARIADB_SOURCE before its first include"
#elif !D_ENV_MYSQL_COMMON_FAMILY_DETECTED
    #error "the MariaDB layer needs mysql.h; see D_CFG_ENV_MARIADB_C_PATH"
#elif (MYSQL_VERSION_ID < D_MARIADB_FLOOR_CLIENT_VERSION_ID)
    #error "the MariaDB layer needs libmysqlclient 5.7, or a MariaDB client"
#endif

// 1.2    Compile-time API choices
//------------------------------------------------------------------------------
// 1.2.1
// D_INTERNAL_MARIADB_ESCAPE_QUOTE
//   brief: 1 where values are escaped with mysql_real_escape_string_quote --
// libmysqlclient from 5.7.6, whose plain mysql_real_escape_string fails
// outright under the NO_BACKSLASH_ESCAPES SQL mode -- and 0 where they are
// escaped with the plain call, which MariaDB's clients adapt to that mode
// themselves. Chosen here, so that no function body needs a conditional.
#if ( (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) &&                                  \
      (MYSQL_VERSION_ID >= 50706) )
    #define D_INTERNAL_MARIADB_ESCAPE_QUOTE 1
#else
    #define D_INTERNAL_MARIADB_ESCAPE_QUOTE 0
#endif

// 1.2.2
// D_INTERNAL_MARIADB_LENGTH_MAX
//   constant: the longest text the client's unsigned long lengths can carry,
// as a size_t: 4 GiB less a byte where unsigned long is 32 bits, as on 64-bit
// Windows, and any length elsewhere.
#if (ULONG_MAX < SIZE_MAX)
    #define D_INTERNAL_MARIADB_LENGTH_MAX ((size_t)ULONG_MAX)
#else
    #define D_INTERNAL_MARIADB_LENGTH_MAX SIZE_MAX
#endif

// 1.2.3
// d_internal_mariadb_flag
//   type: the client's one-byte truth value, as MYSQL_BIND points at it and
// mysql_stmt_attr_set reads it: bool in libmysqlclient from 8.0, which
// dropped the name my_bool, and my_bool, a char, in every other client.
#if ( (D_ENV_MYSQL_COMMON_IS_ORACLE_MYSQL) &&                                  \
      (MYSQL_VERSION_ID >= 80000) )
    typedef bool    d_internal_mariadb_flag;
#else
    typedef my_bool d_internal_mariadb_flag;
#endif

// 1.2.4
// D_INTERNAL_MARIADB_SSL_SWITCHES
//   brief: how the client is told what TLS to insist on: 1 for
// MYSQL_OPT_SSL_MODE, libmysqlclient's from 5.7.11; 2 for
// MYSQL_OPT_SSL_ENFORCE and MYSQL_OPT_SSL_VERIFY_SERVER_CERT, Connector/C's;
// 0 for neither, an older libmysqlclient, with which only the client's
// default can be had.
#if (D_ENV_MYSQL_COMMON_IS_MARIADB)
    #define D_INTERNAL_MARIADB_SSL_SWITCHES 2
#elif (MYSQL_VERSION_ID >= 50711)
    #define D_INTERNAL_MARIADB_SSL_SWITCHES 1
#else
    #define D_INTERNAL_MARIADB_SSL_SWITCHES 0
#endif


//==============================================================================
// 2.  HANDLES
//==============================================================================


// 2.1    The handle
//------------------------------------------------------------------------------
// 2.1.1
// d_mariadb
//   struct: the definition mariadb_common.h declares: the client's
// connection, and what the layer has learned of the server it reached.
struct d_mariadb
{
    MYSQL*                native;            // the client's connection
    bool                  owned;             // closed along with the handle
    bool                  multi_statements;  // left on by a script that
                                             // failed, so that its error
                                             // still describes it
    enum d_mariadb_flavor flavor;            // the server's, by its version
                                             // string
    unsigned long         version;           // the server's true version,
                                             // MAJOR*10000 + MINOR*100 +
                                             // PATCH
    int                   case_rule;         // lower_case_table_names; -1
                                             // until it is first needed
    MYSQL_STMT*           failed;            // a statement whose prepare
                                             // failed, closed when the
                                             // layer next runs a statement:
                                             // closing it clears the error
                                             // the caller is about to read
};

// 2.2    Result codes
//------------------------------------------------------------------------------
// The client's own numbers for conditions the layer can meet itself
// (errmsg.h, which mysql.h does not include, names them CR_UNKNOWN_ERROR and
// CR_OUT_OF_MEMORY). Returned by the layer, they leave the client's error
// state as it was.

// 2.2.1
// D_INTERNAL_MARIADB_UNKNOWN_ERROR
//   constant: a failure the server reported in a result row rather than with
// a number of its own.
#define D_INTERNAL_MARIADB_UNKNOWN_ERROR 2000

// 2.2.2
// D_INTERNAL_MARIADB_OUT_OF_MEMORY
//   constant: memory ran out in the layer.
#define D_INTERNAL_MARIADB_OUT_OF_MEMORY 2008

// 2.2.3
// D_INTERNAL_MARIADB_INVALID_PARAMETER
//   constant: a parameter number the statement does not have
// (CR_INVALID_PARAMETER_NO).
#define D_INTERNAL_MARIADB_INVALID_PARAMETER 2034

// 2.2.4
// D_INTERNAL_MARIADB_SSL_ERROR
//   constant: a TLS connection that could not be made, though one was
// required (CR_SSL_CONNECTION_ERROR).
#define D_INTERNAL_MARIADB_SSL_ERROR 2026

// 2.3    Server facts
//------------------------------------------------------------------------------
// server facts -- _at_least answers whether the server is of the flavor
// named, at the version given (MAJOR*10000 + MINOR*100 + PATCH) or later;
// _case_rule reads the server's lower_case_table_names once, and the copy the
// handle keeps after that: 0 and the rule, or the client's error number for
// the query that failed
bool d_internal_mariadb_at_least(const struct d_mariadb* _db,
                                 enum d_mariadb_flavor   _flavor,
                                 unsigned long           _version);
int  d_internal_mariadb_case_rule(struct d_mariadb* _db,
                                  int*              _rule);


//==============================================================================
// 3.  NAMES
//==============================================================================


// 3.1    Checks
//------------------------------------------------------------------------------
// checks -- none of these allocate or fail. _name_ok is false for a NULL or
// empty name; _schema_ok for an empty one, NULL meaning the current database;
// _variable_ok for anything but a letter or underscore followed by letters,
// digits and underscores, the shape of every system variable's name
bool d_internal_mariadb_name_ok(const char* _name);
bool d_internal_mariadb_schema_ok(const char* _schema);
bool d_internal_mariadb_variable_ok(const char* _name);

// 3.2    Quoting
//------------------------------------------------------------------------------
// quoting -- d_internal_db_quote_fn's for the servers: _quote_name writes a
// name in backquotes, any backquote doubled, and needs no connection;
// _quote_value writes a value in single quotes, escaped by the client for the
// connection `_context` points to. Each fails only for want of memory,
// leaving the buffer's length as it was. _quoting gathers both, for
// d_internal_db_format.
bool d_internal_mariadb_quote_name(void*                        _context,
                                   struct d_internal_db_buffer* _out,
                                   const char*                  _name);
bool d_internal_mariadb_quote_value(void*                        _context,
                                    struct d_internal_db_buffer* _out,
                                    const char*                  _text);
struct d_internal_db_quoting
     d_internal_mariadb_quoting(struct d_mariadb* _db);


//==============================================================================
// 4.  STATEMENTS
//==============================================================================
// Formats are d_internal_db_format's: %Q a quoted value, %I a quoted name,
// %T a qualified one, %s text as written. Each returns 0, the client's error
// number for the statement that failed, D_INTERNAL_MARIADB_OUT_OF_MEMORY when
// the text could not be built, or D_DB_INVALID_ARGUMENT for a format it could
// not read, or a value it could not write (NaN, say).


// 4.1    Running
//------------------------------------------------------------------------------
// the layer's result for how formatting ended: 0, or
// D_INTERNAL_MARIADB_OUT_OF_MEMORY, or D_DB_INVALID_ARGUMENT for a format or
// value the formatter would not write
int  d_internal_mariadb_format_result(enum d_internal_db_format_status _status);
/**
 * @brief Readies the connection for the layer's next statement, undoing what
 *        a failure left so that its error would still read: closes a
 *        statement whose prepare failed, and turns off the multi-statement
 *        mode a failed script left on.
 *
 * @param[in] _db  the connection.
 * @return 0, or the client's error number when the mode could not be turned
 *         off.
 */
int  d_internal_mariadb_ready(struct d_mariadb* _db);
/**
 * @brief Sends SQL as written and reads every result it produces.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _sql     the SQL; several statements only while the client's
 *                      multi-statement mode is on.
 * @param[in]  _length  its length in bytes.
 * @param[out] _rows    receives the first result's rows, or NULL for a
 *                      statement that returned none; `NULL` to discard them.
 * @return 0, or the client's error number for the statement that failed.
 * @post On success with `_rows`, the caller frees `*_rows` with
 *       mysql_free_result.
 *
 * Every result after the first, which only a CALL or a script produces, is
 * read and discarded, so the connection is ready for the next statement.
 */
int  d_internal_mariadb_send(struct d_mariadb* _db,
                             const char*       _sql,
                             size_t            _length,
                             MYSQL_RES**       _rows);
/**
 * @brief Formats a statement and sends it, as d_internal_mariadb_send.
 *
 * @param[in]  _db      the connection.
 * @param[out] _rows    as for d_internal_mariadb_send.
 * @param[in]  _format  the statement, as a d_internal_db_format format.
 * @param[in]  _args    the format's arguments.
 * @return as section 4 says.
 */
int  d_internal_mariadb_vrun(struct d_mariadb* _db,
                             MYSQL_RES**       _rows,
                             const char*       _format,
                             va_list           _args);
// as d_internal_mariadb_vrun, taking the arguments directly
int  d_internal_mariadb_run(struct d_mariadb* _db,
                            MYSQL_RES**       _rows,
                            const char*       _format,
                            ...);

// 4.2    Single values
//------------------------------------------------------------------------------
// single values -- each runs one statement and reads the first column of its
// first row. A row yields 0 and the value, written only then; no row, or no
// result at all, is D_DB_NOT_FOUND; a failure is as section 4 says. A NULL
// reads as a NULL string, or as 0. _vquery's string is the caller's, released
// with d_db_free. _int64 reads an integer exactly, and a number with a
// fraction truncated toward zero; _double rounds to the nearest double;
// either reads text that is not a number, or not one the type can hold, as
// D_DB_INVALID_ARGUMENT.
int  d_internal_mariadb_vquery(struct d_mariadb* _db,
                               char**            _value,
                               const char*       _format,
                               va_list           _args);
int  d_internal_mariadb_query(struct d_mariadb* _db,
                              char**            _value,
                              const char*       _format,
                              ...);
int  d_internal_mariadb_vquery_int64(struct d_mariadb* _db,
                                     int64_t*          _value,
                                     const char*       _format,
                                     va_list           _args);
int  d_internal_mariadb_query_int64(struct d_mariadb* _db,
                                    int64_t*          _value,
                                    const char*       _format,
                                    ...);
int  d_internal_mariadb_vquery_double(struct d_mariadb* _db,
                                      double*           _value,
                                      const char*       _format,
                                      va_list           _args);

// 4.3    Rows
//------------------------------------------------------------------------------
// rows -- a column's type and its type's name, as mariadb_query.h tabulates
// them; _rows_columns describes each field of a result to rows under
// construction; _rows_value adds one value, as the protocol sent it in text,
// read as its field's type says: `_text` NULL for SQL NULL, and `_length` its
// bytes. Each of the last two returns `true`, or `false` when memory ran out.
enum d_db_type
     d_internal_mariadb_field_type(const MYSQL_FIELD* _field);
const char*
     d_internal_mariadb_field_type_name(const MYSQL_FIELD* _field);
bool d_internal_mariadb_rows_columns(struct d_internal_db_rows* _rows,
                                     const MYSQL_FIELD*         _fields,
                                     unsigned                   _count);
bool d_internal_mariadb_rows_value(struct d_internal_db_rows* _rows,
                                   const MYSQL_FIELD*         _field,
                                   const char*                _text,
                                   unsigned long              _length);
/**
 * @brief Reads every row of a result into rows, and frees the result.
 *
 * @param[in]  _result  the result, from mysql_store_result; `NULL` for a
 *                      statement that returned none, which gives empty rows.
 * @param[out] _rows    receives the rows; written only on success.
 * @return 0, or D_INTERNAL_MARIADB_OUT_OF_MEMORY when the rows could not be
 *         built.
 * @post `_result` is freed whatever happens. On success the caller releases
 *       `_rows` with d_db_rows_free.
 */
int  d_internal_mariadb_collect_rows(MYSQL_RES*        _result,
                                     struct d_db_rows* _rows);


//==============================================================================
// 5.  CATALOGUE
//==============================================================================
// What a database holds, read from information_schema. A server matches some
// names by case and others not -- databases, tables and views as its
// lower_case_table_names says, triggers so too on MariaDB and MySQL 5.7 but
// never on MySQL 8, columns and indexes never -- while information_schema
// compares them all without case on MariaDB. Where the server matches by
// case, the lookup compares the names' bytes too, beside the comparison an
// information_schema lookup is optimized for.


// 5.1    Objects
//------------------------------------------------------------------------------
// 5.1.1
// d_internal_mariadb_object
//   enum: what a lookup looks for.
enum d_internal_mariadb_object
{
    D_INTERNAL_MARIADB_DATABASE = 0,  // a database
    D_INTERNAL_MARIADB_TABLE,         // a base table, system-versioned or not
    D_INTERNAL_MARIADB_VIEW,          // a view
    D_INTERNAL_MARIADB_RELATION,      // a table or a view: whatever holds a
                                      // name CREATE VIEW would collide with
    D_INTERNAL_MARIADB_COLUMN,        // a table's column
    D_INTERNAL_MARIADB_INDEX,         // a table's index, its primary key the
                                      // one named PRIMARY
    D_INTERNAL_MARIADB_TRIGGER        // a trigger
};

// 5.1.2
// d_internal_mariadb_path
//   struct: where an object is.
struct d_internal_mariadb_path
{
    const char* schema;  // its database; NULL: the current one
    const char* table;   // the table it belongs to: required for a column or
                         // an index, optional for a trigger, NULL otherwise
    const char* name;    // its name; NULL when listing
};

// 5.2    Lookups
//------------------------------------------------------------------------------
/**
 * @brief Answers whether an object exists.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to look for.
 * @param[in]  _path    where, and its name.
 * @param[out] _exists  receives the answer; `false` on any failure.
 * @return 0, the client's error number for a failure, or
 *         D_DB_INVALID_ARGUMENT for a `NULL` argument, a name that is `NULL`
 *         or empty, or a table missing where the object needs one.
 */
int  d_internal_mariadb_has(struct d_mariadb*                     _db,
                            enum d_internal_mariadb_object        _object,
                            const struct d_internal_mariadb_path* _path,
                            bool*                                 _exists);
/**
 * @brief Lists the objects of a kind, sorted by their names' bytes.
 *
 * @param[in]  _db      the connection.
 * @param[in]  _object  what to list; an index list leaves PRIMARY out.
 * @param[in]  _path    where; its name is not read.
 * @param[out] _names   receives the names; written only on success.
 * @return 0, the client's error number for a failure,
 *         D_INTERNAL_MARIADB_OUT_OF_MEMORY, or D_DB_INVALID_ARGUMENT as for
 *         d_internal_mariadb_has.
 * @post On success the caller releases `_names` with d_db_name_list_free.
 */
int  d_internal_mariadb_list(struct d_mariadb*                     _db,
                             enum d_internal_mariadb_object        _object,
                             const struct d_internal_mariadb_path* _path,
                             struct d_db_name_list*                _names);
/**
 * @brief Appends a condition matching a column to a database's or table's
 *        name, as the server matches it.
 *
 * @param[in]     _db      the connection, whose quoting applies.
 * @param[in,out] _sql     the statement so far.
 * @param[in]     _column  the column, as SQL.
 * @param[in]     _name    the name; `NULL` for the current database.
 * @param[in]     _exact   whether the server matches it by case.
 * @return as d_internal_db_format.
 *
 * Writes ` AND column = name`, and with `_exact` also
 * ` AND HEX(column) = HEX(name)`.
 */
enum d_internal_db_format_status
     d_internal_mariadb_match(struct d_mariadb*            _db,
                              struct d_internal_db_buffer* _sql,
                              const char*                  _column,
                              const char*                  _name,
                              bool                         _exact);


#endif  // D_INTERNAL_MARIADB


#endif  // DJINTERP_C_DB_MARIADB_MARIADB_INTERNAL_H
