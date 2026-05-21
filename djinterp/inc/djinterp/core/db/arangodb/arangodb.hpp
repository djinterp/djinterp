/******************************************************************************
* djinterp [database]                                             arangodb.hpp
* 
* djinterp ArangoDB connection module:
*   This header provides the ArangoDB-specific connection implementation
* and associated data type infrastructure for the djinterp database
* module, including:
*   - VelocyPack value type enumeration (ArangoDB's native binary format)
*   - VelocyPack-to-field_type mapping
*   - collection type enumeration (document vs edge)
*   - compile-time feature availability gated on version AND edition
*     (Community vs Enterprise) via D_ENV_ARANGO_* macros
*   - ArangoDB-specific connection configuration (endpoint URL, database
*     name, authentication method, VelocyStream vs HTTP/2, cursor batch
*     size)
*   - the concrete arango_connection CRTP leaf class with AQL execution,
*     document CRUD, collection management, graph traversals, streaming
*     transactions, cursor iteration, index management, ArangoSearch
*     view management, and database-level operations
*   - version-gated methods for schema validation (3.7+), computed
*     values (3.10+), search highlighting (3.11+), and MDI indexes
*     (3.12+)
*
*   ArangoDB is fundamentally different from SQL-based databases:
*   - multi-model: documents + graphs + key-value in one engine
*   - schema-free: JSON documents with optional schema validation
*   - AQL: native query language (not SQL)
*   - HTTP REST API: all operations via HTTP (or deprecated VST)
*   - collections: documents stored in collections, not tables
*   - edges: special _from/_to collections for graph relationships
*   - _key/_id/_rev: system attributes on every document
*   - VelocyPack: binary JSON-compatible format for wire and storage
*
*   LAYER DIAGRAM:
*     arango_connection (this file)
*       -> database_connection<arango_connection, database_type::arangodb>
*         -> connection_template<arango_connection, database_type::arangodb>
*           -> connection<arango_connection>
*
*   Note: ArangoDB uses execute_aql() instead of execute_query() for its
* native query language. The generic execute_query() is mapped to
* execute_aql() internally so the CRTP chain remains satisfied.
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include ArangoDB
* client headers; concrete _impl methods in arangodb.cpp would use the
* fuerte driver or direct HTTP calls.
*
* 
* path:      /inc/djinterp/core/db/arangodb/arangodb.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.06
******************************************************************************/

#ifndef DJINTERP_DATABASE_ARANGODB_
#define DJINTERP_DATABASE_ARANGODB_

// djinterp
#include "../../../djinterp.hpp"
#include "../../../env/db/env_arangodb.h"
#include "../database_connection.hpp"
#include "./arango_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   VELOCYPACK VALUE TYPES
// =============================================================================
// ArangoDB stores and transmits data in VelocyPack (VPack), a compact
// binary format that is a superset of JSON. Every VPack value has a
// type tag. These are declared here so that VelocyPack headers do not
// need to be included.
//
// Source: VelocyPack specification and velocypack/velocypack-common.h.

// vpack_type
//   enumeration: VelocyPack value types.
enum class vpack_type : std::uint8_t
{
    type_none       = 0x00,     // none / uninitialized
    type_null       = 0x18,     // JSON null
    type_bool_false = 0x19,     // JSON false
    type_bool_true  = 0x1A,     // JSON true
    type_double     = 0x1B,     // IEEE 754 double
    type_utc_date   = 0x1C,     // UTC date (milliseconds since epoch)
    type_external   = 0x1D,     // external pointer
    type_min_key    = 0x1E,     // sort-before-everything sentinel
    type_max_key    = 0x1F,     // sort-after-everything sentinel
    type_int        = 0x20,     // signed integer (1-8 bytes)
    type_uint       = 0x28,     // unsigned integer (1-8 bytes)
    type_small_int  = 0x30,     // small integer (-6 to 9)
    type_string     = 0x40,     // UTF-8 string (short, 1-byte length)
    type_long_string= 0xBF,     // UTF-8 string (long, 8-byte length)
    type_binary     = 0xC0,     // binary data (1-byte length)
    type_bcd        = 0xC8,     // BCD (binary-coded decimal)
    type_array       = 0x01,    // JSON array (compact)
    type_object     = 0x0B,     // JSON object (compact)
    type_custom     = 0xF0      // custom extension type
};


// =============================================================================
// II.  COLLECTION TYPES
// =============================================================================

// arango_collection_type
//   enumeration: ArangoDB collection types.
// value 2 = document collection, value 3 = edge collection.
// These match the integer values used in the ArangoDB REST API.
enum class arango_collection_type : int
{
    document = 2,
    edge     = 3
};


// =============================================================================
// III. TYPE MAPPING
// =============================================================================

// vpack_type_to_field_type
//   function: maps a VelocyPack value type to the generic djinterp
// field_type. Because ArangoDB is schema-free and JSON-based, many
// VPack types map to string or custom.
inline field_type vpack_type_to_field_type(
    vpack_type _vpack) noexcept
{
    switch (_vpack)
    {
        case vpack_type::type_null:
            return field_type::null;

        case vpack_type::type_bool_false:
        case vpack_type::type_bool_true:
            return field_type::boolean;

        case vpack_type::type_int:
        case vpack_type::type_uint:
        case vpack_type::type_small_int:
            return field_type::big_integer;

        case vpack_type::type_double:
            return field_type::floating_point;

        case vpack_type::type_bcd:
            return field_type::decimal;

        case vpack_type::type_string:
        case vpack_type::type_long_string:
            return field_type::string;

        case vpack_type::type_binary:
            return field_type::binary;

        case vpack_type::type_utc_date:
            return field_type::timestamp;

        case vpack_type::type_array:
            return field_type::array;

        case vpack_type::type_object:
            return field_type::json;

        case vpack_type::type_none:
        case vpack_type::type_external:
        case vpack_type::type_min_key:
        case vpack_type::type_max_key:
        case vpack_type::type_custom:
        default:
            return field_type::custom;
    }
}

// field_type_to_arango_json
//   function: returns the conventional JSON type name for a given
// field_type as it would appear in ArangoDB schema validation
// definitions.
inline const char* field_type_to_arango_json(
    field_type _type) noexcept
{
    switch (_type)
    {
        case field_type::null:           return "null";
        case field_type::boolean:        return "boolean";
        case field_type::integer:
        case field_type::big_integer:    return "integer";
        case field_type::floating_point:
        case field_type::decimal:        return "number";
        case field_type::string:
        case field_type::date:
        case field_type::time:
        case field_type::datetime:
        case field_type::timestamp:
        case field_type::uuid:
        case field_type::xml:            return "string";
        case field_type::binary:         return "string";
        case field_type::json:           return "object";
        case field_type::array:          return "array";
        case field_type::custom:
        default:                         return "string";
    }
}


// =============================================================================
// IV.  FEATURE SUPPORT (compile-time, version × edition gated)
// =============================================================================

// arango_type_support
//   struct: compile-time data type and model availability flags.
struct arango_type_support
{
#if D_ENV_ARANGO_DETECTED

    // multi-model
    static constexpr bool has_document_collections =
    #if D_ENV_ARANGO_HAS_DOCUMENT_COLLECTIONS
        true;  #else  false;  #endif
    static constexpr bool has_edge_collections =
    #if D_ENV_ARANGO_HAS_EDGE_COLLECTIONS
        true;  #else  false;  #endif
    static constexpr bool has_named_graphs =
    #if D_ENV_ARANGO_HAS_NAMED_GRAPHS
        true;  #else  false;  #endif

    // schema
    static constexpr bool has_schema_validation =
    #if D_ENV_ARANGO_HAS_SCHEMA_VALIDATION
        true;  #else  false;  #endif
    static constexpr bool has_computed_values =
    #if D_ENV_ARANGO_HAS_COMPUTED_VALUES
        true;  #else  false;  #endif
    static constexpr bool has_key_generators =
    #if D_ENV_ARANGO_HAS_KEY_GENERATORS
        true;  #else  false;  #endif

    // protocol
    static constexpr bool has_velocypack =
    #if D_ENV_ARANGO_HAS_VELOCYPACK
        true;  #else  false;  #endif
    static constexpr bool has_http2 =
    #if D_ENV_ARANGO_HAS_HTTP2
        true;  #else  false;  #endif
    static constexpr bool has_vst_protocol =
    #if D_ENV_ARANGO_HAS_VST_PROTOCOL
        true;  #else  false;  #endif

#else
    static constexpr bool has_document_collections = false;
    static constexpr bool has_edge_collections     = false;
    static constexpr bool has_named_graphs         = false;
    static constexpr bool has_schema_validation    = false;
    static constexpr bool has_computed_values      = false;
    static constexpr bool has_key_generators       = false;
    static constexpr bool has_velocypack           = false;
    static constexpr bool has_http2                = false;
    static constexpr bool has_vst_protocol         = false;
#endif
};

// arango_feature_support
//   struct: compile-time feature availability flags.
struct arango_feature_support
{
#if D_ENV_ARANGO_DETECTED

    // AQL
    static constexpr bool has_aql =
    #if D_ENV_ARANGO_HAS_AQL
        true;  #else  false;  #endif
    static constexpr bool has_aql_window =
    #if D_ENV_ARANGO_HAS_AQL_WINDOW
        true;  #else  false;  #endif
    static constexpr bool has_aql_k_paths =
    #if D_ENV_ARANGO_HAS_AQL_K_PATHS
        true;  #else  false;  #endif
    static constexpr bool has_aql_all_shortest_paths =
    #if D_ENV_ARANGO_HAS_AQL_ALL_SHORTEST_PATHS
        true;  #else  false;  #endif
    static constexpr bool has_aql_late_materialization =
    #if D_ENV_ARANGO_HAS_AQL_LATE_MATERIALIZATION
        true;  #else  false;  #endif
    static constexpr bool has_aql_insert_update =
    #if D_ENV_ARANGO_HAS_AQL_INSERT_UPDATE
        true;  #else  false;  #endif

    // search
    static constexpr bool has_arangosearch =
    #if D_ENV_ARANGO_HAS_ARANGOSEARCH
        true;  #else  false;  #endif
    static constexpr bool has_search_alias_views =
    #if D_ENV_ARANGO_HAS_SEARCH_ALIAS_VIEWS
        true;  #else  false;  #endif
    static constexpr bool has_analyzers =
    #if D_ENV_ARANGO_HAS_ANALYZERS
        true;  #else  false;  #endif
    static constexpr bool has_nested_search =
    #if D_ENV_ARANGO_HAS_NESTED_SEARCH
        true;  #else  false;  #endif
    static constexpr bool has_search_highlight =
    #if D_ENV_ARANGO_HAS_SEARCH_HIGHLIGHT
        true;  #else  false;  #endif

    // indexes
    static constexpr bool has_index_inverted =
    #if D_ENV_ARANGO_HAS_INDEX_INVERTED
        true;  #else  false;  #endif
    static constexpr bool has_index_mdi =
    #if D_ENV_ARANGO_HAS_INDEX_MDI
        true;  #else  false;  #endif
    static constexpr bool has_index_mdi_prefixed =
    #if D_ENV_ARANGO_HAS_INDEX_MDI_PREFIXED
        true;  #else  false;  #endif
    static constexpr bool has_index_ttl =
    #if D_ENV_ARANGO_HAS_INDEX_TTL
        true;  #else  false;  #endif
    static constexpr bool has_stored_values =
    #if D_ENV_ARANGO_HAS_STORED_VALUES
        true;  #else  false;  #endif
    static constexpr bool has_cache_on_index =
    #if D_ENV_ARANGO_HAS_CACHE_ON_INDEX
        true;  #else  false;  #endif

    // graph (enterprise)
    static constexpr bool has_smart_graphs =
    #if D_ENV_ARANGO_HAS_SMART_GRAPHS
        true;  #else  false;  #endif
    static constexpr bool has_enterprise_graphs =
    #if D_ENV_ARANGO_HAS_ENTERPRISE_GRAPHS
        true;  #else  false;  #endif
    static constexpr bool has_satellite_graphs =
    #if D_ENV_ARANGO_HAS_SATELLITE_GRAPHS
        true;  #else  false;  #endif
    static constexpr bool has_pregel =
    #if D_ENV_ARANGO_HAS_PREGEL
        true;  #else  false;  #endif

    // transactions
    static constexpr bool has_streaming_trx =
    #if D_ENV_ARANGO_HAS_STREAMING_TRX
        true;  #else  false;  #endif
    static constexpr bool has_cluster_trx =
    #if D_ENV_ARANGO_HAS_CLUSTER_TRX
        true;  #else  false;  #endif

    // clustering
    static constexpr bool has_oneshard =
    #if D_ENV_ARANGO_HAS_ONESHARD
        true;  #else  false;  #endif
    static constexpr bool has_satellite_collections =
    #if D_ENV_ARANGO_HAS_SATELLITE_COLLECTIONS
        true;  #else  false;  #endif
    static constexpr bool has_smart_joins =
    #if D_ENV_ARANGO_HAS_SMART_JOINS
        true;  #else  false;  #endif
    static constexpr bool has_dc2dc_repl =
    #if D_ENV_ARANGO_HAS_DC2DC_REPL
        true;  #else  false;  #endif

    // security
    static constexpr bool has_encryption_at_rest =
    #if D_ENV_ARANGO_HAS_ENCRYPTION_AT_REST
        true;  #else  false;  #endif
    static constexpr bool has_audit_log =
    #if D_ENV_ARANGO_HAS_AUDIT_LOG
        true;  #else  false;  #endif
    static constexpr bool has_auth_ldap =
    #if D_ENV_ARANGO_HAS_AUTH_LDAP
        true;  #else  false;  #endif

    // backup
    static constexpr bool has_hot_backup =
    #if D_ENV_ARANGO_HAS_HOT_BACKUP
        true;  #else  false;  #endif

    // foxx
    static constexpr bool has_foxx =
    #if D_ENV_ARANGO_HAS_FOXX
        true;  #else  false;  #endif

    // edition
    static constexpr bool is_enterprise =
    #if D_ENV_ARANGO_IS_ENTERPRISE
        true;  #else  false;  #endif

    // composite
    static constexpr bool has_modern_search =
    #if D_ENV_ARANGO_HAS_MODERN_SEARCH
        true;  #else  false;  #endif
    static constexpr bool has_modern_aql =
    #if D_ENV_ARANGO_HAS_MODERN_AQL
        true;  #else  false;  #endif
    static constexpr bool has_modern_graph =
    #if D_ENV_ARANGO_HAS_MODERN_GRAPH
        true;  #else  false;  #endif
    static constexpr bool is_fully_modern =
    #if D_ENV_ARANGO_IS_FULLY_MODERN
        true;  #else  false;  #endif

#else
    static constexpr bool has_aql                      = false;
    static constexpr bool has_aql_window               = false;
    static constexpr bool has_aql_k_paths              = false;
    static constexpr bool has_aql_all_shortest_paths   = false;
    static constexpr bool has_aql_late_materialization  = false;
    static constexpr bool has_aql_insert_update        = false;
    static constexpr bool has_arangosearch             = false;
    static constexpr bool has_search_alias_views       = false;
    static constexpr bool has_analyzers                = false;
    static constexpr bool has_nested_search            = false;
    static constexpr bool has_search_highlight         = false;
    static constexpr bool has_index_inverted           = false;
    static constexpr bool has_index_mdi                = false;
    static constexpr bool has_index_mdi_prefixed       = false;
    static constexpr bool has_index_ttl                = false;
    static constexpr bool has_stored_values            = false;
    static constexpr bool has_cache_on_index           = false;
    static constexpr bool has_smart_graphs             = false;
    static constexpr bool has_enterprise_graphs        = false;
    static constexpr bool has_satellite_graphs         = false;
    static constexpr bool has_pregel                   = false;
    static constexpr bool has_streaming_trx            = false;
    static constexpr bool has_cluster_trx              = false;
    static constexpr bool has_oneshard                 = false;
    static constexpr bool has_satellite_collections    = false;
    static constexpr bool has_smart_joins              = false;
    static constexpr bool has_dc2dc_repl               = false;
    static constexpr bool has_encryption_at_rest       = false;
    static constexpr bool has_audit_log                = false;
    static constexpr bool has_auth_ldap                = false;
    static constexpr bool has_hot_backup               = false;
    static constexpr bool has_foxx                     = false;
    static constexpr bool is_enterprise                = false;
    static constexpr bool has_modern_search            = false;
    static constexpr bool has_modern_aql               = false;
    static constexpr bool has_modern_graph             = false;
    static constexpr bool is_fully_modern              = false;
#endif
};


// =============================================================================
// V.   VERSION INFORMATION
// =============================================================================

// arango_version_info
//   struct: compile-time version decomposition.
// ArangoDB uses semantic versioning (MAJOR.MINOR.PATCH) encoded as
// MAJOR*10000 + MINOR*100 + PATCH.
struct arango_version_info
{
#if D_ENV_ARANGO_DETECTED
    static constexpr bool          detected = true;
    static constexpr std::uint32_t id       = D_ENV_ARANGO_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_ARANGO_VERSION_MAJOR;
    static constexpr std::uint16_t minor    = D_ENV_ARANGO_VERSION_MINOR;
    static constexpr std::uint16_t patch    = D_ENV_ARANGO_VERSION_PATCH;
    static constexpr const char*   string   = D_ENV_ARANGO_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t minor    = 0;
    static constexpr std::uint16_t patch    = 0;
    static constexpr const char*   string   = "not detected";
#endif

    static constexpr bool at_least(std::uint16_t _major,
                                   std::uint16_t _minor,
                                   std::uint16_t _patch) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// VI.  ARANGODB CONNECTION CONFIGURATION
// =============================================================================

// arango_auth_method
//   enumeration: authentication methods for ArangoDB HTTP API.
enum class arango_auth_method : std::uint8_t
{
    none     = 0,       // no authentication
    basic    = 1,       // HTTP Basic
    jwt      = 2        // JWT token
};

// arango_connect_config
//   struct: ArangoDB-specific connection configuration.
// ArangoDB uses HTTP endpoints instead of host:port with a wire
// protocol. The endpoint URL includes the scheme (http:// or
// https://) and port (default 8529).
struct arango_connect_config
{
    std::string           endpoint;
    std::string           database_name;
    std::string           username;
    std::string           password;
    std::string           jwt_token;
    arango_auth_method    auth_method;
    std::size_t           cursor_batch_size;
    int                   connect_timeout_seconds;
    int                   request_timeout_seconds;
    bool                  use_vst;
    bool                  verify_ssl;

    std::map<std::string, std::string> extra_headers;

    arango_connect_config()
        : endpoint("http://localhost:8529")
        , database_name("_system")
        , auth_method(arango_auth_method::basic)
        , cursor_batch_size(1000)
        , connect_timeout_seconds(10)
        , request_timeout_seconds(300)
        , use_vst(false)
        , verify_ssl(true)
    {
    }

    explicit arango_connect_config(const std::string& _endpoint)
        : endpoint(_endpoint)
        , database_name("_system")
        , auth_method(arango_auth_method::basic)
        , cursor_batch_size(1000)
        , connect_timeout_seconds(10)
        , request_timeout_seconds(300)
        , use_vst(false)
        , verify_ssl(true)
    {
    }

    // with_jwt
    //   function: factory for JWT-authenticated connection.
    static arango_connect_config with_jwt(
        const std::string& _endpoint,
        const std::string& _token)
    {
        arango_connect_config config(_endpoint);

        config.auth_method = arango_auth_method::jwt;
        config.jwt_token   = _token;

        return config;
    }

    // with_credentials
    //   function: factory for basic-auth connection.
    static arango_connect_config with_credentials(
        const std::string& _endpoint,
        const std::string& _user,
        const std::string& _password,
        const std::string& _database = "_system")
    {
        arango_connect_config config(_endpoint);

        config.username      = _user;
        config.password      = _password;
        config.database_name = _database;

        return config;
    }
};


// =============================================================================
// VII. ARANGODB CONNECTION
// =============================================================================

// arango_connection
//   class: concrete ArangoDB connection implementation.
// This is the CRTP leaf class; _impl methods are defined in
// arangodb.cpp which uses the fuerte driver or direct HTTP calls.
//
// ArangoDB is multi-model (documents, graphs, key-value) and uses
// AQL instead of SQL. The generic execute_query() is mapped to
// execute_aql() internally to satisfy the CRTP connection interface.
//
// Usage:
//   arango_connection conn;
//   conn.connect(arango_connect_config::with_credentials(
//       "http://localhost:8529", "root", "", "mydb"));
//   auto result = conn.execute_aql("FOR d IN myCollection RETURN d");
class arango_connection
    : public database_connection<arango_connection,
                                 database_type::arangodb>
{
public:
    using base_type       = database_connection<
        arango_connection, database_type::arangodb>;
    using type_support    = arango_type_support;
    using feature_support = arango_feature_support;
    using version_info    = arango_version_info;

    arango_connection()
        : base_type()
    {
    }

    explicit arango_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit arango_connection(const arango_connect_config& _config)
        : base_type()
        , m_arango_config(_config)
    {
        // map arango endpoint into the generic config
        this->m_config.host     = _config.endpoint;
        this->m_config.database = _config.database_name;
        this->m_config.username = _config.username;
        this->m_config.password = _config.password;
    }

    ~arango_connection() = default;

    // disable copying
    arango_connection(const arango_connection&)            = delete;
    arango_connection& operator=(const arango_connection&) = delete;

    // enable moving
    arango_connection(arango_connection&&) noexcept            = default;
    arango_connection& operator=(arango_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // AQL execution
    // -----------------------------------------------------------------

    // execute_aql
    //   function: executes an AQL query and returns the result as a
    // JSON string (or VelocyPack if configured).
    std::string execute_aql(const std::string& _aql)
    {
        this->ensure_connected();

        return self().execute_aql_impl(_aql);
    }

    // execute_aql (with bind variables)
    //   function: executes an AQL query with named bind variables.
    std::string execute_aql(
        const std::string&                         _aql,
        const std::map<std::string, std::string>&  _bind_vars)
    {
        this->ensure_connected();

        return self().execute_aql_bind_impl(_aql, _bind_vars);
    }

    // explain_aql
    //   function: returns the execution plan for an AQL query
    // without running it.
    std::string explain_aql(const std::string& _aql) const
    {
        return self().explain_aql_impl(_aql);
    }

    // -----------------------------------------------------------------
    // document CRUD
    // -----------------------------------------------------------------

    // insert_document
    //   function: inserts a JSON document into a collection.
    // returns the document key (_key).
    std::string insert_document(const std::string& _collection,
                                const std::string& _json_document)
    {
        this->ensure_connected();

        return self().insert_document_impl(_collection,
                                           _json_document);
    }

    // get_document
    //   function: retrieves a document by collection and key.
    // returns the document as a JSON string.
    std::string get_document(const std::string& _collection,
                             const std::string& _key) const
    {
        return self().get_document_impl(_collection, _key);
    }

    // update_document
    //   function: partially updates a document (merge patch).
    void update_document(const std::string& _collection,
                         const std::string& _key,
                         const std::string& _json_patch)
    {
        this->ensure_connected();
        self().update_document_impl(_collection, _key, _json_patch);
    }

    // replace_document
    //   function: fully replaces a document.
    void replace_document(const std::string& _collection,
                          const std::string& _key,
                          const std::string& _json_document)
    {
        this->ensure_connected();
        self().replace_document_impl(_collection, _key,
                                     _json_document);
    }

    // remove_document
    //   function: removes a document by key.
    void remove_document(const std::string& _collection,
                         const std::string& _key)
    {
        this->ensure_connected();
        self().remove_document_impl(_collection, _key);
    }

    // -----------------------------------------------------------------
    // collection management
    // -----------------------------------------------------------------

    // create_collection
    //   function: creates a new collection.
    void create_collection(const std::string&     _name,
                           arango_collection_type _type =
                               arango_collection_type::document)
    {
        this->ensure_connected();
        self().create_collection_impl(_name,
                                      static_cast<int>(_type));
    }

    // drop_collection
    //   function: drops a collection.
    void drop_collection(const std::string& _name)
    {
        this->ensure_connected();
        self().drop_collection_impl(_name);
    }

    // collection_exists
    //   function: tests whether a collection exists.
    bool collection_exists(const std::string& _name) const
    {
        return self().collection_exists_impl(_name);
    }

    // get_collection_names
    //   function: returns all collection names in the current
    // database.
    std::vector<std::string> get_collection_names() const
    {
        return self().get_collection_names_impl();
    }

    // -----------------------------------------------------------------
    // graph operations
    // -----------------------------------------------------------------

    // create_graph
    //   function: creates a named graph with the given edge
    // definition (provided as a JSON string).
    void create_graph(const std::string& _name)
    {
        this->ensure_connected();
        self().create_graph_impl(_name);
    }

    // traverse
    //   function: performs a graph traversal from the given start
    // vertex, up to the given depth. Returns the result as JSON.
    std::string traverse(const std::string& _graph_name,
                         const std::string& _start_vertex,
                         int                _max_depth)
    {
        this->ensure_connected();

        return self().traverse_impl(_graph_name,
                                     _start_vertex,
                                     _max_depth);
    }

    // shortest_path
    //   function: finds the shortest path between two vertices
    // in a named graph. Returns the result as JSON.
    std::string shortest_path(const std::string& _graph_name,
                              const std::string& _from,
                              const std::string& _to)
    {
        this->ensure_connected();

        return self().shortest_path_impl(_graph_name, _from, _to);
    }

    // -----------------------------------------------------------------
    // streaming transactions
    // -----------------------------------------------------------------

    // begin_stream_trx
    //   function: begins a streaming transaction over the listed
    // collections. Returns a transaction ID.
    std::string begin_stream_trx(
        const std::vector<std::string>& _collections)
    {
        this->ensure_connected();

        return self().begin_stream_trx_impl(_collections);
    }

    // commit_trx
    //   function: commits a streaming transaction by ID.
    void commit_trx(const std::string& _trx_id)
    {
        self().commit_trx_impl(_trx_id);
    }

    // abort_trx
    //   function: aborts a streaming transaction by ID.
    void abort_trx(const std::string& _trx_id)
    {
        self().abort_trx_impl(_trx_id);
    }

    // -----------------------------------------------------------------
    // cursor iteration
    // -----------------------------------------------------------------

    // create_cursor
    //   function: creates a server-side cursor for an AQL query.
    // Returns a cursor ID for batch iteration.
    std::string create_cursor(const std::string& _aql)
    {
        this->ensure_connected();

        return self().create_cursor_impl(_aql);
    }

    // next_batch
    //   function: retrieves the next batch of results from a cursor.
    // Returns the batch as JSON.
    std::string next_batch(const std::string& _cursor_id)
    {
        return self().next_batch_impl(_cursor_id);
    }

    // -----------------------------------------------------------------
    // index management
    // -----------------------------------------------------------------

    // create_index
    //   function: creates an index on a collection. The index
    // definition is provided as a JSON string.
    void create_index(const std::string& _collection,
                      const std::string& _index_def_json)
    {
        this->ensure_connected();
        self().create_index_impl(_collection, _index_def_json);
    }

    // get_indexes
    //   function: returns the indexes on a collection as JSON.
    std::string get_indexes(const std::string& _collection) const
    {
        return self().get_indexes_impl(_collection);
    }

    // -----------------------------------------------------------------
    // view management (ArangoSearch)
    // -----------------------------------------------------------------

    // create_view
    //   function: creates an ArangoSearch View with the given
    // definition (JSON string).
    void create_view(const std::string& _name,
                     const std::string& _view_def_json)
    {
        this->ensure_connected();
        self().create_view_impl(_name, _view_def_json);
    }

    // -----------------------------------------------------------------
    // database-level operations
    // -----------------------------------------------------------------

    // current_database
    //   function: returns the name of the current database.
    std::string current_database() const
    {
        return self().current_database_impl();
    }

    // list_databases
    //   function: returns the names of all accessible databases.
    std::vector<std::string> list_databases() const
    {
        return self().list_databases_impl();
    }

    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_arangosearch() noexcept
    {
        return feature_support::has_arangosearch;
    }

    static constexpr bool supports_streaming_trx() noexcept
    {
        return feature_support::has_streaming_trx;
    }

    static constexpr bool supports_smart_graphs() noexcept
    {
        return feature_support::has_smart_graphs;
    }

    static constexpr bool supports_inverted_index() noexcept
    {
        return feature_support::has_index_inverted;
    }

    static constexpr bool supports_aql_window() noexcept
    {
        return feature_support::has_aql_window;
    }

    static constexpr bool supports_schema_validation() noexcept
    {
        return type_support::has_schema_validation;
    }

    static constexpr bool supports_modern_search() noexcept
    {
        return feature_support::has_modern_search;
    }

    static constexpr bool is_enterprise() noexcept
    {
        return feature_support::is_enterprise;
    }

    // -----------------------------------------------------------------
    // type mapping
    // -----------------------------------------------------------------

    static field_type map_vpack_type(vpack_type _vpack) noexcept
    {
        return vpack_type_to_field_type(_vpack);
    }

    static const char* json_type_name(field_type _type) noexcept
    {
        return field_type_to_arango_json(_type);
    }

    // -----------------------------------------------------------------
    // configuration
    // -----------------------------------------------------------------

    const arango_connect_config& get_arango_config() const noexcept
    {
        return m_arango_config;
    }

    void set_arango_config(const arango_connect_config& _config)
    {
        m_arango_config     = _config;
        this->m_config.host     = _config.endpoint;
        this->m_config.database = _config.database_name;
        this->m_config.username = _config.username;
        this->m_config.password = _config.password;
    }

    // -----------------------------------------------------------------
    // _impl methods (defined in arangodb.cpp)
    // -----------------------------------------------------------------

    void connect_impl();
    void disconnect_impl();
    bool is_connected_impl() const;
    bool ping_impl() const;

    // generic execute_query/execute_update map to AQL
    auto execute_query_impl(const std::string& _query)
        -> std::unique_ptr<
            result_set<struct arango_result_set_impl>>;
    std::int64_t execute_update_impl(const std::string& _query);
    bool execute_impl(const std::string& _query);

    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;
    std::int64_t get_last_insert_id_impl() const;
    std::int64_t get_affected_rows_impl() const;

    // ArangoDB-specific _impl methods
    std::string execute_aql_impl(const std::string& _aql);
    std::string execute_aql_bind_impl(
        const std::string& _aql,
        const std::map<std::string, std::string>& _bind_vars);
    std::string explain_aql_impl(const std::string& _aql) const;
    std::string insert_document_impl(
        const std::string& _collection,
        const std::string& _json);
    std::string get_document_impl(
        const std::string& _collection,
        const std::string& _key) const;
    void update_document_impl(
        const std::string& _collection,
        const std::string& _key,
        const std::string& _json);
    void replace_document_impl(
        const std::string& _collection,
        const std::string& _key,
        const std::string& _json);
    void remove_document_impl(
        const std::string& _collection,
        const std::string& _key);
    void create_collection_impl(const std::string& _name, int _type);
    void drop_collection_impl(const std::string& _name);
    bool collection_exists_impl(const std::string& _name) const;
    std::vector<std::string> get_collection_names_impl() const;
    void create_graph_impl(const std::string& _name);
    std::string traverse_impl(
        const std::string& _graph,
        const std::string& _start,
        int _depth);
    std::string shortest_path_impl(
        const std::string& _graph,
        const std::string& _from,
        const std::string& _to);
    std::string begin_stream_trx_impl(
        const std::vector<std::string>& _collections);
    void commit_trx_impl(const std::string& _trx_id);
    void abort_trx_impl(const std::string& _trx_id);
    std::string create_cursor_impl(const std::string& _aql);
    std::string next_batch_impl(const std::string& _cursor_id);
    void create_index_impl(
        const std::string& _collection,
        const std::string& _index_def);
    std::string get_indexes_impl(
        const std::string& _collection) const;
    void create_view_impl(
        const std::string& _name,
        const std::string& _view_def);
    std::string current_database_impl() const;
    std::vector<std::string> list_databases_impl() const;

    // transaction _impl (for the generic CRTP chain)
    void begin_transaction_impl();
    void commit_impl();
    void rollback_impl();

    // version-gated methods

#if D_ENV_ARANGO_DETECTED
    #if D_ENV_ARANGO_HAS_SCHEMA_VALIDATION
    // set_collection_schema
    //   function: sets JSON Schema validation on a collection.
    // Available since ArangoDB 3.7.
    void set_collection_schema(const std::string& _collection,
                               const std::string& _schema_json);
    #endif

    #if D_ENV_ARANGO_HAS_COMPUTED_VALUES
    // set_computed_values
    //   function: configures computed values on a collection.
    // Available since ArangoDB 3.10.
    void set_computed_values(const std::string& _collection,
                             const std::string& _config_json);
    #endif

    #if D_ENV_ARANGO_HAS_SEARCH_HIGHLIGHT
    // search_highlight
    //   function: executes an AQL SEARCH query with highlighting.
    // Available since ArangoDB 3.11.
    std::string search_highlight(const std::string& _aql);
    #endif

    #if D_ENV_ARANGO_HAS_INDEX_MDI_PREFIXED
    // create_mdi_prefixed_index
    //   function: creates a prefixed multi-dimensional index.
    // Available since ArangoDB 3.12.
    void create_mdi_prefixed_index(const std::string& _collection,
                                   const std::string& _index_def);
    #endif

    #if D_ENV_ARANGO_HAS_HOT_BACKUP
    // create_hot_backup / restore_hot_backup
    //   functions: cluster-wide hot backup. Enterprise only, 3.5.1+.
    std::string create_hot_backup();
    void restore_hot_backup(const std::string& _backup_id);
    #endif
#endif  // D_ENV_ARANGO_DETECTED

private:
    arango_connect_config m_arango_config;

    arango_connection& self()
    {
        return *this;
    }

    const arango_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// arango_result_set_impl
//   struct: forward declaration of the ArangoDB result set
// implementation.
struct arango_result_set_impl;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ARANGODB_
