/******************************************************************************
* djinterp [database]                                      database_common.hpp
* 
* djinterp database common module:
*   This header provides comprehensive base classes, interfaces, and utilities
* for database operations that are vendor-agnostic, including:
*   - connection management and pooling
*   - transaction handling with RAII semantics
*   - query execution and result set handling
*   - parameter binding and prepared statements
*   - error handling and diagnostics
*   - type conversion utilities
*   - connection string parsing
*
*   This module serves as the foundation for vendor-specific implementations
* (MariaDB, MySQL, PostgreSQL, SQLite, MongoDB, etc.) providing a unified
* interface and common functionality that can be extended or specialized.
*
*   The design leverages modern C++ features including:
*   - RAII for resource management
*   - move semantics for efficient resource transfer
*   - templates for type-safe operations
*   - CRTP for zero-overhead compile-time polymorphism
*   - smart pointers for automatic memory management
*
*   PORTABILITY:
*   This header requires C++17 or later due to its use of std::optional,
* std::variant, and std::string_view in the public API surface. Internal
* metaprogramming (detection idiom, conjunction, is_invocable) is delegated
* to type_traits.hpp for consistency with the rest of the djinterp
* tool-chain.
*
* 
* path:      /inc/djinterp/core/db/database_common.hpp                                           
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_COMMON_
#define DJINTERP_DATABASE_COMMON_

// std
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "../env/db/env_db.h"
#include "./database_traits.hpp"


NS_DJINTERP
NS_DATABASE

// =============================================================================
// II.  ENUMERATIONS AND TYPE DEFINITIONS
// =============================================================================

// database_type
//   enumeration: database type/vendor identifier.
// Note: these are sequential identifiers, not bit flags. For
// compile-time bit-flag queries use the D_ENV_DB_FLAG_* macros
// in env_db.h directly.
enum class database_type : std::uint16_t
{
    unknown    = 0x00,
    mariadb    = 0x01,
    mysql      = 0x02,
    postgresql = 0x03,
    sqlite     = 0x04,
    mongodb    = 0x05,
    redis      = 0x06,
    arangodb   = 0x07,
    oracle     = 0x08,
    mssql      = 0x09,
    db2        = 0x0A,
    firebase   = 0x0B,
    cassandra  = 0x0C,
    couchdb    = 0x0D,
    neo4j      = 0x0E
};

// isolation_level
//   enumeration: transaction isolation levels per SQL standard.
enum class isolation_level
{
    read_uncommitted,
    read_committed,
    repeatable_read,
    serializable,
    default_level
};

// connection_state
//   enumeration: connection state tracking.
enum class connection_state
{
    disconnected,
    connecting,
    connected,
    executing,
    error,
    closed
};

// field_type
//   enumeration: database field/column types.
enum class field_type
{
    null,
    boolean,
    integer,
    big_integer,
    floating_point,
    decimal,
    string,
    binary,
    date,
    time,
    datetime,
    timestamp,
    json,
    xml,
    uuid,
    array,
    custom
};

// value
//   type alias: variant type for database values supporting common
// types.
using value = std::variant<
    std::monostate,                         // null
    bool,                                   // boolean
    std::int32_t,                           // integer
    std::int64_t,                           // big integer
    double,                                 // floating point
    std::string,                            // string/text
    std::vector<std::uint8_t>,              // binary/blob
    std::chrono::system_clock::time_point   // timestamp
>;

// parameter_map
//   type alias: named parameters for prepared statements.
using parameter_map = std::map<std::string, value>;

// row
//   type alias: single row of database results as named fields.
using row = std::map<std::string, value>;

// result_rows
//   type alias: collection of database result rows.
using result_rows = std::vector<row>;


// =============================================================================
// III. EXCEPTION CLASSES
// =============================================================================

// exception
//   class: base exception class for all database errors.
class exception : public std::runtime_error
{
public:
    explicit exception(const std::string& _message);
    explicit exception(const char* _message);

    const char* what() const noexcept override;
    int         error_code() const noexcept;

protected:
    std::string m_message;
    int         m_error_code;
};

// connection_exception
//   class: connection-related errors.
class connection_exception : public exception
{
public:
    using exception::exception;
};

// query_exception
//   class: query execution errors.
class query_exception : public exception
{
public:
    using exception::exception;
};

// transaction_exception
//   class: transaction-related errors.
class transaction_exception : public exception
{
public:
    using exception::exception;
};

// type_conversion_exception
//   class: type conversion errors.
class type_conversion_exception : public exception
{
public:
    using exception::exception;
};


// =============================================================================
// IV.  CONNECTION CONFIGURATION
// =============================================================================

// connection_config
//   struct: configuration parameters for database connections.
struct connection_config
{
    std::string   host;
    std::uint16_t port;
    std::string   database;
    std::string   username;
    std::string   password;

    std::optional<std::string> charset;
    std::optional<std::string> schema;
    std::optional<std::string> ssl_cert;
    std::optional<std::string> ssl_key;
    std::optional<std::string> ssl_ca;

    std::chrono::milliseconds connect_timeout;
    std::chrono::milliseconds read_timeout;
    std::chrono::milliseconds write_timeout;

    bool        auto_reconnect;
    bool        enable_ssl;
    bool        verify_ssl;
    std::size_t max_packet_size;

    std::map<std::string, std::string> custom_options;

    connection_config();
};

// pool_config
//   struct: configuration for connection pooling.
struct pool_config
{
    std::size_t min_connections;
    std::size_t max_connections;
    std::size_t connection_queue_size;

    std::chrono::milliseconds connection_timeout;
    std::chrono::milliseconds idle_timeout;
    std::chrono::milliseconds max_lifetime;

    bool validate_on_acquire;
    bool validate_on_return;

    std::string validation_query;

    pool_config();
};


// =============================================================================
// IV.b VENDOR INFORMATION
// =============================================================================

// vendor_info
//   struct: runtime-queryable vendor metadata for a database connection.
// Provides a uniform way to inspect vendor identity regardless of the
// concrete CRTP implementation type.
struct vendor_info
{
    database_type   type;
    std::string     name;
    std::string     display_name;
    std::uint16_t   default_port;
    bool            is_relational;
    bool            is_embedded;
    bool            supports_ssl;

    vendor_info()
        : type(database_type::unknown)
        , default_port(0)
        , is_relational(false)
        , is_embedded(false)
        , supports_ssl(false)
    {};
};

// vendor_traits
//   trait: primary template for vendor-specific connection configuration.
// Vendor modules specialize this for each database_type to declare
// native handle types, default ports, and other vendor-specific
// metadata. The unspecialized template provides safe defaults for
// unknown vendors.
template<database_type _DbType>
struct vendor_traits
{
    // native_handle_type
    //   type: the vendor's native connection handle type.
    // defaults to void* for unknown vendors.
    using native_handle_type = void*;

    // db_type
    //   value: the database_type enumerator for this vendor.
    static constexpr database_type db_type = _DbType;

    // default_port
    //   value: the vendor's default TCP port (0 for unknown/embedded).
    static constexpr std::uint16_t default_port = 0;

    // name
    //   value: short identifier string for the vendor.
    static constexpr const char* name = "unknown";

    // display_name
    //   value: human-readable display name for the vendor.
    static constexpr const char* display_name = "Unknown Database";

    // is_relational
    //   value: true if the vendor is a relational DBMS.
    static constexpr bool is_relational = false;

    // is_embedded
    //   value: true if the vendor is an embedded database.
    static constexpr bool is_embedded = false;

    // supports_ssl
    //   value: true if the vendor supports SSL/TLS connections.
    static constexpr bool supports_ssl = false;

    // get_info
    //   function: returns a populated vendor_info struct.
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

    // make_default_config
    //   function: returns a connection_config with vendor-appropriate
    // defaults.
    static connection_config make_default_config()
    {
        connection_config config;

        config.host = "localhost";
        config.port = default_port;

        return config;
    }
};


// =============================================================================
// V.   RESULT SET TEMPLATE
// =============================================================================

// result_set
//   class template: wrapper for accessing query results using CRTP
// pattern.
template<typename _helper>
class result_set
{
public:
    result_set()  = default;
    ~result_set() = default;

    result_set(const result_set&)            = delete;
    result_set& operator=(const result_set&) = delete;

    result_set(result_set&&) noexcept            = default;
    result_set& operator=(result_set&&) noexcept = default;

    // -----------------------------------------------------------------
    // navigation
    // -----------------------------------------------------------------

    bool next()
    {
        return impl().next();
    }

    bool previous()
    {
        return impl().previous();
    }

    bool first()
    {
        return impl().first();
    }

    bool last()
    {
        return impl().last();
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable_r<
                     bool,
                     decltype(&_Type::absolute),
                     _Type&,
                     std::int64_t>::value>>
    bool absolute(std::int64_t _row)
    {
        return impl().absolute(_row);
    }

    // -----------------------------------------------------------------
    // metadata
    // -----------------------------------------------------------------

    std::size_t column_count() const
    {
        return impl().column_count();
    }

    std::string column_name(std::size_t _index) const
    {
        return impl().column_name(_index);
    }

    field_type column_type(std::size_t _index) const
    {
        return impl().column_type(_index);
    }

    std::size_t column_index(const std::string& _name) const
    {
        return impl().column_index(_name);
    }

    std::int64_t row_count() const
    {
        return impl().row_count();
    }

    std::int64_t current_row() const
    {
        return impl().current_row();
    }

    // -----------------------------------------------------------------
    // data access by index
    // -----------------------------------------------------------------

    value get_value(std::size_t _index) const
    {
        return impl().get_value(_index);
    }

    bool is_null(std::size_t _index) const
    {
        return impl().is_null(_index);
    }

    std::optional<bool> get_bool(std::size_t _index) const
    {
        return impl().get_bool(_index);
    }

    std::optional<std::int32_t> get_int(std::size_t _index) const
    {
        return impl().get_int(_index);
    }

    std::optional<std::int64_t> get_long(std::size_t _index) const
    {
        return impl().get_long(_index);
    }

    std::optional<double> get_double(std::size_t _index) const
    {
        return impl().get_double(_index);
    }

    std::optional<std::string> get_string(std::size_t _index) const
    {
        return impl().get_string(_index);
    }

    std::optional<std::vector<std::uint8_t>>
    get_binary(std::size_t _index) const
    {
        return impl().get_binary(_index);
    }

    // -----------------------------------------------------------------
    // data access by name
    // -----------------------------------------------------------------

    value get_value(const std::string& _name) const
    {
        return impl().get_value(_name);
    }

    bool is_null(const std::string& _name) const
    {
        return impl().is_null(_name);
    }

    std::optional<bool> get_bool(const std::string& _name) const
    {
        return impl().get_bool(_name);
    }

    std::optional<std::int32_t>
    get_int(const std::string& _name) const
    {
        return impl().get_int(_name);
    }

    std::optional<std::int64_t>
    get_long(const std::string& _name) const
    {
        return impl().get_long(_name);
    }

    std::optional<double>
    get_double(const std::string& _name) const
    {
        return impl().get_double(_name);
    }

    std::optional<std::string>
    get_string(const std::string& _name) const
    {
        return impl().get_string(_name);
    }

    std::optional<std::vector<std::uint8_t>>
    get_binary(const std::string& _name) const
    {
        return impl().get_binary(_name);
    }

    // -----------------------------------------------------------------
    // convenience
    // -----------------------------------------------------------------

    row get_current_row() const
    {
        return impl().get_current_row();
    }

    result_rows get_all_rows()
    {
        return impl().get_all_rows();
    }

    void close()
    {
        impl().close();
    }

private:
    _helper& impl()
    {
        return static_cast<_helper&>(*this);
    }

    const _helper& impl() const
    {
        return static_cast<const _helper&>(*this);
    }
};


// =============================================================================
// VI.  PREPARED STATEMENT TEMPLATE
// =============================================================================

// statement
//   class template: wrapper for prepared statements using CRTP
// pattern.
template<typename _helper>
class statement
{
public:
    statement()  = default;
    ~statement() = default;

    statement(const statement&)            = delete;
    statement& operator=(const statement&) = delete;

    statement(statement&&) noexcept            = default;
    statement& operator=(statement&&) noexcept = default;

    // -----------------------------------------------------------------
    // parameter binding by index (1-based)
    // -----------------------------------------------------------------

    void bind_null(std::size_t _index)
    {
        impl().bind_null(_index);
    }

    void bind_bool(std::size_t _index, bool _value)
    {
        impl().bind_bool(_index, _value);
    }

    void bind_int(std::size_t _index, std::int32_t _value)
    {
        impl().bind_int(_index, _value);
    }

    void bind_long(std::size_t _index, std::int64_t _value)
    {
        impl().bind_long(_index, _value);
    }

    void bind_double(std::size_t _index, double _value)
    {
        impl().bind_double(_index, _value);
    }

    void bind_string(std::size_t    _index,
                     const std::string& _value)
    {
        impl().bind_string(_index, _value);
    }

    void bind_binary(std::size_t                     _index,
                     const std::vector<std::uint8_t>& _value)
    {
        impl().bind_binary(_index, _value);
    }

    // -----------------------------------------------------------------
    // parameter binding by name (for databases that support it)
    // -----------------------------------------------------------------

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::bind_null),
                     _Type&,
                     const std::string&>::value>>
    void bind_null(const std::string& _name)
    {
        impl().bind_null(_name);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::bind_bool),
                     _Type&,
                     const std::string&,
                     bool>::value>>
    void bind_bool(const std::string& _name, bool _value)
    {
        impl().bind_bool(_name, _value);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::bind_int),
                     _Type&,
                     const std::string&,
                     std::int32_t>::value>>
    void bind_int(const std::string& _name, std::int32_t _value)
    {
        impl().bind_int(_name, _value);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::bind_long),
                     _Type&,
                     const std::string&,
                     std::int64_t>::value>>
    void bind_long(const std::string& _name, std::int64_t _value)
    {
        impl().bind_long(_name, _value);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::bind_double),
                     _Type&,
                     const std::string&,
                     double>::value>>
    void bind_double(const std::string& _name, double _value)
    {
        impl().bind_double(_name, _value);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::bind_string),
                     _Type&,
                     const std::string&,
                     const std::string&>::value>>
    void bind_string(const std::string& _name,
                     const std::string& _value)
    {
        impl().bind_string(_name, _value);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::bind_binary),
                     _Type&,
                     const std::string&,
                     const std::vector<std::uint8_t>&>::value>>
    void bind_binary(const std::string&               _name,
                     const std::vector<std::uint8_t>& _value)
    {
        impl().bind_binary(_name, _value);
    }

    // -----------------------------------------------------------------
    // execution
    // -----------------------------------------------------------------

    auto execute_query()
    {
        return impl().execute_query();
    }

    std::int64_t execute_update()
    {
        return impl().execute_update();
    }

    bool execute()
    {
        return impl().execute();
    }

    // -----------------------------------------------------------------
    // metadata
    // -----------------------------------------------------------------

    std::size_t parameter_count() const
    {
        return impl().parameter_count();
    }

    void clear_parameters()
    {
        impl().clear_parameters();
    }

    void reset()
    {
        impl().reset();
    }

    void close()
    {
        impl().close();
    }

private:
    _helper& impl()
    {
        return static_cast<_helper&>(*this);
    }

    const _helper& impl() const
    {
        return static_cast<const _helper&>(*this);
    }
};


// =============================================================================
// VII. TRANSACTION TEMPLATE
// =============================================================================

// transaction
//   class template: RAII wrapper for database transactions using CRTP
// pattern.
template<typename _Connection>
class transaction
{
public:
    explicit transaction(_Connection& _conn)
        : m_connection(&_conn)
        , m_isolation_level(isolation_level::default_level)
        , m_active(false)
        , m_committed(false)
    {
        m_connection->begin_transaction();
        m_active = true;
    }

    explicit transaction(_Connection&  _conn,
                         isolation_level _isolation)
        : m_connection(&_conn)
        , m_isolation_level(_isolation)
        , m_active(false)
        , m_committed(false)
    {
        m_connection->begin_transaction(_isolation);
        m_active = true;
    }

    ~transaction()
    {
        if ( (m_active) &&
             (!m_committed) )
        {
            try
            {
                m_connection->rollback();
            }
            catch (...)
            {
                // suppress exceptions in destructor
            }
        }
    }

    // disable copying
    transaction(const transaction&)            = delete;
    transaction& operator=(const transaction&) = delete;

    // enable moving
    transaction(transaction&& _other) noexcept
        : m_connection(_other.m_connection)
        , m_isolation_level(_other.m_isolation_level)
        , m_active(_other.m_active)
        , m_committed(_other.m_committed)
    {
        _other.m_connection = nullptr;
        _other.m_active     = false;
    }

    transaction& operator=(transaction&& _other) noexcept
    {
        if (this != &_other)
        {
            if ( (m_active) &&
                 (!m_committed) )
            {
                try
                {
                    m_connection->rollback();
                }
                catch (...)
                {
                    // suppress exceptions
                }
            }

            m_connection      = _other.m_connection;
            m_isolation_level = _other.m_isolation_level;
            m_active          = _other.m_active;
            m_committed       = _other.m_committed;

            _other.m_connection = nullptr;
            _other.m_active     = false;
        }

        return *this;
    }

    void commit()
    {
        if (!m_active)
        {
            throw transaction_exception(
                "transaction is not active");
        }

        m_connection->commit();
        m_committed = true;
        m_active    = false;
    }

    void rollback()
    {
        if (!m_active)
        {
            throw transaction_exception(
                "transaction is not active");
        }

        m_connection->rollback();
        m_active = false;
    }

    template<typename _Type = _Connection,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::create_savepoint),
                     _Type&,
                     const std::string&>::value>>
    void create_savepoint(const std::string& _name)
    {
        if (!m_active)
        {
            throw transaction_exception(
                "transaction is not active");
        }

        m_connection->create_savepoint(_name);
    }

    template<typename _Type = _Connection,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::rollback_to_savepoint),
                     _Type&,
                     const std::string&>::value>>
    void rollback_to_savepoint(const std::string& _name)
    {
        if (!m_active)
        {
            throw transaction_exception(
                "transaction is not active");
        }

        m_connection->rollback_to_savepoint(_name);
    }

    template<typename _Type = _Connection,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::release_savepoint),
                     _Type&,
                     const std::string&>::value>>
    void release_savepoint(const std::string& _name)
    {
        if (!m_active)
        {
            throw transaction_exception(
                "transaction is not active");
        }

        m_connection->release_savepoint(_name);
    }

    bool is_active() const noexcept
    {
        return m_active;
    }

    isolation_level get_isolation_level() const noexcept
    {
        return m_isolation_level;
    }

private:
    _Connection*    m_connection;
    isolation_level m_isolation_level;
    bool            m_active;
    bool            m_committed;
};


// =============================================================================
// VIII. CONNECTION TEMPLATE
// =============================================================================

// connection
//   class template: base wrapper for database connections using CRTP
// pattern.
template<typename _helper>
class connection
{
public:
    connection()
        : m_state(connection_state::disconnected)
        , m_in_transaction(false)
        , m_auto_commit(true)
    {};

    ~connection() = default;

    connection(const connection&)            = delete;
    connection& operator=(const connection&) = delete;

    connection(connection&&) noexcept            = default;
    connection& operator=(connection&&) noexcept = default;

    // -----------------------------------------------------------------
    // connection management
    // -----------------------------------------------------------------

    void connect()
    {
        impl().connect();
    }

    void disconnect()
    {
        impl().disconnect();
    }

    void reconnect()
    {
        impl().reconnect();
    }

    bool is_connected() const noexcept
    {
        return impl().is_connected();
    }

    bool ping() const
    {
        return impl().ping();
    }

    // -----------------------------------------------------------------
    // query execution
    // -----------------------------------------------------------------

    auto execute_query(const std::string& _query)
    {
        return impl().execute_query(_query);
    }

    std::int64_t execute_update(const std::string& _query)
    {
        return impl().execute_update(_query);
    }

    bool execute(const std::string& _query)
    {
        return impl().execute(_query);
    }

    // -----------------------------------------------------------------
    // prepared statements
    // -----------------------------------------------------------------

    auto prepare(const std::string& _query)
    {
        return impl().prepare(_query);
    }

    // -----------------------------------------------------------------
    // transaction management
    // -----------------------------------------------------------------

    void begin_transaction()
    {
        impl().begin_transaction();
        m_in_transaction = true;
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::begin_transaction),
                     _Type&,
                     isolation_level>::value>>
    void begin_transaction(isolation_level _isolation)
    {
        impl().begin_transaction(_isolation);
        m_in_transaction = true;
    }

    void commit()
    {
        impl().commit();
        m_in_transaction = false;
    }

    void rollback()
    {
        impl().rollback();
        m_in_transaction = false;
    }

    bool in_transaction() const noexcept
    {
        return m_in_transaction;
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable_r<
                     isolation_level,
                     decltype(&_Type::get_isolation_level),
                     const _Type&>::value>>
    isolation_level get_isolation_level() const
    {
        return impl().get_isolation_level();
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::set_isolation_level),
                     _Type&,
                     isolation_level>::value>>
    void set_isolation_level(isolation_level _level)
    {
        impl().set_isolation_level(_level);
    }

    // -----------------------------------------------------------------
    // savepoints (if supported)
    // -----------------------------------------------------------------

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::create_savepoint),
                     _Type&,
                     const std::string&>::value>>
    void create_savepoint(const std::string& _name)
    {
        impl().create_savepoint(_name);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::rollback_to_savepoint),
                     _Type&,
                     const std::string&>::value>>
    void rollback_to_savepoint(const std::string& _name)
    {
        impl().rollback_to_savepoint(_name);
    }

    template<typename _Type = _helper,
             typename    = std::enable_if_t<
                 djinterp::is_invocable<
                     decltype(&_Type::release_savepoint),
                     _Type&,
                     const std::string&>::value>>
    void release_savepoint(const std::string& _name)
    {
        impl().release_savepoint(_name);
    }

    // -----------------------------------------------------------------
    // metadata
    // -----------------------------------------------------------------

    database_type get_database_type() const noexcept
    {
        return impl().get_database_type();
    }

    std::string get_database_name() const
    {
        return impl().get_database_name();
    }

    std::string get_server_version() const
    {
        return impl().get_server_version();
    }

    connection_state get_state() const noexcept
    {
        return m_state;
    }

    std::int64_t get_last_insert_id() const
    {
        return impl().get_last_insert_id();
    }

    std::int64_t get_affected_rows() const
    {
        return impl().get_affected_rows();
    }

    // -----------------------------------------------------------------
    // configuration
    // -----------------------------------------------------------------

    const connection_config& get_config() const noexcept
    {
        return m_config;
    }

    void set_auto_commit(bool _enabled)
    {
        impl().set_auto_commit(_enabled);
        m_auto_commit = _enabled;
    }

    bool get_auto_commit() const noexcept
    {
        return m_auto_commit;
    }

    // -----------------------------------------------------------------
    // error handling
    // -----------------------------------------------------------------

    std::string get_last_error() const
    {
        return impl().get_last_error();
    }

    int get_last_error_code() const noexcept
    {
        return impl().get_last_error_code();
    }

protected:
    connection_config m_config;
    connection_state  m_state;
    bool              m_in_transaction;
    bool              m_auto_commit;

private:
    _helper& impl()
    {
        return static_cast<_helper&>(*this);
    }

    const _helper& impl() const
    {
        return static_cast<const _helper&>(*this);
    }
};


// =============================================================================
// IX.  CONNECTION POOL TEMPLATE
// =============================================================================

// forward declaration - connection_pool is defined below but
// referenced by pooled_connection's constructor and m_pool member.
template<typename _Connection>
class connection_pool;

// pooled_connection
//   class template: RAII wrapper for pooled connections.
template<typename _Connection>
class pooled_connection
{
public:
    explicit pooled_connection(
        connection_pool<_Connection>& _pool,
        std::unique_ptr<_Connection>  _connection)
        : m_pool(&_pool),
          m_connection(std::move(_connection))
    {}

    ~pooled_connection()
    {
        release();
    }

    // disable copying
    pooled_connection(const pooled_connection&)            = delete;
    pooled_connection& operator=(const pooled_connection&) = delete;

    // enable moving
    pooled_connection(pooled_connection&& _other) noexcept
        : m_pool(_other.m_pool),
          m_connection(std::move(_other.m_connection))
    {
        _other.m_pool = nullptr;
    }

    pooled_connection& operator=(pooled_connection&& _other) noexcept
    {
        if (this != &_other)
        {
            release();

            m_pool       = _other.m_pool;
            m_connection = std::move(_other.m_connection);

            _other.m_pool = nullptr;
        }

        return *this;
    }

    _Connection* get() noexcept
    {
        return m_connection.get();
    }

    const _Connection* get() const noexcept
    {
        return m_connection.get();
    }

    _Connection* operator->() noexcept
    {
        return m_connection.get();
    }

    const _Connection* operator->() const noexcept
    {
        return m_connection.get();
    }

    _Connection& operator*()
    {
        return *m_connection;
    }

    const _Connection& operator*() const
    {
        return *m_connection;
    }

    void 
    release()
    {
        if ( (m_pool) &&
             (m_connection) )
        {
            m_pool->release(std::move(m_connection));
            m_pool       = nullptr;
            m_connection = nullptr;
        }
    }

private:
    connection_pool<_Connection>* m_pool;   // was: void*
    std::unique_ptr<_Connection>  m_connection;
};

// connection_pool
//   class template: connection pooling for efficient resource
// management.
template<typename _Connection>
class connection_pool
{
public:
    using connection_factory =
        std::function<std::unique_ptr<_Connection>()>;

    explicit connection_pool(
        connection_factory  _factory,
        const pool_config&  _config)
        : m_factory(std::move(_factory))
        , m_config(_config)
    {
        // pre-allocate minimum connections
        for (std::size_t i = 0; i < m_config.min_connections; ++i)
        {
            m_idle_connections.push_back(create_connection());
        }
    }

    ~connection_pool()
    {
        clear();
    }

    // disable copying
    connection_pool(const connection_pool&)            = delete;
    connection_pool& operator=(const connection_pool&) = delete;

    // -----------------------------------------------------------------
    // connection acquisition
    // -----------------------------------------------------------------

    pooled_connection<_Connection> acquire()
    {
        std::unique_ptr<_Connection> conn;

        // try to get from idle pool
        if (!m_idle_connections.empty())
        {
            conn = std::move(m_idle_connections.back());
            m_idle_connections.pop_back();

            // validate if needed
            if ( (m_config.validate_on_acquire) &&
                 (!validate_connection(conn.get())) )
            {
                conn = create_connection();
            }
        }
        else if (total_connections() < m_config.max_connections)
        {
            // create new connection
            conn = create_connection();
        }
        else
        {
            throw connection_exception(
                "connection pool exhausted");
        }

        m_active_connections.push_back(conn.get());

        return pooled_connection<_Connection>(
            *this, std::move(conn));
    }

    void release(std::unique_ptr<_Connection> _conn)
    {
        if (!_conn)
        {
            return;
        }

        // remove from active list
        auto it = std::find(m_active_connections.begin(),
                            m_active_connections.end(),
                            _conn.get());

        if (it != m_active_connections.end())
        {
            m_active_connections.erase(it);
        }

        // validate if needed
        if ( (m_config.validate_on_return) &&
             (!validate_connection(_conn.get())) )
        {
            return;
        }

        // return to idle pool if under max
        if (m_idle_connections.size() < m_config.max_connections)
        {
            m_idle_connections.push_back(std::move(_conn));
        }
    }

    // -----------------------------------------------------------------
    // pool management
    // -----------------------------------------------------------------

    void clear()
    {
        m_idle_connections.clear();
        m_active_connections.clear();
    }

    void resize(std::size_t _min, std::size_t _max)
    {
        m_config.min_connections = _min;
        m_config.max_connections = _max;

        // adjust pool size
        while (m_idle_connections.size() < _min)
        {
            m_idle_connections.push_back(create_connection());
        }

        while (m_idle_connections.size() > _max)
        {
            m_idle_connections.pop_back();
        }
    }

    std::size_t active_connections() const noexcept
    {
        return m_active_connections.size();
    }

    std::size_t idle_connections() const noexcept
    {
        return m_idle_connections.size();
    }

    std::size_t total_connections() const noexcept
    {
        return active_connections() + idle_connections();
    }

    std::size_t waiting_requests() const noexcept
    {
        return 0;
    }

    // -----------------------------------------------------------------
    // configuration
    // -----------------------------------------------------------------

    const pool_config& get_config() const noexcept
    {
        return m_config;
    }

    void set_config(const pool_config& _config)
    {
        m_config = _config;
    }

protected:
    std::unique_ptr<_Connection> create_connection()
    {
        auto conn = m_factory();

        if (!conn)
        {
            throw connection_exception(
                "failed to create connection");
        }

        conn->connect();

        return conn;
    }

    bool validate_connection(_Connection* _conn)
    {
        if (!_conn)
        {
            return false;
        }

        try
        {
            return _conn->ping();
        }
        catch (...)
        {
            return false;
        }
    }

    void cleanup_idle_connections()
    {
        // remove idle connections that exceed idle timeout
        // implementation depends on tracking last use time
    }

private:
    connection_factory                               m_factory;
    pool_config                                      m_config;
    std::vector<std::unique_ptr<_Connection>>        m_idle_connections;
    std::vector<_Connection*>                        m_active_connections;
};


// =============================================================================
// X.   UTILITY FUNCTIONS
// =============================================================================

// parse_connection_string
//   function: parses a database connection string into
// configuration.
connection_config parse_connection_string(
    const std::string& _connection_string);

// build_connection_string
//   function: builds a connection string from configuration.
std::string build_connection_string(
    const connection_config& _config);

// escape_string
//   function: escapes special characters in a string for SQL
// safety.
std::string escape_string(const std::string& _input);

// quote_identifier
//   function: quotes a database identifier (table/column name) for
// safety.
std::string quote_identifier(const std::string& _identifier,
                             database_type      _database_type);

// value_to_string
//   function: converts a value to string representation.
std::string value_to_string(const value& _value);

// string_to_value
//   function: parses a string into a value with type detection.
value string_to_value(const std::string& _str,
                      field_type         _type);

// get_type_name
//   function: returns human-readable name for database type.
std::string get_type_name(database_type _type);

// get_field_type_name
//   function: returns human-readable name for field type.
std::string get_field_type_name(field_type _type);



D_INLINE connection_config::connection_config()
    : host("localhost"),
      port(0),
      connect_timeout(std::chrono::seconds(30)),
      read_timeout(std::chrono::seconds(30)),
      write_timeout(std::chrono::seconds(30)),
      auto_reconnect(false),
      enable_ssl(false),
      verify_ssl(true),
      max_packet_size(16 * 1024 * 1024)
{}

D_INLINE pool_config::pool_config()
    : min_connections(1),
      max_connections(10),
      connection_queue_size(100),
      connection_timeout(std::chrono::seconds(30)),
      idle_timeout(std::chrono::minutes(10)),
      max_lifetime(std::chrono::hours(1)),
      validate_on_acquire(false),
      validate_on_return(false)
{}

D_INLINE exception::exception(
    const std::string& _message
)
    : std::runtime_error(_message),
      m_message(_message),
      m_error_code(0)
{}

D_INLINE exception::exception(
    const char* _message
)
    : std::runtime_error(_message),
      m_message(_message),
      m_error_code(0)
{}

D_INLINE const char*
exception::what() const noexcept
{
    return m_message.c_str();
}

D_INLINE int 
exception::error_code() const noexcept
{
    return m_error_code;
}

NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_COMMON_