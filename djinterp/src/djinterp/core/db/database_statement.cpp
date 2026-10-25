/*******************************************************************************
* djinterp [core]                                         database_statement.cpp
*
* Definitions for database_statement.hpp: the session a connection shares
* with its statements, and buffered_result_set.
*   A result set reads values as db_common.h types them, and converts them
* with db_common.h's conversions, so that a value reads the same through
* core/db as through a face or the C layer; numbers it writes as text go
* through value_to_string (database.cpp), the same under every locale.
*
*
* path:      /src/djinterp/core/db/database_statement.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.16
*******************************************************************************/
#include "../../../../inc/djinterp/core/db/database_statement.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>    // std::size_t
#include <limits>     // std::numeric_limits
#include <optional>   // std::optional, std::nullopt
#include <stdexcept>  // std::out_of_range
#include <string>     // std::string, std::to_string
#include <utility>    // std::move
#include <vector>     // std::vector
// djinterp
#include "../../../../inc/djinterp/c/db/db_common.h"  // d_db_value_*,
                                                       // d_db_rows
#include "../../../../inc/djinterp/core/db/database.hpp"  // value_to_string,
                                                           // the exceptions
// re_std
#include "../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int32_t,
                                                       // int64_t, uint8_t


NS_DJINTERP

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

namespace
{

/**
 * @brief The field type for a column type the C layers give.
 *
 * @param[in] _type  the column's d_db_type.
 * @return the field type, member for member; custom for D_DB_TYPE_NONE, a
 *         type the vendor gives none of, or one that maps to none.
 */
field_type
to_field_type(
    enum d_db_type _type
) noexcept
{
    // the vocabulary the two share, member for member
    switch (_type)
    {
        case D_DB_TYPE_BOOLEAN:        return field_type::boolean;
        case D_DB_TYPE_INTEGER:        return field_type::integer;
        case D_DB_TYPE_BIG_INTEGER:    return field_type::big_integer;
        case D_DB_TYPE_FLOATING_POINT: return field_type::floating_point;
        case D_DB_TYPE_DECIMAL:        return field_type::decimal;
        case D_DB_TYPE_STRING:         return field_type::string;
        case D_DB_TYPE_BINARY:         return field_type::binary;
        case D_DB_TYPE_DATE:           return field_type::date;
        case D_DB_TYPE_TIME:           return field_type::time;
        case D_DB_TYPE_DATETIME:       return field_type::datetime;
        case D_DB_TYPE_TIMESTAMP:      return field_type::timestamp;
        case D_DB_TYPE_JSON:           return field_type::json;
        case D_DB_TYPE_XML:            return field_type::xml;
        case D_DB_TYPE_UUID:           return field_type::uuid;
        case D_DB_TYPE_NONE:
        default:
            return field_type::custom;
    }
}

/**
 * @brief The name of a value's storage type, for messages.
 *
 * @param[in] _value  the value.
 * @return "NULL", "an integer", "a real number", "text" or "bytes".
 */
const char*
storage_name(
    const struct d_db_value& _value
) noexcept
{
    // db_common.h's five types
    switch (_value.type)
    {
        case D_DB_VALUE_INTEGER: return "an integer";
        case D_DB_VALUE_REAL:    return "a real number";
        case D_DB_VALUE_TEXT:    return "text";
        case D_DB_VALUE_BLOB:    return "bytes";
        case D_DB_VALUE_NULL:
        default:
            return "NULL";
    }
}

/**
 * @brief Throws the exception for a value that does not convert.
 *
 * @param[in] _caller  the member that read it.
 * @param[in] _index   the column.
 * @param[in] _value   the value.
 * @param[in] _type    what it was read as.
 * @throws type_conversion_exception with D_DB_MISMATCH, always.
 */
[[noreturn]] void
refuse_conversion(
    const char*              _caller,
    std::size_t              _index,
    const struct d_db_value& _value,
    const char*              _type
)
{
    throw type_conversion_exception(
        std::string("buffered_result_set::") + _caller + ": column " +
            std::to_string(_index) + " holds " + storage_name(_value) +
            ", which is not exactly " + _type,
        D_DB_MISMATCH);
}

}  // namespace

//==============================================================================
// 2.  SESSIONS
//==============================================================================

// 2.1    The session
//------------------------------------------------------------------------------
NS_INTERNAL

    session::session() noexcept
        : m_entries(),
          m_changes(0),
          m_lost(false),
          m_holder(nullptr),
          m_busy_code(0)
    {}

    void
    session::enroll(
        void*  _statement,
        closer _close
    )
    {
        m_entries.push_back(entry{ _statement, _close });

        return;
    }

    void
    session::withdraw(
        void* _statement
    ) noexcept
    {
        // the one entry for the statement, when it is still enrolled
        for (std::size_t i = 0u; i < m_entries.size(); ++i)
        {
            if (m_entries[i].statement == _statement)
            {
                m_entries.erase(m_entries.begin() +
                                static_cast<std::ptrdiff_t>(i));

                break;
            }
        }

        return;
    }

    /*
    session::close_all
      The list is taken first, so that the session is empty while the
    statements finalize, whatever a closer does.
    */
    void
    session::close_all() noexcept
    {
        std::vector<entry> closing;

        closing.swap(m_entries);
        m_holder = nullptr;

        // each statement finalizes itself
        for (const entry& closed : closing)
        {
            closed.close(closed.statement);
        }

        return;
    }

    void
    session::record(
        re_std::int64_t _changes
    ) noexcept
    {
        m_changes = _changes;

        return;
    }

    re_std::int64_t
    session::changes() const noexcept
    {
        return m_changes;
    }

    void
    session::mark_lost() noexcept
    {
        m_lost = true;

        return;
    }

    bool
    session::lost() const noexcept
    {
        return m_lost;
    }

    void
    session::renew() noexcept
    {
        m_changes = 0;
        m_lost    = false;
        m_holder  = nullptr;

        return;
    }

    void
    session::hold(
        const void* _stream,
        int         _busy_code
    ) noexcept
    {
        m_holder    = _stream;
        m_busy_code = _busy_code;

        return;
    }

    void
    session::release(
        const void* _stream
    ) noexcept
    {
        // only the stream that holds the connection lets it go
        if (m_holder == _stream)
        {
            m_holder = nullptr;
        }

        return;
    }

    const void*
    session::holder() const noexcept
    {
        return m_holder;
    }

    int
    session::busy_code() const noexcept
    {
        return m_busy_code;
    }

NS_END  // internal

//==============================================================================
// 3.  RESULT SETS
//==============================================================================

// 3.1    Buffered result sets
//------------------------------------------------------------------------------
buffered_result_set::buffered_result_set() noexcept
    : result_set<buffered_result_set>(),
      m_rows(),
      m_cursor(-1)
{}

buffered_result_set::buffered_result_set(
    face::rows&& _rows
) noexcept
    : result_set<buffered_result_set>(),
      m_rows(std::move(_rows)),
      m_cursor(-1)
{}

/*
buffered_result_set::buffered_result_set (move)
  The cursor moves with the rows, and the object moved from is left empty,
its cursor before the first row, as a new one is.
*/
buffered_result_set::buffered_result_set(
    buffered_result_set&& _other
) noexcept
    : result_set<buffered_result_set>(std::move(_other)),
      m_rows(std::move(_other.m_rows)),
      m_cursor(_other.m_cursor)
{
    _other.m_cursor = -1;
}

buffered_result_set&
buffered_result_set::operator=(
    buffered_result_set&& _other
) noexcept
{
    // a result set assigned to itself keeps its rows
    if (this != &_other)
    {
        m_rows          = std::move(_other.m_rows);
        m_cursor        = _other.m_cursor;
        _other.m_cursor = -1;
    }

    return *this;
}

/*
buffered_result_set::next
  The cursor stops after the last row, so that previous() from there lands on
the last row again.
*/
bool
buffered_result_set::next() noexcept
{
    const re_std::int64_t count = row_count();

    // one row on, but never past the place after the last
    if (m_cursor < count)
    {
        ++m_cursor;
    }

    return (m_cursor < count);
}

bool
buffered_result_set::previous() noexcept
{
    // one row back, but never before the place before the first
    if (m_cursor >= 0)
    {
        --m_cursor;
    }

    return (m_cursor >= 0);
}

bool
buffered_result_set::first() noexcept
{
    m_cursor = (row_count() > 0) ? 0
                                 : -1;

    return (m_cursor == 0);
}

bool
buffered_result_set::last() noexcept
{
    m_cursor = row_count() - 1;

    return (m_cursor >= 0);
}

/*
buffered_result_set::absolute
  A row past either end leaves the cursor at that end's place -- before the
first, after the last -- as next and previous would.
*/
bool
buffered_result_set::absolute(
    re_std::int64_t _row
) noexcept
{
    const re_std::int64_t count  = row_count();
    const re_std::int64_t target = (_row < 0) ? (count + _row)
                                              : _row;

    // a row the result set has, or the place beyond the nearer end
    if (target < 0)
    {
        m_cursor = -1;
    }
    else if (target >= count)
    {
        m_cursor = count;
    }
    else
    {
        m_cursor = target;
    }

    return ( (m_cursor >= 0) &&
             (m_cursor < count) );
}

std::size_t
buffered_result_set::column_count() const noexcept
{
    return m_rows.column_count();
}

std::string
buffered_result_set::column_name(
    std::size_t _index
) const
{
    // a column the rows have
    if (_index >= m_rows.column_count())
    {
        throw std::out_of_range("buffered_result_set::column_name: column " +
                                std::to_string(_index) +
                                " is past the last");
    }

    const char* const name = m_rows.column(_index).name;

    return (name) ? name
                  : "";
}

field_type
buffered_result_set::column_type(
    std::size_t _index
) const
{
    // a column the rows have
    if (_index >= m_rows.column_count())
    {
        throw std::out_of_range("buffered_result_set::column_type: column " +
                                std::to_string(_index) +
                                " is past the last");
    }

    return to_field_type(m_rows.column(_index).type);
}

std::size_t
buffered_result_set::column_index(
    const std::string& _name
) const
{
    std::size_t index = 0u;

    // the column by its name, or none
    if (m_rows.column_index(_name, index) != 0)
    {
        throw std::out_of_range("buffered_result_set::column_index: no "
                                "column is named " + _name);
    }

    return index;
}

re_std::int64_t
buffered_result_set::row_count() const noexcept
{
    return static_cast<re_std::int64_t>(m_rows.row_count());
}

re_std::int64_t
buffered_result_set::current_row() const noexcept
{
    return m_cursor;
}

/*
buffered_result_set::get_value
  An integer is the one storage type whose value's type depends on the
column: SQL's booleans arrive as 0 and 1, and a 32-bit column's values are
given as the 32-bit alternative where they fit, as field_type names them.
*/
value
buffered_result_set::get_value(
    std::size_t _index
) const
{
    const struct d_db_value& held = m_value(_index, "get_value");

    // each storage type to its alternative
    switch (held.type)
    {
        case D_DB_VALUE_INTEGER:
        {
            const field_type type = to_field_type(m_rows.column(_index).type);

            if (type == field_type::boolean)
            {
                return (held.as.integer != 0);
            }

            if ( (type == field_type::integer)                         &&
                 (held.as.integer >=
                      std::numeric_limits<re_std::int32_t>::min())     &&
                 (held.as.integer <=
                      std::numeric_limits<re_std::int32_t>::max()) )
            {
                return static_cast<re_std::int32_t>(held.as.integer);
            }

            return static_cast<re_std::int64_t>(held.as.integer);
        }

        case D_DB_VALUE_REAL:
            return held.as.real;

        case D_DB_VALUE_TEXT:
            return std::string(held.as.text, held.size);

        case D_DB_VALUE_BLOB:
        {
            const re_std::uint8_t* const bytes =
                static_cast<const re_std::uint8_t*>(held.as.blob);

            return std::vector<re_std::uint8_t>(bytes, bytes + held.size);
        }

        case D_DB_VALUE_NULL:
        default:
            return std::monostate();
    }
}

bool
buffered_result_set::is_null(
    std::size_t _index
) const
{
    return (m_value(_index, "is_null").type == D_DB_VALUE_NULL);
}

std::optional<bool>
buffered_result_set::get_bool(
    std::size_t _index
) const
{
    const struct d_db_value& held = m_value(_index, "get_bool");
    bool                     read = false;

    // NULL is no boolean, and a value that is not one does not convert
    if (held.type == D_DB_VALUE_NULL)
    {
        return std::nullopt;
    }

    if (d_db_value_bool(&held, &read) != 0)
    {
        refuse_conversion("get_bool", _index, held, "a boolean");
    }

    return read;
}

std::optional<re_std::int32_t>
buffered_result_set::get_int(
    std::size_t _index
) const
{
    const struct d_db_value& held = m_value(_index, "get_int");
    re_std::int64_t          read = 0;

    // NULL is no integer; a value must convert, and fit in 32 bits
    if (held.type == D_DB_VALUE_NULL)
    {
        return std::nullopt;
    }

    if ( (d_db_value_int64(&held, &read) != 0)                ||
         (read < std::numeric_limits<re_std::int32_t>::min()) ||
         (read > std::numeric_limits<re_std::int32_t>::max()) )
    {
        refuse_conversion("get_int", _index, held, "a 32-bit integer");
    }

    return static_cast<re_std::int32_t>(read);
}

std::optional<re_std::int64_t>
buffered_result_set::get_long(
    std::size_t _index
) const
{
    const struct d_db_value& held = m_value(_index, "get_long");
    re_std::int64_t          read = 0;

    // NULL is no integer, and a value that is not one does not convert
    if (held.type == D_DB_VALUE_NULL)
    {
        return std::nullopt;
    }

    if (d_db_value_int64(&held, &read) != 0)
    {
        refuse_conversion("get_long", _index, held, "a 64-bit integer");
    }

    return read;
}

std::optional<double>
buffered_result_set::get_double(
    std::size_t _index
) const
{
    const struct d_db_value& held = m_value(_index, "get_double");
    double                   read = 0.0;

    // NULL is no number, and a value that is not one does not convert
    if (held.type == D_DB_VALUE_NULL)
    {
        return std::nullopt;
    }

    if (d_db_value_double(&held, &read) != 0)
    {
        refuse_conversion("get_double", _index, held, "a double");
    }

    return read;
}

/*
buffered_result_set::get_string
  Bytes read as a string of those bytes: std::string holds any, and a column
of a binary character set arrives as bytes on some vendors.
*/
std::optional<std::string>
buffered_result_set::get_string(
    std::size_t _index
) const
{
    const struct d_db_value& held = m_value(_index, "get_string");

    // each storage type as text
    switch (held.type)
    {
        case D_DB_VALUE_INTEGER:
            return std::to_string(static_cast<long long>(held.as.integer));

        case D_DB_VALUE_REAL:
            return value_to_string(value(held.as.real));

        case D_DB_VALUE_TEXT:
            return std::string(held.as.text, held.size);

        case D_DB_VALUE_BLOB:
            return std::string(static_cast<const char*>(held.as.blob),
                               held.size);

        case D_DB_VALUE_NULL:
        default:
            return std::nullopt;
    }
}

std::optional<std::vector<re_std::uint8_t>>
buffered_result_set::get_binary(
    std::size_t _index
) const
{
    const struct d_db_value& held = m_value(_index, "get_binary");

    // bytes, and text as its bytes; a number is neither
    switch (held.type)
    {
        case D_DB_VALUE_TEXT:
        {
            const re_std::uint8_t* const bytes =
                reinterpret_cast<const re_std::uint8_t*>(held.as.text);

            return std::vector<re_std::uint8_t>(bytes, bytes + held.size);
        }

        case D_DB_VALUE_BLOB:
        {
            const re_std::uint8_t* const bytes =
                static_cast<const re_std::uint8_t*>(held.as.blob);

            return std::vector<re_std::uint8_t>(bytes, bytes + held.size);
        }

        case D_DB_VALUE_NULL:
            return std::nullopt;

        case D_DB_VALUE_INTEGER:
        case D_DB_VALUE_REAL:
        default:
            refuse_conversion("get_binary", _index, held, "bytes");
    }
}

value
buffered_result_set::get_value(
    const std::string& _name
) const
{
    return get_value(column_index(_name));
}

bool
buffered_result_set::is_null(
    const std::string& _name
) const
{
    return is_null(column_index(_name));
}

std::optional<bool>
buffered_result_set::get_bool(
    const std::string& _name
) const
{
    return get_bool(column_index(_name));
}

std::optional<re_std::int32_t>
buffered_result_set::get_int(
    const std::string& _name
) const
{
    return get_int(column_index(_name));
}

std::optional<re_std::int64_t>
buffered_result_set::get_long(
    const std::string& _name
) const
{
    return get_long(column_index(_name));
}

std::optional<double>
buffered_result_set::get_double(
    const std::string& _name
) const
{
    return get_double(column_index(_name));
}

std::optional<std::string>
buffered_result_set::get_string(
    const std::string& _name
) const
{
    return get_string(column_index(_name));
}

std::optional<std::vector<re_std::uint8_t>>
buffered_result_set::get_binary(
    const std::string& _name
) const
{
    return get_binary(column_index(_name));
}

row
buffered_result_set::get_current_row() const
{
    row current;

    m_require_row("get_current_row");

    // each column by name; emplace keeps the first of two that share one
    for (std::size_t i = 0u; i < m_rows.column_count(); ++i)
    {
        current.emplace(column_name(i), get_value(i));
    }

    return current;
}

result_rows
buffered_result_set::get_all_rows()
{
    result_rows all;

    // each row after the cursor, the cursor ending after the last
    while (next())
    {
        all.push_back(get_current_row());
    }

    return all;
}

void
buffered_result_set::close() noexcept
{
    m_rows.clear();
    m_cursor = -1;

    return;
}

const face::rows&
buffered_result_set::rows() const noexcept
{
    return m_rows;
}

void
buffered_result_set::m_require_row(
    const char* _caller
) const
{
    // the cursor on a row the result set has
    if ( (m_cursor < 0) ||
         (m_cursor >= row_count()) )
    {
        throw std::out_of_range(std::string("buffered_result_set::") +
                                _caller + ": the cursor is on no row");
    }

    return;
}

const struct d_db_value&
buffered_result_set::m_value(
    std::size_t _index,
    const char* _caller
) const
{
    m_require_row(_caller);

    // a column the row has
    if (_index >= m_rows.column_count())
    {
        throw std::out_of_range(std::string("buffered_result_set::") +
                                _caller + ": column " +
                                std::to_string(_index) + " is past the last");
    }

    return *m_rows.value(static_cast<std::size_t>(m_cursor), _index);
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
