/*******************************************************************************
* djinterp [core]                                              mariadb_table.hpp
*
* A table of a MariaDB database held in memory: mysql_common_table over a
* mariadb_connection, with what MariaDB adds.
*   mariadb_table is mysql_common_table<mariadb_connection> -- database_
* table's cache, refresh, commit and table operations, the engine, MODIFY
* COLUMN and SHOW CREATE TABLE -- and adds: system-versioned tables (MariaDB
* 10.3.4), read as they stood at a point in time, between two, or in every
* version; INSERT and DELETE that return the rows they wrote or removed
* (10.5.0); a Galera cluster's causal reads after a commit; and what the
* server supports, asked of it.
*   What a server supports depends on its version, which the layer reads
* when the connection opens: the has_ members ask it, never the headers.
*
*
* path:      /inc/djinterp/core/db/mariadb/mariadb_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.20
*                                                            revised: 2026.10.16
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  MARIADB TABLE
    -------------
    1.  The class
         1.  mariadb_table
*/

#ifndef DJINTERP_DB_MARIADB_MARIADB_TABLE_HPP
#define DJINTERP_DB_MARIADB_MARIADB_TABLE_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>    // std::chrono::system_clock, microseconds
#include <optional>  // std::optional
#include <set>       // std::set
#include <string>    // std::string, std::to_string
#include <utility>   // std::move
#include <vector>    // std::vector
// djinterp
#include "../../../djinterp.hpp"             // framework root
#include "../mysql/mysql_common_table.hpp"  // mysql_common_table
#include "./mariadb.hpp"                    // mariadb_connection


NS_DJINTERP


//==============================================================================
// 1.  MARIADB TABLE
//==============================================================================


// 1.1    The class
//------------------------------------------------------------------------------
// 1.1.1
// mariadb_table
//   class template: a table of a MariaDB database, as the banner describes.
// A refresh from the table's history reads rows that are not the table's
// own, so a commit refuses them until a plain refresh() reads the table
// again. Each member throws as database_table's do; one the server is too
// old for, query_exception with D_DB_UNSUPPORTED. Not copyable; movable.
//
// Usage:
//     mariadb_table<> prices(db, "prices");
//
//     prices.fetch_schema();             // and whether it is versioned
//     prices.refresh_as_of(yesterday);   // the rows as they stood then
template<typename Config = void>
class mariadb_table
    : public mysql_common_table<mariadb_connection, value, Config>
{
private:
    using base_type = mysql_common_table<mariadb_connection, value, Config>;

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

    // construction -- as database_table's
    mariadb_table()
        : base_type(),
          m_system_versioned(),
          m_galera_sync_on_commit(false)
    {}

    explicit mariadb_table(
        mariadb_connection& _conn,
        std::string         _table_name,
        table_kind          _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_table_name), _kind),
          m_system_versioned(),
          m_galera_sync_on_commit(false)
    {}

    explicit mariadb_table(
        mariadb_connection& _conn,
        table_schema        _schema,
        table_kind          _kind = table_kind::base_table
    )
        : base_type(_conn, std::move(_schema), _kind),
          m_system_versioned(),
          m_galera_sync_on_commit(false)
    {}

    explicit mariadb_table(
        mariadb_connection& _conn,
        table_schema        _schema,
        table_kind          _kind,
        const sync_config&  _sync
    )
        : base_type(_conn, std::move(_schema), _kind, _sync),
          m_system_versioned(),
          m_galera_sync_on_commit(false)
    {}

    ~mariadb_table() = default;

    mariadb_table(const mariadb_table&)            = delete;
    mariadb_table& operator=(const mariadb_table&) = delete;

    mariadb_table(mariadb_table&&) noexcept            = default;
    mariadb_table& operator=(mariadb_table&&) noexcept = default;

    /**
     * @brief Describes the table's columns, as database_table's
     *        fetch_schema does, and finds out whether it is system-versioned.
     *
     * @throws as database_table's fetch_schema.
     */
    void
    fetch_schema()
    {
        base_type::fetch_schema();
        m_detect_system_versioning();

        return;
    }

    // is_system_versioned -- whether the table keeps its rows' history, as
    // the server answered when fetch_schema or a refresh from the history
    // last asked; false before either
    bool
    is_system_versioned() const noexcept
    {
        return m_system_versioned.value_or(false);
    }

    /**
     * @brief Reads into the cache the rows the query selects as they stood
     *        at a point in time, from the table's history.
     *
     * @param[in] _when  the point in time.
     * @throws as refresh(); query_exception with D_DB_UNSUPPORTED for a table
     *         that keeps no history.
     * @post As refresh(), but commit() refuses the cache until a plain
     *       refresh() reads the table again.
     */
    void
    refresh_as_of(
        std::chrono::system_clock::time_point _when
    )
    {
        m_refresh_history("refresh_as_of",
                          " FOR SYSTEM_TIME AS OF TIMESTAMP " +
                              m_point_sql(_when));

        return;
    }

    // refresh_as_of -- as above, the point given as SQL: a TIMESTAMP literal,
    // an expression, or TRANSACTION and a transaction's id, as written
    void
    refresh_as_of(
        const std::string& _point_sql
    )
    {
        m_refresh_history("refresh_as_of",
                          " FOR SYSTEM_TIME AS OF " + _point_sql);

        return;
    }

    /**
     * @brief Reads into the cache every version of the rows the query
     *        selects that stood at any time from `_from` to `_to`, both
     *        included.
     *
     * @param[in] _from  the first point in time.
     * @param[in] _to    the last.
     * @throws as refresh_as_of.
     * @post As refresh_as_of.
     */
    void
    refresh_between(
        std::chrono::system_clock::time_point _from,
        std::chrono::system_clock::time_point _to
    )
    {
        m_refresh_history("refresh_between",
                          " FOR SYSTEM_TIME BETWEEN TIMESTAMP " +
                              m_point_sql(_from) + " AND TIMESTAMP " +
                              m_point_sql(_to));

        return;
    }

    // refresh_between -- as above, the points given as SQL, as written
    void
    refresh_between(
        const std::string& _from_sql,
        const std::string& _to_sql
    )
    {
        m_refresh_history("refresh_between",
                          " FOR SYSTEM_TIME BETWEEN " + _from_sql + " AND " +
                              _to_sql);

        return;
    }

    // refresh_all_versions -- every version of the rows the query selects,
    // current and past; as refresh_as_of
    void
    refresh_all_versions()
    {
        m_refresh_history("refresh_all_versions", " FOR SYSTEM_TIME ALL");

        return;
    }

    /**
     * @brief Inserts a row into the table at once, and returns it as the
     *        table holds it, with what the database filled in: a NULL left
     *        to it, as commit() leaves one, a default, a trigger's change.
     *
     * @note A cache that stands for the table's rows takes the row; one the
     *       table was not read for is left as it is. Without a primary key, a
     *       cache that was read is read again, which it must not need to
     *       lose changes for.
     *
     * @param[in] _row  the row, a cell for each column.
     * @return the row as the table holds it.
     * @throws as commit(); query_exception with D_DB_UNSUPPORTED before
     *         MariaDB 10.5.0, and while a cache without a key that was read
     *         holds changes not yet committed.
     */
    row_type
    insert_row_returning(
        const row_type& _row
    )
    {
        this->validate_connected("insert_row_returning");
        this->validate_mutable("insert_row_returning");
        this->check_row_width(_row, "insert_row_returning");
        m_require_returning("insert_row_returning");

        const std::vector<size_type> keys = this->key_columns();

        // a cache without a key is read again after, so it must hold nothing
        // to lose
        if ( (keys.empty()) &&
             (this->m_loaded) )
        {
            this->validate_clean("insert_row_returning");
        }

        std::vector<bool> written(this->cols());
        size_type         number = 0;

        for (size_type c = 0; c < this->cols(); ++c)
        {
            written[c] = !this->left_to_database(_row[c], c, keys);
        }

        auto statement = this->m_connection->prepare(
            this->insert_sql(written) + " RETURNING " + m_column_list());

        for (size_type c = 0; c < this->cols(); ++c)
        {
            if (written[c])
            {
                this->bind_value(*statement, ++number, _row[c]);
            }
        }

        std::unique_ptr<buffered_result_set> returned =
            statement->execute_query();
        row_type                             result;

        // the row the table holds, cell by cell
        if (returned->next())
        {
            result.reserve(this->cols());

            for (size_type c = 0; c < this->cols(); ++c)
            {
                result.push_back(returned->get_value(c));
            }
        }

        m_after_write(keys,
                      [this, &result]() -> void
                      {
                          this->m_data.push_back(result);
                          this->m_original.push_back(result);

                          return;
                      });

        return result;
    }

    /**
     * @brief Deletes from the table at once the rows a condition selects, and
     *        returns them as they were.
     *
     * @note With a primary key, the rows of the deleted keys leave a cache
     *       that stands for the table's rows; without one, a cache that was
     *       read is read again, as for insert_row_returning.
     *
     * @param[in] _where  the condition, SQL as a WHERE clause's, without the
     *                    keyword.
     * @return the rows deleted.
     * @throws as insert_row_returning.
     */
    std::vector<row_type>
    delete_returning(
        const std::string& _where
    )
    {
        this->validate_connected("delete_returning");
        this->validate_mutable("delete_returning");
        m_require_returning("delete_returning");

        const std::vector<size_type> keys = this->key_columns();

        if ( (keys.empty()) &&
             (this->m_loaded) )
        {
            this->validate_clean("delete_returning");
        }

        std::unique_ptr<buffered_result_set> returned =
            this->m_connection->execute_query(
                "DELETE FROM " + this->qualified_name() + " WHERE " + _where +
                " RETURNING " + m_column_list());
        std::vector<row_type>                deleted;

        while (returned->next())
        {
            row_type record;

            record.reserve(this->cols());

            for (size_type c = 0; c < this->cols(); ++c)
            {
                record.push_back(returned->get_value(c));
            }

            deleted.push_back(std::move(record));
        }

        m_after_write(keys,
                      [this, &deleted, &keys]() -> void
                      {
                          m_forget_keys(deleted, keys);

                          return;
                      });

        return deleted;
    }

    // Galera -- whether a commit then makes the session's reads wait until
    // the cluster has applied what came before them (wsrep_sync_wait = 1), so
    // that a refresh on any node sees the commit; a server without Galera
    // has no such setting, and the commit is as any other
    void
    set_galera_sync_on_commit(
        bool _enabled
    ) noexcept
    {
        m_galera_sync_on_commit = _enabled;

        return;
    }

    bool
    is_galera_sync_on_commit() const noexcept
    {
        return m_galera_sync_on_commit;
    }

    /**
     * @brief Writes the cache's changes to the table, as database_table's
     *        commit does, then, where Galera's causal reads are asked for,
     *        turns them on for the session.
     *
     * @throws as database_table's commit; and what setting wsrep_sync_wait
     *         throws, but for its absence (1193).
     */
    void
    commit()
    {
        base_type::commit();

        // causal reads, where the server has Galera
        if ( (m_galera_sync_on_commit) &&
             (this->is_connected()) )
        {
            this->m_connection->run_layer(
                "commit",
                [](face::mariadb& _db) -> int
                {
                    const int result = _db.set_variable("wsrep_sync_wait", "1");

                    // no Galera, no setting
                    return (result == 1193) ? 0 : result;
                });
        }

        return;
    }

    // support -- what the connected server can do, as its version says:
    // RETURNING (10.5.0), system versioning (10.3.4), sequences (10.3.0),
    // the INET6 type (10.5.0) and the UUID type (10.7.0); false while the
    // table has no open connection
    bool
    has_returning_support() const noexcept
    {
        return m_supports(D_MARIADB_FEATURE_RETURNING);
    }

    bool
    has_system_versioning_support() const noexcept
    {
        return m_supports(D_MARIADB_FEATURE_SYSTEM_VERSIONING);
    }

    bool
    has_sequences_support() const noexcept
    {
        return m_supports(D_MARIADB_FEATURE_SEQUENCES);
    }

    bool
    has_inet6_type_support() const noexcept
    {
        return m_at_least(10u, 5u, 0u);
    }

    bool
    has_uuid_type_support() const noexcept
    {
        return m_at_least(10u, 7u, 0u);
    }

protected:
    std::optional<bool> m_system_versioned;
    bool                m_galera_sync_on_commit;

private:
    // m_supports, m_at_least -- the layer's answer for the connected server
    bool
    m_supports(
        d_mariadb_feature _feature
    ) const noexcept
    {
        return ( (this->is_connected()) &&
                 (this->m_connection->get_face().supports(_feature)) );
    }

    bool
    m_at_least(
        unsigned _major,
        unsigned _minor,
        unsigned _patch
    ) const noexcept
    {
        return ( (this->is_connected()) &&
                 (this->m_connection->get_face().server_at_least(
                     D_MARIADB_FLAVOR_MARIADB,
                     _major,
                     _minor,
                     _patch)) );
    }

    // m_require_returning -- query_exception with D_DB_UNSUPPORTED before
    // MariaDB 10.5.0
    void
    m_require_returning(
        const char* _caller
    ) const
    {
        if (!has_returning_support())
        {
            throw query_exception(std::string("mariadb_table::") + _caller +
                                      ": RETURNING needs MariaDB 10.5.0 or "
                                      "later",
                                  D_DB_UNSUPPORTED);
        }

        return;
    }

    // m_detect_system_versioning -- whether the table keeps its rows'
    // history, by asking for it: a table that keeps none, or a server too old
    // for the clause, refuses
    void
    m_detect_system_versioning()
    {
        try
        {
            (void)this->m_connection->execute_query(
                "SELECT 1 FROM " + this->qualified_name() +
                " FOR SYSTEM_TIME ALL WHERE 1=0");
            m_system_versioned = true;
        }
        catch (const query_exception&)
        {
            m_system_versioned = false;
        }

        return;
    }

    // m_refresh_history -- a refresh from the table's history, through the
    // clause after its name
    void
    m_refresh_history(
        const char*        _caller,
        const std::string& _clause
    )
    {
        this->validate_connected(_caller);

        // whether the table keeps a history, asked once
        if (!m_system_versioned.has_value())
        {
            m_detect_system_versioning();
        }

        if (!m_system_versioned.value_or(false))
        {
            throw query_exception(std::string("mariadb_table::") + _caller +
                                      ": the table is not system-versioned",
                                  D_DB_UNSUPPORTED);
        }

        this->m_table_suffix = _clause;

        try
        {
            this->refresh();
        }
        catch (...)
        {
            this->m_table_suffix.clear();

            throw;
        }

        this->m_table_suffix.clear();

        return;
    }

    // m_point_sql -- a point in time as the session reads one: the moment
    // from the Unix epoch, which FROM_UNIXTIME gives in the session's time
    // zone, to the microsecond
    static std::string
    m_point_sql(
        std::chrono::system_clock::time_point _when
    )
    {
        const long long micros =
            static_cast<long long>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    _when.time_since_epoch()).count());
        const long long whole    = (micros >= 0) ? (micros / 1000000)
                                                 : -((-micros) / 1000000);
        std::string     fraction = std::to_string(((micros >= 0) ? micros
                                                                 : -micros) %
                                                  1000000);

        fraction.insert(0, 6u - fraction.size(), '0');

        return "FROM_UNIXTIME(" + std::string((micros < 0) ? "-" : "") +
               std::to_string((whole >= 0) ? whole : -whole) + "." + fraction +
               ")";
    }

    // m_column_list -- the schema's columns, by name, for RETURNING
    std::string
    m_column_list() const
    {
        std::string columns;

        for (size_type c = 0; c < this->cols(); ++c)
        {
            columns += (c > 0) ? ", " : "";
            columns += backtick_quote(this->get_schema().columns[c].name);
        }

        return columns;
    }

    // m_after_write -- the cache after a row was written at once: changed by
    // the key where the cache stands for the table's rows, read again
    // without one, and left as it is where it was never read
    template<typename Function>
    void
    m_after_write(
        const std::vector<size_type>& _keys,
        Function&&                    _by_key
    )
    {
        // a cache the table was not read for stands for nothing written
        if (!this->m_loaded)
        {
            return;
        }

        if (_keys.empty())
        {
            this->refresh();
        }
        else
        {
            _by_key();
        }

        return;
    }

    // m_forget_keys -- the cache's rows of the keys given, and the rows read
    // of them, removed
    void
    m_forget_keys(
        const std::vector<row_type>&  _rows,
        const std::vector<size_type>& _keys
    )
    {
        std::set<row_type> gone;

        for (const row_type& record : _rows)
        {
            row_type key;

            if (this->key_of(record, _keys, key))
            {
                gone.insert(std::move(key));
            }
        }

        // each row held, the cache's and the read ones
        for (typename base_type::storage_type* rows :
             { &this->m_data, &this->m_original })
        {
            typename base_type::storage_type kept;

            for (row_type& record : *rows)
            {
                row_type key;

                if ( (!this->key_of(record, _keys, key)) ||
                     (gone.find(key) == gone.end()) )
                {
                    kept.push_back(std::move(record));
                }
            }

            rows->swap(kept);
        }

        return;
    }
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MARIADB_MARIADB_TABLE_HPP
