/*******************************************************************************
* djinterp [cpp]                                                     mongodb.hpp
*
* djinterp MongoDB connection module:
*   The MongoDB connection, mongo_connection, over the MongoDB layer's C++
* face (c/db/mongodb/mongodb_face.hpp), whose handle -- the layer's struct
* dj_mongodb, around the driver's client -- it holds: every operation is the
* layer's, and core/db adds exceptions, result sets that outlive their
* query, and cursors and change streams that close with the connection.
*   MongoDB has no query language: execute_query, execute_update and execute
* take a command, a document in Extended JSON ({"find": "orders", "filter":
* {...}}, {"serverStatus": 1}), and a document comes back as a row of one
* column, "document", of field_type::json. Beside the operations every
* connection has, it inserts, finds, updates, replaces, deletes, counts and
* aggregates documents, writes many at once, reads a query a page at a time
* and watches changes as they happen, keeps files in GridFS, and manages
* databases, collections, views and indexes, each one call of the layer on
* the connection's current database. Filters, documents, pipelines and
* options are Extended JSON too, an empty string standing for none.
*   Transactions need a replica set, a sharded cluster or a load balancer
* (supports_transactions); they have no isolation levels and no savepoints,
* and there are no prepared statements, so the connection has no prepare
* and no stream_query (B6), and refuses the operations that do not block,
* since the driver has none (ruling 11.26.6).
*     mongo_connection (this file)
*       -> database_connection<mongo_connection, database_type::mongodb>
*         -> connection_template<mongo_connection, database_type::mongodb>
*           -> connection<mongo_connection>
*   With it, BSON's types and how they map to field_type, and what the
* driver's header and the target server env names have, at compile time
* from env_mongodb.h: the client's capabilities. What the server connected
* can do, the connection's supports_* members ask at run time. The members
* are defined in mongodb.cpp. The header needs C++17 and includes no client
* header: env_mongodb.h includes mongoc.h only where a program asks for
* env's detection (D_CFG_ENV_USING_MONGODB). Its trailing sections carry
* MongoDB's capability-detection traits and C++20 concepts.
*
*
* path:      /inc/djinterp/cpp/db/mongodb/mongodb.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.26
*                                                            revised: 2026.10.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  BSON TYPES
    ----------
    1.  The codes
         1.  bson_type
    2.  To and from field_type
         1.  bson_type_to_field_type
         2.  field_type_to_bson_type_name
2.  CLIENT CAPABILITIES
    -------------------
    1.  Compile-time capabilities
         1.  mongo_client_capabilities
         2.  mongo_type_support
         3.  mongo_feature_support
3.  VERSION INFORMATION
    -------------------
    1.  The versions
         1.  mongo_version_info
4.  SETTINGS
    --------
    1.  Concerns and preferences
         1.  mongo_read_concern
         2.  mongo_write_concern
         3.  mongo_read_preference
    2.  Documents
         1.  mongo_document_format
    3.  Deployments
         1.  mongo_topology
    4.  Authentication
         1.  mongo_auth_mechanism
5.  CONNECTION CONFIGURATION
    ------------------------
    1.  The configuration
         1.  mongo_connect_config
6.  MONGODB CONNECTION
    ------------------
    1.  Results
         1.  mongo_write_result
         2.  mongo_validation
    2.  The layer
         1.  mongodb_layer
    3.  Cursors and change streams
         1.  mongo_cursor
         2.  mongo_change_stream
    4.  The class
         1.  mongo_connection
7.  EXPRESSION DETECTORS
    --------------------
    1.  Documents
    2.  Finding and aggregating
    3.  Collections
    4.  Change streams and cursors
    5.  Many writes at once
    6.  GridFS
    7.  Indexes
    8.  Transactions
    9.  Concerns
8.  TAGGED CAPABILITY TRAITS
    ------------------------
    1.  Traits
         1.  has_mongo_document_crud
         2.  has_mongo_collections
         3.  has_mongo_change_streams
         4.  has_mongo_gridfs
         5.  has_mongo_transactions
         6.  has_mongo_indexes
         7.  is_mongo_connection
9.  TAGLESS CAPABILITY TRAITS
    -------------------------
    1.  Individual capabilities
    2.  Compound capabilities
10. SFINAE HELPERS
    --------------
    1.  enable_if helpers
11. C++20 CONCEPTS
    --------------
    1.  Connection concepts
    2.  Capability concepts
    3.  Tagless capability concepts
    4.  Former names
*/

#ifndef DJINTERP_DB_MONGODB_MONGODB_HPP
#define DJINTERP_DB_MONGODB_MONGODB_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t,
                                                   // uint16_t, uint32_t,
                                                   // int64_t

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <map>          // std::map
#include <memory>       // std::shared_ptr, std::unique_ptr
#include <optional>     // std::optional
#include <string>       // std::string
#include <type_traits>  // std::enable_if
#include <utility>      // std::declval
#include <vector>       // std::vector
// djinterp
#include "../../djinterp.hpp"                   // framework root
#include "mongodb_face.hpp"  // face::mongodb, the layer
#include "../../../env/db/mongodb/env_mongodb.h"   // D_ENV_MONGO_*
#include "../../meta/type_utility.hpp"             // clean_t
#include "../database_connection.hpp"              // database_connection;
                                                   // through database.hpp,
                                                   // has_connect, can_connect
#include "../database_statement.hpp"               // failure_kind,
                                                   // buffered_result_set,
                                                   // internal::session


NS_DJINTERP


//==============================================================================
// 1.  BSON TYPES
//==============================================================================
// BSON (Binary JSON) is MongoDB's own format for documents. Every value has
// a type byte the BSON specification defines, stable across every release,
// as libbson's bson_type_t names them (bsonspec.org).


// 1.1    The codes
//------------------------------------------------------------------------------
// 1.1.1
// bson_type
//   enum: the BSON specification's type codes.
enum class bson_type : re_std::uint8_t
{
    type_eod         = 0x00,  // the end of a document
    type_double      = 0x01,  // a 64-bit IEEE 754 floating point number
    type_utf8        = 0x02,  // UTF-8 text
    type_document    = 0x03,  // an embedded document
    type_array       = 0x04,  // an array
    type_binary      = 0x05,  // bytes, with a subtype
    type_undefined   = 0x06,  // undefined (deprecated)
    type_oid         = 0x07,  // an ObjectId, 12 bytes
    type_bool        = 0x08,  // a boolean
    type_date_time   = 0x09,  // a UTC point in time, in milliseconds
    type_null        = 0x0A,  // null
    type_regex       = 0x0B,  // a regular expression
    type_dbpointer   = 0x0C,  // a DBPointer (deprecated)
    type_code        = 0x0D,  // JavaScript code
    type_symbol      = 0x0E,  // a symbol (deprecated)
    type_codewscope  = 0x0F,  // JavaScript code with a scope (deprecated)
    type_int32       = 0x10,  // a 32-bit integer
    type_timestamp   = 0x11,  // the replication log's timestamp
    type_int64       = 0x12,  // a 64-bit integer
    type_decimal128  = 0x13,  // a 128-bit decimal (IEEE 754-2008)
    type_maxkey      = 0x7F,  // MaxKey
    type_minkey      = 0xFF   // MinKey
};

// 1.2    To and from field_type
//------------------------------------------------------------------------------
// 1.2.1
// bson_type_to_field_type
//   function: a BSON type as the generic field_type: an ObjectId as uuid, a
// document as json, a type field_type has no word for as custom.
inline
field_type bson_type_to_field_type(
    bson_type _bson
) noexcept
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

// 1.2.2
// field_type_to_bson_type_name
//   function: the BSON type name a field_type maps to, as $type and
// $jsonSchema's bsonType spell it; "string" for a type with none.
inline
const char* field_type_to_bson_type_name(
    field_type _type
) noexcept
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


//==============================================================================
// 2.  CLIENT CAPABILITIES
//==============================================================================
// What env_mongodb.h found the driver's and libbson's headers to have, and
// what the target server version and edition configured for env have
// (D_CFG_ENV_MONGO_SERVER_VERSION), at compile time -- the client's
// capabilities: BSON's types, the server's features by its release, and the
// driver's own. Every flag is false unless a program asks for env's
// detection (D_CFG_ENV_USING_MONGODB) or supplies its results, and a
// server-gated one too unless a target server is configured. What the server
// connected has, mongo_connection's supports_* members ask at run time.


// 2.1    Compile-time capabilities
//------------------------------------------------------------------------------
// 2.1.1
// mongo_client_capabilities
//   struct: compile-time BSON type, server feature, driver and edition
// flags, as env_mongodb.h gives them.
struct mongo_client_capabilities
{
#if D_ENV_MONGO_BSON_DETECTED

    // BSON types (libbson's)
    static constexpr bool has_bson = true;

    static constexpr bool has_double =
    #if D_ENV_MONGO_HAS_BSON_DOUBLE
        true;
    #else
        false;
    #endif

    static constexpr bool has_string =
    #if D_ENV_MONGO_HAS_BSON_STRING
        true;
    #else
        false;
    #endif

    static constexpr bool has_document =
    #if D_ENV_MONGO_HAS_BSON_DOCUMENT
        true;
    #else
        false;
    #endif

    static constexpr bool has_array =
    #if D_ENV_MONGO_HAS_BSON_ARRAY
        true;
    #else
        false;
    #endif

    static constexpr bool has_binary =
    #if D_ENV_MONGO_HAS_BSON_BINARY
        true;
    #else
        false;
    #endif

    static constexpr bool has_objectid =
    #if D_ENV_MONGO_HAS_BSON_OBJECTID
        true;
    #else
        false;
    #endif

    static constexpr bool has_bool =
    #if D_ENV_MONGO_HAS_BSON_BOOL
        true;
    #else
        false;
    #endif

    static constexpr bool has_datetime =
    #if D_ENV_MONGO_HAS_BSON_DATETIME
        true;
    #else
        false;
    #endif

    static constexpr bool has_int32 =
    #if D_ENV_MONGO_HAS_BSON_INT32
        true;
    #else
        false;
    #endif

    static constexpr bool has_int64 =
    #if D_ENV_MONGO_HAS_BSON_INT64
        true;
    #else
        false;
    #endif

    static constexpr bool has_decimal128 =
    #if D_ENV_MONGO_HAS_BSON_DECIMAL128
        true;
    #else
        false;
    #endif

    static constexpr bool has_regex =
    #if D_ENV_MONGO_HAS_BSON_REGEX
        true;
    #else
        false;
    #endif

    static constexpr bool has_null =
    #if D_ENV_MONGO_HAS_BSON_NULL
        true;
    #else
        false;
    #endif

#else

    static constexpr bool has_bson = false;
    static constexpr bool has_double     = false;
    static constexpr bool has_string     = false;
    static constexpr bool has_document   = false;
    static constexpr bool has_array      = false;
    static constexpr bool has_binary     = false;
    static constexpr bool has_objectid   = false;
    static constexpr bool has_bool       = false;
    static constexpr bool has_datetime   = false;
    static constexpr bool has_int32      = false;
    static constexpr bool has_int64      = false;
    static constexpr bool has_decimal128 = false;
    static constexpr bool has_regex      = false;
    static constexpr bool has_null       = false;

#endif  // D_ENV_MONGO_BSON_DETECTED

#if D_ENV_MONGO_DETECTED

    // aggregation (the server's)
    static constexpr bool has_agg_lookup =
    #if D_ENV_MONGO_HAS_AGG_LOOKUP
        true;
    #else
        false;
    #endif

    static constexpr bool has_agg_graph_lookup =
    #if D_ENV_MONGO_HAS_AGG_GRAPH_LOOKUP
        true;
    #else
        false;
    #endif

    static constexpr bool has_agg_merge =
    #if D_ENV_MONGO_HAS_AGG_MERGE
        true;
    #else
        false;
    #endif

    static constexpr bool has_agg_union_with =
    #if D_ENV_MONGO_HAS_AGG_UNION_WITH
        true;
    #else
        false;
    #endif

    static constexpr bool has_agg_set_window_fields =
    #if D_ENV_MONGO_HAS_AGG_SET_WINDOW_FIELDS
        true;
    #else
        false;
    #endif

    static constexpr bool has_agg_densify =
    #if D_ENV_MONGO_HAS_AGG_DENSIFY
        true;
    #else
        false;
    #endif

    static constexpr bool has_agg_fill =
    #if D_ENV_MONGO_HAS_AGG_FILL
        true;
    #else
        false;
    #endif

    // transactions (the server's)
    static constexpr bool has_replica_set_txn =
    #if D_ENV_MONGO_HAS_REPLICA_SET_TXN
        true;
    #else
        false;
    #endif

    static constexpr bool has_distributed_txn =
    #if D_ENV_MONGO_HAS_DISTRIBUTED_TXN
        true;
    #else
        false;
    #endif

    static constexpr bool has_retryable_writes =
    #if D_ENV_MONGO_HAS_RETRYABLE_WRITES
        true;
    #else
        false;
    #endif

    static constexpr bool has_causal_consistency =
    #if D_ENV_MONGO_HAS_CAUSAL_CONSISTENCY
        true;
    #else
        false;
    #endif

    // change streams (the server's)
    static constexpr bool has_change_streams =
    #if D_ENV_MONGO_HAS_CHANGE_STREAMS
        true;
    #else
        false;
    #endif

    static constexpr bool has_change_streams_cluster =
    #if D_ENV_MONGO_HAS_CHANGE_STREAMS_CLUSTER
        true;
    #else
        false;
    #endif

    static constexpr bool has_change_streams_pre_post =
    #if D_ENV_MONGO_HAS_CHANGE_STREAMS_PRE_POST_IMAGE
        true;
    #else
        false;
    #endif

    // indexes (the server's)
    static constexpr bool has_index_wildcard =
    #if D_ENV_MONGO_HAS_INDEX_WILDCARD
        true;
    #else
        false;
    #endif

    static constexpr bool has_index_clustered =
    #if D_ENV_MONGO_HAS_INDEX_CLUSTERED
        true;
    #else
        false;
    #endif

    static constexpr bool has_index_columnstore =
    #if D_ENV_MONGO_HAS_INDEX_COLUMNSTORE
        true;
    #else
        false;
    #endif

    static constexpr bool has_collation =
    #if D_ENV_MONGO_HAS_COLLATION
        true;
    #else
        false;
    #endif

    // collections, sharding and security (the server's, and its edition's)
    static constexpr bool has_time_series =
    #if D_ENV_MONGO_HAS_TIME_SERIES
        true;
    #else
        false;
    #endif

    static constexpr bool has_resharding =
    #if D_ENV_MONGO_HAS_RESHARDING
        true;
    #else
        false;
    #endif

    static constexpr bool has_csfle =
    #if D_ENV_MONGO_HAS_CSFLE
        true;
    #else
        false;
    #endif

    static constexpr bool has_queryable_encryption =
    #if D_ENV_MONGO_HAS_QUERYABLE_ENCRYPTION
        true;
    #else
        false;
    #endif

    static constexpr bool has_versioned_api_server =
    #if D_ENV_MONGO_HAS_VERSIONED_API_SERVER
        true;
    #else
        false;
    #endif

    // queries (the server's)
    static constexpr bool has_schema_validation =
    #if D_ENV_MONGO_HAS_SCHEMA_VALIDATION
        true;
    #else
        false;
    #endif

    static constexpr bool has_update_pipeline =
    #if D_ENV_MONGO_HAS_UPDATE_PIPELINE
        true;
    #else
        false;
    #endif

    static constexpr bool has_bulk_write_command =
    #if D_ENV_MONGO_HAS_BULK_WRITE_COMMAND
        true;
    #else
        false;
    #endif

    // the driver's
    static constexpr bool has_session_api =
    #if D_ENV_MONGO_HAS_SESSION_API
        true;
    #else
        false;
    #endif

    static constexpr bool has_transaction_api =
    #if D_ENV_MONGO_HAS_TRANSACTION_API
        true;
    #else
        false;
    #endif

    static constexpr bool has_gridfs_bucket =
    #if D_ENV_MONGO_HAS_GRIDFS_BUCKET
        true;
    #else
        false;
    #endif

    static constexpr bool has_versioned_api_driver =
    #if D_ENV_MONGO_HAS_VERSIONED_API
        true;
    #else
        false;
    #endif

    // the edition, and Atlas
    static constexpr bool is_enterprise =
    #if D_ENV_MONGO_IS_ENTERPRISE
        true;
    #else
        false;
    #endif

    static constexpr bool is_atlas =
    #if D_ENV_MONGO_IS_ATLAS
        true;
    #else
        false;
    #endif

    static constexpr bool has_atlas_search =
    #if D_ENV_MONGO_HAS_ATLAS_SEARCH
        true;
    #else
        false;
    #endif

    static constexpr bool has_atlas_vector_search =
    #if D_ENV_MONGO_HAS_ATLAS_VECTOR_SEARCH
        true;
    #else
        false;
    #endif

    // compound
    static constexpr bool has_modern_transactions =
    #if D_ENV_MONGO_HAS_MODERN_TRANSACTIONS
        true;
    #else
        false;
    #endif

    static constexpr bool has_modern_aggregation =
    #if D_ENV_MONGO_HAS_MODERN_AGGREGATION
        true;
    #else
        false;
    #endif

    static constexpr bool is_fully_modern =
    #if D_ENV_MONGO_IS_FULLY_MODERN
        true;
    #else
        false;
    #endif

#else

    static constexpr bool has_agg_lookup              = false;
    static constexpr bool has_agg_graph_lookup        = false;
    static constexpr bool has_agg_merge               = false;
    static constexpr bool has_agg_union_with          = false;
    static constexpr bool has_agg_set_window_fields   = false;
    static constexpr bool has_agg_densify             = false;
    static constexpr bool has_agg_fill                = false;
    static constexpr bool has_replica_set_txn         = false;
    static constexpr bool has_distributed_txn         = false;
    static constexpr bool has_retryable_writes        = false;
    static constexpr bool has_causal_consistency      = false;
    static constexpr bool has_change_streams          = false;
    static constexpr bool has_change_streams_cluster  = false;
    static constexpr bool has_change_streams_pre_post = false;
    static constexpr bool has_index_wildcard          = false;
    static constexpr bool has_index_clustered         = false;
    static constexpr bool has_index_columnstore       = false;
    static constexpr bool has_collation               = false;
    static constexpr bool has_time_series             = false;
    static constexpr bool has_resharding              = false;
    static constexpr bool has_csfle                   = false;
    static constexpr bool has_queryable_encryption    = false;
    static constexpr bool has_versioned_api_server    = false;
    static constexpr bool has_schema_validation       = false;
    static constexpr bool has_update_pipeline         = false;
    static constexpr bool has_bulk_write_command      = false;
    static constexpr bool has_session_api             = false;
    static constexpr bool has_transaction_api         = false;
    static constexpr bool has_gridfs_bucket           = false;
    static constexpr bool has_versioned_api_driver    = false;
    static constexpr bool is_enterprise               = false;
    static constexpr bool is_atlas                    = false;
    static constexpr bool has_atlas_search            = false;
    static constexpr bool has_atlas_vector_search     = false;
    static constexpr bool has_modern_transactions     = false;
    static constexpr bool has_modern_aggregation      = false;
    static constexpr bool is_fully_modern             = false;

#endif  // D_ENV_MONGO_DETECTED
};

// 2.1.2
// mongo_type_support
//   type: mongo_client_capabilities, by its former name (2026.10.28), for
// one release; it held the BSON types.
using mongo_type_support [[deprecated("mongo_client_capabilities")]] =
    mongo_client_capabilities;

// 2.1.3
// mongo_feature_support
//   type: mongo_client_capabilities, by its former name (2026.10.28), for
// one release; it held the features.
using mongo_feature_support [[deprecated("mongo_client_capabilities")]] =
    mongo_client_capabilities;


//==============================================================================
// 3.  VERSION INFORMATION
//==============================================================================


// 3.1    The versions
//------------------------------------------------------------------------------
// 3.1.1
// mongo_version_info
//   struct: the two versions env tracks, each MAJOR*10000 + MINOR*100 +
// PATCH: the driver's, from its header, and the target server's, as
// configured for env (0 for none); the server connected is the connection's
// to ask (get_server_version, server_version_number).
struct mongo_version_info
{
#if D_ENV_MONGO_DRIVER_DETECTED
    static constexpr bool             driver_detected = true;
    static constexpr re_std::uint32_t driver_id       =
        D_ENV_MONGO_DRIVER_VERSION_ID;
    static constexpr re_std::uint16_t driver_major    =
        D_ENV_MONGO_DRIVER_MAJOR;
    static constexpr re_std::uint16_t driver_minor    =
        D_ENV_MONGO_DRIVER_MINOR;
    static constexpr re_std::uint16_t driver_patch    =
        D_ENV_MONGO_DRIVER_PATCH;
    static constexpr const char*      driver_string   =
        D_ENV_MONGO_DRIVER_VERSION_STRING;
#else
    static constexpr bool             driver_detected = false;
    static constexpr re_std::uint32_t driver_id       = 0;
    static constexpr re_std::uint16_t driver_major    = 0;
    static constexpr re_std::uint16_t driver_minor    = 0;
    static constexpr re_std::uint16_t driver_patch    = 0;
    static constexpr const char*      driver_string   = "not detected";
#endif  // D_ENV_MONGO_DRIVER_DETECTED

    // the target server, as configured
    static constexpr re_std::uint32_t server_id    =
        D_ENV_MONGO_SERVER_VERSION_ID;
    static constexpr re_std::uint16_t server_major =
        D_ENV_MONGO_SERVER_MAJOR;
    static constexpr re_std::uint16_t server_minor =
        D_ENV_MONGO_SERVER_MINOR;
    static constexpr bool             server_known =
        (D_ENV_MONGO_SERVER_VERSION_ID > 0);

    // server_at_least, driver_at_least -- whether the configured target
    // server, or the driver's header, is at a version or later
    static constexpr
    bool server_at_least(
        re_std::uint16_t _major,
        re_std::uint16_t _minor,
        re_std::uint16_t _patch
    ) noexcept
    {
        return server_id >= (_major * 10000u + _minor * 100u + _patch);
    }

    static constexpr
    bool driver_at_least(
        re_std::uint16_t _major,
        re_std::uint16_t _minor,
        re_std::uint16_t _patch
    ) noexcept
    {
        return driver_id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


//==============================================================================
// 4.  SETTINGS
//==============================================================================


// 4.1    Concerns and preferences
//------------------------------------------------------------------------------
// 4.1.1
// mongo_read_concern
//   enum: the read concern's levels: how current and how durable what a
// read sees must be.
enum class mongo_read_concern : re_std::uint8_t
{
    local        = 0,  // the member's own data
    available    = 1,  // as local, without checking a sharded cluster's
                       // ownership of the documents
    majority     = 2,  // what a majority of members acknowledged
    linearizable = 3,  // the latest majority-acknowledged data, at the
                       // primary
    snapshot     = 4   // a point in time, in a transaction or from 5.0
};

// 4.1.2
// mongo_write_concern
//   enum: the write concerns for a count of members: one, or a majority. A
// write concern the enum cannot say -- journalled, a tag set, a time limit
// -- is set as JSON; one of no acknowledgement, w: 0, the layer refuses,
// since it reports what every write did.
enum class mongo_write_concern : re_std::uint8_t
{
    acknowledged = 1,  // w: 1, the primary alone
    majority     = 2   // w: "majority"
};

// 4.1.3
// mongo_read_preference
//   enum: the read preference's modes: which members a read may use.
enum class mongo_read_preference : re_std::uint8_t
{
    primary             = 0,
    primary_preferred   = 1,
    secondary           = 2,
    secondary_preferred = 3,
    nearest             = 4
};

// 4.2    Documents
//------------------------------------------------------------------------------
// 4.2.1
// mongo_document_format
//   enum: how the layer writes the documents it returns
// (dj_mongodb_document_format): canonical Extended JSON, every value's type
// written out, which reads back the same; relaxed, numbers as JSON writes
// them, an int64 that fits 32 bits reading back as an int32; or BSON's
// bytes, a column of field_type::binary.
enum class mongo_document_format : re_std::uint8_t
{
    canonical = 0,
    relaxed   = 1,
    bson      = 2
};

// 4.3    Deployments
//------------------------------------------------------------------------------
// 4.3.1
// mongo_topology
//   enum: what a connection reached, as the server it selected first
// described itself (dj_mongodb_topology).
enum class mongo_topology : re_std::uint8_t
{
    unknown       = 0,  // not connected, or a server that did not say
    standalone    = 1,  // one mongod, no replica set
    replica_set   = 2,  // a member of a replica set
    sharded       = 3,  // a mongos, in front of shards
    load_balanced = 4   // a load balancer, in front of mongos
};

// 4.4    Authentication
//------------------------------------------------------------------------------
// 4.4.1
// mongo_auth_mechanism
//   enum: how the connection signs in; none leaves it to the driver, which
// negotiates SCRAM with the server where a user is given.
enum class mongo_auth_mechanism : re_std::uint8_t
{
    none          = 0,
    scram_sha_1   = 1,  // SCRAM-SHA-1
    scram_sha_256 = 2,  // SCRAM-SHA-256
    x509          = 3,  // MONGODB-X509, the client's certificate
    ldap          = 4,  // PLAIN, through LDAP (Enterprise)
    kerberos      = 5,  // GSSAPI (Enterprise)
    oidc          = 6,  // MONGODB-OIDC (7.0, Atlas and Enterprise; the
                        // driver's from 2.0)
    aws           = 7   // MONGODB-AWS, AWS's credentials
};


//==============================================================================
// 5.  CONNECTION CONFIGURATION
//==============================================================================


// 5.1    The configuration
//------------------------------------------------------------------------------
// 5.1.1
// mongo_connect_config
//   struct: MongoDB's settings, beside the generic ones in base. A
// connection string says where -- one host or several, mongodb:// or
// mongodb+srv:// -- and then base's host and port are not read; without one,
// they are. base's user, password and database, where given, replace the
// string's; its connect_timeout is the socket's, its read_timeout the
// reply's (0: as long as it takes); enable_ssl insists on TLS, ssl_ca names
// the authorities, ssl_cert the client's certificate and its key, one PEM
// file -- ssl_key, a key apart, is refused -- and verify_ssl false accepts
// a certificate no authority signed, or for another host. base's charset,
// schema, write_timeout and max_packet_size have no meaning for MongoDB, and
// are not read. Each optional setting left empty is the deployment's, the
// driver's or the layer's default; base's custom_options, then
// extra_options, are passed as a connection string's options, by name.
struct mongo_connect_config
{
    connection_config                    base;
    std::string                          connection_uri;
    std::string                          app_name;        // "": the layer's
    std::string                          auth_source;     // "": the driver's
                                                          // rule
    std::string                          replica_set;     // "": none needed
    mongo_auth_mechanism                 auth_mechanism;  // none
    std::optional<mongo_read_concern>    read_concern;
    std::optional<mongo_write_concern>   write_concern;
    std::optional<mongo_read_preference> read_preference;
    std::optional<int>                   server_selection_timeout_ms;
    std::optional<bool>                  retry_reads;     // the driver's: on
    std::optional<bool>                  retry_writes;    // the driver's: on
    std::string                          compressors;     // "zstd,snappy"
    std::string                          stable_api_version;  // "": none;
                                                              // "1" (5.0)
    bool                                 stable_api_strict;   // false
    bool                                 direct_connection;   // false
    bool                                 load_balanced;       // false

    std::map<std::string, std::string> extra_options;

    // construction -- the defaults (localhost, 27017); a generic
    // configuration; or a connection string alone
    mongo_connect_config()
        : auth_mechanism(mongo_auth_mechanism::none),
          stable_api_strict(false),
          direct_connection(false),
          load_balanced(false)
    {
        base.host = "localhost";
        base.port = 27017;
    }

    explicit mongo_connect_config(
        const connection_config& _base
    )
        : base(_base),
          auth_mechanism(mongo_auth_mechanism::none),
          stable_api_strict(false),
          direct_connection(false),
          load_balanced(false)
    {}

    explicit mongo_connect_config(
        const std::string& _uri
    )
        : connection_uri(_uri),
          auth_mechanism(mongo_auth_mechanism::none),
          stable_api_strict(false),
          direct_connection(false),
          load_balanced(false)
    {
        // the string alone says where
        base.host.clear();
        base.port = 0;
    }

    // with_replica_set -- a replica set's connection string, and the
    // database to use
    static
    mongo_connect_config with_replica_set(
        const std::string& _uri,
        const std::string& _database
    )
    {
        mongo_connect_config config(_uri);

        config.base.database = _database;

        return config;
    }

    // with_tls -- a connection string, TLS insisted on, and the authorities
    // that vouch for the server's certificate
    static
    mongo_connect_config with_tls(
        const std::string& _uri,
        const std::string& _ca_file
    )
    {
        mongo_connect_config config(_uri);

        config.base.enable_ssl = true;
        config.base.ssl_ca     = _ca_file;

        return config;
    }

    // with_atlas -- an Atlas cluster's mongodb+srv:// string, which implies
    // TLS, and the database to use
    static
    mongo_connect_config with_atlas(
        const std::string& _srv_uri,
        const std::string& _database
    )
    {
        mongo_connect_config config(_srv_uri);

        config.base.database   = _database;
        config.base.enable_ssl = true;

        return config;
    }
};


//==============================================================================
// 6.  MONGODB CONNECTION
//==============================================================================


// 6.1    Results
//------------------------------------------------------------------------------
// 6.1.1
// mongo_write_result
//   struct: what a write did, as the server counted it
// (dj_mongodb_write_counts): documents inserted; matched by an update's
// filter; changed by it, one it matched and left as it was not counted;
// deleted; and inserted by an upsert, none having matched.
struct mongo_write_result
{
    re_std::int64_t inserted = 0;
    re_std::int64_t matched  = 0;
    re_std::int64_t modified = 0;
    re_std::int64_t deleted  = 0;
    re_std::int64_t upserted = 0;
};

// 6.1.2
// mongo_validation
//   struct: what validate found of a collection: whether it is sound, and
// the server's report, as canonical Extended JSON.
struct mongo_validation
{
    bool        valid = false;
    std::string report;
};

// 6.2    The layer
//------------------------------------------------------------------------------
// 6.2.1
// mongodb_layer
//   struct: the MongoDB layer, as database_connection takes a layer (its
// run_layer and ask_layer). It has no prepared statements, so it names no
// statement_face, and prepared_statement and streaming_result_set are never
// made of it. A result means the connection was lost when it is the
// driver's failure to reach the deployment (d_mongodb_result_is_unreachable)
// -- a connection that broke or could not be made, or no server to use in
// the time the driver waits for one -- which leaves the connection in error
// until reconnect(), though the driver would try again by itself.
struct mongodb_layer
{
    using handle_type = struct ::dj_mongodb*;

    // holds_connection -- false: a cursor or a change stream reads through
    // the connection while it runs other operations
    static constexpr bool holds_connection = false;

    // connection_lost -- whether a result means the connection was lost, as
    // the description above says
    static
    bool connection_lost(
        int _result
    ) noexcept
    {
        return face::mongodb::result_is_unreachable(_result);
    }

    /**
     * @brief Describes a failed call of the MongoDB layer, as raise does.
     *
     * @note The handle keeps the message of its latest failure, until the
     *       next; a failure the handle did not record -- one before there
     *       was a handle, or one of a NULL handle -- has the result's own
     *       description.
     *
     * @param[in] _db         the handle the call ran on; may be `NULL`.
     * @param[in] _operation  the operation's name.
     * @param[in] _result     the call's result.
     * @return "MongoDB", the operation, and the failure, in the words of the
     *         server, the driver or the layer; then the code: the server's,
     *         and its name ("11000, DuplicateKey"), the driver's, libbson's
     *         or the layer's domain and code, or db_common.h's number.
     * @throws std::bad_alloc when memory runs out.
     */
    static
    std::string describe(
        handle_type _db,
        const char* _operation,
        int         _result
    );

    /**
     * @brief Throws the exception for a failure already described.
     *
     * @param[in] _kind     which exception to throw, unless `_result` means
     *                      the connection was lost.
     * @param[in] _message  the failure, as describe gives it.
     * @param[in] _result   the call's result.
     * @throws connection_exception for a lost connection, otherwise as
     *         `_kind` says, with `_message` and `_result` as its error_code().
     */
    [[noreturn]] static
    void raise_as(
        failure_kind       _kind,
        const std::string& _message,
        int                _result
    );

    /**
     * @brief Throws the exception for a failed call of the MongoDB layer.
     *
     * @param[in] _db         the handle the call ran on; may be `NULL`.
     * @param[in] _kind       which exception to throw, unless `_result`
     *                        means the connection was lost.
     * @param[in] _operation  the operation's name, for the message.
     * @param[in] _result     the call's result.
     * @throws connection_exception for a lost connection, otherwise as
     *         `_kind` says, with `_result` -- the server's code, the
     *         driver's, libbson's or the layer's packed, or a negative D_DB_*
     *         -- as its error_code().
     */
    [[noreturn]] static
    void raise(
        handle_type  _db,
        failure_kind _kind,
        const char*  _operation,
        int          _result
    );
};

// 6.3    Cursors and change streams
//------------------------------------------------------------------------------
class mongo_connection;

// 6.3.1
// mongo_cursor
//   class: a query's documents, read a page at a time as the server sends
// them (mongodb_cursor.h): a find's, an aggregation's, or a command's whose
// reply holds a cursor, as mongo_connection's find_cursor,
// aggregate_cursor and command_cursor open them. Each page is a
// buffered_result_set of documents, one column, "document"; a page shorter
// than the size asked for, none possibly, says the documents are done. A
// cursor opened in a transaction belongs to it: once the transaction ends,
// a read of it throws query_exception with
// D_MONGODB_ERROR_TRANSACTION_CURSOR. It belongs to its connection's
// session: closing the connection closes it, and a closed cursor throws
// query_exception, D_DB_INVALID_ARGUMENT, on a read. A failure throws as
// the connection's operations do, a lost connection marking the
// connection lost. Not copyable or movable: the session knows it by its
// address, and the connection hands it out as std::unique_ptr.
class mongo_cursor
{
public:
    /**
     * @brief Closes the cursor, unless the connection closed it already.
     */
    ~mongo_cursor();

    mongo_cursor(
        const mongo_cursor&
    ) = delete;
    mongo_cursor& operator=(
        const mongo_cursor&
    ) = delete;
    mongo_cursor(
        mongo_cursor&&
    ) = delete;
    mongo_cursor& operator=(
        mongo_cursor&&
    ) = delete;

    /**
     * @brief Reads the next page of documents.
     *
     * @param[in] _max  the most documents the page holds; 1 or more.
     * @return the page, held apart from the cursor: `_max` documents, or
     *         fewer at the end, none once the cursor is done.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a closed cursor
     *         or a `_max` of 0; with the server's code for a batch it could
     *         not send, which ends the cursor; connection_exception where
     *         the deployment cannot be reached.
     */
    std::unique_ptr<buffered_result_set> fetch(
        std::size_t _max
    );

    // state -- whether the cursor is open, and whether its documents are
    // done: true once a page came back short, or the cursor closed
    bool is_open() const noexcept;
    bool is_done() const noexcept;

    /**
     * @brief Closes the cursor, which the server then forgets.
     *
     * @post The cursor is closed: a read throws query_exception.
     */
    void close() noexcept;

private:
    friend class mongo_connection;

    // made by the connection, which has opened the driver's cursor
    mongo_cursor(
        std::shared_ptr<internal::session> _session,
        struct ::dj_mongodb*               _db,
        face::mongodb_cursor&&             _cursor
    );

    // m_closed -- the session closing the cursor, as the connection closes
    static
    void m_closed(
        void* _cursor
    ) noexcept;

    std::shared_ptr<internal::session> m_session;
    struct ::dj_mongodb*                m_db;
    face::mongodb_cursor               m_cursor;
};

// 6.3.2
// mongo_change_stream
//   class: the changes made to a collection, a database or a whole
// deployment, as they happen (mongodb_cursor.h), as mongo_connection's
// watch_collection, watch_database and watch_deployment open them. Each
// fetch waits for events as long as the options' maxAwaitTimeMS -- the
// server's own wait, a second, without one -- and returns those that
// arrived, none when none did, each a document of one column, "document".
// The stream resumes by itself after a failure the driver can resume from;
// its resume token says where it is, for a stream opened later to start
// after ("resumeAfter" or "startAfter" among its options). It belongs to its
// connection's session as a cursor does, and throws as a cursor does. Not
// copyable or movable.
class mongo_change_stream
{
public:
    /**
     * @brief Closes the stream, unless the connection closed it already.
     */
    ~mongo_change_stream();

    mongo_change_stream(
        const mongo_change_stream&
    ) = delete;
    mongo_change_stream& operator=(
        const mongo_change_stream&
    ) = delete;
    mongo_change_stream(
        mongo_change_stream&&
    ) = delete;
    mongo_change_stream& operator=(
        mongo_change_stream&&
    ) = delete;

    /**
     * @brief Reads the events that arrive, waiting as the options said.
     *
     * @param[in] _max  the most events to read; 1 or more.
     * @return the events, held apart from the stream; none when none
     *         arrived in the wait.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a closed stream
     *         or a `_max` of 0; with the server's code, or the driver's, for
     *         a failure the stream could not resume from, which ends it;
     *         connection_exception where the deployment cannot be reached.
     */
    std::unique_ptr<buffered_result_set> fetch(
        std::size_t _max
    );

    /**
     * @brief Says where the stream is: the resume token of the last event
     *        read, or of the latest batch the server sent.
     *
     * @return the token, a document, as canonical Extended JSON;
     *         std::nullopt before the stream has one.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a closed
     *         stream.
     */
    std::optional<std::string> resume_token() const;

    // is_open -- whether the stream is open
    bool is_open() const noexcept;

    /**
     * @brief Closes the stream.
     *
     * @post The stream is closed: a read throws query_exception.
     */
    void close() noexcept;

private:
    friend class mongo_connection;

    // made by the connection, which has opened the driver's stream
    mongo_change_stream(
        std::shared_ptr<internal::session> _session,
        struct ::dj_mongodb*               _db,
        face::mongodb_change_stream&&      _stream
    );

    // m_closed -- the session closing the stream, as the connection closes
    static
    void m_closed(
        void* _stream
    ) noexcept;

    std::shared_ptr<internal::session> m_session;
    struct ::dj_mongodb*                m_db;
    face::mongodb_change_stream        m_stream;
};

// 6.4    The class
//------------------------------------------------------------------------------
// 6.4.1
// mongo_connection
//   class: a connection to a MongoDB deployment, through the driver, over
// the MongoDB layer's face, which it holds and which closes the connection
// when it goes. Connecting gives the layer what mongo_connect_config names,
// a connection string first, the settings after it, and pings the server;
// a connection is either open with all of it or not open. execute_query,
// execute_update and execute run a command on the current database, as the
// banner says: execute_query returns its cursor's documents, every batch,
// or its reply as one row for a command without one, and execute_update
// the documents it wrote.
//   Every other member is one call of the layer on the current database,
// which the configuration's database, or the connection string's, names
// first, and use_database changes; a filter, a document, an update, a
// pipeline or options is Extended JSON, and an empty string means none --
// every document, for a filter -- where the layer takes none. Documents come
// back as the document format writes them, canonical Extended JSON unless
// set otherwise. Each write records what it changed, which
// get_affected_rows() then answers.
//   Transactions run in the connection's own session, on a replica set, a
// sharded cluster or a load balancer: begin_transaction, commit and
// rollback, and begin_transaction with options -- readConcern,
// writeConcern, readPreference, maxCommitTimeMS. A failure inside one
// fails it on the server: everything but a rollback then throws,
// in_failed_transaction() saying so. There is no auto-commit mode to turn
// off: outside a transaction every write stands on its own.
//   A member throws as the operations do (database_connection.hpp):
// connection_exception when the connection is not open, or a failure lost
// it, and otherwise the exception its contract names, with the layer's
// result -- the server's code as it sent it, the driver's, libbson's or the
// layer's packed above every code a server uses, or a negative D_DB_* -- as
// its error_code(); text holding a NUL is refused first, with
// D_DB_INVALID_ARGUMENT. Not copyable; movable.
//
// Usage:
//   mongo_connection conn("mongodb://db.example/shop?replicaSet=rs0");
//   conn.connect();
//   conn.insert_one("orders", R"({"item": "pen", "qty": 3})");
//   auto found = conn.find("orders", R"({"qty": {"$gt": 1}})");
class mongo_connection
    : public database_connection<mongo_connection, database_type::mongodb>
{
public:
    using base_type           = database_connection<mongo_connection,
                                                    database_type::mongodb>;
    using layer_type          = mongodb_layer;
    using client_capabilities = mongo_client_capabilities;
    using version_info        = mongo_version_info;

    // type_support, feature_support -- client_capabilities, by their former
    // names (2026.10.28), for one release
    using type_support [[deprecated("client_capabilities")]] =
        mongo_client_capabilities;
    using feature_support [[deprecated("client_capabilities")]] =
        mongo_client_capabilities;

    // the base's begin_transaction, beside the one with options below
    using base_type::begin_transaction;

    // construction -- with mongo_connect_config's defaults (localhost,
    // 27017); with a generic configuration; with a MongoDB one; or with a
    // connection string alone. Nothing connects before connect()
    mongo_connection();
    explicit
    mongo_connection(
        const connection_config& _config
    );
    explicit
    mongo_connection(
        const mongo_connect_config& _config
    );
    explicit
    mongo_connection(
        const std::string& _uri
    );

    /**
     * @brief Closes the connection, its cursors and change streams first.
     */
    ~mongo_connection();

    mongo_connection(
        const mongo_connection&
    ) = delete;
    mongo_connection& operator=(
        const mongo_connection&
    ) = delete;

    mongo_connection(
        mongo_connection&&
    ) noexcept = default;
    mongo_connection& operator=(
        mongo_connection&&
    ) noexcept = default;

    /**
     * @brief Opens the connection over a client a program made with the
     *        driver's own calls (d_mongodb_adopt), as connect() opens one
     *        from the configuration, which then names only the database.
     *
     * @note The client must read failures in the driver's second error API,
     *       or never have been set to an error API; it is pinged, as a
     *       connect pings. A reconnect() connects from the configuration.
     *
     * @param[in] _native  the mongoc_client_t*.
     * @param[in] _owned   whether closing the connection destroys the client
     *                     as well.
     * @throws connection_exception with D_DB_INVALID_ARGUMENT for a NULL
     *         client, or one set to the first error API; with the server's
     *         or the driver's code when the server cannot be reached; as
     *         connect() otherwise. On failure the client is the program's,
     *         whatever `_owned` said.
     */
    void adopt(
        void* _native,
        bool  _owned
    );

    /**
     * @brief Runs a command on a database, and returns its reply.
     *
     * @param[in] _command   the command, as Extended JSON.
     * @param[in] _database  the database; "" for the current one.
     * @return the reply, as the document format writes it.
     * @throws query_exception with the server's code when it refuses the
     *         command, or the reply reports a write's failure; with
     *         libbson's code, packed, for text that is not JSON.
     */
    std::string run_command(
        const std::string& _command,
        const std::string& _database = ""
    );

    // 6.4.1.1  Documents
    //   mongodb_document.h, on the current database. Each throws
    //   query_exception with the server's code for a write or a read it
    //   refuses (11000 for a duplicate key, 121 for a document its validator
    //   refuses), libbson's for text that is not JSON, and
    //   D_DB_INVALID_ARGUMENT for JSON of the wrong shape or no collection.

    /**
     * @brief Inserts a document, given an ObjectId as its _id where it has
     *        none.
     *
     * @param[in] _collection  the collection, made if it is not there.
     * @param[in] _document    the document.
     * @return the document's _id, as Extended JSON: {"$oid": "..."} for one
     *         the layer made.
     * @throws as the section says.
     */
    std::string insert_one(
        const std::string& _collection,
        const std::string& _document
    );

    /**
     * @brief Inserts the documents of a JSON array, in its order.
     *
     * @param[in] _collection  the collection.
     * @param[in] _documents   the documents, a JSON array.
     * @param[in] _ordered     whether to stop at the first that fails;
     *                         otherwise every one is tried, and the first
     *                         failure thrown.
     * @return how many went in.
     * @throws as the section says, after those before the failure went in.
     */
    re_std::int64_t insert_many(
        const std::string& _collection,
        const std::string& _documents,
        bool               _ordered = true
    );

    // finding -- the documents a filter matches, a find's options among
    // them (sort, projection, skip, limit, hint, collation, ...): every one,
    // a row each; the first in the options' sort, std::nullopt for none; and
    // every one beside its _id, as d_mongodb_find_keyed returns them, both
    // canonical whatever the document format
    std::unique_ptr<buffered_result_set> find(
        const std::string& _collection,
        const std::string& _filter = "",
        const std::string& _options = ""
    );
    std::optional<std::string> find_one(
        const std::string& _collection,
        const std::string& _filter = "",
        const std::string& _options = ""
    );
    std::unique_ptr<buffered_result_set> find_keyed(
        const std::string& _collection,
        const std::string& _filter = "",
        const std::string& _options = ""
    );

    // updating and replacing -- the first document the filter matches, or
    // every one, with update operators ({"$set": ...}) or a pipeline; or the
    // first replaced by a document without operators; the options an
    // update's (upsert, arrayFilters, collation, hint, let, comment). Each
    // returns what it did
    mongo_write_result update_one(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _update,
        const std::string& _options = ""
    );
    mongo_write_result update_many(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _update,
        const std::string& _options = ""
    );
    mongo_write_result replace_one(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _replacement,
        const std::string& _options = ""
    );

    // deleting -- the first document the filter matches, or every one; the
    // options a delete's. Each returns how many it deleted
    re_std::int64_t delete_one(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _options = ""
    );
    re_std::int64_t delete_many(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _options = ""
    );

    // counting -- the documents a filter matches, exactly (skip, limit,
    // hint, collation among the options); and the collection's documents
    // as its metadata counts them, quickly, which a transaction refuses
    // (D_DB_UNSUPPORTED). A collection that is not there has none
    re_std::int64_t count_documents(
        const std::string& _collection,
        const std::string& _filter = "",
        const std::string& _options = ""
    ) const;
    re_std::int64_t estimated_document_count(
        const std::string& _collection,
        const std::string& _options = ""
    ) const;

    // distinct, aggregating -- a field's values among the documents a
    // filter matches, a row each, one column, "value"; and a pipeline's
    // documents, the pipeline a JSON array of stages, on a collection, or
    // on the database for a collection of "" ($currentOp, $documents)
    std::unique_ptr<buffered_result_set> distinct(
        const std::string& _collection,
        const std::string& _field,
        const std::string& _filter = "",
        const std::string& _options = ""
    );
    std::unique_ptr<buffered_result_set> aggregate(
        const std::string& _collection,
        const std::string& _pipeline,
        const std::string& _options = ""
    );

    // finding and modifying -- the first document the filter matches,
    // updated, replaced or deleted in one step, and returned as it was, or
    // as it became with "returnDocument": "after" among the options;
    // std::nullopt where none matched and none was upserted
    std::optional<std::string> find_one_and_update(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _update,
        const std::string& _options = ""
    );
    std::optional<std::string> find_one_and_replace(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _replacement,
        const std::string& _options = ""
    );
    std::optional<std::string> find_one_and_delete(
        const std::string& _collection,
        const std::string& _filter,
        const std::string& _options = ""
    );

    /**
     * @brief Sends many writes to a collection, in as few commands as the
     *        server allows.
     *
     * @param[in] _collection  the collection.
     * @param[in] _operations  the writes, a JSON array of models as the
     *                         drivers write them: {"insertOne": {"document":
     *                         ...}}, {"updateOne" or "updateMany": {"filter",
     *                         "update", ...}}, {"replaceOne": {"filter",
     *                         "replacement", ...}}, {"deleteOne" or
     *                         "deleteMany": {"filter", ...}}.
     * @param[in] _ordered     whether to stop at the first that fails.
     * @param[in] _options     bypassDocumentValidation, comment, let,
     *                         writeConcern; or "".
     * @return what the writes did.
     * @throws as the section says, with the first write error's code, after
     *         the writes before it were made; the server's whole reply then
     *         holds every error (get_error_reply).
     */
    mongo_write_result bulk_write(
        const std::string& _collection,
        const std::string& _operations,
        bool               _ordered = true,
        const std::string& _options = ""
    );

    /**
     * @brief Sends many writes, to any collections of any databases, as the
     *        server's one bulkWrite command (8.0; driver 1.28).
     *
     * @param[in] _operations  the writes, as bulk_write's, each with a
     *                         "namespace", "db.collection", beside its
     *                         fields.
     * @param[in] _ordered     whether to stop at the first that fails.
     * @param[in] _options     as bulk_write's; or "".
     * @return what the writes did.
     * @throws as bulk_write; with D_DB_UNSUPPORTED before server 8.0 or
     *         driver 1.28 (supports_bulk_write).
     */
    mongo_write_result client_bulk_write(
        const std::string& _operations,
        bool               _ordered = true,
        const std::string& _options = ""
    );

    // 6.4.1.2  Cursors and change streams
    //   mongodb_cursor.h. Each returns what it opened, which belongs to the
    //   connection's session (mongo_cursor); a change stream needs a
    //   replica set or a sharded cluster (supports_change_streams), and
    //   throws query_exception with D_DB_UNSUPPORTED on a standalone server.

    // cursors -- a find, an aggregation, or any command whose reply holds a
    // cursor, read a page at a time
    std::unique_ptr<mongo_cursor> find_cursor(
        const std::string& _collection,
        const std::string& _filter = "",
        const std::string& _options = ""
    );
    std::unique_ptr<mongo_cursor> aggregate_cursor(
        const std::string& _collection,
        const std::string& _pipeline,
        const std::string& _options = ""
    );
    std::unique_ptr<mongo_cursor> command_cursor(
        const std::string& _command
    );

    // change streams -- a collection's, the current database's, or the whole
    // deployment's changes, a pipeline of $match and $project stages
    // filtering them, and the options a stream's (fullDocument,
    // resumeAfter, startAfter, startAtOperationTime, maxAwaitTimeMS,
    // batchSize)
    std::unique_ptr<mongo_change_stream> watch_collection(
        const std::string& _collection,
        const std::string& _pipeline = "",
        const std::string& _options = ""
    );
    std::unique_ptr<mongo_change_stream> watch_database(
        const std::string& _pipeline = "",
        const std::string& _options = ""
    );
    std::unique_ptr<mongo_change_stream> watch_deployment(
        const std::string& _pipeline = "",
        const std::string& _options = ""
    );

    // 6.4.1.3  Databases, collections, views and indexes
    //   mongodb_database.h and mongodb_schema.h; a name is the database's,
    //   the collection's, the view's or the index's own. Each throws
    //   query_exception with the server's code when it refuses (48 for a
    //   collection there already, 26 for one that is not), and with
    //   D_DB_INVALID_ARGUMENT for an empty name.

    // databases -- whether the deployment has one of the name (MongoDB has
    // no empty database: one exists from its first collection); dropping
    // one, a database not there a success with _if_exists; the names; and
    // making one current, which the configuration then names
    bool has_database(
        const std::string& _name
    ) const;
    void drop_database(
        const std::string& _name,
        bool               _if_exists = true
    );
    std::vector<std::string> list_databases() const;
    void use_database(
        const std::string& _name
    );

    // collections -- whether the current database has one of the name, a
    // view not counting; making one with the create command's options
    // ({"capped": true, "size": 4096}, {"timeseries": {...}}, {"validator":
    // {...}}), one there a success with _if_not_exists; dropping one, its
    // indexes with it, or a view; renaming one within the database, a
    // collection there under the new name dropped first with _drop_target;
    // the names, views left out; and what listCollections says of one (its
    // type, options, info and _id index), canonical
    bool has_collection(
        const std::string& _name
    ) const;
    void create_collection(
        const std::string& _name,
        const std::string& _options = "",
        bool               _if_not_exists = true
    );
    void drop_collection(
        const std::string& _name,
        bool               _if_exists = true
    );
    void rename_collection(
        const std::string& _from,
        const std::string& _to,
        bool               _drop_target = false
    );
    std::vector<std::string> list_collections() const;
    std::string get_collection_info(
        const std::string& _name
    ) const;

    // views -- whether there is one of the name; making one of a collection
    // or a view, over a pipeline, with a collation among the options; one
    // there a success with _if_not_exists; dropping one, never a
    // collection; and the names
    bool has_view(
        const std::string& _name
    ) const;
    void create_view(
        const std::string& _name,
        const std::string& _source,
        const std::string& _pipeline,
        const std::string& _options = "",
        bool               _if_not_exists = true
    );
    void drop_view(
        const std::string& _name,
        bool               _if_exists = true
    );
    std::vector<std::string> list_views() const;

    // indexes -- whether a collection has one of the name; building one
    // over keys ({"qty": 1}), unique or not, with createIndexes' options,
    // the collection made if it is not there, one of the name there a
    // success with _if_not_exists (whatever its keys); dropping one; the
    // names, "_id_" first; and each one's description, a row each
    bool has_index(
        const std::string& _collection,
        const std::string& _name
    ) const;
    void create_index(
        const std::string& _collection,
        const std::string& _name,
        const std::string& _keys,
        bool               _unique = false,
        const std::string& _options = "",
        bool               _if_not_exists = true
    );
    void drop_index(
        const std::string& _collection,
        const std::string& _name,
        bool               _if_exists = true
    );
    std::vector<std::string> list_indexes(
        const std::string& _collection
    ) const;
    std::unique_ptr<buffered_result_set> describe_indexes(
        const std::string& _collection
    ) const;

    // 6.4.1.4  Transactions
    /**
     * @brief Begins a transaction with options of its own.
     *
     * @param[in] _options  readConcern, writeConcern, readPreference,
     *                      maxCommitTimeMS, as JSON; "" for the connection's
     *                      concerns and preference.
     * @throws transaction_exception with D_DB_UNSUPPORTED where the
     *         deployment has no transactions, with the layer's result for
     *         one already open or options it cannot read; as every
     *         operation when the connection is not open.
     */
    void begin_transaction(
        const std::string& _options
    );

    // in_failed_transaction -- whether the open transaction failed on the
    // server, so that only a rollback is left; false outside one
    bool in_failed_transaction() const noexcept;

    // 6.4.1.5  GridFS
    //   mongodb_gridfs.h: files kept as chunks in a bucket, "fs" for a
    //   bucket of "", of the current database. An _id is Extended JSON, as
    //   an upload returns it. Each throws query_exception with
    //   D_DB_NOT_FOUND for no file of the _id, and with the server's or the
    //   driver's code otherwise.

    // uploading -- bytes, or a file of the client's, under a file name, with
    // metadata, as JSON; each returns the file's _id
    std::string gridfs_upload(
        const std::string&                  _filename,
        const std::vector<re_std::uint8_t>& _data,
        const std::string&                  _metadata = "",
        const std::string&                  _bucket = ""
    );
    std::string gridfs_upload_file(
        const std::string& _filename,
        const std::string& _path,
        const std::string& _metadata = "",
        const std::string& _bucket = ""
    );

    // downloading and deleting -- a file's bytes, or into a file of the
    // client's; and the file, its chunks with it
    std::vector<re_std::uint8_t> gridfs_download(
        const std::string& _id,
        const std::string& _bucket = ""
    );
    void gridfs_download_file(
        const std::string& _id,
        const std::string& _path,
        const std::string& _bucket = ""
    );
    void gridfs_delete(
        const std::string& _id,
        const std::string& _bucket = ""
    );

    // finding -- the newest file of a name, std::nullopt for none; and the
    // files' descriptions a filter matches, a row each
    std::optional<std::string> gridfs_find_id(
        const std::string& _filename,
        const std::string& _bucket = ""
    );
    std::unique_ptr<buffered_result_set> gridfs_find(
        const std::string& _filter = "",
        const std::string& _options = "",
        const std::string& _bucket = ""
    );

    // 6.4.1.6  Upkeep
    //   mongodb_maintenance.h: a collection's size in bytes, its documents
    //   and indexes as stored; the current database's; compacting a
    //   collection, which a replica set's primary runs only when forced, as
    //   the layer forces it; and validating one, the full checks with
    //   _full
    re_std::int64_t get_collection_size(
        const std::string& _collection
    ) const;
    re_std::int64_t get_database_size() const;
    void compact(
        const std::string& _collection
    );
    mongo_validation validate(
        const std::string& _collection,
        bool               _full = false
    ) const;

    // 6.4.1.7  Settings
    //   mongodb_settings.h: the connection's defaults, which the document
    //   members, the cursors and the transactions follow and their options
    //   may replace for one operation; commands run as written. Each is
    //   given and read back as MongoDB writes it in a command, as JSON --
    //   {"level": "majority"}, {"w": "majority", "j": true}, {"mode":
    //   "secondaryPreferred", "maxStalenessSeconds": 120} -- "" setting the
    //   deployment's own default, which reads back as "{}" (a read
    //   preference as primary's); or as its enum. Each throws
    //   query_exception with D_DB_INVALID_ARGUMENT for JSON that is not the
    //   setting, a level, a mode or a w of no such name, and a w of 0
    void set_read_concern(
        const std::string& _concern
    );
    void set_read_concern(
        mongo_read_concern _level
    );
    std::string get_read_concern() const;
    void set_write_concern(
        const std::string& _concern
    );
    void set_write_concern(
        mongo_write_concern _concern
    );
    std::string get_write_concern() const;
    void set_read_preference(
        const std::string& _preference
    );
    void set_read_preference(
        mongo_read_preference _mode
    );
    std::string get_read_preference() const;

    // document format and time limit -- how the documents returned are
    // written, canonical until set; and the maxTimeMS each read whose options
    // name none is given, 0 for none, a negative one refused. Read back while
    // the connection is not open, canonical and 0
    void set_document_format(
        mongo_document_format _format
    );
    mongo_document_format get_document_format() const noexcept;
    void set_max_time_ms(
        re_std::int64_t _milliseconds
    );
    re_std::int64_t get_max_time_ms() const noexcept;

    // 6.4.1.8  The server, the driver and errors
    //   What the server connected said of itself when the connection opened,
    //   read with no round trip: 0, false, "" and unknown while the
    //   connection is not open.

    // the server -- its version's number, MAJOR*10000 + MINOR*100 + PATCH,
    // whether it is at a version or later, the newest wire protocol it
    // speaks, what the connection reached, and the replica set's name
    unsigned long server_version_number() const noexcept;
    bool server_at_least(
        unsigned _major,
        unsigned _minor,
        unsigned _patch = 0u
    ) const noexcept;
    int server_wire_version() const noexcept;
    mongo_topology get_server_topology() const noexcept;
    std::string get_replica_set_name() const;

    // feature queries -- what the server connected, its deployment and the
    // driver can do (d_mongodb_supports): multi-document transactions (a
    // replica set from 4.0, a sharded cluster from 4.2, a load balancer);
    // change streams (not standalone); time series collections (5.0);
    // clustered collections (5.3); a declared API version (5.0, driver
    // 1.18); reads at a point in time outside a transaction (5.0, not
    // standalone); change events carrying a document as it was (6.0, not
    // standalone); one bulk write across collections (8.0, driver 1.28);
    // hidden indexes (4.4); wildcard indexes (4.2). client_capabilities
    // answers for the headers, at compile time
    bool supports_transactions() const noexcept;
    bool supports_change_streams() const noexcept;
    bool supports_time_series() const noexcept;
    bool supports_clustered_collections() const noexcept;
    bool supports_stable_api() const noexcept;
    bool supports_snapshot_reads() const noexcept;
    bool supports_pre_post_images() const noexcept;
    bool supports_bulk_write() const noexcept;
    bool supports_hidden_indexes() const noexcept;
    bool supports_wildcard_indexes() const noexcept;

    // the driver -- its version as the program loaded it, its number, and
    // libbson's version; "" and 0 in a build without the driver
    static
    std::string get_client_version();
    static
    unsigned long get_client_version_number() noexcept;
    static
    std::string get_bson_version();

    /**
     * @brief Sets the most verbose level of the driver's log the layer
     *        passes on, for the whole process (d_mongodb_set_log_level).
     *
     * @param[in] _level  0 (errors) to 6 (traces), or -1 for nothing.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a level outside
     *         -1 to 6; with D_DB_UNSUPPORTED in a build without the driver.
     */
    static
    void set_log_level(
        int _level
    );
    static
    int get_log_level() noexcept;

    // the latest failure, beyond its message and code -- the server's name
    // for its code ("DuplicateKey"), "" for none; whether the server or the
    // driver labelled it so ("TransientTransactionError",
    // "RetryableWriteError"), which says whether a transaction may be tried
    // again; and the server's whole reply to the request, which holds each
    // write error of a bulk write and the key a duplicate repeated,
    // std::nullopt where there was none. Each reads the handle's latest
    // failure, whichever member raised it; "", false and std::nullopt while
    // the connection is not open
    std::string get_error_name() const;
    bool has_error_label(
        const std::string& _label
    ) const;
    std::optional<std::string> get_error_reply() const;

    /**
     * @brief Hands back the _id of the document the latest insert_one gave
     *        the server, or the latest upsert inserted.
     *
     * @return the _id as Extended JSON; std::nullopt where no insert has
     *         happened on the connection, or the latest wrote no new
     *         document. get_last_insert_id() answers the same as a number,
     *         where it is an integer.
     * @throws as every operation when the connection is not open.
     */
    std::optional<std::string> get_last_inserted_id() const;

    // native_client -- the driver's mongoc_client_t*, for its own calls,
    // nullptr while the connection is not open: cast it where mongoc.h is
    // included; it stays the connection's, never to be destroyed
    void* get_native_client() const noexcept;

    // the operations that do not block -- refused, D_DB_UNSUPPORTED, since
    // the driver has none (ruling 11.26.6; database_connection.hpp)

    // type mapping -- a BSON type as field_type, and a field_type as the
    // BSON type name $type and $jsonSchema spell, as section 1 maps them
    static
    field_type map_bson_type(
        bson_type _bson
    ) noexcept
    {
        return bson_type_to_field_type(_bson);
    }

    static
    const char* bson_type_name(
        field_type _type
    ) noexcept
    {
        return field_type_to_bson_type_name(_type);
    }

    // configuration -- the MongoDB settings, base among them as the
    // connection's generic configuration now is, which connect(config)
    // replaces; setting them replaces the generic configuration with
    // theirs, for the next connect
    mongo_connect_config get_mongo_config() const;
    void set_mongo_config(
        const mongo_connect_config& _config
    );

    // get_face -- the MongoDB layer's face, for the layer's other calls --
    // its f forms, a cursor's own options; closing or replacing the handle
    // through it is the connection's to do, not the face's
    face::mongodb& get_face() noexcept;
    const face::mongodb& get_face() const noexcept;

    // hooks (mongodb.cpp) -- database_connection calls these, programs the
    // operations they implement, whose contracts they keep
    void connect_impl();
    void disconnect_impl() noexcept;
    bool is_connected_impl() const noexcept;
    bool ping_impl() const noexcept;
    face::rows execute_query_impl(
        const std::string& _command
    );
    re_std::int64_t execute_update_impl(
        const std::string& _command
    );
    std::string get_server_version_impl() const;
    re_std::int64_t get_last_insert_id_impl() const;
    void begin_transaction_impl();
    void commit_impl();
    void rollback_impl();
    bool in_transaction_impl() const noexcept;
    struct ::dj_mongodb* native_handle_impl() const noexcept;

private:
    // adoption
    //   struct: a client adopt() hands connect_impl, in place of the
    // configuration's.
    struct adoption
    {
        void* native;
        bool  owned;
    };

    /**
     * @brief Raises a failure of the MongoDB layer, as mongodb_layer::raise
     *        does; a success returns.
     *
     * @param[in] _result     the call's result.
     * @param[in] _kind       which exception a failure throws, unless it lost
     *                        the connection.
     * @param[in] _operation  the operation's name, for the message.
     * @throws as mongodb_layer::raise, unless `_result` is 0.
     */
    void m_check(
        int          _result,
        failure_kind _kind,
        const char*  _operation
    ) const;

    // m_wrote -- what the latest write changed, as get_affected_rows() then
    // answers
    void m_wrote() noexcept;

    // m_supports -- the layer's answer for the server connected; false
    // while the connection is not open
    bool m_supports(
        dj_mongodb_feature _feature
    ) const noexcept;

    // m_document -- the first row's document, as text or as bytes;
    // std::nullopt for no row
    static
    std::optional<std::string> m_document(
        const face::rows& _rows
    );

    /**
     * @brief A find-and-modify's document, what it changed recorded first.
     *
     * @param[in] _result     the call's result.
     * @param[in] _found      the rows it wrote.
     * @param[in] _operation  the operation's name, for a failure.
     * @return the document; std::nullopt for none.
     * @throws as m_check, unless `_result` is 0 or D_DB_NOT_FOUND.
     */
    std::optional<std::string> m_modified(
        int               _result,
        const face::rows& _found,
        const char*       _operation
    );

    // m_rows -- rows a call wrote, as a result set
    static
    std::unique_ptr<buffered_result_set> m_rows(
        face::rows&& _rows
    );

    // m_cursor, m_stream -- what a call opened, enrolled in the session
    std::unique_ptr<mongo_cursor> m_cursor(
        face::mongodb_cursor&& _opened
    );
    std::unique_ptr<mongo_change_stream> m_stream(
        face::mongodb_change_stream&& _opened
    );

    // m_connect -- the handle the configuration names, opened into
    // `_opened`; connection_exception on failure
    void m_connect(
        face::mongodb& _opened
    );

    mongo_connect_config      m_mongo_config;
    face::mongodb             m_db;
    std::optional<adoption>   m_adopting;
};


//==============================================================================
// 7.  EXPRESSION DETECTORS
//==============================================================================
// Sections 7 to 11, once mongo_traits.hpp and mongo_concepts.hpp, detect
// which of mongo_connection's members a type has: as traits from C++17, and
// as concepts from C++20, which forward to the traits. Each detector is the
// member's call, its text arguments a const std::string& each (2026.10.28:
// MongoDB's transactions are begin_transaction, commit and rollback, its
// many writes bulk_write, its concerns JSON, and an index takes a name, as
// the layer's do; start_session, which no member is, went).


// 7.1    Documents
//------------------------------------------------------------------------------
// mongo_insert_one_t
//   detector: insert_one(collection, document).
template<typename T>
using mongo_insert_one_t =
    decltype(std::declval<T&>().insert_one(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_find_one_t
//   detector: find_one(collection, filter).
template<typename T>
using mongo_find_one_t =
    decltype(std::declval<T&>().find_one(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_update_one_t
//   detector: update_one(collection, filter, update).
template<typename T>
using mongo_update_one_t =
    decltype(std::declval<T&>().update_one(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_delete_one_t
//   detector: delete_one(collection, filter).
template<typename T>
using mongo_delete_one_t =
    decltype(std::declval<T&>().delete_one(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_replace_one_t
//   detector: replace_one(collection, filter, replacement).
template<typename T>
using mongo_replace_one_t =
    decltype(std::declval<T&>().replace_one(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// 7.2    Finding and aggregating
//------------------------------------------------------------------------------
// mongo_find_t
//   detector: find(collection, filter).
template<typename T>
using mongo_find_t =
    decltype(std::declval<T&>().find(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_count_documents_t
//   detector: count_documents(collection, filter), const.
template<typename T>
using mongo_count_documents_t =
    decltype(std::declval<const T&>().count_documents(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_aggregate_t
//   detector: aggregate(collection, pipeline).
template<typename T>
using mongo_aggregate_t =
    decltype(std::declval<T&>().aggregate(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// 7.3    Collections
//------------------------------------------------------------------------------
// mongo_create_collection_t
//   detector: create_collection(name).
template<typename T>
using mongo_create_collection_t =
    decltype(std::declval<T&>().create_collection(
        std::declval<const std::string&>()));

// mongo_drop_collection_t
//   detector: drop_collection(name).
template<typename T>
using mongo_drop_collection_t =
    decltype(std::declval<T&>().drop_collection(
        std::declval<const std::string&>()));

// mongo_has_collection_t
//   detector: has_collection(name), const.
template<typename T>
using mongo_has_collection_t =
    decltype(std::declval<const T&>().has_collection(
        std::declval<const std::string&>()));

// mongo_list_collections_t
//   detector: list_collections(), const.
template<typename T>
using mongo_list_collections_t =
    decltype(std::declval<const T&>().list_collections());

// 7.4    Change streams and cursors
//------------------------------------------------------------------------------
// mongo_watch_collection_t
//   detector: watch_collection(collection).
template<typename T>
using mongo_watch_collection_t =
    decltype(std::declval<T&>().watch_collection(
        std::declval<const std::string&>()));

// mongo_watch_database_t
//   detector: watch_database().
template<typename T>
using mongo_watch_database_t =
    decltype(std::declval<T&>().watch_database());

// mongo_find_cursor_t
//   detector: find_cursor(collection).
template<typename T>
using mongo_find_cursor_t =
    decltype(std::declval<T&>().find_cursor(
        std::declval<const std::string&>()));

// 7.5    Many writes at once
//------------------------------------------------------------------------------
// mongo_bulk_write_t
//   detector: bulk_write(collection, operations).
template<typename T>
using mongo_bulk_write_t =
    decltype(std::declval<T&>().bulk_write(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// 7.6    GridFS
//------------------------------------------------------------------------------
// mongo_gridfs_upload_t
//   detector: gridfs_upload(filename, bytes).
template<typename T>
using mongo_gridfs_upload_t =
    decltype(std::declval<T&>().gridfs_upload(
        std::declval<const std::string&>(),
        std::declval<const std::vector<re_std::uint8_t>&>()));

// mongo_gridfs_download_t
//   detector: gridfs_download(id).
template<typename T>
using mongo_gridfs_download_t =
    decltype(std::declval<T&>().gridfs_download(
        std::declval<const std::string&>()));

// 7.7    Indexes
//------------------------------------------------------------------------------
// mongo_create_index_t
//   detector: create_index(collection, name, keys).
template<typename T>
using mongo_create_index_t =
    decltype(std::declval<T&>().create_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// mongo_list_indexes_t
//   detector: list_indexes(collection), const.
template<typename T>
using mongo_list_indexes_t =
    decltype(std::declval<const T&>().list_indexes(
        std::declval<const std::string&>()));

// 7.8    Transactions
//------------------------------------------------------------------------------
// mongo_begin_transaction_t
//   detector: begin_transaction().
template<typename T>
using mongo_begin_transaction_t =
    decltype(std::declval<T&>().begin_transaction());

// mongo_commit_t
//   detector: commit().
template<typename T>
using mongo_commit_t =
    decltype(std::declval<T&>().commit());

// mongo_rollback_t
//   detector: rollback().
template<typename T>
using mongo_rollback_t =
    decltype(std::declval<T&>().rollback());

// 7.9    Concerns
//------------------------------------------------------------------------------
// mongo_set_read_concern_t
//   detector: set_read_concern(concern).
template<typename T>
using mongo_set_read_concern_t =
    decltype(std::declval<T&>().set_read_concern(
        std::declval<const std::string&>()));

// mongo_set_write_concern_t
//   detector: set_write_concern(concern).
template<typename T>
using mongo_set_write_concern_t =
    decltype(std::declval<T&>().set_write_concern(
        std::declval<const std::string&>()));


//==============================================================================
// 8.  TAGGED CAPABILITY TRAITS
//==============================================================================


// 8.1    Traits
//------------------------------------------------------------------------------
// 8.1.1
// has_mongo_document_crud
//   trait: whether T inserts, finds, updates and deletes documents.
template<typename T>
struct has_mongo_document_crud
    : djinterp::conjunction<is_detected<mongo_insert_one_t, clean_t<T>>,
                            is_detected<mongo_find_one_t, clean_t<T>>,
                            is_detected<mongo_update_one_t, clean_t<T>>,
                            is_detected<mongo_delete_one_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_mongo_document_crud_v =
        has_mongo_document_crud<clean_t<T>>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// 8.1.2
// has_mongo_collections
//   trait: whether T makes, drops, finds and lists collections.
template<typename T>
struct has_mongo_collections
    : djinterp::conjunction<is_detected<mongo_create_collection_t, clean_t<T>>,
                            is_detected<mongo_drop_collection_t, clean_t<T>>,
                            is_detected<mongo_has_collection_t, clean_t<T>>,
                            is_detected<mongo_list_collections_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_mongo_collections_v =
        has_mongo_collections<clean_t<T>>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// 8.1.3
// has_mongo_change_streams
//   trait: whether T watches a collection and a database.
template<typename T>
struct has_mongo_change_streams
    : djinterp::conjunction<is_detected<mongo_watch_collection_t, clean_t<T>>,
                            is_detected<mongo_watch_database_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_mongo_change_streams_v =
        has_mongo_change_streams<clean_t<T>>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// 8.1.4
// has_mongo_gridfs
//   trait: whether T uploads and downloads GridFS files.
template<typename T>
struct has_mongo_gridfs
    : djinterp::conjunction<is_detected<mongo_gridfs_upload_t, clean_t<T>>,
                            is_detected<mongo_gridfs_download_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_mongo_gridfs_v = has_mongo_gridfs<clean_t<T>>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// 8.1.5
// has_mongo_transactions
//   trait: whether T begins, commits and rolls back transactions.
template<typename T>
struct has_mongo_transactions
    : djinterp::conjunction<is_detected<mongo_begin_transaction_t, clean_t<T>>,
                            is_detected<mongo_commit_t, clean_t<T>>,
                            is_detected<mongo_rollback_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_mongo_transactions_v =
        has_mongo_transactions<clean_t<T>>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// 8.1.6
// has_mongo_indexes
//   trait: whether T builds and lists indexes.
template<typename T>
struct has_mongo_indexes
    : djinterp::conjunction<is_detected<mongo_create_index_t, clean_t<T>>,
                            is_detected<mongo_list_indexes_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_mongo_indexes_v = has_mongo_indexes<clean_t<T>>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// 8.1.7
// is_mongo_connection
//   trait: whether T is a MongoDB connection: it connects and disconnects, has
// document CRUD and collections, and aggregates.
template<typename T>
struct is_mongo_connection
    : djinterp::conjunction<has_connect<clean_t<T>>,
                            has_disconnect<clean_t<T>>,
                            has_mongo_document_crud<clean_t<T>>,
                            has_mongo_collections<clean_t<T>>,
                            is_detected<mongo_aggregate_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool is_mongo_connection_v =
        is_mongo_connection<clean_t<T>>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


//==============================================================================
// 9.  TAGLESS CAPABILITY TRAITS
//==============================================================================


// 9.1    Individual capabilities
//------------------------------------------------------------------------------
// mongo_can_insert, ... -- whether T has the member each detector names
template<typename T, typename = void>
constexpr bool mongo_can_insert = false;

template<typename T>
constexpr bool mongo_can_insert<T,
                          std::void_t<mongo_insert_one_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_can_find = false;

template<typename T>
constexpr bool mongo_can_find<T,
                          std::void_t<mongo_find_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_can_aggregate = false;

template<typename T>
constexpr bool mongo_can_aggregate<T,
                          std::void_t<mongo_aggregate_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_can_watch = false;

template<typename T>
constexpr bool mongo_can_watch<T,
                          std::void_t<mongo_watch_collection_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_can_bulk_write = false;

template<typename T>
constexpr bool mongo_can_bulk_write<T,
                          std::void_t<mongo_bulk_write_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_can_gridfs = false;

template<typename T>
constexpr bool mongo_can_gridfs<T,
                          std::void_t<mongo_gridfs_upload_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_can_transact = false;

template<typename T>
constexpr bool mongo_can_transact<T,
                          std::void_t<mongo_begin_transaction_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_can_create_collection = false;

template<typename T>
constexpr bool mongo_can_create_collection<T,
                          std::void_t<mongo_create_collection_t<T>>> = true;

// 9.2    Compound capabilities
//------------------------------------------------------------------------------
// mongo_does_document_crud, ... -- whether T has every member of a group
template<typename T, typename = void>
constexpr bool mongo_does_document_crud = false;

template<typename T>
constexpr bool mongo_does_document_crud<T,
                          std::void_t<mongo_insert_one_t<T>,
                                      mongo_find_one_t<T>,
                                      mongo_update_one_t<T>,
                                      mongo_delete_one_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_does_change_streams = false;

template<typename T>
constexpr bool mongo_does_change_streams<T,
                          std::void_t<mongo_watch_collection_t<T>,
                                      mongo_watch_database_t<T>>> = true;

template<typename T, typename = void>
constexpr bool mongo_does_transactions = false;

template<typename T>
constexpr bool mongo_does_transactions<T,
                          std::void_t<mongo_begin_transaction_t<T>,
                                      mongo_commit_t<T>,
                                      mongo_rollback_t<T>>> = true;

// mongo_is_full_connection -- whether T connects, disconnects, has document
// CRUD, aggregates and makes collections
template<typename T>
constexpr bool mongo_is_full_connection =
    ( (can_connect<clean_t<T>>)                &&
      (can_disconnect<clean_t<T>>)             &&
      (mongo_does_document_crud<clean_t<T>>)   &&
      (mongo_can_aggregate<clean_t<T>>)        &&
      (mongo_can_create_collection<clean_t<T>>) );


//==============================================================================
// 10. SFINAE HELPERS
//==============================================================================


// 10.1   enable_if helpers
//------------------------------------------------------------------------------
// enable_if_mongo_connection, ... -- void where T has what the trait of the
// name says, and no type otherwise
template<typename T>
using enable_if_mongo_connection =
    typename std::enable_if<is_mongo_connection<clean_t<T>>::value>::type;

template<typename T>
using enable_if_has_mongo_transactions =
    typename std::enable_if<has_mongo_transactions<clean_t<T>>::value>::type;

template<typename T>
using enable_if_has_mongo_change_streams =
    typename std::enable_if<
        has_mongo_change_streams<clean_t<T>>::value>::type;

template<typename T>
using enable_if_has_mongo_gridfs =
    typename std::enable_if<has_mongo_gridfs<clean_t<T>>::value>::type;


//==============================================================================
// 11. C++20 CONCEPTS
//==============================================================================
// Each concept forwards to a trait or a tagless capability above. They live
// in djinterp::concepts, where a concept may take the name of the class it
// constrains; each former name in djinterp forwards to its concept for one
// release (2026.10.28, as PostgreSQL's did on 2026.10.21). They exist where
// the compiler has concepts; the traits serve at C++17 without them.


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace concepts
{

// 11.1   Connection concepts
//------------------------------------------------------------------------------
// mongo_connection
//   concept: constrains types implementing the MongoDB connection interface.
template<typename Type>
concept mongo_connection =
    is_mongo_connection<clean_t<Type>>::value;

// mongodb_connection
//   concept: constrains mongo_connection, by the layer's name for the vendor.
template<typename Type>
concept mongodb_connection =
    mongo_connection<Type>;

// non_mongo_connection
//   concept: constrains types that do not implement the MongoDB connection
// interface.
template<typename Type>
concept non_mongo_connection =
    !mongo_connection<Type>;

// mongo_document_connection
//   concept: constrains MongoDB connections with document CRUD.
template<typename Type>
concept mongo_document_connection =
    has_mongo_document_crud<clean_t<Type>>::value;

// mongo_collection_connection
//   concept: constrains MongoDB connections that manage collections.
template<typename Type>
concept mongo_collection_connection =
    has_mongo_collections<clean_t<Type>>::value;

// mongo_aggregation_connection
//   concept: constrains MongoDB connections that aggregate.
template<typename Type>
concept mongo_aggregation_connection =
    is_detected<mongo_aggregate_t, clean_t<Type>>::value;

// mongo_change_stream_connection
//   concept: constrains MongoDB connections that watch changes.
template<typename Type>
concept mongo_change_stream_connection =
    has_mongo_change_streams<clean_t<Type>>::value;

// mongo_gridfs_connection
//   concept: constrains MongoDB connections with GridFS.
template<typename Type>
concept mongo_gridfs_connection =
    has_mongo_gridfs<clean_t<Type>>::value;

// mongo_transaction_connection
//   concept: constrains MongoDB connections with transactions.
template<typename Type>
concept mongo_transaction_connection =
    has_mongo_transactions<clean_t<Type>>::value;

// mongo_index_connection
//   concept: constrains MongoDB connections that manage indexes.
template<typename Type>
concept mongo_index_connection =
    has_mongo_indexes<clean_t<Type>>::value;

// 11.2   Capability concepts
//------------------------------------------------------------------------------
// mongo_insert_connection
//   concept: constrains types exposing insert_one().
template<typename Type>
concept mongo_insert_connection =
    is_detected<mongo_insert_one_t, clean_t<Type>>::value;

// mongo_find_one_connection
//   concept: constrains types exposing find_one().
template<typename Type>
concept mongo_find_one_connection =
    is_detected<mongo_find_one_t, clean_t<Type>>::value;

// mongo_update_one_connection
//   concept: constrains types exposing update_one().
template<typename Type>
concept mongo_update_one_connection =
    is_detected<mongo_update_one_t, clean_t<Type>>::value;

// mongo_delete_one_connection
//   concept: constrains types exposing delete_one().
template<typename Type>
concept mongo_delete_one_connection =
    is_detected<mongo_delete_one_t, clean_t<Type>>::value;

// mongo_replace_one_connection
//   concept: constrains types exposing replace_one().
template<typename Type>
concept mongo_replace_one_connection =
    is_detected<mongo_replace_one_t, clean_t<Type>>::value;

// mongo_create_collection_connection
//   concept: constrains types exposing create_collection().
template<typename Type>
concept mongo_create_collection_connection =
    is_detected<mongo_create_collection_t, clean_t<Type>>::value;

// mongo_drop_collection_connection
//   concept: constrains types exposing drop_collection().
template<typename Type>
concept mongo_drop_collection_connection =
    is_detected<mongo_drop_collection_t, clean_t<Type>>::value;

// mongo_collection_query_connection
//   concept: constrains types exposing has_collection().
template<typename Type>
concept mongo_collection_query_connection =
    is_detected<mongo_has_collection_t, clean_t<Type>>::value;

// mongo_collection_list_connection
//   concept: constrains types exposing list_collections().
template<typename Type>
concept mongo_collection_list_connection =
    is_detected<mongo_list_collections_t, clean_t<Type>>::value;

// mongo_watch_collection_connection
//   concept: constrains types exposing watch_collection().
template<typename Type>
concept mongo_watch_collection_connection =
    is_detected<mongo_watch_collection_t, clean_t<Type>>::value;

// mongo_watch_database_connection
//   concept: constrains types exposing watch_database().
template<typename Type>
concept mongo_watch_database_connection =
    is_detected<mongo_watch_database_t, clean_t<Type>>::value;

// mongo_cursor_connection
//   concept: constrains types exposing find_cursor().
template<typename Type>
concept mongo_cursor_connection =
    is_detected<mongo_find_cursor_t, clean_t<Type>>::value;

// mongo_bulk_write_connection
//   concept: constrains types exposing bulk_write().
template<typename Type>
concept mongo_bulk_write_connection =
    is_detected<mongo_bulk_write_t, clean_t<Type>>::value;

// mongo_gridfs_upload_connection
//   concept: constrains types exposing gridfs_upload().
template<typename Type>
concept mongo_gridfs_upload_connection =
    is_detected<mongo_gridfs_upload_t, clean_t<Type>>::value;

// mongo_gridfs_download_connection
//   concept: constrains types exposing gridfs_download().
template<typename Type>
concept mongo_gridfs_download_connection =
    is_detected<mongo_gridfs_download_t, clean_t<Type>>::value;

// mongo_create_index_connection
//   concept: constrains types exposing create_index().
template<typename Type>
concept mongo_create_index_connection =
    is_detected<mongo_create_index_t, clean_t<Type>>::value;

// mongo_list_indexes_connection
//   concept: constrains types exposing list_indexes().
template<typename Type>
concept mongo_list_indexes_connection =
    is_detected<mongo_list_indexes_t, clean_t<Type>>::value;

// mongo_begin_transaction_connection
//   concept: constrains types exposing begin_transaction().
template<typename Type>
concept mongo_begin_transaction_connection =
    is_detected<mongo_begin_transaction_t, clean_t<Type>>::value;

// mongo_commit_connection
//   concept: constrains types exposing commit().
template<typename Type>
concept mongo_commit_connection =
    is_detected<mongo_commit_t, clean_t<Type>>::value;

// mongo_rollback_connection
//   concept: constrains types exposing rollback().
template<typename Type>
concept mongo_rollback_connection =
    is_detected<mongo_rollback_t, clean_t<Type>>::value;

// mongo_read_concern_connection
//   concept: constrains types exposing set_read_concern().
template<typename Type>
concept mongo_read_concern_connection =
    is_detected<mongo_set_read_concern_t, clean_t<Type>>::value;

// mongo_write_concern_connection
//   concept: constrains types exposing set_write_concern().
template<typename Type>
concept mongo_write_concern_connection =
    is_detected<mongo_set_write_concern_t, clean_t<Type>>::value;

// mongo_find_connection
//   concept: constrains types exposing find().
template<typename Type>
concept mongo_find_connection =
    is_detected<mongo_find_t, clean_t<Type>>::value;

// mongo_count_documents_connection
//   concept: constrains types exposing count_documents().
template<typename Type>
concept mongo_count_documents_connection =
    is_detected<mongo_count_documents_t, clean_t<Type>>::value;

// 11.3   Tagless capability concepts
//------------------------------------------------------------------------------
// mongo_insertable_connection
//   concept: constrains types for which mongo_can_insert holds.
template<typename Type>
concept mongo_insertable_connection =
    mongo_can_insert<clean_t<Type>>;

// mongo_findable_connection
//   concept: constrains types for which mongo_can_find holds.
template<typename Type>
concept mongo_findable_connection =
    mongo_can_find<clean_t<Type>>;

// mongo_aggregating_connection
//   concept: constrains types for which mongo_can_aggregate holds.
template<typename Type>
concept mongo_aggregating_connection =
    mongo_can_aggregate<clean_t<Type>>;

// mongo_watchable_connection
//   concept: constrains types for which mongo_can_watch holds.
template<typename Type>
concept mongo_watchable_connection =
    mongo_can_watch<clean_t<Type>>;

// mongo_bulk_writable_connection
//   concept: constrains types for which mongo_can_bulk_write holds.
template<typename Type>
concept mongo_bulk_writable_connection =
    mongo_can_bulk_write<clean_t<Type>>;

// mongo_gridfs_capable_connection
//   concept: constrains types for which mongo_can_gridfs holds.
template<typename Type>
concept mongo_gridfs_capable_connection =
    mongo_can_gridfs<clean_t<Type>>;

// mongo_transactable_connection
//   concept: constrains types for which mongo_can_transact holds.
template<typename Type>
concept mongo_transactable_connection =
    mongo_can_transact<clean_t<Type>>;

// mongo_collection_creating_connection
//   concept: constrains types for which mongo_can_create_collection holds.
template<typename Type>
concept mongo_collection_creating_connection =
    mongo_can_create_collection<clean_t<Type>>;

// mongo_crud_connection
//   concept: constrains types for which mongo_does_document_crud holds.
template<typename Type>
concept mongo_crud_connection =
    mongo_does_document_crud<clean_t<Type>>;

// mongo_streaming_connection
//   concept: constrains types for which mongo_does_change_streams holds.
template<typename Type>
concept mongo_streaming_connection =
    mongo_does_change_streams<clean_t<Type>>;

// mongo_transactional_connection
//   concept: constrains types for which mongo_does_transactions holds.
template<typename Type>
concept mongo_transactional_connection =
    mongo_does_transactions<clean_t<Type>>;

// mongo_full_connection
//   concept: constrains types for which mongo_is_full_connection holds.
template<typename Type>
concept mongo_full_connection =
    mongo_is_full_connection<clean_t<Type>>;

}  // namespace concepts

// 11.4   Former names
//------------------------------------------------------------------------------
// Mongo_connection
//   concept: concepts::mongo_connection, by its former name, for one release.
template<typename Type>
concept Mongo_connection =
    concepts::mongo_connection<Type>;

// non_mongo_connection
//   concept: concepts::non_mongo_connection, by its former name, for one
// release.
template<typename Type>
concept non_mongo_connection =
    concepts::non_mongo_connection<Type>;

// mongo_document_connection
//   concept: concepts::mongo_document_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_document_connection =
    concepts::mongo_document_connection<Type>;

// mongo_collection_connection
//   concept: concepts::mongo_collection_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_collection_connection =
    concepts::mongo_collection_connection<Type>;

// mongo_aggregation_connection
//   concept: concepts::mongo_aggregation_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_aggregation_connection =
    concepts::mongo_aggregation_connection<Type>;

// mongo_change_stream_connection
//   concept: concepts::mongo_change_stream_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_change_stream_connection =
    concepts::mongo_change_stream_connection<Type>;

// mongo_gridfs_connection
//   concept: concepts::mongo_gridfs_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_gridfs_connection =
    concepts::mongo_gridfs_connection<Type>;

// mongo_transaction_connection
//   concept: concepts::mongo_transaction_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_transaction_connection =
    concepts::mongo_transaction_connection<Type>;

// mongo_index_connection
//   concept: concepts::mongo_index_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_index_connection =
    concepts::mongo_index_connection<Type>;

// mongo_insert_connection
//   concept: concepts::mongo_insert_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_insert_connection =
    concepts::mongo_insert_connection<Type>;

// mongo_find_one_connection
//   concept: concepts::mongo_find_one_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_find_one_connection =
    concepts::mongo_find_one_connection<Type>;

// mongo_update_one_connection
//   concept: concepts::mongo_update_one_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_update_one_connection =
    concepts::mongo_update_one_connection<Type>;

// mongo_delete_one_connection
//   concept: concepts::mongo_delete_one_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_delete_one_connection =
    concepts::mongo_delete_one_connection<Type>;

// mongo_replace_one_connection
//   concept: concepts::mongo_replace_one_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_replace_one_connection =
    concepts::mongo_replace_one_connection<Type>;

// mongo_create_collection_connection
//   concept: concepts::mongo_create_collection_connection, by its former name,
// for one release.
template<typename Type>
concept mongo_create_collection_connection =
    concepts::mongo_create_collection_connection<Type>;

// mongo_drop_collection_connection
//   concept: concepts::mongo_drop_collection_connection, by its former name,
// for one release.
template<typename Type>
concept mongo_drop_collection_connection =
    concepts::mongo_drop_collection_connection<Type>;

// mongo_collection_query_connection
//   concept: concepts::mongo_collection_query_connection, by its former name,
// for one release.
template<typename Type>
concept mongo_collection_query_connection =
    concepts::mongo_collection_query_connection<Type>;

// mongo_collection_list_connection
//   concept: concepts::mongo_collection_list_connection, by its former name,
// for one release.
template<typename Type>
concept mongo_collection_list_connection =
    concepts::mongo_collection_list_connection<Type>;

// mongo_watch_collection_connection
//   concept: concepts::mongo_watch_collection_connection, by its former name,
// for one release.
template<typename Type>
concept mongo_watch_collection_connection =
    concepts::mongo_watch_collection_connection<Type>;

// mongo_watch_database_connection
//   concept: concepts::mongo_watch_database_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_watch_database_connection =
    concepts::mongo_watch_database_connection<Type>;

// mongo_bulk_write_connection
//   concept: concepts::mongo_bulk_write_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_bulk_write_connection =
    concepts::mongo_bulk_write_connection<Type>;

// mongo_gridfs_upload_connection
//   concept: concepts::mongo_gridfs_upload_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_gridfs_upload_connection =
    concepts::mongo_gridfs_upload_connection<Type>;

// mongo_gridfs_download_connection
//   concept: concepts::mongo_gridfs_download_connection, by its former name,
// for one release.
template<typename Type>
concept mongo_gridfs_download_connection =
    concepts::mongo_gridfs_download_connection<Type>;

// mongo_create_index_connection
//   concept: concepts::mongo_create_index_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_create_index_connection =
    concepts::mongo_create_index_connection<Type>;

// mongo_list_indexes_connection
//   concept: concepts::mongo_list_indexes_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_list_indexes_connection =
    concepts::mongo_list_indexes_connection<Type>;

// mongo_read_concern_connection
//   concept: concepts::mongo_read_concern_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_read_concern_connection =
    concepts::mongo_read_concern_connection<Type>;

// mongo_write_concern_connection
//   concept: concepts::mongo_write_concern_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_write_concern_connection =
    concepts::mongo_write_concern_connection<Type>;

// mongo_find_connection
//   concept: concepts::mongo_find_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_find_connection =
    concepts::mongo_find_connection<Type>;

// mongo_count_documents_connection
//   concept: concepts::mongo_count_documents_connection, by its former name,
// for one release.
template<typename Type>
concept mongo_count_documents_connection =
    concepts::mongo_count_documents_connection<Type>;

// mongo_start_transaction_connection
//   concept: concepts::mongo_begin_transaction_connection, by its former name
// before the renames of 3.2, for one release.
template<typename Type>
concept mongo_start_transaction_connection =
    concepts::mongo_begin_transaction_connection<Type>;

// mongo_commit_transaction_connection
//   concept: concepts::mongo_commit_connection, by its former name before the
// renames of 3.2, for one release.
template<typename Type>
concept mongo_commit_transaction_connection =
    concepts::mongo_commit_connection<Type>;

// mongo_abort_transaction_connection
//   concept: concepts::mongo_rollback_connection, by its former name before the
// renames of 3.2, for one release.
template<typename Type>
concept mongo_abort_transaction_connection =
    concepts::mongo_rollback_connection<Type>;

// mongo_insertable_connection
//   concept: concepts::mongo_insertable_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_insertable_connection =
    concepts::mongo_insertable_connection<Type>;

// mongo_findable_connection
//   concept: concepts::mongo_findable_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_findable_connection =
    concepts::mongo_findable_connection<Type>;

// mongo_aggregating_connection
//   concept: concepts::mongo_aggregating_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_aggregating_connection =
    concepts::mongo_aggregating_connection<Type>;

// mongo_watchable_connection
//   concept: concepts::mongo_watchable_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_watchable_connection =
    concepts::mongo_watchable_connection<Type>;

// mongo_bulk_writable_connection
//   concept: concepts::mongo_bulk_writable_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_bulk_writable_connection =
    concepts::mongo_bulk_writable_connection<Type>;

// mongo_gridfs_capable_connection
//   concept: concepts::mongo_gridfs_capable_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_gridfs_capable_connection =
    concepts::mongo_gridfs_capable_connection<Type>;

// mongo_transactable_connection
//   concept: concepts::mongo_transactable_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_transactable_connection =
    concepts::mongo_transactable_connection<Type>;

// mongo_collection_creating_connection
//   concept: concepts::mongo_collection_creating_connection, by its former
// name, for one release.
template<typename Type>
concept mongo_collection_creating_connection =
    concepts::mongo_collection_creating_connection<Type>;

// mongo_crud_connection
//   concept: concepts::mongo_crud_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_crud_connection =
    concepts::mongo_crud_connection<Type>;

// mongo_streaming_connection
//   concept: concepts::mongo_streaming_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_streaming_connection =
    concepts::mongo_streaming_connection<Type>;

// mongo_transactional_connection
//   concept: concepts::mongo_transactional_connection, by its former name, for
// one release.
template<typename Type>
concept mongo_transactional_connection =
    concepts::mongo_transactional_connection<Type>;

// mongo_full_connection
//   concept: concepts::mongo_full_connection, by its former name, for one
// release.
template<typename Type>
concept mongo_full_connection =
    concepts::mongo_full_connection<Type>;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MONGODB_MONGODB_HPP
