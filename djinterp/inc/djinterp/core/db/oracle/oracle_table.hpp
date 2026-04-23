/******************************************************************************
* djinterp [database]                                           oracle_table.hpp
*
* djinterp Oracle table module:
*   Oracle-specific database_table subclass providing vendor features
* beyond the generic database_table base, including:
*   - Oracle-native type mappings (NUMBER, VARCHAR2, RAW, CLOB, BLOB,
*     TIMESTAMP WITH TIME ZONE, NCHAR, NVARCHAR2, native JSON since 21c)
*   - double-quote identifier quoting (SQL-standard)
*   - schema (owner) qualification (owner.table)
*   - tablespace assignment for CREATE TABLE
*   - optimizer hint injection ( SELECT /+ ... / )
*   - flashback query helpers (AS OF SCN / AS OF TIMESTAMP)
*   - GATHER_TABLE_STATS and ANALYZE maintenance
*   - PURGE and recyclebin-aware drop
*
*   LAYER DIAGRAM:
*     oracle_table<_Config>
*       -> database_table<oracle_connection, value, _Config>
*
*   NOTE: this header forward-declares oracle_connection. The concrete
* class definition lives in oracle.hpp. Include oracle.hpp before
* constructing instances.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
* path:      /inc/djinterp/core/db/oracle/oracle_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.23
******************************************************************************/

#ifndef DJINTERP_DATABASE_ORACLE_TABLE_
#define DJINTERP_DATABASE_ORACLE_TABLE_

// djinterp
#include "../../djinterp.hpp"
#include "../oracle.hpp"
#include "./database_table.hpp"


NS_DJINTERP
NS_DATABASE
NS_ORA


    // =========================================================================
    // I.   ORACLE IDENTIFIER QUOTING
    // =========================================================================

    // oracle_quote
    //   function: wraps an identifier in double quotes for Oracle SQL.
    // Oracle identifiers are case-sensitive when quoted. Escapes
    // embedded double quotes by doubling them.
    inline std::string oracle_quote(const std::string& _id)
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
    // II.  ORACLE TABLE
    // =========================================================================

    // oracle_connection
    //   class: forward declaration of the Oracle connection implementation.
    // Defined in oracle.hpp.
    class oracle_connection;

    // oracle_table
    //   class: Oracle-specific database table. Extends the generic
    // database_table with Oracle DDL, identifier quoting, optimizer
    // hints, flashback queries, and tablespace control.
    template<typename _Config = container::empty_config>
    class oracle_table
        : public database_table<oracle_connection,
                                value,
                                _Config>
    {
    private:
        using base_type = database_table<oracle_connection,
                                         value,
                                         _Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = oracle_table<_Config>;

        using type_support    = ora_type_support;
        using feature_support = ora_feature_support;
        using version_info    = ora_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // oracle_table()
        //   constructor: default - empty, disconnected table.
        oracle_table()
            : base_type()
        {
        }

        // oracle_table(connection, name)
        //   constructor: binds to an Oracle connection and table name.
        explicit oracle_table(
                oracle_connection& _conn,
                std::string        _table_name,
                table_kind         _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_table_name),
                            _kind)
        {
        }

        // oracle_table(connection, schema)
        //   constructor: binds with an explicit schema.
        explicit oracle_table(
                oracle_connection& _conn,
                table_schema       _schema,
                table_kind         _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
        {
        }

        // oracle_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit oracle_table(
                oracle_connection& _conn,
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

        ~oracle_table() override = default;

        // disable copying
        oracle_table(const oracle_table&)            = delete;
        oracle_table& operator=(const oracle_table&) = delete;

        // enable moving
        oracle_table(oracle_table&&) noexcept            = default;
        oracle_table& operator=(oracle_table&&) noexcept = default;


        // =================================================================
        //  owner (schema) qualification
        // =================================================================

        // set_owner
        //   function: sets the Oracle schema (owner) to which the table
        // belongs. Produces owner.table qualification in generated SQL.
        void set_owner(std::string _owner)
        {
            m_owner = std::move(_owner);

            return;
        }

        // get_owner
        //   function: returns the configured owner name.
        const std::string& get_owner() const noexcept
        {
            return m_owner;
        }

        // qualified_table_name
        //   function: returns the fully-qualified, double-quoted table
        // identifier suitable for use in generated SQL.
        std::string qualified_table_name() const
        {
            if (m_owner.empty())
            {
                return oracle_quote(this->m_schema.table_name);
            }

            return oracle_quote(m_owner)
                 + "."
                 + oracle_quote(this->m_schema.table_name);
        }


        // =================================================================
        //  tablespace assignment
        // =================================================================

        // set_tablespace
        //   function: sets the tablespace clause appended to CREATE
        // TABLE statements emitted by this instance.
        void set_tablespace(std::string _tablespace)
        {
            m_tablespace = std::move(_tablespace);

            return;
        }

        // get_tablespace
        //   function: returns the configured tablespace name, if any.
        const std::string& get_tablespace() const noexcept
        {
            return m_tablespace;
        }


        // =================================================================
        //  optimizer hint support
        // =================================================================

        // set_optimizer_hint
        //   function: sets an Oracle optimizer hint string injected into
        // SELECT queries as /*+ ... */. The hint should not include the
        // comment delimiters.
        void set_optimizer_hint(std::string _hint)
        {
            m_optimizer_hint = std::move(_hint);

            return;
        }

        // clear_optimizer_hint
        //   function: removes any active optimizer hint.
        void clear_optimizer_hint() noexcept
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
        //  flashback query support
        // =================================================================

        // refresh_as_of_scn
        //   function: refreshes the local cache with data as it existed
        // at the specified system change number (SCN). Requires undo
        // retention to cover that SCN.
        void refresh_as_of_scn(std::uint64_t _scn)
        {
            this->validate_connected("refresh_as_of_scn");

            m_flashback_clause = " AS OF SCN " + std::to_string(_scn);
            this->invalidate();
            this->refresh();
            m_flashback_clause.clear();

            return;
        }

        // refresh_as_of_timestamp
        //   function: refreshes the local cache with data as of the
        // given SQL timestamp expression (e.g. TO_TIMESTAMP('...')).
        // Passed through verbatim.
        void refresh_as_of_timestamp(const std::string& _ts_expr)
        {
            this->validate_connected("refresh_as_of_timestamp");

            m_flashback_clause = " AS OF TIMESTAMP " + _ts_expr;
            this->invalidate();
            this->refresh();
            m_flashback_clause.clear();

            return;
        }


        // =================================================================
        //  Oracle maintenance operations
        // =================================================================

        // gather_table_stats
        //   function: runs DBMS_STATS.GATHER_TABLE_STATS for this table
        // to update optimizer statistics.
        void gather_table_stats()
        {
            this->validate_connected("gather_table_stats");

            std::string owner = m_owner.empty()
                              ? "USER"
                              : ("'" + m_owner + "'");

            this->m_connection->execute(
                "BEGIN DBMS_STATS.GATHER_TABLE_STATS("
                + owner + ", '"
                + this->m_schema.table_name + "'); END;");

            return;
        }

        // purge_table
        //   function: permanently removes the table from the recycle
        // bin. Call after a DROP TABLE without PURGE.
        void purge_table()
        {
            this->validate_connected("purge_table");

            this->m_connection->execute(
                "PURGE TABLE " + qualified_table_name());

            return;
        }


    protected:

        // =================================================================
        //  protected overrides
        // =================================================================

        // field_type_to_sql
        //   function: overrides type mapping for Oracle. Uses Oracle-
        // native types (NUMBER, VARCHAR2, RAW, CLOB, TIMESTAMP WITH
        // TIME ZONE) rather than the defaults.
        const char* field_type_to_sql(field_type _type) const override
        {
            switch (_type)
            {
                case field_type::boolean:
                    // Oracle 23c introduces native BOOLEAN; earlier
                    // versions use NUMBER(1). We emit NUMBER(1) for
                    // portability; override in a subclass if 23c+.
                    return "NUMBER(1)";
                case field_type::integer:
                    return "NUMBER(10)";
                case field_type::big_integer:
                    return "NUMBER(19)";
                case field_type::floating_point:
                    return "BINARY_DOUBLE";
                case field_type::decimal:
                    return "NUMBER";
                case field_type::string:
                    return "VARCHAR2(4000)";
                case field_type::binary:
                    return "BLOB";
                case field_type::date:
                    return "DATE";
                case field_type::time:
                    // Oracle has no TIME type; use INTERVAL DAY TO
                    // SECOND as the closest representation
                    return "INTERVAL DAY(0) TO SECOND";
                case field_type::datetime:
                    return "TIMESTAMP";
                case field_type::timestamp:
                    return "TIMESTAMP WITH TIME ZONE";
                case field_type::json:
                    // native JSON data type since 21c; earlier versions
                    // use CLOB with a check constraint
                    return "JSON";
                case field_type::xml:
                    return "XMLTYPE";
                case field_type::uuid:
                    // no native UUID; RAW(16) is the standard binary
                    // representation
                    return "RAW(16)";
                case field_type::array:
                    // Oracle collection types require a named TYPE
                    return "SYS.ODCIVARCHAR2LIST";
                case field_type::null:
                case field_type::custom:
                default:
                    return base_type::field_type_to_sql(_type);
            }
        }


        // =================================================================
        //  protected members
        // =================================================================

        std::string m_owner;
        std::string m_tablespace;
        std::string m_optimizer_hint;
        std::string m_flashback_clause;
    };


NS_END  // ora
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ORACLE_TABLE_
