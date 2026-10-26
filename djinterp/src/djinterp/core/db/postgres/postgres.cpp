/*******************************************************************************
* djinterp [core]                                                   postgres.cpp
*
* Definitions for postgres.hpp: pg_connection's hooks and members, and the
* PostgreSQL layer's failures, over the layer's face.
*   Every operation is one call of the face, whose result decides: 0 returns,
* anything else throws, through postgres_layer::raise, with the result -- the
* SQLSTATE packed, or a negative D_DB_* -- as the exception's error_code(),
* and the server's or libpq's own message where the connection holds one
* about the failure. Connecting connects a face of its own, with libpq's
* keywords for the configuration and the connection string after them, and
* sets the session's search path and statement timeout there, so that a
* connection is either open with all of it applied or not open at all.
*
*
* path:      /src/djinterp/core/db/postgres/postgres.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.25
*******************************************************************************/
#include "../../../../../inc/djinterp/core/db/postgres/postgres.hpp"  // corresponding header

#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <chrono>    // std::chrono::ceil, std::chrono::seconds
#include <cstddef>   // std::size_t
#include <deque>     // std::deque
#include <map>       // std::map
#include <memory>    // std::unique_ptr, std::make_unique
#include <new>       // std::bad_alloc
#include <optional>  // std::optional, std::nullopt
#include <string>    // std::string, std::to_string
#include <utility>   // std::pair, std::move
#include <vector>    // std::vector
// djinterp
#include "../../../../../inc/djinterp/c/db/postgres/postgres.h"  // the layer
#include "../../../../../inc/djinterp/core/db/database_statement.hpp"
                                       // prepared_statement, failure_kind
// re_std
#include "../../../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int64_t


NS_DJINTERP

//==============================================================================
// FILE-LOCAL DEFINITIONS
//==============================================================================

namespace
{

// connect_keywords
//   class: the keyword and value arrays d_postgres_connect_with takes, with
// the text they point to kept alive beside them, in a std::deque whose
// elements never move. Not copyable.
class connect_keywords
{
public:
    connect_keywords() = default;

    connect_keywords(const connect_keywords&)            = delete;
    connect_keywords& operator=(const connect_keywords&) = delete;

    /**
     * @brief Adds a keyword and its value, unless the value is empty, which
     *        leaves the keyword to libpq.
     *
     * @param[in] _keyword  the keyword.
     * @param[in] _value    its value.
     * @throws connection_exception with D_DB_INVALID_ARGUMENT for a keyword
     *         or a value that holds a NUL; std::bad_alloc when memory runs
     *         out.
     */
    void
    add(
        const std::string& _keyword,
        const std::string& _value
    )
    {
        // a value of none is libpq's to choose
        if (_value.empty())
        {
            return;
        }

        internal::refuse_nul_setting(_keyword,
                                     "PostgreSQL connect",
                                     "a keyword");
        internal::refuse_nul_setting(_value,
                                     "PostgreSQL connect",
                                     _keyword.c_str());

        m_texts.push_back(_keyword);
        m_keywords.push_back(m_texts.back().c_str());
        m_texts.push_back(_value);
        m_values.push_back(m_texts.back().c_str());

        return;
    }

    const char* const*
    keywords() const noexcept
    {
        return m_keywords.data();
    }

    const char* const*
    values() const noexcept
    {
        return m_values.data();
    }

    std::size_t
    size() const noexcept
    {
        return m_keywords.size();
    }

private:
    std::deque<std::string>  m_texts;
    std::vector<const char*> m_keywords;
    std::vector<const char*> m_values;
};

/**
 * @brief libpq's sslmode for a configured mode.
 *
 * @param[in] _mode  the mode.
 * @return its name, as libpq takes it.
 */
const char*
ssl_mode_name(
    pg_ssl_mode _mode
) noexcept
{
    // the six libpq knows
    switch (_mode)
    {
        case pg_ssl_mode::disable:
            return "disable";

        case pg_ssl_mode::allow:
            return "allow";

        case pg_ssl_mode::require:
            return "require";

        case pg_ssl_mode::verify_ca:
            return "verify-ca";

        case pg_ssl_mode::verify_full:
            return "verify-full";

        case pg_ssl_mode::prefer:
        default:
            return "prefer";
    }
}

/**
 * @brief The layer's transaction mode for an isolation level.
 *
 * @param[in] _level  the level.
 * @return the mode that begins a transaction at it; READ COMMITTED for
 *         read_uncommitted, as PostgreSQL runs it.
 */
enum d_postgres_transaction_mode
transaction_mode(
    isolation_level _level
) noexcept
{
    // the three PostgreSQL runs, and the session's own
    switch (_level)
    {
        case isolation_level::read_uncommitted:
        case isolation_level::read_committed:
            return D_POSTGRES_TRANSACTION_READ_COMMITTED;

        case isolation_level::repeatable_read:
            return D_POSTGRES_TRANSACTION_REPEATABLE_READ;

        case isolation_level::serializable:
            return D_POSTGRES_TRANSACTION_SERIALIZABLE;

        case isolation_level::default_level:
        default:
            return D_POSTGRES_TRANSACTION_DEFAULT;
    }
}

/**
 * @brief The layer's isolation level for core/db's.
 *
 * @param[in] _level  the level; not default_level.
 * @return the layer's.
 */
enum d_postgres_isolation
layer_isolation(
    isolation_level _level
) noexcept
{
    // the four the standard names
    switch (_level)
    {
        case isolation_level::read_uncommitted:
            return D_POSTGRES_ISOLATION_READ_UNCOMMITTED;

        case isolation_level::repeatable_read:
            return D_POSTGRES_ISOLATION_REPEATABLE_READ;

        case isolation_level::serializable:
            return D_POSTGRES_ISOLATION_SERIALIZABLE;

        case isolation_level::read_committed:
        case isolation_level::default_level:
        default:
            return D_POSTGRES_ISOLATION_READ_COMMITTED;
    }
}

/**
 * @brief core/db's isolation level for the layer's.
 *
 * @param[in] _level  the layer's.
 * @return core/db's.
 */
isolation_level
core_isolation(
    enum d_postgres_isolation _level
) noexcept
{
    // the four, one for one
    switch (_level)
    {
        case D_POSTGRES_ISOLATION_READ_UNCOMMITTED:
            return isolation_level::read_uncommitted;

        case D_POSTGRES_ISOLATION_REPEATABLE_READ:
            return isolation_level::repeatable_read;

        case D_POSTGRES_ISOLATION_SERIALIZABLE:
            return isolation_level::serializable;

        case D_POSTGRES_ISOLATION_READ_COMMITTED:
        default:
            return isolation_level::read_committed;
    }
}

/**
 * @brief A message of libpq's or the server's, cut at its first line, with
 *        the severity the server writes before it ("ERROR:  ") left out.
 *
 * @param[in]  _message  the message.
 * @param[out] _rest     receives the lines after the first, with no
 *                       newline at either end; "" for none.
 * @return the first line.
 */
std::string
first_line(
    const std::string& _message,
    std::string&       _rest
)
{
    std::string::size_type end = _message.size();

    // no newline at the end
    while ( (end > 0u) &&
            ( (_message[end - 1u] == '\n') ||
              (_message[end - 1u] == '\r') ) )
    {
        --end;
    }

    const std::string            text     = _message.substr(0u, end);
    const std::string::size_type break_at = text.find('\n');
    std::string                  line     = text.substr(0u, break_at);

    _rest = (break_at == std::string::npos) ? std::string()
                                            : text.substr(break_at + 1u);

    // the severity: one word, a colon and two spaces, as the server writes
    const std::string::size_type label = line.find(":  ");

    if ( (label != std::string::npos) &&
         (label > 0u)                 &&
         (label <= 16u)               &&
         (line.find(' ') > label) )
    {
        line.erase(0u, label + 3u);
    }

    return line;
}

/**
 * @brief The last line of a message that does not continue the one before
 *        it, as an indented line does.
 *
 * @param[in] _message  the message.
 * @return the line; "" for an empty message.
 */
std::string
last_line(
    const std::string& _message
)
{
    std::string::size_type end = _message.size();

    // no newline at the end
    while ( (end > 0u) &&
            ( (_message[end - 1u] == '\n') ||
              (_message[end - 1u] == '\r') ) )
    {
        --end;
    }

    // back to the start of a line that is not indented
    std::string::size_type start = _message.rfind('\n', (end > 0u) ? end - 1u
                                                                   : 0u);

    while (start != std::string::npos)
    {
        const char first = _message[start + 1u];

        if ( (first != ' ') &&
             (first != '\t') )
        {
            break;
        }

        start = (start > 0u) ? _message.rfind('\n', start - 1u)
                             : std::string::npos;
    }

    const std::string::size_type from = (start == std::string::npos)
                                            ? 0u
                                            : start + 1u;

    return _message.substr(from, end - from);
}

/**
 * @brief Splits a list of hosts with their ports, as a URI writes it and
 *        parse_connection_string keeps it, into libpq's host and port
 *        keywords: "a:5432,b,[::1]:5433" is the hosts "a,b,::1" and the
 *        ports "5432,,5433", an empty port libpq's default.
 *
 * @param[in]  _list   the list; split only when it holds a comma.
 * @param[out] _hosts  receives the hosts.
 * @param[out] _ports  receives the ports; "" when no host names one.
 * @return `true` when the list was split.
 */
bool
split_hosts(
    const std::string& _list,
    std::string&       _hosts,
    std::string&       _ports
)
{
    bool        any_port = false;
    std::size_t at       = 0u;

    // a single host is libpq's to read as it is
    if (_list.find(',') == std::string::npos)
    {
        return false;
    }

    _hosts.clear();
    _ports.clear();

    while (at <= _list.size())
    {
        const std::size_t comma = _list.find(',', at);
        const std::string entry = _list.substr(at,
                                               (comma == std::string::npos)
                                                   ? std::string::npos
                                                   : comma - at);
        std::string       host  = entry;
        std::string       port;

        // an IPv6 address in brackets, or a name or address and a port
        if ( (!entry.empty()) &&
             (entry[0] == '[') &&
             (entry.find(']') != std::string::npos) )
        {
            const std::size_t close = entry.find(']');

            host = entry.substr(1u, close - 1u);

            if (entry.compare(close + 1u, 1u, ":") == 0)
            {
                port = entry.substr(close + 2u);
            }
        }
        else if (entry.rfind(':') != std::string::npos)
        {
            const std::size_t colon = entry.rfind(':');

            host = entry.substr(0u, colon);
            port = entry.substr(colon + 1u);
        }

        any_port = ( (any_port) ||
                     (!port.empty()) );
        _hosts  += ((at == 0u) ? "" : ",") + host;
        _ports  += ((at == 0u) ? "" : ",") + port;

        // the last entry ends the list
        if (comma == std::string::npos)
        {
            break;
        }

        at = comma + 1u;
    }

    if (!any_port)
    {
        _ports.clear();
    }

    return true;
}

/**
 * @brief Describes a failed connect, in libpq's words where it gave some.
 *
 * @param[in] _reason  libpq's reason; may be null.
 * @param[in] _result  the connect's result.
 * @return the description.
 */
std::string
connect_failure(
    const face::text& _reason,
    int               _result
)
{
    std::string rest;
    std::string said = (_reason.is_null())
                           ? std::string(d_postgres_result_message(_result))
                           : first_line(_reason.str(), rest);
    std::string message = "PostgreSQL connect: " + said;

    // the SQLSTATE the layer gave, or db_common.h's number
    if (_result > 0)
    {
        message += " (SQLSTATE " + face::postgres::result_sqlstate(_result) +
                   ")";
    }
    else
    {
        message += " (" + std::to_string(_result) + ")";
    }

    // the lines after the first, a hint among them
    if (!rest.empty())
    {
        message += "\n" + rest;
    }

    return message;
}

/*
fill_keywords
  The fields go to libpq as keywords, base's first, then the PostgreSQL
ones, then base.custom_options and extra_params, each where it holds a
value; the connection string follows them, and overrides them, in
d_postgres_connect_with.
*/
void
fill_keywords(
    const pg_connect_config& _pg_config,
    connect_keywords&        _keywords
)
{
    const connection_config& config = _pg_config.base;

    std::string hosts;
    std::string ports;

    // where -- a list of hosts with their ports, as a URI writes one, split
    // into libpq's two lists -- and as whom
    if (split_hosts(config.host, hosts, ports))
    {
        _keywords.add("host", hosts);
        _keywords.add("port",
                      (!ports.empty())
                          ? ports
                          : ( (config.port != 0u) ? std::to_string(config.port)
                                                  : std::string() ));
    }
    else
    {
        _keywords.add("host", config.host);
        _keywords.add("port",
                      (config.port != 0u) ? std::to_string(config.port)
                                          : std::string());
    }

    _keywords.add("user", config.username);
    _keywords.add("password", config.password);
    _keywords.add("dbname", config.database);

    // TLS: PostgreSQL's mode where named, else the generic switches
    if (_pg_config.ssl_mode != pg_ssl_mode::prefer)
    {
        _keywords.add("sslmode", ssl_mode_name(_pg_config.ssl_mode));
    }
    else if (config.enable_ssl)
    {
        _keywords.add("sslmode",
                      (config.verify_ssl) ? "verify-full"
                                          : "require");
    }

    _keywords.add("sslcert", config.ssl_cert.value_or(std::string()));
    _keywords.add("sslkey", config.ssl_key.value_or(std::string()));
    _keywords.add("sslrootcert", config.ssl_ca.value_or(std::string()));

    // the connect timeout, in whole seconds, rounded up
    if (_pg_config.connect_timeout_seconds > 0)
    {
        _keywords.add("connect_timeout",
                      std::to_string(_pg_config.connect_timeout_seconds));
    }
    else if (config.connect_timeout.count() > 0)
    {
        _keywords.add("connect_timeout",
                      std::to_string(
                          std::chrono::ceil<std::chrono::seconds>(
                              config.connect_timeout).count()));
    }

    _keywords.add("application_name", _pg_config.application_name);
    _keywords.add("client_encoding",
                 (!_pg_config.client_encoding.empty())
                      ? _pg_config.client_encoding
                      : config.charset.value_or(std::string()));
    _keywords.add("options", _pg_config.options);

    // any other keyword, by its name; a URI's scheme is no keyword
    for (const std::pair<const std::string, std::string>& option :
         config.custom_options)
    {
        if (option.first != "scheme")
        {
            _keywords.add(option.first, option.second);
        }
    }

    for (const std::pair<const std::string, std::string>& option :
         _pg_config.extra_params)
    {
        _keywords.add(option.first, option.second);
    }

    return;
}

}  // namespace

//==============================================================================
// 7.  POSTGRESQL CONNECTION
//==============================================================================

/*
postgres_layer::describe
  The connection's message is about the latest failure libpq or the server
reported; a result the layer gave on its own (d_postgres_result_is_own)
leaves an older message in place, so for those the result's own description
stands in, and for the same codes from the server, its message. The
server's message comes as several lines -- the message, then any DETAIL,
HINT or the statement's position -- which the description keeps, the
SQLSTATE after the first. libpq adds its refusal of a call, 08000 ("no COPY
in progress"), to the end of what the message held, which may be an older
failure's, so for 08000 its last line alone is the failure.
*/
std::string
postgres_layer::describe(
    handle_type _db,
    const char* _operation,
    int         _result
)
{
    const char* const said = ( (_db) &&
                               (!d_postgres_result_is_own(_db, _result)) )
                                 ? d_postgres_error_message(_db)
                                 : nullptr;
    std::string       rest;
    std::string       line;

    // libpq's refusal, last; or the message from its start
    if ( (said) &&
         (said[0] != '\0') )
    {
        line = (_result == D_POSTGRES_SQLSTATE_CONNECTION_EXCEPTION)
                   ? last_line(said)
                   : first_line(said, rest);
    }

    // the result's words where the connection's are not about it
    if (line.empty())
    {
        line = d_postgres_result_message(_result);
        rest.clear();
    }

    std::string message = std::string("PostgreSQL ") + _operation + ": " +
                          line;

    // the SQLSTATE, spelled; or db_common.h's number
    if (_result > 0)
    {
        message += " (SQLSTATE " + face::postgres::result_sqlstate(_result) +
                   ")";
    }
    else
    {
        message += " (" + std::to_string(_result) + ")";
    }

    if (!rest.empty())
    {
        message += "\n" + rest;
    }

    return message;
}

void
postgres_layer::raise_as(
    failure_kind       _kind,
    const std::string& _message,
    int                _result
)
{
    // a lost connection is the connection's failure, whatever was running
    if (connection_lost(_result))
    {
        throw connection_exception(_message, _result);
    }

    switch (_kind)
    {
        case failure_kind::connection:
            throw connection_exception(_message, _result);

        case failure_kind::transaction:
            throw transaction_exception(_message, _result);

        case failure_kind::query:
        default:
            throw query_exception(_message, _result);
    }
}

void
postgres_layer::raise(
    handle_type  _db,
    failure_kind _kind,
    const char*  _operation,
    int          _result
)
{
    raise_as(_kind, describe(_db, _operation, _result), _result);
}

/*
postgres_layer::cancel
  libpq's reason is the only word on a request that failed.
*/
void
postgres_layer::cancel(
    handle_type _db
)
{
    char*     reason = nullptr;
    const int result = d_postgres_cancel(_db, &reason);

    // made, or not, and why
    if (result != 0)
    {
        const std::string message =
            std::string("PostgreSQL cancel: ") +
            ((reason) ? reason : d_postgres_result_message(result)) + " (" +
            std::to_string(result) + ")";

        d_db_free(reason);

        throw query_exception(message, result);
    }

    return;
}

template class prepared_statement<postgres_layer>;
template class streaming_result_set<postgres_layer>;

pg_connection::pg_connection()
    : base_type(pg_connect_config().base),
      m_pg_config(),
      m_db(),
      m_opening(),
      m_opening_stage(0)
{}

pg_connection::pg_connection(
    const connection_config& _config
)
    : base_type(_config),
      m_pg_config(_config),
      m_db(),
      m_opening(),
      m_opening_stage(0)
{}

pg_connection::pg_connection(
    const pg_connect_config& _config
)
    : base_type(_config.base),
      m_pg_config(_config),
      m_db(),
      m_opening(),
      m_opening_stage(0)
{}

pg_connection::pg_connection(
    const std::string& _conn_string
)
    : base_type(pg_connect_config(_conn_string).base),
      m_pg_config(_conn_string),
      m_db(),
      m_opening(),
      m_opening_stage(0)
{
    // the string alone says where: no default port stands in for its own
    this->m_config.port = 0;
}

/*
pg_connection::~pg_connection
  The statements go first, deallocated on the connection they belong to
while it is open; the face then closes the connection as it goes.
*/
pg_connection::~pg_connection()
{
    this->close_session_noexcept();
}

bool
pg_connection::send_query(
    const std::string& _query
)
{
    internal::refuse_nul(_query, "send_query");

    this->run_guarded(
        [this, &_query]() -> void
        {
            m_check(m_db.send_query(_query), failure_kind::query, "send_query");

            return;
        });

    return true;
}

std::unique_ptr<buffered_result_set>
pg_connection::get_result()
{
    return this->run_guarded(
        [this]() -> std::unique_ptr<buffered_result_set>
        {
            face::rows      found;
            re_std::int64_t changes = 0;
            const int       result  = m_db.next_result(found, changes);

            // every result read
            if (result == D_DB_NOT_FOUND)
            {
                return std::unique_ptr<buffered_result_set>();
            }

            m_check(result, failure_kind::query, "get_result");
            this->record_changes(changes);

            return std::make_unique<buffered_result_set>(std::move(found));
        });
}

bool
pg_connection::is_busy() const noexcept
{
    return m_db.is_busy();
}

void
pg_connection::set_chunked_rows_mode(
    int _chunk_size
)
{
    this->run_guarded(
        [this, _chunk_size]() -> void
        {
            m_check(m_db.set_chunked_rows(_chunk_size),
                    failure_kind::query,
                    "set_chunked_rows_mode");

            return;
        });

    return;
}

void
pg_connection::enter_pipeline()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.enter_pipeline(),
                    failure_kind::query,
                    "enter_pipeline");

            return;
        });

    return;
}

void
pg_connection::exit_pipeline()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.exit_pipeline(), failure_kind::query, "exit_pipeline");

            return;
        });

    return;
}

void
pg_connection::pipeline_sync()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.pipeline_sync(), failure_kind::query, "pipeline_sync");

            return;
        });

    return;
}

void
pg_connection::pipeline_flush()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.pipeline_flush(),
                    failure_kind::query,
                    "pipeline_flush");

            return;
        });

    return;
}

/*
pg_connection::get_pipeline_status
  The layer's three states, one for one; a connection not open has none.
*/
pg_pipeline_status
pg_connection::get_pipeline_status() const noexcept
{
    // only an open connection can be in the mode
    if (!this->is_connected())
    {
        return pg_pipeline_status::off;
    }

    switch (m_db.pipeline_status())
    {
        case D_POSTGRES_PIPELINE_ON:
            return pg_pipeline_status::on;

        case D_POSTGRES_PIPELINE_ABORTED:
            return pg_pipeline_status::aborted;

        case D_POSTGRES_PIPELINE_OFF:
        default:
            return pg_pipeline_status::off;
    }
}

void
pg_connection::close_prepared(
    const std::string& _stmt_name
)
{
    internal::refuse_nul(_stmt_name, "close_prepared", "the name");

    this->run_guarded(
        [this, &_stmt_name]() -> void
        {
            m_check(m_db.close_prepared(_stmt_name),
                    failure_kind::query,
                    "close_prepared");

            return;
        });

    return;
}

unsigned int
pg_connection::lo_import(
    const std::string& _path
)
{
    internal::refuse_nul(_path, "lo_import", "the path");

    return this->run_guarded(
        [this, &_path]() -> unsigned int
        {
            unsigned int oid = 0u;

            m_check(m_db.lo_import(_path, oid),
                    failure_kind::query,
                    "lo_import");

            return oid;
        });
}

void
pg_connection::lo_export(
    unsigned int       _oid,
    const std::string& _path
)
{
    internal::refuse_nul(_path, "lo_export", "the path");

    this->run_guarded(
        [this, _oid, &_path]() -> void
        {
            m_check(m_db.lo_export(_oid, _path),
                    failure_kind::query,
                    "lo_export");

            return;
        });

    return;
}

bool
pg_connection::consume_input()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.consume_input(),
                    failure_kind::connection,
                    "consume_input");

            return;
        });

    return true;
}

std::unique_ptr<buffered_result_set>
pg_connection::exec_params(
    const std::string&              _query,
    const std::vector<std::string>& _params
)
{
    std::vector<const char*> values;

    internal::refuse_nul(_query, "exec_params");
    values.reserve(_params.size());

    // each value as the layer's C string
    for (const std::string& param : _params)
    {
        internal::refuse_nul(param, "exec_params", "a value");
        values.push_back(param.c_str());
    }

    return m_exec_params(_query, values);
}

std::unique_ptr<buffered_result_set>
pg_connection::exec_params(
    const std::string&                              _query,
    const std::vector<std::optional<std::string>>& _params
)
{
    std::vector<const char*> values;

    internal::refuse_nul(_query, "exec_params");
    values.reserve(_params.size());

    // each value as the layer's C string, NULL for none
    for (const std::optional<std::string>& param : _params)
    {
        if (param)
        {
            internal::refuse_nul(*param, "exec_params", "a value");
            values.push_back(param->c_str());
        }
        else
        {
            values.push_back(nullptr);
        }
    }

    return m_exec_params(_query, values);
}

void
pg_connection::copy_in_start(
    const std::string& _copy_sql
)
{
    internal::refuse_nul(_copy_sql, "copy_in_start");

    this->run_guarded(
        [this, &_copy_sql]() -> void
        {
            m_check(m_db.copy_in_start(_copy_sql),
                    failure_kind::query,
                    "copy_in_start");

            return;
        });

    return;
}

void
pg_connection::copy_data(
    const char*  _data,
    std::size_t  _length
)
{
    this->run_guarded(
        [this, _data, _length]() -> void
        {
            m_check(m_db.copy_in_data(_data, _length),
                    failure_kind::query,
                    "copy_data");

            return;
        });

    return;
}

re_std::int64_t
pg_connection::copy_end()
{
    return this->run_guarded(
        [this]() -> re_std::int64_t
        {
            re_std::int64_t changes = 0;

            m_check(m_db.copy_in_end(face::string_arg(), changes),
                    failure_kind::query,
                    "copy_end");
            this->record_changes(changes);

            return changes;
        });
}

/*
pg_connection::copy_abort
  The server answers an abandoned COPY with 57014, query_canceled, which is
what was asked for here; anything else is a failure. A reason must reach
libpq, which takes none as the COPY ended well, so an empty one is given
words.
*/
void
pg_connection::copy_abort(
    const std::string& _reason
)
{
    internal::refuse_nul(_reason, "copy_abort", "the reason");

    const std::string reason = (_reason.empty()) ? std::string("abandoned")
                                                 : _reason;

    this->run_guarded(
        [this, &reason]() -> void
        {
            re_std::int64_t changes = 0;
            const int       result  = m_db.copy_in_end(reason, changes);

            // abandoned, as asked
            if (result == D_POSTGRES_SQLSTATE('5', '7', '0', '1', '4'))
            {
                this->record_changes(0);

                return;
            }

            m_check(result, failure_kind::query, "copy_abort");
            this->record_changes(0);

            return;
        });

    return;
}

void
pg_connection::listen(
    const std::string& _channel
)
{
    internal::refuse_nul(_channel, "listen", "the channel");

    this->run_guarded(
        [this, &_channel]() -> void
        {
            m_check(m_db.listen(_channel), failure_kind::query, "listen");

            return;
        });

    return;
}

void
pg_connection::unlisten(
    const std::string& _channel
)
{
    internal::refuse_nul(_channel, "unlisten", "the channel");

    // an empty channel is no channel: unlisten_all ends every subscription
    if (_channel.empty())
    {
        throw query_exception("unlisten: the channel is empty",
                              D_DB_INVALID_ARGUMENT);
    }

    this->run_guarded(
        [this, &_channel]() -> void
        {
            m_check(m_db.unlisten(_channel), failure_kind::query, "unlisten");

            return;
        });

    return;
}

void
pg_connection::unlisten_all()
{
    this->run_guarded(
        [this]() -> void
        {
            m_check(m_db.unlisten(face::string_arg()),
                    failure_kind::query,
                    "unlisten_all");

            return;
        });

    return;
}

void
pg_connection::notify(
    const std::string& _channel,
    const std::string& _payload
)
{
    internal::refuse_nul(_channel, "notify", "the channel");
    internal::refuse_nul(_payload, "notify", "the payload");

    this->run_guarded(
        [this, &_channel, &_payload]() -> void
        {
            m_check(m_db.notify(_channel, _payload),
                    failure_kind::query,
                    "notify");

            return;
        });

    return;
}

std::optional<pg_notification>
pg_connection::get_notification()
{
    return this->run_guarded(
        [this]() -> std::optional<pg_notification>
        {
            pg_notification arrived{ std::string(), std::string(), 0 };
            const int       result =
                m_db.next_notification(arrived.channel,
                                       arrived.payload,
                                       arrived.backend_pid);

            // nothing has arrived
            if (result == D_DB_NOT_FOUND)
            {
                return std::nullopt;
            }

            m_check(result, failure_kind::query, "get_notification");

            return arrived;
        });
}

std::string
pg_connection::escape_literal(
    const std::string& _input
) const
{
    internal::refuse_nul(_input, "escape_literal");

    return this->ask_guarded(
        [this, &_input]() -> std::string
        {
            face::text quoted;

            m_db.quote_literal(_input, quoted);

            // libpq refused the text, for the encoding, or for memory
            if (quoted.is_null())
            {
                const char* const said =
                    d_postgres_error_message(m_db.handle());
                std::string       rest;

                throw query_exception(
                    std::string("PostgreSQL escape_literal: ") +
                        ( ( (said) &&
                            (said[0] != '\0') )
                              ? first_line(said, rest)
                              : std::string("the text cannot be quoted for "
                                            "the connection's encoding")) +
                        " (" + std::to_string(D_DB_INVALID_ARGUMENT) + ")",
                    D_DB_INVALID_ARGUMENT);
            }

            return quoted.str();
        });
}

std::string
pg_connection::escape_identifier(
    const std::string& _input
) const
{
    face::text quoted;

    internal::refuse_nul(_input, "escape_identifier", "the name");
    face::postgres::quote_identifier(_input, quoted);

    // only memory can fail it
    if (quoted.is_null())
    {
        throw std::bad_alloc();
    }

    return quoted.str();
}

int
pg_connection::get_backend_pid() const noexcept
{
    return m_db.backend_pid();
}

/*
pg_connection::get_transaction_status
  libpq's states, by the letters psql's prompt shows for them, and 'U' for
libpq's PQTRANS_UNKNOWN: no connection, or a lost one.
*/
char
pg_connection::get_transaction_status() const noexcept
{
    // libpq's five
    switch (m_db.transaction_status())
    {
        case D_POSTGRES_STATE_IDLE:
            return 'I';

        case D_POSTGRES_STATE_ACTIVE:
            return 'A';

        case D_POSTGRES_STATE_IN_TRANSACTION:
            return 'T';

        case D_POSTGRES_STATE_IN_FAILED_TRANSACTION:
            return 'E';

        case D_POSTGRES_STATE_UNKNOWN:
        default:
            return 'U';
    }
}

std::string
pg_connection::get_parameter_status(
    const std::string& _name
) const
{
    // a name with a NUL is no parameter's
    if (_name.find('\0') != std::string::npos)
    {
        return std::string();
    }

    const char* const reported = m_db.parameter_status(_name);

    return (reported) ? std::string(reported)
                      : std::string();
}

bool
pg_connection::has_table(
    const std::string& _table_name
) const
{
    internal::refuse_nul(_table_name, "has_table", "the name");

    return this->ask_guarded(
        [this, &_table_name]() -> bool
        {
            bool exists = false;

            m_check(m_db.has_table_s(face::string_arg(), _table_name, exists),
                    failure_kind::query,
                    "has_table");

            return exists;
        });
}

std::vector<std::string>
pg_connection::list_tables() const
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

/*
pg_connection::get_pg_config
  base is the connection's generic configuration, which connect(config)
replaces; the copy kept here is only what was last set or connected with.
*/
pg_connect_config
pg_connection::get_pg_config() const
{
    pg_connect_config config = m_pg_config;

    config.base = this->m_config;

    return config;
}

void
pg_connection::set_pg_config(
    const pg_connect_config& _config
)
{
    m_pg_config    = _config;
    this->m_config = _config.base;

    return;
}

face::postgres&
pg_connection::get_face() noexcept
{
    return m_db;
}

const face::postgres&
pg_connection::get_face() const noexcept
{
    return m_db;
}

/*
pg_connection::connect_impl
  The keywords (fill_keywords) and the connection string open a connection
into a face of its own; the search path and statement timeout are set there
(m_finish_connect), and only then does the object take it, so that a
failure at any step leaves the old state and the new connection closed.
*/
void
pg_connection::connect_impl()
{
    connect_keywords keywords;

    m_pg_config.base = this->m_config;
    fill_keywords(m_pg_config, keywords);
    internal::refuse_nul_setting(m_pg_config.connection_string,
                                 "PostgreSQL connect",
                                 "the connection string");

    face::postgres opened;
    face::text     reason;
    const int      result = opened.connect_with(m_pg_config.connection_string,
                                                keywords.keywords(),
                                                keywords.values(),
                                                keywords.size(),
                                                reason);

    // a failure, in libpq's words where it gave some
    if (result != 0)
    {
        throw connection_exception(connect_failure(reason, result), result);
    }

    m_finish_connect(opened);

    return;
}

/*
pg_connection::connect_start_impl
  As connect_impl begins, into the face kept for a connection being made;
libpq copies the keywords before the start returns. A handshake done at
once goes on to the setup (m_setup_step).
*/
int
pg_connection::connect_start_impl(
    d_db_pending& _pending
)
{
    connect_keywords keywords;

    m_pg_config.base = this->m_config;
    fill_keywords(m_pg_config, keywords);
    internal::refuse_nul_setting(m_pg_config.connection_string,
                                 "PostgreSQL connect",
                                 "the connection string");

    face::text reason;
    const int  result = m_opening.connect_with_start(
                            m_pg_config.connection_string,
                            keywords.keywords(),
                            keywords.values(),
                            keywords.size(),
                            _pending,
                            reason);

    m_opening_stage = 0;

    // under way, or failed in libpq's words
    if ( (result != 0) &&
         (result != D_DB_WAIT) )
    {
        throw connection_exception(connect_failure(reason, result), result);
    }

    return (result == D_DB_WAIT) ? result
                                 : m_setup_step(0u, _pending);
}

/*
pg_connection::connect_continue_impl
  The handshake, then the setup (m_setup_step).
*/
int
pg_connection::connect_continue_impl(
    unsigned      _events,
    d_db_pending& _pending
)
{
    // past the handshake: the setup
    if (m_opening_stage != 0)
    {
        return m_setup_step(_events, _pending);
    }

    face::text reason;
    const int  result = m_opening.connect_continue(_events,
                                                   _pending,
                                                   reason);

    // under way, made, or failed in libpq's words
    if ( (result != 0) &&
         (result != D_DB_WAIT) )
    {
        throw connection_exception(connect_failure(reason, result), result);
    }

    return (result == D_DB_WAIT) ? result
                                 : m_setup_step(0u, _pending);
}

/*
pg_connection::connect_finish_impl
  As connect_impl ends, the setup already made without waiting
(m_setup_step): the object takes the connection.
*/
void
pg_connection::connect_finish_impl()
{
    m_db.swap(m_opening);
    (void)m_opening.close();
    m_opening_stage = 0;

    return;
}

void
pg_connection::connect_abandon_impl() noexcept
{
    (void)m_opening.close();
    m_opening_stage = 0;

    return;
}

/*
pg_connection::m_session_setup
  set_config for each, as set_search_path and set_statement_timeout run it,
in one statement: the search path as m_finish_connect chooses it, quoted as
a literal on the connection made; the timeout in milliseconds.
*/
std::string
pg_connection::m_session_setup(
    face::postgres& _opened
) const
{
    const connection_config& config = this->m_config;
    std::string              setup;

    const std::string search_path =
        (!m_pg_config.search_path.empty())
            ? m_pg_config.search_path
            : ( (!config.schema)
                    ? std::string()
                    : ( (config.schema->find(',') != std::string::npos)
                            ? *config.schema
                            : face::postgres::quote_identifier(
                                  *config.schema) ) );

    // the session's search path, where named
    if (!search_path.empty())
    {
        internal::refuse_nul_setting(search_path,
                                     "PostgreSQL connect",
                                     "the search path");

        face::text quoted;

        _opened.quote_literal(search_path, quoted);

        // libpq quotes on the connection, which may fail
        if (quoted.is_null())
        {
            postgres_layer::raise(_opened.handle(),
                                  failure_kind::connection,
                                  "set_search_path",
                                  D_POSTGRES_SQLSTATE_OUT_OF_MEMORY);
        }

        setup = "pg_catalog.set_config('search_path', " +
                std::string(quoted.c_str()) + ", false)";
    }

    // and its statement timeout
    if (m_pg_config.statement_timeout_ms > 0)
    {
        setup += (setup.empty()) ? "" : ", ";
        setup += "pg_catalog.set_config('statement_timeout', '" +
                 std::to_string(static_cast<unsigned>(
                     m_pg_config.statement_timeout_ms)) +
                 "', false)";
    }

    return (setup.empty()) ? setup
                           : "SELECT " + setup;
}

/*
pg_connection::m_setup_step
  1, the statement sent; 2, its results read to their end, a read under way
continued and each next one started. A failure closes the connection being
made and throws, as m_finish_connect's would.
*/
int
pg_connection::m_setup_step(
    unsigned      _events,
    d_db_pending& _pending
)
{
    try
    {
        int  result    = 0;
        bool under_way = (m_opening_stage == 2);

        // the handshake just done: the statement, if there is one, sent
        if (m_opening_stage == 0)
        {
            const std::string setup = m_session_setup(m_opening);

            if (setup.empty())
            {
                m_opening_stage = 3;

                return 0;
            }

            m_opening_stage = 1;
            result          = m_opening.send_query_start(setup, _pending);
        }
        else if (m_opening_stage == 1)
        {
            result = m_opening.send_query_continue(_events, _pending);
        }

        // sending
        if (m_opening_stage == 1)
        {
            if (result == D_DB_WAIT)
            {
                return result;
            }

            if (result != 0)
            {
                postgres_layer::raise(m_opening.handle(),
                                      failure_kind::connection,
                                      "the session's settings",
                                      result);
            }

            m_opening_stage = 2;
        }

        // the results, each read, to their end
        for (;; under_way = false)
        {
            face::rows      rows;
            re_std::int64_t changes = 0;

            result = (under_way) ? m_opening.next_result_continue(_events,
                                                                  _pending,
                                                                  rows,
                                                                  changes)
                                 : m_opening.next_result_start(_pending,
                                                               rows,
                                                               changes);

            if ( (result == D_DB_WAIT) ||
                 (result == D_DB_NOT_FOUND) )
            {
                break;
            }

            if (result != 0)
            {
                postgres_layer::raise(m_opening.handle(),
                                      failure_kind::connection,
                                      "the session's settings",
                                      result);
            }
        }

        if (result == D_DB_WAIT)
        {
            return result;
        }

        m_opening_stage = 3;

        return 0;
    }
    catch (...)
    {
        (void)m_opening.close();
        m_opening_stage = 0;

        throw;
    }
}

/*
pg_connection::m_finish_connect
  The search path: PostgreSQL's own; else base.schema, one schema's name,
quoted -- or, holding a comma, a list as SQL writes it, as
parse_connection_string reads search_path into it. Then the statement
timeout; then the object takes the connection.
*/
void
pg_connection::m_finish_connect(
    face::postgres& _opened
)
{
    const connection_config& config = this->m_config;

    // the search path: PostgreSQL's own; else base.schema, one schema's
    // name, quoted -- or, holding a comma, a list as SQL writes it, as
    // parse_connection_string reads search_path into it
    const std::string search_path =
        (!m_pg_config.search_path.empty())
            ? m_pg_config.search_path
            : ( (!config.schema)
                    ? std::string()
                    : ( (config.schema->find(',') != std::string::npos)
                            ? *config.schema
                            : face::postgres::quote_identifier(
                                  *config.schema) ) );

    // the session's search path, where named
    if (!search_path.empty())
    {
        internal::refuse_nul_setting(search_path,
                                     "PostgreSQL connect",
                                     "the search path");

        const int set = _opened.set_search_path(search_path);

        if (set != 0)
        {
            postgres_layer::raise(_opened.handle(),
                                  failure_kind::connection,
                                  "set_search_path",
                                  set);
        }
    }

    // and its statement timeout
    if (m_pg_config.statement_timeout_ms > 0)
    {
        const int set = _opened.set_statement_timeout(
                            static_cast<unsigned>(
                                m_pg_config.statement_timeout_ms));

        if (set != 0)
        {
            postgres_layer::raise(_opened.handle(),
                                  failure_kind::connection,
                                  "set_statement_timeout",
                                  set);
        }
    }

    m_db.swap(_opened);

    return;
}

void
pg_connection::disconnect_impl() noexcept
{
    (void)m_db.close();

    return;
}

bool
pg_connection::is_connected_impl() const noexcept
{
    return m_db.is_open();
}

/*
pg_connection::ping_impl
  A ping is a round trip that changes nothing a caller can see, and the
face's is not const, so the face is reached through const_cast. With SQL
sent, or a COPY, still running, a round trip would end it; reading what the
server has sent says as much.
*/
bool
pg_connection::ping_impl() const noexcept
{
    face::postgres& db = const_cast<face::postgres&>(m_db);

    // something running, which a round trip would end
    if (db.transaction_status() == D_POSTGRES_STATE_ACTIVE)
    {
        return (db.consume_input() == 0);
    }

    return (db.ping() == 0);
}

face::rows
pg_connection::execute_query_impl(
    const std::string& _query
)
{
    face::rows found;

    m_check(m_db.query_rows(found, _query),
            failure_kind::query,
            "execute_query");

    return found;
}

re_std::int64_t
pg_connection::execute_update_impl(
    const std::string& _query
)
{
    re_std::int64_t changes = 0;

    m_check(m_db.exec_changes(changes, _query),
            failure_kind::query,
            "execute_update");

    return changes;
}

face::postgres_statement
pg_connection::prepare_impl(
    const std::string& _query
)
{
    face::postgres_statement prepared;

    m_check(m_db.prepare(prepared, _query), failure_kind::query, "prepare");

    return prepared;
}

std::string
pg_connection::get_server_version_impl() const
{
    const char* const version = m_db.server_version();

    return (version) ? version
                     : "";
}

/*
pg_connection::get_last_insert_id_impl
  PostgreSQL keeps no id beside the connection; lastval() is the value the
session's latest nextval() gave, a serial column's or an identity's among
them, and the layer reads it without risking the transaction. None yet
answers 0, as the other vendors do.
*/
re_std::int64_t
pg_connection::get_last_insert_id_impl() const
{
    re_std::int64_t last   = 0;
    const int       result =
        const_cast<face::postgres&>(m_db).last_value(last);

    // no sequence used yet in the session
    if (result == D_DB_NOT_FOUND)
    {
        return 0;
    }

    m_check(result, failure_kind::query, "get_last_insert_id");

    return last;
}

void
pg_connection::begin_transaction_impl()
{
    m_check(m_db.begin_transaction(D_POSTGRES_TRANSACTION_DEFAULT),
            failure_kind::transaction,
            "begin_transaction");

    return;
}

void
pg_connection::begin_transaction_impl(
    isolation_level _isolation
)
{
    m_check(m_db.begin_transaction(transaction_mode(_isolation)),
            failure_kind::transaction,
            "begin_transaction");

    return;
}

void
pg_connection::commit_impl()
{
    m_check(m_db.commit(), failure_kind::transaction, "commit");

    return;
}

void
pg_connection::rollback_impl()
{
    m_check(m_db.rollback(), failure_kind::transaction, "rollback");

    return;
}

bool
pg_connection::in_transaction_impl() const noexcept
{
    return m_db.in_transaction();
}

isolation_level
pg_connection::get_isolation_level_impl() const
{
    enum d_postgres_isolation level = D_POSTGRES_ISOLATION_READ_COMMITTED;

    m_check(m_db.get_isolation_level(level),
            failure_kind::transaction,
            "get_isolation_level");

    return core_isolation(level);
}

/*
pg_connection::set_isolation_level_impl
  The level the session's transactions begin at; default_level returns it
to the server's own default, whatever that is configured as.
*/
void
pg_connection::set_isolation_level_impl(
    isolation_level _level
)
{
    // the server's default, which no level names
    if (_level == isolation_level::default_level)
    {
        m_check(m_db.exec("RESET default_transaction_isolation"),
                failure_kind::transaction,
                "set_isolation_level");

        return;
    }

    m_check(m_db.set_isolation_level(layer_isolation(_level)),
            failure_kind::transaction,
            "set_isolation_level");

    return;
}

void
pg_connection::create_savepoint_impl(
    const std::string& _name
)
{
    m_check(m_db.create_savepoint(_name),
            failure_kind::transaction,
            "create_savepoint");

    return;
}

void
pg_connection::release_savepoint_impl(
    const std::string& _name
)
{
    m_check(m_db.release_savepoint(_name),
            failure_kind::transaction,
            "release_savepoint");

    return;
}

void
pg_connection::rollback_to_savepoint_impl(
    const std::string& _name
)
{
    m_check(m_db.rollback_to_savepoint(_name),
            failure_kind::transaction,
            "rollback_to_savepoint");

    return;
}

struct ::pg_conn*
pg_connection::native_handle_impl() const noexcept
{
    return m_db.handle();
}

void
pg_connection::m_check(
    int          _result,
    failure_kind _kind,
    const char*  _operation
) const
{
    // only a failure throws
    if (_result != 0)
    {
        postgres_layer::raise(m_db.handle(), _kind, _operation, _result);
    }

    return;
}

bool
pg_connection::m_supports(
    d_postgres_feature _feature
) const noexcept
{
    return ( (this->is_connected()) &&
             (m_db.supports(_feature)) );
}

bool
pg_connection::m_server_at_least(
    unsigned _major,
    unsigned _minor
) const noexcept
{
    return ( (this->is_connected()) &&
             (m_db.server_at_least(_major, _minor)) );
}

std::unique_ptr<buffered_result_set>
pg_connection::m_exec_params(
    const std::string&              _query,
    const std::vector<const char*>& _values
)
{
    return this->run_guarded(
        [this, &_query, &_values]() -> std::unique_ptr<buffered_result_set>
        {
            face::rows found;

            m_check(m_db.query_rows_params(found,
                                           _query,
                                           _values.data(),
                                           _values.size()),
                    failure_kind::query,
                    "exec_params");

            return std::make_unique<buffered_result_set>(std::move(found));
        });
}

NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER
