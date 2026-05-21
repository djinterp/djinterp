/******************************************************************************
* djinterp [database]                                           arango_table.hpp
*
* djinterp ArangoDB table module:
*   ArangoDB-specific database_table subclass providing a tabular
* projection over an ArangoDB collection. ArangoDB is a multi-model
* database (document, key/value, graph), and this module treats the
* collection as the table analogue:
*   - each collection maps to one logical table
*   - each document maps to one row, with _key / _id / _rev as system
*     columns
*   - schema type describes the client-side projection; the collection
*     itself remains schema-less unless a schema validator is installed
*   - refresh() issues an AQL query (FOR doc IN collection RETURN doc)
*     with client-side filter/sort/limit clauses, or a custom AQL if one
*     is set
*   - edge-collection awareness: when collection_kind is edge, _from and
*     _to are preserved and exposed as first-class columns
*
*   Vendor features beyond the generic database_table base:
*   - document / edge collection kind distinction
*   - AQL binding and custom AQL refresh
*   - wait-for-sync and synchronous-replication hints on commit
*   - stream-transaction participation
*   - named graph awareness for edge collections
*   - return-new / return-old flags on commit-generated writes
*
*   LAYER DIAGRAM:
*     arango_table<_Config>
*       -> database_table<arango_connection, value, _Config>
*
*   NOTE: this header forward-declares arango_connection. The concrete
* class definition lives in arangodb.hpp. Include arangodb.hpp before
* constructing instances.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
* 
* path:      /inc/djinterp/core/db/arangodb/arango_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.23
******************************************************************************/

#ifndef DJINTERP_DATABASE_ARANGO_TABLE_
#define DJINTERP_DATABASE_ARANGO_TABLE_

// djinterp
#include "../../../djinterp.hpp"
#include "./arangodb.hpp"
#include "../database_table.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   ARANGODB COLLECTION KIND
    // =========================================================================

    // arango_collection_kind
    //   enumeration: distinguishes document collections (vertices /
    // plain documents) from edge collections (graph edges with _from
    // and _to system attributes).
    enum class arango_collection_kind
    {
        document,
        edge
    };


    // =========================================================================
    // II.  ARANGO TABLE
    // =========================================================================

    // arango_connection
    //   class: forward declaration of the ArangoDB connection
    // implementation. Defined in arangodb.hpp.
    class arango_connection;

    // arango_table
    //   class: ArangoDB-collection-backed database table. Supports
    // both document and edge collections, with AQL-backed refresh and
    // graph-aware edge handling.
    template<typename _Config = void>
    class arango_table
        : public database_table<arango_connection,
                                value,
                                _Config>
    {
    private:
        using base_type = database_table<arango_connection,
                                         value,
                                         _Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = arango_table<_Config>;

        using type_support    = arango_type_support;
        using feature_support = arango_feature_support;
        using version_info    = arango_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // arango_table()
        //   constructor: default - empty, disconnected table.
        arango_table()
            : base_type()
            , m_collection_kind(arango_collection_kind::document)
            , m_wait_for_sync(false)
            , m_return_new(false)
            , m_return_old(false)
        {
        }

        // arango_table(connection, name)
        //   constructor: binds to an ArangoDB connection and
        // collection name.
        explicit arango_table(
                arango_connection& _conn,
                std::string        _collection_name,
                table_kind         _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_collection_name),
                            _kind)
                , m_collection_kind(arango_collection_kind::document)
                , m_wait_for_sync(false)
                , m_return_new(false)
                , m_return_old(false)
        {
        }

        // arango_table(connection, schema)
        //   constructor: binds with an explicit projection schema.
        explicit arango_table(
                arango_connection& _conn,
                table_schema       _schema,
                table_kind         _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
                , m_collection_kind(arango_collection_kind::document)
                , m_wait_for_sync(false)
                , m_return_new(false)
                , m_return_old(false)
        {
        }

        // arango_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit arango_table(
                arango_connection& _conn,
                table_schema       _schema,
                table_kind         _kind,
                const sync_config& _sync
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind,
                            _sync)
                , m_collection_kind(arango_collection_kind::document)
                , m_wait_for_sync(false)
                , m_return_new(false)
                , m_return_old(false)
        {
        }

        ~arango_table() override = default;

        // disable copying
        arango_table(const arango_table&)            = delete;
        arango_table& operator=(const arango_table&) = delete;

        // enable moving
        arango_table(arango_table&&) noexcept            = default;
        arango_table& operator=(arango_table&&) noexcept = default;


        // =================================================================
        //  collection identity and kind
        // =================================================================

        // collection_name
        //   function: returns the collection name. Alias for the
        // base table_name for ArangoDB idiom.
        const std::string& collection_name() const noexcept
        {
            return this->m_schema.table_name;
        }

        // set_collection_kind
        //   function: marks this table as backing a document or edge
        // collection. Edge collections require _from and _to system
        // attributes on every document.
        void set_collection_kind(arango_collection_kind _kind) noexcept
        {
            m_collection_kind = _kind;

            return;
        }

        // get_collection_kind
        //   function: returns the configured collection kind.
        arango_collection_kind get_collection_kind() const noexcept
        {
            return m_collection_kind;
        }

        // is_edge_collection
        //   function: returns whether this table backs an edge
        // collection.
        bool is_edge_collection() const noexcept
        {
            return (m_collection_kind == arango_collection_kind::edge);
        }


        // =================================================================
        //  AQL binding and custom refresh
        // =================================================================

        // set_aql_query
        //   function: sets a custom AQL query used in place of the
        // default "FOR doc IN <collection> RETURN doc" on refresh.
        // Empty clears and reverts to the default.
        void set_aql_query(std::string _aql)
        {
            m_custom_aql = std::move(_aql);

            return;
        }

        // clear_aql_query
        //   function: removes any configured custom AQL.
        void clear_aql_query() noexcept
        {
            m_custom_aql.clear();

            return;
        }

        // get_aql_query
        //   function: returns the configured custom AQL, if any.
        const std::string& get_aql_query() const noexcept
        {
            return m_custom_aql;
        }

        // uses_custom_aql
        //   function: returns whether refresh uses a custom AQL query.
        bool uses_custom_aql() const noexcept
        {
            return !m_custom_aql.empty();
        }

        // set_bind_var_json
        //   function: sets bind variables (as a JSON object) for the
        // configured AQL query. e.g. '{"status": "active", "limit": 100}'.
        void set_bind_var_json(std::string _bind_json)
        {
            m_bind_var_json = std::move(_bind_json);

            return;
        }

        // get_bind_var_json
        //   function: returns the configured bind variables JSON.
        const std::string& get_bind_var_json() const noexcept
        {
            return m_bind_var_json;
        }


        // =================================================================
        //  graph awareness
        // =================================================================

        // set_graph_name
        //   function: associates this table with a named graph.
        // Meaningful only for edge collections participating in a named
        // graph; enables the connection to use graph APIs in preference
        // to raw edge operations.
        void set_graph_name(std::string _graph)
        {
            m_graph_name = std::move(_graph);

            return;
        }

        // get_graph_name
        //   function: returns the associated graph name, if any.
        const std::string& get_graph_name() const noexcept
        {
            return m_graph_name;
        }


        // =================================================================
        //  write modifiers
        // =================================================================

        // set_wait_for_sync
        //   function: configures whether commit operations block until
        // the write is synchronized to disk. Enabling this trades
        // throughput for durability.
        void set_wait_for_sync(bool _enabled) noexcept
        {
            m_wait_for_sync = _enabled;

            return;
        }

        // waits_for_sync
        //   function: returns whether commit waits for disk
        // synchronization.
        bool waits_for_sync() const noexcept
        {
            return m_wait_for_sync;
        }

        // set_return_new
        //   function: configures commit writes to return the newly
        // written document content (useful when server-side computed
        // attributes are in play).
        void set_return_new(bool _enabled) noexcept
        {
            m_return_new = _enabled;

            return;
        }

        // set_return_old
        //   function: configures commit writes to return the previous
        // version of modified or removed documents.
        void set_return_old(bool _enabled) noexcept
        {
            m_return_old = _enabled;

            return;
        }


        // =================================================================
        //  ArangoDB-specific operations
        // =================================================================

        // document_count
        //   function: returns the total count of documents in the
        // backing collection (unfiltered).
        std::int64_t document_count()
        {
            this->validate_connected("document_count");

            return this->m_connection->collection_count(
                collection_name());
        }

        // truncate_collection
        //   function: removes all documents from the collection,
        // preserving indexes and collection metadata.
        void truncate_collection()
        {
            this->validate_connected("truncate_collection");

            this->m_connection->truncate_collection(
                collection_name());

            return;
        }

        // drop_collection
        //   function: drops the backing collection. Analogous to
        // DROP TABLE.
        void drop_collection()
        {
            this->validate_connected("drop_collection");

            this->m_connection->drop_collection(collection_name());

            return;
        }

        // create_index
        //   function: creates an ArangoDB index on the backing
        // collection. _index_json is the full index definition
        // document (e.g. '{"type": "persistent", "fields": ["name"]}').
        void create_index(const std::string& _index_json)
        {
            this->validate_connected("create_index");

            this->m_connection->create_index(
                collection_name(),
                _index_json);

            return;
        }


    protected:

        // =================================================================
        //  protected overrides
        // =================================================================

        // field_type_to_sql
        //   function: ArangoDB has no DDL in the SQL sense; the return
        // value here describes the VelocyPack / JSON type used for
        // schema-validator construction. Maps field_type to
        // ArangoDB schema-validator type names.
        const char* field_type_to_sql(field_type _type) const override
        {
            switch (_type)
            {
                case field_type::boolean:
                    return "boolean";
                case field_type::integer:
                case field_type::big_integer:
                    return "integer";
                case field_type::floating_point:
                case field_type::decimal:
                    return "number";
                case field_type::string:
                    return "string";
                case field_type::binary:
                    // VelocyPack has a binary type; in JSON-schema
                    // validator syntax it maps to string with a format
                    return "string";
                case field_type::date:
                case field_type::datetime:
                case field_type::timestamp:
                    // ArangoDB stores dates as ISO-8601 strings by
                    // convention
                    return "string";
                case field_type::time:
                    return "string";
                case field_type::json:
                    return "object";
                case field_type::xml:
                    return "string";
                case field_type::uuid:
                    return "string";
                case field_type::array:
                    return "array";
                case field_type::null:
                    return "null";
                case field_type::custom:
                default:
                    return base_type::field_type_to_sql(_type);
            }
        }


        // =================================================================
        //  protected members
        // =================================================================

        std::string            m_custom_aql;
        std::string            m_bind_var_json;
        std::string            m_graph_name;
        arango_collection_kind m_collection_kind;
        bool                   m_wait_for_sync;
        bool                   m_return_new;
        bool                   m_return_old;
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ARANGO_TABLE_
