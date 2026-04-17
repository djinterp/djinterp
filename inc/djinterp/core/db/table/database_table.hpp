/******************************************************************************
* djinterp [database]                                     database_table.hpp
*
* djinterp database table module:
*   A dynamic, database-backed table class providing the structural interface
* expected by the table and container trait systems, while storing data
* retrieved from (and optionally synchronized with) a live database
* connection.
*
*   Unlike the compile-time-fixed table<T, Rows, Cols>, a database_table has
* runtime-determined dimensions: rows arrive from queries, columns from
* schema introspection. The class bridges the container and database
* subsystems — it satisfies is_table_type detection (rows(), cols(), cell())
* and participates in the container_traits twelve-axis classification, while
* also exposing connection, transaction, and schema management facilities.
*
*   TABLE KINDS:
*     base_table         — an ordinary read/write table
*     view               — a read-only view (immutable)
*     materialized_view  — a cached, periodically refreshed view
*     temporary          — a session-scoped temporary table
*
*   SYNC POLICIES (when local cache refreshes from the database):
*     manual             — only on explicit refresh() calls
*     on_access          — lazy refresh on first read after invalidation
*     on_modify          — refresh after every local modification is committed
*     periodic           — refresh at a caller-configured interval
*
*   STRUCTURAL DETECTION:
*     The class exposes rows(), cols(), cell(), and config_type — the same
*   method-level interface as table<> — but does NOT expose static constexpr
*   num_rows / num_cols / total_cells (dimensions are runtime).  Therefore:
*     - is_table_type<database_table<...>>  → FALSE  (requires fixed dims)
*     - is_database_table<database_table<...>> → TRUE (new trait, this header)
*     - is_any_table<database_table<...>>   → TRUE  (unifying super-trait)
*   Shape modifiers (add_row, remove_row, add_column, remove_column, resize)
*   are always structurally present — view immutability is enforced at
*   runtime via validate_mutable().  This means has_shape_modifiers will
*   be true for all database_table instantiations regardless of table_kind.
*
*   CONTAINER_TRAITS CLASSIFICATION (auto-detected):
*     Lifetime:    mutable_storage (dynamic row count, mutable cells)
*     Bounds:      unbounded (row count is runtime-variable)
*     Storage:     dynamic (allocator-backed)
*     Iteration:   random_access (index-based row/column access)
*     Ordering:    ordered (row insertion order preserved)
*     Sorted:      unsorted
*     Uniqueness:  allows duplicates
*     Structure:   flat
*     Backing:     fundamental (owns its local cache)
*     Database:    round_trip (serialize + deserialize through connection)
*
*   This module is vendor-agnostic. Vendor-specific subclasses (e.g.
* sqlite_database_table) specialize refresh, commit, and schema operations
* by providing a concrete _Connection type that satisfies the database
* connection CRTP interface from database_common.hpp.
*
*   PORTABILITY:
*   Requires C++17 or later (std::optional, std::string_view, std::variant).
*
* path:      \inc\database\database_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_DATABASE_TABLE_
#define DJINTERP_DATABASE_TABLE_ 1

#include "../database_common.hpp"
#include "../../container/table/table_traits.hpp"

#if !D_ENV_LANG_IS_CPP17_OR_HIGHER
    #error "database_table.hpp requires C++17 or later                       \
            (std::optional, std::variant, std::string_view)."
#endif

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>


NS_DJINTERP
NS_DB


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
    // sync_policy::on_modify filtering.
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
            : type(field_type::null)
            , nullable(true)
            , is_primary_key(false)
            , is_auto_increment(false)
            , is_unique(false)
            , is_indexed(false)
        {
        }

        explicit column_info(
                std::string _name,
                field_type  _type        = field_type::string,
                bool        _nullable    = true,
                bool        _primary_key = false
            )
                : name(std::move(_name))
                , type(_type)
                , nullable(_nullable)
                , is_primary_key(_primary_key)
                , is_auto_increment(false)
                , is_unique(_primary_key)
                , is_indexed(_primary_key)
        {
        }
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

        std::size_t column_count() const noexcept
        {
            return columns.size();
        }

        std::optional<std::size_t> column_index(
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

        const column_info* column_by_name(
                std::string_view _name
            ) const
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
    //   struct: configuration for the synchronization policy.
    // Controls refresh timing and modification filtering.
    struct sync_config
    {
        sync_policy                   policy;
        std::chrono::milliseconds     refresh_interval;
        std::vector<modify_action>    tracked_actions;
        bool                          auto_commit;

        sync_config()
            : policy(sync_policy::manual)
            , refresh_interval(std::chrono::milliseconds(0))
            , auto_commit(false)
        {
        }

        explicit sync_config(
                sync_policy _policy
            )
                : policy(_policy)
                , refresh_interval(std::chrono::milliseconds(0))
                , auto_commit(false)
        {
        }

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
    // IV.  DATABASE TABLE CLASS
    // =========================================================================
    //
    // The primary database-backed table.  Satisfies table structural detection
    // (is_table_type) with runtime dimensions and dynamic storage.  Shape
    // modifiers are conditionally defined: views suppress them so that
    // is_structurally_immutable fires correctly.
    //
    // Template parameters:
    //   _Connection — a concrete CRTP connection type satisfying the
    //                 database::connection interface.
    //   _ValueType  — the cell value type.  Defaults to db::value (the
    //                 std::variant covering all common database types).
    //   _Config     — an optional table_traits-compatible configuration
    //                 struct (headers, footers, spans, etc.).
    //

    // database_table
    //   class: dynamic database-backed table with configurable mutability,
    // schema introspection, and synchronization policies.  Provides the
    // structural interface required by is_table_type and the container
    // trait system.
    template<typename _Connection,
             typename _ValueType = value,
             typename _Config    = container::empty_config>
    class database_table
    {
    public:
        // -----------------------------------------------------------------
        //  standard container type aliases (structural detection targets)
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
        //  row-level iterator types (structural detection: iterable)
        //  Iteration is over rows (each element is a row_type vector).
        // -----------------------------------------------------------------
        using row_iterator               = typename storage_type::iterator;
        using const_row_iterator         = typename storage_type::const_iterator;
        using reverse_row_iterator       = typename storage_type::reverse_iterator;
        using const_reverse_row_iterator = typename storage_type::const_reverse_iterator;

        // -----------------------------------------------------------------
        //  self type, config, and database types
        // -----------------------------------------------------------------
        using self_type       = database_table<_Connection, _ValueType, _Config>;
        using config_type     = _Config;
        using connection_type = _Connection;
        using schema_type     = table_schema;
        using row_type        = std::vector<value_type>;
        using storage_type    = std::vector<row_type>;

        // -----------------------------------------------------------------
        //  runtime dimensional constants
        //  (not static constexpr — dimensions are dynamic)
        // -----------------------------------------------------------------

        // num_rows / num_cols are exposed as non-static members so that
        // traits probing T::num_rows will SFINAE-fail; the dynamic
        // counterpart is rows() / cols() which the detection idiom also
        // recognizes.

        // -----------------------------------------------------------------
        //  compile-time kind and policy constants
        // -----------------------------------------------------------------
        static constexpr bool has_dynamic_rows = true;
        static constexpr bool has_dynamic_cols = true;


        // =================================================================
        //  constructors
        // =================================================================

        // database_table()
        //   constructor: default — creates an empty, disconnected table.
        database_table()
            : m_connection(nullptr)
            , m_kind(table_kind::base_table)
            , m_dirty(false)
            , m_stale(true)
            , m_num_rows(0)
            , m_num_cols(0)
        {
        }

        // database_table(connection, name)
        //   constructor: creates a table bound to a connection and table
        // name.  Does not automatically fetch schema or data; call
        // fetch_schema() and refresh() after construction.
        explicit database_table(
                _Connection&       _conn,
                std::string        _table_name,
                table_kind         _kind = table_kind::base_table
            )
                : m_connection(&_conn)
                , m_kind(_kind)
                , m_dirty(false)
                , m_stale(true)
                , m_num_rows(0)
                , m_num_cols(0)
        {
            m_schema.table_name = std::move(_table_name);
        }

        // database_table(connection, schema)
        //   constructor: creates a table bound to a connection with an
        // explicit schema.  Useful when schema is already known or was
        // retrieved externally.
        explicit database_table(
                _Connection&  _conn,
                table_schema  _schema,
                table_kind    _kind = table_kind::base_table
            )
                : m_connection(&_conn)
                , m_schema(std::move(_schema))
                , m_kind(_kind)
                , m_dirty(false)
                , m_stale(true)
                , m_num_rows(0)
                , m_num_cols(0)
        {
            m_num_cols = m_schema.columns.size();
        }

        // database_table(connection, schema, sync)
        //   constructor: creates a table with an explicit sync policy.
        explicit database_table(
                _Connection&      _conn,
                table_schema      _schema,
                table_kind        _kind,
                const sync_config& _sync
            )
                : m_connection(&_conn)
                , m_schema(std::move(_schema))
                , m_kind(_kind)
                , m_sync(_sync)
                , m_dirty(false)
                , m_stale(true)
                , m_num_rows(0)
                , m_num_cols(0)
        {
            m_num_cols = m_schema.columns.size();
        }

        virtual ~database_table() = default;

        // disable copying (connection handle is non-owning)
        database_table(const database_table&)            = delete;
        database_table& operator=(const database_table&) = delete;

        // enable moving
        database_table(database_table&& _other) noexcept
            : m_connection(_other.m_connection)
            , m_schema(std::move(_other.m_schema))
            , m_kind(_other.m_kind)
            , m_sync(std::move(_other.m_sync))
            , m_data(std::move(_other.m_data))
            , m_dirty(_other.m_dirty)
            , m_stale(_other.m_stale)
            , m_num_rows(_other.m_num_rows)
            , m_num_cols(_other.m_num_cols)
            , m_last_refresh(_other.m_last_refresh)
            , m_where_clause(std::move(_other.m_where_clause))
            , m_order_clause(std::move(_other.m_order_clause))
            , m_limit(_other.m_limit)
            , m_offset(_other.m_offset)
        {
            _other.m_connection = nullptr;
            _other.m_num_rows   = 0;
            _other.m_num_cols   = 0;
            _other.m_dirty      = false;
            _other.m_stale      = true;
        }

        database_table& operator=(database_table&& _other) noexcept
        {
            if (this != &_other)
            {
                m_connection    = _other.m_connection;
                m_schema        = std::move(_other.m_schema);
                m_kind          = _other.m_kind;
                m_sync          = std::move(_other.m_sync);
                m_data          = std::move(_other.m_data);
                m_dirty         = _other.m_dirty;
                m_stale         = _other.m_stale;
                m_num_rows      = _other.m_num_rows;
                m_num_cols      = _other.m_num_cols;
                m_last_refresh  = _other.m_last_refresh;
                m_where_clause  = std::move(_other.m_where_clause);
                m_order_clause  = std::move(_other.m_order_clause);
                m_limit         = _other.m_limit;
                m_offset        = _other.m_offset;

                _other.m_connection = nullptr;
                _other.m_num_rows   = 0;
                _other.m_num_cols   = 0;
                _other.m_dirty      = false;
                _other.m_stale      = true;
            }

            return *this;
        }


        // =================================================================
        //  capacity (structural detection: sized, dynamic)
        // =================================================================

        // size
        //   function: returns the total number of cells in the local cache.
        size_type size() const noexcept
        {
            return (m_num_rows * m_num_cols);
        }

        // empty
        //   function: returns whether the local cache contains zero cells.
        bool empty() const noexcept
        {
            return (m_num_rows == 0);
        }

        // rows
        //   function: returns the number of rows in the local cache.
        // Detected by: detect_rows_method (table type detection).
        size_type rows() const noexcept
        {
            return m_num_rows;
        }

        // cols
        //   function: returns the number of columns.
        // Detected by: detect_cols_method (table type detection).
        size_type cols() const noexcept
        {
            return m_num_cols;
        }

        // total_cells
        //   function: returns the total cell count (rows * cols).
        size_type total_cells() const noexcept
        {
            return (m_num_rows * m_num_cols);
        }


        // =================================================================
        //  element access
        // =================================================================

        // cell
        //   function: unchecked two-dimensional element access.
        // Detected by: detect_cell_method (table type detection).
        reference cell(
                size_type _row,
                size_type _col
            )
        {
            ensure_fresh();

            return m_data[_row][_col];
        }

        // cell (const)
        //   function: unchecked two-dimensional const element access.
        const_reference cell(
                size_type _row,
                size_type _col
            ) const
        {
            return m_data[_row][_col];
        }

        // at
        //   function: two-dimensional element access with bounds checking.
        reference at(
                size_type _row,
                size_type _col
            )
        {
            ensure_fresh();

            // bounds validation
            if ( (_row >= m_num_rows) ||
                 (_col >= m_num_cols) )
            {
                throw std::out_of_range(
                    "database_table::at: row or column index out of range.");
            }

            return m_data[_row][_col];
        }

        // at (const)
        //   function: two-dimensional const element access with bounds
        // checking.
        const_reference at(
                size_type _row,
                size_type _col
            ) const
        {
            // bounds validation
            if ( (_row >= m_num_rows) ||
                 (_col >= m_num_cols) )
            {
                throw std::out_of_range(
                    "database_table::at: row or column index out of range.");
            }

            return m_data[_row][_col];
        }

        // operator[]
        //   function: row-indexed access returning a reference to the
        // underlying row vector.
        row_type& operator[](size_type _row)
        {
            ensure_fresh();

            return m_data[_row];
        }

        // operator[] (const)
        //   function: row-indexed const access.
        const row_type& operator[](size_type _row) const
        {
            return m_data[_row];
        }

        // cell_by_name
        //   function: element access by column name with bounds checking.
        reference cell_by_name(
                size_type        _row,
                std::string_view _column_name
            )
        {
            ensure_fresh();

            auto col_idx = m_schema.column_index(_column_name);

            if (!col_idx.has_value())
            {
                throw std::out_of_range(
                    "database_table::cell_by_name: unknown column.");
            }

            // bounds validation
            if (_row >= m_num_rows)
            {
                throw std::out_of_range(
                    "database_table::cell_by_name: row index out of range.");
            }

            return m_data[_row][col_idx.value()];
        }

        // cell_by_name (const)
        //   function: const element access by column name.
        const_reference cell_by_name(
                size_type        _row,
                std::string_view _column_name
            ) const
        {
            auto col_idx = m_schema.column_index(_column_name);

            if (!col_idx.has_value())
            {
                throw std::out_of_range(
                    "database_table::cell_by_name: unknown column.");
            }

            // bounds validation
            if (_row >= m_num_rows)
            {
                throw std::out_of_range(
                    "database_table::cell_by_name: row index out of range.");
            }

            return m_data[_row][col_idx.value()];
        }

        // get_row
        //   function: returns a copy of a single row.
        row_type get_row(size_type _row) const
        {
            // bounds validation
            if (_row >= m_num_rows)
            {
                throw std::out_of_range(
                    "database_table::get_row: row index out of range.");
            }

            return m_data[_row];
        }

        // get_column
        //   function: returns all values in a column by index.
        std::vector<value_type> get_column(size_type _col) const
        {
            // bounds validation
            if (_col >= m_num_cols)
            {
                throw std::out_of_range(
                    "database_table::get_column: column index out of range.");
            }

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
        std::vector<value_type> get_column(
                std::string_view _column_name
            ) const
        {
            auto col_idx = m_schema.column_index(_column_name);

            if (!col_idx.has_value())
            {
                throw std::out_of_range(
                    "database_table::get_column: unknown column.");
            }

            return get_column(col_idx.value());
        }


        // =================================================================
        //  row-level iteration (structural detection: iterable)
        //  Each iteration step yields a row_type (vector of value_type).
        // =================================================================

        // row_begin
        //   function: returns a row iterator to the first row.
        row_iterator row_begin() noexcept
        {
            return m_data.begin();
        }

        // row_begin (const)
        //   function: returns a const row iterator to the first row.
        const_row_iterator row_begin() const noexcept
        {
            return m_data.begin();
        }

        // row_end
        //   function: returns a row iterator past the last row.
        row_iterator row_end() noexcept
        {
            return m_data.end();
        }

        // row_end (const)
        //   function: returns a const row iterator past the last row.
        const_row_iterator row_end() const noexcept
        {
            return m_data.end();
        }

        // row_cbegin
        //   function: returns a const row iterator to the first row.
        const_row_iterator row_cbegin() const noexcept
        {
            return m_data.cbegin();
        }

        // row_cend
        //   function: returns a const row iterator past the last row.
        const_row_iterator row_cend() const noexcept
        {
            return m_data.cend();
        }

        // row_rbegin
        //   function: returns a reverse row iterator to the last row.
        reverse_row_iterator row_rbegin() noexcept
        {
            return m_data.rbegin();
        }

        // row_rbegin (const)
        //   function: returns a const reverse row iterator to the last
        // row.
        const_reverse_row_iterator row_rbegin() const noexcept
        {
            return m_data.rbegin();
        }

        // row_rend
        //   function: returns a reverse row iterator before the first
        // row.
        reverse_row_iterator row_rend() noexcept
        {
            return m_data.rend();
        }

        // row_rend (const)
        //   function: returns a const reverse row iterator before the
        // first row.
        const_reverse_row_iterator row_rend() const noexcept
        {
            return m_data.rend();
        }

        // begin / end  (range-based for loop support)
        //   function: aliases for row_begin / row_end enabling
        // range-based for: `for (auto& row : my_db_table) { ... }`
        row_iterator begin() noexcept
        {
            return row_begin();
        }

        const_row_iterator begin() const noexcept
        {
            return row_begin();
        }

        row_iterator end() noexcept
        {
            return row_end();
        }

        const_row_iterator end() const noexcept
        {
            return row_end();
        }


        // =================================================================
        //  row modification (structural detection: shape modifiers)
        //  Gated: only available for non-view table kinds.
        // =================================================================

        // insert_row
        //   function: appends a row to the local cache.
        // Throws if the table is a view.
        void insert_row(const row_type& _row)
        {
            validate_mutable("insert_row");

            // width validation
            if (_row.size() != m_num_cols)
            {
                throw query_exception(
                    "database_table::insert_row: row width does not "
                    "match column count.");
            }

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

            // width validation
            if (_row.size() != m_num_cols)
            {
                throw query_exception(
                    "database_table::insert_row: row width does not "
                    "match column count.");
            }

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

            for (const auto& r : _rows)
            {
                // width validation
                if (r.size() != m_num_cols)
                {
                    throw query_exception(
                        "database_table::insert_rows: row width does not "
                        "match column count.");
                }
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
        // Detected by: detect_remove_row (shape modifier).
        void remove_row(size_type _row)
        {
            validate_mutable("remove_row");

            // bounds validation
            if (_row >= m_num_rows)
            {
                throw std::out_of_range(
                    "database_table::remove_row: row index out of range.");
            }

            m_data.erase(m_data.begin()
                         + static_cast<difference_type>(_row));
            --m_num_rows;
            mark_dirty(modify_action::remove_row);

            return;
        }

        // add_row
        //   function: appends an empty (default-valued) row.
        // Detected by: detect_add_row (shape modifier).
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
        void update_cell(
                size_type        _row,
                size_type        _col,
                const value_type& _value
            )
        {
            validate_mutable("update_cell");

            // bounds validation
            if ( (_row >= m_num_rows) ||
                 (_col >= m_num_cols) )
            {
                throw std::out_of_range(
                    "database_table::update_cell: index out of range.");
            }

            m_data[_row][_col] = _value;
            mark_dirty(modify_action::update_cell);

            return;
        }

        // update_cell (by column name)
        //   function: sets a cell value addressed by column name.
        void update_cell(
                size_type         _row,
                std::string_view  _column_name,
                const value_type& _value
            )
        {
            auto col_idx = m_schema.column_index(_column_name);

            if (!col_idx.has_value())
            {
                throw std::out_of_range(
                    "database_table::update_cell: unknown column.");
            }

            update_cell(_row,
                        col_idx.value(),
                        _value);

            return;
        }


        // =================================================================
        //  column modification (structural: add_column / remove_column)
        //  Gated: only available for non-view table kinds.
        // =================================================================

        // add_column
        //   function: appends a column to the schema and extends every
        // cached row with a default value.
        // Detected by: detect_add_column (shape modifier).
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
        //   function: appends a column with name and type only.
        void add_column(
                std::string _name,
                field_type  _type = field_type::string
            )
        {
            add_column(column_info(std::move(_name), _type));

            return;
        }

        // remove_column
        //   function: removes a column by index from the schema and
        // all cached rows.
        // Detected by: detect_remove_column (shape modifier).
        void remove_column(size_type _col)
        {
            validate_mutable("remove_column");

            // bounds validation
            if (_col >= m_num_cols)
            {
                throw std::out_of_range(
                    "database_table::remove_column: column index out "
                    "of range.");
            }

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
            auto col_idx = m_schema.column_index(_column_name);

            if (!col_idx.has_value())
            {
                throw std::out_of_range(
                    "database_table::remove_column: unknown column.");
            }

            remove_column(col_idx.value());

            return;
        }

        // resize
        //   function: resizes the local cache to the specified dimensions.
        // New cells are default-initialized.
        // Detected by: detect_resize (shape modifier).
        void resize(
                size_type _rows,
                size_type _cols
            )
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
        //  schema accessors
        // =================================================================

        // get_schema
        //   function: returns a const reference to the table schema.
        const table_schema& get_schema() const noexcept
        {
            return m_schema;
        }

        // column_name
        //   function: returns the name of the column at the given index.
        const std::string& column_name(size_type _col) const
        {
            // bounds validation
            if (_col >= m_num_cols)
            {
                throw std::out_of_range(
                    "database_table::column_name: column index out "
                    "of range.");
            }

            return m_schema.columns[_col].name;
        }

        // column_type
        //   function: returns the field_type of the column at the given
        // index.
        field_type column_type(size_type _col) const
        {
            // bounds validation
            if (_col >= m_num_cols)
            {
                throw std::out_of_range(
                    "database_table::column_type: column index out "
                    "of range.");
            }

            return m_schema.columns[_col].type;
        }

        // column_count
        //   function: returns the number of columns in the schema.
        size_type column_count() const noexcept
        {
            return m_num_cols;
        }


        // =================================================================
        //  database connection and identity
        // =================================================================

        // get_connection
        //   function: returns a pointer to the bound connection.
        _Connection* get_connection() noexcept
        {
            return m_connection;
        }

        // get_connection (const)
        //   function: returns a const pointer to the bound connection.
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
        //   function: returns whether the table has a bound, live
        // connection.
        bool is_connected() const noexcept
        {
            return ( (m_connection != nullptr) &&
                     (m_connection->is_connected()) );
        }

        // table_name
        //   function: returns the database table name.
        const std::string& table_name() const noexcept
        {
            return m_schema.table_name;
        }

        // kind
        //   function: returns the table_kind classification.
        table_kind kind() const noexcept
        {
            return m_kind;
        }

        // is_view
        //   function: returns whether this table is a read-only view.
        bool is_view() const noexcept
        {
            return (m_kind == table_kind::view);
        }

        // is_mutable
        //   function: returns whether this table supports modifications.
        bool is_mutable() const noexcept
        {
            return ( (m_kind == table_kind::base_table) ||
                     (m_kind == table_kind::temporary) );
        }


        // =================================================================
        //  synchronization control
        // =================================================================

        // get_sync_config
        //   function: returns a const reference to the sync configuration.
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
        //   function: returns whether the local cache has uncommitted
        // modifications.
        bool is_dirty() const noexcept
        {
            return m_dirty;
        }

        // is_stale
        //   function: returns whether the local cache may be outdated
        // relative to the database.
        bool is_stale() const noexcept
        {
            if (m_stale)
            {
                return true;
            }

            // periodic staleness check
            if (m_sync.policy == sync_policy::periodic)
            {
                auto now      = std::chrono::steady_clock::now();
                auto elapsed  = std::chrono::duration_cast<
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
        //   function: returns the time point of the last successful
        // refresh.
        std::chrono::steady_clock::time_point
        last_refresh() const noexcept
        {
            return m_last_refresh;
        }


        // =================================================================
        //  query configuration (filtering / ordering / pagination)
        // =================================================================

        // set_where
        //   function: sets a WHERE clause filter for refresh queries.
        // The clause should not include the "WHERE" keyword.
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
        //   function: returns the current WHERE clause, if any.
        const std::string& get_where() const noexcept
        {
            return m_where_clause;
        }

        // set_order
        //   function: sets an ORDER BY clause for refresh queries.
        // The clause should not include the "ORDER BY" keywords.
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
        //   function: returns the current ORDER BY clause, if any.
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
        //   function: returns the current row limit, if any.
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
        //   function: returns the current row offset, if any.
        std::optional<size_type> get_offset() const noexcept
        {
            return m_offset;
        }


        // =================================================================
        //  database operations (virtual — overridden by vendor subclasses)
        // =================================================================

        // fetch_schema
        //   function: retrieves the table schema from the database.
        // Vendor subclasses override this to issue the appropriate
        // introspection queries.
        virtual void fetch_schema()
        {
            validate_connected("fetch_schema");

            // default implementation: use INFORMATION_SCHEMA-style query.
            // vendor subclasses should override with vendor-appropriate
            // introspection.  The base implementation is intentionally
            // minimal — it sets column count from the result set metadata
            // of a zero-row SELECT.

            auto rs = m_connection->execute_query(
                "SELECT * FROM "
                + quote_identifier(m_schema.table_name,
                                   m_connection->get_database_type())
                + " WHERE 1=0");

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
        // cache, respecting query configuration (where, order, limit,
        // offset).  Vendor subclasses override to build vendor-specific
        // SQL.
        virtual void refresh()
        {
            validate_connected("refresh");

            std::string query = build_select_query();

            auto rs = m_connection->execute_query(query);

            // ensure we know the column count
            if (m_num_cols == 0)
            {
                m_num_cols = rs->column_count();
            }

            m_data.clear();

            while (rs->next())
            {
                row_type row;
                row.reserve(m_num_cols);

                for (size_type c = 0; c < m_num_cols; ++c)
                {
                    row.push_back(rs->get_value(c));
                }

                m_data.push_back(std::move(row));
            }

            m_num_rows     = m_data.size();
            m_stale        = false;
            m_last_refresh = std::chrono::steady_clock::now();

            return;
        }

        // commit
        //   function: pushes locally modified data back to the database.
        // Vendor subclasses override to generate appropriate INSERT /
        // UPDATE / DELETE statements.  The base implementation provides
        // a transaction-wrapped stub that vendor modules flesh out.
        virtual void commit()
        {
            validate_connected("commit");
            validate_mutable("commit");

            if (!m_dirty)
            {
                return;
            }

            // base implementation: full-replace strategy wrapped in a
            // transaction.  Vendor subclasses should provide efficient
            // differential updates.
            transaction<_Connection> txn(*m_connection);

            try
            {
                commit_impl();
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
        //   function: checks whether the backing table exists in the
        // database.
        virtual bool exists() const
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
        //   function: returns the row count from the database without
        // fetching all data.
        virtual std::int64_t row_count_remote() const
        {
            validate_connected("row_count_remote");

            std::string query =
                "SELECT COUNT(*) FROM "
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
        //  config-aware accessors (inherited from table_traits system)
        // =================================================================

        // header_rows
        //   function: returns the number of header rows from config.
        static constexpr size_type header_rows() noexcept
        {
            return container::get_header_rows<_Config>::value;
        }

        // header_cols
        //   function: returns the number of header columns from config.
        static constexpr size_type header_cols() noexcept
        {
            return container::get_header_cols<_Config>::value;
        }

        // footer_rows
        //   function: returns the number of footer rows from config.
        static constexpr size_type footer_rows() noexcept
        {
            return container::get_footer_rows<_Config>::value;
        }

        // footer_cols
        //   function: returns the number of footer columns from config.
        static constexpr size_type footer_cols() noexcept
        {
            return container::get_footer_cols<_Config>::value;
        }


        // =================================================================
        //  comparison operators
        // =================================================================

        // operator==
        //   function: compares two database tables by table name and
        // local cache contents.
        bool operator==(const self_type& _other) const
        {
            return ( (m_schema.table_name == _other.m_schema.table_name) &&
                     (m_num_rows == _other.m_num_rows)                   &&
                     (m_num_cols == _other.m_num_cols)                    &&
                     (m_data     == _other.m_data) );
        }

        // operator!=
        //   function: inequality comparison.
        bool operator!=(const self_type& _other) const
        {
            return !(*this == _other);
        }


    protected:

        // =================================================================
        //  protected helpers
        // =================================================================

        // validate_connected
        //   function: throws if no active connection exists.
        void validate_connected(const char* _caller) const
        {
            if (!is_connected())
            {
                throw connection_exception(
                    std::string("database_table::")
                    + _caller
                    + ": no active connection.");
            }

            return;
        }

        // validate_mutable
        //   function: throws if the table kind does not permit
        // modifications.
        void validate_mutable(const char* _caller) const
        {
            if (!is_mutable())
            {
                throw query_exception(
                    std::string("database_table::")
                    + _caller
                    + ": table is not mutable (kind="
                    + std::to_string(static_cast<int>(m_kind))
                    + ").");
            }

            return;
        }

        // mark_dirty
        //   function: flags the local cache as modified and triggers
        // any sync_policy callbacks.
        void mark_dirty(modify_action _action)
        {
            m_dirty = true;

            // auto-commit if configured and the action is tracked
            if ( (m_sync.auto_commit)        &&
                 (m_sync.tracks_action(_action)) &&
                 (is_connected()) )
            {
                commit();
                m_dirty = false;
            }

            return;
        }

        // ensure_fresh
        //   function: refreshes the local cache if the sync policy
        // demands it and the cache is stale.
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
        //   function: constructs a SELECT statement from the table name
        // and active query configuration.  Vendor subclasses may override
        // for dialect-specific syntax.
        virtual std::string build_select_query() const
        {
            std::string query =
                "SELECT * FROM "
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

            if (m_limit.has_value())
            {
                query += " LIMIT "
                         + std::to_string(m_limit.value());
            }

            if (m_offset.has_value())
            {
                query += " OFFSET "
                         + std::to_string(m_offset.value());
            }

            return query;
        }

        // commit_impl
        //   function: internal commit implementation.  Vendor subclasses
        // override this to provide efficient differential updates.
        // The default is an intentional no-op; vendors fill it in.
        virtual void commit_impl()
        {
            // vendor subclasses implement INSERT / UPDATE / DELETE
            // generation here.  The base class wraps this call in
            // a transaction (see commit()).

            return;
        }


        // =================================================================
        //  protected members
        // =================================================================

        _Connection*  m_connection;
        table_schema  m_schema;
        table_kind    m_kind;
        sync_config   m_sync;
        storage_type  m_data;

        bool          m_dirty;
        bool          m_stale;
        size_type     m_num_rows;
        size_type     m_num_cols;

        std::chrono::steady_clock::time_point m_last_refresh;

        // query configuration
        std::string                m_where_clause;
        std::string                m_order_clause;
        std::optional<size_type>   m_limit;
        std::optional<size_type>   m_offset;
    };


    // =========================================================================
    // V.   DATABASE TABLE SFINAE TRAITS
    // =========================================================================
    //
    // Detect database_table-specific structural properties via SFINAE.
    // These traits complement the table detection from table_traits.hpp
    // and the database capability detection from database_traits.hpp.
    //

    NS_INTERNAL

        // detect_connection_type
        //   trait: detection operation for connection_type alias.
        template<typename _T>
        using detect_connection_type = typename _T::connection_type;

        // detect_schema_type
        //   trait: detection operation for schema_type alias.
        template<typename _T>
        using detect_schema_type = typename _T::schema_type;

        // detect_get_schema
        //   trait: detection operation for get_schema() const method.
        template<typename _T>
        using detect_get_schema =
            decltype(std::declval<const _T&>().get_schema());

        // detect_refresh
        //   trait: detection operation for refresh() method.
        template<typename _T>
        using detect_refresh = decltype(std::declval<_T&>().refresh());

        // detect_commit
        //   trait: detection operation for commit() method.
        template<typename _T>
        using detect_commit = decltype(std::declval<_T&>().commit());

        // detect_table_name
        //   trait: detection operation for table_name() method.
        template<typename _T>
        using detect_table_name =
            decltype(std::declval<const _T&>().table_name());

        // detect_is_view
        //   trait: detection operation for is_view() method.
        template<typename _T>
        using detect_is_view =
            decltype(std::declval<const _T&>().is_view());

        // detect_is_dirty
        //   trait: detection operation for is_dirty() method.
        template<typename _T>
        using detect_is_dirty =
            decltype(std::declval<const _T&>().is_dirty());

        // detect_cell_by_name
        //   trait: detection operation for cell_by_name() method.
        template<typename _T>
        using detect_cell_by_name = decltype(
            std::declval<_T&>().cell_by_name(
                std::size_t{},
                std::declval<std::string_view>()));

        // detect_dynamic_rows
        //   trait: detection operation for has_dynamic_rows constant.
        template<typename _T>
        using detect_dynamic_rows = decltype(
            std::integral_constant<bool, _T::has_dynamic_rows>{});

        // detect_rows_method (runtime)
        //   trait: detection operation for non-static rows() method.
        template<typename _T>
        using detect_rows_method_rt =
            decltype(std::declval<const _T&>().rows());

        // detect_cols_method (runtime)
        //   trait: detection operation for non-static cols() method.
        template<typename _T>
        using detect_cols_method_rt =
            decltype(std::declval<const _T&>().cols());

        // detect_cell_method_rt
        //   trait: detection operation for cell(row, col) method
        // on an instance (not static).
        template<typename _T>
        using detect_cell_method_rt = decltype(
            std::declval<_T&>().cell(std::size_t{}, std::size_t{}));

    NS_END  // internal


    // is_database_table
    //   trait: true if a type exposes the structural interface of a
    // database table (connection_type, schema_type, rows(), cols(),
    // cell(), refresh(), table_name()).
    template<typename _Type,
             typename = void>
    struct is_database_table : std::false_type
    {
    };

    // is_database_table (specialization)
    //   trait: SFINAE success case — all database table probes are
    // well-formed.
    template<typename _Type>
    struct is_database_table<_Type,
        djinterp::void_t<
            internal::detect_connection_type<_Type>,
            internal::detect_schema_type<_Type>,
            internal::detect_rows_method_rt<_Type>,
            internal::detect_cols_method_rt<_Type>,
            internal::detect_cell_method_rt<_Type>,
            internal::detect_refresh<_Type>,
            internal::detect_table_name<_Type>>>
        : std::true_type
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_database_table_v
    //   value: convenience variable template for is_database_table.
    template<typename _Type>
    constexpr bool is_database_table_v =
        is_database_table<_Type>::value;
#endif

    // has_database_sync
    //   trait: true if the type supports the synchronization interface
    // (refresh + commit + is_dirty + is_stale).
    template<typename _Type,
             typename = void>
    struct has_database_sync : std::false_type
    {
    };

    // has_database_sync (specialization)
    //   trait: SFINAE success — all sync probes well-formed.
    template<typename _Type>
    struct has_database_sync<_Type,
        djinterp::void_t<
            internal::detect_refresh<_Type>,
            internal::detect_commit<_Type>,
            internal::detect_is_dirty<_Type>>>
        : std::true_type
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_database_sync_v
    //   value: convenience variable template for has_database_sync.
    template<typename _Type>
    constexpr bool has_database_sync_v =
        has_database_sync<_Type>::value;
#endif

    // has_named_column_access
    //   trait: true if the type supports column access by name
    // (cell_by_name).
    template<typename _Type,
             typename = void>
    struct has_named_column_access : std::false_type
    {
    };

    // has_named_column_access (specialization)
    //   trait: SFINAE success — cell_by_name probe well-formed.
    template<typename _Type>
    struct has_named_column_access<_Type,
        djinterp::void_t<internal::detect_cell_by_name<_Type>>>
        : std::true_type
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_named_column_access_v
    //   value: convenience variable template for has_named_column_access.
    template<typename _Type>
    constexpr bool has_named_column_access_v =
        has_named_column_access<_Type>::value;
#endif

    // is_dynamic_table
    //   trait: true if the type has runtime-determined dimensions
    // (has_dynamic_rows constant).
    template<typename _Type,
             typename = void>
    struct is_dynamic_table : std::false_type
    {
    };

    // is_dynamic_table (specialization)
    //   trait: SFINAE success — dynamic rows probe well-formed.
    template<typename _Type>
    struct is_dynamic_table<_Type,
        djinterp::void_t<internal::detect_dynamic_rows<_Type>>>
        : std::true_type
    {
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_dynamic_table_v
    //   value: convenience variable template for is_dynamic_table.
    template<typename _Type>
    constexpr bool is_dynamic_table_v =
        is_dynamic_table<_Type>::value;
#endif


    // is_any_table
    //   trait: unifying super-trait that is true for both fixed-dimension
    // tables (detected by container::is_table_type) and dynamic database
    // tables (detected by is_database_table).  This enables generic code
    // that operates on any table-like type regardless of storage model.
    //
    // NOTE: this trait requires table.hpp to be included (for
    // container::is_table_type).  When only database_table.hpp is
    // included, the fixed-table branch is absent and is_any_table
    // reduces to is_database_table.

#ifdef DJINTERP_TABLE_
    template<typename _Type>
    struct is_any_table
        : std::integral_constant<bool,
            ( container::is_table_type<_Type>::value ||
              is_database_table<_Type>::value )>
    {
    };
#else
    template<typename _Type>
    struct is_any_table
        : is_database_table<_Type>
    {
    };
#endif

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_any_table_v
    //   value: convenience variable template for is_any_table.
    template<typename _Type>
    constexpr bool is_any_table_v = is_any_table<_Type>::value;
#endif


    // =========================================================================
    // VI.  DATABASE TABLE CLASSIFICATION
    // =========================================================================
    //
    // Aggregates all database-table-specific structural detections into a
    // single compile-time classification struct. Analogous to table_class<T>
    // for the table system, but incorporating database capabilities.
    //

    // database_table_class
    //   struct: compile-time classification of a database table type.
    // Aggregates table identity, mutability model, database capabilities,
    // and sync properties.  All members are static constexpr, determined
    // purely by structural SFINAE.
    template<typename _Type,
             bool     _IsDbTable = is_database_table<_Type>::value>
    struct database_table_class
    {
        // identity
        static constexpr bool is_database_table_type = false;
        static constexpr bool is_dynamic             = false;
        static constexpr bool has_named_access       = false;

        // sync capabilities
        static constexpr bool has_sync       = false;

        // mutability (cannot determine without the type being valid)
        static constexpr bool has_shape_modifiers_value = false;
    };

    // database_table_class (specialization)
    //   struct: classification for types that satisfy the database table
    // structural interface.
    template<typename _Type>
    struct database_table_class<_Type, true>
    {
        // identity
        static constexpr bool is_database_table_type = true;
        static constexpr bool is_dynamic =
            is_dynamic_table<_Type>::value;
        static constexpr bool has_named_access =
            has_named_column_access<_Type>::value;

        // sync capabilities
        static constexpr bool has_sync =
            has_database_sync<_Type>::value;

        // mutability — check for shape-modifier methods
        static constexpr bool has_shape_modifiers_value =
            container::has_shape_modifiers<_Type>::value;
    };


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_TABLE_
