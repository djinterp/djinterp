/******************************************************************************
* djinterp [database]                                      cassandra_table.hpp
*
* djinterp Apache Cassandra typed table wrapper:
*   This header provides a thin, typed wrapper over a single Cassandra
* keyspace.table. It is the Cassandra companion to redis_table /
* dynamodb_table — standalone, NOT inheriting from database_table<>.
*
* RATIONALE — WHY STANDALONE (a borderline case):
*   Cassandra IS table-based and exposes a SQL-like query language (CQL),
* so on the surface it is the closest of the recent non-relational
* additions to the relational database_table<> charter (which targets
* MySQL, MariaDB, PostgreSQL, SQLite, and Oracle). However:
*
*     - CQL has no JOIN — every query is single-table.
*     - CQL has no OFFSET — pagination uses an opaque paging-state
*       token returned by the coordinator (the cursor approach), not
*       an integer offset.
*     - schema introspection lives under system_schema.* rather than
*       the SQL-standard INFORMATION_SCHEMA.
*     - identifier quoting is "double-quoted" but the conventions
*       around case sensitivity and folding differ.
*     - the primary key model is split — partition columns determine
*       physical placement and clustering columns determine in-
*       partition row ordering, which has no analogue in the
*       database_table<> contract.
*     - ALLOW FILTERING vs key-driven access is a fundamental
*       efficiency/safety distinction not present in the SQL family.
*
*   Given the gap on JOIN / OFFSET / INFORMATION_SCHEMA and the split
* partition/clustering key model, this wrapper is provided as a
* standalone class rather than a database_table<> leaf — consistent
* with the recent redis_table and dynamodb_table additions. If a
* future refactor pushes the database_table<> contract to admit
* table-based-but-not-relational vendors, this class is the strongest
* candidate to migrate first; the public method names already mirror
* the database_table<> surface (set_row / get_row / delete_row /
* row_exists / refresh / commit / clear_table).
*
* CAPABILITIES:
*   - holds a cassandra_connection*, target keyspace, table name, and
*     full primary-key schema (partition columns + optional clustering
*     columns)
*   - per-table default consistency level
*   - row CRUD via the underlying connection (insert_row,
*     select_rows, update_row, delete_row, row_exists)
*   - field-level get/set helpers
*   - lightweight transaction helpers (insert_if_not_exists,
*     update_if, delete_if)
*   - enumeration via the connection's execute_paged /
*     fetch_next_page cursor (Cassandra's only scan idiom)
*   - in-memory cache for read-mostly workloads, with refresh()
*     (paged scan into cache) and commit() (batched INSERT)
*   - clear_table() via the native CQL TRUNCATE TABLE statement
*   - compile-time feature queries (LWT, materialized views, UDF,
*     vector search) reflecting the underlying server's capabilities
*
* USAGE:
*   cassandra_connection conn;
*   conn.connect(cassandra_connect_config{ ... });
*
*   // partition key only (single-column partition)
*   cassandra_table<> users{ &conn, "app", "users", {"id"}, {} };
*
*   row r = { {"id", value{std::int64_t{1}}},
*             {"name", value{std::string{"teer"}}} };
*   users.set_row(r);
*   auto loaded = users.get_row({ {"id", value{std::int64_t{1}}} });
*
*   // partition + clustering (event log)
*   cassandra_table<> events{
*       &conn,
*       "app",
*       "events",
*       {"user_id"},
*       {"event_time"}
*   };
*
*
* path:      /inc/djinterp/core/db/cassandra/cassandra_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_CASSANDRA_TABLE_
#define DJINTERP_DATABASE_CASSANDRA_TABLE_

// std
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "./cassandra.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   CASSANDRA COMPOSITE-KEY UTILITIES
    // =========================================================================

    // cassandra_value_to_key_string
    //   function: serializes a value into a stable string for use as
    // an in-memory cache key segment. This is a textual representation
    // only — it never touches CQL bind variables or the wire format.
    inline std::string cassandra_value_to_key_string(const value& _v)
    {
        std::ostringstream oss;

        switch (_v.type)
        {
            case field_type::null:
            {
                oss << "<null>";

                break;
            }

            case field_type::boolean:
            {
                oss << (_v.as_bool() ? "true" : "false");

                break;
            }

            case field_type::integer:
            {
                oss << _v.as_int();

                break;
            }

            case field_type::big_integer:
            {
                oss << _v.as_int64();

                break;
            }

            case field_type::floating_point:
            {
                oss << _v.as_double();

                break;
            }

            case field_type::decimal:
            case field_type::string:
            case field_type::json:
            case field_type::xml:
            case field_type::uuid:
            {
                oss << _v.as_string();

                break;
            }

            case field_type::binary:
            {
                oss << "<blob:" << _v.as_string().size() << ">";

                break;
            }

            case field_type::date:
            case field_type::time:
            case field_type::datetime:
            case field_type::timestamp:
            {
                oss << _v.as_string();

                break;
            }

            case field_type::array:
            case field_type::custom:
            default:
            {
                oss << _v.as_string();

                break;
            }
        }

        return oss.str();
    }


    // =========================================================================
    // II.  CASSANDRA TABLE
    // =========================================================================

    // cassandra_table
    //   class: typed wrapper over one Cassandra keyspace.table. The
    // _Config template parameter is reserved for future ABI-stable
    // customization (e.g. row cache policy, alternative key encoders)
    // and is unused by the default instantiation.
    template<typename _Config = void>
    class cassandra_table
    {
    public:
        using connection_type = cassandra_connection;
        using paged_result    = cassandra_connection::paged_result;
        using cache_type      = std::map<std::string, row>;


        // -------------------------------------------------------------
        // constructors
        // -------------------------------------------------------------

        cassandra_table()
            : m_connection(nullptr)
            , m_keyspace()
            , m_table_name()
            , m_partition_keys()
            , m_clustering_keys()
            , m_separator("#")
            , m_consistency(cassandra_consistency_level::local_quorum)
            , m_cache()
        {
        }

        cassandra_table(
                connection_type*   _connection,
                const std::string& _keyspace,
                const std::string& _table_name
            )
            : m_connection(_connection)
            , m_keyspace(_keyspace)
            , m_table_name(_table_name)
            , m_partition_keys()
            , m_clustering_keys()
            , m_separator("#")
            , m_consistency(cassandra_consistency_level::local_quorum)
            , m_cache()
        {
        }

        cassandra_table(
                connection_type*                _connection,
                const std::string&              _keyspace,
                const std::string&              _table_name,
                const std::vector<std::string>& _partition_keys,
                const std::vector<std::string>& _clustering_keys
            )
            : m_connection(_connection)
            , m_keyspace(_keyspace)
            , m_table_name(_table_name)
            , m_partition_keys(_partition_keys)
            , m_clustering_keys(_clustering_keys)
            , m_separator("#")
            , m_consistency(cassandra_consistency_level::local_quorum)
            , m_cache()
        {
        }

        ~cassandra_table() = default;

        cassandra_table(const cassandra_table&)            = default;
        cassandra_table& operator=(const cassandra_table&) = default;
        cassandra_table(cassandra_table&&) noexcept            = default;
        cassandra_table& operator=(cassandra_table&&) noexcept = default;


        // -------------------------------------------------------------
        // connection accessors
        // -------------------------------------------------------------

        // get_connection
        //   function: returns the underlying Cassandra connection
        // pointer.
        connection_type* get_connection() const noexcept
        {
            return m_connection;
        }

        // set_connection
        //   function: replaces the underlying Cassandra connection
        // pointer. Does not modify the cache.
        void set_connection(connection_type* _connection) noexcept
        {
            m_connection = _connection;
        }

        // is_connected
        //   function: true if the wrapper has a non-null connection
        // pointer and the connection reports connected.
        bool is_connected() const
        {
            return ( (m_connection != nullptr)        &&
                     ( m_connection->is_connected() ) );
        }


        // -------------------------------------------------------------
        // identity accessors
        // -------------------------------------------------------------

        const std::string& keyspace() const noexcept
        {
            return m_keyspace;
        }

        void set_keyspace(const std::string& _keyspace)
        {
            m_keyspace = _keyspace;
        }

        const std::string& table_name() const noexcept
        {
            return m_table_name;
        }

        void set_table_name(const std::string& _table_name)
        {
            m_table_name = _table_name;
        }

        // qualified_name
        //   function: returns "<keyspace>.<table_name>" suitable for
        // direct embedding in CQL.
        std::string qualified_name() const
        {
            return m_keyspace + "." + m_table_name;
        }


        // -------------------------------------------------------------
        // key schema accessors
        // -------------------------------------------------------------

        const std::vector<std::string>& partition_keys() const noexcept
        {
            return m_partition_keys;
        }

        void set_partition_keys(
            const std::vector<std::string>& _partition_keys)
        {
            m_partition_keys = _partition_keys;
        }

        const std::vector<std::string>& clustering_keys() const noexcept
        {
            return m_clustering_keys;
        }

        void set_clustering_keys(
            const std::vector<std::string>& _clustering_keys)
        {
            m_clustering_keys = _clustering_keys;
        }

        // has_clustering_keys
        //   function: true if this table's primary key has clustering
        // columns (i.e. multiple rows per partition).
        bool has_clustering_keys() const noexcept
        {
            return ( ! m_clustering_keys.empty() );
        }

        // is_single_partition_column
        //   function: true if the partition key is one column.
        bool is_single_partition_column() const noexcept
        {
            return (m_partition_keys.size() == 1);
        }


        // -------------------------------------------------------------
        // consistency level
        // -------------------------------------------------------------

        cassandra_consistency_level consistency() const noexcept
        {
            return m_consistency;
        }

        void set_consistency(cassandra_consistency_level _level) noexcept
        {
            m_consistency = _level;
        }


        // -------------------------------------------------------------
        // composite-key helpers
        // -------------------------------------------------------------

        // build_key
        //   function: composes a row of key columns from a vector of
        // (partition + clustering) values matching the table's key
        // schema. Throws std::invalid_argument if the value count
        // doesn't match the key schema width.
        row build_key(const std::vector<value>& _key_values) const
        {
            const std::size_t expected =
                m_partition_keys.size() + m_clustering_keys.size();

            if (_key_values.size() != expected)
            {
                throw std::invalid_argument(
                    "cassandra_table::build_key: value count does "
                    "not match the table's key schema width");
            }

            row out;

            std::size_t index = 0;

            for (const auto& col : m_partition_keys)
            {
                out.emplace(col, _key_values[index]);

                ++index;
            }

            for (const auto& col : m_clustering_keys)
            {
                out.emplace(col, _key_values[index]);

                ++index;
            }

            return out;
        }

        // build_partition_key
        //   function: composes a row containing only the partition-key
        // columns from a vector of partition values.
        row build_partition_key(
            const std::vector<value>& _partition_values) const
        {
            if (_partition_values.size() != m_partition_keys.size())
            {
                throw std::invalid_argument(
                    "cassandra_table::build_partition_key: value "
                    "count does not match the partition-key width");
            }

            row out;

            for (std::size_t i = 0; i < m_partition_keys.size(); ++i)
            {
                out.emplace(m_partition_keys[i], _partition_values[i]);
            }

            return out;
        }

        // key_from_row
        //   function: extracts the key columns (partition +
        // clustering) from a full row.
        row key_from_row(const row& _row) const
        {
            row out;

            for (const auto& col : m_partition_keys)
            {
                auto it = _row.find(col);

                if (it != _row.end())
                {
                    out.emplace(col, it->second);
                }
            }

            for (const auto& col : m_clustering_keys)
            {
                auto it = _row.find(col);

                if (it != _row.end())
                {
                    out.emplace(col, it->second);
                }
            }

            return out;
        }


        // -------------------------------------------------------------
        // row CRUD
        // -------------------------------------------------------------

        // set_row
        //   function: INSERT (or UPSERT, since Cassandra INSERT is
        // upsert by default) the row.
        bool set_row(const row& _row)
        {
            validate_connected("set_row");

            const bool ok =
                m_connection->insert_row(m_keyspace, m_table_name, _row);

            if (ok)
            {
                m_cache[cache_key_for(_row)] = _row;
            }

            return ok;
        }

        // get_row
        //   function: SELECT one row by its full primary key.
        // _key must contain values for every partition-key column and
        // every clustering-key column.
        std::optional<row> get_row(const row& _key) const
        {
            validate_connected("get_row");

            const std::string cql =
                "SELECT * FROM " + qualified_name() +
                " WHERE " + build_where_clause(_key);

            result_rows rows = m_connection->select_rows(cql);

            if (rows.empty())
            {
                return std::nullopt;
            }

            return rows.front();
        }

        // update_row
        //   function: UPDATE _updates WHERE _key.
        bool update_row(const row& _key, const row& _updates)
        {
            validate_connected("update_row");

            const bool ok =
                m_connection->update_row(m_keyspace,
                                         m_table_name,
                                         _key,
                                         _updates);

            if (ok)
            {
                const std::string ck = cache_key_for(_key);

                auto it = m_cache.find(ck);

                if (it != m_cache.end())
                {
                    for (const auto& kv : _updates)
                    {
                        it->second[kv.first] = kv.second;
                    }
                }
            }

            return ok;
        }

        // delete_row
        //   function: DELETE WHERE _key.
        bool delete_row(const row& _key)
        {
            validate_connected("delete_row");

            const bool ok =
                m_connection->delete_row(m_keyspace,
                                         m_table_name,
                                         _key);

            if (ok)
            {
                m_cache.erase(cache_key_for(_key));
            }

            return ok;
        }

        // row_exists
        //   function: SELECT-based existence check WHERE _key.
        bool row_exists(const row& _key) const
        {
            validate_connected("row_exists");

            return m_connection->row_exists(m_keyspace,
                                            m_table_name,
                                            _key);
        }


        // -------------------------------------------------------------
        // field-level helpers
        // -------------------------------------------------------------

        // get_field
        //   function: returns one column of the row matching _key.
        // Returns std::nullopt if the row or the field is absent.
        std::optional<value> get_field(const row&         _key,
                                       const std::string& _field) const
        {
            std::optional<row> r = get_row(_key);

            if ( ! r.has_value() )
            {
                return std::nullopt;
            }

            auto it = r->find(_field);

            if (it == r->end())
            {
                return std::nullopt;
            }

            return it->second;
        }

        // set_field
        //   function: UPDATE one column of the row matching _key.
        bool set_field(const row&         _key,
                       const std::string& _field,
                       const value&       _value)
        {
            row updates;
            updates.emplace(_field, _value);

            return update_row(_key, updates);
        }


        // -------------------------------------------------------------
        // lightweight transactions (LWT / Paxos)
        // -------------------------------------------------------------

        // set_row_if_not_exists
        //   function: INSERT ... IF NOT EXISTS — fails if a row with
        // the same primary key already exists.
        bool set_row_if_not_exists(const row& _row)
        {
            validate_connected("set_row_if_not_exists");

            const bool ok =
                m_connection->insert_if_not_exists(m_keyspace,
                                                   m_table_name,
                                                   _row);

            if (ok)
            {
                m_cache[cache_key_for(_row)] = _row;
            }

            return ok;
        }

        // update_row_if
        //   function: UPDATE ... IF <condition> — Paxos-coordinated
        // conditional update.
        bool update_row_if(const row&         _key,
                           const row&         _updates,
                           const std::string& _condition)
        {
            validate_connected("update_row_if");

            return m_connection->update_if(m_keyspace,
                                           m_table_name,
                                           _key,
                                           _updates,
                                           _condition);
        }

        // delete_row_if
        //   function: DELETE ... IF <condition> — Paxos-coordinated
        // conditional delete.
        bool delete_row_if(const row&         _key,
                           const std::string& _condition)
        {
            validate_connected("delete_row_if");

            return m_connection->delete_if(m_keyspace,
                                           m_table_name,
                                           _key,
                                           _condition);
        }


        // -------------------------------------------------------------
        // enumeration
        // -------------------------------------------------------------

        // select_all
        //   function: full-table scan via the paged-cursor idiom. The
        // wrapper loops through every page produced by execute_paged
        // / fetch_next_page until the paging-state is empty. _limit
        // applies a CQL LIMIT to the underlying query (0 = unbounded
        // — use with caution on large partitions).
        result_rows select_all(
                int _page_size = 5000,
                int _limit     = 0
            ) const
        {
            validate_connected("select_all");

            std::string cql =
                "SELECT * FROM " + qualified_name();

            if (_limit > 0)
            {
                cql += " LIMIT " + std::to_string(_limit);
            }

            result_rows accum;

            paged_result page =
                m_connection->execute_paged(cql, _page_size);

            for (auto& r : page.first)
            {
                accum.push_back(std::move(r));
            }

            while ( page.second.has_value() )
            {
                page = const_cast<connection_type*>(m_connection)
                       ->fetch_next_page(*page.second);

                for (auto& r : page.first)
                {
                    accum.push_back(std::move(r));
                }
            }

            return accum;
        }

        // select_by_partition
        //   function: SELECT all rows under a given partition key
        // (efficient: hits one replica set).
        result_rows select_by_partition(
            const row& _partition_key) const
        {
            validate_connected("select_by_partition");

            const std::string cql =
                "SELECT * FROM " + qualified_name() +
                " WHERE " + build_where_clause(_partition_key);

            return m_connection->select_rows(cql);
        }


        // -------------------------------------------------------------
        // bulk operations
        // -------------------------------------------------------------

        // refresh
        //   function: paged scan of the table into the in-memory
        // cache, discarding prior cache contents. Returns the number
        // of rows cached.
        std::size_t refresh(int _page_size = 5000)
        {
            validate_connected("refresh");

            result_rows rows = select_all(_page_size, 0);

            m_cache.clear();

            for (auto& r : rows)
            {
                const std::string k = cache_key_for(r);
                m_cache.emplace(k, std::move(r));
            }

            return m_cache.size();
        }

        // commit
        //   function: writes every cached row back to the table via a
        // single batched dispatch. Returns the number of rows
        // committed. The batch type defaults to UNLOGGED — LOGGED
        // batches cross partitions atomically but at a substantial
        // throughput cost.
        std::size_t commit(
            cassandra_batch_type _batch_type =
                cassandra_batch_type::unlogged)
        {
            validate_connected("commit");

            if (m_cache.empty())
            {
                return 0;
            }

            m_connection->batch_start(static_cast<int>(_batch_type));

            std::size_t count = 0;

            for (const auto& kv : m_cache)
            {
                m_connection->batch_add(
                    build_insert_statement(kv.second));

                ++count;
            }

            m_connection->batch_execute();

            return count;
        }

        // clear_table
        //   function: TRUNCATE TABLE — deletes every row in the table
        // and purges the cache. Note: TRUNCATE in Cassandra requires
        // all nodes to be reachable and is a heavy operation; consider
        // DROP + CREATE for fresh-table workflows.
        bool clear_table()
        {
            validate_connected("clear_table");

            const std::string cql =
                "TRUNCATE TABLE " + qualified_name();

            m_connection->execute_cql(cql);
            m_cache.clear();

            return true;
        }


        // -------------------------------------------------------------
        // cache access
        // -------------------------------------------------------------

        const cache_type& cache() const noexcept
        {
            return m_cache;
        }

        void clear_cache() noexcept
        {
            m_cache.clear();
        }

        std::size_t cached_size() const noexcept
        {
            return m_cache.size();
        }


        // -------------------------------------------------------------
        // compile-time feature queries
        // -------------------------------------------------------------

        static constexpr bool supports_lwt() noexcept
        {
            return cassandra_connection::supports_lwt();
        }

        static constexpr bool supports_materialized_views() noexcept
        {
            return cassandra_connection::supports_materialized_views();
        }

        static constexpr bool supports_udf() noexcept
        {
            return cassandra_connection::supports_udf();
        }

        static constexpr bool supports_vector_search() noexcept
        {
            return cassandra_connection::supports_vector_search();
        }


    protected:

        // -------------------------------------------------------------
        // protected helpers
        // -------------------------------------------------------------

        // validate_connected
        //   function: throws connection_exception when the underlying
        // connection is null or not connected.
        void validate_connected(const char* _op) const
        {
            if (m_connection == nullptr)
            {
                throw connection_exception(
                    std::string{"cassandra_table::"} + _op +
                    ": no connection bound");
            }

            if ( ! m_connection->is_connected() )
            {
                throw connection_exception(
                    std::string{"cassandra_table::"} + _op +
                    ": connection is not open");
            }
        }

        // cache_key_for
        //   function: composes the in-memory cache key string for a
        // row by concatenating partition-column values, then
        // clustering-column values, separated by m_separator.
        std::string cache_key_for(const row& _row) const
        {
            std::string out;

            bool first = true;

            for (const auto& col : m_partition_keys)
            {
                if ( ! first )
                {
                    out += m_separator;
                }

                first = false;

                auto it = _row.find(col);

                if (it != _row.end())
                {
                    out += cassandra_value_to_key_string(it->second);
                }
                else
                {
                    out += "<missing>";
                }
            }

            for (const auto& col : m_clustering_keys)
            {
                out += m_separator;

                auto it = _row.find(col);

                if (it != _row.end())
                {
                    out += cassandra_value_to_key_string(it->second);
                }
                else
                {
                    out += "<missing>";
                }
            }

            return out;
        }

        // build_where_clause
        //   function: builds a CQL `col1 = ? AND col2 = ?` fragment
        // for the columns present in _key, using textual rendering of
        // each value via cql_literal_for. This is a fallback used by
        // get_row / select_by_partition; production paths should
        // prefer prepared statements at the connection layer.
        std::string build_where_clause(const row& _key) const
        {
            std::ostringstream oss;

            bool first = true;

            for (const auto& kv : _key)
            {
                if ( ! first )
                {
                    oss << " AND ";
                }

                first = false;

                oss << kv.first
                    << " = "
                    << cql_literal_for(kv.second);
            }

            return oss.str();
        }

        // build_insert_statement
        //   function: builds a CQL INSERT INTO statement for the row
        // suitable for use inside a batch.
        std::string build_insert_statement(const row& _row) const
        {
            std::ostringstream cols;
            std::ostringstream vals;

            bool first = true;

            for (const auto& kv : _row)
            {
                if ( ! first )
                {
                    cols << ", ";
                    vals << ", ";
                }

                first = false;

                cols << kv.first;
                vals << cql_literal_for(kv.second);
            }

            std::ostringstream oss;
            oss << "INSERT INTO " << qualified_name()
                << " ("  << cols.str() << ")"
                << " VALUES (" << vals.str() << ")";

            return oss.str();
        }

        // cql_literal_for
        //   function: produces a CQL literal fragment for a value.
        // Strings/UUIDs/dates are single-quoted (with internal single
        // quotes doubled); blobs are emitted as 0x-prefixed hex; nulls
        // map to the CQL `NULL` keyword.
        std::string cql_literal_for(const value& _v) const
        {
            std::ostringstream oss;

            switch (_v.type)
            {
                case field_type::null:
                {
                    oss << "NULL";

                    break;
                }

                case field_type::boolean:
                {
                    oss << (_v.as_bool() ? "true" : "false");

                    break;
                }

                case field_type::integer:
                {
                    oss << _v.as_int();

                    break;
                }

                case field_type::big_integer:
                {
                    oss << _v.as_int64();

                    break;
                }

                case field_type::floating_point:
                {
                    oss << _v.as_double();

                    break;
                }

                case field_type::decimal:
                {
                    oss << _v.as_string();

                    break;
                }

                case field_type::binary:
                {
                    oss << "0x" << _v.as_string();

                    break;
                }

                case field_type::string:
                case field_type::json:
                case field_type::xml:
                case field_type::uuid:
                case field_type::date:
                case field_type::time:
                case field_type::datetime:
                case field_type::timestamp:
                case field_type::array:
                case field_type::custom:
                default:
                {
                    std::string s = _v.as_string();

                    std::string escaped;
                    escaped.reserve(s.size() + 2);

                    for (char c : s)
                    {
                        if (c == '\'')
                        {
                            escaped += "''";
                        }
                        else
                        {
                            escaped += c;
                        }
                    }

                    oss << "'" << escaped << "'";

                    break;
                }
            }

            return oss.str();
        }


    private:

        connection_type*                m_connection;
        std::string                     m_keyspace;
        std::string                     m_table_name;
        std::vector<std::string>        m_partition_keys;
        std::vector<std::string>        m_clustering_keys;
        std::string                     m_separator;
        cassandra_consistency_level     m_consistency;
        cache_type                      m_cache;
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_CASSANDRA_TABLE_
