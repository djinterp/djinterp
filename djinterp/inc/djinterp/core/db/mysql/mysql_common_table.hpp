/*******************************************************************************
* djinterp [core]                                         mysql_common_table.hpp
*
* What a table of a MariaDB or a MySQL database adds to database_table, for
* both servers: the base of mariadb_table and mysql_table.
*   mysql_common_table<Connection> is database_table<Connection> -- the
* cache, refresh, commit and the table's operations, through the MariaDB
* layer, which serves both servers through either client -- and adds the
* storage engine a create_table uses, MODIFY COLUMN, and SHOW CREATE TABLE.
* Its database is the table's schema (set_schema_name), "" for the
* connection's current one.
*   Both servers commit any open transaction before a statement that changes
* a table's definition; create_table, drop_table, truncate_table, the alter_
* members and the engine's setting cannot be rolled back.
*
*
* path:      /inc/djinterp/core/db/mysql/mysql_common_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.20
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  IDENTIFIERS
    -----------
    1.  backtick_quote
2.  MYSQL-FAMILY TABLE
    ------------------
    1.  The class
         1.  mysql_common_table
*/

#ifndef DJINTERP_DB_MYSQL_MYSQL_COMMON_TABLE_HPP
#define DJINTERP_DB_MYSQL_MYSQL_COMMON_TABLE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>  // std::find
#include <string>     // std::string
#include <utility>    // std::move
#include <vector>     // std::vector
// djinterp
#include "../../../djinterp.hpp"  // framework root
#include "../database_table.hpp"  // database_table, table_schema, sync_config
#include "./mysql_common.hpp"     // mysql_common_connection, face::mariadb


NS_DJINTERP


//==============================================================================
// 1.  IDENTIFIERS
//==============================================================================


// 1.1    backtick_quote
//------------------------------------------------------------------------------
// backtick_quote
//   function: an identifier quoted as MariaDB and MySQL quote one, in
// backticks, each backtick inside doubled.
inline std::string
backtick_quote(
    const std::string& _id
)
{
    std::string quoted;

    quoted.reserve(_id.size() + 2u);
    quoted += '`';

    // each backtick doubled
    for (const char c : _id)
    {
        if (c == '`')
        {
            quoted += '`';
        }

        quoted += c;
    }

    quoted += '`';

    return quoted;
}


//==============================================================================
// 2.  MYSQL-FAMILY TABLE
//==============================================================================


// 2.1    The class
//------------------------------------------------------------------------------
// 2.1.1
// mysql_common_table
//   class template: a table of a MariaDB or a MySQL database, as the banner
// describes, over Connection, mariadb_connection or mysql_connection. The
// engine is the server's default until set_engine names one, which
// create_table then uses for the one statement, the session's own default
// put back after. Each member throws as database_table's do. Not copyable;
// movable.
template<typename Connection,
         typename ValueType = value,
         typename Config    = void>
class mysql_common_table : public database_table<Connection, ValueType, Config>
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

    // construction -- as database_table's
    mysql_common_table()
        : base_type(),
          m_engine()
    {}

    explicit mysql_common_table(
        Connection& _conn,
        std::string _table_name,
        table_kind  _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_table_name), _kind),
          m_engine()
    {}

    explicit mysql_common_table(
        Connection&  _conn,
        table_schema _schema,
        table_kind   _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_schema), _kind),
          m_engine()
    {}

    explicit mysql_common_table(
        Connection&        _conn,
        table_schema       _schema,
        table_kind         _kind,
        const sync_config& _sync
    )
        : base_type(_conn, std::move(_schema), _kind, _sync),
          m_engine()
    {}

    ~mysql_common_table() = default;

    mysql_common_table(const mysql_common_table&)            = delete;
    mysql_common_table& operator=(const mysql_common_table&) = delete;

    mysql_common_table(mysql_common_table&&) noexcept            = default;
    mysql_common_table& operator=(mysql_common_table&&) noexcept = default;

    // engine -- the storage engine create_table uses, "" for the server's
    // default
    const std::string&
    get_engine() const noexcept
    {
        return m_engine;
    }

    void
    set_engine(
        std::string _engine
    )
    {
        m_engine = std::move(_engine);

        return;
    }

    /**
     * @brief Creates the table the schema describes, as database_table's
     *        create_table does, with the engine set_engine named.
     *
     * @param[in] _if_not_exists  whether a table already there is a success.
     * @throws as database_table's create_table; query_exception with the
     *         server's error number for an engine it does not have, and with
     *         D_DB_INVALID_ARGUMENT for one holding a NUL.
     * @post The session's default engine is what it was, whether the table
     *       was created or not.
     */
    void
    create_table(
        bool _if_not_exists = true
    )
    {
        const unsigned flags = (_if_not_exists) ? D_DB_IF_NOT_EXISTS : 0u;

        // the server's engine, unless one was named
        if (m_engine.empty())
        {
            this->create_table_with(flags);

            return;
        }

        this->validate_connected("create_table");
        internal::refuse_nul(m_engine,
                             "mysql_common_table::create_table",
                             "the engine");

        std::string previous;

        // the session's default, kept, then the engine named
        this->m_connection->ask_layer(
            "create_table",
            [&previous](const auto& _db) -> int
            {
                return _db.get_variable("default_storage_engine", previous);
            });
        m_set_default_engine(m_engine);

        try
        {
            this->create_table_with(flags);
        }
        catch (...)
        {
            // the session as it was; the first failure is the one
            try
            {
                m_set_default_engine(previous);
            }
            catch (...)
            {
            }

            throw;
        }

        m_set_default_engine(previous);

        return;
    }

    /**
     * @brief Redefines a column of the table, through the layer, as MODIFY
     *        COLUMN does, and the schema with it; a cache that was read is
     *        read again, for the values the new type gave.
     *
     * @note An attribute left out of the description -- a default, NOT NULL,
     *       AUTO_INCREMENT -- is dropped. The column keeps its place, and the
     *       keys and indexes it is part of.
     *
     * @param[in] _column  the column's new description, by its name.
     * @throws as database_table's alter_add_column; std::out_of_range for a
     *         column the schema lacks.
     */
    void
    alter_modify_column(
        const column_info& _column
    )
    {
        this->validate_connected("alter_modify_column");
        this->validate_mutable("alter_modify_column");
        this->validate_clean("alter_modify_column");

        const size_type              col  =
            this->resolve_column(_column.name, "alter_modify_column");
        const std::vector<size_type> keys = this->key_columns();
        const bool                   key  =
            (std::find(keys.begin(), keys.end(), col) != keys.end());
        const d_db_column            described =
            internal::to_layer_column(_column,
                                      key,
                                      "mysql_common_table::"
                                      "alter_modify_column");

        this->m_connection->run_layer(
            "modify_column",
            [this, &described](auto& _db) -> int
            {
                return _db.modify_column(
                    this->layer_scope(),
                    face::string_arg(this->table_name()),
                    described);
            });

        // the description, the key as it was
        this->m_schema.columns[col]                = _column;
        this->m_schema.columns[col].is_primary_key = key;

        // what the new definition made of the values
        if (this->m_loaded)
        {
            this->refresh();
        }

        return;
    }

    /**
     * @brief The statement that would create the table as it is, as SHOW
     *        CREATE TABLE writes it.
     *
     * @return the statement.
     * @throws as database_table's members do.
     */
    std::string
    show_create_table() const
    {
        this->validate_connected("show_create_table");

        std::unique_ptr<buffered_result_set> shown =
            this->m_connection->execute_query("SHOW CREATE TABLE " +
                                              this->qualified_name());

        // the statement, the row's second column
        if (shown->next())
        {
            return shown->get_string(static_cast<std::size_t>(1))
                .value_or(std::string());
        }

        return std::string();
    }

protected:
    std::string m_engine;

private:
    // m_set_default_engine -- the session's default storage engine, the
    // name given as a quoted string, which the server reads as the engine's
    void
    m_set_default_engine(
        const std::string& _engine
    )
    {
        this->m_connection->run_layer(
            "create_table",
            [&_engine](auto& _db) -> int
            {
                const std::string quoted = _db.quote_literal(_engine);

                // quoting fails only for want of memory
                if (quoted.empty())
                {
                    return D_DB_INVALID_ARGUMENT;
                }

                return _db.set_variable("default_storage_engine", quoted);
            });

        return;
    }
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MYSQL_MYSQL_COMMON_TABLE_HPP
