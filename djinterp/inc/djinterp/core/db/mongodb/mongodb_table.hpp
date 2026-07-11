/******************************************************************************
* djinterp [database]                                          mongo_table.hpp
*
* djinterp MongoDB table module:
*   MongoDB-specific database_table subclass providing a tabular projection
* over a MongoDB collection. MongoDB is document-oriented with no SQL
* schema, so this module bridges the two models:
*   - each MongoDB collection maps to one logical table
*   - each document maps to one row
*   - the table schema defines a projection (which BSON fields become
*     which columns), with type coercion driven by field_type
*   - refresh() issues a find() with the projection applied
*   - commit() issues bulk_write upserts keyed on the primary field
*
*   Vendor features beyond the generic database_table base:
*   - collection qualification (database.collection)
*   - explicit projection specification (which fields → which columns)
*   - read / write concern configuration
*   - per-operation filter and sort specification
*   - aggregation-pipeline helpers for $lookup, $group, $unwind
*   - index creation and hint specification
*   - change-stream invalidation (for sync policies using on_access)
*
*   LAYER DIAGRAM:
*     mongo_table<_Config>
*       -> database_table<mongo_connection, value, _Config>
*
*   NOTE: MongoDB's "table schema" is a client-side construct. The
* schema_type describes the projection; the actual collection remains
* schema-less on the server. Writes that stray from the schema are
* permitted by the server; only the client-side table representation
* is constrained.
*
*   NOTE: this header forward-declares mongo_connection. The concrete
* class definition lives in mongodb.hpp. Include mongodb.hpp before
* constructing instances.
*
*   PORTABILITY:
*   Requires C++17 or later.
*
* 
* path:      /inc/djinterp/core/db/mongodb/mongo_table.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.23
******************************************************************************/

#ifndef DJINTERP_DATABASE_MONGO_TABLE_
#define DJINTERP_DATABASE_MONGO_TABLE_

// djinterp
#include "../../../djinterp.hpp"
#include "./mongodb.hpp"
#include "../database_table.hpp"


NS_DJINTERP


    // =========================================================================
    // I.   MONGODB READ / WRITE CONCERN
    // =========================================================================

    // mongo_read_concern
    //   enumeration: MongoDB read-concern levels controlling the
    // consistency guarantees of read operations.
    enum class mongo_read_concern
    {
        local,
        available,
        majority,
        linearizable,
        snapshot
    };

    // mongo_write_concern
    //   enumeration: MongoDB write-concern levels controlling
    // acknowledgement semantics for write operations.
    enum class mongo_write_concern
    {
        unacknowledged,
        acknowledged,
        journaled,
        majority
    };


    // =========================================================================
    // II.  MONGO TABLE
    // =========================================================================

    // mongo_connection
    //   class: forward declaration of the MongoDB connection
    // implementation. Defined in mongodb.hpp.
    class mongo_connection;

    // mongo_table
    //   class: MongoDB-collection-backed database table. Extends the
    // generic database_table with projection, filter, sort, concern, and
    // aggregation-pipeline facilities appropriate to a document store.
    template<typename _Config = void>
    class mongo_table
        : public database_table<mongo_connection,
                                value,
                                _Config>
    {
    private:
        using base_type = database_table<mongo_connection,
                                         value,
                                         _Config>;

    public:
        using typename base_type::size_type;
        using typename base_type::value_type;
        using typename base_type::row_type;
        using typename base_type::connection_type;
        using typename base_type::schema_type;
        using self_type = mongo_table<_Config>;

        using type_support    = mongo_type_support;
        using feature_support = mongo_feature_support;
        using version_info    = mongo_version_info;


        // =================================================================
        //  constructors
        // =================================================================

        // mongo_table()
        //   constructor: default - empty, disconnected table.
        mongo_table()
            : base_type()
            , m_read_concern(mongo_read_concern::local)
            , m_write_concern(mongo_write_concern::acknowledged)
        {
        }

        // mongo_table(connection, name)
        //   constructor: binds to a MongoDB connection and collection
        // name (interpreted as the table name).
        explicit mongo_table(
                mongo_connection& _conn,
                std::string       _collection_name,
                table_kind        _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_collection_name),
                            _kind)
                , m_read_concern(mongo_read_concern::local)
                , m_write_concern(mongo_write_concern::acknowledged)
        {
        }

        // mongo_table(connection, schema)
        //   constructor: binds with an explicit projection schema.
        explicit mongo_table(
                mongo_connection& _conn,
                table_schema      _schema,
                table_kind        _kind = table_kind::base_table
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind)
                , m_read_concern(mongo_read_concern::local)
                , m_write_concern(mongo_write_concern::acknowledged)
        {
        }

        // mongo_table(connection, schema, sync)
        //   constructor: binds with schema and sync policy.
        explicit mongo_table(
                mongo_connection&  _conn,
                table_schema       _schema,
                table_kind         _kind,
                const sync_config& _sync
            )
                : base_type(_conn,
                            std::move(_schema),
                            _kind,
                            _sync)
                , m_read_concern(mongo_read_concern::local)
                , m_write_concern(mongo_write_concern::acknowledged)
        {
        }

        ~mongo_table() override = default;

        // disable copying
        mongo_table(const mongo_table&)            = delete;
        mongo_table& operator=(const mongo_table&) = delete;

        // enable moving
        mongo_table(mongo_table&&) noexcept            = default;
        mongo_table& operator=(mongo_table&&) noexcept = default;


        // =================================================================
        //  database-and-collection qualification
        // =================================================================

        // set_database_name
        //   function: sets the MongoDB database containing this
        // collection. Empty (default) uses the connection's current
        // database.
        void set_database_name(std::string _db_name)
        {
            m_database_name = std::move(_db_name);

            return;
        }

        // get_database_name
        //   function: returns the configured database name, or empty
        // if using the connection default.
        const std::string& get_database_name() const noexcept
        {
            return m_database_name;
        }

        // collection_name
        //   function: returns the collection name. Alias for the
        // base table_name for MongoDB idiom.
        const std::string& collection_name() const noexcept
        {
            return this->m_schema.table_name;
        }


        // =================================================================
        //  BSON filter and projection
        // =================================================================

        // set_filter_json
        //   function: sets a BSON filter (expressed as JSON) applied to
        // every refresh. e.g. '{"status": "active"}'. Empty string
        // means no filter.
        void set_filter_json(std::string _filter_json)
        {
            m_filter_json = std::move(_filter_json);

            return;
        }

        // get_filter_json
        //   function: returns the active BSON filter, as JSON.
        const std::string& get_filter_json() const noexcept
        {
            return m_filter_json;
        }

        // set_projection_json
        //   function: sets an explicit projection document (as JSON)
        // overriding the projection derived from the table schema.
        // e.g. '{"_id": 1, "name": 1, "age": 1}'.
        void set_projection_json(std::string _projection_json)
        {
            m_projection_json = std::move(_projection_json);

            return;
        }

        // get_projection_json
        //   function: returns the explicit projection document, if set.
        const std::string& get_projection_json() const noexcept
        {
            return m_projection_json;
        }

        // set_sort_json
        //   function: sets a sort document for refresh queries. e.g.
        // '{"created_at": -1}'.
        void set_sort_json(std::string _sort_json)
        {
            m_sort_json = std::move(_sort_json);

            return;
        }

        // get_sort_json
        //   function: returns the sort document, if set.
        const std::string& get_sort_json() const noexcept
        {
            return m_sort_json;
        }


        // =================================================================
        //  read / write concern
        // =================================================================

        // set_read_concern
        //   function: configures the read concern for refresh
        // operations.
        void set_read_concern(mongo_read_concern _concern) noexcept
        {
            m_read_concern = _concern;

            return;
        }

        // get_read_concern
        //   function: returns the configured read concern.
        mongo_read_concern get_read_concern() const noexcept
        {
            return m_read_concern;
        }

        // set_write_concern
        //   function: configures the write concern for commit
        // operations.
        void set_write_concern(mongo_write_concern _concern) noexcept
        {
            m_write_concern = _concern;

            return;
        }

        // get_write_concern
        //   function: returns the configured write concern.
        mongo_write_concern get_write_concern() const noexcept
        {
            return m_write_concern;
        }


        // =================================================================
        //  aggregation pipeline support
        // =================================================================

        // set_aggregation_pipeline
        //   function: configures a full MongoDB aggregation pipeline
        // (as a JSON array of stage documents) to be used in place of a
        // plain find() on refresh. Empty clears and reverts to find().
        // e.g. '[{"$match": {...}}, {"$lookup": {...}}, {"$sort": {...}}]'
        void set_aggregation_pipeline(std::string _pipeline_json)
        {
            m_pipeline_json = std::move(_pipeline_json);

            return;
        }

        // clear_aggregation_pipeline
        //   function: removes any configured aggregation pipeline.
        void clear_aggregation_pipeline() noexcept
        {
            m_pipeline_json.clear();

            return;
        }

        // get_aggregation_pipeline
        //   function: returns the configured pipeline, if any.
        const std::string& get_aggregation_pipeline() const noexcept
        {
            return m_pipeline_json;
        }

        // uses_aggregation
        //   function: returns whether refresh will use an aggregation
        // pipeline instead of a plain find().
        bool uses_aggregation() const noexcept
        {
            return !m_pipeline_json.empty();
        }


        // =================================================================
        //  MongoDB-specific queries
        // =================================================================

        // count_documents
        //   function: returns the count of documents matching the
        // current filter (or all documents if no filter is set). Uses
        // the server-side countDocuments operation, which is accurate
        // but slower than estimated_document_count for large
        // collections.
        std::int64_t count_documents()
        {
            this->validate_connected("count_documents");

            return this->m_connection->count_documents(
                collection_name(),
                m_filter_json);
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
        //   function: creates an index on the backing collection.
        // _keys_json is a BSON keys document (e.g. '{"name": 1}').
        // _options_json is an optional BSON options document.
        void create_index(
                const std::string& _keys_json,
                const std::string& _options_json = std::string())
        {
            this->validate_connected("create_index");

            this->m_connection->create_index(
                collection_name(),
                _keys_json,
                _options_json);

            return;
        }


    protected:

        // =================================================================
        //  protected overrides
        // =================================================================

        // field_type_to_sql
        //   function: MongoDB has no DDL, but subclasses may still
        // request a schema description string for validation-rule
        // generation (via $jsonSchema). This mapping returns BSON type
        // alias names that are valid in validator documents.
        const char* field_type_to_sql(field_type _type) const override
        {
            switch (_type)
            {
                case field_type::boolean:
                    return "bool";
                case field_type::integer:
                    return "int";
                case field_type::big_integer:
                    return "long";
                case field_type::floating_point:
                    return "double";
                case field_type::decimal:
                    return "decimal";
                case field_type::string:
                    return "string";
                case field_type::binary:
                    return "binData";
                case field_type::date:
                case field_type::datetime:
                case field_type::timestamp:
                    return "date";
                case field_type::time:
                    // no native time type; store as string or date
                    return "string";
                case field_type::json:
                    return "object";
                case field_type::xml:
                    return "string";
                case field_type::uuid:
                    // BSON subtype 4 binary
                    return "binData";
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

        std::string         m_database_name;
        std::string         m_filter_json;
        std::string         m_projection_json;
        std::string         m_sort_json;
        std::string         m_pipeline_json;
        mongo_read_concern  m_read_concern;
        mongo_write_concern m_write_concern;
    };


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MONGO_TABLE_
