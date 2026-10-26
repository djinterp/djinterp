/*******************************************************************************
* djinterp [core]                                                   postgres.hpp
*
* djinterp PostgreSQL connection module:
*   The PostgreSQL connection, pg_connection, over the PostgreSQL layer's C++
* face (c/db/postgres/postgres_face.hpp), whose libpq connection it holds:
* every operation is the layer's, and core/db adds exceptions, result sets
* that outlive their query, and prepared statements that close with the
* connection. Beside the operations every connection has, it sends SQL
* without waiting -- in a pipeline, and its rows read a few at a time, if
* asked -- runs statements with parameters, copies rows in, listens for
* notifications, closes statements by name, and quotes values and names.
* With it, PostgreSQL's types and how they map to field_type -- every
* built-in type has an OID, stable across versions, arrays their own -- and
* what the server connected and libpq have, asked at run time, beside what
* libpq's header has, at compile time from env_postgres.h: the client's
* capabilities.
*     pg_connection (this file)
*       -> database_connection<pg_connection, database_type::postgresql>
*         -> connection_template<pg_connection, database_type::postgresql>
*           -> connection<pg_connection>
*   The hooks are defined in postgres.cpp. The header needs C++17 and
* includes no client header: env_postgres.h includes libpq-fe.h only where a
* program asks for env's detection (D_CFG_ENV_USING_POSTGRESQL), which the
* client's capabilities read. Its trailing sections carry
* PostgreSQL's capability-detection traits and C++20 concepts.
*
*
* path:      /inc/djinterp/core/db/postgres/postgres.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.26
*                                                            revised: 2026.10.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OID TYPE CONSTANTS
    ------------------
    1.  The OIDs
         1.  pg_oid
2.  OID MAPPING
    -----------
    1.  To and from field_type
         1.  pg_oid_to_field_type
         2.  field_type_to_pg_sql
         3.  pg_oid_is_array
3.  CLIENT CAPABILITIES
    -------------------
    1.  Compile-time capabilities
         1.  pg_client_capabilities
         2.  pg_type_support
         3.  pg_feature_support
4.  VERSION INFORMATION
    -------------------
    1.  The version
         1.  pg_version_info
5.  SSL MODES
    ---------
    1.  The modes
         1.  pg_ssl_mode
6.  CONNECTION CONFIGURATION
    ------------------------
    1.  The configuration
         1.  pg_connect_config
7.  POSTGRESQL CONNECTION
    ---------------------
    1.  Notifications
         1.  pg_notification
    2.  Pipelines
         1.  pg_pipeline_status
    3.  The layer
         1.  postgres_layer
         2.  prepared_statement<postgres_layer>
         3.  streaming_result_set<postgres_layer>
    4.  The class
         1.  pg_connection
8.  EXPRESSION DETECTORS
    --------------------
    1.  Asynchronous query dispatch
    2.  Pipeline mode
    3.  COPY protocol
    4.  LISTEN and NOTIFY
    5.  Parameterized execution
    6.  Connection diagnostics
    7.  Schema introspection
    8.  Escaping
    9.  Large objects
    10. Large objects
9.  TAGGED CAPABILITY TRAITS
    ------------------------
    1.  Traits
         1.  has_pg_async
         2.  has_pg_pipeline
         3.  has_pg_copy
         4.  has_pg_listen_notify
         5.  has_pg_diagnostics
         6.  has_pg_escape
         7.  has_pg_large_objects
         8.  has_pg_schema_query
         9.  is_pg_connection
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

#ifndef DJINTERP_DB_POSTGRES_POSTGRES_HPP
#define DJINTERP_DB_POSTGRES_POSTGRES_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor
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


//==============================================================================
// 1.  OID TYPE CONSTANTS
//==============================================================================
// PostgreSQL identifies every data type by a unique OID. Built-in type
// OIDs are stable across all versions. These constants mirror the values
// from the system catalog pg_type and from src/include/catalog/pg_type_d.h.
// They are declared here so that <libpq-fe.h> does not need to be
// included.


// 1.1    The OIDs
//------------------------------------------------------------------------------
// 1.1.1
// pg_oid
//   enumeration: well-known PostgreSQL type OIDs for built-in types.
enum class pg_oid : unsigned int
{
    // boolean
    oid_bool           = 16,

    // binary
    oid_bytea          = 17,

    // integer family
    oid_int8           = 20,        // BIGINT
    oid_int2           = 21,        // SMALLINT
    oid_int4           = 23,        // INTEGER
    oid_oid            = 26,        // OID type itself

    // floating point and numeric
    oid_float4         = 700,       // REAL
    oid_float8         = 701,       // DOUBLE PRECISION
    oid_numeric        = 1700,      // NUMERIC / DECIMAL
    oid_money          = 790,       // MONEY

    // character types
    oid_char           = 18,        // "char" (single byte)
    oid_name           = 19,        // name (63-byte identifier)
    oid_text           = 25,        // TEXT
    oid_varchar        = 1043,      // CHARACTER VARYING
    oid_bpchar         = 1042,      // CHARACTER (blank-padded)

    // date and time
    oid_date           = 1082,      // DATE
    oid_time           = 1083,      // TIME WITHOUT TIME ZONE
    oid_timetz         = 1266,      // TIME WITH TIME ZONE
    oid_timestamp      = 1114,      // TIMESTAMP WITHOUT TIME ZONE
    oid_timestamptz    = 1184,      // TIMESTAMP WITH TIME ZONE
    oid_interval       = 1186,      // INTERVAL

    // network address types
    oid_inet           = 869,       // INET (IPv4/IPv6 host address)
    oid_cidr           = 650,       // CIDR (IPv4/IPv6 network)
    oid_macaddr        = 829,       // MAC address (6 bytes)
    oid_macaddr8       = 774,       // MAC address (8 bytes, PG 10+)

    // geometric types
    oid_point          = 600,
    oid_line           = 628,
    oid_lseg           = 601,
    oid_box            = 603,
    oid_path           = 602,
    oid_polygon        = 604,
    oid_circle         = 718,

    // JSON types
    oid_json           = 114,       // JSON (text-based, PG 9.2+)
    oid_jsonb          = 3802,      // JSONB (binary, PG 9.4+)
    oid_jsonpath       = 4072,      // JSONPATH (PG 12+)

    // UUID
    oid_uuid           = 2950,

    // XML
    oid_xml            = 142,

    // bit string
    oid_bit            = 1560,      // BIT
    oid_varbit         = 1562,      // BIT VARYING

    // range types (PG 9.2+)
    oid_int4range      = 3904,
    oid_int8range      = 3926,
    oid_numrange       = 3906,
    oid_tsrange        = 3908,
    oid_tstzrange      = 3910,
    oid_daterange      = 3912,

    // multirange types (PG 14+)
    oid_int4multirange = 4451,
    oid_int8multirange = 4536,
    oid_nummultirange  = 4532,
    oid_tsmultirange   = 4533,
    oid_tstzmultirange = 4534,
    oid_datemultirange = 4535,

    // array types (OID of the array version of common types)
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

    // pseudo-types and internal
    oid_void           = 2278,
    oid_record         = 2249,      // anonymous composite
    oid_unknown        = 705,

    // tsvector / tsquery (full-text search)
    oid_tsvector       = 3614,
    oid_tsquery        = 3615,

    // large object
    oid_pg_lsn         = 3220       // pg_lsn (WAL position)
};


//==============================================================================
// 2.  OID MAPPING
//==============================================================================


// 2.1    To and from field_type
//------------------------------------------------------------------------------
// 2.1.1
// pg_oid_to_field_type
//   function: maps a PostgreSQL OID to the generic djinterp field_type.
inline field_type
pg_oid_to_field_type(
    unsigned int _oid
) noexcept
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

// 2.1.2
// field_type_to_pg_sql
//   function: returns the SQL type name string for a given field_type
// as it would be used in PostgreSQL DDL.
inline const char*
field_type_to_pg_sql(
    field_type _type
) noexcept
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

// 2.1.3
// pg_oid_is_array
//   function: returns true if the given OID represents an array type.
// Built-in array OIDs are in the 1000-range; this checks the common
// ones. For user-defined array types, query pg_type.typarray at
// runtime.
inline bool
pg_oid_is_array(
    unsigned int _oid
) noexcept
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


//==============================================================================
// 3.  CLIENT CAPABILITIES
//==============================================================================
// What env_postgres.h found libpq's header to have, by version, at compile
// time -- the client's capabilities: the data types and the SQL features a
// server of the header's release has, which the server connected may lack,
// or exceed, and what libpq itself has (pipelines, chunked rows, closing a
// statement). Every flag is false unless a program asks for env's detection
// (D_CFG_ENV_USING_POSTGRESQL) or supplies its results. libpq's header alone
// gives a lower bound (D_ENV_PG_VERSION_IS_LOWER_BOUND). What the server
// connected and the libpq loaded have, pg_connection's supports_* members
// ask at run time.


// 3.1    Compile-time capabilities
//------------------------------------------------------------------------------
// 3.1.1
// pg_client_capabilities
//   struct: compile-time data type, SQL feature and libpq availability flags,
// as libpq's header gives them.
struct pg_client_capabilities
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
    static constexpr bool has_json                     = false;
    static constexpr bool has_jsonb                    = false;
    static constexpr bool has_jsonpath                 = false;
    static constexpr bool has_json_table               = false;
    static constexpr bool has_sqljson_constructors     = false;
    static constexpr bool has_jsonb_subscript          = false;
    static constexpr bool has_range_types              = false;
    static constexpr bool has_multirange               = false;
    static constexpr bool has_array_types              = false;
    static constexpr bool has_uuid_type                = false;
    static constexpr bool has_enum_type                = false;
    static constexpr bool has_domain_types             = false;
    static constexpr bool has_composite_types          = false;
    static constexpr bool has_identity_columns         = false;
    static constexpr bool has_generated_columns        = false;
    static constexpr bool has_generated_virtual        = false;
    static constexpr bool has_cte                      = false;
    static constexpr bool has_cte_materialized         = false;
    static constexpr bool has_window_functions         = false;
    static constexpr bool has_lateral                  = false;
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

// 3.1.2
// pg_type_support
//   type: pg_client_capabilities, by its former name (2026.10.21), for one
// release; it held the data types.
using pg_type_support [[deprecated("pg_client_capabilities")]] =
    pg_client_capabilities;

// 3.1.3
// pg_feature_support
//   type: pg_client_capabilities, by its former name (2026.10.21), for one
// release; it held the features.
using pg_feature_support [[deprecated("pg_client_capabilities")]] =
    pg_client_capabilities;


//==============================================================================
// 4.  VERSION INFORMATION
//==============================================================================


// 4.1    The version
//------------------------------------------------------------------------------
// 4.1.1
// pg_version_info
//   struct: compile-time version decomposition of libpq's header, which env
// detected; the server's is the connection's to ask (get_server_version).
// NOTE: PostgreSQL changed its versioning scheme at version 10.
// Pre-10 uses three-part (e.g. 9.6.24 = 90624); post-10 uses
// two-part (e.g. 16.2 = 160002). The encoding formula is
// MAJOR*10000 + (MINOR_LEGACY*100 if pre-10) + PATCH.
struct pg_version_info
{
#if D_ENV_PG_DETECTED
    static constexpr bool             detected = true;
    static constexpr re_std::uint32_t id       = D_ENV_PG_VERSION_ID;
    static constexpr re_std::uint16_t major    = D_ENV_PG_VERSION_MAJOR;
    static constexpr re_std::uint16_t release  = D_ENV_PG_VERSION_RELEASE;
    static constexpr bool             is_legacy_versioning =
        D_ENV_PG_IS_LEGACY_VERSIONING;
    static constexpr const char*      string   = D_ENV_PG_VERSION_STRING;
#else
    static constexpr bool             detected = false;
    static constexpr re_std::uint32_t id       = 0;
    static constexpr re_std::uint16_t major    = 0;
    static constexpr re_std::uint16_t release  = 0;
    static constexpr bool             is_legacy_versioning = false;
    static constexpr const char*      string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected version is at least
    // the specified major version (post-10 convention).
    static constexpr bool
    at_least(
        re_std::uint16_t _major
    ) noexcept
    {
        return id >= (_major * 10000u);
    }

    // at_least_legacy
    //   function: returns true if the detected version is at least
    // the specified pre-10 (major, minor, patch) version.
    static constexpr bool
    at_least_legacy(
        re_std::uint16_t _major,
        re_std::uint16_t _minor,
        re_std::uint16_t _patch
    ) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _patch);
    }
};


//==============================================================================
// 5.  SSL MODES
//==============================================================================


// 5.1    The modes
//------------------------------------------------------------------------------
// 5.1.1
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


//==============================================================================
// 6.  CONNECTION CONFIGURATION
//==============================================================================


// 6.1    The configuration
//------------------------------------------------------------------------------
// 6.1.1
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

    // construction -- the defaults (localhost, 5432), a generic
    // configuration, or a connection string alone
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


//==============================================================================
// 7.  POSTGRESQL CONNECTION
//==============================================================================


// 7.1    Notifications
//------------------------------------------------------------------------------
// 7.1.1
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

// 7.2    Pipelines
//------------------------------------------------------------------------------
// 7.2.1
// pg_pipeline_status
//   enum: whether a connection is in pipeline mode, as libpq says (the
// layer's enum d_postgres_pipeline).
enum class pg_pipeline_status : re_std::uint8_t
{
    off     = 0,  // statements run one at a time, each operation waiting for
                  // its result
    on      = 1,  // statements are queued, and their results read in order
    aborted = 2   // a statement failed: the server skips the rest, to the
                  // next synchronization point
};

// 7.3    The layer
//------------------------------------------------------------------------------
// 7.3.1
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

    // holds_connection, busy_code -- a stream's run holds the connection,
    // whose every other call that waits the layer refuses meanwhile with
    // 08000 (postgres_statement.h 3.3)
    static constexpr bool holds_connection = true;
    static constexpr int  busy_code        =
        D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION;

    // connection_lost -- whether a result means the connection was lost, as
    // the description above lists
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

    /**
     * @brief Asks the server to stop what a connection runs, as libpq's
     *        cancel request does (d_postgres_cancel), waiting for it: an
     *        operation's cancel().
     *
     * @param[in] _db  the connection whose statement is to stop.
     * @throws query_exception, with the layer's result and libpq's reason,
     *         when the request cannot be made.
     */
    static void cancel(handle_type _db);
};

// 7.3.2
// prepared_statement<postgres_layer>
//   class: PostgreSQL's prepared statements, instantiated once, in
// postgres.cpp.
extern template class prepared_statement<postgres_layer>;

// 7.3.3
// streaming_result_set<postgres_layer>
//   class: its streams, instantiated with them.
extern template class streaming_result_set<postgres_layer>;

// 7.4    The class
//------------------------------------------------------------------------------
// 7.4.1
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
// in a pipeline or not, parameters, COPY, notifications, statements closed
// by name, escaping, the server's parameters, the schema -- throw as the
// operations do (database_connection.hpp):
// connection_exception when the connection is not open, or a failure lost
// it, and otherwise the exception their contracts name, with the SQLSTATE
// packed, or a negative D_DB_*, as its error_code(). Not copyable; movable.
//
// Usage:
//   pg_connection conn("host=localhost dbname=mydb");
//   conn.connect();
//   auto rs = conn.execute_query("SELECT 1");
class pg_connection : public database_connection<pg_connection,
                                                 database_type::postgresql>
{
public:
    using base_type           = database_connection<pg_connection,
                                                    database_type::postgresql>;
    using layer_type          = postgres_layer;
    using statement_type      = prepared_statement<postgres_layer>;
    using client_capabilities = pg_client_capabilities;
    using version_info        = pg_version_info;

    // type_support, feature_support -- client_capabilities, by their former
    // names (2026.10.21), for one release
    using type_support [[deprecated("client_capabilities")]] =
        pg_client_capabilities;
    using feature_support [[deprecated("client_capabilities")]] =
        pg_client_capabilities;

    // construction -- with pg_connect_config's defaults (localhost, 5432);
    // with a generic configuration; with a PostgreSQL one; or with a
    // connection string, keywords or a URI, alone, libpq's defaults standing
    // in for what it leaves out. Nothing connects before connect()
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

    // is_busy -- whether the next result has yet to arrive, get_result then
    // waiting for it; false when nothing was sent or the connection is not
    // open. In pipeline mode the result of a statement queued after the last
    // synchronization point arrives only once asked for (pipeline_flush);
    // and straight after a statement there fails, is_busy is false even
    // while the next synchronization point's own result is still on its way,
    // which get_result then waits for
    bool is_busy() const noexcept;

    /**
     * @brief Has the rows of the statements still to come arrive a few at a
     *        time, as get_result reads them: each result up to `_chunk_size`
     *        of a statement's rows, and then the statement's end, its columns
     *        and none of its rows.
     *
     * @note Called before the rows it is for are looked at: straight after
     *       send_query, or straight after a statement's last result is read,
     *       for the statements after it. Outside pipeline mode it holds for
     *       the rest of the SQL sent; in pipeline mode, for the next
     *       statement alone. One row a result is libpq's single-row mode,
     *       which every libpq has; more is libpq 17's
     *       (supports_chunked_rows).
     *
     * @param[in] _chunk_size  the most rows a result holds; 1 or more.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for fewer than one
     *         row; with D_DB_UNSUPPORTED for more than one with a libpq
     *         before 17; with 08000 when it comes too late: no statement's
     *         results are still to come, or the next one's have begun.
     */
    void set_chunked_rows_mode(int _chunk_size);

    /**
     * @brief Puts the connection in pipeline mode (libpq 14), where
     *        send_query queues statements, one a call, without waiting for
     *        any of them, and get_result reads their results in order.
     *
     * @note The server runs the statements between two synchronization
     *       points (pipeline_sync) as one transaction, unless one begins its
     *       own; a statement that fails has the server skip the rest, to the
     *       next point. get_result returns each statement's rows, throws
     *       query_exception for one that failed, with its SQLSTATE, and for
     *       each one skipped, with 25P02, and answers nullptr at each
     *       synchronization point, and once nothing is left to read. The
     *       server holds a result until a synchronization point or until
     *       asked for it -- get_result asks before it waits, and
     *       pipeline_flush asks -- so a statement queued after the last
     *       point has its results read all the same, though the transaction
     *       it is part of ends only at the next one. Every other operation
     *       that runs SQL throws query_exception, 08000, until
     *       exit_pipeline.
     *
     * @throws query_exception with D_DB_UNSUPPORTED where libpq is older
     *         than 14 (supports_pipeline); with 08000 while SQL sent before
     *         has results to read, or a COPY is in progress.
     */
    void enter_pipeline();

    // leaving, synchronizing and flushing -- exit_pipeline ends pipeline
    // mode, which needs every result read, up to the last synchronization
    // point; pipeline_sync marks a synchronization point and sends what is
    // queued; pipeline_flush sends what is queued and asks the server for
    // the results it has, without a synchronization point. Each throws
    // query_exception: 08000 while results are still to read, or outside
    // pipeline mode; D_DB_UNSUPPORTED where libpq has no pipelines.
    // get_pipeline_status says whether the connection is in the mode, a
    // failure having aborted it or not; off while the connection is not open
    void               exit_pipeline();
    void               pipeline_sync();
    void               pipeline_flush();
    pg_pipeline_status get_pipeline_status() const noexcept;

    /**
     * @brief Closes a statement the session prepared, by its name -- with
     *        PREPARE, say, or through the face -- whether or not the session
     *        has one of the name.
     *
     * @note libpq from 17 closes it by the protocol's own message, which a
     *       failed transaction takes too; before 17, DEALLOCATE, which a
     *       failed transaction refuses (25P02). The statements prepare()
     *       makes close themselves, and their names, d_stmt_ and hexadecimal
     *       digits, are theirs.
     *
     * @param[in] _stmt_name  the statement's name.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for an empty name or
     *         one holding a NUL; with the server's SQLSTATE when it refuses.
     */
    void close_prepared(const std::string& _stmt_name);

    /**
     * @brief Copies a file of the client's into a new large object
     *        (d_postgres_lo_import), in a transaction of its own where none
     *        is open.
     *
     * @param[in] _path  the file, as the client's C library names it.
     * @return the large object's OID.
     * @throws query_exception with D_DB_NOT_FOUND for a file that is not
     *         there, 58030 for one that cannot be read, D_DB_INVALID_ARGUMENT
     *         for a path holding a NUL, or the server's SQLSTATE;
     *         transaction_exception is not thrown: a failed transaction is
     *         query_exception, 25P02.
     */
    unsigned int lo_import(const std::string& _path);

    /**
     * @brief Copies a large object into a file of the client's, made or
     *        emptied first (d_postgres_lo_export).
     *
     * @param[in] _oid   the large object.
     * @param[in] _path  the file.
     * @throws query_exception with 42704 for an OID no large object has,
     *         58030 for a file that cannot be made or written, otherwise as
     *         lo_import.
     */
    void lo_export(unsigned int       _oid,
                   const std::string& _path);

    /**
     * @brief Reads what the server has sent, without waiting.
     *
     * @return true; a failure throws.
     * @throws as every operation when the connection is not open, or is
     *         lost.
     */
    bool consume_input();

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

    /**
     * @brief Sends bytes of rows, in the COPY's format, in chunks that may
     *        end anywhere, a row's middle included.
     *
     * @note A row the server cannot read is copy_end's failure, which the
     *       server's refusal reaches.
     *
     * @param[in] _data    the bytes.
     * @param[in] _length  how many.
     * @throws query_exception with 08000 when no COPY is in progress; as
     *         every operation when the connection is not open, or is lost.
     */
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

    /**
     * @brief Ends the COPY in progress, keeping none of its rows.
     *
     * @param[in] _reason  the server's message for it.
     * @throws query_exception with 08000 when no COPY is in progress; as
     *         every operation when the connection is not open, or is lost.
     */
    void copy_abort(const std::string& _reason);

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

    /**
     * @brief Quotes text as a string literal, for the connection's encoding,
     *        the quotes included ('it''s').
     *
     * @param[in] _input  the text.
     * @return the literal.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for text that holds
     *         a NUL; as every operation when the connection is not open.
     */
    std::string escape_literal(const std::string& _input) const;

    /**
     * @brief Quotes a name in double quotes, any double quote doubled
     *        ("a""b"); it needs no connection.
     *
     * @param[in] _input  the name.
     * @return the quoted name.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a name that
     *         holds a NUL.
     */
    std::string escape_identifier(const std::string& _input) const;

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

    /**
     * @brief Says whether the current schema -- the first of the search
     *        path that exists, as current_schema() names it -- has a table
     *        of a name.
     *
     * @param[in] _table_name  the name.
     * @return whether the table exists.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a name holding a
     *         NUL, and with the server's SQLSTATE when it cannot tell.
     */
    bool has_table(const std::string& _table_name) const;

    /**
     * @brief Lists the current schema's tables.
     *
     * @return the names, sorted.
     * @throws query_exception with the server's SQLSTATE when it cannot
     *         list them.
     */
    std::vector<std::string> list_tables() const;

    // feature queries -- what the server connected has, by its release, from
    // libpq's copy of its version (d_postgres_supports, no round trip), and
    // false while the connection is not open; and what the libpq loaded can
    // do (d_postgres_client_supports), which needs no connection: pipelines
    // (14), and rows read more than one at a time (17).
    // client_capabilities answers for libpq's header at compile time. Every
    // server the layer accepts, from 10, has JSONB, logical replication,
    // declarative partitioning and parallel query. supports_modern_sql is
    // CTEs, window functions, LATERAL, upsert, MERGE and row-level security
    // together, as from 15; supports_modern_json, JSONB, the JSON path
    // language, subscripts and the SQL/JSON constructors, as from 16
    bool
    supports_jsonb() const noexcept
    {
        return m_server_at_least(9u, 4u);
    }

    bool
    supports_jsonpath() const noexcept
    {
        return m_supports(D_POSTGRES_FEATURE_JSONPATH);
    }

    bool
    supports_multirange() const noexcept
    {
        return m_supports(D_POSTGRES_FEATURE_MULTIRANGES);
    }

    bool
    supports_merge() const noexcept
    {
        return m_supports(D_POSTGRES_FEATURE_MERGE);
    }

    static bool
    supports_pipeline() noexcept
    {
        return face::postgres::client_supports(D_POSTGRES_CLIENT_PIPELINE);
    }

    static bool
    supports_chunked_rows() noexcept
    {
        return face::postgres::client_supports(
            D_POSTGRES_CLIENT_CHUNKED_ROWS);
    }

    bool
    supports_logical_repl() const noexcept
    {
        return m_server_at_least(10u, 0u);
    }

    bool
    supports_partitioning() const noexcept
    {
        return m_server_at_least(10u, 0u);
    }

    bool
    supports_parallel_query() const noexcept
    {
        return m_server_at_least(9u, 6u);
    }

    bool
    supports_modern_sql() const noexcept
    {
        return m_supports(D_POSTGRES_FEATURE_MERGE);
    }

    bool
    supports_modern_json() const noexcept
    {
        return m_supports(D_POSTGRES_FEATURE_SQL_JSON);
    }

    // type mapping -- an OID as field_type, whether an OID is an array's,
    // and a field_type as the type a column is declared with, as section 2
    // maps them
    static field_type
    map_oid(
        unsigned int _oid
    ) noexcept
    {
        return pg_oid_to_field_type(_oid);
    }

    static bool
    oid_is_array(
        unsigned int _oid
    ) noexcept
    {
        return pg_oid_is_array(_oid);
    }

    static const char*
    sql_type_name(
        field_type _type
    ) noexcept
    {
        return field_type_to_pg_sql(_type);
    }

    // configuration -- the PostgreSQL settings, base among them as the
    // connection's generic configuration now is, which connect(config)
    // replaces; setting them replaces the generic configuration with
    // theirs, for the next connect
    pg_connect_config get_pg_config() const;
    void              set_pg_config(const pg_connect_config& _config);

    // get_face -- the PostgreSQL layer's face, for the layer's other calls;
    // closing or replacing the connection through it is the connection's to
    // do, not the face's
    face::postgres&       get_face() noexcept;
    const face::postgres& get_face() const noexcept;

    // hooks (postgres.cpp) -- database_connection calls these, programs the
    // operations they implement, whose contracts they keep
    void            connect_impl();
    int             connect_start_impl(d_db_pending& _pending);
    int             connect_continue_impl(unsigned      _events,
                                          d_db_pending& _pending);
    void            connect_finish_impl();
    void            connect_abandon_impl() noexcept;
    void            disconnect_impl() noexcept;
    bool            is_connected_impl() const noexcept;
    bool            ping_impl() const noexcept;
    face::rows      execute_query_impl(const std::string& _query);
    re_std::int64_t execute_update_impl(const std::string& _query);
    face::postgres_statement
                    prepare_impl(const std::string& _query);
    std::string     get_server_version_impl() const;
    re_std::int64_t get_last_insert_id_impl() const;
    void            begin_transaction_impl();
    void            begin_transaction_impl(isolation_level _isolation);
    void            commit_impl();
    void            rollback_impl();
    bool            in_transaction_impl() const noexcept;
    isolation_level get_isolation_level_impl() const;
    void            set_isolation_level_impl(isolation_level _level);
    void            create_savepoint_impl(const std::string& _name);
    void            release_savepoint_impl(const std::string& _name);
    void            rollback_to_savepoint_impl(const std::string& _name);
    struct ::pg_conn*
                    native_handle_impl() const noexcept;

private:
    /**
     * @brief Raises a failure of the PostgreSQL layer, as
     *        postgres_layer::raise does; a success returns.
     *
     * @param[in] _result     the call's result.
     * @param[in] _kind       which exception a failure throws, unless it lost
     *                        the connection.
     * @param[in] _operation  the operation's name, for the message.
     * @throws as postgres_layer::raise, unless `_result` is 0.
     */
    void m_check(int          _result,
                 failure_kind _kind,
                 const char*  _operation) const;

    // m_supports, m_server_at_least -- the layer's answers for the server
    // connected, from libpq's copy of its version; false while the
    // connection is not open
    bool m_supports(d_postgres_feature _feature) const noexcept;
    bool m_server_at_least(unsigned _major,
                           unsigned _minor) const noexcept;

    /**
     * @brief Runs one statement with values for its parameters, and returns
     *        its rows.
     *
     * @param[in] _query   the statement.
     * @param[in] _values  the values, as C strings; NULL for SQL NULL.
     * @return the rows, all of them, held apart from the connection.
     * @throws as exec_params.
     */
    std::unique_ptr<buffered_result_set> m_exec_params(
        const std::string&              _query,
        const std::vector<const char*>& _values);

    // m_finish_connect -- the search path and the statement timeout set on
    // a connection made, which the object then takes
    void m_finish_connect(face::postgres& _opened);

    // m_session_setup -- the same two as one statement, for a connection
    // made without waiting; empty where neither is configured
    std::string m_session_setup(face::postgres& _opened) const;

    // m_setup_step -- connect_async's setup on the connection being made,
    // after the handshake: the statement sent, then its results read, each
    // without waiting; the layer's result, D_DB_WAIT while waiting
    int m_setup_step(unsigned      _events,
                     d_db_pending& _pending);

    pg_connect_config m_pg_config;
    face::postgres    m_db;
    face::postgres    m_opening;
    int               m_opening_stage;  // connect_async: 0 the handshake, 1
                                        // the setup being sent, 2 its
                                        // results being read, 3 done
};



//==============================================================================
// 8.  EXPRESSION DETECTORS
//==============================================================================
// Sections 8 to 12, once postgres_traits.hpp and postgres_concepts.hpp,
// detect which of pg_connection's members a type has: as traits from C++17
// and as concepts under C++20. A detector is the type of one member's call,
// which is_detected tests.


// 8.1    Asynchronous query dispatch
//------------------------------------------------------------------------------
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

// 8.2    Pipeline mode
//------------------------------------------------------------------------------
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

// pg_pipeline_flush_t
//   detector: pipeline_flush() method.
// wraps PQsendFlushRequest().
template<typename T>
using pg_pipeline_flush_t =
    decltype(std::declval<T&>().pipeline_flush());

// 8.3    COPY protocol
//------------------------------------------------------------------------------
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

// 8.4    LISTEN and NOTIFY
//------------------------------------------------------------------------------
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

// 8.5    Parameterized execution
//------------------------------------------------------------------------------
// pg_exec_params_t
//   detector: exec_params(const std::string&,
// const std::vector<std::string>&) method.
// wraps PQexecParams().
template<typename T>
using pg_exec_params_t = decltype(std::declval<T&>().exec_params(
    std::declval<const std::string&>(),
    std::declval<const std::vector<std::string>&>()));

// 8.6    Connection diagnostics
//------------------------------------------------------------------------------
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

// 8.7    Schema introspection
//------------------------------------------------------------------------------
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

// 8.8    Escaping
//------------------------------------------------------------------------------
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

// 8.9    Large objects
//------------------------------------------------------------------------------
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


//==============================================================================
// 9.  TAGGED CAPABILITY TRAITS
//==============================================================================
// Each trait is a type with ::value, true where every detector it names finds
// its member; name_v is the value, where the language has variable templates.


// 9.1    Traits
//------------------------------------------------------------------------------
// 9.1.1
// has_pg_async
//   trait: checks if type T supports asynchronous query dispatch
// (send_query + get_result + is_busy + consume_input).
template<typename T>
struct has_pg_async : djinterp::conjunction<
    is_detected<pg_send_query_t, clean_t<T>>,
    is_detected<pg_get_result_t, clean_t<T>>,
    is_detected<pg_is_busy_t, clean_t<T>>,
    is_detected<pg_consume_input_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_async_v = has_pg_async<clean_t<T>>::value;
#endif

// 9.1.2
// has_pg_pipeline
//   trait: checks if type T supports pipeline mode
// (enter_pipeline + exit_pipeline + pipeline_sync).
template<typename T>
struct has_pg_pipeline : djinterp::conjunction<
    is_detected<pg_enter_pipeline_t, clean_t<T>>,
    is_detected<pg_exit_pipeline_t, clean_t<T>>,
    is_detected<pg_pipeline_sync_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_pipeline_v = has_pg_pipeline<clean_t<T>>::value;
#endif

// 9.1.3
// has_pg_copy
//   trait: checks if type T supports the COPY protocol
// (copy_in_start + copy_data + copy_end).
template<typename T>
struct has_pg_copy : djinterp::conjunction<
    is_detected<pg_copy_in_start_t, clean_t<T>>,
    is_detected<pg_copy_data_t, clean_t<T>>,
    is_detected<pg_copy_end_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_copy_v = has_pg_copy<clean_t<T>>::value;
#endif

// 9.1.4
// has_pg_listen_notify
//   trait: checks if type T supports LISTEN/NOTIFY
// (listen + notify + get_notification).
template<typename T>
struct has_pg_listen_notify : djinterp::conjunction<
    is_detected<pg_listen_t, clean_t<T>>,
    is_detected<pg_notify_t, clean_t<T>>,
    is_detected<pg_get_notification_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_listen_notify_v =
        has_pg_listen_notify<clean_t<T>>::value;
#endif

// 9.1.5
// has_pg_diagnostics
//   trait: checks if type T supports connection diagnostics
// (get_backend_pid + get_transaction_status + get_parameter_status).
template<typename T>
struct has_pg_diagnostics : djinterp::conjunction<
    is_detected<pg_get_backend_pid_t, clean_t<T>>,
    is_detected<pg_transaction_status_t, clean_t<T>>,
    is_detected<pg_parameter_status_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_diagnostics_v =
        has_pg_diagnostics<clean_t<T>>::value;
#endif

// 9.1.6
// has_pg_escape
//   trait: checks if type T supports PostgreSQL escaping
// (escape_literal + escape_identifier).
template<typename T>
struct has_pg_escape : djinterp::conjunction<
    is_detected<pg_escape_literal_t, clean_t<T>>,
    is_detected<pg_escape_identifier_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_escape_v = has_pg_escape<clean_t<T>>::value;
#endif

// 9.1.7
// has_pg_large_objects
//   trait: checks if type T supports large object API
// (lo_import + lo_export).
template<typename T>
struct has_pg_large_objects : djinterp::conjunction<
    is_detected<pg_lo_import_t, clean_t<T>>,
    is_detected<pg_lo_export_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_large_objects_v =
        has_pg_large_objects<clean_t<T>>::value;
#endif

// 9.1.8
// has_pg_schema_query
//   trait: checks if type T supports schema introspection
// (has_table + list_tables).
template<typename T>
struct has_pg_schema_query : djinterp::conjunction<
    is_detected<pg_has_table_t, clean_t<T>>,
    is_detected<pg_list_tables_t, clean_t<T>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_pg_schema_query_v =
        has_pg_schema_query<clean_t<T>>::value;
#endif

// 9.1.9
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
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool is_pg_connection_v = is_pg_connection<clean_t<T>>::value;
#endif


//==============================================================================
// 10.  TAGLESS CAPABILITY TRAITS
//==============================================================================
// The same capabilities as constexpr bool variable templates, for a condition
// that needs no type: one member each, then the sets.


// 10.1   Individual capabilities
//------------------------------------------------------------------------------
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

// 10.2   Compound capabilities
//------------------------------------------------------------------------------
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


//==============================================================================
// 11.  SFINAE HELPERS
//==============================================================================


// 11.1   enable_if helpers
//------------------------------------------------------------------------------
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


//==============================================================================
// 12.  C++20 CONCEPTS
//==============================================================================
// Each concept forwards to a trait or a tagless capability above. They live
// in djinterp::concepts (2026.10.21), where a concept may take the name of
// the class it constrains; each former name in djinterp forwards to its
// concept for one release. They exist where the compiler has concepts; the
// traits serve at C++17 without them, as in functor.hpp and monoid.hpp.


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace concepts
{

// 12.1   Connection concepts
//------------------------------------------------------------------------------
// postgres_connection
//   concept: constrains types implementing the PostgreSQL connection
// interface.
template<typename Type>
concept postgres_connection =
    is_pg_connection<clean_t<Type>>::value;

// pg_connection
//   concept: alias for postgres_connection using the shorter pg prefix.
template<typename Type>
concept pg_connection =
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

// 12.2   Capability concepts
//------------------------------------------------------------------------------
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

// postgres_pipeline_flush_connection
//   concept: constrains types exposing pipeline_flush() (2026.10.22, so it
// has no former name).
template<typename Type>
concept postgres_pipeline_flush_connection =
    is_detected<pg_pipeline_flush_t, clean_t<Type>>::value;

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

// 12.3   Tagless capability concepts
//------------------------------------------------------------------------------
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

}  // namespace concepts

// 12.4   Former names
//------------------------------------------------------------------------------
// postgres_connection
//   concept: concepts::postgres_connection, by its former name, for one
// release.
template<typename Type>
concept postgres_connection =
    concepts::postgres_connection<Type>;

// Pg_connection
//   concept: concepts::pg_connection, by its former name, for one release.
template<typename Type>
concept Pg_connection =
    concepts::pg_connection<Type>;

// non_postgres_connection
//   concept: concepts::non_postgres_connection, by its former name, for one
// release.
template<typename Type>
concept non_postgres_connection =
    concepts::non_postgres_connection<Type>;

// postgres_async_connection
//   concept: concepts::postgres_async_connection, by its former name, for one
// release.
template<typename Type>
concept postgres_async_connection =
    concepts::postgres_async_connection<Type>;

// postgres_pipeline_connection
//   concept: concepts::postgres_pipeline_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_pipeline_connection =
    concepts::postgres_pipeline_connection<Type>;

// postgres_copy_connection
//   concept: concepts::postgres_copy_connection, by its former name, for one
// release.
template<typename Type>
concept postgres_copy_connection =
    concepts::postgres_copy_connection<Type>;

// postgres_listen_notify_connection
//   concept: concepts::postgres_listen_notify_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_listen_notify_connection =
    concepts::postgres_listen_notify_connection<Type>;

// postgres_diagnostics_connection
//   concept: concepts::postgres_diagnostics_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_diagnostics_connection =
    concepts::postgres_diagnostics_connection<Type>;

// postgres_escape_connection
//   concept: concepts::postgres_escape_connection, by its former name, for one
// release.
template<typename Type>
concept postgres_escape_connection =
    concepts::postgres_escape_connection<Type>;

// postgres_large_object_connection
//   concept: concepts::postgres_large_object_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_large_object_connection =
    concepts::postgres_large_object_connection<Type>;

// postgres_schema_query_connection
//   concept: concepts::postgres_schema_query_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_schema_query_connection =
    concepts::postgres_schema_query_connection<Type>;

// postgres_send_query_connection
//   concept: concepts::postgres_send_query_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_send_query_connection =
    concepts::postgres_send_query_connection<Type>;

// postgres_result_polling_connection
//   concept: concepts::postgres_result_polling_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_result_polling_connection =
    concepts::postgres_result_polling_connection<Type>;

// postgres_busy_state_query
//   concept: concepts::postgres_busy_state_query, by its former name, for one
// release.
template<typename Type>
concept postgres_busy_state_query =
    concepts::postgres_busy_state_query<Type>;

// postgres_input_consuming_connection
//   concept: concepts::postgres_input_consuming_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_input_consuming_connection =
    concepts::postgres_input_consuming_connection<Type>;

// postgres_pipeline_enterable_connection
//   concept: concepts::postgres_pipeline_enterable_connection, by its former
// name, for one release.
template<typename Type>
concept postgres_pipeline_enterable_connection =
    concepts::postgres_pipeline_enterable_connection<Type>;

// postgres_pipeline_exitable_connection
//   concept: concepts::postgres_pipeline_exitable_connection, by its former
// name, for one release.
template<typename Type>
concept postgres_pipeline_exitable_connection =
    concepts::postgres_pipeline_exitable_connection<Type>;

// postgres_pipeline_sync_connection
//   concept: concepts::postgres_pipeline_sync_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_pipeline_sync_connection =
    concepts::postgres_pipeline_sync_connection<Type>;

// postgres_copy_in_connection
//   concept: concepts::postgres_copy_in_connection, by its former name, for one
// release.
template<typename Type>
concept postgres_copy_in_connection =
    concepts::postgres_copy_in_connection<Type>;

// postgres_copy_data_connection
//   concept: concepts::postgres_copy_data_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_copy_data_connection =
    concepts::postgres_copy_data_connection<Type>;

// postgres_copy_end_connection
//   concept: concepts::postgres_copy_end_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_copy_end_connection =
    concepts::postgres_copy_end_connection<Type>;

// postgres_listenable_connection
//   concept: concepts::postgres_listenable_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_listenable_connection =
    concepts::postgres_listenable_connection<Type>;

// postgres_notifiable_connection
//   concept: concepts::postgres_notifiable_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_notifiable_connection =
    concepts::postgres_notifiable_connection<Type>;

// postgres_notification_query_connection
//   concept: concepts::postgres_notification_query_connection, by its former
// name, for one release.
template<typename Type>
concept postgres_notification_query_connection =
    concepts::postgres_notification_query_connection<Type>;

// postgres_parameterized_execution_connection
//   concept: concepts::postgres_parameterized_execution_connection, by its
// former name, for one release.
template<typename Type>
concept postgres_parameterized_execution_connection =
    concepts::postgres_parameterized_execution_connection<Type>;

// postgres_backend_pid_connection
//   concept: concepts::postgres_backend_pid_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_backend_pid_connection =
    concepts::postgres_backend_pid_connection<Type>;

// postgres_transaction_status_connection
//   concept: concepts::postgres_transaction_status_connection, by its former
// name, for one release.
template<typename Type>
concept postgres_transaction_status_connection =
    concepts::postgres_transaction_status_connection<Type>;

// postgres_parameter_status_connection
//   concept: concepts::postgres_parameter_status_connection, by its former
// name, for one release.
template<typename Type>
concept postgres_parameter_status_connection =
    concepts::postgres_parameter_status_connection<Type>;

// postgres_has_table_query
//   concept: concepts::postgres_has_table_query, by its former name, for one
// release.
template<typename Type>
concept postgres_has_table_query =
    concepts::postgres_has_table_query<Type>;

// postgres_table_names_query
//   concept: concepts::postgres_table_names_query, by its former name, for one
// release.
template<typename Type>
concept postgres_table_names_query =
    concepts::postgres_table_names_query<Type>;

// postgres_escape_literal_connection
//   concept: concepts::postgres_escape_literal_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_escape_literal_connection =
    concepts::postgres_escape_literal_connection<Type>;

// postgres_escape_identifier_connection
//   concept: concepts::postgres_escape_identifier_connection, by its former
// name, for one release.
template<typename Type>
concept postgres_escape_identifier_connection =
    concepts::postgres_escape_identifier_connection<Type>;

// postgres_lo_import_connection
//   concept: concepts::postgres_lo_import_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_lo_import_connection =
    concepts::postgres_lo_import_connection<Type>;

// postgres_lo_export_connection
//   concept: concepts::postgres_lo_export_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_lo_export_connection =
    concepts::postgres_lo_export_connection<Type>;

// postgres_async_dispatchable
//   concept: concepts::postgres_async_dispatchable, by its former name, for one
// release.
template<typename Type>
concept postgres_async_dispatchable =
    concepts::postgres_async_dispatchable<Type>;

// postgres_pipelined_connection
//   concept: concepts::postgres_pipelined_connection, by its former name, for
// one release.
template<typename Type>
concept postgres_pipelined_connection =
    concepts::postgres_pipelined_connection<Type>;

// postgres_copy_protocol_connection
//   concept: concepts::postgres_copy_protocol_connection, by its former name,
// for one release.
template<typename Type>
concept postgres_copy_protocol_connection =
    concepts::postgres_copy_protocol_connection<Type>;

// postgres_listen_notifiable_connection
//   concept: concepts::postgres_listen_notifiable_connection, by its former
// name, for one release.
template<typename Type>
concept postgres_listen_notifiable_connection =
    concepts::postgres_listen_notifiable_connection<Type>;

// postgres_full_connection
//   concept: concepts::postgres_full_connection, by its former name, for one
// release.
template<typename Type>
concept postgres_full_connection =
    concepts::postgres_full_connection<Type>;

// pg_full_connection
//   concept: concepts::pg_full_connection, by its former name, for one release.
template<typename Type>
concept pg_full_connection =
    concepts::pg_full_connection<Type>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_POSTGRES_POSTGRES_HPP
