/*******************************************************************************
* djinterp [core]                                                        db2.hpp
*
* djinterp IBM Db2 connection module:
*   This header provides the Db2-specific connection implementation and
* associated data type infrastructure for the djinterp database module,
* including:
*   - Db2 native SQL data type enumeration (SMALLINT, INTEGER, BIGINT,
*     DECIMAL/NUMERIC, DECFLOAT, REAL, DOUBLE, CHAR, VARCHAR, CLOB,
*     GRAPHIC, VARGRAPHIC, DBCLOB, BINARY, VARBINARY, BLOB, DATE, TIME,
*     TIMESTAMP, XML, BOOLEAN, ROWID)
*   - db2_type-to-field_type mapping
*   - compile-time type and feature availability via D_ENV_DB2_* macros
*     covering pureXML, MERGE, compound SQL, temporal tables, MQTs,
*     LOB streaming, row/column organization, SSL, Kerberos, and the
*     SYSCAT / SYSIBM catalog
*   - Db2-specific connection configuration (database alias, current
*     schema, isolation level, security mechanism, SSL, code page,
*     connection-string passthrough)
*   - the concrete db2_connection CRTP leaf class with parameterized
*     execution, server-side named statements, compound SQL, catalog
*     introspection, escaping, diagnostics (SQLCODE / SQLSTATE), LOB
*     streaming, row-set array insert, native XML, and MERGE upsert
*
*   Db2 is a full relational SQL database in the same family as MySQL,
* MariaDB, PostgreSQL, SQLite, and Oracle. It is therefore intended to
* be used with the generic database_table<db2_connection, value> — the
* shared relational table template — rather than a bespoke standalone
* wrapper. Identifier quoting follows the SQL standard (double quotes);
* pagination uses OFFSET <m> ROWS FETCH FIRST <n> ROWS ONLY; schema
* introspection lives under SYSCAT.* and SYSIBM.*.
*
*   LAYER DIAGRAM:
*     db2_connection (this file)
*       -> database_connection<db2_connection, database_type::db2>
*         -> connection_template<db2_connection, database_type::db2>
*           -> connection<db2_connection>
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include the Db2 CLI
* header; the concrete _impl method definitions in db2.cpp include
* <sqlcli1.h> / <sqlcli.h>.
*
*
*   DETECTION:
*   Also carries this database's capability-detection traits and C++20 concepts
* (trailing sections), folded in from db2_traits.hpp and the matching *_concepts.hpp;
* detection now lives with the connection. Concepts gated on concept support.
*
*
* path:      /inc/djinterp/core/db/db2/db2.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.28
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_DB_DB2_DB2_HPP
#define DJINTERP_DB_DB2_DB2_HPP

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_utility.hpp"  // clean_t, self
#include "../../../env/db/db2/env_db2.h"
#include "../database_connection.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t,
                                                   // uint32_t, uint8_t,
                                                   // int64_t


NS_DJINTERP


// =============================================================================
// I.   DB2 NATIVE SQL TYPE ENUMERATION
// =============================================================================
// Db2 identifies every column by an SQL type code (the SQL_TYP_* / SQL_*
// values surfaced through the CLI catalog and result-set descriptors).
// The enumerators below mirror those native types at the djinterp
// abstraction layer. Reserved-word collisions (char / double / int) take
// a trailing underscore.

// db2_type
//   enumeration: native Db2 SQL data types.
enum class db2_type : re_std::uint16_t
{
    // -----------------------------------------------------------------
    // none / missing
    // -----------------------------------------------------------------
    none         = 0x00,

    // -----------------------------------------------------------------
    // numeric types
    // -----------------------------------------------------------------
    smallint     = 0x01,    // 16-bit signed integer
    integer      = 0x02,    // 32-bit signed integer
    bigint       = 0x03,    // 64-bit signed integer
    decimal      = 0x04,    // packed decimal (alias NUMERIC)
    decfloat     = 0x05,    // IEEE-754 decimal floating point
    real         = 0x06,    // 32-bit IEEE-754
    double_      = 0x07,    // 64-bit IEEE-754 (alias FLOAT)

    // -----------------------------------------------------------------
    // character / national-character types
    // -----------------------------------------------------------------
    char_        = 0x10,    // fixed-length single-byte
    varchar      = 0x11,    // varying-length single-byte
    clob         = 0x12,    // character large object
    graphic      = 0x13,    // fixed-length double-byte
    vargraphic   = 0x14,    // varying-length double-byte
    dbclob       = 0x15,    // double-byte character large object

    // -----------------------------------------------------------------
    // binary types
    // -----------------------------------------------------------------
    binary       = 0x20,    // fixed-length binary
    varbinary    = 0x21,    // varying-length binary
    blob         = 0x22,    // binary large object

    // -----------------------------------------------------------------
    // datetime types
    // -----------------------------------------------------------------
    date         = 0x30,    // calendar date
    time         = 0x31,    // time of day
    timestamp    = 0x32,    // date + time (+ fractional seconds)

    // -----------------------------------------------------------------
    // structured / special types
    // -----------------------------------------------------------------
    xml          = 0x40,    // pureXML document
    boolean      = 0x41,    // true / false (Db2 11.1+)
    rowid        = 0x42,    // row identifier

    // -----------------------------------------------------------------
    // sentinel: not a recognised type
    // -----------------------------------------------------------------
    unknown      = 0xFF
};


// =============================================================================
// II.  DB2-TYPE-TO-FIELD_TYPE MAPPING
// =============================================================================

// db2_type_to_field_type
//   function: maps a Db2 native SQL type to the generic djinterp
// field_type.
inline field_type db2_type_to_field_type(db2_type _type) noexcept
{
    switch (_type)
    {
        case db2_type::none:
            return field_type::null;

        case db2_type::smallint:
        case db2_type::integer:
            return field_type::integer;

        case db2_type::bigint:
            return field_type::big_integer;

        case db2_type::real:
        case db2_type::double_:
            return field_type::floating_point;

        case db2_type::decimal:
        case db2_type::decfloat:
            // DECIMAL / DECFLOAT carry exact precision; map to decimal
            // rather than floating_point to preserve it.
            return field_type::decimal;

        case db2_type::char_:
        case db2_type::varchar:
        case db2_type::clob:
        case db2_type::graphic:
        case db2_type::vargraphic:
        case db2_type::dbclob:
            return field_type::string;

        case db2_type::binary:
        case db2_type::varbinary:
        case db2_type::blob:
            return field_type::binary;

        case db2_type::date:
            return field_type::date;

        case db2_type::time:
            return field_type::time;

        case db2_type::timestamp:
            return field_type::timestamp;

        case db2_type::xml:
            return field_type::xml;

        case db2_type::boolean:
            return field_type::boolean;

        case db2_type::rowid:
            // ROWID is an opaque binary token.
            return field_type::binary;

        case db2_type::unknown:
        default:
            return field_type::custom;
    }
}

// db2_type_from_name
//   function: maps a Db2 SQL type name to db2_type. Length / precision
// qualifiers are ignored; only the leading keyword is matched.
inline db2_type db2_type_from_name(const std::string& _name) noexcept
{
    if (_name == "SMALLINT")
    {
        return db2_type::smallint;
    }

    if ( (_name == "INTEGER") ||
         (_name == "INT") )
    {
        return db2_type::integer;
    }

    if (_name == "BIGINT")
    {
        return db2_type::bigint;
    }

    if ( (_name == "DECIMAL") ||
         (_name == "NUMERIC") ||
         (_name == "DEC") )
    {
        return db2_type::decimal;
    }

    if (_name == "DECFLOAT")
    {
        return db2_type::decfloat;
    }

    if (_name == "REAL")
    {
        return db2_type::real;
    }

    if ( (_name == "DOUBLE") ||
         (_name == "FLOAT") )
    {
        return db2_type::double_;
    }

    if ( (_name == "CHAR") ||
         (_name == "CHARACTER") )
    {
        return db2_type::char_;
    }

    if (_name == "VARCHAR")
    {
        return db2_type::varchar;
    }

    if (_name == "CLOB")
    {
        return db2_type::clob;
    }

    if (_name == "GRAPHIC")
    {
        return db2_type::graphic;
    }

    if (_name == "VARGRAPHIC")
    {
        return db2_type::vargraphic;
    }

    if (_name == "DBCLOB")
    {
        return db2_type::dbclob;
    }

    if (_name == "BINARY")
    {
        return db2_type::binary;
    }

    if (_name == "VARBINARY")
    {
        return db2_type::varbinary;
    }

    if (_name == "BLOB")
    {
        return db2_type::blob;
    }

    if (_name == "DATE")
    {
        return db2_type::date;
    }

    if (_name == "TIME")
    {
        return db2_type::time;
    }

    if (_name == "TIMESTAMP")
    {
        return db2_type::timestamp;
    }

    if (_name == "XML")
    {
        return db2_type::xml;
    }

    if (_name == "BOOLEAN")
    {
        return db2_type::boolean;
    }

    if (_name == "ROWID")
    {
        return db2_type::rowid;
    }

    return db2_type::unknown;
}

// field_type_to_db2_native
//   function: returns the closest Db2 native SQL type name for a given
// field_type. Used when generating DDL for the db2 dialect (the shared
// database_table emits column types through the dialect helpers; this
// is the Db2-specific source of truth for those helpers).
inline const char* field_type_to_db2_native(field_type _type) noexcept
{
    switch (_type)
    {
        case field_type::null:           return "VARCHAR";
        case field_type::boolean:        return "BOOLEAN";
        case field_type::integer:        return "INTEGER";
        case field_type::big_integer:    return "BIGINT";
        case field_type::floating_point: return "DOUBLE";
        case field_type::decimal:        return "DECIMAL";
        case field_type::string:         return "VARCHAR";
        case field_type::binary:         return "BLOB";
        case field_type::date:           return "DATE";
        case field_type::time:           return "TIME";
        case field_type::datetime:       return "TIMESTAMP";
        case field_type::timestamp:      return "TIMESTAMP";
        case field_type::json:
            // Db2 stores JSON in CLOB / VARCHAR with SYSTOOLS JSON
            // functions, or BSON in BLOB; default to CLOB.
            return "CLOB";
        case field_type::xml:            return "XML";
        case field_type::uuid:
            // no native UUID; store canonical text form.
            return "CHAR(36)";
        case field_type::array:
            // no native array column; caller models as a child table.
            return "VARCHAR";
        case field_type::custom:
        default:                         return "VARCHAR";
    }
}


// =============================================================================
// III. FEATURE SUPPORT (compile-time, version-gated)
// =============================================================================

// db2_type_support
//   struct: compile-time native SQL type availability flags gated by
// D_ENV_DB2_* macros. The relational core types have been stable across
// supported releases; BOOLEAN (11.1+) and DECFLOAT (9.5+) are reported
// behind their own gates.
struct db2_type_support
{
#if D_ENV_DB2_DETECTED

    static constexpr bool has_smallint    = true;
    static constexpr bool has_integer     = true;
    static constexpr bool has_bigint      = true;
    static constexpr bool has_decimal     = true;
    static constexpr bool has_real        = true;
    static constexpr bool has_double      = true;
    static constexpr bool has_char        = true;
    static constexpr bool has_varchar     = true;
    static constexpr bool has_clob        = true;
    static constexpr bool has_graphic     = true;
    static constexpr bool has_vargraphic  = true;
    static constexpr bool has_dbclob      = true;
    static constexpr bool has_binary      = true;
    static constexpr bool has_varbinary   = true;
    static constexpr bool has_blob        = true;
    static constexpr bool has_date        = true;
    static constexpr bool has_time        = true;
    static constexpr bool has_timestamp   = true;
    static constexpr bool has_rowid       = true;

    // DECFLOAT (9.5+)
    static constexpr bool has_decfloat =
    #if D_ENV_DB2_HAS_DECFLOAT
        true;
    #else
        false;
    #endif

    // pureXML (9.1+)
    static constexpr bool has_xml =
    #if D_ENV_DB2_HAS_PUREXML
        true;
    #else
        false;
    #endif

    // BOOLEAN (11.1+)
    static constexpr bool has_boolean =
    #if D_ENV_DB2_HAS_BOOLEAN
        true;
    #else
        false;
    #endif

#else
    static constexpr bool has_smallint    = false;
    static constexpr bool has_integer     = false;
    static constexpr bool has_bigint      = false;
    static constexpr bool has_decimal     = false;
    static constexpr bool has_real        = false;
    static constexpr bool has_double      = false;
    static constexpr bool has_char        = false;
    static constexpr bool has_varchar     = false;
    static constexpr bool has_clob        = false;
    static constexpr bool has_graphic     = false;
    static constexpr bool has_vargraphic  = false;
    static constexpr bool has_dbclob      = false;
    static constexpr bool has_binary      = false;
    static constexpr bool has_varbinary   = false;
    static constexpr bool has_blob        = false;
    static constexpr bool has_date        = false;
    static constexpr bool has_time        = false;
    static constexpr bool has_timestamp   = false;
    static constexpr bool has_rowid       = false;
    static constexpr bool has_decfloat    = false;
    static constexpr bool has_xml         = false;
    static constexpr bool has_boolean     = false;
#endif  // D_ENV_DB2_DETECTED
};

// db2_feature_support
//   struct: compile-time server feature availability flags.
struct db2_feature_support
{
#if D_ENV_DB2_DETECTED

    // MERGE statement (8.2+)
    static constexpr bool has_merge =
    #if D_ENV_DB2_HAS_MERGE
        true;
    #else
        false;
    #endif

    // compound SQL blocks
    static constexpr bool has_compound_sql =
    #if D_ENV_DB2_HAS_COMPOUND_SQL
        true;
    #else
        false;
    #endif

    // pureXML / XQuery (9.1+)
    static constexpr bool has_purexml =
    #if D_ENV_DB2_HAS_PUREXML
        true;
    #else
        false;
    #endif

    // system-period temporal tables (10.1+)
    static constexpr bool has_temporal_tables =
    #if D_ENV_DB2_HAS_TEMPORAL_TABLES
        true;
    #else
        false;
    #endif

    // materialized query tables
    static constexpr bool has_mqt =
    #if D_ENV_DB2_HAS_MQT
        true;
    #else
        false;
    #endif

    // column-organized (BLU) tables (10.5+)
    static constexpr bool has_column_organized =
    #if D_ENV_DB2_HAS_COLUMN_ORGANIZED
        true;
    #else
        false;
    #endif

    // LOB streaming / locators
    static constexpr bool has_lob_streaming =
    #if D_ENV_DB2_HAS_LOB_STREAMING
        true;
    #else
        false;
    #endif

    // row-set / array input binding
    static constexpr bool has_array_input =
    #if D_ENV_DB2_HAS_ARRAY_INPUT
        true;
    #else
        false;
    #endif

    // server-side named statements
    static constexpr bool has_named_statements = true;

    // OFFSET ... FETCH FIRST pagination (always available on supported
    // releases)
    static constexpr bool has_fetch_first = true;

    // SSL / TLS
    static constexpr bool has_ssl =
    #if D_ENV_DB2_HAS_SSL
        true;
    #else
        false;
    #endif

    // Kerberos security mechanism
    static constexpr bool has_kerberos =
    #if D_ENV_DB2_HAS_KERBEROS
        true;
    #else
        false;
    #endif

    // catalog introspection (SYSCAT / SYSIBM) — always present
    static constexpr bool has_catalog = true;

#else
    static constexpr bool has_merge            = false;
    static constexpr bool has_compound_sql     = false;
    static constexpr bool has_purexml          = false;
    static constexpr bool has_temporal_tables  = false;
    static constexpr bool has_mqt              = false;
    static constexpr bool has_column_organized = false;
    static constexpr bool has_lob_streaming    = false;
    static constexpr bool has_array_input      = false;
    static constexpr bool has_named_statements = false;
    static constexpr bool has_fetch_first      = false;
    static constexpr bool has_ssl              = false;
    static constexpr bool has_kerberos         = false;
    static constexpr bool has_catalog          = false;
#endif  // D_ENV_DB2_DETECTED
};


// =============================================================================
// IV.  DB2 VERSION INFORMATION
// =============================================================================

// db2_version_info
//   struct: compile-time version decomposition.
struct db2_version_info
{
#if D_ENV_DB2_DETECTED
    static constexpr bool          detected = true;
    static constexpr re_std::uint32_t id       = D_ENV_DB2_VERSION_ID;
    static constexpr re_std::uint16_t major    = D_ENV_DB2_VERSION_MAJOR;
    static constexpr re_std::uint16_t minor    = D_ENV_DB2_VERSION_MINOR;
    static constexpr re_std::uint16_t mod      = D_ENV_DB2_VERSION_MOD;
    static constexpr const char*   string   = D_ENV_DB2_VERSION_STRING;
#else
    static constexpr bool          detected = false;
    static constexpr re_std::uint32_t id       = 0;
    static constexpr re_std::uint16_t major    = 0;
    static constexpr re_std::uint16_t minor    = 0;
    static constexpr re_std::uint16_t mod      = 0;
    static constexpr const char*   string   = "not detected";
#endif

    // at_least
    //   function: returns true if the detected Db2 version is at least
    // (major, minor, mod).
    static constexpr bool at_least(re_std::uint16_t _major,
                                   re_std::uint16_t _minor,
                                   re_std::uint16_t _mod) noexcept
    {
        return id >= (_major * 10000u + _minor * 100u + _mod);
    }
};


// =============================================================================
// V.   DB2 SECURITY / ISOLATION ENUMERATIONS
// =============================================================================

// db2_isolation_level
//   enumeration: Db2 transaction isolation levels.
enum class db2_isolation_level : re_std::uint8_t
{
    uncommitted_read = 0,   // UR — dirty reads permitted
    cursor_stability = 1,   // CS — default
    read_stability   = 2,   // RS
    repeatable_read  = 3    // RR — serializable
};

// db2_security_mechanism
//   enumeration: Db2 client authentication mechanism.
enum class db2_security_mechanism : re_std::uint8_t
{
    server         = 0,     // SERVER — userid/password at server
    server_encrypt = 1,     // SERVER_ENCRYPT — encrypted password
    kerberos       = 2,     // KERBEROS
    gss_plugin     = 3,     // GSSPLUGIN
    data_encrypt   = 4      // DATA_ENCRYPT — encrypted userid + data
};


// =============================================================================
// VI.  DB2 CONNECTION CONFIGURATION
// =============================================================================

// db2_connect_config
//   struct: Db2-specific connection configuration extending the generic
// connection_config with the database alias, current schema, isolation
// level, security mechanism, SSL, code page, and connection-string
// passthrough.
struct db2_connect_config
{
    connection_config        base;

    // catalogued database alias (or DSN) — Db2 connects by alias rather
    // than by raw database name when one is catalogued locally.
    std::string              database_alias;

    // CURRENT SCHEMA to set after connect (SET SCHEMA).
    std::string              current_schema;

    // CURRENT PATH for unqualified routine / type resolution.
    std::string              current_path;

    // default transaction isolation level.
    db2_isolation_level      isolation;

    // client authentication mechanism.
    db2_security_mechanism   security;

    // SSL / TLS.
    bool                     enable_ssl;
    std::string              ssl_keystore;
    std::string              ssl_keystash;

    // application identification (SET CLIENT / monitoring).
    std::string              application_name;

    // code page (e.g. 1208 for UTF-8).
    int                      code_page;

    // statement timeout (CLI_ATTR query timeout), in seconds.
    int                      query_timeout_seconds;

    // raw CLI / ODBC connection string passthrough (when set, takes
    // precedence over the field-by-field configuration).
    std::string              connection_string;

    std::map<std::string, std::string> extra_params;

    db2_connect_config()
        : isolation(db2_isolation_level::cursor_stability)
        , security(db2_security_mechanism::server)
        , enable_ssl(false)
        , code_page(1208)
        , query_timeout_seconds(0)
    {
        base.host = "localhost";
        base.port = 50000;
    }

    explicit db2_connect_config(const connection_config& _base)
        : base(_base)
        , isolation(db2_isolation_level::cursor_stability)
        , security(db2_security_mechanism::server)
        , enable_ssl(false)
        , code_page(1208)
        , query_timeout_seconds(0)
    {
        if (base.port == 0)
        {
            base.port = 50000;
        }
    }

    explicit db2_connect_config(const std::string& _conn_string)
        : isolation(db2_isolation_level::cursor_stability)
        , security(db2_security_mechanism::server)
        , enable_ssl(false)
        , code_page(1208)
        , query_timeout_seconds(0)
        , connection_string(_conn_string)
    {
    }
};


// =============================================================================
// VII. DB2 CONNECTION
// =============================================================================

// db2_connection
//   class: concrete IBM Db2 connection implementation via the Db2 CLI
// (db2cli / ODBC). This is the CRTP leaf class; _impl methods are
// defined in db2.cpp which includes <sqlcli1.h>.
//
// Usage:
//   db2_connection conn;
//   conn.connect(db2_connect_config{ ... });
//   auto rs = conn.execute_query("SELECT * FROM employees");
//
//   // intended primary use — the shared relational table template:
//   //   database_table<db2_connection, value> t{conn, "employees"};
class db2_connection
    : public database_connection<db2_connection,
                                 database_type::db2>
{
public:
    using base_type       = database_connection<
        db2_connection, database_type::db2>;
    using type_support    = db2_type_support;
    using feature_support = db2_feature_support;
    using version_info    = db2_version_info;

    db2_connection()
        : base_type()
    {
    }

    explicit db2_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit db2_connection(const db2_connect_config& _config)
        : base_type(_config.base)
        , m_db2_config(_config)
    {
    }

    explicit db2_connection(const std::string& _conn_string)
        : base_type()
        , m_db2_config(_conn_string)
    {
    }

    ~db2_connection() = default;

    // disable copying
    db2_connection(const db2_connection&)            = delete;
    db2_connection& operator=(const db2_connection&) = delete;

    // enable moving
    db2_connection(db2_connection&&) noexcept            = default;
    db2_connection& operator=(db2_connection&&) noexcept = default;


    // -----------------------------------------------------------------
    // parameterized execution
    // -----------------------------------------------------------------

    // exec_params
    //   function: binds positional parameter markers (?) and executes,
    // returning the result set.
    auto exec_params(const std::string&              _query,
                     const std::vector<std::string>& _params)
        -> std::unique_ptr<result_set<struct db2_result_set_impl>>
    {
        this->ensure_connected();

        return self().exec_params_impl(_query, _params);
    }


    // -----------------------------------------------------------------
    // server-side named statements
    // -----------------------------------------------------------------

    // prepare_named
    //   function: prepares _sql and associates it with _name for later
    // execution via execute_named().
    void prepare_named(const std::string& _name,
                       const std::string& _sql)
    {
        this->ensure_connected();
        self().prepare_named_impl(_name, _sql);

        return;
    }

    // execute_named
    //   function: executes a previously named prepared statement with
    // positional parameters.
    auto execute_named(const std::string&              _name,
                       const std::vector<std::string>& _params)
        -> std::unique_ptr<result_set<struct db2_result_set_impl>>
    {
        this->ensure_connected();

        return self().execute_named_impl(_name, _params);
    }


    // -----------------------------------------------------------------
    // compound SQL
    // -----------------------------------------------------------------

    // begin_compound
    //   function: begins a compound SQL block. When _atomic is set the
    // block commits or rolls back as a unit.
    void begin_compound(bool _atomic)
    {
        this->ensure_connected();
        self().begin_compound_impl(_atomic);

        return;
    }

    // add_compound
    //   function: appends a statement to the pending compound block.
    void add_compound(const std::string& _sql)
    {
        self().add_compound_impl(_sql);

        return;
    }

    // end_compound
    //   function: dispatches the pending compound block.
    re_std::int64_t end_compound()
    {
        return self().end_compound_impl();
    }


    // -----------------------------------------------------------------
    // catalog / schema introspection
    // -----------------------------------------------------------------

    // table_exists
    //   function: returns true if the named table exists (SYSCAT.TABLES).
    bool table_exists(const std::string& _table_name) const
    {
        return self().table_exists_impl(_table_name);
    }

    // get_table_names
    //   function: returns the table names visible under the current
    // schema (SYSCAT.TABLES).
    std::vector<std::string> get_table_names() const
    {
        return self().get_table_names_impl();
    }

    // get_schema_names
    //   function: returns the schema names (SYSCAT.SCHEMATA).
    std::vector<std::string> get_schema_names() const
    {
        return self().get_schema_names_impl();
    }


    // -----------------------------------------------------------------
    // escaping
    // -----------------------------------------------------------------

    // escape_literal
    //   function: escapes a string literal (doubles embedded single
    // quotes) and wraps it in single quotes.
    std::string escape_literal(const std::string& _input) const
    {
        return self().escape_literal_impl(_input);
    }

    // escape_identifier
    //   function: escapes an identifier (doubles embedded double
    // quotes) and wraps it in double quotes per SQL-standard quoting.
    std::string escape_identifier(const std::string& _input) const
    {
        return self().escape_identifier_impl(_input);
    }


    // -----------------------------------------------------------------
    // diagnostics
    // -----------------------------------------------------------------

    // get_sqlcode
    //   function: returns the SQLCODE of the most recent statement
    // (0 = success, positive = warning, negative = error).
    int get_sqlcode() const
    {
        return self().get_sqlcode_impl();
    }

    // get_sqlstate
    //   function: returns the five-character SQLSTATE of the most
    // recent statement.
    std::string get_sqlstate() const
    {
        return self().get_sqlstate_impl();
    }

    // get_warning
    //   function: returns the most recent diagnostic warning text.
    std::string get_warning() const
    {
        return self().get_warning_impl();
    }


    // -----------------------------------------------------------------
    // LOB streaming
    // -----------------------------------------------------------------

    // lob_read
    //   function: reads up to _length bytes from the LOB identified by
    // _locator.
    std::string lob_read(const std::string& _locator,
                         std::size_t        _length)
    {
        this->ensure_connected();

        return self().lob_read_impl(_locator, _length);
    }

    // lob_write
    //   function: writes _length bytes to the LOB column identified by
    // _locator.
    re_std::int64_t lob_write(const std::string& _locator,
                           const char*        _data,
                           std::size_t        _length)
    {
        this->ensure_connected();

        return self().lob_write_impl(_locator, _data, _length);
    }


    // -----------------------------------------------------------------
    // row-set bulk insert
    // -----------------------------------------------------------------

    // array_insert
    //   function: performs a row-set bound INSERT, binding _rows as a
    // column-wise parameter array. Returns the number of rows inserted.
    re_std::int64_t array_insert(
        const std::string&                          _insert_sql,
        const std::vector<std::vector<std::string>>& _rows)
    {
        this->ensure_connected();

        return self().array_insert_impl(_insert_sql, _rows);
    }


    // -----------------------------------------------------------------
    // native XML
    // -----------------------------------------------------------------

    // xml_insert
    //   function: inserts an XML document into an XML column via the
    // statement _insert_sql with a single XML parameter marker.
    bool xml_insert(const std::string& _insert_sql,
                    const std::string& _xml_document)
    {
        this->ensure_connected();

        return self().xml_insert_impl(_insert_sql, _xml_document);
    }

    // xquery
    //   function: executes a native XQuery expression and returns the
    // result set.
    auto xquery(const std::string& _xquery) const
        -> std::unique_ptr<result_set<struct db2_result_set_impl>>
    {
        return self().xquery_impl(_xquery);
    }


    // -----------------------------------------------------------------
    // MERGE upsert
    // -----------------------------------------------------------------

    // merge
    //   function: executes a MERGE INTO ... statement. Returns the
    // number of affected rows.
    re_std::int64_t merge(const std::string& _merge_sql)
    {
        this->ensure_connected();

        return self().merge_impl(_merge_sql);
    }


    // -----------------------------------------------------------------
    // feature queries (compile-time)
    // -----------------------------------------------------------------

    static constexpr bool supports_merge() noexcept
    {
        return feature_support::has_merge;
    }

    static constexpr bool supports_compound_sql() noexcept
    {
        return feature_support::has_compound_sql;
    }

    static constexpr bool supports_purexml() noexcept
    {
        return feature_support::has_purexml;
    }

    static constexpr bool supports_temporal_tables() noexcept
    {
        return feature_support::has_temporal_tables;
    }

    static constexpr bool supports_column_organized() noexcept
    {
        return feature_support::has_column_organized;
    }

    static constexpr bool supports_lob_streaming() noexcept
    {
        return feature_support::has_lob_streaming;
    }

    static constexpr bool supports_array_input() noexcept
    {
        return feature_support::has_array_input;
    }

    static constexpr bool supports_boolean() noexcept
    {
        return type_support::has_boolean;
    }

    static constexpr bool supports_ssl() noexcept
    {
        return feature_support::has_ssl;
    }


    // -----------------------------------------------------------------
    // data type mapping
    // -----------------------------------------------------------------

    static field_type map_type(db2_type _type) noexcept
    {
        return db2_type_to_field_type(_type);
    }

    static db2_type type_from_name(const std::string& _name) noexcept
    {
        return db2_type_from_name(_name);
    }

    static const char* native_type_name(field_type _type) noexcept
    {
        return field_type_to_db2_native(_type);
    }


    // -----------------------------------------------------------------
    // Db2-specific configuration
    // -----------------------------------------------------------------

    // get_db2_config
    //   function: returns the Db2-specific configuration.
    const db2_connect_config& get_db2_config() const noexcept
    {
        return m_db2_config;
    }

    // set_db2_config
    //   function: replaces the Db2-specific configuration. Must be
    // called before connect().
    void set_db2_config(const db2_connect_config& _config)
    {
        m_db2_config   = _config;
        this->m_config = _config.base;
    }


    // -----------------------------------------------------------------
    // _impl methods (defined in db2.cpp)
    // -----------------------------------------------------------------

    void         connect_helper();
    void         disconnect_helper();
    bool         is_connected_helper() const;
    bool         ping_helper() const;

    auto         execute_query_helper(const std::string& _query)
                     -> std::unique_ptr<
                         result_set<struct db2_result_set_impl>>;
    re_std::int64_t execute_update_helper(const std::string& _query);
    bool         execute_helper(const std::string& _query);

    auto         prepare_impl(const std::string& _query)
                     -> std::unique_ptr<
                         statement<struct db2_statement_impl>>;

    std::string  get_server_version_helper() const;
    std::string  get_last_error_helper() const;
    int          get_last_error_code_helper() const;
    re_std::int64_t get_last_insert_id_impl() const;
    re_std::int64_t get_affected_rows_impl() const;

    // Db2-specific _impl methods
    auto         exec_params_impl(
                     const std::string&              _query,
                     const std::vector<std::string>& _params)
                     -> std::unique_ptr<
                         result_set<struct db2_result_set_impl>>;
    void         prepare_named_impl(const std::string& _name,
                                    const std::string& _sql);
    auto         execute_named_impl(
                     const std::string&              _name,
                     const std::vector<std::string>& _params)
                     -> std::unique_ptr<
                         result_set<struct db2_result_set_impl>>;
    void         begin_compound_impl(bool _atomic);
    void         add_compound_impl(const std::string& _sql);
    re_std::int64_t end_compound_impl();
    bool         table_exists_impl(const std::string& _name) const;
    std::vector<std::string> get_table_names_impl() const;
    std::vector<std::string> get_schema_names_impl() const;
    std::string  escape_literal_impl(const std::string& _input) const;
    std::string  escape_identifier_impl(
                     const std::string& _input) const;
    int          get_sqlcode_impl() const;
    std::string  get_sqlstate_impl() const;
    std::string  get_warning_impl() const;
    std::string  lob_read_impl(const std::string& _locator,
                               std::size_t        _length);
    re_std::int64_t lob_write_impl(const std::string& _locator,
                                const char*        _data,
                                std::size_t        _length);
    re_std::int64_t array_insert_impl(
                     const std::string&                          _sql,
                     const std::vector<std::vector<std::string>>& _rows);
    bool         xml_insert_impl(const std::string& _insert_sql,
                                 const std::string& _xml_document);
    auto         xquery_impl(const std::string& _xquery) const
                     -> std::unique_ptr<
                         result_set<struct db2_result_set_impl>>;
    re_std::int64_t merge_impl(const std::string& _merge_sql);

    // transaction _impl methods
    void         begin_transaction_impl();
    void         commit_impl();
    void         rollback_impl();
    void         set_isolation_impl(int _level);


    // -----------------------------------------------------------------
    // version-gated methods
    // -----------------------------------------------------------------

#if D_ENV_DB2_DETECTED

    #if D_ENV_DB2_HAS_TEMPORAL_TABLES
    // query_as_of
    //   function: system-time temporal query —
    // SELECT ... FROM <t> FOR SYSTEM_TIME AS OF <ts>. Available since
    // Db2 10.1.
    auto query_as_of(const std::string& _table,
                     const std::string& _timestamp) const
        -> std::unique_ptr<result_set<struct db2_result_set_impl>>;
    #endif

    #if D_ENV_DB2_HAS_COLUMN_ORGANIZED
    // create_column_organized
    //   function: CREATE TABLE ... ORGANIZE BY COLUMN (BLU
    // Acceleration). Available since Db2 10.5.
    bool create_column_organized(const std::string& _table,
                                 const std::string& _schema_spec);
    #endif

#endif  // D_ENV_DB2_DETECTED


private:
    db2_connect_config m_db2_config;

    db2_connection& self()
    {
        return *this;
    }

    const db2_connection& self() const
    {
        return *this;
    }
};


// =============================================================================
// VIII. FORWARD DECLARATIONS
// =============================================================================

// db2_result_set_impl
//   struct: forward declaration of the Db2 result set implementation
// (wraps the CLI statement handle and bound column buffers).
struct db2_result_set_impl;

// db2_statement_impl
//   struct: forward declaration of the Db2 prepared statement
// implementation (wraps an SQLHSTMT handle).
struct db2_statement_impl;


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former db2_traits.hpp / db2_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// =============================================================================
// IX.   EXPRESSION DETECTORS
// =============================================================================

// -------------------------------------------------------------------------
// A.  parameterized execution
// -------------------------------------------------------------------------

// db2_exec_params_t
//   detector: exec_params(const std::string&,
// const std::vector<std::string>&) method.
// binds positional parameter markers (?) and executes.
template<typename T>
using db2_exec_params_t = decltype(std::declval<T&>().exec_params(
    std::declval<const std::string&>(),
    std::declval<const std::vector<std::string>&>()));

// -------------------------------------------------------------------------
// B.  server-side named prepare / execute
// -------------------------------------------------------------------------

// db2_prepare_named_t
//   detector: prepare_named(const std::string&, const std::string&)
// method. associates a name with a prepared statement handle.
template<typename T>
using db2_prepare_named_t = decltype(std::declval<T&>().prepare_named(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// db2_execute_named_t
//   detector: execute_named(const std::string&,
// const std::vector<std::string>&) method. executes a previously
// named prepared statement.
template<typename T>
using db2_execute_named_t = decltype(std::declval<T&>().execute_named(
    std::declval<const std::string&>(),
    std::declval<const std::vector<std::string>&>()));

// -------------------------------------------------------------------------
// C.  compound SQL
// -------------------------------------------------------------------------

// db2_begin_compound_t
//   detector: begin_compound(bool) method.
// begins a compound SQL block (atomic when the flag is set).
template<typename T>
using db2_begin_compound_t = decltype(std::declval<T&>().begin_compound(
    std::declval<bool>()));

// db2_add_compound_t
//   detector: add_compound(const std::string&) method.
// appends a statement to the pending compound block.
template<typename T>
using db2_add_compound_t = decltype(std::declval<T&>().add_compound(
    std::declval<const std::string&>()));

// db2_end_compound_t
//   detector: end_compound() method.
// dispatches the pending compound block.
template<typename T>
using db2_end_compound_t =
    decltype(std::declval<T&>().end_compound());

// -------------------------------------------------------------------------
// D.  catalog / schema introspection
// -------------------------------------------------------------------------

// db2_table_exists_t
//   detector: table_exists(const std::string&) const method.
// probes SYSCAT.TABLES.
template<typename T>
using db2_table_exists_t =
    decltype(std::declval<const T&>().table_exists(
        std::declval<const std::string&>()));

// db2_get_table_names_t
//   detector: get_table_names() const method.
// queries SYSCAT.TABLES.
template<typename T>
using db2_get_table_names_t =
    decltype(std::declval<const T&>().get_table_names());

// db2_get_schema_names_t
//   detector: get_schema_names() const method.
// queries SYSCAT.SCHEMATA.
template<typename T>
using db2_get_schema_names_t =
    decltype(std::declval<const T&>().get_schema_names());

// -------------------------------------------------------------------------
// E.  escaping
// -------------------------------------------------------------------------

// db2_escape_literal_t
//   detector: escape_literal(const std::string&) const method.
template<typename T>
using db2_escape_literal_t =
    decltype(std::declval<const T&>().escape_literal(
        std::declval<const std::string&>()));

// db2_escape_identifier_t
//   detector: escape_identifier(const std::string&) const method.
template<typename T>
using db2_escape_identifier_t =
    decltype(std::declval<const T&>().escape_identifier(
        std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// F.  diagnostics
// -------------------------------------------------------------------------

// db2_sqlcode_t
//   detector: get_sqlcode() const method.
// returns the SQLCODE of the most recent statement.
template<typename T>
using db2_sqlcode_t =
    decltype(std::declval<const T&>().get_sqlcode());

// db2_sqlstate_t
//   detector: get_sqlstate() const method.
// returns the five-character SQLSTATE of the most recent statement.
template<typename T>
using db2_sqlstate_t =
    decltype(std::declval<const T&>().get_sqlstate());

// db2_get_warning_t
//   detector: get_warning() const method.
// returns the most recent diagnostic warning text.
template<typename T>
using db2_get_warning_t =
    decltype(std::declval<const T&>().get_warning());

// -------------------------------------------------------------------------
// G.  LOB streaming
// -------------------------------------------------------------------------

// db2_lob_read_t
//   detector: lob_read(const std::string&, std::size_t) method.
template<typename T>
using db2_lob_read_t = decltype(std::declval<T&>().lob_read(
    std::declval<const std::string&>(),
    std::declval<std::size_t>()));

// db2_lob_write_t
//   detector: lob_write(const std::string&, const char*, std::size_t)
// method.
template<typename T>
using db2_lob_write_t = decltype(std::declval<T&>().lob_write(
    std::declval<const std::string&>(),
    std::declval<const char*>(),
    std::declval<std::size_t>()));

// -------------------------------------------------------------------------
// H.  array / row-set bulk insert
// -------------------------------------------------------------------------

// db2_array_insert_t
//   detector: array_insert(const std::string&,
// const std::vector<std::vector<std::string>>&) method.
// performs a row-set bound INSERT.
template<typename T>
using db2_array_insert_t = decltype(std::declval<T&>().array_insert(
    std::declval<const std::string&>(),
    std::declval<const std::vector<std::vector<std::string>>&>()));

// -------------------------------------------------------------------------
// I.  XML operations
// -------------------------------------------------------------------------

// db2_xml_insert_t
//   detector: xml_insert(const std::string&, const std::string&)
// method. inserts an XML document into an XML column.
template<typename T>
using db2_xml_insert_t = decltype(std::declval<T&>().xml_insert(
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// db2_xquery_t
//   detector: xquery(const std::string&) const method.
// executes a native XQuery expression.
template<typename T>
using db2_xquery_t = decltype(std::declval<const T&>().xquery(
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// J.  MERGE upsert
// -------------------------------------------------------------------------

// db2_merge_t
//   detector: merge(const std::string&) method.
// executes a MERGE INTO ... statement.
template<typename T>
using db2_merge_t = decltype(std::declval<T&>().merge(
    std::declval<const std::string&>()));


// =============================================================================
// X.  TAGGED CAPABILITY TRAITS (struct-based)
// =============================================================================

// has_db2_params
//   trait: checks if type T supports parameterized execution
// (exec_params).
template<typename T>
struct has_db2_params : djinterp::conjunction<
    is_detected<db2_exec_params_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_params_v = has_db2_params<clean_t<T>>::value;
#endif

// has_db2_named_statements
//   trait: checks if type T supports server-side named prepared
// statements (prepare_named + execute_named).
template<typename T>
struct has_db2_named_statements : djinterp::conjunction<
    is_detected<db2_prepare_named_t, clean_t<T>>,
    is_detected<db2_execute_named_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_named_statements_v =
        has_db2_named_statements<clean_t<T>>::value;
#endif

// has_db2_compound
//   trait: checks if type T supports compound SQL blocks
// (begin_compound + add_compound + end_compound).
template<typename T>
struct has_db2_compound : djinterp::conjunction<
    is_detected<db2_begin_compound_t, clean_t<T>>,
    is_detected<db2_add_compound_t, clean_t<T>>,
    is_detected<db2_end_compound_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_compound_v = has_db2_compound<clean_t<T>>::value;
#endif

// has_db2_schema_query
//   trait: checks if type T supports catalog introspection
// (table_exists + get_table_names + get_schema_names).
template<typename T>
struct has_db2_schema_query : djinterp::conjunction<
    is_detected<db2_table_exists_t, clean_t<T>>,
    is_detected<db2_get_table_names_t, clean_t<T>>,
    is_detected<db2_get_schema_names_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_schema_query_v =
        has_db2_schema_query<clean_t<T>>::value;
#endif

// has_db2_escape
//   trait: checks if type T supports Db2 escaping
// (escape_literal + escape_identifier).
template<typename T>
struct has_db2_escape : djinterp::conjunction<
    is_detected<db2_escape_literal_t, clean_t<T>>,
    is_detected<db2_escape_identifier_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_escape_v = has_db2_escape<clean_t<T>>::value;
#endif

// has_db2_diagnostics
//   trait: checks if type T supports Db2 diagnostics
// (get_sqlcode + get_sqlstate + get_warning).
template<typename T>
struct has_db2_diagnostics : djinterp::conjunction<
    is_detected<db2_sqlcode_t, clean_t<T>>,
    is_detected<db2_sqlstate_t, clean_t<T>>,
    is_detected<db2_get_warning_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_diagnostics_v =
        has_db2_diagnostics<clean_t<T>>::value;
#endif

// has_db2_lob
//   trait: checks if type T supports LOB streaming
// (lob_read + lob_write).
template<typename T>
struct has_db2_lob : djinterp::conjunction<
    is_detected<db2_lob_read_t, clean_t<T>>,
    is_detected<db2_lob_write_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_lob_v = has_db2_lob<clean_t<T>>::value;
#endif

// has_db2_array_insert
//   trait: checks if type T supports row-set bulk insert
// (array_insert).
template<typename T>
struct has_db2_array_insert : djinterp::conjunction<
    is_detected<db2_array_insert_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_array_insert_v =
        has_db2_array_insert<clean_t<T>>::value;
#endif

// has_db2_xml
//   trait: checks if type T supports native XML operations
// (xml_insert + xquery).
template<typename T>
struct has_db2_xml : djinterp::conjunction<
    is_detected<db2_xml_insert_t, clean_t<T>>,
    is_detected<db2_xquery_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_xml_v = has_db2_xml<clean_t<T>>::value;
#endif

// has_db2_merge
//   trait: checks if type T supports MERGE upsert (merge).
template<typename T>
struct has_db2_merge : djinterp::conjunction<
    is_detected<db2_merge_t, clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool has_db2_merge_v = has_db2_merge<clean_t<T>>::value;
#endif

// is_db2_connection
//   trait: compound trait verifying type T implements an IBM Db2
// connection interface (generic connection + diagnostics + escape +
// schema queries + parameterized execution).
template<typename T>
struct is_db2_connection : djinterp::conjunction<
    is_connection<clean_t<T>>,
    has_db2_params<clean_t<T>>,
    has_db2_diagnostics<clean_t<T>>,
    has_db2_escape<clean_t<T>>,
    has_db2_schema_query<clean_t<T>>>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename T>
    constexpr bool is_db2_connection_v = is_db2_connection<clean_t<T>>::value;
#endif


// =============================================================================
// XI. TAGLESS CAPABILITY TRAITS (constexpr bool)
// =============================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// db2_can_exec_params
//   tagless trait: true if T has exec_params().
template<typename T,
         typename = void>
constexpr bool db2_can_exec_params = false;

template<typename T>
constexpr bool db2_can_exec_params<T,
    std::void_t<db2_exec_params_t<T>>> = true;

// db2_can_prepare_named
//   tagless trait: true if T has prepare_named().
template<typename T,
         typename = void>
constexpr bool db2_can_prepare_named = false;

template<typename T>
constexpr bool db2_can_prepare_named<T,
    std::void_t<db2_prepare_named_t<T>>> = true;

// db2_can_compound
//   tagless trait: true if T has begin_compound().
template<typename T,
         typename = void>
constexpr bool db2_can_compound = false;

template<typename T>
constexpr bool db2_can_compound<T,
    std::void_t<db2_begin_compound_t<T>>> = true;

// db2_can_query_schema
//   tagless trait: true if T has table_exists().
template<typename T,
         typename = void>
constexpr bool db2_can_query_schema = false;

template<typename T>
constexpr bool db2_can_query_schema<T,
    std::void_t<db2_table_exists_t<T>>> = true;

// db2_can_escape_literal
//   tagless trait: true if T has escape_literal().
template<typename T,
         typename = void>
constexpr bool db2_can_escape_literal = false;

template<typename T>
constexpr bool db2_can_escape_literal<T,
    std::void_t<db2_escape_literal_t<T>>> = true;

// db2_can_diagnose
//   tagless trait: true if T has get_sqlcode().
template<typename T,
         typename = void>
constexpr bool db2_can_diagnose = false;

template<typename T>
constexpr bool db2_can_diagnose<T,
    std::void_t<db2_sqlcode_t<T>>> = true;

// db2_can_stream_lob
//   tagless trait: true if T has lob_read().
template<typename T,
         typename = void>
constexpr bool db2_can_stream_lob = false;

template<typename T>
constexpr bool db2_can_stream_lob<T,
    std::void_t<db2_lob_read_t<T>>> = true;

// db2_can_array_insert
//   tagless trait: true if T has array_insert().
template<typename T,
         typename = void>
constexpr bool db2_can_array_insert = false;

template<typename T>
constexpr bool db2_can_array_insert<T,
    std::void_t<db2_array_insert_t<T>>> = true;

// db2_can_xquery
//   tagless trait: true if T has xquery().
template<typename T,
         typename = void>
constexpr bool db2_can_xquery = false;

template<typename T>
constexpr bool db2_can_xquery<T,
    std::void_t<db2_xquery_t<T>>> = true;

// db2_can_merge
//   tagless trait: true if T has merge().
template<typename T,
         typename = void>
constexpr bool db2_can_merge = false;

template<typename T>
constexpr bool db2_can_merge<T,
    std::void_t<db2_merge_t<T>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// db2_does_named_statements
//   tagless trait: true if T supports server-side named statements.
template<typename T,
         typename = void>
constexpr bool db2_does_named_statements = false;

template<typename T>
constexpr bool db2_does_named_statements<T, std::void_t<
    db2_prepare_named_t<T>,
    db2_execute_named_t<T>>> = true;

// db2_does_compound
//   tagless trait: true if T supports compound SQL blocks.
template<typename T,
         typename = void>
constexpr bool db2_does_compound = false;

template<typename T>
constexpr bool db2_does_compound<T, std::void_t<
    db2_begin_compound_t<T>,
    db2_add_compound_t<T>,
    db2_end_compound_t<T>>> = true;

// db2_does_schema_query
//   tagless trait: true if T supports the full catalog-introspection
// surface.
template<typename T,
         typename = void>
constexpr bool db2_does_schema_query = false;

template<typename T>
constexpr bool db2_does_schema_query<T, std::void_t<
    db2_table_exists_t<T>,
    db2_get_table_names_t<T>,
    db2_get_schema_names_t<T>>> = true;

// db2_does_diagnostics
//   tagless trait: true if T supports the full diagnostics surface.
template<typename T,
         typename = void>
constexpr bool db2_does_diagnostics = false;

template<typename T>
constexpr bool db2_does_diagnostics<T, std::void_t<
    db2_sqlcode_t<T>,
    db2_sqlstate_t<T>,
    db2_get_warning_t<T>>> = true;

// db2_does_lob
//   tagless trait: true if T supports the full LOB-streaming surface.
template<typename T,
         typename = void>
constexpr bool db2_does_lob = false;

template<typename T>
constexpr bool db2_does_lob<T, std::void_t<
    db2_lob_read_t<T>,
    db2_lob_write_t<T>>> = true;

// db2_does_xml
//   tagless trait: true if T supports the full native-XML surface.
template<typename T,
         typename = void>
constexpr bool db2_does_xml = false;

template<typename T>
constexpr bool db2_does_xml<T, std::void_t<
    db2_xml_insert_t<T>,
    db2_xquery_t<T>>> = true;

// db2_is_full_connection
//   tagless trait: true if T satisfies the complete IBM Db2
// connection interface.
template<typename T>
constexpr bool db2_is_full_connection =
    ( is_connectable<clean_t<T>>          &&
      db2_can_exec_params<clean_t<T>>     &&
      db2_can_diagnose<clean_t<T>>        &&
      db2_can_escape_literal<clean_t<T>>  &&
      db2_can_query_schema<clean_t<T>> );


// =============================================================================
// XII.  SFINAE HELPERS
// =============================================================================

// enable_if_db2_connection
//   type: SFINAE helper for IBM Db2 connection constraints.
template<typename T>
using enable_if_db2_connection =
    typename std::enable_if<is_db2_connection<clean_t<T>>::value>::type;

// enable_if_has_db2_compound
//   type: SFINAE helper for Db2 compound-SQL constraints.
template<typename T>
using enable_if_has_db2_compound =
    typename std::enable_if<has_db2_compound<clean_t<T>>::value>::type;

// enable_if_has_db2_lob
//   type: SFINAE helper for Db2 LOB-streaming constraints.
template<typename T>
using enable_if_has_db2_lob =
    typename std::enable_if<has_db2_lob<clean_t<T>>::value>::type;

// enable_if_has_db2_xml
//   type: SFINAE helper for Db2 native-XML constraints.
template<typename T>
using enable_if_has_db2_xml =
    typename std::enable_if<has_db2_xml<clean_t<T>>::value>::type;

// enable_if_has_db2_merge
//   type: SFINAE helper for Db2 MERGE constraints.
template<typename T>
using enable_if_has_db2_merge =
    typename std::enable_if<has_db2_merge<clean_t<T>>::value>::type;


// ===========================================================================
// XIII.   C++20 CONCEPTS
// ===========================================================================
//   The IBM Db2 classification concepts, folded in from the former
// db2_concepts.hpp.  Each forwards to a trait / tagless capability declared
// above.  Gated on concept support so the traits remain usable at the C++17
// baseline (matching functor.hpp / monoid.hpp).

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =============================================================================
// A.   Core Db2 Connection Concepts
// =============================================================================

// Db2_connection
//   concept: constrains types implementing the IBM Db2 connection
// interface. Suffixed with `_c` to avoid clashing with the
// db2_connection class type.
template<typename Type>
concept Db2_connection =
    is_db2_connection<clean_t<Type>>::value;

// non_db2_connection
//   concept: constrains types that do not implement the Db2 connection
// interface.
template<typename Type>
concept non_db2_connection =
    !Db2_connection<Type>;

// db2_params_connection
//   concept: constrains Db2 connections supporting parameterized
// execution.
template<typename Type>
concept db2_params_connection =
    has_db2_params<clean_t<Type>>::value;

// db2_named_statement_connection
//   concept: constrains Db2 connections supporting server-side named
// prepared statements.
template<typename Type>
concept db2_named_statement_connection =
    has_db2_named_statements<clean_t<Type>>::value;

// db2_compound_connection
//   concept: constrains Db2 connections supporting compound SQL blocks.
template<typename Type>
concept db2_compound_connection =
    has_db2_compound<clean_t<Type>>::value;

// db2_schema_query_connection
//   concept: constrains Db2 connections supporting catalog
// introspection (SYSCAT / SYSIBM).
template<typename Type>
concept db2_schema_query_connection =
    has_db2_schema_query<clean_t<Type>>::value;

// db2_escape_connection
//   concept: constrains Db2 connections supporting literal and
// identifier escaping.
template<typename Type>
concept db2_escape_connection =
    has_db2_escape<clean_t<Type>>::value;

// db2_diagnostics_connection
//   concept: constrains Db2 connections exposing SQLCODE / SQLSTATE
// diagnostics.
template<typename Type>
concept db2_diagnostics_connection =
    has_db2_diagnostics<clean_t<Type>>::value;

// db2_lob_connection
//   concept: constrains Db2 connections supporting LOB streaming.
template<typename Type>
concept db2_lob_connection =
    has_db2_lob<clean_t<Type>>::value;

// db2_array_insert_connection
//   concept: constrains Db2 connections supporting row-set bulk insert.
template<typename Type>
concept db2_array_insert_connection =
    has_db2_array_insert<clean_t<Type>>::value;

// db2_xml_connection
//   concept: constrains Db2 connections supporting native XML
// operations.
template<typename Type>
concept db2_xml_connection =
    has_db2_xml<clean_t<Type>>::value;

// db2_merge_connection
//   concept: constrains Db2 connections supporting MERGE upsert.
template<typename Type>
concept db2_merge_connection =
    has_db2_merge<clean_t<Type>>::value;


// =============================================================================
// B.  Db2 Capability Concepts
// =============================================================================

// db2_params_capable_connection
//   concept: constrains types exposing exec_params(query, params).
template<typename Type>
concept db2_params_capable_connection =
    db2_can_exec_params<clean_t<Type>>;

// db2_named_preparable_connection
//   concept: constrains types exposing prepare_named(name, sql).
template<typename Type>
concept db2_named_preparable_connection =
    db2_can_prepare_named<clean_t<Type>>;

// db2_compound_capable_connection
//   concept: constrains types exposing begin_compound(atomic).
template<typename Type>
concept db2_compound_capable_connection =
    db2_can_compound<clean_t<Type>>;

// db2_schema_queryable_connection
//   concept: constrains types exposing table_exists(name).
template<typename Type>
concept db2_schema_queryable_connection =
    db2_can_query_schema<clean_t<Type>>;

// db2_escapable_connection
//   concept: constrains types exposing escape_literal(input).
template<typename Type>
concept db2_escapable_connection =
    db2_can_escape_literal<clean_t<Type>>;

// db2_diagnosable_connection
//   concept: constrains types exposing get_sqlcode().
template<typename Type>
concept db2_diagnosable_connection =
    db2_can_diagnose<clean_t<Type>>;

// db2_lob_streamable_connection
//   concept: constrains types exposing lob_read(locator, length).
template<typename Type>
concept db2_lob_streamable_connection =
    db2_can_stream_lob<clean_t<Type>>;

// db2_array_insertable_connection
//   concept: constrains types exposing array_insert(sql, rows).
template<typename Type>
concept db2_array_insertable_connection =
    db2_can_array_insert<clean_t<Type>>;

// db2_xquery_capable_connection
//   concept: constrains types exposing xquery(expr).
template<typename Type>
concept db2_xquery_capable_connection =
    db2_can_xquery<clean_t<Type>>;

// db2_mergeable_connection
//   concept: constrains types exposing merge(sql).
template<typename Type>
concept db2_mergeable_connection =
    db2_can_merge<clean_t<Type>>;


// =============================================================================
// C. Tagless Db2 Capability Concepts
// =============================================================================

// db2_named_statement_full
//   concept: constrains types satisfying the full tagless named-
// statement capability set.
template<typename Type>
concept db2_named_statement_full =
    db2_does_named_statements<clean_t<Type>>;

// db2_compound_full
//   concept: constrains types satisfying the full tagless compound-SQL
// capability set.
template<typename Type>
concept db2_compound_full =
    db2_does_compound<clean_t<Type>>;

// db2_schema_queryable
//   concept: constrains types satisfying the full tagless catalog-
// introspection capability set.
template<typename Type>
concept db2_schema_queryable =
    db2_does_schema_query<clean_t<Type>>;

// db2_diagnosable
//   concept: constrains types satisfying the full tagless diagnostics
// capability set.
template<typename Type>
concept db2_diagnosable =
    db2_does_diagnostics<clean_t<Type>>;

// db2_lob_streamable
//   concept: constrains types satisfying the full tagless LOB-streaming
// capability set.
template<typename Type>
concept db2_lob_streamable =
    db2_does_lob<clean_t<Type>>;

// db2_xml_capable
//   concept: constrains types satisfying the full tagless native-XML
// capability set.
template<typename Type>
concept db2_xml_capable =
    db2_does_xml<clean_t<Type>>;

// db2_full_connection
//   concept: constrains types satisfying the complete tagless Db2
// connection capability set.
template<typename Type>
concept db2_full_connection =
    db2_is_full_connection<clean_t<Type>>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DB2_DB2_HPP
