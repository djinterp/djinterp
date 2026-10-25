/*******************************************************************************
* djinterp [c]                                               mariadb_statement.c
*
* Implementation of mariadb_statement.h.
*   A statement keeps its values itself, copies of what was bound, and builds
* the client's MYSQL_BIND array from them only when it runs: a bind need not
* talk to the client, and the values stay valid through the run. Results are
* asked for as text, one buffer per column sized by the column's longest
* value (STMT_ATTR_UPDATE_MAX_LENGTH, read once the result is stored), and
* read by mariadb_internal.c exactly as a query's rows are. A value longer
* than its buffer -- which the stored result's lengths rule out, but which a
* client is free to report -- is fetched again whole.
*   A run read a few rows at a time leaves the result unstored, so the client
* reads each row off the connection as it is fetched. Its buffers start small,
* since no longest value is known, and each grows as a value needs, for the
* rest of the run. Until the run ends, the connection is the run's: the
* handle names its statement (mariadb_internal.h), and every function that
* would send a command refuses first. A statement finalized meanwhile cannot
* tell the server without reading the run's rows away, so the handle keeps
* its client statement until the connection is free.
*   The client reports a server's statement error on the connection too, so
* d_mariadb_error_message describes it as it describes a query's. Closing a
* statement clears that error, so one whose prepare failed is closed later,
* when the layer next runs a statement.
*   The handle lists the statements prepared on it, so that closing it can
* leave each without a connection, and a statement can tell, when it is
* finalized, whether its connection is still there to be told.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_statement.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.22
*******************************************************************************/
// D_INTERNAL_MARIADB_SOURCE
//   macro: this file is one of the MariaDB layer's own sources, for which
// cfg_mariadb_common.h has env include mysql.h; it precedes every include.
#define D_INTERNAL_MARIADB_SOURCE 1

#include "../../../../../inc/djinterp/c/db/mariadb/mariadb_statement.h"  // corresponding header
// std
#include <stdarg.h>  // va_list, va_start, va_end
#include <stddef.h>  // size_t, NULL
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, memset, strlen
// djinterp
#include "../../../../../inc/djinterp/c/db/db_common.h"  // D_DB_*, d_db_rows
#include "../db_internal.h"  // the rows builder, formatting
#include "../../../../../inc/djinterp/config/c/db/mariadb/cfg_mariadb_common.h"  // D_INTERNAL_MARIADB
#include "./mariadb_internal.h"  // mysql.h, the floor, the plumbing
// re_std
#include "../../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


#if (D_INTERNAL_MARIADB == 1)

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

// D_INTERNAL_MARIADB_FIRST_CAPACITY, D_INTERNAL_MARIADB_LEAST_CAPACITY
//   constants: the bytes a column's buffer starts with, its terminator among
// them, when rows are read as they arrive and no longest value is known: the
// column's declared width, but no more than the first and no less than the
// second, whatever is declared; a longer value grows it.
#define D_INTERNAL_MARIADB_FIRST_CAPACITY 256u
#define D_INTERNAL_MARIADB_LEAST_CAPACITY 32u

// d_internal_mariadb_value
//   struct: what one parameter is bound to, until the statement runs.
struct d_internal_mariadb_value
{
    enum d_db_value_type type;     // NULL until a bind says otherwise
    long long            integer;  // an INTEGER, a boolean as 1 or 0
    double               real;     // a REAL
    char*                bytes;    // TEXT or a BLOB: the statement's copy
    unsigned long        length;   // its length, where MYSQL_BIND reads it
};

// d_internal_mariadb_column
//   struct: where one column of a result is fetched to.
struct d_internal_mariadb_column
{
    char*                   buffer;    // the value's text, terminated
    unsigned long           capacity;  // the buffer's bytes
    unsigned long           length;    // the value's bytes, as fetched
    d_internal_mariadb_flag is_null;   // whether it is NULL
    d_internal_mariadb_flag error;     // whether it was cut short
};

// d_internal_mariadb_run
//   struct: a run d_mariadb_open_cursor began, its rows still arriving: what
// the rows are, and where each value is fetched to, kept from one fetch to
// the next. Meaningful only while the connection's cursor is its statement.
struct d_internal_mariadb_run
{
    MYSQL_RES*                        metadata;  // the result's fields; NULL
                                                 // for a statement without
                                                 // rows
    unsigned                          count;     // how many
    struct d_internal_mariadb_column* columns;   // count of them
    MYSQL_BIND*                       binds;     // count of them
};

// d_mariadb_statement
//   struct: the definition mariadb_common.h declares.
struct d_mariadb_statement
{
    struct d_mariadb*                db;        // the connection it belongs
                                                // to; NULL once that closed
    MYSQL_STMT*                      native;    // the client's statement
    size_t                           count;     // its parameters
    MYSQL_BIND*                      binds;     // count of them, built to run
    struct d_internal_mariadb_value* values;    // count of them
    struct d_mariadb_statement*      previous;  // the neighbours in the
    struct d_mariadb_statement*      next;      // handle's list: its
                                                // statements, or those it
                                                // retired
    struct d_internal_mariadb_run    run;       // a run whose rows are still
                                                // arriving
};

/**
 * @brief Empties a parameter's value, releasing its copy.
 *
 * @param[in,out] _value  the value; NULL after.
 */
static void
d_internal_mariadb_value_clear(
    struct d_internal_mariadb_value* _value
)
{
    free(_value->bytes);

    _value->type    = D_DB_VALUE_NULL;
    _value->integer = 0;
    _value->real    = 0.0;
    _value->bytes   = NULL;
    _value->length  = 0u;

    return;
}

/**
 * @brief Finds the value a parameter's number names.
 *
 * @param[in]  _statement  the statement.
 * @param[in]  _index      the number, from 1.
 * @param[out] _value      receives the value.
 * @return 0, D_INTERNAL_MARIADB_INVALID_PARAMETER for a number the statement
 *         does not have, or D_DB_INVALID_ARGUMENT for a `NULL` statement.
 */
static int
d_internal_mariadb_value_at(
    struct d_mariadb_statement*       _statement,
    size_t                            _index,
    struct d_internal_mariadb_value** _value
)
{
    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a number the statement has
    if ( (_index == 0u) ||
         (_index > _statement->count) )
    {
        return D_INTERNAL_MARIADB_INVALID_PARAMETER;
    }

    *_value = &_statement->values[_index - 1u];

    return 0;
}

/**
 * @brief Binds a copy of bytes, as TEXT or a BLOB.
 *
 * @param[in] _statement  the statement.
 * @param[in] _index      the parameter.
 * @param[in] _type       D_DB_VALUE_TEXT or D_DB_VALUE_BLOB.
 * @param[in] _bytes      the bytes; not `NULL`.
 * @param[in] _size       how many.
 * @return 0; D_INTERNAL_MARIADB_INVALID_PARAMETER;
 *         D_INTERNAL_MARIADB_OUT_OF_MEMORY; D_DB_INVALID_ARGUMENT for a
 *         `NULL` statement or a size the client cannot send.
 *
 * The copy is made before the old value goes, so a failure leaves the
 * parameter as it was. It is one byte longer than the value, so an empty
 * value is still a pointer, which the client sends as empty, not NULL.
 */
static int
d_internal_mariadb_bind_bytes(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    enum d_db_value_type        _type,
    const void*                 _bytes,
    size_t                      _size
)
{
    struct d_internal_mariadb_value* value  = NULL;
    const int                        result =
        d_internal_mariadb_value_at(_statement, _index, &value);

    // a statement and a parameter it has
    if (result != 0)
    {
        return result;
    }

    // a length the client's unsigned long cannot carry
    if (_size >= D_INTERNAL_MARIADB_LENGTH_MAX)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    char* const copy = (char*)malloc(_size + 1u);

    // allocation failure
    if (!copy)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    memcpy(copy, _bytes, _size);
    copy[_size] = '\0';

    d_internal_mariadb_value_clear(value);

    value->type   = _type;
    value->bytes  = copy;
    value->length = (unsigned long)_size;

    return 0;
}

/**
 * @brief Reads, and discards, every result after the first, as only a CALL
 *        produces, so that the connection is ready for its next statement.
 *
 * @param[in] _native  the statement, its first result already read.
 * @return 0, or the client's error number for a result that failed.
 */
static int
d_internal_mariadb_drain(
    MYSQL_STMT* _native
)
{
    int next = mysql_stmt_next_result(_native);

    // each further result, its rows read and freed
    while (next == 0)
    {
        if (mysql_stmt_field_count(_native) > 0u)
        {
            (void)mysql_stmt_store_result(_native);
            (void)mysql_stmt_free_result(_native);
        }

        next = mysql_stmt_next_result(_native);
    }

    return (next > 0) ? (int)mysql_stmt_errno(_native) : 0;
}

/**
 * @brief Releases a result's column buffers.
 *
 * @param[in] _columns  the columns; may be `NULL`.
 * @param[in] _count    how many.
 */
static void
d_internal_mariadb_columns_free(
    struct d_internal_mariadb_column* _columns,
    unsigned                          _count
)
{
    // accept NULL, as free does
    if (_columns)
    {
        for (unsigned i = 0u; i < _count; ++i)
        {
            free(_columns[i].buffer);
        }

        free(_columns);
    }

    return;
}

/**
 * @brief Makes a buffer and a bind for each column of a result, every value
 *        to be fetched as text.
 *
 * @param[in]  _fields    the result's fields.
 * @param[in]  _count     how many.
 * @param[in]  _measured  whether the result is stored, each field's
 *                        max_length its longest value: each buffer holds
 *                        that and a terminator. Otherwise each starts as
 *                        D_INTERNAL_MARIADB_FIRST_CAPACITY describes.
 * @param[out] _columns   receives the buffers; written only on success.
 * @param[out] _binds     receives the binds, pointing into them; written
 *                        only on success.
 * @return 0, or D_INTERNAL_MARIADB_OUT_OF_MEMORY.
 */
static int
d_internal_mariadb_columns_make(
    const MYSQL_FIELD*                 _fields,
    unsigned                           _count,
    bool                               _measured,
    struct d_internal_mariadb_column** _columns,
    MYSQL_BIND**                       _binds
)
{
    struct d_internal_mariadb_column* const columns =
        (struct d_internal_mariadb_column*)malloc(
            (_count ? _count : 1u) * sizeof(struct d_internal_mariadb_column));
    MYSQL_BIND* const binds =
        (MYSQL_BIND*)malloc((_count ? _count : 1u) * sizeof(MYSQL_BIND));
    int result = 0;

    // allocation failure
    if ( (!columns) ||
         (!binds) )
    {
        free(columns);
        free(binds);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    // a buffer for each column, its terminator among its bytes
    for (unsigned i = 0u; i < _count; ++i)
    {
        unsigned long bytes = 0u;

        if (_measured)
        {
            const unsigned long longest = _fields[i].max_length;

            bytes = (longest < (unsigned long)-1) ? (longest + 1u) : longest;
        }
        else
        {
            const unsigned long declared = _fields[i].length;

            bytes = (declared < D_INTERNAL_MARIADB_FIRST_CAPACITY)
                        ? (declared + 1u)
                        : D_INTERNAL_MARIADB_FIRST_CAPACITY;
            bytes = (bytes < D_INTERNAL_MARIADB_LEAST_CAPACITY)
                        ? D_INTERNAL_MARIADB_LEAST_CAPACITY
                        : bytes;
        }

        columns[i].capacity = bytes;
        columns[i].buffer   = (char*)malloc(bytes);
        columns[i].length   = 0u;
        columns[i].is_null  = 0;
        columns[i].error    = 0;

        memset(&binds[i], 0, sizeof(binds[i]));
        binds[i].buffer_type   = MYSQL_TYPE_STRING;
        binds[i].buffer        = columns[i].buffer;
        binds[i].buffer_length = columns[i].capacity;
        binds[i].length        = &columns[i].length;
        binds[i].is_null       = &columns[i].is_null;
        binds[i].error         = &columns[i].error;

        // allocation failure; every buffer is still made or NULL, for the
        // release
        if (!columns[i].buffer)
        {
            result = D_INTERNAL_MARIADB_OUT_OF_MEMORY;
        }
    }

    if (result != 0)
    {
        d_internal_mariadb_columns_free(columns, _count);
        free(binds);

        return result;
    }

    *_columns = columns;
    *_binds   = binds;

    return 0;
}

/**
 * @brief Fetches the next row of a result, its binds already given to the
 *        client, into rows under construction.
 *
 * @param[in]     _native   the statement, its result stored or arriving.
 * @param[in]     _connection
 *                          the client's connection, the statement's.
 * @param[in]     _fields   the result's fields.
 * @param[in]     _count    how many.
 * @param[in,out] _columns  the buffers; one too small for a value grows, and
 *                          keeps its size after.
 * @param[in,out] _binds    the binds, pointed at a grown buffer and given to
 *                          the client again before the next fetch.
 * @param[in,out] _rows     the builder, its columns described.
 * @param[out]    _ended    receives whether the result has ended: at its end,
 *                          with 0, or when the client's fetch failed, with
 *                          its error number. A failure after a row arrived
 *                          leaves the result where it was.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_MEMORY; or the client's error number
 *         for the failure.
 *
 * A server error that ends a result whose rows are arriving is the
 * connection's: Connector/C, reading it, leaves the statement's error unset,
 * where libmysqlclient copies it there too, so the connection's stands in.
 */
static int
d_internal_mariadb_fetch_row(
    MYSQL_STMT*                       _native,
    MYSQL*                            _connection,
    const MYSQL_FIELD*                _fields,
    unsigned                          _count,
    struct d_internal_mariadb_column* _columns,
    MYSQL_BIND*                       _binds,
    struct d_internal_db_rows*        _rows,
    bool*                             _ended
)
{
    const int fetched = mysql_stmt_fetch(_native);
    bool      grown   = false;
    int       result  = 0;

    *_ended = false;

    // the end
    if (fetched == MYSQL_NO_DATA)
    {
        *_ended = true;

        return 0;
    }

    // a failure, the statement's or else the connection's
    if (fetched == 1)
    {
        const unsigned own = mysql_stmt_errno(_native);
        const unsigned its = (own != 0u) ? own : mysql_errno(_connection);

        *_ended = true;

        return (its != 0u) ? (int)its : D_INTERNAL_MARIADB_UNKNOWN_ERROR;
    }

    // each value, fetched whole when the buffer was too small
    for (unsigned i = 0u; ( (result == 0) && (i < _count) ); ++i)
    {
        struct d_internal_mariadb_column* const column = &_columns[i];

        if ( (!column->is_null) &&
             (column->length >= column->capacity) )
        {
            char* const larger = (column->length < (unsigned long)-1)
                                     ? (char*)realloc(column->buffer,
                                                      column->length + 1u)
                                     : NULL;

            // the true length, then the value again
            if (!larger)
            {
                result = D_INTERNAL_MARIADB_OUT_OF_MEMORY;

                break;
            }

            column->buffer          = larger;
            column->capacity        = column->length + 1u;
            _binds[i].buffer        = larger;
            _binds[i].buffer_length = column->capacity;
            grown                   = true;

            if (mysql_stmt_fetch_column(_native, &_binds[i], i, 0u) != 0)
            {
                result = (int)mysql_stmt_errno(_native);

                break;
            }
        }

        // terminated, as the rows builder's text is
        if (!column->is_null)
        {
            column->buffer[column->length] = '\0';
        }

        if (!d_internal_mariadb_rows_value(
                 _rows,
                 &_fields[i],
                 (column->is_null) ? NULL : column->buffer,
                 column->length))
        {
            result = D_INTERNAL_MARIADB_OUT_OF_MEMORY;
        }
    }

    // the client keeps a copy of the binds, which a grown buffer left
    // pointing at freed memory: given again, before the next fetch, whatever
    // else happened
    if ( (grown) &&
         (mysql_stmt_bind_result(_native, _binds)) &&
         (result == 0) )
    {
        result = (int)mysql_stmt_errno(_native);
    }

    return result;
}

/**
 * @brief Fetches the stored result of a statement that ran into rows under
 *        construction.
 *
 * @param[in]     _native  the statement, its result stored.
 * @param[in]     _connection
 *                         the client's connection, the statement's.
 * @param[in]     _fields  the result's fields.
 * @param[in]     _count   how many.
 * @param[in,out] _rows    the builder, its columns described.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_MEMORY; or the client's error number
 *         for a fetch that failed.
 *
 * Every column is fetched as text into a buffer of its longest value and a
 * terminator. A column the client reports cut short is fetched again into a
 * buffer of its true length.
 */
static int
d_internal_mariadb_fetch_rows(
    MYSQL_STMT*                _native,
    MYSQL*                     _connection,
    const MYSQL_FIELD*         _fields,
    unsigned                   _count,
    struct d_internal_db_rows* _rows
)
{
    struct d_internal_mariadb_column* columns = NULL;
    MYSQL_BIND*                       binds   = NULL;
    bool                              ended   = false;
    int                               result  =
        d_internal_mariadb_columns_make(_fields, _count, true, &columns,
                                        &binds);

    // no buffers, no rows
    if (result != 0)
    {
        return result;
    }

    // the binds, then each row
    if (mysql_stmt_bind_result(_native, binds))
    {
        result = (int)mysql_stmt_errno(_native);
    }

    while ( (result == 0) &&
            (!ended) )
    {
        result = d_internal_mariadb_fetch_row(_native, _connection, _fields,
                                              _count, columns, binds, _rows,
                                              &ended);
    }

    d_internal_mariadb_columns_free(columns, _count);
    free(binds);

    return result;
}

/**
 * @brief Ends the run that holds a statement's connection, and releases
 *        what the run kept.
 *
 * @param[in,out] _statement  the statement whose run holds its connection.
 * @param[in]     _read_rest  whether the rows still to come are read, and
 *                            every result after them, so that the connection
 *                            is free for its next command; one about to
 *                            close needs nothing read, and one whose fetch
 *                            failed has nothing left.
 * @return 0, or the client's error number for a result after the first that
 *         failed.
 * @post The connection holds no run.
 *
 * Reading the rest clears the client's error, so a run that failed is ended
 * without it.
 */
static int
d_internal_mariadb_run_end(
    struct d_mariadb_statement* _statement,
    bool                        _read_rest
)
{
    struct d_internal_mariadb_run* const run    = &_statement->run;
    int                                  result = 0;

    // the rest of the rows, then every result after them
    if (_read_rest)
    {
        (void)mysql_stmt_free_result(_statement->native);

        result = d_internal_mariadb_drain(_statement->native);
    }

    if (run->metadata)
    {
        mysql_free_result(run->metadata);
    }

    d_internal_mariadb_columns_free(run->columns, run->count);
    free(run->binds);

    run->metadata          = NULL;
    run->count             = 0u;
    run->columns           = NULL;
    run->binds             = NULL;
    _statement->db->cursor = NULL;

    return result;
}

/**
 * @brief Takes a statement out of its handle's list.
 *
 * @param[in,out] _statement  the statement, its connection still there.
 */
static void
d_internal_mariadb_unlink(
    struct d_mariadb_statement* _statement
)
{
    // the one before it, or the list's head
    if (_statement->previous)
    {
        _statement->previous->next = _statement->next;
    }
    else if (_statement->db->statements == _statement)
    {
        _statement->db->statements = _statement->next;
    }

    if (_statement->next)
    {
        _statement->next->previous = _statement->previous;
    }

    _statement->previous = NULL;
    _statement->next     = NULL;

    return;
}

/**
 * @brief Runs a statement with its values: builds the client's binds from
 *        them, and executes.
 *
 * @param[in,out] _statement  the statement, its connection still there.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_SYNC while another statement's run
 *         holds the connection; or the client's error number for the
 *         failure.
 *
 * The statement's own run, if it has one open, ends first, read to its end.
 * What a failure left on the connection is undone next, as before any
 * statement the layer runs (d_internal_mariadb_ready).
 */
static int
d_internal_mariadb_execute(
    struct d_mariadb_statement* _statement
)
{
    MYSQL_STMT* const native = _statement->native;

    // a run of its own ends before another begins
    if (_statement->db->cursor == _statement)
    {
        (void)d_internal_mariadb_run_end(_statement, true);
        d_internal_mariadb_replied(_statement->db, false);
    }

    const int ready = d_internal_mariadb_ready(_statement->db);

    // the connection as the layer's statements need it
    if (ready != 0)
    {
        return ready;
    }

    // a bind for each value, pointing into it
    for (size_t i = 0u; i < _statement->count; ++i)
    {
        MYSQL_BIND* const                      bind  = &_statement->binds[i];
        struct d_internal_mariadb_value* const value = &_statement->values[i];

        memset(bind, 0, sizeof(*bind));

        switch (value->type)
        {
            case D_DB_VALUE_INTEGER:
                bind->buffer_type = MYSQL_TYPE_LONGLONG;
                bind->buffer      = &value->integer;

                break;

            case D_DB_VALUE_REAL:
                bind->buffer_type = MYSQL_TYPE_DOUBLE;
                bind->buffer      = &value->real;

                break;

            case D_DB_VALUE_TEXT:
            case D_DB_VALUE_BLOB:
                bind->buffer_type   = (value->type == D_DB_VALUE_TEXT)
                                          ? MYSQL_TYPE_STRING
                                          : MYSQL_TYPE_BLOB;
                bind->buffer        = value->bytes;
                bind->buffer_length = value->length;
                bind->length        = &value->length;

                break;

            case D_DB_VALUE_NULL:
            default:
                bind->buffer_type = MYSQL_TYPE_NULL;

                break;
        }
    }

    // the binds, which the client copies, then the statement
    if ( ( (_statement->count > 0u) &&
           (mysql_stmt_bind_param(native, _statement->binds)) ) ||
         (mysql_stmt_execute(native) != 0) )
    {
        return (int)mysql_stmt_errno(native);
    }

    return 0;
}

/**
 * @brief Runs a statement and reads its first result: the rows into rows
 *        under construction, when there are any and somewhere to put them,
 *        and the count it changed or returned.
 *
 * @param[in,out] _statement  the statement.
 * @param[in,out] _rows       the builder, empty; `NULL` to discard rows.
 * @param[out]    _changes    receives the count, as d_mariadb_execute_update
 *                            describes it; written only on success.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_MEMORY; or the client's error number
 *         for the failure.
 * @post Every result has been read and freed, whatever happened.
 */
static int
d_internal_mariadb_read(
    struct d_mariadb_statement* _statement,
    struct d_internal_db_rows*  _rows,
    int64_t*                    _changes
)
{
    MYSQL_STMT* const native = _statement->native;
    int               result = d_internal_mariadb_execute(_statement);

    // a statement that did not run has nothing to read
    if (result != 0)
    {
        d_internal_mariadb_replied(_statement->db, false);

        return result;
    }

    int64_t changes = 0;

    // a statement without rows counts the rows it changed
    if (mysql_stmt_field_count(native) == 0u)
    {
        changes = (int64_t)mysql_stmt_affected_rows(native);
    }
    else if (mysql_stmt_store_result(native) != 0)
    {
        result = (int)mysql_stmt_errno(native);
    }
    else
    {
        MYSQL_RES* const metadata = mysql_stmt_result_metadata(native);

        changes = (int64_t)mysql_stmt_num_rows(native);

        // the rows, when they are wanted
        if ( (_rows) &&
             (!metadata) )
        {
            result = (int)mysql_stmt_errno(native);
        }
        else if (_rows)
        {
            const unsigned           count  = mysql_num_fields(metadata);
            const MYSQL_FIELD* const fields = mysql_fetch_fields(metadata);

            result = (d_internal_mariadb_rows_columns(_rows, fields, count))
                         ? d_internal_mariadb_fetch_rows(
                               native, _statement->db->native, fields, count,
                               _rows)
                         : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
        }

        // the metadata and the stored rows, in that order
        if (metadata)
        {
            mysql_free_result(metadata);
        }

        (void)mysql_stmt_free_result(native);
    }

    const int drained = d_internal_mariadb_drain(native);

    // the first failure stands
    if (result == 0)
    {
        result = drained;
    }

    // the reply's end: the last result's, an OK packet unless rows end it
    // without one
    d_internal_mariadb_replied(
        _statement->db,
        ( (result == 0) &&
          ( (mysql_stmt_field_count(native) == 0u) ||
            (d_internal_mariadb_rows_end_ok(_statement->db)) ) ));

    // the count, on success
    if (result == 0)
    {
        *_changes = changes;
    }

    return result;
}

/**
 * @brief Has the server compile SQL, and makes the layer's statement around
 *        it.
 *
 * @param[in]  _db         the connection.
 * @param[out] _statement  receives the statement; left as it was on
 *                         failure.
 * @param[in]  _sql        the statement's text.
 * @param[in]  _length     its length.
 * @return as d_mariadb_prepare.
 *
 * A prepare that fails leaves the client's statement with the connection,
 * to be closed when the layer next runs a statement: closing it sends
 * nothing, since the server never prepared it, but clears the connection's
 * error, which d_mariadb_error_message is about to be asked for.
 */
static int
d_internal_mariadb_prepare_text(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _sql,
    size_t                       _length
)
{
    // a length the client's unsigned long cannot carry
    if (_length > D_INTERNAL_MARIADB_LENGTH_MAX)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    int result = d_internal_mariadb_ready(_db);

    // the connection as the layer's statements need it
    if (result != 0)
    {
        return result;
    }

    struct d_mariadb_statement* const made =
        (struct d_mariadb_statement*)malloc(sizeof(*made));

    // allocation failure
    if (!made)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    made->db           = _db;
    made->native       = mysql_stmt_init(_db->native);
    made->count        = 0u;
    made->binds        = NULL;
    made->values       = NULL;
    made->previous     = NULL;
    made->next         = NULL;
    made->run.metadata = NULL;
    made->run.count    = 0u;
    made->run.columns  = NULL;
    made->run.binds    = NULL;

    // the client's statement, compiled by the server
    if (!made->native)
    {
        free(made);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    const int prepared = mysql_stmt_prepare(made->native,
                                            _sql,
                                            (unsigned long)_length);

    // a reply that reports no changes
    d_internal_mariadb_replied(_db, false);

    if (prepared != 0)
    {
        result = (int)mysql_stmt_errno(made->native);

        // kept, closed when the layer next runs a statement, since closing it
        // now would clear the error the caller is about to read
        _db->failed = made->native;
        free(made);

        return result;
    }

    // listed with the handle, which closing leaves without a connection
    made->next = _db->statements;

    if (_db->statements)
    {
        _db->statements->previous = made;
    }

    _db->statements = made;

    // the longest value of each column, measured when a result is stored
    d_internal_mariadb_flag measure = 1;

    (void)mysql_stmt_attr_set(made->native, STMT_ATTR_UPDATE_MAX_LENGTH,
                              &measure);

    made->count = (size_t)mysql_stmt_param_count(made->native);

    // room for the parameters' values, each NULL to start
    if (made->count > 0u)
    {
        made->binds  = (MYSQL_BIND*)malloc(made->count * sizeof(MYSQL_BIND));
        made->values = (struct d_internal_mariadb_value*)malloc(
                           made->count *
                           sizeof(struct d_internal_mariadb_value));

        if ( (!made->binds) ||
             (!made->values) )
        {
            free(made->binds);
            free(made->values);

            made->binds  = NULL;
            made->values = NULL;
            made->count  = 0u;

            (void)d_mariadb_finalize(made);

            return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
        }

        for (size_t i = 0u; i < made->count; ++i)
        {
            made->values[i].bytes = NULL;

            d_internal_mariadb_value_clear(&made->values[i]);
        }
    }

    *_statement = made;

    return 0;
}

//==============================================================================
// 1.  STATEMENTS
//==============================================================================

// 1.1    Preparing
//------------------------------------------------------------------------------
/*
d_mariadb_prepare
  The server compiles the text as it stands.
*/
int
d_mariadb_prepare(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _sql
)
{
    // no place to put a statement
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_statement = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_sql) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_prepare_text(_db, _statement, _sql,
                                           strlen(_sql));
}

/*
d_mariadb_preparef
  Formatted with the connection's quoting, then compiled.
*/
int
d_mariadb_preparef(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _format,
    ...
)
{
    struct d_internal_db_buffer sql = { NULL, 0u, 0u };
    va_list                     args;

    // no place to put a statement
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    *_statement = NULL;

    // parameter validation
    if ( (!_db) ||
         (!_format) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const struct d_internal_db_quoting quoting =
        d_internal_mariadb_quoting(_db);

    va_start(args, _format);

    int result = d_internal_mariadb_format_result(
        d_internal_db_vformat(&sql, &quoting, _format, args));

    va_end(args);

    // formatting released the buffer if it failed
    if (result != 0)
    {
        return result;
    }

    result = d_internal_mariadb_prepare_text(_db, _statement, sql.data,
                                             sql.length);

    d_internal_db_buffer_release(&sql);

    return result;
}

// 1.2    Finalizing
//------------------------------------------------------------------------------
/*
d_mariadb_finalize
  A statement whose connection closed has nothing to tell it: the client
closed the server's side with the connection, and mysql_stmt_close only
frees. Otherwise its own run ends first, read to its end, and it leaves the
handle's list. While another statement's run holds the connection, closing
the client's statement would read that run's rows away, so the handle keeps
it, to be closed once the connection is free; the rest goes at once.
*/
int
d_mariadb_finalize(
    struct d_mariadb_statement* _statement
)
{
    int result = 0;

    // accept NULL, as free does
    if (!_statement)
    {
        return 0;
    }

    struct d_mariadb* const db = _statement->db;

    // the values' copies
    for (size_t i = 0u; i < _statement->count; ++i)
    {
        free(_statement->values[i].bytes);
    }

    free(_statement->binds);
    free(_statement->values);

    _statement->count  = 0u;
    _statement->binds  = NULL;
    _statement->values = NULL;

    if (db)
    {
        // its own run, read to its end
        if (db->cursor == _statement)
        {
            (void)d_internal_mariadb_run_end(_statement, true);
            d_internal_mariadb_replied(db, false);
        }

        d_internal_mariadb_unlink(_statement);

        // another's run holds the connection: retired, the server told later
        if (db->cursor)
        {
            _statement->next = db->retired;
            db->retired      = _statement;

            return 0;
        }
    }

    // the client's statement, and the server's copy
    if ( (_statement->native) &&
         (mysql_stmt_close(_statement->native)) )
    {
        result = D_INTERNAL_MARIADB_UNKNOWN_ERROR;
    }

    free(_statement);

    return result;
}

// 1.3    Describing
//------------------------------------------------------------------------------
/*
d_mariadb_parameter_count
  Counted by the client when the server compiled the statement.
*/
size_t
d_mariadb_parameter_count(
    const struct d_mariadb_statement* _statement
)
{
    return (_statement) ? _statement->count : 0u;
}

/*
d_mariadb_statement_connection
  The handle the statement was prepared on, until it is closed.
*/
struct d_mariadb*
d_mariadb_statement_connection(
    const struct d_mariadb_statement* _statement
)
{
    return (_statement) ? _statement->db : NULL;
}

//==============================================================================
// 2.  VALUES
//==============================================================================

// 2.1    Binding
//------------------------------------------------------------------------------
/*
d_mariadb_bind_null
  The value emptied: a run binds it as MYSQL_TYPE_NULL.
*/
int
d_mariadb_bind_null(
    struct d_mariadb_statement* _statement,
    size_t                      _index
)
{
    struct d_internal_mariadb_value* value  = NULL;
    const int                        result =
        d_internal_mariadb_value_at(_statement, _index, &value);

    // a statement and a parameter it has
    if (result == 0)
    {
        d_internal_mariadb_value_clear(value);
    }

    return result;
}

/*
d_mariadb_bind_bool
  The servers have no boolean but TINYINT(1): 1 or 0.
*/
int
d_mariadb_bind_bool(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    bool                        _value
)
{
    struct d_internal_mariadb_value* value  = NULL;
    const int                        result =
        d_internal_mariadb_value_at(_statement, _index, &value);

    // a statement and a parameter it has
    if (result == 0)
    {
        d_internal_mariadb_value_clear(value);

        value->type    = D_DB_VALUE_INTEGER;
        value->integer = (_value) ? 1 : 0;
    }

    return result;
}

/*
d_mariadb_bind_int64
  Sent as a BIGINT.
*/
int
d_mariadb_bind_int64(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    int64_t                     _value
)
{
    struct d_internal_mariadb_value* value  = NULL;
    const int                        result =
        d_internal_mariadb_value_at(_statement, _index, &value);

    // a statement and a parameter it has
    if (result == 0)
    {
        d_internal_mariadb_value_clear(value);

        value->type    = D_DB_VALUE_INTEGER;
        value->integer = (long long)_value;
    }

    return result;
}

/*
d_mariadb_bind_double
  Sent as a DOUBLE.
*/
int
d_mariadb_bind_double(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    double                      _value
)
{
    struct d_internal_mariadb_value* value  = NULL;
    const int                        result =
        d_internal_mariadb_value_at(_statement, _index, &value);

    // a statement and a parameter it has
    if (result == 0)
    {
        d_internal_mariadb_value_clear(value);

        value->type = D_DB_VALUE_REAL;
        value->real = _value;
    }

    return result;
}

/*
d_mariadb_bind_text
  A copy, by its length; NULL binds NULL.
*/
int
d_mariadb_bind_text(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    const char*                 _text
)
{
    return (_text) ? d_internal_mariadb_bind_bytes(_statement, _index,
                                                   D_DB_VALUE_TEXT, _text,
                                                   strlen(_text))
                   : d_mariadb_bind_null(_statement, _index);
}

/*
d_mariadb_bind_blob
  A copy; NULL binds NULL, and a pointer to no bytes the empty string of
bytes.
*/
int
d_mariadb_bind_blob(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    const void*                 _bytes,
    size_t                      _size
)
{
    return (_bytes) ? d_internal_mariadb_bind_bytes(_statement, _index,
                                                    D_DB_VALUE_BLOB, _bytes,
                                                    _size)
                    : d_mariadb_bind_null(_statement, _index);
}

// 2.2    Clearing
//------------------------------------------------------------------------------
/*
d_mariadb_clear_bindings
  Every value emptied; nothing is sent.
*/
int
d_mariadb_clear_bindings(
    struct d_mariadb_statement* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < _statement->count; ++i)
    {
        d_internal_mariadb_value_clear(&_statement->values[i]);
    }

    return 0;
}

//==============================================================================
// 3.  RUNNING
//==============================================================================

// 3.1    Rows changed
//------------------------------------------------------------------------------
/*
d_mariadb_execute_update
  A statement that returns rows has them stored, so the client counts them,
and freed unread.
*/
int
d_mariadb_execute_update(
    struct d_mariadb_statement* _statement,
    int64_t*                    _changes
)
{
    int64_t changes = 0;

    // parameter validation; a statement whose connection closed cannot run
    if ( (!_statement) ||
         (!_statement->db) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_read(_statement, NULL, &changes);

    // the count, when there is one and somewhere to put it
    if ( (result == 0) &&
         (_changes) )
    {
        *_changes = changes;
    }

    return result;
}

// 3.2    Rows
//------------------------------------------------------------------------------
/*
d_mariadb_execute_query
  The rows of the first result, read as a query's; a statement with no result
gives rows with no columns.
*/
int
d_mariadb_execute_query(
    struct d_mariadb_statement* _statement,
    struct d_db_rows*           _rows
)
{
    struct d_internal_db_rows builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u, 0u
    };
    int64_t changes = 0;

    // parameter validation; a statement whose connection closed cannot run
    if ( (!_statement)     ||
         (!_statement->db) ||
         (!_rows) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int result = d_internal_mariadb_read(_statement, &builder,
                                               &changes);

    // a failure drops whatever was read
    if (result != 0)
    {
        d_internal_db_rows_release(&builder);

        return result;
    }

    return (d_internal_db_rows_finish(&builder, _rows))
               ? 0
               : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
}

// 3.3    Rows a few at a time
//------------------------------------------------------------------------------
/*
d_mariadb_open_cursor
  The result is left unstored, so the client reads each row as it is fetched.
A statement without rows has run to its end already, but its run stays open
for its first fetch to end, as every run's last fetch does. A run that cannot
begin -- its fields or buffers wanting -- ends at once, read to its end.
*/
int
d_mariadb_open_cursor(
    struct d_mariadb_statement* _statement
)
{
    // parameter validation; a statement whose connection closed cannot run
    if ( (!_statement) ||
         (!_statement->db) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    struct d_mariadb* const              db     = _statement->db;
    MYSQL_STMT* const                    native = _statement->native;
    struct d_internal_mariadb_run* const run    = &_statement->run;
    int                                  result =
        d_internal_mariadb_execute(_statement);

    // a statement that did not run holds nothing
    if (result != 0)
    {
        d_internal_mariadb_replied(db, false);

        return result;
    }

    // the connection is the run's from here until it ends
    db->cursor = _statement;

    // rows to come: what they are, and where each value goes
    if (mysql_stmt_field_count(native) > 0u)
    {
        run->metadata = mysql_stmt_result_metadata(native);

        if (!run->metadata)
        {
            const unsigned error = mysql_stmt_errno(native);

            result = (error != 0u) ? (int)error
                                   : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
        }
        else
        {
            run->count = mysql_num_fields(run->metadata);
            result     = d_internal_mariadb_columns_make(
                             mysql_fetch_fields(run->metadata),
                             run->count,
                             false,
                             &run->columns,
                             &run->binds);

            // no buffers, so none to release
            if (result != 0)
            {
                run->columns = NULL;
                run->binds   = NULL;
            }
            else if (mysql_stmt_bind_result(native, run->binds))
            {
                result = (int)mysql_stmt_errno(native);
            }
        }
    }

    // a run that cannot begin ends at once
    if (result != 0)
    {
        (void)d_internal_mariadb_run_end(_statement, true);
        d_internal_mariadb_replied(db, false);
    }

    return result;
}

/*
d_mariadb_fetch_rows
  Each fetch describes the columns again, so that each page stands alone. A
fetch that fails ends the run as the client left it: with nothing more to
read when the client's fetch failed, which also keeps its error for the
caller; otherwise, the row having arrived, read to its end. A run that
reaches its end reads every result after its rows, as a whole run does, and
the reply's end says whether it carried the session's changes.
*/
int
d_mariadb_fetch_rows(
    struct d_mariadb_statement* _statement,
    size_t                      _max,
    struct d_db_rows*           _rows
)
{
    struct d_internal_db_rows builder =
    {
        { NULL, 0u, 0u }, { NULL, 0u, 0u }, { NULL, 0u, 0u }, 0u, 0u
    };
    size_t fetched = 0u;

    // parameter validation; only a run still open has rows to give
    if ( (!_statement)                     ||
         (!_rows)                          ||
         (_max == 0u)                      ||
         (!_statement->db)                 ||
         (_statement->db->cursor != _statement) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    struct d_mariadb* const              db     = _statement->db;
    MYSQL_STMT* const                    native = _statement->native;
    struct d_internal_mariadb_run* const run    = &_statement->run;
    const MYSQL_FIELD* const             fields =
        (run->metadata) ? mysql_fetch_fields(run->metadata) : NULL;
    bool                                 ended  = (run->count == 0u);
    int                                  result =
        (d_internal_mariadb_rows_columns(&builder, fields, run->count))
            ? 0
            : D_INTERNAL_MARIADB_OUT_OF_MEMORY;

    // up to the rows asked for, each kept as it arrives
    while ( (result == 0) &&
            (!ended)      &&
            (fetched < _max) )
    {
        result = d_internal_mariadb_fetch_row(native, db->native, fields,
                                              run->count, run->columns,
                                              run->binds, &builder, &ended);

        if ( (result == 0) &&
             (!ended) )
        {
            ++fetched;
        }
    }

    // a failure ends the run, read to its end only where rows remain
    if (result != 0)
    {
        (void)d_internal_mariadb_run_end(_statement, !ended);
        d_internal_mariadb_replied(db, false);
        d_internal_db_rows_release(&builder);

        return result;
    }

    // the run's end: every result after its rows, and the reply's end
    if (ended)
    {
        result = d_internal_mariadb_run_end(_statement, true);

        d_internal_mariadb_replied(
            db,
            ( (result == 0) &&
              ( (mysql_stmt_field_count(native) == 0u) ||
                (d_internal_mariadb_rows_end_ok(db)) ) ));

        if (result != 0)
        {
            d_internal_db_rows_release(&builder);

            return result;
        }
    }

    // rows that cannot be handed over end the run too, rather than leave a
    // gap in it
    if (!d_internal_db_rows_finish(&builder, _rows))
    {
        if (!ended)
        {
            (void)d_internal_mariadb_run_end(_statement, true);
            d_internal_mariadb_replied(db, false);
        }

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    return 0;
}

/*
d_mariadb_close_cursor
  The client reads the rest of the rows away, and the layer every result
after them; what the session's changes were is not kept.
*/
int
d_mariadb_close_cursor(
    struct d_mariadb_statement* _statement
)
{
    // parameter validation
    if (!_statement)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // no run open, whose connection is still there
    if ( (!_statement->db) ||
         (_statement->db->cursor != _statement) )
    {
        return 0;
    }

    const int result = d_internal_mariadb_run_end(_statement, true);

    d_internal_mariadb_replied(_statement->db, false);

    return result;
}

//==============================================================================
// THE HANDLE'S STATEMENTS (mariadb_internal.h, 2.6)
//==============================================================================

/*
d_internal_mariadb_close_retired
  Each retired statement's client statement, which tells the server, then
the rest of it.
*/
void
d_internal_mariadb_close_retired(
    struct d_mariadb* _db
)
{
    while (_db->retired)
    {
        struct d_mariadb_statement* const statement = _db->retired;

        _db->retired = statement->next;

        (void)mysql_stmt_close(statement->native);
        free(statement);
    }

    return;
}

/*
d_internal_mariadb_detach
  The run first, while its statement still has the connection; then each
statement, which keeps nothing of the connection but finalizing.
*/
void
d_internal_mariadb_detach(
    struct d_mariadb* _db,
    bool              _read_rest
)
{
    // a run still open ends
    if (_db->cursor)
    {
        (void)d_internal_mariadb_run_end(_db->cursor, _read_rest);
    }

    // every statement, without the connection
    while (_db->statements)
    {
        struct d_mariadb_statement* const statement = _db->statements;

        _db->statements     = statement->next;
        statement->db       = NULL;
        statement->previous = NULL;
        statement->next     = NULL;
    }

    return;
}

#else

//==============================================================================
// WITHOUT A CLIENT
//==============================================================================
// Built as stubs (D_CFG_MARIADB 0), each function reports what a build without
// a client library can: D_DB_UNSUPPORTED, or 0 or NULL where it cannot fail.
// A statement a failed prepare promises is still written: NULL.

/*
d_mariadb_prepare
  Without a client there is nothing to compile: D_DB_UNSUPPORTED, and the
statement NULL, as a failed prepare leaves it.
*/
int
d_mariadb_prepare(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _sql
)
{
    (void)_db;
    (void)_sql;

    // a failed prepare leaves NULL behind
    if (_statement)
    {
        *_statement = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_preparef
  As d_mariadb_prepare.
*/
int
d_mariadb_preparef(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _format,
    ...
)
{
    (void)_db;
    (void)_format;

    // a failed prepare leaves NULL behind
    if (_statement)
    {
        *_statement = NULL;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_finalize
  Without a client no statement exists, and releasing none succeeds.
*/
int
d_mariadb_finalize(
    struct d_mariadb_statement* _statement
)
{
    (void)_statement;

    return 0;
}

/*
d_mariadb_parameter_count
  Without a client, 0.
*/
size_t
d_mariadb_parameter_count(
    const struct d_mariadb_statement* _statement
)
{
    (void)_statement;

    return 0u;
}

/*
d_mariadb_statement_connection
  Without a client, NULL.
*/
struct d_mariadb*
d_mariadb_statement_connection(
    const struct d_mariadb_statement* _statement
)
{
    (void)_statement;

    return NULL;
}

/*
d_mariadb_bind_null
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_bind_null(
    struct d_mariadb_statement* _statement,
    size_t                      _index
)
{
    (void)_statement;
    (void)_index;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_bind_bool
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_bind_bool(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    bool                        _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_bind_int64
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_bind_int64(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    int64_t                     _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_bind_double
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_bind_double(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    double                      _value
)
{
    (void)_statement;
    (void)_index;
    (void)_value;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_bind_text
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_bind_text(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    const char*                 _text
)
{
    (void)_statement;
    (void)_index;
    (void)_text;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_bind_blob
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_bind_blob(
    struct d_mariadb_statement* _statement,
    size_t                      _index,
    const void*                 _bytes,
    size_t                      _size
)
{
    (void)_statement;
    (void)_index;
    (void)_bytes;
    (void)_size;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_clear_bindings
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_clear_bindings(
    struct d_mariadb_statement* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_execute_update
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_execute_update(
    struct d_mariadb_statement* _statement,
    int64_t*                    _changes
)
{
    (void)_statement;
    (void)_changes;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_execute_query
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_execute_query(
    struct d_mariadb_statement* _statement,
    struct d_db_rows*           _rows
)
{
    (void)_statement;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_open_cursor
  Without a client there is nothing to run: D_DB_UNSUPPORTED.
*/
int
d_mariadb_open_cursor(
    struct d_mariadb_statement* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_fetch_rows
  Without a client there are no rows: D_DB_UNSUPPORTED, and the rows left as
they were.
*/
int
d_mariadb_fetch_rows(
    struct d_mariadb_statement* _statement,
    size_t                      _max,
    struct d_db_rows*           _rows
)
{
    (void)_statement;
    (void)_max;
    (void)_rows;

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_close_cursor
  Without a client there is no run to end: D_DB_UNSUPPORTED.
*/
int
d_mariadb_close_cursor(
    struct d_mariadb_statement* _statement
)
{
    (void)_statement;

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MARIADB
