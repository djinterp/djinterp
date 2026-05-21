/******************************************************************************
* djinterp [database]                         database_connection_template.hpp
* 
* djinterp database connection template module:
*   This header provides the vendor-parameterized connection template that
* serves as the bridge between the generic CRTP connection base in
* database.hpp and concrete vendor-specific implementations. It
* includes:
*   - vendor_traits specializations for all supported database vendors
*   - connection_template CRTP base with vendor-aware defaults
*   - native handle management scaffolding
*   - default configuration factories per vendor
*
*   Vendor modules (e.g. mysql_connection, pg_connection) derive from
* connection_template<_helper, database_type::mysql> rather than from
* connection<_helper> directly. This automatically wires in the correct
* native handle type, default port, vendor metadata, and other
* vendor-specific configuration.
*
*   VENDOR SPECIALIZATION:
*   Each vendor_traits<database_type::X> specialization declares:
*   - native_handle_type:  the vendor's C API handle type
*   - default_port:        standard TCP port for the vendor
*   - name / display_name: vendor identification strings
*   - is_relational:       true for RDBMS vendors
*   - is_embedded:         true for embedded databases (SQLite)
*   - supports_ssl:        true if SSL/TLS is available
*   - get_info():          returns a populated vendor_info struct
*   - make_default_config(): returns vendor-appropriate connection_config
*
*   PORTABILITY:
*   This header requires C++17 or later. Vendor-specific C API headers
* are NOT included here; concrete vendor connection implementations
* include those headers themselves. The native_handle_type is forward-
* declared as void* when the vendor header is not present.
*
* 
* path:      /inc/djinterp/core/db/database_connection_template.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_CONNECTION_TEMPLATE_
#define DJINTERP_DATABASE_CONNECTION_TEMPLATE_

// std
#include <type_traits>
// djinterp
#include "./database.hpp"


NS_DJINTERP

// ===========================================================================
// I.   VENDOR TRAITS SPECIALIZATIONS
// ===========================================================================
// Each specialization provides compile-time metadata for a specific
// database vendor. The primary template in database.hpp provides
// safe defaults for unknown vendors.
//
// Native handle types are declared as void* here because this header
// does not include vendor-specific C API headers. Concrete vendor
// connection implementations reinterpret_cast to the actual handle
// type (e.g. MYSQL*, PGconn*, sqlite3*) after including the
// appropriate vendor header.


// -------------------------------------------------------------------------
// vendor_traits<database_type::mariadb>
//   trait: vendor configuration for MariaDB connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::mariadb>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::mariadb;
    static constexpr std::uint16_t default_port   = 3306;
    static constexpr const char*   name           = "mariadb";
    static constexpr const char*   display_name   = "MariaDB";
    static constexpr bool          is_relational  = true;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host    = "localhost";
        config.port    = default_port;
        config.charset = "utf8mb4";

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::mysql>
//   trait: vendor configuration for Oracle MySQL connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::mysql>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::mysql;
    static constexpr std::uint16_t default_port   = 3306;
    static constexpr const char*   name           = "mysql";
    static constexpr const char*   display_name   = "MySQL";
    static constexpr bool          is_relational  = true;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host    = "localhost";
        config.port    = default_port;
        config.charset = "utf8mb4";

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::postgresql>
//   trait: vendor configuration for PostgreSQL connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::postgresql>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::postgresql;
    static constexpr std::uint16_t default_port   = 5432;
    static constexpr const char*   name           = "postgresql";
    static constexpr const char*   display_name   = "PostgreSQL";
    static constexpr bool          is_relational  = true;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host   = "localhost";
        config.port   = default_port;
        config.schema = "public";

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::sqlite>
//   trait: vendor configuration for SQLite connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::sqlite>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::sqlite;
    static constexpr std::uint16_t default_port   = 0;
    static constexpr const char*   name           = "sqlite";
    static constexpr const char*   display_name   = "SQLite";
    static constexpr bool          is_relational  = true;
    static constexpr bool          is_embedded    = true;
    static constexpr bool          supports_ssl   = false;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        // SQLite uses the database field as the file path
        config.host = "";
        config.port = 0;

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::mongodb>
//   trait: vendor configuration for MongoDB connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::mongodb>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::mongodb;
    static constexpr std::uint16_t default_port   = 27017;
    static constexpr const char*   name           = "mongodb";
    static constexpr const char*   display_name   = "MongoDB";
    static constexpr bool          is_relational  = false;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host = "localhost";
        config.port = default_port;

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::arangodb>
//   trait: vendor configuration for ArangoDB connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::arangodb>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::arangodb;
    static constexpr std::uint16_t default_port   = 8529;
    static constexpr const char*   name           = "arangodb";
    static constexpr const char*   display_name   = "ArangoDB";
    static constexpr bool          is_relational  = false;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host     = "localhost";
        config.port     = default_port;
        config.database = "_system";

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::oracle>
//   trait: vendor configuration for Oracle Database connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::oracle>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::oracle;
    static constexpr std::uint16_t default_port   = 1521;
    static constexpr const char*   name           = "oracle";
    static constexpr const char*   display_name   = "Oracle Database";
    static constexpr bool          is_relational  = true;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host = "localhost";
        config.port = default_port;

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::redis>
//   trait: vendor configuration for Redis connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::redis>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::redis;
    static constexpr std::uint16_t default_port   = 6379;
    static constexpr const char*   name           = "redis";
    static constexpr const char*   display_name   = "Redis";
    static constexpr bool          is_relational  = false;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host     = "localhost";
        config.port     = default_port;
        config.database = "0";

        return config;
    }
};

// -------------------------------------------------------------------------
// vendor_traits<database_type::mssql>
//   trait: vendor configuration for Microsoft SQL Server connections.
// -------------------------------------------------------------------------
template<>
struct vendor_traits<database_type::mssql>
{
    using native_handle_type = void*;

    static constexpr database_type db_type        = database_type::mssql;
    static constexpr std::uint16_t default_port   = 1433;
    static constexpr const char*   name           = "mssql";
    static constexpr const char*   display_name   = "Microsoft SQL Server";
    static constexpr bool          is_relational  = true;
    static constexpr bool          is_embedded    = false;
    static constexpr bool          supports_ssl   = true;

    static vendor_info get_info()
    {
        vendor_info info;

        info.type          = db_type;
        info.name          = name;
        info.display_name  = display_name;
        info.default_port  = default_port;
        info.is_relational = is_relational;
        info.is_embedded   = is_embedded;
        info.supports_ssl  = supports_ssl;

        return info;
    }

    static connection_config make_default_config()
    {
        connection_config config;

        config.host   = "localhost";
        config.port   = default_port;
        config.schema = "dbo";

        return config;
    }
};


// ===========================================================================
// II.  CONNECTION TEMPLATE
// ===========================================================================

// connection_template
//   class template: vendor-parameterized CRTP base for database connections.
// Extends the generic connection<_helper> base with vendor-specific defaults,
// native handle scaffolding, and vendor metadata. Concrete vendor
// implementations derive from this template rather than from
// connection<_helper> directly.
//
// Template parameters:
//   _helper:   the concrete CRTP implementation class
//   _DbType: the database_type enumerator identifying the vendor
//
// Example usage (vendor implementation):
//   class mysql_connection_helper
//       : public connection_template<mysql_connection_helper,
//                                    database_type::mysql>
//   { ... };
template<typename      _helper,
         database_type _DbType>
class connection_template : public connection<_helper>
{
public:
    // vendor type aliases
    using traits_type        = vendor_traits<_DbType>;
    using native_handle_type = typename traits_type::native_handle_type;
    using base_type          = connection<_helper>;

    // db_type
    //   value: the database_type enumerator for this connection.
    static constexpr database_type db_type = _DbType;

    connection_template()
        : base_type(),
          m_native_handle(native_handle_type{})
    {
        // apply vendor defaults to base config
        this->m_config = traits_type::make_default_config();
    }

    explicit connection_template(const connection_config& _config)
        : base_type()
        , m_native_handle(native_handle_type{})
    {
        this->m_config = _config;

        // apply vendor default port if not specified
        if (this->m_config.port == 0)
        {
            this->m_config.port = traits_type::default_port;
        }
    }

    ~connection_template() = default;

    // disable copying
    connection_template(const connection_template&)            = delete;
    connection_template& operator=(const connection_template&) = delete;

    // enable moving
    connection_template(connection_template&& _other) noexcept
        : base_type(std::move(_other))
        , m_native_handle(_other.m_native_handle)
    {
        _other.m_native_handle = native_handle_type{};
    }

    connection_template& operator=(connection_template&& _other) noexcept
    {
        if (this != &_other)
        {
            base_type::operator=(std::move(_other));

            m_native_handle        = _other.m_native_handle;
            _other.m_native_handle = native_handle_type{};
        }

        return *this;
    }

    // -----------------------------------------------------------------
    // vendor metadata (non-virtual, compile-time resolved)
    // -----------------------------------------------------------------

    // get_database_type
    //   function: returns the vendor's database_type enumerator.
    database_type get_database_type() const noexcept
    {
        return db_type;
    }

    // get_vendor_name
    //   function: returns the vendor's short identifier string.
    const char* get_vendor_name() const noexcept
    {
        return traits_type::name;
    }

    // get_vendor_display_name
    //   function: returns the vendor's human-readable display name.
    const char* get_vendor_display_name() const noexcept
    {
        return traits_type::display_name;
    }

    // get_vendor_info
    //   function: returns a populated vendor_info struct.
    vendor_info get_vendor_info() const
    {
        return traits_type::get_info();
    }

    // get_default_port
    //   function: returns the vendor's default TCP port.
    std::uint16_t get_default_port() const noexcept
    {
        return traits_type::default_port;
    }

    // -----------------------------------------------------------------
    // native handle access
    // -----------------------------------------------------------------

    // get_native_handle
    //   function: returns the vendor's native connection handle.
    // The returned pointer should be reinterpret_cast to the actual
    // vendor handle type (e.g. MYSQL*, PGconn*) by vendor-specific
    // code.
    native_handle_type get_native_handle() noexcept
    {
        return m_native_handle;
    }

    // get_native_handle (const)
    //   function: returns the vendor's native connection handle (const).
    native_handle_type get_native_handle() const noexcept
    {
        return m_native_handle;
    }

    // -----------------------------------------------------------------
    // compile-time vendor queries
    // -----------------------------------------------------------------

    // is_relational
    //   function: returns true if this is a relational database.
    static constexpr bool is_relational() noexcept
    {
        return traits_type::is_relational;
    }

    // is_embedded
    //   function: returns true if this is an embedded database.
    static constexpr bool is_embedded() noexcept
    {
        return traits_type::is_embedded;
    }

    // vendor_supports_ssl
    //   function: returns true if the vendor supports SSL/TLS.
    static constexpr bool vendor_supports_ssl() noexcept
    {
        return traits_type::supports_ssl;
    }

protected:
    native_handle_type m_native_handle;
};


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_CONNECTION_TEMPLATE_