/******************************************************************************
* djinterp [database]                                              mariadb.hpp
* 
* djinterp MariaDB connection module:
*   This header provides the MariaDB-specific connection implementation
* and associated data type infrastructure for the djinterp database
* module, including:
*   - MySQL-family native field type enumeration (shared with Oracle MySQL)
*   - MariaDB-specific extended field types (INET6, UUID, JSON alias)
*   - compile-time type mapping between native MYSQL_TYPE_* constants and
*     the djinterp field_type enumeration
*   - version-gated data type availability queries using
*     D_ENV_MARIADB_* macros from env_mariadb.h
*   - MariaDB-specific SQL feature availability (sequences, system-
*     versioned tables, CTEs, window functions, etc.)
*   - MariaDB-specific connection configuration and Galera awareness
*   - the concrete mariadb_connection CRTP leaf class
*
*   This module sits at the top of the connection hierarchy and provides
* the final CRTP leaf class. The _helper methods declared here would be
* defined in a corresponding mariadb.cpp source file that includes the
* MariaDB C API header (<mariadb/mysql.h> or <mysql.h>).
*
*   LAYER DIAGRAM:
*     mariadb_connection (this file)
*       -> mysql_common_connection<mariadb_connection, database_type::mariadb>
*         -> database_connection<mariadb_connection, database_type::mariadb>
*           -> connection_template<mariadb_connection, database_type::mariadb>
*             -> connection<mariadb_connection>
*
*   DATA TYPE DESIGN:
*   The MySQL/MariaDB wire protocol uses MYSQL_TYPE_* constants to
* identify column types. These are identical across Oracle MySQL and
* MariaDB for the shared set (inherited from the pre-fork common
* codebase). MariaDB extends this with vendor-specific types (INET6,
* UUID). This header provides:
*   1. mysql_field_type: a scoped enum mirroring MYSQL_TYPE_* values,
*      usable without including <mysql.h>.
*   2. mariadb_field_type: MariaDB-specific type extensions.
*   3. Mapping functions between native types and djinterp::database::
*      field_type.
*   4. Compile-time predicates gated by version macros for type
*      availability.
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include the
* MariaDB C API header. All version-gated features use
* D_ENV_MARIADB_* and D_ENV_MYSQL_COMMON_* macros from the env
* headers.
*
*
*   DETECTION:
*   Also carries this database's capability-detection traits and C++20 concepts
* (trailing sections), folded in from mariadb_traits.hpp and the matching *_concepts.hpp;
* detection now lives with the connection. Concepts gated on concept support.
*
* path:      /inc/djinterp/core/db/mariadb/mariadb.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_MARIADB_
#define DJINTERP_DATABASE_MARIADB_

// djinterp
#include "../../../djinterp.hpp"
#include "../database_traits.hpp"
#include "../../../env/db/mariadb/env_mariadb.h"
#include "../database.hpp"
#include "../mysql/mysql_common.hpp"


NS_DJINTERP


// ===========================================================================
// I.   MYSQL-FAMILY NATIVE FIELD TYPES
// ===========================================================================
// These values mirror the MYSQL_TYPE_* / FIELD_TYPE_* constants from the
// MySQL wire protocol. They are identical in both Oracle MySQL and
// MariaDB for the shared set. Declared as a scoped enum so that the
// C API header does not need to be included.
//
// Source: MySQL client/server protocol documentation and mysql_com.h.

// mysql_field_type
//   enumeration: native MySQL/MariaDB wire protocol field type constants.
// values correspond 1:1 with the MYSQL_TYPE_* macros from mysql_com.h.
enum class mysql_field_type : std::uint16_t
{
    // integer types
    type_tiny         = 0x01,   // TINYINT           (1 byte)
    type_short        = 0x02,   // SMALLINT          (2 bytes)
    type_long         = 0x03,   // INT               (4 bytes)
    type_float        = 0x04,   // FLOAT             (4 bytes)
    type_double       = 0x05,   // DOUBLE            (8 bytes)
    type_null         = 0x06,   // NULL
    type_timestamp    = 0x07,   // TIMESTAMP
    type_longlong     = 0x08,   // BIGINT            (8 bytes)
    type_int24        = 0x09,   // MEDIUMINT         (3 bytes)

    // date and time types
    type_date         = 0x0A,   // DATE
    type_time         = 0x0B,   // TIME
    type_datetime     = 0x0C,   // DATETIME
    type_year         = 0x0D,   // YEAR

    // newdate is internal; exposed for completeness
    type_newdate      = 0x0E,   // internal DATE representation

    // string types
    type_varchar      = 0x0F,   // VARCHAR (wire protocol)
    type_bit          = 0x10,   // BIT

    // high-precision temporal (MySQL 5.6+ / MariaDB 10.0+)
    type_timestamp2   = 0x11,   // TIMESTAMP with fractional seconds
    type_datetime2    = 0x12,   // DATETIME  with fractional seconds
    type_time2        = 0x13,   // TIME      with fractional seconds

    // JSON
    // MySQL 5.7.8+ uses 0xF5 for native binary JSON. MariaDB's JSON
    // alias maps to LONGTEXT (0xFB) with a CHECK constraint; it does
    // NOT send 0xF5 over the wire. See mariadb_field_type below.
    type_json         = 0xF5,   // MySQL native JSON

    // decimal types
    type_newdecimal   = 0xF6,   // DECIMAL (high-precision, post-4.1)
    type_enum         = 0xF7,   // ENUM
    type_set          = 0xF8,   // SET

    // blob/text types
    type_tiny_blob    = 0xF9,   // TINYBLOB   / TINYTEXT
    type_medium_blob  = 0xFA,   // MEDIUMBLOB / MEDIUMTEXT
    type_long_blob    = 0xFB,   // LONGBLOB   / LONGTEXT
    type_blob         = 0xFC,   // BLOB       / TEXT

    // variable-length and spatial types
    type_var_string   = 0xFD,   // VARCHAR (legacy encoding)
    type_string       = 0xFE,   // CHAR / BINARY
    type_geometry     = 0xFF    // spatial geometry
};


// ===========================================================================
// II.  MARIADB-SPECIFIC EXTENDED FIELD TYPES
// ===========================================================================
// MariaDB extends the MySQL type system with vendor-specific types.
// These are NOT present in the MYSQL_TYPE_* enum; they are sent over
// the wire using existing type codes plus column metadata flags, or
// as MariaDB-specific extensions.

// mariadb_field_type
//   enumeration: MariaDB-specific data type extensions that do not
// exist in the Oracle MySQL type system.
enum class mariadb_field_type : std::uint16_t
{
    // INET6: native IPv6 address type (16 bytes, binary).
    // sent over the wire as MYSQL_TYPE_STRING with a length of 16
    // and a special charset. Introduced in MariaDB 10.5.0.
    inet6          = 0x0100,

    // UUID: native UUID type (16 bytes, binary storage).
    // sent as MYSQL_TYPE_STRING with a special metadata flag.
    // Introduced in MariaDB 10.7.0.
    uuid           = 0x0101,

    // JSON alias: MariaDB's JSON is LONGTEXT with CHECK(JSON_VALID()).
    // the wire protocol sends MYSQL_TYPE_LONG_BLOB (0xFB), NOT
    // MYSQL_TYPE_JSON (0xF5). This enumerator marks the logical
    // distinction. Available since MariaDB 10.2.7.
    json_alias     = 0x0102
};


// ===========================================================================
// III. DATA TYPE AVAILABILITY (compile-time, version-gated)
// ===========================================================================
// Compile-time predicates for MariaDB-specific data type availability.
// These use D_ENV_MARIADB_* macros to gate by version, enabling code
// to conditionally handle types that may not exist in older versions.

// mariadb_type_support
//   struct: compile-time data type availability flags.
// All members are static constexpr bools gated by env macros.
struct mariadb_type_support
{
    // MySQL-family common types (always available when client lib is
    // present)

    static constexpr bool has_blob_types =
#if D_ENV_MYSQL_COMMON_HAS_BLOB_TYPES
        true;
#else
        false;
#endif

    static constexpr bool has_bit_type =
#if D_ENV_MYSQL_COMMON_HAS_BIT_TYPE
        true;
#else
        false;
#endif

    static constexpr bool has_enum_type =
#if D_ENV_MYSQL_COMMON_HAS_ENUM_TYPE
        true;
#else
        false;
#endif

    static constexpr bool has_set_type =
#if D_ENV_MYSQL_COMMON_HAS_SET_TYPE
        true;
#else
        false;
#endif

    static constexpr bool has_geometry_types =
#if D_ENV_MYSQL_COMMON_HAS_GEOMETRY_TYPES
        true;
#else
        false;
#endif

    // MariaDB-specific types (version-gated)

#if D_ENV_MARIADB_DETECTED

    static constexpr bool has_json_type =
    #if D_ENV_MARIADB_HAS_JSON_TYPE
        true;
    #else
        false;
    #endif

    // MariaDB JSON is always a LONGTEXT alias, never native binary
    static constexpr bool json_is_native_binary = false;

    static constexpr bool has_json_table =
    #if D_ENV_MARIADB_HAS_JSON_TABLE
        true;
    #else
        false;
    #endif

    static constexpr bool has_json_arrayagg =
    #if D_ENV_MARIADB_HAS_JSON_ARRAYAGG
        true;
    #else
        false;
    #endif

    static constexpr bool has_inet6_type =
    #if D_ENV_MARIADB_HAS_INET6_TYPE
        true;
    #else
        false;
    #endif

    static constexpr bool has_uuid_type =
    #if D_ENV_MARIADB_HAS_UUID_TYPE
        true;
    #else
        false;
    #endif

    static constexpr bool has_generated_columns =
    #if D_ENV_MARIADB_HAS_GENERATED_COLUMNS
        true;
    #else
        false;
    #endif

    static constexpr bool has_invisible_columns =
    #if D_ENV_MARIADB_HAS_INVISIBLE_COLUMNS
        true;
    #else
        false;
    #endif

    static constexpr bool has_check_constraints =
    #if D_ENV_MARIADB_HAS_CHECK_CONSTRAINTS
        true;
    #else
        false;
    #endif

    static constexpr bool has_default_expression =
    #if D_ENV_MARIADB_HAS_DEFAULT_EXPRESSION
        true;
    #else
        false;
    #endif

    static constexpr bool has_descending_index =
    #if D_ENV_MARIADB_HAS_DESCENDING_INDEX
        true;
    #else
        false;
    #endif

#else
    // MariaDB not detected: all MariaDB-specific types unavailable
    static constexpr bool has_json_type           = false;
    static constexpr bool json_is_native_binary   = false;
    static constexpr bool has_json_table          = false;
    static constexpr bool has_json_arrayagg       = false;
    static constexpr bool has_inet6_type          = false;
    static constexpr bool has_uuid_type           = false;
    static constexpr bool has_generated_columns   = false;
    static constexpr bool has_invisible_columns   = false;
    static constexpr bool has_check_constraints   = false;
    static constexpr bool has_default_expression  = false;
    static constexpr bool has_descending_index    = false;
#endif  // D_ENV_MARIADB_DETECTED
};


// ===========================================================================
// IV.  DATA TYPE MAPPING
// ===========================================================================
// Conversion between MySQL/MariaDB wire protocol types and the generic
// djinterp::database::field_type enumeration.

// mysql_type_to_field_type
//   function: maps a mysql_field_type wire protocol value to the
// generic djinterp field_type. Handles the MariaDB JSON alias
// (LONGBLOB appearing as JSON) when _is_json_column is true.
inline field_type mysql_type_to_field_type(
    mysql_field_type _native_type,
    bool             _is_json_column = false
) noexcept
{
    // handle the MariaDB JSON alias: JSON columns arrive as
    // LONGBLOB/LONGTEXT over the wire, but the column metadata
    // indicates JSON via the CHECK constraint or charset
    if ( (_is_json_column) &&
         (_native_type == mysql_field_type::type_long_blob) )
    {
        return field_type::json;
    }

    switch (_native_type)
    {
        // null
        case mysql_field_type::type_null:
            return field_type::null;

        // boolean (TINYINT(1) is the conventional boolean)
        case mysql_field_type::type_tiny:
            return field_type::boolean;

        // integer family
        case mysql_field_type::type_short:
        case mysql_field_type::type_int24:
        case mysql_field_type::type_long:
        case mysql_field_type::type_year:
            return field_type::integer;

        case mysql_field_type::type_longlong:
            return field_type::big_integer;

        // floating point
        case mysql_field_type::type_float:
        case mysql_field_type::type_double:
            return field_type::floating_point;

        // decimal
        case mysql_field_type::type_newdecimal:
            return field_type::decimal;

        // string / varchar / char / enum / set
        case mysql_field_type::type_varchar:
        case mysql_field_type::type_var_string:
        case mysql_field_type::type_string:
        case mysql_field_type::type_enum:
        case mysql_field_type::type_set:
            return field_type::string;

        // binary / blob family
        case mysql_field_type::type_tiny_blob:
        case mysql_field_type::type_medium_blob:
        case mysql_field_type::type_long_blob:
        case mysql_field_type::type_blob:
        case mysql_field_type::type_bit:
            return field_type::binary;

        // date
        case mysql_field_type::type_date:
        case mysql_field_type::type_newdate:
            return field_type::date;

        // time
        case mysql_field_type::type_time:
        case mysql_field_type::type_time2:
            return field_type::time;

        // datetime
        case mysql_field_type::type_datetime:
        case mysql_field_type::type_datetime2:
            return field_type::datetime;

        // timestamp
        case mysql_field_type::type_timestamp:
        case mysql_field_type::type_timestamp2:
            return field_type::timestamp;

        // JSON (native MySQL binary JSON; MariaDB does not use this)
        case mysql_field_type::type_json:
            return field_type::json;

        // geometry / spatial
        case mysql_field_type::type_geometry:
            return field_type::custom;

        default:
            return field_type::custom;
    }
}

// mariadb_extended_type_to_field_type
//   function: maps a mariadb_field_type to the generic djinterp
// field_type.
inline field_type mariadb_extended_type_to_field_type(
    mariadb_field_type _mariadb_type
) noexcept
{
    switch (_mariadb_type)
    {
        case mariadb_field_type::inet6:
            return field_type::string;

        case mariadb_field_type::uuid:
            return field_type::uuid;

        case mariadb_field_type::json_alias:
            return field_type::json;

        default:
            return field_type::custom;
    }
}

// field_type_to_mariadb_sql
//   function: returns the SQL type name string for a given field_type
// as it would be used in MariaDB DDL. Version-dependent types fall
// back to compatible alternatives.
inline const char* field_type_to_mariadb_sql(
    field_type _type
) noexcept
{
    switch (_type)
    {
        case field_type::null:           return "NULL";
        case field_type::boolean:        return "TINYINT(1)";
        case field_type::integer:        return "INT";
        case field_type::big_integer:    return "BIGINT";
        case field_type::floating_point: return "DOUBLE";
        case field_type::decimal:        return "DECIMAL";
        case field_type::string:         return "VARCHAR(255)";
        case field_type::binary:         return "BLOB";
        case field_type::date:           return "DATE";
        case field_type::time:           return "TIME";
        case field_type::datetime:       return "DATETIME";
        case field_type::timestamp:      return "TIMESTAMP";

        case field_type::json:
            // MariaDB JSON is always LONGTEXT with a constraint
            if constexpr (mariadb_type_support::has_json_type)
            {
                return "JSON";
            }
            else
            {
                return "LONGTEXT";
            }

        case field_type::xml:
            return "LONGTEXT";

        case field_type::uuid:
            if constexpr (mariadb_type_support::has_uuid_type)
            {
                return "UUID";
            }
            else
            {
                return "CHAR(36)";
            }

        case field_type::array:
            return "JSON";

        case field_type::custom:
        default:
            return "BLOB";
    }
}


// ===========================================================================
// V.   MARIADB SQL FEATURE SUPPORT (compile-time, version-gated)
// ===========================================================================

// mariadb_feature_support
//   struct: compile-time SQL feature availability flags.
// All members are static constexpr bools gated by D_ENV_MARIADB_*
// macros from env_mariadb.h.
struct mariadb_feature_support
{
#if D_ENV_MARIADB_DETECTED

    // SQL extensions

    static constexpr bool has_sequences =
    #if D_ENV_MARIADB_HAS_SEQUENCES
        true;
    #else
        false;
    #endif

    static constexpr bool has_system_versioned_tables =
    #if D_ENV_MARIADB_HAS_SYSTEM_VERSIONED_TABLES
        true;
    #else
        false;
    #endif

    static constexpr bool has_returning =
    #if D_ENV_MARIADB_HAS_RETURNING
        true;
    #else
        false;
    #endif

    static constexpr bool has_intersect =
    #if D_ENV_MARIADB_HAS_INTERSECT
        true;
    #else
        false;
    #endif

    static constexpr bool has_window_functions =
    #if D_ENV_MARIADB_HAS_WINDOW_FUNCTIONS
        true;
    #else
        false;
    #endif

    static constexpr bool has_cte =
    #if D_ENV_MARIADB_HAS_CTE
        true;
    #else
        false;
    #endif

    static constexpr bool has_oracle_mode =
    #if D_ENV_MARIADB_HAS_ORACLE_MODE
        true;
    #else
        false;
    #endif

    static constexpr bool has_skip_locked =
    #if D_ENV_MARIADB_HAS_SKIP_LOCKED
        true;
    #else
        false;
    #endif

    static constexpr bool has_lateral_derived =
    #if D_ENV_MARIADB_HAS_LATERAL_DERIVED
        true;
    #else
        false;
    #endif

    static constexpr bool has_natural_sort =
    #if D_ENV_MARIADB_HAS_NATURAL_SORT
        true;
    #else
        false;
    #endif

    // replication and high availability

    static constexpr bool has_galera =
    #if D_ENV_MARIADB_HAS_GALERA
        true;
    #else
        false;
    #endif

    static constexpr bool has_galera_4 =
    #if D_ENV_MARIADB_HAS_GALERA_4
        true;
    #else
        false;
    #endif

    static constexpr bool has_gtid =
    #if D_ENV_MARIADB_HAS_GTID
        true;
    #else
        false;
    #endif

    static constexpr bool has_parallel_repl =
    #if D_ENV_MARIADB_HAS_PARALLEL_REPL
        true;
    #else
        false;
    #endif

    // storage engines

    static constexpr bool has_aria =
    #if D_ENV_MARIADB_HAS_ARIA
        true;
    #else
        false;
    #endif

    static constexpr bool has_columnstore =
    #if D_ENV_MARIADB_HAS_COLUMNSTORE
        true;
    #else
        false;
    #endif

    static constexpr bool has_s3_engine =
    #if D_ENV_MARIADB_HAS_S3_ENGINE
        true;
    #else
        false;
    #endif

    static constexpr bool has_spider =
    #if D_ENV_MARIADB_HAS_SPIDER
        true;
    #else
        false;
    #endif

    static constexpr bool has_rocksdb =
    #if D_ENV_MARIADB_HAS_ROCKSDB
        true;
    #else
        false;
    #endif

    // security

    static constexpr bool has_roles =
    #if D_ENV_MARIADB_HAS_ROLES
        true;
    #else
        false;
    #endif

    static constexpr bool has_data_at_rest_encryption =
    #if D_ENV_MARIADB_HAS_DATA_AT_REST_ENCRYPTION
        true;
    #else
        false;
    #endif

    static constexpr bool has_account_locking =
    #if D_ENV_MARIADB_HAS_ACCOUNT_LOCKING
        true;
    #else
        false;
    #endif

    // authentication

    static constexpr bool has_auth_ed25519 =
    #if D_ENV_MARIADB_HAS_AUTH_ED25519
        true;
    #else
        false;
    #endif

    static constexpr bool has_auth_pam_v2 =
    #if D_ENV_MARIADB_HAS_AUTH_PAM_V2
        true;
    #else
        false;
    #endif

    static constexpr bool has_auth_gssapi =
    #if D_ENV_MARIADB_HAS_AUTH_GSSAPI
        true;
    #else
        false;
    #endif

    // C API

    static constexpr bool has_async_api =
    #if D_ENV_MARIADB_HAS_ASYNC_API
        true;
    #else
        false;
    #endif

    static constexpr bool has_reset_connection =
    #if D_ENV_MARIADB_HAS_RESET_CONNECTION
        true;
    #else
        false;
    #endif

    static constexpr bool has_session_track =
    #if D_ENV_MARIADB_HAS_SESSION_TRACK
        true;
    #else
        false;
    #endif

    // InnoDB features

    static constexpr bool has_innodb_instant_ddl =
    #if D_ENV_MARIADB_HAS_INNODB_INSTANT_DDL
        true;
    #else
        false;
    #endif

    static constexpr bool has_innodb_page_compression =
    #if D_ENV_MARIADB_HAS_INNODB_PAGE_COMPRESSION
        true;
    #else
        false;
    #endif

    static constexpr bool has_atomic_ddl =
    #if D_ENV_MARIADB_HAS_ATOMIC_DDL
        true;
    #else
        false;
    #endif

    // composite feature checks

    static constexpr bool has_modern_sql =
    #if D_ENV_MARIADB_HAS_MODERN_SQL
        true;
    #else
        false;
    #endif

    static constexpr bool is_fully_modern =
    #if D_ENV_MARIADB_IS_FULLY_MODERN
        true;
    #else
        false;
    #endif

    static constexpr bool is_lts =
    #if D_ENV_MARIADB_IS_LTS
        true;
    #else
        false;
    #endif

#else
    // MariaDB not detected: everything unavailable
    static constexpr bool has_sequences                = false;
    static constexpr bool has_system_versioned_tables  = false;
    static constexpr bool has_returning                = false;
    static constexpr bool has_intersect                = false;
    static constexpr bool has_window_functions         = false;
    static constexpr bool has_cte                      = false;
    static constexpr bool has_oracle_mode              = false;
    static constexpr bool has_skip_locked              = false;
    static constexpr bool has_lateral_derived           = false;
    static constexpr bool has_natural_sort             = false;
    static constexpr bool has_galera                   = false;
    static constexpr bool has_galera_4                 = false;
    static constexpr bool has_gtid                     = false;
    static constexpr bool has_parallel_repl            = false;
    static constexpr bool has_aria                     = false;
    static constexpr bool has_columnstore              = false;
    static constexpr bool has_s3_engine                = false;
    static constexpr bool has_spider                   = false;
    static constexpr bool has_rocksdb                  = false;
    static constexpr bool has_roles                    = false;
    static constexpr bool has_data_at_rest_encryption  = false;
    static constexpr bool has_account_locking          = false;
    static constexpr bool has_auth_ed25519             = false;
    static constexpr bool has_auth_pam_v2              = false;
    static constexpr bool has_auth_gssapi              = false;
    static constexpr bool has_async_api                = false;
    static constexpr bool has_reset_connection         = false;
    static constexpr bool has_session_track            = false;
    static constexpr bool has_innodb_instant_ddl       = false;
    static constexpr bool has_innodb_page_compression  = false;
    static constexpr bool has_atomic_ddl               = false;
    static constexpr bool has_modern_sql               = false;
    static constexpr bool is_fully_modern              = false;
    static constexpr bool is_lts                       = false;
#endif  // D_ENV_MARIADB_DETECTED
};


// ===========================================================================
// VI.  MARIADB VERSION INFORMATION
// ===========================================================================

// mariadb_version_info
//   struct: compile-time version decomposition.
struct mariadb_version_info
{
#if D_ENV_MARIADB_DETECTED
    static constexpr bool          detected = true;
    static constexpr std::uint32_t id       = D_ENV_MARIADB_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_MARIADB_VERSION_MAJOR;
    static constexpr std::uint16_t minor    = D_ENV_MARIADB_VERSION_MINOR;
    static constexpr std::uint16_t patch    = D_ENV_MARIADB_VERSION_PATCH;
    static constexpr const char*   string   = D_ENV_MARIADB_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t minor    = 0;
    static constexpr std::uint16_t patch    = 0;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected MariaDB version is at
    // least (major, minor, patch).
    static constexpr bool at_least(std::uint16_t _major,
                                   std::uint16_t _minor,
                                   std::uint16_t _patch) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// ===========================================================================
// VII. MARIADB CONNECTION CONFIGURATION
// ===========================================================================

// mariadb_connect_config
//   struct: MariaDB-specific connection configuration extending the
// MySQL-family common configuration with Galera and MariaDB-specific
// options.
struct mariadb_connect_config
{
    mysql_connect_config  mysql_config;

    // Galera-aware options
    bool galera_wsrep_sync_wait;
    bool galera_wsrep_causal_reads;

    // MariaDB-specific init
    std::string default_storage_engine;

    mariadb_connect_config()
        : galera_wsrep_sync_wait(false),
          galera_wsrep_causal_reads(false)
    {
        // MariaDB defaults
        mysql_config.base.port       = 3306;
        mysql_config.base.host       = "localhost";
        mysql_config.default_charset = "utf8mb4";
    }

    explicit mariadb_connect_config(
        const connection_config& _base)
        : mysql_config(_base)
        , galera_wsrep_sync_wait(false)
        , galera_wsrep_causal_reads(false)
    {};
};


// ===========================================================================
// VIII. MARIADB CONNECTION
// ===========================================================================

// mariadb_connection
//   class: concrete MariaDB connection implementation. This is the
// CRTP leaf class that provides the _helper methods required by the
// base class chain. The _helper method bodies would be defined in the
// corresponding mariadb.cpp source file that includes the MariaDB
// C API header.
//
// Usage:
//   mariadb_connection conn;
//   conn.connect(config);
//   auto rs = conn.execute_query("SELECT 1");
class mariadb_connection
    : public mysql_common_connection<mariadb_connection, 
                                     database_type::mariadb>
{
public:
    using base_type = mysql_common_connection<mariadb_connection,
                                              database_type::mariadb>;

    using type_support    = mariadb_type_support;
    using feature_support = mariadb_feature_support;
    using version_info    = mariadb_version_info;

    mariadb_connection()
        : base_type()
    {};

    explicit mariadb_connection(
        const connection_config& _config
    )
        : base_type(_config)
    {};

    explicit mariadb_connection(
        const mysql_connect_config& _config)
        : base_type(_config)
    {};

    explicit mariadb_connection(
        const mariadb_connect_config& _config
    )
        : base_type(_config.mysql_config),
          m_mariadb_config(_config)
    {};

    ~mariadb_connection() = default;

    // disable copying
    mariadb_connection(const mariadb_connection&)            = delete;
    mariadb_connection& operator=(const mariadb_connection&) = delete;

    // enable moving
    mariadb_connection(mariadb_connection&&) noexcept            = default;
    mariadb_connection& operator=(mariadb_connection&&) noexcept = default;

    // MariaDB-specific feature queries (runtime)

    // supports_inet6_type
    //   function: returns true if the detected MariaDB version
    // supports the INET6 data type. Compile-time constant when the
    // version is known at build time; falls back to runtime version
    // string parsing otherwise.
    static constexpr bool supports_inet6_type() noexcept
    {
        return type_support::has_inet6_type;
    }

    // supports_uuid_type
    //   function: returns true if the detected MariaDB version
    // supports the native UUID data type.
    static constexpr bool supports_uuid_type() noexcept
    {
        return type_support::has_uuid_type;
    }

    // supports_json
    //   function: returns true if JSON type (alias) is available.
    static constexpr bool supports_json() noexcept
    {
        return type_support::has_json_type;
    }

    // json_is_native
    //   function: returns false. MariaDB JSON is always a LONGTEXT
    // alias, never a native binary type.
    static constexpr bool json_is_native() noexcept
    {
        return type_support::json_is_native_binary;
    }

    // supports_sequences
    //   function: returns true if CREATE SEQUENCE syntax is available.
    static constexpr bool supports_sequences() noexcept
    {
        return feature_support::has_sequences;
    }

    // supports_system_versioning
    //   function: returns true if system-versioned tables are
    // available.
    static constexpr bool supports_system_versioning() noexcept
    {
        return feature_support::has_system_versioned_tables;
    }

    // supports_returning
    //   function: returns true if the RETURNING clause is available.
    static constexpr bool supports_returning() noexcept
    {
        return feature_support::has_returning;
    }

    // supports_galera
    //   function: returns true if Galera Cluster integration is
    // available.
    static constexpr bool supports_galera() noexcept
    {
        return feature_support::has_galera;
    }

    // supports_modern_sql
    //   function: returns true if a comprehensive modern SQL feature
    // set is available (window functions + CTEs + sequences +
    // system-versioned tables).
    static constexpr bool supports_modern_sql() noexcept
    {
        return feature_support::has_modern_sql;
    }

    // data type mapping

    // map_native_type
    //   function: maps a native wire protocol type to djinterp
    // field_type, with awareness of MariaDB's JSON alias behavior.
    static field_type map_native_type(
        mysql_field_type _native_type,
        bool             _is_json_column = false) noexcept
    {
        return mysql_type_to_field_type(_native_type,
                                        _is_json_column);
    }

    // map_extended_type
    //   function: maps a MariaDB-specific extended type to djinterp
    // field_type.
    static field_type map_extended_type(
        mariadb_field_type _mariadb_type) noexcept
    {
        return mariadb_extended_type_to_field_type(_mariadb_type);
    }

    // sql_type_name
    //   function: returns the MariaDB SQL type name for a given
    // field_type.
    static const char* sql_type_name(field_type _type) noexcept
    {
        return field_type_to_mariadb_sql(_type);
    }

    // MariaDB-specific configuration

    // get_mariadb_config
    //   function: returns the MariaDB-specific configuration.
    const mariadb_connect_config&
    get_mariadb_config() const noexcept
    {
        return m_mariadb_config;
    }

    // set_mariadb_config
    //   function: replaces the MariaDB-specific configuration. Must
    // be called before connect().
    void set_mariadb_config(const mariadb_connect_config& _config)
    {
        m_mariadb_config = _config;
        this->set_mysql_config(_config.mysql_config);
    }

    // _helper methods (defined in mariadb.cpp)
    // These provide the actual C API calls. They are declared here so
    // the CRTP chain can resolve them at compile time; definitions
    // live in the source file that includes the MariaDB C API header.

    void        connect_helper();
    void        disconnect_helper();
    bool        is_connected_helper() const;
    bool        ping_helper() const;

    auto        execute_query_helper(const std::string& _query)
                    -> std::unique_ptr<
                        result_set<struct mariadb_result_set_helper>>;
    std::int64_t execute_update_helper(const std::string& _query);
    bool        execute_helper(const std::string& _query);

    auto        prepare_helper(const std::string& _query)
                    -> std::unique_ptr<
                        statement<struct mariadb_statement_helper>>;

    std::string   get_server_version_helper() const;
    std::string   get_last_error_helper() const;
    int           get_last_error_code_helper() const;
                  
    std::int64_t  get_last_insert_id_helper() const;
    std::int64_t  get_affected_rows_helper() const;

    // MySQL-family common _helper methods
    void          set_charset_helper(const std::string& _charset);
    std::string   get_charset_helper() const;
    int           next_result_helper();
    bool          more_results_helper() const;
    std::string   escape_string_helper(const std::string& _input) const;
    void          select_db_helper(const std::string& _database);
    void          change_user_helper(const std::string& _user,
                                     const std::string& _password,
                                     const std::string& _database);
    std::string   get_stat_helper() const;
    unsigned long get_thread_id_helper() const;
    unsigned int  get_warning_count_helper() const;
    std::string   get_sqlstate_helper() const;
    void          set_autocommit_helper(bool _enabled);
    void          set_option_helper(int _option, const void* _value);

    // transaction _helper methods
    void begin_transaction_helper();
    void commit_helper();
    void rollback_helper();

#if D_ENV_MARIADB_DETECTED
    #if D_ENV_MARIADB_HAS_RESET_CONNECTION
    // reset_connection
    //   function: resets the connection to a clean state without
    // re-authenticating. Available in MariaDB 10.2.4+.
    void reset_connection();
    #endif

    #if D_ENV_MARIADB_HAS_ASYNC_API
    // connect_async_start / connect_async_cont
    //   functions: non-blocking connection API. Available since
    // MariaDB 5.5 (Connector/C).
    int connect_async_start();
    int connect_async_cont(int _status);
    #endif
#endif  // D_ENV_MARIADB_DETECTED

private:
    mariadb_connect_config m_mariadb_config;
};


// ===========================================================================
// IX.  FORWARD DECLARATIONS
// ===========================================================================
// Vendor-specific result_set and statement implementations.
// These would be defined in separate headers or in the mariadb.cpp
// source file.

// mariadb_result_set_helper
//   struct: forward declaration of the MariaDB result set
// implementation.
struct mariadb_result_set_helper;

// mariadb_statement_helper
//   struct: forward declaration of the MariaDB prepared statement
// implementation.
struct mariadb_statement_helper;


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former mariadb_traits.hpp / mariadb_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// ===========================================================================
// X.   EXPRESSION DETECTORS (MariaDB-specific)
// ===========================================================================
// These detect methods that exist on MariaDB connections but not on
// generic MySQL connections. All MySQL-common detectors (charset,
// multi-result, options, diagnostics, result modes, autocommit,
// escape, select_db, change_user, storage engine) are inherited from
// mysql_common_traits.hpp in the mysql_common namespace.

// -------------------------------------------------------------------------
// A.  connection reset
// -------------------------------------------------------------------------

// mariadb_reset_connection_t
//   detector: reset_connection() method.
// wraps mysql_reset_connection() which was introduced in MariaDB
// 10.2.4. Resets the connection to a clean state (clears session
// variables, temporary tables, prepared statements) without
// re-authenticating.
template<typename _Type>
using mariadb_reset_connection_t =
    decltype(std::declval<_Type&>().reset_connection());

// -------------------------------------------------------------------------
// B.  non-blocking / async API
// -------------------------------------------------------------------------

// mariadb_connect_async_start_t
//   detector: connect_async_start() method.
// wraps mysql_real_connect_start() from the MariaDB non-blocking API.
template<typename _Type>
using mariadb_connect_async_start_t =
    decltype(std::declval<_Type&>().connect_async_start());

// mariadb_connect_async_cont_t
//   detector: connect_async_cont(int) method.
// wraps mysql_real_connect_cont() for continuing the non-blocking
// connection handshake.
template<typename _Type>
using mariadb_connect_async_cont_t =
    decltype(std::declval<_Type&>().connect_async_cont(
        std::declval<int>()));

// -------------------------------------------------------------------------
// C.  schema introspection
// -------------------------------------------------------------------------

// mariadb_table_exists_t
//   detector: table_exists(const std::string&) const method.
template<typename _Type>
using mariadb_table_exists_t =
    decltype(std::declval<const _Type&>().table_exists(
        std::declval<const std::string&>()));

// mariadb_get_table_names_t
//   detector: get_table_names() const method.
template<typename _Type>
using mariadb_get_table_names_t =
    decltype(std::declval<const _Type&>().get_table_names());


// ===========================================================================
// XI.  TAGGED CAPABILITY TRAITS (struct-based)
// ===========================================================================
// MariaDB-specific traits. All mysql_common tagged traits (has_mysql_charset,
// has_mysql_multi_result, has_mysql_options, has_mysql_diagnostics,
// has_mysql_result_modes, has_mysql_autocommit, has_mysql_escape,
// has_mysql_select_db, is_mysql_connection, etc.) are inherited from
// mysql_common_traits.hpp and available in the mysql_common namespace.

// has_mariadb_reset
//   trait: checks if type _Type supports connection reset.
template<typename _Type>
struct has_mariadb_reset
    : is_detected<mariadb_reset_connection_t, clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mariadb_reset_v =
        has_mariadb_reset<clean_t<_Type>>::value;
#endif

// has_mariadb_async
//   trait: checks if type _Type supports the non-blocking API
// (connect_async_start + connect_async_cont).
template<typename _Type>
struct has_mariadb_async : djinterp::conjunction<
    is_detected<mariadb_connect_async_start_t, clean_t<_Type>>,
    is_detected<mariadb_connect_async_cont_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mariadb_async_v =
        has_mariadb_async<clean_t<_Type>>::value;
#endif

// has_mariadb_schema_query
//   trait: checks if type _Type supports schema introspection
// (table_exists + get_table_names).
template<typename _Type>
struct has_mariadb_schema_query : djinterp::conjunction<
    is_detected<mariadb_table_exists_t, clean_t<_Type>>,
    is_detected<mariadb_get_table_names_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_mariadb_schema_query_v =
        has_mariadb_schema_query<clean_t<_Type>>::value;
#endif

// is_mariadb_connection
//   trait: compound trait verifying type _Type implements a MariaDB
// connection interface. This extends is_mysql_connection (from the
// mysql_common namespace) with MariaDB-specific capabilities.
// A type satisfies this if it is a valid MySQL-family connection
// AND has schema query support (which all Mariadb_connection
// instances provide).
template<typename _Type>
struct is_mariadb_connection : djinterp::conjunction<
    is_mysql_connection<clean_t<_Type>>,
    has_mariadb_schema_query<clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_mariadb_connection_v =
        is_mariadb_connection<clean_t<_Type>>::value;
#endif


// ===========================================================================
// XII. TAGLESS CAPABILITY TRAITS (constexpr bool)
// ===========================================================================
// All mysql_common tagless traits (mysql_can_set_charset,
// mysql_can_iterate_results, mysql_can_escape_string,
// mysql_does_charset, mysql_does_multi_result,
// mysql_is_full_connection, etc.) are available via the mysql_common
// namespace and do not need to be redeclared here.

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// mariadb_can_reset_connection
//   tagless trait: true if _Type has reset_connection().
template<typename _Type,
         typename = void>
constexpr bool mariadb_can_reset_connection = false;

template<typename _Type>
constexpr bool mariadb_can_reset_connection<_Type,
    std::void_t<mariadb_reset_connection_t<_Type>>> = true;

// mariadb_can_async_connect
//   tagless trait: true if _Type has connect_async_start().
template<typename _Type,
         typename = void>
constexpr bool mariadb_can_async_connect = false;

template<typename _Type>
constexpr bool mariadb_can_async_connect<_Type,
    std::void_t<mariadb_connect_async_start_t<_Type>>> = true;

// mariadb_can_query_schema
//   tagless trait: true if _Type has table_exists().
template<typename _Type,
         typename = void>
constexpr bool mariadb_can_query_schema = false;

template<typename _Type>
constexpr bool mariadb_can_query_schema<_Type,
    std::void_t<mariadb_table_exists_t<_Type>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// mariadb_does_async
//   tagless trait: true if _Type supports the full non-blocking API.
template<typename _Type,
         typename = void>
constexpr bool mariadb_does_async = false;

template<typename _Type>
constexpr bool mariadb_does_async<_Type, std::void_t<
    mariadb_connect_async_start_t<_Type>,
    mariadb_connect_async_cont_t<_Type>>> = true;

// mariadb_does_schema_query
//   tagless trait: true if _Type supports full schema introspection.
template<typename _Type,
         typename = void>
constexpr bool mariadb_does_schema_query = false;

template<typename _Type>
constexpr bool mariadb_does_schema_query<_Type, std::void_t<
    mariadb_table_exists_t<_Type>,
    mariadb_get_table_names_t<_Type>>> = true;

// mariadb_is_full_connection
//   tagless trait: true if _Type satisfies the complete MariaDB
// connection interface (full MySQL-family connection + schema query).
template<typename _Type>
constexpr bool mariadb_is_full_connection =
    ( mysql_is_full_connection<clean_t<_Type>>  &&
      mariadb_can_query_schema<clean_t<_Type>> );


// ===========================================================================
// XIII.  SFINAE HELPERS
// ===========================================================================

// enable_if_mariadb_connection
//   type: SFINAE helper for MariaDB connection constraints.
template<typename _Type>
using enable_if_mariadb_connection =
    typename std::enable_if<is_mariadb_connection<clean_t<_Type>>::value>::type;

// enable_if_has_mariadb_reset
//   type: SFINAE helper for MariaDB reset_connection constraints.
template<typename _Type>
using enable_if_has_mariadb_reset =
    typename std::enable_if<has_mariadb_reset<clean_t<_Type>>::value>::type;

// enable_if_has_mariadb_async
//   type: SFINAE helper for MariaDB async API constraints.
template<typename _Type>
using enable_if_has_mariadb_async =
    typename std::enable_if<has_mariadb_async<clean_t<_Type>>::value>::type;

// enable_if_has_mariadb_schema_query
//   type: SFINAE helper for MariaDB schema query constraints.
template<typename _Type>
using enable_if_has_mariadb_schema_query =
    typename std::enable_if<has_mariadb_schema_query<clean_t<_Type>>::value>::type;


// ===========================================================================
// XIV.   C++20 CONCEPTS
// ===========================================================================
//   The MariaDB classification concepts, folded in from the former
// mariadb_concepts.hpp.  Each forwards to a trait / tagless capability declared
// above.  Gated on concept support so the traits remain usable at the C++17
// baseline (matching functor.hpp / monoid.hpp).  All names are mariadb_-prefixed,
// so they do not collide with the MySQL-family concepts carried by the included
// mysql_common_traits.hpp.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


// -------------------------------------------------------------------------
// A.  core MariaDB connection concepts
// -------------------------------------------------------------------------

// Mariadb_connection
//   concept: constrains types implementing the MariaDB connection
// interface.
template<typename _Type>
concept Mariadb_connection =
    is_mariadb_connection<clean_t<_Type>>::value;

// non_mariadb_connection
//   concept: constrains types that do not implement the MariaDB
// connection interface.
template<typename _Type>
concept non_mariadb_connection =
    !Mariadb_connection<_Type>;

// mariadb_schema_connection
//   concept: constrains MariaDB connections supporting schema
// introspection.
template<typename _Type>
concept mariadb_schema_connection =
    has_mariadb_schema_query<clean_t<_Type>>::value;

// mariadb_resettable_connection
//   concept: constrains MariaDB connections supporting connection reset.
template<typename _Type>
concept mariadb_resettable_connection =
    has_mariadb_reset<clean_t<_Type>>::value;

// mariadb_async_connection
//   concept: constrains MariaDB connections supporting the non-blocking
// connection API.
template<typename _Type>
concept mariadb_async_connection =
    has_mariadb_async<clean_t<_Type>>::value;


// -------------------------------------------------------------------------
// B.  MariaDB capability concepts
// -------------------------------------------------------------------------

// mariadb_reset_connection_capable
//   concept: constrains types exposing reset_connection().
template<typename _Type>
concept mariadb_reset_connection_capable =
    mariadb_can_reset_connection<clean_t<_Type>>;

// mariadb_async_connect_startable
//   concept: constrains types exposing connect_async_start().
template<typename _Type>
concept mariadb_async_connect_startable =
    mariadb_can_async_connect<clean_t<_Type>>;

// mariadb_async_connect_continuable
//   concept: constrains types exposing connect_async_cont(status).
template<typename _Type>
concept mariadb_async_connect_continuable =
    is_detected<mariadb_connect_async_cont_t, clean_t<_Type>>::value;

// mariadb_table_exists_query
//   concept: constrains types exposing table_exists(name).
template<typename _Type>
concept mariadb_table_exists_query =
    mariadb_can_query_schema<clean_t<_Type>>;

// mariadb_table_names_query
//   concept: constrains types exposing get_table_names().
template<typename _Type>
concept mariadb_table_names_query =
    is_detected<mariadb_get_table_names_t, clean_t<_Type>>::value;


// -------------------------------------------------------------------------
// C.  tagless MariaDB capability concepts
// -------------------------------------------------------------------------

// mariadb_async_handshakeable
//   concept: constrains types satisfying the full tagless async handshake
// capability set.
template<typename _Type>
concept mariadb_async_handshakeable =
    mariadb_does_async<clean_t<_Type>>;

// mariadb_schema_queryable
//   concept: constrains types satisfying the full tagless schema-query
// capability set.
template<typename _Type>
concept mariadb_schema_queryable =
    mariadb_does_schema_query<clean_t<_Type>>;

// mariadb_full_connection
//   concept: constrains types satisfying the complete tagless MariaDB
// connection capability set.
template<typename _Type>
concept mariadb_full_connection =
    mariadb_is_full_connection<clean_t<_Type>>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MARIADB_
