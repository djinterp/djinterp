/******************************************************************************
* djinterp [database]                                  database_connection.hpp
* 
* djinterp database connection module:
*   This header provides a rudimentary but functional database connection
* abstraction built on the connection_template CRTP base. It augments the
* vendor-parameterized template with common operational patterns that all
* database connections share, including:
*   - connect-from-config with state tracking
*   - automatic reconnection with attempt tracking
*   - connection state machine (disconnected -> connecting -> connected)
*   - error state capture and propagation
*   - compile-time interface verification via static_assert
*   - RAII-based connection lifetime management
*   - convenience query helpers (execute_scalar, execute_single_row)
*
*   This module is the intended base class for all concrete vendor
* connection implementations. It sits between connection_template (which
* provides vendor metadata and native handle scaffolding) and the
* vendor-specific implementation class (which provides the actual
* connect/disconnect/execute logic against a specific C API).
*
*   LAYER DIAGRAM:
*     vendor_helper (e.g. mysql_connection_helper)
*       -> database_connection<vendor_helper, database_type::mysql>
*         -> connection_template<vendor_helper, database_type::mysql>
*           -> connection<vendor_helper>
*
*   PORTABILITY:
*   This header requires C++17 or later. It does not include any
* vendor-specific headers.
*
* 
* path:      /inc/djinterp/core/db/database_connection.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.25
******************************************************************************/

#ifndef DJINTERP_DATABASE_CONNECTION_
#define DJINTERP_DATABASE_CONNECTION_

// std
#include <atomic>
#include <utility>
// djinterp
#include "../djinterp.hpp"
#include "./database_common.hpp"
#include "./database_connection_template.hpp"


NS_DJINTERP
NS_DATABASE


// =============================================================================
// I.   DATABASE CONNECTION
// =============================================================================

// database_connection
//   class template: operational connection base with state management,
// reconnection tracking, and convenience query helpers. Concrete vendor
// implementations derive from this class.
//
// Template parameters:
//   _helper:   the concrete CRTP implementation class
//   _DbType: the database_type enumerator identifying the vendor
//
// The implementation class (_helper) must provide the following methods:
//   void connect_helper()         - establish a connection using m_config
//   void disconnect_helper()      - tear down the native connection
//   bool is_connected_helper()    - test if the native connection is alive
//   bool ping_helper()            - lightweight liveness check
//   std::string get_server_version_helper()
//   std::string get_last_error_helper()
//   int         get_last_error_code_helper()
//
// The _helper suffix convention avoids name collisions with the CRTP
// forwarding methods in the base classes.
template<typename      _helper,
         database_type _DbType>
class database_connection
    : public connection_template<_helper, _DbType>
{
public:
    using base_type          = connection_template<_helper, _DbType>;
    using traits_type        = typename base_type::traits_type;
    using native_handle_type = typename base_type::native_handle_type;

    database_connection()
        : base_type(),
          m_reconnect_attempts(0),
          m_max_reconnect_attempts(3),
          m_last_error_code(0)
    {}

    explicit database_connection(const connection_config& _config)
        : base_type(_config),
          m_reconnect_attempts(0),
          m_max_reconnect_attempts(3),
          m_last_error_code(0)
    {}

    ~database_connection()
    {
        // ensure clean disconnection
        if (this->m_state == connection_state::connected)
        {
            try
            {
                self().disconnect_helper();
            }
            catch (...)
            {
                // suppress exceptions in destructor
            }
        }
    }

    // disable copying
    database_connection(const database_connection&)            = delete;
    database_connection& operator=(const database_connection&) = delete;

    // enable moving
    database_connection(database_connection&& _other) noexcept
        : base_type(std::move(_other)),
          m_reconnect_attempts(_other.m_reconnect_attempts),
          m_max_reconnect_attempts(_other.m_max_reconnect_attempts),
          m_last_error_code(_other.m_last_error_code),
          m_last_error(_other.m_last_error),
          m_server_version(std::move(_other.m_server_version))
    {
        _other.m_reconnect_attempts = 0;
        _other.m_last_error_code    = 0;

        _other.m_last_error.clear();
    }

    database_connection& operator=(database_connection&& _other) noexcept
    {
        if (this != &_other)
        {
            // disconnect current connection if active
            if (this->m_state == connection_state::connected)
            {
                try
                {
                    self().disconnect_helper();
                }
                catch (...)
                {
                    // suppress
                }
            }

            base_type::operator=(std::move(_other));

            m_reconnect_attempts     = _other.m_reconnect_attempts;
            m_max_reconnect_attempts = _other.m_max_reconnect_attempts;
            m_last_error_code        = _other.m_last_error_code;
            m_last_error             = _other.m_last_error;
            m_server_version         = std::move(_other.m_server_version);

            _other.m_reconnect_attempts = 0;
            _other.m_last_error_code    = 0;
            _other.m_last_error.clear();
        }

        return *this;
    }

    // -----------------------------------------------------------------
    // connection management
    // -----------------------------------------------------------------

    // connect
    //   function: establishes a connection using the current
    // configuration. Updates connection state and captures the server
    // version on success, or captures the error on failure.
    void 
    connect()
    {
        if (this->m_state == connection_state::connected)
        {
            return;
        }

        this->m_state = connection_state::connecting;
        clear_error();

        try
        {
            self().connect_helper();

            this->m_state        = connection_state::connected;
            m_reconnect_attempts = 0;

            // cache server version on successful connection
            try
            {
                m_server_version = self().get_server_version_helper();
            }
            catch (...)
            {
                // non-fatal: version query failure does not
                // invalidate the connection
            }
        }
        catch (const connection_exception& _e)
        {
            this->m_state = connection_state::error;
            capture_error(_e.error_code(), _e.what());

            throw;
        }
        catch (const std::exception& _e)
        {
            this->m_state = connection_state::error;
            capture_error(-1, _e.what());

            throw connection_exception(_e.what());
        }
    }

    // connect (config overload)
    //   function: replaces the current configuration and connects.
    void
    connect(const connection_config& _config)
    {
        this->m_config = _config;

        // apply vendor default port if not specified
        if (this->m_config.port == 0)
        {
            this->m_config.port = traits_type::default_port;
        }

        connect();
    }

    // disconnect
    //   function: tears down the current connection and resets state.
    void
    disconnect()
    {
        if ( (this->m_state == connection_state::disconnected) ||
             (this->m_state == connection_state::closed) )
        {
            return;
        }

        try
        {
            self().disconnect_helper();
        }
        catch (const std::exception& _e)
        {
            capture_error(-1, _e.what());
        }

        this->m_state        = connection_state::disconnected;
        this->m_in_transaction = false;
    }

    // reconnect
    //   function: disconnects (if connected) and re-establishes the
    // connection using the current configuration. Tracks reconnection
    // attempts and throws if the maximum is exceeded.
    void
    reconnect()
    {
        if (m_reconnect_attempts >= m_max_reconnect_attempts)
        {
            throw connection_exception(
                "maximum reconnection attempts exceeded ("
                + std::to_string(m_max_reconnect_attempts) + ")");
        }

        ++m_reconnect_attempts;

        // disconnect if currently connected
        if ( (this->m_state == connection_state::connected) ||
             (this->m_state == connection_state::error) )
        {
            try
            {
                self().disconnect_helper();
            }
            catch (...)
            {
                // suppress: we are reconnecting anyway
            }

            this->m_state = connection_state::disconnected;
        }

        connect();
    }

    // is_connected
    //   function: tests whether the connection is alive by delegating
    // to the implementation's is_connected_helper method.
    bool
    is_connected() const noexcept
    {
        if (this->m_state != connection_state::connected)
        {
            return false;
        }

        try
        {
            return self().is_connected_helper();
        }
        catch (...)
        {
            return false;
        }
    }

    // ping
    //   function: performs a lightweight liveness check against the
    // server.
    bool
    ping() const
    {
        if (this->m_state != connection_state::connected)
        {
            return false;
        }

        try
        {
            return self().ping_helper();
        }
        catch (...)
        {
            return false;
        }
    }

    // -----------------------------------------------------------------
    // query execution wrappers
    // -----------------------------------------------------------------

    // execute_query
    //   function: executes a query and returns a result set. Updates
    // the connection state to executing for the duration.
    auto 
    execute_query(
        const std::string& _query
    )
    {
        ensure_connected();

        this->m_state = connection_state::executing;

        try
        {
            auto result = self().execute_query_helper(_query);

            this->m_state = connection_state::connected;

            return result;
        }
        catch (const query_exception& _e)
        {
            this->m_state = connection_state::connected;
            capture_error(_e.error_code(), _e.what());

            throw;
        }
        catch (const std::exception& _e)
        {
            this->m_state = connection_state::connected;
            capture_error(-1, _e.what());

            throw query_exception(_e.what());
        }
    }

    // execute_update
    //   function: executes an update/insert/delete statement and
    // returns the number of affected rows.
    std::int64_t
    execute_update(
        const std::string& _query
    )
    {
        ensure_connected();

        this->m_state = connection_state::executing;

        try
        {
            std::int64_t rows = self().execute_update_helper(_query);

            this->m_state = connection_state::connected;

            return rows;
        }
        catch (const query_exception& _e)
        {
            this->m_state = connection_state::connected;
            capture_error(_e.error_code(), _e.what());

            throw;
        }
        catch (const std::exception& _e)
        {
            this->m_state = connection_state::connected;
            capture_error(-1, _e.what());

            throw query_exception(_e.what());
        }
    }

    // execute
    //   function: executes a statement that does not return a result
    // set (DDL, DCL, etc.). Returns true on success.
    bool execute(
        const std::string& _query
    )
    {
        ensure_connected();

        this->m_state = connection_state::executing;

        try
        {
            bool result = self().execute_helper(_query);

            this->m_state = connection_state::connected;

            return result;
        }
        catch (const std::exception& _e)
        {
            this->m_state = connection_state::connected;
            capture_error(-1, _e.what());

            throw query_exception(_e.what());
        }
    }

    // -----------------------------------------------------------------
    // metadata
    // -----------------------------------------------------------------

    // get_server_version
    //   function: returns the cached server version string, or queries
    // the server if not yet cached.
    std::string 
    get_server_version() const
    {
        if (!m_server_version.empty())
        {
            return m_server_version;
        }

        if (this->m_state == connection_state::connected)
        {
            return self().get_server_version_helper();
        }

        return "";
    }

    // get_database_name
    //   function: returns the database name from the current config.
    std::string
    get_database_name() const
    {
        return this->m_config.database;
    }

    // -----------------------------------------------------------------
    // error handling
    // -----------------------------------------------------------------

    // get_last_error
    //   function: returns the last error message.
    std::string
    get_last_error() const
    {
        return m_last_error;
    }

    // get_last_error_code
    //   function: returns the last vendor error code.
    int
    get_last_error_code() const noexcept
    {
        return m_last_error_code;
    }

    // has_error
    //   function: returns true if an error has been captured.
    bool
    has_error() const noexcept
    {
        return (m_last_error_code != 0) ||
               (!m_last_error.empty());
    }

    // clear_error
    //   function: clears the captured error state.
    void
    clear_error() noexcept
    {
        m_last_error_code = 0;
        m_last_error.clear();
    }

    // -----------------------------------------------------------------
    // reconnection configuration
    // -----------------------------------------------------------------

    // get_max_reconnect_attempts
    //   function: returns the maximum number of reconnection attempts.
    std::size_t
    get_max_reconnect_attempts() const noexcept
    {
        return m_max_reconnect_attempts;
    }

    // set_max_reconnect_attempts
    //   function: sets the maximum number of reconnection attempts.
    void
    set_max_reconnect_attempts(std::size_t _max) noexcept
    {
        m_max_reconnect_attempts = _max;
    }

    // get_reconnect_attempts
    //   function: returns the number of reconnection attempts since the
    // last successful connection.
    std::size_t
    get_reconnect_attempts() const noexcept
    {
        return m_reconnect_attempts;
    }

    // reset_reconnect_counter
    //   function: resets the reconnection attempt counter.
    void reset_reconnect_counter() noexcept
    {
        m_reconnect_attempts = 0;
    }

    // -----------------------------------------------------------------
    // convenience helpers
    // -----------------------------------------------------------------

    // execute_scalar
    //   function: executes a query and returns the first column of the
    // first row as a value. Returns std::nullopt if no rows are
    // returned.
    std::optional<value>
    execute_scalar(const std::string& _query)
    {
        auto rs = execute_query(_query);

        if ( (rs) &&
             (rs->next()) )
        {
            return rs->get_value(static_cast<std::size_t>(0));
        }

        return std::nullopt;
    }

    // execute_single_row
    //   function: executes a query and returns the first row as a
    // map. Returns std::nullopt if no rows are returned.
    std::optional<row>
    execute_single_row(const std::string& _query)
    {
        auto rs = execute_query(_query);

        if ( (rs) &&
             (rs->next()) )
        {
            return rs->get_current_row();
        }

        return std::nullopt;
    }

protected:
    // ensure_connected
    //   function: throws if the connection is not in a connected state.
    void
    ensure_connected() const
    {
        if (this->m_state != connection_state::connected)
        {
            throw connection_exception(
                "not connected to " + std::string(traits_type::display_name)
            );
        }
    }

    // capture_error
    //   function: captures a vendor error code and message.
    void 
    capture_error(
        int                _code,
        const std::string& _message
    )
    {
        m_last_error_code = _code;
        m_last_error      = _message;
    }

    // capture_error (const char* overload)
    //   function: captures a vendor error code and message.
    void capture_error
    (
        int         _code,
        const char* _message)
    {
        m_last_error_code = _code;
        m_last_error      = _message ? _message : "";
    }

    std::size_t m_reconnect_attempts;
    std::size_t m_max_reconnect_attempts;
    int         m_last_error_code;
    std::string m_last_error;
    std::string m_server_version;

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


NS_END  // database
NS_END  // djinterp


#endif  // DJINTERP_DATABASE_CONNECTION_
