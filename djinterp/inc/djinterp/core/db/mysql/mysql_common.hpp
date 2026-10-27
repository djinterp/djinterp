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
* user, resetting the session, what the server reports changing in it, the
* server's diagnostics, the default storage engine, client options and
* flags, connect attributes, and the schema's tables.
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
*                                                            revised: 2026.10.27
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CLIENT FLAGS
    ------------
    1.  The flags
         1.  mysql_client_flag
    2.  Operators
         1.  operator|
         2.  operator&
         3.  operator|=
2.  CONNECTION CONFIGURATION
    ------------------------
    1.  The configuration
         1.  mysql_connect_config
3.  MYSQL-FAMILY CONNECTION
    -----------------------
    1.  The layer
         1.  mariadb_layer
         2.  prepared_statement<mariadb_layer>
         3.  streaming_result_set<mariadb_layer>
    2.  Settings for a connect
         1.  internal::mariadb_settings
    3.  Session tracking
         1.  mysql_session_track
    4.  The class
         1.  mysql_common_connection
4.  MYSQL-FAMILY CONNECTION DEFINITIONS
    -----------------------------------
    1.  Helpers
         1.  internal::mysql_isolation_to_layer
         2.  internal::mysql_seconds
         3.  internal::mysql_or_null
    2.  Members
5.  EXPRESSION DETECTORS
    --------------------
    1.  Character set management
    2.  Multi-result set iteration
    3.  Options API
    4.  Server diagnostics
    5.  Result set mode
    6.  Auto-commit
    7.  Escape and select
    8.  Storage engine
6.  TAGGED CAPABILITY TRAITS
    ------------------------
    1.  Traits
         1.  has_mysql_charset
         2.  has_mysql_multi_result
         3.  has_mysql_options
         4.  has_mysql_diagnostics
         5.  has_mysql_result_modes
         6.  has_mysql_escape
         7.  has_mysql_change_user
         8.  has_mysql_engine
         9.  is_mysql_connection
7.  TAGLESS CAPABILITY TRAITS
    -------------------------
    1.  Individual capabilities
    2.  Compound capabilities
8.  SFINAE HELPERS
    --------------
    1.  enable_if helpers
9.  C++20 CONCEPTS
    --------------
    1.  Connection concepts
    2.  Capability concepts
    3.  Tagless capability concepts
    4.  Former names
*/

#ifndef DJINTERP_DB_MYSQL_MYSQL_COMMON_HPP
#define DJINTERP_DB_MYSQL_MYSQL_COMMON_HPP 1

// djinterp
#include "../../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>   // std::chrono::milliseconds
#include <cstddef>  // std::size_t
#include <cstring>  // std::strlen
#include <deque>    // std::deque
#include <limits>   // std::numeric_limits
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


//==============================================================================
// 1.  CLIENT FLAGS
//==============================================================================
// Portable compile-time constants for commonly used MySQL client flags.
// The actual CLIENT_* macros come from the MySQL C API header; these
// provide named constants usable without including <mysql.h>.


// 1.1    The flags
//------------------------------------------------------------------------------
// 1.1.1
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

// 1.2    Operators
//------------------------------------------------------------------------------
// 1.2.1
// operator|
//   function: bitwise OR for combining mysql_client_flag values.
D_INLINE constexpr mysql_client_flag
operator|(
    mysql_client_flag _a,
    mysql_client_flag _b
)
{
    return static_cast<mysql_client_flag>(
        static_cast<unsigned long>(_a) |
        static_cast<unsigned long>(_b));
}

// 1.2.2
// operator&
//   function: bitwise AND for testing mysql_client_flag values.
D_INLINE constexpr mysql_client_flag
operator&(
    mysql_client_flag _a,
    mysql_client_flag _b
)
{
    return static_cast<mysql_client_flag>(
        static_cast<unsigned long>(_a) &
        static_cast<unsigned long>(_b));
}

// 1.2.3
// operator|=
//   function: bitwise OR assignment for mysql_client_flag values.
D_INLINE constexpr mysql_client_flag&
operator|=(
    mysql_client_flag& _a,
    mysql_client_flag  _b
)
{
    _a = _a | _b;

    return _a;
}


//==============================================================================
// 2.  CONNECTION CONFIGURATION
//==============================================================================


// 2.1    The configuration
//------------------------------------------------------------------------------
// 2.1.1
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

    // construction -- the family's defaults, over a generic configuration's
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


//==============================================================================
// 3.  MYSQL-FAMILY CONNECTION
//==============================================================================


// 3.1    The layer
//------------------------------------------------------------------------------
// 3.1.1
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

    // holds_connection, busy_code -- a stream's run holds the connection,
    // whose every other command the layer refuses meanwhile with
    // CR_COMMANDS_OUT_OF_SYNC (2014), as the clients do
    static constexpr bool holds_connection = true;
    static constexpr int  busy_code        = 2014;

    // connection_lost -- whether a result means the connection was lost, as
    // the description above lists
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
     * @note A packet too large for the client (2020) is a statement the
     *       client refused to send, the connection kept, or a row larger than
     *       it takes, the connection closed; a ping, after the failure is
     *       described, tells which.
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

    /**
     * @brief Asks the server to stop what a connection runs, from a
     *        connection of the layer's own (d_mariadb_cancel), waiting for
     *        it: an operation's cancel().
     *
     * @note KILL QUERY, which MariaDB discards when it reaches a connection
     *       between statements: a statement sent but not yet begun is not
     *       stopped (mariadb_connection.h, 1.9).
     *
     * @param[in] _db  the connection whose statement is to stop.
     * @throws query_exception, with the layer's result and the reason it
     *         gave, when the request cannot be made.
     */
    static void cancel(handle_type _db);
};

// 3.1.2
// prepared_statement<mariadb_layer>
//   class: MariaDB's and MySQL's prepared statements, instantiated once, in
// mysql_common.cpp.
extern template class prepared_statement<mariadb_layer>;

// 3.1.3
// streaming_result_set<mariadb_layer>
//   class: its streams, instantiated with them.
extern template class streaming_result_set<mariadb_layer>;

// 3.2    Settings for a connect
//------------------------------------------------------------------------------
// 3.2.1
// internal::mariadb_settings
//   class: the layer's client settings for one connect, with the text they
// point to kept alive beside them, in a std::deque whose elements never
// move. Not copyable.
NS_INTERNAL

    class mariadb_settings
    {
    public:
        // construction -- empty
        mariadb_settings() = default;

        mariadb_settings(const mariadb_settings&)            = delete;
        mariadb_settings& operator=(const mariadb_settings&) = delete;

        // adding -- a number, a text, a connect attribute, or a native
        // option; each throws std::bad_alloc when memory runs out, and a
        // text or an attribute connection_exception with
        // D_DB_INVALID_ARGUMENT for text holding a NUL
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

// 3.3    Session tracking
//------------------------------------------------------------------------------
// 3.3.1
// mysql_session_track
//   enum: the kinds of session state a server reports changes to, as the
// protocol numbers them, and the layer's enum d_mariadb_track; a server
// reports a kind only as the session variable named here asks.
enum class mysql_session_track : re_std::uint8_t
{
    system_variables            = 0,  // each variable changed, then its
                                      // value: those
                                      // session_track_system_variables names
    schema                      = 1,  // the new current database
                                      // (session_track_schema)
    state_change                = 2,  // "1" for any change at all
                                      // (session_track_state_change)
    gtids                       = 3,  // the transaction's GTIDs
                                      // (session_track_gtids)
    transaction_characteristics = 4,  // the SQL that would begin the
                                      // transaction again
                                      // (session_track_transaction_info)
    transaction_state           = 5   // eight characters, one for each of
                                      // the transaction's states
                                      // (session_track_transaction_info)
};

// 3.4    The class
//------------------------------------------------------------------------------
// 3.4.1
// mysql_common_connection
//   class template: the connection MariaDB and MySQL share, over the MariaDB
// layer's face, which it holds and which closes the connection when it
// goes. Derived is mariadb_connection or mysql_connection, DbType its
// database_type; each adds its own settings and session set-up through two
// hooks, add_settings_impl(internal::mariadb_settings&), which may also
// refuse what the layer cannot apply, and on_connected_impl(face::mariadb&),
// run on the new connection before the object takes it. A set-up the
// client can run as an init command is better added as one: the client runs
// those within the connect, so connect_async never waits for them, and a
// reset runs them again.
//   Connecting applies connection_config and mysql_connect_config: where and
// as whom; the timeouts, in seconds rounded up, 0 leaving the client's; TLS,
// REQUIRED with enable_ssl, VERIFY_IDENTITY with verify_ssl too; the files;
// compression; LOAD DATA LOCAL; an init command, the leaf's after it;
// connect attributes; client flags beyond those the other settings stand
// for; a character set other than the layer's, which the handshake carries
// and the layer sets again where the handshake loses it; and native options
// (set_option), which come last and so win. max_packet_size is the largest
// packet the client takes from the server, 0 leaving the client's own,
// 1 GiB: connection_config's 16 MiB is below MySQL 8's 64 MiB, and a row
// past it loses the connection (connection_exception, 2013 or 2020).
// reset_connection gives the connection a fresh session, set up as connect
// set it up. The operations that do not block are the layer's
// (mariadb_connection.h 1.8): where the client cannot carry TLS so
// (Connector/C before 3.4.10 with Schannel; face::mariadb::
// client_nonblocking_tls), they throw with D_DB_UNSUPPORTED over a connection
// under TLS, and connect_async for one that would ask for it, the layer's
// PREFERRED among them; mysql_connection's ssl_mode, or Connector/C's own
// switches through set_option, make one without. Parameters are written
// `?`. Each statement reads every result it produces, so
// next_result has no result left to read. Each member the connection adds
// throws as the operations do (database_connection.hpp):
// connection_exception when the connection is not open, or a failure lost
// it, and otherwise the exception its contract names, with the client's or
// the server's error number, or a negative D_DB_*, as its error_code(). Not
// copyable; movable.
template<typename      Derived,
         database_type DbType>
class mysql_common_connection : public database_connection<Derived, DbType>
{
public:
    using base_type      = database_connection<Derived, DbType>;
    using traits_type    = typename base_type::traits_type;
    using layer_type     = mariadb_layer;
    using statement_type = prepared_statement<mariadb_layer>;

    /**
     * @brief Sets the session's character set, through the client's own
     *        call, so that escaping follows it; the next connect takes it
     *        too.
     *
     * @param[in] _charset  the character set's name, e.g. "utf8mb4".
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a name holding a
     *         NUL, and with the client's number when it refuses: 2019 for a
     *         character set it does not know.
     * @post On success both configurations name the character set.
     */
    void set_charset(const std::string& _charset);

    /**
     * @brief Reads the session's character set, as the client records it,
     *        without asking the server.
     *
     * @return the character set's name.
     * @throws query_exception with the client's number when it cannot say.
     */
    std::string get_charset() const;

    /**
     * @brief Moves to a statement's next result: none is ever left, since
     *        each statement reads every result it produces.
     *
     * @return -1, no more results.
     * @throws connection_exception when the connection is not open.
     */
    int next_result();

    // more_results -- false, for the same reason
    bool more_results() const;

    /**
     * @brief Runs several statements, separated by semicolons, with the
     *        client's multi-statement mode on for that call alone; the first
     *        that fails stops it, the ones before having run.
     *
     * @param[in] _sql  the statements.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for SQL holding a
     *         NUL, and with the client's or the server's number for the
     *         statement that failed.
     */
    void execute_script(const std::string& _sql);

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

    /**
     * @brief Makes a database the connection's current one, which the
     *        configuration then names.
     *
     * @param[in] _database  the database.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a name holding a
     *         NUL, or an empty one, and with the server's number when it
     *         refuses: 1049 for no such database.
     */
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

    /**
     * @brief Gives the connection a fresh session without signing in again
     *        (mysql_reset_connection), set up as connect set it up.
     *
     * @note The server rolls back an open transaction and ends the
     *       session's temporary tables, user variables, session variables
     *       and prepared statements, keeping the current database; the
     *       statements prepared on the connection are closed first. The
     *       layer then runs the init command again and sets the character
     *       set again where the session's can differ from the client's, and
     *       the leaf sets its session up as after a connect (MariaDB's
     *       storage engine and Galera's causal reads).
     *
     * @throws query_exception with D_DB_UNSUPPORTED where the server (MySQL
     *         before 5.7.3, MariaDB before 10.2.4) or the client (a MariaDB
     *         client before Connector/C 3.0) has no reset, nothing having
     *         changed; with the client's or the server's number when the
     *         reset, or the session's set-up after it, fails;
     *         connection_exception when the connection is not open, the
     *         reset lost it, or the leaf's set-up fails.
     * @post On success no transaction is open, auto-commit is the server's
     *       default, and every statement prepared on the connection is
     *       closed.
     */
    void reset_connection();

    /**
     * @brief Reads what the server reported changing in the session, of one
     *        kind, with its reply to the connection's last command, its own
     *        among them (has_table's query, say).
     *
     * @note Through Connector/C, a statement that returns rows, use_database
     *       and reset_connection report none (d_mariadb_session_track); a
     *       failed statement and a ping report none through either client.
     *
     * @param[in] _kind  the kind of state.
     * @return the entries, in the server's order -- for system variables a
     *         name and its value, one after the other, for each -- and none
     *         when the reply reported nothing of the kind.
     * @throws query_exception with D_DB_UNSUPPORTED where the server (MySQL
     *         before 5.7.4, MariaDB before 10.2.2) or the client (a MariaDB
     *         client before Connector/C 3.0) has no session tracking.
     */
    std::vector<std::string>
         get_session_track_info(mysql_session_track _kind) const;

    // session features -- whether the server connected and the client have
    // the reset and session tracking, asked of the layer at run time
    // (d_mariadb_supports); false while the connection is not open
    bool
    supports_reset_connection() const noexcept
    {
        return m_supports(D_MARIADB_FEATURE_RESET_CONNECTION);
    }

    bool
    supports_session_tracking() const noexcept
    {
        return m_supports(D_MARIADB_FEATURE_SESSION_TRACKING);
    }

    // result modes -- the client's names for core/db's two ways of reading
    // rows: store_result reads them all when the query runs, as
    // execute_query does (mysql_store_result); use_result reads them a page
    // at a time as the stream's cursor reaches them, holding the connection
    // until the run ends, as stream_query does (mysql_use_result)
    std::unique_ptr<buffered_result_set>
    store_result(const std::string& _query)
    {
        return this->execute_query(_query);
    }

    std::unique_ptr<streaming_result_set<mariadb_layer>>
    use_result(const std::string& _query,
               std::size_t        _page_size = stream_page_size)
    {
        return this->stream_query(_query, _page_size);
    }

    /**
     * @brief Reads the server's summary of itself, as mysql_stat words it.
     *
     * @return the summary: "Uptime: 3600  Threads: 2  Questions: 15 ...".
     * @throws query_exception with the client's number when the server does
     *         not answer.
     */
    std::string get_stat() const;

    // diagnostics -- the connection's id on the server, and the latest
    // statement's warning count and SQLSTATE, as the client keeps them; 0,
    // 0 and "" when closed
    unsigned long get_thread_id() const noexcept;
    unsigned int  get_warning_count() const noexcept;
    std::string   get_sqlstate() const;

    /**
     * @brief Reads the session's default storage engine.
     *
     * @return the engine's name, e.g. "InnoDB".
     * @throws query_exception with the server's number when it cannot say.
     */
    std::string get_engine() const;

    /**
     * @brief Sets the session's default storage engine.
     *
     * @param[in] _engine  the engine's name, passed as a quoted literal.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a name holding a
     *         NUL, and with the server's number when it refuses the engine.
     */
    void set_engine(const std::string& _engine);

    /**
     * @brief Turns auto-commit mode on or off: set_auto_commit's MySQL
     *        spelling.
     *
     * @param[in] _enabled  whether each statement commits on its own.
     * @throws as set_auto_commit.
     */
    void set_autocommit(bool _enabled);

    /**
     * @brief Adds a mysql_options option and its argument, applied as given
     *        at each connect from now on.
     *
     * @param[in] _option  the option, as mysql.h numbers it.
     * @param[in] _value   its argument, as mysql_options takes it.
     * @pre `_value` outlives every connect that applies it.
     */
    void set_option(int         _option,
                    const void* _value);

    /**
     * @brief Reads one of the client's options for the open connection back,
     *        as mysql_get_option reads it (d_mariadb_get_option).
     *
     * @param[in]  _option  the option, as mysql.h numbers it.
     * @param[out] _value   points at the option's own type, as mysql.h
     *                      documents it for mysql_options; receives its
     *                      value.
     * @throws connection_exception when the connection is not open;
     *         query_exception with D_DB_UNSUPPORTED for an option the client
     *         cannot read back, or a client without mysql_get_option
     *         (Connector/C before 3.0); D_DB_INVALID_ARGUMENT for a `NULL`
     *         value.
     */
    void get_option(int   _option,
                    void* _value) const;

    // client flags -- CLIENT_* bits the next connect asks for
    mysql_client_flag get_client_flags() const noexcept;
    void              set_client_flags(mysql_client_flag _flags) noexcept;
    void              add_client_flag(mysql_client_flag _flag) noexcept;
    bool              has_client_flag(mysql_client_flag _flag) const noexcept;

    // connect attributes -- name and value, which performance_schema shows,
    // for the next connect, which refuses a NUL in either
    void set_connect_attribute(const std::string& _key,
                               const std::string& _value);
    const std::map<std::string, std::string>&
         get_connect_attributes() const noexcept;

    // configuration -- the family's settings, the generic configuration
    // among them as it now is (connect(config) and use_database change it);
    // setting them replaces the generic configuration with theirs, for the
    // next connect
    mysql_connect_config
         get_mysql_config() const;
    void set_mysql_config(const mysql_connect_config& _config);

    /**
     * @brief Says whether the current database has a base table of a name;
     *        a view is not one, nor a temporary table.
     *
     * @param[in] _table_name  the name.
     * @return whether the table exists.
     * @throws query_exception with D_DB_INVALID_ARGUMENT for a name holding a
     *         NUL, and with the client's or the server's number when it
     *         cannot tell.
     */
    bool has_table(const std::string& _table_name) const;

    /**
     * @brief Lists the current database's base tables.
     *
     * @return the names, sorted by bytes.
     * @throws query_exception with the client's or the server's number when
     *         it cannot list them.
     */
    std::vector<std::string> list_tables() const;

    // vendor queries -- the family, and which of its servers DbType names,
    // at compile time
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

    // hooks -- database_connection calls these, programs the operations
    // they implement, whose contracts they keep; connect_start_impl to
    // connect_abandon_impl are connect_async's (database_connection.hpp,
    // internal::connect_steps), into a face of their own until connected
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
    face::mariadb_statement
                    prepare_impl(const std::string& _query);
    std::string     get_server_version_impl() const;
    re_std::int64_t get_last_insert_id_impl() const noexcept;
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
    void            set_auto_commit_impl(bool _enabled);
    bool            get_auto_commit_impl() const;
    struct ::d_mariadb*
                    native_handle_impl() const noexcept;

protected:
    // construction -- the leaves': with the family's defaults, with a
    // generic configuration, or with the family's. Nothing connects before
    // connect()
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

    /**
     * @brief Raises a failure of the MariaDB layer, as mariadb_layer::raise
     *        does; a success returns.
     *
     * @param[in] _result     the call's result.
     * @param[in] _kind       which exception a failure throws, unless it lost
     *                        the connection.
     * @param[in] _operation  the operation's name, for the message.
     * @throws as mariadb_layer::raise, unless `_result` is 0.
     */
    void m_check(int          _result,
                 failure_kind _kind,
                 const char*  _operation) const;

    // m_supports -- the layer's answer for the server connected
    // (d_mariadb_supports), by its flavor and release; false while the
    // connection is not open
    bool m_supports(d_mariadb_feature _feature) const noexcept;

    // m_supports_modern_sql -- window functions and CTEs, with LATERAL on
    // MySQL, and with sequences and system versioning on MariaDB, as env
    // defines each flavor's (D_ENV_MYSQL_HAS_MODERN_SQL,
    // D_ENV_MARIADB_HAS_MODERN_SQL), for the server connected; false while
    // the connection is not open
    bool m_supports_modern_sql() const noexcept;

    mysql_connect_config m_mysql_config;

private:
    // m_derived -- the leaf, for its two hooks
    Derived& m_derived() noexcept;

    // m_prepare_connect -- the settings and the options a connect passes,
    // the configuration's refusals thrown first; m_finish_connect -- the
    // set-up on a connection made, which the object then takes;
    // m_refuse_connect -- the connection_exception for a connect that
    // failed, in the client's words where it gave some
    struct d_mariadb_options
                m_prepare_connect(internal::mariadb_settings& _settings);
    void        m_finish_connect(face::mariadb& _opened);
    [[noreturn]] void
                m_refuse_connect(const face::text& _reason,
                                 int               _result) const;

    face::mariadb                            m_db;
    face::mariadb                            m_opening;
    std::vector<std::pair<int, const void*>> m_native_options;
};


//==============================================================================
// 4.  MYSQL-FAMILY CONNECTION DEFINITIONS
//==============================================================================
// mysql_common_connection's members, which each leaf's source instantiates
// explicitly for its leaf, and its header declares that instantiation
// extern.


// 4.1    Helpers
//------------------------------------------------------------------------------
NS_INTERNAL

    // 4.1.1
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

    // 4.1.2
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

    // 4.1.3
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

// 4.2    Members
//------------------------------------------------------------------------------
// construction
template<typename      Derived,
         database_type DbType>
mysql_common_connection<Derived, DbType>::mysql_common_connection()
    : base_type(),
      m_mysql_config(),
      m_db(),
      m_opening(),
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
      m_opening(),
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
      m_opening(),
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

/*
mysql_common_connection::reset_connection
  The reset is checked for first, so that one the server or the client
lacks changes nothing; the statements prepared on the connection are closed
next, while the server still has them, and the layer resets the session and
sets it up as its connect did -- the init command, the character set --
after which the leaf's set-up runs, as after a connect.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::reset_connection()
{
    this->run_guarded(
        [this]() -> void
        {
            // a server or a client without the reset
            if (!m_db.supports(D_MARIADB_FEATURE_RESET_CONNECTION))
            {
                m_check(D_DB_UNSUPPORTED,
                        failure_kind::query,
                        "reset_connection");
            }

            this->close_statements();
            m_check(m_db.reset_connection(),
                    failure_kind::query,
                    "reset_connection");
            m_derived().on_connected_impl(m_db);

            return;
        });

    this->m_in_transaction = false;
    this->m_auto_commit    = true;

    return;
}

/*
mysql_common_connection::get_session_track_info
  Reading the client's record changes nothing a caller can see, and the
face's call is not const, so the face is reached through const_cast, as
ping_impl reaches it. The kinds number as the layer's.
*/
template<typename      Derived,
         database_type DbType>
std::vector<std::string>
mysql_common_connection<Derived, DbType>::get_session_track_info(
    mysql_session_track _kind
) const
{
    return this->ask_guarded(
        [this, _kind]() -> std::vector<std::string>
        {
            std::vector<std::string> entries;

            m_check(const_cast<face::mariadb&>(m_db).session_track(
                        static_cast<d_mariadb_track>(_kind),
                        entries),
                    failure_kind::query,
                    "get_session_track_info");

            return entries;
        });
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
void
mysql_common_connection<Derived, DbType>::get_option(
    int   _option,
    void* _value
) const
{
    this->ask_guarded(
        [this, _option, _value]() -> void
        {
            m_check(m_db.get_option(_option, _value),
                    failure_kind::query,
                    "get_option");

            return;
        });

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

/*
mysql_common_connection::get_mysql_config
  The generic configuration is the connection's own, which connect(config)
and use_database change; the copy in the family's settings is only what was
set, so it is replaced on the way out.
*/
template<typename      Derived,
         database_type DbType>
mysql_connect_config
mysql_common_connection<Derived, DbType>::get_mysql_config() const
{
    mysql_connect_config config = m_mysql_config;

    config.base = this->m_config;

    return config;
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
mysql_common_connection::connect_impl
  The leaf's settings go first, so that a TLS mode of its own stands in for
enable_ssl's (m_prepare_connect); the connection opens into a face of its
own -- the character set and the init commands applied within the connect
-- the leaf's on_connected_impl runs there, and only then does the object
take it (m_finish_connect), so that a failure at any step leaves the old
state and the new connection closed. A failed
connect is described by the client's own words, which the layer copies
before it closes what failed.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::connect_impl()
{
    internal::mariadb_settings     settings;
    const struct d_mariadb_options options = m_prepare_connect(settings);
    face::mariadb                  opened;
    face::text                     reason;
    const int                      result = opened.connect_with(
                                                &options,
                                                settings.data(),
                                                settings.size(),
                                                reason);

    // a failure, in the client's words where it gave some
    if (result != 0)
    {
        m_refuse_connect(reason, result);
    }

    m_finish_connect(opened);

    return;
}

/*
mysql_common_connection::connect_start_impl
  As connect_impl begins, into the face kept for a connection being made; the
settings are the client's by the time the start returns.
*/
template<typename      Derived,
         database_type DbType>
int
mysql_common_connection<Derived, DbType>::connect_start_impl(
    d_db_pending& _pending
)
{
    internal::mariadb_settings     settings;
    const struct d_mariadb_options options = m_prepare_connect(settings);
    face::text                     reason;
    const int                      result = m_opening.connect_with_start(
                                                &options,
                                                settings.data(),
                                                settings.size(),
                                                _pending,
                                                reason);

    // under way, made, or failed in the client's words
    if ( (result != 0) &&
         (result != D_DB_WAIT) )
    {
        m_refuse_connect(reason, result);
    }

    return result;
}

template<typename      Derived,
         database_type DbType>
int
mysql_common_connection<Derived, DbType>::connect_continue_impl(
    unsigned      _events,
    d_db_pending& _pending
)
{
    face::text reason;
    const int  result = m_opening.connect_continue(_events,
                                                   _pending,
                                                   reason);

    // under way, made, or failed in the client's words
    if ( (result != 0) &&
         (result != D_DB_WAIT) )
    {
        m_refuse_connect(reason, result);
    }

    return result;
}

/*
mysql_common_connection::connect_finish_impl
  As connect_impl ends; a setup that fails closes the connection made.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::connect_finish_impl()
{
    try
    {
        m_finish_connect(m_opening);
    }
    catch (...)
    {
        (void)m_opening.close();

        throw;
    }

    (void)m_opening.close();

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::connect_abandon_impl() noexcept
{
    (void)m_opening.close();

    return;
}

/*
mysql_common_connection::m_prepare_connect
  The family's refusals, then every setting a connect passes on, the leaf's
first; the options point into the configuration.
*/
template<typename      Derived,
         database_type DbType>
struct d_mariadb_options
mysql_common_connection<Derived, DbType>::m_prepare_connect(
    internal::mariadb_settings& _settings
)
{
    const connection_config& config  = this->m_config;
    const unsigned long      handled =
        static_cast<unsigned long>(mysql_client_flag::compress        |
                                   mysql_client_flag::ssl             |
                                   mysql_client_flag::connect_with_db |
                                   mysql_client_flag::local_files     |
                                   mysql_client_flag::multi_results);
    const unsigned long      flags   =
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

    // the configuration's init command, ahead of the leaf's, which the
    // configuration's own fields make and which so win
    if (!m_mysql_config.init_command.empty())
    {
        _settings.add_text(D_MARIADB_SETTING_INIT_COMMAND,
                          m_mysql_config.init_command);
    }

    m_derived().add_settings_impl(_settings);

    // the timeouts the configuration gives
    if (config.connect_timeout.count() > 0)
    {
        _settings.add_number(D_MARIADB_SETTING_CONNECT_TIMEOUT,
                            internal::mysql_seconds(config.connect_timeout));
    }

    if (config.read_timeout.count() > 0)
    {
        _settings.add_number(D_MARIADB_SETTING_READ_TIMEOUT,
                            internal::mysql_seconds(config.read_timeout));
    }

    if (config.write_timeout.count() > 0)
    {
        _settings.add_number(D_MARIADB_SETTING_WRITE_TIMEOUT,
                            internal::mysql_seconds(config.write_timeout));
    }

    // the largest packet the client takes, as many bytes as the client's
    // unsigned long holds (max parenthesized, against <windows.h>'s macro);
    // 0 leaves the client's own
    if (config.max_packet_size > 0u)
    {
        if (config.max_packet_size >
            static_cast<std::size_t>(
                (std::numeric_limits<unsigned long>::max)()))
        {
            throw connection_exception(std::string(traits_type::display_name) +
                                       " connect: max_packet_size is past "
                                       "what the client takes",
                                       D_DB_INVALID_ARGUMENT);
        }

        _settings.add_number(D_MARIADB_SETTING_MAX_PACKET,
                            static_cast<unsigned long>(config.max_packet_size));
    }

    // TLS, where the leaf named no mode of its own, and the files
    if ( (config.enable_ssl) &&
         (!_settings.has(D_MARIADB_SETTING_SSL_MODE)) )
    {
        _settings.add_number(
            D_MARIADB_SETTING_SSL_MODE,
            static_cast<unsigned long>((config.verify_ssl)
                                           ? D_MARIADB_SSL_VERIFY_IDENTITY
                                           : D_MARIADB_SSL_REQUIRED));
    }

    if (config.ssl_key)
    {
        _settings.add_text(D_MARIADB_SETTING_SSL_KEY, *config.ssl_key);
    }

    if (config.ssl_cert)
    {
        _settings.add_text(D_MARIADB_SETTING_SSL_CERT, *config.ssl_cert);
    }

    if (config.ssl_ca)
    {
        _settings.add_text(D_MARIADB_SETTING_SSL_CA, *config.ssl_ca);
    }

    // the family's own settings
    if (m_mysql_config.use_compression)
    {
        _settings.add_number(D_MARIADB_SETTING_COMPRESS, 1ul);
    }

    if (m_mysql_config.use_local_infile)
    {
        _settings.add_number(D_MARIADB_SETTING_LOCAL_INFILE, 1ul);
    }

    for (const std::pair<const std::string, std::string>& attribute :
         m_mysql_config.connect_attributes)
    {
        _settings.add_attribute(attribute.first, attribute.second);
    }

    if (flags != 0ul)
    {
        _settings.add_number(D_MARIADB_SETTING_CLIENT_FLAGS, flags);
    }

    // a character set other than the one the layer asks for: the
    // handshake's, so that nothing follows the connect
    const std::string& charset = (config.charset)
                                     ? *config.charset
                                     : m_mysql_config.default_charset;

    if ( (!charset.empty()) &&
         (charset != D_CFG_MARIADB_CHARSET) )
    {
        _settings.add_text(D_MARIADB_SETTING_CHARSET, charset);
    }

    for (const std::pair<int, const void*>& option : m_native_options)
    {
        _settings.add_native(option.first, option.second);
    }

    return d_mariadb_options
    {
        internal::mysql_or_null(config.host),
        internal::mysql_or_null(config.username),
        internal::mysql_or_null(config.password),
        internal::mysql_or_null(config.database),
        static_cast<unsigned>(config.port),
        internal::mysql_or_null(m_mysql_config.unix_socket)
    };
}

/*
mysql_common_connection::m_finish_connect
  The leaf's set-up on the new connection, the character set and the init
commands already applied within the connect (m_prepare_connect); then the
object takes it.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::m_finish_connect(
    face::mariadb& _opened
)
{
    m_derived().on_connected_impl(_opened);
    m_db.swap(_opened);

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::m_refuse_connect(
    const face::text& _reason,
    int               _result
) const
{
    throw connection_exception(
        std::string(traits_type::display_name) + " connect: " +
            ((_reason.is_null()) ? d_mariadb_result_message(_result)
                                 : _reason.c_str()) +
            " (" + std::to_string(_result) + ")",
        _result);
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::disconnect_impl() noexcept
{
    (void)m_db.close();

    return;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::is_connected_impl()
    const noexcept
{
    return m_db.is_open();
}

/*
mysql_common_connection::ping_impl
  A ping is a round trip that changes nothing a caller can see, and the
face's is not const, so the face is reached through const_cast.
*/
template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::ping_impl() const noexcept
{
    return (const_cast<face::mariadb&>(m_db).ping() == 0);
}

template<typename      Derived,
         database_type DbType>
face::rows
mysql_common_connection<Derived, DbType>::execute_query_impl(
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
mysql_common_connection<Derived, DbType>::execute_update_impl(
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
mysql_common_connection<Derived, DbType>::prepare_impl(
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
mysql_common_connection<Derived, DbType>::get_server_version_impl() const
{
    const char* const version = m_db.server_version();

    return (version) ? version
                     : "";
}

template<typename      Derived,
         database_type DbType>
re_std::int64_t
mysql_common_connection<Derived, DbType>::get_last_insert_id_impl()
    const noexcept
{
    return static_cast<re_std::int64_t>(m_db.last_insert_id());
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::begin_transaction_impl()
{
    m_check(m_db.begin_transaction(face::mariadb::default_transaction_mode()),
            failure_kind::transaction,
            "begin_transaction");

    return;
}

/*
mysql_common_connection::begin_transaction_impl (isolation_level)
  SET TRANSACTION ISOLATION LEVEL without SESSION sets the next transaction's
level alone, so the session's own level is untouched; the default level sets
nothing.
*/
template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::begin_transaction_impl(
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

    begin_transaction_impl();

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::commit_impl()
{
    m_check(m_db.commit(), failure_kind::transaction, "commit");

    return;
}

template<typename      Derived,
         database_type DbType>
void
mysql_common_connection<Derived, DbType>::rollback_impl()
{
    m_check(m_db.rollback(), failure_kind::transaction, "rollback");

    return;
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::in_transaction_impl()
    const noexcept
{
    return m_db.in_transaction();
}

template<typename      Derived,
         database_type DbType>
isolation_level
mysql_common_connection<Derived, DbType>::get_isolation_level_impl() const
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
mysql_common_connection<Derived, DbType>::set_isolation_level_impl(
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
mysql_common_connection<Derived, DbType>::create_savepoint_impl(
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
mysql_common_connection<Derived, DbType>::release_savepoint_impl(
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
mysql_common_connection<Derived, DbType>::rollback_to_savepoint_impl(
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
mysql_common_connection<Derived, DbType>::set_auto_commit_impl(
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
mysql_common_connection<Derived, DbType>::get_auto_commit_impl() const
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
mysql_common_connection<Derived, DbType>::native_handle_impl()
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
bool
mysql_common_connection<Derived, DbType>::m_supports(
    d_mariadb_feature _feature
) const noexcept
{
    return ( (this->is_connected()) &&
             (m_db.supports(_feature)) );
}

template<typename      Derived,
         database_type DbType>
bool
mysql_common_connection<Derived, DbType>::m_supports_modern_sql()
    const noexcept
{
    // each flavor's set, beside what both have
    const bool own =
        (m_db.server_flavor() == D_MARIADB_FLAVOR_MARIADB)
            ? ( (m_supports(D_MARIADB_FEATURE_SEQUENCES)) &&
                (m_supports(D_MARIADB_FEATURE_SYSTEM_VERSIONING)) )
            : m_supports(D_MARIADB_FEATURE_LATERAL_DERIVED);

    return ( (own) &&
             (m_supports(D_MARIADB_FEATURE_WINDOW_FUNCTIONS)) &&
             (m_supports(D_MARIADB_FEATURE_CTE)) );
}

template<typename      Derived,
         database_type DbType>
Derived&
mysql_common_connection<Derived, DbType>::m_derived() noexcept
{
    return static_cast<Derived&>(*this);
}



//==============================================================================
// 5.  EXPRESSION DETECTORS
//==============================================================================
// Sections 5 to 9, once mysql_common_traits.hpp and
// mysql_common_concepts.hpp, detect which of the family's members a type
// has, as database.hpp's section 5 does for every connection: as traits
// from C++17 and as concepts under C++20. A detector is the type of one
// member's call, which is_detected tests; some name calls
// mysql_common_connection does not offer, so the traits built on them are
// false for it.


// 5.1    Character set management
//------------------------------------------------------------------------------
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

// 5.2    Multi-result set iteration
//------------------------------------------------------------------------------
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

// 5.3    Options API
//------------------------------------------------------------------------------
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

// 5.4    Server diagnostics
//------------------------------------------------------------------------------
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

// 5.5    Result set mode
//------------------------------------------------------------------------------
// mysql_store_result_t
//   detector: store_result(const std::string&) method: rows read when the
// query runs (mysql_store_result).
template<typename Type>
using mysql_store_result_t = decltype(std::declval<Type&>().store_result(
    std::declval<const std::string&>()));

// mysql_use_result_t
//   detector: use_result(const std::string&) method: rows read a page at a
// time (mysql_use_result).
template<typename Type>
using mysql_use_result_t = decltype(std::declval<Type&>().use_result(
    std::declval<const std::string&>()));

// 5.6    Auto-commit
//------------------------------------------------------------------------------
// mysql_set_autocommit_t
//   detector: set_autocommit(bool) method.
// wraps mysql_autocommit().
template<typename Type>
using mysql_set_autocommit_t = decltype(std::declval<Type&>().set_autocommit(
    std::declval<bool>()));

// 5.7    Escape and select
//------------------------------------------------------------------------------
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

// 5.8    Storage engine
//------------------------------------------------------------------------------
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


//==============================================================================
// 6.  TAGGED CAPABILITY TRAITS
//==============================================================================
// Each trait is a type with ::value, true where every detector it names finds
// its member; name_v is the value, where the language has variable templates.


// 6.1    Traits
//------------------------------------------------------------------------------
// 6.1.1
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
    constexpr bool has_mysql_charset_v =
        has_mysql_charset<clean_t<Type>>::value;
#endif

// 6.1.2
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

// 6.1.3
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
    constexpr bool has_mysql_options_v =
        has_mysql_options<clean_t<Type>>::value;
#endif

// 6.1.4
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

// 6.1.5
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

// 6.1.6
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

// 6.1.7
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

// 6.1.8
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

// 6.1.9
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


//==============================================================================
// 7.  TAGLESS CAPABILITY TRAITS
//==============================================================================
// The same capabilities as constexpr bool variable templates, for a condition
// that needs no type: one member each, then the sets.


// 7.1    Individual capabilities
//------------------------------------------------------------------------------
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
//   tagless trait: true if Type has store_result(const std::string&).
template<typename Type,
         typename = void>
constexpr bool mysql_can_store_result = false;

template<typename Type>
constexpr bool mysql_can_store_result<Type,
    std::void_t<mysql_store_result_t<Type>>> = true;

// mysql_can_use_result
//   tagless trait: true if Type has use_result(const std::string&): rows a
// page at a time.
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

// 7.2    Compound capabilities
//------------------------------------------------------------------------------
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


//==============================================================================
// 8.  SFINAE HELPERS
//==============================================================================


// 8.1    enable_if helpers
//------------------------------------------------------------------------------
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


//==============================================================================
// 9.  C++20 CONCEPTS
//==============================================================================
// Each concept forwards to a trait or a tagless capability above. They live
// in djinterp::concepts (2026.10.21), where a concept may take the name of
// the class it constrains; each former name in djinterp forwards to its
// concept for one release. They exist where the compiler has concepts; the
// traits serve at C++17 without them, as in functor.hpp and monoid.hpp.


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace concepts
{

// 9.1    Connection concepts
//------------------------------------------------------------------------------
// mysql_family_connection
//   concept: constrains types implementing the MySQL-family connection
// interface.
template<typename Type>
concept mysql_family_connection =
    is_mysql_connection<clean_t<Type>>::value;

// non_mysql_connection
//   concept: constrains types that do not implement the MySQL-family
// connection interface.
template<typename Type>
concept non_mysql_connection =
    !mysql_family_connection<Type>;

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

// 9.2    Capability concepts
//------------------------------------------------------------------------------
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
//   concept: constrains types exposing store_result(const std::string&).
template<typename Type>
concept mysql_store_result_connection =
    mysql_can_store_result<clean_t<Type>>;

// mysql_use_result_connection
//   concept: constrains types exposing use_result(const std::string&).
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

// 9.3    Tagless capability concepts
//------------------------------------------------------------------------------
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

}  // namespace concepts

// 9.4    Former names
//------------------------------------------------------------------------------
// Mysql_connection
//   concept: concepts::mysql_family_connection, by its former name, for one
// release.
template<typename Type>
concept Mysql_connection =
    concepts::mysql_family_connection<Type>;

// non_mysql_connection
//   concept: concepts::non_mysql_connection, by its former name, for one
// release.
template<typename Type>
concept non_mysql_connection =
    concepts::non_mysql_connection<Type>;

// mysql_charset_connection
//   concept: concepts::mysql_charset_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_charset_connection =
    concepts::mysql_charset_connection<Type>;

// mysql_multi_result_connection
//   concept: concepts::mysql_multi_result_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_multi_result_connection =
    concepts::mysql_multi_result_connection<Type>;

// mysql_diagnostics_connection
//   concept: concepts::mysql_diagnostics_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_diagnostics_connection =
    concepts::mysql_diagnostics_connection<Type>;

// mysql_result_modes_connection
//   concept: concepts::mysql_result_modes_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_result_modes_connection =
    concepts::mysql_result_modes_connection<Type>;

// mysql_escape_connection
//   concept: concepts::mysql_escape_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_escape_connection =
    concepts::mysql_escape_connection<Type>;

// mysql_options_connection
//   concept: concepts::mysql_options_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_options_connection =
    concepts::mysql_options_connection<Type>;

// mysql_engine_connection
//   concept: concepts::mysql_engine_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_engine_connection =
    concepts::mysql_engine_connection<Type>;

// mysql_use_database_connection
//   concept: concepts::mysql_use_database_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_use_database_connection =
    concepts::mysql_use_database_connection<Type>;

// mysql_autocommit_connection
//   concept: concepts::mysql_autocommit_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_autocommit_connection =
    concepts::mysql_autocommit_connection<Type>;

// mysql_set_option_connection
//   concept: concepts::mysql_set_option_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_set_option_connection =
    concepts::mysql_set_option_connection<Type>;

// mysql_get_option_connection
//   concept: concepts::mysql_get_option_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_get_option_connection =
    concepts::mysql_get_option_connection<Type>;

// mysql_stat_connection
//   concept: concepts::mysql_stat_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_stat_connection =
    concepts::mysql_stat_connection<Type>;

// mysql_thread_id_connection
//   concept: concepts::mysql_thread_id_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_thread_id_connection =
    concepts::mysql_thread_id_connection<Type>;

// mysql_warning_count_connection
//   concept: concepts::mysql_warning_count_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_warning_count_connection =
    concepts::mysql_warning_count_connection<Type>;

// mysql_sqlstate_connection
//   concept: concepts::mysql_sqlstate_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_sqlstate_connection =
    concepts::mysql_sqlstate_connection<Type>;

// mysql_store_result_connection
//   concept: concepts::mysql_store_result_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_store_result_connection =
    concepts::mysql_store_result_connection<Type>;

// mysql_use_result_connection
//   concept: concepts::mysql_use_result_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_use_result_connection =
    concepts::mysql_use_result_connection<Type>;

// mysql_change_user_connection
//   concept: concepts::mysql_change_user_connection, by its former name, for
// one release.
template<typename Type>
concept mysql_change_user_connection =
    concepts::mysql_change_user_connection<Type>;

// mysql_set_engine_connection
//   concept: concepts::mysql_set_engine_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_set_engine_connection =
    concepts::mysql_set_engine_connection<Type>;

// mysql_get_engine_connection
//   concept: concepts::mysql_get_engine_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_get_engine_connection =
    concepts::mysql_get_engine_connection<Type>;

// mysql_charset_manageable
//   concept: concepts::mysql_charset_manageable, by its former name, for one
// release.
template<typename Type>
concept mysql_charset_manageable =
    concepts::mysql_charset_manageable<Type>;

// mysql_multi_result_iterable
//   concept: concepts::mysql_multi_result_iterable, by its former name, for one
// release.
template<typename Type>
concept mysql_multi_result_iterable =
    concepts::mysql_multi_result_iterable<Type>;

// mysql_result_mode_selectable
//   concept: concepts::mysql_result_mode_selectable, by its former name, for
// one release.
template<typename Type>
concept mysql_result_mode_selectable =
    concepts::mysql_result_mode_selectable<Type>;

// mysql_diagnostic_connection_tagless
//   concept: concepts::mysql_diagnostic_connection_tagless, by its former name,
// for one release.
template<typename Type>
concept mysql_diagnostic_connection_tagless =
    concepts::mysql_diagnostic_connection_tagless<Type>;

// mysql_charset_settable
//   concept: concepts::mysql_charset_settable, by its former name, for one
// release.
template<typename Type>
concept mysql_charset_settable =
    concepts::mysql_charset_settable<Type>;

// mysql_charset_gettable
//   concept: concepts::mysql_charset_gettable, by its former name, for one
// release.
template<typename Type>
concept mysql_charset_gettable =
    concepts::mysql_charset_gettable<Type>;

// mysql_result_iterable
//   concept: concepts::mysql_result_iterable, by its former name, for one
// release.
template<typename Type>
concept mysql_result_iterable =
    concepts::mysql_result_iterable<Type>;

// mysql_more_results_query
//   concept: concepts::mysql_more_results_query, by its former name, for one
// release.
template<typename Type>
concept mysql_more_results_query =
    concepts::mysql_more_results_query<Type>;

// mysql_string_escapable
//   concept: concepts::mysql_string_escapable, by its former name, for one
// release.
template<typename Type>
concept mysql_string_escapable =
    concepts::mysql_string_escapable<Type>;

// mysql_database_selectable
//   concept: concepts::mysql_database_selectable, by its former name, for one
// release.
template<typename Type>
concept mysql_database_selectable =
    concepts::mysql_database_selectable<Type>;

// mysql_user_switchable
//   concept: concepts::mysql_user_switchable, by its former name, for one
// release.
template<typename Type>
concept mysql_user_switchable =
    concepts::mysql_user_switchable<Type>;

// mysql_full_connection
//   concept: concepts::mysql_full_connection, by its former name, for one
// release.
template<typename Type>
concept mysql_full_connection =
    concepts::mysql_full_connection<Type>;


#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_MYSQL_MYSQL_COMMON_HPP
