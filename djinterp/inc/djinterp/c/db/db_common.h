/*******************************************************************************
* djinterp [c]                                                       db_common.h
*
* The vocabulary every djinterp database layer shares.
*   Each vendor's convenience layer (c/db/sqlite, and those to come) speaks in
* the terms defined here, so code that creates a table, lists what a database
* holds or frees what a call returned reads the same whatever is underneath:
*     results     0 is success; a failure the database reports is the
*                 vendor's own code, unchanged; the few conditions a vendor
*                 has no code for are the negative D_DB_* values below
*     flags       D_DB_IF_NOT_EXISTS / D_DB_IF_EXISTS make a create or a drop
*                 idempotent; a vendor's own flags start at 0x0100
*     columns     struct d_db_column describes a column once, for every vendor
*     lists       names and column descriptions come back in one allocation
*                 each, released by the layer that made them
*     rows        what a query returns comes back the same way, whatever the
*                 vendor: each value typed as the database gave it, each
*                 column described, all in one allocation
*     predicates  a yes-or-no question comes in two forms: `has_x` answers
*                 it, reading any failure as `false`; `has_x_s` returns 0 or
*                 the failure's code, and writes the answer through its last
*                 argument -- `false` whenever the call fails
*     throwing    no function of a layer throws, nor runs code of its
*                 caller's that could, and each is declared D_NOTHROW, so
*                 that C++ keeps no unwinding around a call
*   Nothing here touches a database, so this header and its source build
* without any vendor's client library. Rows need a 64-bit integer type: below
* the 64-bit floor -- ISO strict C++98 wherever `long` is 32 bits -- section
* 5 compiles to nothing, as the vendor layers do.
*
*
* path:      /inc/djinterp/c/db/db_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  RESULT CODES
    ------------
    1.  Status codes
         1.  d_db_status
    2.  Status messages
    3.  Waiting
         1.  d_db_socket
         2.  d_db_wait
         3.  d_db_pending
2.  FLAGS
    -----
    1.  Definition flags
         1.  D_DB_IF_NOT_EXISTS
         2.  D_DB_IF_EXISTS
         3.  D_DB_UNIQUE
         4.  D_DB_VENDOR_FLAGS
    2.  Column flags
         1.  D_DB_COLUMN_PRIMARY_KEY
         2.  D_DB_COLUMN_AUTO_INCREMENT
         3.  D_DB_COLUMN_NOT_NULL
         4.  D_DB_COLUMN_UNIQUE
3.  COLUMN DEFINITIONS
    ------------------
    1.  Types
         1.  d_db_type
         2.  d_db_column
    2.  Type names
4.  RESULTS
    -------
    1.  Result types
         1.  d_db_name_list
         2.  d_db_column_info
         3.  d_db_column_list
    2.  Release
5.  ROWS
    ----
    1.  Values
         1.  d_db_value_type
         2.  d_db_value
    2.  Rows
         1.  d_db_field
         2.  d_db_rows
    3.  Reading
    4.  Conversions
    5.  Release
*/

#ifndef DJINTERP_C_DB_DB_COMMON_H
#define DJINTERP_C_DB_DB_COMMON_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"  // framework root
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: section 5's floor;
                                              // int64_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  RESULT CODES
//==============================================================================
// A database layer returns 0 on success. When the database itself reports a
// failure, the layer returns the vendor's own code unchanged -- an SQLite
// result code, a MariaDB error number -- so that the vendor's error functions
// still describe it. The values below cover what a vendor has no code for, and
// are negative so that they can never collide with one.


// 1.1    Status codes
//------------------------------------------------------------------------------
// 1.1.1
// d_db_status
//   enum: the outcomes every database layer reports in the same way.
enum d_db_status
{
    D_DB_OK               =  0,  // success
    D_DB_UNSUPPORTED      = -1,  // the library or server in use cannot do
                                 // this
    D_DB_EXISTS           = -2,  // the thing to be created is already there
    D_DB_NOT_FOUND        = -3,  // the thing named is not there, or a query
                                 // that should have produced a row produced
                                 // none
    D_DB_INVALID_ARGUMENT = -4,  // an argument that cannot be right, from a
                                 // vendor with no code of its own for that
                                 // (SQLite has one: SQLITE_MISUSE)
    D_DB_MISMATCH         = -5,  // a value read as a type it neither is nor
                                 // converts to exactly
    D_DB_WAIT             = -6   // a call made without blocking has not
                                 // finished: wait as its d_db_pending says,
                                 // then continue it (1.3)
};

// 1.2    Status messages
//------------------------------------------------------------------------------
// status messages -- an English description of a d_db_status value, or NULL
// for any other value, which a vendor layer then describes itself; never to
// be freed
const char* d_db_status_message(int _status) D_NOTHROW;

// 1.3    Waiting
//------------------------------------------------------------------------------
// A call a layer makes without blocking (the audit's 11.26.6) starts, and
// returns D_DB_WAIT until it is done, each time saying what it waits for: a
// socket to become readable or writable, a deadline, or both. The program
// waits so, in whatever loop it has, and continues the call with what came.

// 1.3.1
// d_db_socket
//   type: a client's socket, wide enough for POSIX's int and Windows' SOCKET
// alike; D_DB_SOCKET_NONE for none.
typedef size_t d_db_socket;

// D_DB_SOCKET_NONE
//   constant: no socket.
#define D_DB_SOCKET_NONE ((d_db_socket)-1)

// 1.3.2
// d_db_wait
//   enum: what a call waits for, as a mask; and, given back, what came.
enum d_db_wait
{
    D_DB_WAIT_READ    = 1,  // the socket readable
    D_DB_WAIT_WRITE   = 2,  // the socket writable
    D_DB_WAIT_TIMEOUT = 4   // a deadline the client keeps: continue the call
                            // when it passes, ready or not
};

// 1.3.3
// d_db_pending
//   struct: what a call that has not finished waits for.
struct d_db_pending
{
    d_db_socket socket;      // the socket to wait on
    unsigned    events;      // d_db_wait, ORed
    unsigned    timeout_ms;  // with D_DB_WAIT_TIMEOUT: the deadline, from now
};


//==============================================================================
// 2.  FLAGS
//==============================================================================
// Flags are unsigned bit constants rather than enumerators: C++20 deprecates
// combining two different enumeration types with `|`, and a call routinely
// mixes the shared flags with a vendor's own.


// 2.1    Definition flags
//------------------------------------------------------------------------------
// 2.1.1
// D_DB_IF_NOT_EXISTS
//   constant: a create succeeds without effect when the object already exists.
#define D_DB_IF_NOT_EXISTS          0x0001u

// 2.1.2
// D_DB_IF_EXISTS
//   constant: a drop succeeds without effect when the object does not exist.
#define D_DB_IF_EXISTS              0x0002u

// 2.1.3
// D_DB_UNIQUE
//   constant: an index allows no two rows the same key.
#define D_DB_UNIQUE                 0x0004u

// 2.1.4
// D_DB_VENDOR_FLAGS
//   constant: the first bit a vendor may assign to flags of its own; every bit
// below it belongs to this header.
#define D_DB_VENDOR_FLAGS           0x0100u

// 2.2    Column flags
//------------------------------------------------------------------------------
// 2.2.1
// D_DB_COLUMN_PRIMARY_KEY
//   constant: the column is (part of) the primary key. Set on several columns,
// the key is composite.
#define D_DB_COLUMN_PRIMARY_KEY     0x0001u

// 2.2.2
// D_DB_COLUMN_AUTO_INCREMENT
//   constant: the column draws a new, never reused value for each row. Valid
// only on the sole primary-key column, of an integer type.
#define D_DB_COLUMN_AUTO_INCREMENT  0x0002u

// 2.2.3
// D_DB_COLUMN_NOT_NULL
//   constant: the column refuses NULL.
#define D_DB_COLUMN_NOT_NULL        0x0004u

// 2.2.4
// D_DB_COLUMN_UNIQUE
//   constant: no two rows may hold the same value in the column.
#define D_DB_COLUMN_UNIQUE          0x0008u


//==============================================================================
// 3.  COLUMN DEFINITIONS
//==============================================================================
// A column described once creates the same table on every vendor: each layer
// maps d_db_type to its own spelling, and documents the mapping. Where a
// vendor's own type is wanted, `type_sql` names it and the mapping is skipped.


// 3.1    Types
//------------------------------------------------------------------------------
// 3.1.1
// d_db_type
//   enum: what a column holds, independent of any vendor. It mirrors the C++
// layer's field_type, member for member, from BOOLEAN to UUID.
enum d_db_type
{
    D_DB_TYPE_NONE = 0,        // no declared type (SQLite only), or type_sql
    D_DB_TYPE_BOOLEAN,         // true or false
    D_DB_TYPE_INTEGER,         // 32-bit signed integer
    D_DB_TYPE_BIG_INTEGER,     // 64-bit signed integer
    D_DB_TYPE_FLOATING_POINT,  // double-precision floating point
    D_DB_TYPE_DECIMAL,         // exact decimal; size and scale apply
    D_DB_TYPE_STRING,          // text; size is a maximum length where enforced
    D_DB_TYPE_BINARY,          // bytes; size is a maximum length where enforced
    D_DB_TYPE_DATE,            // calendar date
    D_DB_TYPE_TIME,            // time of day
    D_DB_TYPE_DATETIME,        // date and time, without a time zone
    D_DB_TYPE_TIMESTAMP,       // a point in time
    D_DB_TYPE_JSON,            // a JSON document
    D_DB_TYPE_XML,             // an XML document
    D_DB_TYPE_UUID             // a universally unique identifier
};

// 3.1.2
// d_db_column
//   struct: one column of a table to create or extend. Members are in the order
// a positional initializer fills them, so C99, C++98 and later all write
//     { "id", D_DB_TYPE_BIG_INTEGER, D_DB_COLUMN_PRIMARY_KEY }
// and the members left out are zero. GCC's and Clang's -Wextra warn of the
// members such a list leaves out; listing them all keeps it quiet.
//   A composite primary key takes its columns in the order their key_place
// numbers them, 1 to the key's count, each once; where every key_place is 0,
// in the columns' order. A key_place on a column outside the key, or places
// that skip or repeat a number, are the caller's error.
struct d_db_column
{
    const char*    name;         // the column's name; the layer quotes it
    enum d_db_type type;         // what it holds
    unsigned       flags;        // D_DB_COLUMN_* bits
    const char*    default_sql;  // the DEFAULT expression, as SQL; NULL: none
    unsigned       size;         // length, or DECIMAL precision; 0: default
    unsigned       scale;        // DECIMAL scale
    const char*    type_sql;     // the vendor's own type; overrides `type`
    unsigned       key_place;    // its place in the primary key, from 1;
                                 // 0: the columns' order
};

// 3.2    Type names
//------------------------------------------------------------------------------
// type names -- never NULL, never allocate; "unknown" for a value outside
// enum d_db_type
const char* d_db_type_name(enum d_db_type _type) D_NOTHROW;


//==============================================================================
// 4.  RESULTS
//==============================================================================
// A list a layer returns is one allocation: the array, and the strings it
// points into, together. Release it with the matching function below and with
// nothing else -- on Windows the caller's free() may belong to another C
// runtime than the layer's.


// 4.1    Result types
//------------------------------------------------------------------------------
// 4.1.1
// d_db_name_list
//   struct: names, in the order the layer documents. On success `names` is
// never NULL, and `names[count]` is NULL, so either bound ends a loop.
struct d_db_name_list
{
    size_t count;  // the number of names
    char** names;  // the names, then a NULL
};

// 4.1.2
// d_db_column_info
//   struct: one column of an existing table, as the database describes it. A
// generated column is one the database computes from the row's others
// (GENERATED ALWAYS AS, a virtual or stored column): it takes no value an
// INSERT or an UPDATE gives it.
struct d_db_column_info
{
    const char* name;         // the column's name
    const char* type;         // its declared type as written; "" for none
    const char* default_sql;  // its DEFAULT expression as SQL; NULL: none
    int         primary_key;  // 0, or its 1-based place in the primary key
    bool        not_null;     // true when the column refuses NULL
    bool        generated;    // true when the database computes it
};

// 4.1.3
// d_db_column_list
//   struct: the columns of a table, in the table's order. `columns` is NULL
// when `count` is 0.
struct d_db_column_list
{
    size_t                   count;    // the number of columns
    struct d_db_column_info* columns;  // the columns
};

// 4.2    Release
//------------------------------------------------------------------------------
/**
 * @brief Releases a name list a database layer filled in.
 *
 * @param[in,out] _list  the list; may be `NULL`, or empty.
 * @post `_list` is empty -- `count` 0 and `names` `NULL` -- and every string
 *       it held is invalid.
 */
void d_db_name_list_free(struct d_db_name_list* _list) D_NOTHROW;
/**
 * @brief Releases a column list a database layer filled in.
 *
 * @param[in,out] _list  the list; may be `NULL`, or empty.
 * @post `_list` is empty, and every string it held is invalid.
 */
void d_db_column_list_free(struct d_db_column_list* _list) D_NOTHROW;
/**
 * @brief Releases a string or block a database layer returned on its own,
 *        such as a quoted identifier or a query's text result.
 *
 * @param[in] _block  the block; may be `NULL`.
 * @post `_block` is invalid.
 */
void d_db_free(void* _block) D_NOTHROW;


//==============================================================================
// 5.  ROWS
//==============================================================================
// The rows a query returned come back as one allocation, as the lists do:
// the descriptions of the columns, the values, and the text and bytes they
// hold, released together by d_db_rows_free. A value keeps the type the
// database gave it -- an integer, a real number, text, bytes or NULL -- and a
// column the type its definition gave, named in the vocabulary of section 3
// where the vendor's type maps to one. Each vendor layer documents how its
// types arrive. The conversions convert only where nothing is lost.

// 64-bit floor: a value holds a 64-bit integer, which dstdint.h declares only
// where the build can spell one. Below it -- ISO strict C++98 wherever int64_t
// is `long long`: a 32-bit target, 64-bit Windows or macOS -- this section
// compiles to nothing.
#if defined(INT64_MAX)


// 5.1    Values
//------------------------------------------------------------------------------
// 5.1.1
// d_db_value_type
//   enum: what a value is, whatever the vendor.
enum d_db_value_type
{
    D_DB_VALUE_NULL = 0,  // SQL NULL
    D_DB_VALUE_INTEGER,   // a 64-bit signed integer; a boolean is 0 or 1
    D_DB_VALUE_REAL,      // a double
    D_DB_VALUE_TEXT,      // text, in the connection's character set
    D_DB_VALUE_BLOB       // bytes
};

// 5.1.2
// d_db_value
//   struct: one value of one row. Text and bytes are followed by a NUL that
// `size` does not count, so text reads as a C string; bytes may hold NULs of
// their own. An empty text or BLOB points at its terminator, never NULL.
struct d_db_value
{
    enum d_db_value_type type;  // which member of `as` holds the value
    size_t               size;  // the bytes of a TEXT or BLOB; 0 otherwise
    union
    {
        int64_t     integer;    // D_DB_VALUE_INTEGER
        double      real;       // D_DB_VALUE_REAL
        const char* text;       // D_DB_VALUE_TEXT
        const void* blob;       // D_DB_VALUE_BLOB
    } as;
};

// 5.2    Rows
//------------------------------------------------------------------------------
// 5.2.1
// d_db_field
//   struct: one column of a query's rows.
struct d_db_field
{
    const char*    name;       // as the query names it: the column's name,
                               // an alias, or the expression as written
    enum d_db_type type;       // its type in section 3's vocabulary;
                               // D_DB_TYPE_NONE where the vendor gives none
                               // or one that maps to none
    const char*    type_name;  // the vendor's own name for its type, as each
                               // layer documents; "" for none, never NULL
};

// 5.2.2
// d_db_rows
//   struct: the rows a query returned. `values` holds `row_count` rows of
// `column_count` values each, row by row. `columns` is NULL when there are no
// columns, and `values` when there are no values. All zero is empty.
struct d_db_rows
{
    size_t             column_count;  // the values in each row
    size_t             row_count;     // the rows
    struct d_db_field* columns;       // column_count descriptions
    struct d_db_value* values;        // row_count * column_count values
};

// 5.3    Reading
//------------------------------------------------------------------------------
/**
 * @brief Finds the value at a row and a column.
 *
 * @param[in] _rows    the rows; may be `NULL`.
 * @param[in] _row     the row, from 0.
 * @param[in] _column  the column, from 0.
 * @return the value, valid until the rows are released; `NULL` when `_rows`
 *         is `NULL` or either index is out of range.
 */
const struct d_db_value*
    d_db_rows_value(const struct d_db_rows* _rows,
                    size_t                  _row,
                    size_t                  _column) D_NOTHROW;
/**
 * @brief Finds a column by its name: the first named exactly so, or failing
 *        that the first whose name matches ignoring ASCII case, as SQL
 *        compares a name it was given unquoted.
 *
 * @param[in]  _rows   the rows.
 * @param[in]  _name   the name.
 * @param[out] _index  receives the column's index, from 0; written only on
 *                     success.
 * @return 0; D_DB_NOT_FOUND when no column matches; D_DB_INVALID_ARGUMENT
 *         for a `NULL` argument.
 */
int d_db_rows_column_index(const struct d_db_rows* _rows,
                           const char*             _name,
                           size_t*                 _index) D_NOTHROW;

// 5.4    Conversions
//------------------------------------------------------------------------------
// conversions -- each writes a value as one type, and returns 0, where the
// value is of that type or converts to it exactly:
//     _int64   an INTEGER; a REAL with no fraction that int64_t holds; text
//              that is an integer -- an optional sign and digits -- that
//              int64_t holds
//     _double  a REAL; an INTEGER that a double holds exactly; text that is
//              a number, rounded to the nearest double, its point a `.`
//              whatever the C locale says
//     _bool    an INTEGER, 0 false and any other true; text PostgreSQL reads
//              as a boolean, in any ASCII case: t, true, y, yes, on and 1,
//              or f, false, n, no, off and 0
// Anything else -- NULL among it -- is D_DB_MISMATCH, and writes nothing. A
// `NULL` argument is D_DB_INVALID_ARGUMENT.
int d_db_value_int64(const struct d_db_value* _value,
                     int64_t*                 _out) D_NOTHROW;
int d_db_value_double(const struct d_db_value* _value,
                      double*                  _out) D_NOTHROW;
int d_db_value_bool(const struct d_db_value* _value,
                    bool*                    _out) D_NOTHROW;

// 5.5    Release
//------------------------------------------------------------------------------
/**
 * @brief Releases the rows a database layer filled in.
 *
 * @param[in,out] _rows  the rows; may be `NULL`, or empty.
 * @post `_rows` is empty -- all zero -- and every value, name and string it
 *       held is invalid.
 */
void d_db_rows_free(struct d_db_rows* _rows) D_NOTHROW;


#endif  // defined(INT64_MAX)


D_EXTERN_C_END


#endif  // DJINTERP_C_DB_DB_COMMON_H
