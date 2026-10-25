/*******************************************************************************
* djinterp [core]                                                    mariadb.hpp
*
* djinterp MariaDB connection module:
*   The MariaDB connection, mariadb_connection, over the MariaDB layer's C++
* face through mysql_common_connection, which holds the face and implements
* what MariaDB and MySQL share; this leaf adds MariaDB's configuration --
* Galera's causal reads, the default storage engine -- the wire protocol's
* field types (MYSQL_TYPE_*, as mysql_field_type, without the client's
* header), MariaDB's own (INET6, UUID, the JSON alias), how each maps to
* field_type, and what the server connected has, asked at run time, beside
* what the client's header has, at compile time from env_mariadb.h: the
* client's capabilities.
*     mariadb_connection (this file)
*       -> mysql_common_connection<mariadb_connection, database_type::mariadb>
*         -> database_connection<mariadb_connection, database_type::mariadb>
*           -> connection_template<mariadb_connection, ...>
*             -> connection<mariadb_connection>
*   The leaf's hooks are defined in mariadb.cpp, which instantiates the
* family's members for it. The header needs C++17 and includes no client
* header: env includes mysql.h only where a program asks for env's
* detection (D_CFG_ENV_USING_MARIADB), which the client's capabilities read.
* Such a program, under `using namespace djinterp`, names the
* class djinterp::mariadb_connection, since Connector/C declares a function
* of that name. Its trailing sections carry MariaDB's capability-detection
* traits and C++20 concepts.
*
*
* path:      /inc/djinterp/core/db/mariadb/mariadb.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  MYSQL-FAMILY NATIVE FIELD TYPES
    -------------------------------
    1.  Wire protocol types
         1.  mysql_field_type
2.  MARIADB EXTENDED FIELD TYPES
    ----------------------------
    1.  MariaDB's own types
         1.  mariadb_field_type
3.  CLIENT CAPABILITIES
    -------------------
    1.  Compile-time capabilities
         1.  mariadb_client_capabilities
         2.  mariadb_type_support
         3.  mariadb_feature_support
4.  DATA TYPE MAPPING
    -----------------
    1.  To and from field_type
         1.  mysql_type_to_field_type
         2.  mariadb_extended_type_to_field_type
         3.  field_type_to_mariadb_sql
5.  VERSION INFORMATION
    -------------------
    1.  The version
         1.  mariadb_version_info
6.  CONNECTION CONFIGURATION
    ------------------------
    1.  The configuration
         1.  mariadb_connect_config
7.  MARIADB CONNECTION
    ------------------
    1.  The class
         1.  mariadb_connection
         2.  mysql_common_connection<mariadb_connection, ...>
8.  EXPRESSION DETECTORS
    --------------------
    1.  Connection reset
    2.  Schema introspection
9.  TAGGED CAPABILITY TRAITS
    ------------------------
    1.  Traits
         1.  has_mariadb_reset
         2.  has_mariadb_schema_query
         3.  is_mariadb_connection
10. TAGLESS CAPABILITY TRAITS
    -------------------------
    1.  Individual capabilities
    2.  Compound capabilities
11. SFINAE HELPERS
    --------------
    1.  enable_if helpers
12. C++20 CONCEPTS
    --------------
    1.  Connection concepts
    2.  Capability concepts
    3.  Tagless capability concepts
    4.  Former names
*/

#ifndef DJINTERP_DB_MARIADB_MARIADB_HPP
#define DJINTERP_DB_MARIADB_MARIADB_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t,
                                                   // uint32_t, int64_t

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <string>  // std::string
// djinterp
#include "../../../djinterp.hpp"                   // framework root
#include "../../../c/db/mariadb/mariadb_face.hpp"  // face::mariadb
#include "../../../env/db/mariadb/env_mariadb.h"   // D_ENV_MARIADB_*
#include "../../meta/type_utility.hpp"             // clean_t
#include "../database.hpp"                         // the vocabulary
#include "../mysql/mysql_common.hpp"               // mysql_common_connection


NS_DJINTERP


//==============================================================================
// 1.  MYSQL-FAMILY NATIVE FIELD TYPES
//==============================================================================
// These values mirror the MYSQL_TYPE_* / FIELD_TYPE_* constants from the
// MySQL wire protocol. They are identical in both Oracle MySQL and
// MariaDB for the shared set. Declared as a scoped enum so that the
// C API header does not need to be included.
//
// Source: MySQL client/server protocol documentation and mysql_com.h.


// 1.1    Wire protocol types
//------------------------------------------------------------------------------
// 1.1.1
// mysql_field_type
//   enumeration: native MySQL/MariaDB wire protocol field type constants.
// values correspond 1:1 with the MYSQL_TYPE_* macros from mysql_com.h.
enum class mysql_field_type : re_std::uint16_t
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


//==============================================================================
// 2.  MARIADB EXTENDED FIELD TYPES
//==============================================================================
// MariaDB extends the MySQL type system with vendor-specific types.
// These are NOT present in the MYSQL_TYPE_* enum; they are sent over
// the wire using existing type codes plus column metadata flags, or
// as MariaDB-specific extensions.


// 2.1    MariaDB's own types
//------------------------------------------------------------------------------
// 2.1.1
// mariadb_field_type
//   enumeration: MariaDB-specific data type extensions that do not
// exist in the Oracle MySQL type system.
enum class mariadb_field_type : re_std::uint16_t
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


//==============================================================================
// 3.  CLIENT CAPABILITIES
//==============================================================================
// What env_mariadb.h found MariaDB's client header to have, by version, at
// compile time -- the client's capabilities: the data types and the SQL
// features a server of the header's release has, which the server connected
// may lack, or exceed. Every MariaDB flag is false unless a program asks for
// env's detection (D_CFG_ENV_USING_MARIADB) or supplies its results; the
// family's types, which every release has, are always true. What the server
// connected has, mariadb_connection's supports_* members ask at run time.


// 3.1    Compile-time capabilities
//------------------------------------------------------------------------------
// 3.1.1
// mariadb_client_capabilities
//   struct: compile-time data type and SQL feature availability flags, as
// MariaDB's client header gives them.
struct mariadb_client_capabilities
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
    static constexpr bool has_json_type               = false;
    static constexpr bool json_is_native_binary       = false;
    static constexpr bool has_json_table              = false;
    static constexpr bool has_json_arrayagg           = false;
    static constexpr bool has_inet6_type              = false;
    static constexpr bool has_uuid_type               = false;
    static constexpr bool has_generated_columns       = false;
    static constexpr bool has_invisible_columns       = false;
    static constexpr bool has_check_constraints       = false;
    static constexpr bool has_default_expression      = false;
    static constexpr bool has_descending_index        = false;
    static constexpr bool has_sequences               = false;
    static constexpr bool has_system_versioned_tables = false;
    static constexpr bool has_returning               = false;
    static constexpr bool has_intersect               = false;
    static constexpr bool has_window_functions        = false;
    static constexpr bool has_cte                     = false;
    static constexpr bool has_oracle_mode             = false;
    static constexpr bool has_skip_locked             = false;
    static constexpr bool has_lateral_derived         = false;
    static constexpr bool has_natural_sort            = false;
    static constexpr bool has_galera                  = false;
    static constexpr bool has_galera_4                = false;
    static constexpr bool has_gtid                    = false;
    static constexpr bool has_parallel_repl           = false;
    static constexpr bool has_aria                    = false;
    static constexpr bool has_columnstore             = false;
    static constexpr bool has_s3_engine               = false;
    static constexpr bool has_spider                  = false;
    static constexpr bool has_rocksdb                 = false;
    static constexpr bool has_roles                   = false;
    static constexpr bool has_data_at_rest_encryption = false;
    static constexpr bool has_account_locking         = false;
    static constexpr bool has_auth_ed25519            = false;
    static constexpr bool has_auth_pam_v2             = false;
    static constexpr bool has_auth_gssapi             = false;
    static constexpr bool has_async_api               = false;
    static constexpr bool has_reset_connection        = false;
    static constexpr bool has_session_track           = false;
    static constexpr bool has_innodb_instant_ddl      = false;
    static constexpr bool has_innodb_page_compression = false;
    static constexpr bool has_atomic_ddl              = false;
    static constexpr bool has_modern_sql              = false;
    static constexpr bool is_fully_modern             = false;
    static constexpr bool is_lts                      = false;
#endif  // D_ENV_MARIADB_DETECTED
};

// 3.1.2
// mariadb_type_support
//   type: mariadb_client_capabilities, by its former name (2026.10.21), for
// one release; it held the data types.
using mariadb_type_support [[deprecated("mariadb_client_capabilities")]] =
    mariadb_client_capabilities;

// 3.1.3
// mariadb_feature_support
//   type: mariadb_client_capabilities, by its former name (2026.10.21), for
// one release; it held the SQL features.
using mariadb_feature_support [[deprecated("mariadb_client_capabilities")]] =
    mariadb_client_capabilities;


//==============================================================================
// 4.  DATA TYPE MAPPING
//==============================================================================
// Conversion between MySQL/MariaDB wire protocol types and the generic
// field_type enumeration (database.hpp).


// 4.1    To and from field_type
//------------------------------------------------------------------------------
// 4.1.1
// mysql_type_to_field_type
//   function: maps a mysql_field_type wire protocol value to the
// generic djinterp field_type. Handles the MariaDB JSON alias
// (LONGBLOB appearing as JSON) when _is_json_column is true.
inline field_type
mysql_type_to_field_type(
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

// 4.1.2
// mariadb_extended_type_to_field_type
//   function: maps a mariadb_field_type to the generic djinterp
// field_type.
inline field_type
mariadb_extended_type_to_field_type(
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

// 4.1.3
// field_type_to_mariadb_sql
//   function: returns the SQL type name string for a given field_type
// as it would be used in MariaDB DDL. Version-dependent types fall
// back to compatible alternatives.
inline const char*
field_type_to_mariadb_sql(
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
            if constexpr (mariadb_client_capabilities::has_json_type)
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
            if constexpr (mariadb_client_capabilities::has_uuid_type)
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


//==============================================================================
// 5.  VERSION INFORMATION
//==============================================================================


// 5.1    The version
//------------------------------------------------------------------------------
// 5.1.1
// mariadb_version_info
//   struct: compile-time version decomposition of the client header env
// detected; the server's is the connection's to ask (get_server_version).
struct mariadb_version_info
{
#if D_ENV_MARIADB_DETECTED
    static constexpr bool             detected = true;
    static constexpr re_std::uint32_t id       = D_ENV_MARIADB_VERSION_ID;
    static constexpr re_std::uint16_t major    = D_ENV_MARIADB_VERSION_MAJOR;
    static constexpr re_std::uint16_t minor    = D_ENV_MARIADB_VERSION_MINOR;
    static constexpr re_std::uint16_t patch    = D_ENV_MARIADB_VERSION_PATCH;
    static constexpr const char*      string   = D_ENV_MARIADB_VERSION_STRING;
#else
    static constexpr bool             detected = false;
    static constexpr re_std::uint32_t id       = 0;
    static constexpr re_std::uint16_t major    = 0;
    static constexpr re_std::uint16_t minor    = 0;
    static constexpr re_std::uint16_t patch    = 0;
    static constexpr const char*      string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected MariaDB version is at
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
// 6.  CONNECTION CONFIGURATION
//==============================================================================


// 6.1    The configuration
//------------------------------------------------------------------------------
// 6.1.1
// mariadb_connect_config
//   struct: what a MariaDB connection needs beyond the family's: Galera's
// causal reads, and the session's default storage engine. With either Galera
// switch on, the session sets wsrep_sync_wait to 1, so that each read waits
// for the writes the cluster has committed; the default storage engine,
// where named, is set for the session. Both are applied when the connection
// opens.
struct mariadb_connect_config
{
    mysql_connect_config mysql_config;

    // Galera-aware options
    bool galera_wsrep_sync_wait;     // false
    bool galera_wsrep_causal_reads;  // false; the same, by its old name

    // MariaDB-specific init
    std::string default_storage_engine;  // "": the server's

    // construction -- MariaDB's defaults (localhost, 3306, utf8mb4), over a
    // generic configuration's
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
        const connection_config& _base
    )
        : mysql_config(_base),
          galera_wsrep_sync_wait(false),
          galera_wsrep_causal_reads(false)
    {}
};


//==============================================================================
// 7.  MARIADB CONNECTION
//==============================================================================


// 7.1    The class
//------------------------------------------------------------------------------
// 7.1.1
// mariadb_connection
//   class: a connection to a MariaDB server, through either client, over the
// MariaDB layer's face (mysql_common_connection, which holds it). Beside the
// family's operations it says what the server connected has (supports_*),
// maps types, and applies mariadb_connect_config's Galera and storage-engine
// settings when it connects. Its members throw as the family's do. Not
// copyable; movable.
//
// Usage:
//   mariadb_connection conn(config);
//   conn.connect();
//   auto rs = conn.execute_query("SELECT 1");
class mariadb_connection
    : public mysql_common_connection<mariadb_connection,
                                     database_type::mariadb>
{
public:
    using base_type = mysql_common_connection<mariadb_connection,
                                              database_type::mariadb>;

    using client_capabilities = mariadb_client_capabilities;
    using version_info        = mariadb_version_info;

    // type_support, feature_support -- client_capabilities, by their former
    // names (2026.10.21), for one release
    using type_support [[deprecated("client_capabilities")]] =
        mariadb_client_capabilities;
    using feature_support [[deprecated("client_capabilities")]] =
        mariadb_client_capabilities;

    // construction -- with MariaDB's defaults, or a generic, family or
    // MariaDB configuration
    mariadb_connection();
    explicit mariadb_connection(const connection_config& _config);
    explicit mariadb_connection(const mysql_connect_config& _config);
    explicit mariadb_connection(const mariadb_connect_config& _config);

    ~mariadb_connection() = default;

    mariadb_connection(const mariadb_connection&)            = delete;
    mariadb_connection& operator=(const mariadb_connection&) = delete;

    mariadb_connection(mariadb_connection&&) noexcept            = default;
    mariadb_connection& operator=(mariadb_connection&&) noexcept = default;

    // feature queries -- what the server connected has, by its flavor and
    // release, asked of the layer at run time (d_mariadb_supports); false
    // while the connection is not open. client_capabilities answers for the
    // client's header at compile time. json_is_native: JSON stored as binary,
    // as MySQL stores it, where MariaDB's JSON is text (LONGTEXT) checked to
    // be JSON; supports_galera: Galera replication built in, which this
    // server runs or not as it is configured; supports_modern_sql: window
    // functions, CTEs, sequences and system versioning together (on a MySQL
    // server, its own set: window functions, CTEs and LATERAL)
    bool
    supports_inet6_type() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_INET6);
    }

    bool
    supports_uuid_type() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_UUID);
    }

    bool
    supports_json() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_JSON);
    }

    bool
    json_is_native() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_BINARY_JSON);
    }

    bool
    supports_sequences() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_SEQUENCES);
    }

    bool
    supports_system_versioning() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_SYSTEM_VERSIONING);
    }

    bool
    supports_returning() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_RETURNING);
    }

    bool
    supports_galera() const noexcept
    {
        return this->m_supports(D_MARIADB_FEATURE_GALERA);
    }

    bool
    supports_modern_sql() const noexcept
    {
        return this->m_supports_modern_sql();
    }

    // data type mapping

    // map_native_type
    //   function: maps a native wire protocol type to djinterp
    // field_type, with awareness of MariaDB's JSON alias behavior.
    static field_type
    map_native_type(
        mysql_field_type _native_type,
        bool             _is_json_column = false
    ) noexcept
    {
        return mysql_type_to_field_type(_native_type,
                                        _is_json_column);
    }

    // map_extended_type
    //   function: maps a MariaDB-specific extended type to djinterp
    // field_type.
    static field_type
    map_extended_type(
        mariadb_field_type _mariadb_type
    ) noexcept
    {
        return mariadb_extended_type_to_field_type(_mariadb_type);
    }

    // sql_type_name
    //   function: returns the MariaDB SQL type name for a given
    // field_type.
    static const char*
    sql_type_name(
        field_type _type
    ) noexcept
    {
        return field_type_to_mariadb_sql(_type);
    }

    // configuration -- MariaDB's settings; setting them replaces the
    // family's and the generic configuration with theirs, for the next
    // connect
    mariadb_connect_config
         get_mariadb_config() const;
    void set_mariadb_config(const mariadb_connect_config& _config);

    // hooks (mariadb.cpp), for mysql_common_connection's connect: MariaDB
    // adds no client settings of its own, and once connected sets
    // wsrep_sync_wait and the default storage engine where configured,
    // throwing connection_exception when the server refuses either
    void add_settings_impl(internal::mariadb_settings& _settings) const;
    void on_connected_impl(face::mariadb& _db);

private:
    mariadb_connect_config m_mariadb_config;
};

// 7.1.2
// mysql_common_connection<mariadb_connection, ...>
//   class: the family's members for MariaDB, instantiated once, in
// mariadb.cpp.
extern template class mysql_common_connection<mariadb_connection,
                                              database_type::mariadb>;



//==============================================================================
// 8.  EXPRESSION DETECTORS
//==============================================================================
// Sections 8 to 12, once mariadb_traits.hpp and mariadb_concepts.hpp, detect
// which of MariaDB's own members a type has: as traits from C++17 and as
// concepts under C++20. The family's -- the character set, results,
// options, diagnostics, escaping, the current database, users, the storage
// engine -- are mysql_common.hpp's. A detector is the type of one member's
// call, which is_detected tests.


// 8.1    Connection reset
//------------------------------------------------------------------------------
// mariadb_reset_connection_t
//   detector: reset_connection() method.
// wraps mysql_reset_connection() which was introduced in MariaDB
// 10.2.4. Resets the connection to a clean state (clears session
// variables, temporary tables, prepared statements) without
// re-authenticating.
template<typename Type>
using mariadb_reset_connection_t =
    decltype(std::declval<Type&>().reset_connection());

// 8.2    Schema introspection
//------------------------------------------------------------------------------
// mariadb_has_table_t
//   detector: has_table(const std::string&) const method.
template<typename Type>
using mariadb_has_table_t =
    decltype(std::declval<const Type&>().has_table(
        std::declval<const std::string&>()));

// mariadb_list_tables_t
//   detector: list_tables() const method.
template<typename Type>
using mariadb_list_tables_t =
    decltype(std::declval<const Type&>().list_tables());


//==============================================================================
// 9.  TAGGED CAPABILITY TRAITS
//==============================================================================
// Each trait is a type with ::value, true where every detector it names finds
// its member; name_v is the value, where the language has variable templates.
// The family's traits (has_mysql_charset, is_mysql_connection, ...) are
// mysql_common.hpp's.


// 9.1    Traits
//------------------------------------------------------------------------------
// 9.1.1
// has_mariadb_reset
//   trait: checks if type Type supports connection reset.
template<typename Type>
struct has_mariadb_reset
    : is_detected<mariadb_reset_connection_t, clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_mariadb_reset_v =
        has_mariadb_reset<clean_t<Type>>::value;
#endif

// 9.1.2
// has_mariadb_schema_query
//   trait: checks if type Type supports schema introspection
// (has_table + list_tables).
template<typename Type>
struct has_mariadb_schema_query : djinterp::conjunction<
    is_detected<mariadb_has_table_t, clean_t<Type>>,
    is_detected<mariadb_list_tables_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool has_mariadb_schema_query_v =
        has_mariadb_schema_query<clean_t<Type>>::value;
#endif

// 9.1.3
// is_mariadb_connection
//   trait: compound trait verifying type Type implements a MariaDB
// connection interface: is_mysql_connection (mysql_common.hpp), the
// family's, with schema queries, which every mariadb_connection has.
template<typename Type>
struct is_mariadb_connection : djinterp::conjunction<
    is_mysql_connection<clean_t<Type>>,
    has_mariadb_schema_query<clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    constexpr bool is_mariadb_connection_v =
        is_mariadb_connection<clean_t<Type>>::value;
#endif


//==============================================================================
// 10.  TAGLESS CAPABILITY TRAITS
//==============================================================================
// The same capabilities as constexpr bool variable templates, for a condition
// that needs no type: one member each, then the sets. The family's
// (mysql_can_set_charset, mysql_is_full_connection, ...) are
// mysql_common.hpp's.


// 10.1   Individual capabilities
//------------------------------------------------------------------------------
// mariadb_can_reset_connection
//   tagless trait: true if Type has reset_connection().
template<typename Type,
         typename = void>
constexpr bool mariadb_can_reset_connection = false;

template<typename Type>
constexpr bool mariadb_can_reset_connection<Type,
    std::void_t<mariadb_reset_connection_t<Type>>> = true;

// mariadb_can_query_schema
//   tagless trait: true if Type has has_table().
template<typename Type,
         typename = void>
constexpr bool mariadb_can_query_schema = false;

template<typename Type>
constexpr bool mariadb_can_query_schema<Type,
    std::void_t<mariadb_has_table_t<Type>>> = true;

// 10.2   Compound capabilities
//------------------------------------------------------------------------------
// mariadb_does_schema_query
//   tagless trait: true if Type supports full schema introspection.
template<typename Type,
         typename = void>
constexpr bool mariadb_does_schema_query = false;

template<typename Type>
constexpr bool mariadb_does_schema_query<Type, std::void_t<
    mariadb_has_table_t<Type>,
    mariadb_list_tables_t<Type>>> = true;

// mariadb_is_full_connection
//   tagless trait: true if Type satisfies the complete MariaDB
// connection interface (full MySQL-family connection + schema query).
template<typename Type>
constexpr bool mariadb_is_full_connection =
    ( mysql_is_full_connection<clean_t<Type>>  &&
      mariadb_can_query_schema<clean_t<Type>> );


//==============================================================================
// 11.  SFINAE HELPERS
//==============================================================================


// 11.1   enable_if helpers
//------------------------------------------------------------------------------
// enable_if_mariadb_connection
//   type: SFINAE helper for MariaDB connection constraints.
template<typename Type>
using enable_if_mariadb_connection =
    typename std::enable_if<is_mariadb_connection<clean_t<Type>>::value>::type;

// enable_if_has_mariadb_reset
//   type: SFINAE helper for MariaDB reset_connection constraints.
template<typename Type>
using enable_if_has_mariadb_reset =
    typename std::enable_if<has_mariadb_reset<clean_t<Type>>::value>::type;

// enable_if_has_mariadb_schema_query
//   type: SFINAE helper for MariaDB schema query constraints.
template<typename Type>
using enable_if_has_mariadb_schema_query =
    typename std::enable_if<
        has_mariadb_schema_query<clean_t<Type>>::value>::type;


//==============================================================================
// 12.  C++20 CONCEPTS
//==============================================================================
// Each concept forwards to a trait or a tagless capability above. They live
// in djinterp::concepts (2026.10.21), where a concept may take the name of
// the class it constrains; each former name in djinterp forwards to its
// concept for one release. They exist where the compiler has concepts; the
// traits serve at C++17 without them, as in functor.hpp and monoid.hpp.
// Every name is prefixed mariadb_, apart from the family's concepts, which
// are mysql_common.hpp's.


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace concepts
{

// 12.1   Connection concepts
//------------------------------------------------------------------------------
// mariadb_connection
//   concept: constrains types implementing the MariaDB connection
// interface.
template<typename Type>
concept mariadb_connection =
    is_mariadb_connection<clean_t<Type>>::value;

// non_mariadb_connection
//   concept: constrains types that do not implement the MariaDB
// connection interface.
template<typename Type>
concept non_mariadb_connection =
    !mariadb_connection<Type>;

// mariadb_schema_connection
//   concept: constrains MariaDB connections supporting schema
// introspection.
template<typename Type>
concept mariadb_schema_connection =
    has_mariadb_schema_query<clean_t<Type>>::value;

// mariadb_resettable_connection
//   concept: constrains MariaDB connections supporting connection reset.
template<typename Type>
concept mariadb_resettable_connection =
    has_mariadb_reset<clean_t<Type>>::value;

// 12.2   Capability concepts
//------------------------------------------------------------------------------
// mariadb_reset_connection_capable
//   concept: constrains types exposing reset_connection().
template<typename Type>
concept mariadb_reset_connection_capable =
    mariadb_can_reset_connection<clean_t<Type>>;

// mariadb_has_table_query
//   concept: constrains types exposing has_table(name).
template<typename Type>
concept mariadb_has_table_query =
    mariadb_can_query_schema<clean_t<Type>>;

// mariadb_table_names_query
//   concept: constrains types exposing list_tables().
template<typename Type>
concept mariadb_table_names_query =
    is_detected<mariadb_list_tables_t, clean_t<Type>>::value;

// 12.3   Tagless capability concepts
//------------------------------------------------------------------------------
// mariadb_schema_queryable
//   concept: constrains types satisfying the full tagless schema-query
// capability set.
template<typename Type>
concept mariadb_schema_queryable =
    mariadb_does_schema_query<clean_t<Type>>;

// mariadb_full_connection
//   concept: constrains types satisfying the complete tagless MariaDB
// connection capability set.
template<typename Type>
concept mariadb_full_connection =
    mariadb_is_full_connection<clean_t<Type>>;

}  // namespace concepts

// 12.4   Former names
//------------------------------------------------------------------------------
// Mariadb_connection
//   concept: concepts::mariadb_connection, by its former name, for one release.
template<typename Type>
concept Mariadb_connection =
    concepts::mariadb_connection<Type>;

// non_mariadb_connection
//   concept: concepts::non_mariadb_connection, by its former name, for one
// release.
template<typename Type>
concept non_mariadb_connection =
    concepts::non_mariadb_connection<Type>;

// mariadb_schema_connection
//   concept: concepts::mariadb_schema_connection, by its former name, for one
// release.
template<typename Type>
concept mariadb_schema_connection =
    concepts::mariadb_schema_connection<Type>;

// mariadb_resettable_connection
//   concept: concepts::mariadb_resettable_connection, by its former name, for
// one release.
template<typename Type>
concept mariadb_resettable_connection =
    concepts::mariadb_resettable_connection<Type>;

// mariadb_reset_connection_capable
//   concept: concepts::mariadb_reset_connection_capable, by its former name,
// for one release.
template<typename Type>
concept mariadb_reset_connection_capable =
    concepts::mariadb_reset_connection_capable<Type>;

// mariadb_has_table_query
//   concept: concepts::mariadb_has_table_query, by its former name, for one
// release.
template<typename Type>
concept mariadb_has_table_query =
    concepts::mariadb_has_table_query<Type>;

// mariadb_table_names_query
//   concept: concepts::mariadb_table_names_query, by its former name, for one
// release.
template<typename Type>
concept mariadb_table_names_query =
    concepts::mariadb_table_names_query<Type>;

// mariadb_schema_queryable
//   concept: concepts::mariadb_schema_queryable, by its former name, for one
// release.
template<typename Type>
concept mariadb_schema_queryable =
    concepts::mariadb_schema_queryable<Type>;

// mariadb_full_connection
//   concept: concepts::mariadb_full_connection, by its former name, for one
// release.
template<typename Type>
concept mariadb_full_connection =
    concepts::mariadb_full_connection<Type>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MARIADB_MARIADB_HPP
