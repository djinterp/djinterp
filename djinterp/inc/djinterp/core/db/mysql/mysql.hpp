/******************************************************************************
* djinterp [database]                                                mysql.hpp
* 
* djinterp Oracle MySQL connection module:
*   This header provides the Oracle MySQL-specific connection
* implementation and associated infrastructure for the djinterp database
* module, including:
*   - MySQL wire protocol MYSQL_TYPE_* field type codes (shared enum from
*     mysql_common.hpp) with MySQL-specific additions
*   - compile-time type and feature availability via D_ENV_MYSQL_* macros
*     covering JSON, X Protocol, authentication, InnoDB, replication,
*     Group Replication, InnoDB Cluster, and optimizer features
*   - Oracle MySQL-specific connection configuration (SSL mode,
*     authentication plugin, X Protocol port, session tracking, query
*     attributes, Group Replication monitoring)
*   - the concrete mysql_connection CRTP leaf class inheriting from
*     mysql_common_connection with MySQL-specific version-gated methods
*     for reset_connection (5.7.3+), async API (8.0.16+), session
*     tracking (5.7.4+), and query attributes (8.0.25+)
*
*   Oracle MySQL diverges from MariaDB in several key areas:
*   - X Protocol and X DevAPI (5.7.12+) - MariaDB does not have this
*   - Group Replication / InnoDB Cluster / ClusterSet - MariaDB uses
*     Galera Cluster instead
*   - HeatWave (cloud analytics engine) - MySQL-only
*   - caching_sha2_password as default auth (8.0.4+) - MariaDB defaults
*     to mysql_native_password or ed25519
*   - non-blocking API via _nonblocking suffix (8.0.16+) - MariaDB uses
*     _start/_cont suffix (Connector/C 5.5+)
*   - data dictionary (8.0+) replaces FRM files - MariaDB keeps FRM
*
*   LAYER DIAGRAM:
*     mysql_connection (this file)
*       -> mysql_common_connection<mysql_connection, database_type::mysql>
*         -> database_connection<mysql_connection, database_type::mysql>
*           -> connection_template<mysql_connection, database_type::mysql>
*             -> connection<mysql_connection>
*
* 
* path:      /inc/djinterp/core/db/mysql/mysql.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_MYSQL_
#define DJINTERP_DATABASE_MYSQL_

// djinterp
#include "../djinterp.hpp"
#include "../env/db/env_mysql.h"
#include "./mysql_common.hpp"
#include "./mysql_traits.hpp"


NS_DJINTERP
NS_DATABASE


// =============================================================================
// I.   MYSQL-SPECIFIC TYPE SUPPORT
// =============================================================================
// The wire protocol MYSQL_TYPE_* codes and mysql_type_to_field_type()
// mapping are defined in mysql_common.hpp and shared with MariaDB.
// This struct adds compile-time flags for MySQL-specific type features
// that MariaDB lacks (e.g. JSON_TABLE, multi-value index,
// check constraints, functional indexes).

// mysql_type_support
//   struct: compile-time data type availability flags specific to
// Oracle MySQL.
struct mysql_type_support
{
#if D_ENV_MYSQL_DETECTED

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
    #endif  // D_ENV_MYSQL_HAS_DESCENDING_INDEX

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
#endif
};


// =============================================================================
// II.  FEATURE SUPPORT (compile-time, version-gated)
// =============================================================================

// mysql_feature_support
//   struct: compile-time feature availability flags for Oracle MySQL.
struct mysql_feature_support
{
#if D_ENV_MYSQL_DETECTED

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
    static constexpr bool has_window_functions   = false;
    static constexpr bool has_cte                = false;
    static constexpr bool has_lateral_derived    = false;
    static constexpr bool has_hash_join          = false;
    static constexpr bool has_histograms         = false;
    static constexpr bool has_explain_analyze    = false;
    static constexpr bool has_x_protocol         = false;
    static constexpr bool has_group_replication  = false;
    static constexpr bool has_innodb_cluster     = false;
    static constexpr bool has_clone_plugin       = false;
    static constexpr bool has_gtid               = false;
    static constexpr bool has_innodb_instant_ddl = false;
    static constexpr bool has_atomic_ddl         = false;
    static constexpr bool has_data_dictionary    = false;
    static constexpr bool has_auth_caching_sha2  = false;
    static constexpr bool has_multi_factor_auth  = false;
    static constexpr bool has_roles              = false;
    static constexpr bool has_partial_revoke     = false;
    static constexpr bool has_async_api          = false;
    static constexpr bool has_reset_connection   = false;
    static constexpr bool has_session_track      = false;
    static constexpr bool has_query_attrs        = false;
    static constexpr bool has_modern_sql         = false;
    static constexpr bool has_modern_json        = false;
    static constexpr bool has_modern_ddl         = false;
    static constexpr bool has_ha_suite           = false;
    static constexpr bool is_fully_modern        = false;
    static constexpr bool is_lts                 = false;
#endif
};


// =============================================================================
// III. VERSION INFORMATION
// =============================================================================

// mysql_version_info
//   struct: compile-time version decomposition for Oracle MySQL.
// uses MAJOR*10000 + MINOR*100 + PATCH encoding (same as the
// MYSQL_VERSION_ID macro from mysql_version.h).
struct mysql_version_info
{
#if D_ENV_MYSQL_DETECTED
    static constexpr bool          detected = true;
    static constexpr std::uint32_t id       = D_ENV_MYSQL_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_MYSQL_VERSION_MAJOR;
    static constexpr std::uint16_t minor    = D_ENV_MYSQL_VERSION_MINOR;
    static constexpr std::uint16_t patch    = D_ENV_MYSQL_VERSION_PATCH;
    static constexpr const char*   string   = D_ENV_MYSQL_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t minor    = 0;
    static constexpr std::uint16_t patch    = 0;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected MySQL version is at
    // least (major, minor, patch).
    static constexpr bool at_least(
        std::uint16_t _major,
        std::uint16_t _minor,
        std::uint16_t _patch) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// IV.  MYSQL-SPECIFIC CONNECTION CONFIGURATION
// =============================================================================

// mysql_ssl_mode
//   enum: MySQL SSL connection modes (mysql_ssl_mode from mysql.h).
enum class mysql_ssl_mode : std::uint8_t
{
    disabled       = 1,     // SSL_MODE_DISABLED
    preferred      = 2,     // SSL_MODE_PREFERRED
    required       = 3,     // SSL_MODE_REQUIRED
    verify_ca      = 4,     // SSL_MODE_VERIFY_CA
    verify_identity = 5     // SSL_MODE_VERIFY_IDENTITY
};

// oracle_mysql_connect_config
//   struct: Oracle MySQL-specific connection configuration extending
// the shared mysql_connect_config.
struct oracle_mysql_connect_config
{
    mysql_connect_config common;
    mysql_ssl_mode       ssl_mode;
    std::string          auth_plugin;
    std::string          tls_version;
    std::string          tls_ciphersuites;
    std::uint16_t        x_protocol_port;
    bool                 use_x_protocol;
    bool                 enable_session_tracking;
    bool                 enable_cleartext_plugin;
    std::size_t          statement_cache_size;

    std::map<std::string, std::string>  connect_attributes;

    oracle_mysql_connect_config()
        : ssl_mode(mysql_ssl_mode::preferred)
        , x_protocol_port(33060)
        , use_x_protocol(false)
        , enable_session_tracking(false)
        , enable_cleartext_plugin(false)
        , statement_cache_size(0)
    {
        common.base.host = "localhost";
        common.base.port = 3306;
    }

    explicit oracle_mysql_connect_config(
        const connection_config& _base
    )
        : ssl_mode(mysql_ssl_mode::preferred)
        , x_protocol_port(33060)
        , use_x_protocol(false)
        , enable_session_tracking(false)
        , enable_cleartext_plugin(false)
        , statement_cache_size(0)
    {
        common.base = _base;
    }
};


// =============================================================================
// V.   MYSQL CONNECTION
// =============================================================================

// mysql_connection
//   class: concrete Oracle MySQL connection implementation. This is the
// CRTP leaf class inheriting from mysql_common_connection, which
// provides charset, multi-result, escape, autocommit, options, and
// diagnostics. This class adds MySQL-specific features: reset_connection,
// async API, session tracking, query attributes, and X Protocol
// awareness.
class mysql_connection
    : public mysql_common_connection<
          mysql_connection,
          database_type::mysql>
{
private:
    using base_type = mysql_common_connection<
        mysql_connection, database_type::mysql>;

public:
    using type_support    = mysql_type_support;
    using feature_support = mysql_feature_support;
    using version_info    = mysql_version_info;

    mysql_connection()
        : base_type()
    {}

    explicit mysql_connection(
        const connection_config& _config
    )
        : base_type(_config)
    {}

    explicit mysql_connection(
        const oracle_mysql_connect_config& _config
    )
        : base_type(_config.common.base)
        , m_mysql_config(_config)
    {}

    ~mysql_connection() = default;

    // disable copying
    mysql_connection(const mysql_connection&)            = delete;
    mysql_connection& operator=(const mysql_connection&) = delete;

    // enable moving
    mysql_connection(mysql_connection&&) noexcept            = default;
    mysql_connection& operator=(mysql_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    // table_exists
    //   function: tests whether a table exists in the current database.
    bool table_exists(const std::string& _table_name) const
    {
        return self().table_exists_helper(_table_name);
    }

    // get_table_names
    //   function: returns all table names in the current database.
    std::vector<std::string> get_table_names() const
    {
        return self().get_table_names_helper();
    }

    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_json() noexcept
    {
        return type_support::has_json_type;
    }

    static constexpr bool supports_x_protocol() noexcept
    {
        return feature_support::has_x_protocol;
    }

    static constexpr bool supports_window_functions() noexcept
    {
        return feature_support::has_window_functions;
    }

    static constexpr bool supports_cte() noexcept
    {
        return feature_support::has_cte;
    }

    static constexpr bool supports_group_replication() noexcept
    {
        return feature_support::has_group_replication;
    }

    static constexpr bool supports_innodb_cluster() noexcept
    {
        return feature_support::has_innodb_cluster;
    }

    static constexpr bool supports_roles() noexcept
    {
        return feature_support::has_roles;
    }

    static constexpr bool supports_atomic_ddl() noexcept
    {
        return feature_support::has_atomic_ddl;
    }

    static constexpr bool supports_modern_sql() noexcept
    {
        return feature_support::has_modern_sql;
    }

    static constexpr bool is_lts_release() noexcept
    {
        return feature_support::is_lts;
    }

    // -----------------------------------------------------------------
    // MySQL-specific configuration
    // -----------------------------------------------------------------

    const oracle_mysql_connect_config& get_mysql_config() const noexcept
    {
        return m_mysql_config;
    }

    void set_mysql_config(
        const oracle_mysql_connect_config& _config
    )
    {
        m_mysql_config      = _config;
        this->m_config      = _config.common.base;
    }

    // -----------------------------------------------------------------
    // _helper methods (defined in mysql.cpp)
    // -----------------------------------------------------------------

    void        connect_helper();
    void        disconnect_helper();
    bool        is_connected_helper() const;
    bool        ping_helper() const;

    auto        execute_query_helper(const std::string& _query)
                    -> std::unique_ptr<
                        result_set<struct mysql_result_set_helper>>;
    std::int64_t execute_update_helper(const std::string& _query);
    bool        execute_helper(const std::string& _query);

    auto        prepare_helper(const std::string& _query)
                    -> std::unique_ptr<
                        statement<struct mysql_statement_helper>>;

    std::string  get_server_version_helper() const;
    std::string  get_last_error_helper() const;
    int          get_last_error_code_helper() const;
    std::int64_t get_last_insert_id_helper() const;
    std::int64_t get_affected_rows_helper() const;

    // MySQL-family common _helper methods
    void          set_charset_helper(const std::string& _charset);
    std::string   get_charset_helper() const;
    int           next_result_helper();
    bool          more_results_helper() const;
    std::string   escape_string_helper(
                      const std::string& _input) const;
    void          select_db_helper(const std::string& _database);
    void          change_user_helper(const std::string& _user,
                                   const std::string& _password,
                                   const std::string& _database);
    std::string   get_stat_helper() const;
    unsigned long get_thread_id_helper() const;
    unsigned int  get_warning_count_helper() const;
    std::string   get_sqlstate_helper() const;
    void          set_autocommit_helper(bool _enabled);
    void          set_option_helper(int         _option,
                                  const void* _value);

    // schema introspection _helper
    bool table_exists_helper(const std::string& _name) const;
    std::vector<std::string> get_table_names_helper() const;

    // transaction _helper
    void begin_transaction_helper();
    void commit_helper();
    void rollback_helper();

    // version-gated methods

#if D_ENV_MYSQL_DETECTED
    #if D_ENV_MYSQL_HAS_RESET_CONNECTION
    // reset_connection
    //   function: resets the connection to a clean state without
    // re-authenticating. Available in Oracle MySQL 5.7.3+.
    // wraps mysql_reset_connection().
    void reset_connection();
    #endif

    #if D_ENV_MYSQL_HAS_ASYNC_API
    // async_query_start / async_query_cont / async_connect_start
    //   functions: non-blocking C API. Available in Oracle MySQL
    // 8.0.16+. wraps mysql_real_query_nonblocking() and
    // mysql_real_connect_nonblocking().
    int  async_query_start(const std::string& _query);
    int  async_query_cont();
    int  async_connect_start();
    #endif

    #if D_ENV_MYSQL_HAS_SESSION_TRACK
    // get_session_track_info
    //   function: retrieves session state change information.
    // Available in Oracle MySQL 5.7.4+.
    // wraps mysql_session_track_get_first().
    std::string get_session_track_info(int _type) const;
    #endif

    #if D_ENV_MYSQL_HAS_QUERY_ATTRS
    // set_query_attribute
    //   function: sets a query attribute for the next query.
    // Available in Oracle MySQL 8.0.25+.
    // wraps mysql_bind_param().
    void set_query_attribute(const std::string& _name,
                             const std::string& _value);
    #endif
#endif  // D_ENV_MYSQL_DETECTED

private:
    oracle_mysql_connect_config m_mysql_config;

    mysql_connection& self()
    {
        return *this;
    }

    const mysql_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VI.  FORWARD DECLARATIONS
// =============================================================================

// mysql_result_set_helper
//   struct: forward declaration of the Oracle MySQL result set
// implementation.
struct mysql_result_set_helper;

// mysql_statement_helper
//   struct: forward declaration of the Oracle MySQL prepared
// statement implementation.
struct mysql_statement_helper;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MYSQL_