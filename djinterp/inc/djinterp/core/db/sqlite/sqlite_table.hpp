/******************************************************************************
* djinterp [database]                                           sqlite_table.hpp
*
* djinterp SQLite table module:
*   SQLite-specific database_table subclass providing vendor features
* beyond the generic database_table base, including:
*   - SQLite type-affinity mappings (INTEGER / TEXT / REAL / BLOB / NUMERIC)
*   - WITHOUT ROWID table support (since 3.8.2)
*   - STRICT table support (since 3.37.0)
*   - cross-database qualification via ATTACH aliases
*   - identifier quoting via double quotes or square brackets
*   - VACUUM and ANALYZE maintenance
*   - PRAGMA-based per-table configuration
*
*   LAYER DIAGRAM:
*     sqlite_table<_Config>
*       -> database_table<sqlite_connection, value, _Config>
*
*   NOTE: SQLite uses dynamic typing via type affinities rather than
* strict column types. This class emits affinity names in CREATE TABLE
* statements, and the type-support struct reflects SQLite 3.x behavior.
*
*   NOTE: this header forward-declares sqlite_connection. The concrete
* class definition lives in sqlite.hpp. Include sqlite.hpp before
* constructing instances.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
* path:      /inc/djinterp/core/db/sqlite/sqlite_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.23
******************************************************************************/

#ifndef DJINTERP_DATABASE_SQLITE_TABLE_
#define DJINTERP_DATABASE_SQLITE_TABLE_

// djinterp
#include "../../../djinterp.hpp"
#include "./sqlite.hpp"
#include "../database_table.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   SQLITE IDENTIFIER QUOTING
    // =========================================================================

    // sqlite_quote
    //   function: wraps an identifier in double quotes (the SQL-standard,
    // SQLite-preferred form). Escapes embedded double quotes by doubling
    // them.
    inline std::string sqlite_quote(const std::string& _id)
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
    // II.  SQLITE TABLE
    // =========================================================================

    // sqlite_connection
    //   class: forward declaration of the SQLite connection
    // implementation. Defined in sqlite.hpp.
    class sqlite_connection;

    // sqlite_table
    //   class: SQLite-specific database table. Extends the generic
    // database_table with SQLite DDL (WITHOUT ROWID, STRICT), affinity-
    // based typing, ATTACH qualification, and PRAGMA helpers.
    template<typename _Config = void>
    class sqlite_table
        : public database_table<sqlite_connection,
                                value,
                                _Config>
    {
    private:
        using base_type = database_table<sqlite_connection,
                                         value,
                                         _Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = sqlite_table<_Config>;

        using type_support = sqlite_type_support;
        using version_info = sqlite_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // sqlite_table()
        //   constructor: default - empty, disconnected table.
        sqlite_table()
            : base_type()
            , m_without_rowid(false)
            , m_strict(false)
        {
        }

        // sqlite_table(connection, name)
        //   constructor: binds to a SQLite connection and table name.
        explicit sqlite_table(
                sqlite_connection& _conn,
                std::string        _table_name,
                table_kind         _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_table_name),
                            _kind)
                , m_without_rowid(false)
                , m_strict(false)
        {
        }

        // sqlite_table(connection, schema)
        //   constructor: binds with an explicit schema.
        explicit sqlite_table(
                sqlite_connection& _conn,
                table_schema       _schema,
                table_kind         _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
                , m_without_rowid(false)
                , m_strict(false)
        {
        }

        // sqlite_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit sqlite_table(
                sqlite_connection& _conn,
                table_schema       _schema,
                table_kind         _kind,
                const sync_config& _sync
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind,
                            _sync)
                , m_without_rowid(false)
                , m_strict(false)
        {
        }

        ~sqlite_table() override = default;

        // disable copying
        sqlite_table(const sqlite_table&)            = delete;
        sqlite_table& operator=(const sqlite_table&) = delete;

        // enable moving
        sqlite_table(sqlite_table&&) noexcept            = default;
        sqlite_table& operator=(sqlite_table&&) noexcept = default;


        // =================================================================
        //  ATTACH-alias qualification
        // =================================================================

        // set_attach_alias
        //   function: sets the ATTACH alias under which this table lives.
        // Empty alias (default) means the main database.
        void set_attach_alias(std::string _alias)
        {
            m_attach_alias = std::move(_alias);

            return;
        }

        // get_attach_alias
        //   function: returns the configured ATTACH alias, or empty
        // string for the main database.
        const std::string& get_attach_alias() const noexcept
        {
            return m_attach_alias;
        }

        // qualified_table_name
        //   function: returns the attach-qualified, quoted table
        // identifier suitable for use in generated SQL.
        std::string qualified_table_name() const
        {
            if (m_attach_alias.empty())
            {
                return sqlite_quote(this->m_schema.table_name);
            }

            return sqlite_quote(m_attach_alias)
                 + "."
                 + sqlite_quote(this->m_schema.table_name);
        }


        // =================================================================
        //  SQLite DDL modifiers
        // =================================================================

        // set_without_rowid
        //   function: marks the table as WITHOUT ROWID. Applied on the
        // next CREATE TABLE. Requires a non-auto-increment PRIMARY KEY.
        // Available in SQLite 3.8.2+.
        void set_without_rowid(bool _enabled) noexcept
        {
            m_without_rowid = _enabled;

            return;
        }

        // is_without_rowid
        //   function: returns whether the WITHOUT ROWID modifier is
        // configured for this table.
        bool is_without_rowid() const noexcept
        {
            return m_without_rowid;
        }

        // set_strict
        //   function: marks the table as STRICT. Applied on the next
        // CREATE TABLE. Enforces declared column types instead of type
        // affinity. Available in SQLite 3.37.0+.
        void set_strict(bool _enabled) noexcept
        {
            m_strict = _enabled;

            return;
        }

        // is_strict
        //   function: returns whether the STRICT modifier is configured.
        bool is_strict() const noexcept
        {
            return m_strict;
        }


        // =================================================================
        //  SQLite maintenance operations
        // =================================================================

        // analyze_table
        //   function: runs ANALYZE on this table to update the
        // sqlite_stat tables used by the query planner.
        void analyze_table()
        {
            this->validate_connected("analyze_table");

            this->m_connection->execute(
                "ANALYZE " + qualified_table_name());

            return;
        }

        // vacuum_database
        //   function: runs VACUUM on the entire database (SQLite VACUUM
        // is a database-level operation, not a table-level one).
        void vacuum_database()
        {
            this->validate_connected("vacuum_database");

            this->m_connection->execute("VACUUM");

            return;
        }

        // reindex_table
        //   function: rebuilds all indexes associated with the table.
        void reindex_table()
        {
            this->validate_connected("reindex_table");

            this->m_connection->execute(
                "REINDEX " + qualified_table_name());

            return;
        }


    protected:

        // =================================================================
        //  protected overrides
        // =================================================================

        // field_type_to_sql
        //   function: overrides type mapping for SQLite. Emits type
        // affinity names rather than strict type names, since SQLite
        // uses dynamic typing. When STRICT is enabled, only the five
        // native types (INTEGER, REAL, TEXT, BLOB, ANY) are permitted.
        const char* field_type_to_sql(field_type _type) const override
        {
            switch (_type)
            {
                case field_type::boolean:
                    // SQLite has no native boolean; stored as INTEGER
                    return "INTEGER";
                case field_type::integer:
                case field_type::big_integer:
                    // SQLite INTEGER is variable-width up to 8 bytes
                    return "INTEGER";
                case field_type::floating_point:
                    return "REAL";
                case field_type::decimal:
                    // no fixed-point in SQLite; NUMERIC affinity is the
                    // closest semantic match
                    return m_strict ? "ANY" : "NUMERIC";
                case field_type::string:
                    return "TEXT";
                case field_type::binary:
                    return "BLOB";
                case field_type::date:
                case field_type::time:
                case field_type::datetime:
                case field_type::timestamp:
                    // SQLite stores dates as TEXT, REAL (Julian), or
                    // INTEGER (unix epoch); TEXT is the default.
                    return "TEXT";
                case field_type::json:
                    // JSON1 extension stores as TEXT
                    return "TEXT";
                case field_type::xml:
                    return "TEXT";
                case field_type::uuid:
                    // no native UUID; stored as TEXT (36 chars) or BLOB
                    // (16 bytes). TEXT is the more portable default.
                    return "TEXT";
                case field_type::array:
                    // no native array; JSON-as-TEXT is the idiomatic
                    // SQLite representation
                    return "TEXT";
                case field_type::null:
                case field_type::custom:
                default:
                    return base_type::field_type_to_sql(_type);
            }
        }


        // =================================================================
        //  protected members
        // =================================================================

        std::string m_attach_alias;
        bool        m_without_rowid;
        bool        m_strict;
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_SQLITE_TABLE_
