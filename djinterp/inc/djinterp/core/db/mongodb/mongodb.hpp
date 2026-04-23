/******************************************************************************
* djinterp [database]                                              mongodb.hpp
* 
* djinterp MongoDB connection module:
*   This header provides the MongoDB-specific connection implementation
* and associated data type infrastructure for the djinterp database
* module, including:
*   - BSON type code enumeration (the 19 BSON specification types)
*   - BSON-to-field_type mapping
*   - compile-time feature availability using a DUAL-VERSION model:
*     1. driver version (D_ENV_MONGO_DRIVER_*) — compile-time from
*        libmongoc headers; gates client-side API availability
*     2. target server version (D_ENV_MONGO_SERVER_*) — configured
*        manually; gates server-side feature availability
*     additionally gated by edition (Community / Enterprise / Atlas)
*   - MongoDB-specific connection configuration (connection URI, auth
*     mechanism, read/write concern, read preference, TLS, Stable API)
*   - the concrete mongo_connection CRTP leaf class with document CRUD,
*     find / aggregation pipeline, change streams, bulk write, GridFS,
*     index management, session/transaction management, and collection
*     management
*   - version-gated methods for time series (5.0+), queryable encryption
*     (7.0+), and server-side bulkWrite command (8.0+)
*
*   MongoDB is fundamentally different from SQL and even ArangoDB:
*   - document model: BSON documents, not JSON text or VelocyPack
*   - no query language: operations are method calls with BSON filter/
*     update documents, not query strings
*   - aggregation pipeline: multi-stage transformation expressed as an
*     array of BSON stage documents
*   - _id: every document has a unique _id (default ObjectId)
*   - replica set: core deployment model; standalone is for dev only
*   - wire protocol: OP_MSG binary protocol, not HTTP REST
*
*   LAYER DIAGRAM:
*     mongo_connection (this file)
*       -> database_connection<mongo_connection, database_type::mongodb>
*         -> connection_template<mongo_connection, database_type::mongodb>
*           -> connection<mongo_connection>
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include <mongoc.h>
* or <bson.h>; the concrete _impl methods in mongodb.cpp include them.
*
* 
* path:      /inc/djinterp/core/db/mongodb/mongodb.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.26
******************************************************************************/

#ifndef DJINTERP_DATABASE_MONGODB_
#define DJINTERP_DATABASE_MONGODB_

#include "database_connection.hpp"
#include "mongo_traits.hpp"

#include "../env/db/env_mongodb.h"


NS_DJINTERP
NS_DATABASE
NS_MONGO


// =============================================================================
// I.   BSON TYPE CODES
// =============================================================================
// BSON (Binary JSON) is MongoDB's native document format. Every value
// has a type byte defined by the BSON specification. These are stable
// across all MongoDB versions and match the constants in libbson's
// bson_type_t enum.
//
// Source: bsonspec.org and libbson bson-types.h.

// bson_type
//   enumeration: BSON specification type codes.
enum class bson_type : std::uint8_t
{
    type_eod         = 0x00,    // end of document (internal)
    type_double      = 0x01,    // 64-bit IEEE 754 float
    type_utf8        = 0x02,    // UTF-8 string
    type_document    = 0x03,    // embedded document
    type_array       = 0x04,    // array
    type_binary      = 0x05,    // binary data (with subtype)
    type_undefined   = 0x06,    // undefined (deprecated)
    type_oid         = 0x07,    // ObjectId (12 bytes)
    type_bool        = 0x08,    // boolean
    type_date_time   = 0x09,    // UTC datetime (ms since epoch)
    type_null        = 0x0A,    // null
    type_regex       = 0x0B,    // regular expression
    type_dbpointer   = 0x0C,    // DBPointer (deprecated)
    type_code        = 0x0D,    // JavaScript code
    type_symbol      = 0x0E,    // symbol (deprecated)
    type_codewscope  = 0x0F,    // JavaScript code with scope (dep.)
    type_int32       = 0x10,    // 32-bit integer
    type_timestamp   = 0x11,    // internal timestamp (oplog)
    type_int64       = 0x12,    // 64-bit integer
    type_decimal128  = 0x13,    // 128-bit decimal (IEEE 754-2008)
    type_maxkey      = 0x7F,    // MaxKey
    type_minkey      = 0xFF     // MinKey
};


// =============================================================================
// II.  BSON-TO-FIELD_TYPE MAPPING
// =============================================================================

// bson_type_to_field_type
//   function: maps a BSON type code to the generic djinterp field_type.
inline field_type bson_type_to_field_type(
    bson_type _bson) noexcept
{
    switch (_bson)
    {
        case bson_type::type_double:
            return field_type::floating_point;

        case bson_type::type_utf8:
        case bson_type::type_code:
        case bson_type::type_symbol:
        case bson_type::type_regex:
            return field_type::string;

        case bson_type::type_document:
            return field_type::json;

        case bson_type::type_array:
            return field_type::array;

        case bson_type::type_binary:
            return field_type::binary;

        case bson_type::type_oid:
            return field_type::uuid;

        case bson_type::type_bool:
            return field_type::boolean;

        case bson_type::type_date_time:
            return field_type::timestamp;

        case bson_type::type_null:
            return field_type::null;

        case bson_type::type_int32:
            return field_type::integer;

        case bson_type::type_int64:
        case bson_type::type_timestamp:
            return field_type::big_integer;

        case bson_type::type_decimal128:
            return field_type::decimal;

        case bson_type::type_eod:
        case bson_type::type_undefined:
        case bson_type::type_dbpointer:
        case bson_type::type_codewscope:
        case bson_type::type_maxkey:
        case bson_type::type_minkey:
        default:
            return field_type::custom;
    }
}

// field_type_to_bson_type_name
//   function: returns the conventional BSON type name string for a
// given field_type, as used in $type queries and $jsonSchema.
inline const char* field_type_to_bson_type_name(
    field_type _type) noexcept
{
    switch (_type)
    {
        case field_type::null:           return "null";
        case field_type::boolean:        return "bool";
        case field_type::integer:        return "int";
        case field_type::big_integer:    return "long";
        case field_type::floating_point: return "double";
        case field_type::decimal:        return "decimal";
        case field_type::string:         return "string";
        case field_type::binary:         return "binData";
        case field_type::date:
        case field_type::time:
        case field_type::datetime:
        case field_type::timestamp:      return "date";
        case field_type::json:           return "object";
        case field_type::uuid:           return "objectId";
        case field_type::xml:            return "string";
        case field_type::array:          return "array";
        case field_type::custom:
        default:                         return "string";
    }
}


// =============================================================================
// III. FEATURE SUPPORT (compile-time, driver × server × edition gated)
// =============================================================================

// mongo_type_support
//   struct: compile-time BSON type availability flags.
struct mongo_type_support
{
#if D_ENV_MONGO_BSON_DETECTED

    static constexpr bool has_bson           = true;
    static constexpr bool has_double         =
    #if D_ENV_MONGO_HAS_BSON_DOUBLE
        true;  #else  false;  #endif
    static constexpr bool has_string         =
    #if D_ENV_MONGO_HAS_BSON_STRING
        true;  #else  false;  #endif
    static constexpr bool has_document       =
    #if D_ENV_MONGO_HAS_BSON_DOCUMENT
        true;  #else  false;  #endif
    static constexpr bool has_array          =
    #if D_ENV_MONGO_HAS_BSON_ARRAY
        true;  #else  false;  #endif
    static constexpr bool has_binary         =
    #if D_ENV_MONGO_HAS_BSON_BINARY
        true;  #else  false;  #endif
    static constexpr bool has_objectid       =
    #if D_ENV_MONGO_HAS_BSON_OBJECTID
        true;  #else  false;  #endif
    static constexpr bool has_bool           =
    #if D_ENV_MONGO_HAS_BSON_BOOL
        true;  #else  false;  #endif
    static constexpr bool has_datetime       =
    #if D_ENV_MONGO_HAS_BSON_DATETIME
        true;  #else  false;  #endif
    static constexpr bool has_int32          =
    #if D_ENV_MONGO_HAS_BSON_INT32
        true;  #else  false;  #endif
    static constexpr bool has_int64          =
    #if D_ENV_MONGO_HAS_BSON_INT64
        true;  #else  false;  #endif
    static constexpr bool has_decimal128     =
    #if D_ENV_MONGO_HAS_BSON_DECIMAL128
        true;  #else  false;  #endif
    static constexpr bool has_regex          =
    #if D_ENV_MONGO_HAS_BSON_REGEX
        true;  #else  false;  #endif
    static constexpr bool has_null           =
    #if D_ENV_MONGO_HAS_BSON_NULL
        true;  #else  false;  #endif

#else
    static constexpr bool has_bson           = false;
    static constexpr bool has_double         = false;
    static constexpr bool has_string         = false;
    static constexpr bool has_document       = false;
    static constexpr bool has_array          = false;
    static constexpr bool has_binary         = false;
    static constexpr bool has_objectid       = false;
    static constexpr bool has_bool           = false;
    static constexpr bool has_datetime       = false;
    static constexpr bool has_int32          = false;
    static constexpr bool has_int64          = false;
    static constexpr bool has_decimal128     = false;
    static constexpr bool has_regex          = false;
    static constexpr bool has_null           = false;
#endif
};

// mongo_feature_support
//   struct: compile-time feature availability flags gated by
// server version, driver version, and edition.
struct mongo_feature_support
{
#if D_ENV_MONGO_DETECTED

    // aggregation (server-gated)
    static constexpr bool has_agg_lookup =
    #if D_ENV_MONGO_HAS_AGG_LOOKUP
        true;  #else  false;  #endif
    static constexpr bool has_agg_graph_lookup =
    #if D_ENV_MONGO_HAS_AGG_GRAPH_LOOKUP
        true;  #else  false;  #endif
    static constexpr bool has_agg_merge =
    #if D_ENV_MONGO_HAS_AGG_MERGE
        true;  #else  false;  #endif
    static constexpr bool has_agg_union_with =
    #if D_ENV_MONGO_HAS_AGG_UNION_WITH
        true;  #else  false;  #endif
    static constexpr bool has_agg_set_window_fields =
    #if D_ENV_MONGO_HAS_AGG_SET_WINDOW_FIELDS
        true;  #else  false;  #endif
    static constexpr bool has_agg_densify =
    #if D_ENV_MONGO_HAS_AGG_DENSIFY
        true;  #else  false;  #endif
    static constexpr bool has_agg_fill =
    #if D_ENV_MONGO_HAS_AGG_FILL
        true;  #else  false;  #endif

    // transactions (server-gated)
    static constexpr bool has_replica_set_txn =
    #if D_ENV_MONGO_HAS_REPLICA_SET_TXN
        true;  #else  false;  #endif
    static constexpr bool has_distributed_txn =
    #if D_ENV_MONGO_HAS_DISTRIBUTED_TXN
        true;  #else  false;  #endif
    static constexpr bool has_retryable_writes =
    #if D_ENV_MONGO_HAS_RETRYABLE_WRITES
        true;  #else  false;  #endif
    static constexpr bool has_causal_consistency =
    #if D_ENV_MONGO_HAS_CAUSAL_CONSISTENCY
        true;  #else  false;  #endif

    // change streams (server-gated)
    static constexpr bool has_change_streams =
    #if D_ENV_MONGO_HAS_CHANGE_STREAMS
        true;  #else  false;  #endif
    static constexpr bool has_change_streams_cluster =
    #if D_ENV_MONGO_HAS_CHANGE_STREAMS_CLUSTER
        true;  #else  false;  #endif
    static constexpr bool has_change_streams_pre_post =
    #if D_ENV_MONGO_HAS_CHANGE_STREAMS_PRE_POST_IMAGE
        true;  #else  false;  #endif

    // indexes (server-gated)
    static constexpr bool has_index_wildcard =
    #if D_ENV_MONGO_HAS_INDEX_WILDCARD
        true;  #else  false;  #endif
    static constexpr bool has_index_clustered =
    #if D_ENV_MONGO_HAS_INDEX_CLUSTERED
        true;  #else  false;  #endif
    static constexpr bool has_index_columnstore =
    #if D_ENV_MONGO_HAS_INDEX_COLUMNSTORE
        true;  #else  false;  #endif
    static constexpr bool has_collation =
    #if D_ENV_MONGO_HAS_COLLATION
        true;  #else  false;  #endif

    // time series (server-gated)
    static constexpr bool has_time_series =
    #if D_ENV_MONGO_HAS_TIME_SERIES
        true;  #else  false;  #endif

    // sharding (server-gated)
    static constexpr bool has_resharding =
    #if D_ENV_MONGO_HAS_RESHARDING
        true;  #else  false;  #endif

    // security (server + edition)
    static constexpr bool has_csfle =
    #if D_ENV_MONGO_HAS_CSFLE
        true;  #else  false;  #endif
    static constexpr bool has_queryable_encryption =
    #if D_ENV_MONGO_HAS_QUERYABLE_ENCRYPTION
        true;  #else  false;  #endif

    // versioned API (server-gated)
    static constexpr bool has_versioned_api_server =
    #if D_ENV_MONGO_HAS_VERSIONED_API_SERVER
        true;  #else  false;  #endif

    // query features
    static constexpr bool has_schema_validation =
    #if D_ENV_MONGO_HAS_SCHEMA_VALIDATION
        true;  #else  false;  #endif
    static constexpr bool has_update_pipeline =
    #if D_ENV_MONGO_HAS_UPDATE_PIPELINE
        true;  #else  false;  #endif
    static constexpr bool has_bulk_write_command =
    #if D_ENV_MONGO_HAS_BULK_WRITE_COMMAND
        true;  #else  false;  #endif

    // driver-gated
    static constexpr bool has_session_api =
    #if D_ENV_MONGO_HAS_SESSION_API
        true;  #else  false;  #endif
    static constexpr bool has_transaction_api =
    #if D_ENV_MONGO_HAS_TRANSACTION_API
        true;  #else  false;  #endif
    static constexpr bool has_gridfs_bucket =
    #if D_ENV_MONGO_HAS_GRIDFS_BUCKET
        true;  #else  false;  #endif
    static constexpr bool has_versioned_api_driver =
    #if D_ENV_MONGO_HAS_VERSIONED_API
        true;  #else  false;  #endif

    // edition
    static constexpr bool is_enterprise =
    #if D_ENV_MONGO_IS_ENTERPRISE
        true;  #else  false;  #endif
    static constexpr bool is_atlas =
    #if D_ENV_MONGO_IS_ATLAS
        true;  #else  false;  #endif

    // Atlas-specific
    static constexpr bool has_atlas_search =
    #if D_ENV_MONGO_HAS_ATLAS_SEARCH
        true;  #else  false;  #endif
    static constexpr bool has_atlas_vector_search =
    #if D_ENV_MONGO_HAS_ATLAS_VECTOR_SEARCH
        true;  #else  false;  #endif

    // composite
    static constexpr bool has_modern_transactions =
    #if D_ENV_MONGO_HAS_MODERN_TRANSACTIONS
        true;  #else  false;  #endif
    static constexpr bool has_modern_aggregation =
    #if D_ENV_MONGO_HAS_MODERN_AGGREGATION
        true;  #else  false;  #endif
    static constexpr bool is_fully_modern =
    #if D_ENV_MONGO_IS_FULLY_MODERN
        true;  #else  false;  #endif

#else
    static constexpr bool has_agg_lookup               = false;
    static constexpr bool has_agg_graph_lookup         = false;
    static constexpr bool has_agg_merge                = false;
    static constexpr bool has_agg_union_with           = false;
    static constexpr bool has_agg_set_window_fields    = false;
    static constexpr bool has_agg_densify              = false;
    static constexpr bool has_agg_fill                 = false;
    static constexpr bool has_replica_set_txn          = false;
    static constexpr bool has_distributed_txn          = false;
    static constexpr bool has_retryable_writes         = false;
    static constexpr bool has_causal_consistency       = false;
    static constexpr bool has_change_streams           = false;
    static constexpr bool has_change_streams_cluster   = false;
    static constexpr bool has_change_streams_pre_post  = false;
    static constexpr bool has_index_wildcard           = false;
    static constexpr bool has_index_clustered          = false;
    static constexpr bool has_index_columnstore        = false;
    static constexpr bool has_collation                = false;
    static constexpr bool has_time_series              = false;
    static constexpr bool has_resharding               = false;
    static constexpr bool has_csfle                    = false;
    static constexpr bool has_queryable_encryption     = false;
    static constexpr bool has_versioned_api_server     = false;
    static constexpr bool has_schema_validation        = false;
    static constexpr bool has_update_pipeline          = false;
    static constexpr bool has_bulk_write_command       = false;
    static constexpr bool has_session_api              = false;
    static constexpr bool has_transaction_api          = false;
    static constexpr bool has_gridfs_bucket            = false;
    static constexpr bool has_versioned_api_driver     = false;
    static constexpr bool is_enterprise                = false;
    static constexpr bool is_atlas                     = false;
    static constexpr bool has_atlas_search             = false;
    static constexpr bool has_atlas_vector_search      = false;
    static constexpr bool has_modern_transactions      = false;
    static constexpr bool has_modern_aggregation       = false;
    static constexpr bool is_fully_modern              = false;
#endif
};


// =============================================================================
// IV.  VERSION INFORMATION
// =============================================================================

// mongo_version_info
//   struct: compile-time dual-version decomposition.
// MongoDB has separate driver and server versions; both are tracked.
struct mongo_version_info
{
    // driver version (from libmongoc headers)
#if D_ENV_MONGO_DRIVER_DETECTED
    static constexpr bool          driver_detected = true;
    static constexpr std::uint32_t driver_id   = D_ENV_MONGO_DRIVER_VERSION_ID;
    static constexpr std::uint16_t driver_major = D_ENV_MONGO_DRIVER_MAJOR;
    static constexpr std::uint16_t driver_minor = D_ENV_MONGO_DRIVER_MINOR;
    static constexpr std::uint16_t driver_patch = D_ENV_MONGO_DRIVER_PATCH;
    static constexpr const char*   driver_string =
        D_ENV_MONGO_DRIVER_VERSION_STRING;
#else
    static constexpr bool          driver_detected = false;
    static constexpr std::uint32_t driver_id   = 0;
    static constexpr std::uint16_t driver_major = 0;
    static constexpr std::uint16_t driver_minor = 0;
    static constexpr std::uint16_t driver_patch = 0;
    static constexpr const char*   driver_string = "not detected";
#endif

    // target server version (manually configured)
    static constexpr std::uint32_t server_id   = D_ENV_MONGO_SERVER_VERSION_ID;
    static constexpr std::uint16_t server_major =
        D_ENV_MONGO_SERVER_MAJOR;
    static constexpr std::uint16_t server_minor =
        D_ENV_MONGO_SERVER_MINOR;
    static constexpr bool          server_known =
        (D_ENV_MONGO_SERVER_VERSION_ID > 0);

    // at_least (server)
    static constexpr bool server_at_least(
        std::uint16_t _major,
        std::uint16_t _minor,
        std::uint16_t _patch) noexcept
    {
        return server_id >= (_major * 10000u + _minor * 100u + _patch);
    }

    // driver_at_least
    static constexpr bool driver_at_least(
        std::uint16_t _major,
        std::uint16_t _minor,
        std::uint16_t _patch) noexcept
    {
        return driver_id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// V.   READ / WRITE CONCERN AND READ PREFERENCE
// =============================================================================

// mongo_read_concern
//   enumeration: MongoDB read concern levels.
enum class mongo_read_concern : std::uint8_t
{
    local          = 0,     // read from primary or secondary
    available      = 1,     // read without checking replication
    majority       = 2,     // read confirmed by majority
    linearizable   = 3,     // read the latest committed data
    snapshot       = 4      // read from a consistent snapshot (4.0+)
};

// mongo_read_preference
//   enumeration: MongoDB read preference modes.
enum class mongo_read_preference : std::uint8_t
{
    primary              = 0,
    primary_preferred    = 1,
    secondary            = 2,
    secondary_preferred  = 3,
    nearest              = 4
};

// mongo_write_concern
//   enumeration: common MongoDB write concern levels.
enum class mongo_write_concern : std::int8_t
{
    unacknowledged   = 0,   // w: 0
    acknowledged     = 1,   // w: 1 (default)
    majority         = -1   // w: "majority"
};


// =============================================================================
// VI.  MONGODB CONNECTION CONFIGURATION
// =============================================================================

// mongo_auth_mechanism
//   enumeration: MongoDB authentication mechanisms.
enum class mongo_auth_mechanism : std::uint8_t
{
    none            = 0,
    scram_sha_1     = 1,
    scram_sha_256   = 2,
    x509            = 3,
    ldap            = 4,        // Enterprise only
    kerberos        = 5,        // Enterprise only
    oidc            = 6         // 7.0+ Atlas/Enterprise
};

// mongo_connect_config
//   struct: MongoDB-specific connection configuration.
// MongoDB uses a connection URI string as the primary addressing
// format, which includes host(s), authentication, replica set name,
// TLS, and read/write preferences.
struct mongo_connect_config
{
    std::string              connection_uri;
    std::string              database_name;
    std::string              app_name;
    mongo_auth_mechanism     auth_mechanism;
    mongo_read_concern       read_concern;
    mongo_write_concern      write_concern;
    mongo_read_preference    read_preference;
    int                      connect_timeout_ms;
    int                      server_selection_timeout_ms;
    bool                     enable_tls;
    bool                     retry_reads;
    bool                     retry_writes;
    std::string              tls_ca_file;
    std::string              tls_certificate_key_file;
    std::string              stable_api_version;

    std::map<std::string, std::string> extra_options;

    mongo_connect_config()
        : connection_uri("mongodb://localhost:27017")
        , database_name("test")
        , auth_mechanism(mongo_auth_mechanism::none)
        , read_concern(mongo_read_concern::local)
        , write_concern(mongo_write_concern::acknowledged)
        , read_preference(mongo_read_preference::primary)
        , connect_timeout_ms(10000)
        , server_selection_timeout_ms(30000)
        , enable_tls(false)
        , retry_reads(true)
        , retry_writes(true)
    {
    }

    explicit mongo_connect_config(const std::string& _uri)
        : connection_uri(_uri)
        , database_name("test")
        , auth_mechanism(mongo_auth_mechanism::none)
        , read_concern(mongo_read_concern::local)
        , write_concern(mongo_write_concern::acknowledged)
        , read_preference(mongo_read_preference::primary)
        , connect_timeout_ms(10000)
        , server_selection_timeout_ms(30000)
        , enable_tls(false)
        , retry_reads(true)
        , retry_writes(true)
    {
    }

    // with_replica_set
    //   function: factory for replica set connection.
    static mongo_connect_config with_replica_set(
        const std::string& _uri,
        const std::string& _database)
    {
        mongo_connect_config config(_uri);

        config.database_name = _database;
        config.retry_reads   = true;
        config.retry_writes  = true;

        return config;
    }

    // with_tls
    //   function: factory for TLS-enabled connection.
    static mongo_connect_config with_tls(
        const std::string& _uri,
        const std::string& _ca_file)
    {
        mongo_connect_config config(_uri);

        config.enable_tls  = true;
        config.tls_ca_file = _ca_file;

        return config;
    }

    // with_atlas
    //   function: factory for MongoDB Atlas connection using SRV URI.
    static mongo_connect_config with_atlas(
        const std::string& _srv_uri,
        const std::string& _database)
    {
        mongo_connect_config config(_srv_uri);

        config.database_name = _database;
        config.enable_tls    = true;
        config.retry_reads   = true;
        config.retry_writes  = true;

        return config;
    }
};


// =============================================================================
// VII. MONGODB CONNECTION
// =============================================================================

// mongo_connection
//   class: concrete MongoDB connection implementation via libmongoc.
// This is the CRTP leaf class; _impl methods are defined in
// mongodb.cpp which includes <mongoc/mongoc.h>.
//
// MongoDB operations use BSON documents rather than query strings.
// The generic execute_query() maps to a find() with an ABI-compatible
// JSON-string filter so the CRTP chain remains satisfied; native usage
// should prefer find(), aggregate(), insert_one(), etc.
//
// Usage:
//   mongo_connection conn;
//   conn.connect(mongo_connect_config::with_atlas(
//       "mongodb+srv://user:pass@cluster.mongodb.net", "mydb"));
//   auto doc = conn.find_one("users", R"({"email": "a@b.com"})");
class mongo_connection
    : public database_connection<mongo_connection,
                                 database_type::mongodb>
{
public:
    using base_type       = database_connection<
        mongo_connection, database_type::mongodb>;
    using type_support    = mongo_type_support;
    using feature_support = mongo_feature_support;
    using version_info    = mongo_version_info;

    mongo_connection()
        : base_type()
    {
    }

    explicit mongo_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit mongo_connection(const mongo_connect_config& _config)
        : base_type()
        , m_mongo_config(_config)
    {
        this->m_config.host     = _config.connection_uri;
        this->m_config.database = _config.database_name;
    }

    explicit mongo_connection(const std::string& _uri)
        : base_type()
        , m_mongo_config(_uri)
    {
        this->m_config.host = _uri;
    }

    ~mongo_connection() = default;

    // disable copying
    mongo_connection(const mongo_connection&)            = delete;
    mongo_connection& operator=(const mongo_connection&) = delete;

    // enable moving
    mongo_connection(mongo_connection&&) noexcept            = default;
    mongo_connection& operator=(mongo_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // document CRUD
    // -----------------------------------------------------------------

    std::string insert_one(const std::string& _collection,
                           const std::string& _document_json)
    {
        this->ensure_connected();

        return self().insert_one_impl(_collection, _document_json);
    }

    std::string find_one(const std::string& _collection,
                         const std::string& _filter_json) const
    {
        return self().find_one_impl(_collection, _filter_json);
    }

    void update_one(const std::string& _collection,
                    const std::string& _filter_json,
                    const std::string& _update_json)
    {
        this->ensure_connected();
        self().update_one_impl(_collection, _filter_json,
                               _update_json);
    }

    void replace_one(const std::string& _collection,
                     const std::string& _filter_json,
                     const std::string& _replacement_json)
    {
        this->ensure_connected();
        self().replace_one_impl(_collection, _filter_json,
                                _replacement_json);
    }

    void delete_one(const std::string& _collection,
                    const std::string& _filter_json)
    {
        this->ensure_connected();
        self().delete_one_impl(_collection, _filter_json);
    }

    std::int64_t delete_many(const std::string& _collection,
                              const std::string& _filter_json)
    {
        this->ensure_connected();

        return self().delete_many_impl(_collection, _filter_json);
    }

    // -----------------------------------------------------------------
    // find / query
    // -----------------------------------------------------------------

    std::string find(const std::string& _collection,
                     const std::string& _filter_json)
    {
        this->ensure_connected();

        return self().find_impl(_collection, _filter_json);
    }

    std::int64_t count_documents(const std::string& _collection,
                                  const std::string& _filter_json) const
    {
        return self().count_documents_impl(_collection, _filter_json);
    }

    // -----------------------------------------------------------------
    // aggregation pipeline
    // -----------------------------------------------------------------

    std::string aggregate(const std::string& _collection,
                          const std::string& _pipeline_json)
    {
        this->ensure_connected();

        return self().aggregate_impl(_collection, _pipeline_json);
    }

    // -----------------------------------------------------------------
    // collection management
    // -----------------------------------------------------------------

    void create_collection(const std::string& _name)
    {
        this->ensure_connected();
        self().create_collection_impl(_name);
    }

    void drop_collection(const std::string& _name)
    {
        this->ensure_connected();
        self().drop_collection_impl(_name);
    }

    bool collection_exists(const std::string& _name) const
    {
        return self().collection_exists_impl(_name);
    }

    std::vector<std::string> list_collection_names() const
    {
        return self().list_collection_names_impl();
    }

    // -----------------------------------------------------------------
    // index management
    // -----------------------------------------------------------------

    void create_index(const std::string& _collection,
                      const std::string& _keys_json)
    {
        this->ensure_connected();
        self().create_index_impl(_collection, _keys_json);
    }

    std::string list_indexes(const std::string& _collection) const
    {
        return self().list_indexes_impl(_collection);
    }

    // -----------------------------------------------------------------
    // change streams
    // -----------------------------------------------------------------

    std::string watch_collection(const std::string& _collection)
    {
        this->ensure_connected();

        return self().watch_collection_impl(_collection);
    }

    std::string watch_database()
    {
        this->ensure_connected();

        return self().watch_database_impl();
    }

    // -----------------------------------------------------------------
    // bulk write
    // -----------------------------------------------------------------

    void execute_bulk(const std::string& _collection,
                      const std::string& _operations_json)
    {
        this->ensure_connected();
        self().execute_bulk_impl(_collection, _operations_json);
    }

    // -----------------------------------------------------------------
    // GridFS
    // -----------------------------------------------------------------

    std::string gridfs_upload(
        const std::string&              _filename,
        const std::vector<std::uint8_t>& _data)
    {
        this->ensure_connected();

        return self().gridfs_upload_impl(_filename, _data);
    }

    std::vector<std::uint8_t> gridfs_download(
        const std::string& _file_id)
    {
        return self().gridfs_download_impl(_file_id);
    }

    // -----------------------------------------------------------------
    // sessions and transactions
    // -----------------------------------------------------------------

    void start_session()
    {
        this->ensure_connected();
        self().start_session_impl();
    }

    void start_transaction()
    {
        self().start_transaction_impl();
    }

    void commit_transaction()
    {
        self().commit_transaction_impl();
    }

    void abort_transaction()
    {
        self().abort_transaction_impl();
    }

    // -----------------------------------------------------------------
    // read/write concern
    // -----------------------------------------------------------------

    void set_read_concern(const std::string& _level)
    {
        self().set_read_concern_impl(_level);
    }

    void set_write_concern(int _w)
    {
        self().set_write_concern_impl(_w);
    }

    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_transactions() noexcept
    {
        return feature_support::has_replica_set_txn;
    }

    static constexpr bool supports_distributed_txn() noexcept
    {
        return feature_support::has_distributed_txn;
    }

    static constexpr bool supports_change_streams() noexcept
    {
        return feature_support::has_change_streams;
    }

    static constexpr bool supports_time_series() noexcept
    {
        return feature_support::has_time_series;
    }

    static constexpr bool supports_window_fields() noexcept
    {
        return feature_support::has_agg_set_window_fields;
    }

    static constexpr bool supports_queryable_encryption() noexcept
    {
        return feature_support::has_queryable_encryption;
    }

    static constexpr bool supports_atlas_search() noexcept
    {
        return feature_support::has_atlas_search;
    }

    static constexpr bool supports_versioned_api() noexcept
    {
        return feature_support::has_versioned_api_server &&
               feature_support::has_versioned_api_driver;
    }

    static constexpr bool is_enterprise() noexcept
    {
        return feature_support::is_enterprise;
    }

    static constexpr bool is_atlas() noexcept
    {
        return feature_support::is_atlas;
    }

    // -----------------------------------------------------------------
    // type mapping
    // -----------------------------------------------------------------

    static field_type map_bson_type(bson_type _bson) noexcept
    {
        return bson_type_to_field_type(_bson);
    }

    static const char* bson_type_name(field_type _type) noexcept
    {
        return field_type_to_bson_type_name(_type);
    }

    // -----------------------------------------------------------------
    // configuration
    // -----------------------------------------------------------------

    const mongo_connect_config& get_mongo_config() const noexcept
    {
        return m_mongo_config;
    }

    void set_mongo_config(const mongo_connect_config& _config)
    {
        m_mongo_config      = _config;
        this->m_config.host     = _config.connection_uri;
        this->m_config.database = _config.database_name;
    }

    // -----------------------------------------------------------------
    // _impl methods (defined in mongodb.cpp)
    // -----------------------------------------------------------------

    void connect_impl();
    void disconnect_impl();
    bool is_connected_impl() const;
    bool ping_impl() const;

    // generic execute_query maps to find
    auto execute_query_impl(const std::string& _query)
        -> std::unique_ptr<
            result_set<struct mongo_result_set_impl>>;
    std::int64_t execute_update_impl(const std::string& _query);
    bool execute_impl(const std::string& _query);

    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;
    std::int64_t get_last_insert_id_impl() const;
    std::int64_t get_affected_rows_impl() const;

    // MongoDB-specific _impl methods
    std::string insert_one_impl(const std::string& _collection,
                                 const std::string& _doc);
    std::string find_one_impl(const std::string& _collection,
                               const std::string& _filter) const;
    void update_one_impl(const std::string& _collection,
                          const std::string& _filter,
                          const std::string& _update);
    void replace_one_impl(const std::string& _collection,
                           const std::string& _filter,
                           const std::string& _replacement);
    void delete_one_impl(const std::string& _collection,
                          const std::string& _filter);
    std::int64_t delete_many_impl(const std::string& _collection,
                                   const std::string& _filter);
    std::string find_impl(const std::string& _collection,
                           const std::string& _filter);
    std::int64_t count_documents_impl(
        const std::string& _collection,
        const std::string& _filter) const;
    std::string aggregate_impl(const std::string& _collection,
                                const std::string& _pipeline);
    void create_collection_impl(const std::string& _name);
    void drop_collection_impl(const std::string& _name);
    bool collection_exists_impl(const std::string& _name) const;
    std::vector<std::string> list_collection_names_impl() const;
    void create_index_impl(const std::string& _collection,
                            const std::string& _keys);
    std::string list_indexes_impl(
        const std::string& _collection) const;
    std::string watch_collection_impl(
        const std::string& _collection);
    std::string watch_database_impl();
    void execute_bulk_impl(const std::string& _collection,
                            const std::string& _operations);
    std::string gridfs_upload_impl(
        const std::string& _filename,
        const std::vector<std::uint8_t>& _data);
    std::vector<std::uint8_t> gridfs_download_impl(
        const std::string& _file_id);
    void start_session_impl();
    void start_transaction_impl();
    void commit_transaction_impl();
    void abort_transaction_impl();
    void set_read_concern_impl(const std::string& _level);
    void set_write_concern_impl(int _w);

    // transaction _impl (for generic CRTP chain)
    void begin_transaction_impl();
    void commit_impl();
    void rollback_impl();

    // version-gated methods

#if D_ENV_MONGO_DETECTED
    #if D_ENV_MONGO_HAS_TIME_SERIES
    // create_time_series_collection
    //   function: creates a time series collection.
    // Available since MongoDB 5.0.
    void create_time_series_collection(
        const std::string& _name,
        const std::string& _time_field,
        const std::string& _meta_field = "");
    #endif

    #if D_ENV_MONGO_HAS_QUERYABLE_ENCRYPTION
    // enable_queryable_encryption
    //   function: configures queryable encryption on a collection.
    // Available since MongoDB 7.0.
    void enable_queryable_encryption(
        const std::string& _collection,
        const std::string& _encryption_config_json);
    #endif

    #if D_ENV_MONGO_HAS_BULK_WRITE_COMMAND
    // server_bulk_write
    //   function: executes the server-side bulkWrite command.
    // Available since MongoDB 8.0.
    std::string server_bulk_write(
        const std::string& _operations_json);
    #endif

    #if D_ENV_MONGO_HAS_CHANGE_STREAMS_PRE_POST_IMAGE
    // enable_pre_post_images
    //   function: enables pre/post-image capture on a collection
    // for change streams. Available since MongoDB 6.0.
    void enable_pre_post_images(const std::string& _collection);
    #endif
#endif  // D_ENV_MONGO_DETECTED

private:
    mongo_connect_config m_mongo_config;

    mongo_connection& self()
    {
        return *this;
    }

    const mongo_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// mongo_result_set_impl
//   struct: forward declaration of the MongoDB result set
// (cursor wrapper) implementation.
struct mongo_result_set_impl;


NS_END  // mongo
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MONGODB_
