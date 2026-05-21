/******************************************************************************
* djinterp [database]                                       database_table.hpp
*
* djinterp database-table module:
*   The foundational module for all table-based database back-ends —
* MySQL, MariaDB, PostgreSQL, SQLite, and Oracle. Vendors with a non-
* table-based model (Redis key-value, ArangoDB / MongoDB document) are
* explicitly OUT of scope here and use their own primitives.
*
*   DESIGN
*   ======
*   `database_table<_Connection, _ValueType, _Config>` is a CONCRETE
* class. It is NOT a CRTP base, NOT designed for virtual inheritance,
* and exposes NO `virtual` methods. Vendor variation flows through the
* `_Connection` template parameter — a vendor's concrete connection
* type drives every interaction with the back-end, and that connection
* type already encapsulates dialect-specific behaviour via the
* `djinterp::connection<_helper>` CRTP surface in `database.hpp`.
* Different vendors therefore give rise to different concrete
* instantiations of the same template — there is no inheritance
* hierarchy to maintain.
*
*   The class internally holds the diverse data types that arrive
* from a back-end (defaulting to the `value` variant from
* `database.hpp`, which covers null / bool / int / long /
* double / string / binary / timestamp) and exposes a row-oriented
* container surface (`rows()`, `cols()`, `cell()`, `insert_row()`,
* row iteration, …) suitable for plugging into the broader djinterp
* trait machinery.
*
*   What lives WHERE
*   ----------------
*     This file:                  the basics — enums, schema descriptors,
*                                 sync configuration, the class itself,
*                                 SQL-dialect helpers.
*     database_table_traits.hpp:  the type traits (is_database_table,
*                                 has_database_sync, etc.) and the
*                                 `database_table_class` classification.
*                                 (NOT in this header — separate module.)
*     table_common.hpp:           the foundation shared with `table.hpp`
*                                 (axis tags, option keys, alias surface).
*     database.hpp:        the vendor-agnostic connection /
*                                 statement / result-set CRTP surface.
*
*   COMPILE-TIME EMPHASIS
*   =====================
*   Templates everywhere; `virtual` nowhere. The vendor-specific bits
* — dialect quirks for `LIMIT` / `OFFSET`, identifier quoting, schema
* introspection — are resolved either via the connection's interface
* (the connection knows its dialect) or via small free-function
* dispatch on a runtime `database_type` enum (a no-cost switch for
* an enum-of-a-handful-of-values).
*
*   PORTABILITY
*   ===========
*     version: C++17 or later (std::optional, std::variant,
*              std::string_view).
*     dependencies:
*       - djinterp.hpp           : NS_DJINTERP, D_CONSTEXPR, D_INLINE
*       - core/table_common.hpp  : axis tags, option keys (consumed
*                                  optionally for classification)
*       - core/db/database.hpp
*                                : `value`, `database_type`, connection /
*                                  statement / result_set surfaces,
*                                  `quote_identifier`, exception types
*
*
* path:      /inc/djinterp/core/db/database_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.18
******************************************************************************/

#ifndef DJINTERP_DATABASE_TABLE_
#define DJINTERP_DATABASE_TABLE_ 1

#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    #error "`database_table.hpp` requires C++17 or later                       \
            (std::optional, std::variant, std::string_view)."
#endif

// std
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./table_common.hpp"
#include "./db/database.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   ENUMERATIONS
    // =========================================================================

    // table_kind
    //   enum: classification of a database table's mutability and
    // lifetime semantics.
    enum class table_kind
    {
        base_table,
        view,
        materialized_view,
        temporary
    };

    // sync_policy
    //   enum: strategy controlling when the local cache refreshes
    // from the backing database.
    enum class sync_policy
    {
        manual,
        on_access,
        on_modify,
        periodic
    };

    // modify_action
    //   enum: categorization of modification operations for
    // `sync_policy::on_modify` filtering.
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


    // =========================================================================
    // II.  COLUMN SCHEMA DESCRIPTOR
    // =========================================================================

    // column_info
    //   struct: metadata for a single column in a database table.
    // Populated from schema introspection or explicit specification.
    struct column_info
    {
        std::string                name;
        field_type                 type;
        bool                       nullable;
        bool                       is_primary_key;
        bool                       is_auto_increment;
        bool                       is_unique;
        bool                       is_indexed;
        std::optional<std::string> default_value;
        std::optional<std::size_t> max_length;
        std::optional<std::string> foreign_table;
        std::optional<std::string> foreign_column;
        std::optional<std::string> comment;

        column_info()
            : type(field_type::null),
              nullable(true),
              is_primary_key(false),
              is_auto_increment(false),
              is_unique(false),
              is_indexed(false)
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
              is_indexed(_primary_key)
        {}
    };

    // table_schema
    //   struct: aggregate schema descriptor for a database table.
    // Holds column definitions and table-level metadata.
    struct table_schema
    {
        std::string              table_name;
        std::string              schema_name;
        std::vector<column_info> columns;
        std::vector<std::string> primary_key_columns;

        // column_count
        //   function: number of columns described by the schema.
        std::size_t column_count() const noexcept
        {
            return columns.size();
        }

        // column_index
        //   function: returns the index of the column with the given
        // name, or `std::nullopt` when no such column exists.
        std::optional<std::size_t>
        column_index(std::string_view _name) const
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

        // column_by_name
        //   function: returns a pointer to the column_info matching
        // `_name`, or nullptr when no such column exists.
        const column_info*
        column_by_name(std::string_view _name) const
        {
            auto idx = column_index(_name);

            if (!idx.has_value())
            {
                return nullptr;
            }

            return &columns[idx.value()];
        }
    };


    // =========================================================================
    // III. SYNC CONFIGURATION
    // =========================================================================

    // sync_config
    //   struct: configuration for the synchronization policy. Controls
    // refresh timing and modification-tracking filtering.
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

        // tracks_action
        //   function: returns whether `_action` is one of the actions
        // this sync config has opted to track. An empty `tracked_actions`
        // list is treated as "track everything".
        bool tracks_action(modify_action _action) const
        {
            // empty tracked_actions means track all
            if (tracked_actions.empty())
            {
                return true;
            }

            return (std::find(tracked_actions.begin(),
                              tracked_actions.end(),
                              _action)
                    != tracked_actions.end());
        }
    };


    // =========================================================================
    // IV.  SQL DIALECT HELPERS
    // =========================================================================
    //   Free-function dispatch on `database_type` for the small set of
    // SQL-syntax quirks that vary across the supported back-ends. Each
    // helper is a thin switch — no inheritance, no virtual calls — and
    // is invoked from `database_table` at runtime (the table holds a
    // connection whose `database_type` is a small enum).
    //
    //   Identifier quoting is delegated to
    // `djinterp::quote_identifier(name, db_type)` from
    // `database.hpp` — that function already covers the
    // backtick / double-quote / square-bracket variants per vendor.

    NS_INTERNAL

        // dialect_format_limit_offset
        //   helper: emits the dialect-appropriate LIMIT/OFFSET clause.
        //   - MySQL, MariaDB, SQLite, PostgreSQL: `LIMIT n OFFSET m`.
        //   - Oracle, MSSQL:                      `OFFSET m ROWS FETCH
        //                                          NEXT n ROWS ONLY`.
        //   When neither limit nor offset is set, returns an empty
        // string.
        D_INLINE std::string
        dialect_format_limit_offset(
            database_type                    _db_type,
            const std::optional<std::size_t>& _limit,
            const std::optional<std::size_t>& _offset
        )
        {
            std::string result;

            // no pagination clause needed
            if ( (!_limit.has_value()) &&
                 (!_offset.has_value()) )
            {
                return result;
            }

            // OFFSET-FETCH dialects (Oracle 12c+ / SQL Server 2012+)
            if ( (_db_type == database_type::oracle) ||
                 (_db_type == database_type::mssql)  ||
                 (_db_type == database_type::db2) )
            {
                // OFFSET is required for FETCH NEXT in these dialects
                result += " OFFSET ";
                result += std::to_string(_offset.value_or(0));
                result += " ROWS";

                if (_limit.has_value())
                {
                    result += " FETCH NEXT ";
                    result += std::to_string(_limit.value());
                    result += " ROWS ONLY";
                }

                return result;
            }

            // LIMIT-OFFSET dialects (MySQL / MariaDB / Postgres / SQLite)
            if (_limit.has_value())
            {
                result += " LIMIT ";
                result += std::to_string(_limit.value());
            }

            if (_offset.has_value())
            {
                result += " OFFSET ";
                result += std::to_string(_offset.value());
            }

            return result;
        }

    NS_END  // internal


    // =========================================================================
    // V.   DATABASE TABLE
    // =========================================================================
    //
    // `database_table<_Connection, _ValueType, _Config>`
    //   The concrete class. One template instantiation per vendor,
    // driven by the `_Connection` template parameter — a MySQL
    // connection gives `database_table<mysql_connection, ...>`,
    // a PostgreSQL connection gives `database_table<pg_connection,
    // ...>`, and so on. No inheritance, no virtual functions, no
    // CRTP — every variation in behaviour is supplied through the
    // connection type.
    //
    //   Template parameters:
    //     _Connection  - the concrete connection type. Must satisfy the
    //                    `djinterp::connection<_helper>` CRTP surface
    //                    from `database.hpp` (execute_query,
    //                    get_database_type, is_connected, …).
    //     _ValueType   - the cell value type. Defaults to `value`, the
    //                    `std::variant` covering null / bool / ints /
    //                    double / string / binary / timestamp.
    //     _Config      - an opaque pass-through used by downstream
    //                    layout / decoration layers. Not interpreted
    //                    here; surfaced as the `config_type` alias for
    //                    consumers that want to attach metadata.

    // database_table
    //   class: concrete, non-polymorphic database-backed table. Owns
    // a local cache of rows plus a non-owning connection handle; all
    // back-end interaction flows through the connection's interface.
    template<typename _Connection,
             typename _ValueType = value,
             typename _Config    = void>
    class database_table
    {
    public:
        // -----------------------------------------------------------------
        //  standard container type aliases
        // -----------------------------------------------------------------
        using value_type      = _ValueType;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = value_type*;
        using const_pointer   = const value_type*;
        using allocator_type  = std::allocator<value_type>;

        // -----------------------------------------------------------------
        //  identity / row / storage aliases
        // -----------------------------------------------------------------
        using self_type       = database_table<_Connection, _ValueType, _Config>;
        using config_type     = _Config;
        using connection_type = _Connection;
        using schema_type     = table_schema;
        using row_type        = std::vector<value_type>;
        using storage_type    = std::vector<row_type>;

        // -----------------------------------------------------------------
        //  row-level iterator types. Iteration is over ROWS — each step
        //  yields a `row_type` (a `std::vector<value_type>`). Cell
        //  access is via `cell(r, c)` / `at(r, c)`.
        // -----------------------------------------------------------------
        using iterator               = typename storage_type::iterator;
        using const_iterator         = typename storage_type::const_iterator;
        using reverse_iterator       = typename storage_type::reverse_iterator;
        using const_reverse_iterator = typename storage_type::const_reverse_iterator;

        // -----------------------------------------------------------------
        //  compile-time classification flags
        // -----------------------------------------------------------------

        // has_dynamic_rows / has_dynamic_cols
        //   value: database tables always have runtime-determined
        // dimensions. These flags let the trait machinery distinguish
        // dynamic tables from fixed-extent tables at compile time.
        static constexpr bool has_dynamic_rows = true;
        static constexpr bool has_dynamic_cols = true;


        // =================================================================
        //  CONSTRUCTORS
        // =================================================================

        // database_table()
        //   constructor: default — empty, disconnected table.
        database_table()
            : m_connection(nullptr),
              m_kind(table_kind::base_table),
              m_dirty(false),
              m_stale(true),
              m_num_rows(0),
              m_num_cols(0)
        {}

        // database_table(connection, name)
        //   constructor: bound to a connection and identified by name.
        // Does not automatically fetch schema or data — call
        // `fetch_schema()` and `refresh()` after construction.
        explicit database_table(
            _Connection& _conn,
            std::string  _table_name,
            table_kind   _kind = table_kind::base_table
        )
            : m_connection(&_conn),
              m_kind(_kind),
              m_dirty(false),
              m_stale(true),
              m_num_rows(0),
              m_num_cols(0)
        {
            m_schema.table_name = std::move(_table_name);
        }

        // database_table(connection, schema)
        //   constructor: bound to a connection with an explicit schema.
        // Useful when schema is already known or was retrieved externally.
        explicit database_table(
            _Connection& _conn,
            table_schema _schema,
            table_kind   _kind = table_kind::base_table
        )
            : m_connection(&_conn),
              m_schema(std::move(_schema)),
              m_kind(_kind),
              m_dirty(false),
              m_stale(true),
              m_num_rows(0),
              m_num_cols(0)
        {
            m_num_cols = m_schema.columns.size();
        }

        // database_table(connection, schema, kind, sync)
        //   constructor: bound to a connection with an explicit schema
        // and sync policy.
        explicit database_table(
            _Connection&       _conn,
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
              m_num_rows(0),
              m_num_cols(0)
        {
            m_num_cols = m_schema.columns.size();
        }

        // disable copying — connection handle is non-owning, copy
        // semantics for "shares a connection" vs "duplicates the
        // cache" are ambiguous.
        database_table(const database_table&)            = delete;
        database_table& operator=(const database_table&) = delete;

        // database_table(database_table&&)
        //   constructor: move — leaves `_other` in a disconnected,
        // empty-but-stale state.
        database_table(database_table&& _other) noexcept
            : m_connection(_other.m_connection),
              m_schema(std::move(_other.m_schema)),
              m_kind(_other.m_kind),
              m_sync(std::move(_other.m_sync)),
              m_data(std::move(_other.m_data)),
              m_dirty(_other.m_dirty),
              m_stale(_other.m_stale),
              m_num_rows(_other.m_num_rows),
              m_num_cols(_other.m_num_cols),
              m_last_refresh(_other.m_last_refresh),
              m_where_clause(std::move(_other.m_where_clause)),
              m_order_clause(std::move(_other.m_order_clause)),
              m_limit(_other.m_limit),
              m_offset(_other.m_offset)
        {
            _other.m_connection = nullptr;
            _other.m_num_rows   = 0;
            _other.m_num_cols   = 0;
            _other.m_dirty      = false;
            _other.m_stale      = true;
        }

        // operator=(database_table&&)
        //   assignment: move — same semantics as the move constructor.
        database_table&
        operator=(database_table&& _other) noexcept
        {
            if (this != &_other)
            {
                m_connection   = _other.m_connection;
                m_schema       = std::move(_other.m_schema);
                m_kind         = _other.m_kind;
                m_sync         = std::move(_other.m_sync);
                m_data         = std::move(_other.m_data);
                m_dirty        = _other.m_dirty;
                m_stale        = _other.m_stale;
                m_num_rows     = _other.m_num_rows;
                m_num_cols     = _other.m_num_cols;
                m_last_refresh = _other.m_last_refresh;
                m_where_clause = std::move(_other.m_where_clause);
                m_order_clause = std::move(_other.m_order_clause);
                m_limit        = _other.m_limit;
                m_offset       = _other.m_offset;

                _other.m_connection = nullptr;
                _other.m_num_rows   = 0;
                _other.m_num_cols   = 0;
                _other.m_dirty      = false;
                _other.m_stale      = true;
            }

            return *this;
        }

        ~database_table() = default;


        // =================================================================
        //  CAPACITY
        // =================================================================

        // size
        //   function: total cell count in the local cache (rows * cols).
        size_type size() const noexcept
        {
            return (m_num_rows * m_num_cols);
        }

        // empty
        //   function: true when the local cache holds zero rows.
        bool empty() const noexcept
        {
            return (m_num_rows == 0);
        }

        // rows
        //   function: number of rows currently in the local cache.
        size_type rows() const noexcept
        {
            return m_num_rows;
        }

        // cols
        //   function: number of columns described by the schema.
        size_type cols() const noexcept
        {
            return m_num_cols;
        }

        // total_cells
        //   function: total cell count (rows * cols). Convenience
        // alias for `size()` for callers preferring the explicit name.
        size_type total_cells() const noexcept
        {
            return (m_num_rows * m_num_cols);
        }


        // =================================================================
        //  ELEMENT ACCESS
        // =================================================================

        // cell
        //   function: unchecked two-dimensional element access. Triggers
        // a lazy refresh when sync policy is `on_access` and the cache
        // is stale.
        reference cell(size_type _row,
                       size_type _col)
        {
            ensure_fresh();

            return m_data[_row][_col];
        }

        // cell (const)
        //   function: unchecked two-dimensional const element access.
        const_reference cell(size_type _row,
                             size_type _col) const
        {
            return m_data[_row][_col];
        }

        // at
        //   function: bounds-checked two-dimensional element access.
        reference at(size_type _row,
                     size_type _col)
        {
            ensure_fresh();
            check_bounds(_row, _col, "at");

            return m_data[_row][_col];
        }

        // at (const)
        //   function: bounds-checked two-dimensional const element access.
        const_reference at(size_type _row,
                           size_type _col) const
        {
            check_bounds(_row, _col, "at");

            return m_data[_row][_col];
        }

        // operator[]
        //   function: row-indexed access returning a reference to the
        // underlying row vector. No bounds check.
        row_type& operator[](size_type _row)
        {
            ensure_fresh();

            return m_data[_row];
        }

        // operator[] (const)
        //   function: row-indexed const access. No bounds check.
        const row_type& operator[](size_type _row) const
        {
            return m_data[_row];
        }

        // cell_by_name
        //   function: bounds-checked element access by column name.
        reference cell_by_name(size_type        _row,
                               std::string_view _column_name)
        {
            ensure_fresh();

            size_type col_idx = resolve_column(_column_name, "cell_by_name");
            check_row(_row, "cell_by_name");

            return m_data[_row][col_idx];
        }

        // cell_by_name (const)
        //   function: bounds-checked const element access by column name.
        const_reference cell_by_name(size_type        _row,
                                     std::string_view _column_name) const
        {
            size_type col_idx = resolve_column(_column_name, "cell_by_name");
            check_row(_row, "cell_by_name");

            return m_data[_row][col_idx];
        }

        // get_row
        //   function: returns a copy of a single row.
        row_type get_row(size_type _row) const
        {
            check_row(_row, "get_row");

            return m_data[_row];
        }

        // get_column
        //   function: returns all values in a column by index.
        std::vector<value_type>
        get_column(size_type _col) const
        {
            check_col(_col, "get_column");

            std::vector<value_type> result;
            result.reserve(m_num_rows);

            for (size_type i = 0; i < m_num_rows; ++i)
            {
                result.push_back(m_data[i][_col]);
            }

            return result;
        }

        // get_column (by name)
        //   function: returns all values in a column by name.
        std::vector<value_type>
        get_column(std::string_view _column_name) const
        {
            size_type col_idx = resolve_column(_column_name, "get_column");

            return get_column(col_idx);
        }


        // =================================================================
        //  ITERATION (row-level)
        // =================================================================
        //   `begin()` / `end()` yield row iterators (each `*it` is a
        // `row_type` vector of value_types). Cell-level iteration is
        // not directly supported — drop into the row to scan cells.

        iterator       begin()        noexcept { return m_data.begin();   }
        const_iterator begin()  const noexcept { return m_data.begin();   }
        const_iterator cbegin() const noexcept { return m_data.cbegin();  }
        iterator       end()          noexcept { return m_data.end();     }
        const_iterator end()    const noexcept { return m_data.end();     }
        const_iterator cend()   const noexcept { return m_data.cend();    }

        reverse_iterator       rbegin()        noexcept { return m_data.rbegin();  }
        const_reverse_iterator rbegin()  const noexcept { return m_data.rbegin();  }
        const_reverse_iterator crbegin() const noexcept { return m_data.crbegin(); }
        reverse_iterator       rend()          noexcept { return m_data.rend();    }
        const_reverse_iterator rend()    const noexcept { return m_data.rend();    }
        const_reverse_iterator crend()   const noexcept { return m_data.crend();   }


        // =================================================================
        //  ROW MUTATION
        // =================================================================
        //   Mutation is gated by `is_mutable()` — runtime kind check.
        // View tables (kind == view) throw on any mutation attempt.

        // insert_row
        //   function: appends a row to the local cache.
        void insert_row(const row_type& _row)
        {
            validate_mutable("insert_row");
            check_row_width(_row, "insert_row");

            m_data.push_back(_row);
            ++m_num_rows;
            mark_dirty(modify_action::insert_row);

            return;
        }

        // insert_row (move)
        //   function: move-appends a row to the local cache.
        void insert_row(row_type&& _row)
        {
            validate_mutable("insert_row");
            check_row_width(_row, "insert_row");

            m_data.push_back(std::move(_row));
            ++m_num_rows;
            mark_dirty(modify_action::insert_row);

            return;
        }

        // insert_rows
        //   function: appends multiple rows to the local cache.
        void insert_rows(const std::vector<row_type>& _rows)
        {
            validate_mutable("insert_rows");

            // pre-validate all widths before mutating
            for (const auto& r : _rows)
            {
                check_row_width(r, "insert_rows");
            }

            m_data.insert(m_data.end(),
                          _rows.begin(),
                          _rows.end());
            m_num_rows += _rows.size();
            mark_dirty(modify_action::bulk_insert);

            return;
        }

        // remove_row
        //   function: removes a row by index from the local cache.
        void remove_row(size_type _row)
        {
            validate_mutable("remove_row");
            check_row(_row, "remove_row");

            m_data.erase(m_data.begin()
                         + static_cast<difference_type>(_row));
            --m_num_rows;
            mark_dirty(modify_action::remove_row);

            return;
        }

        // add_row
        //   function: appends an empty (default-valued) row.
        void add_row()
        {
            validate_mutable("add_row");

            m_data.emplace_back(m_num_cols);
            ++m_num_rows;
            mark_dirty(modify_action::insert_row);

            return;
        }

        // clear
        //   function: removes all rows from the local cache.
        void clear()
        {
            validate_mutable("clear");

            m_data.clear();
            m_num_rows = 0;
            mark_dirty(modify_action::clear_all);

            return;
        }

        // update_cell
        //   function: sets a single cell value in the local cache.
        void update_cell(size_type         _row,
                         size_type         _col,
                         const value_type& _value)
        {
            validate_mutable("update_cell");
            check_bounds(_row, _col, "update_cell");

            m_data[_row][_col] = _value;
            mark_dirty(modify_action::update_cell);

            return;
        }

        // update_cell (by column name)
        //   function: sets a cell value addressed by column name.
        void update_cell(size_type         _row,
                         std::string_view  _column_name,
                         const value_type& _value)
        {
            size_type col_idx = resolve_column(_column_name, "update_cell");

            update_cell(_row, col_idx, _value);

            return;
        }


        // =================================================================
        //  COLUMN MUTATION
        // =================================================================

        // add_column
        //   function: appends a column to the schema and extends every
        // cached row with a default value.
        void add_column(const column_info& _col_info)
        {
            validate_mutable("add_column");

            m_schema.columns.push_back(_col_info);
            ++m_num_cols;

            // extend all existing rows
            for (auto& r : m_data)
            {
                r.emplace_back();
            }

            mark_dirty(modify_action::add_column);

            return;
        }

        // add_column (convenience)
        //   function: appends a column with only name and type specified.
        void add_column(std::string _name,
                        field_type  _type = field_type::string)
        {
            add_column(column_info(std::move(_name), _type));

            return;
        }

        // remove_column
        //   function: removes a column by index from the schema and
        // all cached rows.
        void remove_column(size_type _col)
        {
            validate_mutable("remove_column");
            check_col(_col, "remove_column");

            m_schema.columns.erase(
                m_schema.columns.begin()
                + static_cast<difference_type>(_col));
            --m_num_cols;

            // shrink all existing rows
            for (auto& r : m_data)
            {
                if (_col < r.size())
                {
                    r.erase(r.begin()
                            + static_cast<difference_type>(_col));
                }
            }

            mark_dirty(modify_action::remove_column);

            return;
        }

        // remove_column (by name)
        //   function: removes a column by name.
        void remove_column(std::string_view _column_name)
        {
            size_type col_idx = resolve_column(_column_name, "remove_column");

            remove_column(col_idx);

            return;
        }

        // resize
        //   function: resizes the local cache to the specified dimensions.
        // New cells are default-initialized.
        void resize(size_type _rows,
                    size_type _cols)
        {
            validate_mutable("resize");

            m_data.resize(_rows);

            for (auto& r : m_data)
            {
                r.resize(_cols);
            }

            m_num_rows = _rows;
            m_num_cols = _cols;
            mark_dirty(modify_action::clear_all);

            return;
        }


        // =================================================================
        //  SCHEMA ACCESSORS
        // =================================================================

        // get_schema
        //   function: const reference to the table schema.
        const table_schema& get_schema() const noexcept
        {
            return m_schema;
        }

        // column_name
        //   function: name of the column at the given index.
        const std::string& column_name(size_type _col) const
        {
            check_col(_col, "column_name");

            return m_schema.columns[_col].name;
        }

        // column_type
        //   function: field_type of the column at the given index.
        field_type column_type(size_type _col) const
        {
            check_col(_col, "column_type");

            return m_schema.columns[_col].type;
        }

        // column_count
        //   function: number of columns in the schema.
        size_type column_count() const noexcept
        {
            return m_num_cols;
        }


        // =================================================================
        //  CONNECTION / IDENTITY
        // =================================================================

        // get_connection
        //   function: pointer to the bound connection (may be null).
        _Connection* get_connection() noexcept
        {
            return m_connection;
        }

        // get_connection (const)
        //   function: const pointer to the bound connection.
        const _Connection* get_connection() const noexcept
        {
            return m_connection;
        }

        // set_connection
        //   function: rebinds the table to a different connection.
        // Marks the local cache as stale.
        void set_connection(_Connection& _conn)
        {
            m_connection = &_conn;
            m_stale      = true;

            return;
        }

        // is_connected
        //   function: true iff the table has a bound, live connection.
        bool is_connected() const noexcept
        {
            return ( (m_connection != nullptr) &&
                     (m_connection->is_connected()) );
        }

        // table_name
        //   function: the database table name.
        const std::string& table_name() const noexcept
        {
            return m_schema.table_name;
        }

        // kind
        //   function: the table_kind classification.
        table_kind kind() const noexcept
        {
            return m_kind;
        }

        // is_view
        //   function: true iff this table is a read-only view.
        bool is_view() const noexcept
        {
            return (m_kind == table_kind::view);
        }

        // is_mutable
        //   function: true iff this table accepts modifications. Base
        // tables and temporary tables are mutable; views and
        // materialized views are not.
        bool is_mutable() const noexcept
        {
            return ( (m_kind == table_kind::base_table) ||
                     (m_kind == table_kind::temporary) );
        }


        // =================================================================
        //  SYNCHRONIZATION
        // =================================================================

        // get_sync_config
        //   function: const reference to the sync configuration.
        const sync_config& get_sync_config() const noexcept
        {
            return m_sync;
        }

        // set_sync_config
        //   function: replaces the sync configuration.
        void set_sync_config(const sync_config& _config)
        {
            m_sync = _config;

            return;
        }

        // is_dirty
        //   function: true iff the local cache has uncommitted
        // modifications.
        bool is_dirty() const noexcept
        {
            return m_dirty;
        }

        // is_stale
        //   function: true iff the local cache may be outdated relative
        // to the database. For `sync_policy::periodic`, returns true
        // when the refresh interval has elapsed since the last refresh.
        bool is_stale() const noexcept
        {
            if (m_stale)
            {
                return true;
            }

            // periodic staleness check
            if (m_sync.policy == sync_policy::periodic)
            {
                auto now     = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<
                    std::chrono::milliseconds>(now - m_last_refresh);

                return (elapsed >= m_sync.refresh_interval);
            }

            return false;
        }

        // invalidate
        //   function: explicitly marks the local cache as stale.
        void invalidate() noexcept
        {
            m_stale = true;

            return;
        }

        // last_refresh
        //   function: the time point of the last successful refresh.
        std::chrono::steady_clock::time_point
        last_refresh() const noexcept
        {
            return m_last_refresh;
        }


        // =================================================================
        //  QUERY CONFIGURATION
        // =================================================================
        //   Filtering, ordering, and pagination clauses applied to
        // refresh queries. Setting any of these marks the local cache
        // as stale so the next access (under the appropriate sync
        // policy) re-fetches with the new clause.

        // set_where
        //   function: sets a WHERE clause filter for refresh queries.
        // The clause should NOT include the "WHERE" keyword.
        void set_where(std::string _clause)
        {
            m_where_clause = std::move(_clause);
            m_stale        = true;

            return;
        }

        // clear_where
        //   function: removes any active WHERE clause.
        void clear_where()
        {
            m_where_clause.clear();
            m_stale = true;

            return;
        }

        // get_where
        //   function: the current WHERE clause (empty if unset).
        const std::string& get_where() const noexcept
        {
            return m_where_clause;
        }

        // set_order
        //   function: sets an ORDER BY clause. The clause should NOT
        // include the "ORDER BY" keywords.
        void set_order(std::string _clause)
        {
            m_order_clause = std::move(_clause);
            m_stale        = true;

            return;
        }

        // clear_order
        //   function: removes any active ORDER BY clause.
        void clear_order()
        {
            m_order_clause.clear();
            m_stale = true;

            return;
        }

        // get_order
        //   function: the current ORDER BY clause (empty if unset).
        const std::string& get_order() const noexcept
        {
            return m_order_clause;
        }

        // set_limit
        //   function: sets the maximum number of rows to retrieve.
        void set_limit(std::optional<size_type> _limit)
        {
            m_limit = _limit;
            m_stale = true;

            return;
        }

        // get_limit
        //   function: the current row limit (nullopt if unset).
        std::optional<size_type> get_limit() const noexcept
        {
            return m_limit;
        }

        // set_offset
        //   function: sets the row offset for paginated retrieval.
        void set_offset(std::optional<size_type> _offset)
        {
            m_offset = _offset;
            m_stale  = true;

            return;
        }

        // get_offset
        //   function: the current row offset (nullopt if unset).
        std::optional<size_type> get_offset() const noexcept
        {
            return m_offset;
        }


        // =================================================================
        //  DATABASE OPERATIONS
        // =================================================================
        //   All of these are CONCRETE — no `virtual`, no override hooks.
        // Vendor variation is supplied by the `_Connection` template
        // parameter (which knows its own dialect) plus dialect-aware
        // free helpers (`quote_identifier`, `dialect_format_limit_offset`).

        // fetch_schema
        //   function: retrieves the table schema from the database
        // using the universal "zero-row SELECT" probe — issues
        // `SELECT * FROM <table> WHERE 1=0` and reads column metadata
        // from the resulting empty result set. Works on every supported
        // SQL back-end (MySQL / MariaDB / PostgreSQL / SQLite / Oracle)
        // because the result-set metadata path is part of every
        // vendor's wire protocol.
        //
        //   Richer schema information (primary keys, foreign keys,
        // indexes) requires vendor-specific introspection queries
        // (`INFORMATION_SCHEMA.*`, `pg_catalog.*`, `sqlite_master`,
        // `USER_TAB_COLUMNS`, …) and is left to the consumer to add
        // when needed.
        void fetch_schema()
        {
            validate_connected("fetch_schema");

            std::string query = "SELECT * FROM "
                + quote_identifier(m_schema.table_name,
                                   m_connection->get_database_type())
                + " WHERE 1=0";

            auto rs = m_connection->execute_query(query);

            m_schema.columns.clear();

            size_type col_count = rs->column_count();

            for (size_type i = 0; i < col_count; ++i)
            {
                column_info ci;

                ci.name = rs->column_name(i);
                ci.type = rs->column_type(i);

                m_schema.columns.push_back(std::move(ci));
            }

            m_num_cols = col_count;

            return;
        }

        // refresh
        //   function: re-fetches data from the database into the local
        // cache, respecting the active query configuration (where,
        // order, limit, offset). The select statement is built by
        // `build_select_query()` and uses dialect-aware helpers for
        // identifier quoting and LIMIT / OFFSET syntax.
        void refresh()
        {
            validate_connected("refresh");

            std::string query = build_select_query();
            auto        rs    = m_connection->execute_query(query);

            // ensure we know the column count
            if (m_num_cols == 0)
            {
                m_num_cols = rs->column_count();
            }

            m_data.clear();

            while (rs->next())
            {
                row_type r;
                r.reserve(m_num_cols);

                for (size_type c = 0; c < m_num_cols; ++c)
                {
                    r.push_back(rs->get_value(c));
                }

                m_data.push_back(std::move(r));
            }

            m_num_rows     = m_data.size();
            m_stale        = false;
            m_dirty        = false;
            m_last_refresh = std::chrono::steady_clock::now();

            return;
        }

        // commit
        //   function: pushes locally-modified data back to the database
        // inside a transaction. The full-replace strategy below is the
        // safe vendor-agnostic default — callers needing efficient
        // differential updates (INSERT/UPDATE/DELETE per row delta)
        // can call back with a hand-built statement; that customization
        // is out of scope for the generic class.
        void commit()
        {
            validate_connected("commit");
            validate_mutable("commit");

            if (!m_dirty)
            {
                return;
            }

            transaction<_Connection> txn(*m_connection);

            try
            {
                commit_replace();
                txn.commit();
                m_dirty = false;
            }
            catch (...)
            {
                txn.rollback();
                throw;
            }

            return;
        }

        // exists
        //   function: probes the database for the existence of the
        // backing table using the same zero-row select trick used by
        // `fetch_schema`.
        bool exists() const
        {
            validate_connected("exists");

            try
            {
                m_connection->execute_query(
                    "SELECT 1 FROM "
                    + quote_identifier(m_schema.table_name,
                                       m_connection->get_database_type())
                    + " WHERE 1=0");

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        // row_count_remote
        //   function: queries the database for the row count without
        // fetching the rows themselves. Respects any active WHERE
        // clause.
        std::int64_t row_count_remote() const
        {
            validate_connected("row_count_remote");

            std::string query = "SELECT COUNT(*) FROM "
                + quote_identifier(m_schema.table_name,
                                   m_connection->get_database_type());

            if (!m_where_clause.empty())
            {
                query += " WHERE " + m_where_clause;
            }

            auto rs = m_connection->execute_query(query);

            if (rs->next())
            {
                auto val = rs->get_long(static_cast<std::size_t>(0));

                if (val.has_value())
                {
                    return val.value();
                }
            }

            return 0;
        }


        // =================================================================
        //  COMPARISON
        // =================================================================

        // operator==
        //   function: compares two database tables by table name and
        // local cache contents.
        bool operator==(const self_type& _other) const
        {
            return ( (m_schema.table_name == _other.m_schema.table_name) &&
                     (m_num_rows          == _other.m_num_rows)          &&
                     (m_num_cols          == _other.m_num_cols)          &&
                     (m_data              == _other.m_data) );
        }

        // operator!=
        //   function: inequality comparison.
        bool operator!=(const self_type& _other) const
        {
            return !(*this == _other);
        }


    protected:

        // =================================================================
        //  INTERNAL HELPERS
        // =================================================================

        // validate_connected
        //   helper: throws `connection_exception` if no live connection
        // is bound.
        void validate_connected(const char* _caller) const
        {
            if (!is_connected())
            {
                throw connection_exception(
                    std::string("database_table::") + _caller
                    + ": no active connection.");
            }

            return;
        }

        // validate_mutable
        //   helper: throws `query_exception` if the table kind does not
        // permit modifications.
        void validate_mutable(const char* _caller) const
        {
            if (!is_mutable())
            {
                throw query_exception(
                    std::string("database_table::") + _caller
                    + ": table is not mutable (kind="
                    + std::to_string(static_cast<int>(m_kind))
                    + ").");
            }

            return;
        }

        // check_row
        //   helper: bounds-checks a row index.
        void check_row(size_type   _row,
                       const char* _caller) const
        {
            if (_row >= m_num_rows)
            {
                throw std::out_of_range(
                    std::string("database_table::") + _caller
                    + ": row index out of range.");
            }

            return;
        }

        // check_col
        //   helper: bounds-checks a column index.
        void check_col(size_type   _col,
                       const char* _caller) const
        {
            if (_col >= m_num_cols)
            {
                throw std::out_of_range(
                    std::string("database_table::") + _caller
                    + ": column index out of range.");
            }

            return;
        }

        // check_bounds
        //   helper: bounds-checks a (row, col) pair.
        void check_bounds(size_type   _row,
                          size_type   _col,
                          const char* _caller) const
        {
            if ( (_row >= m_num_rows) ||
                 (_col >= m_num_cols) )
            {
                throw std::out_of_range(
                    std::string("database_table::") + _caller
                    + ": row or column index out of range.");
            }

            return;
        }

        // check_row_width
        //   helper: validates that `_row` has the expected column count.
        void check_row_width(const row_type& _row,
                             const char*     _caller) const
        {
            if (_row.size() != m_num_cols)
            {
                throw query_exception(
                    std::string("database_table::") + _caller
                    + ": row width does not match column count.");
            }

            return;
        }

        // resolve_column
        //   helper: resolves a column name to an index, throwing on
        // unknown name.
        size_type resolve_column(std::string_view _name,
                                 const char*      _caller) const
        {
            auto idx = m_schema.column_index(_name);

            if (!idx.has_value())
            {
                throw std::out_of_range(
                    std::string("database_table::") + _caller
                    + ": unknown column '"
                    + std::string(_name) + "'.");
            }

            return idx.value();
        }

        // mark_dirty
        //   helper: flags the local cache as modified and triggers an
        // auto-commit when configured and the action is tracked.
        void mark_dirty(modify_action _action)
        {
            m_dirty = true;

            // auto-commit when configured and the action is tracked
            if ( (m_sync.auto_commit)             &&
                 (m_sync.tracks_action(_action))  &&
                 (is_connected()) )
            {
                commit();
                m_dirty = false;
            }

            return;
        }

        // ensure_fresh
        //   helper: refreshes the local cache when the sync policy is
        // `on_access` and the cache is stale.
        void ensure_fresh()
        {
            if ( (m_sync.policy == sync_policy::on_access) &&
                 (is_stale())                              &&
                 (is_connected()) )
            {
                refresh();
            }

            return;
        }

        // build_select_query
        //   helper: builds the SELECT statement used by `refresh()`.
        // Identifier quoting is dialect-aware via `quote_identifier`;
        // LIMIT / OFFSET formatting is dialect-aware via
        // `internal::dialect_format_limit_offset`.
        std::string build_select_query() const
        {
            std::string query = "SELECT * FROM "
                + quote_identifier(m_schema.table_name,
                                   m_connection->get_database_type());

            if (!m_where_clause.empty())
            {
                query += " WHERE " + m_where_clause;
            }

            if (!m_order_clause.empty())
            {
                query += " ORDER BY " + m_order_clause;
            }

            query += internal::dialect_format_limit_offset(
                m_connection->get_database_type(),
                m_limit,
                m_offset);

            return query;
        }

        // commit_replace
        //   helper: vendor-agnostic full-replace commit strategy used
        // by `commit()`. Truncates the backing table and re-inserts
        // every cached row. Wrapped in a transaction by the caller.
        //
        //   The strategy is intentionally simple and safe; users
        // needing differential commits should issue their own
        // INSERT / UPDATE / DELETE statements through the connection.
        void commit_replace()
        {
            std::string qname = quote_identifier(
                m_schema.table_name,
                m_connection->get_database_type());

            // truncate the table
            m_connection->execute_update("DELETE FROM " + qname);

            // re-insert every cached row, if any
            if ( (m_num_rows == 0) ||
                 (m_num_cols == 0) )
            {
                return;
            }

            // build the column-name list and parameter placeholder list
            std::string col_list;
            std::string ph_list;

            for (size_type c = 0; c < m_num_cols; ++c)
            {
                if (c > 0)
                {
                    col_list += ", ";
                    ph_list  += ", ";
                }

                col_list += quote_identifier(
                    m_schema.columns[c].name,
                    m_connection->get_database_type());
                ph_list  += "?";
            }

            std::string insert_sql = "INSERT INTO " + qname
                + " (" + col_list + ") VALUES (" + ph_list + ")";

            auto stmt = m_connection->prepare(insert_sql);

            for (size_type r = 0; r < m_num_rows; ++r)
            {
                stmt->clear_parameters();

                for (size_type c = 0; c < m_num_cols; ++c)
                {
                    bind_value(*stmt, c + 1, m_data[r][c]);
                }

                stmt->execute_update();
            }

            return;
        }

        // bind_value
        //   helper: dispatches a `value` variant onto the appropriate
        // statement bind method. Index is 1-based per the statement
        // CRTP surface convention.
        template<typename _Statement>
        static void bind_value(_Statement&       _stmt,
                               std::size_t       _index,
                               const value_type& _v)
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
                    else if constexpr (std::is_same_v<arg_t, std::int32_t>)
                    {
                        _stmt.bind_int(_index, _arg);
                    }
                    else if constexpr (std::is_same_v<arg_t, std::int64_t>)
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
                        arg_t, std::vector<std::uint8_t>>)
                    {
                        _stmt.bind_binary(_index, _arg);
                    }
                    else
                    {
                        // timestamp and any future variant arms fall
                        // through to a null bind by default; vendors
                        // requiring native timestamp binding should
                        // override at the call site.
                        _stmt.bind_null(_index);
                    }

                    return;
                },
                _v);

            return;
        }


        // =================================================================
        //  PROTECTED MEMBERS
        // =================================================================

        _Connection* m_connection;
        table_schema m_schema;
        table_kind   m_kind;
        sync_config  m_sync;
        storage_type m_data;

        bool      m_dirty;
        bool      m_stale;
        size_type m_num_rows;
        size_type m_num_cols;

        std::chrono::steady_clock::time_point m_last_refresh;

        // query configuration
        std::string              m_where_clause;
        std::string              m_order_clause;
        std::optional<size_type> m_limit;
        std::optional<size_type> m_offset;
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_TABLE_
