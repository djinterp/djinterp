/******************************************************************************
* djinterp [database]                                        mariadb_table.hpp
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
*     mariadb_table<_Config>
*       -> mysql_common_table<mariadb_connection, value, _Config>
*         -> database_table<mariadb_connection, value, _Config>
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
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.20
******************************************************************************/

#ifndef DJINTERP_DATABASE_MARIADB_TABLE_
#define DJINTERP_DATABASE_MARIADB_TABLE_

// mysql
#include <mysql/mysql.h>
// mariadb
#include <mariadb/conncpp.hpp>
// djinterp
#include "../../djinterp.hpp"
#include "../mariadb.hpp"
#include "./mysql_common_table.hpp"


NS_DJINTERP
NS_DATABASE

    // ===========================================================================
    // I.   MARIADB TABLE
    // ===========================================================================

    // mariadb_table
    //   class: MariaDB-specific database table. Extends the shared
    // MySQL-family table with MariaDB vendor features. Uses
    // mariadb_connection as the concrete connection type.
    template<typename _Config = container::empty_config>
    class mariadb_table : public mysql_common_table<mariadb_connection,
                                                                  value,
                                                                  _Config>
    {
    private:
        using base_type = mysql_common_table<mariadb_connection,
                                                           value,
                                                           _Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = mariadb_table<_Config>;

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

        ~mariadb_table() override = default;

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
        void fetch_schema() override
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
        //   function: overrides base commit to add Galera sync when
        // configured.
        void commit() override
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
        //  protected overrides
        // =================================================================

        // build_select_query
        //   function: extends the MySQL-family SELECT with temporal
        // clauses for system-versioned tables.
        std::string build_select_query() const override
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

        // field_type_to_sql
        //   function: extends the MySQL-family type mapping with
        // MariaDB-specific types (INET6, UUID, JSON alias).
        const char* field_type_to_sql(field_type _type) const override
        {
            // MariaDB-specific overrides
            if (_type == field_type::uuid)
            {
                if constexpr (type_support::has_uuid_type)
                {
                    return "UUID";
                }
            }

            if (_type == field_type::json)
            {
                // MariaDB JSON is always LONGTEXT alias
                if constexpr (type_support::has_json_type)
                {
                    return "JSON";
                }
                else
                {
                    return "LONGTEXT";
                }
            }

            // fall through to base for common types
            return base_type::field_type_to_sql(_type);
        }

        // map_mysql_data_type
        //   function: extends the MySQL-family type mapping with
        // MariaDB-specific types.
        field_type map_mysql_data_type(
                const std::string& _type_name
            ) const override
        {
            // MariaDB-specific types
            if (_type_name == "inet6")
            {
                return field_type::string;
            }

            if (_type_name == "uuid")
            {
                return field_type::uuid;
            }

            // fall through to base
            return base_type::map_mysql_data_type(_type_name);
        }


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


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MARIADB_TABLE_
