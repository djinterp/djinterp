/*******************************************************************************
* djinterp [core]                                                      mysql.hpp
*
* djinterp Oracle MySQL connection module:
*   The MySQL connection, mysql_connection, over the MariaDB layer's C++ face
* through mysql_common_connection, which holds the face and implements what
* MySQL and MariaDB share; this leaf adds MySQL's configuration -- its TLS
* modes and versions, connect attributes, its authentication, and session
* tracking -- query attributes, and what the server connected has, asked
* at run time (JSON, the X Protocol, Group Replication, roles, atomic DDL,
* query attributes, the SQL of each release), beside what the client's
* header has, at compile time from env_mysql.h: the client's capabilities.
*   MySQL parts from MariaDB in the X Protocol and X DevAPI (5.7.12), Group
* Replication and InnoDB Cluster where MariaDB has Galera, HeatWave,
* caching_sha2_password by default (8.0.4), its _nonblocking calls (8.0.16)
* where MariaDB's client has _start and _cont, and its data dictionary (8.0).
*     mysql_connection (this file)
*       -> mysql_common_connection<mysql_connection, database_type::mysql>
*         -> database_connection<mysql_connection, database_type::mysql>
*           -> connection_template<mysql_connection, database_type::mysql>
*             -> connection<mysql_connection>
*   The leaf's hooks are defined in mysql.cpp, which instantiates the
* family's members for it. The header needs C++17 and includes no client
* header: env includes mysql.h only where a program asks for env's
* detection (D_CFG_ENV_USING_MYSQL), which the client's capabilities read.
* Such a program, under `using namespace djinterp`, names the enum
* djinterp::mysql_ssl_mode, since libmysqlclient declares one of that name.
* Its trailing sections carry MySQL's capability-detection traits and C++20
* concepts.
*
*
* path:      /inc/djinterp/core/db/mysql/mysql.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.23
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CLIENT CAPABILITIES
    -------------------
    1.  Compile-time capabilities
         1.  mysql_client_capabilities
         2.  mysql_type_support
         3.  mysql_feature_support
2.  VERSION INFORMATION
    -------------------
    1.  The version
         1.  mysql_version_info
3.  CONNECTION CONFIGURATION
    ------------------------
    1.  TLS modes
         1.  mysql_ssl_mode
    2.  The configuration
         1.  oracle_mysql_connect_config
4.  MYSQL CONNECTION
    ----------------
    1.  The class
         1.  mysql_connection
         2.  mysql_common_connection<mysql_connection, ...>
5.  EXPRESSION DETECTORS
    --------------------
    1.  Connection reset
    2.  Session tracking
    3.  Query attributes
    4.  X Protocol
    5.  Schema introspection
6.  TAGGED CAPABILITY TRAITS
    ------------------------
    1.  Traits
         1.  has_mysql_ora_reset
         2.  has_mysql_ora_session_tracking
         3.  has_mysql_ora_query_attributes
         4.  has_mysql_ora_schema_query
         5.  is_mysql_ora_connection
7.  TAGLESS CAPABILITY TRAITS
    -------------------------
    1.  Individual capabilities
    2.  Compound capabilities
8.  SFINAE HELPERS
    --------------
    1.  enable_if helpers
9.  C++20 CONCEPTS
    --------------
    1.  Connection concepts
    2.  Capability concepts
    3.  Tagless capability concepts
    4.  Former names
*/

#ifndef DJINTERP_DB_MYSQL_MYSQL_HPP
#define DJINTERP_DB_MYSQL_MYSQL_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t,
                                                   // uint16_t, uint8_t, int64_t

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>  // std::size_t
#include <map>      // std::map
#include <string>   // std::string
// djinterp
#include "../../../djinterp.hpp"                   // framework root
#include "../../../c/db/mariadb/mariadb_face.hpp"  // face::mariadb
#include "../../../env/db/mysql/env_mysql.h"       // D_ENV_MYSQL_*
#include "../../meta/type_utility.hpp"             // clean_t
#include "./mysql_common.hpp"                      // mysql_common_connection


NS_DJINTERP


//==============================================================================
// 1.  CLIENT CAPABILITIES
//==============================================================================
// What env_mysql.h found MySQL's client header to have, by version, at
// compile time -- the client's capabilities: the data types and the features
// a server of the header's release has, which the server connected may lack,
// or exceed. Every flag is false unless a program asks for env's detection
// (D_CFG_ENV_USING_MYSQL) or supplies its results. What the server connected
// has, mysql_connection's supports_* members ask at run time. The wire
// protocol's MYSQL_TYPE_* codes, as mysql_field_type, and
// mysql_type_to_field_type() are mariadb.hpp's, for the whole family.


// 1.1    Compile-time capabilities
//------------------------------------------------------------------------------
// 1.1.1
// mysql_client_capabilities
//   struct: compile-time data type and feature availability flags for Oracle
// MySQL, as its client header gives them.
struct mysql_client_capabilities
{
#if D_ENV_MYSQL_DETECTED

    // data types
    static constexpr bool has_json_type =
    #if D_ENV_MYSQL_HAS_JSON_TYPE
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_JSON_TYPE

    static constexpr bool has_json_table =
    #if D_ENV_MYSQL_HAS_JSON_TABLE
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_JSON_TABLE

    static constexpr bool has_json_schema_validation =
    #if D_ENV_MYSQL_HAS_JSON_SCHEMA_VALIDATION
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_JSON_SCHEMA_VALIDATION

    static constexpr bool has_json_value =
    #if D_ENV_MYSQL_HAS_JSON_VALUE
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_JSON_VALUE

    static constexpr bool has_json_arrayagg =
    #if D_ENV_MYSQL_HAS_JSON_ARRAYAGG
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_JSON_ARRAYAGG

    static constexpr bool has_multi_value_index =
    #if D_ENV_MYSQL_HAS_MULTI_VALUE_INDEX
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_MULTI_VALUE_INDEX

    static constexpr bool has_generated_columns =
    #if D_ENV_MYSQL_HAS_GENERATED_COLUMNS
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_GENERATED_COLUMNS

    static constexpr bool has_check_constraints =
    #if D_ENV_MYSQL_HAS_CHECK_CONSTRAINTS
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_CHECK_CONSTRAINTS

    static constexpr bool has_invisible_columns =
    #if D_ENV_MYSQL_HAS_INVISIBLE_COLUMNS
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_INVISIBLE_COLUMNS

    static constexpr bool has_functional_index =
    #if D_ENV_MYSQL_HAS_FUNCTIONAL_INDEX
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_FUNCTIONAL_INDEX

    static constexpr bool has_descending_index =
    #if D_ENV_MYSQL_HAS_DESCENDING_INDEX
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_DESCENDING_INDEX

    static constexpr bool has_srid_support =
    #if D_ENV_MYSQL_HAS_SRID_SUPPORT
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_SRID_SUPPORT

    // SQL and optimizer
    static constexpr bool has_window_functions =
    #if D_ENV_MYSQL_HAS_WINDOW_FUNCTIONS
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_WINDOW_FUNCTIONS

    static constexpr bool has_cte =
    #if D_ENV_MYSQL_HAS_CTE
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_CTE

    static constexpr bool has_lateral_derived =
    #if D_ENV_MYSQL_HAS_LATERAL_DERIVED
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_LATERAL_DERIVED

    static constexpr bool has_hash_join =
    #if D_ENV_MYSQL_HAS_HASH_JOIN
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_HASH_JOIN

    static constexpr bool has_histograms =
    #if D_ENV_MYSQL_HAS_HISTOGRAMS
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_HISTOGRAMS

    static constexpr bool has_explain_analyze =
    #if D_ENV_MYSQL_HAS_EXPLAIN_ANALYZE
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_EXPLAIN_ANALYZE


    // X Protocol
    static constexpr bool has_x_protocol =
    #if D_ENV_MYSQL_HAS_X_PROTOCOL
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_X_PROTOCOL

    // replication and HA
    static constexpr bool has_group_replication =
    #if D_ENV_MYSQL_HAS_GROUP_REPLICATION
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_GROUP_REPLICATION

    static constexpr bool has_innodb_cluster =
    #if D_ENV_MYSQL_HAS_INNODB_CLUSTER
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_INNODB_CLUSTER

    static constexpr bool has_clone_plugin =
    #if D_ENV_MYSQL_HAS_CLONE_PLUGIN
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_CLONE_PLUGIN

    static constexpr bool has_gtid =
    #if D_ENV_MYSQL_HAS_GTID
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_GTID

    // InnoDB
    static constexpr bool has_innodb_instant_ddl =
    #if D_ENV_MYSQL_HAS_INNODB_INSTANT_DDL
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_INNODB_INSTANT_DDL

    static constexpr bool has_atomic_ddl =
    #if D_ENV_MYSQL_HAS_ATOMIC_DDL
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_ATOMIC_DDL

    static constexpr bool has_data_dictionary =
    #if D_ENV_MYSQL_HAS_DATA_DICTIONARY
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_DATA_DICTIONARY


    // authentication
    static constexpr bool has_auth_caching_sha2 =
    #if D_ENV_MYSQL_HAS_AUTH_CACHING_SHA2
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_AUTH_CACHING_SHA2

    static constexpr bool has_multi_factor_auth =
    #if D_ENV_MYSQL_HAS_MULTI_FACTOR_AUTH
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_MULTI_FACTOR_AUTH


    // security
    static constexpr bool has_roles =
    #if D_ENV_MYSQL_HAS_ROLES
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_ROLES

    static constexpr bool has_partial_revoke =
    #if D_ENV_MYSQL_HAS_PARTIAL_REVOKE
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_PARTIAL_REVOKE


    // C API
    static constexpr bool has_async_api =
    #if D_ENV_MYSQL_HAS_ASYNC_API
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_ASYNC_API

    static constexpr bool has_reset_connection =
    #if D_ENV_MYSQL_HAS_RESET_CONNECTION
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_RESET_CONNECTION

    static constexpr bool has_session_track =
    #if D_ENV_MYSQL_HAS_SESSION_TRACK
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_SESSION_TRACK

    static constexpr bool has_query_attrs =
    #if D_ENV_MYSQL_HAS_QUERY_ATTRS
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_QUERY_ATTRS


    // composite
    static constexpr bool has_modern_sql =
    #if D_ENV_MYSQL_HAS_MODERN_SQL
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_MODERN_SQL

    static constexpr bool has_modern_json =
    #if D_ENV_MYSQL_HAS_MODERN_JSON
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_MODERN_JSON

    static constexpr bool has_modern_ddl =
    #if D_ENV_MYSQL_HAS_MODERN_DDL
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_MODERN_DDL

    static constexpr bool has_ha_suite =
    #if D_ENV_MYSQL_HAS_HA_SUITE
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_HAS_HA_SUITE

    static constexpr bool is_fully_modern =
    #if D_ENV_MYSQL_IS_FULLY_MODERN
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_IS_FULLY_MODERN

    // release model
    static constexpr bool is_lts =
    #if D_ENV_MYSQL_IS_LTS
        true;
    #else
        false;
    #endif  // D_ENV_MYSQL_IS_LTS

#else
    static constexpr bool has_json_type              = false;
    static constexpr bool has_json_table             = false;
    static constexpr bool has_json_schema_validation = false;
    static constexpr bool has_json_value             = false;
    static constexpr bool has_json_arrayagg          = false;
    static constexpr bool has_multi_value_index      = false;
    static constexpr bool has_generated_columns      = false;
    static constexpr bool has_check_constraints      = false;
    static constexpr bool has_invisible_columns      = false;
    static constexpr bool has_functional_index       = false;
    static constexpr bool has_descending_index       = false;
    static constexpr bool has_srid_support           = false;
    static constexpr bool has_window_functions       = false;
    static constexpr bool has_cte                    = false;
    static constexpr bool has_lateral_derived        = false;
    static constexpr bool has_hash_join              = false;
    static constexpr bool has_histograms             = false;
    static constexpr bool has_explain_analyze        = false;
    static constexpr bool has_x_protocol             = false;
    static constexpr bool has_group_replication      = false;
    static constexpr bool has_innodb_cluster         = false;
    static constexpr bool has_clone_plugin           = false;
    static constexpr bool has_gtid                   = false;
    static constexpr bool has_innodb_instant_ddl     = false;
    static constexpr bool has_atomic_ddl             = false;
    static constexpr bool has_data_dictionary        = false;
    static constexpr bool has_auth_caching_sha2      = false;
    static constexpr bool has_multi_factor_auth      = false;
    static constexpr bool has_roles                  = false;
    static constexpr bool has_partial_revoke         = false;
    static constexpr bool has_async_api              = false;
    static constexpr bool has_reset_connection       = false;
    static constexpr bool has_session_track          = false;
    static constexpr bool has_query_attrs            = false;
    static constexpr bool has_modern_sql             = false;
    static constexpr bool has_modern_json            = false;
    static constexpr bool has_modern_ddl             = false;
    static constexpr bool has_ha_suite               = false;
    static constexpr bool is_fully_modern            = false;
    static constexpr bool is_lts                     = false;
#endif
};

// 1.1.2
// mysql_type_support
//   type: mysql_client_capabilities, by its former name (2026.10.21), for one
// release; it held the data types.
using mysql_type_support [[deprecated("mysql_client_capabilities")]] =
    mysql_client_capabilities;

// 1.1.3
// mysql_feature_support
//   type: mysql_client_capabilities, by its former name (2026.10.21), for one
// release; it held the features.
using mysql_feature_support [[deprecated("mysql_client_capabilities")]] =
    mysql_client_capabilities;


//==============================================================================
// 2.  VERSION INFORMATION
//==============================================================================


// 2.1    The version
//------------------------------------------------------------------------------
// 2.1.1
// mysql_version_info
//   struct: compile-time version decomposition of the client header env
// detected, for Oracle MySQL; the server's is the connection's to ask
// (get_server_version).
// uses MAJOR*10000 + MINOR*100 + PATCH encoding (same as the
// MYSQL_VERSION_ID macro from mysql_version.h).
struct mysql_version_info
{
#if D_ENV_MYSQL_DETECTED
    static constexpr bool             detected = true;
    static constexpr re_std::uint32_t id       = D_ENV_MYSQL_VERSION_ID;
    static constexpr re_std::uint16_t major    = D_ENV_MYSQL_VERSION_MAJOR;
    static constexpr re_std::uint16_t minor    = D_ENV_MYSQL_VERSION_MINOR;
    static constexpr re_std::uint16_t patch    = D_ENV_MYSQL_VERSION_PATCH;
    static constexpr const char*      string   = D_ENV_MYSQL_VERSION_STRING;
#else
    static constexpr bool             detected = false;
    static constexpr re_std::uint32_t id       = 0;
    static constexpr re_std::uint16_t major    = 0;
    static constexpr re_std::uint16_t minor    = 0;
    static constexpr re_std::uint16_t patch    = 0;
    static constexpr const char*      string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected MySQL version is at
    // least (major, minor, patch).
    static constexpr bool
    at_least(
        re_std::uint16_t _major,
        re_std::uint16_t _minor,
        re_std::uint16_t _patch
    ) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


//==============================================================================
// 3.  CONNECTION CONFIGURATION
//==============================================================================


// 3.1    TLS modes
//------------------------------------------------------------------------------
// 3.1.1
// mysql_ssl_mode
//   enum: MySQL SSL connection modes (mysql_ssl_mode from mysql.h).
enum class mysql_ssl_mode : re_std::uint8_t
{
    disabled        = 1,    // SSL_MODE_DISABLED
    preferred       = 2,    // SSL_MODE_PREFERRED
    required        = 3,    // SSL_MODE_REQUIRED
    verify_ca       = 4,    // SSL_MODE_VERIFY_CA
    verify_identity = 5     // SSL_MODE_VERIFY_IDENTITY
};

// 3.2    The configuration
//------------------------------------------------------------------------------
// 3.2.1
// oracle_mysql_connect_config
//   struct: what a MySQL connection needs beyond the family's. ssl_mode,
// where not `preferred`, replaces enable_ssl's mode; connect_attributes join
// the family's. tls_version ("TLSv1.2,TLSv1.3") and tls_ciphersuites (TLS
// 1.3's), auth_plugin (the plugin the client answers with first, before the
// server names its own) and enable_cleartext_plugin (a password sent in clear
// text, when the server's authentication asks for it) go to the client's
// own options, where it has them, and connect refuses them, with
// D_DB_UNSUPPORTED, where it has not: tls_ciphersuites needs libmysqlclient
// from 8.0.16; Connector/C reads a TLS version it does not know as none
// asked for, and sends a clear-text password unasked, whatever
// enable_cleartext_plugin says. server_public_key names a file holding the
// server's RSA public key, with which caching_sha2_password and
// sha256_password encrypt the password where the connection has no TLS;
// get_server_public_key lets the client ask the server for it instead --
// libmysqlclient from 8.0.3, which otherwise does not, and Connector/C,
// which always does. enable_session_tracking has the server
// report every kind of session state it tracks (get_session_track_info):
// the schema, state changes, every system variable, the transaction's
// characteristics and the session's own GTIDs, which a server before 5.7.8
// refuses. use_x_protocol and statement_cache_size neither client has:
// connect refuses them unless they are left as they are made.
struct oracle_mysql_connect_config
{
    mysql_connect_config common;
    mysql_ssl_mode       ssl_mode;                 // preferred
    std::string          auth_plugin;              // "": the client's own
    std::string          tls_version;              // "": the client's own
    std::string          tls_ciphersuites;         // "": the client's own
    re_std::uint16_t     x_protocol_port;          // 33060
    bool                 use_x_protocol;           // refused unless false
    bool                 enable_session_tracking;  // false: the server's own
    bool                 enable_cleartext_plugin;  // false
    std::string          server_public_key;        // "": none
    bool                 get_server_public_key;    // false: the client's own
    std::size_t          statement_cache_size;     // refused unless 0

    std::map<std::string, std::string> connect_attributes;

    // construction -- MySQL's defaults (localhost, 3306), over a generic
    // configuration's
    oracle_mysql_connect_config()
        : ssl_mode(mysql_ssl_mode::preferred),
          x_protocol_port(33060),
          use_x_protocol(false),
          enable_session_tracking(false),
          enable_cleartext_plugin(false),
          get_server_public_key(false),
          statement_cache_size(0)
    {
        common.base.host = "localhost";
        common.base.port = 3306;
    }

    explicit oracle_mysql_connect_config(
        const connection_config& _base
    )
        : ssl_mode(mysql_ssl_mode::preferred),
          x_protocol_port(33060),
          use_x_protocol(false),
          enable_session_tracking(false),
          enable_cleartext_plugin(false),
          get_server_public_key(false),
          statement_cache_size(0)
    {
        common.base = _base;
    }
};


//==============================================================================
// 4.  MYSQL CONNECTION
//==============================================================================


// 4.1    The class
//------------------------------------------------------------------------------
// 4.1.1
// mysql_connection
//   class: a connection to Oracle's MySQL, through either client, over the
// MariaDB layer's face (mysql_common_connection, which holds it). Beside the
// family's operations it says what the server connected has (supports_*),
// sets query attributes, and applies oracle_mysql_connect_config's TLS mode
// and connect attributes when it connects. Its members throw as the
// family's do. Not copyable; movable.
class mysql_connection : public mysql_common_connection<mysql_connection,
                                                       database_type::mysql>
{
public:
    using base_type = mysql_common_connection<mysql_connection,
                                              database_type::mysql>;

    using client_capabilities = mysql_client_capabilities;
    using version_info        = mysql_version_info;

    // type_support, feature_support -- client_capabilities, by their former
    // names (2026.10.21), for one release
    using type_support [[deprecated("client_capabilities")]] =
        mysql_client_capabilities;
    using feature_support [[deprecated("client_capabilities")]] =
        mysql_client_capabilities;

    // construction -- with MySQL's defaults, or a generic, a family or a
    // MySQL configuration, whose `common` the family keeps
    mysql_connection();
    explicit mysql_connection(const connection_config& _config);
    explicit mysql_connection(const mysql_connect_config& _config);
    explicit mysql_connection(const oracle_mysql_connect_config& _config);

    ~mysql_connection() = default;

    mysql_connection(const mysql_connection&)            = delete;
    mysql_connection& operator=(const mysql_connection&) = delete;

    mysql_connection(mysql_connection&&) noexcept            = default;
    mysql_connection& operator=(mysql_connection&&) noexcept = default;

    // feature queries -- what the server connected has, by its flavor and
    // release, asked of the layer at run time (d_mariadb_supports); false
    // while the connection is not open. client_capabilities answers for the
    // client's header at compile time. What a plugin or a setting adds -- the
    // X Plugin, Group Replication, on which InnoDB Cluster is built -- is
    // whether the server's release has it; whether this server runs it is
    // its configuration. supports_modern_sql is window functions, CTEs and
    // LATERAL together (on a MariaDB server, its own set: window functions,
    // CTEs, sequences and system versioning); is_lts_release, a release of a
    // long-term-support series: MySQL's 8.4 and 9.7, MariaDB's 10.6, 10.11,
    // 11.4, 11.8 and 12.3
    bool
    supports_json() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_JSON);
    }

    bool
    supports_x_protocol() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_X_PROTOCOL);
    }

    bool
    supports_window_functions() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_WINDOW_FUNCTIONS);
    }

    bool
    supports_cte() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_CTE);
    }

    bool
    supports_group_replication() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_GROUP_REPLICATION);
    }

    bool
    supports_innodb_cluster() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_GROUP_REPLICATION);
    }

    bool
    supports_roles() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_ROLES);
    }

    bool
    supports_atomic_ddl() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_ATOMIC_DDL);
    }

    bool
    supports_modern_sql() const noexcept
    {
        return this->m_supports_modern_sql();
    }

    // supports_query_attributes -- the server from 8.0.23, and a client
    // with them: libmysqlclient from 8.0.23, never Connector/C
    bool
    supports_query_attributes() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_QUERY_ATTRIBUTES);
    }

    bool is_lts_release() const noexcept;

    /**
     * @brief Sets an attribute the next statement the connection sends
     *        carries (mysql_bind_param), which the server's query_attributes
     *        component reads with mysql_query_attribute_string(); setting a
     *        name again replaces its value.
     *
     * @note The next statement is whatever the connection sends next, its
     *       own among them -- has_table's query, set_charset's SET NAMES --
     *       which drops every attribute set, whether it succeeds or not. A
     *       prepared statement's execution neither carries nor drops them;
     *       a reset and a change of user drop them.
     *
     * @param[in] _name   the attribute's name; not empty.
     * @param[in] _value  its value, sent as a string.
     * @throws query_exception with D_DB_UNSUPPORTED where the server is
     *         older than 8.0.23 or the client has no query attributes
     *         (supports_query_attributes); with D_DB_INVALID_ARGUMENT for an
     *         empty name, or a NUL in either; with the client's number when
     *         it refuses, the attributes then as they were.
     */
    void set_query_attribute(const std::string& _name,
                             const std::string& _value);

    // clear_query_attributes -- drops those set and not yet sent; throws
    // only as every operation does, when the connection is not open
    void clear_query_attributes();

    // configuration -- MySQL's settings; setting them replaces the
    // family's and the generic configuration with theirs, for the next
    // connect
    oracle_mysql_connect_config
         get_oracle_config() const;
    void set_oracle_config(const oracle_mysql_connect_config& _config);

    // hooks (mysql.cpp), for mysql_common_connection's connect: MySQL's
    // settings (oracle_mysql_connect_config), and the refusal of those no
    // client can apply, which throws connection_exception with
    // D_DB_UNSUPPORTED; nothing to set once connected
    void add_settings_impl(internal::mariadb_settings& _settings) const;
    void on_connected_impl(face::mariadb& _db);

private:
    oracle_mysql_connect_config m_oracle_config;
};

// 4.1.2
// mysql_common_connection<mysql_connection, ...>
//   class: the family's members for MySQL, instantiated once, in mysql.cpp.
extern template class mysql_common_connection<mysql_connection,
                                              database_type::mysql>;



//==============================================================================
// 5.  EXPRESSION DETECTORS
//==============================================================================
// Sections 5 to 9, once mysql_traits.hpp and mysql_concepts.hpp, detect
// which of MySQL's own members a type has, beside the family's
// (mysql_common.hpp): as traits from C++17 and as concepts under C++20, all
// prefixed mysql_ora_. A detector is the type of one member's call, which
// is_detected tests.


// 5.1    Connection reset
//------------------------------------------------------------------------------
// mysql_ora_reset_connection_t
//   detector: reset_connection() method.
// wraps mysql_reset_connection() introduced in Oracle MySQL 5.7.3.
// note: MariaDB has the same function but at a different version gate
// (10.2.4); the detector expression is identical but the version-gated
// availability differs.
template<typename Type>
using mysql_ora_reset_connection_t =
    decltype(std::declval<Type&>().reset_connection());

// 5.2    Session tracking
//------------------------------------------------------------------------------
// mysql_ora_get_session_track_info_t
//   detector: get_session_track_info(mysql_session_track) const method.
// wraps mysql_session_track_get_first/next() introduced in 5.7.4.
template<typename Type>
using mysql_ora_get_session_track_info_t =
    decltype(std::declval<const Type&>().get_session_track_info(
        std::declval<mysql_session_track>()));

// 5.3    Query attributes
//------------------------------------------------------------------------------
// mysql_ora_set_query_attribute_t
//   detector: set_query_attribute(const std::string&,
// const std::string&) method.
// wraps mysql_bind_param() for query attributes introduced in 8.0.23.
template<typename Type>
using mysql_ora_set_query_attribute_t =
    decltype(std::declval<Type&>().set_query_attribute(
        std::declval<const std::string&>(),
        std::declval<const std::string&>()));

// 5.4    X Protocol
//------------------------------------------------------------------------------
// mysql_ora_has_x_protocol_t
//   detector: supports_x_protocol() const method.
template<typename Type>
using mysql_ora_has_x_protocol_t =
    decltype(std::declval<const Type&>().supports_x_protocol());

// 5.5    Schema introspection
//------------------------------------------------------------------------------
// mysql_ora_has_table_t
//   detector: has_table(const std::string&) const method.
template<typename Type>
using mysql_ora_has_table_t =
    decltype(std::declval<const Type&>().has_table(
        std::declval<const std::string&>()));

// mysql_ora_list_tables_t
//   detector: list_tables() const method.
template<typename Type>
using mysql_ora_list_tables_t =
    decltype(std::declval<const Type&>().list_tables());


//==============================================================================
// 6.  TAGGED CAPABILITY TRAITS
//==============================================================================
// Each trait is a type with ::value, true where every detector it names finds
// its member; name_v is the value, where the language has variable templates.


// 6.1    Traits
//------------------------------------------------------------------------------
// 6.1.1
// has_mysql_ora_reset
//   trait: checks if type Type supports connection reset.
template<typename Type>
struct has_mysql_ora_reset
    : is_detected<mysql_ora_reset_connection_t, clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_mysql_ora_reset_v =
        has_mysql_ora_reset<clean_t<Type>>::value;
#endif

// 6.1.2
// has_mysql_ora_session_tracking
//   trait: checks if type Type supports session tracking.
template<typename Type>
struct has_mysql_ora_session_tracking
    : is_detected<mysql_ora_get_session_track_info_t, clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_mysql_ora_session_tracking_v =
        has_mysql_ora_session_tracking<clean_t<Type>>::value;
#endif

// 6.1.3
// has_mysql_ora_query_attributes
//   trait: checks if type Type supports query attributes.
template<typename Type>
struct has_mysql_ora_query_attributes
    : is_detected<mysql_ora_set_query_attribute_t, clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_mysql_ora_query_attributes_v =
        has_mysql_ora_query_attributes<clean_t<Type>>::value;
#endif

// 6.1.4
// has_mysql_ora_schema_query
//   trait: checks if type Type supports schema introspection.
template<typename Type>
struct has_mysql_ora_schema_query : djinterp::conjunction<
    is_detected<mysql_ora_has_table_t, clean_t<Type>>,
    is_detected<mysql_ora_list_tables_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_mysql_ora_schema_query_v =
        has_mysql_ora_schema_query<clean_t<Type>>::value;
#endif

// 6.1.5
// is_mysql_ora_connection
//   trait: compound trait verifying type Type implements an Oracle MySQL
// connection interface. Extends is_mysql_connection with schema query
// support.
template<typename Type>
struct is_mysql_ora_connection : djinterp::conjunction<
    is_mysql_connection<clean_t<Type>>,
    has_mysql_ora_schema_query<clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_mysql_ora_connection_v =
        is_mysql_ora_connection<clean_t<Type>>::value;
#endif


//==============================================================================
// 7.  TAGLESS CAPABILITY TRAITS
//==============================================================================
// The same capabilities as constexpr bool variable templates, for a condition
// that needs no type: one member each, then the sets.


// 7.1    Individual capabilities
//------------------------------------------------------------------------------
// mysql_ora_can_reset_connection
//   tagless trait: true if Type has reset_connection().
template<typename Type,
         typename = void>
constexpr bool mysql_ora_can_reset_connection = false;

template<typename Type>
constexpr bool mysql_ora_can_reset_connection<Type,
    std::void_t<mysql_ora_reset_connection_t<Type>>> = true;

// mysql_ora_can_track_session
//   tagless trait: true if Type has get_session_track_info().
template<typename Type,
         typename = void>
constexpr bool mysql_ora_can_track_session = false;

template<typename Type>
constexpr bool mysql_ora_can_track_session<Type,
    std::void_t<mysql_ora_get_session_track_info_t<Type>>> = true;

// mysql_ora_can_set_query_attribute
//   tagless trait: true if Type has set_query_attribute().
template<typename Type,
         typename = void>
constexpr bool mysql_ora_can_set_query_attribute = false;

template<typename Type>
constexpr bool mysql_ora_can_set_query_attribute<Type,
    std::void_t<mysql_ora_set_query_attribute_t<Type>>> = true;

// mysql_ora_can_query_schema
//   tagless trait: true if Type has has_table().
template<typename Type,
         typename = void>
constexpr bool mysql_ora_can_query_schema = false;

template<typename Type>
constexpr bool mysql_ora_can_query_schema<Type,
    std::void_t<mysql_ora_has_table_t<Type>>> = true;

// 7.2    Compound capabilities
//------------------------------------------------------------------------------
// mysql_ora_does_schema_query
//   tagless trait: true if Type supports full schema introspection.
template<typename Type,
         typename = void>
constexpr bool mysql_ora_does_schema_query = false;

template<typename Type>
constexpr bool mysql_ora_does_schema_query<Type, std::void_t<
    mysql_ora_has_table_t<Type>,
    mysql_ora_list_tables_t<Type>>> = true;

// mysql_ora_is_full_connection
//   tagless trait: true if Type satisfies the complete Oracle MySQL
// connection interface.
template<typename Type>
constexpr bool mysql_ora_is_full_connection =
    ( mysql_is_full_connection<clean_t<Type>> &&
      mysql_ora_can_query_schema<clean_t<Type>> );


//==============================================================================
// 8.  SFINAE HELPERS
//==============================================================================


// 8.1    enable_if helpers
//------------------------------------------------------------------------------
// enable_if_mysql_ora_connection
//   type: SFINAE helper for Oracle MySQL connection constraints.
template<typename Type>
using enable_if_mysql_ora_connection =
    typename std::enable_if<
        is_mysql_ora_connection<clean_t<Type>>::value>::type;

// enable_if_has_mysql_ora_reset
//   type: SFINAE helper for Oracle MySQL reset_connection constraints.
template<typename Type>
using enable_if_has_mysql_ora_reset =
    typename std::enable_if<has_mysql_ora_reset<clean_t<Type>>::value>::type;

// enable_if_has_mysql_ora_session_tracking
//   type: SFINAE helper for Oracle MySQL session tracking constraints.
template<typename Type>
using enable_if_has_mysql_ora_session_tracking =
    typename std::enable_if<
        has_mysql_ora_session_tracking<clean_t<Type>>::value>::type;

// enable_if_has_mysql_ora_query_attributes
//   type: SFINAE helper for Oracle MySQL query attribute constraints.
template<typename Type>
using enable_if_has_mysql_ora_query_attributes =
    typename std::enable_if<
        has_mysql_ora_query_attributes<clean_t<Type>>::value>::type;


//==============================================================================
// 9.  C++20 CONCEPTS
//==============================================================================
// Each concept forwards to a trait or a tagless capability above. They live
// in djinterp::concepts (2026.10.21), where a concept may take the name of
// the class it constrains; each former name in djinterp forwards to its
// concept for one release. They exist where the compiler has concepts; the
// traits serve at C++17 without them, as in functor.hpp and monoid.hpp. The
// family's concepts are mysql_common.hpp's (mysql_family_connection,
// non_mysql_connection, mysql_full_connection), so MySQL's own take the
// mysql_ora_ prefix its traits use.


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace concepts
{

// 9.1    Connection concepts
//------------------------------------------------------------------------------
// mysql_ora_connection
//   concept: constrains types implementing the Oracle MySQL connection
// interface. (The family's is mysql_family_connection, mysql_common.hpp's.)
template<typename Type>
concept mysql_ora_connection =
    is_mysql_ora_connection<clean_t<Type>>::value;

// non_mysql_ora_connection
//   concept: constrains types that do not implement the Oracle MySQL
// connection interface.
template<typename Type>
concept non_mysql_ora_connection =
    !mysql_ora_connection<Type>;

// mysql_ora_schema_connection
//   concept: constrains Oracle MySQL connections supporting schema
// introspection.
template<typename Type>
concept mysql_ora_schema_connection =
    has_mysql_ora_schema_query<clean_t<Type>>::value;

// mysql_ora_resettable_connection
//   concept: constrains Oracle MySQL connections supporting connection
// reset.
template<typename Type>
concept mysql_ora_resettable_connection =
    has_mysql_ora_reset<clean_t<Type>>::value;

// mysql_ora_session_tracking_connection
//   concept: constrains Oracle MySQL connections supporting session
// tracking.
template<typename Type>
concept mysql_ora_session_tracking_connection =
    has_mysql_ora_session_tracking<clean_t<Type>>::value;

// mysql_ora_query_attributes_connection
//   concept: constrains Oracle MySQL connections supporting query
// attributes.
template<typename Type>
concept mysql_ora_query_attributes_connection =
    has_mysql_ora_query_attributes<clean_t<Type>>::value;

// 9.2    Capability concepts
//------------------------------------------------------------------------------
// mysql_ora_reset_connection_capable
//   concept: constrains types exposing reset_connection().
template<typename Type>
concept mysql_ora_reset_connection_capable =
    mysql_ora_can_reset_connection<clean_t<Type>>;

// mysql_ora_session_trackable
//   concept: constrains types exposing get_session_track_info(kind).
template<typename Type>
concept mysql_ora_session_trackable =
    mysql_ora_can_track_session<clean_t<Type>>;

// mysql_ora_query_attribute_settable
//   concept: constrains types exposing set_query_attribute(name, value).
template<typename Type>
concept mysql_ora_query_attribute_settable =
    mysql_ora_can_set_query_attribute<clean_t<Type>>;

// mysql_ora_has_table_query
//   concept: constrains types exposing has_table(name).
template<typename Type>
concept mysql_ora_has_table_query =
    mysql_ora_can_query_schema<clean_t<Type>>;

// mysql_ora_table_names_query
//   concept: constrains types exposing list_tables().
template<typename Type>
concept mysql_ora_table_names_query =
    is_detected<mysql_ora_list_tables_t, clean_t<Type>>::value;

// mysql_ora_x_protocol_query
//   concept: constrains types exposing supports_x_protocol().
template<typename Type>
concept mysql_ora_x_protocol_query =
    is_detected<mysql_ora_has_x_protocol_t, clean_t<Type>>::value;

// 9.3    Tagless capability concepts
//------------------------------------------------------------------------------
// mysql_ora_schema_queryable
//   concept: constrains types satisfying the full tagless schema-query
// capability set.
template<typename Type>
concept mysql_ora_schema_queryable =
    mysql_ora_does_schema_query<clean_t<Type>>;

// mysql_ora_full_connection
//   concept: constrains types satisfying the complete tagless Oracle MySQL
// connection capability set. (The family's is mysql_full_connection,
// mysql_common.hpp's.)
template<typename Type>
concept mysql_ora_full_connection =
    mysql_ora_is_full_connection<clean_t<Type>>;

}  // namespace concepts

// 9.4    Former names
//------------------------------------------------------------------------------
// mysql_ora_connection
//   concept: concepts::mysql_ora_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_connection =
    concepts::mysql_ora_connection<Type>;

// non_mysql_ora_connection
//   concept: concepts::non_mysql_ora_connection, by its former name, for one
// release.
template<typename Type>
concept non_mysql_ora_connection =
    concepts::non_mysql_ora_connection<Type>;

// mysql_ora_schema_connection
//   concept: concepts::mysql_ora_schema_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_schema_connection =
    concepts::mysql_ora_schema_connection<Type>;

// mysql_ora_resettable_connection
//   concept: concepts::mysql_ora_resettable_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_ora_resettable_connection =
    concepts::mysql_ora_resettable_connection<Type>;

// mysql_ora_session_tracking_connection
//   concept: concepts::mysql_ora_session_tracking_connection, by its former
// name, for one release.
template<typename Type>
concept mysql_ora_session_tracking_connection =
    concepts::mysql_ora_session_tracking_connection<Type>;

// mysql_ora_query_attributes_connection
//   concept: concepts::mysql_ora_query_attributes_connection, by its former
// name, for one release.
template<typename Type>
concept mysql_ora_query_attributes_connection =
    concepts::mysql_ora_query_attributes_connection<Type>;

// mysql_ora_reset_connection_capable
//   concept: concepts::mysql_ora_reset_connection_capable, by its former name,
// for one release.
template<typename Type>
concept mysql_ora_reset_connection_capable =
    concepts::mysql_ora_reset_connection_capable<Type>;

// mysql_ora_session_trackable
//   concept: concepts::mysql_ora_session_trackable, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_session_trackable =
    concepts::mysql_ora_session_trackable<Type>;

// mysql_ora_query_attribute_settable
//   concept: concepts::mysql_ora_query_attribute_settable, by its former name,
// for one release.
template<typename Type>
concept mysql_ora_query_attribute_settable =
    concepts::mysql_ora_query_attribute_settable<Type>;

// mysql_ora_has_table_query
//   concept: concepts::mysql_ora_has_table_query, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_has_table_query =
    concepts::mysql_ora_has_table_query<Type>;

// mysql_ora_table_names_query
//   concept: concepts::mysql_ora_table_names_query, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_table_names_query =
    concepts::mysql_ora_table_names_query<Type>;

// mysql_ora_x_protocol_query
//   concept: concepts::mysql_ora_x_protocol_query, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_x_protocol_query =
    concepts::mysql_ora_x_protocol_query<Type>;

// mysql_ora_schema_queryable
//   concept: concepts::mysql_ora_schema_queryable, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_schema_queryable =
    concepts::mysql_ora_schema_queryable<Type>;

// mysql_ora_full_connection
//   concept: concepts::mysql_ora_full_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_ora_full_connection =
    concepts::mysql_ora_full_connection<Type>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MYSQL_MYSQL_HPP
