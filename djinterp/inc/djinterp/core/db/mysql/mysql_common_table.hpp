/*******************************************************************************
* djinterp [core]                                         mysql_common_table.hpp
*
* djinterp MySQL-family common table module:
*   Shared database_table subclass for the MySQL-compatible family (Oracle
* MySQL and MariaDB). Sits between the generic database_table<> and the
* concrete vendor-specific table classes (mysql_table, mariadb_table).
*
*   Provides:
*   - MySQL-dialect SQL generation (backtick identifier quoting,
*     LIMIT/OFFSET syntax, ON DUPLICATE KEY UPDATE)
*   - INFORMATION_SCHEMA-based schema introspection
*   - DDL operations (CREATE TABLE, DROP TABLE, ALTER TABLE, TRUNCATE)
*   - commit logic using INSERT ... ON DUPLICATE KEY UPDATE for upsert
*   - SHOW CREATE TABLE / SHOW COLUMNS wrappers
*   - storage engine selection
*
*   LAYER DIAGRAM:
*     mysql_table / mariadb_table  (vendor-specific)
*       -> mysql_common_table<Connection, ValueType, Config>
*         -> database_table<Connection, ValueType, Config>
*
*   DESIGN NOTE — NO VIRTUAL OVERRIDES (updated for the concrete base):
*   The database_table base is now a CONCRETE, non-polymorphic template
* ("templates everywhere, virtual nowhere") — no `virtual` members, no
* override hooks — and every leaf is used through its own concrete type,
* never through a database_table& / database_table*. This module was
* rewritten to match:
*     - it no longer overrides base methods. The base already emits
*       dialect-correct SELECT / INSERT through quote_identifier(name,
*       db_type) + dialect_format_limit_offset(db_type, ...), so the old
*       backtick-quoting build_select_query override was redundant and has
*       been removed (exactly as db2_table carries no such override);
*     - the richer INFORMATION_SCHEMA introspection (fetch_schema), the
*       existence probe (exists), and the MySQL upsert commit are now
*       CONCRETE members that shadow the base for direct calls on the
*       concrete leaf — no `override`, no `virtual`;
*     - the destructor is non-virtual, matching the non-polymorphic base;
*     - vendor JSON / UUID spelling differences (native JSON binary vs the
*       MariaDB LONGTEXT alias; native UUID vs CHAR(36)) are resolved at
*       COMPILE TIME from connection_type::type_support (has_json_type /
*       has_uuid_type). field_type_to_sql therefore needs neither virtual
*       dispatch nor a per-leaf override: the shared DDL picks the right
*       vendor spelling from the connection template argument, the same way
*       db2_table defers to db2_connection's type mapping. The two entry
*       points mysql_table / mariadb_table used to override
*       (field_type_to_sql, map_mysql_data_type) are consequently plain
*       concrete helpers; vendor leaves add NEW concrete behaviour instead.
*
*   PORTABILITY:
*   Requires C++17 or later. Does not include the MySQL C API header.
*
*
* path:      /inc/djinterp/core/db/mysql/mysql_common_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.20
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_DB_MYSQL_MYSQL_COMMON_TABLE_HPP
#define DJINTERP_DB_MYSQL_MYSQL_COMMON_TABLE_HPP

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../database_table.hpp"
#include "./mysql_common.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   MYSQL-FAMILY TYPE-SUPPORT PROBES
    // =========================================================================

    namespace internal
    {
        // mysql_ts_has_uuid_type
        //   trait: reads TypeSupport::has_uuid_type when the member exists,
        // and is false otherwise. Oracle MySQL's mysql_type_support does not
        // declare has_uuid_type (it has no native UUID), whereas MariaDB's
        // mariadb_type_support does; this probe lets the shared
        // field_type_to_sql below branch uniformly without a per-vendor
        // override. (Partial specialization must live at namespace scope, so
        // it cannot be nested in the class template.)
        template<typename TypeSupport, typename = void>
        struct mysql_ts_has_uuid_type : std::false_type
        {
        };

        template<typename TypeSupport>
        struct mysql_ts_has_uuid_type<
                TypeSupport,
                std::void_t<decltype(TypeSupport::has_uuid_type)>>
            : std::bool_constant<TypeSupport::has_uuid_type>
        {
        };
    } // namespace internal


    // =========================================================================
    // I.   MYSQL IDENTIFIER QUOTING
    // =========================================================================

    // backtick_quote
    //   function: wraps an identifier in backticks for MySQL-family SQL.
    // Escapes embedded backticks by doubling them.
    inline std::string backtick_quote(const std::string& _id)
    {
        std::string result;
        result.reserve(_id.size() + 2);
        result += '`';

        for (char c : _id)
        {
            if (c == '`')
            {
                result += "``";
            }
            else
            {
                result += c;
            }
        }

        result += '`';

        return result;
    }


    // =========================================================================
    // II.  MYSQL-FAMILY COMMON TABLE
    // =========================================================================

    // mysql_common_table
    //   class template: shared MySQL-family table base. Adds concrete
    // MySQL-dialect introspection, DDL/ALTER, and upsert-commit on top of
    // the concrete database_table base (no virtual dispatch); shared by
    // both Oracle MySQL and MariaDB. Vendor JSON/UUID spelling is taken
    // from connection_type::type_support at compile time.
    template<typename Connection,
             typename ValueType = value,
             typename Config     = void>
    class mysql_common_table
        : public database_table<Connection, ValueType, Config>
    {
    private:
        using base_type = database_table<Connection, ValueType, Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = mysql_common_table<Connection, ValueType, Config>;


        // =================================================================
        //  constructors
        // =================================================================

        // mysql_common_table()
        //   constructor: default - empty, disconnected table.
        mysql_common_table()
            : base_type()
            , m_engine("InnoDB")
        {
        }

        // mysql_common_table(connection, name)
        //   constructor: binds to a connection and table name.
        explicit mysql_common_table(
                Connection&       _conn,
                std::string        _table_name,
                table_kind         _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_table_name),
                            _kind)
                , m_engine("InnoDB")
        {
        }

        // mysql_common_table(connection, schema)
        //   constructor: binds with an explicit schema.
        explicit mysql_common_table(
                Connection&  _conn,
                table_schema  _schema,
                table_kind    _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
                , m_engine("InnoDB")
        {
        }

        // mysql_common_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit mysql_common_table(
                Connection&       _conn,
                table_schema       _schema,
                table_kind         _kind,
                const sync_config& _sync
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind,
                            _sync)
                , m_engine("InnoDB")
        {
        }

        // non-virtual: the base is not a polymorphic type.
        ~mysql_common_table() = default;

        // disable copying
        mysql_common_table(const mysql_common_table&)            = delete;
        mysql_common_table& operator=(const mysql_common_table&) = delete;

        // enable moving
        mysql_common_table(mysql_common_table&&) noexcept            = default;
        mysql_common_table& operator=(mysql_common_table&&) noexcept = default;


        // =================================================================
        //  database operations (MySQL-dialect, concrete)
        // =================================================================

        // fetch_schema
        //   function: retrieves column metadata from
        // INFORMATION_SCHEMA.COLUMNS.
        // concrete: shadows the base zero-row probe with richer
        // INFORMATION_SCHEMA introspection; call on the concrete leaf.
        void fetch_schema()
        {
            this->validate_connected("fetch_schema");

            std::string query =
                "SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE,"
                " COLUMN_KEY, EXTRA, CHARACTER_MAXIMUM_LENGTH,"
                " COLUMN_DEFAULT, COLUMN_COMMENT"
                " FROM INFORMATION_SCHEMA.COLUMNS"
                " WHERE TABLE_SCHEMA = DATABASE()"
                " AND TABLE_NAME = '"
                + this->m_schema.table_name
                + "' ORDER BY ORDINAL_POSITION";

            auto rs = this->m_connection->execute_query(query);

            this->m_schema.columns.clear();
            this->m_schema.primary_key_columns.clear();

            while (rs->next())
            {
                column_info ci;

                ci.name = rs->get_string(static_cast<size_type>(0))
                              .value_or("");
                ci.type = map_mysql_data_type(
                    rs->get_string(static_cast<size_type>(1))
                        .value_or("varchar"));
                ci.nullable =
                    (rs->get_string(static_cast<size_type>(2))
                         .value_or("YES") == "YES");

                std::string key =
                    rs->get_string(static_cast<size_type>(3))
                        .value_or("");
                ci.is_primary_key    = (key == "PRI");
                ci.is_unique         = ( (key == "PRI") ||
                                         (key == "UNI") );
                ci.is_indexed        = (!key.empty());

                std::string extra =
                    rs->get_string(static_cast<size_type>(4))
                        .value_or("");
                ci.is_auto_increment =
                    (extra.find("auto_increment") != std::string::npos);

                auto max_len =
                    rs->get_long(static_cast<size_type>(5));
                if (max_len.has_value())
                {
                    ci.max_length = static_cast<size_type>(
                        max_len.value());
                }

                ci.default_value =
                    rs->get_string(static_cast<size_type>(6));
                ci.comment =
                    rs->get_string(static_cast<size_type>(7));

                if (ci.is_primary_key)
                {
                    this->m_schema.primary_key_columns.push_back(
                        ci.name);
                }

                this->m_schema.columns.push_back(std::move(ci));
            }

            this->m_num_cols = this->m_schema.columns.size();

            return;
        }

        // exists
        //   function: checks table existence via INFORMATION_SCHEMA.
        // concrete: shadows the base existence probe.
        bool exists() const
        {
            this->validate_connected("exists");

            std::string query =
                "SELECT 1 FROM INFORMATION_SCHEMA.TABLES"
                " WHERE TABLE_SCHEMA = DATABASE()"
                " AND TABLE_NAME = '"
                + this->m_schema.table_name + "'";

            try
            {
                auto rs = this->m_connection->execute_query(query);

                return rs->next();
            }
            catch (...)
            {
                return false;
            }
        }

        // commit
        //   function: concrete MySQL-family commit. Shadows the base
        // full-replace commit and writes the local cache back inside a
        // transaction, using INSERT ... ON DUPLICATE KEY UPDATE when the
        // schema declares primary keys and a full DELETE + INSERT otherwise.
        // (Called on the concrete leaf; mariadb_table shadows this again to
        // fold in a Galera sync wait.)
        void commit()
        {
            this->validate_connected("commit");
            this->validate_mutable("commit");

            if (!this->m_dirty)
            {
                return;
            }

            transaction<Connection> txn(*this->m_connection);

            try
            {
                if (!this->m_schema.primary_key_columns.empty())
                {
                    commit_upsert();
                }
                else
                {
                    commit_replace();
                }

                txn.commit();
                this->m_dirty = false;
            }
            catch (...)
            {
                txn.rollback();
                throw;
            }

            return;
        }


        // =================================================================
        //  DDL operations
        // =================================================================

        // create_table
        //   function: generates and executes a CREATE TABLE statement
        // from the current schema. Uses the configured storage engine.
        void create_table(bool _if_not_exists = true)
        {
            this->validate_connected("create_table");

            std::string query = "CREATE TABLE ";

            if (_if_not_exists)
            {
                query += "IF NOT EXISTS ";
            }

            query += backtick_quote(this->m_schema.table_name)
                     + " (";

            // column definitions
            bool first = true;

            for (const auto& col : this->m_schema.columns)
            {
                if (!first)
                {
                    query += ", ";
                }

                first = false;
                query += backtick_quote(col.name) + " "
                         + field_type_to_sql(col.type);

                if (col.max_length.has_value())
                {
                    query += "(" + std::to_string(col.max_length.value())
                             + ")";
                }

                if (!col.nullable)
                {
                    query += " NOT NULL";
                }

                if (col.is_auto_increment)
                {
                    query += " AUTO_INCREMENT";
                }

                if (col.default_value.has_value())
                {
                    query += " DEFAULT " + col.default_value.value();
                }
            }

            // primary key constraint
            if (!this->m_schema.primary_key_columns.empty())
            {
                query += ", PRIMARY KEY (";
                bool pk_first = true;

                for (const auto& pk : this->m_schema.primary_key_columns)
                {
                    if (!pk_first)
                    {
                        query += ", ";
                    }

                    pk_first = false;
                    query += backtick_quote(pk);
                }

                query += ")";
            }

            query += ") ENGINE=" + m_engine;

            this->m_connection->execute(query);

            return;
        }

        // drop_table
        //   function: generates and executes a DROP TABLE statement.
        void drop_table(bool _if_exists = true)
        {
            this->validate_connected("drop_table");

            std::string query = "DROP TABLE ";

            if (_if_exists)
            {
                query += "IF EXISTS ";
            }

            query += backtick_quote(this->m_schema.table_name);

            this->m_connection->execute(query);

            return;
        }

        // truncate_table
        //   function: truncates all data from the table.
        void truncate_table()
        {
            this->validate_connected("truncate_table");

            this->m_connection->execute(
                "TRUNCATE TABLE "
                + backtick_quote(this->m_schema.table_name));

            // sync local cache
            this->m_data.clear();
            this->m_num_rows = 0;
            this->m_dirty    = false;

            return;
        }

        // alter_add_column
        //   function: adds a column to the database table via ALTER
        // TABLE and updates the local schema.
        void alter_add_column(const column_info& _col_info)
        {
            this->validate_connected("alter_add_column");
            this->validate_mutable("alter_add_column");

            std::string query =
                "ALTER TABLE "
                + backtick_quote(this->m_schema.table_name)
                + " ADD COLUMN "
                + backtick_quote(_col_info.name) + " "
                + field_type_to_sql(_col_info.type);

            if (_col_info.max_length.has_value())
            {
                query += "("
                         + std::to_string(_col_info.max_length.value())
                         + ")";
            }

            if (!_col_info.nullable)
            {
                query += " NOT NULL";
            }

            if (_col_info.default_value.has_value())
            {
                query += " DEFAULT "
                         + _col_info.default_value.value();
            }

            this->m_connection->execute(query);

            // update local schema and extend cached rows
            this->add_column(_col_info);

            return;
        }

        // alter_drop_column
        //   function: drops a column from the database table via ALTER
        // TABLE and updates the local schema.
        void alter_drop_column(const std::string& _column_name)
        {
            this->validate_connected("alter_drop_column");
            this->validate_mutable("alter_drop_column");

            this->m_connection->execute(
                "ALTER TABLE "
                + backtick_quote(this->m_schema.table_name)
                + " DROP COLUMN "
                + backtick_quote(_column_name));

            // update local schema and shrink cached rows
            this->remove_column(_column_name);

            return;
        }

        // alter_modify_column
        //   function: modifies a column definition via ALTER TABLE
        // MODIFY COLUMN.
        void alter_modify_column(const column_info& _col_info)
        {
            this->validate_connected("alter_modify_column");
            this->validate_mutable("alter_modify_column");

            std::string query =
                "ALTER TABLE "
                + backtick_quote(this->m_schema.table_name)
                + " MODIFY COLUMN "
                + backtick_quote(_col_info.name) + " "
                + field_type_to_sql(_col_info.type);

            if (_col_info.max_length.has_value())
            {
                query += "("
                         + std::to_string(_col_info.max_length.value())
                         + ")";
            }

            if (!_col_info.nullable)
            {
                query += " NOT NULL";
            }
            else
            {
                query += " NULL";
            }

            if (_col_info.default_value.has_value())
            {
                query += " DEFAULT "
                         + _col_info.default_value.value();
            }

            this->m_connection->execute(query);

            return;
        }

        // show_create_table
        //   function: returns the SHOW CREATE TABLE output as a string.
        std::string show_create_table() const
        {
            this->validate_connected("show_create_table");

            auto rs = this->m_connection->execute_query(
                "SHOW CREATE TABLE "
                + backtick_quote(this->m_schema.table_name));

            if (rs->next())
            {
                return rs->get_string(static_cast<size_type>(1))
                           .value_or("");
            }

            return "";
        }


        // =================================================================
        //  engine management
        // =================================================================

        // get_engine
        //   function: returns the configured storage engine name.
        const std::string& get_engine() const noexcept
        {
            return m_engine;
        }

        // set_engine
        //   function: sets the storage engine for CREATE TABLE.
        void set_engine(std::string _engine)
        {
            m_engine = std::move(_engine);

            return;
        }


    protected:

        // =================================================================
        //  protected helpers (concrete — not overrides)
        // =================================================================

        // (build_select_query removed.) The concrete base already emits a
        // dialect-correct SELECT: for the MySQL family quote_identifier(name,
        // db_type) yields backtick quoting and dialect_format_limit_offset()
        // yields LIMIT/OFFSET — identical to the old hand-rolled override —
        // so base::refresh() is already MySQL-correct. mysql_table adds the
        // optimizer-hint SELECT as its own concrete refresh() where wanted.

        // field_type_to_sql
        //   function: maps a field_type to MySQL-family SQL type syntax.
        // Concrete (not virtual): the two spellings that differ across the
        // family — JSON (native binary on Oracle MySQL vs a LONGTEXT alias
        // on MariaDB) and UUID (native on MariaDB 10.7+ vs CHAR(36)) — are
        // selected at compile time from connection_type::type_support, so
        // the shared DDL below produces the right vendor spelling without a
        // per-leaf override.
        const char* field_type_to_sql(field_type _type) const
        {
            // Use the template parameter directly: connection_type is only
            // aliased in the dependent base, so an unqualified name would not
            // be found by phase-1 lookup here.
            using ts = typename Connection::type_support;

            switch (_type)
            {
                case field_type::json:
                    // native binary JSON (Oracle MySQL) vs LONGTEXT alias
                    // (MariaDB) — chosen at compile time from type_support.
                    return ts::has_json_type ? "JSON" : "LONGTEXT";
                case field_type::uuid:
                    // native UUID (MariaDB 10.7+) vs CHAR(36); has_uuid_type
                    // is absent on Oracle MySQL, so probe it absent-safely.
                    return internal::mysql_ts_has_uuid_type<ts>::value
                               ? "UUID"
                               : "CHAR(36)";
                default:
                    break;
            }

            return field_type_to_sql_common(_type);
        }

        // field_type_to_sql_common
        //   function: the family-invariant part of the type mapping (every
        // spelling that does not vary between Oracle MySQL and MariaDB).
        static const char* field_type_to_sql_common(field_type _type)
        {
            switch (_type)
            {
                case field_type::null:           return "TEXT";
                case field_type::boolean:        return "TINYINT(1)";
                case field_type::integer:        return "INT";
                case field_type::big_integer:    return "BIGINT";
                case field_type::floating_point: return "DOUBLE";
                case field_type::decimal:        return "DECIMAL";
                case field_type::string:         return "VARCHAR(255)";
                case field_type::binary:         return "BLOB";
                case field_type::date:           return "DATE";
                case field_type::time:           return "TIME";
                case field_type::datetime:       return "DATETIME";
                case field_type::timestamp:      return "TIMESTAMP";
                case field_type::xml:            return "LONGTEXT";
                // json / uuid are family-variant and handled by the caller
                // (field_type_to_sql) from connection_type::type_support.
                case field_type::json:
                case field_type::uuid:
                case field_type::array:          return "JSON";
                case field_type::custom:
                default:                         return "BLOB";
            }
        }

        // map_mysql_data_type
        //   function: maps an INFORMATION_SCHEMA DATA_TYPE string to
        // field_type. Concrete (not virtual); the family variations are
        // additive and handled by vendor leaves' own concrete introspection
        // (e.g. mysql_table::detect_json_columns).
        field_type map_mysql_data_type(
                const std::string& _type_name
            ) const
        {
            if ( (_type_name == "tinyint") ||
                 (_type_name == "bool")    ||
                 (_type_name == "boolean") )
            {
                return field_type::boolean;
            }

            if ( (_type_name == "smallint")  ||
                 (_type_name == "mediumint") ||
                 (_type_name == "int")       ||
                 (_type_name == "integer")   ||
                 (_type_name == "year") )
            {
                return field_type::integer;
            }

            if (_type_name == "bigint")
            {
                return field_type::big_integer;
            }

            if ( (_type_name == "float") ||
                 (_type_name == "double") )
            {
                return field_type::floating_point;
            }

            if ( (_type_name == "decimal")  ||
                 (_type_name == "numeric") )
            {
                return field_type::decimal;
            }

            if ( (_type_name == "char")       ||
                 (_type_name == "varchar")    ||
                 (_type_name == "tinytext")   ||
                 (_type_name == "text")       ||
                 (_type_name == "mediumtext") ||
                 (_type_name == "longtext")   ||
                 (_type_name == "enum")       ||
                 (_type_name == "set") )
            {
                return field_type::string;
            }

            if ( (_type_name == "binary")     ||
                 (_type_name == "varbinary")  ||
                 (_type_name == "tinyblob")   ||
                 (_type_name == "blob")       ||
                 (_type_name == "mediumblob") ||
                 (_type_name == "longblob")   ||
                 (_type_name == "bit") )
            {
                return field_type::binary;
            }

            if (_type_name == "date")
            {
                return field_type::date;
            }

            if (_type_name == "time")
            {
                return field_type::time;
            }

            if (_type_name == "datetime")
            {
                return field_type::datetime;
            }

            if (_type_name == "timestamp")
            {
                return field_type::timestamp;
            }

            if (_type_name == "json")
            {
                return field_type::json;
            }

            // family-union names (MariaDB emits these; Oracle MySQL never
            // does, so recognising them unconditionally in the shared base
            // is safe and removes the need for a vendor-leaf override).
            if (_type_name == "uuid")
            {
                return field_type::uuid;
            }

            if (_type_name == "inet6")
            {
                return field_type::string;
            }

            return field_type::custom;
        }


        // =================================================================
        //  protected members
        // =================================================================

        std::string m_engine;


    private:

        // =================================================================
        //  commit strategies
        // =================================================================

        // commit_upsert
        //   function: uses INSERT ... ON DUPLICATE KEY UPDATE for tables
        // with primary keys.
        void commit_upsert()
        {
            for (size_type r = 0; r < this->m_num_rows; ++r)
            {
                std::string query =
                    "INSERT INTO "
                    + backtick_quote(this->m_schema.table_name)
                    + " (";

                // column list
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

                // value list
                for (size_type c = 0; c < this->m_num_cols; ++c)
                {
                    if (c > 0)
                    {
                        query += ", ";
                    }

                    query += value_to_sql(this->m_data[r][c]);
                }

                query += ") ON DUPLICATE KEY UPDATE ";

                // update non-pk columns
                bool first = true;

                for (size_type c = 0; c < this->m_num_cols; ++c)
                {
                    if (this->m_schema.columns[c].is_primary_key)
                    {
                        continue;
                    }

                    if (!first)
                    {
                        query += ", ";
                    }

                    first = false;
                    std::string col_name = backtick_quote(
                        this->m_schema.columns[c].name);
                    query += col_name + " = VALUES("
                             + col_name + ")";
                }

                this->m_connection->execute(query);
            }

            return;
        }

        // commit_replace
        //   function: full DELETE + INSERT replacement for tables without
        // primary keys.
        void commit_replace()
        {
            // delete all existing rows
            std::string where_clause;

            if (!this->m_where_clause.empty())
            {
                where_clause = " WHERE " + this->m_where_clause;
            }

            this->m_connection->execute(
                "DELETE FROM "
                + backtick_quote(this->m_schema.table_name)
                + where_clause);

            // batch insert
            if (this->m_num_rows == 0)
            {
                return;
            }

            for (size_type r = 0; r < this->m_num_rows; ++r)
            {
                std::string query =
                    "INSERT INTO "
                    + backtick_quote(this->m_schema.table_name)
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

                    query += value_to_sql(this->m_data[r][c]);
                }

                query += ")";

                this->m_connection->execute(query);
            }

            return;
        }

        // value_to_sql
        //   function: converts a db::value variant to a SQL literal
        // string. Uses the connection's escape_string when available.
        std::string value_to_sql(const value_type& _val) const
        {
            return value_to_string(_val);
        }
    };


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MYSQL_MYSQL_COMMON_TABLE_HPP
