/******************************************************************************
* djinterp [database]                                       mysql_common.hpp
* 
* djinterp MySQL-family common connection module:
*   This header provides the shared CRTP connection base for the MySQL-
* compatible database family (Oracle MySQL and MariaDB). It sits between
* the generic database_connection template and the concrete vendor-specific
* implementations (mysql_connection, mariadb_connection), providing:
*   - character set management (set_charset, get_charset)
*   - multi-result set iteration (next_result, more_results)
*   - result set mode selection (store vs stream)
*   - MySQL-specific options API wrapper
*   - server diagnostics (stat, thread_id, warning_count, sqlstate)
*   - MySQL-style string escaping
*   - database selection (select_db)
*   - user switching (change_user)
*   - client flags and connect attributes
*   - compile-time gating via D_ENV_MYSQL_COMMON_* macros
*
*   The MySQL-family C API (libmysqlclient / MariaDB Connector/C) uses
* the MYSQL* opaque handle type, shared by both products. This module
* declares the interface methods that wrap the shared C API surface;
* actual C API calls are left to the vendor-specific _impl methods in
* the final CRTP leaf class.
*
*   LAYER DIAGRAM:
*     mysql_connection_impl (vendor-specific)
*       -> mysql_common_connection<mysql_connection_impl, database_type::mysql>
*         -> database_connection<mysql_connection_impl, database_type::mysql>
*           -> connection_template<mysql_connection_impl, database_type::mysql>
*             -> connection<mysql_connection_impl>
*
*   The _Impl class must provide the following MySQL-specific methods
* (in addition to those required by database_connection):
*   - void        set_charset_impl(const std::string&)
*   - std::string get_charset_impl() const
*   - int         next_result_impl()
*   - bool        more_results_impl() const
*   - std::string escape_string_impl(const std::string&) const
*   - std::string get_stat_impl() const
*   - unsigned long get_thread_id_impl() const
*   - unsigned int  get_warning_count_impl() const
*   - std::string   get_sqlstate_impl() const
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include the MySQL
* C API headers; concrete implementations include <mysql.h> or
* <mariadb/mysql.h> themselves.
*
* path:      \inc\database\mysql\mysql_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.06.15
******************************************************************************/

#ifndef DJINTERP_DATABASE_MYSQL_COMMON_
#define DJINTERP_DATABASE_MYSQL_COMMON_

#include "..\database_connection.hpp"
#include "mysql_common_traits.hpp"

// env_mysql_common.h provides D_ENV_MYSQL_COMMON_* compile-time
// feature macros. It is pulled in transitively through env_db.h
// -> database_traits.hpp if a MySQL-family client library is
// detected. Include it explicitly to guarantee availability.
#include "..\..\core\env\env_mysql_common.h"


NS_DJINTERP
NS_DB
NS_MYSQL_COMMON


// =============================================================================
// I.   MYSQL-FAMILY CLIENT FLAGS
// =============================================================================
// Portable compile-time constants for commonly used MySQL client flags.
// The actual CLIENT_* macros come from the MySQL C API header; these
// provide named constants usable without including <mysql.h>.

// mysql_client_flag
//   enumeration: portable client flags for mysql_real_connect().
// values mirror the CLIENT_* macros from the MySQL wire protocol.
enum class mysql_client_flag : unsigned long
{
    none               = 0x00000000,
    long_password      = 0x00000001,
    found_rows         = 0x00000002,
    long_flag          = 0x00000004,
    connect_with_db    = 0x00000008,
    no_schema          = 0x00000010,
    compress           = 0x00000020,
    local_files        = 0x00000080,
    ignore_space       = 0x00000100,
    protocol_41        = 0x00000200,
    interactive        = 0x00000400,
    ssl                = 0x00000800,
    transactions       = 0x00002000,
    secure_connection  = 0x00008000,
    multi_statements   = 0x00010000,
    multi_results      = 0x00020000,
    ps_multi_results   = 0x00040000,
    plugin_auth        = 0x00080000,
    remember_options   = 0x80000000
};

// operator|
//   function: bitwise OR for combining mysql_client_flag values.
inline constexpr mysql_client_flag operator|(mysql_client_flag _a,
                                             mysql_client_flag _b)
{
    return static_cast<mysql_client_flag>(
        static_cast<unsigned long>(_a) |
        static_cast<unsigned long>(_b));
}

// operator&
//   function: bitwise AND for testing mysql_client_flag values.
inline constexpr mysql_client_flag operator&(mysql_client_flag _a,
                                             mysql_client_flag _b)
{
    return static_cast<mysql_client_flag>(
        static_cast<unsigned long>(_a) &
        static_cast<unsigned long>(_b));
}

// operator|=
//   function: bitwise OR assignment for mysql_client_flag values.
inline constexpr mysql_client_flag& operator|=(mysql_client_flag&  _a,
                                               mysql_client_flag   _b)
{
    _a = _a | _b;

    return _a;
}


// =============================================================================
// II.  MYSQL-FAMILY CONNECTION CONFIGURATION
// =============================================================================

// mysql_connect_config
//   struct: extended configuration for MySQL-family connections.
// Augments the generic connection_config with MySQL-specific fields.
struct mysql_connect_config
{
    connection_config   base;
    mysql_client_flag   client_flags;
    std::string         unix_socket;
    std::string         default_charset;
    std::string         init_command;
    bool                use_compression;
    bool                use_local_infile;
    bool                multi_statements;

    std::map<std::string, std::string> connect_attributes;

    mysql_connect_config()
        : client_flags(mysql_client_flag::none)
        , default_charset("utf8mb4")
        , use_compression(false)
        , use_local_infile(false)
        , multi_statements(false)
    {
    }

    explicit mysql_connect_config(const connection_config& _base)
        : base(_base)
        , client_flags(mysql_client_flag::none)
        , default_charset("utf8mb4")
        , use_compression(false)
        , use_local_infile(false)
        , multi_statements(false)
    {
    }
};


// =============================================================================
// III. MYSQL-FAMILY COMMON CONNECTION TEMPLATE
// =============================================================================

// mysql_common_connection
//   class template: shared CRTP base for MySQL-family connections.
// Provides the MySQL-specific interface methods shared by both Oracle
// MySQL and MariaDB. Concrete vendor implementations derive from this
// template.
//
// Template parameters:
//   _Impl:   the concrete CRTP implementation class
//   _DbType: database_type::mysql or database_type::mariadb
template<typename      _Impl,
         database_type _DbType>
class mysql_common_connection
    : public database_connection<_Impl, _DbType>
{
public:
    using base_type     = database_connection<_Impl, _DbType>;
    using traits_type   = typename base_type::traits_type;

    mysql_common_connection()
        : base_type()
    {
    }

    explicit mysql_common_connection(const connection_config& _config)
        : base_type(_config)
    {
    }

    explicit mysql_common_connection(const mysql_connect_config& _config)
        : base_type(_config.base)
        , m_mysql_config(_config)
    {
    }

    ~mysql_common_connection() = default;

    // disable copying
    mysql_common_connection(const mysql_common_connection&)            = delete;
    mysql_common_connection& operator=(const mysql_common_connection&) = delete;

    // enable moving
    mysql_common_connection(mysql_common_connection&&) noexcept            = default;
    mysql_common_connection& operator=(mysql_common_connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // character set management
    // -----------------------------------------------------------------

    // set_charset
    //   function: sets the connection character set.
    // wraps mysql_set_character_set().
    void set_charset(const std::string& _charset)
    {
        this->ensure_connected();

        try
        {
            self().set_charset_impl(_charset);
        }
        catch (const std::exception& _e)
        {
            this->capture_error(-1, _e.what());

            throw connection_exception(_e.what());
        }
    }

    // get_charset
    //   function: returns the current connection character set name.
    std::string get_charset() const
    {
        return self().get_charset_impl();
    }

    // -----------------------------------------------------------------
    // multi-result set iteration
    // -----------------------------------------------------------------

    // next_result
    //   function: advances to the next result set from a multi-
    // statement query or stored procedure call. Returns 0 on
    // success, >0 on error, -1 when no more results.
    int next_result()
    {
        this->ensure_connected();

        return self().next_result_impl();
    }

    // more_results
    //   function: tests whether additional result sets remain.
    bool more_results() const
    {
        return self().more_results_impl();
    }

    // -----------------------------------------------------------------
    // string escaping
    // -----------------------------------------------------------------

    // escape_string
    //   function: escapes a string for safe inclusion in SQL using
    // the connection's current character set.
    // wraps mysql_real_escape_string().
    std::string escape_string(const std::string& _input) const
    {
        return self().escape_string_impl(_input);
    }

    // -----------------------------------------------------------------
    // database selection
    // -----------------------------------------------------------------

    // select_db
    //   function: switches the active database on this connection.
    // wraps mysql_select_db().
    void select_db(const std::string& _database)
    {
        this->ensure_connected();

        try
        {
            self().select_db_impl(_database);

            // update config to reflect the new database
            this->m_config.database = _database;
        }
        catch (const std::exception& _e)
        {
            this->capture_error(-1, _e.what());

            throw connection_exception(_e.what());
        }
    }

    // -----------------------------------------------------------------
    // user switching
    // -----------------------------------------------------------------

    // change_user
    //   function: changes the authenticated user on an established
    // connection. wraps mysql_change_user().
    void change_user(const std::string& _user,
                     const std::string& _password,
                     const std::string& _database)
    {
        this->ensure_connected();

        try
        {
            self().change_user_impl(_user, _password, _database);

            // update config to reflect the new credentials
            this->m_config.username = _user;
            this->m_config.password = _password;

            if (!_database.empty())
            {
                this->m_config.database = _database;
            }
        }
        catch (const std::exception& _e)
        {
            this->capture_error(-1, _e.what());

            throw connection_exception(_e.what());
        }
    }

    // -----------------------------------------------------------------
    // server diagnostics
    // -----------------------------------------------------------------

    // get_stat
    //   function: returns the server status string.
    // wraps mysql_stat().
    std::string get_stat() const
    {
        return self().get_stat_impl();
    }

    // get_thread_id
    //   function: returns this connection's thread identifier on
    // the server.
    unsigned long get_thread_id() const
    {
        return self().get_thread_id_impl();
    }

    // get_warning_count
    //   function: returns the number of warnings generated by the
    // most recent statement.
    unsigned int get_warning_count() const
    {
        return self().get_warning_count_impl();
    }

    // get_sqlstate
    //   function: returns the SQLSTATE error code for the most
    // recent statement.
    std::string get_sqlstate() const
    {
        return self().get_sqlstate_impl();
    }

    // -----------------------------------------------------------------
    // auto-commit
    // -----------------------------------------------------------------

    // set_autocommit
    //   function: enables or disables auto-commit mode.
    // wraps mysql_autocommit().
    void set_autocommit(bool _enabled)
    {
        this->ensure_connected();

        try
        {
            self().set_autocommit_impl(_enabled);
            this->m_auto_commit = _enabled;
        }
        catch (const std::exception& _e)
        {
            this->capture_error(-1, _e.what());

            throw connection_exception(_e.what());
        }
    }

    // -----------------------------------------------------------------
    // MySQL-specific options
    // -----------------------------------------------------------------

    // set_option
    //   function: sets a MySQL connection option. Should be called
    // before connect(). wraps mysql_options().
    void set_option(int _option, const void* _value)
    {
        self().set_option_impl(_option, _value);
    }

    // -----------------------------------------------------------------
    // client flag helpers
    // -----------------------------------------------------------------

    // get_client_flags
    //   function: returns the configured client flags.
    mysql_client_flag get_client_flags() const noexcept
    {
        return m_mysql_config.client_flags;
    }

    // set_client_flags
    //   function: replaces the client flags. Must be called before
    // connect().
    void set_client_flags(mysql_client_flag _flags) noexcept
    {
        m_mysql_config.client_flags = _flags;
    }

    // add_client_flag
    //   function: adds a client flag to the current set. Must be
    // called before connect().
    void add_client_flag(mysql_client_flag _flag) noexcept
    {
        m_mysql_config.client_flags |= _flag;
    }

    // has_client_flag
    //   function: tests whether a specific client flag is set.
    bool has_client_flag(mysql_client_flag _flag) const noexcept
    {
        return static_cast<unsigned long>(
            m_mysql_config.client_flags & _flag) != 0;
    }

    // -----------------------------------------------------------------
    // connect attributes
    // -----------------------------------------------------------------

    // set_connect_attribute
    //   function: sets a key-value connect attribute. Must be called
    // before connect().
    void set_connect_attribute(const std::string& _key,
                               const std::string& _value)
    {
        m_mysql_config.connect_attributes[_key] = _value;
    }

    // get_connect_attributes
    //   function: returns the current connect attribute map.
    const std::map<std::string, std::string>&
    get_connect_attributes() const noexcept
    {
        return m_mysql_config.connect_attributes;
    }

    // -----------------------------------------------------------------
    // MySQL-specific configuration access
    // -----------------------------------------------------------------

    // get_mysql_config
    //   function: returns the MySQL-specific connection configuration.
    const mysql_connect_config& get_mysql_config() const noexcept
    {
        return m_mysql_config;
    }

    // set_mysql_config
    //   function: replaces the MySQL-specific connection
    // configuration. Must be called before connect().
    void set_mysql_config(const mysql_connect_config& _config)
    {
        m_mysql_config = _config;
        this->m_config = _config.base;
    }

    // -----------------------------------------------------------------
    // compile-time vendor queries
    // -----------------------------------------------------------------

    // is_mysql_family
    //   function: returns true (this is always a MySQL-family
    // connection).
    static constexpr bool is_mysql_family() noexcept
    {
        return true;
    }

    // is_mariadb
    //   function: returns true if this connection targets MariaDB.
    static constexpr bool is_mariadb() noexcept
    {
        return (_DbType == database_type::mariadb);
    }

    // is_oracle_mysql
    //   function: returns true if this connection targets Oracle
    // MySQL.
    static constexpr bool is_oracle_mysql() noexcept
    {
        return (_DbType == database_type::mysql);
    }

protected:
    mysql_connect_config m_mysql_config;

    // build_client_flags
    //   function: assembles the final CLIENT_* flags from the
    // configuration. Called by vendor connect_impl() before
    // mysql_real_connect().
    unsigned long build_client_flags() const noexcept
    {
        unsigned long flags =
            static_cast<unsigned long>(m_mysql_config.client_flags);

        if (m_mysql_config.use_compression)
        {
            flags |= static_cast<unsigned long>(
                mysql_client_flag::compress);
        }

        if (m_mysql_config.multi_statements)
        {
            flags |= static_cast<unsigned long>(
                mysql_client_flag::multi_statements);
            flags |= static_cast<unsigned long>(
                mysql_client_flag::multi_results);
        }

        if (this->m_config.enable_ssl)
        {
            flags |= static_cast<unsigned long>(
                mysql_client_flag::ssl);
        }

        if (!this->m_config.database.empty())
        {
            flags |= static_cast<unsigned long>(
                mysql_client_flag::connect_with_db);
        }

        return flags;
    }

    // get_unix_socket_ptr
    //   function: returns the unix socket path as a C string, or
    // nullptr if not configured. Used by vendor connect_impl().
    const char* get_unix_socket_ptr() const noexcept
    {
        if (m_mysql_config.unix_socket.empty())
        {
            return nullptr;
        }

        return m_mysql_config.unix_socket.c_str();
    }

private:
    _Impl& self()
    {
        return static_cast<_Impl&>(*this);
    }

    const _Impl& self() const
    {
        return static_cast<const _Impl&>(*this);
    }
};


NS_END  // mysql_common
NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MYSQL_COMMON_
