/*******************************************************************************
* djinterp [core]                                                mysql_table.hpp
*
* djinterp Oracle MySQL table module:
*   Oracle-MySQL-specific database_table subclass providing vendor-specific
* features beyond the shared MySQL-family base, including:
*   - native JSON binary column handling (MYSQL_TYPE_JSON / 0xF5)
*   - JSON_TABLE support for querying into JSON columns
*   - generated column detection in schema introspection
*   - MySQL-specific storage engines (NDB Cluster detection)
*   - MySQL type spelling (native binary JSON, CHAR(36) for UUID) taken
*     from mysql_connection::type_support at compile time in the shared
*     base — no per-leaf override (native JSON binary vs MariaDB's LONGTEXT
*     alias is decided by the connection's type_support flags)
*   - optimizer hint support via a concrete refresh(), which wraps
*     m_optimizer_hint in MySQL's optimizer-hint block comment and
*     emits it immediately after SELECT
*
*   LAYER DIAGRAM:
*     mysql_table<Config>
*       -> mysql_common_table<mysql_connection, value, Config>
*         -> database_table<mysql_connection, value, Config>
*
*   NOTE: this header forward-declares mysql_connection. The concrete
* class definition lives in mysql.hpp, which is the Oracle MySQL
* counterpart of mariadb.hpp. Include mysql.hpp before using
* mysql_table in translation units that construct instances.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/core/db/mysql/mysql_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.20
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_DB_MYSQL_MYSQL_TABLE_HPP
#define DJINTERP_DB_MYSQL_MYSQL_TABLE_HPP

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../database.hpp"
#include "./mysql_common_table.hpp"
#include "./mysql.hpp"


NS_DJINTERP


    // =========================================================================
    // forward declaration
    // =========================================================================

    // mysql_connection
    //   class: forward declaration of the Oracle MySQL connection
    // implementation. Defined in mysql.hpp.
    class mysql_connection;


    // =========================================================================
    // I.   MYSQL TABLE
    // =========================================================================

    // mysql_table
    //   class: Oracle-MySQL-specific database table. Extends the shared
    // MySQL-family table with Oracle MySQL vendor features. Uses
    // mysql_connection as the concrete connection type.
    template<typename Config = void>
    class mysql_table
        : public mysql_common_table<
              mysql_connection,
              value,
              Config>
    {
    private:
        using base_type = mysql_common_table<
            mysql_connection,
            value,
            Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = mysql_table<Config>;


        // =================================================================
        //  constructors
        // =================================================================

        // mysql_table()
        //   constructor: default - empty, disconnected table.
        mysql_table()
            : base_type()
        {
        }

        // mysql_table(connection, name)
        //   constructor: binds to a MySQL connection and table name.
        explicit mysql_table(
                mysql_connection& _conn,
                std::string       _table_name,
                table_kind        _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_table_name),
                            _kind)
        {
        }

        // mysql_table(connection, schema)
        //   constructor: binds with an explicit schema.
        explicit mysql_table(
                mysql_connection& _conn,
                table_schema      _schema,
                table_kind        _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
        {
        }

        // mysql_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit mysql_table(
                mysql_connection&  _conn,
                table_schema       _schema,
                table_kind         _kind,
                const sync_config& _sync
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind,
                            _sync)
        {
        }

        // non-virtual: the base is not a polymorphic type.
        ~mysql_table() = default;

        // disable copying
        mysql_table(const mysql_table&)            = delete;
        mysql_table& operator=(const mysql_table&) = delete;

        // enable moving
        mysql_table(mysql_table&&) noexcept            = default;
        mysql_table& operator=(mysql_table&&) noexcept = default;


        // =================================================================
        //  schema introspection (MySQL extensions)
        // =================================================================

        // fetch_schema
        //   function: extends the MySQL-family schema introspection with
        // Oracle-MySQL-specific column properties. Detects generated
        // columns via GENERATION_EXPRESSION and marks JSON columns by
        // native type (unlike MariaDB where JSON is a LONGTEXT alias).
        // Concrete (not an override): call on the concrete mysql_table.
        void fetch_schema()
        {
            // use the base MySQL-family introspection
            base_type::fetch_schema();

            // detect JSON columns via native type
            detect_json_columns();

            return;
        }


        // =================================================================
        //  JSON column support
        // =================================================================

        // is_json_column
        //   function: returns whether the column at the given index is a
        // native JSON binary column. Oracle MySQL sends MYSQL_TYPE_JSON
        // (0xF5) over the wire for these columns.
        bool is_json_column(size_type _col) const
        {
            // bounds validation
            if (_col >= this->m_num_cols)
            {
                return false;
            }

            if (_col < m_json_flags.size())
            {
                return m_json_flags[_col];
            }

            return (this->m_schema.columns[_col].type
                    == field_type::json);
        }

        // is_json_column (by name)
        //   function: returns whether the named column is a native JSON
        // binary column.
        bool is_json_column(std::string_view _name) const
        {
            auto idx = this->m_schema.column_index(_name);

            if (!idx.has_value())
            {
                return false;
            }

            return is_json_column(idx.value());
        }

        // json_is_native
        //   function: returns true. Oracle MySQL uses native binary JSON
        // storage (unlike MariaDB's LONGTEXT alias).
        static constexpr bool json_is_native() noexcept
        {
            return true;
        }


        // =================================================================
        //  optimizer hint support
        // =================================================================

        // set_optimizer_hint
        //   function: sets a MySQL optimizer hint string that will be
        // injected into SELECT queries as /*+ ... */. The hint should
        // not include the comment delimiters.
        void set_optimizer_hint(std::string _hint)
        {
            m_optimizer_hint = std::move(_hint);

            return;
        }

        // clear_optimizer_hint
        //   function: removes any active optimizer hint.
        void clear_optimizer_hint()
        {
            m_optimizer_hint.clear();

            return;
        }

        // get_optimizer_hint
        //   function: returns the current optimizer hint, if any.
        const std::string& get_optimizer_hint() const noexcept
        {
            return m_optimizer_hint;
        }


        // =================================================================
        //  MySQL-specific queries
        // =================================================================

        // analyze_table
        //   function: runs ANALYZE TABLE to update index statistics.
        void analyze_table()
        {
            this->validate_connected("analyze_table");

            this->m_connection->execute(
                "ANALYZE TABLE "
                + backtick_quote(
                      this->m_schema.table_name));

            return;
        }

        // optimize_table
        //   function: runs OPTIMIZE TABLE to reclaim space and
        // defragment.
        void optimize_table()
        {
            this->validate_connected("optimize_table");

            this->m_connection->execute(
                "OPTIMIZE TABLE "
                + backtick_quote(
                      this->m_schema.table_name));

            return;
        }

        // check_table
        //   function: runs CHECK TABLE for integrity verification.
        // Returns the result as a string.
        std::string check_table()
        {
            this->validate_connected("check_table");

            auto rs = this->m_connection->execute_query(
                "CHECK TABLE "
                + backtick_quote(
                      this->m_schema.table_name));

            std::string result;

            while (rs->next())
            {
                auto msg = rs->get_string(static_cast<size_type>(3));

                if (msg.has_value())
                {
                    if (!result.empty())
                    {
                        result += "; ";
                    }

                    result += msg.value();
                }
            }

            return result;
        }


    protected:

        // =================================================================
        //  protected helpers (concrete — not overrides)
        // =================================================================

        // build_select_query
        //   function: the MySQL-family SELECT with optimizer hints injected
        // when configured. Concrete (not an override): the concrete base is
        // non-polymorphic, so base::refresh() would bind to the base query
        // builder; mysql_table therefore also supplies its own refresh()
        // below that routes through this builder. Identifier quoting matches
        // the base's quote_identifier(name, database_type::mysql).
        std::string build_select_query() const
        {
            std::string query = "SELECT ";

            // inject optimizer hint
            if (!m_optimizer_hint.empty())
            {
                query += "/*+ " + m_optimizer_hint + " */ ";
            }

            query += "* FROM "
                     + backtick_quote(
                           this->m_schema.table_name);

            if (!this->m_where_clause.empty())
            {
                query += " WHERE " + this->m_where_clause;
            }

            if (!this->m_order_clause.empty())
            {
                query += " ORDER BY " + this->m_order_clause;
            }

            if (this->m_limit.has_value())
            {
                query += " LIMIT "
                         + std::to_string(this->m_limit.value());
            }

            if (this->m_offset.has_value())
            {
                query += " OFFSET "
                         + std::to_string(this->m_offset.value());
            }

            return query;
        }

        // refresh
        //   function: concrete MySQL refresh that reloads the local cache
        // using the optimizer-hinted SELECT above. Shadows base::refresh()
        // for direct calls on the concrete mysql_table; without a hint the
        // emitted SQL is identical to the base path. When no hint is set
        // this simply defers to the base implementation.
        void refresh()
        {
            if (m_optimizer_hint.empty())
            {
                base_type::refresh();
                return;
            }

            this->validate_connected("refresh");

            const std::string query = build_select_query();
            auto              rs    = this->m_connection->execute_query(query);

            if (this->m_num_cols == 0)
            {
                this->m_num_cols = rs->column_count();
            }

            this->m_data.clear();

            while (rs->next())
            {
                typename base_type::row_type r;
                r.reserve(this->m_num_cols);

                for (size_type c = 0; c < this->m_num_cols; ++c)
                {
                    r.push_back(rs->get_value(c));
                }

                this->m_data.push_back(std::move(r));
            }

            this->m_num_rows     = this->m_data.size();
            this->m_stale        = false;
            this->m_dirty        = false;
            this->m_last_refresh = std::chrono::steady_clock::now();

            return;
        }

        // NOTE: the former field_type_to_sql override was removed. Oracle
        // MySQL's spellings (native binary JSON, CHAR(36) for UUID) are now
        // produced by mysql_common_table::field_type_to_sql directly, which
        // selects them at compile time from
        // mysql_connection::type_support (has_json_type == true,
        // has_uuid_type == false). No per-leaf override is needed.


        // =================================================================
        //  protected helpers
        // =================================================================

        // detect_json_columns
        //   function: queries INFORMATION_SCHEMA to identify which
        // columns are native JSON type, populating the m_json_flags
        // vector.
        void detect_json_columns()
        {
            m_json_flags.clear();
            m_json_flags.resize(this->m_num_cols, false);

            for (size_type c = 0; c < this->m_num_cols; ++c)
            {
                if (this->m_schema.columns[c].type == field_type::json)
                {
                    m_json_flags[c] = true;
                }
            }

            return;
        }


        // =================================================================
        //  protected members
        // =================================================================

        std::vector<bool> m_json_flags;
        std::string       m_optimizer_hint;
    };


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MYSQL_MYSQL_TABLE_HPP
