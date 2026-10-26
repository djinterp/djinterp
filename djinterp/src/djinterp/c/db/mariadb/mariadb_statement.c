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
*   The calls that do not block (MariaDB's client alone) are operations in
* the handle's record (mariadb_internal.h 2.7): the record names the
* statement whose call it is, keeps the rows read so far between waits, and
* each stage begins the next client call. A statement whose call is under way
* cannot be closed -- the client's suspended call still uses it -- so one
* finalized meanwhile is retired, its call abandoned, and closed with the
* connection.
*
*
* path:      /src/djinterp/c/db/mariadb/mariadb_statement.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.10.25
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
    bool                             prepared;  // compiled by the server; a
                                                // prepare that did not block
                                                // may have failed
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
 * @brief Whether a call of the statement's own that does not block is under
 *        way, which reads its values and its client statement as it goes.
 *
 * @param[in] _statement  the statement; not `NULL`.
 * @return whether it is.
 */
static bool
d_internal_mariadb_own_call(
    const struct d_mariadb_statement* _statement
)
{
    return ( (_statement->db)                              &&
             (_statement->db->async)                       &&
             (_statement->db->async->advance)              &&
             (_statement->db->async->statement == _statement) );
}

/**
 * @brief Finds the value a parameter's number names.
 *
 * @param[in]  _statement  the statement.
 * @param[in]  _index      the number, from 1.
 * @param[out] _value      receives the value.
 * @return 0, D_INTERNAL_MARIADB_INVALID_PARAMETER for a number the statement
 *         does not have, D_INTERNAL_MARIADB_OUT_OF_SYNC while a call of its
 *         own that does not block reads the values, or
 *         D_DB_INVALID_ARGUMENT for a `NULL` statement.
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

    // the values a call under way is sending
    if (d_internal_mariadb_own_call(_statement))
    {
        return D_INTERNAL_MARIADB_OUT_OF_SYNC;
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
 * @brief Takes the row a fetch of a result brought, its binds already given
 *        to the client, into rows under construction.
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
 * @param[in]     _fetched  what the client's fetch answered.
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
d_internal_mariadb_take_row(
    MYSQL_STMT*                       _native,
    MYSQL*                            _connection,
    const MYSQL_FIELD*                _fields,
    unsigned                          _count,
    struct d_internal_mariadb_column* _columns,
    MYSQL_BIND*                       _binds,
    struct d_internal_db_rows*        _rows,
    int                               _fetched,
    bool*                             _ended
)
{
    const int fetched = _fetched;
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
 * @brief Fetches the next row of a result into rows under construction, as
 *        d_internal_mariadb_take_row takes it.
 *
 * @return as d_internal_mariadb_take_row.
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
    return d_internal_mariadb_take_row(_native,
                                       _connection,
                                       _fields,
                                       _count,
                                       _columns,
                                       _binds,
                                       _rows,
                                       mysql_stmt_fetch(_native),
                                       _ended);
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
 * @brief Begins the run whose rows a statement that just ran gives a few at
 *        a time: what the rows are, and where each value goes, given to the
 *        client.
 *
 * @param[in,out] _statement  the statement, its result unstored, its
 *                            connection's run.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_MEMORY; or the client's error number
 *         for a failure. A failure leaves the run to be ended.
 */
static int
d_internal_mariadb_run_begin(
    struct d_mariadb_statement* _statement
)
{
    MYSQL_STMT* const                    native = _statement->native;
    struct d_internal_mariadb_run* const run    = &_statement->run;
    int                                  result = 0;

    // a statement without rows has none to describe
    if (mysql_stmt_field_count(native) == 0u)
    {
        return 0;
    }

    run->metadata = mysql_stmt_result_metadata(native);

    if (!run->metadata)
    {
        const unsigned error = mysql_stmt_errno(native);

        return (error != 0u) ? (int)error : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

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
 * @brief Builds the client's binds from a statement's values, and gives
 *        them to the client, which copies them.
 *
 * @param[in,out] _statement  the statement.
 * @return 0, or the client's error number for binds it refused.
 */
static int
d_internal_mariadb_bind_values(
    struct d_mariadb_statement* _statement
)
{
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

    // the binds, which the client copies
    if ( (_statement->count > 0u) &&
         (mysql_stmt_bind_param(_statement->native, _statement->binds)) )
    {
        return (int)mysql_stmt_errno(_statement->native);
    }

    return 0;
}

/**
 * @brief Runs a statement with its values: builds the client's binds from
 *        them, and executes.
 *
 * @param[in,out] _statement  the statement, its connection still there.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_SYNC while another statement's run,
 *         or a call that does not block, holds the connection; or the
 *         client's error number for the failure.
 *
 * The statement's own run, if it has one open, ends first, read to its end,
 * unless a call that does not block holds the connection. What a failure
 * left on the connection is undone next, as before any statement the layer
 * runs (d_internal_mariadb_ready).
 */
static int
d_internal_mariadb_execute(
    struct d_mariadb_statement* _statement
)
{
    MYSQL_STMT* const native = _statement->native;

    // a statement whose prepare failed has nothing to run
    if (!_statement->prepared)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // nothing is read while a call that does not block is under way
    if (d_internal_mariadb_busy(_statement->db))
    {
        return D_INTERNAL_MARIADB_OUT_OF_SYNC;
    }

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

    const int bound = d_internal_mariadb_bind_values(_statement);

    // the binds, then the statement
    if (bound != 0)
    {
        return bound;
    }

    if (mysql_stmt_execute(native) != 0)
    {
        return (int)mysql_stmt_errno(native);
    }

    return 0;
}

/**
 * @brief Reads a statement's stored result into rows under construction.
 *
 * @param[in]     _statement  the statement, its first result stored.
 * @param[in,out] _rows       the builder, empty.
 * @return 0; D_INTERNAL_MARIADB_OUT_OF_MEMORY; or the client's error number
 *         for a failure.
 */
static int
d_internal_mariadb_stored_rows(
    struct d_mariadb_statement* _statement,
    struct d_internal_db_rows*  _rows
)
{
    MYSQL_STMT* const native   = _statement->native;
    MYSQL_RES* const  metadata = mysql_stmt_result_metadata(native);

    // what the rows are
    if (!metadata)
    {
        return (int)mysql_stmt_errno(native);
    }

    const unsigned           count  = mysql_num_fields(metadata);
    const MYSQL_FIELD* const fields = mysql_fetch_fields(metadata);
    const int                result =
        (d_internal_mariadb_rows_columns(_rows, fields, count))
            ? d_internal_mariadb_fetch_rows(native,
                                            _statement->db->native,
                                            fields,
                                            count,
                                            _rows)
            : D_INTERNAL_MARIADB_OUT_OF_MEMORY;

    mysql_free_result(metadata);

    return result;
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
        changes = (int64_t)mysql_stmt_num_rows(native);

        // the rows, when they are wanted
        if (_rows)
        {
            result = d_internal_mariadb_stored_rows(_statement, _rows);
        }

        // the stored rows
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
 * @brief Makes the layer's statement around a new client statement, its
 *        values none, in no list.
 *
 * @param[in] _db  the connection.
 * @return the statement, or `NULL` when memory ran out.
 */
static struct d_mariadb_statement*
d_internal_mariadb_statement_make(
    struct d_mariadb* _db
)
{
    struct d_mariadb_statement* const made =
        (struct d_mariadb_statement*)malloc(sizeof(*made));

    // allocation failure
    if (!made)
    {
        return NULL;
    }

    made->db           = _db;
    made->native       = mysql_stmt_init(_db->native);
    made->prepared     = false;
    made->count        = 0u;
    made->binds        = NULL;
    made->values       = NULL;
    made->previous     = NULL;
    made->next         = NULL;
    made->run.metadata = NULL;
    made->run.count    = 0u;
    made->run.columns  = NULL;
    made->run.binds    = NULL;

    // the client's statement
    if (!made->native)
    {
        free(made);

        return NULL;
    }

    return made;
}

/**
 * @brief Lists a statement with its handle, which closing leaves without a
 *        connection.
 *
 * @param[in,out] _statement  the statement, in no list.
 */
static void
d_internal_mariadb_link(
    struct d_mariadb_statement* _statement
)
{
    struct d_mariadb* const db = _statement->db;

    _statement->next = db->statements;

    if (db->statements)
    {
        db->statements->previous = _statement;
    }

    db->statements = _statement;

    return;
}

/**
 * @brief Readies a statement the server compiled: its longest values
 *        measured when a result is stored, and room for its parameters'
 *        values, each NULL to start.
 *
 * @param[in,out] _statement  the statement, compiled.
 * @return 0, or D_INTERNAL_MARIADB_OUT_OF_MEMORY, the statement then with no
 *         parameters.
 */
static int
d_internal_mariadb_room(
    struct d_mariadb_statement* _statement
)
{
    // the longest value of each column, measured when a result is stored
    d_internal_mariadb_flag measure = 1;

    (void)mysql_stmt_attr_set(_statement->native,
                              STMT_ATTR_UPDATE_MAX_LENGTH,
                              &measure);

    const size_t count = (size_t)mysql_stmt_param_count(_statement->native);

    _statement->prepared = true;

    // none to hold
    if (count == 0u)
    {
        return 0;
    }

    _statement->binds  = (MYSQL_BIND*)malloc(count * sizeof(MYSQL_BIND));
    _statement->values = (struct d_internal_mariadb_value*)malloc(
                             count * sizeof(struct d_internal_mariadb_value));

    if ( (!_statement->binds) ||
         (!_statement->values) )
    {
        free(_statement->binds);
        free(_statement->values);

        _statement->binds  = NULL;
        _statement->values = NULL;

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    _statement->count = count;

    for (size_t i = 0u; i < count; ++i)
    {
        _statement->values[i].bytes = NULL;

        d_internal_mariadb_value_clear(&_statement->values[i]);
    }

    return 0;
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
        d_internal_mariadb_statement_make(_db);

    // allocation failure
    if (!made)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    // the client's statement, compiled by the server
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
    d_internal_mariadb_link(made);

    // room for the parameters' values
    if (d_internal_mariadb_room(made) != 0)
    {
        (void)d_mariadb_finalize(made);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    *_statement = made;

    return 0;
}

#if (D_INTERNAL_MARIADB_ASYNC == 2)

// D_INTERNAL_MARIADB_GIVES_NONE, D_INTERNAL_MARIADB_GIVES_COUNT,
// D_INTERNAL_MARIADB_GIVES_ROWS
//   constants: what a run that does not block gives, its record's `mode`:
// nothing chosen yet, the count d_mariadb_execute_update gives, or the rows
// d_mariadb_execute_query gives.
#define D_INTERNAL_MARIADB_GIVES_NONE  0
#define D_INTERNAL_MARIADB_GIVES_COUNT 1
#define D_INTERNAL_MARIADB_GIVES_ROWS  2

/**
 * @brief The client's error number for a statement's call that failed, or
 *        the unknown error where it left none.
 *
 * @param[in] _native  the client's statement.
 * @return the number.
 */
static int
d_internal_mariadb_stmt_refused(
    MYSQL_STMT* _native
)
{
    return (mysql_stmt_errno(_native) != 0u)
               ? (int)mysql_stmt_errno(_native)
               : D_INTERNAL_MARIADB_UNKNOWN_ERROR;
}

/**
 * @brief Says to wait for what is there at once: the connection's socket
 *        writable, the client having sent everything it had.
 *
 * @param[in]  _db       the connection.
 * @param[out] _pending  receives the wait.
 * @return D_DB_WAIT.
 */
static int
d_internal_mariadb_at_once(
    struct d_mariadb*    _db,
    struct d_db_pending* _pending
)
{
    const my_socket fd = mysql_get_socket(_db->native);

    _pending->socket     = (fd != (my_socket)-1) ? (d_db_socket)fd
                                                 : D_DB_SOCKET_NONE;
    _pending->events     = (unsigned)D_DB_WAIT_WRITE;
    _pending->timeout_ms = 0u;

    return D_DB_WAIT;
}

/**
 * @brief Ends a statement's call in the record: no statement named, nothing
 *        chosen, no rows held.
 *
 * @param[in,out] _async   the record.
 * @param[in]     _result  the call's result.
 * @return `_result`.
 */
static int
d_internal_mariadb_call_over(
    struct d_internal_mariadb_async* _async,
    int                              _result
)
{
    d_internal_db_rows_release(&_async->page);
    memset(&_async->page, 0, sizeof(_async->page));

    _async->statement = NULL;
    _async->stmt      = NULL;
    _async->mode      = D_INTERNAL_MARIADB_GIVES_NONE;
    _async->rows      = NULL;
    _async->changes   = NULL;
    _async->count     = 0;
    _async->max       = 0u;
    _async->fetched   = 0u;

    return _result;
}

/**
 * @brief Begins a client call of a statement's operation: its stage moved on
 *        first, so that its answer is read at the next.
 *
 * @param[in,out] _db       the connection, an operation under way.
 * @param[in]     _step     the call.
 * @param[in]     _next     the stage that reads its answer.
 * @param[out]    _pending  receives what to wait for.
 * @return 0, finished at once; D_DB_WAIT; or, for a call the client cannot
 *         make, D_DB_UNSUPPORTED, the call over.
 */
static int
d_internal_mariadb_call_step(
    struct d_mariadb*            _db,
    enum d_internal_mariadb_step _step,
    int                          _next,
    struct d_db_pending*         _pending
)
{
    struct d_internal_mariadb_async* const async = _db->async;

    async->stage = _next;

    const int stepped = d_internal_mariadb_step(_db, _step, _pending);

    // waiting, or no such call
    if ( (stepped != 0) &&
         (stepped != D_DB_WAIT) )
    {
        return d_internal_mariadb_call_over(async, stepped);
    }

    return stepped;
}

/**
 * @brief Reads, and discards, every result after the first, as
 *        d_internal_mariadb_drain does, without blocking.
 *
 * @param[in,out] _db       the connection, a statement's call under way, its
 *                          record's `sub` 0 to begin.
 * @param[out]    _pending  receives what to wait for.
 * @return 0, none left; D_DB_WAIT; or the client's error number for a
 *         result that failed.
 *
 * Its place is the record's `sub`: 0 the next result begun; 1 its answer,
 * a result with rows stored next; 2 the stored rows freed, which sends
 * nothing.
 */
static int
d_internal_mariadb_drain_async(
    struct d_mariadb*    _db,
    struct d_db_pending* _pending
)
{
    struct d_internal_mariadb_async* const async  = _db->async;
    MYSQL_STMT* const                      native = async->stmt;

    for (;;)
    {
        enum d_internal_mariadb_step step = D_INTERNAL_MARIADB_STEP_NONE;

        switch (async->sub)
        {
            case 0:
                step = D_INTERNAL_MARIADB_STEP_STMT_NEXT;

                break;

            case 1:
                // none left; a failure; a result, whose rows are stored
                if (async->ret < 0)
                {
                    return 0;
                }

                if (async->ret > 0)
                {
                    return d_internal_mariadb_stmt_refused(native);
                }

                if (mysql_stmt_field_count(native) == 0u)
                {
                    async->sub = 0;

                    continue;
                }

                step = D_INTERNAL_MARIADB_STEP_STMT_STORE;

                break;

            default:
                (void)mysql_stmt_free_result(native);
                async->sub = 0;

                continue;
        }

        ++async->sub;

        const int stepped = d_internal_mariadb_step(_db, step, _pending);

        // waiting, or no such call
        if (stepped != 0)
        {
            return stepped;
        }
    }
}

/**
 * @brief The stages of d_mariadb_prepare_start's operation.
 *
 * @param[in,out] _db       the connection.
 * @param[out]    _pending  receives what to wait for.
 * @return D_DB_WAIT; 0, the statement compiled and readied; or a failure,
 *         the statement left unprepared.
 *
 * 0 and 1, the multi-statement mode off where a failed script left it on;
 * 2, the prepare begun; 3, its answer, a reply that reports no changes.
 */
static int
d_internal_mariadb_prepare_advance(
    struct d_mariadb*    _db,
    struct d_db_pending* _pending
)
{
    struct d_internal_mariadb_async* const async     = _db->async;
    struct d_mariadb_statement* const      statement = async->statement;

    // the mode off first
    if (async->stage < 2)
    {
        const int off = d_internal_mariadb_mode_off(_db, 0, _pending);

        if (off == D_DB_WAIT)
        {
            return D_DB_WAIT;
        }

        if (off != 0)
        {
            return d_internal_mariadb_call_over(async, off);
        }

        async->stage = 2;
    }

    // the prepare begun
    if (async->stage == 2)
    {
        const int stepped = d_internal_mariadb_call_step(
                                _db,
                                D_INTERNAL_MARIADB_STEP_PREPARE,
                                3,
                                _pending);

        if (stepped != 0)
        {
            return stepped;
        }
    }

    // its answer
    d_internal_mariadb_replied(_db, false);

    return d_internal_mariadb_call_over(
               async,
               (async->ret == 0)
                   ? d_internal_mariadb_room(statement)
                   : d_internal_mariadb_stmt_refused(statement->native));
}

/**
 * @brief Gives what a run that does not block read, once every result has
 *        been read: its rows or its count, as its mode chose.
 *
 * @param[in,out] _async    the record.
 * @param[in]     _failure  the run's first failure, or 0.
 * @return 0, `_failure`, or D_INTERNAL_MARIADB_OUT_OF_MEMORY when the rows
 *         could not be handed over.
 */
static int
d_internal_mariadb_give_run(
    struct d_internal_mariadb_async* _async,
    int                              _failure
)
{
    int result = _failure;

    // the rows, or the count where there is somewhere to put it
    if (result == 0)
    {
        if (_async->mode == D_INTERNAL_MARIADB_GIVES_ROWS)
        {
            result = (d_internal_db_rows_finish(&_async->page, _async->rows))
                         ? 0
                         : D_INTERNAL_MARIADB_OUT_OF_MEMORY;
        }
        else if (_async->changes)
        {
            *_async->changes = _async->count;
        }
    }

    return d_internal_mariadb_call_over(_async, result);
}

/**
 * @brief The stages of d_mariadb_execute_start's operation.
 *
 * @param[in,out] _db       the connection.
 * @param[out]    _pending  receives what to wait for.
 * @return D_DB_WAIT; 0, every result read, the first given; or the first
 *         failure, every result read all the same where the statement ran.
 *
 * 0 and 1, the multi-statement mode off; 2, the statement run; 3, its
 * answer, which waits, ready at once, until a continuation says what to
 * give; 4, a result's rows stored; 5, their answer, the rows read where
 * they are wanted, then freed; 6, every result after the first, then the
 * reply's end, as d_mariadb_execute_update and _query read them.
 */
static int
d_internal_mariadb_execute_advance(
    struct d_mariadb*    _db,
    struct d_db_pending* _pending
)
{
    struct d_internal_mariadb_async* const async     = _db->async;
    struct d_mariadb_statement* const      statement = async->statement;
    MYSQL_STMT* const                      native    = async->stmt;

    for (;;)
    {
        switch (async->stage)
        {
            case 0:
            case 1:
            {
                const int off = d_internal_mariadb_mode_off(_db, 0, _pending);

                if (off == D_DB_WAIT)
                {
                    return D_DB_WAIT;
                }

                if (off != 0)
                {
                    return d_internal_mariadb_call_over(async, off);
                }

                async->stage = 2;

                continue;
            }

            case 2:
            {
                const int stepped = d_internal_mariadb_call_step(
                                        _db,
                                        D_INTERNAL_MARIADB_STEP_EXECUTE,
                                        3,
                                        _pending);

                if (stepped != 0)
                {
                    return stepped;
                }

                continue;
            }

            case 3:
                // what to give, still to be said
                if (async->mode == D_INTERNAL_MARIADB_GIVES_NONE)
                {
                    return d_internal_mariadb_at_once(_db, _pending);
                }

                // a statement that did not run has nothing to read
                if (async->ret != 0)
                {
                    d_internal_mariadb_replied(_db, false);

                    return d_internal_mariadb_call_over(
                               async,
                               d_internal_mariadb_stmt_refused(native));
                }

                // a statement without rows counts the rows it changed
                if (mysql_stmt_field_count(native) == 0u)
                {
                    async->count = (int64_t)mysql_stmt_affected_rows(native);
                    async->stage = 6;
                    async->sub   = 0;

                    continue;
                }

                async->stage = 4;

                continue;

            case 4:
            {
                const int stepped = d_internal_mariadb_call_step(
                                        _db,
                                        D_INTERNAL_MARIADB_STEP_STMT_STORE,
                                        5,
                                        _pending);

                if (stepped != 0)
                {
                    return stepped;
                }

                continue;
            }

            case 5:
                // the rows counted, and read where they are wanted
                if (async->ret != 0)
                {
                    async->failure = d_internal_mariadb_stmt_refused(native);
                }
                else
                {
                    async->count = (int64_t)mysql_stmt_num_rows(native);

                    if (async->mode == D_INTERNAL_MARIADB_GIVES_ROWS)
                    {
                        async->failure = d_internal_mariadb_stored_rows(
                                             statement,
                                             &async->page);
                    }
                }

                (void)mysql_stmt_free_result(native);
                async->stage = 6;
                async->sub   = 0;

                continue;

            default:
            {
                const int drained = d_internal_mariadb_drain_async(_db,
                                                                   _pending);

                if (drained == D_DB_WAIT)
                {
                    return D_DB_WAIT;
                }

                const int failure = (async->failure != 0) ? async->failure
                                                          : drained;

                // the reply's end: the last result's, an OK packet unless
                // rows end it without one
                d_internal_mariadb_replied(
                    _db,
                    ( (failure == 0) &&
                      ( (mysql_stmt_field_count(native) == 0u) ||
                        (d_internal_mariadb_rows_end_ok(_db)) ) ));

                return d_internal_mariadb_give_run(async, failure);
            }
        }
    }
}

/**
 * @brief The stages of d_mariadb_open_cursor_start's operation.
 *
 * @param[in,out] _db       the connection.
 * @param[out]    _pending  receives what to wait for.
 * @return D_DB_WAIT; 0, the run open; or a failure, the run not begun.
 *
 * 0 and 1, the multi-statement mode off; 2, the statement run; 3, its
 * answer, the run begun; a run that cannot begin ends, read to its end: 4,
 * the rest of its rows freed; 5, every result after them.
 */
static int
d_internal_mariadb_cursor_advance(
    struct d_mariadb*    _db,
    struct d_db_pending* _pending
)
{
    struct d_internal_mariadb_async* const async     = _db->async;
    struct d_mariadb_statement* const      statement = async->statement;

    for (;;)
    {
        switch (async->stage)
        {
            case 0:
            case 1:
            {
                const int off = d_internal_mariadb_mode_off(_db, 0, _pending);

                if (off == D_DB_WAIT)
                {
                    return D_DB_WAIT;
                }

                if (off != 0)
                {
                    return d_internal_mariadb_call_over(async, off);
                }

                async->stage = 2;

                continue;
            }

            case 2:
            {
                const int stepped = d_internal_mariadb_call_step(
                                        _db,
                                        D_INTERNAL_MARIADB_STEP_EXECUTE,
                                        3,
                                        _pending);

                if (stepped != 0)
                {
                    return stepped;
                }

                continue;
            }

            case 3:
            {
                // a statement that did not run holds nothing
                if (async->ret != 0)
                {
                    d_internal_mariadb_replied(_db, false);

                    return d_internal_mariadb_call_over(
                               async,
                               d_internal_mariadb_stmt_refused(async->stmt));
                }

                // the connection the run's from here until it ends
                _db->cursor = statement;

                const int begun = d_internal_mariadb_run_begin(statement);

                if (begun == 0)
                {
                    return d_internal_mariadb_call_over(async, 0);
                }

                async->failure = begun;

                const int stepped = d_internal_mariadb_call_step(
                                        _db,
                                        D_INTERNAL_MARIADB_STEP_STMT_FREE,
                                        4,
                                        _pending);

                if (stepped != 0)
                {
                    return stepped;
                }

                continue;
            }

            case 4:
                async->stage = 5;
                async->sub   = 0;

                continue;

            default:
            {
                const int drained = d_internal_mariadb_drain_async(_db,
                                                                   _pending);

                if (drained == D_DB_WAIT)
                {
                    return D_DB_WAIT;
                }

                (void)d_internal_mariadb_run_end(statement, false);
                d_internal_mariadb_replied(_db, false);

                return d_internal_mariadb_call_over(async, async->failure);
            }
        }
    }
}

/**
 * @brief The stages of d_mariadb_fetch_rows_start's operation.
 *
 * @param[in,out] _db       the connection.
 * @param[out]    _pending  receives what to wait for.
 * @return D_DB_WAIT; 0, the page given; or a failure, which ends the run.
 *
 * 0, the page given once whole, or the next row fetched; 1, the row taken;
 * at the run's end, 2, its rows freed, which sends nothing now, and 3, every
 * result after them, then the page given, as d_mariadb_fetch_rows ends a
 * run. A failure with rows still to come ends the run read to its end: 4,
 * the rest freed; 5, every result after them.
 */
static int
d_internal_mariadb_fetch_advance(
    struct d_mariadb*    _db,
    struct d_db_pending* _pending
)
{
    struct d_internal_mariadb_async* const async     = _db->async;
    struct d_mariadb_statement* const      statement = async->statement;
    MYSQL_STMT* const                      native    = async->stmt;
    struct d_internal_mariadb_run* const   run       = &statement->run;
    const MYSQL_FIELD* const               fields    =
        (run->metadata) ? mysql_fetch_fields(run->metadata) : NULL;

    for (;;)
    {
        int stepped = 0;

        switch (async->stage)
        {
            case 0:
                // a statement without rows ends at its first fetch
                if (run->count == 0u)
                {
                    stepped = d_internal_mariadb_call_step(
                                  _db,
                                  D_INTERNAL_MARIADB_STEP_STMT_FREE,
                                  3,
                                  _pending);

                    break;
                }

                // the page whole: given, the run still open; or, the rows
                // not handed over, ended
                if (async->fetched >= async->max)
                {
                    if (d_internal_db_rows_finish(&async->page, async->rows))
                    {
                        return d_internal_mariadb_call_over(async, 0);
                    }

                    async->failure = D_INTERNAL_MARIADB_OUT_OF_MEMORY;
                    stepped        = d_internal_mariadb_call_step(
                                         _db,
                                         D_INTERNAL_MARIADB_STEP_STMT_FREE,
                                         5,
                                         _pending);

                    break;
                }

                stepped = d_internal_mariadb_call_step(
                              _db,
                              D_INTERNAL_MARIADB_STEP_FETCH,
                              1,
                              _pending);

                break;

            case 1:
            {
                bool      ended = false;
                const int taken = d_internal_mariadb_take_row(
                                      native,
                                      _db->native,
                                      fields,
                                      run->count,
                                      run->columns,
                                      run->binds,
                                      &async->page,
                                      async->ret,
                                      &ended);

                // a failure ends the run, read to its end where rows remain
                if (taken != 0)
                {
                    async->failure = taken;

                    if (ended)
                    {
                        (void)d_internal_mariadb_run_end(statement, false);
                        d_internal_mariadb_replied(_db, false);

                        return d_internal_mariadb_call_over(async, taken);
                    }

                    stepped = d_internal_mariadb_call_step(
                                  _db,
                                  D_INTERNAL_MARIADB_STEP_STMT_FREE,
                                  5,
                                  _pending);

                    break;
                }

                // the run's end, or the next row
                if (ended)
                {
                    stepped = d_internal_mariadb_call_step(
                                  _db,
                                  D_INTERNAL_MARIADB_STEP_STMT_FREE,
                                  3,
                                  _pending);

                    break;
                }

                ++async->fetched;
                async->stage = 0;

                continue;
            }

            case 3:
                async->stage = 4;
                async->sub   = 0;

                continue;

            case 4:
            {
                // every result after its rows, then the reply's end
                const int drained = d_internal_mariadb_drain_async(_db,
                                                                   _pending);

                if (drained == D_DB_WAIT)
                {
                    return D_DB_WAIT;
                }

                (void)d_internal_mariadb_run_end(statement, false);
                d_internal_mariadb_replied(
                    _db,
                    ( (drained == 0) &&
                      ( (mysql_stmt_field_count(native) == 0u) ||
                        (d_internal_mariadb_rows_end_ok(_db)) ) ));

                if (drained != 0)
                {
                    return d_internal_mariadb_call_over(async, drained);
                }

                return d_internal_mariadb_call_over(
                           async,
                           (d_internal_db_rows_finish(&async->page,
                                                      async->rows))
                               ? 0
                               : D_INTERNAL_MARIADB_OUT_OF_MEMORY);
            }

            case 5:
                async->stage = 6;
                async->sub   = 0;

                continue;

            default:
            {
                // a failure's end: every result after the rows read away
                const int drained = d_internal_mariadb_drain_async(_db,
                                                                   _pending);

                if (drained == D_DB_WAIT)
                {
                    return D_DB_WAIT;
                }

                (void)d_internal_mariadb_run_end(statement, false);
                d_internal_mariadb_replied(_db, false);

                return d_internal_mariadb_call_over(async, async->failure);
            }
        }

        // the stage's call, waiting, or finished at once
        if (stepped != 0)
        {
            return stepped;
        }
    }
}

/**
 * @brief Finds a statement's call under way in its connection's record.
 *
 * @param[in] _statement  the statement; may be `NULL`.
 * @param[in] _advance    the operation it must be.
 * @param[in] _pending    the pending call; may be `NULL`.
 * @return the record, or `NULL` for a `NULL` argument, or no such call of
 *         the statement's under way.
 */
static struct d_internal_mariadb_async*
d_internal_mariadb_call_of(
    const struct d_mariadb_statement* _statement,
    int                             (*_advance)(struct d_mariadb*,
                                                struct d_db_pending*),
    const struct d_db_pending*        _pending
)
{
    // parameter validation
    if ( (!_statement) ||
         (!_pending)   ||
         (!d_internal_mariadb_own_call(_statement)) )
    {
        return NULL;
    }

    return (_statement->db->async->advance == _advance)
               ? _statement->db->async
               : NULL;
}

/**
 * @brief Begins a statement's run that does not block: the connection
 *        tidied as before any command, the values given to the client, the
 *        record told whose call it is.
 *
 * @param[in,out] _statement  the statement, its connection still there.
 * @param[in]     _advance    the operation.
 * @param[out]    _pending    receives what to wait for.
 * @return as the operation's start answers; 2014 while the connection is
 *         held; D_INTERNAL_MARIADB_OUT_OF_MEMORY; or the client's error
 *         number for values it refused.
 */
static int
d_internal_mariadb_run_start(
    struct d_mariadb_statement* _statement,
    int                       (*_advance)(struct d_mariadb*,
                                          struct d_db_pending*),
    struct d_db_pending*        _pending
)
{
    struct d_mariadb* const db = _statement->db;

    // a statement whose prepare failed has nothing to run
    if (!_statement->prepared)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int tidied = d_internal_mariadb_tidy(db);

    // the connection free, as for any command
    if (tidied != 0)
    {
        return tidied;
    }

    struct d_internal_mariadb_async* const async =
        d_internal_mariadb_async_get(db);

    if (!async)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    const int bound = d_internal_mariadb_bind_values(_statement);

    if (bound != 0)
    {
        return bound;
    }

    (void)d_internal_mariadb_call_over(async, 0);
    async->statement = _statement;
    async->stmt      = _statement->native;

    return d_internal_mariadb_begin(db, _advance, _pending);
}

#endif  // D_INTERNAL_MARIADB_ASYNC == 2

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
the client's statement would read that run's rows away, and while a call
that does not block is under way it would write in the middle of that
call's exchange, so the handle keeps it, to be closed once the connection is
free; the rest goes at once. A call of its own is abandoned: the handle
keeps the client's statement until the connection closes.
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
        // its own call that does not block, abandoned: the client's suspended
        // call still uses the client's statement, which the connection keeps
        // until it closes; the run's buffers go, read no further
        if (d_internal_mariadb_own_call(_statement))
        {
            if (db->cursor == _statement)
            {
                (void)d_internal_mariadb_run_end(_statement, false);
            }

            db->async->statement = NULL;
            d_internal_mariadb_unlink(_statement);
            _statement->next = db->retired;
            db->retired      = _statement;

            return 0;
        }

        // its own run, read to its end
        if (db->cursor == _statement)
        {
            (void)d_internal_mariadb_run_end(_statement, true);
            d_internal_mariadb_replied(db, false);
        }

        d_internal_mariadb_unlink(_statement);

        // another's run, or a call that does not block, holds the connection:
        // retired, the server told later
        if ( (db->cursor) ||
             (d_internal_mariadb_busy(db)) )
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

// 1.4    Preparing without blocking
//------------------------------------------------------------------------------
/*
d_mariadb_prepare_start
  The statement made and listed first, so that it is the caller's however the
prepare ends. One that fails at once goes, its client statement kept, as
d_mariadb_prepare keeps it, for the error the caller is about to read.
*/
int
d_mariadb_prepare_start(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _sql,
    struct d_db_pending*         _pending
)
{
    // nothing yet, whatever follows
    if (_pending)
    {
        d_internal_mariadb_unpend(_pending);
    }

    if (_statement)
    {
        *_statement = NULL;
    }

    // parameter validation
    if ( (!_db)        ||
         (!_statement) ||
         (!_sql)       ||
         (!_pending) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

#if (D_INTERNAL_MARIADB_ASYNC == 2)
    // TLS the client cannot carry without blocking
    if (d_internal_mariadb_tls_blocks(_db))
    {
        return D_DB_UNSUPPORTED;
    }

    const size_t length = strlen(_sql);

    // a length the client's unsigned long cannot carry
    if (length > D_INTERNAL_MARIADB_LENGTH_MAX)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    const int tidied = d_internal_mariadb_tidy(_db);

    // the connection free, as for any command
    if (tidied != 0)
    {
        return tidied;
    }

    struct d_internal_mariadb_async* const async =
        d_internal_mariadb_async_get(_db);
    struct d_mariadb_statement* const      made  =
        (async) ? d_internal_mariadb_statement_make(_db) : NULL;

    // the record, the statement, and the copy of the text
    if (!made)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    (void)d_internal_mariadb_call_over(async, 0);

    if (!d_internal_mariadb_keep_text(&async->text, _sql))
    {
        (void)mysql_stmt_close(made->native);
        free(made);

        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    d_internal_mariadb_link(made);
    async->length    = (unsigned long)length;
    async->statement = made;
    async->stmt      = made->native;

    const int result = d_internal_mariadb_begin(
                           _db,
                           d_internal_mariadb_prepare_advance,
                           _pending);

    // a failure at once leaves no statement
    if ( (result != 0) &&
         (result != D_DB_WAIT) )
    {
        d_internal_mariadb_unlink(made);
        _db->failed = made->native;
        free(made);

        return result;
    }

    *_statement = made;

    return result;
#else
    return D_DB_UNSUPPORTED;
#endif  // D_INTERNAL_MARIADB_ASYNC
}

/*
d_mariadb_prepare_continue
  The prepare driven.
*/
int
d_mariadb_prepare_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending
)
{
#if (D_INTERNAL_MARIADB_ASYNC == 2)
    // parameter validation: a prepare of the statement's under way
    if (!d_internal_mariadb_call_of(_statement,
                                    d_internal_mariadb_prepare_advance,
                                    _pending))
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_drive(_statement->db, _events, _pending);
#else
    (void)_statement;
    (void)_events;

    if (_pending)
    {
        d_internal_mariadb_unpend(_pending);
    }

    return D_DB_UNSUPPORTED;
#endif  // D_INTERNAL_MARIADB_ASYNC
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

    // the values a call under way is sending
    if (d_internal_mariadb_own_call(_statement))
    {
        return D_INTERNAL_MARIADB_OUT_OF_SYNC;
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

    struct d_mariadb* const db     = _statement->db;
    int                     result = d_internal_mariadb_execute(_statement);

    // a statement that did not run holds nothing
    if (result != 0)
    {
        d_internal_mariadb_replied(db, false);

        return result;
    }

    // the connection is the run's from here until it ends; rows to come:
    // what they are, and where each value goes
    db->cursor = _statement;
    result     = d_internal_mariadb_run_begin(_statement);

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

    // a page that does not block, under way
    if (d_internal_mariadb_busy(_statement->db))
    {
        return D_INTERNAL_MARIADB_OUT_OF_SYNC;
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

    // a page that does not block, under way
    if (d_internal_mariadb_busy(_statement->db))
    {
        return D_INTERNAL_MARIADB_OUT_OF_SYNC;
    }

    const int result = d_internal_mariadb_run_end(_statement, true);

    d_internal_mariadb_replied(_statement->db, false);

    return result;
}

// 3.4    Running without blocking
//------------------------------------------------------------------------------
/*
d_internal_mariadb_starting
  What every run's start checks: the arguments, nothing pending first, the
statement's connection still there, and a client that can.
*/
static int
d_internal_mariadb_starting(
    struct d_mariadb_statement* _statement,
    struct d_db_pending*        _pending
)
{
    // nothing pending yet, whatever follows
    if (_pending)
    {
        d_internal_mariadb_unpend(_pending);
    }

    // parameter validation; a statement whose connection closed cannot run
    if ( (!_statement)     ||
         (!_pending)       ||
         (!_statement->db) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // a client that can, over the connection's TLS
    return ( (D_INTERNAL_MARIADB_ASYNC == 2) &&
             (!d_internal_mariadb_tls_blocks(_statement->db)) )
               ? 0
               : D_DB_UNSUPPORTED;
}

#if (D_INTERNAL_MARIADB_ASYNC != 2)

/*
d_internal_mariadb_continuing
  What a continuation answers where the client has no statement calls that
do not block: nothing pending, D_DB_UNSUPPORTED.
*/
static int
d_internal_mariadb_continuing(
    struct d_db_pending* _pending
)
{
    if (_pending)
    {
        d_internal_mariadb_unpend(_pending);
    }

    return D_DB_UNSUPPORTED;
}

#endif  // D_INTERNAL_MARIADB_ASYNC != 2

/*
d_mariadb_execute_start
  Sent, its answer read where it comes at once; either way D_DB_WAIT, the
continuation saying what to give.
*/
int
d_mariadb_execute_start(
    struct d_mariadb_statement* _statement,
    struct d_db_pending*        _pending
)
{
    const int checked = d_internal_mariadb_starting(_statement, _pending);

    if (checked != 0)
    {
        return checked;
    }

#if (D_INTERNAL_MARIADB_ASYNC == 2)
    return d_internal_mariadb_run_start(_statement,
                                        d_internal_mariadb_execute_advance,
                                        _pending);
#else
    return D_DB_UNSUPPORTED;
#endif  // D_INTERNAL_MARIADB_ASYNC
}

#if (D_INTERNAL_MARIADB_ASYNC == 2)

/*
d_internal_mariadb_execute_continue
  A continuation's checks: a run of the statement's sent, its results read
for what `_gives` says, which the first continuation chooses.
*/
static int
d_internal_mariadb_execute_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending,
    int                         _gives,
    struct d_db_rows*           _rows,
    int64_t*                    _changes
)
{
    struct d_internal_mariadb_async* const async =
        d_internal_mariadb_call_of(_statement,
                                   d_internal_mariadb_execute_advance,
                                   _pending);

    // parameter validation: a run sent, read for the same thing
    if ( (!async) ||
         ( (async->mode != D_INTERNAL_MARIADB_GIVES_NONE) &&
           (async->mode != _gives) ) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    async->mode    = _gives;
    async->rows    = _rows;
    async->changes = _changes;

    return d_internal_mariadb_drive(_statement->db, _events, _pending);
}

#endif  // D_INTERNAL_MARIADB_ASYNC == 2

/*
d_mariadb_execute_update_continue
  The results read for their count.
*/
int
d_mariadb_execute_update_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending,
    int64_t*                    _changes
)
{
#if (D_INTERNAL_MARIADB_ASYNC == 2)
    return d_internal_mariadb_execute_continue(
               _statement,
               _events,
               _pending,
               D_INTERNAL_MARIADB_GIVES_COUNT,
               NULL,
               _changes);
#else
    (void)_statement;
    (void)_events;
    (void)_changes;

    return d_internal_mariadb_continuing(_pending);
#endif  // D_INTERNAL_MARIADB_ASYNC
}

/*
d_mariadb_execute_query_continue
  The results read for the first's rows.
*/
int
d_mariadb_execute_query_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending,
    struct d_db_rows*           _rows
)
{
#if (D_INTERNAL_MARIADB_ASYNC == 2)
    // parameter validation: somewhere to put the rows
    if (!_rows)
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_execute_continue(_statement,
                                               _events,
                                               _pending,
                                               D_INTERNAL_MARIADB_GIVES_ROWS,
                                               _rows,
                                               NULL);
#else
    (void)_statement;
    (void)_events;
    (void)_rows;

    return d_internal_mariadb_continuing(_pending);
#endif  // D_INTERNAL_MARIADB_ASYNC
}

/*
d_mariadb_open_cursor_start
  Run, its result left unstored, as d_mariadb_open_cursor runs it.
*/
int
d_mariadb_open_cursor_start(
    struct d_mariadb_statement* _statement,
    struct d_db_pending*        _pending
)
{
    const int checked = d_internal_mariadb_starting(_statement, _pending);

    if (checked != 0)
    {
        return checked;
    }

#if (D_INTERNAL_MARIADB_ASYNC == 2)
    return d_internal_mariadb_run_start(_statement,
                                        d_internal_mariadb_cursor_advance,
                                        _pending);
#else
    return D_DB_UNSUPPORTED;
#endif  // D_INTERNAL_MARIADB_ASYNC
}

/*
d_mariadb_open_cursor_continue
  The opening driven.
*/
int
d_mariadb_open_cursor_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending
)
{
#if (D_INTERNAL_MARIADB_ASYNC == 2)
    // parameter validation: a run of the statement's being opened
    if (!d_internal_mariadb_call_of(_statement,
                                    d_internal_mariadb_cursor_advance,
                                    _pending))
    {
        return D_DB_INVALID_ARGUMENT;
    }

    return d_internal_mariadb_drive(_statement->db, _events, _pending);
#else
    (void)_statement;
    (void)_events;

    return d_internal_mariadb_continuing(_pending);
#endif  // D_INTERNAL_MARIADB_ASYNC
}

/*
d_mariadb_fetch_rows_start
  The page's columns described first, as d_mariadb_fetch_rows describes
them, so that each page stands alone; then a row at a time.
*/
int
d_mariadb_fetch_rows_start(
    struct d_mariadb_statement* _statement,
    size_t                      _max,
    struct d_db_pending*        _pending,
    struct d_db_rows*           _rows
)
{
    // nothing pending yet, whatever follows
    if (_pending)
    {
        d_internal_mariadb_unpend(_pending);
    }

    // parameter validation; only a run still open has rows to give
    if ( (!_statement)                     ||
         (!_pending)                       ||
         (!_rows)                          ||
         (_max == 0u)                      ||
         (!_statement->db)                 ||
         (_statement->db->cursor != _statement) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

#if (D_INTERNAL_MARIADB_ASYNC == 2)
    struct d_mariadb* const db = _statement->db;

    // TLS the client cannot carry without blocking; a call that does not
    // block holds the connection
    if (d_internal_mariadb_tls_blocks(db))
    {
        return D_DB_UNSUPPORTED;
    }

    if (d_internal_mariadb_busy(db))
    {
        return D_INTERNAL_MARIADB_OUT_OF_SYNC;
    }

    struct d_internal_mariadb_async* const async =
        d_internal_mariadb_async_get(db);
    struct d_internal_mariadb_run* const   run   = &_statement->run;

    if (!async)
    {
        return D_INTERNAL_MARIADB_OUT_OF_MEMORY;
    }

    (void)d_internal_mariadb_call_over(async, 0);

    // the page's columns
    if (!d_internal_mariadb_rows_columns(
             &async->page,
             (run->metadata) ? mysql_fetch_fields(run->metadata) : NULL,
             run->count))
    {
        return d_internal_mariadb_call_over(async,
                                            D_INTERNAL_MARIADB_OUT_OF_MEMORY);
    }

    async->statement = _statement;
    async->stmt      = _statement->native;
    async->rows      = _rows;
    async->max       = _max;

    return d_internal_mariadb_begin(db,
                                    d_internal_mariadb_fetch_advance,
                                    _pending);
#else
    return D_DB_UNSUPPORTED;
#endif  // D_INTERNAL_MARIADB_ASYNC
}

/*
d_mariadb_fetch_rows_continue
  The page driven, given into the rows this call was given.
*/
int
d_mariadb_fetch_rows_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending,
    struct d_db_rows*           _rows
)
{
#if (D_INTERNAL_MARIADB_ASYNC == 2)
    struct d_internal_mariadb_async* const async =
        d_internal_mariadb_call_of(_statement,
                                   d_internal_mariadb_fetch_advance,
                                   _pending);

    // parameter validation: a page of the statement's under way, and
    // somewhere to put it
    if ( (!async) ||
         (!_rows) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    async->rows = _rows;

    return d_internal_mariadb_drive(_statement->db, _events, _pending);
#else
    (void)_statement;
    (void)_events;
    (void)_rows;

    return d_internal_mariadb_continuing(_pending);
#endif  // D_INTERNAL_MARIADB_ASYNC
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

/*
d_internal_mariadb_nothing_pending
  What a call that does not block leaves without a client: nothing pending.
D_DB_UNSUPPORTED.
*/
static int
d_internal_mariadb_nothing_pending(
    struct d_db_pending* _pending
)
{
    if (_pending)
    {
        _pending->socket     = D_DB_SOCKET_NONE;
        _pending->events     = 0u;
        _pending->timeout_ms = 0u;
        _pending->deadline   = 0.0;
        _pending->state      = 0u;
    }

    return D_DB_UNSUPPORTED;
}

/*
d_mariadb_prepare_start
  Without a client there is nothing to compile: D_DB_UNSUPPORTED, the
statement NULL.
*/
int
d_mariadb_prepare_start(
    struct d_mariadb*            _db,
    struct d_mariadb_statement** _statement,
    const char*                  _sql,
    struct d_db_pending*         _pending
)
{
    (void)_db;
    (void)_sql;

    // a failed start leaves NULL behind
    if (_statement)
    {
        *_statement = NULL;
    }

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_prepare_continue
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_prepare_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending
)
{
    (void)_statement;
    (void)_events;

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_execute_start
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_execute_start(
    struct d_mariadb_statement* _statement,
    struct d_db_pending*        _pending
)
{
    (void)_statement;

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_execute_update_continue
  Without a client, D_DB_UNSUPPORTED, the count left as it was.
*/
int
d_mariadb_execute_update_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending,
    int64_t*                    _changes
)
{
    (void)_statement;
    (void)_events;
    (void)_changes;

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_execute_query_continue
  Without a client, D_DB_UNSUPPORTED, the rows left as they were.
*/
int
d_mariadb_execute_query_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending,
    struct d_db_rows*           _rows
)
{
    (void)_statement;
    (void)_events;
    (void)_rows;

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_open_cursor_start
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_open_cursor_start(
    struct d_mariadb_statement* _statement,
    struct d_db_pending*        _pending
)
{
    (void)_statement;

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_open_cursor_continue
  Without a client, D_DB_UNSUPPORTED.
*/
int
d_mariadb_open_cursor_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending
)
{
    (void)_statement;
    (void)_events;

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_fetch_rows_start
  Without a client, D_DB_UNSUPPORTED, the rows left as they were.
*/
int
d_mariadb_fetch_rows_start(
    struct d_mariadb_statement* _statement,
    size_t                      _max,
    struct d_db_pending*        _pending,
    struct d_db_rows*           _rows
)
{
    (void)_statement;
    (void)_max;
    (void)_rows;

    return d_internal_mariadb_nothing_pending(_pending);
}

/*
d_mariadb_fetch_rows_continue
  Without a client, D_DB_UNSUPPORTED, the rows left as they were.
*/
int
d_mariadb_fetch_rows_continue(
    struct d_mariadb_statement* _statement,
    unsigned                    _events,
    struct d_db_pending*        _pending,
    struct d_db_rows*           _rows
)
{
    (void)_statement;
    (void)_events;
    (void)_rows;

    return d_internal_mariadb_nothing_pending(_pending);
}

#endif  // D_INTERNAL_MARIADB
