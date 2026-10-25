/*******************************************************************************
* djinterp [core]                                             postgres_table.hpp
*
* A table of a PostgreSQL database held in memory: database_table over a
* pg_connection, with what PostgreSQL adds.
*   postgres_table is database_table<pg_connection> -- the cache, refresh,
* commit and the table's operations, through the PostgreSQL layer -- and
* adds: the schema the table lives in; COPY, for the rows a commit inserts;
* and upkeep through the layer: VACUUM, VACUUM FULL, ANALYZE and REINDEX of
* the table.
*
*
* path:      /inc/djinterp/core/db/postgres/postgres_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.23
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  IDENTIFIERS
    -----------
    1.  double_quote
2.  POSTGRESQL TABLE
    ----------------
    1.  The class
         1.  postgres_table
*/

#ifndef DJINTERP_DB_POSTGRES_POSTGRES_TABLE_HPP
#define DJINTERP_DB_POSTGRES_POSTGRES_TABLE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>       // std::chrono::system_clock
#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <type_traits>  // std::is_same_v, std::decay_t
#include <utility>      // std::move
#include <variant>      // std::visit, std::monostate
#include <vector>       // std::vector
// djinterp
#include "../../../djinterp.hpp"  // framework root
#include "../database_table.hpp"  // database_table, table_schema, sync_config
#include "./postgres.hpp"         // pg_connection, D_POSTGRES_FULL,
                                  // D_POSTGRES_CONCURRENTLY
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::int32_t, int64_t,
                                                   // uint8_t


NS_DJINTERP


//==============================================================================
// 1.  IDENTIFIERS
//==============================================================================


// 1.1    double_quote
//------------------------------------------------------------------------------
// double_quote
//   function: an identifier quoted as PostgreSQL and the standard quote one,
// in double quotes, each double quote inside doubled.
inline std::string
double_quote(
    const std::string& _id
)
{
    std::string quoted;

    quoted.reserve(_id.size() + 2u);
    quoted += '"';

    // each double quote doubled
    for (const char c : _id)
    {
        if (c == '"')
        {
            quoted += '"';
        }

        quoted += c;
    }

    quoted += '"';

    return quoted;
}


//==============================================================================
// 2.  POSTGRESQL TABLE
//==============================================================================


// 2.1    The class
//------------------------------------------------------------------------------
// 2.1.1
// postgres_table
//   class template: a table of a PostgreSQL database, as the banner
// describes. Its schema is set_schema_name's, "" for the current one, the
// search path's first that exists. With COPY on, a commit sends the rows it
// inserts as one COPY ... FROM STDIN, rows of text, unless one of them
// leaves a value to the database -- a NULL a default or an identity fills,
// a generated column's -- which only an INSERT can: such a commit inserts
// row by row. Each member throws as database_table's do. Not copyable;
// movable.
//
// Usage:
//     postgres_table<> events(db, "events");
//
//     events.set_schema_name("audit");
//     events.fetch_schema();
//     events.set_use_copy_on_commit(true);
//     ...                                // rows added
//     events.commit();                   // one COPY
template<typename Config = void>
class postgres_table : public database_table<pg_connection, value, Config>
{
private:
    using base_type = database_table<pg_connection, value, Config>;

public:
    using typename base_type::size_type;
    using typename base_type::value_type;
    using typename base_type::row_type;
    using typename base_type::connection_type;
    using typename base_type::schema_type;
    using self_type = postgres_table<Config>;

    using client_capabilities = pg_client_capabilities;
    using version_info        = pg_version_info;

    // type_support, feature_support -- client_capabilities, by their former
    // names (2026.10.21), for one release
    using type_support [[deprecated("client_capabilities")]] =
        pg_client_capabilities;
    using feature_support [[deprecated("client_capabilities")]] =
        pg_client_capabilities;

    // construction -- as database_table's
    postgres_table()
        : base_type(),
          m_use_copy_on_commit(false)
    {}

    explicit postgres_table(
        pg_connection& _conn,
        std::string    _table_name,
        table_kind     _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_table_name), _kind),
          m_use_copy_on_commit(false)
    {}

    explicit postgres_table(
        pg_connection& _conn,
        table_schema   _schema,
        table_kind     _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_schema), _kind),
          m_use_copy_on_commit(false)
    {}

    explicit postgres_table(
        pg_connection&     _conn,
        table_schema       _schema,
        table_kind         _kind,
        const sync_config& _sync
    )
        : base_type(_conn, std::move(_schema), _kind, _sync),
          m_use_copy_on_commit(false)
    {}

    ~postgres_table() = default;

    postgres_table(const postgres_table&)            = delete;
    postgres_table& operator=(const postgres_table&) = delete;

    postgres_table(postgres_table&&) noexcept            = default;
    postgres_table& operator=(postgres_table&&) noexcept = default;

    // get_schema_name -- the table's schema, as schema_name()
    const std::string&
    get_schema_name() const noexcept
    {
        return this->schema_name();
    }

    // qualified_table_name -- the table's name for SQL, as qualified_name()
    std::string
    qualified_table_name() const
    {
        return this->qualified_name();
    }

    // COPY -- whether a commit sends the rows it inserts as one COPY
    void
    set_use_copy_on_commit(
        bool _enabled
    ) noexcept
    {
        m_use_copy_on_commit = _enabled;

        return;
    }

    bool
    uses_copy_on_commit() const noexcept
    {
        return m_use_copy_on_commit;
    }

    /**
     * @brief Writes the cache's changes to the table, as database_table's
     *        commit does, the rows it inserts sent as one COPY where COPY is
     *        on and no row leaves a value to the database.
     *
     * @throws as database_table's commit; a row the server refuses fails the
     *         COPY, with its SQLSTATE, and the whole commit is rolled back.
     */
    void
    commit()
    {
        // without COPY, as every table commits
        if (!m_use_copy_on_commit)
        {
            base_type::commit();

            return;
        }

        this->commit_with(
            [this](const std::vector<const row_type*>& _rows) -> bool
            {
                return m_copy_rows(_rows);
            });

        return;
    }

    // upkeep -- through the layer: ANALYZE, for the planner's statistics;
    // VACUUM, reclaiming dead rows' space, and VACUUM FULL, rewriting the
    // table under an exclusive lock, neither inside a transaction (25001);
    // REINDEX TABLE, CONCURRENTLY from PostgreSQL 12 (D_DB_UNSUPPORTED
    // before), without blocking writes, and then outside a transaction. Each
    // throws as database_table's members do.
    void
    analyze_table()
    {
        this->validate_connected("analyze_table");

        this->m_connection->run_layer(
            "analyze",
            [this](face::postgres& _db) -> int
            {
                return _db.analyze(this->layer_scope(),
                                   face::string_arg(this->table_name()));
            });

        return;
    }

    void
    vacuum_table()
    {
        m_vacuum("vacuum_table", 0u);

        return;
    }

    void
    vacuum_full_table()
    {
        m_vacuum("vacuum_full_table", D_POSTGRES_FULL);

        return;
    }

    void
    reindex_table(
        bool _concurrently = false
    )
    {
        this->validate_connected("reindex_table");

        this->m_connection->run_layer(
            "reindex",
            [this, _concurrently](face::postgres& _db) -> int
            {
                return _db.reindex(this->layer_scope(),
                                   face::string_arg(this->table_name()),
                                   (_concurrently) ? D_POSTGRES_CONCURRENTLY
                                                   : 0u);
            });

        return;
    }

protected:
    bool m_use_copy_on_commit;

private:
    // m_vacuum -- VACUUM of the table, with the layer's flags
    void
    m_vacuum(
        const char* _caller,
        unsigned    _flags
    )
    {
        this->validate_connected(_caller);

        this->m_connection->run_layer(
            "vacuum",
            [this, _flags](face::postgres& _db) -> int
            {
                return _db.vacuum(this->layer_scope(),
                                  face::string_arg(this->table_name()),
                                  _flags);
            });

        return;
    }

    /**
     * @brief Inserts rows with one COPY, or row by row where a row leaves a
     *        value to the database.
     *
     * @param[in] _rows  the rows.
     * @return whether a value was left to the database.
     * @throws what the COPY throws, after ending it.
     */
    bool
    m_copy_rows(
        const std::vector<const row_type*>& _rows
    )
    {
        const std::vector<size_type> keys = this->key_columns();

        // a row that leaves a value to the database needs an INSERT, which
        // can leave a column out; COPY writes every column
        for (const row_type* record : _rows)
        {
            for (size_type c = 0; c < this->cols(); ++c)
            {
                if (this->left_to_database((*record)[c], c, keys))
                {
                    return this->insert_cached_rows(_rows);
                }
            }
        }

        pg_connection& db      = *this->m_connection;
        std::string    columns;

        for (size_type c = 0; c < this->cols(); ++c)
        {
            columns += (c > 0) ? ", " : "";
            columns += double_quote(this->get_schema().columns[c].name);
        }

        db.copy_in_start("COPY " + this->qualified_name() + " (" + columns +
                         ") FROM STDIN");

        try
        {
            std::string chunk;

            // rows of text, a tab between cells, sent in chunks
            for (const row_type* record : _rows)
            {
                for (size_type c = 0; c < this->cols(); ++c)
                {
                    chunk += (c > 0) ? "\t" : "";
                    m_append_copy_text(chunk, (*record)[c]);
                }

                chunk += '\n';

                if (chunk.size() >= 65536u)
                {
                    db.copy_data(chunk.data(), chunk.size());
                    chunk.clear();
                }
            }

            if (!chunk.empty())
            {
                db.copy_data(chunk.data(), chunk.size());
            }

            (void)db.copy_end();
        }
        catch (...)
        {
            // a COPY still waiting for rows is ended, keeping none
            try
            {
                db.copy_abort("the commit failed");
            }
            catch (...)
            {
                // the COPY ended already; the first failure is the one
            }

            throw;
        }

        return false;
    }

    // m_append_copy_text -- a cell as COPY's text format writes it: \N for
    // NULL, t or f, numbers as value_to_string writes them, bytes as bytea's
    // hexadecimal, a point in time in UTC, and every backslash, tab, newline
    // and carriage return escaped
    static void
    m_append_copy_text(
        std::string&      _text,
        const value_type& _cell
    )
    {
        std::visit(
            [&_text](const auto& _held) -> void
            {
                using held_t = std::decay_t<decltype(_held)>;

                if constexpr (std::is_same_v<held_t, std::monostate>)
                {
                    _text += "\\N";
                }
                else if constexpr (std::is_same_v<held_t, bool>)
                {
                    _text += (_held) ? "t" : "f";
                }
                else if constexpr (std::is_same_v<
                                       held_t,
                                       std::vector<re_std::uint8_t>>)
                {
                    // bytea's \x, its backslash escaped for COPY
                    _text += "\\\\x" + value_to_string(value(_held));
                }
                else if constexpr (std::is_same_v<
                                       held_t,
                                       std::chrono::system_clock::time_point>)
                {
                    _text += value_to_string(value(_held)) +
                             postgres_layer::utc_offset;
                }
                else
                {
                    m_append_escaped(_text, value_to_string(value(_held)));
                }

                return;
            },
            _cell);

        return;
    }

    // m_append_escaped -- text with COPY's escapes
    static void
    m_append_escaped(
        std::string&       _text,
        const std::string& _raw
    )
    {
        for (const char c : _raw)
        {
            switch (c)
            {
                case '\\': _text += "\\\\"; break;
                case '\t': _text += "\\t";  break;
                case '\n': _text += "\\n";  break;
                case '\r': _text += "\\r";  break;
                default:   _text += c;      break;
            }
        }

        return;
    }
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_POSTGRES_POSTGRES_TABLE_HPP
