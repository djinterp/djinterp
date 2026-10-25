/*******************************************************************************
* djinterp [core]                               database_connection_template.hpp
*
* Each vendor's compile-time identity, and the connection base that carries
* it.
*   vendor_traits<database_type::X> names a vendor's native handle type,
* default port, names and defaults; connection_template<Derived, X> is the
* CRTP base that gives a connection those traits: the vendor's metadata,
* resolved at compile time, its default configuration, and its native handle,
* which the vendor's connection lends with get_native_handle(). A vendor's
* connection derives from it through database_connection.
*   The handles are the C layers' own -- an SQLite connection is SQLite's
* struct sqlite3, a MariaDB or MySQL one the layer's struct d_mariadb, whose
* MYSQL* d_mariadb_native lends, a PostgreSQL one libpq's struct pg_conn --
* declared here without the clients' headers, which this header does not
* include. A vendor no layer stands behind yet keeps a void*.
*
*
* path:      /inc/djinterp/core/db/database_connection_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.20
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VENDOR TRAITS
    -------------
    1.  Shared
         1.  internal::vendor_traits_info
    2.  Vendors
         1.  vendor_traits<database_type::mariadb>
         2.  vendor_traits<database_type::mysql>
         3.  vendor_traits<database_type::postgresql>
         4.  vendor_traits<database_type::sqlite>
         5.  vendor_traits<database_type::mongodb>
         6.  vendor_traits<database_type::arangodb>
         7.  vendor_traits<database_type::oracle>
         8.  vendor_traits<database_type::redis>
         9.  vendor_traits<database_type::mssql>
2.  CONNECTION TEMPLATE
    -------------------
    1.  The template
         1.  connection_template
*/

#ifndef DJINTERP_DB_DATABASE_CONNECTION_TEMPLATE_HPP
#define DJINTERP_DB_DATABASE_CONNECTION_TEMPLATE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"  // framework root
#include "./database.hpp"      // vendor_traits, connection, connection_config
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint16_t


// the C layers' connection handles, as their headers declare them: SQLite's
// own, the MariaDB layer's, libpq's PGconn
struct sqlite3;
struct d_mariadb;
struct pg_conn;


NS_DJINTERP


//==============================================================================
// 1.  VENDOR TRAITS
//==============================================================================
// The primary template, in database.hpp, describes an unknown vendor; each
// specialization here describes one, for connection_template to carry.


// 1.1    Shared
//------------------------------------------------------------------------------
// 1.1.1
// internal::vendor_traits_info
//   trait: get_info() for a vendor_traits specialization Traits, read from
// its constants, which every specialization would otherwise restate.
NS_INTERNAL

    template<typename Traits>
    struct vendor_traits_info
    {
        static vendor_info
        get_info()
        {
            vendor_info info;

            info.type          = Traits::db_type;
            info.name          = Traits::name;
            info.display_name  = Traits::display_name;
            info.default_port  = Traits::default_port;
            info.is_relational = Traits::is_relational;
            info.is_embedded   = Traits::is_embedded;
            info.supports_ssl  = Traits::supports_ssl;

            return info;
        }
    };

NS_END  // internal

// 1.2    Vendors
//------------------------------------------------------------------------------
// 1.2.1
// vendor_traits<database_type::mariadb>
//   trait: MariaDB, through the MariaDB layer's connection.
template<>
struct vendor_traits<database_type::mariadb>
    : internal::vendor_traits_info<vendor_traits<database_type::mariadb>>
{
    using native_handle_type = struct ::d_mariadb*;

    static constexpr database_type    db_type       = database_type::mariadb;
    static constexpr re_std::uint16_t default_port  = 3306;
    static constexpr const char*      name          = "mariadb";
    static constexpr const char*      display_name  = "MariaDB";
    static constexpr bool             is_relational = true;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host    = "localhost";
        config.port    = default_port;
        config.charset = "utf8mb4";

        return config;
    }
};

// 1.2.2
// vendor_traits<database_type::mysql>
//   trait: Oracle's MySQL, through the MariaDB layer's connection, which
// serves both servers.
template<>
struct vendor_traits<database_type::mysql>
    : internal::vendor_traits_info<vendor_traits<database_type::mysql>>
{
    using native_handle_type = struct ::d_mariadb*;

    static constexpr database_type    db_type       = database_type::mysql;
    static constexpr re_std::uint16_t default_port  = 3306;
    static constexpr const char*      name          = "mysql";
    static constexpr const char*      display_name  = "MySQL";
    static constexpr bool             is_relational = true;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host    = "localhost";
        config.port    = default_port;
        config.charset = "utf8mb4";

        return config;
    }
};

// 1.2.3
// vendor_traits<database_type::postgresql>
//   trait: PostgreSQL, through libpq's own connection.
template<>
struct vendor_traits<database_type::postgresql>
    : internal::vendor_traits_info<vendor_traits<database_type::postgresql>>
{
    using native_handle_type = struct ::pg_conn*;

    static constexpr database_type    db_type       = database_type::postgresql;
    static constexpr re_std::uint16_t default_port  = 5432;
    static constexpr const char*      name          = "postgresql";
    static constexpr const char*      display_name  = "PostgreSQL";
    static constexpr bool             is_relational = true;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host   = "localhost";
        config.port   = default_port;
        config.schema = "public";

        return config;
    }
};

// 1.2.4
// vendor_traits<database_type::sqlite>
//   trait: SQLite, through SQLite's own connection; the configuration's
// `database` is the file.
template<>
struct vendor_traits<database_type::sqlite>
    : internal::vendor_traits_info<vendor_traits<database_type::sqlite>>
{
    using native_handle_type = struct ::sqlite3*;

    static constexpr database_type    db_type       = database_type::sqlite;
    static constexpr re_std::uint16_t default_port  = 0;
    static constexpr const char*      name          = "sqlite";
    static constexpr const char*      display_name  = "SQLite";
    static constexpr bool             is_relational = true;
    static constexpr bool             is_embedded   = true;
    static constexpr bool             supports_ssl  = false;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host = "";
        config.port = 0;

        return config;
    }
};

// 1.2.5
// vendor_traits<database_type::mongodb>
//   trait: MongoDB; no layer stands behind it yet.
template<>
struct vendor_traits<database_type::mongodb>
    : internal::vendor_traits_info<vendor_traits<database_type::mongodb>>
{
    using native_handle_type = void*;

    static constexpr database_type    db_type       = database_type::mongodb;
    static constexpr re_std::uint16_t default_port  = 27017;
    static constexpr const char*      name          = "mongodb";
    static constexpr const char*      display_name  = "MongoDB";
    static constexpr bool             is_relational = false;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host = "localhost";
        config.port = default_port;

        return config;
    }
};

// 1.2.6
// vendor_traits<database_type::arangodb>
//   trait: ArangoDB; no layer stands behind it yet.
template<>
struct vendor_traits<database_type::arangodb>
    : internal::vendor_traits_info<vendor_traits<database_type::arangodb>>
{
    using native_handle_type = void*;

    static constexpr database_type    db_type       = database_type::arangodb;
    static constexpr re_std::uint16_t default_port  = 8529;
    static constexpr const char*      name          = "arangodb";
    static constexpr const char*      display_name  = "ArangoDB";
    static constexpr bool             is_relational = false;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host     = "localhost";
        config.port     = default_port;
        config.database = "_system";

        return config;
    }
};

// 1.2.7
// vendor_traits<database_type::oracle>
//   trait: Oracle Database; no layer stands behind it yet.
template<>
struct vendor_traits<database_type::oracle>
    : internal::vendor_traits_info<vendor_traits<database_type::oracle>>
{
    using native_handle_type = void*;

    static constexpr database_type    db_type       = database_type::oracle;
    static constexpr re_std::uint16_t default_port  = 1521;
    static constexpr const char*      name          = "oracle";
    static constexpr const char*      display_name  = "Oracle Database";
    static constexpr bool             is_relational = true;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host = "localhost";
        config.port = default_port;

        return config;
    }
};

// 1.2.8
// vendor_traits<database_type::redis>
//   trait: Redis; no layer stands behind it yet.
template<>
struct vendor_traits<database_type::redis>
    : internal::vendor_traits_info<vendor_traits<database_type::redis>>
{
    using native_handle_type = void*;

    static constexpr database_type    db_type       = database_type::redis;
    static constexpr re_std::uint16_t default_port  = 6379;
    static constexpr const char*      name          = "redis";
    static constexpr const char*      display_name  = "Redis";
    static constexpr bool             is_relational = false;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host     = "localhost";
        config.port     = default_port;
        config.database = "0";

        return config;
    }
};

// 1.2.9
// vendor_traits<database_type::mssql>
//   trait: Microsoft SQL Server; reserved, as database_type says.
template<>
struct vendor_traits<database_type::mssql>
    : internal::vendor_traits_info<vendor_traits<database_type::mssql>>
{
    using native_handle_type = void*;

    static constexpr database_type    db_type       = database_type::mssql;
    static constexpr re_std::uint16_t default_port  = 1433;
    static constexpr const char*      name          = "mssql";
    static constexpr const char*      display_name  = "Microsoft SQL Server";
    static constexpr bool             is_relational = true;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = true;

    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host   = "localhost";
        config.port   = default_port;
        config.schema = "dbo";

        return config;
    }
};


//==============================================================================
// 2.  CONNECTION TEMPLATE
//==============================================================================


// 2.1    The template
//------------------------------------------------------------------------------
// 2.1.1
// connection_template
//   class template: connection<Derived> with a vendor's traits: its
// metadata at compile time, its default configuration, and its native
// handle. Derived lends the handle through native_handle_impl(), which
// returns native_handle_type, and is needed only where get_native_handle()
// is called. Made with no configuration, a connection has the vendor's
// defaults; made with one, the vendor's port where it names none. Not
// copyable; movable.
template<typename      Derived,
         database_type DbType>
class connection_template : public connection<Derived>
{
public:
    using traits_type        = vendor_traits<DbType>;
    using native_handle_type = typename traits_type::native_handle_type;
    using base_type          = connection<Derived>;

    // db_type
    //   constant: the vendor this connection serves.
    static constexpr database_type db_type = DbType;

    // vendor metadata -- resolved at compile time
    database_type
    get_database_type() const noexcept
    {
        return db_type;
    }

    const char*
    get_vendor_name() const noexcept
    {
        return traits_type::name;
    }

    const char*
    get_vendor_display_name() const noexcept
    {
        return traits_type::display_name;
    }

    vendor_info
    get_vendor_info() const
    {
        return traits_type::get_info();
    }

    re_std::uint16_t
    get_default_port() const noexcept
    {
        return traits_type::default_port;
    }

    /**
     * @brief Lends the connection's native handle, for calls of the C layer
     *        or of the client itself.
     *
     * @return the handle, or `NULL` while the connection is closed; it stays
     *         the connection's, and is invalid once the connection closes.
     */
    native_handle_type
    get_native_handle() const noexcept
    {
        return static_cast<const Derived&>(*this).native_handle_impl();
    }

    // compile-time vendor queries
    static constexpr bool
    is_relational() noexcept
    {
        return traits_type::is_relational;
    }

    static constexpr bool
    is_embedded() noexcept
    {
        return traits_type::is_embedded;
    }

    static constexpr bool
    vendor_supports_ssl() noexcept
    {
        return traits_type::supports_ssl;
    }

protected:
    connection_template()
        : base_type()
    {
        this->m_config = traits_type::make_default_config();
    }

    explicit connection_template(
        const connection_config& _config
    )
        : base_type()
    {
        this->m_config = _config;

        // the vendor's port where the configuration names none
        if (this->m_config.port == 0)
        {
            this->m_config.port = traits_type::default_port;
        }
    }

    ~connection_template() = default;

    connection_template(const connection_template&)            = delete;
    connection_template& operator=(const connection_template&) = delete;

    connection_template(connection_template&&) noexcept            = default;
    connection_template& operator=(connection_template&&) noexcept = default;
};


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_CONNECTION_TEMPLATE_HPP
