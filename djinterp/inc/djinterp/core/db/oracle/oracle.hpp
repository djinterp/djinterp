/******************************************************************************
* djinterp [database]                                               oracle.hpp
* 
* djinterp Oracle Database connection module:
*   This header provides the Oracle-specific connection implementation and
* associated data type infrastructure for the djinterp database module,
* including:
*   - OCI SQLT_* type code enumeration for bind/define operations
*   - SQLT-to-field_type mapping
*   - compile-time feature availability gated on THREE axes:
*     1. server version (D_ENV_ORA_VERSION_AT_LEAST)
*     2. edition (D_ENV_ORA_EDITION_XE / SE2 / EE)
*     3. separately-licensed EE options (D_ENV_ORA_OPTION_*)
*   - Oracle-specific connection configuration (service name / SID,
*     TNS connect descriptor, Easy Connect syntax, wallet, session mode)
*   - the concrete oracle_connection CRTP leaf class with statement
*     caching, batch DML, LOB operations, PL/SQL execution, flashback
*     query, DBMS_OUTPUT capture, edition-based redefinition, SODA,
*     continuous query notification, and schema introspection
*   - version-gated method declarations for 23ai features (JSON duality,
*     SQL domains, boolean type, property graph, SQL Firewall)
*
*   Oracle Database has the richest and most complex feature matrix of
* any supported vendor:
*   - commercial RDBMS: features depend on license, not just version
*   - OCI (Oracle Call Interface): handle-based C API with OCIEnv,
*     OCIError, OCISvcCtx, OCIStmt hierarchy
*   - SQLT_* type codes: used for binding and defining column values,
*     distinct from both PG OIDs and MySQL MYSQL_TYPE_*
*   - PL/SQL: first-class procedural language with packages, triggers,
*     types, and native compilation
*   - multitenant: CDB/PDB architecture mandatory from 21c
*
*   LAYER DIAGRAM:
*     oracle_connection (this file)
*       -> database_connection<oracle_connection, database_type::oracle>
*         -> connection_template<oracle_connection, database_type::oracle>
*           -> connection<oracle_connection>
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include <oci.h>; the
* concrete _impl method definitions in oracle.cpp include it.
*
* 
* path:      /inc/djinterp/core/db/oracle/oracle.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.26
******************************************************************************/

#ifndef DJINTERP_DATABASE_ORACLE_
#define DJINTERP_DATABASE_ORACLE_

#include "../database_connection.hpp"
#include "./oracle_traits.hpp"

#include "../../../env/db/env_oracle.h"


NS_DJINTERP


// =============================================================================
// I.   OCI SQLT TYPE CODES
// =============================================================================
// OCI uses SQLT_* constants for binding (OCIBindByPos/Name) and defining
// (OCIDefineByPos) column/parameter types. These constants are stable
// across all OCI versions. Declared as a scoped enum so that <oci.h>
// does not need to be included.
//
// Source: OCI Programmer's Guide and oci.h / ocidfn.h

// oci_sqlt
//   enumeration: OCI external data type codes (SQLT_*).
enum class oci_sqlt : std::uint16_t
{
    // -----------------------------------------------------------------
    // character types
    // -----------------------------------------------------------------
    sqlt_chr           = 1,         // VARCHAR2 (variable-length)
    sqlt_str           = 5,         // null-terminated string
    sqlt_afc           = 96,        // CHAR (fixed-length, blank-padded)
    sqlt_vcs           = 9,         // VARCHAR (with 2-byte length prefix)
    sqlt_lng           = 8,         // LONG (up to 2 GB text, deprecated)

    // -----------------------------------------------------------------
    // numeric types
    // -----------------------------------------------------------------
    sqlt_num           = 2,         // NUMBER (Oracle internal format)
    sqlt_int           = 3,         // native integer
    sqlt_flt           = 4,         // native float
    sqlt_vnu           = 6,         // VARNUM (variable-length NUMBER)
    sqlt_pdn           = 7,         // packed decimal number
    sqlt_bfloat        = 21,        // BINARY_FLOAT (IEEE 754 single)
    sqlt_bdouble       = 22,        // BINARY_DOUBLE (IEEE 754 double)

    // -----------------------------------------------------------------
    // date and time types
    // -----------------------------------------------------------------
    sqlt_dat           = 12,        // DATE (7-byte Oracle format)
    sqlt_timestamp     = 187,       // TIMESTAMP
    sqlt_timestamp_tz  = 188,       // TIMESTAMP WITH TIME ZONE
    sqlt_timestamp_ltz = 232,       // TIMESTAMP WITH LOCAL TIME ZONE
    sqlt_interval_ym   = 189,       // INTERVAL YEAR TO MONTH
    sqlt_interval_ds   = 190,       // INTERVAL DAY TO SECOND

    // -----------------------------------------------------------------
    // binary / raw types
    // -----------------------------------------------------------------
    sqlt_bin           = 23,        // RAW (variable-length binary)
    sqlt_lbi           = 24,        // LONG RAW (deprecated)
    sqlt_lvb           = 95,        // LONG VARRAW

    // -----------------------------------------------------------------
    // LOB types
    // -----------------------------------------------------------------
    sqlt_clob          = 112,       // CLOB (character large object)
    sqlt_blob          = 113,       // BLOB (binary large object)
    sqlt_bfile         = 114,       // BFILE (external file pointer)

    // -----------------------------------------------------------------
    // object types
    // -----------------------------------------------------------------
    sqlt_nty           = 108,       // named object type (user-defined)
    sqlt_ref           = 110,       // REF (object reference)

    // -----------------------------------------------------------------
    // cursor / result set
    // -----------------------------------------------------------------
    sqlt_rset          = 116,       // result set (REF CURSOR)

    // -----------------------------------------------------------------
    // ROWID types
    // -----------------------------------------------------------------
    sqlt_rid           = 11,        // ROWID (physical)
    sqlt_rdd           = 104,       // ROWID (descriptor form)

    // -----------------------------------------------------------------
    // national character types
    // -----------------------------------------------------------------
    sqlt_nchar         = 96,        // NCHAR (fixed-length, national)
    sqlt_nvarchar      = 1,         // NVARCHAR2 (uses charset form)
    sqlt_nclob         = 112,       // NCLOB (national CLOB)

    // -----------------------------------------------------------------
    // JSON (21c+)
    // -----------------------------------------------------------------
    sqlt_json          = 119,       // JSON (OSON binary format)

    // -----------------------------------------------------------------
    // boolean (23ai+)
    // -----------------------------------------------------------------
    sqlt_bol           = 252,       // BOOLEAN

    // -----------------------------------------------------------------
    // XML
    // -----------------------------------------------------------------
    sqlt_xml           = 108        // XMLType (named type)
};


// =============================================================================
// II.  SQLT-TO-FIELD_TYPE MAPPING
// =============================================================================

// oci_sqlt_to_field_type
//   function: maps an OCI SQLT_* type code to the generic djinterp
// field_type.
inline field_type oci_sqlt_to_field_type(
    std::uint16_t _sqlt_code) noexcept
{
    switch (static_cast<oci_sqlt>(_sqlt_code))
    {
        // string types
        case oci_sqlt::sqlt_chr:
        case oci_sqlt::sqlt_str:
        case oci_sqlt::sqlt_afc:
        case oci_sqlt::sqlt_vcs:
        case oci_sqlt::sqlt_lng:
            return field_type::string;

        // numeric types
        case oci_sqlt::sqlt_num:
        case oci_sqlt::sqlt_vnu:
        case oci_sqlt::sqlt_pdn:
            return field_type::decimal;

        case oci_sqlt::sqlt_int:
            return field_type::big_integer;

        case oci_sqlt::sqlt_flt:
        case oci_sqlt::sqlt_bfloat:
        case oci_sqlt::sqlt_bdouble:
            return field_type::floating_point;

        // date and time
        case oci_sqlt::sqlt_dat:
            return field_type::datetime;

        case oci_sqlt::sqlt_timestamp:
        case oci_sqlt::sqlt_timestamp_tz:
        case oci_sqlt::sqlt_timestamp_ltz:
            return field_type::timestamp;

        case oci_sqlt::sqlt_interval_ym:
        case oci_sqlt::sqlt_interval_ds:
            return field_type::time;

        // binary / raw
        case oci_sqlt::sqlt_bin:
        case oci_sqlt::sqlt_lbi:
        case oci_sqlt::sqlt_lvb:
            return field_type::binary;

        // LOBs
        case oci_sqlt::sqlt_clob:
            return field_type::string;

        case oci_sqlt::sqlt_blob:
        case oci_sqlt::sqlt_bfile:
            return field_type::binary;

        // JSON
        case oci_sqlt::sqlt_json:
            return field_type::json;

        // boolean
        case oci_sqlt::sqlt_bol:
            return field_type::boolean;

        // ROWID
        case oci_sqlt::sqlt_rid:
        case oci_sqlt::sqlt_rdd:
            return field_type::string;

        // cursor (REF CURSOR)
        case oci_sqlt::sqlt_rset:
            return field_type::custom;

        // object, REF
        case oci_sqlt::sqlt_nty:
        case oci_sqlt::sqlt_ref:
            return field_type::custom;

        default:
            return field_type::custom;
    }
}

// field_type_to_oracle_sql
//   function: returns the SQL type name string for a given field_type
// as it would be used in Oracle DDL. Version-dependent types fall
// back to compatible alternatives.
inline const char* field_type_to_oracle_sql(
    field_type _type) noexcept
{
    switch (_type)
    {
        case field_type::null:           return "NULL";

        case field_type::boolean:
#if D_ENV_ORA_DETECTED && D_ENV_ORA_HAS_BOOLEAN_TYPE
            return "BOOLEAN";
#else
            return "NUMBER(1)";
#endif

        case field_type::integer:        return "NUMBER(10)";
        case field_type::big_integer:    return "NUMBER(19)";
        case field_type::floating_point: return "BINARY_DOUBLE";
        case field_type::decimal:        return "NUMBER";
        case field_type::string:         return "VARCHAR2(4000)";
        case field_type::binary:         return "RAW(2000)";
        case field_type::date:           return "DATE";
        case field_type::time:           return "INTERVAL DAY TO SECOND";
        case field_type::datetime:       return "DATE";
        case field_type::timestamp:      return "TIMESTAMP WITH TIME ZONE";

        case field_type::json:
#if D_ENV_ORA_DETECTED && D_ENV_ORA_HAS_JSON_DATA_TYPE
            return "JSON";
#else
            return "CLOB";
#endif

        case field_type::xml:            return "XMLTYPE";
        case field_type::uuid:           return "RAW(16)";
        case field_type::array:          return "CLOB";
        case field_type::custom:
        default:                         return "VARCHAR2(4000)";
    }
}


// =============================================================================
// III. SESSION MODE
// =============================================================================

// oci_session_mode
//   enumeration: OCI session modes for OCISessionBegin / OCILogon2.
enum class oci_session_mode : std::uint32_t
{
    default_mode      = 0x00000000,
    sysdba            = 0x00000002,     // OCI_SYSDBA
    sysoper           = 0x00000004,     // OCI_SYSOPER
    sysasm            = 0x00008000,     // OCI_SYSASM (11gR2+)
    sysbkp            = 0x00020000,     // OCI_SYSBKP (12.1+)
    sysdgd            = 0x00040000,     // OCI_SYSDGD (12.1+)
    syskmt            = 0x00080000,     // OCI_SYSKMT (12.1+)
    sysrac            = 0x00100000,     // OCI_SYSRAC (12.1+)
    prelim_auth       = 0x00000008      // OCI_PRELIM_AUTH
};


// =============================================================================
// IV.  FEATURE SUPPORT (compile-time, version × edition × option gated)
// =============================================================================

// ora_type_support
//   struct: compile-time data type availability flags.
struct ora_type_support
{
#if D_ENV_ORA_DETECTED

    // types (version-gated)
    static constexpr bool has_json_functions =
    #if D_ENV_ORA_HAS_JSON_FUNCTIONS
        true;  #else  false;  #endif
    static constexpr bool has_json_data_type =
    #if D_ENV_ORA_HAS_JSON_DATA_TYPE
        true;  #else  false;  #endif
    static constexpr bool has_json_duality_views =
    #if D_ENV_ORA_HAS_JSON_DUALITY_VIEWS
        true;  #else  false;  #endif
    static constexpr bool has_json_schema_validation =
    #if D_ENV_ORA_HAS_JSON_SCHEMA_VALIDATION
        true;  #else  false;  #endif
    static constexpr bool has_soda =
    #if D_ENV_ORA_HAS_SODA
        true;  #else  false;  #endif
    static constexpr bool has_boolean_type =
    #if D_ENV_ORA_HAS_BOOLEAN_TYPE
        true;  #else  false;  #endif
    static constexpr bool has_sql_domains =
    #if D_ENV_ORA_HAS_SQL_DOMAINS
        true;  #else  false;  #endif
    static constexpr bool has_identity_columns =
    #if D_ENV_ORA_HAS_IDENTITY_COLUMNS
        true;  #else  false;  #endif
    static constexpr bool has_virtual_columns =
    #if D_ENV_ORA_HAS_VIRTUAL_COLUMNS
        true;  #else  false;  #endif
    static constexpr bool has_invisible_columns =
    #if D_ENV_ORA_HAS_INVISIBLE_COLUMNS
        true;  #else  false;  #endif
    static constexpr bool has_xml_db =
    #if D_ENV_ORA_HAS_XML_DB
        true;  #else  false;  #endif

#else
    static constexpr bool has_json_functions         = false;
    static constexpr bool has_json_data_type         = false;
    static constexpr bool has_json_duality_views     = false;
    static constexpr bool has_json_schema_validation = false;
    static constexpr bool has_soda                   = false;
    static constexpr bool has_boolean_type           = false;
    static constexpr bool has_sql_domains            = false;
    static constexpr bool has_identity_columns       = false;
    static constexpr bool has_virtual_columns        = false;
    static constexpr bool has_invisible_columns      = false;
    static constexpr bool has_xml_db                 = false;
#endif
};

// ora_feature_support
//   struct: compile-time feature availability flags. Features are
// gated by version, edition, AND licensed options as appropriate.
struct ora_feature_support
{
#if D_ENV_ORA_DETECTED

    // SQL features
    static constexpr bool has_analytic_functions =
    #if D_ENV_ORA_HAS_ANALYTIC_FUNCTIONS
        true;  #else  false;  #endif
    static constexpr bool has_recursive_with =
    #if D_ENV_ORA_HAS_RECURSIVE_WITH
        true;  #else  false;  #endif
    static constexpr bool has_lateral_inline_view =
    #if D_ENV_ORA_HAS_LATERAL_INLINE_VIEW
        true;  #else  false;  #endif
    static constexpr bool has_match_recognize =
    #if D_ENV_ORA_HAS_MATCH_RECOGNIZE
        true;  #else  false;  #endif
    static constexpr bool has_row_limiting =
    #if D_ENV_ORA_HAS_ROW_LIMITING
        true;  #else  false;  #endif
    static constexpr bool has_if_not_exists =
    #if D_ENV_ORA_HAS_IF_NOT_EXISTS
        true;  #else  false;  #endif
    static constexpr bool has_sql_macro =
    #if D_ENV_ORA_HAS_SQL_MACRO
        true;  #else  false;  #endif
    static constexpr bool has_annotations =
    #if D_ENV_ORA_HAS_ANNOTATIONS
        true;  #else  false;  #endif
    static constexpr bool has_model_clause =
    #if D_ENV_ORA_HAS_MODEL_CLAUSE
        true;  #else  false;  #endif
    static constexpr bool has_property_graph =
    #if D_ENV_ORA_HAS_PROPERTY_GRAPH
        true;  #else  false;  #endif

    // multitenant
    static constexpr bool has_multitenant =
    #if D_ENV_ORA_HAS_MULTITENANT
        true;  #else  false;  #endif
    static constexpr bool multitenant_is_mandatory =
    #if D_ENV_ORA_MULTITENANT_IS_MANDATORY
        true;  #else  false;  #endif

    // flashback
    static constexpr bool has_flashback_query =
    #if D_ENV_ORA_HAS_FLASHBACK_QUERY
        true;  #else  false;  #endif
    static constexpr bool has_flashback_data_archive =
    #if D_ENV_ORA_HAS_FLASHBACK_DATA_ARCHIVE
        true;  #else  false;  #endif

    // security
    static constexpr bool has_unified_audit =
    #if D_ENV_ORA_HAS_UNIFIED_AUDIT
        true;  #else  false;  #endif
    static constexpr bool has_sql_firewall =
    #if D_ENV_ORA_HAS_SQL_FIREWALL
        true;  #else  false;  #endif

    // HA (edition + option gated)
    static constexpr bool has_data_guard =
    #if D_ENV_ORA_HAS_DATA_GUARD
        true;  #else  false;  #endif
    static constexpr bool has_rac =
    #if D_ENV_ORA_HAS_RAC
        true;  #else  false;  #endif
    static constexpr bool has_application_continuity =
    #if D_ENV_ORA_HAS_APPLICATION_CONTINUITY
        true;  #else  false;  #endif

    // in-memory (option gated)
    static constexpr bool has_in_memory =
    #if D_ENV_ORA_HAS_IN_MEMORY
        true;  #else  false;  #endif

    // TDE (option gated)
    static constexpr bool has_tde =
    #if D_ENV_ORA_HAS_TDE
        true;  #else  false;  #endif

    // partitioning (option gated)
    static constexpr bool has_interval_partitioning =
    #if D_ENV_ORA_HAS_INTERVAL_PARTITIONING
        true;  #else  false;  #endif

    // OCI
    static constexpr bool has_oci_json =
    #if D_ENV_ORA_HAS_OCI_JSON
        true;  #else  false;  #endif
    static constexpr bool has_oci_soda =
    #if D_ENV_ORA_HAS_OCI_SODA
        true;  #else  false;  #endif
    static constexpr bool has_oci_implicit_results =
    #if D_ENV_ORA_HAS_OCI_IMPLICIT_RESULTS
        true;  #else  false;  #endif

    // composite
    static constexpr bool has_modern_sql =
    #if D_ENV_ORA_HAS_MODERN_SQL
        true;  #else  false;  #endif
    static constexpr bool has_modern_json =
    #if D_ENV_ORA_HAS_MODERN_JSON
        true;  #else  false;  #endif
    static constexpr bool has_23ai_features =
    #if D_ENV_ORA_HAS_23AI_FEATURES
        true;  #else  false;  #endif
    static constexpr bool is_fully_modern =
    #if D_ENV_ORA_IS_FULLY_MODERN
        true;  #else  false;  #endif
    static constexpr bool is_lts =
    #if D_ENV_ORA_IS_LTS
        true;  #else  false;  #endif

    // edition
    static constexpr bool is_enterprise =
    #if D_ENV_ORA_EDITION_EE
        true;  #else  false;  #endif
    static constexpr bool is_standard =
    #if D_ENV_ORA_EDITION_SE2
        true;  #else  false;  #endif
    static constexpr bool is_express =
    #if D_ENV_ORA_EDITION_XE
        true;  #else  false;  #endif

#else
    static constexpr bool has_analytic_functions    = false;
    static constexpr bool has_recursive_with        = false;
    static constexpr bool has_lateral_inline_view   = false;
    static constexpr bool has_match_recognize       = false;
    static constexpr bool has_row_limiting          = false;
    static constexpr bool has_if_not_exists         = false;
    static constexpr bool has_sql_macro             = false;
    static constexpr bool has_annotations           = false;
    static constexpr bool has_model_clause          = false;
    static constexpr bool has_property_graph        = false;
    static constexpr bool has_multitenant           = false;
    static constexpr bool multitenant_is_mandatory  = false;
    static constexpr bool has_flashback_query       = false;
    static constexpr bool has_flashback_data_archive = false;
    static constexpr bool has_unified_audit         = false;
    static constexpr bool has_sql_firewall          = false;
    static constexpr bool has_data_guard            = false;
    static constexpr bool has_rac                   = false;
    static constexpr bool has_application_continuity = false;
    static constexpr bool has_in_memory             = false;
    static constexpr bool has_tde                   = false;
    static constexpr bool has_interval_partitioning = false;
    static constexpr bool has_oci_json              = false;
    static constexpr bool has_oci_soda              = false;
    static constexpr bool has_oci_implicit_results  = false;
    static constexpr bool has_modern_sql            = false;
    static constexpr bool has_modern_json           = false;
    static constexpr bool has_23ai_features         = false;
    static constexpr bool is_fully_modern           = false;
    static constexpr bool is_lts                    = false;
    static constexpr bool is_enterprise             = false;
    static constexpr bool is_standard               = false;
    static constexpr bool is_express                = false;
#endif
};


// =============================================================================
// V.   VERSION INFORMATION
// =============================================================================

// ora_version_info
//   struct: compile-time version decomposition.
// NOTE: Oracle changed its versioning at 18c. Pre-18c uses four-part
// (12.2.0.1); post-18c uses year-based (19, 21, 23). The encoding
// is MAJOR*10000 + MINOR*100 + UPDATE.
struct ora_version_info
{
#if D_ENV_ORA_DETECTED
    static constexpr bool          detected = true;
    static constexpr std::uint32_t id       = D_ENV_ORA_VERSION_ID;
    static constexpr std::uint16_t major    = D_ENV_ORA_VERSION_MAJOR;
    static constexpr std::uint16_t minor    = D_ENV_ORA_VERSION_MINOR;
    static constexpr bool          is_legacy_versioning =
        D_ENV_ORA_IS_LEGACY_VERSIONING;
    static constexpr const char*   string   = D_ENV_ORA_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr std::uint32_t id       = 0;
    static constexpr std::uint16_t major    = 0;
    static constexpr std::uint16_t minor    = 0;
    static constexpr bool          is_legacy_versioning = false;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected Oracle version is at
    // least the given major version.
    static constexpr bool at_least(std::uint16_t _major) noexcept
    {
        return id >= (_major * 10000u);
    }

    // at_least_legacy
    //   function: returns true if the detected Oracle version is at
    // least (major, minor, update) using the pre-18c encoding.
    static constexpr bool at_least_legacy(
        std::uint16_t _major,
        std::uint16_t _minor,
        std::uint16_t _update) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _update);
    }
};


// =============================================================================
// VI.  ORACLE CONNECTION CONFIGURATION
// =============================================================================

// ora_connect_config
//   struct: Oracle-specific connection configuration. Oracle supports
// multiple connection addressing formats: TNS connect descriptor,
// Easy Connect string, TNS alias, and service name / SID.
struct ora_connect_config
{
    connection_config       base;
    std::string             service_name;
    std::string             sid;
    std::string             tns_descriptor;
    std::string             tns_alias;
    std::string             wallet_location;
    oci_session_mode        session_mode;
    std::string             edition;
    std::string             nls_lang;
    std::size_t             statement_cache_size;
    bool                    use_statement_cache;
    bool                    enable_events;
    bool                    prelim_auth;

    std::map<std::string, std::string> session_attributes;

    ora_connect_config()
        : session_mode(oci_session_mode::default_mode)
        , nls_lang("AMERICAN_AMERICA.AL32UTF8")
        , statement_cache_size(20)
        , use_statement_cache(true)
        , enable_events(false)
        , prelim_auth(false)
    {
        base.host = "localhost";
        base.port = 1521;
    }

    explicit ora_connect_config(const connection_config& _base)
        : base(_base)
        , session_mode(oci_session_mode::default_mode)
        , nls_lang("AMERICAN_AMERICA.AL32UTF8")
        , statement_cache_size(20)
        , use_statement_cache(true)
        , enable_events(false)
        , prelim_auth(false)
    {
    }

    // easy_connect
    //   function: factory using Easy Connect syntax.
    // Format: //host[:port]/service_name
    static ora_connect_config easy_connect(
        const std::string& _host,
        const std::string& _service_name,
        std::uint16_t      _port = 1521)
    {
        ora_connect_config config;

        config.base.host    = _host;
        config.base.port    = _port;
        config.service_name = _service_name;

        return config;
    }

    // with_tns_alias
    //   function: factory using a TNS alias (resolved via tnsnames.ora).
    static ora_connect_config with_tns_alias(
        const std::string& _alias)
    {
        ora_connect_config config;

        config.tns_alias = _alias;

        return config;
    }

    // with_wallet
    //   function: factory for wallet-based authentication.
    static ora_connect_config with_wallet(
        const std::string& _wallet_path,
        const std::string& _service_name)
    {
        ora_connect_config config;

        config.wallet_location = _wallet_path;
        config.service_name    = _service_name;

        return config;
    }
};


// =============================================================================
// VII. ORACLE CONNECTION
// =============================================================================

// oracle_connection
//   class: concrete Oracle Database connection implementation via OCI.
// This is the CRTP leaf class; _impl methods are defined in
// oracle.cpp which includes <oci.h>.
//
// Usage:
//   oracle_connection conn;
//   conn.connect(ora_connect_config::easy_connect(
//       "myhost", "myservice"));
//   auto rs = conn.execute_query("SELECT 1 FROM dual");
class oracle_connection
    : public database_connection<oracle_connection,
                                 database_type::oracle>
{
public:
    using base_type       = database_connection<
        oracle_connection, database_type::oracle>;
    using type_support    = ora_type_support;
    using feature_support = ora_feature_support;
    using version_info    = ora_version_info;

    oracle_connection()
        : base_type()
    {
    }

    explicit oracle_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit oracle_connection(const ora_connect_config& _config)
        : base_type(_config.base)
        , m_ora_config(_config)
    {
    }

    ~oracle_connection() = default;

    // disable copying
    oracle_connection(const oracle_connection&)            = delete;
    oracle_connection& operator=(const oracle_connection&) = delete;

    // enable moving
    oracle_connection(oracle_connection&&) noexcept            = default;
    oracle_connection& operator=(oracle_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // statement caching
    // -----------------------------------------------------------------

    // enable_statement_cache
    //   function: enables OCI statement caching with the given size.
    void enable_statement_cache(std::size_t _size)
    {
        this->ensure_connected();
        m_ora_config.statement_cache_size = _size;
        m_ora_config.use_statement_cache  = true;
        self().enable_statement_cache_impl(_size);
    }

    // get_statement_cache_size
    //   function: returns the current statement cache size.
    std::size_t get_statement_cache_size() const noexcept
    {
        return m_ora_config.statement_cache_size;
    }

    // -----------------------------------------------------------------
    // batch / array DML
    // -----------------------------------------------------------------

    // execute_batch
    //   function: executes a statement with array bindings for the
    // given number of rows. wraps OCIStmtExecute with iters > 1.
    std::int64_t execute_batch(const std::string& _query,
                               std::size_t        _row_count)
    {
        this->ensure_connected();

        return self().execute_batch_impl(_query, _row_count);
    }

    // -----------------------------------------------------------------
    // PL/SQL execution
    // -----------------------------------------------------------------

    // execute_plsql
    //   function: executes an anonymous PL/SQL block.
    void execute_plsql(const std::string& _plsql_block)
    {
        this->ensure_connected();
        self().execute_plsql_impl(_plsql_block);
    }

    // -----------------------------------------------------------------
    // DBMS_OUTPUT capture
    // -----------------------------------------------------------------

    // enable_server_output
    //   function: enables DBMS_OUTPUT and captures output lines.
    void enable_server_output(bool _enabled)
    {
        this->ensure_connected();
        self().enable_server_output_impl(_enabled);
    }

    // get_server_output
    //   function: retrieves captured DBMS_OUTPUT lines.
    std::vector<std::string> get_server_output()
    {
        return self().get_server_output_impl();
    }

    // -----------------------------------------------------------------
    // flashback query
    // -----------------------------------------------------------------

    // set_scn
    //   function: sets the System Change Number for flashback queries.
    void set_scn(std::uint64_t _scn)
    {
        this->ensure_connected();
        self().set_scn_impl(_scn);
    }

    // set_as_of_timestamp
    //   function: sets the AS OF TIMESTAMP for flashback queries.
    void set_as_of_timestamp(const std::string& _timestamp)
    {
        this->ensure_connected();
        self().set_as_of_timestamp_impl(_timestamp);
    }

    // -----------------------------------------------------------------
    // edition-based redefinition
    // -----------------------------------------------------------------

    // set_edition
    //   function: sets the session edition for EBR.
    void set_edition(const std::string& _edition)
    {
        this->ensure_connected();
        self().set_edition_impl(_edition);
    }

    // -----------------------------------------------------------------
    // connection diagnostics
    // -----------------------------------------------------------------

    // get_instance_name
    //   function: returns the Oracle instance name.
    std::string get_instance_name() const
    {
        return self().get_instance_name_impl();
    }

    // get_service_name
    //   function: returns the service name of this connection.
    std::string get_service_name() const
    {
        return self().get_service_name_impl();
    }

    // get_session_id
    //   function: returns the Oracle session ID (SID).
    std::int64_t get_session_id() const
    {
        return self().get_session_id_impl();
    }

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    bool table_exists(const std::string& _table_name) const
    {
        return self().table_exists_impl(_table_name);
    }

    std::vector<std::string> get_table_names() const
    {
        return self().get_table_names_impl();
    }

    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_json_data_type() noexcept
    {
        return type_support::has_json_data_type;
    }

    static constexpr bool supports_boolean_type() noexcept
    {
        return type_support::has_boolean_type;
    }

    static constexpr bool supports_sql_domains() noexcept
    {
        return type_support::has_sql_domains;
    }

    static constexpr bool supports_multitenant() noexcept
    {
        return feature_support::has_multitenant;
    }

    static constexpr bool supports_flashback_query() noexcept
    {
        return feature_support::has_flashback_query;
    }

    static constexpr bool supports_modern_sql() noexcept
    {
        return feature_support::has_modern_sql;
    }

    static constexpr bool supports_rac() noexcept
    {
        return feature_support::has_rac;
    }

    static constexpr bool supports_in_memory() noexcept
    {
        return feature_support::has_in_memory;
    }

    static constexpr bool is_enterprise_edition() noexcept
    {
        return feature_support::is_enterprise;
    }

    static constexpr bool is_23ai() noexcept
    {
        return feature_support::has_23ai_features;
    }

    // -----------------------------------------------------------------
    // data type mapping
    // -----------------------------------------------------------------

    static field_type map_sqlt(std::uint16_t _sqlt_code) noexcept
    {
        return oci_sqlt_to_field_type(_sqlt_code);
    }

    static const char* sql_type_name(field_type _type) noexcept
    {
        return field_type_to_oracle_sql(_type);
    }

    // -----------------------------------------------------------------
    // Oracle-specific configuration
    // -----------------------------------------------------------------

    const ora_connect_config& get_ora_config() const noexcept
    {
        return m_ora_config;
    }

    void set_ora_config(const ora_connect_config& _config)
    {
        m_ora_config   = _config;
        this->m_config = _config.base;
    }

    // -----------------------------------------------------------------
    // _impl methods (defined in oracle.cpp)
    // -----------------------------------------------------------------

    void        connect_impl();
    void        disconnect_impl();
    bool        is_connected_impl() const;
    bool        ping_impl() const;

    auto        execute_query_impl(const std::string& _query)
                    -> std::unique_ptr<
                        result_set<struct oracle_result_set_impl>>;
    std::int64_t execute_update_impl(const std::string& _query);
    bool        execute_impl(const std::string& _query);

    auto        prepare_impl(const std::string& _query)
                    -> std::unique_ptr<
                        statement<struct oracle_statement_impl>>;

    std::string  get_server_version_impl() const;
    std::string  get_last_error_impl() const;
    int          get_last_error_code_impl() const;
    std::int64_t get_last_insert_id_impl() const;
    std::int64_t get_affected_rows_impl() const;

    // Oracle-specific _impl methods
    void enable_statement_cache_impl(std::size_t _size);
    std::int64_t execute_batch_impl(const std::string& _query,
                                     std::size_t _row_count);
    void execute_plsql_impl(const std::string& _plsql_block);
    void enable_server_output_impl(bool _enabled);
    std::vector<std::string> get_server_output_impl();
    void set_scn_impl(std::uint64_t _scn);
    void set_as_of_timestamp_impl(const std::string& _timestamp);
    void set_edition_impl(const std::string& _edition);
    std::string  get_instance_name_impl() const;
    std::string  get_service_name_impl() const;
    std::int64_t get_session_id_impl() const;
    bool table_exists_impl(const std::string& _name) const;
    std::vector<std::string> get_table_names_impl() const;

    // transaction _impl methods
    void begin_transaction_impl();
    void commit_impl();
    void rollback_impl();

    // version-gated methods

#if D_ENV_ORA_DETECTED
    #if D_ENV_ORA_HAS_OCI_SODA
    // soda_create_collection / soda_get_collection
    //   functions: SODA document access. Available since Oracle 18c.
    void soda_create_collection(const std::string& _name);
    auto soda_get_collection(const std::string& _name)
        -> std::optional<std::string>;
    #endif

    #if D_ENV_ORA_HAS_OCI_IMPLICIT_RESULTS
    // get_implicit_results
    //   function: retrieves implicit result sets from PL/SQL.
    // Available since Oracle 12.1.
    auto get_implicit_results()
        -> std::vector<std::unique_ptr<
            result_set<struct oracle_result_set_impl>>>;
    #endif

    #if D_ENV_ORA_HAS_OCI_CONTINUOUS_QUERY
    // subscribe / unsubscribe
    //   functions: continuous query notification.
    void subscribe(const std::string& _query);
    void unsubscribe();
    #endif

    #if D_ENV_ORA_HAS_EDITION_BASED_REDEFINITION
    // get_current_edition
    //   function: returns the current session edition name.
    // Available since Oracle 11gR2.
    std::string get_current_edition() const;
    #endif
#endif  // D_ENV_ORA_DETECTED

private:
    ora_connect_config m_ora_config;

    oracle_connection& self()
    {
        return *this;
    }

    const oracle_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// oracle_result_set_impl
//   struct: forward declaration of the Oracle result set implementation.
struct oracle_result_set_impl;

// oracle_statement_impl
//   struct: forward declaration of the Oracle prepared statement
// implementation.
struct oracle_statement_impl;


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_ORACLE_
