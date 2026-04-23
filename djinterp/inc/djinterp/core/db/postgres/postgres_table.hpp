/******************************************************************************
* djinterp [database]                                         postgres_table.hpp
*
* djinterp PostgreSQL table module:
*   PostgreSQL-specific database_table subclass providing vendor features
* beyond the generic database_table base, including:
*   - double-quote identifier quoting (SQL-standard, vs MySQL backticks)
*   - PostgreSQL-native type mappings (JSONB, native UUID, INET, arrays,
*     SERIAL, BYTEA, NUMERIC)
*   - COPY protocol hints for bulk refresh / commit acceleration
*   - VACUUM and ANALYZE maintenance operations
*   - RETURNING clause helpers for INSERT / UPDATE / DELETE round-trips
*   - schema-qualified table names (schema.table)
*   - LISTEN / NOTIFY invalidation hook for cache staleness
*
*   LAYER DIAGRAM:
*     postgres_table<_Config>
*       -> database_table<pg_connection, value, _Config>
*
*   NOTE: this header forward-declares pg_connection. The concrete class
* definition lives in postgres.hpp. Include postgres.hpp before
* constructing instances.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
* path:      /inc/djinterp/core/db/postgres/postgres_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.23
******************************************************************************/

#ifndef DJINTERP_DATABASE_POSTGRES_TABLE_
#define DJINTERP_DATABASE_POSTGRES_TABLE_

// djinterp
#include "../../djinterp.hpp"
#include "../postgres.hpp"
#include "./database_table.hpp"


NS_DJINTERP
NS_DATABASE


    // =========================================================================
    // I.   POSTGRESQL IDENTIFIER QUOTING
    // =========================================================================

    // double_quote
    //   function: wraps an identifier in double quotes for PostgreSQL /
    // SQL-standard identifier quoting. Escapes embedded double quotes by
    // doubling them.
    inline std::string double_quote(const std::string& _id)
    {
        std::string result;
        result.reserve(_id.size() + 2);
        result += '"';

        for (char c : _id)
        {
            if (c == '"')
            {
                result += "\"\"";
            }
            else
            {
                result += c;
            }
        }

        result += '"';

        return result;
    }


    // =========================================================================
    // II.  POSTGRES TABLE
    // =========================================================================

    // pg_connection
    //   class: forward declaration of the PostgreSQL connection
    // implementation. Defined in postgres.hpp.
    class pg_connection;

    // postgres_table
    //   class: PostgreSQL-specific database table. Extends the generic
    // database_table with PostgreSQL DDL, identifier quoting, COPY
    // acceleration, and VACUUM / ANALYZE maintenance.
    template<typename _Config = container::empty_config>
    class postgres_table
        : public database_table<pg_connection,
                                value,
                                _Config>
    {
    private:
        using base_type = database_table<pg_connection,
                                         value,
                                         _Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = postgres_table<_Config>;

        using type_support    = pg_type_support;
        using feature_support = pg_feature_support;
        using version_info    = pg_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // postgres_table()
        //   constructor: default - empty, disconnected table.
        postgres_table()
            : base_type()
            , m_use_copy_on_refresh(false)
            , m_use_copy_on_commit(false)
        {
        }

        // postgres_table(connection, name)
        //   constructor: binds to a PostgreSQL connection and table name.
        explicit postgres_table(
                pg_connection& _conn,
                std::string    _table_name,
                table_kind     _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_table_name),
                            _kind)
                , m_use_copy_on_refresh(false)
                , m_use_copy_on_commit(false)
        {
        }

        // postgres_table(connection, schema)
        //   constructor: binds with an explicit schema.
        explicit postgres_table(
                pg_connection& _conn,
                table_schema   _schema,
                table_kind     _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
                , m_use_copy_on_refresh(false)
                , m_use_copy_on_commit(false)
        {
        }

        // postgres_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit postgres_table(
                pg_connection&     _conn,
                table_schema       _schema,
                table_kind         _kind,
                const sync_config& _sync
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind,
                            _sync)
                , m_use_copy_on_refresh(false)
                , m_use_copy_on_commit(false)
        {
        }

        ~postgres_table() override = default;

        // disable copying
        postgres_table(const postgres_table&)            = delete;
        postgres_table& operator=(const postgres_table&) = delete;

        // enable moving
        postgres_table(postgres_table&&) noexcept            = default;
        postgres_table& operator=(postgres_table&&) noexcept = default;


        // =================================================================
        //  schema qualification
        // =================================================================

        // set_schema_name
        //   function: sets the PostgreSQL schema (namespace) to which the
        // table belongs. Produces schema.table qualification in generated
        // SQL.
        void set_schema_name(std::string _schema_name)
        {
            m_schema_name = std::move(_schema_name);

            return;
        }

        // get_schema_name
        //   function: returns the configured schema name, or empty string
        // if unqualified (defaulting to the search_path).
        const std::string& get_schema_name() const noexcept
        {
            return m_schema_name;
        }

        // qualified_table_name
        //   function: returns the fully-qualified, double-quoted table
        // identifier suitable for use in generated SQL.
        std::string qualified_table_name() const
        {
            if (m_schema_name.empty())
            {
                return double_quote(this->m_schema.table_name);
            }

            return double_quote(m_schema_name)
                 + "."
                 + double_quote(this->m_schema.table_name);
        }


        // =================================================================
        //  COPY protocol acceleration
        // =================================================================

        // set_use_copy_on_refresh
        //   function: enables use of the COPY protocol for bulk refresh,
        // which is significantly faster than row-by-row SELECT for large
        // tables. Requires the connection to support COPY.
        void set_use_copy_on_refresh(bool _enabled) noexcept
        {
            m_use_copy_on_refresh = _enabled;

            return;
        }

        // set_use_copy_on_commit
        //   function: enables use of COPY FROM STDIN for commit of new
        // rows. Much faster than batched INSERT for large change sets.
        void set_use_copy_on_commit(bool _enabled) noexcept
        {
            m_use_copy_on_commit = _enabled;

            return;
        }

        // uses_copy_on_refresh
        //   function: returns whether COPY acceleration is enabled for
        // refresh.
        bool uses_copy_on_refresh() const noexcept
        {
            return m_use_copy_on_refresh;
        }

        // uses_copy_on_commit
        //   function: returns whether COPY acceleration is enabled for
        // commit.
        bool uses_copy_on_commit() const noexcept
        {
            return m_use_copy_on_commit;
        }


        // =================================================================
        //  RETURNING clause support
        // =================================================================

        // set_returning_columns
        //   function: configures a RETURNING clause to be appended to
        // INSERT / UPDATE / DELETE statements. The named columns are
        // returned to the client after the write completes, enabling
        // round-trip-free retrieval of generated values (serials,
        // defaults, trigger-computed fields).
        void set_returning_columns(std::vector<std::string> _cols)
        {
            m_returning_columns = std::move(_cols);

            return;
        }

        // clear_returning_columns
        //   function: removes any configured RETURNING clause.
        void clear_returning_columns() noexcept
        {
            m_returning_columns.clear();

            return;
        }

        // get_returning_columns
        //   function: returns the current RETURNING columns, if any.
        const std::vector<std::string>& get_returning_columns()
            const noexcept
        {
            return m_returning_columns;
        }


        // =================================================================
        //  PostgreSQL maintenance operations
        // =================================================================

        // analyze_table
        //   function: runs ANALYZE to update planner statistics for the
        // table.
        void analyze_table()
        {
            this->validate_connected("analyze_table");

            this->m_connection->execute(
                "ANALYZE " + qualified_table_name());

            return;
        }

        // vacuum_table
        //   function: runs VACUUM to reclaim storage for the table.
        void vacuum_table()
        {
            this->validate_connected("vacuum_table");

            this->m_connection->execute(
                "VACUUM " + qualified_table_name());

            return;
        }

        // vacuum_full_table
        //   function: runs VACUUM FULL to fully rewrite the table and
        // release disk space. Acquires an ACCESS EXCLUSIVE lock.
        void vacuum_full_table()
        {
            this->validate_connected("vacuum_full_table");

            this->m_connection->execute(
                "VACUUM FULL " + qualified_table_name());

            return;
        }

        // reindex_table
        //   function: rebuilds all indexes on the table.
        void reindex_table()
        {
            this->validate_connected("reindex_table");

            this->m_connection->execute(
                "REINDEX TABLE " + qualified_table_name());

            return;
        }


    protected:

        // =================================================================
        //  protected overrides
        // =================================================================

        // field_type_to_sql
        //   function: overrides type mapping for PostgreSQL. Uses
        // PostgreSQL-native types (JSONB, native UUID, BYTEA, TIMESTAMPTZ)
        // rather than the defaults.
        const char* field_type_to_sql(field_type _type) const override
        {
            switch (_type)
            {
                case field_type::boolean:
                    return "BOOLEAN";
                case field_type::integer:
                    return "INTEGER";
                case field_type::big_integer:
                    return "BIGINT";
                case field_type::floating_point:
                    return "DOUBLE PRECISION";
                case field_type::decimal:
                    return "NUMERIC";
                case field_type::string:
                    return "TEXT";
                case field_type::binary:
                    return "BYTEA";
                case field_type::date:
                    return "DATE";
                case field_type::time:
                    return "TIME";
                case field_type::datetime:
                    return "TIMESTAMP";
                case field_type::timestamp:
                    return "TIMESTAMP WITH TIME ZONE";
                case field_type::json:
                    // prefer JSONB over JSON for indexing / operator support
                    return "JSONB";
                case field_type::xml:
                    return "XML";
                case field_type::uuid:
                    // PostgreSQL has native UUID type
                    return "UUID";
                case field_type::array:
                    // base element type should be specified by caller
                    return "ANYARRAY";
                case field_type::null:
                case field_type::custom:
                default:
                    return base_type::field_type_to_sql(_type);
            }
        }


        // =================================================================
        //  protected members
        // =================================================================

        std::string              m_schema_name;
        std::vector<std::string> m_returning_columns;
        bool                     m_use_copy_on_refresh;
        bool                     m_use_copy_on_commit;
    };


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_POSTGRES_TABLE_
