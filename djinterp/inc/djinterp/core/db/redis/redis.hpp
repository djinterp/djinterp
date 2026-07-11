/******************************************************************************
* djinterp [database]                                                redis.hpp
*
* djinterp Redis connection module:
*   This header provides the Redis-specific connection implementation and
* associated data type infrastructure for the djinterp database module,
* including:
*   - Redis native data type enumeration (string, list, hash, set, zset,
*     stream, plus extended module types: ReJSON, RediSearch index,
*     RedisGraph, RedisTimeSeries, RedisBloom)
*   - redis_type-to-field_type mapping (one rung above wire protocol)
*   - compile-time type and feature availability via D_ENV_REDIS_* macros
*     covering data structures, scripting, pub/sub, cluster, replication,
*     persistence, RESP versions, modules, and security
*   - Redis-specific connection configuration (TLS, ACL user, database
*     number, sentinel master name, cluster discovery, RESP3 negotiation)
*   - the concrete redis_connection CRTP leaf class with command dispatch,
*     PUB/SUB, transactions, pipelining, Lua scripting, key/hash/list/set/
*     sorted-set/stream operations, server diagnostics, cluster ops, and
*     persistence commands
*   - version-gated method declarations for ACL (6+), client tracking
*     (6+), RESP3 (6+), and streams (5+)
*
*   Redis is a key-value store, not a relational database. There is no
* SQL parser, no joins, no schema-level introspection. Every operation
* is a one-shot RESP command against a flat key-space with per-type
* semantics. The connection surface mirrors the underlying C client API
* (hiredis or equivalent) rather than any SQL connection idiom.
*
*   LAYER DIAGRAM:
*     redis_connection (this file)
*       -> database_connection<redis_connection, database_type::redis>
*         -> connection_template<redis_connection, database_type::redis>
*           -> connection<redis_connection>
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include <hiredis.h>;
* the concrete _impl method definitions in redis.cpp include it.
*
*
*   DETECTION:
*   Also carries this database's capability-detection traits and C++20 concepts
* (trailing sections), folded in from redis_traits.hpp and the matching *_concepts.hpp;
* detection now lives with the connection. Concepts gated on concept support.
*
* path:      /inc/djinterp/core/db/redis/redis.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.27
******************************************************************************/

#ifndef DJINTERP_DATABASE_REDIS_
#define DJINTERP_DATABASE_REDIS_

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
#include "../../../env/db/redis/env_redis.h"
#include "../database_connection.hpp"
#include "../database_traits.hpp"


NS_DJINTERP


// =============================================================================
// I.   REDIS DATA TYPE ENUMERATION
// =============================================================================
// Redis identifies stored values by a small set of canonical types. These
// are returned by the TYPE <key> command. Module-provided types (ReJSON,
// search index, graph, ts) extend this set at runtime through the
// modules subsystem; their string names are recognised here but do not
// have stable numeric identifiers in the way SQL OIDs do.

// redis_type
//   enumeration: canonical Redis value types as returned by TYPE.
enum class redis_type : std::uint16_t
{
    // -----------------------------------------------------------------
    // none / missing
    // -----------------------------------------------------------------
    none           = 0x00,

    // -----------------------------------------------------------------
    // built-in core types
    // -----------------------------------------------------------------
    string         = 0x01,      // Strings (binary-safe up to 512 MB)
    list           = 0x02,      // Lists  (linked list of strings)
    hash           = 0x03,      // Hashes (string -> string maps)
    set            = 0x04,      // Sets   (unordered string sets)
    zset           = 0x05,      // Sorted sets (member + score)
    stream         = 0x06,      // Streams (Redis 5.0+)

    // -----------------------------------------------------------------
    // module-provided types (well-known)
    // -----------------------------------------------------------------
    rejson         = 0x10,      // ReJSON / RedisJSON
    rsearch_index  = 0x11,      // RediSearch index
    rgraph         = 0x12,      // RedisGraph graph
    rts            = 0x13,      // RedisTimeSeries series
    rbloom         = 0x14,      // RedisBloom filter

    // -----------------------------------------------------------------
    // sentinel: not a built-in or known module type
    // -----------------------------------------------------------------
    unknown        = 0xFF
};


// =============================================================================
// II.  REDIS-TYPE-TO-FIELD_TYPE MAPPING
// =============================================================================

// redis_type_to_field_type
//   function: maps a Redis native type to the generic djinterp
// field_type. Most Redis values are binary strings; aggregate types
// map to field_type::custom since their structure is opaque at this
// abstraction layer.
inline field_type redis_type_to_field_type(redis_type _type) noexcept
{
    switch (_type)
    {
        case redis_type::none:
            return field_type::null;

        case redis_type::string:
            // Redis strings are binary-safe blobs; expose as string
            // by default. Callers that know the contents are binary
            // can request field_type::binary explicitly.
            return field_type::string;

        case redis_type::rejson:
            // ReJSON values are JSON documents.
            return field_type::json;

        case redis_type::list:
        case redis_type::hash:
        case redis_type::set:
        case redis_type::zset:
        case redis_type::stream:
        case redis_type::rsearch_index:
        case redis_type::rgraph:
        case redis_type::rts:
        case redis_type::rbloom:
            // aggregate / module types have no single field-level
            // mapping; expose as custom.
            return field_type::custom;

        case redis_type::unknown:
        default:
            return field_type::custom;
    }
}

// redis_type_from_name
//   function: maps the TYPE command string ("string", "list", "hash",
// "set", "zset", "stream", "ReJSON-RL", ...) to redis_type.
inline redis_type redis_type_from_name(const std::string& _name)
    noexcept
{
    if (_name == "none")
    {
        return redis_type::none;
    }

    if (_name == "string")
    {
        return redis_type::string;
    }

    if (_name == "list")
    {
        return redis_type::list;
    }

    if (_name == "hash")
    {
        return redis_type::hash;
    }

    if (_name == "set")
    {
        return redis_type::set;
    }

    if (_name == "zset")
    {
        return redis_type::zset;
    }

    if (_name == "stream")
    {
        return redis_type::stream;
    }

    // well-known module types
    if (_name == "ReJSON-RL")
    {
        return redis_type::rejson;
    }

    if (_name == "ft_index0")
    {
        return redis_type::rsearch_index;
    }

    if (_name == "graphdata")
    {
        return redis_type::rgraph;
    }

    if (_name == "TSDB-TYPE")
    {
        return redis_type::rts;
    }

    if (_name == "MBbloom--")
    {
        return redis_type::rbloom;
    }

    return redis_type::unknown;
}

// field_type_to_redis_native
//   function: returns the closest Redis native data structure for
// a given field_type. Used by the table layer when laying out
// columns in hashes.
inline const char* field_type_to_redis_native(field_type _type)
    noexcept
{
    switch (_type)
    {
        case field_type::null:           return "none";
        case field_type::boolean:        return "string";
        case field_type::integer:        return "string";
        case field_type::big_integer:    return "string";
        case field_type::floating_point: return "string";
        case field_type::decimal:        return "string";
        case field_type::string:         return "string";
        case field_type::binary:         return "string";
        case field_type::date:           return "string";
        case field_type::time:           return "string";
        case field_type::datetime:       return "string";
        case field_type::timestamp:      return "string";
        case field_type::json:           return "ReJSON-RL";
        case field_type::xml:            return "string";
        case field_type::uuid:           return "string";
        case field_type::array:          return "list";
        case field_type::custom:
        default:                         return "string";
    }
}


// =============================================================================
// III. FEATURE SUPPORT (compile-time, version-gated)
// =============================================================================

// redis_type_support
//   struct: compile-time data type availability flags gated by
// D_ENV_REDIS_* macros.
struct redis_type_support
{
#if D_ENV_REDIS_DETECTED

    // core types — strings/lists/hashes/sets/zsets are available
    // in all supported Redis versions.
    static constexpr bool has_strings       = true;
    static constexpr bool has_lists         = true;
    static constexpr bool has_hashes        = true;
    static constexpr bool has_sets          = true;
    static constexpr bool has_sorted_sets   = true;

    // streams (Redis 5.0+)
    static constexpr bool has_streams =
    #if D_ENV_REDIS_HAS_STREAMS
        true;
    #else
        false;
    #endif

    // HyperLogLog (Redis 2.8.9+)
    static constexpr bool has_hyperloglog =
    #if D_ENV_REDIS_HAS_HYPERLOGLOG
        true;
    #else
        false;
    #endif

    // Geo commands (Redis 3.2+)
    static constexpr bool has_geo =
    #if D_ENV_REDIS_HAS_GEO
        true;
    #else
        false;
    #endif

    // Bitmaps (Redis 2.2+ via SETBIT/GETBIT; BITCOUNT 2.6+)
    static constexpr bool has_bitmaps =
    #if D_ENV_REDIS_HAS_BITMAPS
        true;
    #else
        false;
    #endif

    // BITFIELD (Redis 3.2+)
    static constexpr bool has_bitfield =
    #if D_ENV_REDIS_HAS_BITFIELD
        true;
    #else
        false;
    #endif

    // module-provided types
    static constexpr bool has_rejson =
    #if D_ENV_REDIS_HAS_REJSON
        true;
    #else
        false;
    #endif

    static constexpr bool has_rsearch =
    #if D_ENV_REDIS_HAS_RSEARCH
        true;
    #else
        false;
    #endif

    static constexpr bool has_rgraph =
    #if D_ENV_REDIS_HAS_RGRAPH
        true;
    #else
        false;
    #endif

    static constexpr bool has_rts =
    #if D_ENV_REDIS_HAS_RTS
        true;
    #else
        false;
    #endif

    static constexpr bool has_rbloom =
    #if D_ENV_REDIS_HAS_RBLOOM
        true;
    #else
        false;
    #endif

#else
    static constexpr bool has_strings       = false;
    static constexpr bool has_lists         = false;
    static constexpr bool has_hashes        = false;
    static constexpr bool has_sets          = false;
    static constexpr bool has_sorted_sets   = false;
    static constexpr bool has_streams       = false;
    static constexpr bool has_hyperloglog   = false;
    static constexpr bool has_geo           = false;
    static constexpr bool has_bitmaps       = false;
    static constexpr bool has_bitfield      = false;
    static constexpr bool has_rejson        = false;
    static constexpr bool has_rsearch       = false;
    static constexpr bool has_rgraph        = false;
    static constexpr bool has_rts           = false;
    static constexpr bool has_rbloom        = false;
#endif  // D_ENV_REDIS_DETECTED
};

// redis_feature_support
//   struct: compile-time server feature availability flags.
struct redis_feature_support
{
#if D_ENV_REDIS_DETECTED

    // protocol
    static constexpr bool has_resp2 =
    #if D_ENV_REDIS_HAS_RESP2
        true;
    #else
        false;
    #endif

    static constexpr bool has_resp3 =
    #if D_ENV_REDIS_HAS_RESP3
        true;
    #else
        false;
    #endif

    // scripting
    static constexpr bool has_lua_scripting =
    #if D_ENV_REDIS_HAS_LUA_SCRIPTING
        true;
    #else
        false;
    #endif

    static constexpr bool has_functions =
    #if D_ENV_REDIS_HAS_FUNCTIONS
        true;
    #else
        false;
    #endif

    // pub/sub
    static constexpr bool has_pubsub =
    #if D_ENV_REDIS_HAS_PUBSUB
        true;
    #else
        false;
    #endif

    static constexpr bool has_sharded_pubsub =
    #if D_ENV_REDIS_HAS_SHARDED_PUBSUB
        true;
    #else
        false;
    #endif

    // transactions
    static constexpr bool has_transactions =
    #if D_ENV_REDIS_HAS_TRANSACTIONS
        true;
    #else
        false;
    #endif

    static constexpr bool has_optimistic_locking =
    #if D_ENV_REDIS_HAS_OPTIMISTIC_LOCKING
        true;
    #else
        false;
    #endif

    // pipelining (always available since 1.0)
    static constexpr bool has_pipelining    = true;

    // cluster (Redis 3.0+)
    static constexpr bool has_cluster =
    #if D_ENV_REDIS_HAS_CLUSTER
        true;
    #else
        false;
    #endif

    // sentinel (Redis 2.4+)
    static constexpr bool has_sentinel =
    #if D_ENV_REDIS_HAS_SENTINEL
        true;
    #else
        false;
    #endif

    // replication (always; PSYNC since 4.0)
    static constexpr bool has_psync =
    #if D_ENV_REDIS_HAS_PSYNC
        true;
    #else
        false;
    #endif

    // persistence
    static constexpr bool has_rdb =
    #if D_ENV_REDIS_HAS_RDB
        true;
    #else
        false;
    #endif

    static constexpr bool has_aof =
    #if D_ENV_REDIS_HAS_AOF
        true;
    #else
        false;
    #endif

    // security
    static constexpr bool has_tls =
    #if D_ENV_REDIS_HAS_TLS
        true;
    #else
        false;
    #endif

    static constexpr bool has_acl =
    #if D_ENV_REDIS_HAS_ACL
        true;
    #else
        false;
    #endif

    // client tracking / client-side caching (Redis 6+)
    static constexpr bool has_client_tracking =
    #if D_ENV_REDIS_HAS_CLIENT_TRACKING
        true;
    #else
        false;
    #endif

    // expiration
    static constexpr bool has_lazy_expiration =
    #if D_ENV_REDIS_HAS_LAZY_EXPIRATION
        true;
    #else
        false;
    #endif

    // modules (Redis 4.0+)
    static constexpr bool has_modules =
    #if D_ENV_REDIS_HAS_MODULES
        true;
    #else
        false;
    #endif

    // SCAN cursor-based iteration (Redis 2.8+)
    static constexpr bool has_scan =
    #if D_ENV_REDIS_HAS_SCAN
        true;
    #else
        false;
    #endif

    // composite
    static constexpr bool has_modern_protocol =
    #if D_ENV_REDIS_HAS_MODERN_PROTOCOL
        true;
    #else
        false;
    #endif

#else
    static constexpr bool has_resp2              = false;
    static constexpr bool has_resp3              = false;
    static constexpr bool has_lua_scripting      = false;
    static constexpr bool has_functions          = false;
    static constexpr bool has_pubsub             = false;
    static constexpr bool has_sharded_pubsub     = false;
    static constexpr bool has_transactions       = false;
    static constexpr bool has_optimistic_locking = false;
    static constexpr bool has_pipelining         = false;
    static constexpr bool has_cluster            = false;
    static constexpr bool has_sentinel           = false;
    static constexpr bool has_psync              = false;
    static constexpr bool has_rdb                = false;
    static constexpr bool has_aof                = false;
    static constexpr bool has_tls                = false;
    static constexpr bool has_acl                = false;
    static constexpr bool has_client_tracking    = false;
    static constexpr bool has_lazy_expiration    = false;
    static constexpr bool has_modules            = false;
    static constexpr bool has_scan               = false;
    static constexpr bool has_modern_protocol    = false;
#endif  // D_ENV_REDIS_DETECTED
};


// =============================================================================
// IV.  REDIS VERSION INFORMATION
// =============================================================================

// redis_version_info
//   struct: compile-time version decomposition.
struct redis_version_info
{
#if D_ENV_REDIS_DETECTED
    static constexpr bool          detected = true;
    static constexpr std::uint32_t id       = D_ENV_REDIS_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_REDIS_VERSION_MAJOR;
    static constexpr std::uint16_t minor    = D_ENV_REDIS_VERSION_MINOR;
    static constexpr std::uint16_t patch    = D_ENV_REDIS_VERSION_PATCH;
    static constexpr const char*   string   = D_ENV_REDIS_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t minor    = 0;
    static constexpr std::uint16_t patch    = 0;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected Redis version is at
    // least (major, minor, patch).
    static constexpr bool at_least(std::uint16_t _major,
                                   std::uint16_t _minor,
                                   std::uint16_t _patch) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// V.   REDIS CONNECTION MODE / TLS ENUMERATIONS
// =============================================================================

// redis_connection_mode
//   enumeration: deployment topology the connection talks to.
enum class redis_connection_mode : std::uint8_t
{
    standalone = 0,     // single-node Redis
    sentinel   = 1,     // Redis Sentinel-managed master
    cluster    = 2      // Redis Cluster
};

// redis_resp_version
//   enumeration: wire protocol version to negotiate via HELLO.
enum class redis_resp_version : std::uint8_t
{
    auto_negotiate = 0,     // let the client decide
    resp2          = 2,     // legacy RESP2
    resp3          = 3      // RESP3 (Redis 6+)
};

// redis_tls_mode
//   enumeration: TLS negotiation modes.
enum class redis_tls_mode : std::uint8_t
{
    disable     = 0,        // no TLS
    enable      = 1,        // TLS required, no peer verification
    verify_peer = 2         // TLS required + peer cert verification
};


// =============================================================================
// VI.  REDIS CONNECTION CONFIGURATION
// =============================================================================

// redis_connect_config
//   struct: Redis-specific connection configuration extending the
// generic connection_config with hiredis options, ACL, TLS, RESP3
// negotiation, sentinel/cluster discovery, and module-load hints.
struct redis_connect_config
{
    connection_config       base;

    // logical database (0–15 by default; ignored for cluster).
    int                     database_index;

    // deployment topology
    redis_connection_mode   mode;

    // sentinel
    std::string             sentinel_master_name;
    std::vector<std::string> sentinel_hosts;

    // cluster
    std::vector<std::string> cluster_seed_nodes;

    // ACL (Redis 6+) — username overrides connection_config::username.
    std::string             acl_username;

    // protocol
    redis_resp_version      resp_version;
    bool                    enable_client_tracking;

    // TLS
    redis_tls_mode          tls_mode;
    std::string             tls_cert_file;
    std::string             tls_key_file;
    std::string             tls_ca_file;
    std::string             tls_sni;

    // health
    std::chrono::seconds    keep_alive_interval;

    // command timeouts (separate from connect_timeout in base).
    std::chrono::milliseconds command_timeout;

    // optional auto-loading of modules (path -> args).
    std::map<std::string, std::vector<std::string>> module_loads;

    redis_connect_config()
        : database_index(0)
        , mode(redis_connection_mode::standalone)
        , resp_version(redis_resp_version::auto_negotiate)
        , enable_client_tracking(false)
        , tls_mode(redis_tls_mode::disable)
        , keep_alive_interval(std::chrono::seconds(30))
        , command_timeout(std::chrono::milliseconds(5000))
    {
        base.host = "localhost";
        base.port = 6379;
    }

    explicit redis_connect_config(const connection_config& _base)
        : base(_base)
        , database_index(0)
        , mode(redis_connection_mode::standalone)
        , resp_version(redis_resp_version::auto_negotiate)
        , enable_client_tracking(false)
        , tls_mode(redis_tls_mode::disable)
        , keep_alive_interval(std::chrono::seconds(30))
        , command_timeout(std::chrono::milliseconds(5000))
    {
        if (base.port == 0)
        {
            base.port = 6379;
        }
    }
};


// =============================================================================
// VII. REDIS CONNECTION
// =============================================================================

// redis_connection
//   class: concrete Redis connection implementation via hiredis (or
// equivalent). This is the CRTP leaf class; _impl methods are defined
// in redis.cpp which includes <hiredis/hiredis.h>.
//
// Usage:
//   redis_connection conn;
//   conn.connect(redis_connect_config{...});
//   conn.set("greeting", "hello");
//   auto v = conn.get("greeting");
class redis_connection
    : public database_connection<redis_connection,
                                 database_type::redis>
{
public:
    using base_type       = database_connection<
        redis_connection, database_type::redis>;
    using type_support    = redis_type_support;
    using feature_support = redis_feature_support;
    using version_info    = redis_version_info;

    redis_connection()
        : base_type()
    {
    }

    explicit redis_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit redis_connection(const redis_connect_config& _config)
        : base_type(_config.base),
          m_redis_config(_config)
    {
    }

    ~redis_connection() = default;

    // disable copying
    redis_connection(const redis_connection&)            = delete;
    redis_connection& operator=(const redis_connection&) = delete;

    // enable moving
    redis_connection(redis_connection&&) noexcept            = default;
    redis_connection& operator=(redis_connection&&) noexcept = default;


    // -----------------------------------------------------------------
    // command dispatch
    // -----------------------------------------------------------------

    // send_command
    //   function: dispatches a raw RESP command asynchronously.
    // wraps redisAppendCommand().
    bool send_command(const std::string& _command)
    {
        this->ensure_connected();

        return self().send_command_impl(_command);
    }

    // execute_command
    //   function: dispatches a command and waits for the reply.
    // wraps redisCommand().
    auto execute_command(const std::string& _command)
    {
        this->ensure_connected();

        return self().execute_command_impl(_command);
    }

    // get_reply
    //   function: retrieves the next pending reply.
    // wraps redisGetReply().
    auto get_reply()
    {
        return self().get_reply_impl();
    }

    // ping
    //   function: PING the server; returns true on PONG.
    bool ping() const
    {
        return self().ping_impl();
    }


    // -----------------------------------------------------------------
    // PUB/SUB
    // -----------------------------------------------------------------

    // subscribe
    //   function: subscribes to a channel. SUBSCRIBE <channel>.
    void subscribe(const std::string& _channel)
    {
        this->ensure_connected();
        self().subscribe_impl(_channel);
    }

    // unsubscribe
    //   function: unsubscribes from a channel. UNSUBSCRIBE <channel>.
    void unsubscribe(const std::string& _channel)
    {
        self().unsubscribe_impl(_channel);
    }

    // psubscribe
    //   function: pattern subscribe. PSUBSCRIBE <pattern>.
    void psubscribe(const std::string& _pattern)
    {
        this->ensure_connected();
        self().psubscribe_impl(_pattern);
    }

    // punsubscribe
    //   function: pattern unsubscribe. PUNSUBSCRIBE <pattern>.
    void punsubscribe(const std::string& _pattern)
    {
        self().punsubscribe_impl(_pattern);
    }

    // publish
    //   function: publishes a message. PUBLISH <channel> <message>.
    std::int64_t publish(const std::string& _channel,
                         const std::string& _message)
    {
        this->ensure_connected();

        return self().publish_impl(_channel, _message);
    }

    // get_message
    //   function: retrieves the next pending pub/sub message, if any.
    auto get_message()
    {
        return self().get_message_impl();
    }


    // -----------------------------------------------------------------
    // transactions (MULTI / EXEC / DISCARD / WATCH)
    // -----------------------------------------------------------------

    // multi
    //   function: begins a transaction block.
    void multi()
    {
        this->ensure_connected();
        self().multi_impl();
    }

    // exec
    //   function: executes the queued transaction.
    auto exec()
    {
        return self().exec_impl();
    }

    // discard
    //   function: discards the queued transaction.
    void discard()
    {
        self().discard_impl();
    }

    // watch
    //   function: WATCH a key for optimistic concurrency control.
    void watch(const std::string& _key)
    {
        this->ensure_connected();
        self().watch_impl(_key);
    }

    // unwatch
    //   function: clears all watched keys.
    void unwatch()
    {
        self().unwatch_impl();
    }


    // -----------------------------------------------------------------
    // pipelining
    // -----------------------------------------------------------------

    // pipeline_start
    //   function: begins a pipeline batch.
    void pipeline_start()
    {
        this->ensure_connected();
        self().pipeline_start_impl();
    }

    // pipeline_exec
    //   function: flushes the pipeline batch and returns the reply
    // vector.
    auto pipeline_exec()
    {
        return self().pipeline_exec_impl();
    }

    // pipeline_discard
    //   function: discards the pending pipeline batch.
    void pipeline_discard()
    {
        self().pipeline_discard_impl();
    }


    // -----------------------------------------------------------------
    // Lua scripting
    // -----------------------------------------------------------------

    // eval
    //   function: EVAL a Lua script.
    auto eval(const std::string&              _script,
              const std::vector<std::string>& _keys,
              const std::vector<std::string>& _args)
    {
        this->ensure_connected();

        return self().eval_impl(_script, _keys, _args);
    }

    // evalsha
    //   function: EVALSHA a previously-loaded script.
    auto evalsha(const std::string&              _sha1,
                 const std::vector<std::string>& _keys,
                 const std::vector<std::string>& _args)
    {
        this->ensure_connected();

        return self().evalsha_impl(_sha1, _keys, _args);
    }

    // script_load
    //   function: SCRIPT LOAD <script>; returns the SHA1 digest.
    std::string script_load(const std::string& _script)
    {
        this->ensure_connected();

        return self().script_load_impl(_script);
    }


    // -----------------------------------------------------------------
    // key-space operations
    // -----------------------------------------------------------------

    // get
    //   function: GET <key>.
    std::optional<std::string> get(const std::string& _key)
    {
        this->ensure_connected();

        return self().get_impl(_key);
    }

    // set
    //   function: SET <key> <value>.
    bool set(const std::string& _key,
             const std::string& _value)
    {
        this->ensure_connected();

        return self().set_impl(_key, _value);
    }

    // del
    //   function: DEL <key>; returns number of keys removed.
    std::int64_t del(const std::string& _key)
    {
        this->ensure_connected();

        return self().del_impl(_key);
    }

    // key_exists
    //   function: EXISTS <key>.
    bool key_exists(const std::string& _key) const
    {
        return self().key_exists_impl(_key);
    }

    // expire
    //   function: EXPIRE <key> <seconds>.
    bool expire(const std::string& _key,
                std::int64_t       _seconds)
    {
        this->ensure_connected();

        return self().expire_impl(_key, _seconds);
    }

    // ttl
    //   function: TTL <key>. Returns -1 for no TTL, -2 for missing.
    std::int64_t ttl(const std::string& _key) const
    {
        return self().ttl_impl(_key);
    }

    // keys
    //   function: KEYS <pattern>. WARNING: blocks the server on
    // large key-spaces; prefer scan() in production.
    std::vector<std::string> keys(const std::string& _pattern) const
    {
        return self().keys_impl(_pattern);
    }

    // scan
    //   function: SCAN <cursor> [MATCH <pattern>] — cursor-based
    // iteration. Returns (next_cursor, keys).
    auto scan(std::int64_t       _cursor,
              const std::string& _pattern)
    {
        this->ensure_connected();

        return self().scan_impl(_cursor, _pattern);
    }


    // -----------------------------------------------------------------
    // hash operations
    // -----------------------------------------------------------------

    // hget
    //   function: HGET <key> <field>.
    std::optional<std::string> hget(
        const std::string& _key,
        const std::string& _field) const
    {
        return self().hget_impl(_key, _field);
    }

    // hset
    //   function: HSET <key> <field> <value>.
    std::int64_t hset(const std::string& _key,
                      const std::string& _field,
                      const std::string& _value)
    {
        this->ensure_connected();

        return self().hset_impl(_key, _field, _value);
    }

    // hdel
    //   function: HDEL <key> <field>.
    std::int64_t hdel(const std::string& _key,
                      const std::string& _field)
    {
        this->ensure_connected();

        return self().hdel_impl(_key, _field);
    }

    // hgetall
    //   function: HGETALL <key> — returns all fields and values.
    std::map<std::string, std::string> hgetall(
        const std::string& _key) const
    {
        return self().hgetall_impl(_key);
    }

    // hkeys
    //   function: HKEYS <key> — returns the field names.
    std::vector<std::string> hkeys(const std::string& _key) const
    {
        return self().hkeys_impl(_key);
    }


    // -----------------------------------------------------------------
    // list operations
    // -----------------------------------------------------------------

    // lpush
    //   function: LPUSH <key> <value>.
    std::int64_t lpush(const std::string& _key,
                       const std::string& _value)
    {
        this->ensure_connected();

        return self().lpush_impl(_key, _value);
    }

    // rpush
    //   function: RPUSH <key> <value>.
    std::int64_t rpush(const std::string& _key,
                       const std::string& _value)
    {
        this->ensure_connected();

        return self().rpush_impl(_key, _value);
    }

    // lpop
    //   function: LPOP <key>.
    std::optional<std::string> lpop(const std::string& _key)
    {
        this->ensure_connected();

        return self().lpop_impl(_key);
    }

    // rpop
    //   function: RPOP <key>.
    std::optional<std::string> rpop(const std::string& _key)
    {
        this->ensure_connected();

        return self().rpop_impl(_key);
    }

    // lrange
    //   function: LRANGE <key> <start> <stop>.
    std::vector<std::string> lrange(const std::string& _key,
                                    std::int64_t       _start,
                                    std::int64_t       _stop) const
    {
        return self().lrange_impl(_key, _start, _stop);
    }


    // -----------------------------------------------------------------
    // set operations
    // -----------------------------------------------------------------

    // sadd
    //   function: SADD <key> <member>.
    std::int64_t sadd(const std::string& _key,
                      const std::string& _member)
    {
        this->ensure_connected();

        return self().sadd_impl(_key, _member);
    }

    // srem
    //   function: SREM <key> <member>.
    std::int64_t srem(const std::string& _key,
                      const std::string& _member)
    {
        this->ensure_connected();

        return self().srem_impl(_key, _member);
    }

    // smembers
    //   function: SMEMBERS <key>.
    std::vector<std::string> smembers(const std::string& _key) const
    {
        return self().smembers_impl(_key);
    }


    // -----------------------------------------------------------------
    // sorted set operations
    // -----------------------------------------------------------------

    // zadd
    //   function: ZADD <key> <score> <member>.
    std::int64_t zadd(const std::string& _key,
                      double             _score,
                      const std::string& _member)
    {
        this->ensure_connected();

        return self().zadd_impl(_key, _score, _member);
    }

    // zrem
    //   function: ZREM <key> <member>.
    std::int64_t zrem(const std::string& _key,
                      const std::string& _member)
    {
        this->ensure_connected();

        return self().zrem_impl(_key, _member);
    }

    // zrange
    //   function: ZRANGE <key> <start> <stop>.
    std::vector<std::string> zrange(const std::string& _key,
                                    std::int64_t       _start,
                                    std::int64_t       _stop) const
    {
        return self().zrange_impl(_key, _start, _stop);
    }


    // -----------------------------------------------------------------
    // stream operations
    // -----------------------------------------------------------------

    // xadd
    //   function: XADD <key> <id> <field value ...>.
    std::string xadd(const std::string&              _key,
                     const std::string&              _id,
                     const std::vector<std::string>& _fields)
    {
        this->ensure_connected();

        return self().xadd_impl(_key, _id, _fields);
    }

    // xread
    //   function: XREAD COUNT <count> STREAMS <key> <last_id>.
    auto xread(const std::string& _key,
               const std::string& _last_id,
               std::int64_t       _count) const
    {
        return self().xread_impl(_key, _last_id, _count);
    }


    // -----------------------------------------------------------------
    // server info and diagnostics
    // -----------------------------------------------------------------

    // info
    //   function: INFO — returns the server status block.
    std::string info() const
    {
        return self().info_impl();
    }

    // client_id
    //   function: CLIENT ID — the connection's unique identifier.
    std::int64_t client_id() const
    {
        return self().client_id_impl();
    }

    // dbsize
    //   function: DBSIZE — number of keys in the current database.
    std::int64_t dbsize() const
    {
        return self().dbsize_impl();
    }

    // select_db
    //   function: SELECT <index> — switches the logical database.
    void select_db(int _index)
    {
        this->ensure_connected();
        self().select_db_impl(_index);
    }


    // -----------------------------------------------------------------
    // cluster operations
    // -----------------------------------------------------------------

    // cluster_info
    //   function: CLUSTER INFO.
    std::string cluster_info() const
    {
        return self().cluster_info_impl();
    }

    // cluster_slots
    //   function: CLUSTER SLOTS.
    std::string cluster_slots() const
    {
        return self().cluster_slots_impl();
    }

    // cluster_nodes
    //   function: CLUSTER NODES.
    std::string cluster_nodes() const
    {
        return self().cluster_nodes_impl();
    }


    // -----------------------------------------------------------------
    // persistence operations
    // -----------------------------------------------------------------

    // save
    //   function: SAVE — synchronous RDB snapshot. Blocks the server
    // for the duration.
    void save()
    {
        this->ensure_connected();
        self().save_impl();
    }

    // bgsave
    //   function: BGSAVE — background RDB snapshot.
    void bgsave()
    {
        this->ensure_connected();
        self().bgsave_impl();
    }

    // bgrewriteaof
    //   function: BGREWRITEAOF — background AOF rewrite.
    void bgrewriteaof()
    {
        this->ensure_connected();
        self().bgrewriteaof_impl();
    }

    // lastsave
    //   function: LASTSAVE — Unix timestamp of last successful RDB.
    std::int64_t lastsave() const
    {
        return self().lastsave_impl();
    }


    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_streams() noexcept
    {
        return type_support::has_streams;
    }

    static constexpr bool supports_geo() noexcept
    {
        return type_support::has_geo;
    }

    static constexpr bool supports_resp3() noexcept
    {
        return feature_support::has_resp3;
    }

    static constexpr bool supports_lua_scripting() noexcept
    {
        return feature_support::has_lua_scripting;
    }

    static constexpr bool supports_functions() noexcept
    {
        return feature_support::has_functions;
    }

    static constexpr bool supports_cluster() noexcept
    {
        return feature_support::has_cluster;
    }

    static constexpr bool supports_sentinel() noexcept
    {
        return feature_support::has_sentinel;
    }

    static constexpr bool supports_acl() noexcept
    {
        return feature_support::has_acl;
    }

    static constexpr bool supports_tls() noexcept
    {
        return feature_support::has_tls;
    }

    static constexpr bool supports_client_tracking() noexcept
    {
        return feature_support::has_client_tracking;
    }

    static constexpr bool supports_modules() noexcept
    {
        return feature_support::has_modules;
    }


    // -----------------------------------------------------------------
    // data type mapping
    // -----------------------------------------------------------------

    static field_type map_type(redis_type _type) noexcept
    {
        return redis_type_to_field_type(_type);
    }

    static redis_type type_from_name(const std::string& _name) noexcept
    {
        return redis_type_from_name(_name);
    }

    static const char* native_type_name(field_type _type) noexcept
    {
        return field_type_to_redis_native(_type);
    }


    // -----------------------------------------------------------------
    // Redis-specific configuration
    // -----------------------------------------------------------------

    // get_redis_config
    //   function: returns the Redis-specific configuration.
    const redis_connect_config& get_redis_config() const noexcept
    {
        return m_redis_config;
    }

    // set_redis_config
    //   function: replaces the Redis-specific configuration. Must be
    // called before connect().
    void set_redis_config(const redis_connect_config& _config)
    {
        m_redis_config = _config;
        this->m_config = _config.base;
    }


    // -----------------------------------------------------------------
    // _impl methods (defined in redis.cpp)
    // -----------------------------------------------------------------

    void         connect_impl();
    void         disconnect_impl();
    bool         is_connected_impl() const;
    bool         ping_impl() const;
    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;

    // command dispatch
    bool         send_command_impl(const std::string& _command);
    auto         execute_command_impl(const std::string& _command)
                     -> std::unique_ptr<
                         result_set<struct redis_result_set_impl>>;
    auto         get_reply_impl()
                     -> std::unique_ptr<
                         result_set<struct redis_result_set_impl>>;

    // PUB/SUB
    void         subscribe_impl(const std::string& _channel);
    void         unsubscribe_impl(const std::string& _channel);
    void         psubscribe_impl(const std::string& _pattern);
    void         punsubscribe_impl(const std::string& _pattern);
    std::int64_t publish_impl(const std::string& _channel,
                              const std::string& _message);
    auto         get_message_impl()
                     -> std::optional<std::string>;

    // transactions
    void         multi_impl();
    auto         exec_impl()
                     -> std::unique_ptr<
                         result_set<struct redis_result_set_impl>>;
    void         discard_impl();
    void         watch_impl(const std::string& _key);
    void         unwatch_impl();

    // pipelining
    void         pipeline_start_impl();
    auto         pipeline_exec_impl()
                     -> std::vector<std::string>;
    void         pipeline_discard_impl();

    // scripting
    auto         eval_impl(const std::string&              _script,
                           const std::vector<std::string>& _keys,
                           const std::vector<std::string>& _args)
                     -> std::unique_ptr<
                         result_set<struct redis_result_set_impl>>;
    auto         evalsha_impl(const std::string&              _sha1,
                              const std::vector<std::string>& _keys,
                              const std::vector<std::string>& _args)
                     -> std::unique_ptr<
                         result_set<struct redis_result_set_impl>>;
    std::string  script_load_impl(const std::string& _script);

    // key-space
    std::optional<std::string>
                 get_impl(const std::string& _key);
    bool         set_impl(const std::string& _key,
                          const std::string& _value);
    std::int64_t del_impl(const std::string& _key);
    bool         key_exists_impl(const std::string& _key) const;
    bool         expire_impl(const std::string& _key,
                             std::int64_t       _seconds);
    std::int64_t ttl_impl(const std::string& _key) const;
    std::vector<std::string>
                 keys_impl(const std::string& _pattern) const;
    auto         scan_impl(std::int64_t       _cursor,
                           const std::string& _pattern)
                     -> std::pair<std::int64_t,
                                  std::vector<std::string>>;

    // hashes
    std::optional<std::string>
                 hget_impl(const std::string& _key,
                           const std::string& _field) const;
    std::int64_t hset_impl(const std::string& _key,
                           const std::string& _field,
                           const std::string& _value);
    std::int64_t hdel_impl(const std::string& _key,
                           const std::string& _field);
    std::map<std::string, std::string>
                 hgetall_impl(const std::string& _key) const;
    std::vector<std::string>
                 hkeys_impl(const std::string& _key) const;

    // lists
    std::int64_t lpush_impl(const std::string& _key,
                            const std::string& _value);
    std::int64_t rpush_impl(const std::string& _key,
                            const std::string& _value);
    std::optional<std::string>
                 lpop_impl(const std::string& _key);
    std::optional<std::string>
                 rpop_impl(const std::string& _key);
    std::vector<std::string>
                 lrange_impl(const std::string& _key,
                             std::int64_t       _start,
                             std::int64_t       _stop) const;

    // sets
    std::int64_t sadd_impl(const std::string& _key,
                           const std::string& _member);
    std::int64_t srem_impl(const std::string& _key,
                           const std::string& _member);
    std::vector<std::string>
                 smembers_impl(const std::string& _key) const;

    // sorted sets
    std::int64_t zadd_impl(const std::string& _key,
                           double             _score,
                           const std::string& _member);
    std::int64_t zrem_impl(const std::string& _key,
                           const std::string& _member);
    std::vector<std::string>
                 zrange_impl(const std::string& _key,
                             std::int64_t       _start,
                             std::int64_t       _stop) const;

    // streams
    std::string  xadd_impl(const std::string&              _key,
                           const std::string&              _id,
                           const std::vector<std::string>& _fields);
    auto         xread_impl(const std::string& _key,
                            const std::string& _last_id,
                            std::int64_t       _count) const
                     -> std::vector<std::pair<
                         std::string,
                         std::map<std::string, std::string>>>;

    // diagnostics
    std::string  info_impl() const;
    std::int64_t client_id_impl() const;
    std::int64_t dbsize_impl() const;
    void         select_db_impl(int _index);

    // cluster
    std::string  cluster_info_impl() const;
    std::string  cluster_slots_impl() const;
    std::string  cluster_nodes_impl() const;

    // persistence
    void         save_impl();
    void         bgsave_impl();
    void         bgrewriteaof_impl();
    std::int64_t lastsave_impl() const;


    // -----------------------------------------------------------------
    // version-gated methods
    // -----------------------------------------------------------------

#if D_ENV_REDIS_DETECTED

    #if D_ENV_REDIS_HAS_ACL
    // acl_whoami / acl_list
    //   functions: ACL introspection. Available since Redis 6.0.
    std::string              acl_whoami() const;
    std::vector<std::string> acl_list() const;
    #endif

    #if D_ENV_REDIS_HAS_CLIENT_TRACKING
    // client_tracking_on / client_tracking_off
    //   functions: CLIENT TRACKING ON/OFF. Available since Redis 6.0.
    void client_tracking_on();
    void client_tracking_off();
    #endif

    #if D_ENV_REDIS_HAS_FUNCTIONS
    // function_load
    //   function: FUNCTION LOAD <code>. Available since Redis 7.0.
    std::string function_load(const std::string& _code);
    #endif

#endif  // D_ENV_REDIS_DETECTED


private:
    redis_connect_config m_redis_config;

    redis_connection& self()
    {
        return *this;
    }

    const redis_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// redis_result_set_impl
//   struct: forward declaration of the Redis result set
// implementation.
struct redis_result_set_impl;


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former redis_traits.hpp / redis_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// =============================================================================
// IX.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  command dispatch
// -------------------------------------------------------------------------

// redis_send_command_t
//   detector: send_command(const std::string&) method.
// wraps redisAppendCommand() / redisCommand() — the basic command
// dispatcher.
template<typename _Type>
using redis_send_command_t =
    decltype(std::declval<_Type&>().send_command(
        std::declval<const std::string&>()));

// redis_execute_command_t
//   detector: execute_command(const std::string&) method.
// synchronous round-trip variant.
template<typename _Type>
using redis_execute_command_t =
    decltype(std::declval<_Type&>().execute_command(
        std::declval<const std::string&>()));

// redis_get_reply_t
//   detector: get_reply() method.
// wraps redisGetReply() — pulls the next reply from the receive
// buffer.
template<typename _Type>
using redis_get_reply_t =
    decltype(std::declval<_Type&>().get_reply());

// redis_ping_t
//   detector: ping() const method.
// wraps the PING command.
template<typename _Type>
using redis_ping_t =
    decltype(std::declval<const _Type&>().ping());


// -------------------------------------------------------------------------
// B.  PUB/SUB messaging
// -------------------------------------------------------------------------

// redis_subscribe_t
//   detector: subscribe(const std::string&) method.
// SUBSCRIBE <channel>.
template<typename _Type>
using redis_subscribe_t =
    decltype(std::declval<_Type&>().subscribe(
        std::declval<const std::string&>()));

// redis_unsubscribe_t
//   detector: unsubscribe(const std::string&) method.
// UNSUBSCRIBE <channel>.
template<typename _Type>
using redis_unsubscribe_t =
    decltype(std::declval<_Type&>().unsubscribe(
        std::declval<const std::string&>()));

// redis_psubscribe_t
//   detector: psubscribe(const std::string&) method.
// PSUBSCRIBE <pattern>.
template<typename _Type>
using redis_psubscribe_t =
    decltype(std::declval<_Type&>().psubscribe(
        std::declval<const std::string&>()));

// redis_punsubscribe_t
//   detector: punsubscribe(const std::string&) method.
// PUNSUBSCRIBE <pattern>.
template<typename _Type>
using redis_punsubscribe_t =
    decltype(std::declval<_Type&>().punsubscribe(
        std::declval<const std::string&>()));

// redis_publish_t
//   detector: publish(channel, message) method.
// PUBLISH <channel> <message>.
template<typename _Type>
using redis_publish_t =
    decltype(std::declval<_Type&>().publish(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_get_message_t
//   detector: get_message() method.
// retrieves the next pub/sub message.
template<typename _Type>
using redis_get_message_t =
    decltype(std::declval<_Type&>().get_message());


// -------------------------------------------------------------------------
// C.  transactions (MULTI / EXEC / DISCARD / WATCH)
// -------------------------------------------------------------------------

// redis_multi_t
//   detector: multi() method.
// begins a transaction block (MULTI).
template<typename _Type>
using redis_multi_t =
    decltype(std::declval<_Type&>().multi());

// redis_exec_t
//   detector: exec() method.
// executes the queued transaction (EXEC).
template<typename _Type>
using redis_exec_t =
    decltype(std::declval<_Type&>().exec());

// redis_discard_t
//   detector: discard() method.
// aborts the queued transaction (DISCARD).
template<typename _Type>
using redis_discard_t =
    decltype(std::declval<_Type&>().discard());

// redis_watch_t
//   detector: watch(const std::string&) method.
// optimistic lock on a key (WATCH).
template<typename _Type>
using redis_watch_t =
    decltype(std::declval<_Type&>().watch(
        std::declval<const std::string&>()));

// redis_unwatch_t
//   detector: unwatch() method.
// clears all watched keys (UNWATCH).
template<typename _Type>
using redis_unwatch_t =
    decltype(std::declval<_Type&>().unwatch());


// -------------------------------------------------------------------------
// D.  pipelining
// -------------------------------------------------------------------------

// redis_pipeline_start_t
//   detector: pipeline_start() method.
// begins a pipeline batch.
template<typename _Type>
using redis_pipeline_start_t =
    decltype(std::declval<_Type&>().pipeline_start());

// redis_pipeline_exec_t
//   detector: pipeline_exec() method.
// flushes the pipeline batch and gathers replies.
template<typename _Type>
using redis_pipeline_exec_t =
    decltype(std::declval<_Type&>().pipeline_exec());

// redis_pipeline_discard_t
//   detector: pipeline_discard() method.
// drops the pipeline batch without dispatching.
template<typename _Type>
using redis_pipeline_discard_t =
    decltype(std::declval<_Type&>().pipeline_discard());


// -------------------------------------------------------------------------
// E.  Lua scripting
// -------------------------------------------------------------------------

// redis_eval_t
//   detector: eval(script, keys, args) method.
// EVAL <script> <numkeys> <key...> <arg...>.
template<typename _Type>
using redis_eval_t =
    decltype(std::declval<_Type&>().eval(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>(),
        std::declval<const std::vector<std::string>&>()));

// redis_evalsha_t
//   detector: evalsha(sha1, keys, args) method.
// EVALSHA <sha1> <numkeys> <key...> <arg...>.
template<typename _Type>
using redis_evalsha_t =
    decltype(std::declval<_Type&>().evalsha(
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>(),
        std::declval<const std::vector<std::string>&>()));

// redis_script_load_t
//   detector: script_load(const std::string&) method.
// SCRIPT LOAD <script> — caches a script and returns its SHA1.
template<typename _Type>
using redis_script_load_t =
    decltype(std::declval<_Type&>().script_load(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// F.  key-space operations
// -------------------------------------------------------------------------

// redis_get_t
//   detector: get(key) method.
// GET <key>.
template<typename _Type>
using redis_get_t =
    decltype(std::declval<_Type&>().get(
        std::declval<const std::string&>()));

// redis_set_t
//   detector: set(key, value) method.
// SET <key> <value>.
template<typename _Type>
using redis_set_t =
    decltype(std::declval<_Type&>().set(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_del_t
//   detector: del(key) method.
// DEL <key>.
template<typename _Type>
using redis_del_t =
    decltype(std::declval<_Type&>().del(
        std::declval<const std::string&>()));

// redis_exists_t
//   detector: key_exists(key) const method.
// EXISTS <key>.
template<typename _Type>
using redis_exists_t =
    decltype(std::declval<const _Type&>().key_exists(
        std::declval<const std::string&>()));

// redis_expire_t
//   detector: expire(key, seconds) method.
// EXPIRE <key> <seconds>.
template<typename _Type>
using redis_expire_t =
    decltype(std::declval<_Type&>().expire(
        std::declval<const std::string&>(),
        std::declval<std::int64_t>()));

// redis_ttl_t
//   detector: ttl(key) const method.
// TTL <key>.
template<typename _Type>
using redis_ttl_t =
    decltype(std::declval<const _Type&>().ttl(
        std::declval<const std::string&>()));

// redis_keys_t
//   detector: keys(pattern) const method.
// KEYS <pattern>.
template<typename _Type>
using redis_keys_t =
    decltype(std::declval<const _Type&>().keys(
        std::declval<const std::string&>()));

// redis_scan_t
//   detector: scan(cursor, pattern) method.
// SCAN <cursor> [MATCH <pattern>] — cursor-based iteration.
template<typename _Type>
using redis_scan_t =
    decltype(std::declval<_Type&>().scan(
        std::declval<std::int64_t>(),
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// G.  hash operations
// -------------------------------------------------------------------------

// redis_hget_t
//   detector: hget(key, field) const method.
// HGET <key> <field>.
template<typename _Type>
using redis_hget_t =
    decltype(std::declval<const _Type&>().hget(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_hset_t
//   detector: hset(key, field, value) method.
// HSET <key> <field> <value>.
template<typename _Type>
using redis_hset_t =
    decltype(std::declval<_Type&>().hset(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_hdel_t
//   detector: hdel(key, field) method.
// HDEL <key> <field>.
template<typename _Type>
using redis_hdel_t =
    decltype(std::declval<_Type&>().hdel(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_hgetall_t
//   detector: hgetall(key) const method.
// HGETALL <key>.
template<typename _Type>
using redis_hgetall_t =
    decltype(std::declval<const _Type&>().hgetall(
        std::declval<const std::string&>()));

// redis_hkeys_t
//   detector: hkeys(key) const method.
// HKEYS <key>.
template<typename _Type>
using redis_hkeys_t =
    decltype(std::declval<const _Type&>().hkeys(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// H.  list operations
// -------------------------------------------------------------------------

// redis_lpush_t
//   detector: lpush(key, value) method.
// LPUSH <key> <value>.
template<typename _Type>
using redis_lpush_t =
    decltype(std::declval<_Type&>().lpush(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_rpush_t
//   detector: rpush(key, value) method.
// RPUSH <key> <value>.
template<typename _Type>
using redis_rpush_t =
    decltype(std::declval<_Type&>().rpush(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_lpop_t
//   detector: lpop(key) method.
// LPOP <key>.
template<typename _Type>
using redis_lpop_t =
    decltype(std::declval<_Type&>().lpop(
        std::declval<const std::string&>()));

// redis_rpop_t
//   detector: rpop(key) method.
// RPOP <key>.
template<typename _Type>
using redis_rpop_t =
    decltype(std::declval<_Type&>().rpop(
        std::declval<const std::string&>()));

// redis_lrange_t
//   detector: lrange(key, start, stop) const method.
// LRANGE <key> <start> <stop>.
template<typename _Type>
using redis_lrange_t =
    decltype(std::declval<const _Type&>().lrange(
        std::declval<const std::string&>(),
        std::declval<std::int64_t>(),
        std::declval<std::int64_t>()));


// -------------------------------------------------------------------------
// I.  set operations
// -------------------------------------------------------------------------

// redis_sadd_t
//   detector: sadd(key, member) method.
// SADD <key> <member>.
template<typename _Type>
using redis_sadd_t =
    decltype(std::declval<_Type&>().sadd(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_srem_t
//   detector: srem(key, member) method.
// SREM <key> <member>.
template<typename _Type>
using redis_srem_t =
    decltype(std::declval<_Type&>().srem(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_smembers_t
//   detector: smembers(key) const method.
// SMEMBERS <key>.
template<typename _Type>
using redis_smembers_t =
    decltype(std::declval<const _Type&>().smembers(
        std::declval<const std::string&>()));


// -------------------------------------------------------------------------
// J.  sorted set operations
// -------------------------------------------------------------------------

// redis_zadd_t
//   detector: zadd(key, score, member) method.
// ZADD <key> <score> <member>.
template<typename _Type>
using redis_zadd_t =
    decltype(std::declval<_Type&>().zadd(
        std::declval<const std::string&>(),
        std::declval<double>(),
        std::declval<const std::string&>()));

// redis_zrem_t
//   detector: zrem(key, member) method.
// ZREM <key> <member>.
template<typename _Type>
using redis_zrem_t =
    decltype(std::declval<_Type&>().zrem(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// redis_zrange_t
//   detector: zrange(key, start, stop) const method.
// ZRANGE <key> <start> <stop>.
template<typename _Type>
using redis_zrange_t =
    decltype(std::declval<const _Type&>().zrange(
        std::declval<const std::string&>(),
        std::declval<std::int64_t>(),
        std::declval<std::int64_t>()));


// -------------------------------------------------------------------------
// K.  stream operations
// -------------------------------------------------------------------------

// redis_xadd_t
//   detector: xadd(key, id, fields) method.
// XADD <key> <id> <field value...>.
template<typename _Type>
using redis_xadd_t =
    decltype(std::declval<_Type&>().xadd(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<const std::vector<std::string>&>()));

// redis_xread_t
//   detector: xread(key, last_id, count) const method.
// XREAD COUNT <count> STREAMS <key> <last_id>.
template<typename _Type>
using redis_xread_t =
    decltype(std::declval<const _Type&>().xread(
        std::declval<const std::string&>(),
        std::declval<const std::string&>(),
        std::declval<std::int64_t>()));


// -------------------------------------------------------------------------
// L.  server info and diagnostics
// -------------------------------------------------------------------------

// redis_info_t
//   detector: info() const method.
// INFO — returns server status block.
template<typename _Type>
using redis_info_t =
    decltype(std::declval<const _Type&>().info());

// redis_client_id_t
//   detector: client_id() const method.
// CLIENT ID — the connection's unique identifier.
template<typename _Type>
using redis_client_id_t =
    decltype(std::declval<const _Type&>().client_id());

// redis_dbsize_t
//   detector: dbsize() const method.
// DBSIZE — number of keys in the current database.
template<typename _Type>
using redis_dbsize_t =
    decltype(std::declval<const _Type&>().dbsize());

// redis_select_db_t
//   detector: select_db(int) method.
// SELECT <index> — switches the logical database (0–15 by default).
template<typename _Type>
using redis_select_db_t =
    decltype(std::declval<_Type&>().select_db(
        std::declval<int>()));


// -------------------------------------------------------------------------
// M.  cluster operations
// -------------------------------------------------------------------------

// redis_cluster_info_t
//   detector: cluster_info() const method.
// CLUSTER INFO — returns cluster status.
template<typename _Type>
using redis_cluster_info_t =
    decltype(std::declval<const _Type&>().cluster_info());

// redis_cluster_slots_t
//   detector: cluster_slots() const method.
// CLUSTER SLOTS — returns slot-to-node mapping.
template<typename _Type>
using redis_cluster_slots_t =
    decltype(std::declval<const _Type&>().cluster_slots());

// redis_cluster_nodes_t
//   detector: cluster_nodes() const method.
// CLUSTER NODES — returns the cluster node list.
template<typename _Type>
using redis_cluster_nodes_t =
    decltype(std::declval<const _Type&>().cluster_nodes());


// -------------------------------------------------------------------------
// N.  persistence operations
// -------------------------------------------------------------------------

// redis_save_t
//   detector: save() method.
// SAVE — synchronous RDB snapshot.
template<typename _Type>
using redis_save_t =
    decltype(std::declval<_Type&>().save());

// redis_bgsave_t
//   detector: bgsave() method.
// BGSAVE — background RDB snapshot.
template<typename _Type>
using redis_bgsave_t =
    decltype(std::declval<_Type&>().bgsave());

// redis_bgrewriteaof_t
//   detector: bgrewriteaof() method.
// BGREWRITEAOF — background AOF rewrite.
template<typename _Type>
using redis_bgrewriteaof_t =
    decltype(std::declval<_Type&>().bgrewriteaof());

// redis_lastsave_t
//   detector: lastsave() const method.
// LASTSAVE — Unix timestamp of last successful RDB snapshot.
template<typename _Type>
using redis_lastsave_t =
    decltype(std::declval<const _Type&>().lastsave());


// =============================================================================
// X.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_redis_command_dispatch
//   trait: checks if type _Type supports core command dispatch
// (send_command + execute_command + get_reply).
template<typename _Type>
struct has_redis_command_dispatch : djinterp::conjunction<
    is_detected<redis_send_command_t, clean_t<_Type>>,
    is_detected<redis_execute_command_t, clean_t<_Type>>,
    is_detected<redis_get_reply_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_command_dispatch_v =
        has_redis_command_dispatch<clean_t<_Type>>::value;
#endif

// has_redis_pubsub
//   trait: checks if type _Type supports the PUB/SUB surface
// (subscribe + unsubscribe + publish + get_message).
template<typename _Type>
struct has_redis_pubsub : djinterp::conjunction<
    is_detected<redis_subscribe_t, clean_t<_Type>>,
    is_detected<redis_unsubscribe_t, clean_t<_Type>>,
    is_detected<redis_publish_t, clean_t<_Type>>,
    is_detected<redis_get_message_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_pubsub_v =
        has_redis_pubsub<clean_t<_Type>>::value;
#endif

// has_redis_pattern_pubsub
//   trait: checks if type _Type supports pattern-based PUB/SUB
// (psubscribe + punsubscribe).
template<typename _Type>
struct has_redis_pattern_pubsub : djinterp::conjunction<
    is_detected<redis_psubscribe_t, clean_t<_Type>>,
    is_detected<redis_punsubscribe_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_pattern_pubsub_v =
        has_redis_pattern_pubsub<clean_t<_Type>>::value;
#endif

// has_redis_transactions
//   trait: checks if type _Type supports multi-key transactions
// (multi + exec + discard + watch + unwatch).
template<typename _Type>
struct has_redis_transactions : djinterp::conjunction<
    is_detected<redis_multi_t, clean_t<_Type>>,
    is_detected<redis_exec_t, clean_t<_Type>>,
    is_detected<redis_discard_t, clean_t<_Type>>,
    is_detected<redis_watch_t, clean_t<_Type>>,
    is_detected<redis_unwatch_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_transactions_v =
        has_redis_transactions<clean_t<_Type>>::value;
#endif

// has_redis_pipelining
//   trait: checks if type _Type supports pipelining
// (pipeline_start + pipeline_exec).
template<typename _Type>
struct has_redis_pipelining : djinterp::conjunction<
    is_detected<redis_pipeline_start_t, clean_t<_Type>>,
    is_detected<redis_pipeline_exec_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_pipelining_v =
        has_redis_pipelining<clean_t<_Type>>::value;
#endif

// has_redis_scripting
//   trait: checks if type _Type supports Lua scripting
// (eval + evalsha + script_load).
template<typename _Type>
struct has_redis_scripting : djinterp::conjunction<
    is_detected<redis_eval_t, clean_t<_Type>>,
    is_detected<redis_evalsha_t, clean_t<_Type>>,
    is_detected<redis_script_load_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_scripting_v =
        has_redis_scripting<clean_t<_Type>>::value;
#endif

// has_redis_key_ops
//   trait: checks if type _Type supports core key-space operations
// (get + set + del + key_exists).
template<typename _Type>
struct has_redis_key_ops : djinterp::conjunction<
    is_detected<redis_get_t, clean_t<_Type>>,
    is_detected<redis_set_t, clean_t<_Type>>,
    is_detected<redis_del_t, clean_t<_Type>>,
    is_detected<redis_exists_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_key_ops_v =
        has_redis_key_ops<clean_t<_Type>>::value;
#endif

// has_redis_expiration
//   trait: checks if type _Type supports key expiration
// (expire + ttl).
template<typename _Type>
struct has_redis_expiration : djinterp::conjunction<
    is_detected<redis_expire_t, clean_t<_Type>>,
    is_detected<redis_ttl_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_expiration_v =
        has_redis_expiration<clean_t<_Type>>::value;
#endif

// has_redis_key_iteration
//   trait: checks if type _Type supports key-space iteration
// (keys + scan).
template<typename _Type>
struct has_redis_key_iteration : djinterp::conjunction<
    is_detected<redis_keys_t, clean_t<_Type>>,
    is_detected<redis_scan_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_key_iteration_v =
        has_redis_key_iteration<clean_t<_Type>>::value;
#endif

// has_redis_hash_ops
//   trait: checks if type _Type supports hash operations
// (hget + hset + hdel + hgetall).
template<typename _Type>
struct has_redis_hash_ops : djinterp::conjunction<
    is_detected<redis_hget_t, clean_t<_Type>>,
    is_detected<redis_hset_t, clean_t<_Type>>,
    is_detected<redis_hdel_t, clean_t<_Type>>,
    is_detected<redis_hgetall_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_hash_ops_v =
        has_redis_hash_ops<clean_t<_Type>>::value;
#endif

// has_redis_list_ops
//   trait: checks if type _Type supports list operations
// (lpush + rpush + lpop + rpop + lrange).
template<typename _Type>
struct has_redis_list_ops : djinterp::conjunction<
    is_detected<redis_lpush_t, clean_t<_Type>>,
    is_detected<redis_rpush_t, clean_t<_Type>>,
    is_detected<redis_lpop_t, clean_t<_Type>>,
    is_detected<redis_rpop_t, clean_t<_Type>>,
    is_detected<redis_lrange_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_list_ops_v =
        has_redis_list_ops<clean_t<_Type>>::value;
#endif

// has_redis_set_ops
//   trait: checks if type _Type supports set operations
// (sadd + srem + smembers).
template<typename _Type>
struct has_redis_set_ops : djinterp::conjunction<
    is_detected<redis_sadd_t, clean_t<_Type>>,
    is_detected<redis_srem_t, clean_t<_Type>>,
    is_detected<redis_smembers_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_set_ops_v =
        has_redis_set_ops<clean_t<_Type>>::value;
#endif

// has_redis_sorted_set_ops
//   trait: checks if type _Type supports sorted set operations
// (zadd + zrem + zrange).
template<typename _Type>
struct has_redis_sorted_set_ops : djinterp::conjunction<
    is_detected<redis_zadd_t, clean_t<_Type>>,
    is_detected<redis_zrem_t, clean_t<_Type>>,
    is_detected<redis_zrange_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_sorted_set_ops_v =
        has_redis_sorted_set_ops<clean_t<_Type>>::value;
#endif

// has_redis_stream_ops
//   trait: checks if type _Type supports stream operations
// (xadd + xread).
template<typename _Type>
struct has_redis_stream_ops : djinterp::conjunction<
    is_detected<redis_xadd_t, clean_t<_Type>>,
    is_detected<redis_xread_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_stream_ops_v =
        has_redis_stream_ops<clean_t<_Type>>::value;
#endif

// has_redis_diagnostics
//   trait: checks if type _Type supports server diagnostics
// (info + client_id + dbsize + ping).
template<typename _Type>
struct has_redis_diagnostics : djinterp::conjunction<
    is_detected<redis_info_t, clean_t<_Type>>,
    is_detected<redis_client_id_t, clean_t<_Type>>,
    is_detected<redis_dbsize_t, clean_t<_Type>>,
    is_detected<redis_ping_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_diagnostics_v =
        has_redis_diagnostics<clean_t<_Type>>::value;
#endif

// has_redis_cluster_ops
//   trait: checks if type _Type supports cluster operations
// (cluster_info + cluster_slots + cluster_nodes).
template<typename _Type>
struct has_redis_cluster_ops : djinterp::conjunction<
    is_detected<redis_cluster_info_t, clean_t<_Type>>,
    is_detected<redis_cluster_slots_t, clean_t<_Type>>,
    is_detected<redis_cluster_nodes_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_cluster_ops_v =
        has_redis_cluster_ops<clean_t<_Type>>::value;
#endif

// has_redis_persistence
//   trait: checks if type _Type supports persistence operations
// (save + bgsave + bgrewriteaof + lastsave).
template<typename _Type>
struct has_redis_persistence : djinterp::conjunction<
    is_detected<redis_save_t, clean_t<_Type>>,
    is_detected<redis_bgsave_t, clean_t<_Type>>,
    is_detected<redis_bgrewriteaof_t, clean_t<_Type>>,
    is_detected<redis_lastsave_t, clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_persistence_v =
        has_redis_persistence<clean_t<_Type>>::value;
#endif

// has_redis_db_selection
//   trait: checks if type _Type supports logical database selection
// (select_db).
template<typename _Type>
struct has_redis_db_selection
    : is_detected<redis_select_db_t, clean_t<_Type>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_redis_db_selection_v =
        has_redis_db_selection<clean_t<_Type>>::value;
#endif

// is_redis_connection
//   trait: compound trait verifying type _Type implements a Redis
// connection interface (command dispatch + key ops + hash ops +
// diagnostics).
template<typename _Type>
struct is_redis_connection : djinterp::conjunction<
    has_redis_command_dispatch<clean_t<_Type>>,
    has_redis_key_ops<clean_t<_Type>>,
    has_redis_hash_ops<clean_t<_Type>>,
    has_redis_diagnostics<clean_t<_Type>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_redis_connection_v =
        is_redis_connection<clean_t<_Type>>::value;
#endif


// =============================================================================
// XI. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// redis_can_send_command
//   tagless trait: true if _Type has send_command().
template<typename _Type,
         typename = void>
constexpr bool redis_can_send_command = false;

template<typename _Type>
constexpr bool redis_can_send_command<_Type,
    std::void_t<redis_send_command_t<_Type>>> = true;

// redis_can_publish
//   tagless trait: true if _Type has publish().
template<typename _Type,
         typename = void>
constexpr bool redis_can_publish = false;

template<typename _Type>
constexpr bool redis_can_publish<_Type,
    std::void_t<redis_publish_t<_Type>>> = true;

// redis_can_subscribe
//   tagless trait: true if _Type has subscribe().
template<typename _Type,
         typename = void>
constexpr bool redis_can_subscribe = false;

template<typename _Type>
constexpr bool redis_can_subscribe<_Type,
    std::void_t<redis_subscribe_t<_Type>>> = true;

// redis_can_multi
//   tagless trait: true if _Type has multi().
template<typename _Type,
         typename = void>
constexpr bool redis_can_multi = false;

template<typename _Type>
constexpr bool redis_can_multi<_Type,
    std::void_t<redis_multi_t<_Type>>> = true;

// redis_can_watch
//   tagless trait: true if _Type has watch().
template<typename _Type,
         typename = void>
constexpr bool redis_can_watch = false;

template<typename _Type>
constexpr bool redis_can_watch<_Type,
    std::void_t<redis_watch_t<_Type>>> = true;

// redis_can_pipeline
//   tagless trait: true if _Type has pipeline_start().
template<typename _Type,
         typename = void>
constexpr bool redis_can_pipeline = false;

template<typename _Type>
constexpr bool redis_can_pipeline<_Type,
    std::void_t<redis_pipeline_start_t<_Type>>> = true;

// redis_can_eval
//   tagless trait: true if _Type has eval().
template<typename _Type,
         typename = void>
constexpr bool redis_can_eval = false;

template<typename _Type>
constexpr bool redis_can_eval<_Type,
    std::void_t<redis_eval_t<_Type>>> = true;

// redis_can_get
//   tagless trait: true if _Type has get().
template<typename _Type,
         typename = void>
constexpr bool redis_can_get = false;

template<typename _Type>
constexpr bool redis_can_get<_Type,
    std::void_t<redis_get_t<_Type>>> = true;

// redis_can_set
//   tagless trait: true if _Type has set().
template<typename _Type,
         typename = void>
constexpr bool redis_can_set = false;

template<typename _Type>
constexpr bool redis_can_set<_Type,
    std::void_t<redis_set_t<_Type>>> = true;

// redis_can_expire
//   tagless trait: true if _Type has expire().
template<typename _Type,
         typename = void>
constexpr bool redis_can_expire = false;

template<typename _Type>
constexpr bool redis_can_expire<_Type,
    std::void_t<redis_expire_t<_Type>>> = true;

// redis_can_scan
//   tagless trait: true if _Type has scan().
template<typename _Type,
         typename = void>
constexpr bool redis_can_scan = false;

template<typename _Type>
constexpr bool redis_can_scan<_Type,
    std::void_t<redis_scan_t<_Type>>> = true;

// redis_can_hset
//   tagless trait: true if _Type has hset().
template<typename _Type,
         typename = void>
constexpr bool redis_can_hset = false;

template<typename _Type>
constexpr bool redis_can_hset<_Type,
    std::void_t<redis_hset_t<_Type>>> = true;

// redis_can_xadd
//   tagless trait: true if _Type has xadd().
template<typename _Type,
         typename = void>
constexpr bool redis_can_xadd = false;

template<typename _Type>
constexpr bool redis_can_xadd<_Type,
    std::void_t<redis_xadd_t<_Type>>> = true;

// redis_can_cluster_query
//   tagless trait: true if _Type has cluster_info().
template<typename _Type,
         typename = void>
constexpr bool redis_can_cluster_query = false;

template<typename _Type>
constexpr bool redis_can_cluster_query<_Type,
    std::void_t<redis_cluster_info_t<_Type>>> = true;

// redis_can_select_db
//   tagless trait: true if _Type has select_db().
template<typename _Type,
         typename = void>
constexpr bool redis_can_select_db = false;

template<typename _Type>
constexpr bool redis_can_select_db<_Type,
    std::void_t<redis_select_db_t<_Type>>> = true;


// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// redis_does_command_dispatch
//   tagless trait: true if _Type supports the full command-dispatch
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_command_dispatch = false;

template<typename _Type>
constexpr bool redis_does_command_dispatch<_Type, std::void_t<
    redis_send_command_t<_Type>,
    redis_execute_command_t<_Type>,
    redis_get_reply_t<_Type>>> = true;

// redis_does_pubsub
//   tagless trait: true if _Type supports the full PUB/SUB surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_pubsub = false;

template<typename _Type>
constexpr bool redis_does_pubsub<_Type, std::void_t<
    redis_subscribe_t<_Type>,
    redis_unsubscribe_t<_Type>,
    redis_publish_t<_Type>,
    redis_get_message_t<_Type>>> = true;

// redis_does_transactions
//   tagless trait: true if _Type supports the full multi-key
// transaction surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_transactions = false;

template<typename _Type>
constexpr bool redis_does_transactions<_Type, std::void_t<
    redis_multi_t<_Type>,
    redis_exec_t<_Type>,
    redis_discard_t<_Type>,
    redis_watch_t<_Type>,
    redis_unwatch_t<_Type>>> = true;

// redis_does_pipelining
//   tagless trait: true if _Type supports the full pipelining
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_pipelining = false;

template<typename _Type>
constexpr bool redis_does_pipelining<_Type, std::void_t<
    redis_pipeline_start_t<_Type>,
    redis_pipeline_exec_t<_Type>>> = true;

// redis_does_scripting
//   tagless trait: true if _Type supports the full Lua scripting
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_scripting = false;

template<typename _Type>
constexpr bool redis_does_scripting<_Type, std::void_t<
    redis_eval_t<_Type>,
    redis_evalsha_t<_Type>,
    redis_script_load_t<_Type>>> = true;

// redis_does_key_ops
//   tagless trait: true if _Type supports the full key-space
// operation surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_key_ops = false;

template<typename _Type>
constexpr bool redis_does_key_ops<_Type, std::void_t<
    redis_get_t<_Type>,
    redis_set_t<_Type>,
    redis_del_t<_Type>,
    redis_exists_t<_Type>>> = true;

// redis_does_hash_ops
//   tagless trait: true if _Type supports the full hash operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_hash_ops = false;

template<typename _Type>
constexpr bool redis_does_hash_ops<_Type, std::void_t<
    redis_hget_t<_Type>,
    redis_hset_t<_Type>,
    redis_hdel_t<_Type>,
    redis_hgetall_t<_Type>>> = true;

// redis_does_list_ops
//   tagless trait: true if _Type supports the full list operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_list_ops = false;

template<typename _Type>
constexpr bool redis_does_list_ops<_Type, std::void_t<
    redis_lpush_t<_Type>,
    redis_rpush_t<_Type>,
    redis_lpop_t<_Type>,
    redis_rpop_t<_Type>,
    redis_lrange_t<_Type>>> = true;

// redis_does_set_ops
//   tagless trait: true if _Type supports the full set operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_set_ops = false;

template<typename _Type>
constexpr bool redis_does_set_ops<_Type, std::void_t<
    redis_sadd_t<_Type>,
    redis_srem_t<_Type>,
    redis_smembers_t<_Type>>> = true;

// redis_does_sorted_set_ops
//   tagless trait: true if _Type supports the full sorted-set
// operation surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_sorted_set_ops = false;

template<typename _Type>
constexpr bool redis_does_sorted_set_ops<_Type, std::void_t<
    redis_zadd_t<_Type>,
    redis_zrem_t<_Type>,
    redis_zrange_t<_Type>>> = true;

// redis_does_stream_ops
//   tagless trait: true if _Type supports the full stream operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_stream_ops = false;

template<typename _Type>
constexpr bool redis_does_stream_ops<_Type, std::void_t<
    redis_xadd_t<_Type>,
    redis_xread_t<_Type>>> = true;

// redis_does_diagnostics
//   tagless trait: true if _Type supports the full diagnostics
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_diagnostics = false;

template<typename _Type>
constexpr bool redis_does_diagnostics<_Type, std::void_t<
    redis_info_t<_Type>,
    redis_client_id_t<_Type>,
    redis_dbsize_t<_Type>,
    redis_ping_t<_Type>>> = true;

// redis_does_cluster_ops
//   tagless trait: true if _Type supports the full cluster operation
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_cluster_ops = false;

template<typename _Type>
constexpr bool redis_does_cluster_ops<_Type, std::void_t<
    redis_cluster_info_t<_Type>,
    redis_cluster_slots_t<_Type>,
    redis_cluster_nodes_t<_Type>>> = true;

// redis_does_persistence
//   tagless trait: true if _Type supports the full persistence
// surface.
template<typename _Type,
         typename = void>
constexpr bool redis_does_persistence = false;

template<typename _Type>
constexpr bool redis_does_persistence<_Type, std::void_t<
    redis_save_t<_Type>,
    redis_bgsave_t<_Type>,
    redis_bgrewriteaof_t<_Type>,
    redis_lastsave_t<_Type>>> = true;

// redis_is_full_connection
//   tagless trait: true if _Type satisfies the complete Redis
// connection interface (command dispatch + key ops + hash ops +
// diagnostics + expiration).
template<typename _Type>
constexpr bool redis_is_full_connection =
    ( redis_does_command_dispatch<clean_t<_Type>> &&
      redis_does_key_ops<clean_t<_Type>>          &&
      redis_does_hash_ops<clean_t<_Type>>         &&
      redis_does_diagnostics<clean_t<_Type>>      &&
      redis_can_expire<clean_t<_Type>> );


// =============================================================================
// XII.  SFINAE HELPERS
// =============================================================================

// enable_if_redis_connection
//   type: SFINAE helper for Redis connection constraints.
template<typename _Type>
using enable_if_redis_connection =
    typename std::enable_if<is_redis_connection<clean_t<_Type>>::value>::type;

// enable_if_has_redis_pubsub
//   type: SFINAE helper for Redis PUB/SUB constraints.
template<typename _Type>
using enable_if_has_redis_pubsub =
    typename std::enable_if<has_redis_pubsub<clean_t<_Type>>::value>::type;

// enable_if_has_redis_transactions
//   type: SFINAE helper for Redis transaction constraints.
template<typename _Type>
using enable_if_has_redis_transactions =
    typename std::enable_if<has_redis_transactions<clean_t<_Type>>::value>::type;

// enable_if_has_redis_pipelining
//   type: SFINAE helper for Redis pipelining constraints.
template<typename _Type>
using enable_if_has_redis_pipelining =
    typename std::enable_if<has_redis_pipelining<clean_t<_Type>>::value>::type;

// enable_if_has_redis_scripting
//   type: SFINAE helper for Redis Lua scripting constraints.
template<typename _Type>
using enable_if_has_redis_scripting =
    typename std::enable_if<has_redis_scripting<clean_t<_Type>>::value>::type;

// enable_if_has_redis_cluster_ops
//   type: SFINAE helper for Redis cluster operation constraints.
template<typename _Type>
using enable_if_has_redis_cluster_ops =
    typename std::enable_if<has_redis_cluster_ops<clean_t<_Type>>::value>::type;


// ===========================================================================
// XIII.   C++20 CONCEPTS
// ===========================================================================
//   The Redis classification concepts, folded in from the former
// redis_concepts.hpp.  Each forwards to a trait / tagless capability declared
// above.  Gated on concept support so the traits remain usable at the C++17
// baseline (matching functor.hpp / monoid.hpp).

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =============================================================================
// A.   Core Redis Connection Concepts
// =============================================================================

// Redis_connection
//   concept: constrains types implementing the Redis connection
// interface. Suffixed with `_c` to avoid clashing with the
// `redis_connection` class type.
template<typename _Type>
concept Redis_connection =
    is_redis_connection<clean_t<_Type>>::value;

// non_redis_connection
//   concept: constrains types that do not implement the Redis
// connection interface.
template<typename _Type>
concept non_redis_connection =
    !Redis_connection<_Type>;

// redis_command_dispatch_connection
//   concept: constrains Redis connections supporting core command
// dispatch (send_command + execute_command + get_reply).
template<typename _Type>
concept redis_command_dispatch_connection =
    has_redis_command_dispatch<clean_t<_Type>>::value;

// redis_pubsub_connection
//   concept: constrains Redis connections supporting PUB/SUB
// messaging.
template<typename _Type>
concept redis_pubsub_connection =
    has_redis_pubsub<clean_t<_Type>>::value;

// redis_pattern_pubsub_connection
//   concept: constrains Redis connections supporting pattern-based
// PUB/SUB (PSUBSCRIBE / PUNSUBSCRIBE).
template<typename _Type>
concept redis_pattern_pubsub_connection =
    has_redis_pattern_pubsub<clean_t<_Type>>::value;

// redis_transactional_connection
//   concept: constrains Redis connections supporting multi-key
// transactions (MULTI / EXEC / DISCARD / WATCH / UNWATCH).
template<typename _Type>
concept redis_transactional_connection =
    has_redis_transactions<clean_t<_Type>>::value;

// redis_pipelined_connection
//   concept: constrains Redis connections supporting pipelining.
template<typename _Type>
concept redis_pipelined_connection =
    has_redis_pipelining<clean_t<_Type>>::value;

// redis_scripting_connection
//   concept: constrains Redis connections supporting Lua scripting
// (EVAL / EVALSHA / SCRIPT LOAD).
template<typename _Type>
concept redis_scripting_connection =
    has_redis_scripting<clean_t<_Type>>::value;

// redis_key_ops_connection
//   concept: constrains Redis connections supporting core key-space
// operations (GET / SET / DEL / EXISTS).
template<typename _Type>
concept redis_key_ops_connection =
    has_redis_key_ops<clean_t<_Type>>::value;

// redis_expiration_connection
//   concept: constrains Redis connections supporting key expiration
// (EXPIRE / TTL).
template<typename _Type>
concept redis_expiration_connection =
    has_redis_expiration<clean_t<_Type>>::value;

// redis_iterable_connection
//   concept: constrains Redis connections supporting key-space
// iteration (KEYS / SCAN).
template<typename _Type>
concept redis_iterable_connection =
    has_redis_key_iteration<clean_t<_Type>>::value;

// redis_hash_connection
//   concept: constrains Redis connections supporting hash operations.
template<typename _Type>
concept redis_hash_connection =
    has_redis_hash_ops<clean_t<_Type>>::value;

// redis_list_connection
//   concept: constrains Redis connections supporting list operations.
template<typename _Type>
concept redis_list_connection =
    has_redis_list_ops<clean_t<_Type>>::value;

// redis_set_connection
//   concept: constrains Redis connections supporting set operations.
template<typename _Type>
concept redis_set_connection =
    has_redis_set_ops<clean_t<_Type>>::value;

// redis_sorted_set_connection
//   concept: constrains Redis connections supporting sorted set
// operations.
template<typename _Type>
concept redis_sorted_set_connection =
    has_redis_sorted_set_ops<clean_t<_Type>>::value;

// redis_stream_connection
//   concept: constrains Redis connections supporting stream
// operations (Redis 5.0+).
template<typename _Type>
concept redis_stream_connection =
    has_redis_stream_ops<clean_t<_Type>>::value;

// redis_diagnostics_connection
//   concept: constrains Redis connections supporting server
// diagnostics (INFO / CLIENT ID / DBSIZE / PING).
template<typename _Type>
concept redis_diagnostics_connection =
    has_redis_diagnostics<clean_t<_Type>>::value;

// redis_cluster_connection
//   concept: constrains Redis connections supporting cluster
// operations (CLUSTER INFO / SLOTS / NODES).
template<typename _Type>
concept redis_cluster_connection =
    has_redis_cluster_ops<clean_t<_Type>>::value;

// redis_persistent_connection
//   concept: constrains Redis connections supporting persistence
// operations (SAVE / BGSAVE / BGREWRITEAOF / LASTSAVE).
template<typename _Type>
concept redis_persistent_connection =
    has_redis_persistence<clean_t<_Type>>::value;

// redis_db_selectable_connection
//   concept: constrains Redis connections supporting logical
// database selection (SELECT).
template<typename _Type>
concept redis_db_selectable_connection =
    has_redis_db_selection<clean_t<_Type>>::value;


// =============================================================================
// B.  Redis Capability Concepts
// =============================================================================

// redis_send_command_connection
//   concept: constrains types exposing send_command(string).
template<typename _Type>
concept redis_send_command_connection =
    redis_can_send_command<clean_t<_Type>>;

// redis_publishable_connection
//   concept: constrains types exposing publish(channel, message).
template<typename _Type>
concept redis_publishable_connection =
    redis_can_publish<clean_t<_Type>>;

// redis_subscribable_connection
//   concept: constrains types exposing subscribe(channel).
template<typename _Type>
concept redis_subscribable_connection =
    redis_can_subscribe<clean_t<_Type>>;

// redis_multi_capable_connection
//   concept: constrains types exposing multi().
template<typename _Type>
concept redis_multi_capable_connection =
    redis_can_multi<clean_t<_Type>>;

// redis_watchable_connection
//   concept: constrains types exposing watch(key).
template<typename _Type>
concept redis_watchable_connection =
    redis_can_watch<clean_t<_Type>>;

// redis_pipelineable_connection
//   concept: constrains types exposing pipeline_start().
template<typename _Type>
concept redis_pipelineable_connection =
    redis_can_pipeline<clean_t<_Type>>;

// redis_eval_capable_connection
//   concept: constrains types exposing eval(script, keys, args).
template<typename _Type>
concept redis_eval_capable_connection =
    redis_can_eval<clean_t<_Type>>;

// redis_get_capable_connection
//   concept: constrains types exposing get(key).
template<typename _Type>
concept redis_get_capable_connection =
    redis_can_get<clean_t<_Type>>;

// redis_set_capable_connection
//   concept: constrains types exposing set(key, value).
template<typename _Type>
concept redis_set_capable_connection =
    redis_can_set<clean_t<_Type>>;

// redis_expirable_connection
//   concept: constrains types exposing expire(key, seconds).
template<typename _Type>
concept redis_expirable_connection =
    redis_can_expire<clean_t<_Type>>;

// redis_scannable_connection
//   concept: constrains types exposing scan(cursor, pattern).
template<typename _Type>
concept redis_scannable_connection =
    redis_can_scan<clean_t<_Type>>;

// redis_hset_capable_connection
//   concept: constrains types exposing hset(key, field, value).
template<typename _Type>
concept redis_hset_capable_connection =
    redis_can_hset<clean_t<_Type>>;

// redis_xadd_capable_connection
//   concept: constrains types exposing xadd(key, id, fields).
template<typename _Type>
concept redis_xadd_capable_connection =
    redis_can_xadd<clean_t<_Type>>;

// redis_cluster_queryable_connection
//   concept: constrains types exposing cluster_info().
template<typename _Type>
concept redis_cluster_queryable_connection =
    redis_can_cluster_query<clean_t<_Type>>;

// redis_db_switch_connection
//   concept: constrains types exposing select_db(index).
template<typename _Type>
concept redis_db_switch_connection =
    redis_can_select_db<clean_t<_Type>>;


// =============================================================================
// C. Tagless Redis Capability Concepts
// =============================================================================

// redis_command_dispatchable
//   concept: constrains types satisfying the full tagless command-
// dispatch capability set.
template<typename _Type>
concept redis_command_dispatchable =
    redis_does_command_dispatch<clean_t<_Type>>;

// redis_pubsub_capable
//   concept: constrains types satisfying the full tagless PUB/SUB
// capability set.
template<typename _Type>
concept redis_pubsub_capable =
    redis_does_pubsub<clean_t<_Type>>;

// redis_transactional
//   concept: constrains types satisfying the full tagless
// multi-key transaction capability set.
template<typename _Type>
concept redis_transactional =
    redis_does_transactions<clean_t<_Type>>;

// redis_pipelinable
//   concept: constrains types satisfying the full tagless
// pipelining capability set.
template<typename _Type>
concept redis_pipelinable =
    redis_does_pipelining<clean_t<_Type>>;

// redis_scriptable
//   concept: constrains types satisfying the full tagless Lua
// scripting capability set.
template<typename _Type>
concept redis_scriptable =
    redis_does_scripting<clean_t<_Type>>;

// redis_key_addressable
//   concept: constrains types satisfying the full tagless key-space
// operation capability set.
template<typename _Type>
concept redis_key_addressable =
    redis_does_key_ops<clean_t<_Type>>;

// redis_hash_addressable
//   concept: constrains types satisfying the full tagless hash
// operation capability set.
template<typename _Type>
concept redis_hash_addressable =
    redis_does_hash_ops<clean_t<_Type>>;

// redis_list_addressable
//   concept: constrains types satisfying the full tagless list
// operation capability set.
template<typename _Type>
concept redis_list_addressable =
    redis_does_list_ops<clean_t<_Type>>;

// redis_set_addressable
//   concept: constrains types satisfying the full tagless set
// operation capability set.
template<typename _Type>
concept redis_set_addressable =
    redis_does_set_ops<clean_t<_Type>>;

// redis_sorted_set_addressable
//   concept: constrains types satisfying the full tagless sorted-set
// operation capability set.
template<typename _Type>
concept redis_sorted_set_addressable =
    redis_does_sorted_set_ops<clean_t<_Type>>;

// redis_stream_addressable
//   concept: constrains types satisfying the full tagless stream
// operation capability set.
template<typename _Type>
concept redis_stream_addressable =
    redis_does_stream_ops<clean_t<_Type>>;

// redis_diagnostic_capable
//   concept: constrains types satisfying the full tagless
// diagnostics capability set.
template<typename _Type>
concept redis_diagnostic_capable =
    redis_does_diagnostics<clean_t<_Type>>;

// redis_cluster_aware
//   concept: constrains types satisfying the full tagless cluster
// operation capability set.
template<typename _Type>
concept redis_cluster_aware =
    redis_does_cluster_ops<clean_t<_Type>>;

// redis_persistent
//   concept: constrains types satisfying the full tagless
// persistence operation capability set.
template<typename _Type>
concept redis_persistent =
    redis_does_persistence<clean_t<_Type>>;

// redis_full_connection
//   concept: constrains types satisfying the complete tagless
// Redis connection capability set.
template<typename _Type>
concept redis_full_connection =
    redis_is_full_connection<clean_t<_Type>>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_REDIS_
