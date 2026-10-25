/*******************************************************************************
* djinterp [c]                                                       db_common.c
*
* Implementation of db_common.h: type names, the release functions for the
* lists and rows the vendor layers return, and reading rows.
*   Every list, and every set of rows, is a single block -- the arrays at its
* start, the text after them -- so each release frees one pointer, through
* the C runtime that allocated it.
*   The conversions read text as the vendor layers do, through
* db_internal.h's number readers, which take a `.` for the point whatever the
* C locale says.
*
*
* path:      /src/djinterp/c/db/db_common.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.12
*                                                            revised: 2026.10.23
*******************************************************************************/
#include "../../../../inc/djinterp/c/db/db_common.h"  // corresponding header
// std
#include <stddef.h>  // NULL, size_t
#include <stdlib.h>  // free
#include <string.h>  // strcmp, strlen
// djinterp
#include "./db_internal.h"  // the number readers
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int64_t


//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

/**
 * @brief Compares two names, ignoring ASCII case and nothing else.
 *
 * @param[in] _left   one name.
 * @param[in] _right  the other.
 * @return `true` when they match.
 *
 * Only A-Z fold, whatever the C locale says, as SQL folds an unquoted name;
 * every other byte, UTF-8 included, compares as it is.
 */
static bool
d_internal_db_same_name(
    const char* _left,
    const char* _right
)
{
    // byte by byte, to the end of the shorter
    for (;;)
    {
        char left  = *_left++;
        char right = *_right++;

        // fold A-Z to a-z
        if ( (left >= 'A') &&
             (left <= 'Z') )
        {
            left = (char)(left - 'A' + 'a');
        }

        if ( (right >= 'A') &&
             (right <= 'Z') )
        {
            right = (char)(right - 'A' + 'a');
        }

        // a difference, or the end of both
        if (left != right)
        {
            return false;
        }

        if (left == '\0')
        {
            return true;
        }
    }
}

/**
 * @brief Answers whether text holds no NUL before its end, so that a reader
 *        of C strings sees all of it.
 *
 * @param[in] _value  a TEXT value.
 * @return `true` when its size is its C string's length.
 */
static bool
d_internal_db_whole_text(
    const struct d_db_value* _value
)
{
    return (strlen(_value->as.text) == _value->size);
}


//==============================================================================
// 1.  RESULT CODES
//==============================================================================

// 1.2    Status messages
//------------------------------------------------------------------------------
/*
d_db_status_message
  The wording is shared, so that every vendor layer describes the conditions
none of them has a code for in the same words.
*/
const char*
d_db_status_message(
    int _status
)
{
    switch (_status)
    {
        case D_DB_OK:
            return "success";

        case D_DB_UNSUPPORTED:
            return "the library or server in use cannot do this";

        case D_DB_EXISTS:
            return "already exists";

        case D_DB_NOT_FOUND:
            return "not found";

        case D_DB_INVALID_ARGUMENT:
            return "invalid argument";

        case D_DB_MISMATCH:
            return "the value does not convert to that type exactly";

        case D_DB_WAIT:
            return "the call has not finished: wait, then continue it";

        default:
            break;
    }

    return NULL;
}

//==============================================================================
// 3.  COLUMN DEFINITIONS
//==============================================================================

// 3.2    Type names
//------------------------------------------------------------------------------
/*
d_db_type_name
  The names are the C++ layer's field_type enumerators, so a diagnostic reads
the same from either face.
*/
const char*
d_db_type_name(
    enum d_db_type _type
)
{
    switch (_type)
    {
        case D_DB_TYPE_NONE:           return "none";
        case D_DB_TYPE_BOOLEAN:        return "boolean";
        case D_DB_TYPE_INTEGER:        return "integer";
        case D_DB_TYPE_BIG_INTEGER:    return "big_integer";
        case D_DB_TYPE_FLOATING_POINT: return "floating_point";
        case D_DB_TYPE_DECIMAL:        return "decimal";
        case D_DB_TYPE_STRING:         return "string";
        case D_DB_TYPE_BINARY:         return "binary";
        case D_DB_TYPE_DATE:           return "date";
        case D_DB_TYPE_TIME:           return "time";
        case D_DB_TYPE_DATETIME:       return "datetime";
        case D_DB_TYPE_TIMESTAMP:      return "timestamp";
        case D_DB_TYPE_JSON:           return "json";
        case D_DB_TYPE_XML:            return "xml";
        case D_DB_TYPE_UUID:           return "uuid";
        default:                       break;
    }

    return "unknown";
}

//==============================================================================
// 4.  RESULTS
//==============================================================================

// 4.2    Release
//------------------------------------------------------------------------------
/*
d_db_name_list_free
  The names live in the same block as the array that points at them, so one
free releases both.
*/
void
d_db_name_list_free(
    struct d_db_name_list* _list
)
{
    // accept NULL, as free does
    if (_list)
    {
        free(_list->names);

        _list->names = NULL;
        _list->count = 0u;
    }

    return;
}

/*
d_db_column_list_free
  As d_db_name_list_free: the strings share the block.
*/
void
d_db_column_list_free(
    struct d_db_column_list* _list
)
{
    // accept NULL, as free does
    if (_list)
    {
        free(_list->columns);

        _list->columns = NULL;
        _list->count   = 0u;
    }

    return;
}

/*
d_db_free
  Exists so that memory is released by the C runtime that allocated it, which
on Windows need not be the caller's.
*/
void
d_db_free(
    void* _block
)
{
    free(_block);

    return;
}

//==============================================================================
// 5.  ROWS
//==============================================================================

// 5.3    Reading
//------------------------------------------------------------------------------
/*
d_db_rows_value
  Each index is checked against its own count, so no product of the two is
formed until both are known to be in range, where it cannot overflow: the
values exist, and so fit in memory.
*/
const struct d_db_value*
d_db_rows_value(
    const struct d_db_rows* _rows,
    size_t                  _row,
    size_t                  _column
)
{
    // no rows, or a place outside them
    if ( (!_rows)                         ||
         (!_rows->values)                 ||
         (_row >= _rows->row_count)       ||
         (_column >= _rows->column_count) )
    {
        return NULL;
    }

    return &_rows->values[(_row * _rows->column_count) + _column];
}

/*
d_db_rows_column_index
  Two passes: an exact match anywhere wins over a match without case earlier,
so a query that names both "Id" and "id" finds each by its own spelling.
*/
int
d_db_rows_column_index(
    const struct d_db_rows* _rows,
    const char*             _name,
    size_t*                 _index
)
{
    // parameter validation
    if ( (!_rows) ||
         (!_name) ||
         (!_index) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // the name exactly
    for (size_t i = 0u; i < _rows->column_count; ++i)
    {
        if (strcmp(_rows->columns[i].name, _name) == 0)
        {
            *_index = i;

            return D_DB_OK;
        }
    }

    // the name, ignoring ASCII case
    for (size_t i = 0u; i < _rows->column_count; ++i)
    {
        if (d_internal_db_same_name(_rows->columns[i].name, _name))
        {
            *_index = i;

            return D_DB_OK;
        }
    }

    return D_DB_NOT_FOUND;
}

// 5.4    Conversions
//------------------------------------------------------------------------------
/*
d_db_value_int64
  A REAL converts when it lies in [-2^63, 2^63), both bounds exact in a
double, and has no fraction; the range is checked before the cast, which
outside it is undefined. NaN fails both comparisons.
*/
int
d_db_value_int64(
    const struct d_db_value* _value,
    int64_t*                 _out
)
{
    // parameter validation
    if ( (!_value) ||
         (!_out) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    switch (_value->type)
    {
        case D_DB_VALUE_INTEGER:
            *_out = _value->as.integer;

            return D_DB_OK;

        case D_DB_VALUE_REAL:
        {
            const double real = _value->as.real;

            // in range, and whole
            if ( (real >= -9223372036854775808.0) &&
                 (real < 9223372036854775808.0) )
            {
                const int64_t whole = (int64_t)real;

                if ((double)whole == real)
                {
                    *_out = whole;

                    return D_DB_OK;
                }
            }

            return D_DB_MISMATCH;
        }

        case D_DB_VALUE_TEXT:
        {
            int64_t parsed = 0;

            // all of the text, read as an integer
            if ( (d_internal_db_whole_text(_value)) &&
                 (d_internal_db_parse_integer(_value->as.text, &parsed)) )
            {
                *_out = parsed;

                return D_DB_OK;
            }

            return D_DB_MISMATCH;
        }

        case D_DB_VALUE_NULL:
        case D_DB_VALUE_BLOB:
        default:
            break;
    }

    return D_DB_MISMATCH;
}

/*
d_db_value_double
  An INTEGER converts when the double nearest it converts back to it. The
nearest double to a value near INT64_MAX is 2^63, which int64_t cannot hold,
so that case is refused before the cast back.
*/
int
d_db_value_double(
    const struct d_db_value* _value,
    double*                  _out
)
{
    // parameter validation
    if ( (!_value) ||
         (!_out) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    switch (_value->type)
    {
        case D_DB_VALUE_REAL:
            *_out = _value->as.real;

            return D_DB_OK;

        case D_DB_VALUE_INTEGER:
        {
            const double nearest = (double)_value->as.integer;

            // exactly that integer
            if ( (nearest < 9223372036854775808.0) &&
                 ((int64_t)nearest == _value->as.integer) )
            {
                *_out = nearest;

                return D_DB_OK;
            }

            return D_DB_MISMATCH;
        }

        case D_DB_VALUE_TEXT:
        {
            double parsed = 0.0;

            // all of the text, read as a number
            if ( (d_internal_db_whole_text(_value)) &&
                 (d_internal_db_parse_real(_value->as.text, &parsed)) )
            {
                *_out = parsed;

                return D_DB_OK;
            }

            return D_DB_MISMATCH;
        }

        case D_DB_VALUE_NULL:
        case D_DB_VALUE_BLOB:
        default:
            break;
    }

    return D_DB_MISMATCH;
}

/*
d_db_value_bool
  The words are those PostgreSQL's boolean input takes, which cover what the
other servers write for a boolean as well.
*/
int
d_db_value_bool(
    const struct d_db_value* _value,
    bool*                    _out
)
{
    static const struct
    {
        const char* word;   // as written, in any ASCII case
        bool        value;  // what it means
    } words[] =
    {
        { "t", true  }, { "true",  true  }, { "y", true  }, { "yes", true  },
        { "on",  true  }, { "1", true  },
        { "f", false }, { "false", false }, { "n", false }, { "no",  false },
        { "off", false }, { "0", false }
    };

    // parameter validation
    if ( (!_value) ||
         (!_out) )
    {
        return D_DB_INVALID_ARGUMENT;
    }

    // an integer, as C reads one
    if (_value->type == D_DB_VALUE_INTEGER)
    {
        *_out = (_value->as.integer != 0);

        return D_DB_OK;
    }

    // text, one of the words, whole
    if ( (_value->type == D_DB_VALUE_TEXT) &&
         (d_internal_db_whole_text(_value)) )
    {
        for (size_t i = 0u; i < (sizeof(words) / sizeof(words[0])); ++i)
        {
            if (d_internal_db_same_name(_value->as.text, words[i].word))
            {
                *_out = words[i].value;

                return D_DB_OK;
            }
        }
    }

    return D_DB_MISMATCH;
}

// 5.5    Release
//------------------------------------------------------------------------------
/*
d_db_rows_free
  The block starts with the columns, the values and the text following them,
so one free releases everything; rows without columns hold nothing.
*/
void
d_db_rows_free(
    struct d_db_rows* _rows
)
{
    // accept NULL, as free does
    if (_rows)
    {
        free(_rows->columns);

        _rows->column_count = 0u;
        _rows->row_count    = 0u;
        _rows->columns      = NULL;
        _rows->values       = NULL;
    }

    return;
}
