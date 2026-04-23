/******************************************************************************
* djinterp [database]                                             postgres.hpp
* 
* djinterp PostgreSQL connection module:
*   This header provides the PostgreSQL-specific connection implementation
* and associated data type infrastructure for the djinterp database
* module, including:
*   - PostgreSQL OID-based type system (well-known OID constants for all
*     built-in types: integer family, text, bytea, boolean, date/time,
*     numeric, JSON/JSONB, UUID, arrays, range, composite, geometry)
*   - OID-to-field_type mapping with version-gated awareness for types
*     introduced in later releases (JSONB 9.4, multirange 14, etc.)
*   - compile-time type and feature availability via D_ENV_PG_* macros
*     covering data types, SQL features, indexes, partitioning,
*     replication, parallel query, extensions, and security
*   - PostgreSQL-specific connection configuration (SSL mode, search_path,
*     application_name, connection string / keyword-value API)
*   - the concrete pg_connection CRTP leaf class with asynchronous query
*     dispatch, pipeline mode, COPY protocol, LISTEN/NOTIFY, parameterized
*     queries, large objects, escaping, and schema introspection
*   - version-gated method declarations for pipeline (14+), chunked
*     results (17+), and close-prepared (17+)
*
*   PostgreSQL's type system is fundamentally different from MySQL/SQLite:
*   - every type has a unique OID (Object Identifier)
*   - PQftype() returns the OID for each result column
*   - OIDs are stable across versions for built-in types
*   - user-defined types (enums, composites, domains) get dynamically
*     assigned OIDs that must be queried from pg_type at runtime
*   - arrays have their own OIDs (e.g. int4[] = OID 1007)
*
*   LAYER DIAGRAM:
*     pg_connection (this file)
*       -> database_connection<pg_connection, database_type::postgresql>
*         -> connection_template<pg_connection, database_type::postgresql>
*           -> connection<pg_connection>
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include <libpq-fe.h>;
* the concrete _impl method definitions in postgres.cpp include it.
*
* 
* path:      /inc/djinterp/core/db/postgresql/postgres.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.26
******************************************************************************/

#ifndef DJINTERP_DATABASE_POSTGRES_
#define DJINTERP_DATABASE_POSTGRES_
#include "../djinterp.hpp"
#include "../env/db/env_postgresql.h"
#include "database_connection.hpp"
#include "./postgres_traits.hpp"


NS_DJINTERP
NS_DATABASE
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
        true;  #else  false;  #endif
    static constexpr bool has_cte_materialized =
    #if D_ENV_PG_HAS_CTE_MATERIALIZED
        true;  #else  false;  #endif
    static constexpr bool has_window_functions =
    #if D_ENV_PG_HAS_WINDOW_FUNCTIONS
        true;  #else  false;  #endif
    static constexpr bool has_lateral =
    #if D_ENV_PG_HAS_LATERAL
        true;  #else  false;  #endif
    static constexpr bool has_upsert =
    #if D_ENV_PG_HAS_UPSERT
        true;  #else  false;  #endif
    static constexpr bool has_merge =
    #if D_ENV_PG_HAS_MERGE
        true;  #else  false;  #endif
    static constexpr bool has_grouping_sets =
    #if D_ENV_PG_HAS_GROUPING_SETS
        true;  #else  false;  #endif
    static constexpr bool has_row_level_security =
    #if D_ENV_PG_HAS_ROW_LEVEL_SECURITY
        true;  #else  false;  #endif
    static constexpr bool has_stored_procedures =
    #if D_ENV_PG_HAS_STORED_PROCEDURES
        true;  #else  false;  #endif
    static constexpr bool has_returning =
    #if D_ENV_PG_HAS_RETURNING
        true;  #else  false;  #endif
    static constexpr bool has_materialized_views =
    #if D_ENV_PG_HAS_MATERIALIZED_VIEWS
        true;  #else  false;  #endif
    static constexpr bool has_full_text_search =
    #if D_ENV_PG_HAS_FULL_TEXT_SEARCH
        true;  #else  false;  #endif
    static constexpr bool has_tablesample =
    #if D_ENV_PG_HAS_TABLESAMPLE
        true;  #else  false;  #endif

    // partitioning
    static constexpr bool has_declarative_partitioning =
    #if D_ENV_PG_HAS_DECLARATIVE_PARTITIONING
        true;  #else  false;  #endif
    static constexpr bool has_hash_partitioning =
    #if D_ENV_PG_HAS_HASH_PARTITIONING
        true;  #else  false;  #endif

    // replication
    static constexpr bool has_logical_repl =
    #if D_ENV_PG_HAS_LOGICAL_REPL
        true;  #else  false;  #endif
    static constexpr bool has_streaming_repl =
    #if D_ENV_PG_HAS_STREAMING_REPL
        true;  #else  false;  #endif

    // parallel
    static constexpr bool has_parallel_query =
    #if D_ENV_PG_HAS_PARALLEL_QUERY
        true;  #else  false;  #endif
    static constexpr bool has_jit =
    #if D_ENV_PG_HAS_JIT
        true;  #else  false;  #endif

    // indexes
    static constexpr bool has_covering_index =
    #if D_ENV_PG_HAS_COVERING_INDEX
        true;  #else  false;  #endif
    static constexpr bool has_brin =
    #if D_ENV_PG_HAS_INDEX_BRIN
        true;  #else  false;  #endif

    // libpq
    static constexpr bool has_pipeline =
    #if D_ENV_PG_HAS_LIBPQ_PIPELINE
        true;  #else  false;  #endif
    static constexpr bool has_chunked_result =
    #if D_ENV_PG_HAS_LIBPQ_CHUNKED_RESULT
        true;  #else  false;  #endif
    static constexpr bool has_close_prepared =
    #if D_ENV_PG_HAS_LIBPQ_CLOSE_PREPARED
        true;  #else  false;  #endif

    // auth
    static constexpr bool has_scram =
    #if D_ENV_PG_HAS_SCRAM_SHA_256
        true;  #else  false;  #endif

    // extensions
    static constexpr bool has_fdw =
    #if D_ENV_PG_HAS_FDW
        true;  #else  false;  #endif
    static constexpr bool has_create_extension =
    #if D_ENV_PG_HAS_CREATE_EXTENSION
        true;  #else  false;  #endif
    static constexpr bool has_event_triggers =
    #if D_ENV_PG_HAS_EVENT_TRIGGERS
        true;  #else  false;  #endif

    // composite
    static constexpr bool has_modern_sql =
    #if D_ENV_PG_HAS_MODERN_SQL
        true;  #else  false;  #endif
    static constexpr bool has_modern_json =
    #if D_ENV_PG_HAS_MODERN_JSON
        true;  #else  false;  #endif
    static constexpr bool is_fully_modern =
    #if D_ENV_PG_IS_FULLY_MODERN
        true;  #else  false;  #endif

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
    static constexpr std::uint32_t id       = D_ENV_PG_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_PG_VERSION_MAJOR;
    static constexpr std::uint16_t release  = D_ENV_PG_VERSION_RELEASE;
    static constexpr bool          is_legacy_versioning =
        D_ENV_PG_IS_LEGACY_VERSIONING;
    static constexpr const char*   string   = D_ENV_PG_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t release  = 0;
    static constexpr bool          is_legacy_versioning = false;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected version is at least
    // the specified major version (post-10 convention).
    static constexpr bool at_least(std::uint16_t _major) noexcept
    {
        return id >= (_major * 10000u);
    }

    // at_least_legacy
    //   function: returns true if the detected version is at least
    // the specified pre-10 (major, minor, patch) version.
    static constexpr bool at_least_legacy(
        std::uint16_t _major,
        std::uint16_t _minor,
        std::uint16_t _patch) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


// =============================================================================
// V.   SSL MODE ENUMERATION
// =============================================================================

// pg_ssl_mode
//   enumeration: PostgreSQL SSL connection modes (sslmode parameter).
enum class pg_ssl_mode : std::uint8_t
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
//   struct: PostgreSQL-specific connection configuration extending the
// generic connection_config with libpq connection parameters.
struct pg_connect_config
{
    connection_config       base;
    pg_ssl_mode             ssl_mode;
    std::string             application_name;
    std::string             search_path;
    std::string             client_encoding;
    std::string             options;
    std::string             connection_string;
    int                     connect_timeout_seconds;
    int                     statement_timeout_ms;
    bool                    use_binary_results;

    std::map<std::string, std::string> extra_params;

    pg_connect_config()
        : ssl_mode(pg_ssl_mode::prefer)
        , client_encoding("UTF8")
        , connect_timeout_seconds(10)
        , statement_timeout_ms(0)
        , use_binary_results(false)
    {
        base.host   = "localhost";
        base.port   = 5432;
        base.schema = "public";
    }

    explicit pg_connect_config(const connection_config& _base)
        : base(_base)
        , ssl_mode(pg_ssl_mode::prefer)
        , client_encoding("UTF8")
        , connect_timeout_seconds(10)
        , statement_timeout_ms(0)
        , use_binary_results(false)
    {
    }

    explicit pg_connect_config(const std::string& _conn_string)
        : ssl_mode(pg_ssl_mode::prefer)
        , client_encoding("UTF8")
        , connection_string(_conn_string)
        , connect_timeout_seconds(10)
        , statement_timeout_ms(0)
        , use_binary_results(false)
    {
    }
};


// =============================================================================
// VII. POSTGRESQL CONNECTION
// =============================================================================

// pg_connection
//   class: concrete PostgreSQL connection implementation via libpq.
// This is the CRTP leaf class; _impl methods are defined in
// postgres.cpp which includes <libpq-fe.h>.
//
// Usage:
//   pg_connection conn;
//   conn.connect(pg_connect_config("host=localhost dbname=mydb"));
//   auto rs = conn.execute_query("SELECT 1");
class pg_connection
    : public database_connection<pg_connection,
                                 database_type::postgresql>
{
public:
    using base_type      = database_connection<
        pg_connection, database_type::postgresql>;
    using type_support   = pg_type_support;
    using feature_support = pg_feature_support;
    using version_info   = pg_version_info;

    pg_connection()
        : base_type()
    {
    }

    explicit pg_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit pg_connection(const pg_connect_config& _config)
        : base_type(_config.base)
        , m_pg_config(_config)
    {
    }

    explicit pg_connection(const std::string& _conn_string)
        : base_type()
        , m_pg_config(_conn_string)
    {
    }

    ~pg_connection() = default;

    // disable copying
    pg_connection(const pg_connection&)            = delete;
    pg_connection& operator=(const pg_connection&) = delete;

    // enable moving
    pg_connection(pg_connection&&) noexcept            = default;
    pg_connection& operator=(pg_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // asynchronous query dispatch
    // -----------------------------------------------------------------

    // send_query
    //   function: dispatches a query asynchronously.
    // wraps PQsendQuery().
    bool send_query(const std::string& _query)
    {
        this->ensure_connected();

        return self().send_query_impl(_query);
    }

    // get_result
    //   function: retrieves the next result from an async query.
    // wraps PQgetResult(). Returns nullptr when no more results.
    auto get_result()
    {
        return self().get_result_impl();
    }

    // is_busy
    //   function: checks if the server is still processing.
    // wraps PQisBusy().
    bool is_busy() const
    {
        return self().is_busy_impl();
    }

    // consume_input
    //   function: reads available data from the server socket.
    // wraps PQconsumeInput().
    bool consume_input()
    {
        return self().consume_input_impl();
    }

    // -----------------------------------------------------------------
    // parameterized queries
    // -----------------------------------------------------------------

    // exec_params
    //   function: executes a parameterized query.
    // wraps PQexecParams(). Parameters use $1, $2, ... syntax.
    auto exec_params(const std::string&              _query,
                     const std::vector<std::string>&  _params)
    {
        this->ensure_connected();

        return self().exec_params_impl(_query, _params);
    }

    // -----------------------------------------------------------------
    // COPY protocol
    // -----------------------------------------------------------------

    // copy_in_start
    //   function: initiates a COPY ... FROM STDIN operation.
    void copy_in_start(const std::string& _copy_sql)
    {
        this->ensure_connected();
        self().copy_in_start_impl(_copy_sql);
    }

    // copy_data
    //   function: sends a chunk of data during COPY IN.
    // wraps PQputCopyData().
    void copy_data(const char*  _data,
                   std::size_t  _length)
    {
        self().copy_data_impl(_data, _length);
    }

    // copy_end
    //   function: completes a COPY IN operation.
    // wraps PQputCopyEnd().
    void copy_end()
    {
        self().copy_end_impl();
    }

    // -----------------------------------------------------------------
    // LISTEN / NOTIFY
    // -----------------------------------------------------------------

    // listen
    //   function: subscribes to a notification channel.
    // executes LISTEN <channel>.
    void listen(const std::string& _channel)
    {
        this->ensure_connected();
        self().listen_impl(_channel);
    }

    // notify
    //   function: sends a notification on a channel with optional
    // payload. wraps pg_notify() or NOTIFY <channel>, '<payload>'.
    void notify(const std::string& _channel,
                const std::string& _payload = "")
    {
        this->ensure_connected();
        self().notify_impl(_channel, _payload);
    }

    // get_notification
    //   function: retrieves a pending notification (non-blocking).
    // wraps PQnotifies(). Call consume_input() first.
    auto get_notification()
    {
        return self().get_notification_impl();
    }

    // -----------------------------------------------------------------
    // escaping
    // -----------------------------------------------------------------

    // escape_literal
    //   function: escapes a string literal for inclusion in SQL.
    // wraps PQescapeLiteral(). Returns the escaped string including
    // surrounding quotes.
    std::string escape_literal(const std::string& _input) const
    {
        return self().escape_literal_impl(_input);
    }

    // escape_identifier
    //   function: escapes an identifier (table/column name).
    // wraps PQescapeIdentifier(). Returns the escaped string
    // including surrounding double-quotes.
    std::string escape_identifier(const std::string& _input) const
    {
        return self().escape_identifier_impl(_input);
    }

    // -----------------------------------------------------------------
    // connection diagnostics
    // -----------------------------------------------------------------

    // get_backend_pid
    //   function: returns the server process ID for this connection.
    // wraps PQbackendPID().
    int get_backend_pid() const
    {
        return self().get_backend_pid_impl();
    }

    // get_transaction_status
    //   function: returns the transaction status as a char
    // ('I' = idle, 'T' = in transaction, 'E' = in failed
    // transaction).
    // wraps PQtransactionStatus().
    char get_transaction_status() const
    {
        return self().get_transaction_status_impl();
    }

    // get_parameter_status
    //   function: returns the value of a server parameter (e.g.
    // "server_version", "server_encoding", "TimeZone").
    // wraps PQparameterStatus().
    std::string get_parameter_status(
        const std::string& _name) const
    {
        return self().get_parameter_status_impl(_name);
    }

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    // table_exists
    //   function: tests whether a table (or view) exists in the
    // current search_path.
    bool table_exists(const std::string& _table_name) const
    {
        return self().table_exists_impl(_table_name);
    }

    // get_table_names
    //   function: returns a vector of all user table names in the
    // current schema.
    std::vector<std::string> get_table_names() const
    {
        return self().get_table_names_impl();
    }

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

    const pg_connect_config& get_pg_config() const noexcept
    {
        return m_pg_config;
    }

    void set_pg_config(const pg_connect_config& _config)
    {
        m_pg_config    = _config;
        this->m_config = _config.base;
    }

    // -----------------------------------------------------------------
    // _impl methods (defined in postgres.cpp)
    // -----------------------------------------------------------------

    void        connect_impl();
    void        disconnect_impl();
    bool        is_connected_impl() const;
    bool        ping_impl() const;

    auto        execute_query_impl(const std::string& _query)
                    -> std::unique_ptr<
                        result_set<struct pg_result_set_impl>>;
    std::int64_t execute_update_impl(const std::string& _query);
    bool        execute_impl(const std::string& _query);

    auto        prepare_impl(const std::string& _query)
                    -> std::unique_ptr<
                        statement<struct pg_statement_impl>>;

    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;
    std::int64_t get_last_insert_id_impl() const;
    std::int64_t get_affected_rows_impl() const;

    // PostgreSQL-specific _impl methods
    bool send_query_impl(const std::string& _query);
    auto get_result_impl()
        -> std::unique_ptr<result_set<struct pg_result_set_impl>>;
    bool is_busy_impl() const;
    bool consume_input_impl();
    auto exec_params_impl(
        const std::string& _query,
        const std::vector<std::string>& _params)
        -> std::unique_ptr<result_set<struct pg_result_set_impl>>;
    void copy_in_start_impl(const std::string& _copy_sql);
    void copy_data_impl(const char* _data, std::size_t _length);
    void copy_end_impl();
    void listen_impl(const std::string& _channel);
    void notify_impl(const std::string& _channel,
                     const std::string& _payload);
    auto get_notification_impl() -> std::optional<std::string>;
    std::string escape_literal_impl(const std::string& _input) const;
    std::string escape_identifier_impl(
        const std::string& _input) const;
    int  get_backend_pid_impl() const;
    char get_transaction_status_impl() const;
    std::string get_parameter_status_impl(
        const std::string& _name) const;
    bool table_exists_impl(const std::string& _name) const;
    std::vector<std::string> get_table_names_impl() const;

    // transaction _impl methods
    void begin_transaction_impl();
    void commit_impl();
    void rollback_impl();

    // version-gated methods

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
    pg_connect_config m_pg_config;

    pg_connection& self()
    {
        return *this;
    }

    const pg_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// pg_result_set_impl
//   struct: forward declaration of the PostgreSQL result set
// implementation.
struct pg_result_set_impl;

// pg_statement_impl
//   struct: forward declaration of the PostgreSQL prepared statement
// implementation.
struct pg_statement_impl;


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_POSTGRES_
