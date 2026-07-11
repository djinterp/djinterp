/******************************************************************************
* djinterp [database]                                         mysql_common.hpp
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
* actual C API calls are left to the vendor-specific _helper methods in
* the final CRTP leaf class.
*
*   LAYER DIAGRAM:
*   mysql_connection_helper (vendor-specific)
*     -> mysql_common_connection<mysql_connection_helper, database_type::mysql>
*       -> database_connection<mysql_connection_helper, database_type::mysql>
*         -> connection_template<mysql_connection_helper, database_type::mysql>
*           -> connection<mysql_connection_helper>
*
*   The _helper class must provide the following MySQL-specific methods
* (in addition to those required by database_connection):
*   - void        set_charset_helper(const std::string&)
*   - std::string get_charset_helper() const
*   - int         next_result_helper()
*   - bool        more_results_helper() const
*   - std::string escape_string_helper(const std::string&) const
*   - std::string get_stat_helper() const
*   - unsigned long get_thread_id_helper() const
*   - unsigned int  get_warning_count_helper() const
*   - std::string   get_sqlstate_helper() const
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include the MySQL C API
* headers; concrete implementations include <mysql.h> or <mariadb/mysql.h> 
* themselves.
*
* 
*   DETECTION:
*   Also carries this database's capability-detection traits and C++20 concepts
* (trailing sections), folded in from mysql_common_traits.hpp and the matching *_concepts.hpp;
* detection now lives with the connection. Concepts gated on concept support.
*
* path:      /inc/djinterp/core/db/mysql/mysql_common.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_MYSQL_COMMON_
#define DJINTERP_DATABASE_MYSQL_COMMON_

// djinterp
#include "../../../djinterp.hpp"
#include "../../../env/db/mysql/env_mysql_common.h"
#include "../database_connection.hpp"
#include "../database_traits.hpp"


NS_DJINTERP

// ===========================================================================
// I.   MYSQL-FAMILY CLIENT FLAGS
// ===========================================================================
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
D_INLINE constexpr mysql_client_flag operator|(
    mysql_client_flag _a,
    mysql_client_flag _b
)
{
    return static_cast<mysql_client_flag>(
        static_cast<unsigned long>(_a) |
        static_cast<unsigned long>(_b));
}

// operator&
//   function: bitwise AND for testing mysql_client_flag values.
D_INLINE constexpr mysql_client_flag operator&(
    mysql_client_flag _a,
    mysql_client_flag _b
)
{
    return static_cast<mysql_client_flag>(
        static_cast<unsigned long>(_a) &
        static_cast<unsigned long>(_b));
}

// operator|=
//   function: bitwise OR assignment for mysql_client_flag values.
D_INLINE constexpr mysql_client_flag& operator|=(
    mysql_client_flag&  _a,
    mysql_client_flag   _b
)
{
    _a = _a | _b;

    return _a;
}


// ===========================================================================
// II.  MYSQL-FAMILY CONNECTION CONFIGURATION
// ===========================================================================

// mysql_connect_config
//   struct: extended configuration for MySQL-family connections.
// Augments the generic connection_config with MySQL-specific fields.
struct mysql_connect_config
{
    connection_config base;
    mysql_client_flag client_flags;
    std::string       unix_socket;
    std::string       default_charset;
    std::string       init_command;
    bool              use_compression;
    bool              use_local_infile;
    bool              multi_statements;

    std::map<std::string, std::string> connect_attributes;

    mysql_connect_config()
        : client_flags(mysql_client_flag::none),
          default_charset("utf8mb4"),
          use_compression(false),
          use_local_infile(false),
          multi_statements(false)
    {};

    explicit mysql_connect_config(const connection_config& _base)
        : base(_base),
          client_flags(mysql_client_flag::none),
          default_charset("utf8mb4"),
          use_compression(false),
          use_local_infile(false),
          multi_statements(false)
    {};
};


// ===========================================================================
// III. MYSQL-FAMILY COMMON CONNECTION TEMPLATE
// ===========================================================================

// mysql_common_connection
//   class template: shared CRTP base for MySQL-family connections.
// Provides the MySQL-specific interface methods shared by both Oracle
// MySQL and MariaDB. Concrete vendor implementations derive from this
// template.
//
// Template parameters:
//   _helper:   the concrete CRTP implementation class
//   _DbType: database_type::mysql or database_type::mariadb
template<typename      _helper,
         database_type _DbType>
class mysql_common_connection
    : public database_connection<_helper, _DbType>
{
public:
    using base_type     = database_connection<_helper, _DbType>;
    using traits_type   = typename base_type::traits_type;

    mysql_common_connection()
        : base_type()
    {};

    explicit mysql_common_connection(const connection_config& _config)
        : base_type(_config)
    {};

    explicit mysql_common_connection(const mysql_connect_config& _config)
        : base_type(_config.base)
        , m_mysql_config(_config)
    {};

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
            self().set_charset_helper(_charset);
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
        return self().get_charset_helper();
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

        return self().next_result_helper();
    }

    // more_results
    //   function: tests whether additional result sets remain.
    bool more_results() const
    {
        return self().more_results_helper();
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
        return self().escape_string_helper(_input);
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
            self().select_db_helper(_database);

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
            self().change_user_helper(_user, _password, _database);

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
        return self().get_stat_helper();
    }

    // get_thread_id
    //   function: returns this connection's thread identifier on
    // the server.
    unsigned long get_thread_id() const
    {
        return self().get_thread_id_helper();
    }

    // get_warning_count
    //   function: returns the number of warnings generated by the
    // most recent statement.
    unsigned int get_warning_count() const
    {
        return self().get_warning_count_helper();
    }

    // get_sqlstate
    //   function: returns the SQLSTATE error code for the most
    // recent statement.
    std::string get_sqlstate() const
    {
        return self().get_sqlstate_helper();
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
            self().set_autocommit_helper(_enabled);
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
        self().set_option_helper(_option, _value);
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
    const mysql_connect_config& 
    get_mysql_config() const noexcept
    {
        return m_mysql_config;
    }

    // set_mysql_config
    //   function: replaces the MySQL-specific connection
    // configuration. Must be called before connect().
    void set_mysql_config(
        const mysql_connect_config& _config
    )
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
    // configuration. Called by vendor connect_helper() before
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
    // nullptr if not configured. Used by vendor connect_helper().
    const char* get_unix_socket_ptr() const noexcept
    {
        if (m_mysql_config.unix_socket.empty())
        {
            return nullptr;
        }

        return m_mysql_config.unix_socket.c_str();
    }

private:
    _helper& self()
    {
        return static_cast<_helper&>(*this);
    }

    const _helper& self() const
    {
        return static_cast<const _helper&>(*this);
    }
};


// ===========================================================================
//                   CAPABILITY DETECTION (traits & concepts)
// ===========================================================================
//   Folded in from the former mysql_common_traits.hpp / mysql_common_concepts.hpp
// so detection lives with the connection it describes. Traits build at C++17;
// concepts appear under C++20.

// ===========================================================================
// IV.   EXPRESSION DETECTORS
// ===========================================================================
// Expression alias templates for SFINAE-based detection of MySQL-family
// specific methods. These follow the same pattern as the generic
// detectors in database_traits.hpp but target MySQL C API wrapper
// methods.

// -------------------------------------------------------------------------
// A.  character set management
// -------------------------------------------------------------------------

// mysql_set_charset_t
//   detector: set_charset(const std::string&) method.
template<typename _Type>
using mysql_set_charset_t = decltype(std::declval<_Type&>().set_charset(
    std::declval<const std::string&>()));

// mysql_get_charset_t
//   detector: get_charset() const method.
template<typename _Type>
using mysql_get_charset_t =
    decltype(std::declval<const _Type&>().get_charset());

// -------------------------------------------------------------------------
// B.  multi-result set iteration
// -------------------------------------------------------------------------

// mysql_next_result_t
//   detector: next_result() method.
// wraps mysql_next_result() for iterating over multiple result sets
// from multi-statement queries or stored procedures.
template<typename _Type>
using mysql_next_result_t =
    decltype(std::declval<_Type&>().next_result());

// mysql_more_results_t
//   detector: more_results() const method.
// wraps mysql_more_results() to check if additional result sets
// remain.
template<typename _Type>
using mysql_more_results_t =
    decltype(std::declval<const _Type&>().more_results());

// -------------------------------------------------------------------------
// C.  options API
// -------------------------------------------------------------------------

// mysql_set_option_t
//   detector: set_option(int, const void*) method.
// wraps mysql_options() for setting connection options before
// connecting.
template<typename _Type>
using mysql_set_option_t = decltype(std::declval<_Type&>().set_option(
    std::declval<int>(),
    std::declval<const void*>()));

// mysql_get_option_t
//   detector: get_option(int, void*) const method.
// wraps mysql_get_option() for querying connection option values.
template<typename _Type>
using mysql_get_option_t = decltype(std::declval<const _Type&>().get_option(
    std::declval<int>(),
    std::declval<void*>()));

// -------------------------------------------------------------------------
// D.  server diagnostics
// -------------------------------------------------------------------------

// mysql_get_stat_t
//   detector: get_stat() const method.
// wraps mysql_stat() for server status string.
template<typename _Type>
using mysql_get_stat_t =
    decltype(std::declval<const _Type&>().get_stat());

// mysql_get_thread_id_t
//   detector: get_thread_id() const method.
// wraps mysql_thread_id() for the connection's thread identifier.
template<typename _Type>
using mysql_get_thread_id_t =
    decltype(std::declval<const _Type&>().get_thread_id());

// mysql_get_warning_count_t
//   detector: get_warning_count() const method.
// wraps mysql_warning_count().
template<typename _Type>
using mysql_get_warning_count_t =
    decltype(std::declval<const _Type&>().get_warning_count());

// mysql_get_sqlstate_t
//   detector: get_sqlstate() const method.
// wraps mysql_sqlstate() for SQLSTATE error code access.
template<typename _Type>
using mysql_get_sqlstate_t =
    decltype(std::declval<const _Type&>().get_sqlstate());

// -------------------------------------------------------------------------
// E.  result set mode
// -------------------------------------------------------------------------

// mysql_store_result_t
//   detector: store_result() method.
// wraps mysql_store_result() for buffered result sets.
template<typename _Type>
using mysql_store_result_t =
    decltype(std::declval<_Type&>().store_result());

// mysql_use_result_t
//   detector: use_result() method.
// wraps mysql_use_result() for streaming (unbuffered) result sets.
template<typename _Type>
using mysql_use_result_t =
    decltype(std::declval<_Type&>().use_result());

// -------------------------------------------------------------------------
// F.  auto-commit
// -------------------------------------------------------------------------

// mysql_set_autocommit_t
//   detector: set_autocommit(bool) method.
// wraps mysql_autocommit().
template<typename _Type>
using mysql_set_autocommit_t = decltype(std::declval<_Type&>().set_autocommit(
    std::declval<bool>()));

// -------------------------------------------------------------------------
// G.  escape and select
// -------------------------------------------------------------------------

// mysql_escape_string_t
//   detector: escape_string(const std::string&) const method.
// wraps mysql_real_escape_string().
template<typename _Type>
using mysql_escape_string_t =
    decltype(std::declval<const _Type&>().escape_string(
        std::declval<const std::string&>()));

// mysql_select_db_t
//   detector: select_db(const std::string&) method.
// wraps mysql_select_db() for switching the active database.
template<typename _Type>
using mysql_select_db_t = decltype(std::declval<_Type&>().select_db(
    std::declval<const std::string&>()));

// mysql_change_user_t
//   detector: change_user(const std::string&, const std::string&,
// const std::string&) method. wraps mysql_change_user().
template<typename _Type>
using mysql_change_user_t = decltype(std::declval<_Type&>().change_user(
    std::declval<const std::string&>(),
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// H.  storage engine
// -------------------------------------------------------------------------

// mysql_get_engine_t
//   detector: get_engine() const method.
// returns the default storage engine name.
template<typename _Type>
using mysql_get_engine_t =
    decltype(std::declval<const _Type&>().get_engine());

// mysql_set_engine_t
//   detector: set_engine(const std::string&) method.
// sets the session default storage engine.
template<typename _Type>
using mysql_set_engine_t = decltype(std::declval<_Type&>().set_engine(
    std::declval<const std::string&>()));


// ===========================================================================
// V.  TAGGED CAPABILITY TRAITS (struct-based)
// ===========================================================================

// has_mysql_charset
//   trait: checks if type _Type supports character set management
// (set_charset + get_charset).
template<typename _Type>
struct has_mysql_charset : djinterp::conjunction<
    is_detected<mysql_set_charset_t, clean_t<_Type>>,
    is_detected<mysql_get_charset_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_charset_v
    //   value: convenience alias for has_mysql_charset<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_charset_v = has_mysql_charset<clean_t<_Type>>::value;
#endif

// has_mysql_multi_result
//   trait: checks if type _Type supports multi-result set iteration
// (next_result + more_results).
template<typename _Type>
struct has_mysql_multi_result : djinterp::conjunction<
    is_detected<mysql_next_result_t, clean_t<_Type>>,
    is_detected<mysql_more_results_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_multi_result_v
    //   value: convenience alias for has_mysql_multi_result<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_multi_result_v =
        has_mysql_multi_result<clean_t<_Type>>::value;
#endif

// has_mysql_options
//   trait: checks if type _Type supports the MySQL options API
// (set_option + get_option).
template<typename _Type>
struct has_mysql_options : djinterp::conjunction<
    is_detected<mysql_set_option_t, clean_t<_Type>>,
    is_detected<mysql_get_option_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_options_v
    //   value: convenience alias for has_mysql_options<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_options_v = has_mysql_options<clean_t<_Type>>::value;
#endif

// has_mysql_diagnostics
//   trait: checks if type _Type supports MySQL server diagnostics
// (get_stat + get_thread_id + get_warning_count + get_sqlstate).
template<typename _Type>
struct has_mysql_diagnostics : djinterp::conjunction<
    is_detected<mysql_get_stat_t, clean_t<_Type>>,
    is_detected<mysql_get_thread_id_t, clean_t<_Type>>,
    is_detected<mysql_get_warning_count_t, clean_t<_Type>>,
    is_detected<mysql_get_sqlstate_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_diagnostics_v
    //   value: convenience alias for has_mysql_diagnostics<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_diagnostics_v =
        has_mysql_diagnostics<clean_t<_Type>>::value;
#endif

// has_mysql_result_modes
//   trait: checks if type _Type supports both buffered and streaming
// result set modes (store_result + use_result).
template<typename _Type>
struct has_mysql_result_modes : djinterp::conjunction<
    is_detected<mysql_store_result_t, clean_t<_Type>>,
    is_detected<mysql_use_result_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_result_modes_v
    //   value: convenience alias for has_mysql_result_modes<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_result_modes_v =
        has_mysql_result_modes<clean_t<_Type>>::value;
#endif

// has_mysql_escape
//   trait: checks if type _Type supports MySQL string escaping.
template<typename _Type>
struct has_mysql_escape : is_detected<mysql_escape_string_t, clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_escape_v
    //   value: convenience alias for has_mysql_escape<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_escape_v = has_mysql_escape<clean_t<_Type>>::value;
#endif

// has_mysql_change_user
//   trait: checks if type _Type supports runtime user switching.
template<typename _Type>
struct has_mysql_change_user : is_detected<mysql_change_user_t, clean_t<_Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_change_user_v
    //   value: convenience alias for has_mysql_change_user<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_change_user_v =
        has_mysql_change_user<clean_t<_Type>>::value;
#endif

// has_mysql_engine
//   trait: checks if type _Type supports storage engine queries
// (get_engine + set_engine).
template<typename _Type>
struct has_mysql_engine : djinterp::conjunction<
    is_detected<mysql_get_engine_t, clean_t<_Type>>,
    is_detected<mysql_set_engine_t, clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_engine_v
    //   value: convenience alias for has_mysql_engine<_Type>::value.
    template<typename _Type>
    constexpr bool has_mysql_engine_v = has_mysql_engine<clean_t<_Type>>::value;
#endif

// is_mysql_connection
//   trait: compound trait verifying type _Type implements a MySQL-
// family connection interface (vendor connection + charset +
// multi-result + diagnostics + escape).
template<typename _Type>
struct is_mysql_connection : djinterp::conjunction<
    is_vendor_connection<clean_t<_Type>>,
    has_mysql_charset<clean_t<_Type>>,
    has_mysql_multi_result<clean_t<_Type>>,
    has_mysql_diagnostics<clean_t<_Type>>,
    has_mysql_escape<clean_t<_Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_mysql_connection_v
    //   value: convenience alias for is_mysql_connection<_Type>::value.
    template<typename _Type>
    constexpr bool is_mysql_connection_v =
        is_mysql_connection<clean_t<_Type>>::value;
#endif


// ===========================================================================
// VI. TAGLESS CAPABILITY TRAITS (constexpr bool)
// ===========================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// mysql_can_set_charset
//   tagless trait: true if _Type has a set_charset() method.
template<typename _Type,
         typename = void>
constexpr bool mysql_can_set_charset = false;

template<typename _Type>
constexpr bool mysql_can_set_charset<_Type,
    std::void_t<mysql_set_charset_t<_Type>>> = true;

// mysql_can_get_charset
//   tagless trait: true if _Type has a get_charset() method.
template<typename _Type,
         typename = void>
constexpr bool mysql_can_get_charset = false;

template<typename _Type>
constexpr bool mysql_can_get_charset<_Type,
    std::void_t<mysql_get_charset_t<_Type>>> = true;

// mysql_can_iterate_results
//   tagless trait: true if _Type has next_result() for multi-result
// iteration.
template<typename _Type,
         typename = void>
constexpr bool mysql_can_iterate_results = false;

template<typename _Type>
constexpr bool mysql_can_iterate_results<_Type,
    std::void_t<mysql_next_result_t<_Type>>> = true;

// mysql_can_check_more_results
//   tagless trait: true if _Type has more_results().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_check_more_results = false;

template<typename _Type>
constexpr bool mysql_can_check_more_results<_Type,
    std::void_t<mysql_more_results_t<_Type>>> = true;

// mysql_can_set_option
//   tagless trait: true if _Type has set_option().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_set_option = false;

template<typename _Type>
constexpr bool mysql_can_set_option<_Type,
    std::void_t<mysql_set_option_t<_Type>>> = true;

// mysql_can_get_stat
//   tagless trait: true if _Type has get_stat().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_get_stat = false;

template<typename _Type>
constexpr bool mysql_can_get_stat<_Type,
    std::void_t<mysql_get_stat_t<_Type>>> = true;

// mysql_can_get_sqlstate
//   tagless trait: true if _Type has get_sqlstate().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_get_sqlstate = false;

template<typename _Type>
constexpr bool mysql_can_get_sqlstate<_Type,
    std::void_t<mysql_get_sqlstate_t<_Type>>> = true;

// mysql_can_store_result
//   tagless trait: true if _Type has store_result().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_store_result = false;

template<typename _Type>
constexpr bool mysql_can_store_result<_Type,
    std::void_t<mysql_store_result_t<_Type>>> = true;

// mysql_can_use_result
//   tagless trait: true if _Type has use_result() (streaming).
template<typename _Type,
         typename = void>
constexpr bool mysql_can_use_result = false;

template<typename _Type>
constexpr bool mysql_can_use_result<_Type,
    std::void_t<mysql_use_result_t<_Type>>> = true;

// mysql_can_escape_string
//   tagless trait: true if _Type has escape_string().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_escape_string = false;

template<typename _Type>
constexpr bool mysql_can_escape_string<_Type,
    std::void_t<mysql_escape_string_t<_Type>>> = true;

// mysql_can_select_db
//   tagless trait: true if _Type has select_db().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_select_db = false;

template<typename _Type>
constexpr bool mysql_can_select_db<_Type,
    std::void_t<mysql_select_db_t<_Type>>> = true;

// mysql_can_change_user
//   tagless trait: true if _Type has change_user().
template<typename _Type,
         typename = void>
constexpr bool mysql_can_change_user = false;

template<typename _Type>
constexpr bool mysql_can_change_user<_Type,
    std::void_t<mysql_change_user_t<_Type>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// mysql_does_charset
//   tagless trait: true if _Type supports full charset management
// (set + get).
template<typename _Type>
constexpr bool mysql_does_charset =
    ( mysql_can_set_charset<clean_t<_Type>> &&
      mysql_can_get_charset<clean_t<_Type>> );

// mysql_does_multi_result
//   tagless trait: true if _Type supports multi-result iteration
// (next_result + more_results).
template<typename _Type>
constexpr bool mysql_does_multi_result =
    ( mysql_can_iterate_results<clean_t<_Type>>    &&
      mysql_can_check_more_results<clean_t<_Type>> );

// mysql_does_result_modes
//   tagless trait: true if _Type supports both buffered and streaming
// result sets.
template<typename _Type>
constexpr bool mysql_does_result_modes =
    ( mysql_can_store_result<clean_t<_Type>> &&
      mysql_can_use_result<clean_t<_Type>> );

// mysql_does_diagnostics
//   tagless trait: true if _Type supports server diagnostics
// (stat + sqlstate).
template<typename _Type>
constexpr bool mysql_does_diagnostics =
    ( mysql_can_get_stat<clean_t<_Type>>     &&
      mysql_can_get_sqlstate<clean_t<_Type>> );

// mysql_is_full_connection
//   tagless trait: true if _Type satisfies the complete MySQL-family
// connection interface (full vendor + charset + multi-result +
// diagnostics + escape).
template<typename _Type>
constexpr bool mysql_is_full_connection =
    ( is_full_vendor<clean_t<_Type>>              &&
      mysql_does_charset<clean_t<_Type>>          &&
      mysql_does_multi_result<clean_t<_Type>>     &&
      mysql_does_diagnostics<clean_t<_Type>>      &&
      mysql_can_escape_string<clean_t<_Type>> );


// ===========================================================================
// VII.  SFINAE HELPERS
// ===========================================================================

// enable_if_mysql_connection
//   type: SFINAE helper for MySQL-family connection constraints.
template<typename _Type>
using enable_if_mysql_connection =
    typename std::enable_if<is_mysql_connection<clean_t<_Type>>::value>::type;

// enable_if_has_mysql_charset
//   type: SFINAE helper for MySQL charset constraints.
template<typename _Type>
using enable_if_has_mysql_charset =
    typename std::enable_if<has_mysql_charset<clean_t<_Type>>::value>::type;

// enable_if_has_mysql_multi_result
//   type: SFINAE helper for MySQL multi-result constraints.
template<typename _Type>
using enable_if_has_mysql_multi_result =
    typename std::enable_if<has_mysql_multi_result<clean_t<_Type>>::value>::type;

// enable_if_has_mysql_diagnostics
//   type: SFINAE helper for MySQL diagnostics constraints.
template<typename _Type>
using enable_if_has_mysql_diagnostics =
    typename std::enable_if<has_mysql_diagnostics<clean_t<_Type>>::value>::type;


// ===========================================================================
// VIII.   C++20 CONCEPTS
// ===========================================================================
//   The MySQL-family classification concepts, folded in from the former
// mysql_common_concepts.hpp.  Each is a thin forward to a trait, variable
// template, or tagless capability declared above -- no detection is re-
// implemented here.  Gated on concept support (rather than a hard #error) so
// the traits above remain usable at the C++17 baseline; the concepts simply
// appear when the language provides them, exactly as in functor.hpp / monoid.hpp.

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS


// -------------------------------------------------------------------------
// A.  core MySQL-family connection concepts
// -------------------------------------------------------------------------

// Mysql_connection
//   concept: constrains types implementing the MySQL-family connection
// interface.
template<typename _Type>
concept Mysql_connection =
    is_mysql_connection<clean_t<_Type>>::value;

// non_mysql_connection
//   concept: constrains types that do not implement the MySQL-family
// connection interface.
template<typename _Type>
concept non_mysql_connection =
    !Mysql_connection<_Type>;

// mysql_charset_connection
//   concept: constrains MySQL-family connections supporting character-set
// management.
template<typename _Type>
concept mysql_charset_connection =
    has_mysql_charset<clean_t<_Type>>::value;

// mysql_multi_result_connection
//   concept: constrains MySQL-family connections supporting multi-result
// iteration.
template<typename _Type>
concept mysql_multi_result_connection =
    has_mysql_multi_result<clean_t<_Type>>::value;

// mysql_diagnostics_connection
//   concept: constrains MySQL-family connections supporting server
// diagnostics.
template<typename _Type>
concept mysql_diagnostics_connection =
    has_mysql_diagnostics<clean_t<_Type>>::value;

// mysql_result_modes_connection
//   concept: constrains MySQL-family connections supporting both buffered
// and streaming result modes.
template<typename _Type>
concept mysql_result_modes_connection =
    has_mysql_result_modes<clean_t<_Type>>::value;

// mysql_escape_connection
//   concept: constrains MySQL-family connections supporting SQL string
// escaping.
template<typename _Type>
concept mysql_escape_connection =
    has_mysql_escape<clean_t<_Type>>::value;

// mysql_options_connection
//   concept: constrains MySQL-family connections supporting the options API.
template<typename _Type>
concept mysql_options_connection =
    has_mysql_options<clean_t<_Type>>::value;

// mysql_engine_connection
//   concept: constrains MySQL-family connections supporting storage-engine
// queries.
template<typename _Type>
concept mysql_engine_connection =
    has_mysql_engine<clean_t<_Type>>::value;


// -------------------------------------------------------------------------
// B.  capability concepts
// -------------------------------------------------------------------------

// mysql_select_db_connection
//   concept: constrains types exposing select_db(database).
template<typename _Type>
concept mysql_select_db_connection =
    is_detected<mysql_select_db_t, clean_t<_Type>>::value;

// mysql_autocommit_connection
//   concept: constrains types exposing set_autocommit(bool).
template<typename _Type>
concept mysql_autocommit_connection =
    is_detected<mysql_set_autocommit_t, clean_t<_Type>>::value;

// mysql_set_option_connection
//   concept: constrains types exposing set_option(option, value).
template<typename _Type>
concept mysql_set_option_connection =
    mysql_can_set_option<clean_t<_Type>>;

// mysql_get_option_connection
//   concept: constrains types exposing get_option(option, value).
template<typename _Type>
concept mysql_get_option_connection =
    is_detected<mysql_get_option_t, clean_t<_Type>>::value;

// mysql_stat_connection
//   concept: constrains types exposing get_stat().
template<typename _Type>
concept mysql_stat_connection =
    mysql_can_get_stat<clean_t<_Type>>;

// mysql_thread_id_connection
//   concept: constrains types exposing get_thread_id().
template<typename _Type>
concept mysql_thread_id_connection =
    is_detected<mysql_get_thread_id_t, clean_t<_Type>>::value;

// mysql_warning_count_connection
//   concept: constrains types exposing get_warning_count().
template<typename _Type>
concept mysql_warning_count_connection =
    is_detected<mysql_get_warning_count_t, clean_t<_Type>>::value;

// mysql_sqlstate_connection
//   concept: constrains types exposing get_sqlstate().
template<typename _Type>
concept mysql_sqlstate_connection =
    mysql_can_get_sqlstate<clean_t<_Type>>;

// mysql_store_result_connection
//   concept: constrains types exposing store_result().
template<typename _Type>
concept mysql_store_result_connection =
    mysql_can_store_result<clean_t<_Type>>;

// mysql_use_result_connection
//   concept: constrains types exposing use_result().
template<typename _Type>
concept mysql_use_result_connection =
    mysql_can_use_result<clean_t<_Type>>;

// mysql_change_user_connection
//   concept: constrains types exposing change_user(user, password,
// database).
template<typename _Type>
concept mysql_change_user_connection =
    has_mysql_change_user<clean_t<_Type>>::value;

// mysql_set_engine_connection
//   concept: constrains types exposing set_engine(name).
template<typename _Type>
concept mysql_set_engine_connection =
    is_detected<mysql_set_engine_t, clean_t<_Type>>::value;

// mysql_get_engine_connection
//   concept: constrains types exposing get_engine().
template<typename _Type>
concept mysql_get_engine_connection =
    is_detected<mysql_get_engine_t, clean_t<_Type>>::value;


// -------------------------------------------------------------------------
// C.  tagless capability concepts
// -------------------------------------------------------------------------

// mysql_charset_manageable
//   concept: constrains types satisfying the tagless charset capability set.
template<typename _Type>
concept mysql_charset_manageable =
    mysql_does_charset<clean_t<_Type>>;

// mysql_multi_result_iterable
//   concept: constrains types satisfying the tagless multi-result
// capability set.
template<typename _Type>
concept mysql_multi_result_iterable =
    mysql_does_multi_result<clean_t<_Type>>;

// mysql_result_mode_selectable
//   concept: constrains types satisfying the tagless result-mode capability
// set.
template<typename _Type>
concept mysql_result_mode_selectable =
    mysql_does_result_modes<clean_t<_Type>>;

// mysql_diagnostic_connection_tagless
//   concept: constrains types satisfying the tagless diagnostics capability
// set.
template<typename _Type>
concept mysql_diagnostic_connection_tagless =
    mysql_does_diagnostics<clean_t<_Type>>;

// mysql_charset_settable
//   concept: constrains types satisfying the tagless set-charset capability.
template<typename _Type>
concept mysql_charset_settable =
    mysql_can_set_charset<clean_t<_Type>>;

// mysql_charset_gettable
//   concept: constrains types satisfying the tagless get-charset capability.
template<typename _Type>
concept mysql_charset_gettable =
    mysql_can_get_charset<clean_t<_Type>>;

// mysql_result_iterable
//   concept: constrains types satisfying the tagless next-result capability.
template<typename _Type>
concept mysql_result_iterable =
    mysql_can_iterate_results<clean_t<_Type>>;

// mysql_more_results_query
//   concept: constrains types satisfying the tagless more-results capability.
template<typename _Type>
concept mysql_more_results_query =
    mysql_can_check_more_results<clean_t<_Type>>;

// mysql_string_escapable
//   concept: constrains types satisfying the tagless escape-string
// capability.
template<typename _Type>
concept mysql_string_escapable =
    mysql_can_escape_string<clean_t<_Type>>;

// mysql_database_selectable
//   concept: constrains types satisfying the tagless select-db capability.
template<typename _Type>
concept mysql_database_selectable =
    mysql_can_select_db<clean_t<_Type>>;

// mysql_user_switchable
//   concept: constrains types satisfying the tagless change-user capability.
template<typename _Type>
concept mysql_user_switchable =
    mysql_can_change_user<clean_t<_Type>>;

// mysql_full_connection
//   concept: constrains types satisfying the complete tagless MySQL-family
// connection capability set.
template<typename _Type>
concept mysql_full_connection =
    mysql_is_full_connection<clean_t<_Type>>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_DATABASE_MYSQL_COMMON_