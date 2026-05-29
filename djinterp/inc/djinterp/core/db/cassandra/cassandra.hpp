/******************************************************************************
* djinterp [database]                                            cassandra.hpp
*
* djinterp Apache Cassandra connection module:
*   This header provides the Cassandra-specific connection implementation
* and associated data type infrastructure for the djinterp database
* module, including:
*   - Cassandra native CQL data type enumeration (ascii, bigint, blob,
*     boolean, counter, date, decimal, double, duration, float, inet,
*     int, smallint, text/varchar, time, timestamp, timeuuid, tinyint,
*     uuid, varint, list, set, map, tuple, frozen, UDT, vector)
*   - cassandra_type-to-field_type mapping (one rung above the CQL
*     wire format)
*   - compile-time type and feature availability via D_ENV_CASSANDRA_*
*     macros covering protocol versions, LWT, UDTs, materialized views,
*     UDF/UDA, SASI indexes, virtual tables, audit logging, vector
*     search, authentication, and encryption
*   - Cassandra-specific connection configuration (contact points,
*     local data centre, default consistency, authentication
*     credentials, protocol version, compression, SSL/TLS, token-aware
*     routing, speculative execution)
*   - the concrete cassandra_connection CRTP leaf class with CQL
*     execution, asynchronous execution, batch operations, keyspace
*     and table management, data operations, lightweight transactions,
*     UDTs, materialized views, secondary indexes, consistency level
*     and tracing control, cluster topology, UDF/UDA, and paged
*     execution
*   - version-gated method declarations for materialized views (3.0+),
*     UDF/UDA (2.2+), virtual tables (4.0+), vector search (5.0+),
*     and audit logging (4.0+)
*
*   Cassandra is a wide-column / partitioned-row store with a SQL-like
* query language (CQL). Unlike SQL back-ends it has no JOIN support, no
* OFFSET clause (paging proceeds via an opaque paging state token), and
* its schema introspection lives under `system_schema.*` rather than
* `INFORMATION_SCHEMA`. The connection surface mirrors the underlying
* DataStax C driver (CassSession, CassStatement, CassFuture, CassResult)
* rather than any relational SQL connection idiom.
*
*   LAYER DIAGRAM:
*     cassandra_connection (this file)
*       -> database_connection<cassandra_connection,
*                              database_type::cassandra>
*         -> connection_template<cassandra_connection,
*                                database_type::cassandra>
*           -> connection<cassandra_connection>
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include the DataStax
* C driver header; the concrete _impl method definitions in cassandra.cpp
* include <cassandra.h>.
*
*
* path:      /inc/djinterp/core/db/cassandra/cassandra.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.28
******************************************************************************/

#ifndef DJINTERP_DATABASE_CASSANDRA_
#define DJINTERP_DATABASE_CASSANDRA_

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
#include "../../../env/db/cassandra/env_cassandra.h"
#include "../database_connection.hpp"
#include "./cassandra_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   CASSANDRA NATIVE CQL TYPE ENUMERATION
// =============================================================================
// Cassandra identifies every column value by a CQL native type. These are
// returned by the DataStax driver as CASS_VALUE_TYPE_* enumerators and are
// also recorded under system_schema.columns. The enumerators below mirror
// those wire types at the djinterp abstraction layer.

// cassandra_type
//   enumeration: native CQL data types.
enum class cassandra_type : std::uint16_t
{
    // -----------------------------------------------------------------
    // none / missing
    // -----------------------------------------------------------------
    none         = 0x00,

    // -----------------------------------------------------------------
    // scalar types
    // -----------------------------------------------------------------
    ascii        = 0x01,    // US-ASCII text
    bigint       = 0x02,    // 64-bit signed integer
    blob         = 0x03,    // arbitrary bytes
    boolean      = 0x04,    // true / false
    counter      = 0x05,    // distributed 64-bit counter
    date         = 0x06,    // date (no time)
    decimal      = 0x07,    // arbitrary-precision decimal
    double_      = 0x08,    // 64-bit IEEE-754
    duration     = 0x09,    // months / days / nanoseconds tuple
    float_       = 0x0A,    // 32-bit IEEE-754
    inet         = 0x0B,    // IPv4 or IPv6 address
    int_         = 0x0C,    // 32-bit signed integer
    smallint     = 0x0D,    // 16-bit signed integer
    text         = 0x0E,    // UTF-8 text (alias varchar)
    time         = 0x0F,    // time of day (no date)
    timestamp    = 0x10,    // milliseconds since UNIX epoch
    timeuuid     = 0x11,    // type-1 UUID
    tinyint      = 0x12,    // 8-bit signed integer
    uuid         = 0x13,    // type-4 UUID
    varint       = 0x14,    // arbitrary-precision integer

    // -----------------------------------------------------------------
    // collection types
    // -----------------------------------------------------------------
    list         = 0x20,    // ordered, possibly non-unique
    set          = 0x21,    // unordered, unique
    map          = 0x22,    // key -> value mapping
    tuple        = 0x23,    // heterogeneous fixed-size tuple
    frozen       = 0x24,    // immutable wrapper marker
    udt          = 0x25,    // user-defined type instance

    // -----------------------------------------------------------------
    // Cassandra 5.0+ vector search
    // -----------------------------------------------------------------
    vector       = 0x30,    // fixed-dimension float vector

    // -----------------------------------------------------------------
    // sentinel: not a recognised type
    // -----------------------------------------------------------------
    unknown      = 0xFF
};


// =============================================================================
// II.  CASSANDRA-TYPE-TO-FIELD_TYPE MAPPING
// =============================================================================

// cassandra_type_to_field_type
//   function: maps a Cassandra native type to the generic djinterp
// field_type. Composite types (list, set, map, tuple, udt, vector)
// have no single field-level mapping at this layer and resolve to
// field_type::array or field_type::custom.
inline field_type cassandra_type_to_field_type(cassandra_type _type)
    noexcept
{
    switch (_type)
    {
        case cassandra_type::none:
            return field_type::null;

        case cassandra_type::ascii:
        case cassandra_type::text:
            return field_type::string;

        case cassandra_type::tinyint:
        case cassandra_type::smallint:
        case cassandra_type::int_:
            return field_type::integer;

        case cassandra_type::bigint:
        case cassandra_type::counter:
        case cassandra_type::varint:
            return field_type::big_integer;

        case cassandra_type::float_:
        case cassandra_type::double_:
            return field_type::floating_point;

        case cassandra_type::decimal:
            return field_type::decimal;

        case cassandra_type::boolean:
            return field_type::boolean;

        case cassandra_type::blob:
            return field_type::binary;

        case cassandra_type::date:
            return field_type::date;

        case cassandra_type::time:
            return field_type::time;

        case cassandra_type::timestamp:
            return field_type::timestamp;

        case cassandra_type::uuid:
        case cassandra_type::timeuuid:
            return field_type::uuid;

        case cassandra_type::inet:
            // IPv4 / IPv6 addresses arrive textually after the driver
            // formats them; expose as string.
            return field_type::string;

        case cassandra_type::duration:
            // duration is a (months, days, nanos) triplet without a
            // single field-level analogue.
            return field_type::custom;

        case cassandra_type::list:
        case cassandra_type::set:
        case cassandra_type::vector:
            return field_type::array;

        case cassandra_type::map:
        case cassandra_type::tuple:
        case cassandra_type::frozen:
        case cassandra_type::udt:
            return field_type::custom;

        case cassandra_type::unknown:
        default:
            return field_type::custom;
    }
}

// cassandra_type_from_name
//   function: maps the CQL type name ("ascii", "bigint", ...) to
// cassandra_type. Collection types are recognised by their outer
// keyword only; the element types are not parsed here.
inline cassandra_type cassandra_type_from_name(const std::string& _name)
    noexcept
{
    if (_name == "ascii")
    {
        return cassandra_type::ascii;
    }

    if (_name == "bigint")
    {
        return cassandra_type::bigint;
    }

    if (_name == "blob")
    {
        return cassandra_type::blob;
    }

    if (_name == "boolean")
    {
        return cassandra_type::boolean;
    }

    if (_name == "counter")
    {
        return cassandra_type::counter;
    }

    if (_name == "date")
    {
        return cassandra_type::date;
    }

    if (_name == "decimal")
    {
        return cassandra_type::decimal;
    }

    if (_name == "double")
    {
        return cassandra_type::double_;
    }

    if (_name == "duration")
    {
        return cassandra_type::duration;
    }

    if (_name == "float")
    {
        return cassandra_type::float_;
    }

    if (_name == "inet")
    {
        return cassandra_type::inet;
    }

    if (_name == "int")
    {
        return cassandra_type::int_;
    }

    if (_name == "smallint")
    {
        return cassandra_type::smallint;
    }

    if ( (_name == "text") ||
         (_name == "varchar") )
    {
        return cassandra_type::text;
    }

    if (_name == "time")
    {
        return cassandra_type::time;
    }

    if (_name == "timestamp")
    {
        return cassandra_type::timestamp;
    }

    if (_name == "timeuuid")
    {
        return cassandra_type::timeuuid;
    }

    if (_name == "tinyint")
    {
        return cassandra_type::tinyint;
    }

    if (_name == "uuid")
    {
        return cassandra_type::uuid;
    }

    if (_name == "varint")
    {
        return cassandra_type::varint;
    }

    // collection types — match by leading keyword
    if (_name.rfind("list", 0) == 0)
    {
        return cassandra_type::list;
    }

    if (_name.rfind("set", 0) == 0)
    {
        return cassandra_type::set;
    }

    if (_name.rfind("map", 0) == 0)
    {
        return cassandra_type::map;
    }

    if (_name.rfind("tuple", 0) == 0)
    {
        return cassandra_type::tuple;
    }

    if (_name.rfind("frozen", 0) == 0)
    {
        return cassandra_type::frozen;
    }

    if (_name.rfind("vector", 0) == 0)
    {
        return cassandra_type::vector;
    }

    return cassandra_type::unknown;
}

// field_type_to_cassandra_native
//   function: returns the closest Cassandra native CQL type name for a
// given field_type. Used by the table layer when generating column
// definitions.
inline const char* field_type_to_cassandra_native(field_type _type)
    noexcept
{
    switch (_type)
    {
        case field_type::null:           return "text";
        case field_type::boolean:        return "boolean";
        case field_type::integer:        return "int";
        case field_type::big_integer:    return "bigint";
        case field_type::floating_point: return "double";
        case field_type::decimal:        return "decimal";
        case field_type::string:         return "text";
        case field_type::binary:         return "blob";
        case field_type::date:           return "date";
        case field_type::time:           return "time";
        case field_type::datetime:       return "timestamp";
        case field_type::timestamp:      return "timestamp";
        case field_type::json:           return "text";
        case field_type::xml:            return "text";
        case field_type::uuid:           return "uuid";
        case field_type::array:          return "list<text>";
        case field_type::custom:
        default:                         return "text";
    }
}


// =============================================================================
// III. FEATURE SUPPORT (compile-time, version-gated)
// =============================================================================

// cassandra_type_support
//   struct: compile-time native CQL type availability flags gated by
// D_ENV_CASSANDRA_* macros. Scalar / collection types have been stable
// since the 2.0 era; the more recent additions (duration since 3.10,
// vector since 5.0) are reported behind their own gates.
struct cassandra_type_support
{
#if D_ENV_CASSANDRA_DETECTED

    // core scalars (all supported releases)
    static constexpr bool has_ascii        = true;
    static constexpr bool has_bigint       = true;
    static constexpr bool has_blob         = true;
    static constexpr bool has_boolean      = true;
    static constexpr bool has_counter      = true;
    static constexpr bool has_decimal      = true;
    static constexpr bool has_double       = true;
    static constexpr bool has_float        = true;
    static constexpr bool has_inet         = true;
    static constexpr bool has_int          = true;
    static constexpr bool has_text         = true;
    static constexpr bool has_timestamp    = true;
    static constexpr bool has_uuid         = true;
    static constexpr bool has_timeuuid     = true;
    static constexpr bool has_varint       = true;

    // newer fixed-width integers (2.2+)
    static constexpr bool has_tinyint      = true;
    static constexpr bool has_smallint     = true;

    // calendar types (2.2+)
    static constexpr bool has_date         = true;
    static constexpr bool has_time         = true;

    // collections
    static constexpr bool has_list         = true;
    static constexpr bool has_set          = true;
    static constexpr bool has_map          = true;
    static constexpr bool has_tuple        = true;
    static constexpr bool has_frozen       = true;

    // duration (3.10+)
    static constexpr bool has_duration =
    #if D_ENV_CASSANDRA_HAS_DURATION
        true;
    #else
        false;
    #endif

    // user-defined types (2.1+)
    static constexpr bool has_udt =
    #if D_ENV_CASSANDRA_HAS_UDT
        true;
    #else
        false;
    #endif

    // vector (5.0+)
    static constexpr bool has_vector =
    #if D_ENV_CASSANDRA_HAS_VECTOR
        true;
    #else
        false;
    #endif

#else
    static constexpr bool has_ascii        = false;
    static constexpr bool has_bigint       = false;
    static constexpr bool has_blob         = false;
    static constexpr bool has_boolean      = false;
    static constexpr bool has_counter      = false;
    static constexpr bool has_decimal      = false;
    static constexpr bool has_double       = false;
    static constexpr bool has_float        = false;
    static constexpr bool has_inet         = false;
    static constexpr bool has_int          = false;
    static constexpr bool has_text         = false;
    static constexpr bool has_timestamp    = false;
    static constexpr bool has_uuid         = false;
    static constexpr bool has_timeuuid     = false;
    static constexpr bool has_varint       = false;
    static constexpr bool has_tinyint      = false;
    static constexpr bool has_smallint     = false;
    static constexpr bool has_date         = false;
    static constexpr bool has_time         = false;
    static constexpr bool has_list         = false;
    static constexpr bool has_set          = false;
    static constexpr bool has_map          = false;
    static constexpr bool has_tuple        = false;
    static constexpr bool has_frozen       = false;
    static constexpr bool has_duration     = false;
    static constexpr bool has_udt          = false;
    static constexpr bool has_vector       = false;
#endif  // D_ENV_CASSANDRA_DETECTED
};

// cassandra_feature_support
//   struct: compile-time server feature availability flags.
struct cassandra_feature_support
{
#if D_ENV_CASSANDRA_DETECTED

    // protocol versions (the driver negotiates the highest supported)
    static constexpr bool has_protocol_v3 =
    #if D_ENV_CASSANDRA_HAS_PROTOCOL_V3
        true;
    #else
        false;
    #endif

    static constexpr bool has_protocol_v4 =
    #if D_ENV_CASSANDRA_HAS_PROTOCOL_V4
        true;
    #else
        false;
    #endif

    static constexpr bool has_protocol_v5 =
    #if D_ENV_CASSANDRA_HAS_PROTOCOL_V5
        true;
    #else
        false;
    #endif

    // batch (always available; modes vary)
    static constexpr bool has_batch        = true;
    static constexpr bool has_batch_logged = true;
    static constexpr bool has_batch_counter =
    #if D_ENV_CASSANDRA_HAS_COUNTERS
        true;
    #else
        false;
    #endif

    // lightweight transactions / Paxos (2.0+)
    static constexpr bool has_lwt =
    #if D_ENV_CASSANDRA_HAS_LWT
        true;
    #else
        false;
    #endif

    // materialized views (3.0+, experimental in 4.0+)
    static constexpr bool has_materialized_views =
    #if D_ENV_CASSANDRA_HAS_MATERIALIZED_VIEWS
        true;
    #else
        false;
    #endif

    // SASI indexes (3.4+, experimental)
    static constexpr bool has_sasi_indexes =
    #if D_ENV_CASSANDRA_HAS_SASI_INDEXES
        true;
    #else
        false;
    #endif

    // user-defined functions (2.2+)
    static constexpr bool has_udf =
    #if D_ENV_CASSANDRA_HAS_UDF
        true;
    #else
        false;
    #endif

    // user-defined aggregates (2.2+)
    static constexpr bool has_uda =
    #if D_ENV_CASSANDRA_HAS_UDA
        true;
    #else
        false;
    #endif

    // virtual tables (4.0+)
    static constexpr bool has_virtual_tables =
    #if D_ENV_CASSANDRA_HAS_VIRTUAL_TABLES
        true;
    #else
        false;
    #endif

    // audit logging (4.0+)
    static constexpr bool has_audit_logging =
    #if D_ENV_CASSANDRA_HAS_AUDIT_LOGGING
        true;
    #else
        false;
    #endif

    // vector search (5.0+)
    static constexpr bool has_vector_search =
    #if D_ENV_CASSANDRA_HAS_VECTOR
        true;
    #else
        false;
    #endif

    // counters
    static constexpr bool has_counters =
    #if D_ENV_CASSANDRA_HAS_COUNTERS
        true;
    #else
        false;
    #endif

    // wire compression (LZ4 / Snappy)
    static constexpr bool has_compression =
    #if D_ENV_CASSANDRA_HAS_COMPRESSION
        true;
    #else
        false;
    #endif

    // authentication (PlainText / DSE / Kerberos)
    static constexpr bool has_authentication =
    #if D_ENV_CASSANDRA_HAS_AUTH
        true;
    #else
        false;
    #endif

    // role-based access control (2.2+)
    static constexpr bool has_rbac =
    #if D_ENV_CASSANDRA_HAS_RBAC
        true;
    #else
        false;
    #endif

    // SSL / TLS
    static constexpr bool has_tls =
    #if D_ENV_CASSANDRA_HAS_TLS
        true;
    #else
        false;
    #endif

    // token-aware routing (driver-side load balancing)
    static constexpr bool has_token_aware =
    #if D_ENV_CASSANDRA_HAS_TOKEN_AWARE
        true;
    #else
        false;
    #endif

    // speculative execution
    static constexpr bool has_speculative_execution =
    #if D_ENV_CASSANDRA_HAS_SPECULATIVE_EXECUTION
        true;
    #else
        false;
    #endif

    // paging (paging state cursor)
    static constexpr bool has_paging       = true;

    // request tracing
    static constexpr bool has_tracing      = true;

#else
    static constexpr bool has_protocol_v3            = false;
    static constexpr bool has_protocol_v4            = false;
    static constexpr bool has_protocol_v5            = false;
    static constexpr bool has_batch                  = false;
    static constexpr bool has_batch_logged           = false;
    static constexpr bool has_batch_counter          = false;
    static constexpr bool has_lwt                    = false;
    static constexpr bool has_materialized_views     = false;
    static constexpr bool has_sasi_indexes           = false;
    static constexpr bool has_udf                    = false;
    static constexpr bool has_uda                    = false;
    static constexpr bool has_virtual_tables         = false;
    static constexpr bool has_audit_logging          = false;
    static constexpr bool has_vector_search          = false;
    static constexpr bool has_counters               = false;
    static constexpr bool has_compression            = false;
    static constexpr bool has_authentication         = false;
    static constexpr bool has_rbac                   = false;
    static constexpr bool has_tls                    = false;
    static constexpr bool has_token_aware            = false;
    static constexpr bool has_speculative_execution  = false;
    static constexpr bool has_paging                 = false;
    static constexpr bool has_tracing                = false;
#endif  // D_ENV_CASSANDRA_DETECTED
};


// =============================================================================
// IV.  CASSANDRA VERSION INFORMATION
// =============================================================================

// cassandra_version_info
//   struct: compile-time version decomposition.
struct cassandra_version_info
{
#if D_ENV_CASSANDRA_DETECTED
    static constexpr bool          detected = true;
    static constexpr std::uint32_t id       = D_ENV_CASSANDRA_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_CASSANDRA_VERSION_MAJOR;
    static constexpr std::uint16_t minor    = D_ENV_CASSANDRA_VERSION_MINOR;
    static constexpr std::uint16_t patch    = D_ENV_CASSANDRA_VERSION_PATCH;
    static constexpr const char*   string   = D_ENV_CASSANDRA_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t minor    = 0;
    static constexpr std::uint16_t patch    = 0;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected Cassandra version is at
    // least (major, minor, patch).
    static constexpr bool at_least(std::uint16_t _major,
                                   std::uint16_t _minor,
                                   std::uint16_t _patch) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// V.   CASSANDRA MODE / CONSISTENCY ENUMERATIONS
// =============================================================================

// cassandra_consistency_level
//   enumeration: CQL request consistency levels.
enum class cassandra_consistency_level : std::uint8_t
{
    any           = 0x00,
    one           = 0x01,
    two           = 0x02,
    three         = 0x03,
    quorum        = 0x04,
    all           = 0x05,
    local_quorum  = 0x06,
    each_quorum   = 0x07,
    serial        = 0x08,
    local_serial  = 0x09,
    local_one     = 0x0A
};

// cassandra_batch_type
//   enumeration: CQL batch dispatch type.
enum class cassandra_batch_type : std::uint8_t
{
    logged   = 0,       // atomic group
    unlogged = 1,       // best-effort group
    counter  = 2        // counter-only batch
};

// cassandra_protocol_version
//   enumeration: native protocol version negotiated with the
// coordinator.
enum class cassandra_protocol_version : std::uint8_t
{
    auto_negotiate = 0,
    v3             = 3,
    v4             = 4,
    v5             = 5
};

// cassandra_compression
//   enumeration: wire-level compression algorithm.
enum class cassandra_compression : std::uint8_t
{
    none   = 0,
    lz4    = 1,
    snappy = 2
};


// =============================================================================
// VI.  CASSANDRA CONNECTION CONFIGURATION
// =============================================================================

// cassandra_connect_config
//   struct: Cassandra-specific connection configuration extending the
// generic connection_config with contact points, local data centre,
// default consistency, authentication, protocol version, compression,
// SSL/TLS, token-aware routing, and speculative execution.
struct cassandra_connect_config
{
    connection_config             base;

    // contact points — seed nodes the driver bootstraps against.
    std::vector<std::string>      contact_points;

    // local data centre (for DC-aware load balancing).
    std::string                   local_dc;

    // default keyspace to USE on session start.
    std::string                   default_keyspace;

    // default request consistency.
    cassandra_consistency_level   default_consistency;

    // default serial consistency for LWT.
    cassandra_consistency_level   default_serial_consistency;

    // native protocol version preference.
    cassandra_protocol_version    protocol_version;

    // wire compression.
    cassandra_compression         compression;

    // SSL/TLS — when enabled the driver verifies the peer using
    // base.ssl_ca / base.ssl_cert / base.ssl_key.
    bool                          enable_tls;

    // load balancing
    bool                          enable_token_aware_routing;
    bool                          enable_latency_aware_routing;

    // speculative execution (issues an extra request when the first is
    // slow).
    bool                          enable_speculative_execution;
    int                           speculative_max_executions;
    std::chrono::milliseconds     speculative_delay;

    // request paging default page size.
    int                           default_page_size;

    // request timeout (separate from connect_timeout in base).
    std::chrono::milliseconds     request_timeout;

    // connection pool tuning
    int                           core_connections_per_host;
    int                           max_connections_per_host;

    cassandra_connect_config()
        : default_consistency(cassandra_consistency_level::local_quorum)
        , default_serial_consistency(
              cassandra_consistency_level::local_serial)
        , protocol_version(cassandra_protocol_version::auto_negotiate)
        , compression(cassandra_compression::lz4)
        , enable_tls(false)
        , enable_token_aware_routing(true)
        , enable_latency_aware_routing(false)
        , enable_speculative_execution(false)
        , speculative_max_executions(2)
        , speculative_delay(std::chrono::milliseconds(500))
        , default_page_size(5000)
        , request_timeout(std::chrono::milliseconds(12000))
        , core_connections_per_host(1)
        , max_connections_per_host(2)
    {
        base.host = "localhost";
        base.port = 9042;
    }

    explicit cassandra_connect_config(const connection_config& _base)
        : base(_base)
        , default_consistency(cassandra_consistency_level::local_quorum)
        , default_serial_consistency(
              cassandra_consistency_level::local_serial)
        , protocol_version(cassandra_protocol_version::auto_negotiate)
        , compression(cassandra_compression::lz4)
        , enable_tls(false)
        , enable_token_aware_routing(true)
        , enable_latency_aware_routing(false)
        , enable_speculative_execution(false)
        , speculative_max_executions(2)
        , speculative_delay(std::chrono::milliseconds(500))
        , default_page_size(5000)
        , request_timeout(std::chrono::milliseconds(12000))
        , core_connections_per_host(1)
        , max_connections_per_host(2)
    {
        if (base.port == 0)
        {
            base.port = 9042;
        }
    }
};


// =============================================================================
// VII. CASSANDRA CONNECTION
// =============================================================================

// cassandra_connection
//   class: concrete Cassandra connection implementation via the DataStax
// C driver (or equivalent). This is the CRTP leaf class; _impl methods
// are defined in cassandra.cpp which includes <cassandra.h>.
//
// Usage:
//   cassandra_connection conn;
//   conn.connect(cassandra_connect_config{...});
//   conn.use_keyspace("app");
//   row r = { {"id", value{std::int64_t{1}}},
//             {"name", value{std::string{"teer"}}} };
//   conn.insert_row("app", "users", r);
//   auto rows = conn.select_rows("SELECT * FROM users WHERE id=1");
class cassandra_connection
    : public database_connection<cassandra_connection,
                                 database_type::cassandra>
{
public:
    using base_type       = database_connection<
        cassandra_connection, database_type::cassandra>;
    using type_support    = cassandra_type_support;
    using feature_support = cassandra_feature_support;
    using version_info    = cassandra_version_info;

    // paged_result
    //   type: one page of rows plus an opaque paging state cursor
    // (empty optional indicates the final page).
    using paged_result    = std::pair<result_rows,
                                      std::optional<std::string>>;

    cassandra_connection()
        : base_type()
    {
    }

    explicit cassandra_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit cassandra_connection(
            const cassandra_connect_config& _config
        )
        : base_type(_config.base),
          m_cassandra_config(_config)
    {
    }

    ~cassandra_connection() = default;

    // disable copying
    cassandra_connection(const cassandra_connection&)            = delete;
    cassandra_connection& operator=(const cassandra_connection&) = delete;

    // enable moving
    cassandra_connection(cassandra_connection&&) noexcept            = default;
    cassandra_connection& operator=(cassandra_connection&&) noexcept = default;


    // -----------------------------------------------------------------
    // CQL execution
    // -----------------------------------------------------------------

    // execute_cql
    //   function: executes a single CQL statement (DDL or DML) and
    // returns the result rows (empty for statements that produce none).
    result_rows execute_cql(const std::string& _cql)
    {
        this->ensure_connected();

        return self().execute_cql_impl(_cql);
    }

    // prepare_cql
    //   function: prepares a CQL statement and returns an opaque
    // identifier usable with execute_prepared().
    std::string prepare_cql(const std::string& _cql)
    {
        this->ensure_connected();

        return self().prepare_cql_impl(_cql);
    }

    // execute_prepared
    //   function: executes a previously prepared statement with named
    // bind parameters.
    result_rows execute_prepared(
        const std::string&    _query_id,
        const parameter_map&  _params)
    {
        this->ensure_connected();

        return self().execute_prepared_impl(_query_id, _params);
    }

    // ping
    //   function: issues a no-op query against system.local to verify
    // liveness; returns true on success.
    bool ping() const
    {
        return self().ping_impl();
    }


    // -----------------------------------------------------------------
    // asynchronous execution
    // -----------------------------------------------------------------

    // async_execute
    //   function: submits a statement asynchronously and returns a
    // future identifier the caller can resolve later.
    std::string async_execute(const std::string& _cql)
    {
        this->ensure_connected();

        return self().async_execute_impl(_cql);
    }

    // async_prepare
    //   function: submits a prepare asynchronously and returns a
    // future identifier the caller can resolve later.
    std::string async_prepare(const std::string& _cql)
    {
        this->ensure_connected();

        return self().async_prepare_impl(_cql);
    }


    // -----------------------------------------------------------------
    // batch operations
    // -----------------------------------------------------------------

    // batch_start
    //   function: begins a batch of the given type. _type is one of
    // cassandra_batch_type values cast to int.
    void batch_start(int _type)
    {
        this->ensure_connected();
        self().batch_start_impl(_type);

        return;
    }

    // batch_add
    //   function: appends a CQL statement to the pending batch.
    void batch_add(const std::string& _cql)
    {
        self().batch_add_impl(_cql);

        return;
    }

    // batch_execute
    //   function: dispatches the pending batch.
    result_rows batch_execute()
    {
        return self().batch_execute_impl();
    }

    // batch_discard
    //   function: drops the pending batch without dispatching.
    void batch_discard()
    {
        self().batch_discard_impl();

        return;
    }


    // -----------------------------------------------------------------
    // keyspace management
    // -----------------------------------------------------------------

    // create_keyspace
    //   function: CREATE KEYSPACE.
    bool create_keyspace(const std::string& _name,
                         const std::string& _replication_spec)
    {
        this->ensure_connected();

        return self().create_keyspace_impl(_name, _replication_spec);
    }

    // drop_keyspace
    //   function: DROP KEYSPACE.
    bool drop_keyspace(const std::string& _name)
    {
        this->ensure_connected();

        return self().drop_keyspace_impl(_name);
    }

    // use_keyspace
    //   function: USE <keyspace> — sets the session's default
    // keyspace.
    void use_keyspace(const std::string& _name)
    {
        this->ensure_connected();
        self().use_keyspace_impl(_name);

        return;
    }

    // list_keyspaces
    //   function: queries system_schema.keyspaces.
    std::vector<std::string> list_keyspaces() const
    {
        return self().list_keyspaces_impl();
    }


    // -----------------------------------------------------------------
    // table management
    // -----------------------------------------------------------------

    // create_table
    //   function: CREATE TABLE. _schema_spec is a CQL column-definition
    // fragment.
    bool create_table(const std::string& _keyspace,
                      const std::string& _table,
                      const std::string& _schema_spec)
    {
        this->ensure_connected();

        return self().create_table_impl(_keyspace,
                                        _table,
                                        _schema_spec);
    }

    // drop_table
    //   function: DROP TABLE.
    bool drop_table(const std::string& _keyspace,
                    const std::string& _table)
    {
        this->ensure_connected();

        return self().drop_table_impl(_keyspace, _table);
    }

    // alter_table
    //   function: ALTER TABLE.
    bool alter_table(const std::string& _keyspace,
                     const std::string& _table,
                     const std::string& _alter_spec)
    {
        this->ensure_connected();

        return self().alter_table_impl(_keyspace, _table, _alter_spec);
    }

    // describe_table
    //   function: queries system_schema.tables / system_schema.columns
    // for the table description block.
    std::string describe_table(const std::string& _keyspace,
                               const std::string& _table) const
    {
        return self().describe_table_impl(_keyspace, _table);
    }

    // list_tables
    //   function: queries system_schema.tables for the given keyspace.
    std::vector<std::string> list_tables(
        const std::string& _keyspace) const
    {
        return self().list_tables_impl(_keyspace);
    }


    // -----------------------------------------------------------------
    // data operations (CRUD)
    // -----------------------------------------------------------------

    // insert_row
    //   function: builds and executes an INSERT INTO from a row map.
    bool insert_row(const std::string& _keyspace,
                    const std::string& _table,
                    const row&         _row)
    {
        this->ensure_connected();

        return self().insert_row_impl(_keyspace, _table, _row);
    }

    // select_rows
    //   function: executes a SELECT and returns the result rows.
    result_rows select_rows(const std::string& _cql) const
    {
        return self().select_rows_impl(_cql);
    }

    // update_row
    //   function: builds and executes an UPDATE. _key carries the
    // primary key columns; _updates carries the non-key columns to
    // mutate.
    bool update_row(const std::string& _keyspace,
                    const std::string& _table,
                    const row&         _key,
                    const row&         _updates)
    {
        this->ensure_connected();

        return self().update_row_impl(_keyspace,
                                      _table,
                                      _key,
                                      _updates);
    }

    // delete_row
    //   function: builds and executes a DELETE.
    bool delete_row(const std::string& _keyspace,
                    const std::string& _table,
                    const row&         _key)
    {
        this->ensure_connected();

        return self().delete_row_impl(_keyspace, _table, _key);
    }

    // row_exists
    //   function: returns true if a row matching the given key exists.
    bool row_exists(const std::string& _keyspace,
                    const std::string& _table,
                    const row&         _key) const
    {
        return self().row_exists_impl(_keyspace, _table, _key);
    }


    // -----------------------------------------------------------------
    // lightweight transactions (LWT / Paxos)
    // -----------------------------------------------------------------

    // insert_if_not_exists
    //   function: INSERT ... IF NOT EXISTS. Returns false when an
    // existing row blocks the insert.
    bool insert_if_not_exists(const std::string& _keyspace,
                              const std::string& _table,
                              const row&         _row)
    {
        this->ensure_connected();

        return self().insert_if_not_exists_impl(_keyspace,
                                                _table,
                                                _row);
    }

    // update_if
    //   function: UPDATE ... IF <condition>. Returns false when the
    // condition fails.
    bool update_if(const std::string& _keyspace,
                   const std::string& _table,
                   const row&         _key,
                   const row&         _updates,
                   const std::string& _condition)
    {
        this->ensure_connected();

        return self().update_if_impl(_keyspace,
                                     _table,
                                     _key,
                                     _updates,
                                     _condition);
    }

    // delete_if
    //   function: DELETE ... IF <condition>. Returns false when the
    // condition fails.
    bool delete_if(const std::string& _keyspace,
                   const std::string& _table,
                   const row&         _key,
                   const std::string& _condition)
    {
        this->ensure_connected();

        return self().delete_if_impl(_keyspace,
                                     _table,
                                     _key,
                                     _condition);
    }


    // -----------------------------------------------------------------
    // user-defined types (UDT)
    // -----------------------------------------------------------------

    // create_type
    //   function: CREATE TYPE.
    bool create_type(const std::string& _keyspace,
                     const std::string& _type_def)
    {
        this->ensure_connected();

        return self().create_type_impl(_keyspace, _type_def);
    }

    // drop_type
    //   function: DROP TYPE.
    bool drop_type(const std::string& _keyspace,
                   const std::string& _type_name)
    {
        this->ensure_connected();

        return self().drop_type_impl(_keyspace, _type_name);
    }

    // alter_type
    //   function: ALTER TYPE.
    bool alter_type(const std::string& _keyspace,
                    const std::string& _type_name,
                    const std::string& _alter_spec)
    {
        this->ensure_connected();

        return self().alter_type_impl(_keyspace,
                                      _type_name,
                                      _alter_spec);
    }


    // -----------------------------------------------------------------
    // materialized views
    // -----------------------------------------------------------------

    // create_materialized_view
    //   function: CREATE MATERIALIZED VIEW.
    bool create_materialized_view(const std::string& _keyspace,
                                  const std::string& _view,
                                  const std::string& _base_table,
                                  const std::string& _definition)
    {
        this->ensure_connected();

        return self().create_materialized_view_impl(_keyspace,
                                                    _view,
                                                    _base_table,
                                                    _definition);
    }

    // drop_materialized_view
    //   function: DROP MATERIALIZED VIEW.
    bool drop_materialized_view(const std::string& _keyspace,
                                const std::string& _view)
    {
        this->ensure_connected();

        return self().drop_materialized_view_impl(_keyspace, _view);
    }


    // -----------------------------------------------------------------
    // secondary indexes
    // -----------------------------------------------------------------

    // create_index
    //   function: CREATE INDEX.
    bool create_index(const std::string& _keyspace,
                      const std::string& _table,
                      const std::string& _index_name,
                      const std::string& _column)
    {
        this->ensure_connected();

        return self().create_index_impl(_keyspace,
                                        _table,
                                        _index_name,
                                        _column);
    }

    // drop_index
    //   function: DROP INDEX.
    bool drop_index(const std::string& _keyspace,
                    const std::string& _index_name)
    {
        this->ensure_connected();

        return self().drop_index_impl(_keyspace, _index_name);
    }


    // -----------------------------------------------------------------
    // consistency level and tracing
    // -----------------------------------------------------------------

    // set_consistency
    //   function: sets the request consistency level for subsequent
    // statements. _level is one of cassandra_consistency_level values
    // cast to int.
    void set_consistency(int _level)
    {
        self().set_consistency_impl(_level);

        return;
    }

    // get_consistency
    //   function: returns the current request consistency level.
    int get_consistency() const
    {
        return self().get_consistency_impl();
    }

    // set_tracing
    //   function: enables / disables tracing for subsequent statements.
    void set_tracing(bool _enabled)
    {
        self().set_tracing_impl(_enabled);

        return;
    }

    // get_last_trace
    //   function: returns the trace events recorded for the most recent
    // traced statement.
    std::string get_last_trace() const
    {
        return self().get_last_trace_impl();
    }


    // -----------------------------------------------------------------
    // cluster topology
    // -----------------------------------------------------------------

    // cluster_name
    //   function: returns the cluster name (system.local::cluster_name).
    std::string cluster_name() const
    {
        return self().cluster_name_impl();
    }

    // partitioner
    //   function: returns the partitioner (Murmur3Partitioner by
    // default).
    std::string partitioner() const
    {
        return self().partitioner_impl();
    }

    // local_node
    //   function: returns the coordinator node row from system.local.
    row local_node() const
    {
        return self().local_node_impl();
    }

    // peers
    //   function: returns the peer rows from system.peers.
    result_rows peers() const
    {
        return self().peers_impl();
    }


    // -----------------------------------------------------------------
    // user-defined functions and aggregates (UDF / UDA)
    // -----------------------------------------------------------------

    // create_function
    //   function: CREATE FUNCTION.
    bool create_function(const std::string& _keyspace,
                         const std::string& _function_def)
    {
        this->ensure_connected();

        return self().create_function_impl(_keyspace, _function_def);
    }

    // drop_function
    //   function: DROP FUNCTION.
    bool drop_function(const std::string& _keyspace,
                       const std::string& _function_name)
    {
        this->ensure_connected();

        return self().drop_function_impl(_keyspace, _function_name);
    }

    // create_aggregate
    //   function: CREATE AGGREGATE.
    bool create_aggregate(const std::string& _keyspace,
                          const std::string& _aggregate_def)
    {
        this->ensure_connected();

        return self().create_aggregate_impl(_keyspace,
                                            _aggregate_def);
    }

    // drop_aggregate
    //   function: DROP AGGREGATE.
    bool drop_aggregate(const std::string& _keyspace,
                        const std::string& _aggregate_name)
    {
        this->ensure_connected();

        return self().drop_aggregate_impl(_keyspace, _aggregate_name);
    }


    // -----------------------------------------------------------------
    // paging
    // -----------------------------------------------------------------

    // execute_paged
    //   function: executes a SELECT with a paging-size hint; returns
    // the first page plus an opaque paging state continuation.
    paged_result execute_paged(const std::string& _cql,
                               int                _page_size) const
    {
        return self().execute_paged_impl(_cql, _page_size);
    }

    // fetch_next_page
    //   function: continues a paged result by submitting a previously
    // returned paging state.
    paged_result fetch_next_page(const std::string& _paging_state)
    {
        this->ensure_connected();

        return self().fetch_next_page_impl(_paging_state);
    }


    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_lwt() noexcept
    {
        return feature_support::has_lwt;
    }

    static constexpr bool supports_materialized_views() noexcept
    {
        return feature_support::has_materialized_views;
    }

    static constexpr bool supports_udf() noexcept
    {
        return feature_support::has_udf;
    }

    static constexpr bool supports_uda() noexcept
    {
        return feature_support::has_uda;
    }

    static constexpr bool supports_udt() noexcept
    {
        return type_support::has_udt;
    }

    static constexpr bool supports_virtual_tables() noexcept
    {
        return feature_support::has_virtual_tables;
    }

    static constexpr bool supports_vector_search() noexcept
    {
        return feature_support::has_vector_search;
    }

    static constexpr bool supports_audit_logging() noexcept
    {
        return feature_support::has_audit_logging;
    }

    static constexpr bool supports_compression() noexcept
    {
        return feature_support::has_compression;
    }

    static constexpr bool supports_authentication() noexcept
    {
        return feature_support::has_authentication;
    }

    static constexpr bool supports_tls() noexcept
    {
        return feature_support::has_tls;
    }

    static constexpr bool supports_protocol_v5() noexcept
    {
        return feature_support::has_protocol_v5;
    }


    // -----------------------------------------------------------------
    // data type mapping
    // -----------------------------------------------------------------

    static field_type map_type(cassandra_type _type) noexcept
    {
        return cassandra_type_to_field_type(_type);
    }

    static cassandra_type type_from_name(
        const std::string& _name) noexcept
    {
        return cassandra_type_from_name(_name);
    }

    static const char* native_type_name(field_type _type) noexcept
    {
        return field_type_to_cassandra_native(_type);
    }


    // -----------------------------------------------------------------
    // Cassandra-specific configuration
    // -----------------------------------------------------------------

    // get_cassandra_config
    //   function: returns the Cassandra-specific configuration.
    const cassandra_connect_config& get_cassandra_config() const noexcept
    {
        return m_cassandra_config;
    }

    // set_cassandra_config
    //   function: replaces the Cassandra-specific configuration. Must
    // be called before connect().
    void set_cassandra_config(const cassandra_connect_config& _config)
    {
        m_cassandra_config = _config;
        this->m_config     = _config.base;
    }


    // -----------------------------------------------------------------
    // _impl methods (defined in cassandra.cpp)
    // -----------------------------------------------------------------

    void         connect_impl();
    void         disconnect_impl();
    bool         is_connected_impl() const;
    bool         ping_impl() const;
    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;

    // CQL execution
    result_rows  execute_cql_impl(const std::string& _cql);
    std::string  prepare_cql_impl(const std::string& _cql);
    result_rows  execute_prepared_impl(
                     const std::string&   _query_id,
                     const parameter_map& _params);

    // async
    std::string  async_execute_impl(const std::string& _cql);
    std::string  async_prepare_impl(const std::string& _cql);

    // batch
    void         batch_start_impl(int _type);
    void         batch_add_impl(const std::string& _cql);
    result_rows  batch_execute_impl();
    void         batch_discard_impl();

    // keyspace
    bool         create_keyspace_impl(
                     const std::string& _name,
                     const std::string& _replication_spec);
    bool         drop_keyspace_impl(const std::string& _name);
    void         use_keyspace_impl(const std::string& _name);
    std::vector<std::string>
                 list_keyspaces_impl() const;

    // table
    bool         create_table_impl(const std::string& _keyspace,
                                   const std::string& _table,
                                   const std::string& _schema_spec);
    bool         drop_table_impl(const std::string& _keyspace,
                                 const std::string& _table);
    bool         alter_table_impl(const std::string& _keyspace,
                                  const std::string& _table,
                                  const std::string& _alter_spec);
    std::string  describe_table_impl(const std::string& _keyspace,
                                     const std::string& _table) const;
    std::vector<std::string>
                 list_tables_impl(const std::string& _keyspace) const;

    // CRUD
    bool         insert_row_impl(const std::string& _keyspace,
                                 const std::string& _table,
                                 const row&         _row);
    result_rows  select_rows_impl(const std::string& _cql) const;
    bool         update_row_impl(const std::string& _keyspace,
                                 const std::string& _table,
                                 const row&         _key,
                                 const row&         _updates);
    bool         delete_row_impl(const std::string& _keyspace,
                                 const std::string& _table,
                                 const row&         _key);
    bool         row_exists_impl(const std::string& _keyspace,
                                 const std::string& _table,
                                 const row&         _key) const;

    // LWT
    bool         insert_if_not_exists_impl(
                     const std::string& _keyspace,
                     const std::string& _table,
                     const row&         _row);
    bool         update_if_impl(const std::string& _keyspace,
                                const std::string& _table,
                                const row&         _key,
                                const row&         _updates,
                                const std::string& _condition);
    bool         delete_if_impl(const std::string& _keyspace,
                                const std::string& _table,
                                const row&         _key,
                                const std::string& _condition);

    // UDT
    bool         create_type_impl(const std::string& _keyspace,
                                  const std::string& _type_def);
    bool         drop_type_impl(const std::string& _keyspace,
                                const std::string& _type_name);
    bool         alter_type_impl(const std::string& _keyspace,
                                 const std::string& _type_name,
                                 const std::string& _alter_spec);

    // materialized views
    bool         create_materialized_view_impl(
                     const std::string& _keyspace,
                     const std::string& _view,
                     const std::string& _base_table,
                     const std::string& _definition);
    bool         drop_materialized_view_impl(
                     const std::string& _keyspace,
                     const std::string& _view);

    // secondary indexes
    bool         create_index_impl(const std::string& _keyspace,
                                   const std::string& _table,
                                   const std::string& _index_name,
                                   const std::string& _column);
    bool         drop_index_impl(const std::string& _keyspace,
                                 const std::string& _index_name);

    // consistency / tracing
    void         set_consistency_impl(int _level);
    int          get_consistency_impl() const;
    void         set_tracing_impl(bool _enabled);
    std::string  get_last_trace_impl() const;

    // topology
    std::string  cluster_name_impl() const;
    std::string  partitioner_impl() const;
    row          local_node_impl() const;
    result_rows  peers_impl() const;

    // UDF / UDA
    bool         create_function_impl(const std::string& _keyspace,
                                      const std::string& _function_def);
    bool         drop_function_impl(const std::string& _keyspace,
                                    const std::string& _function_name);
    bool         create_aggregate_impl(
                     const std::string& _keyspace,
                     const std::string& _aggregate_def);
    bool         drop_aggregate_impl(
                     const std::string& _keyspace,
                     const std::string& _aggregate_name);

    // paging
    paged_result execute_paged_impl(const std::string& _cql,
                                    int                _page_size) const;
    paged_result fetch_next_page_impl(
                     const std::string& _paging_state);


    // -----------------------------------------------------------------
    // version-gated methods
    // -----------------------------------------------------------------

#if D_ENV_CASSANDRA_DETECTED

    #if D_ENV_CASSANDRA_HAS_MATERIALIZED_VIEWS
    // list_materialized_views
    //   function: queries system_schema.views. Available since
    // Cassandra 3.0.
    std::vector<std::string> list_materialized_views(
        const std::string& _keyspace) const;
    #endif

    #if D_ENV_CASSANDRA_HAS_VIRTUAL_TABLES
    // query_virtual_table
    //   function: queries a virtual table from system_views /
    // system_virtual_schema. Available since Cassandra 4.0.
    result_rows query_virtual_table(
        const std::string& _virtual_keyspace,
        const std::string& _virtual_table) const;
    #endif

    #if D_ENV_CASSANDRA_HAS_VECTOR
    // ann_search
    //   function: vector approximate-nearest-neighbour search via the
    // ORDER BY <vector_col> ANN OF ... clause. Available since
    // Cassandra 5.0.
    result_rows ann_search(const std::string&        _keyspace,
                           const std::string&        _table,
                           const std::string&        _column,
                           const std::vector<float>& _query_vector,
                           int                       _limit) const;
    #endif

    #if D_ENV_CASSANDRA_HAS_AUDIT_LOGGING
    // configure_audit_logging
    //   function: ALTER SYSTEM ... AUDIT — runtime audit-logging
    // configuration. Available since Cassandra 4.0.
    bool configure_audit_logging(const std::string& _spec);
    #endif

#endif  // D_ENV_CASSANDRA_DETECTED


private:
    cassandra_connect_config m_cassandra_config;

    cassandra_connection& self()
    {
        return *this;
    }

    const cassandra_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// cassandra_result_set_impl
//   struct: forward declaration of the Cassandra result set
// implementation (wraps CassResult / CassIterator from the DataStax
// driver).
struct cassandra_result_set_impl;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_CASSANDRA_
