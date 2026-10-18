/*******************************************************************************
* djinterp [core]                                                   postgres.hpp
*
* djinterp PostgreSQL connection module:
*   The PostgreSQL connection, pg_connection, over the PostgreSQL layer's C++
* face (c/db/postgres/postgres_face.hpp), whose libpq connection it holds:
* every operation is the layer's, and core/db adds exceptions, result sets
* that outlive their query, and prepared statements that close with the
* connection. Beside the operations every connection has, it sends SQL
* without waiting, runs statements with parameters, copies rows in, listens
* for notifications, and quotes values and names. With it, PostgreSQL's
* types and how they map to field_type -- every built-in type has an OID,
* stable across versions, arrays their own -- and the SQL features each
* version has, at compile time from env_postgres.h.
*     pg_connection (this file)
*       -> database_connection<pg_connection, database_type::postgresql>
*         -> connection_template<pg_connection, database_type::postgresql>
*           -> connection<pg_connection>
*   The hooks are defined in postgres.cpp. The header needs C++17 and
* includes no client header: env_postgres.h includes libpq-fe.h only where a
* program asks for env's detection (D_CFG_ENV_USING_POSTGRESQL), which the
* compile-time feature queries read. Its trailing sections carry
* PostgreSQL's capability-detection traits and C++20 concepts.
*
*
* path:      /inc/djinterp/core/db/postgres/postgres.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.26
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_DB_POSTGRES_POSTGRES_HPP
#define DJINTERP_DB_POSTGRES_POSTGRES_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t,
                                                   // uint16_t, uint8_t, int64_t

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>   // std::size_t
#include <map>       // std::map
#include <memory>    // std::unique_ptr
#include <optional>  // std::optional
#include <string>    // std::string
#include <vector>    // std::vector
// djinterp
#include "../../../djinterp.hpp"                     // framework root
#include "../../../c/db/postgres/postgres_face.hpp"  // face::postgres, the
                                                     // layer
#include "../../../env/db/postgres/env_postgres.h"   // D_ENV_PG_*
#include "../../meta/type_utility.hpp"               // clean_t
#include "../database_connection.hpp"                // database_connection
#include "../database_statement.hpp"                 // prepared_statement,
                                                     // failure_kind,
                                                     // buffered_result_set


NS_DJINTERP
// =============================================================================
// I.   POSTGRESQL OID TYPE CONSTANTS
// =============================================================================
// PostgreSQL identifies every data type by a unique OID. Built-in type
// OIDs are stable across all versions. These constants mirror the values
// from the system catalog pg_type and from src/include/catalog/pg_type_d.h.
// They are declared here so that <libpq-fe.h> does not need to be
// included.

// pg_oid
//   enumeration: well-known PostgreSQL type OIDs for built-in types.
enum class pg_oid : unsigned int
{
    // -----------------------------------------------------------------
    // boolean
    // -----------------------------------------------------------------
    oid_bool           = 16,

    // -----------------------------------------------------------------
    // binary
    // -----------------------------------------------------------------
    oid_bytea          = 17,

    // -----------------------------------------------------------------
    // integer family
    // -----------------------------------------------------------------
    oid_int8           = 20,        // BIGINT
    oid_int2           = 21,        // SMALLINT
    oid_int4           = 23,        // INTEGER
    oid_oid            = 26,        // OID type itself

    // -----------------------------------------------------------------
    // floating point and numeric
    // -----------------------------------------------------------------
    oid_float4         = 700,       // REAL
    oid_float8         = 701,       // DOUBLE PRECISION
    oid_numeric        = 1700,      // NUMERIC / DECIMAL
    oid_money          = 790,       // MONEY

    // -----------------------------------------------------------------
    // character types
    // -----------------------------------------------------------------
    oid_char           = 18,        // "char" (single byte)
    oid_name           = 19,        // name (63-byte identifier)
    oid_text           = 25,        // TEXT
    oid_varchar        = 1043,      // CHARACTER VARYING
    oid_bpchar         = 1042,      // CHARACTER (blank-padded)

    // -----------------------------------------------------------------
    // date and time
    // -----------------------------------------------------------------
    oid_date           = 1082,      // DATE
    oid_time           = 1083,      // TIME WITHOUT TIME ZONE
    oid_timetz         = 1266,      // TIME WITH TIME ZONE
    oid_timestamp      = 1114,      // TIMESTAMP WITHOUT TIME ZONE
    oid_timestamptz    = 1184,      // TIMESTAMP WITH TIME ZONE
    oid_interval       = 1186,      // INTERVAL

    // -----------------------------------------------------------------
    // network address types
    // -----------------------------------------------------------------
    oid_inet           = 869,       // INET (IPv4/IPv6 host address)
    oid_cidr           = 650,       // CIDR (IPv4/IPv6 network)
    oid_macaddr        = 829,       // MAC address (6 bytes)
    oid_macaddr8       = 774,       // MAC address (8 bytes, PG 10+)

    // -----------------------------------------------------------------
    // geometric types
    // -----------------------------------------------------------------
    oid_point          = 600,
    oid_line           = 628,
    oid_lseg           = 601,
    oid_box            = 603,
    oid_path           = 602,
    oid_polygon        = 604,
    oid_circle         = 718,

    // -----------------------------------------------------------------
    // JSON types
    // -----------------------------------------------------------------
    oid_json           = 114,       // JSON (text-based, PG 9.2+)
    oid_jsonb          = 3802,      // JSONB (binary, PG 9.4+)
    oid_jsonpath       = 4072,      // JSONPATH (PG 12+)

    // -----------------------------------------------------------------
    // UUID
    // -----------------------------------------------------------------
    oid_uuid           = 2950,

    // -----------------------------------------------------------------
    // XML
    // -----------------------------------------------------------------
    oid_xml            = 142,

    // -----------------------------------------------------------------
    // bit string
    // -----------------------------------------------------------------
    oid_bit            = 1560,      // BIT
    oid_varbit         = 1562,      // BIT VARYING

    // -----------------------------------------------------------------
    // range types (PG 9.2+)
    // -----------------------------------------------------------------
    oid_int4range      = 3904,
    oid_int8range      = 3926,
    oid_numrange       = 3906,
    oid_tsrange        = 3908,
    oid_tstzrange      = 3910,
    oid_daterange      = 3912,

    // -----------------------------------------------------------------
    // multirange types (PG 14+)
    // -----------------------------------------------------------------
    oid_int4multirange = 4451,
    oid_int8multirange = 4536,
    oid_nummultirange  = 4532,
    oid_tsmultirange   = 4533,
    oid_tstzmultirange = 4534,
    oid_datemultirange = 4535,

    // -----------------------------------------------------------------
    // array types (OID of the array version of common types)
    // -----------------------------------------------------------------
    oid_bool_array     = 1000,
    oid_int2_array     = 1005,
    oid_int4_array     = 1007,
    oid_int8_array     = 1016,
    oid_float4_array   = 1021,
    oid_float8_array   = 1022,
    oid_text_array     = 1009,
    oid_varchar_array  = 1015,
    oid_uuid_array     = 2951,
    oid_jsonb_array    = 3807,

    // -----------------------------------------------------------------
    // pseudo-types and internal
    // -----------------------------------------------------------------
    oid_void           = 2278,
    oid_record         = 2249,      // anonymous composite
    oid_unknown        = 705,

    // -----------------------------------------------------------------
    // tsvector / tsquery (full-text search)
    // -----------------------------------------------------------------
    oid_tsvector       = 3614,
    oid_tsquery        = 3615,

    // -----------------------------------------------------------------
    // large object
    // -----------------------------------------------------------------
    oid_pg_lsn         = 3220       // pg_lsn (WAL position)
};


// =============================================================================
// II.  OID-TO-FIELD_TYPE MAPPING
// =============================================================================

// pg_oid_to_field_type
//   function: maps a PostgreSQL OID to the generic djinterp field_type.
inline field_type pg_oid_to_field_type(unsigned int _oid) noexcept
{
    switch (static_cast<pg_oid>(_oid))
    {
        // boolean
        case pg_oid::oid_bool:
            return field_type::boolean;

        // integer family
        case pg_oid::oid_int2:
        case pg_oid::oid_int4:
        case pg_oid::oid_oid:
            return field_type::integer;

        case pg_oid::oid_int8:
            return field_type::big_integer;

        // floating point
        case pg_oid::oid_float4:
        case pg_oid::oid_float8:
        case pg_oid::oid_money:
            return field_type::floating_point;

        // numeric / decimal
        case pg_oid::oid_numeric:
            return field_type::decimal;

        // string types
        case pg_oid::oid_char:
        case pg_oid::oid_name:
        case pg_oid::oid_text:
        case pg_oid::oid_varchar:
        case pg_oid::oid_bpchar:
        case pg_oid::oid_inet:
        case pg_oid::oid_cidr:
        case pg_oid::oid_macaddr:
        case pg_oid::oid_macaddr8:
        case pg_oid::oid_tsvector:
        case pg_oid::oid_tsquery:
        case pg_oid::oid_pg_lsn:
            return field_type::string;

        // binary
        case pg_oid::oid_bytea:
        case pg_oid::oid_bit:
        case pg_oid::oid_varbit:
            return field_type::binary;

        // date
        case pg_oid::oid_date:
            return field_type::date;

        // time
        case pg_oid::oid_time:
        case pg_oid::oid_timetz:
        case pg_oid::oid_interval:
            return field_type::time;

        // datetime / timestamp
        case pg_oid::oid_timestamp:
        case pg_oid::oid_timestamptz:
            return field_type::timestamp;

        // JSON
        case pg_oid::oid_json:
        case pg_oid::oid_jsonb:
        case pg_oid::oid_jsonpath:
            return field_type::json;

        // UUID
        case pg_oid::oid_uuid:
            return field_type::uuid;

        // XML
        case pg_oid::oid_xml:
            return field_type::xml;

        // arrays
        case pg_oid::oid_bool_array:
        case pg_oid::oid_int2_array:
        case pg_oid::oid_int4_array:
        case pg_oid::oid_int8_array:
        case pg_oid::oid_float4_array:
        case pg_oid::oid_float8_array:
        case pg_oid::oid_text_array:
        case pg_oid::oid_varchar_array:
        case pg_oid::oid_uuid_array:
        case pg_oid::oid_jsonb_array:
            return field_type::array;

        // range and multirange map to custom
        case pg_oid::oid_int4range:
        case pg_oid::oid_int8range:
        case pg_oid::oid_numrange:
        case pg_oid::oid_tsrange:
        case pg_oid::oid_tstzrange:
        case pg_oid::oid_daterange:
        case pg_oid::oid_int4multirange:
        case pg_oid::oid_int8multirange:
        case pg_oid::oid_nummultirange:
        case pg_oid::oid_tsmultirange:
        case pg_oid::oid_tstzmultirange:
        case pg_oid::oid_datemultirange:
            return field_type::custom;

        // geometric types
        case pg_oid::oid_point:
        case pg_oid::oid_line:
        case pg_oid::oid_lseg:
        case pg_oid::oid_box:
        case pg_oid::oid_path:
        case pg_oid::oid_polygon:
        case pg_oid::oid_circle:
            return field_type::custom;

        // composite / void / unknown
        case pg_oid::oid_record:
        case pg_oid::oid_void:
        case pg_oid::oid_unknown:
        default:
            return field_type::custom;
    }
}

// field_type_to_pg_sql
//   function: returns the SQL type name string for a given field_type
// as it would be used in PostgreSQL DDL.
inline const char* field_type_to_pg_sql(field_type _type) noexcept
{
    switch (_type)
    {
        case field_type::null:           return "NULL";
        case field_type::boolean:        return "BOOLEAN";
        case field_type::integer:        return "INTEGER";
        case field_type::big_integer:    return "BIGINT";
        case field_type::floating_point: return "DOUBLE PRECISION";
        case field_type::decimal:        return "NUMERIC";
        case field_type::string:         return "TEXT";
        case field_type::binary:         return "BYTEA";
        case field_type::date:           return "DATE";
        case field_type::time:           return "TIME";
        case field_type::datetime:       return "TIMESTAMP";
        case field_type::timestamp:      return "TIMESTAMPTZ";
        case field_type::json:           return "JSONB";
        case field_type::xml:            return "XML";
        case field_type::uuid:           return "UUID";
        case field_type::array:          return "TEXT[]";
        case field_type::custom:
        default:                         return "TEXT";
    }
}

// pg_oid_is_array
//   function: returns true if the given OID represents an array type.
// Built-in array OIDs are in the 1000-range; this checks the common
// ones. For user-defined array types, query pg_type.typarray at
// runtime.
inline bool pg_oid_is_array(unsigned int _oid) noexcept
{
    pg_oid oid = static_cast<pg_oid>(_oid);

    return ( (oid == pg_oid::oid_bool_array)    ||
             (oid == pg_oid::oid_int2_array)    ||
             (oid == pg_oid::oid_int4_array)    ||
             (oid == pg_oid::oid_int8_array)    ||
             (oid == pg_oid::oid_float4_array)  ||
             (oid == pg_oid::oid_float8_array)  ||
             (oid == pg_oid::oid_text_array)    ||
             (oid == pg_oid::oid_varchar_array) ||
             (oid == pg_oid::oid_uuid_array)    ||
             (oid == pg_oid::oid_jsonb_array) );
}


// =============================================================================
// III. FEATURE SUPPORT (compile-time, version-gated)
// =============================================================================

// pg_type_support
//   struct: compile-time data type availability flags gated by
// D_ENV_PG_* macros.
struct pg_type_support
{
#if D_ENV_PG_DETECTED

    // data types
    static constexpr bool has_json =
    #if D_ENV_PG_HAS_JSON
        true;
    #else
        false;
    #endif

    static constexpr bool has_jsonb =
    #if D_ENV_PG_HAS_JSONB
        true;
    #else
        false;
    #endif

    static constexpr bool has_jsonpath =
    #if D_ENV_PG_HAS_JSONPATH
        true;
    #else
        false;
    #endif

    static constexpr bool has_json_table =
    #if D_ENV_PG_HAS_JSON_TABLE
        true;
    #else
        false;
    #endif

    static constexpr bool has_sqljson_constructors =
    #if D_ENV_PG_HAS_SQLJSON_CONSTRUCTORS
        true;
    #else
        false;
    #endif

    static constexpr bool has_jsonb_subscript =
    #if D_ENV_PG_HAS_JSONB_SUBSCRIPT
        true;
    #else
        false;
    #endif

    static constexpr bool has_range_types =
    #if D_ENV_PG_HAS_RANGE_TYPES
        true;
    #else
        false;
    #endif

    static constexpr bool has_multirange =
    #if D_ENV_PG_HAS_MULTIRANGE
        true;
    #else
        false;
    #endif

    static constexpr bool has_array_types =
    #if D_ENV_PG_HAS_ARRAY_TYPES
        true;
    #else
        false;
    #endif

    static constexpr bool has_uuid_type =
    #if D_ENV_PG_HAS_UUID_TYPE
        true;
    #else
        false;
    #endif

    static constexpr bool has_enum_type =
    #if D_ENV_PG_HAS_ENUM_TYPE
        true;
    #else
        false;
    #endif

    static constexpr bool has_domain_types =
    #if D_ENV_PG_HAS_DOMAIN_TYPES
        true;
    #else
        false;
    #endif

    static constexpr bool has_composite_types =
    #if D_ENV_PG_HAS_COMPOSITE_TYPES
        true;
    #else
        false;
    #endif

    static constexpr bool has_identity_columns =
    #if D_ENV_PG_HAS_IDENTITY_COLUMNS
        true;
    #else
        false;
    #endif

    static constexpr bool has_generated_columns =
    #if D_ENV_PG_HAS_GENERATED_COLUMNS
        true;
    #else
        false;
    #endif

    static constexpr bool has_generated_virtual =
    #if D_ENV_PG_HAS_GENERATED_VIRTUAL
        true;
    #else
        false;
    #endif

#else
    static constexpr bool has_json                 = false;
    static constexpr bool has_jsonb                = false;
    static constexpr bool has_jsonpath             = false;
    static constexpr bool has_json_table           = false;
    static constexpr bool has_sqljson_constructors = false;
    static constexpr bool has_jsonb_subscript      = false;
    static constexpr bool has_range_types          = false;
    static constexpr bool has_multirange           = false;
    static constexpr bool has_array_types          = false;
    static constexpr bool has_uuid_type            = false;
    static constexpr bool has_enum_type            = false;
    static constexpr bool has_domain_types         = false;
    static constexpr bool has_composite_types      = false;
    static constexpr bool has_identity_columns     = false;
    static constexpr bool has_generated_columns    = false;
    static constexpr bool has_generated_virtual    = false;
#endif  // D_ENV_PG_DETECTED
};

// pg_feature_support
//   struct: compile-time SQL and server feature availability flags.
struct pg_feature_support
{
#if D_ENV_PG_DETECTED

    // SQL features
    static constexpr bool has_cte =
    #if D_ENV_PG_HAS_CTE
        true;
    #else
        false;
    #endif
    static constexpr bool has_cte_materialized =
    #if D_ENV_PG_HAS_CTE_MATERIALIZED
        true;
    #else
        false;
    #endif
    static constexpr bool has_window_functions =
    #if D_ENV_PG_HAS_WINDOW_FUNCTIONS
        true;
    #else
        false;
    #endif
    static constexpr bool has_lateral =
    #if D_ENV_PG_HAS_LATERAL
        true;
    #else
        false;
    #endif
    static constexpr bool has_upsert =
    #if D_ENV_PG_HAS_UPSERT
        true;
    #else
        false;
    #endif
    static constexpr bool has_merge =
    #if D_ENV_PG_HAS_MERGE
        true;
    #else
        false;
    #endif
    static constexpr bool has_grouping_sets =
    #if D_ENV_PG_HAS_GROUPING_SETS
        true;
    #else
        false;
    #endif
    static constexpr bool has_row_level_security =
    #if D_ENV_PG_HAS_ROW_LEVEL_SECURITY
        true;
    #else
        false;
    #endif
    static constexpr bool has_stored_procedures =
    #if D_ENV_PG_HAS_STORED_PROCEDURES
        true;
    #else
        false;
    #endif
    static constexpr bool has_returning =
    #if D_ENV_PG_HAS_RETURNING
        true;
    #else
        false;
    #endif
    static constexpr bool has_materialized_views =
    #if D_ENV_PG_HAS_MATERIALIZED_VIEWS
        true;
    #else
        false;
    #endif
    static constexpr bool has_full_text_search =
    #if D_ENV_PG_HAS_FULL_TEXT_SEARCH
        true;
    #else
        false;
    #endif
    static constexpr bool has_tablesample =
    #if D_ENV_PG_HAS_TABLESAMPLE
        true;
    #else
        false;
    #endif

    // partitioning
    static constexpr bool has_declarative_partitioning =
    #if D_ENV_PG_HAS_DECLARATIVE_PARTITIONING
        true;
    #else
        false;
    #endif
    static constexpr bool has_hash_partitioning =
    #if D_ENV_PG_HAS_HASH_PARTITIONING
        true;
    #else
        false;
    #endif

    // replication
    static constexpr bool has_logical_repl =
    #if D_ENV_PG_HAS_LOGICAL_REPL
        true;
    #else
        false;
    #endif
    static constexpr bool has_streaming_repl =
    #if D_ENV_PG_HAS_STREAMING_REPL
        true;
    #else
        false;
    #endif

    // parallel
    static constexpr bool has_parallel_query =
    #if D_ENV_PG_HAS_PARALLEL_QUERY
        true;
    #else
        false;
    #endif
    static constexpr bool has_jit =
    #if D_ENV_PG_HAS_JIT
        true;
    #else
        false;
    #endif

    // indexes
    static constexpr bool has_covering_index =
    #if D_ENV_PG_HAS_COVERING_INDEX
        true;
    #else
        false;
    #endif
    static constexpr bool has_brin =
    #if D_ENV_PG_HAS_INDEX_BRIN
        true;
    #else
        false;
    #endif

    // libpq
    static constexpr bool has_pipeline =
    #if D_ENV_PG_HAS_LIBPQ_PIPELINE
        true;
    #else
        false;
    #endif
    static constexpr bool has_chunked_result =
    #if D_ENV_PG_HAS_LIBPQ_CHUNKED_RESULT
        true;
    #else
        false;
    #endif
    static constexpr bool has_close_prepared =
    #if D_ENV_PG_HAS_LIBPQ_CLOSE_PREPARED
        true;
    #else
        false;
    #endif

    // auth
    static constexpr bool has_scram =
    #if D_ENV_PG_HAS_SCRAM_SHA_256
        true;
    #else
        false;
    #endif

    // extensions
    static constexpr bool has_fdw =
    #if D_ENV_PG_HAS_FDW
        true;
    #else
        false;
    #endif
    static constexpr bool has_create_extension =
    #if D_ENV_PG_HAS_CREATE_EXTENSION
        true;
    #else
        false;
    #endif
    static constexpr bool has_event_triggers =
    #if D_ENV_PG_HAS_EVENT_TRIGGERS
        true;
    #else
        false;
    #endif

    // composite
    static constexpr bool has_modern_sql =
    #if D_ENV_PG_HAS_MODERN_SQL
        true;
    #else
        false;
    #endif
    static constexpr bool has_modern_json =
    #if D_ENV_PG_HAS_MODERN_JSON
        true;
    #else
        false;
    #endif
    static constexpr bool is_fully_modern =
    #if D_ENV_PG_IS_FULLY_MODERN
        true;
    #else
        false;
    #endif

#else
    static constexpr bool has_cte                      = false;
    static constexpr bool has_cte_materialized         = false;
    static constexpr bool has_window_functions         = false;
    static constexpr bool has_lateral                   = false;
    static constexpr bool has_upsert                   = false;
    static constexpr bool has_merge                    = false;
    static constexpr bool has_grouping_sets            = false;
    static constexpr bool has_row_level_security       = false;
    static constexpr bool has_stored_procedures        = false;
    static constexpr bool has_returning                = false;
    static constexpr bool has_materialized_views       = false;
    static constexpr bool has_full_text_search         = false;
    static constexpr bool has_tablesample              = false;
    static constexpr bool has_declarative_partitioning = false;
    static constexpr bool has_hash_partitioning        = false;
    static constexpr bool has_logical_repl             = false;
    static constexpr bool has_streaming_repl           = false;
    static constexpr bool has_parallel_query           = false;
    static constexpr bool has_jit                      = false;
    static constexpr bool has_covering_index           = false;
    static constexpr bool has_brin                     = false;
    static constexpr bool has_pipeline                 = false;
    static constexpr bool has_chunked_result           = false;
    static constexpr bool has_close_prepared           = false;
    static constexpr bool has_scram                    = false;
    static constexpr bool has_fdw                      = false;
    static constexpr bool has_create_extension         = false;
    static constexpr bool has_event_triggers           = false;
    static constexpr bool has_modern_sql               = false;
    static constexpr bool has_modern_json              = false;
    static constexpr bool is_fully_modern              = false;
#endif  // D_ENV_PG_DETECTED
};


// =============================================================================
// IV.  VERSION INFORMATION
// =============================================================================

// pg_version_info
//   struct: compile-time version decomposition.
// NOTE: PostgreSQL changed its versioning scheme at version 10.
// Pre-10 uses three-part (e.g. 9.6.24 = 90624); post-10 uses
// two-part (e.g. 16.2 = 160002). The encoding formula is
// MAJOR*10000 + (MINOR_LEGACY*100 if pre-10) + PATCH.
struct pg_version_info
{
#if D_ENV_PG_DETECTED
    static constexpr bool          detected = true;
    static constexpr re_std::uint32_t id       = D_ENV_PG_VERSION_ID;
    static constexpr re_std::uint16_t major    = D_ENV_PG_VERSION_MAJOR;
    static constexpr re_std::uint16_t release  = D_ENV_PG_VERSION_RELEASE;
    static constexpr bool          is_legacy_versioning =
        D_ENV_PG_IS_LEGACY_VERSIONING;
    static constexpr const char*   string   = D_ENV_PG_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr re_std::uint32_t id       = 0;
    static constexpr re_std::uint16_t major    = 0;
    static constexpr re_std::uint16_t release  = 0;
    static constexpr bool          is_legacy_versioning = false;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected version is at least
    // the specified major version (post-10 convention).
    static constexpr bool at_least(re_std::uint16_t _major) noexcept
    {
        return id >= (_major * 10000u);
    }

    // at_least_legacy
    //   function: returns true if the detected version is at least
    // the specified pre-10 (major, minor, patch) version.
    static constexpr bool at_least_legacy(
        re_std::uint16_t _major,
        re_std::uint16_t _minor,
        re_std::uint16_t _patch) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// V.   SSL MODE ENUMERATION
// =============================================================================

// pg_ssl_mode
//   enumeration: PostgreSQL SSL connection modes (sslmode parameter).
enum class pg_ssl_mode : re_std::uint8_t
{
    disable       = 0,      // no SSL
    allow         = 1,      // try non-SSL, then SSL
    prefer        = 2,      // try SSL, then non-SSL (default)
    require       = 3,      // require SSL, no CA verification
    verify_ca     = 4,      // require SSL + verify CA
    verify_full   = 5       // require SSL + verify CA + hostname
};


// =============================================================================
// VI.  POSTGRESQL CONNECTION CONFIGURATION
// =============================================================================

// pg_connect_config
//   struct: what a PostgreSQL connection needs beyond connection_config, as
// libpq's connection keywords. A field left as it was made leaves its
// keyword to libpq -- its environment variables (PGHOST, PGSSLMODE, ...),
// its password and service files -- and to the layer's defaults, UTF8 and a
// bounded connect timeout; a field set gives its keyword. connection_string,
// keywords ("host=db.example dbname=app") or a URI, has the last word: what
// it names overrides every field, and the fields fill in what it leaves out,
// base.database among them. Constructed from a string alone, the
// configuration names nothing else, not even base's host, which libpq then
// chooses as for any program given that string.
//   ssl_mode            sslmode, where not prefer, libpq's own default; else,
//                       with base.enable_ssl, verify-full, or require without
//                       base.verify_ssl
//   application_name    application_name
//   search_path         the session's search path, set once connected, as
//                       SQL writes the list ("app, public"); where it is
//                       empty, base.schema names a single schema, or,
//                       holding a comma, is such a list
//   client_encoding     client_encoding; where it is empty, base.charset
//   options             options, the server's command-line options
//                       ("-c work_mem=64MB"), which replace the layer's own
//   connect_timeout_seconds
//                       connect_timeout; where 0, base.connect_timeout
//   statement_timeout_ms
//                       the session's statement_timeout, set once connected,
//                       where not 0
//   use_binary_results  ignored: results arrive as text, which the layer
//                       reads into the values binary would give
//   extra_params        any keyword libpq knows, by its name, given after
//                       the fields and base.custom_options, which are given
//                       the same way, "scheme" left out:
//                       "target_session_attrs", "sslrootcert"
// base.host may list several hosts, each with its port, as a URI writes
// them ("a:5432,b:5433"), which libpq is given as its two lists. base's
// ssl_cert, ssl_key and ssl_ca are sslcert, sslkey and sslrootcert; base's
// read_timeout, write_timeout and max_packet_size have no keyword in libpq,
// and go unused.
struct pg_connect_config
{
    connection_config       base;
    pg_ssl_mode             ssl_mode;                 // prefer
    std::string             application_name;
    std::string             search_path;
    std::string             client_encoding;          // "": the layer's UTF8
    std::string             options;
    std::string             connection_string;
    int                     connect_timeout_seconds;  // 0: base's
    int                     statement_timeout_ms;     // 0: the server's
    bool                    use_binary_results;       // ignored

    std::map<std::string, std::string> extra_params;

    pg_connect_config()
        : ssl_mode(pg_ssl_mode::prefer),
          connect_timeout_seconds(0),
          statement_timeout_ms(0),
          use_binary_results(false)
    {
        base.host = "localhost";
        base.port = 5432;
    }

    explicit pg_connect_config(
        const connection_config& _base
    )
        : base(_base),
          ssl_mode(pg_ssl_mode::prefer),
          connect_timeout_seconds(0),
          statement_timeout_ms(0),
          use_binary_results(false)
    {}

    explicit pg_connect_config(
        const std::string& _conn_string
    )
        : ssl_mode(pg_ssl_mode::prefer),
          connection_string(_conn_string),
          connect_timeout_seconds(0),
          statement_timeout_ms(0),
          use_binary_results(false)
    {
        // the string alone says where: libpq's defaults for what it does not
        base.host.clear();
    }
};


// =============================================================================
// VII. POSTGRESQL CONNECTION
// =============================================================================

// pg_notification
//   struct: a notification that arrived on a channel the connection listens
// on: the channel, the payload ("" for none), and the process id of the
// sender's backend, as pg_backend_pid() names it there.
struct pg_notification
{
    std::string channel;
    std::string payload;
    int         backend_pid;
};

// postgres_layer
//   struct: the PostgreSQL layer, as database_statement.hpp's templates take
// a layer. A result means the connection was lost when the layer says so
// (08006), when the server says the connection does not exist (08003), or
// when the server ended the session: 57P01 (an administrator's command),
// 57P02 (a crash), 57P03 (no connections now), 57P04 (its database
// dropped), 57P05 (idle too long) or 25P03 (idle too long in a
// transaction). A point in time is bound as its UTC date and time with the
// offset "+00:00", which timestamptz reads as the instant it is, and
// timestamp as that date and time.
struct postgres_layer
{
    using statement_face = face::postgres_statement;
    using handle_type    = struct ::pg_conn*;

    static constexpr const char* utc_offset = "+00:00";

    static constexpr bool
    connection_lost(
        int _result
    ) noexcept
    {
        return ( (_result == D_POSTGRES_SQLSTATE_CONNECTION_FAILURE)          ||
                 (_result == D_POSTGRES_SQLSTATE('0', '8', '0', '0', '3'))    ||
                 (_result == D_POSTGRES_SQLSTATE('5', '7', 'P', '0', '1'))    ||
                 (_result == D_POSTGRES_SQLSTATE('5', '7', 'P', '0', '2'))    ||
                 (_result == D_POSTGRES_SQLSTATE('5', '7', 'P', '0', '3'))    ||
                 (_result == D_POSTGRES_SQLSTATE('5', '7', 'P', '0', '4'))    ||
                 (_result == D_POSTGRES_SQLSTATE('5', '7', 'P', '0', '5'))    ||
                 (_result == D_POSTGRES_SQLSTATE('2', '5', 'P', '0', '3')) );
    }

    /**
     * @brief Describes a failed call of the PostgreSQL layer, as raise does.
     *
     * @note The connection's message describes its latest failure only until
     *       the next call on it, which is why an operation that must call
     *       again before it throws describes the failure first.
     *
     * @param[in] _db         the connection the call ran on; may be `NULL`.
     * @param[in] _operation  the operation's name.
     * @param[in] _result     the call's result.
     * @return "PostgreSQL", the operation, and the failure: the server's or
     *         libpq's message, its severity left out, where the connection
     *         has one about it, the result's own description otherwise; then
     *         the SQLSTATE, or the number for a result of db_common.h's; then
     *         the lines that follow the message, its DETAIL and HINT among
     *         them.
     * @throws std::bad_alloc when memory runs out.
     */
    static std::string describe(handle_type _db,
                                const char* _operation,
                                int         _result);

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
    [[noreturn]] static void raise_as(failure_kind       _kind,
                                      const std::string& _message,
                                      int                _result);

    /**
     * @brief Throws the exception for a failed call of the PostgreSQL layer.
     *
     * @param[in] _db         the connection the call ran on; may be `NULL`.
     * @param[in] _kind       which exception to throw, unless `_result`
     *                        means the connection was lost.
     * @param[in] _operation  the operation's name, for the message.
     * @param[in] _result     the call's result.
     * @throws connection_exception for a lost connection, otherwise as
     *         `_kind` says, with `_result` -- the SQLSTATE packed, or a
     *         negative D_DB_* -- as its error_code().
     */
    [[noreturn]] static void raise(handle_type  _db,
                                   failure_kind _kind,
                                   const char*  _operation,
                                   int          _result);
};

// prepared_statement<postgres_layer>
//   class: PostgreSQL's prepared statements, instantiated once, in
// postgres.cpp.
extern template class prepared_statement<postgres_layer>;

// pg_connection
//   class: a connection to a PostgreSQL server, through libpq, over the
// PostgreSQL layer's face, which it holds and which closes the connection
// when it goes. Connecting gives libpq what pg_connect_config names, in the
// order it describes, and then sets the session's search path and statement
// timeout where they are named; a connection is either open with all of it
// or not open. Parameters are written $1, $2, ...; a prepared statement is
// the server's, deallocated when it closes. Transactions begin at the
// session's isolation level, READ COMMITTED unless set_isolation_level or
// the server says otherwise, or at the level begin_transaction asks for:
// read_uncommitted runs as READ COMMITTED, as PostgreSQL runs it. A
// statement that fails inside a transaction leaves it failed, as in any
// PostgreSQL client: everything but a rollback, whole or to a savepoint,
// throws query_exception with 25P02 until then. There is no auto-commit
// mode to turn off: outside a transaction every statement commits on its
// own. The members the connection adds -- statements sent without waiting,
// parameters, COPY, notifications, escaping, the server's parameters, the
// schema -- throw as the operations do (database_connection.hpp). Not
// copyable; movable.
//
// Usage:
//   pg_connection conn("host=localhost dbname=mydb");
//   conn.connect();
//   auto rs = conn.execute_query("SELECT 1");
class pg_connection
    : public database_connection<pg_connection,
                                 database_type::postgresql>
{
public:
    using base_type       = database_connection<pg_connection,
                                                database_type::postgresql>;
    using layer_type      = postgres_layer;
    using statement_type  = prepared_statement<postgres_layer>;
    using type_support    = pg_type_support;
    using feature_support = pg_feature_support;
    using version_info    = pg_version_info;

    // construction -- with pg_connect_config's defaults (localhost, 5432);
    // with a generic configuration; with a PostgreSQL one; or with a
    // connection string, keywords or a URI, alone, libpq's defaults standing
    // in for what it leaves out
    pg_connection();
    explicit pg_connection(const connection_config& _config);
    explicit pg_connection(const pg_connect_config& _config);
    explicit pg_connection(const std::string& _conn_string);

    /**
     * @brief Closes the connection, its prepared statements first.
     */
    ~pg_connection();

    pg_connection(const pg_connection&)            = delete;
    pg_connection& operator=(const pg_connection&) = delete;

    pg_connection(pg_connection&&) noexcept            = default;
    pg_connection& operator=(pg_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // asynchronous query dispatch
    // -----------------------------------------------------------------

    /**
     * @brief Sends SQL -- one statement or several, separated by
     *        semicolons -- without waiting for it to run.
     *
     * @param[in] _query  the SQL.
     * @return true; a failure throws.
     * @throws query_exception with 08000 while SQL sent before still has
     *         results to read, or a COPY is in progress; as every operation
     *         when the connection is not open, or is lost.
     * @post Until get_result returns no result, nothing else can be sent;
     *       an operation that runs SQL first reads, and discards, the
     *       results still to come.
     */
    bool send_query(const std::string& _query);

    /**
     * @brief Reads the next result of the SQL sent, waiting for it.
     *
     * @return the next statement's rows -- none, and no columns, for a
     *         statement that returns none -- whose count get_affected_rows()
     *         then answers; nullptr once every result has been read.
     * @throws query_exception with the SQLSTATE of a statement that failed,
     *         after which only nullptr follows; as every operation when the
     *         connection is not open, or is lost.
     */
    std::unique_ptr<buffered_result_set> get_result();

    // waiting -- is_busy says whether the next result has yet to arrive,
    // get_result then waiting for it, false when nothing was sent or the
    // connection is not open; consume_input reads what the server has
    // sent, without waiting, and returns true, throwing as every operation
    bool is_busy() const noexcept;
    bool consume_input();

    // -----------------------------------------------------------------
    // parameterized queries
    // -----------------------------------------------------------------

    /**
     * @brief Runs one statement with values for its parameters, $1 the
     *        first, and returns its rows.
     *
     * @param[in] _query   the statement.
     * @param[in] _params  the values, as text the parameters' types read;
     *                     in the second form std::nullopt for NULL.
     * @return the rows, all of them, held apart from the connection.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a value that
     *         holds a NUL, and with the server's SQLSTATE when the statement
     *         fails; as every operation when the connection is not open.
     */
    std::unique_ptr<buffered_result_set> exec_params(
        const std::string&              _query,
        const std::vector<std::string>& _params);
    std::unique_ptr<buffered_result_set> exec_params(
        const std::string&                              _query,
        const std::vector<std::optional<std::string>>& _params);

    // -----------------------------------------------------------------
    // COPY protocol
    // -----------------------------------------------------------------

    /**
     * @brief Runs a COPY ... FROM STDIN, which then waits for rows.
     *
     * @param[in] _copy_sql  the COPY, as written ("COPY t (a, b) FROM
     *                       STDIN", "COPY t FROM STDIN (FORMAT csv)").
     * @throws query_exception with D_DB_INVALID_ARGUMENT for SQL that is no
     *         COPY FROM STDIN -- which has run, done what it does --, with
     *         D_DB_UNSUPPORTED for a COPY to the client, and with the
     *         server's SQLSTATE when it fails; as every operation when the
     *         connection is not open.
     * @post The connection runs nothing but copy_data, copy_end and
     *       copy_abort until the second or the third ends the COPY.
     */
    void copy_in_start(const std::string& _copy_sql);

    // copy_data -- bytes of rows in the COPY's format, in chunks that may
    // end anywhere, a row's middle included; query_exception with 08000
    // when no COPY is in progress. A row the server cannot read is
    // copy_end's failure, which the server's refusal reaches
    void copy_data(const char*  _data,
                   std::size_t  _length);

    /**
     * @brief Ends the COPY in progress, keeping every row sent.
     *
     * @return how many rows the COPY took, which get_affected_rows() then
     *         answers.
     * @throws query_exception with the server's SQLSTATE for rows it
     *         refused (22P02, 23505, ...), none of them kept; with 08000 when
     *         no COPY is in progress.
     */
    re_std::int64_t copy_end();

    // copy_abort -- ends the COPY in progress keeping none of its rows, the
    // reason the server's message for it; query_exception with 08000 when
    // no COPY is in progress
    void copy_abort(const std::string& _reason);

    // -----------------------------------------------------------------
    // LISTEN / NOTIFY
    // -----------------------------------------------------------------

    // channels -- listen subscribes the connection to a channel, unlisten
    // ends that, unlisten_all every subscription; inside a transaction each
    // takes effect when it commits. notify sends a notification, with a
    // payload of up to 7999 bytes, to every connection listening, this one
    // included. Each throws query_exception with D_DB_INVALID_ARGUMENT for
    // an empty channel or text that holds a NUL, and with the server's
    // SQLSTATE (22023 for a payload too long)
    void listen(const std::string& _channel);
    void unlisten(const std::string& _channel);
    void unlisten_all();
    void notify(const std::string& _channel,
                const std::string& _payload = "");

    /**
     * @brief Reads the next notification to have arrived, without waiting
     *        for one.
     *
     * @return the notification; std::nullopt when none has arrived.
     * @throws as every operation when the connection is not open, or is
     *         lost.
     */
    std::optional<pg_notification> get_notification();

    // -----------------------------------------------------------------
    // escaping
    // -----------------------------------------------------------------

    // escaping -- escape_literal quotes text as a string literal, for the
    // connection's encoding, the quotes included ('it''s'); escape_identifier
    // quotes a name in double quotes, any double quote doubled ("a""b"),
    // and needs no connection. Each throws query_exception with
    // D_DB_INVALID_ARGUMENT for text that holds a NUL; escape_literal throws
    // as every operation when the connection is not open
    std::string escape_literal(const std::string& _input) const;
    std::string escape_identifier(const std::string& _input) const;

    // -----------------------------------------------------------------
    // connection diagnostics
    // -----------------------------------------------------------------

    // diagnostics, with no round trip -- the process id of the server's
    // backend for the connection, 0 when it is not open; where the
    // connection stands: 'I' idle, 'A' with SQL sent still running or its
    // results to read, or a COPY in progress, 'T' in a transaction, 'E' in
    // a failed one, 'U' not open or lost; and a parameter the server
    // reports ("server_version", "server_encoding", "TimeZone",
    // "application_name", ...), "" for one it does not, or when the
    // connection is not open
    int         get_backend_pid() const noexcept;
    char        get_transaction_status() const noexcept;
    std::string get_parameter_status(const std::string& _name) const;

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    // schema -- whether a table of the current schema exists (the first of
    // the search path that does, as current_schema() names it), which
    // throws when it cannot tell; and its tables' names, sorted
    bool                     has_table(const std::string& _table_name) const;
    std::vector<std::string> list_tables() const;

    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_jsonb() noexcept
    {
        return type_support::has_jsonb;
    }

    static constexpr bool supports_jsonpath() noexcept
    {
        return type_support::has_jsonpath;
    }

    static constexpr bool supports_multirange() noexcept
    {
        return type_support::has_multirange;
    }

    static constexpr bool supports_merge() noexcept
    {
        return feature_support::has_merge;
    }

    static constexpr bool supports_pipeline() noexcept
    {
        return feature_support::has_pipeline;
    }

    static constexpr bool supports_logical_repl() noexcept
    {
        return feature_support::has_logical_repl;
    }

    static constexpr bool supports_partitioning() noexcept
    {
        return feature_support::has_declarative_partitioning;
    }

    static constexpr bool supports_parallel_query() noexcept
    {
        return feature_support::has_parallel_query;
    }

    static constexpr bool supports_modern_sql() noexcept
    {
        return feature_support::has_modern_sql;
    }

    static constexpr bool supports_modern_json() noexcept
    {
        return feature_support::has_modern_json;
    }

    // -----------------------------------------------------------------
    // data type mapping
    // -----------------------------------------------------------------

    static field_type map_oid(unsigned int _oid) noexcept
    {
        return pg_oid_to_field_type(_oid);
    }

    static bool oid_is_array(unsigned int _oid) noexcept
    {
        return pg_oid_is_array(_oid);
    }

    static const char* sql_type_name(field_type _type) noexcept
    {
        return field_type_to_pg_sql(_type);
    }

    // -----------------------------------------------------------------
    // PostgreSQL-specific configuration
    // -----------------------------------------------------------------

    // configuration -- the PostgreSQL settings, base among them, which
    // connect(config) replaces; setting them replaces the generic
    // configuration with theirs, for the next connect
    const pg_connect_config& get_pg_config() const noexcept;
    void                     set_pg_config(const pg_connect_config& _config);

    // get_face -- the PostgreSQL layer's face, for the layer's other calls;
    // closing or replacing the connection through it is the connection's to
    // do, not the face's
    face::postgres&       get_face() noexcept;
    const face::postgres& get_face() const noexcept;

    // -----------------------------------------------------------------
    // hooks (postgres.cpp): database_connection calls these, programs the
    // operations they implement
    // -----------------------------------------------------------------

    void            connect_helper();
    void            disconnect_helper() noexcept;
    bool            is_connected_helper() const noexcept;
    bool            ping_helper() const noexcept;
    face::rows      execute_query_helper(const std::string& _query);
    re_std::int64_t execute_update_helper(const std::string& _query);
    face::postgres_statement
                    prepare_helper(const std::string& _query);
    std::string     get_server_version_helper() const;
    re_std::int64_t get_last_insert_id_helper() const;
    void            begin_transaction_helper();
    void            begin_transaction_helper(isolation_level _isolation);
    void            commit_helper();
    void            rollback_helper();
    bool            in_transaction_helper() const noexcept;
    isolation_level get_isolation_level_helper() const;
    void            set_isolation_level_helper(isolation_level _level);
    void            create_savepoint_helper(const std::string& _name);
    void            release_savepoint_helper(const std::string& _name);
    void            rollback_to_savepoint_helper(const std::string& _name);
    struct ::pg_conn*
                    native_handle_helper() const noexcept;

    // version-gated methods: declared where env detects the feature in
    // libpq's header, and not yet defined -- the PostgreSQL layer has no
    // pipeline, chunked results or closing by name for them to stand on

#if D_ENV_PG_DETECTED
    #if D_ENV_PG_HAS_LIBPQ_PIPELINE
    // enter_pipeline / exit_pipeline / pipeline_sync
    //   functions: pipeline mode API. Available since PostgreSQL 14.
    void enter_pipeline();
    void exit_pipeline();
    void pipeline_sync();
    #endif

    #if D_ENV_PG_HAS_LIBPQ_CHUNKED_RESULT
    // set_chunked_rows_mode
    //   function: enables chunked result retrieval.
    // Available since PostgreSQL 17.
    void set_chunked_rows_mode(int _chunk_size);
    #endif

    #if D_ENV_PG_HAS_LIBPQ_CLOSE_PREPARED
    // close_prepared
    //   function: closes a server-side prepared statement.
    // Available since PostgreSQL 17.
    void close_prepared(const std::string& _stmt_name);
    #endif
#endif  // D_ENV_PG_DETECTED

private:
    // m_check -- nothing for 0; otherwise what the layer raises
    void m_check(int          _result,
                 failure_kind _kind,
                 const char*  _operation) const;

    // m_exec_params -- the rows of one statement run with values for its
    // parameters, NULL for SQL NULL
    std::unique_ptr<buffered_result_set> m_exec_params(
        const std::string&              _query,
        const std::vector<const char*>& _values);

    pg_connect_config m_pg_config;
    face::postgres    m_db;
};


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former postgres_traits.hpp / postgres_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// =============================================================================
// VIII. EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  asynchronous query dispatch
// -------------------------------------------------------------------------

// pg_send_query_t
//   detector: send_query(const std::string&) method.
// wraps PQsendQuery().
template<typename T>
using pg_send_query_t = decltype(std::declval<T&>().send_query(
    std::declval<const std::string&>()));

// pg_get_result_t
//   detector: get_result() method.
// wraps PQgetResult().
template<typename T>
using pg_get_result_t =
    decltype(std::declval<T&>().get_result());

// pg_is_busy_t
//   detector: is_busy() const method.
// wraps PQisBusy().
template<typename T>
using pg_is_busy_t =
    decltype(std::declval<const T&>().is_busy());

// pg_consume_input_t
//   detector: consume_input() method.
// wraps PQconsumeInput().
template<typename T>
using pg_consume_input_t =
    decltype(std::declval<T&>().consume_input());

// -------------------------------------------------------------------------
// B.  pipeline mode
// -------------------------------------------------------------------------

// pg_enter_pipeline_t
//   detector: enter_pipeline() method.
// wraps PQenterPipelineMode().
template<typename T>
using pg_enter_pipeline_t =
    decltype(std::declval<T&>().enter_pipeline());

// pg_exit_pipeline_t
//   detector: exit_pipeline() method.
// wraps PQexitPipelineMode().
template<typename T>
using pg_exit_pipeline_t =
    decltype(std::declval<T&>().exit_pipeline());

// pg_pipeline_sync_t
//   detector: pipeline_sync() method.
// wraps PQpipelineSync().
template<typename T>
using pg_pipeline_sync_t =
    decltype(std::declval<T&>().pipeline_sync());

// -------------------------------------------------------------------------
// C.  COPY protocol
// -------------------------------------------------------------------------

// pg_copy_in_start_t
//   detector: copy_in_start(const std::string&) method.
template<typename T>
using pg_copy_in_start_t =
    decltype(std::declval<T&>().copy_in_start(
        std::declval<const std::string&>()));

// pg_copy_data_t
//   detector: copy_data(const char*, std::size_t) method.
// wraps PQputCopyData().
template<typename T>
using pg_copy_data_t = decltype(std::declval<T&>().copy_data(
    std::declval<const char*>(),
    std::declval<std::size_t>()));

// pg_copy_end_t
//   detector: copy_end() method.
// wraps PQputCopyEnd().
template<typename T>
using pg_copy_end_t =
    decltype(std::declval<T&>().copy_end());

// -------------------------------------------------------------------------
// D.  LISTEN / NOTIFY
// -------------------------------------------------------------------------

// pg_listen_t
//   detector: listen(const std::string&) method.
// executes LISTEN <channel>.
template<typename T>
using pg_listen_t = decltype(std::declval<T&>().listen(
    std::declval<const std::string&>()));

// pg_notify_t
//   detector: notify(const std::string&, const std::string&) method.
// executes pg_notify().
template<typename T>
using pg_notify_t = decltype(std::declval<T&>().notify(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// pg_get_notification_t
//   detector: get_notification() method.
// wraps PQnotifies().
template<typename T>
using pg_get_notification_t =
    decltype(std::declval<T&>().get_notification());

// -------------------------------------------------------------------------
// E.  parameterized execution
// -------------------------------------------------------------------------

// pg_exec_params_t
//   detector: exec_params(const std::string&,
// const std::vector<std::string>&) method.
// wraps PQexecParams().
template<typename T>
using pg_exec_params_t = decltype(std::declval<T&>().exec_params(
    std::declval<const std::string&>(),
    std::declval<const std::vector<std::string>&>()));

// -------------------------------------------------------------------------
// F.  connection diagnostics
// -------------------------------------------------------------------------

// pg_get_backend_pid_t
//   detector: get_backend_pid() const method.
// wraps PQbackendPID().
template<typename T>
using pg_get_backend_pid_t =
    decltype(std::declval<const T&>().get_backend_pid());

// pg_transaction_status_t
//   detector: get_transaction_status() const method.
// wraps PQtransactionStatus().
template<typename T>
using pg_transaction_status_t =
    decltype(std::declval<const T&>().get_transaction_status());

// pg_parameter_status_t
//   detector: get_parameter_status(const std::string&) const method.
// wraps PQparameterStatus().
template<typename T>
using pg_parameter_status_t =
    decltype(std::declval<const T&>().get_parameter_status(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// G.  result field OID introspection
// -------------------------------------------------------------------------

// pg_field_type_oid_t
//   detector: field_type_oid(int) const method.
// wraps PQftype().
template<typename T>
using pg_field_type_oid_t =
    decltype(std::declval<const T&>().field_type_oid(
        std::declval<int>()));

// -------------------------------------------------------------------------
// H.  schema introspection
// -------------------------------------------------------------------------

// pg_has_table_t
//   detector: has_table(const std::string&) const method.
template<typename T>
using pg_has_table_t =
    decltype(std::declval<const T&>().has_table(
        std::declval<const std::string&>()));

// pg_list_tables_t
//   detector: list_tables() const method.
template<typename T>
using pg_list_tables_t =
    decltype(std::declval<const T&>().list_tables());

// -------------------------------------------------------------------------
// I.  escape
// -------------------------------------------------------------------------

// pg_escape_literal_t
//   detector: escape_literal(const std::string&) const method.
// wraps PQescapeLiteral().
template<typename T>
using pg_escape_literal_t =
    decltype(std::declval<const T&>().escape_literal(
        std::declval<const std::string&>()));

// pg_escape_identifier_t
//   detector: escape_identifier(const std::string&) const method.
// wraps PQescapeIdentifier().
template<typename T>
using pg_escape_identifier_t =
    decltype(std::declval<const T&>().escape_identifier(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// J.  large objects
// -------------------------------------------------------------------------

// pg_lo_import_t
//   detector: lo_import(const std::string&) method.
template<typename T>
using pg_lo_import_t = decltype(std::declval<T&>().lo_import(
    std::declval<const std::string&>()));

// pg_lo_export_t
//   detector: lo_export(unsigned int, const std::string&) method.
template<typename T>
using pg_lo_export_t = decltype(std::declval<T&>().lo_export(
    std::declval<unsigned int>(),
    std::declval<const std::string&>()));


// =============================================================================
// IX.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_pg_async
//   trait: checks if type T supports asynchronous query dispatch
// (send_query + get_result + is_busy + consume_input).
template<typename T>
struct has_pg_async : djinterp::conjunction<
    is_detected<pg_send_query_t, clean_t<T>>,
    is_detected<pg_get_result_t, clean_t<T>>,
    is_detected<pg_is_busy_t, clean_t<T>>,
    is_detected<pg_consume_input_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_async_v = has_pg_async<clean_t<T>>::value;
#endif

// has_pg_pipeline
//   trait: checks if type T supports pipeline mode
// (enter_pipeline + exit_pipeline + pipeline_sync).
template<typename T>
struct has_pg_pipeline : djinterp::conjunction<
    is_detected<pg_enter_pipeline_t, clean_t<T>>,
    is_detected<pg_exit_pipeline_t, clean_t<T>>,
    is_detected<pg_pipeline_sync_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_pipeline_v = has_pg_pipeline<clean_t<T>>::value;
#endif

// has_pg_copy
//   trait: checks if type T supports the COPY protocol
// (copy_in_start + copy_data + copy_end).
template<typename T>
struct has_pg_copy : djinterp::conjunction<
    is_detected<pg_copy_in_start_t, clean_t<T>>,
    is_detected<pg_copy_data_t, clean_t<T>>,
    is_detected<pg_copy_end_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_copy_v = has_pg_copy<clean_t<T>>::value;
#endif

// has_pg_listen_notify
//   trait: checks if type T supports LISTEN/NOTIFY
// (listen + notify + get_notification).
template<typename T>
struct has_pg_listen_notify : djinterp::conjunction<
    is_detected<pg_listen_t, clean_t<T>>,
    is_detected<pg_notify_t, clean_t<T>>,
    is_detected<pg_get_notification_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_listen_notify_v =
        has_pg_listen_notify<clean_t<T>>::value;
#endif

// has_pg_diagnostics
//   trait: checks if type T supports connection diagnostics
// (get_backend_pid + get_transaction_status + get_parameter_status).
template<typename T>
struct has_pg_diagnostics : djinterp::conjunction<
    is_detected<pg_get_backend_pid_t, clean_t<T>>,
    is_detected<pg_transaction_status_t, clean_t<T>>,
    is_detected<pg_parameter_status_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_diagnostics_v =
        has_pg_diagnostics<clean_t<T>>::value;
#endif

// has_pg_escape
//   trait: checks if type T supports PostgreSQL escaping
// (escape_literal + escape_identifier).
template<typename T>
struct has_pg_escape : djinterp::conjunction<
    is_detected<pg_escape_literal_t, clean_t<T>>,
    is_detected<pg_escape_identifier_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_escape_v = has_pg_escape<clean_t<T>>::value;
#endif

// has_pg_large_objects
//   trait: checks if type T supports large object API
// (lo_import + lo_export).
template<typename T>
struct has_pg_large_objects : djinterp::conjunction<
    is_detected<pg_lo_import_t, clean_t<T>>,
    is_detected<pg_lo_export_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_large_objects_v =
        has_pg_large_objects<clean_t<T>>::value;
#endif

// has_pg_schema_query
//   trait: checks if type T supports schema introspection
// (has_table + list_tables).
template<typename T>
struct has_pg_schema_query : djinterp::conjunction<
    is_detected<pg_has_table_t, clean_t<T>>,
    is_detected<pg_list_tables_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_schema_query_v =
        has_pg_schema_query<clean_t<T>>::value;
#endif

// is_pg_connection
//   trait: compound trait verifying type T implements a PostgreSQL
// connection interface (connection + async + diagnostics + escape +
// schema queries).
template<typename T>
struct is_pg_connection : djinterp::conjunction<
    is_connection<clean_t<T>>,
    has_pg_async<clean_t<T>>,
    has_pg_diagnostics<clean_t<T>>,
    has_pg_escape<clean_t<T>>,
    has_pg_schema_query<clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool is_pg_connection_v = is_pg_connection<clean_t<T>>::value;
#endif


// =============================================================================
// X.   TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// pg_can_send_query
//   tagless trait: true if T has send_query().
template<typename T,
         typename = void>
constexpr bool pg_can_send_query = false;

template<typename T>
constexpr bool pg_can_send_query<T,
    std::void_t<pg_send_query_t<T>>> = true;

// pg_can_enter_pipeline
//   tagless trait: true if T has enter_pipeline().
template<typename T,
         typename = void>
constexpr bool pg_can_enter_pipeline = false;

template<typename T>
constexpr bool pg_can_enter_pipeline<T,
    std::void_t<pg_enter_pipeline_t<T>>> = true;

// pg_can_copy_data
//   tagless trait: true if T has copy_data().
template<typename T,
         typename = void>
constexpr bool pg_can_copy_data = false;

template<typename T>
constexpr bool pg_can_copy_data<T,
    std::void_t<pg_copy_data_t<T>>> = true;

// pg_can_listen
//   tagless trait: true if T has listen().
template<typename T,
         typename = void>
constexpr bool pg_can_listen = false;

template<typename T>
constexpr bool pg_can_listen<T,
    std::void_t<pg_listen_t<T>>> = true;

// pg_can_notify
//   tagless trait: true if T has notify().
template<typename T,
         typename = void>
constexpr bool pg_can_notify = false;

template<typename T>
constexpr bool pg_can_notify<T,
    std::void_t<pg_notify_t<T>>> = true;

// pg_can_exec_params
//   tagless trait: true if T has exec_params().
template<typename T,
         typename = void>
constexpr bool pg_can_exec_params = false;

template<typename T>
constexpr bool pg_can_exec_params<T,
    std::void_t<pg_exec_params_t<T>>> = true;

// pg_can_escape_literal
//   tagless trait: true if T has escape_literal().
template<typename T,
         typename = void>
constexpr bool pg_can_escape_literal = false;

template<typename T>
constexpr bool pg_can_escape_literal<T,
    std::void_t<pg_escape_literal_t<T>>> = true;

// pg_can_query_schema
//   tagless trait: true if T has has_table().
template<typename T,
         typename = void>
constexpr bool pg_can_query_schema = false;

template<typename T>
constexpr bool pg_can_query_schema<T,
    std::void_t<pg_has_table_t<T>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// pg_does_async
//   tagless trait: true if T supports async query dispatch.
template<typename T,
         typename = void>
constexpr bool pg_does_async = false;

template<typename T>
constexpr bool pg_does_async<T, std::void_t<
    pg_send_query_t<T>,
    pg_get_result_t<T>,
    pg_is_busy_t<T>,
    pg_consume_input_t<T>>> = true;

// pg_does_pipeline
//   tagless trait: true if T supports pipeline mode.
template<typename T,
         typename = void>
constexpr bool pg_does_pipeline = false;

template<typename T>
constexpr bool pg_does_pipeline<T, std::void_t<
    pg_enter_pipeline_t<T>,
    pg_exit_pipeline_t<T>,
    pg_pipeline_sync_t<T>>> = true;

// pg_does_copy
//   tagless trait: true if T supports the COPY protocol.
template<typename T,
         typename = void>
constexpr bool pg_does_copy = false;

template<typename T>
constexpr bool pg_does_copy<T, std::void_t<
    pg_copy_in_start_t<T>,
    pg_copy_data_t<T>,
    pg_copy_end_t<T>>> = true;

// pg_does_listen_notify
//   tagless trait: true if T supports LISTEN/NOTIFY.
template<typename T,
         typename = void>
constexpr bool pg_does_listen_notify = false;

template<typename T>
constexpr bool pg_does_listen_notify<T, std::void_t<
    pg_listen_t<T>,
    pg_notify_t<T>,
    pg_get_notification_t<T>>> = true;

// pg_is_full_connection
//   tagless trait: true if T satisfies the complete PostgreSQL
// connection interface.
template<typename T>
constexpr bool pg_is_full_connection =
    ( is_connectable<clean_t<T>>         &&
      pg_does_async<clean_t<T>>          &&
      pg_can_escape_literal<clean_t<T>>  &&
      pg_can_query_schema<clean_t<T>> );


// =============================================================================
// XI.  SFINAE HELPERS
// =============================================================================

// enable_if_pg_connection
//   type: SFINAE helper for PostgreSQL connection constraints.
template<typename T>
using enable_if_pg_connection =
    typename std::enable_if<is_pg_connection<clean_t<T>>::value>::type;

// enable_if_has_pg_async
//   type: SFINAE helper for PostgreSQL async constraints.
template<typename T>
using enable_if_has_pg_async =
    typename std::enable_if<has_pg_async<clean_t<T>>::value>::type;

// enable_if_has_pg_pipeline
//   type: SFINAE helper for PostgreSQL pipeline constraints.
template<typename T>
using enable_if_has_pg_pipeline =
    typename std::enable_if<has_pg_pipeline<clean_t<T>>::value>::type;

// enable_if_has_pg_copy
//   type: SFINAE helper for PostgreSQL COPY constraints.
template<typename T>
using enable_if_has_pg_copy =
    typename std::enable_if<has_pg_copy<clean_t<T>>::value>::type;

// enable_if_has_pg_listen_notify
//   type: SFINAE helper for PostgreSQL LISTEN/NOTIFY constraints.
template<typename T>
using enable_if_has_pg_listen_notify =
    typename std::enable_if<has_pg_listen_notify<clean_t<T>>::value>::type;


// ===========================================================================
// XII. C++20 CONCEPTS
// ===========================================================================
//   The PostgreSQL classification concepts, folded in from the former
// postgres_concepts.hpp.  Each forwards to a trait / tagless capability declared
// above.  Gated on concept support so the traits remain usable at the C++17
// baseline (matching functor.hpp / monoid.hpp).

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =============================================================================
// A.   Core PostgreSQL Connection Concepts
// =============================================================================

// postgres_connection
//   concept: constrains types implementing the PostgreSQL connection
// interface.
template<typename Type>
concept postgres_connection =
    is_pg_connection<clean_t<Type>>::value;

// Pg_connection
//   concept: alias for postgres_connection using the shorter pg prefix.
template<typename Type>
concept Pg_connection =
    postgres_connection<Type>;

// non_postgres_connection
//   concept: constrains types that do not implement the PostgreSQL
// connection interface.
template<typename Type>
concept non_postgres_connection =
    !postgres_connection<Type>;

// postgres_async_connection
//   concept: constrains PostgreSQL connections supporting asynchronous
// query dispatch.
template<typename Type>
concept postgres_async_connection =
    has_pg_async<clean_t<Type>>::value;

// postgres_pipeline_connection
//   concept: constrains PostgreSQL connections supporting pipeline mode.
template<typename Type>
concept postgres_pipeline_connection =
    has_pg_pipeline<clean_t<Type>>::value;

// postgres_copy_connection
//   concept: constrains PostgreSQL connections supporting the COPY
// protocol.
template<typename Type>
concept postgres_copy_connection =
    has_pg_copy<clean_t<Type>>::value;

// postgres_listen_notify_connection
//   concept: constrains PostgreSQL connections supporting LISTEN /
// NOTIFY.
template<typename Type>
concept postgres_listen_notify_connection =
    has_pg_listen_notify<clean_t<Type>>::value;

// postgres_diagnostics_connection
//   concept: constrains PostgreSQL connections supporting backend
// diagnostics and parameter status queries.
template<typename Type>
concept postgres_diagnostics_connection =
    has_pg_diagnostics<clean_t<Type>>::value;

// postgres_escape_connection
//   concept: constrains PostgreSQL connections supporting literal and
// identifier escaping.
template<typename Type>
concept postgres_escape_connection =
    has_pg_escape<clean_t<Type>>::value;

// postgres_large_object_connection
//   concept: constrains PostgreSQL connections supporting the large
// object API.
template<typename Type>
concept postgres_large_object_connection =
    has_pg_large_objects<clean_t<Type>>::value;

// postgres_schema_query_connection
//   concept: constrains PostgreSQL connections supporting schema
// introspection.
template<typename Type>
concept postgres_schema_query_connection =
    has_pg_schema_query<clean_t<Type>>::value;


// =============================================================================
// B.  PostgreSQL Capability Concepts
// =============================================================================

// postgres_send_query_connection
//   concept: constrains types exposing send_query(const string&).
template<typename Type>
concept postgres_send_query_connection =
    pg_can_send_query<clean_t<Type>>;

// postgres_result_polling_connection
//   concept: constrains types exposing get_result().
template<typename Type>
concept postgres_result_polling_connection =
    is_detected<pg_get_result_t, clean_t<Type>>::value;

// postgres_busy_state_query
//   concept: constrains types exposing is_busy() const.
template<typename Type>
concept postgres_busy_state_query =
    is_detected<pg_is_busy_t, clean_t<Type>>::value;

// postgres_input_consuming_connection
//   concept: constrains types exposing consume_input().
template<typename Type>
concept postgres_input_consuming_connection =
    is_detected<pg_consume_input_t, clean_t<Type>>::value;

// postgres_pipeline_enterable_connection
//   concept: constrains types exposing enter_pipeline().
template<typename Type>
concept postgres_pipeline_enterable_connection =
    pg_can_enter_pipeline<clean_t<Type>>;

// postgres_pipeline_exitable_connection
//   concept: constrains types exposing exit_pipeline().
template<typename Type>
concept postgres_pipeline_exitable_connection =
    is_detected<pg_exit_pipeline_t, clean_t<Type>>::value;

// postgres_pipeline_sync_connection
//   concept: constrains types exposing pipeline_sync().
template<typename Type>
concept postgres_pipeline_sync_connection =
    is_detected<pg_pipeline_sync_t, clean_t<Type>>::value;

// postgres_copy_in_connection
//   concept: constrains types exposing copy_in_start(const string&).
template<typename Type>
concept postgres_copy_in_connection =
    is_detected<pg_copy_in_start_t, clean_t<Type>>::value;

// postgres_copy_data_connection
//   concept: constrains types exposing copy_data(const char*, size_t).
template<typename Type>
concept postgres_copy_data_connection =
    pg_can_copy_data<clean_t<Type>>;

// postgres_copy_end_connection
//   concept: constrains types exposing copy_end().
template<typename Type>
concept postgres_copy_end_connection =
    is_detected<pg_copy_end_t, clean_t<Type>>::value;

// postgres_listenable_connection
//   concept: constrains types exposing listen(const string&).
template<typename Type>
concept postgres_listenable_connection =
    pg_can_listen<clean_t<Type>>;

// postgres_notifiable_connection
//   concept: constrains types exposing notify(channel, payload).
template<typename Type>
concept postgres_notifiable_connection =
    pg_can_notify<clean_t<Type>>;

// postgres_notification_query_connection
//   concept: constrains types exposing get_notification().
template<typename Type>
concept postgres_notification_query_connection =
    is_detected<pg_get_notification_t, clean_t<Type>>::value;

// postgres_parameterized_execution_connection
//   concept: constrains types exposing exec_params(query, params).
template<typename Type>
concept postgres_parameterized_execution_connection =
    pg_can_exec_params<clean_t<Type>>;

// postgres_backend_pid_connection
//   concept: constrains types exposing get_backend_pid() const.
template<typename Type>
concept postgres_backend_pid_connection =
    is_detected<pg_get_backend_pid_t, clean_t<Type>>::value;

// postgres_transaction_status_connection
//   concept: constrains types exposing get_transaction_status() const.
template<typename Type>
concept postgres_transaction_status_connection =
    is_detected<pg_transaction_status_t, clean_t<Type>>::value;

// postgres_parameter_status_connection
//   concept: constrains types exposing get_parameter_status(name) const.
template<typename Type>
concept postgres_parameter_status_connection =
    is_detected<pg_parameter_status_t, clean_t<Type>>::value;

// postgres_field_oid_introspection_connection
//   concept: constrains types exposing field_type_oid(int) const.
template<typename Type>
concept postgres_field_oid_introspection_connection =
    is_detected<pg_field_type_oid_t, clean_t<Type>>::value;

// postgres_has_table_query
//   concept: constrains types exposing has_table(name) const.
template<typename Type>
concept postgres_has_table_query =
    pg_can_query_schema<clean_t<Type>>;

// postgres_table_names_query
//   concept: constrains types exposing list_tables() const.
template<typename Type>
concept postgres_table_names_query =
    is_detected<pg_list_tables_t, clean_t<Type>>::value;

// postgres_escape_literal_connection
//   concept: constrains types exposing escape_literal(text) const.
template<typename Type>
concept postgres_escape_literal_connection =
    pg_can_escape_literal<clean_t<Type>>;

// postgres_escape_identifier_connection
//   concept: constrains types exposing escape_identifier(text) const.
template<typename Type>
concept postgres_escape_identifier_connection =
    is_detected<pg_escape_identifier_t, clean_t<Type>>::value;

// postgres_lo_import_connection
//   concept: constrains types exposing lo_import(path).
template<typename Type>
concept postgres_lo_import_connection =
    is_detected<pg_lo_import_t, clean_t<Type>>::value;

// postgres_lo_export_connection
//   concept: constrains types exposing lo_export(oid, path).
template<typename Type>
concept postgres_lo_export_connection =
    is_detected<pg_lo_export_t, clean_t<Type>>::value;


// =============================================================================
// C. Tagless PostgreSQL Capability Concepts
// =============================================================================

// postgres_async_dispatchable
//   concept: constrains types satisfying the full tagless async query
// dispatch capability set.
template<typename Type>
concept postgres_async_dispatchable =
    pg_does_async<clean_t<Type>>;

// postgres_pipelined_connection
//   concept: constrains types satisfying the full tagless pipeline
// capability set.
template<typename Type>
concept postgres_pipelined_connection =
    pg_does_pipeline<clean_t<Type>>;

// postgres_copy_protocol_connection
//   concept: constrains types satisfying the full tagless COPY protocol
// capability set.
template<typename Type>
concept postgres_copy_protocol_connection =
    pg_does_copy<clean_t<Type>>;

// postgres_listen_notifiable_connection
//   concept: constrains types satisfying the full tagless LISTEN /
// NOTIFY capability set.
template<typename Type>
concept postgres_listen_notifiable_connection =
    pg_does_listen_notify<clean_t<Type>>;

// postgres_full_connection
//   concept: constrains types satisfying the complete tagless
// PostgreSQL connection capability set.
template<typename Type>
concept postgres_full_connection =
    pg_is_full_connection<clean_t<Type>>;

// pg_full_connection
//   concept: alias for postgres_full_connection using the shorter pg
// prefix.
template<typename Type>
concept pg_full_connection =
    postgres_full_connection<Type>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_POSTGRES_POSTGRES_HPP
