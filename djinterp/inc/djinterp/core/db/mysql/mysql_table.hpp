/*******************************************************************************
* djinterp [core]                                                mysql_table.hpp
*
* A table of an Oracle MySQL database held in memory: mysql_common_table
* over a mysql_connection, with what MySQL adds.
*   mysql_table is mysql_common_table<mysql_connection> -- database_table's
* cache, refresh, commit and table operations, the engine, MODIFY COLUMN and
* SHOW CREATE TABLE -- and adds: JSON columns told apart, MySQL storing JSON
* as a type of its own; an optimizer hint written into refresh()'s query;
* and upkeep through the layer: ANALYZE TABLE, OPTIMIZE TABLE and CHECK
* TABLE.
*
*
* path:      /inc/djinterp/core/db/mysql/mysql_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.20
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  MYSQL TABLE
    -----------
    1.  The class
         1.  mysql_table
*/

#ifndef DJINTERP_DB_MYSQL_MYSQL_TABLE_HPP
#define DJINTERP_DB_MYSQL_MYSQL_TABLE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::move
// djinterp
#include "../../../djinterp.hpp"     // framework root
#include "./mysql_common_table.hpp"  // mysql_common_table
#include "./mysql.hpp"               // mysql_connection


NS_DJINTERP


//==============================================================================
// 1.  MYSQL TABLE
//==============================================================================


// 1.1    The class
//------------------------------------------------------------------------------
// 1.1.1
// mysql_table
//   class template: a table of an Oracle MySQL database, as the banner
// describes. Each member throws as database_table's do. Not copyable;
// movable.
//
// Usage:
//     mysql_table<> orders(db, "orders");
//
//     orders.fetch_schema();
//     orders.set_optimizer_hint("MAX_EXECUTION_TIME(1000)");
//     orders.refresh();                  // SELECT /*+ ... */ ...
template<typename Config = void>
class mysql_table
    : public mysql_common_table<mysql_connection, value, Config>
{
private:
    using base_type = mysql_common_table<mysql_connection, value, Config>;

public:
    using typename base_type::size_type;
    using typename base_type::value_type;
    using typename base_type::row_type;
    using typename base_type::connection_type;
    using typename base_type::schema_type;
    using self_type = mysql_table<Config>;

    // construction -- as database_table's
    mysql_table()
        : base_type()
    {}

    explicit mysql_table(
        mysql_connection& _conn,
        std::string       _table_name,
        table_kind        _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_table_name), _kind)
    {}

    explicit mysql_table(
        mysql_connection& _conn,
        table_schema      _schema,
        table_kind        _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_schema), _kind)
    {}

    explicit mysql_table(
        mysql_connection&  _conn,
        table_schema       _schema,
        table_kind         _kind,
        const sync_config& _sync
    )
        : base_type(_conn, std::move(_schema), _kind, _sync)
    {}

    ~mysql_table() = default;

    mysql_table(const mysql_table&)            = delete;
    mysql_table& operator=(const mysql_table&) = delete;

    mysql_table(mysql_table&&) noexcept            = default;
    mysql_table& operator=(mysql_table&&) noexcept = default;

    // JSON -- whether a column holds JSON, by index or by name: a column the
    // schema types field_type::json, as a query describes MySQL's JSON
    // columns; false for a column the table lacks. MySQL stores JSON as a
    // type of its own, where MariaDB's JSON is text.
    bool
    is_json_column(
        size_type _col
    ) const noexcept
    {
        return ( (_col < this->cols()) &&
                 (this->get_schema().columns[_col].type == field_type::json) );
    }

    bool
    is_json_column(
        std::string_view _name
    ) const
    {
        const std::optional<std::size_t> index =
            this->get_schema().column_index(_name);

        return ( (index.has_value()) &&
                 (is_json_column(index.value())) );
    }

    static constexpr bool
    json_is_native() noexcept
    {
        return true;
    }

    /**
     * @brief Writes an optimizer hint into refresh()'s query, after SELECT,
     *        in the comment MySQL reads hints from, whose opening mark is
     *        followed by a plus.
     *
     * @param[in] _hint  the hint, without the comment's marks, which it must
     *                   not hold.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a hint holding
     *         the comment's end, which would end it early.
     * @post The cache is stale.
     */
    void
    set_optimizer_hint(
        std::string _hint
    )
    {
        // a hint that would close its comment
        if (_hint.find("*/") != std::string::npos)
        {
            throw query_exception("mysql_table::set_optimizer_hint: the hint "
                                  "holds */, which ends its comment",
                                  D_DB_INVALID_ARGUMENT);
        }

        m_optimizer_hint    = std::move(_hint);
        this->m_select_hint = (m_optimizer_hint.empty())
                                  ? std::string()
                                  : "/*+ " + m_optimizer_hint + " */";
        this->invalidate();

        return;
    }

    void
    clear_optimizer_hint()
    {
        m_optimizer_hint.clear();
        this->m_select_hint.clear();
        this->invalidate();

        return;
    }

    const std::string&
    get_optimizer_hint() const noexcept
    {
        return m_optimizer_hint;
    }

    // upkeep -- through the layer: ANALYZE TABLE, for the optimizer's key
    // distribution; OPTIMIZE TABLE, which rebuilds the table; CHECK TABLE,
    // which says whether it is sound. Each throws as database_table's
    // members do, D_DB_NOT_FOUND for a table not there.
    void
    analyze_table()
    {
        this->validate_connected("analyze_table");

        this->m_connection->run_layer(
            "analyze_table",
            [this](face::mariadb& _db) -> int
            {
                return _db.analyze_table(this->layer_scope(),
                                         face::string_arg(this->table_name()));
            });

        return;
    }

    void
    optimize_table()
    {
        this->validate_connected("optimize_table");

        this->m_connection->run_layer(
            "optimize_table",
            [this](face::mariadb& _db) -> int
            {
                return _db.optimize_table(
                    this->layer_scope(),
                    face::string_arg(this->table_name()));
            });

        return;
    }

    bool
    check_table() const
    {
        this->validate_connected("check_table");

        bool sound = false;

        this->m_connection->ask_layer(
            "check_table",
            [this, &sound](const face::mariadb& _db) -> int
            {
                return _db.check_table(this->layer_scope(),
                                       face::string_arg(this->table_name()),
                                       sound);
            });

        return sound;
    }

protected:
    std::string m_optimizer_hint;
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MYSQL_MYSQL_TABLE_HPP
