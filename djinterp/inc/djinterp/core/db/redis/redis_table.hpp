/******************************************************************************
* djinterp [database]                                          redis_table.hpp
*
* djinterp Redis table module:
*   A hash-backed "logical table" abstraction over Redis. Redis is a
* key-value store with no native table concept (see the note in
* `database_table.hpp` explicitly placing Redis out of scope for the
* SQL-based `database_table<>` hierarchy), so this header models
* tabular data using Redis's native primitives instead of inheriting
* from `database_table<>`:
*   - each "row" is stored as a Redis hash under a derived key
*     (`<prefix><separator><row_id>` by default)
*   - each "column" is a field within that hash
*   - an optional index set keyed at `<prefix><separator>_index` tracks
*     the set of row IDs for efficient enumeration without KEYS scans
*   - optional default TTL applies to every row hash on commit
*   - per-row, per-namespace, and bulk operations map directly to
*     hiredis-level Redis commands
*
*   This makes `redis_table` a STANDALONE class. It does NOT extend
* `database_table<>` because that template's contract (SQL queries,
* schema introspection via INFORMATION_SCHEMA, identifier quoting) does
* not apply to Redis. It does however expose a similar API surface
* (`refresh`, `commit`, `set_row`, `get_row`, `row_count`, ...) so that
* code generic over storage back-ends can constrain on the
* `database_table` concept where applicable and on more Redis-specific
* concepts where finer-grained behaviour is needed.
*
*   LAYER DIAGRAM:
*     redis_table<_Config>
*       — STANDALONE; no SQL inheritance —
*       wraps redis_connection
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/core/db/redis/redis_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.27
******************************************************************************/

#ifndef DJINTERP_DATABASE_REDIS_TABLE_
#define DJINTERP_DATABASE_REDIS_TABLE_

// std
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../database.hpp"
#include "./redis.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   REDIS KEY-PREFIX UTILITIES
    // =========================================================================

    // redis_default_separator
    //   constant: default separator between namespace prefix and row ID.
    // Redis itself does not reserve any character, but ':' is the
    // community convention for hierarchical key-space layout.
    constexpr char redis_default_separator = ':';

    // redis_index_field
    //   constant: reserved suffix for the per-table index set key.
    inline const std::string redis_index_field = "_index";

    // make_redis_key
    //   function: composes a Redis key from a prefix, separator, and
    // suffix. Used for row-key and index-key composition.
    inline std::string make_redis_key(const std::string& _prefix,
                                      char               _separator,
                                      const std::string& _suffix)
    {
        std::string result;
        result.reserve(_prefix.size() + 1 + _suffix.size());
        result += _prefix;

        if (!_prefix.empty())
        {
            result += _separator;
        }

        result += _suffix;

        return result;
    }


    // =========================================================================
    // II.  REDIS TABLE
    // =========================================================================

    // redis_table
    //   class: hash-backed "logical table" over Redis. Each row is a
    // Redis hash; the hash's key is composed from this table's
    // key prefix and the row's identifier. Optionally maintains an
    // index set tracking row IDs for efficient enumeration.
    //
    // Storage layout (default separator ':'):
    //   <prefix>:<row_id>   HASH      one row per hash
    //   <prefix>:_index     SET       set of all row IDs (optional)
    //
    // Template parameters:
    //   _Config: optional compile-time configuration tag (defaults to
    //            `void`); reserved for downstream specialisations.
    template<typename _Config = void>
    class redis_table
    {
    private:
        using config_type = _Config;

    public:
        using size_type       = std::size_t;
        using value_type      = value;
        using row_type        = std::map<std::string, std::string>;
        using connection_type = redis_connection;
        using self_type       = redis_table<_Config>;

        using type_support    = redis_type_support;
        using feature_support = redis_feature_support;
        using version_info    = redis_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // redis_table()
        //   constructor: default - empty, disconnected table.
        redis_table()
            : m_connection(nullptr)
            , m_separator(redis_default_separator)
            , m_default_ttl(std::chrono::seconds::zero())
            , m_use_index_set(true)
        {
        }

        // redis_table(connection, key_prefix)
        //   constructor: binds to a Redis connection and key prefix.
        explicit redis_table(
                redis_connection& _conn,
                std::string       _key_prefix
            )
                : m_connection(&_conn)
                , m_key_prefix(std::move(_key_prefix))
                , m_separator(redis_default_separator)
                , m_default_ttl(std::chrono::seconds::zero())
                , m_use_index_set(true)
        {
        }

        // redis_table(connection, key_prefix, ttl)
        //   constructor: binds with a default TTL applied to row hashes
        // on commit.
        explicit redis_table(
                redis_connection&    _conn,
                std::string          _key_prefix,
                std::chrono::seconds _default_ttl
            )
                : m_connection(&_conn)
                , m_key_prefix(std::move(_key_prefix))
                , m_separator(redis_default_separator)
                , m_default_ttl(_default_ttl)
                , m_use_index_set(true)
        {
        }

        ~redis_table() = default;

        // disable copying
        redis_table(const redis_table&)            = delete;
        redis_table& operator=(const redis_table&) = delete;

        // enable moving
        redis_table(redis_table&&) noexcept            = default;
        redis_table& operator=(redis_table&&) noexcept = default;


        // =================================================================
        //  connection access
        // =================================================================

        // get_connection
        //   function: returns a pointer to the bound Redis connection,
        // or nullptr if disconnected.
        redis_connection* get_connection() noexcept
        {
            return m_connection;
        }

        // get_connection (const)
        //   function: const overload of get_connection.
        const redis_connection* get_connection() const noexcept
        {
            return m_connection;
        }

        // set_connection
        //   function: rebinds the table to a different Redis connection.
        void set_connection(redis_connection& _conn) noexcept
        {
            m_connection = &_conn;

            return;
        }

        // is_connected
        //   function: returns true if the table has a live underlying
        // Redis connection.
        bool is_connected() const
        {
            return ( (m_connection != nullptr) &&
                     (m_connection->is_connected()) );
        }


        // =================================================================
        //  key prefix and layout
        // =================================================================

        // set_key_prefix
        //   function: sets the namespace prefix under which row hashes
        // are stored.
        void set_key_prefix(std::string _prefix)
        {
            m_key_prefix = std::move(_prefix);

            return;
        }

        // get_key_prefix
        //   function: returns the current key prefix.
        const std::string& get_key_prefix() const noexcept
        {
            return m_key_prefix;
        }

        // set_separator
        //   function: sets the separator character between prefix and
        // row ID (default ':').
        void set_separator(char _sep) noexcept
        {
            m_separator = _sep;

            return;
        }

        // get_separator
        //   function: returns the current separator character.
        char get_separator() const noexcept
        {
            return m_separator;
        }

        // row_key
        //   function: composes the full Redis key for a row with the
        // given ID.
        std::string row_key(const std::string& _row_id) const
        {
            return make_redis_key(m_key_prefix,
                                  m_separator,
                                  _row_id);
        }

        // index_key
        //   function: composes the full Redis key for the index set
        // tracking row IDs in this table.
        std::string index_key() const
        {
            return make_redis_key(m_key_prefix,
                                  m_separator,
                                  redis_index_field);
        }


        // =================================================================
        //  TTL configuration
        // =================================================================

        // set_default_ttl
        //   function: configures a default TTL applied to row hashes on
        // commit. A zero or negative duration disables TTL.
        void set_default_ttl(std::chrono::seconds _ttl) noexcept
        {
            m_default_ttl = _ttl;

            return;
        }

        // get_default_ttl
        //   function: returns the current default TTL.
        std::chrono::seconds get_default_ttl() const noexcept
        {
            return m_default_ttl;
        }


        // =================================================================
        //  index set configuration
        // =================================================================

        // set_use_index_set
        //   function: enables (default) or disables maintenance of the
        // companion index set. Disable when row IDs are derivable from
        // an external source and the SET memory overhead is unwelcome.
        void set_use_index_set(bool _enabled) noexcept
        {
            m_use_index_set = _enabled;

            return;
        }

        // uses_index_set
        //   function: returns whether the index set is maintained.
        bool uses_index_set() const noexcept
        {
            return m_use_index_set;
        }


        // =================================================================
        //  row operations
        // =================================================================

        // set_row
        //   function: writes a row to Redis under <prefix>:<row_id>.
        // Applies the default TTL if set, and updates the index set
        // when enabled.
        void set_row(const std::string& _row_id,
                     const row_type&    _row)
        {
            validate_connected("set_row");

            const std::string key = row_key(_row_id);

            // write each field to the hash
            for (const auto& field : _row)
            {
                m_connection->hset(key,
                                   field.first,
                                   field.second);
            }

            // apply TTL if configured
            if (m_default_ttl.count() > 0)
            {
                m_connection->expire(key,
                                     m_default_ttl.count());
            }

            // update index set if enabled
            if (m_use_index_set)
            {
                m_connection->sadd(index_key(), _row_id);
            }

            return;
        }

        // get_row
        //   function: reads a row from Redis. Returns an empty map if
        // the row does not exist.
        row_type get_row(const std::string& _row_id) const
        {
            validate_connected("get_row");

            return m_connection->hgetall(row_key(_row_id));
        }

        // delete_row
        //   function: removes a row from Redis. Also removes it from
        // the index set when enabled.
        bool delete_row(const std::string& _row_id)
        {
            validate_connected("delete_row");

            const std::string key = row_key(_row_id);
            const auto        n   = m_connection->del(key);

            if (m_use_index_set)
            {
                m_connection->srem(index_key(), _row_id);
            }

            return (n > 0);
        }

        // row_exists
        //   function: returns true if a row with the given ID exists.
        bool row_exists(const std::string& _row_id) const
        {
            validate_connected("row_exists");

            return m_connection->key_exists(row_key(_row_id));
        }

        // get_field
        //   function: reads a single field from a row.
        std::optional<std::string> get_field(
                const std::string& _row_id,
                const std::string& _field
            ) const
        {
            validate_connected("get_field");

            return m_connection->hget(row_key(_row_id),
                                      _field);
        }

        // set_field
        //   function: writes a single field into a row.
        void set_field(const std::string& _row_id,
                       const std::string& _field,
                       const std::string& _value)
        {
            validate_connected("set_field");

            m_connection->hset(row_key(_row_id),
                               _field,
                               _value);

            if (m_use_index_set)
            {
                m_connection->sadd(index_key(), _row_id);
            }

            return;
        }


        // =================================================================
        //  enumeration / scanning
        // =================================================================

        // row_ids
        //   function: returns the set of all row IDs in this table.
        // Uses the index set when enabled; otherwise falls back to a
        // SCAN over <prefix>:* (slower, may include keys other than
        // row hashes if naming collides).
        std::vector<std::string> row_ids() const
        {
            validate_connected("row_ids");

            if (m_use_index_set)
            {
                return m_connection->smembers(index_key());
            }

            // fallback: scan the key prefix and strip it from each
            // key to recover the row ID.
            return scan_row_ids_from_keys();
        }

        // row_count_remote
        //   function: returns the number of rows in this table.
        // Reads from the index set when enabled; otherwise scans.
        size_type row_count_remote() const
        {
            return row_ids().size();
        }

        // scan_keys
        //   function: cursor-based scan of all keys matching the table
        // prefix. Useful for inspecting the raw key-space without going
        // through the index set abstraction.
        std::vector<std::string> scan_keys(
                const std::string& _suffix_pattern = "*"
            ) const
        {
            validate_connected("scan_keys");

            const std::string pattern =
                make_redis_key(m_key_prefix,
                               m_separator,
                               _suffix_pattern);

            std::vector<std::string> result;
            std::int64_t             cursor = 0;

            do
            {
                auto batch = m_connection->scan(cursor, pattern);
                cursor     = batch.first;

                for (auto& k : batch.second)
                {
                    result.push_back(std::move(k));
                }
            }
            while (cursor != 0);

            return result;
        }


        // =================================================================
        //  bulk operations
        // =================================================================

        // refresh
        //   function: read-only sync from Redis. Loads all rows under
        // the table's prefix into the in-memory cache.
        void refresh()
        {
            validate_connected("refresh");

            m_cache.clear();

            const auto ids = row_ids();

            // ensure capacity to amortise the upcoming inserts; map
            // is node-based so there is no exact equivalent of reserve,
            // but allocating up-front keeps the operation tidy.
            for (const auto& id : ids)
            {
                m_cache.emplace(id, m_connection->hgetall(row_key(id)));
            }

            return;
        }

        // commit
        //   function: writes all in-memory rows back to Redis.
        // Each row is HSET as a single hash; TTL and index-set are
        // applied per set_row().
        void commit()
        {
            validate_connected("commit");

            for (const auto& entry : m_cache)
            {
                set_row(entry.first, entry.second);
            }

            return;
        }

        // clear_namespace
        //   function: deletes every key under the table's prefix
        // (including the index set). DESTRUCTIVE. Uses SCAN to avoid
        // blocking the server.
        void clear_namespace()
        {
            validate_connected("clear_namespace");

            const auto keys = scan_keys("*");

            for (const auto& key : keys)
            {
                m_connection->del(key);
            }

            // also clear the index set explicitly in case the scan
            // pattern did not catch it.
            m_connection->del(index_key());

            m_cache.clear();

            return;
        }


        // =================================================================
        //  in-memory cache access
        // =================================================================

        // cache
        //   function: returns a mutable reference to the in-memory row
        // cache populated by refresh() and consumed by commit().
        std::map<std::string, row_type>& cache() noexcept
        {
            return m_cache;
        }

        // cache (const)
        //   function: const overload of cache().
        const std::map<std::string, row_type>& cache() const noexcept
        {
            return m_cache;
        }

        // row_count
        //   function: number of rows currently held in the in-memory
        // cache. Separate from row_count_remote() which queries Redis.
        size_type row_count() const noexcept
        {
            return m_cache.size();
        }


        // =================================================================
        //  compile-time feature queries
        // =================================================================

        // supports_streams
        //   function: returns whether streams (XADD/XREAD) are
        // available.
        static constexpr bool supports_streams() noexcept
        {
            return type_support::has_streams;
        }

        // supports_transactions
        //   function: returns whether multi-key transactions are
        // available.
        static constexpr bool supports_transactions() noexcept
        {
            return feature_support::has_transactions;
        }

        // supports_resp3
        //   function: returns whether RESP3 wire protocol is available.
        static constexpr bool supports_resp3() noexcept
        {
            return feature_support::has_resp3;
        }

        // supports_modules
        //   function: returns whether Redis Modules are available.
        static constexpr bool supports_modules() noexcept
        {
            return feature_support::has_modules;
        }


    protected:

        // =================================================================
        //  protected helpers
        // =================================================================

        // validate_connected
        //   function: throws if no connection is bound or the bound
        // connection is not live. Naming and behaviour mirror the
        // database_table convention so call sites read consistently.
        void validate_connected(const char* _op) const
        {
            if (m_connection == nullptr)
            {
                throw connection_exception(
                    std::string("redis_table::")
                    + _op
                    + ": no connection bound.");
            }

            if (!m_connection->is_connected())
            {
                throw connection_exception(
                    std::string("redis_table::")
                    + _op
                    + ": connection is not live.");
            }

            return;
        }

        // scan_row_ids_from_keys
        //   function: fallback row-ID enumeration when the index set is
        // disabled. SCANs <prefix>:* and strips the prefix from each key.
        std::vector<std::string> scan_row_ids_from_keys() const
        {
            const auto        keys     = scan_keys("*");
            const std::size_t skip_len =
                ( m_key_prefix.empty()
                  ? 0
                  : (m_key_prefix.size() + 1) );

            std::vector<std::string> result;
            result.reserve(keys.size());

            for (const auto& k : keys)
            {
                // skip the reserved index set
                if (k.size() <= skip_len)
                {
                    continue;
                }

                std::string id = k.substr(skip_len);

                if (id == redis_index_field)
                {
                    continue;
                }

                result.push_back(std::move(id));
            }

            return result;
        }


        // =================================================================
        //  protected members
        // =================================================================

        redis_connection*               m_connection;
        std::string                     m_key_prefix;
        char                            m_separator;
        std::chrono::seconds            m_default_ttl;
        bool                            m_use_index_set;
        std::map<std::string, row_type> m_cache;
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_REDIS_TABLE_
