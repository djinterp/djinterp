/*******************************************************************************
* djinterp [core]                                                    mariadb.cpp
*
*
* path:      /src/djinterp/core/db/mariadb/mariadb.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.02
*******************************************************************************/
// definitions for mariadb.hpp. see the header for documentation of
// the mariadb_connection interface, CRTP layer diagram, and the data
// type / feature support infrastructure.
#include "../../../../../inc/djinterp/core/db/mariadb/mariadb.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the header's floor: below it, nothing to define


#if D_ENV_MARIADB_DETECTED
    // MariaDB C API. The MariaDB Connector/C installs its headers
    // under <mariadb/mysql.h>, but build systems commonly alias this
    // to <mysql.h> for cross-compatibility with Oracle MySQL.
    // mariadb
    #include <mysql.h>
#endif

#include <cstring>
#include <sstream>
// re_std
#include "../../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP

// ===========================================================================
// I.   VENDOR HELPER TYPES
// ===========================================================================
// Minimal CRTP leaf definitions for the forward-declared helper structs
// in mariadb.hpp. These own the native MYSQL_RES* / MYSQL_STMT*
// handles and are constructed by execute_query_helper / prepare_helper.
// The full navigation / bind / field-access implementations would
// live in mariadb_result_set.cpp and mariadb_statement.cpp; only the
// RAII lifetime management is provided here so that the unique_ptrs
// returned below can be constructed and destroyed safely.

// mariadb_result_set_helper
//   struct: CRTP leaf for MariaDB result sets. Owns a MYSQL_RES*
// native handle and frees it on destruction. The navigation /
// get_value / column_* methods required by result_set<_helper> live
// in a companion translation unit.
struct mariadb_result_set_helper
    : public result_set<mariadb_result_set_helper>
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*     m_owner;          // non-owning back-pointer to the connection's MYSQL*
    MYSQL_RES* m_native_result;  // owning pointer to the result buffer
#else
    void*      m_owner;
    void*      m_native_result;
#endif

    mariadb_result_set_helper()
        : m_owner(nullptr),
          m_native_result(nullptr)
    {}

#if D_ENV_MARIADB_DETECTED
    mariadb_result_set_helper(
        MYSQL*     _owner,
        MYSQL_RES* _native_result
    ) noexcept
        : m_owner(_owner),
          m_native_result(_native_result)
    {}
#endif

    ~mariadb_result_set_helper()
    {
#if D_ENV_MARIADB_DETECTED
        if (m_native_result)
        {
            mysql_free_result(m_native_result);

            m_native_result = nullptr;
        }
#endif
    }

    mariadb_result_set_helper(const mariadb_result_set_helper&)            = delete;
    mariadb_result_set_helper& operator=(const mariadb_result_set_helper&) = delete;

    mariadb_result_set_helper(
        mariadb_result_set_helper&& _other
    ) noexcept
        : m_owner(_other.m_owner),
          m_native_result(_other.m_native_result)
    {
        _other.m_owner         = nullptr;
        _other.m_native_result = nullptr;
    }

    mariadb_result_set_helper&
    operator=(mariadb_result_set_helper&& _other) noexcept
    {
        if (this != &_other)
        {
#if D_ENV_MARIADB_DETECTED
            if (m_native_result)
            {
                mysql_free_result(m_native_result);
            }
#endif

            m_owner         = _other.m_owner;
            m_native_result = _other.m_native_result;

            _other.m_owner         = nullptr;
            _other.m_native_result = nullptr;
        }

        return *this;
    }
};

// mariadb_statement_helper
//   struct: CRTP leaf for MariaDB prepared statements. Owns a
// MYSQL_STMT* native handle and frees it on destruction. The bind /
// execute methods required by statement<_helper> live in a companion
// translation unit.
struct mariadb_statement_helper
    : public statement<mariadb_statement_helper>
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*      m_owner;          // non-owning back-pointer to the MYSQL*
    MYSQL_STMT* m_native_stmt;    // owning pointer to the prepared statement
#else
    void*       m_owner;
    void*       m_native_stmt;
#endif

    mariadb_statement_helper()
        : m_owner(nullptr),
          m_native_stmt(nullptr)
    {}

#if D_ENV_MARIADB_DETECTED
    mariadb_statement_helper(
        MYSQL*      _owner,
        MYSQL_STMT* _native_stmt
    ) noexcept
        : m_owner(_owner),
          m_native_stmt(_native_stmt)
    {}
#endif

    ~mariadb_statement_helper()
    {
#if D_ENV_MARIADB_DETECTED
        if (m_native_stmt)
        {
            mysql_stmt_close(m_native_stmt);

            m_native_stmt = nullptr;
        }
#endif
    }

    mariadb_statement_helper(const mariadb_statement_helper&)            = delete;
    mariadb_statement_helper& operator=(const mariadb_statement_helper&) = delete;

    mariadb_statement_helper(
        mariadb_statement_helper&& _other
    ) noexcept
        : m_owner(_other.m_owner),
          m_native_stmt(_other.m_native_stmt)
    {
        _other.m_owner       = nullptr;
        _other.m_native_stmt = nullptr;
    }

    mariadb_statement_helper&
    operator=(mariadb_statement_helper&& _other) noexcept
    {
        if (this != &_other)
        {
#if D_ENV_MARIADB_DETECTED
            if (m_native_stmt)
            {
                mysql_stmt_close(m_native_stmt);
            }
#endif

            m_owner       = _other.m_owner;
            m_native_stmt = _other.m_native_stmt;

            _other.m_owner       = nullptr;
            _other.m_native_stmt = nullptr;
        }

        return *this;
    }
};


// ===========================================================================
// II.  INTERNAL UTILITIES
// ===========================================================================

NS_INTERNAL

#if D_ENV_MARIADB_DETECTED

    // as_mysql
    //   function: reinterprets the base class's void* native handle as
    // a MYSQL*. The handle is stored as void* in connection_template
    // for header-isolation reasons.
    inline MYSQL*
    as_mysql(
        void* _handle
    ) noexcept
    {
        return static_cast<MYSQL*>(_handle);
    }

    // build_error_message
    //   function: assembles a "<prefix>: <mysql_error> (errno=<n>)"
    // string for exception payloads.
    inline std::string
    build_error_message(
        const char* _prefix,
        MYSQL*      _handle
    )
    {
        std::ostringstream oss;

        oss << _prefix
            << ": "
            << (_handle ? mysql_error(_handle) : "(null handle)")
            << " (errno="
            << (_handle ? mysql_errno(_handle) : 0u)
            << ")";

        return oss.str();
    }

#endif  // D_ENV_MARIADB_DETECTED

NS_END  // internal


// ===========================================================================
// III. MARIADB_CONNECTION :: CONNECTION MANAGEMENT
// ===========================================================================

/*
mariadb_connection::connect_helper
  Opens a new MariaDB server connection using m_config and
m_mariadb_config. Allocates a fresh MYSQL* via mysql_init, applies
timeouts, charset, SSL, and storage-engine init options from the
configuration, then calls mysql_real_connect to complete the
handshake. On success the native handle is stored in m_native_handle;
on failure a connection_exception is thrown and any partially
allocated handle is freed.

Parameter(s):
  none.
Return:
  none.
*/
void
mariadb_connection::connect_helper()
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*        handle;
    unsigned int  connect_timeout_secs;
    unsigned int  read_timeout_secs;
    unsigned int  write_timeout_secs;
    my_bool       reconnect_flag;
    const char*   unix_socket;
    unsigned long client_flag;

    // allocate a fresh MYSQL handle
    handle = mysql_init(nullptr);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::connect_helper: mysql_init "
            "returned null (out of memory)");
    }

    // apply timeouts (connection_config stores milliseconds;
    // mysql_options takes seconds)
    connect_timeout_secs = static_cast<unsigned int>(
        this->m_config.connect_timeout.count() / 1000);
    read_timeout_secs = static_cast<unsigned int>(
        this->m_config.read_timeout.count() / 1000);
    write_timeout_secs = static_cast<unsigned int>(
        this->m_config.write_timeout.count() / 1000);

    if (connect_timeout_secs > 0)
    {
        mysql_options(handle,
                      MYSQL_OPT_CONNECT_TIMEOUT,
                      &connect_timeout_secs);
    }

    if (read_timeout_secs > 0)
    {
        mysql_options(handle,
                      MYSQL_OPT_READ_TIMEOUT,
                      &read_timeout_secs);
    }

    if (write_timeout_secs > 0)
    {
        mysql_options(handle,
                      MYSQL_OPT_WRITE_TIMEOUT,
                      &write_timeout_secs);
    }

    // auto-reconnect
    reconnect_flag = this->m_config.auto_reconnect ? 1 : 0;
    mysql_options(handle,
                  MYSQL_OPT_RECONNECT,
                  &reconnect_flag);

    // charset: prefer explicit connection_config.charset, fall back
    // to the MariaDB default_charset in the mysql_connect_config
    if (this->m_config.charset.has_value())
    {
        mysql_options(handle,
                      MYSQL_SET_CHARSET_NAME,
                      this->m_config.charset->c_str());
    }
    else if (!m_mariadb_config.mysql_config.default_charset.empty())
    {
        mysql_options(
            handle,
            MYSQL_SET_CHARSET_NAME,
            m_mariadb_config.mysql_config.default_charset.c_str());
    }

    // SSL
    if (this->m_config.enable_ssl)
    {
        mysql_ssl_set(
            handle,
            this->m_config.ssl_key.has_value()
                ? this->m_config.ssl_key->c_str()  : nullptr,
            this->m_config.ssl_cert.has_value()
                ? this->m_config.ssl_cert->c_str() : nullptr,
            this->m_config.ssl_ca.has_value()
                ? this->m_config.ssl_ca->c_str()   : nullptr,
            nullptr,
            nullptr);
    }

    // initial-command: set the default storage engine if requested
    if (!m_mariadb_config.default_storage_engine.empty())
    {
        std::string init_cmd;

        init_cmd  = "SET default_storage_engine=";
        init_cmd += m_mariadb_config.default_storage_engine;

        mysql_options(handle,
                      MYSQL_INIT_COMMAND,
                      init_cmd.c_str());
    }

    // unix_socket is not exposed directly in connection_config; look
    // it up in custom_options for consistency with peer vendors
    unix_socket = nullptr;

    {
        auto it = this->m_config.custom_options.find("unix_socket");

        if (it != this->m_config.custom_options.end())
        {
            unix_socket = it->second.c_str();
        }
    }

    // client_flag: enable multi-statements to support batched DDL
    client_flag = CLIENT_MULTI_STATEMENTS;

    // perform the real connection handshake
    if (!mysql_real_connect(
            handle,
            this->m_config.host.c_str(),
            this->m_config.username.c_str(),
            this->m_config.password.c_str(),
            this->m_config.database.empty()
                ? nullptr
                : this->m_config.database.c_str(),
            static_cast<unsigned int>(this->m_config.port),
            unix_socket,
            client_flag))
    {
        std::string msg =
            internal::build_error_message(
                "mariadb_connection::connect_helper: "
                "mysql_real_connect failed",
                handle);

        mysql_close(handle);

        throw connection_exception(msg);
    }

    // store the now-owned handle in the base class's native-handle slot
    this->m_native_handle = static_cast<void*>(handle);

    // Galera-aware session variables: apply after successful connect
    if (m_mariadb_config.galera_wsrep_sync_wait)
    {
        mysql_query(handle, "SET SESSION wsrep_sync_wait=7");
    }

    if (m_mariadb_config.galera_wsrep_causal_reads)
    {
        mysql_query(handle, "SET SESSION wsrep_causal_reads=ON");
    }

    return;
#else
    throw connection_exception(
        "mariadb_connection::connect_helper: "
        "MariaDB C API not detected at build time "
        "(D_ENV_MARIADB_DETECTED=0)");
#endif  // D_ENV_MARIADB_DETECTED
}

/*
mariadb_connection::disconnect_helper
  Closes the MYSQL* native handle previously allocated by
connect_helper and clears m_native_handle. Safe to call when no
handle is currently held (becomes a no-op).

Parameter(s):
  none.
Return:
  none.
*/
void
mariadb_connection::disconnect_helper()
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (handle)
    {
        mysql_close(handle);

        this->m_native_handle = nullptr;
    }

    return;
#else
    return;
#endif
}

/*
mariadb_connection::is_connected_helper
  Tests whether the native handle is live. Returns true only when
both the handle is non-null and mysql_ping succeeds.

Parameter(s):
  none.
Return:
  true  if the connection is alive,
  false otherwise.
*/
bool
mariadb_connection::is_connected_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    // mysql_ping expects a non-const handle; the const_cast is safe
    // because the handle itself is mutable, only the pointer slot is const
    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return false;
    }

    return (mysql_ping(handle) == 0);
#else
    return false;
#endif
}

/*
mariadb_connection::ping_helper
  Performs a lightweight server liveness check via mysql_ping.

Parameter(s):
  none.
Return:
  true  if the server responded,
  false if the handle is null or mysql_ping reported an error.
*/
bool
mariadb_connection::ping_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return false;
    }

    return (mysql_ping(handle) == 0);
#else
    return false;
#endif
}


// ===========================================================================
// IV.  MARIADB_CONNECTION :: QUERY EXECUTION
// ===========================================================================

/*
mariadb_connection::execute_query_helper
  Executes a SQL query expected to return a result set and wraps the
returned MYSQL_RES* in a mariadb_result_set_helper RAII owner.
Throws query_exception on mysql_query failure or when the query
returns no result metadata.

Parameter(s):
  _query: the SQL query text (UTF-8, per the connection charset).
Return:
  A unique_ptr owning the populated result set.
*/
auto
mariadb_connection::execute_query_helper(
    const std::string& _query
)
    -> std::unique_ptr<result_set<struct mariadb_result_set_helper>>
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*     handle;
    MYSQL_RES* native_result;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw query_exception(
            "mariadb_connection::execute_query_helper: "
            "no active connection");
    }

    if (mysql_real_query(handle,
                         _query.data(),
                         static_cast<unsigned long>(_query.size())) != 0)
    {
        throw query_exception(
            internal::build_error_message(
                "mariadb_connection::execute_query_helper: "
                "mysql_real_query failed",
                handle));
    }

    // buffer the full result set client-side; for streaming use a
    // different helper that calls mysql_use_result
    native_result = mysql_store_result(handle);

    if (!native_result)
    {
        // zero-column statement (e.g. an UPDATE masquerading as
        // execute_query) or an error
        if (mysql_field_count(handle) != 0)
        {
            throw query_exception(
                internal::build_error_message(
                    "mariadb_connection::execute_query_helper: "
                    "mysql_store_result failed",
                    handle));
        }
    }

    return std::unique_ptr<result_set<mariadb_result_set_helper>>(
        new mariadb_result_set_helper(handle, native_result));
#else
    (void)_query;

    throw query_exception(
        "mariadb_connection::execute_query_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::execute_update_helper
  Executes a SQL statement that modifies rows (INSERT, UPDATE,
DELETE) and returns the number of affected rows.

Parameter(s):
  _query: the SQL statement text.
Return:
  The number of rows affected by the statement (>= 0), or throws
  query_exception on failure.
*/
re_std::int64_t
mariadb_connection::execute_update_helper(
    const std::string& _query
)
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*            handle;
    my_ulonglong      affected;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw query_exception(
            "mariadb_connection::execute_update_helper: "
            "no active connection");
    }

    if (mysql_real_query(handle,
                         _query.data(),
                         static_cast<unsigned long>(_query.size())) != 0)
    {
        throw query_exception(
            internal::build_error_message(
                "mariadb_connection::execute_update_helper: "
                "mysql_real_query failed",
                handle));
    }

    affected = mysql_affected_rows(handle);

    // mysql_affected_rows returns (my_ulonglong)-1 on error
    if (affected == static_cast<my_ulonglong>(-1))
    {
        throw query_exception(
            internal::build_error_message(
                "mariadb_connection::execute_update_helper: "
                "mysql_affected_rows reported error",
                handle));
    }

    return static_cast<re_std::int64_t>(affected);
#else
    (void)_query;

    throw query_exception(
        "mariadb_connection::execute_update_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::execute_helper
  Executes a SQL statement that returns neither a result set nor an
affected-rows count (DDL, DCL, etc.).

Parameter(s):
  _query: the SQL statement text.
Return:
  true on success; throws query_exception on failure.
*/
bool
mariadb_connection::execute_helper(
    const std::string& _query
)
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw query_exception(
            "mariadb_connection::execute_helper: "
            "no active connection");
    }

    if (mysql_real_query(handle,
                         _query.data(),
                         static_cast<unsigned long>(_query.size())) != 0)
    {
        throw query_exception(
            internal::build_error_message(
                "mariadb_connection::execute_helper: "
                "mysql_real_query failed",
                handle));
    }

    // drain any result set produced by the statement so the
    // connection is left in a clean state for the next query
    {
        MYSQL_RES* maybe_result;

        maybe_result = mysql_store_result(handle);

        if (maybe_result)
        {
            mysql_free_result(maybe_result);
        }
    }

    return true;
#else
    (void)_query;

    throw query_exception(
        "mariadb_connection::execute_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::prepare_helper
  Prepares a SQL statement for parameterized execution and wraps the
returned MYSQL_STMT* in a mariadb_statement_helper RAII owner.

Parameter(s):
  _query: the SQL statement text, containing '?' placeholders for
          bind parameters.
Return:
  A unique_ptr owning the prepared statement handle.
*/
auto
mariadb_connection::prepare_helper(
    const std::string& _query
)
    -> std::unique_ptr<statement<struct mariadb_statement_helper>>
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*      handle;
    MYSQL_STMT* stmt;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw query_exception(
            "mariadb_connection::prepare_helper: "
            "no active connection");
    }

    stmt = mysql_stmt_init(handle);

    if (!stmt)
    {
        throw query_exception(
            internal::build_error_message(
                "mariadb_connection::prepare_helper: "
                "mysql_stmt_init failed",
                handle));
    }

    if (mysql_stmt_prepare(
            stmt,
            _query.data(),
            static_cast<unsigned long>(_query.size())) != 0)
    {
        std::string msg;

        msg  = "mariadb_connection::prepare_helper: "
               "mysql_stmt_prepare failed: ";
        msg += mysql_stmt_error(stmt);

        mysql_stmt_close(stmt);

        throw query_exception(msg);
    }

    return std::unique_ptr<statement<mariadb_statement_helper>>(
        new mariadb_statement_helper(handle, stmt));
#else
    (void)_query;

    throw query_exception(
        "mariadb_connection::prepare_helper: "
        "MariaDB C API not detected at build time");
#endif
}


// ===========================================================================
// V.   MARIADB_CONNECTION :: METADATA & ERROR REPORTING
// ===========================================================================

/*
mariadb_connection::get_server_version_helper
  Retrieves the MariaDB server version string from the active
connection. Returns an empty string if no connection is active.

Parameter(s):
  none.
Return:
  The server version string (e.g. "10.11.5-MariaDB").
*/
std::string
mariadb_connection::get_server_version_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*      handle;
    const char* info;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return "";
    }

    info = mysql_get_server_info(handle);

    return info ? std::string(info) : std::string();
#else
    return "";
#endif
}

/*
mariadb_connection::get_last_error_helper
  Returns the last error message associated with the MYSQL* handle.

Parameter(s):
  none.
Return:
  The last error message, or an empty string if no connection or no
  error is present.
*/
std::string
mariadb_connection::get_last_error_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*      handle;
    const char* err;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return "";
    }

    err = mysql_error(handle);

    return err ? std::string(err) : std::string();
#else
    return "";
#endif
}

/*
mariadb_connection::get_last_error_code_helper
  Returns the last vendor error number associated with the MYSQL*
handle.

Parameter(s):
  none.
Return:
  The mysql_errno value, or 0 if no connection or no error is present.
*/
int
mariadb_connection::get_last_error_code_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return 0;
    }

    return static_cast<int>(mysql_errno(handle));
#else
    return 0;
#endif
}

/*
mariadb_connection::get_last_insert_id_helper
  Returns the AUTO_INCREMENT value generated by the most recent
INSERT on this connection.

Parameter(s):
  none.
Return:
  The last insert id, or 0 if no connection or no INSERT has
  occurred.
*/
re_std::int64_t
mariadb_connection::get_last_insert_id_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return 0;
    }

    return static_cast<re_std::int64_t>(mysql_insert_id(handle));
#else
    return 0;
#endif
}

/*
mariadb_connection::get_affected_rows_helper
  Returns the number of rows changed by the most recent row-mutating
statement.

Parameter(s):
  none.
Return:
  The affected row count, or -1 if the count is not available.
*/
re_std::int64_t
mariadb_connection::get_affected_rows_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*       handle;
    my_ulonglong affected;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return -1;
    }

    affected = mysql_affected_rows(handle);

    if (affected == static_cast<my_ulonglong>(-1))
    {
        return -1;
    }

    return static_cast<re_std::int64_t>(affected);
#else
    return -1;
#endif
}


// ===========================================================================
// VI.  MARIADB_CONNECTION :: MYSQL-FAMILY COMMON HELPERS
// ===========================================================================

/*
mariadb_connection::set_charset_helper
  Sets the client connection character set.

Parameter(s):
  _charset: the charset name (e.g. "utf8mb4").
Return:
  none.
*/
void
mariadb_connection::set_charset_helper(
    const std::string& _charset
)
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::set_charset_helper: "
            "no active connection");
    }

    if (mysql_set_character_set(handle, _charset.c_str()) != 0)
    {
        throw connection_exception(
            internal::build_error_message(
                "mariadb_connection::set_charset_helper: "
                "mysql_set_character_set failed",
                handle));
    }

    return;
#else
    (void)_charset;

    throw connection_exception(
        "mariadb_connection::set_charset_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::get_charset_helper
  Returns the current client character set.

Parameter(s):
  none.
Return:
  The charset name, or an empty string if no connection is active.
*/
std::string
mariadb_connection::get_charset_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*      handle;
    const char* cs;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return "";
    }

    cs = mysql_character_set_name(handle);

    return cs ? std::string(cs) : std::string();
#else
    return "";
#endif
}

/*
mariadb_connection::next_result_helper
  Advances to the next result set when processing a multi-statement
query.

Parameter(s):
  none.
Return:
  0 on success with another result present,
  -1 when there are no more results,
  >0 on error (the raw mysql_next_result return).
*/
int
mariadb_connection::next_result_helper()
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw query_exception(
            "mariadb_connection::next_result_helper: "
            "no active connection");
    }

    return mysql_next_result(handle);
#else
    throw query_exception(
        "mariadb_connection::next_result_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::more_results_helper
  Reports whether more result sets are available after the current
one.

Parameter(s):
  none.
Return:
  true if additional result sets exist, false otherwise.
*/
bool
mariadb_connection::more_results_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return false;
    }

    return (mysql_more_results(handle) != 0);
#else
    return false;
#endif
}

/*
mariadb_connection::escape_string_helper
  Escapes a string for safe inclusion in a SQL literal, honoring the
current connection charset.

Parameter(s):
  _input: the string to escape.
Return:
  The escaped string.
*/
std::string
mariadb_connection::escape_string_helper(
    const std::string& _input
) const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*        handle;
    std::string   out;
    unsigned long out_len;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::escape_string_helper: "
            "no active connection");
    }

    // mysql_real_escape_string can produce up to 2*N + 1 bytes
    out.resize(_input.size() * 2 + 1);

    out_len = mysql_real_escape_string(
        handle,
        out.data(),
        _input.data(),
        static_cast<unsigned long>(_input.size()));

    out.resize(out_len);

    return out;
#else
    (void)_input;

    throw connection_exception(
        "mariadb_connection::escape_string_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::select_db_helper
  Switches the default database for the active connection.

Parameter(s):
  _database: the database to select.
Return:
  none.
*/
void
mariadb_connection::select_db_helper(
    const std::string& _database
)
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::select_db_helper: "
            "no active connection");
    }

    if (mysql_select_db(handle, _database.c_str()) != 0)
    {
        throw connection_exception(
            internal::build_error_message(
                "mariadb_connection::select_db_helper: "
                "mysql_select_db failed",
                handle));
    }

    // keep the base-class config in sync
    this->m_config.database = _database;

    return;
#else
    (void)_database;

    throw connection_exception(
        "mariadb_connection::select_db_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::change_user_helper
  Re-authenticates the active connection as a different user without
reopening the socket.

Parameter(s):
  _user:     the new username.
  _password: the corresponding password.
  _database: the default database after the switch.
Return:
  none.
*/
void
mariadb_connection::change_user_helper(
    const std::string& _user,
    const std::string& _password,
    const std::string& _database
)
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::change_user_helper: "
            "no active connection");
    }

    if (mysql_change_user(
            handle,
            _user.c_str(),
            _password.c_str(),
            _database.empty() ? nullptr : _database.c_str()) != 0)
    {
        throw connection_exception(
            internal::build_error_message(
                "mariadb_connection::change_user_helper: "
                "mysql_change_user failed",
                handle));
    }

    // keep the base-class config in sync
    this->m_config.username = _user;
    this->m_config.password = _password;
    this->m_config.database = _database;

    return;
#else
    (void)_user;
    (void)_password;
    (void)_database;

    throw connection_exception(
        "mariadb_connection::change_user_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::get_stat_helper
  Returns the server status string produced by mysql_stat
(uptime, threads, queries, etc.).

Parameter(s):
  none.
Return:
  The status string, or an empty string if no connection is active.
*/
std::string
mariadb_connection::get_stat_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*      handle;
    const char* stat;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return "";
    }

    stat = mysql_stat(handle);

    return stat ? std::string(stat) : std::string();
#else
    return "";
#endif
}

/*
mariadb_connection::get_thread_id_helper
  Returns the server-side thread (connection) id for this connection.

Parameter(s):
  none.
Return:
  The thread id, or 0 if no connection is active.
*/
unsigned long
mariadb_connection::get_thread_id_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return 0;
    }

    return mysql_thread_id(handle);
#else
    return 0;
#endif
}

/*
mariadb_connection::get_warning_count_helper
  Returns the number of warnings produced by the most recent
statement.

Parameter(s):
  none.
Return:
  The warning count, or 0 if no connection is active.
*/
unsigned int
mariadb_connection::get_warning_count_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return 0;
    }

    return mysql_warning_count(handle);
#else
    return 0;
#endif
}

/*
mariadb_connection::get_sqlstate_helper
  Returns the five-character SQLSTATE code associated with the most
recent error.

Parameter(s):
  none.
Return:
  The SQLSTATE string, or the standard "00000" on success.
*/
std::string
mariadb_connection::get_sqlstate_helper() const
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*      handle;
    const char* state;

    handle = internal::as_mysql(
        const_cast<void*>(this->m_native_handle));

    if (!handle)
    {
        return "";
    }

    state = mysql_sqlstate(handle);

    return state ? std::string(state) : std::string();
#else
    return "";
#endif
}

/*
mariadb_connection::set_autocommit_helper
  Enables or disables implicit autocommit on the connection.

Parameter(s):
  _enabled: true to enable autocommit, false to disable.
Return:
  none.
*/
void
mariadb_connection::set_autocommit_helper(
    bool _enabled
)
{
#if D_ENV_MARIADB_DETECTED
    MYSQL*  handle;
    my_bool flag;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::set_autocommit_helper: "
            "no active connection");
    }

    flag = _enabled ? 1 : 0;

    if (mysql_autocommit(handle, flag) != 0)
    {
        throw connection_exception(
            internal::build_error_message(
                "mariadb_connection::set_autocommit_helper: "
                "mysql_autocommit failed",
                handle));
    }

    return;
#else
    (void)_enabled;

    throw connection_exception(
        "mariadb_connection::set_autocommit_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::set_option_helper
  Forwards a raw option to mysql_options. The _option argument is
cast to the enum mysql_option type; this allows callers to set
options whose enumerators are not known to djinterp's abstraction.

Parameter(s):
  _option: the option enumerator value (cast to int).
  _value:  pointer to the option-specific payload.
Return:
  none.
*/
void
mariadb_connection::set_option_helper(
    int         _option,
    const void* _value
)
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::set_option_helper: "
            "no active connection");
    }

    if (mysql_options(handle,
                      static_cast<enum mysql_option>(_option),
                      _value) != 0)
    {
        throw connection_exception(
            internal::build_error_message(
                "mariadb_connection::set_option_helper: "
                "mysql_options failed",
                handle));
    }

    return;
#else
    (void)_option;
    (void)_value;

    throw connection_exception(
        "mariadb_connection::set_option_helper: "
        "MariaDB C API not detected at build time");
#endif
}


// ===========================================================================
// VII. MARIADB_CONNECTION :: TRANSACTION HELPERS
// ===========================================================================

/*
mariadb_connection::begin_transaction_helper
  Starts a new explicit transaction on the active connection by
issuing a "START TRANSACTION" statement. This leaves autocommit in
its current state but ensures a transaction boundary exists until
commit/rollback is called.

Parameter(s):
  none.
Return:
  none.
*/
void
mariadb_connection::begin_transaction_helper()
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw transaction_exception(
            "mariadb_connection::begin_transaction_helper: "
            "no active connection");
    }

    if (mysql_real_query(handle, "START TRANSACTION", 17) != 0)
    {
        throw transaction_exception(
            internal::build_error_message(
                "mariadb_connection::begin_transaction_helper: "
                "START TRANSACTION failed",
                handle));
    }

    return;
#else
    throw transaction_exception(
        "mariadb_connection::begin_transaction_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::commit_helper
  Commits the currently-open transaction via mysql_commit.

Parameter(s):
  none.
Return:
  none.
*/
void
mariadb_connection::commit_helper()
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw transaction_exception(
            "mariadb_connection::commit_helper: "
            "no active connection");
    }

    if (mysql_commit(handle) != 0)
    {
        throw transaction_exception(
            internal::build_error_message(
                "mariadb_connection::commit_helper: "
                "mysql_commit failed",
                handle));
    }

    return;
#else
    throw transaction_exception(
        "mariadb_connection::commit_helper: "
        "MariaDB C API not detected at build time");
#endif
}

/*
mariadb_connection::rollback_helper
  Rolls back the currently-open transaction via mysql_rollback.

Parameter(s):
  none.
Return:
  none.
*/
void
mariadb_connection::rollback_helper()
{
#if D_ENV_MARIADB_DETECTED
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw transaction_exception(
            "mariadb_connection::rollback_helper: "
            "no active connection");
    }

    if (mysql_rollback(handle) != 0)
    {
        throw transaction_exception(
            internal::build_error_message(
                "mariadb_connection::rollback_helper: "
                "mysql_rollback failed",
                handle));
    }

    return;
#else
    throw transaction_exception(
        "mariadb_connection::rollback_helper: "
        "MariaDB C API not detected at build time");
#endif
}


// ===========================================================================
// VIII. MARIADB_CONNECTION :: VERSION-GATED FEATURES
// ===========================================================================

#if D_ENV_MARIADB_DETECTED

#if D_ENV_MARIADB_HAS_RESET_CONNECTION
/*
mariadb_connection::reset_connection
  Resets the session state (temporary tables, user variables,
prepared statements) without reauthenticating. Available in
MariaDB 10.2.4+.

Parameter(s):
  none.
Return:
  none.
*/
void
mariadb_connection::reset_connection()
{
    MYSQL* handle;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::reset_connection: "
            "no active connection");
    }

    if (mysql_reset_connection(handle) != 0)
    {
        throw connection_exception(
            internal::build_error_message(
                "mariadb_connection::reset_connection: "
                "mysql_reset_connection failed",
                handle));
    }

    return;
}
#endif  // D_ENV_MARIADB_HAS_RESET_CONNECTION

#if D_ENV_MARIADB_HAS_ASYNC_API
/*
mariadb_connection::connect_async_start
  Begins a non-blocking connection handshake. Returns an I/O
readiness status bitmask indicating which events the caller should
wait on before calling connect_async_cont.

Parameter(s):
  none.
Return:
  0 if the handshake completed immediately, otherwise a non-zero
  bitmask of MYSQL_WAIT_* flags.
*/
int
mariadb_connection::connect_async_start()
{
    MYSQL*        handle;
    MYSQL*        ret;
    const char*   unix_socket;
    unsigned long client_flag;
    int           status;

    // allocate a fresh handle if we don't already have one
    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        handle = mysql_init(nullptr);

        if (!handle)
        {
            throw connection_exception(
                "mariadb_connection::connect_async_start: "
                "mysql_init returned null (out of memory)");
        }

        this->m_native_handle = static_cast<void*>(handle);
    }

    unix_socket = nullptr;

    {
        auto it = this->m_config.custom_options.find("unix_socket");

        if (it != this->m_config.custom_options.end())
        {
            unix_socket = it->second.c_str();
        }
    }

    client_flag = CLIENT_MULTI_STATEMENTS;

    status = mysql_real_connect_start(
        &ret,
        handle,
        this->m_config.host.c_str(),
        this->m_config.username.c_str(),
        this->m_config.password.c_str(),
        this->m_config.database.empty()
            ? nullptr
            : this->m_config.database.c_str(),
        static_cast<unsigned int>(this->m_config.port),
        unix_socket,
        client_flag);

    // status == 0 means the handshake finished during _start; any
    // non-zero value is the MYSQL_WAIT_* bitmask to poll on
    return status;
}

/*
mariadb_connection::connect_async_cont
  Continues a non-blocking connection handshake. Should be called
after the underlying socket signals readiness for the events
reported by connect_async_start / the previous connect_async_cont.

Parameter(s):
  _status: a bitmask of MYSQL_WAIT_* flags describing which I/O
           events are currently ready on the socket.
Return:
  0 if the handshake completed, otherwise a non-zero bitmask of
  MYSQL_WAIT_* flags to wait on again.
*/
int
mariadb_connection::connect_async_cont(
    int _status
)
{
    MYSQL* handle;
    MYSQL* ret;
    int    next_status;

    handle = internal::as_mysql(this->m_native_handle);

    if (!handle)
    {
        throw connection_exception(
            "mariadb_connection::connect_async_cont: "
            "no async handshake in progress (native handle is null)");
    }

    next_status = mysql_real_connect_cont(&ret, handle, _status);

    return next_status;
}
#endif  // D_ENV_MARIADB_HAS_ASYNC_API

#endif  // D_ENV_MARIADB_DETECTED


NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
