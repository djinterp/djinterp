/*******************************************************************************
* djinterp [core]                                              mariadb_table.hpp
*
* djinterp MariaDB table module:
*   MariaDB-specific database_table subclass providing vendor-specific
* features beyond the shared MySQL-family base, including:
*   - system-versioned table support (AS OF, BETWEEN ... AND ...)
*   - RETURNING clause for INSERT/DELETE operations
*   - MariaDB-extended data type mapping (INET6, UUID, JSON alias)
*   - invisible column detection in schema introspection
*   - Galera-aware synchronization (wsrep_sync_wait after commit)
*   - sequence-backed auto-increment alternatives
*   - MariaDB-specific storage engine selection (Aria, ColumnStore, S3)
*
*   LAYER DIAGRAM:
*     mariadb_table<Config>
*       -> mysql_common_table<mariadb_connection, value, Config>
*         -> database_table<mariadb_connection, value, Config>
*
*   All version-gated features use the mariadb_type_support and
* mariadb_feature_support compile-time structs from mariadb.hpp, which
* are backed by D_ENV_MARIADB_* macros from env_mariadb.h.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/core/db/mariadb/mariadb_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.20
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_DB_MARIADB_MARIADB_TABLE_HPP
#define DJINTERP_DB_MARIADB_MARIADB_TABLE_HPP

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../mysql/mysql_common_table.hpp"
#include "./mariadb.hpp"


NS_DJINTERP

    // ===========================================================================
    // I.   MARIADB TABLE
    // ===========================================================================

    // mariadb_table
    //   class: MariaDB-specific database table. Extends the shared
    // MySQL-family table with MariaDB vendor features. Uses
    // mariadb_connection as the concrete connection type.
    template<typename Config = void>
    class mariadb_table : public mysql_common_table<mariadb_connection,
                                                                  value,
                                                                  Config>
    {
    private:
        using base_type = mysql_common_table<mariadb_connection,
                                                           value,
                                                           Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = mariadb_table<Config>;

        using type_support    = mariadb_type_support;
        using feature_support = mariadb_feature_support;
        using version_info    = mariadb_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // mariadb_table()
        //   constructor: default - empty, disconnected table.
        mariadb_table()
            : base_type(),
              m_system_versioned(false),
              m_galera_sync_on_commit(false)
        {}

        // mariadb_table(connection, name)
        //   constructor: binds to a MariaDB connection and table name.
        explicit mariadb_table(
                mariadb_connection& _conn,
                std::string         _table_name,
                table_kind          _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_table_name),
                            _kind),
                  m_system_versioned(false),
                  m_galera_sync_on_commit(false)
        {}

        // mariadb_table(connection, schema)
        //   constructor: binds with an explicit schema.
        explicit mariadb_table(
                mariadb_connection& _conn,
                table_schema        _schema,
                table_kind          _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
                , m_system_versioned(false)
                , m_galera_sync_on_commit(false)
        {
        }

        // mariadb_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit mariadb_table(
                mariadb_connection& _conn,
                table_schema        _schema,
                table_kind          _kind,
                const sync_config&  _sync
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind,
                            _sync)
                , m_system_versioned(false)
                , m_galera_sync_on_commit(false)
        {
        }

        // non-virtual: the base is not a polymorphic type.
        ~mariadb_table() = default;

        // disable copying
        mariadb_table(const mariadb_table&)            = delete;
        mariadb_table& operator=(const mariadb_table&) = delete;

        // enable moving
        mariadb_table(mariadb_table&&) noexcept            = default;
        mariadb_table& operator=(mariadb_table&&) noexcept = default;


        // =================================================================
        //  schema introspection (MariaDB extensions)
        // =================================================================

        // fetch_schema
        //   function: extends the MySQL-family schema introspection with
        // MariaDB-specific column properties (IS_GENERATED, GENERATION_
        // EXPRESSION, visibility via EXTRA column containing "INVISIBLE").
        // Concrete (not an override): call on the concrete mariadb_table.
        void fetch_schema()
        {
            // use the base MySQL-family introspection first
            base_type::fetch_schema();

            // detect system versioning on the table
            detect_system_versioning();

            return;
        }


        // =================================================================
        //  system-versioned table support
        // =================================================================

        // is_system_versioned
        //   function: returns whether this table uses MariaDB system
        // versioning (temporal tables).
        bool is_system_versioned() const noexcept
        {
            return m_system_versioned;
        }

        // refresh_as_of
        //   function: refreshes the local cache with data as it existed
        // at the specified timestamp. Requires system versioning on the
        // table. The timestamp is a SQL expression (e.g. a quoted
        // datetime string or TRANSACTION n).
        void refresh_as_of(const std::string& _timestamp)
        {
            this->validate_connected("refresh_as_of");

            if (!m_system_versioned)
            {
                throw query_exception(
                    "mariadb_table::refresh_as_of: table is not "
                    "system-versioned.");
            }

            m_temporal_clause =
                " FOR SYSTEM_TIME AS OF " + _timestamp;
            this->invalidate();
            this->refresh();
            m_temporal_clause.clear();

            return;
        }

        // refresh_between
        //   function: refreshes the local cache with rows that were
        // valid between two timestamps. Includes rows visible at any
        // point in the range.
        void refresh_between(
                const std::string& _from,
                const std::string& _to
            )
        {
            this->validate_connected("refresh_between");

            if (!m_system_versioned)
            {
                throw query_exception(
                    "mariadb_table::refresh_between: table is not "
                    "system-versioned.");
            }

            m_temporal_clause =
                " FOR SYSTEM_TIME BETWEEN "
                + _from + " AND " + _to;
            this->invalidate();
            this->refresh();
            m_temporal_clause.clear();

            return;
        }

        // refresh_all_versions
        //   function: refreshes the local cache with all historical
        // and current row versions.
        void refresh_all_versions()
        {
            this->validate_connected("refresh_all_versions");

            if (!m_system_versioned)
            {
                throw query_exception(
                    "mariadb_table::refresh_all_versions: table is not "
                    "system-versioned.");
            }

            m_temporal_clause =
                " FOR SYSTEM_TIME ALL";
            this->invalidate();
            this->refresh();
            m_temporal_clause.clear();

            return;
        }


        // =================================================================
        //  RETURNING clause support
        // =================================================================

        // insert_row_returning
        //   function: inserts a row and returns the server-side values
        // (including generated columns, auto-increment, defaults) via
        // the MariaDB RETURNING clause. Requires MariaDB 10.5+.
        row_type insert_row_returning(const row_type& _row)
        {
            this->validate_connected("insert_row_returning");
            this->validate_mutable("insert_row_returning");

            if constexpr (!feature_support::has_returning)
            {
                throw query_exception(
                    "mariadb_table::insert_row_returning: RETURNING "
                    "clause not supported in this MariaDB version.");
            }

            // width validation
            if (_row.size() != this->m_num_cols)
            {
                throw query_exception(
                    "mariadb_table::insert_row_returning: row width "
                    "does not match column count.");
            }

            std::string query = build_insert_query(_row)
                                + " RETURNING *";

            auto rs = this->m_connection->execute_query(query);

            row_type result;

            if (rs->next())
            {
                result.reserve(this->m_num_cols);

                for (size_type c = 0; c < this->m_num_cols; ++c)
                {
                    result.push_back(
                        rs->get_value(c));
                }
            }

            // also add to local cache
            if (!result.empty())
            {
                this->m_data.push_back(result);
                ++this->m_num_rows;
            }

            return result;
        }

        // delete_returning
        //   function: deletes rows matching a WHERE clause and returns
        // the deleted rows via RETURNING.
        std::vector<row_type> delete_returning(
                const std::string& _where
            )
        {
            this->validate_connected("delete_returning");
            this->validate_mutable("delete_returning");

            if constexpr (!feature_support::has_returning)
            {
                throw query_exception(
                    "mariadb_table::delete_returning: RETURNING "
                    "clause not supported in this MariaDB version.");
            }

            std::string query =
                "DELETE FROM "
                + backtick_quote(
                      this->m_schema.table_name)
                + " WHERE " + _where
                + " RETURNING *";

            auto rs = this->m_connection->execute_query(query);

            std::vector<row_type> deleted;

            while (rs->next())
            {
                row_type row;
                row.reserve(this->m_num_cols);

                for (size_type c = 0; c < this->m_num_cols; ++c)
                {
                    row.push_back(rs->get_value(c));
                }

                deleted.push_back(std::move(row));
            }

            // invalidate local cache since rows were removed
            this->invalidate();

            return deleted;
        }


        // =================================================================
        //  Galera-aware synchronization
        // =================================================================

        // set_galera_sync_on_commit
        //   function: when enabled, issues SET wsrep_sync_wait = 1
        // before each refresh after a commit to ensure Galera cluster
        // nodes are synchronized.
        void set_galera_sync_on_commit(bool _enabled) noexcept
        {
            m_galera_sync_on_commit = _enabled;

            return;
        }

        // is_galera_sync_on_commit
        //   function: returns whether Galera sync-on-commit is enabled.
        bool is_galera_sync_on_commit() const noexcept
        {
            return m_galera_sync_on_commit;
        }

        // commit
        //   function: concrete MariaDB commit. Shadows the MySQL-family
        // commit (mysql_common_table::commit, itself concrete) and folds in
        // a Galera cluster sync when configured. Called on the concrete
        // mariadb_table.
        void commit()
        {
            base_type::commit();

            if ( (m_galera_sync_on_commit) &&
                 (this->is_connected()) )
            {
                try
                {
                    this->m_connection->execute(
                        "SET wsrep_sync_wait = 1");
                }
                catch (...)
                {
                    // non-fatal: sync hint may not be available if
                    // not running under Galera
                }
            }

            return;
        }


        // =================================================================
        //  compile-time feature queries
        // =================================================================

        // has_returning_support
        //   function: returns whether the RETURNING clause is available.
        static constexpr bool has_returning_support() noexcept
        {
            return feature_support::has_returning;
        }

        // has_system_versioning_support
        //   function: returns whether system-versioned tables are
        // supported.
        static constexpr bool has_system_versioning_support() noexcept
        {
            return feature_support::has_system_versioned_tables;
        }

        // has_sequences_support
        //   function: returns whether CREATE SEQUENCE is available.
        static constexpr bool has_sequences_support() noexcept
        {
            return feature_support::has_sequences;
        }

        // has_inet6_type_support
        //   function: returns whether the INET6 data type is available.
        static constexpr bool has_inet6_type_support() noexcept
        {
            return type_support::has_inet6_type;
        }

        // has_uuid_type_support
        //   function: returns whether the native UUID type is available.
        static constexpr bool has_uuid_type_support() noexcept
        {
            return type_support::has_uuid_type;
        }


    protected:

        // =================================================================
        //  protected helpers (concrete — not overrides)
        // =================================================================

        // build_select_query
        //   function: the MySQL-family SELECT with a temporal clause (FOR
        // SYSTEM_TIME ...) appended for system-versioned tables. Concrete
        // (not an override): the concrete base is non-polymorphic, so the
        // temporal SELECT is routed through the concrete refresh() below.
        // Identifier quoting matches quote_identifier(name,
        // database_type::mariadb).
        std::string build_select_query() const
        {
            std::string query =
                "SELECT * FROM "
                + backtick_quote(
                      this->m_schema.table_name);

            // temporal clause (FOR SYSTEM_TIME ...)
            if (!m_temporal_clause.empty())
            {
                query += m_temporal_clause;
            }

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
        //   function: concrete MariaDB refresh that reloads the local cache
        // through the temporal-aware SELECT above. Shadows base::refresh()
        // for direct calls on the concrete mariadb_table; with no temporal
        // clause set the emitted SQL is identical to the base path, in which
        // case it simply defers to the base implementation.
        void refresh()
        {
            if (m_temporal_clause.empty())
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

        // NOTE: the former field_type_to_sql and map_mysql_data_type
        // overrides were removed. MariaDB's forward spellings (LONGTEXT for
        // JSON, native UUID on 10.7+) are produced by
        // mysql_common_table::field_type_to_sql from
        // mariadb_connection::type_support (has_json_type,
        // has_uuid_type) at compile time; the reverse names ("uuid",
        // "inet6") are recognised directly by the shared
        // mysql_common_table::map_mysql_data_type. No per-leaf override is
        // needed for either direction.


        // =================================================================
        //  protected helpers
        // =================================================================

        // detect_system_versioning
        //   function: queries INFORMATION_SCHEMA to determine if this
        // table uses system versioning.
        void detect_system_versioning()
        {
            if (!this->is_connected())
            {
                return;
            }

            try
            {
                auto rs = this->m_connection->execute_query(
                    "SELECT TABLE_NAME"
                    " FROM INFORMATION_SCHEMA.TABLES"
                    " WHERE TABLE_SCHEMA = DATABASE()"
                    " AND TABLE_NAME = '"
                    + this->m_schema.table_name + "'"
                    " AND TABLE_TYPE = 'SYSTEM VERSIONED'");

                m_system_versioned = rs->next();
            }
            catch (...)
            {
                // older MariaDB or non-versioned table
                m_system_versioned = false;
            }

            return;
        }

        // build_insert_query
        //   function: constructs a single-row INSERT statement.
        std::string build_insert_query(const row_type& _row) const
        {
            std::string query =
                "INSERT INTO "
                + backtick_quote(
                      this->m_schema.table_name)
                + " (";

            for (size_type c = 0; c < this->m_num_cols; ++c)
            {
                if (c > 0)
                {
                    query += ", ";
                }

                query += backtick_quote(
                    this->m_schema.columns[c].name);
            }

            query += ") VALUES (";

            for (size_type c = 0; c < this->m_num_cols; ++c)
            {
                if (c > 0)
                {
                    query += ", ";
                }

                query += value_to_string(_row[c]);
            }

            query += ")";

            return query;
        }


        // =================================================================
        //  protected members
        // =================================================================

        bool        m_system_versioned;
        bool        m_galera_sync_on_commit;
        std::string m_temporal_clause;
    };


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MARIADB_MARIADB_TABLE_HPP
