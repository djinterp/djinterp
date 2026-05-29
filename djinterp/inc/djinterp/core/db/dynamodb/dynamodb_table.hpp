/******************************************************************************
* djinterp [database]                                       dynamodb_table.hpp
*
* djinterp DynamoDB table module:
*   A typed wrapper over a single native Amazon DynamoDB table. Unlike
* SQL back-ends, DynamoDB is a key-value / document store: it has no SQL
* dialect (PartiQL aside), no INFORMATION_SCHEMA, no identifier quoting,
* and paginates with a LastEvaluatedKey cursor rather than LIMIT/OFFSET.
* The note in `database_table.hpp` placing key-value / document stores
* out of scope for the SQL-based `database_table<>` hierarchy therefore
* applies to DynamoDB as well, so this header models a table using
* DynamoDB's native primitives instead of inheriting from
* `database_table<>`:
*   - the wrapper binds to one real DynamoDB table by name
*   - the table's key schema (partition key, plus an optional sort key)
*     is held alongside the connection
*   - each "row" is a native DynamoDB item (an attribute map)
*   - reads honour a configurable default read consistency
*   - an in-memory cache mirrors `refresh()` / `commit()` semantics for
*     batch round-trips
*
*   This makes `dynamodb_table` a STANDALONE class. It does NOT extend
* `database_table<>` because that template's contract (SQL queries,
* schema introspection via INFORMATION_SCHEMA, identifier quoting) does
* not apply to DynamoDB. Because DynamoDB has genuine native tables, the
* wrapper is a thin typed layer over a real table rather than a synthesised
* one (contrast `redis_table`, which fabricates a table from hashes). It
* still exposes a similar API surface (`refresh`, `commit`, `set_row`,
* `get_row`, `row_count`, ...) so that code generic over storage back-ends
* can constrain on the `database_table` concept where applicable and on
* more DynamoDB-specific concepts where finer-grained behaviour is needed.
*
*   LAYER DIAGRAM:
*     dynamodb_table<_Config>
*       — STANDALONE; no SQL inheritance —
*       wraps dynamodb_connection
*
*   PORTABILITY:
*   Requires C++17 or later.
*
*
* path:      /inc/djinterp/core/db/dynamodb/dynamodb_table.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_DYNAMODB_TABLE_
#define DJINTERP_DATABASE_DYNAMODB_TABLE_

// std
#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../database.hpp"
#include "./dynamodb.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   DYNAMODB COMPOSITE-KEY UTILITIES
    // =========================================================================

    // dynamodb_default_key_separator
    //   constant: default separator joining partition and sort key
    // values into a single composite cache key. DynamoDB itself does
    // not reserve any character; '#' is the community convention for
    // single-table composite key layout.
    constexpr char dynamodb_default_key_separator = '#';

    // make_dynamodb_cache_key
    //   function: composes a composite cache key from a partition value
    // and an optional sort value.
    inline std::string make_dynamodb_cache_key(
        const std::string& _partition_value,
        char               _separator,
        const std::string& _sort_value)
    {
        if (_sort_value.empty())
        {
            return _partition_value;
        }

        std::string result;
        result.reserve(_partition_value.size()
                       + 1
                       + _sort_value.size());
        result += _partition_value;
        result += _separator;
        result += _sort_value;

        return result;
    }


    // =========================================================================
    // II.  DYNAMODB TABLE
    // =========================================================================

    // dynamodb_table
    //   class: typed wrapper over a single native DynamoDB table. Holds
    // the table name and key schema (partition key plus an optional sort
    // key) alongside a bound dynamodb_connection. Rows are native items
    // (attribute maps); an in-memory cache supports refresh()/commit()
    // batch round-trips.
    //
    // Storage layout:
    //   one native DynamoDB table; the primary key is
    //   (<partition_key>) or (<partition_key>, <sort_key>).
    //
    // Template parameters:
    //   _Config: optional compile-time configuration tag (defaults to
    //            `void`); reserved for downstream specialisations.
    template<typename _Config = void>
    class dynamodb_table
    {
    private:
        using config_type = _Config;

    public:
        using size_type       = std::size_t;
        using value_type      = value;
        using item_type       = dynamodb_item;
        using key_type        = dynamodb_key;
        using row_type        = dynamodb_item;
        using connection_type = dynamodb_connection;
        using self_type       = dynamodb_table<_Config>;

        using type_support    = dynamodb_type_support;
        using feature_support = dynamodb_feature_support;
        using version_info    = dynamodb_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // dynamodb_table()
        //   constructor: default - empty, disconnected table.
        dynamodb_table()
            : m_connection(nullptr)
            , m_separator(dynamodb_default_key_separator)
            , m_consistency(dynamodb_read_consistency::eventual)
        {
        }

        // dynamodb_table(connection, table_name)
        //   constructor: binds to a DynamoDB connection and table name
        // with an unspecified key schema (set later via
        // set_key_schema()).
        explicit dynamodb_table(
                dynamodb_connection& _conn,
                std::string          _table_name
            )
                : m_connection(&_conn)
                , m_table_name(std::move(_table_name))
                , m_separator(dynamodb_default_key_separator)
                , m_consistency(dynamodb_read_consistency::eventual)
        {
        }

        // dynamodb_table(connection, table_name, partition_key, sort_key)
        //   constructor: binds with an explicit key schema. An empty
        // _sort_key denotes a simple (partition-only) key schema.
        explicit dynamodb_table(
                dynamodb_connection& _conn,
                std::string          _table_name,
                std::string          _partition_key,
                std::string          _sort_key = std::string()
            )
                : m_connection(&_conn)
                , m_table_name(std::move(_table_name))
                , m_partition_key(std::move(_partition_key))
                , m_sort_key(std::move(_sort_key))
                , m_separator(dynamodb_default_key_separator)
                , m_consistency(dynamodb_read_consistency::eventual)
        {
        }

        ~dynamodb_table() = default;

        // disable copying
        dynamodb_table(const dynamodb_table&)            = delete;
        dynamodb_table& operator=(const dynamodb_table&) = delete;

        // enable moving
        dynamodb_table(dynamodb_table&&) noexcept            = default;
        dynamodb_table& operator=(dynamodb_table&&) noexcept = default;


        // =================================================================
        //  connection access
        // =================================================================

        // get_connection
        //   function: returns a pointer to the bound DynamoDB
        // connection, or nullptr if disconnected.
        dynamodb_connection* get_connection() noexcept
        {
            return m_connection;
        }

        // get_connection (const)
        //   function: const overload of get_connection.
        const dynamodb_connection* get_connection() const noexcept
        {
            return m_connection;
        }

        // set_connection
        //   function: rebinds the table to a different DynamoDB
        // connection.
        void set_connection(dynamodb_connection& _conn) noexcept
        {
            m_connection = &_conn;

            return;
        }

        // is_connected
        //   function: returns true if the table has a live underlying
        // DynamoDB connection.
        bool is_connected() const
        {
            return ( (m_connection != nullptr) &&
                     (m_connection->is_connected()) );
        }


        // =================================================================
        //  table name and key schema
        // =================================================================

        // set_table_name
        //   function: sets the bound table name.
        void set_table_name(std::string _table_name)
        {
            m_table_name = std::move(_table_name);

            return;
        }

        // get_table_name
        //   function: returns the bound table name.
        const std::string& get_table_name() const noexcept
        {
            return m_table_name;
        }

        // set_key_schema
        //   function: sets the partition key name and an optional sort
        // key name. An empty _sort_key denotes a simple key schema.
        void set_key_schema(std::string _partition_key,
                            std::string _sort_key = std::string())
        {
            m_partition_key = std::move(_partition_key);
            m_sort_key      = std::move(_sort_key);

            return;
        }

        // get_partition_key
        //   function: returns the partition key attribute name.
        const std::string& get_partition_key() const noexcept
        {
            return m_partition_key;
        }

        // get_sort_key
        //   function: returns the sort key attribute name (empty for a
        // simple key schema).
        const std::string& get_sort_key() const noexcept
        {
            return m_sort_key;
        }

        // has_sort_key
        //   function: returns true if the table uses a composite
        // (partition + sort) key schema.
        bool has_sort_key() const noexcept
        {
            return (!m_sort_key.empty());
        }

        // set_separator
        //   function: sets the separator joining partition and sort key
        // values into composite cache keys (default '#').
        void set_separator(char _sep) noexcept
        {
            m_separator = _sep;

            return;
        }

        // get_separator
        //   function: returns the composite cache-key separator.
        char get_separator() const noexcept
        {
            return m_separator;
        }


        // =================================================================
        //  key composition
        // =================================================================

        // build_key
        //   function: composes a DynamoDB primary key from a partition
        // value alone (simple key schema).
        dynamodb_key build_key(const std::string& _partition_value) const
        {
            dynamodb_key key;
            key.emplace(m_partition_key,
                        value{_partition_value});

            return key;
        }

        // build_key
        //   function: composes a DynamoDB primary key from a partition
        // value and a sort value (composite key schema).
        dynamodb_key build_key(const std::string& _partition_value,
                               const std::string& _sort_value) const
        {
            dynamodb_key key;
            key.emplace(m_partition_key,
                        value{_partition_value});
            key.emplace(m_sort_key,
                        value{_sort_value});

            return key;
        }


        // =================================================================
        //  read consistency configuration
        // =================================================================

        // set_read_consistency
        //   function: sets the default read consistency for this table's
        // reads.
        void set_read_consistency(dynamodb_read_consistency _c) noexcept
        {
            m_consistency = _c;

            return;
        }

        // get_read_consistency
        //   function: returns the default read consistency.
        dynamodb_read_consistency get_read_consistency() const noexcept
        {
            return m_consistency;
        }


        // =================================================================
        //  row operations (simple key schema)
        // =================================================================

        // set_row
        //   function: writes (or replaces) an item under the given
        // partition value.
        void set_row(const std::string& _partition_value,
                     const item_type&   _item)
        {
            validate_connected("set_row");

            m_connection->put_item(m_table_name, _item);

            return;
        }

        // get_row
        //   function: reads an item by partition value. Returns an empty
        // optional if the item does not exist.
        std::optional<item_type> get_row(
                const std::string& _partition_value
            ) const
        {
            validate_connected("get_row");

            return m_connection->get_item(m_table_name,
                                          build_key(_partition_value));
        }

        // delete_row
        //   function: removes an item by partition value.
        bool delete_row(const std::string& _partition_value)
        {
            validate_connected("delete_row");

            return m_connection->delete_item(
                m_table_name,
                build_key(_partition_value));
        }

        // row_exists
        //   function: returns true if an item with the given partition
        // value exists.
        bool row_exists(const std::string& _partition_value) const
        {
            validate_connected("row_exists");

            return m_connection->get_item(
                m_table_name,
                build_key(_partition_value)).has_value();
        }


        // =================================================================
        //  row operations (composite key schema)
        // =================================================================

        // get_row
        //   function: reads an item by partition + sort value.
        std::optional<item_type> get_row(
                const std::string& _partition_value,
                const std::string& _sort_value
            ) const
        {
            validate_connected("get_row");

            return m_connection->get_item(
                m_table_name,
                build_key(_partition_value, _sort_value));
        }

        // delete_row
        //   function: removes an item by partition + sort value.
        bool delete_row(const std::string& _partition_value,
                        const std::string& _sort_value)
        {
            validate_connected("delete_row");

            return m_connection->delete_item(
                m_table_name,
                build_key(_partition_value, _sort_value));
        }

        // row_exists
        //   function: returns true if an item with the given partition +
        // sort value exists.
        bool row_exists(const std::string& _partition_value,
                        const std::string& _sort_value) const
        {
            validate_connected("row_exists");

            return m_connection->get_item(
                m_table_name,
                build_key(_partition_value, _sort_value)).has_value();
        }


        // =================================================================
        //  field operations
        // =================================================================

        // get_field
        //   function: reads a single attribute from the item identified
        // by partition value. Returns an empty optional if the item or
        // attribute does not exist.
        std::optional<value> get_field(
                const std::string& _partition_value,
                const std::string& _field
            ) const
        {
            validate_connected("get_field");

            const auto item = m_connection->get_item(
                m_table_name,
                build_key(_partition_value));

            if (!item.has_value())
            {
                return std::nullopt;
            }

            const auto it = item->find(_field);

            if (it == item->end())
            {
                return std::nullopt;
            }

            return it->second;
        }

        // set_field
        //   function: writes a single attribute into the item identified
        // by partition value via an UpdateItem call.
        void set_field(const std::string& _partition_value,
                       const std::string& _field,
                       const value&       _value)
        {
            validate_connected("set_field");

            item_type updates;
            updates.emplace(_field, _value);

            m_connection->update_item(m_table_name,
                                      build_key(_partition_value),
                                      updates);

            return;
        }


        // =================================================================
        //  enumeration / scanning
        // =================================================================

        // scan_all
        //   function: reads every item in the table, following the
        // LastEvaluatedKey cursor across pages. WARNING: a full Scan
        // consumes read capacity proportional to table size; prefer
        // query_partition() when the partition is known.
        std::vector<item_type> scan_all() const
        {
            validate_connected("scan_all");

            std::vector<item_type> result;
            auto                   page = m_connection->scan(m_table_name);

            while (true)
            {
                for (auto& item : page.first)
                {
                    result.push_back(std::move(item));
                }

                // no continuation cursor -> final page
                if (!page.second.has_value())
                {
                    break;
                }

                page = m_connection->scan(m_table_name);
            }

            return result;
        }

        // query_partition
        //   function: returns the items sharing the given partition key
        // value, following pagination across pages. Builds a key
        // condition expression over the partition key attribute.
        std::vector<item_type> query_partition(
                const std::string& _partition_value
            ) const
        {
            validate_connected("query_partition");

            const std::string condition =
                m_partition_key + " = " + _partition_value;

            std::vector<item_type> result;
            auto                   page =
                m_connection->query(m_table_name, condition);

            while (true)
            {
                for (auto& item : page.first)
                {
                    result.push_back(std::move(item));
                }

                if (!page.second.has_value())
                {
                    break;
                }

                page = m_connection->query(m_table_name, condition);
            }

            return result;
        }

        // row_count_remote
        //   function: returns the number of items in the table by
        // scanning. Expensive on large tables; DynamoDB's item_count in
        // DescribeTable is only updated periodically, so a scan is used
        // here for an exact figure.
        size_type row_count_remote() const
        {
            return scan_all().size();
        }


        // =================================================================
        //  bulk operations
        // =================================================================

        // refresh
        //   function: read-only sync from DynamoDB. Scans the whole
        // table into the in-memory cache, keyed by the composite of the
        // item's partition (and sort) attribute values.
        void refresh()
        {
            validate_connected("refresh");

            m_cache.clear();

            const auto items = scan_all();

            for (const auto& item : items)
            {
                m_cache.emplace(cache_key_for(item), item);
            }

            return;
        }

        // commit
        //   function: writes all cached items back to DynamoDB. Uses
        // BatchWriteItem, which caps each request at 25 items; the
        // connection's _impl is expected to chunk larger batches.
        void commit()
        {
            validate_connected("commit");

            std::vector<item_type> items;
            items.reserve(m_cache.size());

            for (const auto& entry : m_cache)
            {
                items.push_back(entry.second);
            }

            if (!items.empty())
            {
                m_connection->batch_write_item(m_table_name, items);
            }

            return;
        }

        // clear_table
        //   function: deletes every item in the table by scanning and
        // issuing a DeleteItem per item. DESTRUCTIVE. DynamoDB has no
        // TRUNCATE; for very large tables dropping and recreating the
        // table is cheaper than a per-item delete.
        void clear_table()
        {
            validate_connected("clear_table");

            const auto items = scan_all();

            for (const auto& item : items)
            {
                m_connection->delete_item(m_table_name,
                                          key_from_item(item));
            }

            m_cache.clear();

            return;
        }


        // =================================================================
        //  in-memory cache access
        // =================================================================

        // cache
        //   function: returns a mutable reference to the in-memory item
        // cache populated by refresh() and consumed by commit().
        std::map<std::string, item_type>& cache() noexcept
        {
            return m_cache;
        }

        // cache (const)
        //   function: const overload of cache().
        const std::map<std::string, item_type>& cache() const noexcept
        {
            return m_cache;
        }

        // row_count
        //   function: number of items currently held in the in-memory
        // cache. Separate from row_count_remote() which scans DynamoDB.
        size_type row_count() const noexcept
        {
            return m_cache.size();
        }


        // =================================================================
        //  compile-time feature queries
        // =================================================================

        // supports_transactions
        //   function: returns whether transactions are available.
        static constexpr bool supports_transactions() noexcept
        {
            return feature_support::has_transactions;
        }

        // supports_streams
        //   function: returns whether DynamoDB Streams are available.
        static constexpr bool supports_streams() noexcept
        {
            return feature_support::has_streams;
        }

        // supports_partiql
        //   function: returns whether PartiQL is available.
        static constexpr bool supports_partiql() noexcept
        {
            return feature_support::has_partiql;
        }

        // supports_global_tables
        //   function: returns whether global tables are available.
        static constexpr bool supports_global_tables() noexcept
        {
            return feature_support::has_global_tables;
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
                    std::string("dynamodb_table::")
                    + _op
                    + ": no connection bound.");
            }

            if (!m_connection->is_connected())
            {
                throw connection_exception(
                    std::string("dynamodb_table::")
                    + _op
                    + ": connection is not live.");
            }

            return;
        }

        // value_to_key_string
        //   function: stringifies a key attribute value. DynamoDB key
        // attributes are restricted to S / N / B; this covers the
        // string and numeric alternatives of the generic value variant.
        static std::string value_to_key_string(const value& _v)
        {
            if (std::holds_alternative<std::string>(_v))
            {
                return std::get<std::string>(_v);
            }

            if (std::holds_alternative<std::int32_t>(_v))
            {
                return std::to_string(std::get<std::int32_t>(_v));
            }

            if (std::holds_alternative<std::int64_t>(_v))
            {
                return std::to_string(std::get<std::int64_t>(_v));
            }

            if (std::holds_alternative<double>(_v))
            {
                return std::to_string(std::get<double>(_v));
            }

            if (std::holds_alternative<bool>(_v))
            {
                return ( std::get<bool>(_v)
                         ? std::string("true")
                         : std::string("false") );
            }

            return std::string();
        }

        // cache_key_for
        //   function: derives the composite cache key for an item from
        // its partition (and sort) attribute values.
        std::string cache_key_for(const item_type& _item) const
        {
            std::string partition_value;
            std::string sort_value;

            const auto pit = _item.find(m_partition_key);

            if (pit != _item.end())
            {
                partition_value = value_to_key_string(pit->second);
            }

            if (has_sort_key())
            {
                const auto sit = _item.find(m_sort_key);

                if (sit != _item.end())
                {
                    sort_value = value_to_key_string(sit->second);
                }
            }

            return make_dynamodb_cache_key(partition_value,
                                           m_separator,
                                           sort_value);
        }

        // key_from_item
        //   function: extracts the primary key (partition, plus sort
        // when present) from a full item.
        dynamodb_key key_from_item(const item_type& _item) const
        {
            dynamodb_key key;

            const auto pit = _item.find(m_partition_key);

            if (pit != _item.end())
            {
                key.emplace(m_partition_key, pit->second);
            }

            if (has_sort_key())
            {
                const auto sit = _item.find(m_sort_key);

                if (sit != _item.end())
                {
                    key.emplace(m_sort_key, sit->second);
                }
            }

            return key;
        }


        // =================================================================
        //  protected members
        // =================================================================

        dynamodb_connection*             m_connection;
        std::string                      m_table_name;
        std::string                      m_partition_key;
        std::string                      m_sort_key;
        char                             m_separator;
        dynamodb_read_consistency        m_consistency;
        std::map<std::string, item_type> m_cache;
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_DYNAMODB_TABLE_
