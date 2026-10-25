/*******************************************************************************
* djinterp [core]                                             database_table.hpp
*
* A table of a database held in memory: the rows a query read, changed in
* place, and written back.
*   database_table<Connection> binds one table, or view, of the database a
* connection reaches to a cache of its rows. refresh() reads them -- every
* row, or those a filter, an order and a page select -- and the cache is
* then read and changed as a table of values, row by row, cell by cell;
* commit() writes back what changed. The table's own operations go through
* the C layers, written once for every vendor that has one: fetch_schema()
* describes the columns, exists() asks after the table, and create_table(),
* drop_table(), rename_table(), truncate_table() and the alter_ members
* change it.
*   commit() writes the cache back as one unit: in a transaction of its own,
* or, inside one the program began, under a savepoint, so that a failure
* leaves the database as it was and the program's transaction usable. With
* a primary key it is differential: each row read stays the row of the
* database it was read as (table_row), so a row the cache no longer holds is
* deleted, a changed row is updated -- its key too, in place, so that the
* key's ON UPDATE rules apply, not its ON DELETE ones -- a new row is
* inserted, and no other row is touched. Without one it replaces the rows refresh() read --
* those the filter selected, or every row -- and refuses rows read a page
* at a time, which it could not name. Rows the cache never read are
* inserted. A NULL in a column that refuses NULL, or in a key, is left out
* of the row's INSERT, so that the column's default, identity or sequence
* fills it, and a generated column is never written; the cache is then read
* again, to hold what the database chose or computed.
*   The table is a concrete class, nothing in it virtual: a vendor's table
* (sqlite_table, mysql_table, mariadb_table, postgres_table) derives from
* it, adds what its vendor has, and is used through its own type.
*
*
* path:      /inc/djinterp/core/db/database_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.18
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESCRIPTIONS
    ------------
    1.  Kinds and policies
         1.  table_kind
         2.  sync_policy
         3.  modify_action
    2.  Columns
         1.  column_info
         2.  table_schema
    3.  Synchronization
         1.  sync_config
    4.  Rows
         1.  table_row
2.  DIALECT
    -------
    1.  Clauses
         1.  dialect_format_limit_offset
         2.  dialect_placeholder
    2.  Detectors
    3.  Columns for the layers
         1.  to_layer_type
         2.  to_layer_column
         3.  to_layer_columns
3.  DATABASE TABLE
    --------------
    1.  The class
         1.  database_table
*/

#ifndef DJINTERP_DB_DATABASE_TABLE_HPP
#define DJINTERP_DB_DATABASE_TABLE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>         // std::find, std::sort
#include <atomic>            // std::atomic
#include <chrono>            // std::chrono::steady_clock, milliseconds
#include <cstddef>           // std::size_t, std::ptrdiff_t
#include <initializer_list>  // std::initializer_list
#include <limits>            // std::numeric_limits
#include <map>               // std::map
#include <memory>            // std::allocator, std::unique_ptr
#include <optional>          // std::optional
#include <set>               // std::set
#include <stdexcept>         // std::out_of_range
#include <string>            // std::string, std::to_string
#include <string_view>       // std::string_view
#include <type_traits>       // std::is_same_v, std::decay_t
#include <unordered_map>     // std::unordered_map
#include <utility>           // std::move, std::pair, std::declval
#include <variant>           // std::visit, std::monostate
#include <vector>            // std::vector
// djinterp
#include "../../djinterp.hpp"       // framework root
#include "../../c/db/db_face.hpp"   // face::string_arg, face::column_list,
                                    // d_db_column, the D_DB_* codes
#include "../meta/type_traits.hpp"  // is_detected
#include "./database.hpp"           // value, field_type, database_type,
                                    // quote_identifier, transaction
#include "./database_statement.hpp" // buffered_result_set
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int64_t, int32_t,
                                                // uint8_t, uint64_t


NS_DJINTERP


//==============================================================================
// 1.  DESCRIPTIONS
//==============================================================================


// 1.1    Kinds and policies
//------------------------------------------------------------------------------
// 1.1.1
// table_kind
//   enum: what a database_table stands for. A base table and a temporary one
// can be changed and written back; a view and a materialized view only read.
enum class table_kind
{
    base_table,
    view,
    materialized_view,
    temporary
};

// 1.1.2
// sync_policy
//   enum: when the cache follows the database without being told to.
//     manual             refresh() alone reads it, and commit() alone writes
//     on_access          element access reads a stale cache again, unless
//                        it holds changes not yet committed
//     on_modify          each tracked change commits at once, as
//                        sync_config::auto_commit does under any policy
//     periodic           as on_access, the cache going stale each
//                        refresh_interval after it was read
enum class sync_policy
{
    manual,
    on_access,
    on_modify,
    periodic
};

// 1.1.3
// modify_action
//   enum: the kinds of change to the cache, which sync_config tracks for
// committing at once.
enum class modify_action
{
    insert_row,
    remove_row,
    update_cell,
    add_column,
    remove_column,
    clear_all,
    bulk_insert
};

// 1.2    Columns
//------------------------------------------------------------------------------
// 1.2.1
// column_info
//   struct: one column of a table, as fetch_schema reads it or as a program
// describes one for create_table. fetch_schema fills the name and type from
// the database's own description of a query's columns, and, where the
// vendor's layer can describe the table (every layer can), whether the
// column takes NULL, its default, its place in the primary key, whether the
// database computes it (a generated column), and its type as declared, in
// sql_type, which create_table then writes back as it was. A generated
// column is read and never written, and create_table cannot make one.
// is_indexed, the foreign reference and the comment are a program's own
// notes: neither read nor created.
struct column_info
{
    std::string                name;
    field_type                 type;
    bool                       nullable;
    bool                       is_primary_key;
    bool                       is_auto_increment;
    bool                       is_unique;
    bool                       is_indexed;
    bool                       is_generated;
    std::optional<std::string> default_value;   // the DEFAULT, as SQL
    std::optional<std::size_t> max_length;      // a length, or DECIMAL's
                                                // precision
    std::optional<std::string> foreign_table;
    std::optional<std::string> foreign_column;
    std::optional<std::string> comment;
    std::optional<std::string> sql_type;        // the vendor's own type,
                                                // written in place of type's
    std::optional<std::size_t> scale;           // DECIMAL's scale

    column_info()
        : type(field_type::null),
          nullable(true),
          is_primary_key(false),
          is_auto_increment(false),
          is_unique(false),
          is_indexed(false),
          is_generated(false)
    {}

    explicit column_info(
        std::string _name,
        field_type  _type        = field_type::string,
        bool        _nullable    = true,
        bool        _primary_key = false
    )
        : name(std::move(_name)),
          type(_type),
          nullable(_nullable),
          is_primary_key(_primary_key),
          is_auto_increment(false),
          is_unique(_primary_key),
          is_indexed(_primary_key),
          is_generated(false)
    {}
};

// 1.2.2
// table_schema
//   struct: a table's description: its name, the schema it lives in -- the
// database on MariaDB and MySQL, the attached database on SQLite, the schema
// on PostgreSQL; empty for the connection's own -- its columns, in order,
// and its primary key's columns, in the key's order.
struct table_schema
{
    std::string              table_name;
    std::string              schema_name;
    std::vector<column_info> columns;
    std::vector<std::string> primary_key_columns;

    // column_count -- how many columns the schema describes
    std::size_t
    column_count() const noexcept
    {
        return columns.size();
    }

    // column_index -- the index of the column of that name; std::nullopt
    // when there is none
    std::optional<std::size_t>
    column_index(
        std::string_view _name
    ) const
    {
        for (std::size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i].name == _name)
            {
                return i;
            }
        }

        return std::nullopt;
    }

    // column_by_name -- the column of that name; nullptr when there is none
    const column_info*
    column_by_name(
        std::string_view _name
    ) const
    {
        const std::optional<std::size_t> index = column_index(_name);

        return (index.has_value()) ? &columns[index.value()]
                                   : nullptr;
    }
};

// 1.3    Synchronization
//------------------------------------------------------------------------------
// 1.3.1
// sync_config
//   struct: when the cache follows the database without being told to: the
// policy; the interval after which a periodic cache goes stale; the changes
// that commit at once under on_modify or auto_commit, every kind when the
// list is empty; and whether they do so under any policy.
struct sync_config
{
    sync_policy                policy;
    std::chrono::milliseconds  refresh_interval;
    std::vector<modify_action> tracked_actions;
    bool                       auto_commit;

    sync_config()
        : policy(sync_policy::manual),
          refresh_interval(std::chrono::milliseconds(0)),
          auto_commit(false)
    {}

    explicit sync_config(
        sync_policy _policy
    )
        : policy(_policy),
          refresh_interval(std::chrono::milliseconds(0)),
          auto_commit(false)
    {}

    // tracks_action -- whether a change of that kind commits at once, where
    // the policy or auto_commit commits changes; an empty list tracks all
    bool
    tracks_action(
        modify_action _action
    ) const
    {
        // an empty list tracks every kind
        if (tracked_actions.empty())
        {
            return true;
        }

        return (std::find(tracked_actions.begin(),
                          tracked_actions.end(),
                          _action) != tracked_actions.end());
    }
};

// 1.4    Rows
//------------------------------------------------------------------------------
NS_INTERNAL

    // next_row_origins
    //   function: `_count` numbers for the rows a table reads, the first
    // returned: never the same twice in a program, so that a row carried into
    // another table, or kept past a refresh, stands for no row there.
    inline re_std::uint64_t
    next_row_origins(
        std::size_t _count
    ) noexcept
    {
        static std::atomic<re_std::uint64_t> next{ 1u };

        return next.fetch_add(static_cast<re_std::uint64_t>(_count));
    }

NS_END  // internal

// 1.4.1
// table_row
//   class template: a row of a database_table's cache -- its cells, a
// std::vector of them -- and the row of the database it was read as, which
// travels with it through copies, moves, swaps and sorts, so that a commit
// updates that row, its key with it, rather than deleting it and inserting
// another. A row made from cells stands for none, and is inserted.
// Assigning cells, a std::vector, keeps the row a table_row stands for;
// assigning another table_row takes that one's. Where two rows of a cache
// stand for the same row, as after a copy, the first keeps it and the
// others are new.
template<typename ValueType>
class table_row : public std::vector<ValueType>
{
public:
    using cells_type = std::vector<ValueType>;

    using cells_type::cells_type;

    table_row() = default;

    table_row(
        const cells_type& _cells
    )
        : cells_type(_cells)
    {}

    table_row(
        cells_type&& _cells
    ) noexcept
        : cells_type(std::move(_cells))
    {}

    table_row(const table_row&)                = default;
    table_row(table_row&&) noexcept            = default;
    table_row& operator=(const table_row&)     = default;
    table_row& operator=(table_row&&) noexcept = default;

    ~table_row() = default;

    // assigning cells -- the row stands for what it stood for
    table_row&
    operator=(
        const cells_type& _cells
    )
    {
        cells_type::operator=(_cells);

        return *this;
    }

    table_row&
    operator=(
        cells_type&& _cells
    ) noexcept
    {
        cells_type::operator=(std::move(_cells));

        return *this;
    }

    table_row&
    operator=(
        std::initializer_list<ValueType> _cells
    )
    {
        cells_type::operator=(_cells);

        return *this;
    }

private:
    template<typename,
             typename,
             typename>
    friend class database_table;

    // m_origin -- the row read that this one stands for, by its number; 0 for
    // none
    re_std::uint64_t m_origin = 0u;
};


//==============================================================================
// 2.  DIALECT
//==============================================================================
// The few clauses whose spelling differs between the vendors, chosen by a
// switch on the connection's database_type; identifiers are quoted by
// quote_identifier (database.hpp).


NS_INTERNAL

    // 2.1    Clauses
    //--------------------------------------------------------------------------
    // 2.1.1
    // dialect_format_limit_offset
    //   function: the clause that selects a page of rows, with a space before
    // it; empty when neither a limit nor an offset is given. LIMIT and OFFSET
    // where the vendor has them; SQLite, MariaDB and MySQL accept an offset
    // only after a limit, which is then the largest each takes (-1 on
    // SQLite). Oracle, SQL Server and Db2 write OFFSET ... ROWS FETCH NEXT
    // ... ROWS ONLY.
    inline std::string
    dialect_format_limit_offset(
        database_type                     _db_type,
        const std::optional<std::size_t>& _limit,
        const std::optional<std::size_t>& _offset
    )
    {
        std::string clause;

        // no page, no clause
        if ( (!_limit.has_value()) &&
             (!_offset.has_value()) )
        {
            return clause;
        }

        switch (_db_type)
        {
            // OFFSET ... FETCH, an offset always given
            case database_type::oracle:
            case database_type::mssql:
            case database_type::db2:
                clause += " OFFSET ";
                clause += std::to_string(_offset.value_or(0u));
                clause += " ROWS";

                if (_limit.has_value())
                {
                    clause += " FETCH NEXT ";
                    clause += std::to_string(_limit.value());
                    clause += " ROWS ONLY";
                }

                return clause;

            // an offset only after a limit, the largest when none is given
            case database_type::sqlite:
                clause += " LIMIT ";
                clause += (_limit.has_value()) ? std::to_string(_limit.value())
                                               : std::string("-1");
                break;

            case database_type::mariadb:
            case database_type::mysql:
                clause += " LIMIT ";
                clause += (_limit.has_value())
                              ? std::to_string(_limit.value())
                              : std::string("18446744073709551615");
                break;

            // LIMIT and OFFSET, each alone or together
            default:
                if (_limit.has_value())
                {
                    clause += " LIMIT ";
                    clause += std::to_string(_limit.value());
                }

                break;
        }

        if (_offset.has_value())
        {
            clause += " OFFSET ";
            clause += std::to_string(_offset.value());
        }

        return clause;
    }

    // 2.1.2
    // dialect_placeholder
    //   function: the marker for the statement parameter numbered `_index`,
    // from 1, as the vendor's prepared statements write it: `$1`, `$2` and
    // so on for PostgreSQL, `?` for the others.
    inline std::string
    dialect_placeholder(
        database_type _db_type,
        std::size_t   _index
    )
    {
        // PostgreSQL numbers its parameters; the others place them
        if (_db_type == database_type::postgresql)
        {
            return "$" + std::to_string(_index);
        }

        return "?";
    }

    // 2.2    Detectors
    //--------------------------------------------------------------------------
    // table_savepoint_t -- a connection's create_savepoint(name), under which
    // commit writes inside a transaction the program began
    template<typename Connection>
    using table_savepoint_t =
        decltype(std::declval<Connection&>().create_savepoint(
            std::declval<const std::string&>()));

    // 2.3    Columns for the layers
    //--------------------------------------------------------------------------
    // 2.3.1
    // to_layer_type
    //   function: the C layers' type for a field type, member for member from
    // boolean to uuid; D_DB_TYPE_NONE for null, array and custom, which a
    // column's sql_type then names (SQLite alone takes a column of no type).
    inline enum d_db_type
    to_layer_type(
        field_type _type
    ) noexcept
    {
        switch (_type)
        {
            case field_type::boolean:        return D_DB_TYPE_BOOLEAN;
            case field_type::integer:        return D_DB_TYPE_INTEGER;
            case field_type::big_integer:    return D_DB_TYPE_BIG_INTEGER;
            case field_type::floating_point: return D_DB_TYPE_FLOATING_POINT;
            case field_type::decimal:        return D_DB_TYPE_DECIMAL;
            case field_type::string:         return D_DB_TYPE_STRING;
            case field_type::binary:         return D_DB_TYPE_BINARY;
            case field_type::date:           return D_DB_TYPE_DATE;
            case field_type::time:           return D_DB_TYPE_TIME;
            case field_type::datetime:       return D_DB_TYPE_DATETIME;
            case field_type::timestamp:      return D_DB_TYPE_TIMESTAMP;
            case field_type::json:           return D_DB_TYPE_JSON;
            case field_type::xml:            return D_DB_TYPE_XML;
            case field_type::uuid:           return D_DB_TYPE_UUID;
            case field_type::null:
            case field_type::array:
            case field_type::custom:
            default:
                return D_DB_TYPE_NONE;
        }
    }

    /**
     * @brief Describes one column as the C layers take a column to create.
     *
     * @param[in] _column     the column; the description points into it, so
     *                        it must outlive the description.
     * @param[in] _key        whether the column belongs to the primary key.
     * @param[in] _operation  the operation's name, for a refusal's message.
     * @param[in] _key_place  its place in a composite key, from 1; 0 for the
     *                        columns' order.
     * @return the description.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for text holding a
     *         NUL, which the layer would read cut short, or a length or scale
     *         past what the layer's `unsigned` holds; with D_DB_UNSUPPORTED
     *         for a generated column, which the layers do not make.
     */
    inline d_db_column
    to_layer_column(
        const column_info& _column,
        bool               _key,
        const char*        _operation,
        unsigned           _key_place = 0u
    )
    {
        const std::size_t most = std::numeric_limits<unsigned>::max();
        unsigned          flags = 0u;

        // a column the database computes, which no layer makes
        if (_column.is_generated)
        {
            throw query_exception(std::string(_operation) + ": column '" +
                                      _column.name + "' is generated, which "
                                      "the layer does not make",
                                  D_DB_UNSUPPORTED);
        }

        // text the layer reads as C strings
        refuse_nul(_column.name, _operation, "a column's name");
        refuse_nul(_column.default_value.value_or(std::string()),
                   _operation,
                   "a column's default");
        refuse_nul(_column.sql_type.value_or(std::string()),
                   _operation,
                   "a column's type");

        // numbers the layer reads as unsigned
        if ( (_column.max_length.value_or(0u) > most) ||
             (_column.scale.value_or(0u) > most) )
        {
            throw query_exception(std::string(_operation) + ": column '" +
                                      _column.name + "' has a length or a "
                                      "scale too large",
                                  D_DB_INVALID_ARGUMENT);
        }

        flags |= (_key) ? D_DB_COLUMN_PRIMARY_KEY : 0u;
        flags |= (_column.is_auto_increment) ? D_DB_COLUMN_AUTO_INCREMENT
                                             : 0u;
        flags |= (!_column.nullable) ? D_DB_COLUMN_NOT_NULL : 0u;
        flags |= ( (_column.is_unique) &&
                   (!_key) ) ? D_DB_COLUMN_UNIQUE
                             : 0u;

        const d_db_column described =
        {
            _column.name.c_str(),
            to_layer_type(_column.type),
            flags,
            (_column.default_value.has_value())
                ? _column.default_value->c_str()
                : nullptr,
            static_cast<unsigned>(_column.max_length.value_or(0u)),
            static_cast<unsigned>(_column.scale.value_or(0u)),
            ( (_column.sql_type.has_value()) &&
              (!_column.sql_type->empty()) ) ? _column.sql_type->c_str()
                                             : nullptr,
            _key_place
        };

        return described;
    }

    /**
     * @brief Describes a schema's columns as the C layers take columns to
     *        create a table from.
     *
     * @param[in] _schema     the schema; the descriptions point into it, so
     *                        it must outlive them.
     * @param[in] _operation  the operation's name, for a refusal's message.
     * @return the descriptions, in the schema's order; a column is a key
     *         column when it is marked so or primary_key_columns names it,
     *         and the key's order is primary_key_columns', then the order of
     *         the columns marked so that it leaves out.
     * @throws as to_layer_column; and query_exception with
     *         D_DB_INVALID_ARGUMENT when primary_key_columns names a column
     *         the schema does not describe, or one twice.
     */
    inline std::vector<d_db_column>
    to_layer_columns(
        const table_schema& _schema,
        const char*         _operation
    )
    {
        const std::vector<std::string>& listed = _schema.primary_key_columns;
        std::vector<d_db_column>        described;
        unsigned                        next   =
            static_cast<unsigned>(listed.size());

        // the key's columns, each named once, each the schema's
        for (std::size_t i = 0; i < listed.size(); ++i)
        {
            if ( (!_schema.column_by_name(listed[i])) ||
                 (std::find(listed.begin(),
                            listed.begin() +
                                static_cast<std::ptrdiff_t>(i),
                            listed[i]) !=
                  listed.begin() + static_cast<std::ptrdiff_t>(i)) )
            {
                throw query_exception(std::string(_operation) +
                                          ": primary_key_columns names '" +
                                          listed[i] + "' twice, or a column "
                                          "the schema does not describe",
                                      D_DB_INVALID_ARGUMENT);
            }
        }

        described.reserve(_schema.columns.size());

        // each column, keyed where either list says so, at its place
        for (const column_info& column : _schema.columns)
        {
            const auto     found = std::find(listed.begin(),
                                             listed.end(),
                                             column.name);
            const bool     key   = ( (column.is_primary_key) ||
                                     (found != listed.end()) );
            const unsigned place =
                (found != listed.end())
                    ? static_cast<unsigned>(found - listed.begin()) + 1u
                    : ( (key) && (!listed.empty()) ) ? ++next
                                                     : 0u;

            described.push_back(to_layer_column(column,
                                                 key,
                                                 _operation,
                                                 place));
        }

        return described;
    }

NS_END  // internal


//==============================================================================
// 3.  DATABASE TABLE
//==============================================================================


// 3.1    The class
//------------------------------------------------------------------------------
// 3.1.1
// database_table
//   class template: a table of the database Connection reaches, and a cache
// of its rows, as the banner describes. Connection is the vendor's
// connection class (sqlite_connection, mariadb_connection, mysql_connection,
// pg_connection, or another with the same members); the table holds a
// pointer to it, which must outlive the table, and every exchange with the
// database goes through it. The members that ask the C layers --
// fetch_schema's description of the columns, exists(), and those that
// change the table -- need a connection with a face (get_face()), which
// every vendor with a layer has.
//   ValueType is a cell's type: `value` (database.hpp) by default, whose
// alternatives the table reads and binds; another type must convert from
// `value` for refresh() and hold `value`'s alternatives for commit(), and
// order and compare its values for a keyed commit. Config is carried for
// what builds on the table (config_type), and read by nothing here.
//   Element access is unchecked through cell() and operator[], checked
// through at() and the members by name, which throw std::out_of_range.
// Every member that reaches the database throws as the connection's
// operations do (database_connection.hpp), and connection_exception when
// the table has no open connection; changing a view's cache throws
// query_exception. Not copyable; movable, the table moved from bound to no
// connection and holding nothing.
//
// Usage:
//     pg_connection                 db(config);
//     database_table<pg_connection> people(db, "people");
//
//     db.connect();
//     people.fetch_schema();             // columns, and the key
//     people.set_where("age >= 18");
//     people.refresh();                  // the rows the filter selects
//     people.update_cell(0, "name", value(std::string("Ada")));
//     people.commit();                   // one UPDATE, by the key
template<typename Connection,
         typename ValueType = value,
         typename Config    = void>
class database_table
{
public:
    // container -- the standard container types; a value is a cell
    using value_type      = ValueType;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using allocator_type  = std::allocator<value_type>;

    // identity -- the table, its connection and description, and its rows,
    // each a table_row of cells in the schema's column order
    using self_type       = database_table<Connection, ValueType, Config>;
    using config_type     = Config;
    using connection_type = Connection;
    using schema_type     = table_schema;
    using row_type        = table_row<value_type>;
    using storage_type    = std::vector<row_type>;

    // iteration -- over rows; a cell is reached through its row
    using iterator               = typename storage_type::iterator;
    using const_iterator         = typename storage_type::const_iterator;
    using reverse_iterator       = typename storage_type::reverse_iterator;
    using const_reverse_iterator =
        typename storage_type::const_reverse_iterator;

    // has_dynamic_rows, has_dynamic_cols -- a table's extent is known only
    // once it is read
    static constexpr bool has_dynamic_rows = true;
    static constexpr bool has_dynamic_cols = true;

    // construction -- bound to no connection; to a connection and a table's
    // name, its columns read by fetch_schema() or refresh(); or to a
    // connection and a schema already known. Nothing is read: refresh()
    // reads the rows.
    database_table()
        : m_connection(nullptr),
          m_kind(table_kind::base_table),
          m_dirty(false),
          m_stale(true),
          m_loaded(false),
          m_from_table(true),
          m_read_paged(false)
    {}

    explicit database_table(
        Connection& _conn,
        std::string _table_name,
        table_kind  _kind = table_kind::base_table
    )
        : m_connection(&_conn),
          m_kind(_kind),
          m_dirty(false),
          m_stale(true),
          m_loaded(false),
          m_from_table(true),
          m_read_paged(false)
    {
        m_schema.table_name = std::move(_table_name);
    }

    explicit database_table(
        Connection&  _conn,
        table_schema _schema,
        table_kind   _kind = table_kind::base_table
    )
        : m_connection(&_conn),
          m_schema(std::move(_schema)),
          m_kind(_kind),
          m_dirty(false),
          m_stale(true),
          m_loaded(false),
          m_from_table(true),
          m_read_paged(false)
    {}

    explicit database_table(
        Connection&        _conn,
        table_schema       _schema,
        table_kind         _kind,
        const sync_config& _sync
    )
        : m_connection(&_conn),
          m_schema(std::move(_schema)),
          m_kind(_kind),
          m_sync(_sync),
          m_dirty(false),
          m_stale(true),
          m_loaded(false),
          m_from_table(true),
          m_read_paged(false)
    {}

    // copying -- whether a copy would share the connection or the rows is
    // not clear, so there is none
    database_table(const database_table&)            = delete;
    database_table& operator=(const database_table&) = delete;

    database_table(
        database_table&& _other
    ) noexcept
        : m_connection(_other.m_connection),
          m_schema(std::move(_other.m_schema)),
          m_kind(_other.m_kind),
          m_sync(std::move(_other.m_sync)),
          m_data(std::move(_other.m_data)),
          m_original(std::move(_other.m_original)),
          m_original_keys(std::move(_other.m_original_keys)),
          m_dirty(_other.m_dirty),
          m_stale(_other.m_stale),
          m_loaded(_other.m_loaded),
          m_from_table(_other.m_from_table),
          m_read_paged(_other.m_read_paged),
          m_last_refresh(_other.m_last_refresh),
          m_where_clause(std::move(_other.m_where_clause)),
          m_order_clause(std::move(_other.m_order_clause)),
          m_read_where(std::move(_other.m_read_where)),
          m_limit(_other.m_limit),
          m_offset(_other.m_offset),
          m_select_hint(std::move(_other.m_select_hint)),
          m_table_suffix(std::move(_other.m_table_suffix))
    {
        _other.m_reset_moved();
    }

    database_table&
    operator=(
        database_table&& _other
    ) noexcept
    {
        // a table assigned to itself keeps what it has
        if (this != &_other)
        {
            m_connection    = _other.m_connection;
            m_schema        = std::move(_other.m_schema);
            m_kind          = _other.m_kind;
            m_sync          = std::move(_other.m_sync);
            m_data          = std::move(_other.m_data);
            m_original      = std::move(_other.m_original);
            m_original_keys = std::move(_other.m_original_keys);
            m_dirty         = _other.m_dirty;
            m_stale         = _other.m_stale;
            m_loaded        = _other.m_loaded;
            m_from_table    = _other.m_from_table;
            m_read_paged    = _other.m_read_paged;
            m_last_refresh  = _other.m_last_refresh;
            m_where_clause  = std::move(_other.m_where_clause);
            m_order_clause  = std::move(_other.m_order_clause);
            m_read_where    = std::move(_other.m_read_where);
            m_limit         = _other.m_limit;
            m_offset        = _other.m_offset;
            m_select_hint   = std::move(_other.m_select_hint);
            m_table_suffix  = std::move(_other.m_table_suffix);

            _other.m_reset_moved();
        }

        return *this;
    }

    ~database_table() = default;

    // capacity -- cells, rows and columns of the cache; a table has as many
    // columns as its schema describes
    size_type
    size() const noexcept
    {
        return (rows() * cols());
    }

    bool
    empty() const noexcept
    {
        return m_data.empty();
    }

    size_type
    rows() const noexcept
    {
        return m_data.size();
    }

    size_type
    cols() const noexcept
    {
        return m_schema.columns.size();
    }

    size_type
    total_cells() const noexcept
    {
        return size();
    }

    // cell, operator[] -- unchecked access by row and column, or to a row;
    // the non-const forms read a stale cache again first, as the policy says
    reference
    cell(
        size_type _row,
        size_type _col
    )
    {
        ensure_fresh();

        return m_data[_row][_col];
    }

    const_reference
    cell(
        size_type _row,
        size_type _col
    ) const
    {
        return m_data[_row][_col];
    }

    row_type&
    operator[](
        size_type _row
    )
    {
        ensure_fresh();

        return m_data[_row];
    }

    const row_type&
    operator[](
        size_type _row
    ) const
    {
        return m_data[_row];
    }

    // at, cell_by_name -- checked access, by index or by the column's name;
    // std::out_of_range for a row, a column or a name the table lacks
    reference
    at(
        size_type _row,
        size_type _col
    )
    {
        ensure_fresh();
        check_bounds(_row, _col, "at");

        return m_data[_row][_col];
    }

    const_reference
    at(
        size_type _row,
        size_type _col
    ) const
    {
        check_bounds(_row, _col, "at");

        return m_data[_row][_col];
    }

    reference
    cell_by_name(
        size_type        _row,
        std::string_view _column_name
    )
    {
        ensure_fresh();

        const size_type col = resolve_column(_column_name, "cell_by_name");

        check_row(_row, "cell_by_name");

        return m_data[_row][col];
    }

    const_reference
    cell_by_name(
        size_type        _row,
        std::string_view _column_name
    ) const
    {
        const size_type col = resolve_column(_column_name, "cell_by_name");

        check_row(_row, "cell_by_name");

        return m_data[_row][col];
    }

    // get_row, get_column -- copies of a row, or of a column's cells, by
    // index or by name; checked as at() is
    row_type
    get_row(
        size_type _row
    ) const
    {
        check_row(_row, "get_row");

        return m_data[_row];
    }

    std::vector<value_type>
    get_column(
        size_type _col
    ) const
    {
        check_col(_col, "get_column");

        std::vector<value_type> cells;

        cells.reserve(rows());

        for (const row_type& record : m_data)
        {
            cells.push_back(record[_col]);
        }

        return cells;
    }

    std::vector<value_type>
    get_column(
        std::string_view _column_name
    ) const
    {
        return get_column(resolve_column(_column_name, "get_column"));
    }

    // iteration -- over the cache's rows
    iterator       begin()        noexcept { return m_data.begin();  }
    const_iterator begin()  const noexcept { return m_data.begin();  }
    const_iterator cbegin() const noexcept { return m_data.cbegin(); }
    iterator       end()          noexcept { return m_data.end();    }
    const_iterator end()    const noexcept { return m_data.end();    }
    const_iterator cend()   const noexcept { return m_data.cend();   }

    reverse_iterator       rbegin()        noexcept { return m_data.rbegin();  }
    const_reverse_iterator rbegin()  const noexcept { return m_data.rbegin();  }
    const_reverse_iterator crbegin() const noexcept { return m_data.crbegin(); }
    reverse_iterator       rend()          noexcept { return m_data.rend();    }
    const_reverse_iterator rend()    const noexcept { return m_data.rend();    }
    const_reverse_iterator crend()   const noexcept { return m_data.crend();   }

    /**
     * @brief Appends a row to the cache.
     *
     * @param[in] _row  the row, a cell for each column, in the schema's order.
     * @throws query_exception for a view, or a row of another width; and,
     *         where the change commits at once, what commit() throws, the
     *         row then in the cache and the cache still dirty.
     */
    void
    insert_row(
        const row_type& _row
    )
    {
        validate_mutable("insert_row");
        check_row_width(_row, "insert_row");

        m_data.push_back(_row);
        mark_dirty(modify_action::insert_row);

        return;
    }

    void
    insert_row(
        row_type&& _row
    )
    {
        validate_mutable("insert_row");
        check_row_width(_row, "insert_row");

        m_data.push_back(std::move(_row));
        mark_dirty(modify_action::insert_row);

        return;
    }

    // insert_rows -- appends rows, every width checked before any is added;
    // throws as insert_row
    void
    insert_rows(
        const std::vector<row_type>& _rows
    )
    {
        validate_mutable("insert_rows");

        // every width before any row goes in
        for (const row_type& record : _rows)
        {
            check_row_width(record, "insert_rows");
        }

        m_data.insert(m_data.end(), _rows.begin(), _rows.end());
        mark_dirty(modify_action::bulk_insert);

        return;
    }

    // remove_row -- removes a row from the cache; std::out_of_range for a row
    // it lacks, otherwise throws as insert_row
    void
    remove_row(
        size_type _row
    )
    {
        validate_mutable("remove_row");
        check_row(_row, "remove_row");

        m_data.erase(m_data.begin() + static_cast<difference_type>(_row));
        mark_dirty(modify_action::remove_row);

        return;
    }

    // add_row -- appends a row of NULLs, a cell for each column; throws as
    // insert_row
    void
    add_row()
    {
        validate_mutable("add_row");

        m_data.emplace_back(cols());
        mark_dirty(modify_action::insert_row);

        return;
    }

    // clear -- removes every row from the cache, which commit() then removes
    // from the table; throws as insert_row
    void
    clear()
    {
        validate_mutable("clear");

        m_data.clear();
        mark_dirty(modify_action::clear_all);

        return;
    }

    // update_cell -- sets a cell, by index or by the column's name;
    // std::out_of_range for a cell the table lacks, query_exception with
    // D_DB_UNSUPPORTED for a generated column's, which the database
    // computes, otherwise throws as insert_row
    void
    update_cell(
        size_type         _row,
        size_type         _col,
        const value_type& _value
    )
    {
        validate_mutable("update_cell");
        check_bounds(_row, _col, "update_cell");

        // a column the database computes takes nothing written
        if (m_schema.columns[_col].is_generated)
        {
            throw query_exception("database_table::update_cell: column '" +
                                      m_schema.columns[_col].name + "' is "
                                      "generated; the database computes it",
                                  D_DB_UNSUPPORTED);
        }

        m_data[_row][_col] = _value;
        mark_dirty(modify_action::update_cell);

        return;
    }

    void
    update_cell(
        size_type         _row,
        std::string_view  _column_name,
        const value_type& _value
    )
    {
        update_cell(_row, resolve_column(_column_name, "update_cell"), _value);

        return;
    }

    /**
     * @brief Appends a column to the schema, and a NULL to every cached row;
     *        the table in the database is not changed (alter_add_column
     *        changes it).
     *
     * @param[in] _col_info  the column.
     * @throws as insert_row.
     */
    void
    add_column(
        const column_info& _col_info
    )
    {
        validate_mutable("add_column");

        m_append_column(_col_info);
        mark_dirty(modify_action::add_column);

        return;
    }

    void
    add_column(
        std::string _name,
        field_type  _type = field_type::string
    )
    {
        add_column(column_info(std::move(_name), _type));

        return;
    }

    // remove_column -- removes a column from the schema and from every cached
    // row, by index or by name, leaving the table in the database as it is
    // (alter_drop_column changes it); std::out_of_range for a column the
    // table lacks, otherwise throws as insert_row
    void
    remove_column(
        size_type _col
    )
    {
        validate_mutable("remove_column");
        check_col(_col, "remove_column");

        m_erase_column(_col);
        mark_dirty(modify_action::remove_column);

        return;
    }

    void
    remove_column(
        std::string_view _column_name
    )
    {
        remove_column(resolve_column(_column_name, "remove_column"));

        return;
    }

    /**
     * @brief Gives the cache `_rows` rows, keeping the first and appending
     *        rows of NULLs.
     *
     * @param[in] _rows  the number of rows.
     * @throws as insert_row.
     */
    void
    resize(
        size_type _rows
    )
    {
        validate_mutable("resize");

        m_data.resize(_rows, row_type(cols()));
        mark_dirty(modify_action::clear_all);

        return;
    }

    /**
     * @brief As resize(_rows), for a table of `_cols` columns.
     *
     * @param[in] _rows  the number of rows.
     * @param[in] _cols  the number of columns, which must be cols(): a
     *                   table's columns are its schema's, which add_column
     *                   and remove_column change.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for another number
     *         of columns; and as insert_row.
     */
    void
    resize(
        size_type _rows,
        size_type _cols
    )
    {
        // the columns are the schema's
        if (_cols != cols())
        {
            throw query_exception("database_table::resize: a table has its "
                                  "schema's " + std::to_string(cols()) +
                                      " columns; add_column and remove_column "
                                      "change them",
                                  D_DB_INVALID_ARGUMENT);
        }

        resize(_rows);

        return;
    }

    // schema -- the description; a column's name and type, checked as at()
    // is; the number of columns; and the indexes of the primary key's
    // columns, in the key's order (key_columns throws query_exception with
    // D_DB_MISMATCH for a key column the schema does not describe)
    const table_schema&
    get_schema() const noexcept
    {
        return m_schema;
    }

    const std::string&
    column_name(
        size_type _col
    ) const
    {
        check_col(_col, "column_name");

        return m_schema.columns[_col].name;
    }

    field_type
    column_type(
        size_type _col
    ) const
    {
        check_col(_col, "column_type");

        return m_schema.columns[_col].type;
    }

    size_type
    column_count() const noexcept
    {
        return cols();
    }

    std::vector<size_type>
    key_columns() const
    {
        std::vector<size_type> keys;

        // the key's own list, in its order
        for (const std::string& name : m_schema.primary_key_columns)
        {
            const std::optional<std::size_t> index =
                m_schema.column_index(name);

            if (!index.has_value())
            {
                throw query_exception("database_table: the primary key's "
                                      "column '" + name + "' is not one of "
                                      "the table's columns",
                                      D_DB_MISMATCH);
            }

            keys.push_back(index.value());
        }

        // otherwise the columns marked, in the table's order
        if (keys.empty())
        {
            for (size_type c = 0; c < cols(); ++c)
            {
                if (m_schema.columns[c].is_primary_key)
                {
                    keys.push_back(c);
                }
            }
        }

        return keys;
    }

    // connection -- the connection the table is bound to, nullptr for none;
    // binding another forgets the rows read, the cache keeping them as rows
    // the new connection's table has not seen
    Connection*
    get_connection() noexcept
    {
        return m_connection;
    }

    const Connection*
    get_connection() const noexcept
    {
        return m_connection;
    }

    void
    set_connection(
        Connection& _conn
    )
    {
        m_connection = &_conn;
        m_forget_read();

        return;
    }

    // is_connected -- whether the table is bound to a connection that is open
    bool
    is_connected() const noexcept
    {
        return ( (m_connection != nullptr) &&
                 (m_connection->is_connected()) );
    }

    // identity -- the table's name; the schema it lives in, "" for the
    // connection's own, set_schema_name forgetting the rows read, as
    // set_connection does; the name qualified by the schema and quoted as
    // the vendor quotes names, for SQL; and the kind, which says whether its
    // cache can be changed
    const std::string&
    table_name() const noexcept
    {
        return m_schema.table_name;
    }

    const std::string&
    schema_name() const noexcept
    {
        return m_schema.schema_name;
    }

    void
    set_schema_name(
        std::string _schema_name
    )
    {
        m_schema.schema_name = std::move(_schema_name);
        m_forget_read();

        return;
    }

    /**
     * @brief The table's name for SQL: quoted as the vendor quotes names, and
     *        qualified by its schema when it has one.
     *
     * @return the name.
     * @throws connection_exception when the table has no connection, whose
     *         vendor decides the quoting; exception with D_DB_INVALID_ARGUMENT
     *         for a name holding a NUL.
     */
    std::string
    qualified_name() const
    {
        validate_bound("qualified_name");

        const database_type type   = m_connection->get_database_type();
        std::string         quoted;

        // the schema first, where there is one
        if (!m_schema.schema_name.empty())
        {
            quoted = quote_identifier(m_schema.schema_name, type) + ".";
        }

        return quoted + quote_identifier(m_schema.table_name, type);
    }

    table_kind
    kind() const noexcept
    {
        return m_kind;
    }

    bool
    is_view() const noexcept
    {
        return (m_kind == table_kind::view);
    }

    bool
    is_mutable() const noexcept
    {
        return ( (m_kind == table_kind::base_table) ||
                 (m_kind == table_kind::temporary) );
    }

    // synchronization -- the policy; whether the cache holds changes not yet
    // committed; whether it may be behind the database: never read, its
    // query changed, invalidated, or, under the periodic policy, read longer
    // ago than the interval; and when it was last read
    const sync_config&
    get_sync_config() const noexcept
    {
        return m_sync;
    }

    void
    set_sync_config(
        const sync_config& _config
    )
    {
        m_sync = _config;

        return;
    }

    // is_dirty -- whether the cache holds what commit() would write: a
    // change through the table's members, or a cell or a row changed
    // through the references and iterators element access gives, compared
    // with the rows read; rows only reordered are no change
    bool
    is_dirty() const
    {
        return ( (m_dirty) ||
                 (m_cache_differs()) );
    }

    bool
    is_stale() const noexcept
    {
        // marked so
        if (m_stale)
        {
            return true;
        }

        // or read longer ago than a periodic cache keeps
        if (m_sync.policy == sync_policy::periodic)
        {
            const std::chrono::steady_clock::duration elapsed =
                std::chrono::steady_clock::now() - m_last_refresh;

            return (elapsed >= m_sync.refresh_interval);
        }

        return false;
    }

    void
    invalidate() noexcept
    {
        m_stale = true;

        return;
    }

    std::chrono::steady_clock::time_point
    last_refresh() const noexcept
    {
        return m_last_refresh;
    }

    // query -- what refresh() reads: the filter, written as a WHERE clause's
    // condition without the keyword; the order, as ORDER BY's list; and a
    // page, a limit and an offset. Each is SQL as given, and each change
    // marks the cache stale. commit() replaces, without a key, the rows the
    // filter of the last refresh() selected.
    void
    set_where(
        std::string _clause
    )
    {
        m_where_clause = std::move(_clause);
        m_stale        = true;

        return;
    }

    void
    clear_where()
    {
        m_where_clause.clear();
        m_stale = true;

        return;
    }

    const std::string&
    get_where() const noexcept
    {
        return m_where_clause;
    }

    void
    set_order(
        std::string _clause
    )
    {
        m_order_clause = std::move(_clause);
        m_stale        = true;

        return;
    }

    void
    clear_order()
    {
        m_order_clause.clear();
        m_stale = true;

        return;
    }

    const std::string&
    get_order() const noexcept
    {
        return m_order_clause;
    }

    void
    set_limit(
        std::optional<size_type> _limit
    )
    {
        m_limit = _limit;
        m_stale = true;

        return;
    }

    std::optional<size_type>
    get_limit() const noexcept
    {
        return m_limit;
    }

    void
    set_offset(
        std::optional<size_type> _offset
    )
    {
        m_offset = _offset;
        m_stale  = true;

        return;
    }

    std::optional<size_type>
    get_offset() const noexcept
    {
        return m_offset;
    }

    /**
     * @brief Describes the table's columns from the database: each one's
     *        name and type, as a query that reads no rows reports them, and,
     *        where the vendor's layer can describe the table, whether it
     *        takes NULL, its default, its declared type and its place in the
     *        primary key, which primary_key_columns then lists in order.
     *
     * @note A MariaDB or MySQL temporary table, which no catalog lists, is
     *       described by the query alone.
     *
     * @throws connection_exception when the table has no open connection;
     *         query_exception, with the layer's result, when the table
     *         cannot be read (a missing table among the reasons).
     * @post The schema's columns and key are the database's. Where their
     *       names changed, the cache no longer holds any row, and is stale.
     */
    void
    fetch_schema()
    {
        validate_connected("fetch_schema");

        std::unique_ptr<buffered_result_set> probe =
            m_connection->execute_query("SELECT * FROM " + qualified_name() +
                                        " WHERE 1=0");
        std::vector<column_info> columns;
        std::vector<std::string> keys;

        columns.reserve(probe->column_count());

        // the names and types a query's description gives
        for (size_type c = 0; c < probe->column_count(); ++c)
        {
            column_info column;

            column.name = probe->column_name(c);
            column.type = probe->column_type(c);

            columns.push_back(std::move(column));
        }

        // and what the layer adds, where there is a layer that can see the
        // table: a MariaDB or MySQL temporary one it would take for the base
        // table of its name
        if constexpr (is_detected<internal::face_hook_t, Connection>::value)
        {
            if ( (m_kind != table_kind::temporary) ||
                 (!is_mysql_family()) )
            {
                m_describe(columns, keys);
            }
        }

        m_install_columns(std::move(columns), std::move(keys));

        return;
    }

    /**
     * @brief Reads the rows the query selects into the cache, replacing what
     *        it held: the schema's columns, by name, from the table, filtered,
     *        ordered and paged as the query configuration says.
     *
     * @note A table whose columns are not yet known takes those the table
     *       has, by name and type, as fetch_schema() would without the
     *       layer's description.
     *
     * @throws connection_exception when the table has no open connection;
     *         query_exception, with the layer's result, when the query fails
     *         (a filter in error, a column the table lacks).
     * @post On success the cache holds the rows, is neither dirty nor stale,
     *       and is what commit() compares with; on failure it is as it was.
     */
    void
    refresh()
    {
        validate_connected("refresh");

        storage_type             rows;
        std::vector<column_info> adopted;

        m_read(rows, adopted);
        m_take(std::move(rows), std::move(adopted));

        return;
    }

    /**
     * @brief Writes the cache's changes to the table, as one unit, as the
     *        banner describes.
     *
     * @note A row keeps the row read it stands for (table_row) through
     *       element access, sorts and copies, so a changed key is an UPDATE
     *       of that row, in an order that frees each new key before it is
     *       taken; rows exchanging keys are written only under a key the
     *       database can defer (PostgreSQL's DEFERRABLE).
     *
     * @throws connection_exception when the table has no open connection;
     *         query_exception for a view's cache; with D_DB_UNSUPPORTED
     *         without a primary key for rows read a page at a time, for rows
     *         read from the table's history (mariadb_table), for a changed
     *         generated column's cell, for rows exchanging keys under a key
     *         the database cannot defer, and for a changed or removed row
     *         read with a NULL in its key; with D_DB_MISMATCH when the key is
     *         not the one the rows were read with; with D_DB_INVALID_ARGUMENT
     *         when two rows hold the same key; with D_DB_NOT_FOUND for a row
     *         whose key changed that another session deleted; and what the
     *         statements throw, the layer's result with it, after the unit is
     *         rolled back; transaction_exception when the unit cannot begin
     *         or end. Every refusal but the last three comes before anything
     *         is written.
     * @post On success the cache is not dirty, and is what the next commit()
     *       compares with; a cache that left values to the database, or held
     *       rows the table had not been read for, is read again. On failure
     *       the database is as it was, and the cache as it was.
     */
    void
    commit()
    {
        commit_with(
            [this](const std::vector<const row_type*>& _rows) -> bool
            {
                return insert_cached_rows(_rows);
            });

        return;
    }

    /**
     * @brief Says whether the table is in the database: a base table, a view,
     *        or, for the other kinds, anything the layer can describe by the
     *        name, in the table's schema.
     *
     * @return whether it is there.
     * @throws connection_exception when the table has no open connection;
     *         query_exception, with the layer's result, when the database
     *         cannot tell. Asking never fails a transaction.
     */
    bool
    exists() const
    {
        validate_connected("exists");

        // the layer's question, where there is a layer
        if constexpr (is_detected<internal::face_hook_t, Connection>::value)
        {
            // a MariaDB or MySQL temporary table, which no catalog lists
            if ( (m_kind == table_kind::temporary) &&
                 (is_mysql_family()) )
            {
                return m_probe_mysql_temporary();
            }

            bool found = false;

            m_connection->ask_layer(
                "exists",
                [this, &found](const auto& _db) -> int
                {
                    const face::string_arg scope = layer_scope();
                    const face::string_arg name(m_schema.table_name);

                    // a table, a view, or anything with columns
                    switch (m_kind)
                    {
                        case table_kind::base_table:
                            return _db.has_table_s(scope, name, found);

                        case table_kind::view:
                            return _db.has_view_s(scope, name, found);

                        default:
                        {
                            face::column_list columns;
                            const int         result =
                                _db.list_columns(scope, name, columns);

                            found = (result == 0);

                            return (result == D_DB_NOT_FOUND) ? 0 : result;
                        }
                    }
                });

            return found;
        }
        else
        {
            // otherwise a query that reads nothing, failing for no table
            try
            {
                (void)m_connection->execute_query("SELECT 1 FROM " +
                                                  qualified_name() +
                                                  " WHERE 1=0");
            }
            catch (const query_exception&)
            {
                return false;
            }

            return true;
        }
    }

    /**
     * @brief Counts the table's rows in the database, those the filter
     *        selects where there is one, without reading them.
     *
     * @return the count.
     * @throws connection_exception when the table has no open connection;
     *         query_exception, with the layer's result, when the query fails.
     */
    re_std::int64_t
    row_count_remote() const
    {
        validate_connected("row_count_remote");

        std::string query = "SELECT COUNT(*) FROM " + qualified_name();

        if (!m_where_clause.empty())
        {
            query += " WHERE " + m_where_clause;
        }

        std::unique_ptr<buffered_result_set> counted =
            m_connection->execute_query(query);

        // the one row's one number
        if (counted->next())
        {
            const std::optional<re_std::int64_t> count =
                counted->get_long(static_cast<std::size_t>(0));

            if (count.has_value())
            {
                return count.value();
            }
        }

        return 0;
    }

    /**
     * @brief Creates the table the schema describes, in its schema, through
     *        the vendor's layer, which spells each column's type.
     *
     * @note A composite primary key takes its columns in the order
     *       primary_key_columns lists them, then any other column marked
     *       is_primary_key, in the table's order. MariaDB and MySQL commit an
     *       open transaction before it.
     *
     * @param[in] _if_not_exists  whether a table already there is a success.
     * @throws connection_exception when the table has no open connection;
     *         query_exception for a view; with D_DB_INVALID_ARGUMENT for a
     *         schema of no columns, or text holding a NUL; with
     *         D_DB_UNSUPPORTED for a temporary table on MariaDB or MySQL,
     *         whose layer creates none; and with the layer's result when the
     *         database refuses (the table exists, a column the layer
     *         refuses).
     */
    void
    create_table(
        bool _if_not_exists = true
    )
    {
        create_table_with((_if_not_exists) ? D_DB_IF_NOT_EXISTS : 0u);

        return;
    }

    /**
     * @brief Drops the table, its indexes and triggers with it.
     *
     * @param[in] _if_exists  whether a table not there is a success.
     * @throws as create_table, without its refusals of the schema.
     * @post The cache holds no row, and no rows read.
     */
    void
    drop_table(
        bool _if_exists = true
    )
    {
        validate_connected("drop_table");
        validate_mutable("drop_table");

        m_connection->run_layer(
            "drop_table",
            [this, _if_exists](auto& _db) -> int
            {
                return _db.drop_table(layer_scope(),
                                      face::string_arg(m_schema.table_name),
                                      (_if_exists) ? D_DB_IF_EXISTS : 0u);
            });

        m_data.clear();
        m_forget_read();
        m_dirty = false;

        return;
    }

    /**
     * @brief Renames the table, within its schema.
     *
     * @param[in] _new_name  the new name.
     * @throws as drop_table; query_exception with D_DB_INVALID_ARGUMENT for a
     *         name holding a NUL.
     * @post The table and its cache go by the new name.
     */
    void
    rename_table(
        const std::string& _new_name
    )
    {
        validate_connected("rename_table");
        validate_mutable("rename_table");
        internal::refuse_nul(_new_name,
                             "database_table::rename_table",
                             "the name");

        m_connection->run_layer(
            "rename_table",
            [this, &_new_name](auto& _db) -> int
            {
                return _db.rename_table(layer_scope(),
                                        face::string_arg(m_schema.table_name),
                                        face::string_arg(_new_name));
            });

        m_schema.table_name = _new_name;

        return;
    }

    /**
     * @brief Removes every row from the table, and from the cache.
     *
     * @note SQLite's layer restarts the table's AUTOINCREMENT; MariaDB,
     *       MySQL and PostgreSQL do as their TRUNCATE does.
     *
     * @throws as drop_table.
     * @post The cache holds no row, and stands for the empty table.
     */
    void
    truncate_table()
    {
        validate_connected("truncate_table");
        validate_mutable("truncate_table");

        m_connection->run_layer(
            "truncate_table",
            [this](auto& _db) -> int
            {
                return _db.truncate_table(
                    layer_scope(),
                    face::string_arg(m_schema.table_name));
            });

        m_data.clear();
        m_original.clear();
        m_original_keys = key_columns();
        m_loaded        = true;
        m_from_table    = true;
        m_read_paged    = false;
        m_read_where.clear();
        m_dirty         = false;

        return;
    }

    /**
     * @brief Adds a column to the table, through the vendor's layer, and to
     *        the schema; a cache that was read is read again, for the values
     *        the database gave the new column.
     *
     * @param[in] _column  the column; a key column only on a table without a
     *                     key.
     * @throws as create_table; query_exception with D_DB_UNSUPPORTED while
     *         the cache holds changes not yet committed.
     */
    void
    alter_add_column(
        const column_info& _column
    )
    {
        validate_connected("alter_add_column");
        validate_mutable("alter_add_column");
        validate_clean("alter_add_column");

        const d_db_column described =
            internal::to_layer_column(_column,
                                      _column.is_primary_key,
                                      "database_table::alter_add_column");

        m_connection->run_layer(
            "add_column",
            [this, &described](auto& _db) -> int
            {
                return _db.add_column(layer_scope(),
                                      face::string_arg(m_schema.table_name),
                                      described);
            });

        m_append_column(_column);

        // a key named by the column alone
        if (_column.is_primary_key)
        {
            m_schema.primary_key_columns.push_back(_column.name);
        }

        // what the database gave the rows it had
        if (m_loaded)
        {
            refresh();
        }

        return;
    }

    /**
     * @brief Drops a column from the table, through the vendor's layer, and
     *        from the schema and the cache.
     *
     * @param[in] _name  the column's name.
     * @throws as alter_add_column; std::out_of_range for a column the schema
     *         lacks.
     */
    void
    alter_drop_column(
        std::string_view _name
    )
    {
        validate_connected("alter_drop_column");
        validate_mutable("alter_drop_column");
        validate_clean("alter_drop_column");

        const size_type   col  = resolve_column(_name, "alter_drop_column");
        const std::string name = m_schema.columns[col].name;

        m_connection->run_layer(
            "drop_column",
            [this, &name](auto& _db) -> int
            {
                return _db.drop_column(layer_scope(),
                                       face::string_arg(m_schema.table_name),
                                       face::string_arg(name));
            });

        m_erase_column(col);

        return;
    }

    /**
     * @brief Renames a column of the table, through the vendor's layer, and
     *        in the schema.
     *
     * @param[in] _name      the column's name.
     * @param[in] _new_name  its new name.
     * @throws as alter_drop_column; query_exception with
     *         D_DB_INVALID_ARGUMENT for a name holding a NUL.
     */
    void
    alter_rename_column(
        std::string_view   _name,
        const std::string& _new_name
    )
    {
        validate_connected("alter_rename_column");
        validate_mutable("alter_rename_column");
        internal::refuse_nul(_new_name,
                             "database_table::alter_rename_column",
                             "the name");

        const size_type   col  = resolve_column(_name, "alter_rename_column");
        const std::string name = m_schema.columns[col].name;

        m_connection->run_layer(
            "rename_column",
            [this, &name, &_new_name](auto& _db) -> int
            {
                return _db.rename_column(
                    layer_scope(),
                    face::string_arg(m_schema.table_name),
                    face::string_arg(name),
                    face::string_arg(_new_name));
            });

        m_schema.columns[col].name = _new_name;

        // the key's list, where it names the column
        for (std::string& key : m_schema.primary_key_columns)
        {
            if (key == name)
            {
                key = _new_name;
            }
        }

        return;
    }

    // comparison -- two tables are equal when they have the same name and
    // their caches hold the same rows
    bool
    operator==(
        const self_type& _other
    ) const
    {
        return ( (m_schema.table_name == _other.m_schema.table_name) &&
                 (cols() == _other.cols()) &&
                 (m_data == _other.m_data) );
    }

    bool
    operator!=(
        const self_type& _other
    ) const
    {
        return !(*this == _other);
    }

protected:
    // commit_plan -- what a keyed commit writes, in the order it writes it:
    // the rows read that the cache no longer holds, deleted; the rows whose
    // key changed, each with the row read it stands for, updated in place in
    // an order that frees each new key before it is taken; the other changed
    // rows, updated; the rows the cache adds, inserted. Rows that exchange
    // keys are updated under the key's constraint deferred, `deferred` naming
    // it, where the database can defer it (PostgreSQL's DEFERRABLE)
    struct commit_plan
    {
        using pair_type = std::pair<const row_type*, const row_type*>;

        std::vector<const row_type*> gone;
        std::vector<pair_type>       rekeyed;
        std::vector<pair_type>       changed;
        std::vector<const row_type*> fresh;
        std::string                  deferred;
        bool                         initially_deferred = false;
    };

    // adopt_read_row -- numbers a row the database now holds as a row read,
    // for a vendor's table that adds one to the cache and to the rows read
    // alike (mariadb_table's RETURNING)
    static void
    adopt_read_row(
        row_type& _row
    ) noexcept
    {
        _row.m_origin = internal::next_row_origins(1u);

        return;
    }

    // validate_bound, validate_connected -- connection_exception unless the
    // table has a connection, an open one
    void
    validate_bound(
        const char* _caller
    ) const
    {
        if (m_connection == nullptr)
        {
            throw connection_exception(std::string("database_table::") +
                                       _caller +
                                       ": the table has no connection");
        }

        return;
    }

    void
    validate_connected(
        const char* _caller
    ) const
    {
        if (!is_connected())
        {
            throw connection_exception(std::string("database_table::") +
                                       _caller + ": no active connection");
        }

        return;
    }

    // validate_mutable -- query_exception unless the table's kind can be
    // changed
    void
    validate_mutable(
        const char* _caller
    ) const
    {
        if (!is_mutable())
        {
            throw query_exception(std::string("database_table::") + _caller +
                                  ": a view cannot be changed");
        }

        return;
    }

    // validate_clean -- query_exception with D_DB_UNSUPPORTED while the cache
    // holds changes not yet committed, which reading it again would lose
    void
    validate_clean(
        const char* _caller
    ) const
    {
        if (is_dirty())
        {
            throw query_exception(std::string("database_table::") + _caller +
                                      ": the cache holds changes not yet "
                                      "committed; commit() or refresh() "
                                      "first",
                                  D_DB_UNSUPPORTED);
        }

        return;
    }

    // check_row, check_col, check_bounds -- std::out_of_range for a row, a
    // column or a cell the table lacks
    void
    check_row(
        size_type   _row,
        const char* _caller
    ) const
    {
        if (_row >= rows())
        {
            throw std::out_of_range(std::string("database_table::") +
                                    _caller + ": row index out of range");
        }

        return;
    }

    void
    check_col(
        size_type   _col,
        const char* _caller
    ) const
    {
        if (_col >= cols())
        {
            throw std::out_of_range(std::string("database_table::") +
                                    _caller + ": column index out of range");
        }

        return;
    }

    void
    check_bounds(
        size_type   _row,
        size_type   _col,
        const char* _caller
    ) const
    {
        check_row(_row, _caller);
        check_col(_col, _caller);

        return;
    }

    // check_row_width -- query_exception for a row of another width than the
    // table's
    void
    check_row_width(
        const row_type& _row,
        const char*     _caller
    ) const
    {
        if (_row.size() != cols())
        {
            throw query_exception(std::string("database_table::") + _caller +
                                  ": the row has " +
                                  std::to_string(_row.size()) +
                                  " cells for " + std::to_string(cols()) +
                                  " columns");
        }

        return;
    }

    // resolve_column -- a column's index by name; std::out_of_range for a
    // name the schema lacks
    size_type
    resolve_column(
        std::string_view _name,
        const char*      _caller
    ) const
    {
        const std::optional<std::size_t> index = m_schema.column_index(_name);

        if (!index.has_value())
        {
            throw std::out_of_range(std::string("database_table::") +
                                    _caller + ": unknown column '" +
                                    std::string(_name) + "'");
        }

        return index.value();
    }

    // mark_dirty -- the cache changed; committed at once where the policy or
    // auto_commit says so for a change of that kind
    void
    mark_dirty(
        modify_action _action
    )
    {
        m_dirty = true;

        if ( ( (m_sync.auto_commit) ||
               (m_sync.policy == sync_policy::on_modify) ) &&
             (m_sync.tracks_action(_action)) &&
             (is_connected()) )
        {
            commit();
        }

        return;
    }

    // ensure_fresh -- reads a stale cache again where the policy says so,
    // unless it holds changes not yet committed
    void
    ensure_fresh()
    {
        if ( ( (m_sync.policy == sync_policy::on_access) ||
               (m_sync.policy == sync_policy::periodic) ) &&
             (is_stale())                                   &&
             (is_connected())                               &&
             (!is_dirty()) )
        {
            refresh();
        }

        return;
    }

    // build_select_query -- refresh()'s query: the schema's columns, or every
    // column while none is known; the hint a vendor writes after SELECT and
    // the clause after the table's name; then the filter, the order and the
    // page
    std::string
    build_select_query() const
    {
        const database_type type  = m_connection->get_database_type();
        std::string         query = "SELECT ";

        if (!m_select_hint.empty())
        {
            query += m_select_hint + " ";
        }

        // the columns, by name
        if (m_schema.columns.empty())
        {
            query += "*";
        }
        else
        {
            for (size_type c = 0; c < cols(); ++c)
            {
                query += (c > 0) ? ", " : "";
                query += quote_identifier(m_schema.columns[c].name, type);
            }
        }

        query += " FROM " + qualified_name() + m_table_suffix;

        if (!m_where_clause.empty())
        {
            query += " WHERE " + m_where_clause;
        }

        if (!m_order_clause.empty())
        {
            query += " ORDER BY " + m_order_clause;
        }

        return query + internal::dialect_format_limit_offset(type,
                                                             m_limit,
                                                             m_offset);
    }

    /**
     * @brief create_table, with the flags the vendor's layer takes: the
     *        shared ones and the vendor's own (sqlite_table's STRICT, ...).
     *
     * @param[in] _flags  the flags.
     * @throws as create_table.
     */
    void
    create_table_with(
        unsigned _flags
    )
    {
        validate_connected("create_table");
        validate_mutable("create_table");

        // a table needs a column
        if (m_schema.columns.empty())
        {
            throw query_exception("database_table::create_table: the schema "
                                  "describes no columns",
                                  D_DB_INVALID_ARGUMENT);
        }

        // MariaDB's layer creates no temporary table
        if ( (m_kind == table_kind::temporary) &&
             (is_mysql_family()) )
        {
            throw query_exception("database_table::create_table: the MariaDB "
                                  "layer creates no temporary table; execute "
                                  "CREATE TEMPORARY TABLE",
                                  D_DB_UNSUPPORTED);
        }

        const std::vector<d_db_column> columns =
            internal::to_layer_columns(m_schema,
                                       "database_table::create_table");

        m_connection->run_layer(
            "create_table",
            [this, &columns, _flags](auto& _db) -> int
            {
                return _db.create_table(layer_scope(),
                                        face::string_arg(m_schema.table_name),
                                        columns.data(),
                                        columns.size(),
                                        _flags);
            });

        return;
    }

    /**
     * @brief The commit commit() describes, the rows to insert inserted by
     *        `_insert`, which a vendor's table may do its own way.
     *
     * @param[in] _insert  called at most once, inside the unit, with the rows
     *                     to insert, in the cache's order; inserts them, and
     *                     returns whether it left a value to the database.
     * @throws as commit().
     */
    template<typename Inserter>
    void
    commit_with(
        Inserter&& _insert
    )
    {
        validate_connected("commit");
        validate_mutable("commit");

        // nothing changed, nothing to write
        if (!is_dirty())
        {
            return;
        }

        // rows read from elsewhere than the table itself
        if (!m_from_table)
        {
            throw query_exception("database_table::commit: the cache holds "
                                  "rows read from the table's history; "
                                  "refresh() reads the table",
                                  D_DB_UNSUPPORTED);
        }

        // rows of no columns
        if ( (m_schema.columns.empty()) &&
             (!m_data.empty()) )
        {
            throw query_exception("database_table::commit: the schema "
                                  "describes no columns",
                                  D_DB_INVALID_ARGUMENT);
        }

        const std::vector<size_type> keys       = key_columns();
        const bool                   was_loaded = m_loaded;

        // without a key, rows read a page at a time cannot be replaced
        if ( (keys.empty()) &&
             (was_loaded) &&
             (m_read_paged) )
        {
            throw query_exception("database_table::commit: without a primary "
                                  "key, commit replaces the rows refresh() "
                                  "read, which a limit or an offset leaves "
                                  "unnamed",
                                  D_DB_UNSUPPORTED);
        }

        // with one, the rows are matched by the key they were read with
        if ( (!keys.empty()) &&
             (was_loaded) &&
             (keys != m_original_keys) )
        {
            throw query_exception("database_table::commit: the primary key is "
                                  "not the one the rows were read with; "
                                  "refresh() reads them again",
                                  D_DB_MISMATCH);
        }

        // what the commit writes, settled and checked before anything is
        // written, so that a refusal leaves the database and the cache as
        // they were
        m_refuse_generated_changes();

        const commit_plan        plan = (keys.empty())
                                            ? commit_plan()
                                            : m_plan(keys, was_loaded);
        storage_type             reread;
        std::vector<column_info> adopted;
        bool                     read_again = false;

        m_in_unit(
            [&]() -> void
            {
                const bool left = (keys.empty())
                                      ? m_replace(was_loaded, _insert)
                                      : m_differ(plan, keys, _insert);

                // what the database chose, or the rows a replace needs
                read_again = ( (left) ||
                               ( (keys.empty()) &&
                                 (!was_loaded) ) );

                if (read_again)
                {
                    m_read(reread, adopted);
                }

                return;
            });

        // the cache as the database now holds it
        if (read_again)
        {
            m_take(std::move(reread), std::move(adopted));
        }
        else
        {
            m_mark_read();
            m_original_keys = keys;
            m_loaded        = true;
            m_dirty         = false;
        }

        return;
    }

    /**
     * @brief Inserts rows, one statement for each set of columns written.
     *
     * @param[in] _rows  the rows.
     * @return whether a cell was left to the database: a generated column's,
     *         or a NULL in a column that refuses NULL, or in a key.
     * @throws what the statements throw.
     */
    bool
    insert_cached_rows(
        const std::vector<const row_type*>& _rows
    )
    {
        using statement_ptr =
            decltype(std::declval<Connection&>().prepare(
                std::declval<const std::string&>()));

        const std::vector<size_type>              keys = key_columns();
        std::map<std::vector<bool>, statement_ptr> statements;
        bool                                       left = false;

        for (const row_type* record : _rows)
        {
            std::vector<bool> written(cols());

            // the cells written, the others left to the database
            for (size_type c = 0; c < cols(); ++c)
            {
                written[c] = !left_to_database((*record)[c], c, keys);
                left       = ( (left) ||
                               (!written[c]) );
            }

            typename std::map<std::vector<bool>, statement_ptr>::iterator
                found = statements.find(written);

            // one statement for each set of columns
            if (found == statements.end())
            {
                found = statements.emplace(
                    written,
                    m_connection->prepare(insert_sql(written))).first;
            }

            auto&     statement = *found->second;
            size_type number    = 0;

            statement.clear_parameters();

            for (size_type c = 0; c < cols(); ++c)
            {
                if (written[c])
                {
                    bind_value(statement, ++number, (*record)[c]);
                }
            }

            (void)statement.execute_update();
        }

        return left;
    }

    /**
     * @brief Whether a cell is left out of its row's INSERT, for the database
     *        to fill: a generated column's, whatever it holds, or a NULL in a
     *        column that refuses NULL, or in a key.
     *
     * @param[in] _cell  the cell.
     * @param[in] _col   its column.
     * @param[in] _keys  the key's columns.
     * @return whether it is left out.
     */
    bool
    left_to_database(
        const value_type&             _cell,
        size_type                     _col,
        const std::vector<size_type>& _keys
    ) const
    {
        // a column the database computes takes nothing written
        if (m_schema.columns[_col].is_generated)
        {
            return true;
        }

        // otherwise only a NULL is left to the database
        if (!m_is_null(_cell))
        {
            return false;
        }

        return ( (!m_schema.columns[_col].nullable) ||
                 (std::find(_keys.begin(), _keys.end(), _col) !=
                  _keys.end()) );
    }

    /**
     * @brief The INSERT of a row whose cells `_written` marks.
     *
     * @param[in] _written  for each column, whether its cell is written.
     * @return the statement, a parameter for each cell written; with none,
     *         a row of defaults. PostgreSQL's overrides an identity that
     *         would refuse a value given (GENERATED ALWAYS).
     */
    std::string
    insert_sql(
        const std::vector<bool>& _written
    ) const
    {
        const database_type type    = m_connection->get_database_type();
        std::string         columns;
        std::string         values;
        size_type           number  = 0;

        // the columns written, each with its parameter
        for (size_type c = 0; c < cols(); ++c)
        {
            if (_written[c])
            {
                columns += (number > 0) ? ", " : "";
                values  += (number > 0) ? ", " : "";
                columns += quote_identifier(m_schema.columns[c].name, type);
                values  += internal::dialect_placeholder(type, ++number);
            }
        }

        // no column written: a row of defaults
        if (number == 0)
        {
            return "INSERT INTO " + qualified_name() +
                   ((is_mysql_family()) ? " () VALUES ()"
                                          : " DEFAULT VALUES");
        }

        return "INSERT INTO " + qualified_name() + " (" + columns + ")" +
               ((type == database_type::postgresql)
                    ? " OVERRIDING SYSTEM VALUE"
                    : "") +
               " VALUES (" + values + ")";
    }

    // bind_value -- binds one cell, the index from 1: a `value` through a
    // statement's own bind_value, which binds a point in time as its vendor
    // reads one; otherwise each alternative through its bind method, a point
    // in time as UTC text
    template<typename Statement>
    static void
    bind_value(
        Statement&        _stmt,
        std::size_t       _index,
        const value_type& _v
    )
    {
        // the statement binds the whole value, where it can
        if constexpr ( (std::is_same_v<value_type, value>) &&
                       (internal::has_own<
                            internal::statement_bind_value_base_t,
                            internal::statement_bind_value_t,
                            Statement>::value) )
        {
            _stmt.bind_value(_index, _v);
        }
        else
        {
            bind_each(_stmt, _index, _v);
        }

        return;
    }

    // bind_each -- bind_value's way for a statement without bind_value
    template<typename Statement>
    static void
    bind_each(
        Statement&        _stmt,
        std::size_t       _index,
        const value_type& _v
    )
    {
        std::visit(
            [&_stmt, _index](const auto& _arg) -> void
            {
                using arg_t = std::decay_t<decltype(_arg)>;

                if constexpr (std::is_same_v<arg_t, std::monostate>)
                {
                    _stmt.bind_null(_index);
                }
                else if constexpr (std::is_same_v<arg_t, bool>)
                {
                    _stmt.bind_bool(_index, _arg);
                }
                else if constexpr (std::is_same_v<arg_t, re_std::int32_t>)
                {
                    _stmt.bind_int(_index, _arg);
                }
                else if constexpr (std::is_same_v<arg_t, re_std::int64_t>)
                {
                    _stmt.bind_long(_index, _arg);
                }
                else if constexpr (std::is_same_v<arg_t, double>)
                {
                    _stmt.bind_double(_index, _arg);
                }
                else if constexpr (std::is_same_v<arg_t, std::string>)
                {
                    _stmt.bind_string(_index, _arg);
                }
                else if constexpr (std::is_same_v<
                                       arg_t,
                                       std::vector<re_std::uint8_t>>)
                {
                    _stmt.bind_binary(_index, _arg);
                }
                else if constexpr (std::is_same_v<
                                       arg_t,
                                       std::chrono::system_clock::time_point>)
                {
                    // a point in time, as its UTC date and time: never NULL,
                    // which would lose it
                    _stmt.bind_string(_index, value_to_string(value(_arg)));
                }
                else
                {
                    static_assert(!std::is_same_v<arg_t, arg_t>,
                                  "database_table binds the alternatives of "
                                  "djinterp::value; this value_type holds "
                                  "another");
                }

                return;
            },
            _v);

        return;
    }

    // key_of -- a row's key, its integers widened so that the same number
    // read and written matches; false for a key holding a NULL
    bool
    key_of(
        const row_type&               _row,
        const std::vector<size_type>& _keys,
        std::vector<value_type>&      _key
    ) const
    {
        _key.clear();
        _key.reserve(_keys.size());

        for (const size_type key : _keys)
        {
            if (m_is_null(_row[key]))
            {
                return false;
            }

            _key.push_back(m_widened(_row[key]));
        }

        return true;
    }

    // is_mysql_family -- whether the connection's vendor is MariaDB or
    // MySQL
    bool
    is_mysql_family() const noexcept
    {
        const database_type type = m_connection->get_database_type();

        return ( (type == database_type::mariadb) ||
                 (type == database_type::mysql) );
    }

    // layer_scope -- the schema the layer acts in: the table's, or, for a
    // temporary table, SQLite's temp and PostgreSQL's pg_temp; NULL, the
    // connection's own, otherwise
    face::string_arg
    layer_scope() const noexcept
    {
        if (!m_schema.schema_name.empty())
        {
            return face::string_arg(m_schema.schema_name);
        }

        if (m_kind == table_kind::temporary)
        {
            switch (m_connection->get_database_type())
            {
                case database_type::sqlite:
                    return face::string_arg("temp");

                case database_type::postgresql:
                    return face::string_arg("pg_temp");

                default:
                    break;
            }
        }

        return face::string_arg();
    }

    // npos -- no row: a cached row standing for no row read
    static constexpr size_type npos = static_cast<size_type>(-1);

    Connection*  m_connection;
    table_schema m_schema;
    table_kind   m_kind;
    sync_config  m_sync;
    storage_type m_data;

    // m_original, m_original_keys -- the rows as the last refresh() or
    // commit() left them, with the key's columns then, which a keyed commit
    // compares the cache with
    storage_type           m_original;
    std::vector<size_type> m_original_keys;

    // state -- whether the cache holds changes not yet committed; whether it
    // may be behind the database; whether it holds the rows the query read;
    // whether those came from the table itself; and whether they were read a
    // page at a time
    bool m_dirty;
    bool m_stale;
    bool m_loaded;
    bool m_from_table;
    bool m_read_paged;

    std::chrono::steady_clock::time_point m_last_refresh;

    // query -- refresh()'s, and the filter the cache's rows were read with,
    // which a commit without a key replaces
    std::string              m_where_clause;
    std::string              m_order_clause;
    std::string              m_read_where;
    std::optional<size_type> m_limit;
    std::optional<size_type> m_offset;

    // m_select_hint -- SQL a vendor's table writes after SELECT in
    // refresh()'s query (mysql_table's optimizer hint)
    std::string m_select_hint;

    // m_table_suffix -- SQL a vendor's table writes after the table's name in
    // refresh()'s query (mariadb_table's FOR SYSTEM_TIME); rows read with one
    // are not the table's own, and commit() refuses them
    std::string m_table_suffix;

private:
    // m_reset_moved -- the table moved from: bound to nothing, holding
    // nothing
    void
    m_reset_moved() noexcept
    {
        m_connection = nullptr;
        m_dirty      = false;
        m_stale      = true;
        m_loaded     = false;
        m_from_table = true;
        m_read_paged = false;

        return;
    }

    // m_forget_read -- the cache no longer stands for rows the table was read
    // for: another table, connection or schema
    void
    m_forget_read() noexcept
    {
        m_original.clear();
        m_original_keys.clear();
        m_loaded     = false;
        m_from_table = true;
        m_read_paged = false;
        m_stale      = true;
        m_read_where.clear();

        return;
    }

    // m_append_column -- a column appended to the schema, a NULL to every
    // row, the cache and the rows read alike
    void
    m_append_column(
        const column_info& _column
    )
    {
        m_schema.columns.push_back(_column);

        for (row_type& record : m_data)
        {
            record.emplace_back();
        }

        for (row_type& record : m_original)
        {
            record.emplace_back();
        }

        return;
    }

    // m_erase_column -- a column removed from the schema, the key's list,
    // every row and the rows read; the key read with follows
    void
    m_erase_column(
        size_type _col
    )
    {
        const std::string name = m_schema.columns[_col].name;

        m_schema.columns.erase(m_schema.columns.begin() +
                               static_cast<difference_type>(_col));
        m_schema.primary_key_columns.erase(
            std::remove(m_schema.primary_key_columns.begin(),
                        m_schema.primary_key_columns.end(),
                        name),
            m_schema.primary_key_columns.end());

        // each row, the cache's and the read ones
        for (storage_type* rows : { &m_data, &m_original })
        {
            for (row_type& record : *rows)
            {
                if (_col < record.size())
                {
                    record.erase(record.begin() +
                              static_cast<difference_type>(_col));
                }
            }
        }

        // the rows read are now matched by what is left of the key
        if (!m_original_keys.empty())
        {
            m_original_keys = key_columns();
        }

        return;
    }

    // m_install_columns -- fetch_schema's columns and key, the cache emptied
    // where the columns' names changed
    void
    m_install_columns(
        std::vector<column_info>&& _columns,
        std::vector<std::string>&& _keys
    )
    {
        bool same = (_columns.size() == m_schema.columns.size());

        // the same names, in the same order
        for (size_type c = 0; (same) && (c < _columns.size()); ++c)
        {
            same = (_columns[c].name == m_schema.columns[c].name);
        }

        m_schema.columns             = std::move(_columns);
        m_schema.primary_key_columns = std::move(_keys);

        // rows of other columns stand for nothing now
        if (!same)
        {
            m_data.clear();
            m_forget_read();
            m_dirty = false;
        }

        return;
    }

    // m_describe -- what the layer adds to a query's description of the
    // columns; nothing for a table it cannot see
    void
    m_describe(
        std::vector<column_info>& _columns,
        std::vector<std::string>& _keys
    ) const
    {
        std::vector<std::pair<int, std::string>> places;

        m_connection->ask_layer(
            "fetch_schema",
            [this, &_columns, &places](const auto& _db) -> int
            {
                face::column_list described;
                const int         result =
                    _db.list_columns(layer_scope(),
                                     face::string_arg(m_schema.table_name),
                                     described);

                // a table the layer cannot see keeps the query's description
                if (result != 0)
                {
                    return (result == D_DB_NOT_FOUND) ? 0 : result;
                }

                for (std::size_t i = 0; i < described.size(); ++i)
                {
                    const d_db_column_info& info = described[i];

                    // the column of the same name
                    for (column_info& column : _columns)
                    {
                        if (column.name != info.name)
                        {
                            continue;
                        }

                        column.nullable     = !info.not_null;
                        column.is_generated = info.generated;

                        if (info.default_sql)
                        {
                            column.default_value =
                                std::string(info.default_sql);
                        }

                        if ( (info.type) &&
                             (info.type[0] != '\0') )
                        {
                            column.sql_type = std::string(info.type);
                        }

                        // a key column, its place kept for the key's order
                        if (info.primary_key > 0)
                        {
                            column.is_primary_key = true;
                            column.is_unique      = true;
                            column.is_indexed     = true;
                            places.emplace_back(info.primary_key, column.name);
                        }

                        break;
                    }
                }

                return 0;
            });

        std::sort(places.begin(), places.end());

        for (const std::pair<int, std::string>& place : places)
        {
            _keys.push_back(place.second);
        }

        return;
    }

    // m_read -- the rows refresh()'s query selects, and the columns a table
    // not yet described takes; nothing else changes
    void
    m_read(
        storage_type&             _rows,
        std::vector<column_info>& _adopted
    ) const
    {
        std::unique_ptr<buffered_result_set> found =
            m_connection->execute_query(build_select_query());
        const size_type width = found->column_count();

        // a table not yet described takes the result's columns
        if (m_schema.columns.empty())
        {
            for (size_type c = 0; c < width; ++c)
            {
                column_info column;

                column.name = found->column_name(c);
                column.type = found->column_type(c);

                _adopted.push_back(std::move(column));
            }
        }
        else if (width != cols())
        {
            throw query_exception("database_table::refresh: the query read " +
                                      std::to_string(width) + " columns, the "
                                      "schema describes " +
                                      std::to_string(cols()),
                                  D_DB_MISMATCH);
        }

        // each row, each cell as the database typed it
        while (found->next())
        {
            row_type record;

            record.reserve(width);

            for (size_type c = 0; c < width; ++c)
            {
                record.push_back(found->get_value(c));
            }

            _rows.push_back(std::move(record));
        }

        return;
    }

    // m_take -- rows m_read read, as the cache and as the rows read
    void
    m_take(
        storage_type&&             _rows,
        std::vector<column_info>&& _adopted
    )
    {
        // the columns of a table described by its rows
        if (!_adopted.empty())
        {
            m_schema.columns = std::move(_adopted);
        }

        // the key, which can refuse, before the rows change
        std::vector<size_type> keys = key_columns();

        m_data          = std::move(_rows);
        m_mark_read();
        m_original_keys = std::move(keys);
        m_loaded        = true;
        m_from_table    = m_table_suffix.empty();
        m_read_paged    = ( (m_limit.has_value()) ||
                            (m_offset.has_value()) );
        m_read_where    = m_where_clause;
        m_stale         = false;
        m_dirty         = false;
        m_last_refresh  = std::chrono::steady_clock::now();

        return;
    }

    /**
     * @brief Runs a commit's statements as one unit: in a transaction of its
     *        own, or under a savepoint inside the program's.
     *
     * @param[in] _work  the statements; called once.
     * @throws what `_work` throws, the unit rolled back first; and what the
     *         transaction or the savepoint throws.
     */
    template<typename Function>
    void
    m_in_unit(
        Function&& _work
    )
    {
        // inside the program's transaction, a savepoint undoes this alone
        if (m_connection->in_transaction())
        {
            if constexpr (is_detected<internal::table_savepoint_t,
                                      Connection>::value)
            {
                const std::string name = "djinterp_table_commit";

                m_connection->create_savepoint(name);

                try
                {
                    _work();
                }
                catch (...)
                {
                    // the statements undone, the transaction kept usable
                    try
                    {
                        m_connection->rollback_to_savepoint(name);
                        m_connection->release_savepoint(name);
                    }
                    catch (...)
                    {
                        // the first failure is the one to report
                    }

                    throw;
                }

                m_connection->release_savepoint(name);
            }
            else
            {
                _work();
            }

            return;
        }

        transaction<Connection> unit(*m_connection);

        _work();
        unit.commit();

        return;
    }

    // m_replace -- a commit without a key: the rows read deleted, those the
    // filter selected, then every cached row inserted; whether a value was
    // left to the database
    template<typename Inserter>
    bool
    m_replace(
        bool       _was_loaded,
        Inserter&& _insert
    )
    {
        // the rows the cache was read for, where it was read
        if (_was_loaded)
        {
            std::string sql = "DELETE FROM " + qualified_name();

            if (!m_read_where.empty())
            {
                sql += " WHERE " + m_read_where;
            }

            (void)m_connection->execute_update(sql);
        }

        std::vector<const row_type*> rows;

        rows.reserve(m_data.size());

        for (const row_type& record : m_data)
        {
            rows.push_back(&record);
        }

        return (rows.empty()) ? false
                              : _insert(rows);
    }

    // m_differ -- a commit by the key, as the plan settled it: the rows read
    // that the cache no longer holds deleted, those whose key changed
    // updated in place, the other changed rows updated, the new rows
    // inserted; whether a value was left to the database, a generated
    // column's that an update changed among them
    template<typename Inserter>
    bool
    m_differ(
        const commit_plan&            _plan,
        const std::vector<size_type>& _keys,
        Inserter&&                    _insert
    )
    {
        m_delete_rows(_plan.gone, _keys);
        m_rekey_rows(_plan, _keys);
        m_update_rows(_plan.changed, _keys);

        // a row updated changes what a generated column computes
        const bool recomputed = ( ( (!_plan.changed.empty()) ||
                                    (!_plan.rekeyed.empty()) ) &&
                                  (m_has_generated()) );
        const bool left       = (_plan.fresh.empty()) ? false
                                                      : _insert(_plan.fresh);

        return ( (left) ||
                 (recomputed) );
    }

    // m_delete_rows -- each row deleted by its key, as it was read
    void
    m_delete_rows(
        const std::vector<const row_type*>& _rows,
        const std::vector<size_type>&       _keys
    )
    {
        // nothing gone, nothing prepared
        if (_rows.empty())
        {
            return;
        }

        auto statement = m_connection->prepare("DELETE FROM " +
                                               qualified_name() +
                                               m_key_condition(_keys, 0));

        for (const row_type* record : _rows)
        {
            size_type number = 0;

            statement->clear_parameters();

            for (const size_type key : _keys)
            {
                bind_value(*statement, ++number, (*record)[key]);
            }

            (void)statement->execute_update();
        }

        return;
    }

    // m_update_rows -- each changed row's other columns set, but those the
    // database computes, the row found by its key as it was read
    void
    m_update_rows(
        const std::vector<std::pair<const row_type*, const row_type*>>& _rows,
        const std::vector<size_type>&                                   _keys
    )
    {
        const database_type    type = m_connection->get_database_type();
        std::vector<size_type> others;
        std::string            sets;

        // the columns outside the key the database does not compute, each
        // with its parameter
        for (size_type c = 0; c < cols(); ++c)
        {
            if ( (std::find(_keys.begin(), _keys.end(), c) == _keys.end()) &&
                 (!m_schema.columns[c].is_generated) )
            {
                sets += (others.empty()) ? "" : ", ";
                sets += quote_identifier(m_schema.columns[c].name, type) +
                        " = " +
                        internal::dialect_placeholder(type,
                                                      others.size() + 1u);
                others.push_back(c);
            }
        }

        // a row of key columns alone is changed only by its key
        if ( (_rows.empty()) ||
             (others.empty()) )
        {
            return;
        }

        auto statement = m_connection->prepare(
            "UPDATE " + qualified_name() + " SET " + sets +
            m_key_condition(_keys, others.size()));

        for (const std::pair<const row_type*, const row_type*>& entry : _rows)
        {
            size_type number = 0;

            statement->clear_parameters();

            for (const size_type c : others)
            {
                bind_value(*statement, ++number, (*entry.first)[c]);
            }

            for (const size_type key : _keys)
            {
                bind_value(*statement, ++number, (*entry.second)[key]);
            }

            (void)statement->execute_update();
        }

        return;
    }

    // m_key_condition -- " WHERE k1 = ... AND k2 = ...", the parameters
    // numbered after the `_before` that precede them
    std::string
    m_key_condition(
        const std::vector<size_type>& _keys,
        size_type                     _before
    ) const
    {
        const database_type type      = m_connection->get_database_type();
        std::string         condition = " WHERE ";
        size_type           number    = _before;

        for (const size_type key : _keys)
        {
            condition += (number > _before) ? " AND " : "";
            condition += quote_identifier(m_schema.columns[key].name, type) +
                         " = " +
                         internal::dialect_placeholder(type, ++number);
        }

        return condition;
    }

    // m_same_row -- whether two rows hold the same values in the columns a
    // commit writes, integers compared by number: a generated column's cell
    // is never written, so a change to it is no change
    bool
    m_same_row(
        const row_type& _row,
        const row_type& _other
    ) const
    {
        if (_row.size() != _other.size())
        {
            return false;
        }

        for (size_type c = 0; c < _row.size(); ++c)
        {
            if ( (!m_schema.columns[c].is_generated) &&
                 (!(m_widened(_row[c]) == m_widened(_other[c]))) )
            {
                return false;
            }
        }

        return true;
    }

    // m_mark_read -- the cache as the rows read: each row numbered afresh,
    // the number it travels with (table_row), and the rows read a copy of it
    void
    m_mark_read()
    {
        const re_std::uint64_t first =
            internal::next_row_origins(m_data.size());

        for (size_type r = 0; r < m_data.size(); ++r)
        {
            m_data[r].m_origin = first + static_cast<re_std::uint64_t>(r);
        }

        m_original = m_data;

        return;
    }

    // m_origins -- each row read, by the number it travels with
    std::unordered_map<re_std::uint64_t, size_type>
    m_origins() const
    {
        std::unordered_map<re_std::uint64_t, size_type> origins;

        origins.reserve(m_original.size());

        for (size_type r = 0; r < m_original.size(); ++r)
        {
            origins.emplace(m_original[r].m_origin, r);
        }

        return origins;
    }

    // m_read_index -- the row read a cached row stands for, claimed for it;
    // npos for a new row, or one standing for a row another claimed first
    static size_type
    m_read_index(
        const std::unordered_map<re_std::uint64_t, size_type>& _origins,
        const row_type&                                        _record,
        std::vector<bool>&                                     _claimed
    )
    {
        const auto found = _origins.find(_record.m_origin);

        // a row read, not yet claimed
        if ( (_record.m_origin == 0u)      ||
             (found == _origins.end())     ||
             (_claimed[found->second]) )
        {
            return npos;
        }

        _claimed[found->second] = true;

        return found->second;
    }

    // m_cache_differs -- whether the cache holds a row, a cell or a missing
    // row the rows read do not, each row matched with the row read it stands
    // for; every cell compared, a generated column's too, which commit()
    // then refuses
    bool
    m_cache_differs() const
    {
        // rows never read are all to be inserted
        if (!m_loaded)
        {
            return !m_data.empty();
        }

        if (m_data.size() != m_original.size())
        {
            return true;
        }

        const auto        origins = m_origins();
        std::vector<bool> claimed(m_original.size(), false);

        for (const row_type& record : m_data)
        {
            const size_type read = m_read_index(origins, record, claimed);

            if ( (read == npos) ||
                 (!m_same_cells(record, m_original[read])) )
            {
                return true;
            }
        }

        return false;
    }

    // m_refuse_generated_changes -- query_exception with D_DB_UNSUPPORTED
    // when a cached row changed a generated column's cell of the row read it
    // stands for: the database computes it, and nothing would write it
    void
    m_refuse_generated_changes() const
    {
        // only rows read have a generated cell to change, where there is one
        if ( (!m_loaded) ||
             (!m_has_generated()) )
        {
            return;
        }

        const auto        origins = m_origins();
        std::vector<bool> claimed(m_original.size(), false);

        for (const row_type& record : m_data)
        {
            const size_type read = m_read_index(origins, record, claimed);

            for (size_type c = 0; (read != npos) && (c < cols()); ++c)
            {
                if ( (m_schema.columns[c].is_generated) &&
                     (!(m_widened(record[c]) ==
                        m_widened(m_original[read][c]))) )
                {
                    throw query_exception("database_table::commit: the cache "
                                              "changed generated column '" +
                                              m_schema.columns[c].name +
                                              "'; the database computes it",
                                          D_DB_UNSUPPORTED);
                }
            }
        }

        return;
    }

    // m_plan -- a keyed commit's plan, every refusal thrown before anything
    // is written: two cached rows of one key, D_DB_INVALID_ARGUMENT; a row
    // read with a NULL in its key that the cache changed or removed, which
    // no key can find, and rows exchanging keys where the key cannot be
    // deferred, D_DB_UNSUPPORTED
    commit_plan
    m_plan(
        const std::vector<size_type>& _keys,
        bool                          _was_loaded
    ) const
    {
        const auto                                origins = m_origins();
        std::vector<bool>                         claimed(m_original.size(),
                                                          false);
        std::set<std::vector<value_type>>         held;
        std::vector<typename commit_plan::pair_type>       rekeyed;
        commit_plan                               plan;

        for (const row_type& record : m_data)
        {
            std::vector<value_type> key;
            const bool              keyed = key_of(record, _keys, key);

            // two rows of one key cannot both be written
            if ( (keyed) &&
                 (!held.insert(key).second) )
            {
                throw query_exception("database_table::commit: two rows hold "
                                      "the same primary key",
                                      D_DB_INVALID_ARGUMENT);
            }

            const size_type read = (_was_loaded)
                                       ? m_read_index(origins, record, claimed)
                                       : npos;

            // a row the cache adds, or one standing for a row read
            if (read == npos)
            {
                plan.fresh.push_back(&record);
            }
            else
            {
                m_plan_row(plan,
                           rekeyed,
                           record,
                           m_original[read],
                           _keys,
                           key,
                           keyed);
            }
        }

        // the rows read that no cached row stands for
        for (size_type r = 0; (_was_loaded) && (r < m_original.size()); ++r)
        {
            if (!claimed[r])
            {
                m_require_found(m_original[r], _keys);
                plan.gone.push_back(&m_original[r]);
            }
        }

        plan.rekeyed = m_order_rekeyed(std::move(rekeyed), _keys, plan);

        return plan;
    }

    // m_plan_row -- a cached row standing for a row read: unchanged, changed
    // under its key, given a new key, or given a NULL in its key, which
    // deletes the row read and inserts the cached one, for the database to
    // fill the key
    void
    m_plan_row(
        commit_plan&                         _plan,
        std::vector<typename commit_plan::pair_type>& _rekeyed,
        const row_type&                      _record,
        const row_type&                      _read,
        const std::vector<size_type>&        _keys,
        const std::vector<value_type>&       _key,
        bool                                 _keyed
    ) const
    {
        std::vector<value_type> old;

        // a row read with a NULL in its key, which no key can find
        if (!key_of(_read, _keys, old))
        {
            if (!m_same_row(_record, _read))
            {
                m_require_found(_read, _keys);
            }

            return;
        }

        if (!_keyed)
        {
            _plan.gone.push_back(&_read);
            _plan.fresh.push_back(&_record);
        }
        else if (_key == old)
        {
            if (!m_same_row(_record, _read))
            {
                _plan.changed.emplace_back(&_record, &_read);
            }
        }
        else
        {
            _rekeyed.emplace_back(&_record, &_read);
        }

        return;
    }

    // m_require_found -- query_exception with D_DB_UNSUPPORTED for a row
    // read with a NULL in its key (SQLite allows one), which no key finds
    void
    m_require_found(
        const row_type&               _read,
        const std::vector<size_type>& _keys
    ) const
    {
        std::vector<value_type> key;

        if (!key_of(_read, _keys, key))
        {
            throw query_exception("database_table::commit: a row read with a "
                                  "NULL in its primary key cannot be found "
                                  "by it, to update or delete",
                                  D_DB_UNSUPPORTED);
        }

        return;
    }

    /**
     * @brief Orders the rows whose key changes so that each new key is free
     *        when its row takes it: a row waits for the row read under its new
     *        key to leave it.
     *
     * @param[in]     _rekeyed  the rows, each with the row read it stands for.
     * @param[in]     _keys     the key's columns.
     * @param[in,out] _plan     receives the key constraint to defer, where
     *                          rows exchange keys.
     * @return the rows, in the order to update them.
     * @throws query_exception with D_DB_UNSUPPORTED where rows exchange keys
     *         and the database cannot defer the key.
     */
    std::vector<typename commit_plan::pair_type>
    m_order_rekeyed(
        std::vector<typename commit_plan::pair_type>&& _rekeyed,
        const std::vector<size_type>&                  _keys,
        commit_plan&                                   _plan
    ) const
    {
        const size_type                                count = _rekeyed.size();
        std::map<std::vector<value_type>, size_type>   by_old;
        std::vector<size_type>                         waits(count, npos);
        std::vector<int>                               state(count, 0);
        std::vector<typename commit_plan::pair_type>   ordered;
        bool                                           cycle = false;

        // each row by the key it leaves, then the row it waits for
        for (size_type i = 0; i < count; ++i)
        {
            std::vector<value_type> old;

            (void)key_of(*_rekeyed[i].second, _keys, old);
            by_old.emplace(std::move(old), i);
        }

        for (size_type i = 0; i < count; ++i)
        {
            std::vector<value_type> taken;

            (void)key_of(*_rekeyed[i].first, _keys, taken);

            const auto found = by_old.find(taken);

            waits[i] = (found == by_old.end()) ? npos : found->second;
        }

        // each chain from its end; a chain that meets itself is a cycle
        for (size_type start = 0; start < count; ++start)
        {
            std::vector<size_type> path;
            size_type              at = start;

            while ( (at != npos) &&
                    (state[at] == 0) )
            {
                state[at] = 1;
                path.push_back(at);
                at = waits[at];
            }

            cycle = ( (cycle) ||
                      ( (at != npos) &&
                        (state[at] == 1) ) );

            for (auto it = path.rbegin(); it != path.rend(); ++it)
            {
                state[*it] = 2;
                ordered.push_back(_rekeyed[*it]);
            }
        }

        // rows exchanging keys, under the key deferred, or refused
        if (cycle)
        {
            _plan.deferred = m_deferrable_key(_plan.initially_deferred);

            if (_plan.deferred.empty())
            {
                throw query_exception("database_table::commit: rows exchange "
                                      "their primary keys, which the "
                                      "database checks row by row; remove "
                                      "the rows and insert them again, or, "
                                      "on PostgreSQL, make the key "
                                      "DEFERRABLE",
                                      D_DB_UNSUPPORTED);
            }
        }

        return ordered;
    }

    // m_deferrable_key -- PostgreSQL's DEFERRABLE primary key constraint on
    // the table, schema-qualified and quoted, and whether it is initially
    // deferred; "" for none, and on every other vendor, which defers none
    std::string
    m_deferrable_key(
        bool& _initially_deferred
    ) const
    {
        _initially_deferred = false;

        // only PostgreSQL defers a primary key
        if (m_connection->get_database_type() != database_type::postgresql)
        {
            return std::string();
        }

        auto statement = m_connection->prepare(
            "SELECT n.nspname, c.conname, c.condeferred "
            "FROM pg_catalog.pg_constraint c "
            "JOIN pg_catalog.pg_namespace n ON n.oid = c.connamespace "
            "WHERE c.conrelid = $1::regclass AND c.contype = 'p' "
            "AND c.condeferrable");

        statement->bind_string(1u, qualified_name());

        std::unique_ptr<buffered_result_set> found =
            statement->execute_query();

        // no such constraint, or not deferrable
        if (!found->next())
        {
            return std::string();
        }

        _initially_deferred =
            found->get_bool(static_cast<std::size_t>(2)).value_or(false);

        return quote_identifier(
                   found->get_string(static_cast<std::size_t>(0)).value_or(""),
                   database_type::postgresql) +
               "." +
               quote_identifier(
                   found->get_string(static_cast<std::size_t>(1)).value_or(""),
                   database_type::postgresql);
    }

    // m_rekey_rows -- each row whose key changed updated in place, its key
    // and every other column the database does not compute, in the plan's
    // order: found by the key it was read with, or, under a deferred key,
    // where keys repeat until the last update, by its place in the table,
    // taken before any row moves (m_locate_rows)
    void
    m_rekey_rows(
        const commit_plan&            _plan,
        const std::vector<size_type>& _keys
    )
    {
        // nothing to move, nothing prepared
        if (_plan.rekeyed.empty())
        {
            return;
        }

        const database_type    type = m_connection->get_database_type();
        std::vector<size_type> written(_keys);
        std::string            sets;

        // the key's columns, then those outside it the database does not
        // compute, each with its parameter
        for (size_type c = 0; c < cols(); ++c)
        {
            if ( (std::find(_keys.begin(), _keys.end(), c) == _keys.end()) &&
                 (!m_schema.columns[c].is_generated) )
            {
                written.push_back(c);
            }
        }

        for (size_type w = 0; w < written.size(); ++w)
        {
            sets += (w > 0) ? ", " : "";
            sets += quote_identifier(m_schema.columns[written[w]].name, type) +
                    " = " + internal::dialect_placeholder(type, w + 1u);
        }

        const bool deferred = !_plan.deferred.empty();
        const auto places   = (deferred) ? m_locate_rows(_plan, _keys)
                                         : std::vector<std::string>();
        auto       statement = m_connection->prepare(
            "UPDATE " + qualified_name() + " SET " + sets +
            ( (deferred)
                  ? " WHERE tableoid = " +
                        internal::dialect_placeholder(type,
                                                      written.size() + 1u) +
                        "::oid AND ctid = " +
                        internal::dialect_placeholder(type,
                                                      written.size() + 2u) +
                        "::tid"
                  : m_key_condition(_keys, written.size()) ));

        m_defer_key(_plan, true);

        for (size_type e = 0; e < _plan.rekeyed.size(); ++e)
        {
            const typename commit_plan::pair_type& entry  = _plan.rekeyed[e];
            size_type                              number = 0;

            statement->clear_parameters();

            for (const size_type c : written)
            {
                bind_value(*statement, ++number, (*entry.first)[c]);
            }

            if (deferred)
            {
                statement->bind_string(++number, places[2u * e]);
                statement->bind_string(++number, places[2u * e + 1u]);
            }
            else
            {
                for (const size_type key : _keys)
                {
                    bind_value(*statement, ++number, (*entry.second)[key]);
                }
            }

            (void)statement->execute_update();
        }

        m_defer_key(_plan, false);

        return;
    }

    // m_locate_rows -- PostgreSQL's place of each row the plan moves, found
    // by the key it was read with and locked until the unit ends, so that it
    // stays where it was found: its tableoid and ctid, as text, two strings
    // a row; query_exception with D_DB_NOT_FOUND for a row no longer there
    std::vector<std::string>
    m_locate_rows(
        const commit_plan&            _plan,
        const std::vector<size_type>& _keys
    )
    {
        std::vector<std::string> places;
        auto                     locate = m_connection->prepare(
            "SELECT tableoid::text, ctid::text FROM " + qualified_name() +
            m_key_condition(_keys, 0) + " FOR UPDATE");

        places.reserve(2u * _plan.rekeyed.size());

        for (const typename commit_plan::pair_type& entry : _plan.rekeyed)
        {
            size_type number = 0;

            locate->clear_parameters();

            for (const size_type key : _keys)
            {
                bind_value(*locate, ++number, (*entry.second)[key]);
            }

            std::unique_ptr<buffered_result_set> found =
                locate->execute_query();

            // a row another session deleted since it was read
            if (!found->next())
            {
                throw query_exception("database_table::commit: a row whose "
                                      "key changed is no longer in the table",
                                      D_DB_NOT_FOUND);
            }

            for (std::size_t c = 0; c < 2u; ++c)
            {
                places.push_back(found->get_string(c).value_or(""));
            }
        }

        return places;
    }

    // m_defer_key -- the plan's deferred key constraint, deferred before the
    // updates, and after them checked at once, then returned to deferred
    // where it starts so; nothing where the plan defers none
    void
    m_defer_key(
        const commit_plan& _plan,
        bool               _before
    )
    {
        if (_plan.deferred.empty())
        {
            return;
        }

        if (_before)
        {
            (void)m_connection->execute_update("SET CONSTRAINTS " +
                                               _plan.deferred + " DEFERRED");

            return;
        }

        (void)m_connection->execute_update("SET CONSTRAINTS " +
                                           _plan.deferred + " IMMEDIATE");

        if (_plan.initially_deferred)
        {
            (void)m_connection->execute_update("SET CONSTRAINTS " +
                                               _plan.deferred + " DEFERRED");
        }

        return;
    }

    // m_same_cells -- whether two rows hold the same values in every cell,
    // integers compared by number
    bool
    m_same_cells(
        const row_type& _row,
        const row_type& _other
    ) const
    {
        if (_row.size() != _other.size())
        {
            return false;
        }

        for (size_type c = 0; c < _row.size(); ++c)
        {
            if (!(m_widened(_row[c]) == m_widened(_other[c])))
            {
                return false;
            }
        }

        return true;
    }

    // m_has_generated -- whether a column of the table is generated
    bool
    m_has_generated() const noexcept
    {
        for (const column_info& column : m_schema.columns)
        {
            if (column.is_generated)
            {
                return true;
            }
        }

        return false;
    }

    // m_widened -- a `value`'s 32-bit integer as a 64-bit one; any other
    // value, and any other value type's, as it is
    static value_type
    m_widened(
        const value_type& _cell
    )
    {
        if constexpr (std::is_same_v<value_type, value>)
        {
            if (const re_std::int32_t* number =
                    std::get_if<re_std::int32_t>(&_cell))
            {
                return value(static_cast<re_std::int64_t>(*number));
            }
        }

        return _cell;
    }

    // m_is_null -- whether a cell is NULL: a `value`'s monostate; a cell of
    // another type is never NULL
    static bool
    m_is_null(
        const value_type& _cell
    ) noexcept
    {
        if constexpr (std::is_same_v<value_type, value>)
        {
            return std::holds_alternative<std::monostate>(_cell);
        }
        else
        {
            static_cast<void>(_cell);

            return false;
        }
    }

    // m_probe_mysql_temporary -- whether a MariaDB or MySQL temporary table
    // is there, asked of the server itself: 1146 is no such table
    bool
    m_probe_mysql_temporary() const
    {
        try
        {
            (void)m_connection->execute_query("SELECT 1 FROM " +
                                              qualified_name() +
                                              " WHERE 1=0");
        }
        catch (const query_exception& _e)
        {
            if (_e.error_code() == 1146)
            {
                return false;
            }

            throw;
        }

        return true;
    }
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_TABLE_HPP
