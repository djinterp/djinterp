/*******************************************************************************
* djinterp [core]                                               mysql_common.hpp
*
* djinterp MySQL-family common connection module:
*   The connection MariaDB and Oracle's MySQL share, mysql_common_connection,
* over the MariaDB layer's C++ face (c/db/mariadb/mariadb_face.hpp), whose
* connection it holds: the layer serves both servers through either client,
* MariaDB Connector/C or libmysqlclient. Every operation is the layer's, and
* core/db adds exceptions, result sets that outlive their query, and
* prepared statements that close with the connection. Beside the
* operations every connection has, the family's own: the character set,
* scripts of several statements, escaping, the current database, changing
* user, the server's diagnostics, the default storage engine, client
* options and flags, connect attributes, and the schema's tables.
*     mariadb_connection, mysql_connection (the leaves)
*       -> mysql_common_connection<leaf, X>     this file
*         -> database_connection<leaf, X>
*           -> connection_template<leaf, X>
*             -> connection<leaf>
*   Each leaf adds its settings and its session set-up through two hooks
* (mysql_common_connection's description), and instantiates this template's
* members explicitly in its source. The header needs C++17 and includes no
* client header: env includes mysql.h only where a program asks for env's
* detection (D_CFG_ENV_USING_MARIADB, D_CFG_ENV_USING_MYSQL). Its trailing
* sections carry the family's capability-detection traits and C++20
* concepts.
*
*
* path:      /inc/djinterp/core/db/mysql/mysql_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.16
*******************************************************************************/

#ifndef DJINTERP_DB_MYSQL_MYSQL_COMMON_HPP
#define DJINTERP_DB_MYSQL_MYSQL_COMMON_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: this header's floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>   // std::chrono::milliseconds
#include <cstddef>  // std::size_t
#include <cstring>  // std::strlen
#include <deque>    // std::deque
#include <map>      // std::map
#include <string>   // std::string, std::to_string
#include <utility>  // std::pair
#include <vector>   // std::vector
// djinterp
#include "../../../djinterp.hpp"                   // framework root
#include "../../../c/db/mariadb/mariadb_face.hpp"  // face::mariadb, the layer
#include "../../../config/c/db/mariadb/cfg_mariadb_common.h"  // the charset
#include "../../../env/db/mysql/env_mysql_common.h"  // D_ENV_MYSQL_*
#include "../../meta/type_utility.hpp"             // clean_t
#include "../database_connection.hpp"              // database_connection
#include "../database_statement.hpp"               // prepared_statement,
                                                   // failure_kind
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::int64_t


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
//   struct: what a MySQL-family connection needs beyond connection_config:
// the client flags, the Unix socket, the character set, an init command,
// compression, LOAD DATA LOCAL, and connect attributes. multi_statements is
// refused at connect: the layer runs several statements through
// execute_script, which turns the client's mode on for that call alone.
struct mysql_connect_config
{
    connection_config base;
    mysql_client_flag client_flags;
    std::string       unix_socket;
    std::string       default_charset;   // "utf8mb4"
    std::string       init_command;
    bool              use_compression;
    bool              use_local_infile;
    bool              multi_statements;  // refused: execute_script instead

    std::map<std::string, std::string> connect_attributes;

    mysql_connect_config()
        : client_flags(mysql_client_flag::none),
          default_charset("utf8mb4"),
          use_compression(false),
          use_local_infile(false),
          multi_statements(false)
    {}

    explicit mysql_connect_config(
        const connection_config& _base
    )
        : base(_base),
          client_flags(mysql_client_flag::none),
          default_charset("utf8mb4"),
          use_compression(false),
          use_local_infile(false),
          multi_statements(false)
    {}
};


// ===========================================================================
// III. MYSQL-FAMILY COMMON CONNECTION TEMPLATE
// ===========================================================================

// mariadb_layer
//   struct: the MariaDB layer, which serves MariaDB and MySQL through either
// client, as database_statement.hpp's templates take a layer. A result
// means the connection was lost when it is the client's 2006 (gone away),
// 2013 (lost) or 2055 (lost, with a system error), MySQL's 4031 (dropped
// for inactivity), MariaDB's 1927 (killed) or either server's 1053 (shutting
// down). A point in time is bound as its UTC date and time alone, which
// neither server reads an offset in before MySQL 8.0.19.
struct mariadb_layer
{
    using statement_face = face::mariadb_statement;
    using handle_type    = struct ::d_mariadb*;

    static constexpr const char* utc_offset = "";

    static constexpr bool
    connection_lost(
        int _result
    ) noexcept
    {
        return ( (_result == 2006) ||
                 (_result == 2013) ||
                 (_result == 2055) ||
                 (_result == 4031) ||
                 (_result == 1927) ||
                 (_result == 1053) );
    }

    /**
     * @brief Describes a failed call of the MariaDB layer, as raise does.
     *
     * @note The connection's message describes its latest failure only
     *       until the next call on it, which is why an operation that must
     *       call again before it throws -- change_user's ping -- describes
     *       the failure first.
     *
     * @param[in] _db         the connection the call ran on; may be `NULL`.
     * @param[in] _operation  the operation's name.
     * @param[in] _result     the call's result.
     * @return the server's name, the operation, the connection's message --
     *         the result's own, for a result of the layer's -- the result,
     *         and an SQLSTATE that says more than the number.
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
     * @brief Throws the exception for a failed call of the MariaDB layer.
     *
     * @param[in] _db         the connection the call ran on; may be `NULL`.
     * @param[in] _kind       which exception to throw, unless `_result`
     *                        means the connection was lost.
     * @param[in] _operation  the operation's name, for the message.
     * @param[in] _result     the call's result.
     * @throws connection_exception for a lost connection, otherwise as
     *         `_kind` says, with `_result` as its error_code() and, for a
     *         client or server error, the connection's message and SQLSTATE.
     */
    [[noreturn]] static void raise(handle_type  _db,
                                   failure_kind _kind,
                                   const char*  _operation,
                                   int          _result);
};

// prepared_statement<mariadb_layer>
//   class: MariaDB's and MySQL's prepared statements, instantiated once, in
// mysql_common.cpp.
extern template class prepared_statement<mariadb_layer>;

// internal::mariadb_settings
//   class: the layer's client settings for one connect, with the text they
// point to kept alive beside them, in a std::deque whose elements never
// move. Not copyable.
NS_INTERNAL

    class mariadb_settings
    {
    public:
        mariadb_settings() = default;

        mariadb_settings(const mariadb_settings&)            = delete;
        mariadb_settings& operator=(const mariadb_settings&) = delete;

        // adding -- a number, a text, a connect attribute, or a native
        // option; each throws std::bad_alloc when memory runs out
        void
        add_number(
            enum d_mariadb_setting_name _name,
            unsigned long               _number
        )
        {
            m_settings.push_back(
                d_mariadb_setting{ _name, _number, nullptr, nullptr });

            return;
        }

        void
        add_text(
            enum d_mariadb_setting_name _name,
            const std::string&          _text
        )
        {
            internal::refuse_nul_setting(_text, "connect", "a setting");
            m_texts.push_back(_text);
            m_settings.push_back(
                d_mariadb_setting{ _name, 0ul, m_texts.back().c_str(),
                                   nullptr });

            return;
        }

        void
        add_attribute(
            const std::string& _name,
            const std::string& _value
        )
        {
            internal::refuse_nul_setting(_name,
                                         "connect",
                                         "a connect attribute's name");
            internal::refuse_nul_setting(_value, "connect", _name.c_str());
            m_texts.push_back(_name);

            const char* const name = m_texts.back().c_str();

            m_texts.push_back(_value);
            m_settings.push_back(
                d_mariadb_setting{ D_MARIADB_SETTING_CONNECT_ATTRIBUTE, 0ul,
                                   name, m_texts.back().c_str() });

            return;
        }

        void
        add_native(
            int         _option,
            const void* _value
        )
        {
            m_settings.push_back(
                d_mariadb_setting{ D_MARIADB_SETTING_NATIVE,
                                   static_cast<unsigned long>(_option),
                                   nullptr,
                                   _value });

            return;
        }

        // reading -- whether a setting of a name is there, and the array
        bool
        has(
            enum d_mariadb_setting_name _name
        ) const noexcept
        {
            // any setting of the name
            for (const d_mariadb_setting& setting : m_settings)
            {
                if (setting.name == _name)
                {
                    return true;
                }
            }

            return false;
        }

        const d_mariadb_setting*
        data() const noexcept
        {
            return m_settings.data();
        }

        std::size_t
        size() const noexcept
        {
            return m_settings.size();
        }

    private:
        std::vector<d_mariadb_setting> m_settings;
        std::deque<std::string>        m_texts;
    };

NS_END  // internal

// mysql_common_connection
//   class template: the connection MariaDB and MySQL share, over the MariaDB
// layer's face, which it holds and which closes the connection when it
// goes. Derived is mariadb_connection or mysql_connection, DbType its
// database_type; each adds its own settings and session set-up through two
// hooks, add_settings_helper(internal::mariadb_settings&), which may also
// refuse what the layer cannot apply, and on_connected_helper(face::mariadb&),
// run on the new connection before the object takes it.
//   Connecting applies connection_config and mysql_connect_config: where and
// as whom; the timeouts, in seconds rounded up, 0 leaving the client's; TLS,
// REQUIRED with enable_ssl, VERIFY_IDENTITY with verify_ssl too; the files;
// compression; LOAD DATA LOCAL; an init command; connect attributes; client
// flags beyond those the other settings stand for; native options
// (set_option); and, once connected, a character set other than the
// layer's. max_packet_size is left to the client. Parameters are written
// `?`. Each statement reads every result it produces, so next_result has no
// result left to read. Not copyable; movable.
template<typename      Derived,
         database_type DbType>
class mysql_common_connection
    : public database_connection<Derived, DbType>
{
public:
    using base_type      = database_connection<Derived, DbType>;
    using traits_type    = typename base_type::traits_type;
    using layer_type     = mariadb_layer;
    using statement_type = prepared_statement<mariadb_layer>;

    // -----------------------------------------------------------------
    // character set management
    // -----------------------------------------------------------------

    // character set -- the session's, as the client sets it, so that
    // escaping follows it; set keeps it for the next connect too
    void        set_charset(const std::string& _charset);
    std::string get_charset() const;

    // -----------------------------------------------------------------
    // multi-result sets and scripts
    // -----------------------------------------------------------------

    // results -- every statement reads all the results it produces, so
    // next_result answers -1, no more, and more_results false
    int  next_result();
    bool more_results() const;

    // execute_script -- several statements, separated by semicolons, the
    // client's multi-statement mode on for that call alone
    void execute_script(const std::string& _sql);

    // -----------------------------------------------------------------
    // string escaping
    // -----------------------------------------------------------------

    /**
     * @brief Escapes text for a string literal, as the connection's
     *        character set and SQL mode require (mysql_real_escape_string),
     *        without the quotes around it.
     *
     * @param[in] _input  the text.
     * @return the escaped text.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for text that holds
     *         a NUL, and with the client's number when it refuses; as every
     *         operation when the connection is not open.
     */
    std::string escape_string(const std::string& _input) const;

    // -----------------------------------------------------------------
    // database selection and users
    // -----------------------------------------------------------------

    // use_database -- the connection's current database, which the
    // configuration then names
    void use_database(const std::string& _database);

    /**
     * @brief Changes the user the connection is authenticated as.
     *
     * @note MariaDB keeps the connection when the server refuses the user;
     *       MySQL closes it, which is then lost.
     *
     * @param[in] _user      the user.
     * @param[in] _password  the password.
     * @param[in] _database  the current database to take; "" for none.
     * @throws connection_exception when the refusal closed the connection;
     *         query_exception, with the client's number, when it did not.
     */
    void change_user(const std::string& _user,
                     const std::string& _password,
                     const std::string& _database);

    // -----------------------------------------------------------------
    // server diagnostics and settings
    // -----------------------------------------------------------------

    // diagnostics -- the server's summary of itself (mysql_stat), which
    // throws as every operation; the connection's id on the server, the
    // latest statement's warning count and SQLSTATE, as the client keeps
    // them, 0 and "" when closed
    std::string   get_stat() const;
    unsigned long get_thread_id() const noexcept;
    unsigned int  get_warning_count() const noexcept;
    std::string   get_sqlstate() const;

    // engine -- the session's default storage engine
    std::string get_engine() const;
    void        set_engine(const std::string& _engine);

    // set_autocommit -- set_auto_commit's MySQL spelling
    void set_autocommit(bool _enabled);

    // -----------------------------------------------------------------
    // options, flags and attributes, for the next connect
    // -----------------------------------------------------------------

    // set_option -- a mysql_options option and its argument, applied as
    // given at each connect from now on; the argument must outlive them
    void set_option(int         _option,
                    const void* _value);

    // client flags -- CLIENT_* bits the next connect asks for
    mysql_client_flag get_client_flags() const noexcept;
    void              set_client_flags(mysql_client_flag _flags) noexcept;
    void              add_client_flag(mysql_client_flag _flag) noexcept;
    bool              has_client_flag(mysql_client_flag _flag) const noexcept;

    // connect attributes -- name and value, which performance_schema shows
    void set_connect_attribute(const std::string& _key,
                               const std::string& _value);
    const std::map<std::string, std::string>&
         get_connect_attributes() const noexcept;

    // configuration -- the family's settings; setting them replaces the
    // generic configuration with theirs, for the next connect
    const mysql_connect_config& get_mysql_config() const noexcept;
    void                        set_mysql_config(
                                    const mysql_connect_config& _config);

    // -----------------------------------------------------------------
    // schema introspection
    // -----------------------------------------------------------------

    // schema -- whether a table of the current database exists, which
    // throws when it cannot tell; and its tables' names, sorted by bytes
    bool                     has_table(const std::string& _table_name) const;
    std::vector<std::string> list_tables() const;

    // -----------------------------------------------------------------
    // compile-time vendor queries
    // -----------------------------------------------------------------

    static constexpr bool
    is_mysql_family() noexcept
    {
        return true;
    }

    static constexpr bool
    is_mariadb() noexcept
    {
        return (DbType == database_type::mariadb);
    }

    static constexpr bool
    is_oracle_mysql() noexcept
    {
        return (DbType == database_type::mysql);
    }

    // get_face -- the MariaDB layer's face, for the layer's other calls;
    // closing or replacing the connection through it is the connection's to
    // do, not the face's
    face::mariadb&       get_face() noexcept;
    const face::mariadb& get_face() const noexcept;

    // -----------------------------------------------------------------
    // hooks: database_connection calls these, programs the operations
    // they implement
    // -----------------------------------------------------------------

    void            connect_helper();
    void            disconnect_helper() noexcept;
    bool            is_connected_helper() const noexcept;
    bool            ping_helper() const noexcept;
    face::rows      execute_query_helper(const std::string& _query);
    re_std::int64_t execute_update_helper(const std::string& _query);
    face::mariadb_statement
                    prepare_helper(const std::string& _query);
    std::string     get_server_version_helper() const;
    re_std::int64_t get_last_insert_id_helper() const noexcept;
    void            begin_transaction_helper();
    void            begin_transaction_helper(isolation_level _isolation);
    void            commit_helper();
    void            rollback_helper();
    bool            in_transaction_helper() const noexcept;
    isolation_level get_isolation_level_helper() const;
    void            set_isolation_level_helper(isolation_level _level);
    void            create_savepoint_helper(const std::string& _name);
    void            release_savepoint_helper(const std::string& _name);
    void            rollback_to_savepoint_helper(const std::string& _name);
    void            set_auto_commit_helper(bool _enabled);
    bool            get_auto_commit_helper() const;
    struct ::d_mariadb*
                    native_handle_helper() const noexcept;

protected:
    mysql_common_connection();
    explicit mysql_common_connection(const connection_config& _config);
    explicit mysql_common_connection(const mysql_connect_config& _config);

    /**
     * @brief Closes the connection, its prepared statements first.
     */
    ~mysql_common_connection();

    mysql_common_connection(const mysql_common_connection&)            = delete;
    mysql_common_connection& operator=(const mysql_common_connection&) = delete;

    mysql_common_connection(mysql_common_connection&&) noexcept = default;
    mysql_common_connection&
    operator=(mysql_common_connection&&) noexcept = default;

    // m_check -- nothing for 0; otherwise what the layer raises
    void m_check(int          _result,
                 failure_kind _kind,
                 const char*  _operation) const;

    mysql_connect_config m_mysql_config;

private:
    // m_derived -- the leaf, for its two hooks
    Derived& m_derived() noexcept;

    face::mariadb                            m_db;
    std::vector<std::pair<int, const void*>> m_native_options;
};


// ===========================================================================
// III.b MYSQL-FAMILY COMMON CONNECTION DEFINITIONS
// ===========================================================================
// mysql_common_connection's members, which each leaf's source instantiates
// explicitly for its leaf, and its header declares that instantiation
// extern.

NS_INTERNAL

    // mysql_isolation_to_layer, mysql_isolation_from_layer
    //   function: isolation_level and the layer's levels, both ways; the
    // default level is the layer's default, repeatable read.
    inline enum d_mariadb_isolation
    mysql_isolation_to_layer(
        isolation_level _level
    ) noexcept
    {
        // the four the standard names
        switch (_level)
        {
            case isolation_level::read_uncommitted:
                return D_MARIADB_ISOLATION_READ_UNCOMMITTED;

            case isolation_level::read_committed:
                return D_MARIADB_ISOLATION_READ_COMMITTED;

            case isolation_level::serializable:
                return D_MARIADB_ISOLATION_SERIALIZABLE;

            case isolation_level::repeatable_read:
            case isolation_level::default_level:
            default:
                return D_MARIADB_ISOLATION_REPEATABLE_READ;
        }
    }

    inline isolation_level
    mysql_isolation_from_layer(
        enum d_mariadb_isolation _level
    ) noexcept
    {
        // the four the servers have
        switch (_level)
        {
            case D_MARIADB_ISOLATION_READ_UNCOMMITTED:
                return isolation_level::read_uncommitted;

            case D_MARIADB_ISOLATION_READ_COMMITTED:
                return isolation_level::read_committed;

            case D_MARIADB_ISOLATION_SERIALIZABLE:
                return isolation_level::serializable;

            case D_MARIADB_ISOLATION_REPEATABLE_READ:
            default:
                return isolation_level::repeatable_read;
        }
    }

    // mysql_seconds
    //   function: a timeout in whole seconds, rounded up, as the client
    // takes one.
    inline unsigned long
    mysql_seconds(
        std::chrono::milliseconds _timeout
    ) noexcept
    {
        return static_cast<unsigned long>(
            (_timeout.count() + 999) / 1000);
    }

    // mysql_or_null
    //   function: a string the client reads as a C string, NULL for empty,
    // which it reads as its default.
    inline const char*
    mysql_or_null(
        const std::string& _text
    ) noexcept
    {
        return (_text.empty()) ? nullptr
                               : _text.c_str();
    }

NS_END  // internal

template<typename      Derived,
         database_type DbType>
mysql_common_connection<Derived, DbType>::mysql_common_connection()
    : base_type(),
      m_mysql_config(),
      m_db(),
      m_native_options()
{
    m_mysql_config.base = this->m_config;
}

template<typename      Derived,
         database_type DbType>
mysql_common_connection<Derived, DbType>::mysql_common_connection(
    const connection_config& _config
)
    : base_type(_config),
      m_mysql_config(_config),
      m_db(),
      m_native_options()
{
    m_mysql_config.base = this->m_config;
}

template<typename      Derived,
         database_type DbType>
mysql_common_connection<Derived, DbType>::mysql_common_connection(
    const mysql_connect_config& _config
)
    : base_type(_config.base),
      m_mysql_config(_config),
      m_db(),
      m_native_options()
{
    m_mysql_config.base = this->m_config;
}

/*
mysql_common_connection::~mysql_common_connection
  This class holds the face, so it closes the session: by the time the base's
destructor runs, the face is gone, and the leaf's is gone already, so no
hook is called.
*/
template<typename      Derived,
         database_type DbType>
mysql_common_connection<Derived, DbType>::~mysql_common_connection()
{
    this->close_session_noexcept();
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_charset(
    const std::string& _charset
)
{
    this->run_guarded(
        [this, &_charset]() -> void
        {
            internal::refuse_nul(_charset, "set_charset", "the name");
            m_check(m_db.set_charset(_charset),
                    failure_kind::query,
                    "set_charset");

            return;
        });

    m_mysql_config.default_charset = _charset;
    this->m_config.charset         = _charset;

    return;
}

template<typename      Derived,
         database_type DbType>
std::string
mysql_common_connection<Derived, DbType>::get_charset() const
{
    return this->ask_guarded(
        [this]() -> std::string
        {
            std::string charset;

            m_check(m_db.get_charset(charset),
                    failure_kind::query,
                    "get_charset");

            return charset;
        });
}

template<typename      Derived,
         database_type DbType>
int
mysql_common_connection<Derived, DbType>::next_result()
{
    this->ensure_connected();

    return -1;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::more_results() const
{
    return false;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::execute_script(
    const std::string& _sql
)
{
    this->run_guarded(
        [this, &_sql]() -> void
        {
            internal::refuse_nul(_sql, "execute_script", "the SQL");
            m_check(m_db.exec_script(_sql),
                    failure_kind::query,
                    "execute_script");

            return;
        });

    return;
}

/*
mysql_common_connection::escape_string
  The client quotes a literal whole; the quotes it adds are taken off, so
that the text goes between a caller's own.
*/
template<typename      Derived,
         database_type DbType>
std::string
mysql_common_connection<Derived, DbType>::escape_string(
    const std::string& _input
) const
{
    // the layer takes a C string, which a NUL would end
    if (_input.find('\0') != std::string::npos)
    {
        throw query_exception(std::string(traits_type::display_name) +
                              " escape_string: the text holds a NUL "
                              "character",
                              D_DB_INVALID_ARGUMENT);
    }

    return this->ask_guarded(
        [this, &_input]() -> std::string
        {
            face::text quoted;

            m_db.quote_literal(_input, quoted);

            const std::size_t length = (quoted.is_null())
                                           ? 0u
                                           : std::strlen(quoted.c_str());

            // the client refused, for memory or for the text
            if (length < 2u)
            {
                m_check(D_DB_INVALID_ARGUMENT,
                        failure_kind::query,
                        "escape_string");
            }

            // the literal without its quotes
            return std::string(quoted.c_str() + 1, length - 2u);
        });
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::use_database(
    const std::string& _database
)
{
    this->run_guarded(
        [this, &_database]() -> void
        {
            internal::refuse_nul(_database, "use_database", "the name");
            m_check(m_db.use_database(_database),
                    failure_kind::query,
                    "use_database");

            return;
        });

    this->m_config.database = _database;

    return;
}

/*
mysql_common_connection::change_user
  MySQL closes the connection when it refuses the new user, MariaDB does not;
a ping after a refusal tells which happened, so that a closed connection is
reported as lost rather than left looking open. The refusal is described
before the ping, whose success would leave the client no words for it.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::change_user(
    const std::string& _user,
    const std::string& _password,
    const std::string& _database
)
{
    this->run_guarded(
        [this, &_user, &_password, &_database]() -> void
        {
            internal::refuse_nul(_user, "change_user", "the user");
            internal::refuse_nul(_password, "change_user", "the password");
            internal::refuse_nul(_database, "change_user", "the database");

            const int result = m_db.change_user(
                _user,
                _password,
                internal::mysql_or_null(_database));

            // a refusal, which closed the connection when the ping fails
            if (result != 0)
            {
                const std::string message =
                    mariadb_layer::describe(m_db.handle(),
                                            "change_user",
                                            result);

                mariadb_layer::raise_as((m_db.ping() != 0)
                                            ? failure_kind::connection
                                            : failure_kind::query,
                                        message,
                                        result);
            }

            return;
        });

    this->m_config.username = _user;
    this->m_config.password = _password;

    // a database named, or none
    if (!_database.empty())
    {
        this->m_config.database = _database;
    }

    return;
}

template<typename      Derived,
         database_type DbType>
std::string
mysql_common_connection<Derived, DbType>::get_stat() const
{
    return this->ask_guarded(
        [this]() -> std::string
        {
            std::string status;

            m_check(m_db.server_status(status),
                    failure_kind::query,
                    "get_stat");

            return status;
        });
}

template<typename      Derived,
         database_type DbType>
unsigned long
mysql_common_connection<Derived, DbType>::get_thread_id() const noexcept
{
    return m_db.connection_id();
}

template<typename      Derived,
         database_type DbType>
unsigned int
mysql_common_connection<Derived, DbType>::get_warning_count() const noexcept
{
    return m_db.warning_count();
}

template<typename      Derived,
         database_type DbType>
std::string
mysql_common_connection<Derived, DbType>::get_sqlstate() const
{
    const char* const state = m_db.sqlstate();

    return (state) ? state
                   : "";
}

template<typename      Derived,
         database_type DbType>
std::string
mysql_common_connection<Derived, DbType>::get_engine() const
{
    return this->ask_guarded(
        [this]() -> std::string
        {
            std::string engine;

            m_check(m_db.get_variable("default_storage_engine", engine),
                    failure_kind::query,
                    "get_engine");

            return engine;
        });
}

/*
mysql_common_connection::set_engine
  The layer takes a variable's value as SQL, so the name goes as a quoted
literal, which both servers read for an engine, and which nothing in it can
break out of.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_engine(
    const std::string& _engine
)
{
    // the layer takes a C string, which a NUL would end
    if (_engine.find('\0') != std::string::npos)
    {
        throw query_exception(std::string(traits_type::display_name) +
                              " set_engine: the name holds a NUL character",
                              D_DB_INVALID_ARGUMENT);
    }

    this->run_guarded(
        [this, &_engine]() -> void
        {
            face::text quoted;

            m_db.quote_literal(_engine, quoted);

            // the client refused, for memory or for the text
            if (quoted.is_null())
            {
                m_check(D_DB_INVALID_ARGUMENT,
                        failure_kind::query,
                        "set_engine");
            }

            m_check(m_db.set_variable("default_storage_engine",
                                      quoted.c_str()),
                    failure_kind::query,
                    "set_engine");

            return;
        });

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_autocommit(
    bool _enabled
)
{
    this->set_auto_commit(_enabled);

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_option(
    int         _option,
    const void* _value
)
{
    m_native_options.emplace_back(_option, _value);

    return;
}

template<typename      Derived,
         database_type DbType>
mysql_client_flag
mysql_common_connection<Derived, DbType>::get_client_flags() const noexcept
{
    return m_mysql_config.client_flags;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_client_flags(
    mysql_client_flag _flags
) noexcept
{
    m_mysql_config.client_flags = _flags;

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::add_client_flag(
    mysql_client_flag _flag
) noexcept
{
    m_mysql_config.client_flags |= _flag;

    return;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::has_client_flag(
    mysql_client_flag _flag
) const noexcept
{
    return (static_cast<unsigned long>(m_mysql_config.client_flags & _flag) !=
            0ul);
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_connect_attribute(
    const std::string& _key,
    const std::string& _value
)
{
    m_mysql_config.connect_attributes[_key] = _value;

    return;
}

template<typename      Derived,
         database_type DbType>
const std::map<std::string, std::string>&
mysql_common_connection<Derived, DbType>::get_connect_attributes()
    const noexcept
{
    return m_mysql_config.connect_attributes;
}

template<typename      Derived,
         database_type DbType>
const mysql_connect_config&
mysql_common_connection<Derived, DbType>::get_mysql_config() const noexcept
{
    return m_mysql_config;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_mysql_config(
    const mysql_connect_config& _config
)
{
    m_mysql_config = _config;
    this->m_config = _config.base;

    // the vendor's port where the configuration names none
    if (this->m_config.port == 0u)
    {
        this->m_config.port = traits_type::default_port;
    }

    return;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::has_table(
    const std::string& _table_name
) const
{
    return this->ask_guarded(
        [this, &_table_name]() -> bool
        {
            bool exists = false;

            internal::refuse_nul(_table_name, "has_table", "the name");
            m_check(m_db.has_table_s(face::string_arg(), _table_name, exists),
                    failure_kind::query,
                    "has_table");

            return exists;
        });
}

template<typename      Derived,
         database_type DbType>
std::vector<std::string>
mysql_common_connection<Derived, DbType>::list_tables() const
{
    return this->ask_guarded(
        [this]() -> std::vector<std::string>
        {
            std::vector<std::string> names;

            m_check(m_db.list_tables(face::string_arg(), names),
                    failure_kind::query,
                    "list_tables");

            return names;
        });
}

template<typename      Derived,
         database_type DbType>
face::mariadb&
mysql_common_connection<Derived, DbType>::get_face() noexcept
{
    return m_db;
}

template<typename      Derived,
         database_type DbType>
const face::mariadb&
mysql_common_connection<Derived, DbType>::get_face() const noexcept
{
    return m_db;
}

/*
mysql_common_connection::connect_helper
  The leaf's settings go first, so that a TLS mode of its own stands in for
enable_ssl's; the connection opens into a face of its own, is set up there --
the character set, the leaf's session settings -- and only then does the
object take it, so that a failure at any step leaves the old state and the
new connection closed. A failed connect is described by the client's own
words, which the layer copies before it closes what failed.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::connect_helper()
{
    const connection_config&   config = this->m_config;
    internal::mariadb_settings settings;
    const unsigned long        handled =
        static_cast<unsigned long>(mysql_client_flag::compress        |
                                   mysql_client_flag::ssl             |
                                   mysql_client_flag::connect_with_db |
                                   mysql_client_flag::local_files     |
                                   mysql_client_flag::multi_results);
    const unsigned long        flags =
        static_cast<unsigned long>(m_mysql_config.client_flags) & ~handled;

    // several statements a call are execute_script's, for that call alone
    if ( (m_mysql_config.multi_statements) ||
         ( (flags &
            static_cast<unsigned long>(mysql_client_flag::multi_statements))
           != 0ul ) )
    {
        throw connection_exception(std::string(traits_type::display_name) +
                                   " connect: multi_statements is not "
                                   "supported; execute_script runs several "
                                   "statements",
                                   D_DB_UNSUPPORTED);
    }

    // settings the client has no keyword for; a URI's scheme is none
    for (const std::pair<const std::string, std::string>& option :
         config.custom_options)
    {
        if (option.first != "scheme")
        {
            throw connection_exception(std::string(traits_type::display_name) +
                                       " connect: the custom option '" +
                                       option.first + "' is not supported; "
                                       "set the client's option with "
                                       "set_option",
                                       D_DB_UNSUPPORTED);
        }
    }

    internal::refuse_nul_setting(m_mysql_config.unix_socket,
                                 "connect",
                                 "unix_socket");
    internal::refuse_nul_setting(m_mysql_config.default_charset,
                                 "connect",
                                 "default_charset");
    m_derived().add_settings_helper(settings);

    // the timeouts the configuration gives
    if (config.connect_timeout.count() > 0)
    {
        settings.add_number(D_MARIADB_SETTING_CONNECT_TIMEOUT,
                            internal::mysql_seconds(config.connect_timeout));
    }

    if (config.read_timeout.count() > 0)
    {
        settings.add_number(D_MARIADB_SETTING_READ_TIMEOUT,
                            internal::mysql_seconds(config.read_timeout));
    }

    if (config.write_timeout.count() > 0)
    {
        settings.add_number(D_MARIADB_SETTING_WRITE_TIMEOUT,
                            internal::mysql_seconds(config.write_timeout));
    }

    // TLS, where the leaf named no mode of its own, and the files
    if ( (config.enable_ssl) &&
         (!settings.has(D_MARIADB_SETTING_SSL_MODE)) )
    {
        settings.add_number(
            D_MARIADB_SETTING_SSL_MODE,
            static_cast<unsigned long>((config.verify_ssl)
                                           ? D_MARIADB_SSL_VERIFY_IDENTITY
                                           : D_MARIADB_SSL_REQUIRED));
    }

    if (config.ssl_key)
    {
        settings.add_text(D_MARIADB_SETTING_SSL_KEY, *config.ssl_key);
    }

    if (config.ssl_cert)
    {
        settings.add_text(D_MARIADB_SETTING_SSL_CERT, *config.ssl_cert);
    }

    if (config.ssl_ca)
    {
        settings.add_text(D_MARIADB_SETTING_SSL_CA, *config.ssl_ca);
    }

    // the family's own settings
    if (m_mysql_config.use_compression)
    {
        settings.add_number(D_MARIADB_SETTING_COMPRESS, 1ul);
    }

    if (m_mysql_config.use_local_infile)
    {
        settings.add_number(D_MARIADB_SETTING_LOCAL_INFILE, 1ul);
    }

    if (!m_mysql_config.init_command.empty())
    {
        settings.add_text(D_MARIADB_SETTING_INIT_COMMAND,
                          m_mysql_config.init_command);
    }

    for (const std::pair<const std::string, std::string>& attribute :
         m_mysql_config.connect_attributes)
    {
        settings.add_attribute(attribute.first, attribute.second);
    }

    if (flags != 0ul)
    {
        settings.add_number(D_MARIADB_SETTING_CLIENT_FLAGS, flags);
    }

    for (const std::pair<int, const void*>& option : m_native_options)
    {
        settings.add_native(option.first, option.second);
    }

    const struct d_mariadb_options options =
    {
        internal::mysql_or_null(config.host),
        internal::mysql_or_null(config.username),
        internal::mysql_or_null(config.password),
        internal::mysql_or_null(config.database),
        static_cast<unsigned>(config.port),
        internal::mysql_or_null(m_mysql_config.unix_socket)
    };
    face::mariadb opened;
    face::text    reason;
    const int     result = opened.connect_with(&options,
                                               settings.data(),
                                               settings.size(),
                                               reason);

    // a failure, in the client's words where it gave some
    if (result != 0)
    {
        throw connection_exception(
            std::string(traits_type::display_name) + " connect: " +
                ((reason.is_null()) ? d_mariadb_result_message(result)
                                    : reason.c_str()) +
                " (" + std::to_string(result) + ")",
            result);
    }

    const std::string charset = (config.charset)
                                    ? *config.charset
                                    : m_mysql_config.default_charset;

    // a character set other than the one the layer asked for
    if ( (!charset.empty()) &&
         (charset != D_CFG_MARIADB_CHARSET) )
    {
        const int changed = opened.set_charset(charset);

        if (changed != 0)
        {
            mariadb_layer::raise(opened.handle(),
                                 failure_kind::connection,
                                 "set_charset",
                                 changed);
        }
    }

    m_derived().on_connected_helper(opened);
    m_db.swap(opened);

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::disconnect_helper() noexcept
{
    (void)m_db.close();

    return;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::is_connected_helper()
    const noexcept
{
    return m_db.is_open();
}

/*
mysql_common_connection::ping_helper
  A ping is a round trip that changes nothing a caller can see, and the
face's is not const, so the face is reached through const_cast.
*/
template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::ping_helper() const noexcept
{
    return (const_cast<face::mariadb&>(m_db).ping() == 0);
}

template<typename      Derived,
         database_type DbType>
face::rows
mysql_common_connection<Derived, DbType>::execute_query_helper(
    const std::string& _query
)
{
    face::rows found;

    m_check(m_db.query_rows(found, _query),
            failure_kind::query,
            "execute_query");

    return found;
}

template<typename      Derived,
         database_type DbType>
re_std::int64_t
mysql_common_connection<Derived, DbType>::execute_update_helper(
    const std::string& _query
)
{
    re_std::int64_t changes = 0;

    m_check(m_db.exec_changes(changes, _query),
            failure_kind::query,
            "execute_update");

    return changes;
}

template<typename      Derived,
         database_type DbType>
face::mariadb_statement
mysql_common_connection<Derived, DbType>::prepare_helper(
    const std::string& _query
)
{
    face::mariadb_statement prepared;

    m_check(m_db.prepare(prepared, _query), failure_kind::query, "prepare");

    return prepared;
}

template<typename      Derived,
         database_type DbType>
std::string
mysql_common_connection<Derived, DbType>::get_server_version_helper() const
{
    const char* const version = m_db.server_version();

    return (version) ? version
                     : "";
}

template<typename      Derived,
         database_type DbType>
re_std::int64_t
mysql_common_connection<Derived, DbType>::get_last_insert_id_helper()
    const noexcept
{
    return static_cast<re_std::int64_t>(m_db.last_insert_id());
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::begin_transaction_helper()
{
    m_check(m_db.begin_transaction(face::mariadb::default_transaction_mode()),
            failure_kind::transaction,
            "begin_transaction");

    return;
}

/*
mysql_common_connection::begin_transaction_helper (isolation_level)
  SET TRANSACTION ISOLATION LEVEL without SESSION sets the next transaction's
level alone, so the session's own level is untouched; the default level sets
nothing.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::begin_transaction_helper(
    isolation_level _isolation
)
{
    // the next transaction's level, where one is asked for
    if (_isolation != isolation_level::default_level)
    {
        m_check(m_db.set_next_isolation_level(
                    internal::mysql_isolation_to_layer(_isolation)),
                failure_kind::transaction,
                "begin_transaction");
    }

    begin_transaction_helper();

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::commit_helper()
{
    m_check(m_db.commit(), failure_kind::transaction, "commit");

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::rollback_helper()
{
    m_check(m_db.rollback(), failure_kind::transaction, "rollback");

    return;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::in_transaction_helper()
    const noexcept
{
    return m_db.in_transaction();
}

template<typename      Derived,
         database_type DbType>
isolation_level
mysql_common_connection<Derived, DbType>::get_isolation_level_helper() const
{
    enum d_mariadb_isolation level = D_MARIADB_ISOLATION_REPEATABLE_READ;

    m_check(m_db.get_isolation_level(level),
            failure_kind::transaction,
            "get_isolation_level");

    return internal::mysql_isolation_from_layer(level);
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_isolation_level_helper(
    isolation_level _level
)
{
    m_check(m_db.set_isolation_level(
                internal::mysql_isolation_to_layer(_level)),
            failure_kind::transaction,
            "set_isolation_level");

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::create_savepoint_helper(
    const std::string& _name
)
{
    m_check(m_db.create_savepoint(_name),
            failure_kind::transaction,
            "create_savepoint");

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::release_savepoint_helper(
    const std::string& _name
)
{
    m_check(m_db.release_savepoint(_name),
            failure_kind::transaction,
            "release_savepoint");

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::rollback_to_savepoint_helper(
    const std::string& _name
)
{
    m_check(m_db.rollback_to_savepoint(_name),
            failure_kind::transaction,
            "rollback_to_savepoint");

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::set_auto_commit_helper(
    bool _enabled
)
{
    m_check(m_db.set_autocommit(_enabled),
            failure_kind::transaction,
            "set_auto_commit");

    return;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::get_auto_commit_helper() const
{
    bool enabled = true;

    m_check(m_db.get_autocommit(enabled),
            failure_kind::transaction,
            "get_auto_commit");

    return enabled;
}

template<typename      Derived,
         database_type DbType>
struct ::d_mariadb*
mysql_common_connection<Derived, DbType>::native_handle_helper()
    const noexcept
{
    return m_db.handle();
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::m_check(
    int          _result,
    failure_kind _kind,
    const char*  _operation
) const
{
    // only a failure throws
    if (_result != 0)
    {
        mariadb_layer::raise(m_db.handle(), _kind, _operation, _result);
    }

    return;
}

template<typename      Derived,
         database_type DbType>
Derived&
mysql_common_connection<Derived, DbType>::m_derived() noexcept
{
    return static_cast<Derived&>(*this);
}


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
template<typename Type>
using mysql_set_charset_t = decltype(std::declval<Type&>().set_charset(
    std::declval<const std::string&>()));

// mysql_get_charset_t
//   detector: get_charset() const method.
template<typename Type>
using mysql_get_charset_t =
    decltype(std::declval<const Type&>().get_charset());

// -------------------------------------------------------------------------
// B.  multi-result set iteration
// -------------------------------------------------------------------------

// mysql_next_result_t
//   detector: next_result() method.
// wraps mysql_next_result() for iterating over multiple result sets
// from multi-statement queries or stored procedures.
template<typename Type>
using mysql_next_result_t =
    decltype(std::declval<Type&>().next_result());

// mysql_more_results_t
//   detector: more_results() const method.
// wraps mysql_more_results() to check if additional result sets
// remain.
template<typename Type>
using mysql_more_results_t =
    decltype(std::declval<const Type&>().more_results());

// -------------------------------------------------------------------------
// C.  options API
// -------------------------------------------------------------------------

// mysql_set_option_t
//   detector: set_option(int, const void*) method.
// wraps mysql_options() for setting connection options before
// connecting.
template<typename Type>
using mysql_set_option_t = decltype(std::declval<Type&>().set_option(
    std::declval<int>(),
    std::declval<const void*>()));

// mysql_get_option_t
//   detector: get_option(int, void*) const method.
// wraps mysql_get_option() for querying connection option values.
template<typename Type>
using mysql_get_option_t = decltype(std::declval<const Type&>().get_option(
    std::declval<int>(),
    std::declval<void*>()));

// -------------------------------------------------------------------------
// D.  server diagnostics
// -------------------------------------------------------------------------

// mysql_get_stat_t
//   detector: get_stat() const method.
// wraps mysql_stat() for server status string.
template<typename Type>
using mysql_get_stat_t =
    decltype(std::declval<const Type&>().get_stat());

// mysql_get_thread_id_t
//   detector: get_thread_id() const method.
// wraps mysql_thread_id() for the connection's thread identifier.
template<typename Type>
using mysql_get_thread_id_t =
    decltype(std::declval<const Type&>().get_thread_id());

// mysql_get_warning_count_t
//   detector: get_warning_count() const method.
// wraps mysql_warning_count().
template<typename Type>
using mysql_get_warning_count_t =
    decltype(std::declval<const Type&>().get_warning_count());

// mysql_get_sqlstate_t
//   detector: get_sqlstate() const method.
// wraps mysql_sqlstate() for SQLSTATE error code access.
template<typename Type>
using mysql_get_sqlstate_t =
    decltype(std::declval<const Type&>().get_sqlstate());

// -------------------------------------------------------------------------
// E.  result set mode
// -------------------------------------------------------------------------

// mysql_store_result_t
//   detector: store_result() method.
// wraps mysql_store_result() for buffered result sets.
template<typename Type>
using mysql_store_result_t =
    decltype(std::declval<Type&>().store_result());

// mysql_use_result_t
//   detector: use_result() method.
// wraps mysql_use_result() for streaming (unbuffered) result sets.
template<typename Type>
using mysql_use_result_t =
    decltype(std::declval<Type&>().use_result());

// -------------------------------------------------------------------------
// F.  auto-commit
// -------------------------------------------------------------------------

// mysql_set_autocommit_t
//   detector: set_autocommit(bool) method.
// wraps mysql_autocommit().
template<typename Type>
using mysql_set_autocommit_t = decltype(std::declval<Type&>().set_autocommit(
    std::declval<bool>()));

// -------------------------------------------------------------------------
// G.  escape and select
// -------------------------------------------------------------------------

// mysql_escape_string_t
//   detector: escape_string(const std::string&) const method.
// wraps mysql_real_escape_string().
template<typename Type>
using mysql_escape_string_t =
    decltype(std::declval<const Type&>().escape_string(
        std::declval<const std::string&>()));

// mysql_use_database_t
//   detector: use_database(const std::string&) method.
// wraps mysql_select_db() for switching the active database.
template<typename Type>
using mysql_use_database_t = decltype(std::declval<Type&>().use_database(
    std::declval<const std::string&>()));

// mysql_change_user_t
//   detector: change_user(const std::string&, const std::string&,
// const std::string&) method. wraps mysql_change_user().
template<typename Type>
using mysql_change_user_t = decltype(std::declval<Type&>().change_user(
    std::declval<const std::string&>(),
    std::declval<const std::string&>(),
    std::declval<const std::string&>()));

// -------------------------------------------------------------------------
// H.  storage engine
// -------------------------------------------------------------------------

// mysql_get_engine_t
//   detector: get_engine() const method.
// returns the default storage engine name.
template<typename Type>
using mysql_get_engine_t =
    decltype(std::declval<const Type&>().get_engine());

// mysql_set_engine_t
//   detector: set_engine(const std::string&) method.
// sets the session default storage engine.
template<typename Type>
using mysql_set_engine_t = decltype(std::declval<Type&>().set_engine(
    std::declval<const std::string&>()));


// ===========================================================================
// V.  TAGGED CAPABILITY TRAITS (struct-based)
// ===========================================================================

// has_mysql_charset
//   trait: checks if type Type supports character set management
// (set_charset + get_charset).
template<typename Type>
struct has_mysql_charset : djinterp::conjunction<
    is_detected<mysql_set_charset_t, clean_t<Type>>,
    is_detected<mysql_get_charset_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_charset_v
    //   value: convenience alias for has_mysql_charset<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_charset_v = has_mysql_charset<clean_t<Type>>::value;
#endif

// has_mysql_multi_result
//   trait: checks if type Type supports multi-result set iteration
// (next_result + more_results).
template<typename Type>
struct has_mysql_multi_result : djinterp::conjunction<
    is_detected<mysql_next_result_t, clean_t<Type>>,
    is_detected<mysql_more_results_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_multi_result_v
    //   value: convenience alias for has_mysql_multi_result<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_multi_result_v =
        has_mysql_multi_result<clean_t<Type>>::value;
#endif

// has_mysql_options
//   trait: checks if type Type supports the MySQL options API
// (set_option + get_option).
template<typename Type>
struct has_mysql_options : djinterp::conjunction<
    is_detected<mysql_set_option_t, clean_t<Type>>,
    is_detected<mysql_get_option_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_options_v
    //   value: convenience alias for has_mysql_options<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_options_v = has_mysql_options<clean_t<Type>>::value;
#endif

// has_mysql_diagnostics
//   trait: checks if type Type supports MySQL server diagnostics
// (get_stat + get_thread_id + get_warning_count + get_sqlstate).
template<typename Type>
struct has_mysql_diagnostics : djinterp::conjunction<
    is_detected<mysql_get_stat_t, clean_t<Type>>,
    is_detected<mysql_get_thread_id_t, clean_t<Type>>,
    is_detected<mysql_get_warning_count_t, clean_t<Type>>,
    is_detected<mysql_get_sqlstate_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_diagnostics_v
    //   value: convenience alias for has_mysql_diagnostics<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_diagnostics_v =
        has_mysql_diagnostics<clean_t<Type>>::value;
#endif

// has_mysql_result_modes
//   trait: checks if type Type supports both buffered and streaming
// result set modes (store_result + use_result).
template<typename Type>
struct has_mysql_result_modes : djinterp::conjunction<
    is_detected<mysql_store_result_t, clean_t<Type>>,
    is_detected<mysql_use_result_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_result_modes_v
    //   value: convenience alias for has_mysql_result_modes<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_result_modes_v =
        has_mysql_result_modes<clean_t<Type>>::value;
#endif

// has_mysql_escape
//   trait: checks if type Type supports MySQL string escaping.
template<typename Type>
struct has_mysql_escape : is_detected<mysql_escape_string_t, clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_escape_v
    //   value: convenience alias for has_mysql_escape<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_escape_v = has_mysql_escape<clean_t<Type>>::value;
#endif

// has_mysql_change_user
//   trait: checks if type Type supports runtime user switching.
template<typename Type>
struct has_mysql_change_user : is_detected<mysql_change_user_t, clean_t<Type>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_change_user_v
    //   value: convenience alias for has_mysql_change_user<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_change_user_v =
        has_mysql_change_user<clean_t<Type>>::value;
#endif

// has_mysql_engine
//   trait: checks if type Type supports storage engine queries
// (get_engine + set_engine).
template<typename Type>
struct has_mysql_engine : djinterp::conjunction<
    is_detected<mysql_get_engine_t, clean_t<Type>>,
    is_detected<mysql_set_engine_t, clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_mysql_engine_v
    //   value: convenience alias for has_mysql_engine<Type>::value.
    template<typename Type>
    constexpr bool has_mysql_engine_v = has_mysql_engine<clean_t<Type>>::value;
#endif

// is_mysql_connection
//   trait: compound trait verifying type Type implements a MySQL-
// family connection interface (vendor connection + charset +
// multi-result + diagnostics + escape).
template<typename Type>
struct is_mysql_connection : djinterp::conjunction<
    is_vendor_connection<clean_t<Type>>,
    has_mysql_charset<clean_t<Type>>,
    has_mysql_multi_result<clean_t<Type>>,
    has_mysql_diagnostics<clean_t<Type>>,
    has_mysql_escape<clean_t<Type>>>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_mysql_connection_v
    //   value: convenience alias for is_mysql_connection<Type>::value.
    template<typename Type>
    constexpr bool is_mysql_connection_v =
        is_mysql_connection<clean_t<Type>>::value;
#endif


// ===========================================================================
// VI. TAGLESS CAPABILITY TRAITS (constexpr bool)
// ===========================================================================

// -------------------------------------------------------------------------
// A.  individual capability tags
// -------------------------------------------------------------------------

// mysql_can_set_charset
//   tagless trait: true if Type has a set_charset() method.
template<typename Type,
         typename = void>
constexpr bool mysql_can_set_charset = false;

template<typename Type>
constexpr bool mysql_can_set_charset<Type,
    std::void_t<mysql_set_charset_t<Type>>> = true;

// mysql_can_get_charset
//   tagless trait: true if Type has a get_charset() method.
template<typename Type,
         typename = void>
constexpr bool mysql_can_get_charset = false;

template<typename Type>
constexpr bool mysql_can_get_charset<Type,
    std::void_t<mysql_get_charset_t<Type>>> = true;

// mysql_can_iterate_results
//   tagless trait: true if Type has next_result() for multi-result
// iteration.
template<typename Type,
         typename = void>
constexpr bool mysql_can_iterate_results = false;

template<typename Type>
constexpr bool mysql_can_iterate_results<Type,
    std::void_t<mysql_next_result_t<Type>>> = true;

// mysql_can_check_more_results
//   tagless trait: true if Type has more_results().
template<typename Type,
         typename = void>
constexpr bool mysql_can_check_more_results = false;

template<typename Type>
constexpr bool mysql_can_check_more_results<Type,
    std::void_t<mysql_more_results_t<Type>>> = true;

// mysql_can_set_option
//   tagless trait: true if Type has set_option().
template<typename Type,
         typename = void>
constexpr bool mysql_can_set_option = false;

template<typename Type>
constexpr bool mysql_can_set_option<Type,
    std::void_t<mysql_set_option_t<Type>>> = true;

// mysql_can_get_stat
//   tagless trait: true if Type has get_stat().
template<typename Type,
         typename = void>
constexpr bool mysql_can_get_stat = false;

template<typename Type>
constexpr bool mysql_can_get_stat<Type,
    std::void_t<mysql_get_stat_t<Type>>> = true;

// mysql_can_get_sqlstate
//   tagless trait: true if Type has get_sqlstate().
template<typename Type,
         typename = void>
constexpr bool mysql_can_get_sqlstate = false;

template<typename Type>
constexpr bool mysql_can_get_sqlstate<Type,
    std::void_t<mysql_get_sqlstate_t<Type>>> = true;

// mysql_can_store_result
//   tagless trait: true if Type has store_result().
template<typename Type,
         typename = void>
constexpr bool mysql_can_store_result = false;

template<typename Type>
constexpr bool mysql_can_store_result<Type,
    std::void_t<mysql_store_result_t<Type>>> = true;

// mysql_can_use_result
//   tagless trait: true if Type has use_result() (streaming).
template<typename Type,
         typename = void>
constexpr bool mysql_can_use_result = false;

template<typename Type>
constexpr bool mysql_can_use_result<Type,
    std::void_t<mysql_use_result_t<Type>>> = true;

// mysql_can_escape_string
//   tagless trait: true if Type has escape_string().
template<typename Type,
         typename = void>
constexpr bool mysql_can_escape_string = false;

template<typename Type>
constexpr bool mysql_can_escape_string<Type,
    std::void_t<mysql_escape_string_t<Type>>> = true;

// mysql_can_use_database
//   tagless trait: true if Type has use_database().
template<typename Type,
         typename = void>
constexpr bool mysql_can_use_database = false;

template<typename Type>
constexpr bool mysql_can_use_database<Type,
    std::void_t<mysql_use_database_t<Type>>> = true;

// mysql_can_change_user
//   tagless trait: true if Type has change_user().
template<typename Type,
         typename = void>
constexpr bool mysql_can_change_user = false;

template<typename Type>
constexpr bool mysql_can_change_user<Type,
    std::void_t<mysql_change_user_t<Type>>> = true;

// -------------------------------------------------------------------------
// B.  compound capability tags
// -------------------------------------------------------------------------

// mysql_does_charset
//   tagless trait: true if Type supports full charset management
// (set + get).
template<typename Type>
constexpr bool mysql_does_charset =
    ( mysql_can_set_charset<clean_t<Type>> &&
      mysql_can_get_charset<clean_t<Type>> );

// mysql_does_multi_result
//   tagless trait: true if Type supports multi-result iteration
// (next_result + more_results).
template<typename Type>
constexpr bool mysql_does_multi_result =
    ( mysql_can_iterate_results<clean_t<Type>>    &&
      mysql_can_check_more_results<clean_t<Type>> );

// mysql_does_result_modes
//   tagless trait: true if Type supports both buffered and streaming
// result sets.
template<typename Type>
constexpr bool mysql_does_result_modes =
    ( mysql_can_store_result<clean_t<Type>> &&
      mysql_can_use_result<clean_t<Type>> );

// mysql_does_diagnostics
//   tagless trait: true if Type supports server diagnostics
// (stat + sqlstate).
template<typename Type>
constexpr bool mysql_does_diagnostics =
    ( mysql_can_get_stat<clean_t<Type>>     &&
      mysql_can_get_sqlstate<clean_t<Type>> );

// mysql_is_full_connection
//   tagless trait: true if Type satisfies the complete MySQL-family
// connection interface (full vendor + charset + multi-result +
// diagnostics + escape).
template<typename Type>
constexpr bool mysql_is_full_connection =
    ( is_full_vendor<clean_t<Type>>              &&
      mysql_does_charset<clean_t<Type>>          &&
      mysql_does_multi_result<clean_t<Type>>     &&
      mysql_does_diagnostics<clean_t<Type>>      &&
      mysql_can_escape_string<clean_t<Type>> );


// ===========================================================================
// VII.  SFINAE HELPERS
// ===========================================================================

// enable_if_mysql_connection
//   type: SFINAE helper for MySQL-family connection constraints.
template<typename Type>
using enable_if_mysql_connection =
    typename std::enable_if<is_mysql_connection<clean_t<Type>>::value>::type;

// enable_if_has_mysql_charset
//   type: SFINAE helper for MySQL charset constraints.
template<typename Type>
using enable_if_has_mysql_charset =
    typename std::enable_if<has_mysql_charset<clean_t<Type>>::value>::type;

// enable_if_has_mysql_multi_result
//   type: SFINAE helper for MySQL multi-result constraints.
template<typename Type>
using enable_if_has_mysql_multi_result =
    typename std::enable_if<has_mysql_multi_result<clean_t<Type>>::value>::type;

// enable_if_has_mysql_diagnostics
//   type: SFINAE helper for MySQL diagnostics constraints.
template<typename Type>
using enable_if_has_mysql_diagnostics =
    typename std::enable_if<has_mysql_diagnostics<clean_t<Type>>::value>::type;


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
template<typename Type>
concept Mysql_connection =
    is_mysql_connection<clean_t<Type>>::value;

// non_mysql_connection
//   concept: constrains types that do not implement the MySQL-family
// connection interface.
template<typename Type>
concept non_mysql_connection =
    !Mysql_connection<Type>;

// mysql_charset_connection
//   concept: constrains MySQL-family connections supporting character-set
// management.
template<typename Type>
concept mysql_charset_connection =
    has_mysql_charset<clean_t<Type>>::value;

// mysql_multi_result_connection
//   concept: constrains MySQL-family connections supporting multi-result
// iteration.
template<typename Type>
concept mysql_multi_result_connection =
    has_mysql_multi_result<clean_t<Type>>::value;

// mysql_diagnostics_connection
//   concept: constrains MySQL-family connections supporting server
// diagnostics.
template<typename Type>
concept mysql_diagnostics_connection =
    has_mysql_diagnostics<clean_t<Type>>::value;

// mysql_result_modes_connection
//   concept: constrains MySQL-family connections supporting both buffered
// and streaming result modes.
template<typename Type>
concept mysql_result_modes_connection =
    has_mysql_result_modes<clean_t<Type>>::value;

// mysql_escape_connection
//   concept: constrains MySQL-family connections supporting SQL string
// escaping.
template<typename Type>
concept mysql_escape_connection =
    has_mysql_escape<clean_t<Type>>::value;

// mysql_options_connection
//   concept: constrains MySQL-family connections supporting the options API.
template<typename Type>
concept mysql_options_connection =
    has_mysql_options<clean_t<Type>>::value;

// mysql_engine_connection
//   concept: constrains MySQL-family connections supporting storage-engine
// queries.
template<typename Type>
concept mysql_engine_connection =
    has_mysql_engine<clean_t<Type>>::value;


// -------------------------------------------------------------------------
// B.  capability concepts
// -------------------------------------------------------------------------

// mysql_use_database_connection
//   concept: constrains types exposing use_database(database).
template<typename Type>
concept mysql_use_database_connection =
    is_detected<mysql_use_database_t, clean_t<Type>>::value;

// mysql_autocommit_connection
//   concept: constrains types exposing set_autocommit(bool).
template<typename Type>
concept mysql_autocommit_connection =
    is_detected<mysql_set_autocommit_t, clean_t<Type>>::value;

// mysql_set_option_connection
//   concept: constrains types exposing set_option(option, value).
template<typename Type>
concept mysql_set_option_connection =
    mysql_can_set_option<clean_t<Type>>;

// mysql_get_option_connection
//   concept: constrains types exposing get_option(option, value).
template<typename Type>
concept mysql_get_option_connection =
    is_detected<mysql_get_option_t, clean_t<Type>>::value;

// mysql_stat_connection
//   concept: constrains types exposing get_stat().
template<typename Type>
concept mysql_stat_connection =
    mysql_can_get_stat<clean_t<Type>>;

// mysql_thread_id_connection
//   concept: constrains types exposing get_thread_id().
template<typename Type>
concept mysql_thread_id_connection =
    is_detected<mysql_get_thread_id_t, clean_t<Type>>::value;

// mysql_warning_count_connection
//   concept: constrains types exposing get_warning_count().
template<typename Type>
concept mysql_warning_count_connection =
    is_detected<mysql_get_warning_count_t, clean_t<Type>>::value;

// mysql_sqlstate_connection
//   concept: constrains types exposing get_sqlstate().
template<typename Type>
concept mysql_sqlstate_connection =
    mysql_can_get_sqlstate<clean_t<Type>>;

// mysql_store_result_connection
//   concept: constrains types exposing store_result().
template<typename Type>
concept mysql_store_result_connection =
    mysql_can_store_result<clean_t<Type>>;

// mysql_use_result_connection
//   concept: constrains types exposing use_result().
template<typename Type>
concept mysql_use_result_connection =
    mysql_can_use_result<clean_t<Type>>;

// mysql_change_user_connection
//   concept: constrains types exposing change_user(user, password,
// database).
template<typename Type>
concept mysql_change_user_connection =
    has_mysql_change_user<clean_t<Type>>::value;

// mysql_set_engine_connection
//   concept: constrains types exposing set_engine(name).
template<typename Type>
concept mysql_set_engine_connection =
    is_detected<mysql_set_engine_t, clean_t<Type>>::value;

// mysql_get_engine_connection
//   concept: constrains types exposing get_engine().
template<typename Type>
concept mysql_get_engine_connection =
    is_detected<mysql_get_engine_t, clean_t<Type>>::value;


// -------------------------------------------------------------------------
// C.  tagless capability concepts
// -------------------------------------------------------------------------

// mysql_charset_manageable
//   concept: constrains types satisfying the tagless charset capability set.
template<typename Type>
concept mysql_charset_manageable =
    mysql_does_charset<clean_t<Type>>;

// mysql_multi_result_iterable
//   concept: constrains types satisfying the tagless multi-result
// capability set.
template<typename Type>
concept mysql_multi_result_iterable =
    mysql_does_multi_result<clean_t<Type>>;

// mysql_result_mode_selectable
//   concept: constrains types satisfying the tagless result-mode capability
// set.
template<typename Type>
concept mysql_result_mode_selectable =
    mysql_does_result_modes<clean_t<Type>>;

// mysql_diagnostic_connection_tagless
//   concept: constrains types satisfying the tagless diagnostics capability
// set.
template<typename Type>
concept mysql_diagnostic_connection_tagless =
    mysql_does_diagnostics<clean_t<Type>>;

// mysql_charset_settable
//   concept: constrains types satisfying the tagless set-charset capability.
template<typename Type>
concept mysql_charset_settable =
    mysql_can_set_charset<clean_t<Type>>;

// mysql_charset_gettable
//   concept: constrains types satisfying the tagless get-charset capability.
template<typename Type>
concept mysql_charset_gettable =
    mysql_can_get_charset<clean_t<Type>>;

// mysql_result_iterable
//   concept: constrains types satisfying the tagless next-result capability.
template<typename Type>
concept mysql_result_iterable =
    mysql_can_iterate_results<clean_t<Type>>;

// mysql_more_results_query
//   concept: constrains types satisfying the tagless more-results capability.
template<typename Type>
concept mysql_more_results_query =
    mysql_can_check_more_results<clean_t<Type>>;

// mysql_string_escapable
//   concept: constrains types satisfying the tagless escape-string
// capability.
template<typename Type>
concept mysql_string_escapable =
    mysql_can_escape_string<clean_t<Type>>;

// mysql_database_selectable
//   concept: constrains types satisfying the tagless select-db capability.
template<typename Type>
concept mysql_database_selectable =
    mysql_can_use_database<clean_t<Type>>;

// mysql_user_switchable
//   concept: constrains types satisfying the tagless change-user capability.
template<typename Type>
concept mysql_user_switchable =
    mysql_can_change_user<clean_t<Type>>;

// mysql_full_connection
//   concept: constrains types satisfying the complete tagless MySQL-family
// connection capability set.
template<typename Type>
concept mysql_full_connection =
    mysql_is_full_connection<clean_t<Type>>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MYSQL_MYSQL_COMMON_HPP
