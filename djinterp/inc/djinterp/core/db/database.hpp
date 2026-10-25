/*******************************************************************************
* djinterp [core]                                                   database.hpp
*
* The vocabulary every djinterp C++17 database connection shares, and the
* interfaces each one presents.
*   A value is a std::variant -- NULL, a boolean, a 32- or 64-bit integer, a
* double, text, bytes or a point in time -- and a row maps column names to
* values. A failure throws: exception, or one of the four kinds derived from
* it, each carrying error_code(), the result of the layer call that failed,
* so that nothing the database said is lost on the way.
*   The interfaces are CRTP bases -- connection, statement, result_set -- each
* forwarding to the class that derives from it, so code written against
* connection<Derived> runs on any vendor's connection with no virtual call.
* database_connection.hpp implements connection's operations once for every
* vendor; database_statement.hpp implements statement and result_set over
* the C layers' prepared statements and rows. transaction is the scope guard
* over any connection, connection_pool a pool of them.
*   The utilities read and write connection strings, quote and escape for
* SQL, and convert values to text and back, the same under every locale.
* They are defined in database.cpp, which every program that uses core/db
* builds with it.
*   Every database header builds on this one:
*     database.hpp                       this vocabulary and these interfaces
*       database_connection_template.hpp each vendor's compile-time traits
*         database_statement.hpp         statements and result sets over the
*                                        C layers
*           database_connection.hpp      the connection, once for every vendor
*             <vendor>.hpp               a vendor's connection
*   database_table.hpp builds on this header too. The header needs C++17
* (std::optional, std::variant) and below it compiles to nothing.
*
*
* path:      /inc/djinterp/core/db/database.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  VOCABULARY
    ----------
    1.  Enumerations
         1.  database_type
         2.  isolation_level
         3.  connection_state
         4.  field_type
    2.  Values
         1.  value
         2.  parameter_map
         3.  row
         4.  result_rows
2.  EXCEPTIONS
    ----------
    1.  Exceptions
         1.  exception
         2.  connection_exception
         3.  query_exception
         4.  transaction_exception
         5.  type_conversion_exception
    2.  Refusals
         1.  internal::refuse_nul
         2.  internal::refuse_nul_setting
         3.  internal::relabel
3.  CONFIGURATION
    -------------
    1.  Connections
         1.  connection_config
         2.  pool_config
    2.  Vendors
         1.  vendor_info
         2.  vendor_traits
4.  INTERFACES
    ----------
    1.  Hooks
         1.  internal hook detectors
         2.  interface probes
    2.  Result sets
         1.  result_set
    3.  Statements
         1.  statement
    4.  Transactions
         1.  transaction
    5.  Connections
         1.  connection
    6.  Pools
         1.  pooled_connection
         2.  connection_pool
5.  DETECTION
    ---------
    1.  Connection interface
         1.  connection_connect_t
         2.  has_connect
         3.  is_connection
         4.  is_vendor_connection
         5.  is_connectable
         6.  can_connect
6.  UTILITIES
    ---------
    1.  Connection strings
    2.  Quoting and escaping
    3.  Values as text
    4.  Names
7.  SERIALIZABILITY
    ---------------
    1.  The store target
         1.  store_direction
         2.  store_table
         3.  load_table
*/

#ifndef DJINTERP_DB_DATABASE_HPP
#define DJINTERP_DB_DATABASE_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>    // std::find_if
#include <chrono>       // std::chrono::milliseconds, system_clock,
                        // steady_clock
#include <cstddef>      // std::size_t, std::ptrdiff_t
#include <functional>   // std::function
#include <map>          // std::map
#include <memory>       // std::unique_ptr
#include <optional>     // std::optional
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string
#include <type_traits>  // std::enable_if_t, std::is_same_v
#include <utility>      // std::declval, std::move
#include <variant>      // std::variant, std::monostate
#include <vector>       // std::vector
// djinterp
#include "../../djinterp.hpp"       // framework root
#include "../../c/db/db_common.h"   // D_DB_INVALID_ARGUMENT, the D_DB_* codes
#include "../meta/type_traits.hpp"  // is_detected, conjunction
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int32_t, int64_t,
                                                // uint8_t, uint16_t


NS_DJINTERP


//==============================================================================
// 1.  VOCABULARY
//==============================================================================


// 1.1    Enumerations
//------------------------------------------------------------------------------
// 1.1.1
// database_type
//   enum: a database vendor. The values are identifiers, not bit flags: for
// compile-time flag queries, env_db.h's D_ENV_DB_FLAG_* macros. MSSQL,
// Firebase, CouchDB and Neo4j are reserved: no connection stands behind them.
enum class database_type : re_std::uint16_t
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
    neo4j      = 0x0E,
    dynamodb   = 0x0F
};

// 1.1.2
// isolation_level
//   enum: the SQL standard's transaction isolation levels, and the
// connection's default. A connection may run a transaction at a stronger
// level than the one asked for, as the standard allows: SQLite runs every
// one serializable, PostgreSQL a read-uncommitted one read committed.
enum class isolation_level
{
    read_uncommitted,
    read_committed,
    repeatable_read,
    serializable,
    default_level
};

// 1.1.3
// connection_state
//   enum: where a connection is in its life. `error` is a connection the
// database dropped, or one that failed to open; reconnect() leaves it.
enum class connection_state
{
    disconnected,
    connecting,
    connected,
    executing,
    error,
    closed
};

// 1.1.4
// field_type
//   enum: what a column holds, independent of any vendor. From `boolean` to
// `uuid` it mirrors the C layers' enum d_db_type member for member; `null`
// is a column of no type, `custom` one whose type maps to none of these.
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

// 1.2    Values
//------------------------------------------------------------------------------
// 1.2.1
// value
//   type: one value of one row, or of one parameter: NULL (std::monostate),
// a boolean, a 32- or 64-bit integer, a double, text, bytes, or a point in
// time, which core/db writes and reads as UTC.
using value = std::variant<
    std::monostate,                         // NULL
    bool,                                   // boolean
    re_std::int32_t,                        // integer
    re_std::int64_t,                        // big integer
    double,                                 // floating point
    std::string,                            // text
    std::vector<re_std::uint8_t>,           // bytes
    std::chrono::system_clock::time_point   // a point in time
>;

// 1.2.2
// parameter_map
//   type: values by parameter name.
using parameter_map = std::map<std::string, value>;

// 1.2.3
// row
//   type: one row's values by column name.
using row = std::map<std::string, value>;

// 1.2.4
// result_rows
//   type: rows, in the order a query returned them.
using result_rows = std::vector<row>;


//==============================================================================
// 2.  EXCEPTIONS
//==============================================================================
// core/db throws what fails, as the faces return it: each exception carries
// the result of the layer call that failed -- an SQLite result code, a MySQL
// client's error number, a PostgreSQL SQLSTATE packed as the layer packs it,
// or a negative D_DB_* (db_common.h) -- beside a message that says what
// failed and why, in the database's words where it gave some. A refusal of
// core/db's own carries the D_DB_* code that names it, D_DB_INVALID_ARGUMENT
// for an argument that cannot be right, or 0 where none does.


// 2.1    Exceptions
//------------------------------------------------------------------------------
// 2.1.1
// exception
//   class: what every core/db failure throws, or derives from: a message,
// what(), and error_code(), the result of the call that failed.
class exception : public std::runtime_error
{
public:
    /**
     * @brief Makes an exception from a message and the result that failed.
     *
     * @param[in] _message     what failed, and why.
     * @param[in] _error_code  the layer's result, or a D_DB_* code; 0 for
     *                         none.
     */
    explicit exception(const std::string& _message,
                       int                _error_code = 0);
    explicit exception(const char* _message,
                       int         _error_code = 0);

    // error_code -- the result of the call that failed, or 0 for none
    int error_code() const noexcept;

private:
    int m_error_code;
};

// 2.1.2
// connection_exception
//   class: a connection that could not be made, or that the database
// dropped; the connection is then in connection_state::error.
class connection_exception : public exception
{
public:
    using exception::exception;
};

// 2.1.3
// query_exception
//   class: a statement that could not be prepared, bound or run.
class query_exception : public exception
{
public:
    using exception::exception;
};

// 2.1.4
// transaction_exception
//   class: a transaction that could not begin, commit or roll back, or a
// savepoint that could not be made, released or returned to.
class transaction_exception : public exception
{
public:
    using exception::exception;
};

// 2.1.5
// type_conversion_exception
//   class: a value read as a type it neither is nor converts to exactly;
// error_code() is D_DB_MISMATCH.
class type_conversion_exception : public exception
{
public:
    using exception::exception;
};

// 2.2    Refusals
//------------------------------------------------------------------------------
// Every layer takes text as a C string, which a NUL character would end
// early: a statement would run cut short, a file or a name other than the
// one meant would be opened. core/db refuses such text before any layer
// sees it, with D_DB_INVALID_ARGUMENT.
NS_INTERNAL

    // 2.2.1
    // refuse_nul
    //   function: throws query_exception with D_DB_INVALID_ARGUMENT, naming
    // the operation and what was refused, when the text holds a NUL.
    inline void
    refuse_nul(
        const std::string& _text,
        const char*        _operation,
        const char*        _what = "the text"
    )
    {
        // a C string ends at its first NUL
        if (_text.find('\0') != std::string::npos)
        {
            throw query_exception(std::string(_operation) + ": " + _what +
                                      " holds a NUL character",
                                  D_DB_INVALID_ARGUMENT);
        }

        return;
    }

    // 2.2.2
    // refuse_nul_setting
    //   function: refuse_nul for a setting a connect passes on, which throws
    // connection_exception, as a connect does.
    inline void
    refuse_nul_setting(
        const std::string& _text,
        const char*        _operation,
        const char*        _what
    )
    {
        // a C string ends at its first NUL
        if (_text.find('\0') != std::string::npos)
        {
            throw connection_exception(std::string(_operation) + ": " +
                                           _what + " holds a NUL character",
                                       D_DB_INVALID_ARGUMENT);
        }

        return;
    }

    // 2.2.3
    // relabel
    /**
     * @brief Throws a core/db exception again, under the name of the
     *        operation that ran another's hook, so that what fails names
     *        the operation the program called.
     *
     * @note The message's label is its start, or the word after the
     *       vendor's name, before ": ". Where it is `_from` it becomes `_to`;
     *       anything else is thrown again as it is.
     *
     * @param[in] _e     the exception.
     * @param[in] _from  the label it carries.
     * @param[in] _to    the label it is to carry.
     * @throws an exception of `_e`'s type, connection_exception,
     *         query_exception, transaction_exception or
     *         type_conversion_exception, with `_e`'s error_code().
     */
    [[noreturn]] void relabel(const exception& _e,
                              const char*      _from,
                              const char*      _to);

NS_END  // internal


//==============================================================================
// 3.  CONFIGURATION
//==============================================================================


// 3.1    Connections
//------------------------------------------------------------------------------
// 3.1.1
// connection_config
//   struct: what any vendor needs to connect, in its own terms where it has
// them: SQLite reads `database` as the file. A connection never reconnects
// on its own, so `auto_reconnect` must stay false: a program reconnects with
// reconnect(), which says that it did. Timeouts of 0 leave the vendor's own,
// which for reading and writing is none: a query may take as long as it
// takes. custom_options are settings a vendor has no field for, by the
// vendor's own names: PostgreSQL gives each to libpq as a connection
// keyword; SQLite and the MySQL family have nothing to give them to, and
// refuse any at connect. "scheme", under which parse_connection_string
// records a URI's scheme, is no setting, and every vendor leaves it out.
struct connection_config
{
    std::string      host;      // "localhost" by default
    re_std::uint16_t port;      // 0: the vendor's default
    std::string      database;
    std::string      username;
    std::string      password;

    std::optional<std::string> charset;
    std::optional<std::string> schema;
    std::optional<std::string> ssl_cert;
    std::optional<std::string> ssl_key;
    std::optional<std::string> ssl_ca;

    std::chrono::milliseconds connect_timeout;  // 30 s by default
    std::chrono::milliseconds read_timeout;     // 0: none, by default
    std::chrono::milliseconds write_timeout;    // 0: none, by default

    bool        auto_reconnect;   // false; true is refused (above)
    bool        enable_ssl;       // false
    bool        verify_ssl;       // true: with enable_ssl, verify the server
    std::size_t max_packet_size;  // 16 MiB

    std::map<std::string, std::string> custom_options;

    connection_config();
};

// 3.1.2
// pool_config
//   struct: how many connections a connection_pool keeps, how long, and when
// it checks them. A duration of 0 is no limit. connection_queue_size and
// connection_timeout are a waiting pool's; connection_pool never waits,
// since it is not thread-safe, and reads neither.
struct pool_config
{
    std::size_t min_connections;        // 1: made up front
    std::size_t max_connections;        // 10: lent and idle together
    std::size_t connection_queue_size;  // 100: not read

    std::chrono::milliseconds connection_timeout;  // 30 s: not read
    std::chrono::milliseconds idle_timeout;        // 10 min: then retired,
                                                   // down to the minimum
    std::chrono::milliseconds max_lifetime;        // 1 h: then retired

    bool validate_on_acquire;  // false
    bool validate_on_return;   // false

    std::string validation_query;  // empty: a check is a ping

    pool_config();
};

// 3.2    Vendors
//------------------------------------------------------------------------------
// 3.2.1
// vendor_info
//   struct: a vendor's identity, at run time, whatever the connection's type.
struct vendor_info
{
    database_type    type;
    std::string      name;
    std::string      display_name;
    re_std::uint16_t default_port;
    bool             is_relational;
    bool             is_embedded;
    bool             supports_ssl;

    vendor_info()
        : type(database_type::unknown),
          default_port(0),
          is_relational(false),
          is_embedded(false),
          supports_ssl(false)
    {}
};

// 3.2.2
// vendor_traits
//   trait: a vendor's compile-time identity: its native handle type, default
// port and names. database_connection_template.hpp specializes it for each
// vendor; this primary template describes an unknown one.
template<database_type DbType>
struct vendor_traits
{
    // native_handle_type
    //   type: the vendor's connection handle; void* for an unknown vendor.
    using native_handle_type = void*;

    static constexpr database_type    db_type       = DbType;
    static constexpr re_std::uint16_t default_port  = 0;
    static constexpr const char*      name          = "unknown";
    static constexpr const char*      display_name  = "Unknown Database";
    static constexpr bool             is_relational = false;
    static constexpr bool             is_embedded   = false;
    static constexpr bool             supports_ssl  = false;

    // get_info -- the traits as a vendor_info
    static vendor_info
    get_info()
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

    // make_default_config -- a configuration with the vendor's defaults
    static connection_config
    make_default_config()
    {
        connection_config config;

        config.host = "localhost";
        config.port = default_port;

        return config;
    }
};


//==============================================================================
// 4.  INTERFACES
//==============================================================================
// Each interface is a CRTP base: its members forward to the class Derived
// that derives from it, so that a call through the base costs what a direct
// call does. No base's destructor is virtual, so nothing may be deleted
// through its base, which would skip the derived class's destructor:
// connections, statements and result sets are handed out as std::unique_ptr
// to the derived class. connection's and statement's destructors are
// protected, which makes such a deletion a compile-time error; result_set's
// stays public while the Db2, DynamoDB and Redis skeletons, which no layer
// stands behind yet, declare members that return std::unique_ptr<result_set<
// ...>>.


// 4.1    Hooks
//------------------------------------------------------------------------------
// 4.1.1
// internal hook detectors
//   type: the calls that tell whether a connection class can do what the
// gated operations below need. database_connection (database_connection.hpp)
// implements each operation over a hook of the derived class, named after it
// with the suffix `_impl`; an operation exists where its hook does.
NS_INTERNAL

    // interface_probe
    //   type: the first template argument of every member of the
    // interfaces below, which no member of a class derived from one takes:
    // naming it tells the interface's member from the derived class's.
    struct interface_probe
    {};

    template<typename Type>
    using begin_transaction_level_hook_t =
        decltype(std::declval<Type&>().begin_transaction_impl(
            std::declval<isolation_level>()));

    template<typename Type>
    using get_isolation_level_hook_t =
        decltype(std::declval<const Type&>().get_isolation_level_impl());

    template<typename Type>
    using set_isolation_level_hook_t =
        decltype(std::declval<Type&>().set_isolation_level_impl(
            std::declval<isolation_level>()));

    template<typename Type>
    using create_savepoint_hook_t =
        decltype(std::declval<Type&>().create_savepoint_impl(
            std::declval<const std::string&>()));

    template<typename Type>
    using release_savepoint_hook_t =
        decltype(std::declval<Type&>().release_savepoint_impl(
            std::declval<const std::string&>()));

    template<typename Type>
    using rollback_to_savepoint_hook_t =
        decltype(std::declval<Type&>().rollback_to_savepoint_impl(
            std::declval<const std::string&>()));

    template<typename Type>
    using set_auto_commit_hook_t =
        decltype(std::declval<Type&>().set_auto_commit_impl(
            std::declval<bool>()));

    template<typename Type>
    using get_auto_commit_hook_t =
        decltype(std::declval<const Type&>().get_auto_commit_impl());

    template<typename Type>
    using in_transaction_hook_t =
        decltype(std::declval<const Type&>().in_transaction_impl());

    // face_hook_t -- the face of a vendor with a layer, through which
    // run_layer and ask_layer reach the layer's calls
    template<typename Type>
    using face_hook_t =
        decltype(std::declval<const Type&>().get_face());

    // pool_execute_t -- a pooled connection that runs a statement, which
    // connection_pool then checks with the configuration's validation_query
    template<typename Type>
    using pool_execute_t =
        decltype(std::declval<Type&>().execute(
            std::declval<const std::string&>()));

    // result_absolute_t, ... -- the members a result set or a statement
    // offers beyond what every one has, as the call
    template<typename Type>
    using result_absolute_t =
        decltype(std::declval<Type&>().absolute(
            std::declval<re_std::int64_t>()));

    template<typename Type>
    using statement_bind_value_t =
        decltype(std::declval<Type&>().bind_value(
            std::declval<std::size_t>(),
            std::declval<const value&>()));

    template<typename Type>
    using statement_named_t =
        decltype(std::declval<Type&>().bind_null(
            std::declval<const std::string&>()));

    // result_absolute_base_t, ... -- the same calls made through the
    // interface's own gated member template, its gate given rather than
    // asked: well formed exactly where Type declares no member of the name,
    // so that the name still reaches the interface's template
    template<typename Type>
    using result_absolute_base_t =
        decltype(std::declval<Type&>()
                     .template absolute<interface_probe, Type, void>(
                         std::declval<re_std::int64_t>()));

    template<typename Type>
    using statement_bind_value_base_t =
        decltype(std::declval<Type&>()
                     .template bind_value<interface_probe, Type, void>(
                         std::declval<std::size_t>(),
                         std::declval<const value&>()));

    template<typename Type>
    using statement_named_base_t =
        decltype(std::declval<Type&>()
                     .template bind_null<interface_probe, Type, void>(
                         std::declval<const std::string&>()));

    // has_own
    //   trait: whether Type's own member answers Call, Base telling first
    // whether Type declares a member of the name at all. Call alone would
    // reach the interface's gated template where Type declares none, and
    // that template's gate asks Call again, without end: a hard error where
    // a false is meant. conjunction stops at Base.
    template<template<typename...> class Base,
             template<typename...> class Call,
             typename                    Type>
    struct has_own : conjunction<negation<is_detected<Base, Type>>,
                                 is_detected<Call, Type>>
    {};

    // result_absolute_gate_t, ... -- the gates: void where Type's own member
    // answers the call, and no type otherwise
    template<typename Type>
    using result_absolute_gate_t =
        std::enable_if_t<has_own<result_absolute_base_t,
                                 result_absolute_t,
                                 Type>::value>;

    template<typename Type>
    using statement_bind_value_gate_t =
        std::enable_if_t<has_own<statement_bind_value_base_t,
                                 statement_bind_value_t,
                                 Type>::value>;

    template<typename Type>
    using statement_named_gate_t =
        std::enable_if_t<has_own<statement_named_base_t,
                                 statement_named_t,
                                 Type>::value>;

    // 4.1.2
    // interface probes
    //   type: for each member an interface forwards, the call made
    // through the interface's own forwarder, interface_probe given
    // first, and every gate given rather than asked: well formed exactly
    // where Type declares no member of the name, so that the name's
    // lookup reaches the interface. Each forwarder asserts that its probe
    // is not: a member Derived lacks would otherwise reach the forwarder
    // again, and call it without end, at run time.
    template<typename Type>
    using result_set_next_probe_t =
        decltype(std::declval<Type&>().template next<interface_probe>());

    template<typename Type>
    using result_set_previous_probe_t =
        decltype(std::declval<Type&>().template previous<interface_probe>());

    template<typename Type>
    using result_set_first_probe_t =
        decltype(std::declval<Type&>().template first<interface_probe>());

    template<typename Type>
    using result_set_last_probe_t =
        decltype(std::declval<Type&>().template last<interface_probe>());

    template<typename Type>
    using result_set_absolute_probe_t =
        decltype(std::declval<Type&>()
                     .template absolute<interface_probe, Type, void>(
                         std::declval<re_std::int64_t>()));

    template<typename Type>
    using result_set_column_count_probe_t =
        decltype(std::declval<Type&>()
                     .template column_count<interface_probe>());

    template<typename Type>
    using result_set_column_name_probe_t =
        decltype(std::declval<Type&>().template column_name<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_column_type_probe_t =
        decltype(std::declval<Type&>().template column_type<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_column_index_probe_t =
        decltype(std::declval<Type&>().template column_index<interface_probe>(
            std::declval<const std::string&>()));

    template<typename Type>
    using result_set_row_count_probe_t =
        decltype(std::declval<Type&>().template row_count<interface_probe>());

    template<typename Type>
    using result_set_current_row_probe_t =
        decltype(std::declval<Type&>().template current_row<interface_probe>());

    template<typename Type>
    using result_set_get_value_probe_t =
        decltype(std::declval<Type&>().template get_value<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_is_null_probe_t =
        decltype(std::declval<Type&>().template is_null<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_get_bool_probe_t =
        decltype(std::declval<Type&>().template get_bool<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_get_int_probe_t =
        decltype(std::declval<Type&>().template get_int<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_get_long_probe_t =
        decltype(std::declval<Type&>().template get_long<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_get_double_probe_t =
        decltype(std::declval<Type&>().template get_double<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_get_string_probe_t =
        decltype(std::declval<Type&>().template get_string<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_get_binary_probe_t =
        decltype(std::declval<Type&>().template get_binary<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using result_set_get_current_row_probe_t =
        decltype(std::declval<Type&>()
                     .template get_current_row<interface_probe>());

    template<typename Type>
    using result_set_get_all_rows_probe_t =
        decltype(std::declval<Type&>()
                     .template get_all_rows<interface_probe>());

    template<typename Type>
    using result_set_close_probe_t =
        decltype(std::declval<Type&>().template close<interface_probe>());

    template<typename Type>
    using statement_bind_null_probe_t =
        decltype(std::declval<Type&>().template bind_null<interface_probe>(
            std::declval<std::size_t>()));

    template<typename Type>
    using statement_bind_bool_probe_t =
        decltype(std::declval<Type&>().template bind_bool<interface_probe>(
            std::declval<std::size_t>(),
            std::declval<bool>()));

    template<typename Type>
    using statement_bind_int_probe_t =
        decltype(std::declval<Type&>().template bind_int<interface_probe>(
            std::declval<std::size_t>(),
            std::declval<re_std::int32_t>()));

    template<typename Type>
    using statement_bind_long_probe_t =
        decltype(std::declval<Type&>().template bind_long<interface_probe>(
            std::declval<std::size_t>(),
            std::declval<re_std::int64_t>()));

    template<typename Type>
    using statement_bind_double_probe_t =
        decltype(std::declval<Type&>().template bind_double<interface_probe>(
            std::declval<std::size_t>(),
            std::declval<double>()));

    template<typename Type>
    using statement_bind_string_probe_t =
        decltype(std::declval<Type&>().template bind_string<interface_probe>(
            std::declval<std::size_t>(),
            std::declval<const std::string&>()));

    template<typename Type>
    using statement_bind_binary_probe_t =
        decltype(std::declval<Type&>().template bind_binary<interface_probe>(
            std::declval<std::size_t>(),
            std::declval<const std::vector<re_std::uint8_t>&>()));

    template<typename Type>
    using statement_bind_value_probe_t =
        decltype(std::declval<Type&>()
                     .template bind_value<interface_probe, Type, void>(
                         std::declval<std::size_t>(),
                         std::declval<const value&>()));

    template<typename Type>
    using statement_execute_query_probe_t =
        decltype(std::declval<Type&>()
                     .template execute_query<interface_probe>());

    template<typename Type>
    using statement_execute_update_probe_t =
        decltype(std::declval<Type&>()
                     .template execute_update<interface_probe>());

    template<typename Type>
    using statement_execute_probe_t =
        decltype(std::declval<Type&>().template execute<interface_probe>());

    template<typename Type>
    using statement_parameter_count_probe_t =
        decltype(std::declval<Type&>()
                     .template parameter_count<interface_probe>());

    template<typename Type>
    using statement_clear_parameters_probe_t =
        decltype(std::declval<Type&>()
                     .template clear_parameters<interface_probe>());

    template<typename Type>
    using statement_reset_probe_t =
        decltype(std::declval<Type&>().template reset<interface_probe>());

    template<typename Type>
    using statement_close_probe_t =
        decltype(std::declval<Type&>().template close<interface_probe>());

    template<typename Type>
    using connection_connect_probe_t =
        decltype(std::declval<Type&>().template connect<interface_probe>());

    template<typename Type>
    using connection_disconnect_probe_t =
        decltype(std::declval<Type&>().template disconnect<interface_probe>());

    template<typename Type>
    using connection_reconnect_probe_t =
        decltype(std::declval<Type&>().template reconnect<interface_probe>());

    template<typename Type>
    using connection_is_connected_probe_t =
        decltype(std::declval<Type&>()
                     .template is_connected<interface_probe>());

    template<typename Type>
    using connection_ping_probe_t =
        decltype(std::declval<Type&>().template ping<interface_probe>());

    template<typename Type>
    using connection_execute_query_probe_t =
        decltype(std::declval<Type&>().template execute_query<interface_probe>(
            std::declval<const std::string&>()));

    template<typename Type>
    using connection_execute_update_probe_t =
        decltype(std::declval<Type&>().template execute_update<interface_probe>(
            std::declval<const std::string&>()));

    template<typename Type>
    using connection_execute_probe_t =
        decltype(std::declval<Type&>().template execute<interface_probe>(
            std::declval<const std::string&>()));

    template<typename Type>
    using connection_prepare_probe_t =
        decltype(std::declval<Type&>().template prepare<interface_probe>(
            std::declval<const std::string&>()));

    template<typename Type>
    using connection_begin_transaction_probe_t =
        decltype(std::declval<Type&>()
                     .template begin_transaction<interface_probe>());

    template<typename Type>
    using connection_commit_probe_t =
        decltype(std::declval<Type&>().template commit<interface_probe>());

    template<typename Type>
    using connection_rollback_probe_t =
        decltype(std::declval<Type&>().template rollback<interface_probe>());

    template<typename Type>
    using connection_in_transaction_probe_t =
        decltype(std::declval<Type&>()
                     .template in_transaction<interface_probe>());

    template<typename Type>
    using connection_get_isolation_level_probe_t =
        decltype(std::declval<Type&>()
                     .template get_isolation_level<interface_probe,
                                                   Type,
                                                   void>());

    template<typename Type>
    using connection_set_isolation_level_probe_t =
        decltype(std::declval<Type&>()
                     .template set_isolation_level<interface_probe,
                                                   Type,
                                                   void>(
                         std::declval<isolation_level>()));

    template<typename Type>
    using connection_create_savepoint_probe_t =
        decltype(std::declval<Type&>()
                     .template create_savepoint<interface_probe, Type, void>(
                         std::declval<const std::string&>()));

    template<typename Type>
    using connection_rollback_to_savepoint_probe_t =
        decltype(std::declval<Type&>()
                     .template rollback_to_savepoint<interface_probe,
                                                     Type,
                                                     void>(
                         std::declval<const std::string&>()));

    template<typename Type>
    using connection_release_savepoint_probe_t =
        decltype(std::declval<Type&>()
                     .template release_savepoint<interface_probe,
                                                 Type,
                                                 void>(
                         std::declval<const std::string&>()));

    template<typename Type>
    using connection_get_database_type_probe_t =
        decltype(std::declval<Type&>()
                     .template get_database_type<interface_probe>());

    template<typename Type>
    using connection_get_database_name_probe_t =
        decltype(std::declval<Type&>()
                     .template get_database_name<interface_probe>());

    template<typename Type>
    using connection_get_server_version_probe_t =
        decltype(std::declval<Type&>()
                     .template get_server_version<interface_probe>());

    template<typename Type>
    using connection_get_state_probe_t =
        decltype(std::declval<Type&>().template get_state<interface_probe>());

    template<typename Type>
    using connection_get_last_insert_id_probe_t =
        decltype(std::declval<Type&>()
                     .template get_last_insert_id<interface_probe>());

    template<typename Type>
    using connection_get_affected_rows_probe_t =
        decltype(std::declval<Type&>()
                     .template get_affected_rows<interface_probe>());

    template<typename Type>
    using connection_set_auto_commit_probe_t =
        decltype(std::declval<Type&>()
                     .template set_auto_commit<interface_probe, Type, void>(
                         std::declval<bool>()));

    template<typename Type>
    using connection_get_auto_commit_probe_t =
        decltype(std::declval<Type&>()
                     .template get_auto_commit<interface_probe>());

    template<typename Type>
    using connection_get_last_error_probe_t =
        decltype(std::declval<Type&>()
                     .template get_last_error<interface_probe>());

    template<typename Type>
    using connection_get_last_error_code_probe_t =
        decltype(std::declval<Type&>()
                     .template get_last_error_code<interface_probe>());

NS_END  // internal

// 4.2    Result sets
//------------------------------------------------------------------------------
// 4.2.1
// result_set
//   class template: the rows a query returned, read through a cursor that
// starts before the first row. Derived implements every member; the cursor,
// the columns and the conversions are Derived's, as database_statement.hpp's
// buffered_result_set describes them. Not copyable.
template<typename Derived>
class result_set
{
public:
    // navigation -- move the cursor, answering whether it is on a row
    template<typename Probe = internal::interface_probe>
    bool
    next()
    {
        static_assert(
            !is_detected<internal::result_set_next_probe_t,
                         Derived>::value,
            "result_set<Derived>::next forwards to "
            "Derived, which declares no member of the name");

        return impl().next();
    }

    template<typename Probe = internal::interface_probe>
    bool
    previous()
    {
        static_assert(
            !is_detected<internal::result_set_previous_probe_t,
                         Derived>::value,
            "result_set<Derived>::previous forwards to "
            "Derived, which declares no member of the name");

        return impl().previous();
    }

    template<typename Probe = internal::interface_probe>
    bool
    first()
    {
        static_assert(
            !is_detected<internal::result_set_first_probe_t,
                         Derived>::value,
            "result_set<Derived>::first forwards to "
            "Derived, which declares no member of the name");

        return impl().first();
    }

    template<typename Probe = internal::interface_probe>
    bool
    last()
    {
        static_assert(
            !is_detected<internal::result_set_last_probe_t,
                         Derived>::value,
            "result_set<Derived>::last forwards to "
            "Derived, which declares no member of the name");

        return impl().last();
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::result_absolute_gate_t<Type>>
    bool
    absolute(
        re_std::int64_t _row
    )
    {
        static_assert(
            !is_detected<internal::result_set_absolute_probe_t,
                         Derived>::value,
            "result_set<Derived>::absolute forwards to "
            "Derived, which declares no member of the name");

        return impl().absolute(_row);
    }

    // metadata -- the columns, and where the cursor is
    template<typename Probe = internal::interface_probe>
    std::size_t
    column_count() const
    {
        static_assert(
            !is_detected<internal::result_set_column_count_probe_t,
                         Derived>::value,
            "result_set<Derived>::column_count forwards to "
            "Derived, which declares no member of the name");

        return impl().column_count();
    }

    template<typename Probe = internal::interface_probe>
    std::string
    column_name(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_column_name_probe_t,
                         Derived>::value,
            "result_set<Derived>::column_name forwards to "
            "Derived, which declares no member of the name");

        return impl().column_name(_index);
    }

    template<typename Probe = internal::interface_probe>
    field_type
    column_type(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_column_type_probe_t,
                         Derived>::value,
            "result_set<Derived>::column_type forwards to "
            "Derived, which declares no member of the name");

        return impl().column_type(_index);
    }

    template<typename Probe = internal::interface_probe>
    std::size_t
    column_index(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_column_index_probe_t,
                         Derived>::value,
            "result_set<Derived>::column_index forwards to "
            "Derived, which declares no member of the name");

        return impl().column_index(_name);
    }

    template<typename Probe = internal::interface_probe>
    re_std::int64_t
    row_count() const
    {
        static_assert(
            !is_detected<internal::result_set_row_count_probe_t,
                         Derived>::value,
            "result_set<Derived>::row_count forwards to "
            "Derived, which declares no member of the name");

        return impl().row_count();
    }

    template<typename Probe = internal::interface_probe>
    re_std::int64_t
    current_row() const
    {
        static_assert(
            !is_detected<internal::result_set_current_row_probe_t,
                         Derived>::value,
            "result_set<Derived>::current_row forwards to "
            "Derived, which declares no member of the name");

        return impl().current_row();
    }

    // values by column index -- the current row's
    template<typename Probe = internal::interface_probe>
    value
    get_value(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_value_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_value forwards to "
            "Derived, which declares no member of the name");

        return impl().get_value(_index);
    }

    template<typename Probe = internal::interface_probe>
    bool
    is_null(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_is_null_probe_t,
                         Derived>::value,
            "result_set<Derived>::is_null forwards to "
            "Derived, which declares no member of the name");

        return impl().is_null(_index);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<bool>
    get_bool(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_bool_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_bool forwards to "
            "Derived, which declares no member of the name");

        return impl().get_bool(_index);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<re_std::int32_t>
    get_int(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_int_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_int forwards to "
            "Derived, which declares no member of the name");

        return impl().get_int(_index);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<re_std::int64_t>
    get_long(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_long_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_long forwards to "
            "Derived, which declares no member of the name");

        return impl().get_long(_index);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<double>
    get_double(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_double_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_double forwards to "
            "Derived, which declares no member of the name");

        return impl().get_double(_index);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<std::string>
    get_string(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_string_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_string forwards to "
            "Derived, which declares no member of the name");

        return impl().get_string(_index);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<std::vector<re_std::uint8_t>>
    get_binary(
        std::size_t _index
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_binary_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_binary forwards to "
            "Derived, which declares no member of the name");

        return impl().get_binary(_index);
    }

    // values by column name -- the current row's
    template<typename Probe = internal::interface_probe>
    value
    get_value(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_value_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_value forwards to "
            "Derived, which declares no member of the name");

        return impl().get_value(_name);
    }

    template<typename Probe = internal::interface_probe>
    bool
    is_null(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_is_null_probe_t,
                         Derived>::value,
            "result_set<Derived>::is_null forwards to "
            "Derived, which declares no member of the name");

        return impl().is_null(_name);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<bool>
    get_bool(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_bool_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_bool forwards to "
            "Derived, which declares no member of the name");

        return impl().get_bool(_name);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<re_std::int32_t>
    get_int(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_int_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_int forwards to "
            "Derived, which declares no member of the name");

        return impl().get_int(_name);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<re_std::int64_t>
    get_long(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_long_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_long forwards to "
            "Derived, which declares no member of the name");

        return impl().get_long(_name);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<double>
    get_double(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_double_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_double forwards to "
            "Derived, which declares no member of the name");

        return impl().get_double(_name);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<std::string>
    get_string(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_string_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_string forwards to "
            "Derived, which declares no member of the name");

        return impl().get_string(_name);
    }

    template<typename Probe = internal::interface_probe>
    std::optional<std::vector<re_std::uint8_t>>
    get_binary(
        const std::string& _name
    ) const
    {
        static_assert(
            !is_detected<internal::result_set_get_binary_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_binary forwards to "
            "Derived, which declares no member of the name");

        return impl().get_binary(_name);
    }

    // rows -- the current one by name, the rest, and releasing them all
    template<typename Probe = internal::interface_probe>
    row
    get_current_row() const
    {
        static_assert(
            !is_detected<internal::result_set_get_current_row_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_current_row forwards to "
            "Derived, which declares no member of the name");

        return impl().get_current_row();
    }

    template<typename Probe = internal::interface_probe>
    result_rows
    get_all_rows()
    {
        static_assert(
            !is_detected<internal::result_set_get_all_rows_probe_t,
                         Derived>::value,
            "result_set<Derived>::get_all_rows forwards to "
            "Derived, which declares no member of the name");

        return impl().get_all_rows();
    }

    template<typename Probe = internal::interface_probe>
    void
    close()
    {
        static_assert(
            !is_detected<internal::result_set_close_probe_t,
                         Derived>::value,
            "result_set<Derived>::close forwards to "
            "Derived, which declares no member of the name");

        impl().close();

        return;
    }

    // ~result_set -- public, and not virtual: see section 4's description
    ~result_set() = default;

protected:
    result_set() = default;

    result_set(const result_set&)            = delete;
    result_set& operator=(const result_set&) = delete;

    result_set(result_set&&) noexcept            = default;
    result_set& operator=(result_set&&) noexcept = default;

private:
    Derived&
    impl() noexcept
    {
        return static_cast<Derived&>(*this);
    }

    const Derived&
    impl() const noexcept
    {
        return static_cast<const Derived&>(*this);
    }
};

// 4.3    Statements
//------------------------------------------------------------------------------
// 4.3.1
// statement
//   class template: a prepared statement, whose parameters are numbered from
// 1 and keep the values bound to them until they are bound again or cleared.
// Derived implements every member; binding by name and bind_value exist
// where Derived has them. Not copyable.
template<typename Derived>
class statement
{
public:
    // binding by index -- the parameter, from 1, and its value
    template<typename Probe = internal::interface_probe>
    void
    bind_null(
        std::size_t _index
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_null_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_null forwards to "
            "Derived, which declares no member of the name");

        impl().bind_null(_index);

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    bind_bool(
        std::size_t _index,
        bool        _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_bool_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_bool forwards to "
            "Derived, which declares no member of the name");

        impl().bind_bool(_index, _value);

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    bind_int(
        std::size_t     _index,
        re_std::int32_t _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_int_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_int forwards to "
            "Derived, which declares no member of the name");

        impl().bind_int(_index, _value);

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    bind_long(
        std::size_t     _index,
        re_std::int64_t _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_long_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_long forwards to "
            "Derived, which declares no member of the name");

        impl().bind_long(_index, _value);

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    bind_double(
        std::size_t _index,
        double      _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_double_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_double forwards to "
            "Derived, which declares no member of the name");

        impl().bind_double(_index, _value);

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    bind_string(
        std::size_t        _index,
        const std::string& _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_string_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_string forwards to "
            "Derived, which declares no member of the name");

        impl().bind_string(_index, _value);

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    bind_binary(
        std::size_t                         _index,
        const std::vector<re_std::uint8_t>& _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_binary_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_binary forwards to "
            "Derived, which declares no member of the name");

        impl().bind_binary(_index, _value);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_bind_value_gate_t<Type>>
    void
    bind_value(
        std::size_t  _index,
        const value& _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_value_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_value forwards to "
            "Derived, which declares no member of the name");

        impl().bind_value(_index, _value);

        return;
    }

    // binding by name -- where the database names parameters
    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_named_gate_t<Type>>
    void
    bind_null(
        const std::string& _name
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_null_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_null forwards to "
            "Derived, which declares no member of the name");

        impl().bind_null(_name);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_named_gate_t<Type>>
    void
    bind_bool(
        const std::string& _name,
        bool               _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_bool_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_bool forwards to "
            "Derived, which declares no member of the name");

        impl().bind_bool(_name, _value);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_named_gate_t<Type>>
    void
    bind_int(
        const std::string& _name,
        re_std::int32_t    _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_int_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_int forwards to "
            "Derived, which declares no member of the name");

        impl().bind_int(_name, _value);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_named_gate_t<Type>>
    void
    bind_long(
        const std::string& _name,
        re_std::int64_t    _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_long_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_long forwards to "
            "Derived, which declares no member of the name");

        impl().bind_long(_name, _value);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_named_gate_t<Type>>
    void
    bind_double(
        const std::string& _name,
        double             _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_double_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_double forwards to "
            "Derived, which declares no member of the name");

        impl().bind_double(_name, _value);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_named_gate_t<Type>>
    void
    bind_string(
        const std::string& _name,
        const std::string& _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_string_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_string forwards to "
            "Derived, which declares no member of the name");

        impl().bind_string(_name, _value);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::statement_named_gate_t<Type>>
    void
    bind_binary(
        const std::string&                  _name,
        const std::vector<re_std::uint8_t>& _value
    )
    {
        static_assert(
            !is_detected<internal::statement_bind_binary_probe_t,
                         Derived>::value,
            "statement<Derived>::bind_binary forwards to "
            "Derived, which declares no member of the name");

        impl().bind_binary(_name, _value);

        return;
    }

    // execution -- run the statement with the values bound
    template<typename Probe = internal::interface_probe>
    auto
    execute_query()
    {
        static_assert(
            !is_detected<internal::statement_execute_query_probe_t,
                         Derived>::value,
            "statement<Derived>::execute_query forwards to "
            "Derived, which declares no member of the name");

        return impl().execute_query();
    }

    template<typename Probe = internal::interface_probe>
    re_std::int64_t
    execute_update()
    {
        static_assert(
            !is_detected<internal::statement_execute_update_probe_t,
                         Derived>::value,
            "statement<Derived>::execute_update forwards to "
            "Derived, which declares no member of the name");

        return impl().execute_update();
    }

    template<typename Probe = internal::interface_probe>
    bool
    execute()
    {
        static_assert(
            !is_detected<internal::statement_execute_probe_t,
                         Derived>::value,
            "statement<Derived>::execute forwards to "
            "Derived, which declares no member of the name");

        return impl().execute();
    }

    // metadata and lifetime -- the parameters, and releasing the statement
    template<typename Probe = internal::interface_probe>
    std::size_t
    parameter_count() const
    {
        static_assert(
            !is_detected<internal::statement_parameter_count_probe_t,
                         Derived>::value,
            "statement<Derived>::parameter_count forwards to "
            "Derived, which declares no member of the name");

        return impl().parameter_count();
    }

    template<typename Probe = internal::interface_probe>
    void
    clear_parameters()
    {
        static_assert(
            !is_detected<internal::statement_clear_parameters_probe_t,
                         Derived>::value,
            "statement<Derived>::clear_parameters forwards to "
            "Derived, which declares no member of the name");

        impl().clear_parameters();

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    reset()
    {
        static_assert(
            !is_detected<internal::statement_reset_probe_t,
                         Derived>::value,
            "statement<Derived>::reset forwards to "
            "Derived, which declares no member of the name");

        impl().reset();

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    close()
    {
        static_assert(
            !is_detected<internal::statement_close_probe_t,
                         Derived>::value,
            "statement<Derived>::close forwards to "
            "Derived, which declares no member of the name");

        impl().close();

        return;
    }

protected:
    statement()  = default;
    ~statement() = default;

    statement(const statement&)            = delete;
    statement& operator=(const statement&) = delete;

    statement(statement&&) noexcept            = default;
    statement& operator=(statement&&) noexcept = default;

private:
    Derived&
    impl() noexcept
    {
        return static_cast<Derived&>(*this);
    }

    const Derived&
    impl() const noexcept
    {
        return static_cast<const Derived&>(*this);
    }
};

// 4.4    Transactions
//------------------------------------------------------------------------------
// 4.4.1
// transaction
//   class template: begins a transaction on a connection when made, and rolls
// it back when it goes unless commit or rollback ended it first; a rollback
// that fails then is swallowed, since a destructor cannot throw. Connection
// is any connection with begin_transaction, commit, rollback and
// in_transaction, as face::transaction asks of a face; the level and the
// savepoints need the members of the same names. Not copyable; movable, the
// object moved from ending nothing.
template<typename Connection>
class transaction
{
public:
    /**
     * @brief Begins a transaction at the connection's default level.
     *
     * @param[in,out] _conn  the connection; it must outlive the guard.
     * @throws transaction_exception, or connection_exception, when the
     *         transaction cannot begin; nothing is then to be rolled back.
     */
    explicit transaction(
        Connection& _conn
    )
        : m_connection(&_conn),
          m_isolation_level(isolation_level::default_level),
          m_active(false)
    {
        m_connection->begin_transaction();
        m_active = true;
    }

    /**
     * @brief Begins a transaction at an isolation level.
     *
     * @param[in,out] _conn       the connection; it must outlive the guard.
     * @param[in]     _isolation  the level, as the connection's
     *                            begin_transaction(isolation_level) takes it.
     * @throws as the constructor above.
     */
    transaction(
        Connection&     _conn,
        isolation_level _isolation
    )
        : m_connection(&_conn),
          m_isolation_level(_isolation),
          m_active(false)
    {
        m_connection->begin_transaction(_isolation);
        m_active = true;
    }

    ~transaction()
    {
        m_end_quietly();
    }

    transaction(const transaction&)            = delete;
    transaction& operator=(const transaction&) = delete;

    transaction(
        transaction&& _other
    ) noexcept
        : m_connection(_other.m_connection),
          m_isolation_level(_other.m_isolation_level),
          m_active(_other.m_active)
    {
        _other.m_connection = nullptr;
        _other.m_active     = false;
    }

    transaction&
    operator=(
        transaction&& _other
    ) noexcept
    {
        // what this guard still holds goes back before it takes another's
        if (this != &_other)
        {
            m_end_quietly();

            m_connection      = _other.m_connection;
            m_isolation_level = _other.m_isolation_level;
            m_active          = _other.m_active;

            _other.m_connection = nullptr;
            _other.m_active     = false;
        }

        return *this;
    }

    /**
     * @brief Commits the transaction.
     *
     * @throws transaction_exception when the guard is not active; and what
     *         the connection's commit throws.
     * @post The guard is not active, unless the commit failed and left the
     *       transaction open, as SQLite's busy commit does: then it stays
     *       active, to retry, or to roll back when it goes.
     */
    void
    commit()
    {
        m_end(true);

        return;
    }

    /**
     * @brief Rolls the transaction back.
     *
     * @throws transaction_exception when the guard is not active; and what
     *         the connection's rollback throws.
     * @post As for commit.
     */
    void
    rollback()
    {
        m_end(false);

        return;
    }

    // savepoints -- inside the active transaction, by name; each throws
    // transaction_exception when the guard is not active, or when the
    // connection's own call fails
    template<typename Type = Connection,
             typename      = decltype(std::declval<Type&>().create_savepoint(
                                 std::declval<const std::string&>()))>
    void
    create_savepoint(
        const std::string& _name
    )
    {
        m_require_active();
        m_connection->create_savepoint(_name);

        return;
    }

    template<typename Type = Connection,
             typename      = decltype(
                                 std::declval<Type&>().rollback_to_savepoint(
                                     std::declval<const std::string&>()))>
    void
    rollback_to_savepoint(
        const std::string& _name
    )
    {
        m_require_active();
        m_connection->rollback_to_savepoint(_name);

        return;
    }

    template<typename Type = Connection,
             typename      = decltype(std::declval<Type&>().release_savepoint(
                                 std::declval<const std::string&>()))>
    void
    release_savepoint(
        const std::string& _name
    )
    {
        m_require_active();
        m_connection->release_savepoint(_name);

        return;
    }

    // state -- whether the guard still has a transaction to end, and the
    // level it asked for
    bool
    is_active() const noexcept
    {
        return m_active;
    }

    isolation_level
    get_isolation_level() const noexcept
    {
        return m_isolation_level;
    }

private:
    void
    m_require_active() const
    {
        // only an active guard has a transaction to act on
        if (!m_active)
        {
            throw transaction_exception("transaction is not active");
        }

        return;
    }

    // m_end -- commits or rolls back; the guard stays active only while the
    // connection is still in the transaction it began
    void
    m_end(
        bool _commit
    )
    {
        m_require_active();

        try
        {
            // the connection ends the transaction, or says why not
            if (_commit)
            {
                m_connection->commit();
            }
            else
            {
                m_connection->rollback();
            }
        }
        catch (...)
        {
            m_active = m_connection->in_transaction();

            throw;
        }

        m_active = false;

        return;
    }

    void
    m_end_quietly() noexcept
    {
        // what was neither committed nor rolled back goes back
        if (m_active)
        {
            m_active = false;

            try
            {
                m_connection->rollback();
            }
            catch (...)
            {
                // a destructor cannot throw; the connection keeps the error
            }
        }

        return;
    }

    Connection*     m_connection;
    isolation_level m_isolation_level;
    bool            m_active;
};

// 4.5    Connections
//------------------------------------------------------------------------------
// 4.5.1
// connection
//   class template: a connection to a database, and its configuration and
// state. Every operation forwards to Derived, which database_connection
// (database_connection.hpp) implements once for every vendor, with the
// contracts written there; the operations marked gated exist where Derived
// has the hook database_connection implements them over (4.1.1). Not
// copyable; movable.
template<typename Derived>
class connection
{
public:
    // management -- open, close and test the connection
    template<typename Probe = internal::interface_probe>
    void
    connect()
    {
        static_assert(
            !is_detected<internal::connection_connect_probe_t,
                         Derived>::value,
            "connection<Derived>::connect forwards to "
            "Derived, which declares no member of the name");

        impl().connect();

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    disconnect()
    {
        static_assert(
            !is_detected<internal::connection_disconnect_probe_t,
                         Derived>::value,
            "connection<Derived>::disconnect forwards to "
            "Derived, which declares no member of the name");

        impl().disconnect();

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    reconnect()
    {
        static_assert(
            !is_detected<internal::connection_reconnect_probe_t,
                         Derived>::value,
            "connection<Derived>::reconnect forwards to "
            "Derived, which declares no member of the name");

        impl().reconnect();

        return;
    }

    template<typename Probe = internal::interface_probe>
    bool
    is_connected() const noexcept
    {
        static_assert(
            !is_detected<internal::connection_is_connected_probe_t,
                         Derived>::value,
            "connection<Derived>::is_connected forwards to "
            "Derived, which declares no member of the name");

        return impl().is_connected();
    }

    template<typename Probe = internal::interface_probe>
    bool
    ping() const
    {
        static_assert(
            !is_detected<internal::connection_ping_probe_t,
                         Derived>::value,
            "connection<Derived>::ping forwards to "
            "Derived, which declares no member of the name");

        return impl().ping();
    }

    // execution -- run SQL, or prepare it
    template<typename Probe = internal::interface_probe>
    auto
    execute_query(
        const std::string& _query
    )
    {
        static_assert(
            !is_detected<internal::connection_execute_query_probe_t,
                         Derived>::value,
            "connection<Derived>::execute_query forwards to "
            "Derived, which declares no member of the name");

        return impl().execute_query(_query);
    }

    template<typename Probe = internal::interface_probe>
    re_std::int64_t
    execute_update(
        const std::string& _query
    )
    {
        static_assert(
            !is_detected<internal::connection_execute_update_probe_t,
                         Derived>::value,
            "connection<Derived>::execute_update forwards to "
            "Derived, which declares no member of the name");

        return impl().execute_update(_query);
    }

    template<typename Probe = internal::interface_probe>
    bool
    execute(
        const std::string& _query
    )
    {
        static_assert(
            !is_detected<internal::connection_execute_probe_t,
                         Derived>::value,
            "connection<Derived>::execute forwards to "
            "Derived, which declares no member of the name");

        return impl().execute(_query);
    }

    template<typename Probe = internal::interface_probe>
    auto
    prepare(
        const std::string& _query
    )
    {
        static_assert(
            !is_detected<internal::connection_prepare_probe_t,
                         Derived>::value,
            "connection<Derived>::prepare forwards to "
            "Derived, which declares no member of the name");

        return impl().prepare(_query);
    }

    // transactions -- begin_transaction with a level, the level itself and
    // the savepoints are gated
    template<typename Probe = internal::interface_probe>
    void
    begin_transaction()
    {
        static_assert(
            !is_detected<internal::connection_begin_transaction_probe_t,
                         Derived>::value,
            "connection<Derived>::begin_transaction forwards to "
            "Derived, which declares no member of the name");

        impl().begin_transaction();

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::begin_transaction_level_hook_t<Type>>
    void
    begin_transaction(
        isolation_level _isolation
    )
    {
        static_assert(
            !is_detected<internal::connection_begin_transaction_probe_t,
                         Derived>::value,
            "connection<Derived>::begin_transaction forwards to "
            "Derived, which declares no member of the name");

        impl().begin_transaction(_isolation);

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    commit()
    {
        static_assert(
            !is_detected<internal::connection_commit_probe_t,
                         Derived>::value,
            "connection<Derived>::commit forwards to "
            "Derived, which declares no member of the name");

        impl().commit();

        return;
    }

    template<typename Probe = internal::interface_probe>
    void
    rollback()
    {
        static_assert(
            !is_detected<internal::connection_rollback_probe_t,
                         Derived>::value,
            "connection<Derived>::rollback forwards to "
            "Derived, which declares no member of the name");

        impl().rollback();

        return;
    }

    template<typename Probe = internal::interface_probe>
    bool
    in_transaction() const noexcept
    {
        static_assert(
            !is_detected<internal::connection_in_transaction_probe_t,
                         Derived>::value,
            "connection<Derived>::in_transaction forwards to "
            "Derived, which declares no member of the name");

        return impl().in_transaction();
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::get_isolation_level_hook_t<Type>>
    isolation_level
    get_isolation_level() const
    {
        static_assert(
            !is_detected<internal::connection_get_isolation_level_probe_t,
                         Derived>::value,
            "connection<Derived>::get_isolation_level forwards to "
            "Derived, which declares no member of the name");

        return impl().get_isolation_level();
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::set_isolation_level_hook_t<Type>>
    void
    set_isolation_level(
        isolation_level _level
    )
    {
        static_assert(
            !is_detected<internal::connection_set_isolation_level_probe_t,
                         Derived>::value,
            "connection<Derived>::set_isolation_level forwards to "
            "Derived, which declares no member of the name");

        impl().set_isolation_level(_level);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::create_savepoint_hook_t<Type>>
    void
    create_savepoint(
        const std::string& _name
    )
    {
        static_assert(
            !is_detected<internal::connection_create_savepoint_probe_t,
                         Derived>::value,
            "connection<Derived>::create_savepoint forwards to "
            "Derived, which declares no member of the name");

        impl().create_savepoint(_name);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::rollback_to_savepoint_hook_t<Type>>
    void
    rollback_to_savepoint(
        const std::string& _name
    )
    {
        static_assert(
            !is_detected<internal::connection_rollback_to_savepoint_probe_t,
                         Derived>::value,
            "connection<Derived>::rollback_to_savepoint forwards to "
            "Derived, which declares no member of the name");

        impl().rollback_to_savepoint(_name);

        return;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::release_savepoint_hook_t<Type>>
    void
    release_savepoint(
        const std::string& _name
    )
    {
        static_assert(
            !is_detected<internal::connection_release_savepoint_probe_t,
                         Derived>::value,
            "connection<Derived>::release_savepoint forwards to "
            "Derived, which declares no member of the name");

        impl().release_savepoint(_name);

        return;
    }

    // metadata -- the vendor, the database, the server, and what the latest
    // statements did
    template<typename Probe = internal::interface_probe>
    database_type
    get_database_type() const noexcept
    {
        static_assert(
            !is_detected<internal::connection_get_database_type_probe_t,
                         Derived>::value,
            "connection<Derived>::get_database_type forwards to "
            "Derived, which declares no member of the name");

        return impl().get_database_type();
    }

    template<typename Probe = internal::interface_probe>
    std::string
    get_database_name() const
    {
        static_assert(
            !is_detected<internal::connection_get_database_name_probe_t,
                         Derived>::value,
            "connection<Derived>::get_database_name forwards to "
            "Derived, which declares no member of the name");

        return impl().get_database_name();
    }

    template<typename Probe = internal::interface_probe>
    std::string
    get_server_version() const
    {
        static_assert(
            !is_detected<internal::connection_get_server_version_probe_t,
                         Derived>::value,
            "connection<Derived>::get_server_version forwards to "
            "Derived, which declares no member of the name");

        return impl().get_server_version();
    }

    template<typename Probe = internal::interface_probe>
    connection_state
    get_state() const noexcept
    {
        static_assert(
            !is_detected<internal::connection_get_state_probe_t,
                         Derived>::value,
            "connection<Derived>::get_state forwards to "
            "Derived, which declares no member of the name");

        return impl().get_state();
    }

    template<typename Probe = internal::interface_probe>
    re_std::int64_t
    get_last_insert_id() const
    {
        static_assert(
            !is_detected<internal::connection_get_last_insert_id_probe_t,
                         Derived>::value,
            "connection<Derived>::get_last_insert_id forwards to "
            "Derived, which declares no member of the name");

        return impl().get_last_insert_id();
    }

    template<typename Probe = internal::interface_probe>
    re_std::int64_t
    get_affected_rows() const
    {
        static_assert(
            !is_detected<internal::connection_get_affected_rows_probe_t,
                         Derived>::value,
            "connection<Derived>::get_affected_rows forwards to "
            "Derived, which declares no member of the name");

        return impl().get_affected_rows();
    }

    // configuration -- set_auto_commit is gated
    const connection_config&
    get_config() const noexcept
    {
        return m_config;
    }

    template<typename Probe = internal::interface_probe,
             typename Type  = Derived,
             typename       = internal::set_auto_commit_hook_t<Type>>
    void
    set_auto_commit(
        bool _enabled
    )
    {
        static_assert(
            !is_detected<internal::connection_set_auto_commit_probe_t,
                         Derived>::value,
            "connection<Derived>::set_auto_commit forwards to "
            "Derived, which declares no member of the name");

        impl().set_auto_commit(_enabled);

        return;
    }

    template<typename Probe = internal::interface_probe>
    bool
    get_auto_commit() const
    {
        static_assert(
            !is_detected<internal::connection_get_auto_commit_probe_t,
                         Derived>::value,
            "connection<Derived>::get_auto_commit forwards to "
            "Derived, which declares no member of the name");

        return impl().get_auto_commit();
    }

    // errors -- the latest failure an operation threw
    template<typename Probe = internal::interface_probe>
    std::string
    get_last_error() const
    {
        static_assert(
            !is_detected<internal::connection_get_last_error_probe_t,
                         Derived>::value,
            "connection<Derived>::get_last_error forwards to "
            "Derived, which declares no member of the name");

        return impl().get_last_error();
    }

    template<typename Probe = internal::interface_probe>
    int
    get_last_error_code() const noexcept
    {
        static_assert(
            !is_detected<internal::connection_get_last_error_code_probe_t,
                         Derived>::value,
            "connection<Derived>::get_last_error_code forwards to "
            "Derived, which declares no member of the name");

        return impl().get_last_error_code();
    }

protected:
    connection()
        : m_state(connection_state::disconnected),
          m_in_transaction(false),
          m_auto_commit(true)
    {}

    ~connection() = default;

    connection(const connection&)            = delete;
    connection& operator=(const connection&) = delete;

    connection(connection&&) noexcept            = default;
    connection& operator=(connection&&) noexcept = default;

    // m_config, ... -- the configuration and the state database_connection
    // keeps; m_in_transaction and m_auto_commit answer for a vendor whose
    // connection cannot say for itself
    connection_config m_config;
    connection_state  m_state;
    bool              m_in_transaction;
    bool              m_auto_commit;

private:
    Derived&
    impl() noexcept
    {
        return static_cast<Derived&>(*this);
    }

    const Derived&
    impl() const noexcept
    {
        return static_cast<const Derived&>(*this);
    }
};

// 4.6    Pools
//------------------------------------------------------------------------------
// connection_pool is defined below, and referred to first by
// pooled_connection
template<typename Connection>
class connection_pool;

// 4.6.1
// pooled_connection
//   class template: a connection borrowed from a pool, which returns it when
// it goes or when released. Not copyable; movable, the object moved from
// returning nothing.
template<typename Connection>
class pooled_connection
{
public:
    pooled_connection(
        connection_pool<Connection>& _pool,
        std::unique_ptr<Connection>  _connection
    )
        : m_pool(&_pool),
          m_connection(std::move(_connection))
    {}

    ~pooled_connection()
    {
        release();
    }

    pooled_connection(const pooled_connection&)            = delete;
    pooled_connection& operator=(const pooled_connection&) = delete;

    pooled_connection(
        pooled_connection&& _other
    ) noexcept
        : m_pool(_other.m_pool),
          m_connection(std::move(_other.m_connection))
    {
        _other.m_pool = nullptr;
    }

    pooled_connection&
    operator=(
        pooled_connection&& _other
    ) noexcept
    {
        // what this object borrowed goes back before it takes another's
        if (this != &_other)
        {
            release();

            m_pool       = _other.m_pool;
            m_connection = std::move(_other.m_connection);

            _other.m_pool = nullptr;
        }

        return *this;
    }

    // access -- the connection borrowed, or NULL once it went back
    Connection*
    get() noexcept
    {
        return m_connection.get();
    }

    const Connection*
    get() const noexcept
    {
        return m_connection.get();
    }

    Connection*
    operator->() noexcept
    {
        return m_connection.get();
    }

    const Connection*
    operator->() const noexcept
    {
        return m_connection.get();
    }

    Connection&
    operator*() noexcept
    {
        return *m_connection;
    }

    const Connection&
    operator*() const noexcept
    {
        return *m_connection;
    }

    /**
     * @brief Returns the connection to its pool.
     *
     * @post The object holds no connection; every pointer and reference it
     *       gave is invalid.
     */
    void
    release() noexcept
    {
        // only a connection still borrowed goes back
        if ( (m_pool) &&
             (m_connection) )
        {
            m_pool->release(std::move(m_connection));

            m_pool       = nullptr;
            m_connection = nullptr;
        }

        return;
    }

private:
    connection_pool<Connection>* m_pool;
    std::unique_ptr<Connection>  m_connection;
};

// 4.6.2
// connection_pool
//   class template: connections made by a factory, connected, and lent out
// as pooled_connection. Connection is any type with connect() and ping();
// one with execute(const std::string&) is checked by the configuration's
// validation_query, where it has one. The pool retires what waited idle or
// lived too long (pool_config's idle_timeout and max_lifetime) when it lends
// and when it takes back, never while a connection is lent. Not
// thread-safe: a program that shares a pool between threads guards it, and
// so the pool never waits for a connection. Not copyable or movable.
template<typename Connection>
class connection_pool
{
private:
    // clock
    //   type: the clock a pool's durations are measured on.
    using clock = std::chrono::steady_clock;

    // idle_slot
    //   struct: an idle connection, when it was made, and since when it
    // waits.
    struct idle_slot
    {
        std::unique_ptr<Connection> connection;
        clock::time_point           created;
        clock::time_point           idle_since;
    };

    // lent_slot
    //   struct: a connection lent out, and when it was made.
    struct lent_slot
    {
        Connection*       connection;
        clock::time_point created;
    };

public:
    // connection_factory
    //   type: makes one connection, not yet connected.
    using connection_factory =
        std::function<std::unique_ptr<Connection>()>;

    /**
     * @brief Makes a pool, and connects its minimum of connections.
     *
     * @param[in] _factory  makes each connection.
     * @param[in] _config   the pool's sizes, durations and checks.
     * @throws connection_exception when the factory makes no connection, and
     *         whatever a connection's connect() throws.
     */
    connection_pool(
        connection_factory _factory,
        const pool_config& _config
    )
        : m_factory(std::move(_factory)),
          m_config(_config)
    {
        // the minimum, made and connected up front
        for (std::size_t i = 0; i < m_config.min_connections; ++i)
        {
            m_idle_connections.push_back(m_make_slot());
        }
    }

    ~connection_pool()
    {
        clear();
    }

    connection_pool(const connection_pool&)            = delete;
    connection_pool& operator=(const connection_pool&) = delete;

    /**
     * @brief Lends a connection: after retiring what waited or lived too
     *        long, the idle one that came back last, checked first when the
     *        configuration says so, or a new one while the pool is below its
     *        maximum.
     *
     * @return the connection, which goes back when the object does.
     * @throws connection_exception when the pool is at its maximum, and
     *         whatever making a connection throws.
     */
    pooled_connection<Connection>
    acquire()
    {
        cleanup_idle_connections();

        idle_slot slot;

        // an idle connection first, a new one while there is room
        if (!m_idle_connections.empty())
        {
            slot = std::move(m_idle_connections.back());
            m_idle_connections.pop_back();

            // one that fails its check is replaced
            if ( (m_config.validate_on_acquire) &&
                 (!validate_connection(slot.connection.get())) )
            {
                slot = m_make_slot();
            }
        }
        else if (total_connections() < m_config.max_connections)
        {
            slot = m_make_slot();
        }
        else
        {
            throw connection_exception("connection pool exhausted");
        }

        m_active_connections.push_back(
            lent_slot{slot.connection.get(), slot.created});

        return pooled_connection<Connection>(*this,
                                             std::move(slot.connection));
    }

    /**
     * @brief Takes a connection back: idle again, or dropped when it lived
     *        past max_lifetime, fails its check, or finds the pool already
     *        holding its maximum.
     *
     * @param[in] _conn  the connection; may be empty. One the pool did not
     *                   lend counts as made now.
     */
    void
    release(
        std::unique_ptr<Connection> _conn
    ) noexcept
    {
        // nothing to take back
        if (!_conn)
        {
            return;
        }

        const clock::time_point now     = clock::now();
        clock::time_point       created = now;

        auto it = std::find_if(m_active_connections.begin(),
                               m_active_connections.end(),
                               [&_conn](const lent_slot& _slot)
                               {
                                   return (_slot.connection == _conn.get());
                               });

        // no longer lent out
        if (it != m_active_connections.end())
        {
            created = it->created;
            m_active_connections.erase(it);
        }

        // one that lived too long, or fails its check, is dropped
        if ( (m_lived_too_long(created, now)) ||
             ( (m_config.validate_on_return) &&
               (!validate_connection(_conn.get())) ) )
        {
            return;
        }

        // idle again while there is room, dropped otherwise
        if (m_idle_connections.size() < m_config.max_connections)
        {
            try
            {
                m_idle_connections.push_back(
                    idle_slot{std::move(_conn), created, now});
            }
            catch (...)
            {
                // no room for the slot: the connection is dropped
            }
        }

        return;
    }

    // management -- drop every idle connection, or resize the pool
    void
    clear() noexcept
    {
        m_idle_connections.clear();
        m_active_connections.clear();

        return;
    }

    void
    resize(
        std::size_t _min,
        std::size_t _max
    )
    {
        m_config.min_connections = _min;
        m_config.max_connections = _max;

        // up to the new minimum
        while (m_idle_connections.size() < _min)
        {
            m_idle_connections.push_back(m_make_slot());
        }

        // down to the new maximum
        while (m_idle_connections.size() > _max)
        {
            m_idle_connections.pop_back();
        }

        return;
    }

    // counts -- connections lent out, idle, both, and requests waiting,
    // which a pool that never waits keeps at 0
    std::size_t
    active_connections() const noexcept
    {
        return m_active_connections.size();
    }

    std::size_t
    idle_connections() const noexcept
    {
        return m_idle_connections.size();
    }

    std::size_t
    total_connections() const noexcept
    {
        return active_connections() + idle_connections();
    }

    std::size_t
    waiting_requests() const noexcept
    {
        return 0;
    }

    // configuration
    const pool_config&
    get_config() const noexcept
    {
        return m_config;
    }

    void
    set_config(
        const pool_config& _config
    )
    {
        m_config = _config;

        return;
    }

protected:
    // create_connection -- one from the factory, connected; throws as
    // acquire does
    std::unique_ptr<Connection>
    create_connection()
    {
        std::unique_ptr<Connection> conn = m_factory();

        // the factory made nothing
        if (!conn)
        {
            throw connection_exception("failed to create connection");
        }

        conn->connect();

        return conn;
    }

    // validate_connection -- whether a connection runs the configuration's
    // validation_query, where it has one and the connection runs queries,
    // or else answers a ping
    bool
    validate_connection(
        Connection* _conn
    ) noexcept
    {
        // nothing to ask
        if (!_conn)
        {
            return false;
        }

        try
        {
            // the query, where both the configuration and the connection
            // have what it takes
            if constexpr (is_detected<internal::pool_execute_t,
                                      Connection>::value)
            {
                if (!m_config.validation_query.empty())
                {
                    _conn->execute(m_config.validation_query);

                    return true;
                }
            }

            return _conn->ping();
        }
        catch (...)
        {
            return false;
        }
    }

    // cleanup_idle_connections -- retires the idle connections that lived
    // past max_lifetime, and, while the pool holds more than its minimum,
    // those that waited past idle_timeout, the longest waiting first;
    // acquire calls it first
    void
    cleanup_idle_connections() noexcept
    {
        const clock::time_point now   = clock::now();
        std::size_t             total = total_connections();
        std::size_t             kept  = 0;

        // the slots kept close up, in order, over the slots retired
        for (std::size_t i = 0; i < m_idle_connections.size(); ++i)
        {
            if ( (m_lived_too_long(m_idle_connections[i].created, now)) ||
                 ( (total > m_config.min_connections) &&
                   (m_waited_too_long(m_idle_connections[i].idle_since,
                                      now)) ) )
            {
                --total;

                continue;
            }

            // a retired slot before this one takes its place
            if (kept != i)
            {
                m_idle_connections[kept] = std::move(m_idle_connections[i]);
            }

            ++kept;
        }

        m_idle_connections.erase(m_idle_connections.begin() +
                                     static_cast<std::ptrdiff_t>(kept),
                                 m_idle_connections.end());

        return;
    }

private:
    // m_make_slot -- a new connection, connected, as an idle slot made now
    idle_slot
    m_make_slot()
    {
        const clock::time_point now = clock::now();

        return idle_slot{create_connection(), now, now};
    }

    // m_lived_too_long -- whether a connection made at _created is past
    // max_lifetime; a max_lifetime of 0 is none
    bool
    m_lived_too_long(
        clock::time_point _created,
        clock::time_point _now
    ) const noexcept
    {
        return ( (m_config.max_lifetime >
                  std::chrono::milliseconds::zero()) &&
                 (_now - _created >= m_config.max_lifetime) );
    }

    // m_waited_too_long -- whether a connection idle since _idle_since is
    // past idle_timeout; an idle_timeout of 0 is none
    bool
    m_waited_too_long(
        clock::time_point _idle_since,
        clock::time_point _now
    ) const noexcept
    {
        return ( (m_config.idle_timeout >
                  std::chrono::milliseconds::zero()) &&
                 (_now - _idle_since >= m_config.idle_timeout) );
    }

    connection_factory     m_factory;
    pool_config            m_config;
    std::vector<idle_slot> m_idle_connections;
    std::vector<lent_slot> m_active_connections;
};


//==============================================================================
// 5.  DETECTION
//==============================================================================
// The generic connection surface as detection traits, which the vendor
// headers build their compound traits on (is_sqlite_connection, ...). Each
// comes tagged, a type with ::value for conjunction, and tagless, a constexpr
// bool.


// 5.1    Connection interface
//------------------------------------------------------------------------------
// 5.1.1
// connection_connect_t
//   type: the call each has_ trait below detects on an lvalue Type, with
// connection_disconnect_t, connection_is_connected_t, connection_execute_t,
// connection_server_version_t and connection_last_error_t.
template<typename Type>
using connection_connect_t = decltype(std::declval<Type&>().connect());

template<typename Type>
using connection_disconnect_t = decltype(std::declval<Type&>().disconnect());

template<typename Type>
using connection_is_connected_t =
    decltype(std::declval<Type&>().is_connected());

template<typename Type>
using connection_execute_t =
    decltype(std::declval<Type&>().execute(std::declval<const std::string&>()));

template<typename Type>
using connection_server_version_t =
    decltype(std::declval<Type&>().get_server_version());

template<typename Type>
using connection_last_error_t =
    decltype(std::declval<Type&>().get_last_error());

// 5.1.2
// has_connect
//   trait: whether Type offers connect(); with has_disconnect,
// has_is_connected, has_execute_query (execute(query)), has_server_version
// and has_last_error.
template<typename Type>
struct has_connect : is_detected<connection_connect_t, Type>
{};

template<typename Type>
struct has_disconnect : is_detected<connection_disconnect_t, Type>
{};

template<typename Type>
struct has_is_connected : is_detected<connection_is_connected_t, Type>
{};

template<typename Type>
struct has_execute_query : is_detected<connection_execute_t, Type>
{};

template<typename Type>
struct has_server_version : is_detected<connection_server_version_t, Type>
{};

template<typename Type>
struct has_last_error : is_detected<connection_last_error_t, Type>
{};

// 5.1.3
// is_connection
//   trait: the generic connection interface -- connect, disconnect,
// is_connected and execute -- that every vendor connection offers.
template<typename Type>
struct is_connection : conjunction<has_connect<Type>,
                                   has_disconnect<Type>,
                                   has_is_connected<Type>,
                                   has_execute_query<Type>>
{};

// 5.1.4
// is_vendor_connection
//   trait: a connection that also reports its server version and its last
// error, the surface database_connection adds for every vendor.
template<typename Type>
struct is_vendor_connection : conjunction<is_connection<Type>,
                                          has_server_version<Type>,
                                          has_last_error<Type>>
{};

// 5.1.5
// is_connectable
//   constant: the tagless form of is_connection; is_full_vendor, of
// is_vendor_connection.
template<typename Type>
inline constexpr bool is_connectable = is_connection<Type>::value;

template<typename Type>
inline constexpr bool is_full_vendor = is_vendor_connection<Type>::value;

// 5.1.6
// can_connect
//   constant: the tagless form of has_connect; can_disconnect and
// can_execute_query, of has_disconnect and has_execute_query.
template<typename Type>
inline constexpr bool can_connect = has_connect<Type>::value;

template<typename Type>
inline constexpr bool can_disconnect = has_disconnect<Type>::value;

template<typename Type>
inline constexpr bool can_execute_query = has_execute_query<Type>::value;


//==============================================================================
// 6.  UTILITIES
//==============================================================================
// Defined in database.cpp. Text is read and written the same under every C
// locale: a number's point is always `.`.


// 6.1    Connection strings
//------------------------------------------------------------------------------
/**
 * @brief Reads a connection string into a configuration.
 *
 * @note Two forms are read. A URI,
 *       `scheme://[user[:password]@]host[:port][/database][?key=value&...]`,
 *       its parts percent-decoded, an IPv6 host in brackets, and a path for
 *       SQLite's file (`sqlite:///data/app.db`, `sqlite::memory:`); and
 *       keywords, `key=value` pairs separated by spaces, as libpq writes them
 *       (`host=db port=5432 dbname=app`, a value with spaces in single quotes,
 *       `\` escaping a quote or a backslash), or by semicolons, as ODBC writes
 *       them (`Server=db;Database=app;Uid=me`). Keys are read without ASCII
 *       case. host, server and hostaddr name the host; port; dbname,
 *       database and initial catalog the database; user, username, uid and
 *       user id the user; password and pwd the password; charset and
 *       client_encoding the character set; schema and search_path the
 *       schema; sslmode (disable, allow, prefer, require, verify-ca,
 *       verify-full) enable_ssl and verify_ssl; sslcert, sslkey, sslrootcert
 *       and sslca the files; connect_timeout, in seconds, the connect
 *       timeout. Any other key goes into custom_options, as does the URI's
 *       scheme, under "scheme".
 *
 * @param[in] _connection_string  the string.
 * @return the configuration, every field the string does not set left as
 *         connection_config() leaves it.
 * @throws exception with D_DB_INVALID_ARGUMENT for a string that is neither
 *         form: an unterminated quote, a pair without `=`, a port that is no
 *         number from 1 to 65535, a malformed percent escape.
 */
connection_config parse_connection_string(
    const std::string& _connection_string);

/**
 * @brief Writes a configuration as a connection string, in the keyword form
 *        that parse_connection_string reads back as the same connection.
 *
 * @note Writes host, port, dbname, user and password where set;
 *       client_encoding (the charset), schema and the SSL files where set;
 *       sslmode when enable_ssl is true (`verify-full`, or `require` without
 *       verify_ssl); connect_timeout, in whole seconds, when it is not the
 *       default; and custom_options. A value that is empty or holds a space,
 *       a quote or a backslash is quoted. libpq reads the string as well
 *       where it names no schema and no custom option libpq does not know,
 *       "scheme" among them.
 *
 * @param[in] _config  the configuration.
 * @return the string.
 */
std::string build_connection_string(
    const connection_config& _config);

// 6.2    Quoting and escaping
//------------------------------------------------------------------------------
/**
 * @brief Escapes text for a string literal in standard SQL: each `'` doubled.
 *
 * @note The result goes between single quotes, which it does not include,
 *       in a dialect where a backslash is an ordinary character: SQLite,
 *       PostgreSQL (standard_conforming_strings, on by default), MySQL and
 *       MariaDB only under NO_BACKSLASH_ESCAPES. A MySQL-family
 *       connection's own escape_string suits that family in any mode, and a
 *       bound parameter suits every database.
 *
 * @param[in] _input  the text.
 * @return the escaped text.
 * @throws exception with D_DB_INVALID_ARGUMENT for text that holds a NUL,
 *         which no SQL string literal can.
 */
std::string escape_string(const std::string& _input);

/**
 * @brief Quotes a name -- a table's, a column's -- as one identifier in a
 *        vendor's dialect.
 *
 * @note MySQL, MariaDB, ArangoDB and Neo4j quote with backticks, Microsoft
 *       SQL Server with brackets, and every other vendor with double quotes,
 *       as the SQL standard does; a quote character inside the name is
 *       doubled. The whole name is one identifier: a qualified name is its
 *       quoted parts joined by `.`.
 *
 * @param[in] _identifier     the name.
 * @param[in] _database_type  the vendor.
 * @return the quoted name.
 * @throws exception with D_DB_INVALID_ARGUMENT for a name that holds a NUL.
 */
std::string quote_identifier(const std::string& _identifier,
                             database_type      _database_type);

// 6.3    Values as text
//------------------------------------------------------------------------------
/**
 * @brief Writes a value as text.
 *
 * @note NULL is `NULL`; a boolean `true` or `false`; an integer in decimal;
 *       a double in the fewest digits that read back as the same double, or
 *       `NaN`, `Infinity` and `-Infinity`; text as it is; bytes in lower-case
 *       hexadecimal; a point in time as its UTC date and time,
 *       `YYYY-MM-DD HH:MM:SS`, with a fraction of a second where there is
 *       one, to the clock's precision and without trailing zeros.
 *
 * @param[in] _value  the value.
 * @return the text, which string_to_value reads back as the same value given
 *         the matching field_type.
 */
std::string value_to_string(const value& _value);

/**
 * @brief Reads text as a value of a column type.
 *
 * @note `null` gives NULL, whatever the text; `boolean` reads true or false
 *       as PostgreSQL does, in any ASCII case (t, true, y, yes, on, 1 and
 *       their opposites); `integer` a 32-bit integer and `big_integer` a 64-bit
 *       one, an optional sign and digits; `floating_point` a number, or NaN,
 *       Infinity and -Infinity; `binary` hexadecimal, optionally after `\x`
 *       or `0x`; `date`, `datetime` and `timestamp` a point in time,
 *       `YYYY-MM-DD`, then optionally ` HH:MM:SS` or `THH:MM:SS`, a
 *       fraction, and `Z` or an offset (`+00`, `+05:30`, `-0800`), UTC when
 *       there is none, a date alone being its midnight; every other type --
 *       decimal, string, time, json, xml, uuid, array, custom -- the text as
 *       it is, so that an exact decimal stays exact.
 *
 * @param[in] _str   the text.
 * @param[in] _type  the column type it is read as.
 * @return the value.
 * @throws type_conversion_exception with D_DB_MISMATCH for text that is not
 *         a value of the type, or an integer out of the type's range.
 */
value string_to_value(const std::string& _str,
                      field_type         _type);

// 6.4    Names
//------------------------------------------------------------------------------
// names -- a vendor's display name ("PostgreSQL", "Microsoft SQL Server",
// "unknown" for unknown), and a field type's enumerator name ("big_integer"),
// the C layers' d_db_type_name for the types the two share
std::string get_type_name(database_type _type);
std::string get_field_type_name(field_type _type);


//==============================================================================
// 7.  SERIALIZABILITY
//==============================================================================
// The database target of the container render fold (containers.tex,
// "Externalisation as a fold" and "Database serializability"). A container
// renders to a store through the tabular template Theta, and folds twice:
//
//       container  --enc_T-->  Theta  --[[.]]_D-->  store
//
// Adequacy (enc_T: the container is table-shaped) times coverage ([[.]]_D:
// the store speaks Theta) is database serializability. Every SQL engine
// covers Theta, so one encoder per container shape and one renderer per
// engine suffice: N + M, never N x M. A vendor's dialect -- identifier
// quoting, type spelling, the statements that change a table -- is the
// store side, and already lives below the fold: in the vendor's C layer,
// which spells each column's type and every change to a table, in
// quote_identifier, and in database_table's write path, its commit().
// store_table and load_table are declared here and defined in
// database_table_render.hpp, which owns the encoder, so that this header
// includes nothing of database_table.hpp.


// 7.1    The store target
//------------------------------------------------------------------------------
// 7.1.1
// store_direction
//   enum: which way the store target of the render fold runs.
enum class store_direction
{
    store,     // container -> Theta -> store
    retrieve   // store -> Theta -> container
};

// 7.1.2
// store_table
//   function template: renders a table-shaped container into the store
// behind a connection, through Theta -- a database_table bound to that
// connection by its commit(), which changes it, so it is taken by a
// forwarding reference; defined in database_table_render.hpp.
template<typename Connection,
         typename RenderableTable>
void store_table(Connection&       _connection,
                 RenderableTable&& _table);

// 7.1.3
// load_table
//   function template: the adjoint direction, a table-shaped container
// filled from the store behind a connection -- a database_table bound to
// that connection by its refresh(); defined in database_table_render.hpp.
template<typename Connection,
         typename RenderableTable>
void load_table(Connection&      _connection,
                RenderableTable& _table);


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_HPP
