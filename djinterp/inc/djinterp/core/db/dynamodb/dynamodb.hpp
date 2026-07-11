/******************************************************************************
* djinterp [database]                                             dynamodb.hpp
*
* djinterp DynamoDB connection module:
*   This header provides the Amazon DynamoDB-specific connection
* implementation and associated data type infrastructure for the djinterp
* database module, including:
*   - DynamoDB attribute type enumeration (S, N, B, BOOL, NULL, M, L,
*     SS, NS, BS)
*   - attribute-type-to-field_type mapping (one rung above the wire
*     descriptor)
*   - compile-time type and feature availability via D_ENV_DYNAMODB_*
*     macros covering attribute types, transactions, PartiQL, streams,
*     global tables, on-demand capacity, PITR, DAX, and encryption
*   - DynamoDB-specific connection configuration (AWS region, endpoint
*     override, credential profile, DAX cluster endpoint, default read
*     consistency, retry policy, capacity mode)
*   - the concrete dynamodb_connection CRTP leaf class with item
*     operations, batch operations, query/scan, transactions, PartiQL,
*     conditional writes, table management, secondary indexes, Streams,
*     TTL management, backup / PITR, diagnostics, global tables, and
*     resource tagging
*   - feature-gated method declarations for transactions, PartiQL,
*     Streams, and global tables
*
*   DynamoDB is a managed key-value / document store, not a relational
* database. There is no SQL dialect (PartiQL aside), no server-side
* joins, and no INFORMATION_SCHEMA. Every operation is a request against
* the control plane (table administration) or data plane (item access)
* over a fleet of tables holding schemaless items keyed by a partition
* key plus an optional sort key. The connection surface mirrors the
* AWS SDK for C++ client (Aws::DynamoDB::DynamoDBClient) rather than any
* SQL connection idiom.
*
*   LAYER DIAGRAM:
*     dynamodb_connection (this file)
*       -> database_connection<dynamodb_connection, database_type::dynamodb>
*         -> connection_template<dynamodb_connection,
*                                database_type::dynamodb>
*           -> connection<dynamodb_connection>
*
*   NOTE: database_type::dynamodb must be present in the database_type
* enumeration in database.hpp (e.g. dynamodb = 0x0F). The corresponding
* db_traits<database_type::dynamodb> specialization supplies the default
* endpoint/port metadata.
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include the AWS SDK
* headers; the concrete _impl method definitions in dynamodb.cpp include
* <aws/dynamodb/DynamoDBClient.h> and related headers.
*
*
*   DETECTION:
*   Also carries this database's capability-detection traits and C++20 concepts
* (trailing sections), folded in from dynamodb_traits.hpp and the matching *_concepts.hpp;
* detection now lives with the connection. Concepts gated on concept support.
*
* path:      /inc/djinterp/core/db/dynamodb/dynamodb.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_DYNAMODB_
#define DJINTERP_DATABASE_DYNAMODB_

// std
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../env/db/dynamodb/env_dynamodb.h"
#include "../database_connection.hpp"
#include "../database_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   DYNAMODB ATTRIBUTE TYPE ENUMERATION
// =============================================================================
// DynamoDB describes every attribute value with a single-letter type
// descriptor on the wire. Unlike SQL OIDs these are not numeric identifiers;
// the values below are djinterp-local enumerators mapping onto those
// descriptors. The concrete connection's marshalling code in dynamodb.cpp
// translates between this enum and the AWS SDK's AttributeValue type.

// dynamodb_attribute_type
//   enumeration: DynamoDB attribute value descriptors.
enum class dynamodb_attribute_type : std::uint16_t
{
    // -----------------------------------------------------------------
    // none / missing
    // -----------------------------------------------------------------
    none        = 0x00,

    // -----------------------------------------------------------------
    // scalar types
    // -----------------------------------------------------------------
    string      = 0x01,     // "S"    — UTF-8 string
    number      = 0x02,     // "N"    — number (sent as a string)
    binary      = 0x03,     // "B"    — binary blob
    boolean     = 0x04,     // "BOOL" — boolean
    null_value  = 0x05,     // "NULL" — explicit null

    // -----------------------------------------------------------------
    // document types
    // -----------------------------------------------------------------
    map         = 0x10,     // "M"    — nested attribute map
    list        = 0x11,     // "L"    — ordered list of values

    // -----------------------------------------------------------------
    // set types
    // -----------------------------------------------------------------
    string_set  = 0x20,     // "SS"   — set of strings
    number_set  = 0x21,     // "NS"   — set of numbers
    binary_set  = 0x22,     // "BS"   — set of binary blobs

    // -----------------------------------------------------------------
    // sentinel: not a recognised descriptor
    // -----------------------------------------------------------------
    unknown     = 0xFF
};


// =============================================================================
// II.  ATTRIBUTE-TYPE-TO-FIELD_TYPE MAPPING
// =============================================================================

// dynamodb_attribute_type_to_field_type
//   function: maps a DynamoDB attribute type to the generic djinterp
// field_type. Aggregate types (map, list, sets) have no single field-
// level mapping and resolve to field_type::custom.
inline field_type dynamodb_attribute_type_to_field_type(
    dynamodb_attribute_type _type) noexcept
{
    switch (_type)
    {
        case dynamodb_attribute_type::none:
        case dynamodb_attribute_type::null_value:
            return field_type::null;

        case dynamodb_attribute_type::string:
            return field_type::string;

        case dynamodb_attribute_type::number:
            // DynamoDB numbers carry up to 38 digits of precision and
            // are transmitted as strings; expose as decimal so callers
            // do not silently lose precision to a binary double.
            return field_type::decimal;

        case dynamodb_attribute_type::binary:
        case dynamodb_attribute_type::binary_set:
            return field_type::binary;

        case dynamodb_attribute_type::boolean:
            return field_type::boolean;

        case dynamodb_attribute_type::map:
            // maps are JSON-like documents.
            return field_type::json;

        case dynamodb_attribute_type::list:
        case dynamodb_attribute_type::string_set:
        case dynamodb_attribute_type::number_set:
            return field_type::array;

        case dynamodb_attribute_type::unknown:
        default:
            return field_type::custom;
    }
}

// dynamodb_attribute_type_from_descriptor
//   function: maps the wire descriptor ("S", "N", "B", "BOOL",
// "NULL", "M", "L", "SS", "NS", "BS") to dynamodb_attribute_type.
inline dynamodb_attribute_type dynamodb_attribute_type_from_descriptor(
    const std::string& _descriptor) noexcept
{
    if (_descriptor == "S")
    {
        return dynamodb_attribute_type::string;
    }

    if (_descriptor == "N")
    {
        return dynamodb_attribute_type::number;
    }

    if (_descriptor == "B")
    {
        return dynamodb_attribute_type::binary;
    }

    if (_descriptor == "BOOL")
    {
        return dynamodb_attribute_type::boolean;
    }

    if (_descriptor == "NULL")
    {
        return dynamodb_attribute_type::null_value;
    }

    if (_descriptor == "M")
    {
        return dynamodb_attribute_type::map;
    }

    if (_descriptor == "L")
    {
        return dynamodb_attribute_type::list;
    }

    if (_descriptor == "SS")
    {
        return dynamodb_attribute_type::string_set;
    }

    if (_descriptor == "NS")
    {
        return dynamodb_attribute_type::number_set;
    }

    if (_descriptor == "BS")
    {
        return dynamodb_attribute_type::binary_set;
    }

    return dynamodb_attribute_type::unknown;
}

// field_type_to_dynamodb_descriptor
//   function: returns the closest DynamoDB attribute descriptor for a
// given field_type. Used by the table layer when laying out item
// attributes.
inline const char* field_type_to_dynamodb_descriptor(field_type _type)
    noexcept
{
    switch (_type)
    {
        case field_type::null:           return "NULL";
        case field_type::boolean:        return "BOOL";
        case field_type::integer:        return "N";
        case field_type::big_integer:    return "N";
        case field_type::floating_point: return "N";
        case field_type::decimal:        return "N";
        case field_type::string:         return "S";
        case field_type::binary:         return "B";
        case field_type::date:           return "S";
        case field_type::time:           return "S";
        case field_type::datetime:       return "S";
        case field_type::timestamp:      return "N";
        case field_type::json:           return "M";
        case field_type::xml:            return "S";
        case field_type::uuid:           return "S";
        case field_type::array:          return "L";
        case field_type::custom:
        default:                         return "S";
    }
}


// =============================================================================
// III. FEATURE SUPPORT (compile-time, version-gated)
// =============================================================================

// dynamodb_type_support
//   struct: compile-time attribute type availability flags gated by
// D_ENV_DYNAMODB_* macros. The scalar/document/set descriptors have
// been stable since the 2012-08-10 API version, so they are reported
// available whenever DynamoDB support is detected.
struct dynamodb_type_support
{
#if D_ENV_DYNAMODB_DETECTED

    static constexpr bool has_string        = true;
    static constexpr bool has_number        = true;
    static constexpr bool has_binary        = true;
    static constexpr bool has_boolean       = true;
    static constexpr bool has_null          = true;
    static constexpr bool has_map           = true;
    static constexpr bool has_list          = true;
    static constexpr bool has_string_set    = true;
    static constexpr bool has_number_set    = true;
    static constexpr bool has_binary_set    = true;

#else
    static constexpr bool has_string        = false;
    static constexpr bool has_number        = false;
    static constexpr bool has_binary        = false;
    static constexpr bool has_boolean       = false;
    static constexpr bool has_null          = false;
    static constexpr bool has_map           = false;
    static constexpr bool has_list          = false;
    static constexpr bool has_string_set    = false;
    static constexpr bool has_number_set    = false;
    static constexpr bool has_binary_set    = false;
#endif  // D_ENV_DYNAMODB_DETECTED
};

// dynamodb_feature_support
//   struct: compile-time service feature availability flags.
struct dynamodb_feature_support
{
#if D_ENV_DYNAMODB_DETECTED

    // core data-plane operations (always present)
    static constexpr bool has_item_ops      = true;
    static constexpr bool has_query_scan    = true;
    static constexpr bool has_batch_ops     = true;

    // conditional writes (condition expressions)
    static constexpr bool has_conditional_writes =
    #if D_ENV_DYNAMODB_HAS_CONDITIONAL_WRITES
        true;
    #else
        false;
    #endif

    // transactions (TransactWriteItems / TransactGetItems)
    static constexpr bool has_transactions =
    #if D_ENV_DYNAMODB_HAS_TRANSACTIONS
        true;
    #else
        false;
    #endif

    // PartiQL
    static constexpr bool has_partiql =
    #if D_ENV_DYNAMODB_HAS_PARTIQL
        true;
    #else
        false;
    #endif

    // DynamoDB Streams
    static constexpr bool has_streams =
    #if D_ENV_DYNAMODB_HAS_STREAMS
        true;
    #else
        false;
    #endif

    // secondary indexes (GSI / LSI)
    static constexpr bool has_global_secondary_indexes =
    #if D_ENV_DYNAMODB_HAS_GSI
        true;
    #else
        false;
    #endif

    static constexpr bool has_local_secondary_indexes =
    #if D_ENV_DYNAMODB_HAS_LSI
        true;
    #else
        false;
    #endif

    // global tables (multi-region replication)
    static constexpr bool has_global_tables =
    #if D_ENV_DYNAMODB_HAS_GLOBAL_TABLES
        true;
    #else
        false;
    #endif

    // on-demand (pay-per-request) capacity mode
    static constexpr bool has_on_demand_capacity =
    #if D_ENV_DYNAMODB_HAS_ON_DEMAND
        true;
    #else
        false;
    #endif

    // time-to-live
    static constexpr bool has_ttl =
    #if D_ENV_DYNAMODB_HAS_TTL
        true;
    #else
        false;
    #endif

    // on-demand backup
    static constexpr bool has_backup =
    #if D_ENV_DYNAMODB_HAS_BACKUP
        true;
    #else
        false;
    #endif

    // point-in-time recovery
    static constexpr bool has_pitr =
    #if D_ENV_DYNAMODB_HAS_PITR
        true;
    #else
        false;
    #endif

    // DynamoDB Accelerator (DAX) in-memory cache
    static constexpr bool has_dax =
    #if D_ENV_DYNAMODB_HAS_DAX
        true;
    #else
        false;
    #endif

    // server-side encryption at rest
    static constexpr bool has_encryption_at_rest =
    #if D_ENV_DYNAMODB_HAS_ENCRYPTION
        true;
    #else
        false;
    #endif

    // strongly consistent reads
    static constexpr bool has_strong_consistency =
    #if D_ENV_DYNAMODB_HAS_STRONG_CONSISTENCY
        true;
    #else
        false;
    #endif

    // resource tagging
    static constexpr bool has_tagging =
    #if D_ENV_DYNAMODB_HAS_TAGGING
        true;
    #else
        false;
    #endif

#else
    static constexpr bool has_item_ops                 = false;
    static constexpr bool has_query_scan               = false;
    static constexpr bool has_batch_ops                = false;
    static constexpr bool has_conditional_writes       = false;
    static constexpr bool has_transactions             = false;
    static constexpr bool has_partiql                  = false;
    static constexpr bool has_streams                  = false;
    static constexpr bool has_global_secondary_indexes = false;
    static constexpr bool has_local_secondary_indexes  = false;
    static constexpr bool has_global_tables            = false;
    static constexpr bool has_on_demand_capacity       = false;
    static constexpr bool has_ttl                      = false;
    static constexpr bool has_backup                   = false;
    static constexpr bool has_pitr                     = false;
    static constexpr bool has_dax                      = false;
    static constexpr bool has_encryption_at_rest       = false;
    static constexpr bool has_strong_consistency       = false;
    static constexpr bool has_tagging                  = false;
#endif  // D_ENV_DYNAMODB_DETECTED
};


// =============================================================================
// IV.  DYNAMODB API VERSION INFORMATION
// =============================================================================
// DynamoDB is a managed service: there is no on-premises server version to
// query. What is versioned is the service API contract (date-stamped, e.g.
// "2012-08-10") and the AWS SDK build in use. This struct exposes that
// metadata in the same shape as the other vendors' version_info structs.

// dynamodb_version_info
//   struct: compile-time API / SDK version metadata.
struct dynamodb_version_info
{
#if D_ENV_DYNAMODB_DETECTED
    static constexpr bool          detected    = true;
    static constexpr const char*   api_version = D_ENV_DYNAMODB_API_VERSION;
    static constexpr std::uint32_t sdk_id      = D_ENV_DYNAMODB_SDK_VERSION_ID;
    static constexpr std::uint16_t sdk_major   = D_ENV_DYNAMODB_SDK_VERSION_MAJOR;
    static constexpr std::uint16_t sdk_minor   = D_ENV_DYNAMODB_SDK_VERSION_MINOR;
    static constexpr std::uint16_t sdk_patch   = D_ENV_DYNAMODB_SDK_VERSION_PATCH;
    static constexpr const char*   sdk_string  = D_ENV_DYNAMODB_SDK_VERSION_STRING;
#else
    static constexpr bool          detected    = false;
    static constexpr const char*   api_version = "not detected";
    static constexpr std::uint32_t sdk_id      = 0;
    static constexpr std::uint16_t sdk_major   = 0;
    static constexpr std::uint16_t sdk_minor   = 0;
    static constexpr std::uint16_t sdk_patch   = 0;
    static constexpr const char*   sdk_string  = "not detected";
#endif

    // sdk_at_least
    //   function: returns true if the detected AWS SDK build is at
    // least (major, minor, patch).
    static constexpr bool sdk_at_least(std::uint16_t _major,
                                       std::uint16_t _minor,
                                       std::uint16_t _patch) noexcept
    {
        return sdk_id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// V.   DYNAMODB MODE / CONSISTENCY ENUMERATIONS
// =============================================================================

// dynamodb_capacity_mode
//   enumeration: table throughput billing mode.
enum class dynamodb_capacity_mode : std::uint8_t
{
    provisioned = 0,    // fixed RCU / WCU
    on_demand   = 1     // pay-per-request
};

// dynamodb_read_consistency
//   enumeration: read consistency model for reads that support it.
enum class dynamodb_read_consistency : std::uint8_t
{
    eventual = 0,       // eventually consistent (default, cheaper)
    strong   = 1        // strongly consistent
};

// dynamodb_endpoint_mode
//   enumeration: which endpoint the connection targets.
enum class dynamodb_endpoint_mode : std::uint8_t
{
    standard = 0,       // regional DynamoDB endpoint
    dax      = 1,       // DynamoDB Accelerator (DAX) cluster
    local    = 2        // DynamoDB Local (dev/test)
};

// dynamodb_return_values
//   enumeration: ReturnValues option controlling what a write returns.
enum class dynamodb_return_values : std::uint8_t
{
    none        = 0,    // NONE
    all_old     = 1,    // ALL_OLD
    updated_old = 2,    // UPDATED_OLD
    all_new     = 3,    // ALL_NEW
    updated_new = 4     // UPDATED_NEW
};


// =============================================================================
// VI.  DYNAMODB CONNECTION CONFIGURATION
// =============================================================================

// dynamodb_connect_config
//   struct: DynamoDB-specific connection configuration extending the
// generic connection_config with AWS region, endpoint override,
// credential profile, DAX endpoint, retry policy, capacity mode, and
// default read consistency.
struct dynamodb_connect_config
{
    connection_config           base;

    // AWS region (e.g. "us-east-1"). Required for the standard endpoint.
    std::string                 region;

    // optional explicit endpoint override (DynamoDB Local, VPC endpoint,
    // FIPS endpoint, etc.); when empty the SDK derives it from region.
    std::string                 endpoint_override;

    // endpoint topology
    dynamodb_endpoint_mode      endpoint_mode;

    // DAX cluster endpoint (used when endpoint_mode == dax).
    std::string                 dax_endpoint;

    // credentials — when use_profile is true, profile_name selects a
    // named profile from the shared credentials file; otherwise the
    // base.username / base.password fields carry the access key id and
    // secret access key respectively.
    bool                        use_profile;
    std::string                 profile_name;
    std::string                 session_token;

    // default read consistency for reads that accept it.
    dynamodb_read_consistency   default_consistency;

    // default capacity mode for tables created through this connection.
    dynamodb_capacity_mode      default_capacity_mode;

    // retry policy
    int                         max_retries;
    std::chrono::milliseconds   request_timeout;

    // optional tagging applied to created resources.
    std::map<std::string, std::string> default_tags;

    dynamodb_connect_config()
        : region("us-east-1")
        , endpoint_mode(dynamodb_endpoint_mode::standard)
        , use_profile(false)
        , default_consistency(dynamodb_read_consistency::eventual)
        , default_capacity_mode(dynamodb_capacity_mode::on_demand)
        , max_retries(3)
        , request_timeout(std::chrono::milliseconds(10000))
    {
        // DynamoDB is reached over HTTPS; host/port in the generic
        // config are informational and overridden by region/endpoint.
        base.host        = "dynamodb.us-east-1.amazonaws.com";
        base.port        = 443;
        base.enable_ssl  = true;
    }

    explicit dynamodb_connect_config(const connection_config& _base)
        : base(_base)
        , region("us-east-1")
        , endpoint_mode(dynamodb_endpoint_mode::standard)
        , use_profile(false)
        , default_consistency(dynamodb_read_consistency::eventual)
        , default_capacity_mode(dynamodb_capacity_mode::on_demand)
        , max_retries(3)
        , request_timeout(std::chrono::milliseconds(10000))
    {
        if (base.port == 0)
        {
            base.port = 443;
        }
    }
};


// =============================================================================
// VII. DYNAMODB CONNECTION
// =============================================================================

// dynamodb_connection
//   class: concrete DynamoDB connection implementation via the AWS SDK
// for C++ (or equivalent). This is the CRTP leaf class; _impl methods
// are defined in dynamodb.cpp which includes the AWS SDK headers.
//
// Usage:
//   dynamodb_connection conn;
//   conn.connect(dynamodb_connect_config{...});
//   dynamodb_item item = { {"id", value{std::string{"42"}}},
//                          {"name", value{std::string{"teer"}}} };
//   conn.put_item("users", item);
//   auto got = conn.get_item("users", {{"id", value{std::string{"42"}}}});
class dynamodb_connection
    : public database_connection<dynamodb_connection,
                                 database_type::dynamodb>
{
public:
    using base_type       = database_connection<
        dynamodb_connection, database_type::dynamodb>;
    using type_support    = dynamodb_type_support;
    using feature_support = dynamodb_feature_support;
    using version_info    = dynamodb_version_info;

    // item_type / key_type
    //   types: convenience aliases re-exporting the trait-layer item
    // and key representations.
    using item_type   = dynamodb_item;
    using key_type    = dynamodb_key;

    // scan_page / query_page
    //   types: a page of results plus the LastEvaluatedKey cursor for
    // continuation (empty optional indicates the final page).
    using result_page = std::pair<std::vector<dynamodb_item>,
                                  std::optional<dynamodb_key>>;

    dynamodb_connection()
        : base_type()
    {
    }

    explicit dynamodb_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit dynamodb_connection(const dynamodb_connect_config& _config)
        : base_type(_config.base),
          m_dynamodb_config(_config)
    {
    }

    ~dynamodb_connection() = default;

    // disable copying
    dynamodb_connection(const dynamodb_connection&)            = delete;
    dynamodb_connection& operator=(const dynamodb_connection&) = delete;

    // enable moving
    dynamodb_connection(dynamodb_connection&&) noexcept            = default;
    dynamodb_connection& operator=(dynamodb_connection&&) noexcept = default;


    // -----------------------------------------------------------------
    // item operations
    // -----------------------------------------------------------------

    // put_item
    //   function: PutItem — writes (or replaces) a single item.
    bool put_item(const std::string&  _table,
                  const dynamodb_item& _item)
    {
        this->ensure_connected();

        return self().put_item_impl(_table, _item);
    }

    // get_item
    //   function: GetItem — reads a single item by primary key.
    std::optional<dynamodb_item> get_item(
        const std::string& _table,
        const dynamodb_key& _key) const
    {
        return self().get_item_impl(_table, _key);
    }

    // update_item
    //   function: UpdateItem — mutates attributes of an existing item.
    bool update_item(const std::string&  _table,
                     const dynamodb_key&  _key,
                     const dynamodb_item& _updates)
    {
        this->ensure_connected();

        return self().update_item_impl(_table, _key, _updates);
    }

    // delete_item
    //   function: DeleteItem — removes a single item by primary key.
    bool delete_item(const std::string& _table,
                     const dynamodb_key& _key)
    {
        this->ensure_connected();

        return self().delete_item_impl(_table, _key);
    }


    // -----------------------------------------------------------------
    // batch operations
    // -----------------------------------------------------------------

    // batch_get_item
    //   function: BatchGetItem — reads up to 100 items in one call.
    std::vector<dynamodb_item> batch_get_item(
        const std::string&             _table,
        const std::vector<dynamodb_key>& _keys) const
    {
        return self().batch_get_item_impl(_table, _keys);
    }

    // batch_write_item
    //   function: BatchWriteItem — writes up to 25 items in one call.
    bool batch_write_item(
        const std::string&              _table,
        const std::vector<dynamodb_item>& _items)
    {
        this->ensure_connected();

        return self().batch_write_item_impl(_table, _items);
    }


    // -----------------------------------------------------------------
    // query and scan
    // -----------------------------------------------------------------

    // query
    //   function: Query — partition-key-bounded retrieval with an
    // optional sort-key condition expression. Returns one page plus a
    // continuation cursor.
    result_page query(const std::string& _table,
                      const std::string& _key_condition) const
    {
        return self().query_impl(_table, _key_condition);
    }

    // scan
    //   function: Scan — full-table sequential read. Returns one page
    // plus a continuation cursor. Expensive; prefer query().
    result_page scan(const std::string& _table) const
    {
        return self().scan_impl(_table);
    }


    // -----------------------------------------------------------------
    // transactions
    // -----------------------------------------------------------------

    // transact_write_items
    //   function: TransactWriteItems — all-or-nothing write of up to
    // 100 items.
    bool transact_write_items(
        const std::vector<dynamodb_item>& _items)
    {
        this->ensure_connected();

        return self().transact_write_items_impl(_items);
    }

    // transact_get_items
    //   function: TransactGetItems — consistent snapshot read of up to
    // 100 items.
    std::vector<dynamodb_item> transact_get_items(
        const std::vector<dynamodb_key>& _keys) const
    {
        return self().transact_get_items_impl(_keys);
    }


    // -----------------------------------------------------------------
    // PartiQL
    // -----------------------------------------------------------------

    // execute_statement
    //   function: ExecuteStatement — a single PartiQL statement.
    auto execute_statement(const std::string& _statement)
    {
        this->ensure_connected();

        return self().execute_statement_impl(_statement);
    }

    // batch_execute_statement
    //   function: BatchExecuteStatement — multiple PartiQL statements.
    auto batch_execute_statement(
        const std::vector<std::string>& _statements)
    {
        this->ensure_connected();

        return self().batch_execute_statement_impl(_statements);
    }

    // execute_transaction
    //   function: ExecuteTransaction — transactional PartiQL statements.
    auto execute_transaction(
        const std::vector<std::string>& _statements)
    {
        this->ensure_connected();

        return self().execute_transaction_impl(_statements);
    }


    // -----------------------------------------------------------------
    // conditional writes
    // -----------------------------------------------------------------

    // put_item_conditional
    //   function: PutItem with a ConditionExpression. Returns false if
    // the condition fails (ConditionalCheckFailed).
    bool put_item_conditional(const std::string&  _table,
                              const dynamodb_item& _item,
                              const std::string&   _condition)
    {
        this->ensure_connected();

        return self().put_item_conditional_impl(_table,
                                                _item,
                                                _condition);
    }

    // delete_item_conditional
    //   function: DeleteItem with a ConditionExpression. Returns false
    // if the condition fails.
    bool delete_item_conditional(const std::string& _table,
                                 const dynamodb_key& _key,
                                 const std::string&  _condition)
    {
        this->ensure_connected();

        return self().delete_item_conditional_impl(_table,
                                                   _key,
                                                   _condition);
    }


    // -----------------------------------------------------------------
    // table management (control plane)
    // -----------------------------------------------------------------

    // create_table
    //   function: CreateTable. An empty _sort_key denotes a simple
    // (partition-only) key schema.
    bool create_table(const std::string& _table,
                      const std::string& _partition_key,
                      const std::string& _sort_key)
    {
        this->ensure_connected();

        return self().create_table_impl(_table,
                                        _partition_key,
                                        _sort_key);
    }

    // delete_table
    //   function: DeleteTable.
    bool delete_table(const std::string& _table)
    {
        this->ensure_connected();

        return self().delete_table_impl(_table);
    }

    // describe_table
    //   function: DescribeTable — returns the table description block.
    std::string describe_table(const std::string& _table) const
    {
        return self().describe_table_impl(_table);
    }

    // update_table
    //   function: UpdateTable — throughput / billing / index changes
    // expressed as a JSON spec string.
    bool update_table(const std::string& _table,
                      const std::string& _spec)
    {
        this->ensure_connected();

        return self().update_table_impl(_table, _spec);
    }

    // list_tables
    //   function: ListTables — returns the table names in the account
    // and region.
    std::vector<std::string> list_tables() const
    {
        return self().list_tables_impl();
    }


    // -----------------------------------------------------------------
    // secondary indexes
    // -----------------------------------------------------------------

    // create_global_secondary_index
    //   function: UpdateTable with a GSI create action.
    bool create_global_secondary_index(
        const std::string& _table,
        const std::string& _index,
        const std::string& _partition_key)
    {
        this->ensure_connected();

        return self().create_global_secondary_index_impl(_table,
                                                         _index,
                                                         _partition_key);
    }

    // delete_global_secondary_index
    //   function: UpdateTable with a GSI delete action.
    bool delete_global_secondary_index(const std::string& _table,
                                       const std::string& _index)
    {
        this->ensure_connected();

        return self().delete_global_secondary_index_impl(_table,
                                                        _index);
    }

    // query_index
    //   function: Query against a secondary index. Returns one page
    // plus a continuation cursor.
    result_page query_index(const std::string& _table,
                            const std::string& _index,
                            const std::string& _key_condition) const
    {
        return self().query_index_impl(_table, _index, _key_condition);
    }


    // -----------------------------------------------------------------
    // DynamoDB Streams
    // -----------------------------------------------------------------

    // describe_stream
    //   function: DescribeStream.
    std::string describe_stream(const std::string& _stream_arn) const
    {
        return self().describe_stream_impl(_stream_arn);
    }

    // get_shard_iterator
    //   function: GetShardIterator.
    std::string get_shard_iterator(const std::string& _stream_arn,
                                   const std::string& _shard_id) const
    {
        return self().get_shard_iterator_impl(_stream_arn, _shard_id);
    }

    // get_records
    //   function: GetRecords — reads stream records for a shard
    // iterator.
    std::vector<dynamodb_item> get_records(
        const std::string& _shard_iterator) const
    {
        return self().get_records_impl(_shard_iterator);
    }

    // list_streams
    //   function: ListStreams — stream ARNs for a table.
    std::vector<std::string> list_streams(
        const std::string& _table) const
    {
        return self().list_streams_impl(_table);
    }


    // -----------------------------------------------------------------
    // TTL management
    // -----------------------------------------------------------------

    // update_time_to_live
    //   function: UpdateTimeToLive — enables/disables a TTL attribute.
    bool update_time_to_live(const std::string& _table,
                             const std::string& _attribute,
                             bool               _enabled)
    {
        this->ensure_connected();

        return self().update_time_to_live_impl(_table,
                                              _attribute,
                                              _enabled);
    }

    // describe_time_to_live
    //   function: DescribeTimeToLive — TTL configuration status.
    std::string describe_time_to_live(const std::string& _table) const
    {
        return self().describe_time_to_live_impl(_table);
    }


    // -----------------------------------------------------------------
    // backup / point-in-time recovery
    // -----------------------------------------------------------------

    // create_backup
    //   function: CreateBackup — on-demand backup; returns the backup
    // ARN.
    std::string create_backup(const std::string& _table,
                              const std::string& _backup_name)
    {
        this->ensure_connected();

        return self().create_backup_impl(_table, _backup_name);
    }

    // restore_table_from_backup
    //   function: RestoreTableFromBackup.
    bool restore_table_from_backup(const std::string& _table,
                                   const std::string& _backup_arn)
    {
        this->ensure_connected();

        return self().restore_table_from_backup_impl(_table,
                                                    _backup_arn);
    }

    // describe_continuous_backups
    //   function: DescribeContinuousBackups — PITR status.
    std::string describe_continuous_backups(
        const std::string& _table) const
    {
        return self().describe_continuous_backups_impl(_table);
    }


    // -----------------------------------------------------------------
    // diagnostics
    // -----------------------------------------------------------------

    // describe_limits
    //   function: DescribeLimits — account/table capacity limits.
    std::string describe_limits() const
    {
        return self().describe_limits_impl();
    }

    // describe_endpoints
    //   function: DescribeEndpoints — regional endpoint discovery.
    std::string describe_endpoints() const
    {
        return self().describe_endpoints_impl();
    }

    // table_status
    //   function: convenience accessor over DescribeTable returning the
    // table state (CREATING / ACTIVE / UPDATING / DELETING).
    std::string table_status(const std::string& _table) const
    {
        return self().table_status_impl(_table);
    }


    // -----------------------------------------------------------------
    // global tables
    // -----------------------------------------------------------------

    // create_global_table
    //   function: CreateGlobalTable — multi-region replication.
    bool create_global_table(
        const std::string&              _table,
        const std::vector<std::string>& _regions)
    {
        this->ensure_connected();

        return self().create_global_table_impl(_table, _regions);
    }

    // describe_global_table
    //   function: DescribeGlobalTable.
    std::string describe_global_table(const std::string& _table) const
    {
        return self().describe_global_table_impl(_table);
    }

    // update_global_table
    //   function: UpdateGlobalTable — add/remove replica regions.
    bool update_global_table(
        const std::string&              _table,
        const std::vector<std::string>& _regions)
    {
        this->ensure_connected();

        return self().update_global_table_impl(_table, _regions);
    }


    // -----------------------------------------------------------------
    // resource tagging
    // -----------------------------------------------------------------

    // tag_resource
    //   function: TagResource.
    bool tag_resource(
        const std::string&                        _arn,
        const std::map<std::string, std::string>& _tags)
    {
        this->ensure_connected();

        return self().tag_resource_impl(_arn, _tags);
    }

    // untag_resource
    //   function: UntagResource.
    bool untag_resource(const std::string&              _arn,
                        const std::vector<std::string>& _keys)
    {
        this->ensure_connected();

        return self().untag_resource_impl(_arn, _keys);
    }

    // list_tags_of_resource
    //   function: ListTagsOfResource.
    std::map<std::string, std::string> list_tags_of_resource(
        const std::string& _arn) const
    {
        return self().list_tags_of_resource_impl(_arn);
    }


    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_transactions() noexcept
    {
        return feature_support::has_transactions;
    }

    static constexpr bool supports_partiql() noexcept
    {
        return feature_support::has_partiql;
    }

    static constexpr bool supports_streams() noexcept
    {
        return feature_support::has_streams;
    }

    static constexpr bool supports_global_tables() noexcept
    {
        return feature_support::has_global_tables;
    }

    static constexpr bool supports_on_demand_capacity() noexcept
    {
        return feature_support::has_on_demand_capacity;
    }

    static constexpr bool supports_ttl() noexcept
    {
        return feature_support::has_ttl;
    }

    static constexpr bool supports_backup() noexcept
    {
        return feature_support::has_backup;
    }

    static constexpr bool supports_pitr() noexcept
    {
        return feature_support::has_pitr;
    }

    static constexpr bool supports_dax() noexcept
    {
        return feature_support::has_dax;
    }

    static constexpr bool supports_strong_consistency() noexcept
    {
        return feature_support::has_strong_consistency;
    }


    // -----------------------------------------------------------------
    // data type mapping
    // -----------------------------------------------------------------

    static field_type map_type(dynamodb_attribute_type _type) noexcept
    {
        return dynamodb_attribute_type_to_field_type(_type);
    }

    static dynamodb_attribute_type type_from_descriptor(
        const std::string& _descriptor) noexcept
    {
        return dynamodb_attribute_type_from_descriptor(_descriptor);
    }

    static const char* native_descriptor(field_type _type) noexcept
    {
        return field_type_to_dynamodb_descriptor(_type);
    }


    // -----------------------------------------------------------------
    // DynamoDB-specific configuration
    // -----------------------------------------------------------------

    // get_dynamodb_config
    //   function: returns the DynamoDB-specific configuration.
    const dynamodb_connect_config& get_dynamodb_config() const noexcept
    {
        return m_dynamodb_config;
    }

    // set_dynamodb_config
    //   function: replaces the DynamoDB-specific configuration. Must be
    // called before connect().
    void set_dynamodb_config(const dynamodb_connect_config& _config)
    {
        m_dynamodb_config = _config;
        this->m_config    = _config.base;
    }

    // get_default_consistency / set_default_consistency
    //   functions: read/modify the default read consistency.
    dynamodb_read_consistency get_default_consistency() const noexcept
    {
        return m_dynamodb_config.default_consistency;
    }

    void set_default_consistency(dynamodb_read_consistency _c) noexcept
    {
        m_dynamodb_config.default_consistency = _c;

        return;
    }


    // -----------------------------------------------------------------
    // _impl methods (defined in dynamodb.cpp)
    // -----------------------------------------------------------------

    void         connect_impl();
    void         disconnect_impl();
    bool         is_connected_impl() const;
    bool         ping_impl() const;
    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;

    // item operations
    bool         put_item_impl(const std::string&  _table,
                               const dynamodb_item& _item);
    std::optional<dynamodb_item>
                 get_item_impl(const std::string& _table,
                               const dynamodb_key& _key) const;
    bool         update_item_impl(const std::string&  _table,
                                  const dynamodb_key&  _key,
                                  const dynamodb_item& _updates);
    bool         delete_item_impl(const std::string& _table,
                                  const dynamodb_key& _key);

    // batch operations
    std::vector<dynamodb_item>
                 batch_get_item_impl(
                     const std::string&             _table,
                     const std::vector<dynamodb_key>& _keys) const;
    bool         batch_write_item_impl(
                     const std::string&              _table,
                     const std::vector<dynamodb_item>& _items);

    // query and scan
    result_page  query_impl(const std::string& _table,
                            const std::string& _key_condition) const;
    result_page  scan_impl(const std::string& _table) const;

    // transactions
    bool         transact_write_items_impl(
                     const std::vector<dynamodb_item>& _items);
    std::vector<dynamodb_item>
                 transact_get_items_impl(
                     const std::vector<dynamodb_key>& _keys) const;

    // PartiQL
    auto         execute_statement_impl(const std::string& _statement)
                     -> std::unique_ptr<
                         result_set<struct dynamodb_result_set_impl>>;
    auto         batch_execute_statement_impl(
                     const std::vector<std::string>& _statements)
                     -> std::unique_ptr<
                         result_set<struct dynamodb_result_set_impl>>;
    auto         execute_transaction_impl(
                     const std::vector<std::string>& _statements)
                     -> std::unique_ptr<
                         result_set<struct dynamodb_result_set_impl>>;

    // conditional writes
    bool         put_item_conditional_impl(
                     const std::string&  _table,
                     const dynamodb_item& _item,
                     const std::string&   _condition);
    bool         delete_item_conditional_impl(
                     const std::string& _table,
                     const dynamodb_key& _key,
                     const std::string&  _condition);

    // table management
    bool         create_table_impl(const std::string& _table,
                                   const std::string& _partition_key,
                                   const std::string& _sort_key);
    bool         delete_table_impl(const std::string& _table);
    std::string  describe_table_impl(const std::string& _table) const;
    bool         update_table_impl(const std::string& _table,
                                   const std::string& _spec);
    std::vector<std::string>
                 list_tables_impl() const;

    // secondary indexes
    bool         create_global_secondary_index_impl(
                     const std::string& _table,
                     const std::string& _index,
                     const std::string& _partition_key);
    bool         delete_global_secondary_index_impl(
                     const std::string& _table,
                     const std::string& _index);
    result_page  query_index_impl(
                     const std::string& _table,
                     const std::string& _index,
                     const std::string& _key_condition) const;

    // streams
    std::string  describe_stream_impl(
                     const std::string& _stream_arn) const;
    std::string  get_shard_iterator_impl(
                     const std::string& _stream_arn,
                     const std::string& _shard_id) const;
    std::vector<dynamodb_item>
                 get_records_impl(
                     const std::string& _shard_iterator) const;
    std::vector<std::string>
                 list_streams_impl(const std::string& _table) const;

    // TTL management
    bool         update_time_to_live_impl(const std::string& _table,
                                          const std::string& _attribute,
                                          bool               _enabled);
    std::string  describe_time_to_live_impl(
                     const std::string& _table) const;

    // backup / PITR
    std::string  create_backup_impl(const std::string& _table,
                                    const std::string& _backup_name);
    bool         restore_table_from_backup_impl(
                     const std::string& _table,
                     const std::string& _backup_arn);
    std::string  describe_continuous_backups_impl(
                     const std::string& _table) const;

    // diagnostics
    std::string  describe_limits_impl() const;
    std::string  describe_endpoints_impl() const;
    std::string  table_status_impl(const std::string& _table) const;

    // global tables
    bool         create_global_table_impl(
                     const std::string&              _table,
                     const std::vector<std::string>& _regions);
    std::string  describe_global_table_impl(
                     const std::string& _table) const;
    bool         update_global_table_impl(
                     const std::string&              _table,
                     const std::vector<std::string>& _regions);

    // tagging
    bool         tag_resource_impl(
                     const std::string&                        _arn,
                     const std::map<std::string, std::string>& _tags);
    bool         untag_resource_impl(
                     const std::string&              _arn,
                     const std::vector<std::string>& _keys);
    std::map<std::string, std::string>
                 list_tags_of_resource_impl(
                     const std::string& _arn) const;


    // -----------------------------------------------------------------
    // feature-gated methods
    // -----------------------------------------------------------------

#if D_ENV_DYNAMODB_DETECTED

    #if D_ENV_DYNAMODB_HAS_ON_DEMAND
    // set_billing_mode
    //   function: switches a table between provisioned and on-demand
    // capacity. Requires on-demand capacity support.
    bool set_billing_mode(const std::string&     _table,
                          dynamodb_capacity_mode _mode);
    #endif

    #if D_ENV_DYNAMODB_HAS_ENCRYPTION
    // enable_encryption
    //   function: enables server-side encryption at rest with an
    // optional KMS key ARN (empty -> AWS-owned key).
    bool enable_encryption(const std::string& _table,
                           const std::string& _kms_key_arn);
    #endif

#endif  // D_ENV_DYNAMODB_DETECTED


private:
    dynamodb_connect_config m_dynamodb_config;

    dynamodb_connection& self()
    {
        return *this;
    }

    const dynamodb_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// dynamodb_result_set_impl
//   struct: forward declaration of the DynamoDB result set
// implementation (used by the PartiQL surface).
struct dynamodb_result_set_impl;


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former dynamodb_traits.hpp / dynamodb_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// =============================================================================
// 0.   DYNAMODB ITEM / KEY TYPE ALIASES
// =============================================================================
// DynamoDB items are schemaless attribute maps. At this abstraction layer an
// attribute value is represented by the generic djinterp `value` variant
// (from database.hpp); the wire-level distinction between DynamoDB's S / N /
// B / BOOL / NULL / M / L / SS / NS / BS descriptors is reconstructed by the
// concrete connection's marshalling code in dynamodb.cpp.

// dynamodb_item
//   type: a DynamoDB item as a map of attribute name to value.
using dynamodb_item = std::map<std::string, value>;

// dynamodb_key
//   type: a DynamoDB primary key (partition key, plus optional sort key)
// as a map of attribute name to value.
using dynamodb_key = std::map<std::string, value>;


// =============================================================================
// IX.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  item operations
// -------------------------------------------------------------------------

// dynamodb_put_item_t
//   detector: put_item(table, item) method.
// wraps PutItem — writes (or replaces) a single item.
template<typename _Type>
using dynamodb_put_item_t =
    decltype(std::declval<_Type&>().put_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_item&>()));

// dynamodb_get_item_t
//   detector: get_item(table, key) const method.
// wraps GetItem — reads a single item by primary key.
template<typename _Type>
using dynamodb_get_item_t =
    decltype(std::declval<const _Type&>().get_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>()));

// dynamodb_update_item_t
//   detector: update_item(table, key, updates) method.
// wraps UpdateItem — mutates attributes of an existing item.
template<typename _Type>
using dynamodb_update_item_t =
    decltype(std::declval<_Type&>().update_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>(),
        std::declval<const dynamodb_item&>()));

// dynamodb_delete_item_t
//   detector: delete_item(table, key) method.
// wraps DeleteItem — removes a single item by primary key.
template<typename _Type>
using dynamodb_delete_item_t =
    decltype(std::declval<_Type&>().delete_item(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>()));


// -------------------------------------------------------------------------
// B.  batch operations
// -------------------------------------------------------------------------

// dynamodb_batch_get_item_t
//   detector: batch_get_item(table, keys) const method.
// wraps BatchGetItem — reads up to 100 items in one round-trip.
template<typename _Type>
using dynamodb_batch_get_item_t =
    decltype(std::declval<const _Type&>().batch_get_item(
        std::declval<const std::string&>(),
        std::declval<const std::vector<dynamodb_key>&>()));

// dynamodb_batch_write_item_t
//   detector: batch_write_item(table, items) method.
// wraps BatchWriteItem — writes/deletes up to 25 items in one
// round-trip.
template<typename _Type>
using dynamodb_batch_write_item_t =
    decltype(std::declval<_Type&>().batch_write_item(
        std::declval<const std::string&>(),
        std::declval<const std::vector<dynamodb_item>&>()));


// -------------------------------------------------------------------------
// C.  query and scan
// -------------------------------------------------------------------------

// dynamodb_query_t
//   detector: query(table, key_condition) const method.
// wraps Query — partition-key-bounded retrieval with an optional
// sort-key condition expression.
template<typename _Type>
using dynamodb_query_t =
    decltype(std::declval<const _Type&>().query(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_scan_t
//   detector: scan(table) const method.
// wraps Scan — full-table sequential read (expensive; prefer query).
template<typename _Type>
using dynamodb_scan_t =
    decltype(std::declval<const _Type&>().scan(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// D.  transactions
// -------------------------------------------------------------------------

// dynamodb_transact_write_items_t
//   detector: transact_write_items(items) method.
// wraps TransactWriteItems — all-or-nothing write of up to 100 items.
template<typename _Type>
using dynamodb_transact_write_items_t =
    decltype(std::declval<_Type&>().transact_write_items(
        std::declval<const std::vector<dynamodb_item>&>()));

// dynamodb_transact_get_items_t
//   detector: transact_get_items(keys) const method.
// wraps TransactGetItems — consistent snapshot read of up to 100 items.
template<typename _Type>
using dynamodb_transact_get_items_t =
    decltype(std::declval<const _Type&>().transact_get_items(
        std::declval<const std::vector<dynamodb_key>&>()));


// -------------------------------------------------------------------------
// E.  PartiQL
// -------------------------------------------------------------------------

// dynamodb_execute_statement_t
//   detector: execute_statement(statement) method.
// wraps ExecuteStatement — a single PartiQL statement.
template<typename _Type>
using dynamodb_execute_statement_t =
    decltype(std::declval<_Type&>().execute_statement(
        std::declval<const std::string&>()));

// dynamodb_batch_execute_statement_t
//   detector: batch_execute_statement(statements) method.
// wraps BatchExecuteStatement — multiple PartiQL statements.
template<typename _Type>
using dynamodb_batch_execute_statement_t =
    decltype(std::declval<_Type&>().batch_execute_statement(
        std::declval<const std::vector<std::string>&>()));

// dynamodb_execute_transaction_t
//   detector: execute_transaction(statements) method.
// wraps ExecuteTransaction — transactional PartiQL statements.
template<typename _Type>
using dynamodb_execute_transaction_t =
    decltype(std::declval<_Type&>().execute_transaction(
        std::declval<const std::vector<std::string>&>()));


// -------------------------------------------------------------------------
// F.  conditional writes
// -------------------------------------------------------------------------

// dynamodb_put_item_conditional_t
//   detector: put_item_conditional(table, item, condition) method.
// wraps PutItem with a ConditionExpression.
template<typename _Type>
using dynamodb_put_item_conditional_t =
    decltype(std::declval<_Type&>().put_item_conditional(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_item&>(),
        std::declval<const std::string&>()));

// dynamodb_delete_item_conditional_t
//   detector: delete_item_conditional(table, key, condition) method.
// wraps DeleteItem with a ConditionExpression.
template<typename _Type>
using dynamodb_delete_item_conditional_t =
    decltype(std::declval<_Type&>().delete_item_conditional(
        std::declval<const std::string&>(),
        std::declval<const dynamodb_key&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// G.  table management (control plane)
// -------------------------------------------------------------------------

// dynamodb_create_table_t
//   detector: create_table(table, partition_key, sort_key) method.
// wraps CreateTable. An empty sort_key denotes a simple (partition-only)
// key schema.
template<typename _Type>
using dynamodb_create_table_t =
    decltype(std::declval<_Type&>().create_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_delete_table_t
//   detector: delete_table(table) method.
// wraps DeleteTable.
template<typename _Type>
using dynamodb_delete_table_t =
    decltype(std::declval<_Type&>().delete_table(
        std::declval<const std::string&>()));

// dynamodb_describe_table_t
//   detector: describe_table(table) const method.
// wraps DescribeTable.
template<typename _Type>
using dynamodb_describe_table_t =
    decltype(std::declval<const _Type&>().describe_table(
        std::declval<const std::string&>()));

// dynamodb_update_table_t
//   detector: update_table(table, spec) method.
// wraps UpdateTable — throughput / billing / index changes.
template<typename _Type>
using dynamodb_update_table_t =
    decltype(std::declval<_Type&>().update_table(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_list_tables_t
//   detector: list_tables() const method.
// wraps ListTables.
template<typename _Type>
using dynamodb_list_tables_t =
    decltype(std::declval<const _Type&>().list_tables());


// -------------------------------------------------------------------------
// H.  secondary indexes
// -------------------------------------------------------------------------

// dynamodb_create_gsi_t
//   detector: create_global_secondary_index(table, index, partition_key)
// method. wraps UpdateTable with a GSI create action.
template<typename _Type>
using dynamodb_create_gsi_t =
    decltype(std::declval<_Type&>().create_global_secondary_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_delete_gsi_t
//   detector: delete_global_secondary_index(table, index) method.
// wraps UpdateTable with a GSI delete action.
template<typename _Type>
using dynamodb_delete_gsi_t =
    decltype(std::declval<_Type&>().delete_global_secondary_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_query_index_t
//   detector: query_index(table, index, key_condition) const method.
// wraps Query against a secondary index.
template<typename _Type>
using dynamodb_query_index_t =
    decltype(std::declval<const _Type&>().query_index(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// I.  DynamoDB Streams
// -------------------------------------------------------------------------

// dynamodb_describe_stream_t
//   detector: describe_stream(stream_arn) const method.
// wraps DescribeStream.
template<typename _Type>
using dynamodb_describe_stream_t =
    decltype(std::declval<const _Type&>().describe_stream(
        std::declval<const std::string&>()));

// dynamodb_get_shard_iterator_t
//   detector: get_shard_iterator(stream_arn, shard_id) const method.
// wraps GetShardIterator.
template<typename _Type>
using dynamodb_get_shard_iterator_t =
    decltype(std::declval<const _Type&>().get_shard_iterator(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_get_records_t
//   detector: get_records(shard_iterator) const method.
// wraps GetRecords.
template<typename _Type>
using dynamodb_get_records_t =
    decltype(std::declval<const _Type&>().get_records(
        std::declval<const std::string&>()));

// dynamodb_list_streams_t
//   detector: list_streams(table) const method.
// wraps ListStreams.
template<typename _Type>
using dynamodb_list_streams_t =
    decltype(std::declval<const _Type&>().list_streams(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// J.  TTL management
// -------------------------------------------------------------------------

// dynamodb_update_ttl_t
//   detector: update_time_to_live(table, attribute, enabled) method.
// wraps UpdateTimeToLive.
template<typename _Type>
using dynamodb_update_ttl_t =
    decltype(std::declval<_Type&>().update_time_to_live(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<bool>()));

// dynamodb_describe_ttl_t
//   detector: describe_time_to_live(table) const method.
// wraps DescribeTimeToLive.
template<typename _Type>
using dynamodb_describe_ttl_t =
    decltype(std::declval<const _Type&>().describe_time_to_live(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// K.  backup / point-in-time recovery
// -------------------------------------------------------------------------

// dynamodb_create_backup_t
//   detector: create_backup(table, backup_name) method.
// wraps CreateBackup — on-demand backup.
template<typename _Type>
using dynamodb_create_backup_t =
    decltype(std::declval<_Type&>().create_backup(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_restore_from_backup_t
//   detector: restore_table_from_backup(table, backup_arn) method.
// wraps RestoreTableFromBackup.
template<typename _Type>
using dynamodb_restore_from_backup_t =
    decltype(std::declval<_Type&>().restore_table_from_backup(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// dynamodb_describe_continuous_backups_t
//   detector: describe_continuous_backups(table) const method.
// wraps DescribeContinuousBackups — point-in-time recovery status.
template<typename _Type>
using dynamodb_describe_continuous_backups_t =
    decltype(std::declval<const _Type&>().describe_continuous_backups(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// L.  diagnostics
// -------------------------------------------------------------------------

// dynamodb_describe_limits_t
//   detector: describe_limits() const method.
// wraps DescribeLimits — account/table capacity limits.
template<typename _Type>
using dynamodb_describe_limits_t =
    decltype(std::declval<const _Type&>().describe_limits());

// dynamodb_describe_endpoints_t
//   detector: describe_endpoints() const method.
// wraps DescribeEndpoints — regional endpoint discovery.
template<typename _Type>
using dynamodb_describe_endpoints_t =
    decltype(std::declval<const _Type&>().describe_endpoints());

// dynamodb_table_status_t
//   detector: table_status(table) const method.
// convenience accessor over DescribeTable returning the table state
// (CREATING / ACTIVE / UPDATING / DELETING).
template<typename _Type>
using dynamodb_table_status_t =
    decltype(std::declval<const _Type&>().table_status(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// M.  global tables
// -------------------------------------------------------------------------

// dynamodb_create_global_table_t
//   detector: create_global_table(table, regions) method.
// wraps CreateGlobalTable — multi-region replication.
template<typename _Type>
using dynamodb_create_global_table_t =
    decltype(std::declval<_Type&>().create_global_table(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>()));

// dynamodb_describe_global_table_t
//   detector: describe_global_table(table) const method.
// wraps DescribeGlobalTable.
template<typename _Type>
using dynamodb_describe_global_table_t =
    decltype(std::declval<const _Type&>().describe_global_table(
        std::declval<const std::string&>()));

// dynamodb_update_global_table_t
//   detector: update_global_table(table, regions) method.
// wraps UpdateGlobalTable — add/remove replica regions.
template<typename _Type>
using dynamodb_update_global_table_t =
    decltype(std::declval<_Type&>().update_global_table(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>()));


// -------------------------------------------------------------------------
// N.  resource tagging
// -------------------------------------------------------------------------

// dynamodb_tag_resource_t
//   detector: tag_resource(arn, tags) method.
// wraps TagResource.
template<typename _Type>
using dynamodb_tag_resource_t =
    decltype(std::declval<_Type&>().tag_resource(
        std::declval<const std::string&>(),
        std::declval<const std::map<std::string, std::string>&>()));

// dynamodb_untag_resource_t
//   detector: untag_resource(arn, keys) method.
// wraps UntagResource.
template<typename _Type>
using dynamodb_untag_resource_t =
    decltype(std::declval<_Type&>().untag_resource(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>()));

// dynamodb_list_tags_of_resource_t
//   detector: list_tags_of_resource(arn) const method.
// wraps ListTagsOfResource.
template<typename _Type>
using dynamodb_list_tags_of_resource_t =
    decltype(std::declval<const _Type&>().list_tags_of_resource(
        std::declval<const std::string&>()));


// =============================================================================
// X.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_dynamodb_item_ops
//   trait: checks if type _Type supports core item operations
// (put_item + get_item + update_item + delete_item).
template<typename _Type>
struct has_dynamodb_item_ops : djinterp::conjunction<
    is_detected<dynamodb_put_item_t, clean_t<_Type>>,
    is_detected<dynamodb_get_item_t, clean_t<_Type>>,
    is_detected<dynamodb_update_item_t, clean_t<_Type>>,
    is_detected<dynamodb_delete_item_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_item_ops_v =
        has_dynamodb_item_ops<clean_t<_Type>>::value;
#endif

// has_dynamodb_batch_ops
//   trait: checks if type _Type supports batch operations
// (batch_get_item + batch_write_item).
template<typename _Type>
struct has_dynamodb_batch_ops : djinterp::conjunction<
    is_detected<dynamodb_batch_get_item_t, clean_t<_Type>>,
    is_detected<dynamodb_batch_write_item_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_batch_ops_v =
        has_dynamodb_batch_ops<clean_t<_Type>>::value;
#endif

// has_dynamodb_query_scan
//   trait: checks if type _Type supports query and scan
// (query + scan).
template<typename _Type>
struct has_dynamodb_query_scan : djinterp::conjunction<
    is_detected<dynamodb_query_t, clean_t<_Type>>,
    is_detected<dynamodb_scan_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_query_scan_v =
        has_dynamodb_query_scan<clean_t<_Type>>::value;
#endif

// has_dynamodb_transactions
//   trait: checks if type _Type supports transactions
// (transact_write_items + transact_get_items).
template<typename _Type>
struct has_dynamodb_transactions : djinterp::conjunction<
    is_detected<dynamodb_transact_write_items_t, clean_t<_Type>>,
    is_detected<dynamodb_transact_get_items_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_transactions_v =
        has_dynamodb_transactions<clean_t<_Type>>::value;
#endif

// has_dynamodb_partiql
//   trait: checks if type _Type supports PartiQL
// (execute_statement + batch_execute_statement + execute_transaction).
template<typename _Type>
struct has_dynamodb_partiql : djinterp::conjunction<
    is_detected<dynamodb_execute_statement_t, clean_t<_Type>>,
    is_detected<dynamodb_batch_execute_statement_t, clean_t<_Type>>,
    is_detected<dynamodb_execute_transaction_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_partiql_v =
        has_dynamodb_partiql<clean_t<_Type>>::value;
#endif

// has_dynamodb_conditional_writes
//   trait: checks if type _Type supports conditional writes
// (put_item_conditional + delete_item_conditional).
template<typename _Type>
struct has_dynamodb_conditional_writes : djinterp::conjunction<
    is_detected<dynamodb_put_item_conditional_t, clean_t<_Type>>,
    is_detected<dynamodb_delete_item_conditional_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_conditional_writes_v =
        has_dynamodb_conditional_writes<clean_t<_Type>>::value;
#endif

// has_dynamodb_table_management
//   trait: checks if type _Type supports table management
// (create_table + delete_table + describe_table + update_table +
// list_tables).
template<typename _Type>
struct has_dynamodb_table_management : djinterp::conjunction<
    is_detected<dynamodb_create_table_t, clean_t<_Type>>,
    is_detected<dynamodb_delete_table_t, clean_t<_Type>>,
    is_detected<dynamodb_describe_table_t, clean_t<_Type>>,
    is_detected<dynamodb_update_table_t, clean_t<_Type>>,
    is_detected<dynamodb_list_tables_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_table_management_v =
        has_dynamodb_table_management<clean_t<_Type>>::value;
#endif

// has_dynamodb_secondary_indexes
//   trait: checks if type _Type supports secondary index operations
// (create_gsi + delete_gsi + query_index).
template<typename _Type>
struct has_dynamodb_secondary_indexes : djinterp::conjunction<
    is_detected<dynamodb_create_gsi_t, clean_t<_Type>>,
    is_detected<dynamodb_delete_gsi_t, clean_t<_Type>>,
    is_detected<dynamodb_query_index_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_secondary_indexes_v =
        has_dynamodb_secondary_indexes<clean_t<_Type>>::value;
#endif

// has_dynamodb_streams
//   trait: checks if type _Type supports DynamoDB Streams
// (describe_stream + get_shard_iterator + get_records + list_streams).
template<typename _Type>
struct has_dynamodb_streams : djinterp::conjunction<
    is_detected<dynamodb_describe_stream_t, clean_t<_Type>>,
    is_detected<dynamodb_get_shard_iterator_t, clean_t<_Type>>,
    is_detected<dynamodb_get_records_t, clean_t<_Type>>,
    is_detected<dynamodb_list_streams_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_streams_v =
        has_dynamodb_streams<clean_t<_Type>>::value;
#endif

// has_dynamodb_ttl_management
//   trait: checks if type _Type supports TTL management
// (update_time_to_live + describe_time_to_live).
template<typename _Type>
struct has_dynamodb_ttl_management : djinterp::conjunction<
    is_detected<dynamodb_update_ttl_t, clean_t<_Type>>,
    is_detected<dynamodb_describe_ttl_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_ttl_management_v =
        has_dynamodb_ttl_management<clean_t<_Type>>::value;
#endif

// has_dynamodb_backup
//   trait: checks if type _Type supports backup / PITR operations
// (create_backup + restore_from_backup + describe_continuous_backups).
template<typename _Type>
struct has_dynamodb_backup : djinterp::conjunction<
    is_detected<dynamodb_create_backup_t, clean_t<_Type>>,
    is_detected<dynamodb_restore_from_backup_t, clean_t<_Type>>,
    is_detected<dynamodb_describe_continuous_backups_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_backup_v =
        has_dynamodb_backup<clean_t<_Type>>::value;
#endif

// has_dynamodb_diagnostics
//   trait: checks if type _Type supports diagnostics
// (describe_limits + describe_endpoints + table_status).
template<typename _Type>
struct has_dynamodb_diagnostics : djinterp::conjunction<
    is_detected<dynamodb_describe_limits_t, clean_t<_Type>>,
    is_detected<dynamodb_describe_endpoints_t, clean_t<_Type>>,
    is_detected<dynamodb_table_status_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_diagnostics_v =
        has_dynamodb_diagnostics<clean_t<_Type>>::value;
#endif

// has_dynamodb_global_tables
//   trait: checks if type _Type supports global table operations
// (create_global_table + describe_global_table + update_global_table).
template<typename _Type>
struct has_dynamodb_global_tables : djinterp::conjunction<
    is_detected<dynamodb_create_global_table_t, clean_t<_Type>>,
    is_detected<dynamodb_describe_global_table_t, clean_t<_Type>>,
    is_detected<dynamodb_update_global_table_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_global_tables_v =
        has_dynamodb_global_tables<clean_t<_Type>>::value;
#endif

// has_dynamodb_tagging
//   trait: checks if type _Type supports resource tagging
// (tag_resource + untag_resource + list_tags_of_resource).
template<typename _Type>
struct has_dynamodb_tagging : djinterp::conjunction<
    is_detected<dynamodb_tag_resource_t, clean_t<_Type>>,
    is_detected<dynamodb_untag_resource_t, clean_t<_Type>>,
    is_detected<dynamodb_list_tags_of_resource_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_dynamodb_tagging_v =
        has_dynamodb_tagging<clean_t<_Type>>::value;
#endif

// is_dynamodb_connection
//   trait: compound trait verifying type _Type implements a DynamoDB
// connection interface (item ops + query/scan + table management +
// diagnostics).
template<typename _Type>
struct is_dynamodb_connection : djinterp::conjunction<
    has_dynamodb_item_ops<clean_t<_Type>>,
    has_dynamodb_query_scan<clean_t<_Type>>,
    has_dynamodb_table_management<clean_t<_Type>>,
    has_dynamodb_diagnostics<clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_dynamodb_connection_v =
        is_dynamodb_connection<clean_t<_Type>>::value;
#endif


// =============================================================================
// XI. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// dynamodb_can_put_item
//   tagless trait: true if _Type has put_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_put_item = false;

template<typename _Type>
constexpr bool dynamodb_can_put_item<_Type,
    std::void_t<dynamodb_put_item_t<_Type>>> = true;

// dynamodb_can_get_item
//   tagless trait: true if _Type has get_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_get_item = false;

template<typename _Type>
constexpr bool dynamodb_can_get_item<_Type,
    std::void_t<dynamodb_get_item_t<_Type>>> = true;

// dynamodb_can_update_item
//   tagless trait: true if _Type has update_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_update_item = false;

template<typename _Type>
constexpr bool dynamodb_can_update_item<_Type,
    std::void_t<dynamodb_update_item_t<_Type>>> = true;

// dynamodb_can_delete_item
//   tagless trait: true if _Type has delete_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_delete_item = false;

template<typename _Type>
constexpr bool dynamodb_can_delete_item<_Type,
    std::void_t<dynamodb_delete_item_t<_Type>>> = true;

// dynamodb_can_batch_write
//   tagless trait: true if _Type has batch_write_item().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_batch_write = false;

template<typename _Type>
constexpr bool dynamodb_can_batch_write<_Type,
    std::void_t<dynamodb_batch_write_item_t<_Type>>> = true;

// dynamodb_can_query
//   tagless trait: true if _Type has query().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_query = false;

template<typename _Type>
constexpr bool dynamodb_can_query<_Type,
    std::void_t<dynamodb_query_t<_Type>>> = true;

// dynamodb_can_scan
//   tagless trait: true if _Type has scan().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_scan = false;

template<typename _Type>
constexpr bool dynamodb_can_scan<_Type,
    std::void_t<dynamodb_scan_t<_Type>>> = true;

// dynamodb_can_transact_write
//   tagless trait: true if _Type has transact_write_items().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_transact_write = false;

template<typename _Type>
constexpr bool dynamodb_can_transact_write<_Type,
    std::void_t<dynamodb_transact_write_items_t<_Type>>> = true;

// dynamodb_can_execute_statement
//   tagless trait: true if _Type has execute_statement().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_execute_statement = false;

template<typename _Type>
constexpr bool dynamodb_can_execute_statement<_Type,
    std::void_t<dynamodb_execute_statement_t<_Type>>> = true;

// dynamodb_can_put_conditional
//   tagless trait: true if _Type has put_item_conditional().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_put_conditional = false;

template<typename _Type>
constexpr bool dynamodb_can_put_conditional<_Type,
    std::void_t<dynamodb_put_item_conditional_t<_Type>>> = true;

// dynamodb_can_create_table
//   tagless trait: true if _Type has create_table().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_create_table = false;

template<typename _Type>
constexpr bool dynamodb_can_create_table<_Type,
    std::void_t<dynamodb_create_table_t<_Type>>> = true;

// dynamodb_can_query_index
//   tagless trait: true if _Type has query_index().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_query_index = false;

template<typename _Type>
constexpr bool dynamodb_can_query_index<_Type,
    std::void_t<dynamodb_query_index_t<_Type>>> = true;

// dynamodb_can_get_records
//   tagless trait: true if _Type has get_records().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_get_records = false;

template<typename _Type>
constexpr bool dynamodb_can_get_records<_Type,
    std::void_t<dynamodb_get_records_t<_Type>>> = true;

// dynamodb_can_update_ttl
//   tagless trait: true if _Type has update_time_to_live().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_update_ttl = false;

template<typename _Type>
constexpr bool dynamodb_can_update_ttl<_Type,
    std::void_t<dynamodb_update_ttl_t<_Type>>> = true;

// dynamodb_can_create_backup
//   tagless trait: true if _Type has create_backup().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_create_backup = false;

template<typename _Type>
constexpr bool dynamodb_can_create_backup<_Type,
    std::void_t<dynamodb_create_backup_t<_Type>>> = true;

// dynamodb_can_create_global_table
//   tagless trait: true if _Type has create_global_table().
template<typename _Type,
         typename = void>
constexpr bool dynamodb_can_create_global_table = false;

template<typename _Type>
constexpr bool dynamodb_can_create_global_table<_Type,
    std::void_t<dynamodb_create_global_table_t<_Type>>> = true;


// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// dynamodb_does_item_ops
//   tagless trait: true if _Type supports the full item-operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_item_ops = false;

template<typename _Type>
constexpr bool dynamodb_does_item_ops<_Type, std::void_t<
    dynamodb_put_item_t<_Type>,
    dynamodb_get_item_t<_Type>,
    dynamodb_update_item_t<_Type>,
    dynamodb_delete_item_t<_Type>>> = true;

// dynamodb_does_batch_ops
//   tagless trait: true if _Type supports the full batch-operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_batch_ops = false;

template<typename _Type>
constexpr bool dynamodb_does_batch_ops<_Type, std::void_t<
    dynamodb_batch_get_item_t<_Type>,
    dynamodb_batch_write_item_t<_Type>>> = true;

// dynamodb_does_query_scan
//   tagless trait: true if _Type supports the full query/scan surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_query_scan = false;

template<typename _Type>
constexpr bool dynamodb_does_query_scan<_Type, std::void_t<
    dynamodb_query_t<_Type>,
    dynamodb_scan_t<_Type>>> = true;

// dynamodb_does_transactions
//   tagless trait: true if _Type supports the full transaction surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_transactions = false;

template<typename _Type>
constexpr bool dynamodb_does_transactions<_Type, std::void_t<
    dynamodb_transact_write_items_t<_Type>,
    dynamodb_transact_get_items_t<_Type>>> = true;

// dynamodb_does_partiql
//   tagless trait: true if _Type supports the full PartiQL surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_partiql = false;

template<typename _Type>
constexpr bool dynamodb_does_partiql<_Type, std::void_t<
    dynamodb_execute_statement_t<_Type>,
    dynamodb_batch_execute_statement_t<_Type>,
    dynamodb_execute_transaction_t<_Type>>> = true;

// dynamodb_does_conditional_writes
//   tagless trait: true if _Type supports the full conditional-write
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_conditional_writes = false;

template<typename _Type>
constexpr bool dynamodb_does_conditional_writes<_Type, std::void_t<
    dynamodb_put_item_conditional_t<_Type>,
    dynamodb_delete_item_conditional_t<_Type>>> = true;

// dynamodb_does_table_management
//   tagless trait: true if _Type supports the full table-management
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_table_management = false;

template<typename _Type>
constexpr bool dynamodb_does_table_management<_Type, std::void_t<
    dynamodb_create_table_t<_Type>,
    dynamodb_delete_table_t<_Type>,
    dynamodb_describe_table_t<_Type>,
    dynamodb_update_table_t<_Type>,
    dynamodb_list_tables_t<_Type>>> = true;

// dynamodb_does_secondary_indexes
//   tagless trait: true if _Type supports the full secondary-index
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_secondary_indexes = false;

template<typename _Type>
constexpr bool dynamodb_does_secondary_indexes<_Type, std::void_t<
    dynamodb_create_gsi_t<_Type>,
    dynamodb_delete_gsi_t<_Type>,
    dynamodb_query_index_t<_Type>>> = true;

// dynamodb_does_streams
//   tagless trait: true if _Type supports the full Streams surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_streams = false;

template<typename _Type>
constexpr bool dynamodb_does_streams<_Type, std::void_t<
    dynamodb_describe_stream_t<_Type>,
    dynamodb_get_shard_iterator_t<_Type>,
    dynamodb_get_records_t<_Type>,
    dynamodb_list_streams_t<_Type>>> = true;

// dynamodb_does_ttl_management
//   tagless trait: true if _Type supports the full TTL-management
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_ttl_management = false;

template<typename _Type>
constexpr bool dynamodb_does_ttl_management<_Type, std::void_t<
    dynamodb_update_ttl_t<_Type>,
    dynamodb_describe_ttl_t<_Type>>> = true;

// dynamodb_does_backup
//   tagless trait: true if _Type supports the full backup/PITR surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_backup = false;

template<typename _Type>
constexpr bool dynamodb_does_backup<_Type, std::void_t<
    dynamodb_create_backup_t<_Type>,
    dynamodb_restore_from_backup_t<_Type>,
    dynamodb_describe_continuous_backups_t<_Type>>> = true;

// dynamodb_does_diagnostics
//   tagless trait: true if _Type supports the full diagnostics surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_diagnostics = false;

template<typename _Type>
constexpr bool dynamodb_does_diagnostics<_Type, std::void_t<
    dynamodb_describe_limits_t<_Type>,
    dynamodb_describe_endpoints_t<_Type>,
    dynamodb_table_status_t<_Type>>> = true;

// dynamodb_does_global_tables
//   tagless trait: true if _Type supports the full global-table surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_global_tables = false;

template<typename _Type>
constexpr bool dynamodb_does_global_tables<_Type, std::void_t<
    dynamodb_create_global_table_t<_Type>,
    dynamodb_describe_global_table_t<_Type>,
    dynamodb_update_global_table_t<_Type>>> = true;

// dynamodb_does_tagging
//   tagless trait: true if _Type supports the full resource-tagging
// surface.
template<typename _Type,
         typename = void>
constexpr bool dynamodb_does_tagging = false;

template<typename _Type>
constexpr bool dynamodb_does_tagging<_Type, std::void_t<
    dynamodb_tag_resource_t<_Type>,
    dynamodb_untag_resource_t<_Type>,
    dynamodb_list_tags_of_resource_t<_Type>>> = true;

// dynamodb_is_full_connection
//   tagless trait: true if _Type satisfies the complete DynamoDB
// connection interface (item ops + query/scan + table management +
// diagnostics + batch ops).
template<typename _Type>
constexpr bool dynamodb_is_full_connection =
    ( dynamodb_does_item_ops<clean_t<_Type>>         &&
      dynamodb_does_query_scan<clean_t<_Type>>       &&
      dynamodb_does_table_management<clean_t<_Type>> &&
      dynamodb_does_diagnostics<clean_t<_Type>>      &&
      dynamodb_does_batch_ops<clean_t<_Type>> );


// =============================================================================
// XII.  SFINAE HELPERS
// =============================================================================

// enable_if_dynamodb_connection
//   type: SFINAE helper for DynamoDB connection constraints.
template<typename _Type>
using enable_if_dynamodb_connection =
    typename std::enable_if<is_dynamodb_connection<clean_t<_Type>>::value>::type;

// enable_if_has_dynamodb_transactions
//   type: SFINAE helper for DynamoDB transaction constraints.
template<typename _Type>
using enable_if_has_dynamodb_transactions =
    typename std::enable_if<has_dynamodb_transactions<clean_t<_Type>>::value>::type;

// enable_if_has_dynamodb_partiql
//   type: SFINAE helper for DynamoDB PartiQL constraints.
template<typename _Type>
using enable_if_has_dynamodb_partiql =
    typename std::enable_if<has_dynamodb_partiql<clean_t<_Type>>::value>::type;

// enable_if_has_dynamodb_streams
//   type: SFINAE helper for DynamoDB Streams constraints.
template<typename _Type>
using enable_if_has_dynamodb_streams =
    typename std::enable_if<has_dynamodb_streams<clean_t<_Type>>::value>::type;

// enable_if_has_dynamodb_global_tables
//   type: SFINAE helper for DynamoDB global table constraints.
template<typename _Type>
using enable_if_has_dynamodb_global_tables =
    typename std::enable_if<has_dynamodb_global_tables<clean_t<_Type>>::value>::type;


// ===========================================================================
// XIII.   C++20 CONCEPTS
// ===========================================================================
//   The DynamoDB classification concepts, folded in from the former
// dynamodb_concepts.hpp.  Each forwards to a trait / tagless capability declared
// above.  Gated on concept support so the traits remain usable at the C++17
// baseline (matching functor.hpp / monoid.hpp).

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =============================================================================
// A.   Core DynamoDB Connection Concepts
// =============================================================================

// Dynamodb_connection
//   concept: constrains types implementing the DynamoDB connection
// interface. Suffixed with `_c` to avoid clashing with the
// `dynamodb_connection` class type.
template<typename _Type>
concept Dynamodb_connection =
    is_dynamodb_connection<clean_t<_Type>>::value;

// non_dynamodb_connection
//   concept: constrains types that do not implement the DynamoDB
// connection interface.
template<typename _Type>
concept non_dynamodb_connection =
    !Dynamodb_connection<_Type>;

// dynamodb_item_ops_connection
//   concept: constrains DynamoDB connections supporting core item
// operations (PutItem / GetItem / UpdateItem / DeleteItem).
template<typename _Type>
concept dynamodb_item_ops_connection =
    has_dynamodb_item_ops<clean_t<_Type>>::value;

// dynamodb_batch_connection
//   concept: constrains DynamoDB connections supporting batch
// operations (BatchGetItem / BatchWriteItem).
template<typename _Type>
concept dynamodb_batch_connection =
    has_dynamodb_batch_ops<clean_t<_Type>>::value;

// dynamodb_query_scan_connection
//   concept: constrains DynamoDB connections supporting query and
// scan (Query / Scan).
template<typename _Type>
concept dynamodb_query_scan_connection =
    has_dynamodb_query_scan<clean_t<_Type>>::value;

// dynamodb_transactional_connection
//   concept: constrains DynamoDB connections supporting transactions
// (TransactWriteItems / TransactGetItems).
template<typename _Type>
concept dynamodb_transactional_connection =
    has_dynamodb_transactions<clean_t<_Type>>::value;

// dynamodb_partiql_connection
//   concept: constrains DynamoDB connections supporting PartiQL
// (ExecuteStatement / BatchExecuteStatement / ExecuteTransaction).
template<typename _Type>
concept dynamodb_partiql_connection =
    has_dynamodb_partiql<clean_t<_Type>>::value;

// dynamodb_conditional_connection
//   concept: constrains DynamoDB connections supporting conditional
// writes (condition-expression PutItem / DeleteItem).
template<typename _Type>
concept dynamodb_conditional_connection =
    has_dynamodb_conditional_writes<clean_t<_Type>>::value;

// dynamodb_table_admin_connection
//   concept: constrains DynamoDB connections supporting table
// management (CreateTable / DeleteTable / DescribeTable / UpdateTable /
// ListTables).
template<typename _Type>
concept dynamodb_table_admin_connection =
    has_dynamodb_table_management<clean_t<_Type>>::value;

// dynamodb_indexable_connection
//   concept: constrains DynamoDB connections supporting secondary
// index operations.
template<typename _Type>
concept dynamodb_indexable_connection =
    has_dynamodb_secondary_indexes<clean_t<_Type>>::value;

// dynamodb_stream_connection
//   concept: constrains DynamoDB connections supporting DynamoDB
// Streams.
template<typename _Type>
concept dynamodb_stream_connection =
    has_dynamodb_streams<clean_t<_Type>>::value;

// dynamodb_ttl_connection
//   concept: constrains DynamoDB connections supporting TTL
// management.
template<typename _Type>
concept dynamodb_ttl_connection =
    has_dynamodb_ttl_management<clean_t<_Type>>::value;

// dynamodb_backup_connection
//   concept: constrains DynamoDB connections supporting backup /
// point-in-time recovery operations.
template<typename _Type>
concept dynamodb_backup_connection =
    has_dynamodb_backup<clean_t<_Type>>::value;

// dynamodb_diagnostics_connection
//   concept: constrains DynamoDB connections supporting diagnostics
// (DescribeLimits / DescribeEndpoints / table status).
template<typename _Type>
concept dynamodb_diagnostics_connection =
    has_dynamodb_diagnostics<clean_t<_Type>>::value;

// dynamodb_global_table_connection
//   concept: constrains DynamoDB connections supporting global table
// operations.
template<typename _Type>
concept dynamodb_global_table_connection =
    has_dynamodb_global_tables<clean_t<_Type>>::value;

// dynamodb_taggable_connection
//   concept: constrains DynamoDB connections supporting resource
// tagging.
template<typename _Type>
concept dynamodb_taggable_connection =
    has_dynamodb_tagging<clean_t<_Type>>::value;


// =============================================================================
// B.  DynamoDB Capability Concepts
// =============================================================================

// dynamodb_put_capable_connection
//   concept: constrains types exposing put_item(table, item).
template<typename _Type>
concept dynamodb_put_capable_connection =
    dynamodb_can_put_item<clean_t<_Type>>;

// dynamodb_get_capable_connection
//   concept: constrains types exposing get_item(table, key).
template<typename _Type>
concept dynamodb_get_capable_connection =
    dynamodb_can_get_item<clean_t<_Type>>;

// dynamodb_update_capable_connection
//   concept: constrains types exposing update_item(table, key, updates).
template<typename _Type>
concept dynamodb_update_capable_connection =
    dynamodb_can_update_item<clean_t<_Type>>;

// dynamodb_delete_capable_connection
//   concept: constrains types exposing delete_item(table, key).
template<typename _Type>
concept dynamodb_delete_capable_connection =
    dynamodb_can_delete_item<clean_t<_Type>>;

// dynamodb_batch_write_capable_connection
//   concept: constrains types exposing batch_write_item(table, items).
template<typename _Type>
concept dynamodb_batch_write_capable_connection =
    dynamodb_can_batch_write<clean_t<_Type>>;

// dynamodb_queryable_connection
//   concept: constrains types exposing query(table, key_condition).
template<typename _Type>
concept dynamodb_queryable_connection =
    dynamodb_can_query<clean_t<_Type>>;

// dynamodb_scannable_connection
//   concept: constrains types exposing scan(table).
template<typename _Type>
concept dynamodb_scannable_connection =
    dynamodb_can_scan<clean_t<_Type>>;

// dynamodb_transact_write_capable_connection
//   concept: constrains types exposing transact_write_items(items).
template<typename _Type>
concept dynamodb_transact_write_capable_connection =
    dynamodb_can_transact_write<clean_t<_Type>>;

// dynamodb_statement_capable_connection
//   concept: constrains types exposing execute_statement(statement).
template<typename _Type>
concept dynamodb_statement_capable_connection =
    dynamodb_can_execute_statement<clean_t<_Type>>;

// dynamodb_conditional_put_connection
//   concept: constrains types exposing put_item_conditional(...).
template<typename _Type>
concept dynamodb_conditional_put_connection =
    dynamodb_can_put_conditional<clean_t<_Type>>;

// dynamodb_table_creatable_connection
//   concept: constrains types exposing create_table(...).
template<typename _Type>
concept dynamodb_table_creatable_connection =
    dynamodb_can_create_table<clean_t<_Type>>;

// dynamodb_index_queryable_connection
//   concept: constrains types exposing query_index(table, index, cond).
template<typename _Type>
concept dynamodb_index_queryable_connection =
    dynamodb_can_query_index<clean_t<_Type>>;

// dynamodb_records_readable_connection
//   concept: constrains types exposing get_records(shard_iterator).
template<typename _Type>
concept dynamodb_records_readable_connection =
    dynamodb_can_get_records<clean_t<_Type>>;

// dynamodb_ttl_updatable_connection
//   concept: constrains types exposing update_time_to_live(...).
template<typename _Type>
concept dynamodb_ttl_updatable_connection =
    dynamodb_can_update_ttl<clean_t<_Type>>;

// dynamodb_backup_creatable_connection
//   concept: constrains types exposing create_backup(table, name).
template<typename _Type>
concept dynamodb_backup_creatable_connection =
    dynamodb_can_create_backup<clean_t<_Type>>;

// dynamodb_global_table_creatable_connection
//   concept: constrains types exposing create_global_table(table, regions).
template<typename _Type>
concept dynamodb_global_table_creatable_connection =
    dynamodb_can_create_global_table<clean_t<_Type>>;


// =============================================================================
// C. Tagless DynamoDB Capability Concepts
// =============================================================================

// dynamodb_item_addressable
//   concept: constrains types satisfying the full tagless item-
// operation capability set.
template<typename _Type>
concept dynamodb_item_addressable =
    dynamodb_does_item_ops<clean_t<_Type>>;

// dynamodb_batch_capable
//   concept: constrains types satisfying the full tagless batch-
// operation capability set.
template<typename _Type>
concept dynamodb_batch_capable =
    dynamodb_does_batch_ops<clean_t<_Type>>;

// dynamodb_query_scannable
//   concept: constrains types satisfying the full tagless query/scan
// capability set.
template<typename _Type>
concept dynamodb_query_scannable =
    dynamodb_does_query_scan<clean_t<_Type>>;

// dynamodb_transactional
//   concept: constrains types satisfying the full tagless transaction
// capability set.
template<typename _Type>
concept dynamodb_transactional =
    dynamodb_does_transactions<clean_t<_Type>>;

// dynamodb_partiql_capable
//   concept: constrains types satisfying the full tagless PartiQL
// capability set.
template<typename _Type>
concept dynamodb_partiql_capable =
    dynamodb_does_partiql<clean_t<_Type>>;

// dynamodb_conditional_capable
//   concept: constrains types satisfying the full tagless conditional-
// write capability set.
template<typename _Type>
concept dynamodb_conditional_capable =
    dynamodb_does_conditional_writes<clean_t<_Type>>;

// dynamodb_table_manageable
//   concept: constrains types satisfying the full tagless table-
// management capability set.
template<typename _Type>
concept dynamodb_table_manageable =
    dynamodb_does_table_management<clean_t<_Type>>;

// dynamodb_index_manageable
//   concept: constrains types satisfying the full tagless secondary-
// index capability set.
template<typename _Type>
concept dynamodb_index_manageable =
    dynamodb_does_secondary_indexes<clean_t<_Type>>;

// dynamodb_stream_capable
//   concept: constrains types satisfying the full tagless Streams
// capability set.
template<typename _Type>
concept dynamodb_stream_capable =
    dynamodb_does_streams<clean_t<_Type>>;

// dynamodb_ttl_manageable
//   concept: constrains types satisfying the full tagless TTL-
// management capability set.
template<typename _Type>
concept dynamodb_ttl_manageable =
    dynamodb_does_ttl_management<clean_t<_Type>>;

// dynamodb_backup_capable
//   concept: constrains types satisfying the full tagless backup/PITR
// capability set.
template<typename _Type>
concept dynamodb_backup_capable =
    dynamodb_does_backup<clean_t<_Type>>;

// dynamodb_diagnostic_capable
//   concept: constrains types satisfying the full tagless diagnostics
// capability set.
template<typename _Type>
concept dynamodb_diagnostic_capable =
    dynamodb_does_diagnostics<clean_t<_Type>>;

// dynamodb_global_table_capable
//   concept: constrains types satisfying the full tagless global-table
// capability set.
template<typename _Type>
concept dynamodb_global_table_capable =
    dynamodb_does_global_tables<clean_t<_Type>>;

// dynamodb_taggable
//   concept: constrains types satisfying the full tagless resource-
// tagging capability set.
template<typename _Type>
concept dynamodb_taggable =
    dynamodb_does_tagging<clean_t<_Type>>;

// dynamodb_full_connection
//   concept: constrains types satisfying the complete tagless
// DynamoDB connection capability set.
template<typename _Type>
concept dynamodb_full_connection =
    dynamodb_is_full_connection<clean_t<_Type>>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_DYNAMODB_
