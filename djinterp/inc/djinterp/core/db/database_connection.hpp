/*******************************************************************************
* djinterp [core]                                        database_connection.hpp
*
* The connection every djinterp C++17 database connection derives from, its
* operations written once for every vendor.
*   database_connection implements connection's operations over hooks of the
* vendor's class -- connect_impl, execute_update_impl, and so on, each
* named after its operation -- and around each call keeps what every vendor
* shares: the connection's state, the error each failure threw, the rows the
* latest statement changed, and the session its prepared statements belong
* to, which closes them before the connection closes.
*   A failure throws, with the layer's result (database.hpp, section 2); a
* connection_exception -- a connection that could not be made, or that the
* database dropped -- leaves the connection in connection_state::error, which
* reconnect() leaves. A connection never reconnects on its own.
*   connect_async and the SQL members ending in _async are the operations
* that do not block (database_async.hpp), over the layer's calls that do
* not wait where the vendor's face has them -- MariaDB's, MySQL's and
* PostgreSQL's -- and done as they start where it does not, as SQLite's.
* While one is under way it holds the connection, as a stream's run does.
* has_async_operations, and from C++20 concepts::async_connection, say
* whether a class has connect_async and the three SQL members (section 2):
* every vendor's has.
*   A vendor's class derives from database_connection<Derived, X>, Derived
* being itself, and provides the hooks the class's description lists, public
* so that the gated operations can tell which it has; programs call the
* operations, never the hooks. The class that holds the face closes the
* session in its own destructor (close_session_noexcept), since by the time
* database_connection's destructor runs the face is gone; database_connection
* itself never calls a hook from its destructor.
*     vendor class (sqlite_connection, ...)
*       database_connection<vendor class, X>    this header
*         connection_template<vendor class, X>   the vendor's traits
*           connection<vendor class>             the interface
*
*
* path:      /inc/djinterp/core/db/database_connection.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.25
*                                                            revised: 2026.10.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DATABASE CONNECTION
    -------------------
    1.  The class
         1.  database_connection
    2.  Definitions
    3.  Operations that do not block
         1.  internal::sent_steps
         2.  internal::connect_steps
    4.  Their definitions
2.  CAPABILITIES
    ------------
    1.  Detectors
         1.  connect_async_t
         2.  execute_async_t
         3.  execute_query_async_t
         4.  execute_update_async_t
    2.  Traits
         1.  has_async_operations
    3.  C++20 concepts
         1.  concepts::async_connection
*/

#ifndef DJINTERP_DB_DATABASE_CONNECTION_HPP
#define DJINTERP_DB_DATABASE_CONNECTION_HPP 1

// djinterp
#include "../../env/env.h"  // D_ENV_LANG_IS_CPP17_OR_HIGHER: the floor

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>      // std::size_t
#include <memory>       // std::shared_ptr, std::make_shared, std::unique_ptr
#include <optional>     // std::optional
#include <string>       // std::string, std::to_string
#include <type_traits>  // std::is_void_v
#include <utility>      // std::move, std::pair
// djinterp
#include "../../djinterp.hpp"                  // framework root
#include "../meta/type_traits.hpp"             // is_detected
#include "./database.hpp"                      // the vocabulary, connection
#include "./database_connection_template.hpp"  // connection_template
#include "./database_async.hpp"                // async_operation
#include "./database_statement.hpp"            // internal::session,
                                               // buffered_result_set,
                                               // prepared_statement
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP

NS_INTERNAL

    // the operations that do not block, section 3: friends of the
    // connection, whose state they keep as its blocking operations do
    template<typename      Derived,
             database_type DbType,
             typename      Result>
    class sent_steps;

    template<typename      Derived,
             database_type DbType>
    class connect_steps;

NS_END  // internal


//==============================================================================
// 1.  DATABASE CONNECTION
//==============================================================================


// 1.1    The class
//------------------------------------------------------------------------------
// 1.1.1
// database_connection
//   class template: the connection, for the vendor DbType, over the hooks of
// Derived, which derives from it. Derived provides:
//     layer_type                the layer's description, as
//                               database_statement.hpp says
//     connect_impl()            opens the connection the configuration
//                               names, or throws
//     disconnect_impl()         closes it; noexcept
//     is_connected_impl()       whether it is open; const, noexcept
//     ping_impl()               whether the database answers; const
//     execute_query_impl(sql)   runs one statement: its face::rows
//     execute_update_impl(sql)  runs SQL: the rows it changed
//     prepare_impl(sql)         prepares one statement: a statement_face
//     get_server_version_impl() the server's or library's version; const
//     get_last_insert_id_impl() the latest generated id; const
//     begin_transaction_impl(), commit_impl(), rollback_impl()
//     in_transaction_impl()     whether a transaction is open; const,
//                               noexcept
//     native_handle_impl()      the handle (connection_template)
// and, for the gated operations, which exist where their hooks do:
//     begin_transaction_impl(isolation_level), get_isolation_level_impl(),
//     set_isolation_level_impl(isolation_level), create_savepoint_impl(
//     name), release_savepoint_impl(name), rollback_to_savepoint_impl(
//     name), set_auto_commit_impl(bool), get_auto_commit_impl(); and
//     get_face(), the layer's face, const and not, through which run_layer
//     and ask_layer call the layer, raising through layer_type's raise.
// A hook reports a failure by throwing, through its layer's raise. Not
// copyable; movable, the connection moved from disconnected.
template<typename      Derived,
         database_type DbType>
class database_connection : public connection_template<Derived, DbType>
{
public:
    using base_type          = connection_template<Derived, DbType>;
    using traits_type        = typename base_type::traits_type;
    using native_handle_type = typename base_type::native_handle_type;

    /**
     * @brief Opens the connection the configuration names, unless it is
     *        open already.
     *
     * @throws connection_exception with D_DB_UNSUPPORTED when the
     *         configuration asks for auto_reconnect, which core/db never
     *         does; connection_exception, with the layer's result, when the
     *         connection cannot be made.
     * @post On success the connection is connected, outside any transaction,
     *       in auto-commit mode, and the reconnect counter is 0; on failure it
     *       is in connection_state::error.
     */
    void connect();

    /**
     * @brief Replaces the configuration, the vendor's port standing in for a
     *        port of 0, and opens the connection it names.
     *
     * @param[in] _config  the configuration.
     * @throws as connect().
     */
    void connect(const connection_config& _config);

    /**
     * @brief Closes the connection: its prepared statements first, then the
     *        connection itself.
     *
     * @post The connection is disconnected; every statement prepared on it
     *       is closed.
     */
    void disconnect() noexcept;

    /**
     * @brief Closes the connection, if it is open or in error, and opens it
     *        again.
     *
     * @throws connection_exception when as many reconnections in a row as
     *         get_max_reconnect_attempts() allows have failed already; and as
     *         connect().
     * @post As connect(); every statement prepared on the old connection is
     *       closed.
     */
    void reconnect();

    // liveness -- whether the connection is open, as the vendor sees it;
    // whether the database answers, false for any failure; and the state,
    // `error` once a statement found the connection lost
    bool             is_connected() const noexcept;
    bool             ping() const noexcept;
    connection_state get_state() const noexcept;

    /**
     * @brief Runs one statement, and returns its rows.
     *
     * @param[in] _query  the statement.
     * @return the rows, all of them, held apart from the connection.
     * @throws connection_exception when the connection is not open, or the
     *         database drops it; query_exception, with the layer's result,
     *         when the statement fails.
     */
    std::unique_ptr<buffered_result_set> execute_query(
        const std::string& _query);

    /**
     * @brief Runs SQL, and counts the rows it changed.
     *
     * @note SQLite and PostgreSQL run several statements separated by
     *       semicolons, and count the last; MariaDB and MySQL run one.
     *
     * @param[in] _query  the SQL.
     * @return the rows it inserted, updated or deleted, or returned for a
     *         statement that returns rows, as the layer counts them;
     *         get_affected_rows() answers the same until another statement
     *         runs.
     * @throws as execute_query.
     */
    re_std::int64_t execute_update(const std::string& _query);

    /**
     * @brief Runs SQL, as execute_update does.
     *
     * @param[in] _query  the SQL.
     * @return true; a failure throws, as execute_update's does, and names
     *         execute.
     */
    bool execute(const std::string& _query);

    /**
     * @brief Prepares one statement.
     *
     * @param[in] _query  the statement, its parameters written as the
     *                    database writes them (prepared_statement).
     * @return the statement, which belongs to this connection's session:
     *         closing the connection closes it.
     * @throws as execute_query.
     */
    auto prepare(const std::string& _query);

    /**
     * @brief Runs a query whose rows are read a page at a time, as the
     *        stream's cursor reaches them, over a statement of its own; gated
     *        (a vendor with a layer).
     *
     * @note On MariaDB, MySQL and PostgreSQL the run holds the connection
     *       until it ends: every other operation throws query_exception
     *       meanwhile (streaming_result_set).
     *
     * @param[in] _query      one statement.
     * @param[in] _page_size  the most rows a page holds; 1 or more.
     * @return the stream, which holds its statement; closing the connection
     *         closes it.
     * @throws as prepare(); and as prepared_statement's execute_stream.
     */
    template<typename Type = Derived,
             typename      = internal::face_hook_t<Type>>
    auto stream_query(const std::string& _query,
                      std::size_t        _page_size = stream_page_size);

    /**
     * @brief Runs a query, and reads the first column of its first row.
     *
     * @param[in] _query  the query.
     * @return the value, NULL among them; std::nullopt when there is no row.
     * @throws as execute_query, the failure naming execute_scalar.
     */
    std::optional<value> execute_scalar(const std::string& _query);

    /**
     * @brief Runs a query, and reads its first row by column name.
     *
     * @param[in] _query  the query.
     * @return the row; std::nullopt when there is none.
     * @throws as execute_query, the failure naming execute_single_row.
     */
    std::optional<row> execute_single_row(const std::string& _query);

    /**
     * @brief Opens the connection, as connect() does, without waiting on
     *        the network: an operation to drive (database_async.hpp);
     *        gated (a vendor with a layer).
     *
     * @note MariaDB, MySQL and PostgreSQL make the handshake without
     *       waiting, but resolve the host's name before it, waiting for
     *       that; the setup a configuration asks for beyond the handshake
     *       -- on MariaDB and MySQL a character set other than utf8mb4,
     *       Galera's causal reads, a default storage engine; on PostgreSQL
     *       a search path, a statement timeout -- is made once connected,
     *       waiting, a round trip each. SQLite opens as it starts. An open
     *       connection stays as it is, the operation done as it starts.
     *
     * @return the operation; its result() throws as connect() does, the
     *         connection then in connection_state::error. Destroyed while
     *         under way, it abandons the connection being made.
     */
    template<typename Type = Derived,
             typename      = internal::face_hook_t<Type>>
    async_operation<void> connect_async();

    /**
     * @brief Runs one statement, and returns its rows, as execute_query
     *        does, without waiting: an operation to drive; gated.
     *
     * @param[in] _query  the statement, copied.
     * @return the operation; its result() is execute_query's, or throws
     *         what execute_query throws. Every result the SQL produced is
     *         read, the rows of the first kept.
     */
    template<typename Type = Derived,
             typename      = internal::face_hook_t<Type>>
    async_operation<std::unique_ptr<buffered_result_set>>
        execute_query_async(const std::string& _query);

    /**
     * @brief Runs SQL, and counts the rows it changed, as execute_update
     *        does, without waiting: an operation to drive; gated.
     *
     * @param[in] _query  the SQL, copied.
     * @return the operation; its result() is execute_update's -- the count
     *         of the last result -- or throws what execute_update throws.
     */
    template<typename Type = Derived,
             typename      = internal::face_hook_t<Type>>
    async_operation<re_std::int64_t>
        execute_update_async(const std::string& _query);

    /**
     * @brief Runs SQL, as execute does, without waiting: an operation to
     *        drive; gated.
     *
     * @param[in] _query  the SQL, copied.
     * @return the operation; its result() is true, or throws what execute
     *         throws.
     */
    template<typename Type = Derived,
             typename      = internal::face_hook_t<Type>>
    async_operation<bool> execute_async(const std::string& _query);

    /**
     * @brief Calls the layer, through the vendor's face, as one of this
     *        connection's operations: once the connection is open, its state
     *        `executing` meanwhile, and a failure raised and recorded as the
     *        vendor's own operations raise and record theirs; gated (a
     *        vendor with a layer).
     *
     * @note For the layer's calls the connection has no member of its own
     *       for: get_face() lends the face for any of them, but leaves their
     *       results to the caller.
     *
     * @param[in] _operation  the operation's name, for the message.
     * @param[in] _function   called once with the face (Derived's
     *                        get_face()); returns the layer's result, 0 for
     *                        success, and keeps what a call writes through
     *                        a parameter in what it captured.
     * @param[in] _kind       which exception a failure throws, unless its
     *                        result means the connection was lost.
     * @throws connection_exception when the connection is not open, or when
     *         the result means the database dropped it; otherwise what
     *         `_kind` names, with the layer's result as its error_code(),
     *         described as the vendor describes its failures.
     */
    template<typename Function,
             typename Type = Derived,
             typename      = internal::face_hook_t<Type>>
    void run_layer(const char*  _operation,
                   Function&&   _function,
                   failure_kind _kind = failure_kind::query);

    /**
     * @brief Calls the layer, as run_layer does, for calls that only ask:
     *        the face is lent const, and the state is left as it is; gated.
     *
     * @param[in] _operation  the operation's name, for the message.
     * @param[in] _function   called once with the face, const.
     * @param[in] _kind       as run_layer's.
     * @throws as run_layer; a connection_exception marks the connection
     *         lost.
     */
    template<typename Function,
             typename Type = Derived,
             typename      = internal::face_hook_t<Type>>
    void ask_layer(const char*  _operation,
                   Function&&   _function,
                   failure_kind _kind = failure_kind::query) const;

    /**
     * @brief Begins a transaction, in the vendor's default mode.
     *
     * @throws connection_exception when the connection is not open, or the
     *         database drops it; transaction_exception, with the layer's
     *         result, when the transaction cannot begin -- one already open
     *         among the reasons.
     */
    void begin_transaction();

    /**
     * @brief Begins a transaction at an isolation level; gated.
     *
     * @note A vendor may run it at a stronger level, as isolation_level
     *       says; default_level begins it as begin_transaction() does.
     *
     * @param[in] _isolation  the level.
     * @throws as begin_transaction().
     */
    template<typename Type = Derived,
             typename      = internal::begin_transaction_level_hook_t<Type>>
    void begin_transaction(isolation_level _isolation);

    // ending -- commit and rollback end the transaction, and throw as
    // begin_transaction() does when the database refuses; PostgreSQL's commit
    // of a failed transaction rolls it back and throws, 25P02
    void commit();
    void rollback();

    // in_transaction -- whether a transaction is open, as the vendor sees it:
    // one begun with SQL counts too
    bool in_transaction() const noexcept;

    // isolation -- the level of the transactions the session begins from
    // now on; each gated, and throwing as begin_transaction() does
    template<typename Type = Derived,
             typename      = internal::get_isolation_level_hook_t<Type>>
    isolation_level get_isolation_level() const;

    template<typename Type = Derived,
             typename      = internal::set_isolation_level_hook_t<Type>>
    void set_isolation_level(isolation_level _level);

    // savepoints -- by name, inside a transaction; each gated, and throwing
    // as begin_transaction() does
    template<typename Type = Derived,
             typename      = internal::create_savepoint_hook_t<Type>>
    void create_savepoint(const std::string& _name);

    template<typename Type = Derived,
             typename      = internal::release_savepoint_hook_t<Type>>
    void release_savepoint(const std::string& _name);

    template<typename Type = Derived,
             typename      = internal::rollback_to_savepoint_hook_t<Type>>
    void rollback_to_savepoint(const std::string& _name);

    // metadata -- the server's version, read once when the connection opens
    // ("" while it is closed); the configured database; the id the latest
    // INSERT generated on this connection, 0 when none has, throwing as
    // execute_query does; and the rows the latest statement run on it
    // changed, through the connection or a prepared statement, 0 before any
    std::string     get_server_version() const;
    std::string     get_database_name() const;
    re_std::int64_t get_last_insert_id() const;
    re_std::int64_t get_affected_rows() const noexcept;

    /**
     * @brief Turns auto-commit mode on or off; gated (MariaDB and MySQL).
     *
     * @param[in] _enabled  whether each statement commits on its own.
     * @throws as begin_transaction().
     */
    template<typename Type = Derived,
             typename      = internal::set_auto_commit_hook_t<Type>>
    void set_auto_commit(bool _enabled);

    /**
     * @brief Says whether each statement commits on its own: the session's
     *        setting where the vendor has one, and otherwise whether no
     *        transaction is open.
     *
     * @return the mode; for a closed connection, the mode it opens in.
     * @throws as execute_query, where the vendor asks the server.
     */
    bool get_auto_commit() const;

    // errors -- the latest failure an operation threw: its message and the
    // layer's result, until clear_error() or the next connect()
    std::string get_last_error() const;
    int         get_last_error_code() const noexcept;
    bool        has_error() const noexcept;
    void        clear_error() noexcept;

    // reconnection -- how many failed reconnections in a row reconnect()
    // allows (3 by default), and how many there have been
    std::size_t get_max_reconnect_attempts() const noexcept;
    void        set_max_reconnect_attempts(std::size_t _max) noexcept;
    std::size_t get_reconnect_attempts() const noexcept;
    void        reset_reconnect_counter() noexcept;

protected:
    database_connection();
    explicit database_connection(const connection_config& _config);

    ~database_connection() = default;

    database_connection(const database_connection&)            = delete;
    database_connection& operator=(const database_connection&) = delete;

    database_connection(database_connection&& _other) noexcept;
    database_connection& operator=(database_connection&& _other) noexcept;

    // ensure_connected -- connection_exception unless the connection is open
    // and no statement found it lost; query_exception, with the layer's
    // busy_code, while a stream's run, or an operation that does not block,
    // holds it
    void ensure_connected() const;

    // capture_error -- records a failure as the latest, for vendor members
    // that report one without throwing it
    void capture_error(int                _code,
                       const std::string& _message) const;
    void capture_error(int         _code,
                       const char* _message) const;

    /**
     * @brief Runs a vendor operation as every operation here runs: once the
     *        connection is open, its state `executing` meanwhile, and its
     *        failure recorded.
     *
     * @param[in] _function  the operation; called once.
     * @return what the operation returns.
     * @throws connection_exception when the connection is not open; and what
     *         the operation throws, a connection_exception leaving the
     *         connection in connection_state::error.
     */
    template<typename Function>
    auto run_guarded(Function&& _function) -> decltype(_function());

    /**
     * @brief Runs a vendor operation that only asks, as run_guarded does,
     *        without the `executing` state; a connection_exception marks the
     *        connection lost.
     *
     * @param[in] _function  the operation; called once.
     * @return what the operation returns.
     * @throws as run_guarded.
     */
    template<typename Function>
    auto ask_guarded(Function&& _function) const -> decltype(_function());

    // record_changes -- the rows a vendor operation changed, as
    // get_affected_rows() then answers
    void record_changes(re_std::int64_t _changes) noexcept;

    // close_statements -- closes every statement prepared on the
    // connection, which stays open: for a vendor operation that ends the
    // session's statements without closing the connection, as a reset does
    void close_statements() noexcept;

    /**
     * @brief Closes every statement prepared on the connection, and marks it
     *        disconnected, without calling a hook: for the destructor of the
     *        class that holds the face, before the face closes.
     *
     * @post Every statement prepared on the connection is closed.
     */
    void close_session_noexcept() noexcept;

private:
    template<typename      Type,
             database_type Vendor,
             typename      Result>
    friend class internal::sent_steps;

    template<typename      Type,
             database_type Vendor>
    friend class internal::connect_steps;

    // m_close_quietly -- the statements, then the connection; nothing thrown
    void m_close_quietly() noexcept;

    // m_query_as -- execute_query_impl, for an operation of another name:
    // its rows, or its failure under `_operation`
    face::rows m_query_as(const char*        _operation,
                          const std::string& _query);

    // m_refuse_nul_settings -- connection_exception with
    // D_DB_INVALID_ARGUMENT for a setting of the generic configuration that
    // holds a NUL, which the layer would read cut short
    void m_refuse_nul_settings() const;

    // m_effective_state -- the state, `error` where a statement found the
    // connection lost
    connection_state m_effective_state() const noexcept;

    // m_capture -- records a core/db exception as the latest failure
    void m_capture(const exception& _exception) const;

    Derived&       m_self() noexcept;
    const Derived& m_self() const noexcept;

    std::shared_ptr<internal::session> m_session;
    std::size_t                        m_reconnect_attempts;
    std::size_t                        m_max_reconnect_attempts;
    mutable int                        m_last_error_code;
    mutable std::string                m_last_error;
    std::string                        m_server_version;
};

// 1.2    Definitions
//------------------------------------------------------------------------------

template<typename      Derived,
         database_type DbType>
database_connection<Derived, DbType>::database_connection()
    : base_type(),
      m_session(),
      m_reconnect_attempts(0),
      m_max_reconnect_attempts(3),
      m_last_error_code(0)
{}

template<typename      Derived,
         database_type DbType>
database_connection<Derived, DbType>::database_connection(
    const connection_config& _config
)
    : base_type(_config),
      m_session(),
      m_reconnect_attempts(0),
      m_max_reconnect_attempts(3),
      m_last_error_code(0)
{}

/*
database_connection::database_connection (move)
  The session moves with the connection, so that the statements prepared on
it, which hold the session and not the connection, still belong to it. The
connection moved from is left disconnected, with no session: its next
connect() makes one.
*/
template<typename      Derived,
         database_type DbType>
database_connection<Derived, DbType>::database_connection(
    database_connection&& _other
) noexcept
    : base_type(std::move(_other)),
      m_session(std::move(_other.m_session)),
      m_reconnect_attempts(_other.m_reconnect_attempts),
      m_max_reconnect_attempts(_other.m_max_reconnect_attempts),
      m_last_error_code(_other.m_last_error_code),
      m_last_error(std::move(_other.m_last_error)),
      m_server_version(std::move(_other.m_server_version))
{
    _other.m_state              = connection_state::disconnected;
    _other.m_in_transaction     = false;
    _other.m_reconnect_attempts = 0;
    _other.m_last_error_code    = 0;
    _other.m_last_error.clear();
}

/*
database_connection::operator= (move)
  This connection closes first, through its hooks, while the whole object is
alive; the class that holds the face then moves the face, whose move closes
nothing more, the connection being closed already.
*/
template<typename      Derived,
         database_type DbType>
database_connection<Derived, DbType>&
database_connection<Derived, DbType>::operator=(
    database_connection&& _other
) noexcept
{
    // a connection assigned to itself keeps what it has
    if (this != &_other)
    {
        m_close_quietly();

        base_type::operator=(std::move(_other));

        m_session                = std::move(_other.m_session);
        m_reconnect_attempts     = _other.m_reconnect_attempts;
        m_max_reconnect_attempts = _other.m_max_reconnect_attempts;
        m_last_error_code        = _other.m_last_error_code;
        m_last_error             = std::move(_other.m_last_error);
        m_server_version         = std::move(_other.m_server_version);

        _other.m_state              = connection_state::disconnected;
        _other.m_in_transaction     = false;
        _other.m_reconnect_attempts = 0;
        _other.m_last_error_code    = 0;
        _other.m_last_error.clear();
    }

    return *this;
}

/*
database_connection::connect
  The session is made before the connection opens: making it can fail for
memory, and a failure after the open would leave an open connection marked
otherwise. A connection in error -- one a statement found lost, or one that
failed to open -- is closed first, so that its old handle goes.
*/
template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::connect()
{
    // an open connection stays as it is
    if (m_effective_state() == connection_state::connected)
    {
        return;
    }

    // core/db never reconnects behind a program's back
    if (this->m_config.auto_reconnect)
    {
        throw connection_exception(std::string(traits_type::display_name) +
                                   " connect: auto_reconnect is not "
                                   "supported; reconnect() reconnects",
                                   D_DB_UNSUPPORTED);
    }

    // what is left of an earlier connection goes first: one in error, or
    // one a statement found lost
    if (this->m_state != connection_state::disconnected)
    {
        m_close_quietly();
    }

    // a session for the statements to come, made or renewed
    if (m_session)
    {
        m_session->renew();
    }
    else
    {
        m_session = std::make_shared<internal::session>();
    }

    this->m_state = connection_state::connecting;
    clear_error();

    try
    {
        m_refuse_nul_settings();
        m_self().connect_impl();
    }
    catch (const exception& _e)
    {
        this->m_state = connection_state::error;
        m_capture(_e);

        throw;
    }
    catch (...)
    {
        this->m_state = connection_state::error;

        throw;
    }

    this->m_state          = connection_state::connected;
    this->m_in_transaction = false;
    this->m_auto_commit    = true;
    m_reconnect_attempts   = 0;

    try
    {
        m_server_version = m_self().get_server_version_impl();
    }
    catch (...)
    {
        // the version is for reading later; not having it costs nothing
        m_server_version.clear();
    }

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::connect(
    const connection_config& _config
)
{
    this->m_config = _config;

    // the vendor's port where the configuration names none
    if (this->m_config.port == 0)
    {
        this->m_config.port = traits_type::default_port;
    }

    connect();

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::disconnect() noexcept
{
    // nothing open to close
    if ( (this->m_state == connection_state::disconnected) ||
         (this->m_state == connection_state::closed) )
    {
        return;
    }

    m_close_quietly();

    this->m_state          = connection_state::disconnected;
    this->m_in_transaction = false;

    return;
}

/*
database_connection::reconnect
  The counter counts reconnections in a row that did not succeed; connect()
sets it to 0 when one does.
*/
template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::reconnect()
{
    // as many failures in a row as allowed is enough
    if (m_reconnect_attempts >= m_max_reconnect_attempts)
    {
        throw connection_exception(std::string(traits_type::display_name) +
                                   " reconnect: the maximum of " +
                                   std::to_string(m_max_reconnect_attempts) +
                                   " reconnection attempts is reached");
    }

    ++m_reconnect_attempts;

    // the old connection goes, whether open or in error
    if (this->m_state != connection_state::disconnected)
    {
        m_close_quietly();

        this->m_state = connection_state::disconnected;
    }

    connect();

    return;
}

template<typename      Derived,
         database_type DbType>
bool
database_connection<Derived, DbType>::is_connected() const noexcept
{
    // only a connection open and not found lost can be connected
    if (m_effective_state() != connection_state::connected)
    {
        return false;
    }

    try
    {
        return m_self().is_connected_impl();
    }
    catch (...)
    {
        return false;
    }
}

template<typename      Derived,
         database_type DbType>
bool
database_connection<Derived, DbType>::ping() const noexcept
{
    // a closed connection answers nothing
    if (m_effective_state() != connection_state::connected)
    {
        return false;
    }

    try
    {
        return m_self().ping_impl();
    }
    catch (...)
    {
        return false;
    }
}

template<typename      Derived,
         database_type DbType>
connection_state
database_connection<Derived, DbType>::get_state() const noexcept
{
    return m_effective_state();
}

template<typename      Derived,
         database_type DbType>
std::unique_ptr<buffered_result_set>
database_connection<Derived, DbType>::execute_query(
    const std::string& _query
)
{
    return run_guarded(
        [this, &_query]() -> std::unique_ptr<buffered_result_set>
        {
            internal::refuse_nul(_query, "execute_query", "the SQL");

            return std::make_unique<buffered_result_set>(
                m_self().execute_query_impl(_query));
        });
}

template<typename      Derived,
         database_type DbType>
re_std::int64_t
database_connection<Derived, DbType>::execute_update(
    const std::string& _query
)
{
    return run_guarded(
        [this, &_query]() -> re_std::int64_t
        {
            internal::refuse_nul(_query, "execute_update", "the SQL");

            const re_std::int64_t changes =
                m_self().execute_update_impl(_query);

            m_session->record(changes);

            return changes;
        });
}

/*
database_connection::execute
  Runs the hook execute_update runs, under its own name: the vendor's hook
labels its failures execute_update, which internal::relabel turns into
execute.
*/
template<typename      Derived,
         database_type DbType>
bool
database_connection<Derived, DbType>::execute(
    const std::string& _query
)
{
    run_guarded(
        [this, &_query]() -> void
        {
            internal::refuse_nul(_query, "execute", "the SQL");

            re_std::int64_t changes = 0;

            try
            {
                changes = m_self().execute_update_impl(_query);
            }
            catch (const exception& _e)
            {
                internal::relabel(_e, "execute_update", "execute");
            }

            m_session->record(changes);

            return;
        });

    return true;
}

/*
database_connection::prepare
  The statement shares the session, never the connection object, which can
move while the statement lives.
*/
template<typename      Derived,
         database_type DbType>
auto
database_connection<Derived, DbType>::prepare(
    const std::string& _query
)
{
    using layer_type     = typename Derived::layer_type;
    using statement_type = prepared_statement<layer_type>;

    return run_guarded(
        [this, &_query]() -> std::unique_ptr<statement_type>
        {
            internal::refuse_nul(_query, "prepare", "the SQL");

            return std::make_unique<statement_type>(
                m_session,
                m_self().prepare_impl(_query));
        });
}

/*
database_connection::stream_query
  The statement is prepared as prepare() prepares any, then handed to the
stream, which outlives the call; the run is one of the connection's
operations, its failure recorded as theirs are.
*/
template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
auto
database_connection<Derived, DbType>::stream_query(
    const std::string& _query,
    std::size_t        _page_size
)
{
    using layer_type     = typename Derived::layer_type;
    using statement_type = prepared_statement<layer_type>;

    std::unique_ptr<statement_type> statement = prepare(_query);

    return run_guarded(
        [&statement, _page_size]()
            -> std::unique_ptr<streaming_result_set<layer_type>>
        {
            return statement_type::stream(std::move(statement), _page_size);
        });
}

template<typename      Derived,
         database_type DbType>
std::optional<value>
database_connection<Derived, DbType>::execute_scalar(
    const std::string& _query
)
{
    return run_guarded(
        [this, &_query]() -> std::optional<value>
        {
            buffered_result_set found(m_query_as("execute_scalar", _query));

            // the first column of the first row, when there is a row
            if ( (found.next()) &&
                 (found.column_count() > 0u) )
            {
                return found.get_value(static_cast<std::size_t>(0));
            }

            return std::nullopt;
        });
}

template<typename      Derived,
         database_type DbType>
std::optional<row>
database_connection<Derived, DbType>::execute_single_row(
    const std::string& _query
)
{
    return run_guarded(
        [this, &_query]() -> std::optional<row>
        {
            buffered_result_set found(m_query_as("execute_single_row",
                                                 _query));

            // the first row, when there is one
            if (found.next())
            {
                return found.get_current_row();
            }

            return std::nullopt;
        });
}

template<typename      Derived,
         database_type DbType>
template<typename Function,
         typename Type,
         typename>
void
database_connection<Derived, DbType>::run_layer(
    const char*  _operation,
    Function&&   _function,
    failure_kind _kind
)
{
    run_guarded(
        [this, _operation, &_function, _kind]() -> void
        {
            auto&     face   = m_self().get_face();
            const int result = _function(face);

            // a failure, raised as the vendor raises its own
            if (result != 0)
            {
                Derived::layer_type::raise(face.handle(),
                                           _kind,
                                           _operation,
                                           result);
            }

            return;
        });

    return;
}

template<typename      Derived,
         database_type DbType>
template<typename Function,
         typename Type,
         typename>
void
database_connection<Derived, DbType>::ask_layer(
    const char*  _operation,
    Function&&   _function,
    failure_kind _kind
) const
{
    ask_guarded(
        [this, _operation, &_function, _kind]() -> void
        {
            const auto& face   = m_self().get_face();
            const int   result = _function(face);

            // a failure, raised as the vendor raises its own
            if (result != 0)
            {
                Derived::layer_type::raise(face.handle(),
                                           _kind,
                                           _operation,
                                           result);
            }

            return;
        });

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::begin_transaction()
{
    run_guarded(
        [this]() -> void
        {
            m_self().begin_transaction_impl();

            return;
        });

    this->m_in_transaction = true;

    return;
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
void
database_connection<Derived, DbType>::begin_transaction(
    isolation_level _isolation
)
{
    run_guarded(
        [this, _isolation]() -> void
        {
            m_self().begin_transaction_impl(_isolation);

            return;
        });

    this->m_in_transaction = true;

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::commit()
{
    run_guarded(
        [this]() -> void
        {
            m_self().commit_impl();

            return;
        });

    this->m_in_transaction = false;

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::rollback()
{
    run_guarded(
        [this]() -> void
        {
            m_self().rollback_impl();

            return;
        });

    this->m_in_transaction = false;

    return;
}

/*
database_connection::in_transaction
  A vendor whose connection can say -- every one with a layer -- is asked,
so that a transaction begun with SQL counts; one that cannot is answered
from what this class saw begin and end.
*/
template<typename      Derived,
         database_type DbType>
bool
database_connection<Derived, DbType>::in_transaction() const noexcept
{
    // the vendor's own answer, where it has one
    if constexpr (is_detected<internal::in_transaction_hook_t,
                              Derived>::value)
    {
        return ( (m_effective_state() == connection_state::connected) &&
                 (m_self().in_transaction_impl()) );
    }
    else
    {
        return this->m_in_transaction;
    }
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
isolation_level
database_connection<Derived, DbType>::get_isolation_level() const
{
    return ask_guarded(
        [this]() -> isolation_level
        {
            return m_self().get_isolation_level_impl();
        });
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
void
database_connection<Derived, DbType>::set_isolation_level(
    isolation_level _level
)
{
    run_guarded(
        [this, _level]() -> void
        {
            m_self().set_isolation_level_impl(_level);

            return;
        });

    return;
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
void
database_connection<Derived, DbType>::create_savepoint(
    const std::string& _name
)
{
    run_guarded(
        [this, &_name]() -> void
        {
            internal::refuse_nul(_name, "create_savepoint", "the name");
            m_self().create_savepoint_impl(_name);

            return;
        });

    return;
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
void
database_connection<Derived, DbType>::release_savepoint(
    const std::string& _name
)
{
    run_guarded(
        [this, &_name]() -> void
        {
            internal::refuse_nul(_name, "release_savepoint", "the name");
            m_self().release_savepoint_impl(_name);

            return;
        });

    return;
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
void
database_connection<Derived, DbType>::rollback_to_savepoint(
    const std::string& _name
)
{
    run_guarded(
        [this, &_name]() -> void
        {
            internal::refuse_nul(_name, "rollback_to_savepoint", "the name");
            m_self().rollback_to_savepoint_impl(_name);

            return;
        });

    return;
}

template<typename      Derived,
         database_type DbType>
std::string
database_connection<Derived, DbType>::get_server_version() const
{
    return m_server_version;
}

template<typename      Derived,
         database_type DbType>
std::string
database_connection<Derived, DbType>::get_database_name() const
{
    return this->m_config.database;
}

template<typename      Derived,
         database_type DbType>
re_std::int64_t
database_connection<Derived, DbType>::get_last_insert_id() const
{
    return ask_guarded(
        [this]() -> re_std::int64_t
        {
            return m_self().get_last_insert_id_impl();
        });
}

template<typename      Derived,
         database_type DbType>
re_std::int64_t
database_connection<Derived, DbType>::get_affected_rows() const noexcept
{
    return (m_session) ? m_session->changes()
                       : 0;
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
void
database_connection<Derived, DbType>::set_auto_commit(
    bool _enabled
)
{
    run_guarded(
        [this, _enabled]() -> void
        {
            m_self().set_auto_commit_impl(_enabled);

            return;
        });

    this->m_auto_commit = _enabled;

    return;
}

/*
database_connection::get_auto_commit
  SQLite and PostgreSQL have no auto-commit setting: a statement outside a
transaction commits on its own, so the mode is whether none is open.
*/
template<typename      Derived,
         database_type DbType>
bool
database_connection<Derived, DbType>::get_auto_commit() const
{
    // a closed connection opens in the mode it was last set to, or on
    if (!is_connected())
    {
        return this->m_auto_commit;
    }

    // the session's setting, where the vendor has one
    if constexpr (is_detected<internal::get_auto_commit_hook_t,
                              Derived>::value)
    {
        return ask_guarded(
            [this]() -> bool
            {
                return m_self().get_auto_commit_impl();
            });
    }
    else
    {
        return !in_transaction();
    }
}

template<typename      Derived,
         database_type DbType>
std::string
database_connection<Derived, DbType>::get_last_error() const
{
    return m_last_error;
}

template<typename      Derived,
         database_type DbType>
int
database_connection<Derived, DbType>::get_last_error_code() const noexcept
{
    return m_last_error_code;
}

template<typename      Derived,
         database_type DbType>
bool
database_connection<Derived, DbType>::has_error() const noexcept
{
    return ( (m_last_error_code != 0) ||
             (!m_last_error.empty()) );
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::clear_error() noexcept
{
    m_last_error_code = 0;
    m_last_error.clear();

    return;
}

template<typename      Derived,
         database_type DbType>
std::size_t
database_connection<Derived, DbType>::get_max_reconnect_attempts()
    const noexcept
{
    return m_max_reconnect_attempts;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::set_max_reconnect_attempts(
    std::size_t _max
) noexcept
{
    m_max_reconnect_attempts = _max;

    return;
}

template<typename      Derived,
         database_type DbType>
std::size_t
database_connection<Derived, DbType>::get_reconnect_attempts() const noexcept
{
    return m_reconnect_attempts;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::reset_reconnect_counter() noexcept
{
    m_reconnect_attempts = 0;

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::ensure_connected() const
{
    // only an open connection, not found lost, runs anything
    if (m_effective_state() != connection_state::connected)
    {
        throw connection_exception(std::string("not connected to ") +
                                   traits_type::display_name);
    }

    // nor while a stream's run, or an operation that does not block, holds
    // it, which the layer would refuse
    if ( (m_session) &&
         (m_session->holder() != nullptr) )
    {
        throw query_exception(std::string("a stream's run, or an operation "
                                          "not yet done, holds the "
                                          "connection to ") +
                              traits_type::display_name,
                              m_session->busy_code());
    }

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::capture_error(
    int                _code,
    const std::string& _message
) const
{
    m_last_error_code = _code;
    m_last_error      = _message;

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::capture_error(
    int         _code,
    const char* _message
) const
{
    m_last_error_code = _code;
    m_last_error      = (_message) ? _message
                                   : "";

    return;
}

/*
database_connection::run_guarded
  The state is put back on every path out: `connected` after success or a
failure of the statement's own, `error` after a connection_exception, and
`connected` after anything else that propagates -- a std::bad_alloc among
it -- which is rethrown untouched.
*/
template<typename      Derived,
         database_type DbType>
template<typename Function>
auto
database_connection<Derived, DbType>::run_guarded(
    Function&& _function
) -> decltype(_function())
{
    ensure_connected();

    this->m_state = connection_state::executing;

    try
    {
        // the operation, its result kept across the state's restoring
        if constexpr (std::is_void_v<decltype(_function())>)
        {
            _function();

            this->m_state = connection_state::connected;

            return;
        }
        else
        {
            auto result = _function();

            this->m_state = connection_state::connected;

            return result;
        }
    }
    catch (const connection_exception& _e)
    {
        this->m_state = connection_state::error;
        m_capture(_e);

        throw;
    }
    catch (const exception& _e)
    {
        this->m_state = connection_state::connected;
        m_capture(_e);

        throw;
    }
    catch (...)
    {
        this->m_state = connection_state::connected;

        throw;
    }
}

/*
database_connection::ask_guarded
  A const operation cannot change the state; the session, which it reaches
through a pointer, carries the loss instead, and m_effective_state reads it.
*/
template<typename      Derived,
         database_type DbType>
template<typename Function>
auto
database_connection<Derived, DbType>::ask_guarded(
    Function&& _function
) const -> decltype(_function())
{
    ensure_connected();

    try
    {
        return _function();
    }
    catch (const connection_exception& _e)
    {
        // the loss is the session's to keep until the next connect
        if (m_session)
        {
            m_session->mark_lost();
        }

        m_capture(_e);

        throw;
    }
    catch (const exception& _e)
    {
        m_capture(_e);

        throw;
    }
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::record_changes(
    re_std::int64_t _changes
) noexcept
{
    // only a connection that ran something has a session to tell
    if (m_session)
    {
        m_session->record(_changes);
    }

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::close_statements() noexcept
{
    // only a connection that ran something has statements to close
    if (m_session)
    {
        m_session->close_all();
    }

    return;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::close_session_noexcept() noexcept
{
    // the statements go while the connection they run on is still there
    if (m_session)
    {
        m_session->close_all();
    }

    this->m_state          = connection_state::disconnected;
    this->m_in_transaction = false;

    return;
}

/*
database_connection::m_refuse_nul_settings
  Every text field of the configuration reaches a layer as a C string, a
file's path or a host's name among them, which a NUL would end early.
*/
template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::m_refuse_nul_settings() const
{
    const connection_config& config    = this->m_config;
    const std::string        operation =
        std::string(traits_type::display_name) + " connect";

    internal::refuse_nul_setting(config.host, operation.c_str(), "host");
    internal::refuse_nul_setting(config.database, operation.c_str(),
                                 "database");
    internal::refuse_nul_setting(config.username, operation.c_str(),
                                 "username");
    internal::refuse_nul_setting(config.password, operation.c_str(),
                                 "password");
    internal::refuse_nul_setting(config.charset.value_or(std::string()),
                                 operation.c_str(),
                                 "charset");
    internal::refuse_nul_setting(config.schema.value_or(std::string()),
                                 operation.c_str(),
                                 "schema");
    internal::refuse_nul_setting(config.ssl_cert.value_or(std::string()),
                                 operation.c_str(),
                                 "ssl_cert");
    internal::refuse_nul_setting(config.ssl_key.value_or(std::string()),
                                 operation.c_str(),
                                 "ssl_key");
    internal::refuse_nul_setting(config.ssl_ca.value_or(std::string()),
                                 operation.c_str(),
                                 "ssl_ca");

    // each custom option, by name and value
    for (const std::pair<const std::string, std::string>& option :
         config.custom_options)
    {
        internal::refuse_nul_setting(option.first,
                                     operation.c_str(),
                                     "a custom option's name");
        internal::refuse_nul_setting(option.second,
                                     operation.c_str(),
                                     option.first.c_str());
    }

    return;
}

/*
database_connection::m_close_quietly
  Statements first: PostgreSQL's must be deallocated on a live connection,
and none may be finalized after its connection closes.
*/
template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::m_close_quietly() noexcept
{
    // the statements go while the connection they run on is still there
    if (m_session)
    {
        m_session->close_all();
    }

    try
    {
        m_self().disconnect_impl();
    }
    catch (...)
    {
        // closing has nothing to report: the connection is gone either way
    }

    return;
}

/*
database_connection::m_query_as
  The caller runs it inside run_guarded, which records the failure as
relabeled here.
*/
template<typename      Derived,
         database_type DbType>
face::rows
database_connection<Derived, DbType>::m_query_as(
    const char*        _operation,
    const std::string& _query
)
{
    internal::refuse_nul(_query, _operation, "the SQL");

    try
    {
        return m_self().execute_query_impl(_query);
    }
    catch (const exception& _e)
    {
        internal::relabel(_e, "execute_query", _operation);
    }
}

template<typename      Derived,
         database_type DbType>
connection_state
database_connection<Derived, DbType>::m_effective_state() const noexcept
{
    // a statement may have found the connection lost
    if ( (this->m_state == connection_state::connected) &&
         (m_session)                                    &&
         (m_session->lost()) )
    {
        return connection_state::error;
    }

    return this->m_state;
}

template<typename      Derived,
         database_type DbType>
void
database_connection<Derived, DbType>::m_capture(
    const exception& _exception
) const
{
    capture_error(_exception.error_code(), _exception.what());

    return;
}

template<typename      Derived,
         database_type DbType>
Derived&
database_connection<Derived, DbType>::m_self() noexcept
{
    return static_cast<Derived&>(*this);
}

template<typename      Derived,
         database_type DbType>
const Derived&
database_connection<Derived, DbType>::m_self() const noexcept
{
    return static_cast<const Derived&>(*this);
}


// 1.3    Operations that do not block
//------------------------------------------------------------------------------
NS_INTERNAL

    // send_query_start_t, connect_start_hook_t -- a face that sends SQL
    // without waiting, and a vendor that connects without waiting, as the
    // calls; their absence makes the operation done as it starts
    template<typename Face>
    using send_query_start_t =
        decltype(std::declval<Face&>().send_query_start(
            std::declval<face::string_arg>(),
            std::declval<d_db_pending&>()));

    template<typename Type>
    using connect_start_hook_t =
        decltype(std::declval<Type&>().connect_start_impl(
            std::declval<d_db_pending&>()));

    // 1.3.1
    // sent_steps
    //   class template: SQL sent without waiting, and every result it
    // produces read as it arrives -- the rows of the first kept, the count of
    // each -- for execute_query_async (Result the rows), execute_update_async
    // (the count) and execute_async (true). A failure among the results is
    // described at once, while the connection's message is its own, and
    // thrown once the results are read to their end, so that the connection
    // is free again; a second failure ends the reading.
    template<typename      Derived,
             database_type DbType,
             typename      Result>
    class sent_steps final : public async_steps<Result>
    {
    private:
        using connection_type = database_connection<Derived, DbType>;
        using layer_type      = typename Derived::layer_type;

    public:
        sent_steps(
            connection_type& _connection,
            const char*      _operation,
            std::string      _sql
        )
            : m_connection(&_connection),
              m_operation(_operation),
              m_sql(std::move(_sql)),
              m_sending(true),
              m_first(false),
              m_rows(),
              m_reading(),
              m_changes(0),
              m_counted(0),
              m_failure()
        {}

        bool   begin(d_db_pending& _pending) override;
        bool   advance(unsigned      _events,
                       d_db_pending& _pending) override;
        Result take() override;
        void   ended() noexcept override;
        void   cancel() override;
        void   abandon() noexcept override;

    private:
        // m_sent, m_read -- the send's answer, and a result's: false while
        // waiting, true once every result is read
        bool m_sent(int           _result,
                    d_db_pending& _pending);
        bool m_read(int           _result,
                    d_db_pending& _pending);

        // m_raise -- the layer's exception for a failed call
        [[noreturn]] void m_raise(int _result) const;

        // m_guarded -- a step, its failure recorded as the connection's
        // blocking operations record theirs
        template<typename Step>
        bool m_guarded(Step&& _step);

        connection_type*   m_connection;
        const char*        m_operation;
        std::string        m_sql;
        bool               m_sending;
        bool               m_first;
        face::rows         m_rows;
        face::rows         m_reading;
        re_std::int64_t    m_changes;
        re_std::int64_t    m_counted;
        std::exception_ptr m_failure;
    };

    // 1.3.2
    // connect_steps
    //   class template: connect_async over the vendor's hooks that connect
    // without waiting -- connect_start_impl, connect_continue_impl, each the
    // layer's result, D_DB_WAIT while waiting, throwing for a failure;
    // connect_finish_impl, the setup that follows; connect_abandon_impl,
    // the connection being made closed -- around which it keeps the
    // connection's state as connect() keeps it.
    template<typename      Derived,
             database_type DbType>
    class connect_steps final : public async_steps<void>
    {
    private:
        using connection_type = database_connection<Derived, DbType>;

    public:
        explicit connect_steps(
            connection_type& _connection
        ) noexcept
            : m_connection(&_connection)
        {}

        bool begin(d_db_pending& _pending) override;
        bool advance(unsigned      _events,
                     d_db_pending& _pending) override;
        void take() override;
        void ended() noexcept override;
        void cancel() override;
        void abandon() noexcept override;

    private:
        // m_after -- a hook's answer: false while waiting; true once
        // connected, the setup made and the state kept
        bool m_after(int _result);

        // m_guarded -- a step, a failure leaving the connection in error
        template<typename Step>
        bool m_guarded(Step&& _step);

        connection_type* m_connection;
    };

NS_END  // internal

// 1.4    Their definitions
//------------------------------------------------------------------------------
// the connection's members
template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
async_operation<void>
database_connection<Derived, DbType>::connect_async()
{
    // the vendor's hooks, or a connect done as it starts
    if constexpr (is_detected_v<internal::connect_start_hook_t, Derived>)
    {
        return async_operation<void>(
            std::make_unique<internal::connect_steps<Derived, DbType>>(
                *this));
    }
    else
    {
        return async_operation<void>(
            std::make_unique<internal::at_once_steps<void>>(
                [this]() -> void
                {
                    connect();

                    return;
                }));
    }
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
async_operation<std::unique_ptr<buffered_result_set>>
database_connection<Derived, DbType>::execute_query_async(
    const std::string& _query
)
{
    using result_type = std::unique_ptr<buffered_result_set>;
    using face_type   = std::decay_t<decltype(m_self().get_face())>;

    // SQL sent without waiting, or a query run as it starts
    if constexpr (is_detected_v<internal::send_query_start_t, face_type>)
    {
        return async_operation<result_type>(
            std::make_unique<internal::sent_steps<Derived,
                                                  DbType,
                                                  result_type>>(
                *this,
                "execute_query",
                _query));
    }
    else
    {
        return async_operation<result_type>(
            std::make_unique<internal::at_once_steps<result_type>>(
                [this, _query]() -> result_type
                {
                    return execute_query(_query);
                }));
    }
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
async_operation<re_std::int64_t>
database_connection<Derived, DbType>::execute_update_async(
    const std::string& _query
)
{
    using face_type = std::decay_t<decltype(m_self().get_face())>;

    // SQL sent without waiting, or run as it starts
    if constexpr (is_detected_v<internal::send_query_start_t, face_type>)
    {
        return async_operation<re_std::int64_t>(
            std::make_unique<internal::sent_steps<Derived,
                                                  DbType,
                                                  re_std::int64_t>>(
                *this,
                "execute_update",
                _query));
    }
    else
    {
        return async_operation<re_std::int64_t>(
            std::make_unique<internal::at_once_steps<re_std::int64_t>>(
                [this, _query]() -> re_std::int64_t
                {
                    return execute_update(_query);
                }));
    }
}

template<typename      Derived,
         database_type DbType>
template<typename Type,
         typename>
async_operation<bool>
database_connection<Derived, DbType>::execute_async(
    const std::string& _query
)
{
    using face_type = std::decay_t<decltype(m_self().get_face())>;

    // SQL sent without waiting, or run as it starts
    if constexpr (is_detected_v<internal::send_query_start_t, face_type>)
    {
        return async_operation<bool>(
            std::make_unique<internal::sent_steps<Derived, DbType, bool>>(
                *this,
                "execute",
                _query));
    }
    else
    {
        return async_operation<bool>(
            std::make_unique<internal::at_once_steps<bool>>(
                [this, _query]() -> bool
                {
                    return execute(_query);
                }));
    }
}

NS_INTERNAL

    // sent_steps's members
    /*
    sent_steps::begin
      The connection must be free, as for any operation; the operation holds
    it from here until it ends.
    */
    template<typename      Derived,
             database_type DbType,
             typename      Result>
    bool
    sent_steps<Derived, DbType, Result>::begin(
        d_db_pending& _pending
    )
    {
        m_connection->ensure_connected();
        refuse_nul(m_sql, m_operation, "the SQL");
        m_connection->m_session->hold(this, layer_type::busy_code);

        return m_guarded(
            [this, &_pending]() -> bool
            {
                auto& face = m_connection->m_self().get_face();

                return m_sent(face.send_query_start(m_sql, _pending),
                              _pending);
            });
    }

    template<typename      Derived,
             database_type DbType,
             typename      Result>
    bool
    sent_steps<Derived, DbType, Result>::advance(
        unsigned      _events,
        d_db_pending& _pending
    )
    {
        return m_guarded(
            [this, _events, &_pending]() -> bool
            {
                auto& face = m_connection->m_self().get_face();

                // the send, or the result being read
                if (m_sending)
                {
                    return m_sent(face.send_query_continue(_events,
                                                           _pending),
                                  _pending);
                }

                return m_read(face.next_result_continue(_events,
                                                        _pending,
                                                        m_reading,
                                                        m_counted),
                              _pending);
            });
    }

    /*
    sent_steps::take
      The count goes to the session, as the blocking forms record theirs.
    */
    template<typename      Derived,
             database_type DbType,
             typename      Result>
    Result
    sent_steps<Derived, DbType, Result>::take()
    {
        // the rows of the first result, or the count, or true
        if constexpr (std::is_same_v<Result,
                                     std::unique_ptr<buffered_result_set>>)
        {
            return std::make_unique<buffered_result_set>(std::move(m_rows));
        }
        else
        {
            m_connection->m_session->record(m_changes);

            if constexpr (std::is_same_v<Result, bool>)
            {
                return true;
            }
            else
            {
                return m_changes;
            }
        }
    }

    template<typename      Derived,
             database_type DbType,
             typename      Result>
    void
    sent_steps<Derived, DbType, Result>::ended() noexcept
    {
        m_connection->m_session->release(this);

        return;
    }

    template<typename      Derived,
             database_type DbType,
             typename      Result>
    void
    sent_steps<Derived, DbType, Result>::cancel()
    {
        layer_type::cancel(m_connection->m_self().get_face().handle());

        return;
    }

    /*
    sent_steps::abandon
      The exchange is left in the middle, which nothing can take up again:
    the connection is lost, for reconnect() to replace.
    */
    template<typename      Derived,
             database_type DbType,
             typename      Result>
    void
    sent_steps<Derived, DbType, Result>::abandon() noexcept
    {
        m_connection->m_session->release(this);
        m_connection->m_session->mark_lost();

        return;
    }

    template<typename      Derived,
             database_type DbType,
             typename      Result>
    bool
    sent_steps<Derived, DbType, Result>::m_sent(
        int           _result,
        d_db_pending& _pending
    )
    {
        // waiting still, or a failure of the send's own
        if (_result == D_DB_WAIT)
        {
            return false;
        }

        if (_result != 0)
        {
            m_raise(_result);
        }

        m_sending = false;

        auto& face = m_connection->m_self().get_face();

        return m_read(face.next_result_start(_pending, m_reading, m_counted),
                      _pending);
    }

    /*
    sent_steps::m_read
      Each result read begins the next, until there are none left: the
    first's rows kept, each one's count, the last standing.
    */
    template<typename      Derived,
             database_type DbType,
             typename      Result>
    bool
    sent_steps<Derived, DbType, Result>::m_read(
        int           _result,
        d_db_pending& _pending
    )
    {
        auto& face   = m_connection->m_self().get_face();
        int   result = _result;

        for (;;)
        {
            // waiting still
            if (result == D_DB_WAIT)
            {
                return false;
            }

            // none left: the failure, if one came, or done
            if (result == D_DB_NOT_FOUND)
            {
                if (m_failure)
                {
                    std::rethrow_exception(m_failure);
                }

                return true;
            }

            // a failure, described now, thrown at the end; a second ends it
            if (result != 0)
            {
                if (m_failure)
                {
                    std::rethrow_exception(m_failure);
                }

                try
                {
                    m_raise(result);
                }
                catch (...)
                {
                    m_failure = std::current_exception();
                }
            }
            else
            {
                // the first result's rows kept; each one's count
                if (!m_first)
                {
                    m_rows  = std::move(m_reading);
                    m_first = true;
                }

                m_changes = m_counted;
            }

            result = face.next_result_start(_pending, m_reading, m_counted);
        }
    }

    template<typename      Derived,
             database_type DbType,
             typename      Result>
    void
    sent_steps<Derived, DbType, Result>::m_raise(
        int _result
    ) const
    {
        layer_type::raise(m_connection->m_self().get_face().handle(),
                          failure_kind::query,
                          m_operation,
                          _result);
    }

    /*
    sent_steps::m_guarded
      As run_guarded records a failure: a connection_exception leaves the
    connection in error.
    */
    template<typename      Derived,
             database_type DbType,
             typename      Result>
    template<typename Step>
    bool
    sent_steps<Derived, DbType, Result>::m_guarded(
        Step&& _step
    )
    {
        try
        {
            return _step();
        }
        catch (const connection_exception& _e)
        {
            m_connection->m_state = connection_state::error;
            m_connection->m_capture(_e);

            throw;
        }
        catch (const exception& _e)
        {
            m_connection->m_capture(_e);

            throw;
        }
    }

    // connect_steps's members
    /*
    connect_steps::begin
      As connect() begins: an open connection stays as it is; what is left
    of an earlier one goes; a session made or renewed; the state connecting.
    */
    template<typename      Derived,
             database_type DbType>
    bool
    connect_steps<Derived, DbType>::begin(
        d_db_pending& _pending
    )
    {
        connection_type& db = *m_connection;

        // an open connection stays as it is
        if (db.m_effective_state() == connection_state::connected)
        {
            return true;
        }

        // core/db never reconnects behind a program's back
        if (db.m_config.auto_reconnect)
        {
            throw connection_exception(
                std::string(connection_type::traits_type::display_name) +
                    " connect_async: auto_reconnect is not supported; "
                    "reconnect() reconnects",
                D_DB_UNSUPPORTED);
        }

        // what is left of an earlier connection goes first
        if (db.m_state != connection_state::disconnected)
        {
            db.m_close_quietly();
        }

        // a session for the statements to come, made or renewed
        if (db.m_session)
        {
            db.m_session->renew();
        }
        else
        {
            db.m_session = std::make_shared<session>();
        }

        db.m_state = connection_state::connecting;
        db.clear_error();

        return m_guarded(
            [this, &db, &_pending]() -> bool
            {
                db.m_refuse_nul_settings();

                return m_after(db.m_self().connect_start_impl(_pending));
            });
    }

    template<typename      Derived,
             database_type DbType>
    bool
    connect_steps<Derived, DbType>::advance(
        unsigned      _events,
        d_db_pending& _pending
    )
    {
        return m_guarded(
            [this, _events, &_pending]() -> bool
            {
                return m_after(m_connection->m_self().connect_continue_impl(
                                   _events,
                                   _pending));
            });
    }

    template<typename      Derived,
             database_type DbType>
    void
    connect_steps<Derived, DbType>::take()
    {
        return;
    }

    template<typename      Derived,
             database_type DbType>
    void
    connect_steps<Derived, DbType>::ended() noexcept
    {
        return;
    }

    // cancel -- a connect has nothing on the server to stop
    template<typename      Derived,
             database_type DbType>
    void
    connect_steps<Derived, DbType>::cancel()
    {
        return;
    }

    template<typename      Derived,
             database_type DbType>
    void
    connect_steps<Derived, DbType>::abandon() noexcept
    {
        m_connection->m_self().connect_abandon_impl();
        m_connection->m_state = connection_state::disconnected;

        return;
    }

    /*
    connect_steps::m_after
      Connected: the vendor's setup, then the state connect() leaves.
    */
    template<typename      Derived,
             database_type DbType>
    bool
    connect_steps<Derived, DbType>::m_after(
        int _result
    )
    {
        connection_type& db = *m_connection;

        // waiting still
        if (_result == D_DB_WAIT)
        {
            return false;
        }

        db.m_self().connect_finish_impl();

        db.m_state              = connection_state::connected;
        db.m_in_transaction     = false;
        db.m_auto_commit        = true;
        db.m_reconnect_attempts = 0;

        try
        {
            db.m_server_version = db.m_self().get_server_version_impl();
        }
        catch (...)
        {
            // the version is for reading later; not having it costs nothing
            db.m_server_version.clear();
        }

        return true;
    }

    template<typename      Derived,
             database_type DbType>
    template<typename Step>
    bool
    connect_steps<Derived, DbType>::m_guarded(
        Step&& _step
    )
    {
        try
        {
            return _step();
        }
        catch (const exception& _e)
        {
            m_connection->m_state = connection_state::error;
            m_connection->m_capture(_e);

            throw;
        }
        catch (...)
        {
            m_connection->m_state = connection_state::error;

            throw;
        }
    }

NS_END  // internal


//==============================================================================
// 2.  CAPABILITIES
//==============================================================================
// Whether a class has the operations that do not block, connect_async and
// the three SQL members (1.1): every class derived from database_connection
// has them, done as they start where the vendor's face has no calls that do
// not wait, so the answer is a class's, not a client's. The trait serves from
// C++17; the concept forwards to it where the compiler has concepts.

// 2.1    Detectors
//------------------------------------------------------------------------------
// 2.1.1
// connect_async_t
//   detector: connect_async() method.
template<typename Type>
using connect_async_t = decltype(std::declval<Type&>().connect_async());

// 2.1.2
// execute_async_t
//   detector: execute_async(const std::string&) method.
template<typename Type>
using execute_async_t = decltype(std::declval<Type&>().execute_async(
    std::declval<const std::string&>()));

// 2.1.3
// execute_query_async_t
//   detector: execute_query_async(const std::string&) method.
template<typename Type>
using execute_query_async_t = decltype(
    std::declval<Type&>().execute_query_async(
        std::declval<const std::string&>()));

// 2.1.4
// execute_update_async_t
//   detector: execute_update_async(const std::string&) method.
template<typename Type>
using execute_update_async_t = decltype(
    std::declval<Type&>().execute_update_async(
        std::declval<const std::string&>()));

// 2.2    Traits
//------------------------------------------------------------------------------
// 2.2.1
// has_async_operations
//   trait: true if Type has connect_async and the three SQL members that do
// not block.
template<typename Type>
struct has_async_operations
{
    static constexpr bool value =
        ( is_detected<connect_async_t, clean_t<Type>>::value        &&
          is_detected<execute_async_t, clean_t<Type>>::value        &&
          is_detected<execute_query_async_t, clean_t<Type>>::value  &&
          is_detected<execute_update_async_t, clean_t<Type>>::value );
};

// 2.3    C++20 concepts
//------------------------------------------------------------------------------
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_CONCEPTS

    // 2.3.1
    // async_connection
    //   concept: constrains connections with the operations that do not
    // block (has_async_operations).
    template<typename Type>
    concept async_connection =
        has_async_operations<Type>::value;

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

#endif  // DJINTERP_DB_DATABASE_CONNECTION_HPP
