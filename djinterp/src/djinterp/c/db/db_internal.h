/*******************************************************************************
* djinterp [c]                                                     db_internal.h
*
* Private to the database layers' sources: how a result list is built.
*   A vendor layer reads names, column descriptions or rows one row at a
* time, not knowing how many will come. It adds each to a builder, which keeps
* the text in one growing buffer and the records in another, and finishing
* packs both into the single allocation db_common.h promises its callers --
* the arrays first, the strings after them -- so that one free releases
* everything.
*   Offsets, not pointers, are kept while building: the text buffer moves each
* time it grows, and an offset survives that where a pointer would not.
*   It also formats SQL for the layers whose client has no formatter of its
* own -- a printf-like format, with conversions for a quoted value and a
* quoted name, each quoted by the vendor's own rules -- and reads the numbers
* a server sends as text. Both write and read the decimal point as SQL spells
* it, whatever the C locale says.
*
*
* path:      /src/djinterp/c/db/db_internal.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BUFFERS
    -------
    1.  Buffer type
         1.  d_internal_db_buffer
         2.  D_INTERNAL_DB_NO_TEXT
    2.  Buffer operations
2.  BUILDERS
    --------
    1.  Name lists
         1.  d_internal_db_names
    2.  Column lists
         1.  d_internal_db_columns
    3.  Rows
         1.  d_internal_db_rows
3.  STATEMENT TEXT
    --------------
    1.  Quoting
         1.  d_internal_db_quote_fn
         2.  d_internal_db_quoting
    2.  Formatting
         1.  d_internal_db_format_status
4.  VALUES
    ------
    1.  Numbers
*/

#ifndef DJINTERP_C_DB_DB_INTERNAL_H
#define DJINTERP_C_DB_DB_INTERNAL_H 1

// std
#include <stdarg.h>  // va_list
#include <stddef.h>  // size_t
// djinterp
#include "../../../../inc/djinterp/c/djinterp.h"      // framework root
#include "../../../../inc/djinterp/c/db/db_common.h"  // the list types
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


//==============================================================================
// 1.  BUFFERS
//==============================================================================


// 1.1    Buffer type
//------------------------------------------------------------------------------
// 1.1.1
// d_internal_db_buffer
//   struct: a run of bytes that grows as it is appended to. All zero is empty.
struct d_internal_db_buffer
{
    char*  data;      // the bytes, or NULL before the first append
    size_t length;    // bytes in use
    size_t capacity;  // bytes allocated
};

// 1.1.2
// D_INTERNAL_DB_NO_TEXT
//   constant: the offset recorded for a NULL string, which no real offset can
// equal.
#define D_INTERNAL_DB_NO_TEXT ((size_t)-1)

// 1.2    Buffer operations
//------------------------------------------------------------------------------
/**
 * @brief Makes room for more bytes, growing the buffer if they would not
 *        fit, without changing its contents or length.
 *
 * @param[in,out] _buffer  the buffer.
 * @param[in]     _count   how many bytes, past its length, to make room for.
 * @return `true`, or `false` when memory ran out, the buffer unchanged.
 * @post On success `_count` bytes from `data + length` may be written, and
 *       kept by raising `length`.
 */
bool   d_internal_db_buffer_reserve(struct d_internal_db_buffer* _buffer,
                                    size_t                       _count);
/**
 * @brief Appends bytes to a buffer, growing it as needed.
 *
 * @param[in,out] _buffer  the buffer.
 * @param[in]     _bytes   the bytes to append; may be `NULL` when `_count`
 *                         is 0.
 * @param[in]     _count   how many bytes.
 * @return `true`, or `false` when memory ran out, the buffer unchanged.
 */
bool   d_internal_db_buffer_append(struct d_internal_db_buffer* _buffer,
                                   const void*                  _bytes,
                                   size_t                       _count);
/**
 * @brief Appends a string and its terminator, and reports where it starts.
 *
 * @param[in,out] _buffer  the buffer.
 * @param[in]     _text    the string; `NULL` appends nothing.
 * @param[out]    _offset  receives the string's offset, or
 *                         D_INTERNAL_DB_NO_TEXT for a `NULL` string.
 * @return `true`, or `false` when memory ran out, the buffer unchanged.
 */
bool   d_internal_db_buffer_add_text(struct d_internal_db_buffer* _buffer,
                                     const char*                  _text,
                                     size_t*                      _offset);
// releases the buffer's memory and leaves it empty; NULL is accepted
void   d_internal_db_buffer_release(struct d_internal_db_buffer* _buffer);


//==============================================================================
// 2.  BUILDERS
//==============================================================================
// Each builder starts all zero, takes entries until it is finished or
// released, and must be one or the other: finishing hands its memory to the
// list, releasing frees it. Both leave the builder empty and reusable.


// 2.1    Name lists
//------------------------------------------------------------------------------
// 2.1.1
// d_internal_db_names
//   struct: a d_db_name_list under construction.
struct d_internal_db_names
{
    struct d_internal_db_buffer text;     // the names, each terminated
    struct d_internal_db_buffer offsets;  // a size_t per name, into `text`
    size_t                      count;    // the names added
};

/**
 * @brief Adds a name to a list under construction.
 *
 * @param[in,out] _names  the builder.
 * @param[in]     _name   the name, copied; must not be `NULL`.
 * @return `true`, or `false` when memory ran out, the builder unchanged.
 */
bool d_internal_db_names_add(struct d_internal_db_names* _names,
                             const char*                 _name);
/**
 * @brief Packs the names added into one allocation, as d_db_name_list
 *        describes, and empties the builder.
 *
 * @param[in,out] _names  the builder.
 * @param[out]    _list   receives the list; written only on success.
 * @return `true`, or `false` when memory ran out; the builder is released
 *         either way.
 * @post On success the caller owns `_list`, released by d_db_name_list_free.
 */
bool d_internal_db_names_finish(struct d_internal_db_names* _names,
                                struct d_db_name_list*      _list);
// frees whatever the builder holds and leaves it empty; NULL is accepted
void d_internal_db_names_release(struct d_internal_db_names* _names);

// 2.2    Column lists
//------------------------------------------------------------------------------
// 2.2.1
// d_internal_db_columns
//   struct: a d_db_column_list under construction.
struct d_internal_db_columns
{
    struct d_internal_db_buffer text;     // every string, each terminated
    struct d_internal_db_buffer records;  // a record of offsets per column
    size_t                      count;    // the columns added
};

/**
 * @brief Adds a column to a list under construction.
 *
 * @param[in,out] _columns  the builder.
 * @param[in]     _column   the column, its strings copied: `name` must not
 *                          be `NULL`, a `NULL` `type` reads as "", and
 *                          `default_sql` may be `NULL`.
 * @return `true`, or `false` when memory ran out, the builder unchanged.
 */
bool d_internal_db_columns_add(struct d_internal_db_columns*  _columns,
                               const struct d_db_column_info* _column);
/**
 * @brief Packs the columns added into one allocation, as d_db_column_list
 *        describes, and empties the builder.
 *
 * @param[in,out] _columns  the builder.
 * @param[out]    _list     receives the list; written only on success.
 * @return `true`, or `false` when memory ran out; the builder is released
 *         either way.
 * @post On success the caller owns `_list`, released by
 *       d_db_column_list_free.
 */
bool d_internal_db_columns_finish(struct d_internal_db_columns* _columns,
                                  struct d_db_column_list*      _list);
// frees whatever the builder holds and leaves it empty; NULL is accepted
void d_internal_db_columns_release(struct d_internal_db_columns* _columns);

// 2.3    Rows
//------------------------------------------------------------------------------
// 2.3.1
// d_internal_db_rows
//   struct: a d_db_rows under construction. Every column is described before
// the first value is added; then values go in row by row, each row whole
// before the next begins.
struct d_internal_db_rows
{
    struct d_internal_db_buffer text;          // names, type names, text and
                                               // bytes, each terminated
    struct d_internal_db_buffer columns;       // a record per column
    struct d_internal_db_buffer values;        // a record per value
    size_t                      column_count;  // the columns described
    size_t                      value_count;   // the values added
};

/**
 * @brief Describes the next column of rows under construction.
 *
 * @param[in,out] _rows       the builder, which holds no value yet.
 * @param[in]     _name       the column's name, copied; `NULL` reads as "".
 * @param[in]     _type       its type in db_common.h's vocabulary.
 * @param[in]     _type_name  the vendor's name for its type, copied; `NULL`
 *                            reads as "".
 * @return `true`, or `false` when memory ran out, the builder unchanged.
 */
bool d_internal_db_rows_column(struct d_internal_db_rows* _rows,
                               const char*                _name,
                               enum d_db_type             _type,
                               const char*                _type_name);
// values -- each adds the next value of the current row, which begins once
// the row before it is whole: SQL NULL; an integer; a real number; text or
// bytes, `_size` bytes copied, and a terminator after them. Each returns
// `true`, or `false` when memory ran out, the builder unchanged.
bool d_internal_db_rows_null(struct d_internal_db_rows* _rows);
bool d_internal_db_rows_integer(struct d_internal_db_rows* _rows,
                                int64_t                    _value);
bool d_internal_db_rows_real(struct d_internal_db_rows* _rows,
                             double                     _value);
bool d_internal_db_rows_text(struct d_internal_db_rows* _rows,
                             const char*                _text,
                             size_t                     _size);
bool d_internal_db_rows_blob(struct d_internal_db_rows* _rows,
                             const void*                _bytes,
                             size_t                     _size);
/**
 * @brief Packs the rows built into one allocation, as d_db_rows describes,
 *        and empties the builder.
 *
 * @param[in,out] _rows  the builder, its last row whole.
 * @param[out]    _out   receives the rows; written only on success.
 * @return `true`, or `false` when memory ran out or the last row is not
 *         whole; the builder is released either way.
 * @post On success the caller owns `_out`, released by d_db_rows_free.
 */
bool d_internal_db_rows_finish(struct d_internal_db_rows* _rows,
                               struct d_db_rows*          _out);
// frees whatever the builder holds and leaves it empty; NULL is accepted
void d_internal_db_rows_release(struct d_internal_db_rows* _rows);


//==============================================================================
// 3.  STATEMENT TEXT
//==============================================================================


// 3.1    Quoting
//------------------------------------------------------------------------------
// 3.1.1
// d_internal_db_quote_fn
//   type: a vendor's way of writing one string into SQL: it appends the
// string, quoted, to the buffer, and returns `false` when it cannot -- for
// want of memory, or for a string the vendor cannot quote -- leaving the
// buffer's length as it found it.
typedef bool (*d_internal_db_quote_fn)(void*                        _context,
                                       struct d_internal_db_buffer* _out,
                                       const char*                  _text);

// 3.1.2
// d_internal_db_quoting
//   struct: how a vendor writes a value and a name into SQL.
struct d_internal_db_quoting
{
    d_internal_db_quote_fn literal;     // a string as a value, quotes and all
    d_internal_db_quote_fn identifier;  // a string as a name, quotes and all
    void*                  context;     // passed to both: a connection, say
};

// 3.2    Formatting
//------------------------------------------------------------------------------
// 3.2.1
// d_internal_db_format_status
//   enum: how formatting ended.
enum d_internal_db_format_status
{
    D_INTERNAL_DB_FORMAT_OK = 0,     // the text is in the buffer
    D_INTERNAL_DB_FORMAT_NO_MEMORY,  // memory ran out
    D_INTERNAL_DB_FORMAT_REFUSED,    // the vendor could not quote a value
    D_INTERNAL_DB_FORMAT_BAD         // the format has a conversion unknown
                                     // here
};

/**
 * @brief Formats SQL, quoting what the format asks to have quoted, and
 *        appends it to a buffer.
 *
 * @param[in,out] _out      the buffer, empty or holding text formatted
 *                          before, which the new text follows;
 *                          NUL-terminated after, the terminator not counted
 *                          in its length.
 * @param[in]     _quoting  the vendor's quoting.
 * @param[in]     _format   the format. Its conversions are `%%`; `%s`, text
 *                          as written (NULL writes NULL); `%c`, a character
 *                          other than NUL; `%d`, `%i`, `%u` and their `l`,
 *                          `ll` and `z` forms (`%zu` only), as printf writes
 *                          them; `%f`, as printf does, and `%g`, to the 17
 *                          digits that read back as the same double, both
 *                          with a `.` for the point; `%Q`, a string written
 *                          as a quoted value, NULL as NULL; `%I`, a string
 *                          written as a quoted name; and `%T`, two strings,
 *                          a qualifier and a name, written `qualifier.name`
 *                          with both quoted, or the name alone when the
 *                          qualifier is NULL. No flags, widths or
 *                          precisions.
 * @param[in]     _args     the format's arguments.
 * @return D_INTERNAL_DB_FORMAT_OK, or why not: a NaN, an infinity or a NUL
 *         character is REFUSED, as is a value the vendor cannot quote.
 * @post On failure `_out` is released, text formatted before included; on
 *       success the caller releases it.
 */
enum d_internal_db_format_status
d_internal_db_vformat(struct d_internal_db_buffer*        _out,
                      const struct d_internal_db_quoting* _quoting,
                      const char*                         _format,
                      va_list                             _args);
// as d_internal_db_vformat, taking the arguments directly
enum d_internal_db_format_status
d_internal_db_format(struct d_internal_db_buffer*        _out,
                     const struct d_internal_db_quoting* _quoting,
                     const char*                         _format,
                     ...);


//==============================================================================
// 4.  VALUES
//==============================================================================


// 4.1    Numbers
//------------------------------------------------------------------------------
// numbers -- read as a server writes them in text: an optional sign, digits,
// and for a real number a `.` point and an exponent, with nothing before or
// after. Text of any other shape -- spaces, "0x10", "inf", a comma for the
// point -- is not a number. Each returns `true` and writes the value, or
// `false` and writes nothing: _integer for text that is not an integer or
// one outside int64_t, exactly; _real for text that is not a number or one
// beyond double's range, rounding to the nearest double and reading the point
// the same whatever the C locale says.
bool   d_internal_db_parse_integer(const char* _text,
                                   int64_t*    _value);
bool   d_internal_db_parse_real(const char* _text,
                                double*     _value);


#endif  // DJINTERP_C_DB_DB_INTERNAL_H
