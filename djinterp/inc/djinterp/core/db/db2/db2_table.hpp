/*******************************************************************************
* djinterp [core]                                                  db2_table.hpp
*
* djinterp IBM Db2 table module:
*   Db2-specific database_table subclass adding vendor features on top of
* the generic relational database_table base, including:
*   - schema-qualified, SQL-standard double-quoted table naming
*   - RUNSTATS statistics maintenance (via SYSPROC.ADMIN_CMD)
*   - REORG table / index reorganization (via SYSPROC.ADMIN_CMD)
*   - SET INTEGRITY check-pending resolution
*   - MERGE upsert convenience forwarding to the connection
*   - system-period temporal querying (FOR SYSTEM_TIME AS OF), gated by
*     server feature availability
*
*   Db2 is a full relational SQL database, so — unlike the recent
* non-relational additions (redis_table / dynamodb_table /
* cassandra_table, which are standalone) — this wrapper is a genuine
* subclass of the shared relational database_table template, exactly as
* postgres_table and mariadb_table are.
*
*   LAYER DIAGRAM:
*     db2_table<Config>
*       -> database_table<db2_connection, value, Config>
*
*   DESIGN NOTE — NO VIRTUAL OVERRIDES:
*   The current database_table base is a CONCRETE, non-polymorphic
* template: it declares no `virtual` members and exposes no override
* hooks ("templates everywhere, virtual nowhere"). Vendor variation
* flows through (a) the db2_connection template argument, which carries
* the dialect via get_database_type(), and (b) the dialect-aware free
* helpers (quote_identifier, dialect_format_limit_offset) that switch on
* database_type. This subclass therefore ADDS new, concrete Db2-specific
* methods rather than overriding base behaviour, and its destructor is
* non-virtual to match the base. Db2 SQL-type name mapping lives on
* db2_connection (field_type_to_db2_native), not as a table override.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/core/db/db2/db2_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.28
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_DB_DB2_DB2_TABLE_HPP
#define DJINTERP_DB_DB2_DB2_TABLE_HPP

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::int64_t

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "./db2.hpp"
#include "../database_table.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   DB2 TABLE
    // =========================================================================

    // db2_table
    //   class: IBM Db2-specific database table. Extends the generic
    // database_table with schema qualification, RUNSTATS / REORG
    // maintenance, SET INTEGRITY, MERGE upsert, and temporal querying.
    template<typename Config = void>
    class db2_table
        : public database_table<db2_connection,
                                value,
                                Config>
    {
    private:
        using base_type = database_table<db2_connection,
                                         value,
                                         Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = db2_table<Config>;

        using type_support    = db2_type_support;
        using feature_support = db2_feature_support;
        using version_info    = db2_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // db2_table()
        //   constructor: default - empty, disconnected table.
        db2_table()
            : base_type()
        {
        }

        // db2_table(connection, name)
        //   constructor: binds to a Db2 connection and table name.
        explicit db2_table(
                db2_connection& _conn,
                std::string     _table_name,
                table_kind      _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_table_name),
                            _kind)
        {
        }

        // db2_table(connection, schema)
        //   constructor: binds with an explicit schema.
        explicit db2_table(
                db2_connection& _conn,
                table_schema    _schema,
                table_kind      _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
        {
        }

        // db2_table(connection, schema, kind, sync)
        //   constructor: binds with schema and sync policy.
        explicit db2_table(
                db2_connection&    _conn,
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

        // ~db2_table()
        //   destructor: non-virtual to match the concrete base (the base
        // is not a polymorphic type).
        ~db2_table() = default;

        // disable copying
        db2_table(const db2_table&)            = delete;
        db2_table& operator=(const db2_table&) = delete;

        // enable moving
        db2_table(db2_table&&) noexcept            = default;
        db2_table& operator=(db2_table&&) noexcept = default;


        // =================================================================
        //  schema qualification
        // =================================================================

        // set_schema_name
        //   function: sets the Db2 schema to which the table belongs.
        // Produces "SCHEMA"."TABLE" qualification in generated SQL.
        void set_schema_name(std::string _schema_name)
        {
            m_schema_name = std::move(_schema_name);

            return;
        }

        // get_schema_name
        //   function: returns the configured schema name, or empty
        // string if unqualified (defaulting to CURRENT SCHEMA).
        const std::string& get_schema_name() const noexcept
        {
            return m_schema_name;
        }

        // qualified_table_name
        //   function: returns the fully-qualified, double-quoted table
        // identifier suitable for use in generated SQL. Uses the shared
        // dialect-aware quote_identifier helper with the Db2 dialect.
        std::string qualified_table_name() const
        {
            const std::string table =
                quote_identifier(this->m_schema.table_name,
                                 database_type::db2);

            if (m_schema_name.empty())
            {
                return table;
            }

            return quote_identifier(m_schema_name, database_type::db2)
                 + "."
                 + table;
        }


        // =================================================================
        //  Db2 maintenance operations
        // =================================================================

        // runstats
        //   function: updates table and index statistics for the query
        // optimizer. Routed through SYSPROC.ADMIN_CMD, the supported way
        // to invoke RUNSTATS over an SQL connection.
        void runstats()
        {
            this->validate_connected("runstats");

            const std::string cmd =
                "RUNSTATS ON TABLE " + qualified_table_name()
                + " WITH DISTRIBUTION AND DETAILED INDEXES ALL";

            this->m_connection->execute(
                "CALL SYSPROC.ADMIN_CMD('" + escape_admin_arg(cmd) + "')");

            return;
        }

        // reorg_table
        //   function: physically reorganizes the table to reclaim space
        // and restore clustering. Routed through SYSPROC.ADMIN_CMD.
        void reorg_table()
        {
            this->validate_connected("reorg_table");

            const std::string cmd =
                "REORG TABLE " + qualified_table_name();

            this->m_connection->execute(
                "CALL SYSPROC.ADMIN_CMD('" + escape_admin_arg(cmd) + "')");

            return;
        }

        // reorg_indexes
        //   function: reorganizes all indexes defined on the table.
        // Routed through SYSPROC.ADMIN_CMD.
        void reorg_indexes()
        {
            this->validate_connected("reorg_indexes");

            const std::string cmd =
                "REORG INDEXES ALL FOR TABLE " + qualified_table_name();

            this->m_connection->execute(
                "CALL SYSPROC.ADMIN_CMD('" + escape_admin_arg(cmd) + "')");

            return;
        }

        // set_integrity_checked
        //   function: resolves a check-pending state (e.g. after a LOAD
        // or ALTER) by bringing the table back online with integrity
        // checking. Plain SQL, no ADMIN_CMD needed.
        void set_integrity_checked()
        {
            this->validate_connected("set_integrity_checked");

            this->m_connection->execute(
                "SET INTEGRITY FOR " + qualified_table_name()
                + " IMMEDIATE CHECKED");

            return;
        }


        // =================================================================
        //  MERGE upsert
        // =================================================================

        // merge_upsert
        //   function: executes a MERGE INTO statement targeting this
        // table, forwarding to the connection's merge(). The caller
        // supplies the USING / ON / WHEN clauses; this method prepends
        // the MERGE INTO <qualified table> header. Returns the number of
        // affected rows.
        re_std::int64_t merge_upsert(const std::string& _using_on_when)
        {
            this->validate_connected("merge_upsert");

            const std::string sql =
                "MERGE INTO " + qualified_table_name() + " "
                + _using_on_when;

            return this->m_connection->merge(sql);
        }


        // =================================================================
        //  temporal querying
        // =================================================================

#if D_ENV_DB2_DETECTED
    #if D_ENV_DB2_HAS_TEMPORAL_TABLES

        // build_system_time_as_of
        //   function: builds a SELECT against this table with a
        // FOR SYSTEM_TIME AS OF <timestamp> clause for system-period
        // temporal querying. Available since Db2 10.1. Returns the SQL
        // text; execute it through the connection or the base refresh
        // path with a custom WHERE as needed.
        std::string build_system_time_as_of(
            const std::string& _timestamp) const
        {
            return "SELECT * FROM " + qualified_table_name()
                 + " FOR SYSTEM_TIME AS OF "
                 + this->m_connection->escape_literal(_timestamp);
        }

    #endif  // D_ENV_DB2_HAS_TEMPORAL_TABLES
#endif  // D_ENV_DB2_DETECTED


        // =================================================================
        //  compile-time feature queries
        // =================================================================

        static constexpr bool supports_merge() noexcept
        {
            return db2_connection::supports_merge();
        }

        static constexpr bool supports_temporal_tables() noexcept
        {
            return db2_connection::supports_temporal_tables();
        }

        static constexpr bool supports_column_organized() noexcept
        {
            return db2_connection::supports_column_organized();
        }

        static constexpr bool supports_purexml() noexcept
        {
            return db2_connection::supports_purexml();
        }


    protected:

        // =================================================================
        //  protected helpers
        // =================================================================

        // escape_admin_arg
        //   helper: escapes a command string for embedding inside the
        // single-quoted SYSPROC.ADMIN_CMD argument by doubling embedded
        // single quotes.
        static std::string escape_admin_arg(const std::string& _cmd)
        {
            std::string out;
            out.reserve(_cmd.size());

            for (char c : _cmd)
            {
                if (c == '\'')
                {
                    out += "''";
                }
                else
                {
                    out += c;
                }
            }

            return out;
        }


        // =================================================================
        //  protected members
        // =================================================================

        std::string m_schema_name;
    };


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DB2_DB2_TABLE_HPP
